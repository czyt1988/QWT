// 3D surface VBO/VAO pipeline benchmark for QWT (qwt::plot3d).
// Measures: steady-state frame time vs mesh size / plot style / lighting /
// coordinate system / MSAA / glFinish, the lazy data-upload path
// (loadFromData CPU prep vs first-draw VBO build+upload vs steady draw,
// invalidateColors / setResolution rebuild cost), and gl2ps vector export
// (time, file size, captured primitive count).
//
// Build (Windows, MSVC 2019 x64, Qt 5.15.16, QWT release build in <repo>/build-release):
//   call vcvars64.bat
//   cl /nologo /O2 /EHsc /std:c++17 /MD ^
//     /I<repo>\src /I<repo>\src\core /I<repo>\src\plot3d ^
//     /I<qt>\include /I<qt>\include\QtCore /I<qt>\include\QtGui ^
//     /I<qt>\include\QtWidgets /I<qt>\include\QtOpenGL ^
//     surface3d_bench.cpp /Fe:surface3d_bench.exe ^
//     /link /LIBPATH:<repo>\build-release\lib /LIBPATH:<qt>\lib ^
//     qwtplot3d.lib qwtcore.lib Qt5Widgets.lib Qt5Gui.lib Qt5Core.lib ^
//     Qt5OpenGL.lib opengl32.lib glu32.lib
//
// Run (needs <qt>\bin and <repo>\build-release\bin on PATH; real desktop
// session required - GL rendering in offscreen mode is meaningless):
//   surface3d_bench.exe frames <cols> <style> <light> [options]
//   surface3d_bench.exe upload <cols> [options]
//   surface3d_bench.exe export <cols> <fmt> [options]
//   surface3d_bench.exe env
//
//   style    filled | filledmesh | wireframe | hiddenline | points | noplot
//   light    0 | 1                 (Qwt3DPlot::enableLighting)
//   fmt      PDF | SVG | EPS | PS | PGF | PNG   (PNG = raster savePixmap)
//
//   options  --coord box|nocoord   coordinate system style (default box)
//            --finish on|off      glFinish() at end of paintGL (frames mode)
//            --msaa 0|1|4         default surface format samples (default 4)
//            --res N              Qwt3DSurface::setResolution (default 1)
//            --sort simple|bsp|no gl2ps sort mode (export mode)
//            --rounds 3           frame-time rounds (default 3)
//            --fpp 100            frames per round (default 100)
//            --reps 3             repetitions for upload stages (default 3)
//            --w 900 --h 600      widget size
//
// Frame-time protocol: 5 warmup frames, then <rounds> rounds x <fpp> timed
// frames. Each frame rotates the view by 0.7 deg (simulated interaction) and
// pumps update()+processEvents() (repaint() is a no-op on QOpenGLWidget);
// per-frame times are pooled (min/median/p99) and per-round medians are
// recorded. If max(round_med)/min(round_med) > 1.5 the whole measurement is
// repeated (up to 2 retries); retries land in the CSV.
//
// Output: exactly one CSV data line on stdout (schema in the results file
// header), diagnostics on stderr prefixed with '#'.

#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QSurfaceFormat>
#include <QTextStream>

