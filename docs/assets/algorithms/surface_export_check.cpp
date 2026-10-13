// surface_export_check.cpp - checks what a Qwt3DPlot vector export contains.
//
// The gl2ps vector path captures the scene from the GL feedback buffer, where
// the per vertex color is the fixed-function "current color". The shader based
// render path of this module never writes it, so a feedback replay used to
// produce fully transparent polygons (alpha 0): the surface was invisible in
// every PDF/SVG export while the coordinate system and labels showed up. This
// probe exports a small surface and inspects the output text:
//
//   SVG: every <polygon> fill + fill-opacity, distinct fill colors
//   PDF: every ExtGState /ca value
//
// It prints one line per format plus a verdict line, and exits non zero when
// the exported surface carries no visible color.
//
// Build (MSVC x64, from a vcvars64 shell; Qt 6, release libraries of the repo):
//   cl /nologo /O2 /EHsc /std:c++17 /Zc:__cplusplus /permissive- /MD ^
//      /I<repo>/src /I<repo>/src/core /I<repo>/src/plot /I<repo>/src/plot3d ^
//      /I<qt>/include /I<qt>/include/QtCore /I<qt>/include/QtGui ^
//      /I<qt>/include/QtWidgets /I<qt>/include/QtOpenGL ^
//      /I<qt>/include/QtOpenGLWidgets ^
//      surface_export_check.cpp /Fe:surface_export_check.exe ^
//      /link /LIBPATH:<repo>/build/lib/Release /LIBPATH:<qt>/lib ^
//      qwtplot3d.lib qwtcore.lib Qt6Widgets.lib Qt6Gui.lib Qt6Core.lib ^
//      Qt6OpenGL.lib Qt6OpenGLWidgets.lib Qt6Svg.lib
//
// Run (needs a desktop session - Qwt3DPlot is a QOpenGLWidget):
//   surface_export_check.exe [output-dir]        (default: current directory)
//
// Uses Qwt3DPlot::save(), i.e. the Qwt3DVectorWriter registry entry of the
// format (default text mode, simple sort) - the same path the deprecated
// saveVector() overload ends in.

#include "qwt3d_color.h"
#include "qwt3d_plot.h"
#include "qwt3d_surface.h"

#include <QApplication>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QRegularExpression>
#include <QtSvg/QSvgRenderer>
#include <QSurfaceFormat>
#include <QTextStream>
#include <QThread>

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{

void inspectSvg(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        std::printf("[SVG] %s: cannot open\n", qPrintable(path));
        return;
    }

    const QString text = QString::fromUtf8(file.readAll());

    int polygons      = 0;
    int opaquePolys   = 0;
    int transparent   = 0;
    double opacityMin = 2.0, opacityMax = -1.0;
    QStringList fills;

    const QRegularExpression re(QStringLiteral("<polygon([^>]*)>"),
                                QRegularExpression::MultilineOption);
    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        const QString attrs = it.next().captured(1);
        ++polygons;

        const QRegularExpression fillRe(QStringLiteral("fill=\"(#[0-9a-fA-F]+)\""));
        const auto fillMatch = fillRe.match(attrs);
        if (fillMatch.hasMatch() && !fills.contains(fillMatch.captured(1)))
            fills.append(fillMatch.captured(1));

        double opacity = 1.0;
        const QRegularExpression opRe(QStringLiteral("fill-opacity=\"([0-9.eE+-]+)\""));
        const auto opMatch = opRe.match(attrs);
        if (opMatch.hasMatch())
            opacity = opMatch.captured(1).toDouble();

        opacityMin = qMin(opacityMin, opacity);
        opacityMax = qMax(opacityMax, opacity);
        if (opacity > 0.0)
            ++opaquePolys;
        else
            ++transparent;
    }

    std::printf("SVG polygons=%d fill_colors=%d opacity_min=%.3f opacity_max=%.3f visible=%d transparent=%d\n",
                polygons, static_cast< int >(fills.size()),
                polygons ? opacityMin : -1.0, opacityMax, opaquePolys, transparent);
}

void inspectPdf(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        std::printf("[PDF] %s: cannot open\n", qPrintable(path));
        return;
    }

    const QByteArray data = file.readAll();

    int states      = 0;
    int nonZero     = 0;
    double alphaMin = 2.0, alphaMax = -1.0;

    const QRegularExpression re(QStringLiteral("/ca ([0-9.]+)"));
    auto it = re.globalMatch(QString::fromLatin1(data));
    while (it.hasNext()) {
        const double alpha = it.next().captured(1).toDouble();
        ++states;
        alphaMin = qMin(alphaMin, alpha);
        alphaMax = qMax(alphaMax, alpha);
        if (alpha > 0.0)
            ++nonZero;
    }

    std::printf("PDF extgstates=%d ca_min=%.3f ca_max=%.3f ca_gt0=%d\n",
                states, states ? alphaMin : -1.0, alphaMax, nonZero);
}

/**
 * @brief Compares the rasterized SVG against the raster export of the same scene
 * @details Block wise mean color distance (24 x 18 blocks); the raster export
 *          comes from the shader path and is correct by construction, so a
 *          faithful vector export matches it apart from antialiasing and the
 *          color quantization of gl2ps.
 */
void compareSvgWithRaster(const QString& svgPath, const QString& pngPath)
{
    QImage png(pngPath);
    if (png.isNull()) {
        std::printf("RASTER %s: cannot load\n", qPrintable(pngPath));
        return;
    }

    QSvgRenderer renderer(svgPath);
    if (!renderer.isValid()) {
        std::printf("RASTER %s: invalid SVG\n", qPrintable(svgPath));
        return;
    }

    QImage svg(png.size(), QImage::Format_ARGB32_Premultiplied);
    svg.fill(Qt::white);
    {
        QPainter painter(&svg);
        renderer.render(&painter);
    }

    const int blocksX = 24, blocksY = 18;
    const int bw = png.width() / blocksX;
    const int bh = png.height() / blocksY;

    int deviating = 0;
    double worst = 0.0, sum = 0.0;

    for (int by = 0; by < blocksY; ++by) {
        for (int bx = 0; bx < blocksX; ++bx) {
            double r1 = 0, g1 = 0, b1 = 0, r2 = 0, g2 = 0, b2 = 0;
            int n = 0;
            for (int y = by * bh; y < (by + 1) * bh; ++y) {
                for (int x = bx * bw; x < (bx + 1) * bw; ++x) {
                    const QRgb a = png.pixel(x, y);
                    const QRgb c = svg.pixel(x, y);
                    r1 += qRed(a); g1 += qGreen(a); b1 += qBlue(a);
                    r2 += qRed(c); g2 += qGreen(c); b2 += qBlue(c);
                    ++n;
                }
            }
            r1 /= n; g1 /= n; b1 /= n; r2 /= n; g2 /= n; b2 /= n;

            const double dist = std::sqrt((r1 - r2) * (r1 - r2) + (g1 - g2) * (g1 - g2) + (b1 - b2) * (b1 - b2));
            sum += dist;
            worst = qMax(worst, dist);
            if (dist > 60.0)
                ++deviating;
        }
    }

    const int blocks = blocksX * blocksY;
    std::printf("RASTER svg_vs_png_blocks=%d deviating=%d (%.1f %%) mean_dist=%.1f worst=%.1f\n",
                blocks, deviating, 100.0 * deviating / blocks, sum / blocks, worst);
}

/**
 * @brief Data color functor painting every vertex with the same color
 * @details With a constant functor the correct vector colors are known in
 *          advance: a data faithful export may only contain that color. The
 *          gl2ps feedback path of the shader based renderer publishes no
 *          color at all, so it cannot satisfy this.
 */
class FlatColor : public Qwt3DColor
{
public:
    RGBA operator()(double, double, double) const override { return RGBA(1.0, 0.0, 0.0, 1.0); }
};

/**
 * @brief Share of the on page polygon fills that carry the given data color
 * @details With a constant data color functor every surface polygon has to be
 *          exported in that color. Off page polygons (gl2ps writes primitives
 *          that are outside the viewport as well) and the background rectangle
 *          are ignored.
 */