#include <qwt_version_info.h>
#include <qwt3d_plot.h>
#include <qwt3d_surface.h>
#include <qwt3d_types.h>
#include <qwt3d_io_gl2ps.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace
{

// paintGL-counting subclass; optional glFinish to measure GPU completion
class BenchPlot : public Qwt3DPlot
{
public:
    explicit BenchPlot(QWidget* parent = nullptr) : Qwt3DPlot(parent) {}
    long long paintCount = 0;
    bool finishAfterPaint = false;

protected:
    void paintGL() override
    {
        Qwt3DPlot::paintGL();
        ++paintCount;
        if (finishAfterPaint) {
            QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
            if (f)
                f->glFinish();
        }
    }
};

PLOTSTYLE parseStyle(const QString& s)
{
    if (s == "filled")
        return FILLED;
    if (s == "wireframe")
        return WIREFRAME;
    if (s == "hiddenline")
        return HIDDENLINE;
    if (s == "points")
        return QWT3D_POINTS;
    if (s == "noplot")
        return NOPLOT;
    return FILLEDMESH;
}

// z = sin(2r) * exp(-r^2/8), r = sqrt(x^2+y^2), domain [-3,3]^2
void makeGrid(std::vector<std::vector<double> >& z, int cols, int rows)
{
    z.assign(cols, std::vector<double>(rows, 0.0));
    for (int i = 0; i < cols; ++i) {
        const double x = -3.0 + 6.0 * i / (cols - 1);
        for (int j = 0; j < rows; ++j) {
            const double y = -3.0 + 6.0 * j / (rows - 1);
            const double r2 = x * x + y * y;
            z[i][j] = std::sin(2.0 * std::sqrt(r2)) * std::exp(-r2 / 8.0);
        }
    }
}

struct GridPtrs {
    std::vector<double*> ptrs;
    explicit GridPtrs(std::vector<std::vector<double> >& z)
    {
        ptrs.resize(z.size());
        for (size_t i = 0; i < z.size(); ++i)
            ptrs[i] = z[i].data();
    }
    double** data() { return ptrs.data(); }
};

double median(std::vector<double> v)
{
    if (v.empty())
        return 0.0;
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    return (n % 2) ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

double percentile(std::vector<double> v, double p)
{
    if (v.empty())
        return 0.0;
    std::sort(v.begin(), v.end());
    size_t idx = static_cast<size_t>(p * (v.size() - 1) + 0.5);
    return v[idx];
}

QString csvField(double v)
{
    return QString::number(v, 'f', 4);
}

QString optArg(int argc, char* argv[], const QString& name, const QString& def)
{
    for (int i = 1; i < argc - 1; ++i)
        if (name == argv[i])
            return argv[i + 1];
    return def;
}

void printEnv(QOpenGLWidget* widget)
{
    QTextStream err(stderr);
    if (widget && widget->isValid()) {
        widget->makeCurrent();
        QOpenGLContext* ctx = widget->context();
        QOpenGLFunctions* f = ctx->functions();
        err << "# GL_VENDOR: " << reinterpret_cast<const char*>(f->glGetString(GL_VENDOR)) << "\n";
        err << "# GL_RENDERER: " << reinterpret_cast<const char*>(f->glGetString(GL_RENDERER)) << "\n";
        err << "# GL_VERSION: " << reinterpret_cast<const char*>(f->glGetString(GL_VERSION)) << "\n";
        err << "# context swapInterval: " << ctx->format().swapInterval()
            << " samples: " << ctx->format().samples() << "\n";
    }
    err << "# Qt " << QT_VERSION_STR << ", QWT " << QWT_VERSION_STR << "\n";
}

// counts total occurrences of all needles in a (text) file
int countInFile(const QString& fname, const QList<QByteArray>& needles)
{
    QFile f(fname);
    if (!f.open(QIODevice::ReadOnly))
        return -1;
    const QByteArray all = f.readAll();
    int total = 0;
    for (const QByteArray& n : needles) {
        int pos = 0;
        while ((pos = all.indexOf(n, pos)) != -1) {
            ++total;
            pos += n.size();
        }
    }
    return total;
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: surface3d_bench <frames|upload|export|env> ...\n");
        return 2;
    }

    const QString mode = argv[1];

    const int msaa = optArg(argc, argv, "--msaa", "4").toInt();
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    fmt.setSamples(msaa);
    fmt.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);

    QTextStream out(stdout);
    QTextStream err(stderr);

    if (mode == "env") {
        BenchPlot plot;
        plot.resize(64, 64);
        plot.show();
        app.processEvents();
        plot.update();
        app.processEvents();
        printEnv(&plot);
        out << "env,,,,,,,,,,,,,,,,,,,,,,,,,,,\n";
        return 0;
    }

    const int cols = (argc > 2) ? atoi(argv[2]) : 64;
    const int rows = cols;
    const QString styleStr = (mode == "frames" && argc > 3) ? argv[3] : "filledmesh";
    const int light = (mode == "frames" && argc > 4) ? atoi(argv[4]) : 0;
    const QString coordStr = optArg(argc, argv, "--coord", "box");
    const bool finish = optArg(argc, argv, "--finish", "off") == "on";
    const int res = optArg(argc, argv, "--res", "1").toInt();
    const QString sortStr = optArg(argc, argv, "--sort", "simple");
    const int rounds = optArg(argc, argv, "--rounds", "3").toInt();
    const int fpp = optArg(argc, argv, "--fpp", "100").toInt();
    const int reps = optArg(argc, argv, "--reps", "3").toInt();
    const int w = optArg(argc, argv, "--w", "900").toInt();
    const int h = optArg(argc, argv, "--h", "600").toInt();

    // scene data
    std::vector<std::vector<double> > z;
    makeGrid(z, cols, rows);

    BenchPlot* plot = new BenchPlot();
    plot->resize(w, h);
    plot->finishAfterPaint = finish;

    Qwt3DSurface* surface = new Qwt3DSurface();
    surface->setPlotStyle(parseStyle(styleStr));
    if (res > 1)
        surface->setResolution(res);
    surface->attach(plot);

    if (light)
        plot->enableLighting();

    plot->show();
    app.processEvents();
    plot->update(); // triggers initializeGL + first paintGL
    app.processEvents();

    if (coordStr == "nocoord")
        plot->coordinates()->setStyle(NOCOORD);

    err << "# mode=" << mode << " cols=" << cols << " style=" << styleStr
        << " light=" << light << " coord=" << coordStr << " finish=" << finish
        << " msaa=" << msaa << " res=" << res << " size=" << w << "x" << h << "\n";
    printEnv(plot);

    const int vertices = cols * rows;
    const long long triIdx = 6LL * (cols - 1) * (rows - 1);
    const long long lineIdx = 4LL * (cols - 1) * (rows - 1) + 2LL * ((cols - 1) + (rows - 1));
    err << "# analytic: vertices=" << vertices
        << " triangles=" << (triIdx / 3) << " triIdx=" << triIdx << " lineIdx=" << lineIdx
        << " vboBytes=" << (40LL * vertices)
        << " eboBytes=" << (4LL * (triIdx + lineIdx)) << "\n";

    if (mode == "frames") {
        // load data once, then measure steady-state interaction frames
        GridPtrs gp(z);
        surface->loadFromData(gp.data(), cols, rows, -3.0, 3.0, -3.0, 3.0);
        plot->update();
        app.processEvents();

        int retries = 0;
        std::vector<double> pooled;
        std::vector<double> roundMeds(rounds, 0.0);
        const long long basePaints = plot->paintCount;

        for (int attempt = 0; attempt <= 2; ++attempt) {
            pooled.clear();
            roundMeds.assign(rounds, 0.0);
            double rz = 0.0;
            // warmup
            for (int i = 0; i < 5; ++i) {
                rz += 0.7;
                plot->setRotation(30.0, 0.0, rz);
                plot->update();
                app.processEvents();
            }
            for (int r = 0; r < rounds; ++r) {
                std::vector<double> roundTimes;
                QElapsedTimer t;
                for (int f = 0; f < fpp; ++f) {
                    rz += 0.7;
                    plot->setRotation(30.0, 0.0, rz);
                    t.start();
                    plot->update();
                    app.processEvents();
                    roundTimes.push_back(t.nsecsElapsed() / 1e6);
                }
                roundMeds[r] = median(roundTimes);
                pooled.insert(pooled.end(), roundTimes.begin(), roundTimes.end());
            }
            const double lo = *std::min_element(roundMeds.begin(), roundMeds.end());
            const double hi = *std::max_element(roundMeds.begin(), roundMeds.end());
            if (lo > 0 && hi / lo <= 1.5)
                break;
            retries = attempt + 1;
            err << "# round-median jitter " << hi / lo << " > 1.5, retry " << retries << "\n";
        }
        app.processEvents();

        const long long paints = plot->paintCount - basePaints;
        QStringList row;
        row << "frames" << QString::number(cols) << styleStr << QString::number(light)
            << coordStr << (finish ? "1" : "0") << QString::number(res)
            << QString("msaa%1").arg(msaa) << ""
            << QString::number(rounds) << QString::number(fpp) << QString::number(retries)
            << QString::number(paints)
            << "" << "" << "" << "" << "" << ""
            << csvField(percentile(pooled, 0.0)) << csvField(median(pooled))
            << csvField(percentile(pooled, 0.99));
        for (int r = 0; r < 3; ++r)
            row << (r < rounds ? csvField(roundMeds[r]) : QString(""));
        row << "" << "" << "";
        out << row.join(",") << "\n";
    } else if (mode == "upload") {
        // stage timing around the lazy VBO build path; each stage followed by
        // glFinish so the numbers include GPU completion
        QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
        plot->makeCurrent();

        std::vector<double> loadT, firstT, steadyT, recolorT, res2T, propT;
        GridPtrs gp(z);
        QElapsedTimer t;

        for (int rep = 0; rep < reps; ++rep) {
            t.start();
            surface->loadFromData(gp.data(), cols, rows, -3.0, 3.0, -3.0, 3.0);
            loadT.push_back(t.nsecsElapsed() / 1e6);

            t.start();
            surface->draw(); // dirty -> buildVBO() + draw
            f->glFinish();
            firstT.push_back(t.nsecsElapsed() / 1e6);

            t.start();
            surface->draw(); // clean -> uniforms + glDrawElements only
            f->glFinish();
            steadyT.push_back(t.nsecsElapsed() / 1e6);

            surface->invalidateColors();
            t.start();
            surface->draw(); // color re-eval + full VBO re-upload
            f->glFinish();
            recolorT.push_back(t.nsecsElapsed() / 1e6);

            surface->setResolution(2);
            t.start();
            surface->draw(); // index rebuild (+ full vertex rebuild)
            f->glFinish();
            res2T.push_back(t.nsecsElapsed() / 1e6);
            surface->setResolution(1);
            surface->draw();
            f->glFinish();

            t.start();
            surface->setMeshColor(RGBA(0.1, 0.1, 0.1, 1.0)); // plain setter -> itemChanged()
            propT.push_back(t.nsecsElapsed() / 1e6);
        }

        err << "# upload reps: load=";
        for (double v : loadT) err << v << " ";
        err << " first=";
        for (double v : firstT) err << v << " ";
        err << " steady=";
        for (double v : steadyT) err << v << " ";
        err << " recolor=";
        for (double v : recolorT) err << v << " ";
        err << " res2=";
        for (double v : res2T) err << v << " ";
        err << " propset=";
        for (double v : propT) err << v << "\n";

        QStringList row;
        row << "upload" << QString::number(cols) << styleStr << QString::number(light)
            << coordStr << "1" << QString::number(res) << "" << ""
            << "" << "" << "0" << QString::number(plot->paintCount)
            << csvField(median(loadT)) << csvField(median(firstT)) << csvField(median(steadyT))
            << csvField(median(recolorT)) << csvField(median(res2T)) << csvField(median(propT))
            << "" << "" << "" << "" << "" << ""
            << "" << "" << "";
        out << row.join(",") << "\n";
    } else if (mode == "export") {
        const QString fmtStr = (argc > 3) ? argv[2 + 1] : "PDF";
        Qwt3DVectorWriter::SORTMODE sortmode = Qwt3DVectorWriter::SIMPLESORT;
        if (sortStr == "bsp")
            sortmode = Qwt3DVectorWriter::BSPSORT;
        else if (sortStr == "no")
            sortmode = Qwt3DVectorWriter::NOSORT;

        GridPtrs gp(z);
        surface->loadFromData(gp.data(), cols, rows, -3.0, 3.0, -3.0, 3.0);
        plot->update();
        app.processEvents();

        const QString ext = fmtStr.toLower();
        const QString fname = QString("D:/tmp/qwt-scratch/plot3d/out/surface_%1_%2.%3")
                                  .arg(cols).arg(ext).arg(ext == "ps" ? "ps" : ext);
        QDir().mkpath("D:/tmp/qwt-scratch/plot3d/out");

        QElapsedTimer t;
        t.start();
        bool ok = false;
        if (fmtStr.toLower() == "png")
            ok = plot->savePixmap(fname, "png"); // registry keys are lowercase, exact match
        else
            ok = plot->saveVector(fname, fmtStr.toUpper(), Qwt3DVectorWriter::PIXEL, sortmode);
        const double exportMs = t.nsecsElapsed() / 1e6;
        app.processEvents();

        const qint64 bytes = ok ? QFileInfo(fname).size() : -1;
        int prims = -1;
        if (ok && ext == "svg") {
            prims = countInFile(fname,
                QList<QByteArray>() << "<path" << "<polygon" << "<polyline" << "<line " << "<text");
        }
        err << "# export " << fmtStr << " sort=" << sortStr << " ok=" << ok
            << " bytes=" << bytes << " prims=" << prims << " time=" << exportMs << " ms\n";

        QStringList row;
        row << "export" << QString::number(cols) << styleStr << QString::number(light)
            << coordStr << "0" << QString::number(res) << fmtStr << sortStr
            << "" << "" << "0" << QString::number(plot->paintCount)
            << "" << "" << "" << "" << "" << ""
            << "" << "" << "" << "" << "" << ""
            << csvField(exportMs) << QString::number(bytes) << QString::number(prims);
        out << row.join(",") << "\n";
    } else {
        fprintf(stderr, "unknown mode\n");
        return 2;
    }

    plot->close();
    delete plot;
    return 0;
}