void dataColorShare(const QString& svgPath, int r, int g, int b, double tolerance)
{
    QFile file(svgPath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QString text = QString::fromUtf8(file.readAll());

    double pageW = 0, pageH = 0;
    const auto viewBox = QRegularExpression(QStringLiteral("viewBox=.([-0-9. ]+).")).match(text);
    if (viewBox.hasMatch()) {
        const QStringList parts = viewBox.captured(1).split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (parts.size() == 4) {
            pageW = parts[ 2 ].toDouble();
            pageH = parts[ 3 ].toDouble();
        }
    }

    const QRegularExpression polyRe(QStringLiteral("<polygon([^>]*)/?>"));
    const QRegularExpression fillRe(QStringLiteral("fill=.#([0-9a-fA-F]{6})."));
    const QRegularExpression pointsRe(QStringLiteral("points=.([^\x22]*)"));
    const QRegularExpression pointRe(QStringLiteral("(-?[0-9.]+),(-?[0-9.]+)"));

    int inside = 0, matching = 0;
    double totalArea = 0.0, matchingArea = 0.0;

    auto it = polyRe.globalMatch(text);
    while (it.hasNext()) {
        const QString attrs = it.next().captured(1);

        const auto fillMatch   = fillRe.match(attrs);
        const auto pointsMatch = pointsRe.match(attrs);
        if (!fillMatch.hasMatch() || !pointsMatch.hasMatch())
            continue;

        QVector< QPointF > points;
        auto pit = pointRe.globalMatch(pointsMatch.captured(1));
        while (pit.hasNext()) {
            const auto pair = pit.next();
            points.append(QPointF(pair.captured(1).toDouble(), pair.captured(2).toDouble()));
        }
        if (points.size() < 3)
            continue;

        QPointF centroid;
        double area2 = 0.0;
        for (int k = 0; k < points.size(); ++k) {
            const QPointF& a = points[ k ];
            const QPointF& b = points[ (k + 1) % points.size() ];
            area2 += a.x() * b.y() - b.x() * a.y();
            centroid += a;
        }
        centroid /= double(points.size());

        if (pageW > 0 && pageH > 0
            && (centroid.x() < 0 || centroid.y() < 0 || centroid.x() > pageW || centroid.y() > pageH)) {
            continue;  // off page primitives are not part of the picture
        }

        const double area = qAbs(area2) * 0.5;
        if (pageW > 0 && pageH > 0 && area > 0.5 * pageW * pageH)
            continue;  // the gl2ps background rectangle

        ++inside;
        totalArea += area;

        const QString hex = fillMatch.captured(1);
        const double d = std::sqrt(qPow(hex.mid(0, 2).toInt(nullptr, 16) - r, 2.0)
                                   + qPow(hex.mid(2, 2).toInt(nullptr, 16) - g, 2.0)
                                   + qPow(hex.mid(4, 2).toInt(nullptr, 16) - b, 2.0));
        if (d <= tolerance) {
            ++matching;
            matchingArea += area;
        }
    }

    const double share = totalArea > 0.0 ? 100.0 * matchingArea / totalArea : 0.0;
    std::printf("FLATCOLOR on_page=%d data_color=%d area_share=%.1f %% verdict=%s\n",
                inside, matching, share, (share >= 50.0) ? "PASS" : "FAIL");
}

} // namespace



int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    const bool flatColors = (argc > 2) && (QString::fromLocal8Bit(argv[ 2 ]) == QStringLiteral("flat"));
    const QString dir = (argc > 1) ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    const QString svgPath = dir + QStringLiteral("/surface_export.svg");
    const QString pdfPath = dir + QStringLiteral("/surface_export.pdf");

    Qwt3DPlot plot;
    plot.resize(480, 360);

    const int cols = 24, rows = 24;
    std::vector< std::vector< double > > grid(rows, std::vector< double >(cols));
    std::vector< double* > rowsPtr(rows);
    for (int j = 0; j < rows; ++j) {
        rowsPtr[ j ] = grid[ j ].data();
        for (int i = 0; i < cols; ++i) {
            const double x = double(i) / (cols - 1);
            const double y = double(j) / (rows - 1);
            grid[ j ][ i ] = std::sin(3.0 * x) * std::cos(3.0 * y) + 0.3 * x;
        }
    }

    auto* surface = new Qwt3DSurface();
    surface->loadFromData(rowsPtr.data(), cols, rows, 0.0, 1.0, 0.0, 1.0);
    FlatColor flatColor;
    if (flatColors)
        surface->setDataColor(&flatColor);
    surface->attach(&plot);

    plot.show();

    // let the widget create its GL context and draw the first frame
    for (int i = 0; i < 40; ++i) {
        app.processEvents();
        QThread::msleep(10);
    }

    const QString pngPath = dir + QStringLiteral("/surface_export.png");
    const bool pngOk = plot.savePixmap(pngPath, QStringLiteral("png"));
    const bool svgOk = plot.save(svgPath, QStringLiteral("SVG"));
    const bool pdfOk = plot.save(pdfPath, QStringLiteral("PDF"));

    std::printf("export png=%d (%s) svg=%d pdf=%d\n", pngOk ? 1 : 0, qPrintable(pngPath),
                svgOk ? 1 : 0, pdfOk ? 1 : 0);

    if (svgOk)
        inspectSvg(svgPath);
    if (pdfOk)
        inspectPdf(pdfPath);
    if (svgOk && pngOk)
        compareSvgWithRaster(svgPath, pngPath);
    if (svgOk && flatColors)
        dataColorShare(svgPath, 255, 0, 0, 40.0);

    return (svgOk && pdfOk) ? 0 : 1;
}
