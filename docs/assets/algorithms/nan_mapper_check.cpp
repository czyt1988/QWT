// Semantic cross-validation harness for NaN rendering in Qwt 7.x.
//
// Companion to docs/zh/tech-report/nan-rendering.md. The Python
// re-implementation in nan_experiment.py mirrors every mapper path
// line-by-line; this harness runs the REAL library code (qwtplot.dll)
// and dumps results so both sides can be compared exactly.
//
// Subcommands:
//   matrix              directed NaN pattern cases through
//                       QwtPointMapper::toPolygonF (full vertex dumps)
//   random <count> <seed>
//                       randomized campaign; dumps case data + a
//                       deterministic polygon hash per case/mode
//   visual [outdir]     offscreen render scan: one curve with NaN vs the
//                       ideal split-segment rendering, per pixel diff
//   probes              curve-style level behaviour probes (Steps/Sticks/
//                       Dots), boundingRect edge cases, x-NaN hazards
//
// Output grammar (stdout, one record per line):
//   CANVAS <id> <W> <H> <xs1> <xs2> <ys1> <ys2>
//   DATA <id> <n> <x0> <y0> ... <xn-1> <yn-1>     (%.17g, nan/inf/-inf)
//   V <id> <mode> <nVerts> <x0> <y0> ...          (%.17g, nan/inf/-inf)
//   RD <idx> <W> <H> <n> <x0> <y0> ...            (random campaign data)
//   RH <idx> <W> <H> <mode> <nVerts> <hash>       (uint64 polygon hash)
//   VIS <pattern> <mode> <idealSegs> <diffPx> <imgPx> <maxDev> <bbL> <bbT> <bbR> <bbB>
//   PROBE <name> <value>
//
// QwtPointMapper is exported (QWT_EXPORT in qwt_point_mapper.h), so the
// harness drives the mapping layer directly - no pixel guessing needed for
// the Lines paths. Steps/Sticks/Dots live inside QwtPlotCurve and are
// verified behaviourally through rendered probe pixels.
//
// Build (MSVC 2019 x64, Qt 5.15.16, prebuilt qwt in build-release):
//   cl /nologo /O2 /EHsc /std:c++17 /MD
//      /I<repo>/src /I<repo>/src/core /I<repo>/src/plot
//      /I<qt>/include /I<qt>/include/QtCore /I<qt>/include/QtGui
//      /I<qt>/include/QtWidgets /I<qt>/include/QtPrintSupport
//      /I<qt>/include/QtConcurrent
//      nan_mapper_check.cpp /Fe:nan_mapper_check.exe
//      /link /LIBPATH:<repo>/build-release/lib /LIBPATH:<qt>/lib
//            qwtplot.lib qwtcore.lib Qt5Widgets.lib Qt5Gui.lib Qt5Core.lib
//            Qt5PrintSupport.lib Qt5Concurrent.lib opengl32.lib
//   run with PATH containing <qt>/bin and <repo>/build-release/bin
//   (use "-platform offscreen" to keep the render scan headless)

#include <QApplication>
#include <QImage>
#include <QPen>
#include <QVector>
#include <QtMath>

#include <qwt_axis.h>
#include <qwt_math.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_renderer.h>
#include <qwt_point_data.h>
#include <qwt_point_mapper.h>
#include <qwt_scale_map.h>
#include <qwt_series_data.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

// ---------------------------------------------------------------- utilities

static void emitDouble(double v)
{
    if (std::isnan(v))
        std::printf(" nan");
    else if (std::isinf(v))
        std::printf(v > 0 ? " inf" : " -inf");
    else
        std::printf(" %.17g", v);
}

// Deterministic RNG shared bit-exactly with nan_experiment.py (xorshift64*).
struct Rng64
{
    uint64_t s;
    explicit Rng64(uint64_t seed) : s(seed ? seed : 88172645463325252ULL) {}
    uint64_t next()
    {
        s ^= s >> 12;
        s ^= s << 25;
        s ^= s >> 27;
        return s * 2685821657736338717ULL;
    }
};

// Quantize a polygon coordinate for hashing. Tokens for non-finite/huge
// values make the hash total (all-NaN and x-NaN garbage cases included).
static long long quantVertex(double v)
{
    if (std::isnan(v))
        return -2;
    if (v > 1e12)
        return -3;
    if (v < -1e12)
        return -4;
    return (long long) std::floor(v * 1e6 + 0.5);
}

static const uint64_t kHashMod = 4294967291ULL;  // 2^32 - 5

static uint64_t hashPolygon(const QPolygonF& poly)
{
    uint64_t h = (uint64_t) poly.size() % kHashMod;
    for (const QPointF& p : poly) {
        long long qx = quantVertex(p.x());
        long long qy = quantVertex(p.y());
        uint64_t ux = (uint64_t) (qx % (long long) kHashMod + (long long) kHashMod) % kHashMod;
        uint64_t uy = (uint64_t) (qy % (long long) kHashMod + (long long) kHashMod) % kHashMod;
        h = (h * 1000003ULL + ux) % kHashMod;
        h = (h * 1000003ULL + uy) % kHashMod;
    }
    return h;
}

// ---------------------------------------------------------------- modes

struct ModeDef
{
    const char* name;
    QwtPointMapper::TransformationFlags flags;
    bool genericSeries;  // use QwtPointSeriesData (no raw pointer access)
};

static const std::vector< ModeDef >& modes()
{
    static const std::vector< ModeDef > m = {
        { "none", QwtPointMapper::RoundPoints, false },
        { "points", QwtPointMapper::TransformationFlags(QwtPointMapper::RoundPoints | QwtPointMapper::WeedOutPoints), false },
        { "aggressive", QwtPointMapper::TransformationFlags(QwtPointMapper::RoundPoints | QwtPointMapper::WeedOutIntermediatePoints | QwtPointMapper::WeedOutPoints), false },
        { "pixel", QwtPointMapper::PixelColumnReduce, false },
        { "lttb", QwtPointMapper::MinMaxReduce, false },
        { "lttbgen", QwtPointMapper::MinMaxReduce, true },
    };
    return m;
}

// Flags exactly as QwtPlotCurve::drawLines() sets them for a raster painter
// with rounding alignment (doAlign == true).

static QwtScaleMap makeMap(double s1, double s2, double p1, double p2)
{
    QwtScaleMap map;
    map.setScaleInterval(s1, s2);
    map.setPaintInterval(p1, p2);
    return map;
}

static void yScaleFromData(const QVector< double >& x, const QVector< double >& y,
                           double& xs1, double& xs2, double& ys1, double& ys2)
{
    xs1 = 0.0;
    xs2 = qMax(1.0, (double) x.size() - 1.0);
    double lo = DBL_MAX, hi = -DBL_MAX;
    for (int i = 0; i < y.size(); i++) {
        if (!std::isfinite(y[ i ]) || !std::isfinite(x[ i ]))
            continue;
        lo = qMin(lo, y[ i ]);
        hi = qMax(hi, y[ i ]);
    }
    if (lo > hi) {
        ys1 = -1.0;
        ys2 = 1.0;
    } else if (lo == hi) {
        ys1 = lo - 1.0;
        ys2 = hi + 1.0;
    } else {
        ys1 = lo;
        ys2 = hi;
    }
}

// Run one mode over a dataset and return the mapped polygon.
static QPolygonF runMapper(const ModeDef& mode, const QVector< double >& x, const QVector< double >& y,
                           double xs1, double xs2, double ys1, double ys2, int W, int H)
{
    const QwtScaleMap xMap = makeMap(xs1, xs2, 0.0, (double) (W - 1));
    const QwtScaleMap yMap = makeMap(ys1, ys2, (double) (H - 1), 0.0);

    QwtPointMapper mapper;
    mapper.setFlags(mode.flags);

    const int n = x.size();
    if (mode.genericSeries) {
        QVector< QPointF > pts(n);
        for (int i = 0; i < n; i++)
            pts[ i ] = QPointF(x[ i ], y[ i ]);
        QwtPointSeriesData series(pts);
        return mapper.toPolygonF(xMap, yMap, &series, 0, n - 1);
    }

    QwtPointArrayData< double > series(x, y);
    return mapper.toPolygonF(xMap, yMap, &series, 0, n - 1);
}

static void emitPolygon(const char* id, const char* modeName, const QPolygonF& poly)
{
    std::printf("V %s %s %d", id, modeName, (int) poly.size());
    for (const QPointF& p : poly) {
        emitDouble(p.x());
        emitDouble(p.y());
    }
    std::printf("\n");
    std::printf("MH %s %s %d %llu\n", id, modeName, (int) poly.size(),
                (unsigned long long) hashPolygon(poly));
}

// ---------------------------------------------------------------- data patterns

// Cross-language signal: uses ONLY + - * / on doubles (no libm
// transcendentals), so C++ and numpy evaluate it bit-identically and the
// Python re-implementation can be compared exactly.
static double exactSignal(int i, int period)
{
    const double t = (double) (i % period) - (double) period * 0.5;
    double v = 2.4 / (1.0 + t * t / ((double) period * (double) period / 400.0));
    const double u = (double) (i % 47);
    v += 0.35 * (u * u - 21.0 * u) / 250.0;
    if (i % 991 == 0)
        v += 2.2;   // narrow positive spike
    if (i % 733 == 17)
        v -= 1.9;   // narrow negative spike
    return v;
}

static void addSpikes(QVector< double >& y, const std::vector< int >& where, double amp)
{
    for (int idx : where) {
        if (idx >= 0 && idx < y.size())
            y[ idx ] += amp;
    }
}

// Directed NaN patterns. kind selects the placement; n samples, x = index
// unless the pattern injects x-NaN too.
static void makePattern(const QString& kind, int n, int period, QVector< double >& x, QVector< double >& y)
{
    const double nan = qQNaN();
    x.resize(n);
    y.resize(n);
    for (int i = 0; i < n; i++) {
        x[ i ] = i;
        y[ i ] = exactSignal(i, period);
    }

    if (kind == "baseline") {
        // no NaN
    } else if (kind == "isolated") {
        for (int i = 137; i < n; i += 997)
            y[ i ] = nan;
    } else if (kind == "run") {
        for (int c = 0; c < 3; c++) {
            const int start = n / 4 * (c + 1);
            for (int i = start; i < start + qMax(8, n / 25) && i < n; i++)
                y[ i ] = nan;
        }
    } else if (kind == "leading") {
        for (int i = 0; i < n / 10; i++)
            y[ i ] = nan;
    } else if (kind == "trailing") {
        for (int i = n - n / 10; i < n; i++)
            y[ i ] = nan;
    } else if (kind == "leadtrail") {
        for (int i = 0; i < n / 10; i++)
            y[ i ] = nan;
        for (int i = n - n / 10; i < n; i++)
            y[ i ] = nan;
    } else if (kind == "all") {
        for (int i = 0; i < n; i++)
            y[ i ] = nan;
    } else if (kind == "extremum") {
        // global max/min and spike tips are NaN
        int iMin = 0, iMax = 0;
        for (int i = 1; i < n; i++) {
            if (y[ i ] < y[ iMin ])
                iMin = i;
            if (y[ i ] > y[ iMax ])
                iMax = i;
        }
        y[ iMin ] = nan;
        y[ iMax ] = nan;
        y[ n / 2 ] = nan;
    } else if (kind == "periodic") {
        for (int i = 0; i < n; i++)
            if ((i % 50) < 5)
                y[ i ] = nan;
    } else if (kind == "xynan") {
        const int start = (n - n / 10) / 2;
        for (int i = start; i < start + n / 10; i++) {
            x[ i ] = nan;
            y[ i ] = nan;
        }
    } else if (kind == "xyinter") {
        const int start = (n - n / 10) / 2;
        for (int i = start; i < start + n / 10; i++) {
            if ((i - start) % 2 == 0)
                x[ i ] = nan;  // x-only NaN, y finite
            else
                y[ i ] = nan;  // y-only NaN, x finite
        }
    } else if (kind == "xonly") {
        x[ n / 2 ] = nan;  // single x-NaN, y finite everywhere
    } else if (kind == "inf") {
        y[ n / 3 ] = std::numeric_limits< double >::infinity();
        y[ 2 * n / 3 ] = -std::numeric_limits< double >::infinity();
    } else if (kind == "spikenan") {
        // spikes whose tips are NaN: the finite envelope must not invent them
        const std::vector< int > where = { n / 8, 3 * n / 8, 5 * n / 8, 7 * n / 8 };
        for (int idx : where)
            y[ idx ] = nan;
    }
}

// ---------------------------------------------------------------- matrix

static void runMatrix()
{
    // Worked example: tiny case with a full vertex dump in the article.
    {
        QVector< double > x = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        QVector< double > y = { 1, 2, qQNaN(), 0, qQNaN(), qQNaN(), 3, qQNaN(), 2, 1, qQNaN(), 0 };
        const int W = 12, H = 8;
        std::printf("CANVAS tiny %d %d %.17g %.17g %.17g %.17g\n", W, H, 0.0, 11.0, 0.0, 3.0);
        std::printf("DATA tiny %d", (int) x.size());
        for (int i = 0; i < x.size(); i++) {
            emitDouble(x[ i ]);
            emitDouble(y[ i ]);
        }
        std::printf("\n");
        for (const ModeDef& m : modes()) {
            const QPolygonF poly = runMapper(m, x, y, 0.0, 11.0, 0.0, 3.0, W, H);
            emitPolygon("tiny", m.name, poly);
        }
    }

    // Pattern cases on a 900x500 canvas (bucket path active: n > 2*2W).
    const std::vector< QString > patterns = {
        "baseline", "isolated", "run", "leading", "trailing", "leadtrail",
        "all", "extremum", "periodic", "xynan", "xyinter", "xonly", "inf", "spikenan"
    };
    const int n = 8000;
    for (const QString& kind : patterns) {
        QVector< double > x, y;
        makePattern(kind, n, 2000, x, y);
        if (kind == "extremum" || kind == "spikenan")
            addSpikes(y, { n / 8, 3 * n / 8, 5 * n / 8, 7 * n / 8 }, 2.2);
        if (kind == "extremum") {
            // re-NaN the spike tips AFTER adding spikes so the global max is NaN
            int iMax = -1;
            double best = -DBL_MAX;
            for (int i = 0; i < n; i++) {
                if (std::isfinite(y[ i ]) && y[ i ] > best) {
                    best = y[ i ];
                    iMax = i;
                }
            }
            if (iMax >= 0)
                y[ iMax ] = qQNaN();
        }

        double xs1, xs2, ys1, ys2;
        yScaleFromData(x, y, xs1, xs2, ys1, ys2);
        const QString id = QStringLiteral("pat_%1").arg(kind);
        std::printf("CANVAS %s %d %d %.17g %.17g %.17g %.17g\n", qPrintable(id), 900, 500, xs1, xs2, ys1, ys2);
        std::printf("DATA %s %d", qPrintable(id), n);
        for (int i = 0; i < n; i++) {
            emitDouble(x[ i ]);
            emitDouble(y[ i ]);
        }
        std::printf("\n");
        for (const ModeDef& m : modes()) {
            const QPolygonF poly = runMapper(m, x, y, xs1, xs2, ys1, ys2, 900, 500);
            emitPolygon(qPrintable(id), m.name, poly);
        }
    }
}

// ---------------------------------------------------------------- random campaign

static void runRandom(int count, uint64_t seed)
{
    Rng64 rng(seed);
    const int canvases[ 2 ][ 2 ] = { { 40, 30 }, { 900, 500 } };

    for (int c = 0; c < count; c++) {
        const int n = 3 + (int) (rng.next() % 500);
        const int pY = (int) (rng.next() % 61);   // 0..60 %
        const int pX = (int) (rng.next() % 11);   // 0..10 %
        const bool allNan = (rng.next() % 25) == 0;

        QVector< double > x(n), y(n);
        for (int i = 0; i < n; i++) {
            x[ i ] = i;
            {
            const double t = (double) (i % 173) - 86.0;
            y[ i ] = 1.7 / (1.0 + t * t / 300.0) + (double) (i % 13) * 0.07 - 0.42;
            if (i % 97 == 5)
                y[ i ] += 1.1;
        }
            if (allNan || (int) (rng.next() % 100) < pY)
                y[ i ] = qQNaN();
            if (!allNan && (int) (rng.next() % 100) < pX)
                x[ i ] = qQNaN();
        }

        for (const auto& cv : canvases) {
            const int W = cv[ 0 ], H = cv[ 1 ];
            double xs1, xs2, ys1, ys2;
            yScaleFromData(x, y, xs1, xs2, ys1, ys2);

            std::printf("RD %d %d %d %d", c, W, H, n);
            emitDouble(xs1);
            emitDouble(xs2);
            emitDouble(ys1);
            emitDouble(ys2);
            for (int i = 0; i < n; i++) {
                emitDouble(x[ i ]);
                emitDouble(y[ i ]);
            }
            std::printf("\n");

            for (const ModeDef& m : modes()) {
                const QPolygonF poly = runMapper(m, x, y, xs1, xs2, ys1, ys2, W, H);
                std::printf("RH %d %d %d %s %d %llu\n", c, W, H, m.name, (int) poly.size(),
                            (unsigned long long) hashPolygon(poly));
            }
        }
    }
}

// ---------------------------------------------------------------- visual scan

static void applyModeToCurve(QwtPlotCurve* curve, const QString& mode)
{
    curve->setPaintAttribute(QwtPlotCurve::ClipPolygons, false);
    curve->setPaintAttribute(QwtPlotCurve::FilterPoints, false);
    curve->setPaintAttribute(QwtPlotCurve::FilterPointsAggressive, false);
    curve->setPaintAttribute(QwtPlotCurve::FilterPointsPixel, false);
    curve->setPaintAttribute(QwtPlotCurve::FilterPointsLTTB, false);

    curve->setPaintAttribute(QwtPlotCurve::ClipPolygons, true);
    if (mode == "points")
        curve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    else if (mode == "aggressive")
        curve->setPaintAttribute(QwtPlotCurve::FilterPointsAggressive, true);
    else if (mode == "pixel")
        curve->setPaintAttribute(QwtPlotCurve::FilterPointsPixel, true);
    else if (mode == "lttb")
        curve->setPaintAttribute(QwtPlotCurve::FilterPointsLTTB, true);
    // "none": ClipPolygons only
}

static QImage renderPlot(QwtPlot* plot, int W, int H)
{
    QImage img(W, H, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::white);
    QwtPlotRenderer renderer;
    renderer.renderTo(plot, img);
    return img;
}

static void runVisual(const QString& outdir)
{
    const int imgW = 960, imgH = 560;
    const int n = 20000;

    const std::vector< QString > patterns = {
        "baseline", "isolated", "run", "leading", "trailing", "all",
        "extremum", "periodic", "xynan", "xyinter", "spikenan"
    };
    const std::vector< QString > modeNames = { "none", "points", "aggressive", "pixel", "lttb" };

    QwtPlot plot;
    plot.setCanvasBackground(Qt::white);
    static_cast< QwtPlotCanvas* >(plot.canvas())->setPaintAttribute(QwtPlotCanvas::ImmediatePaint, true);
    plot.setAutoReplot(false);
    plot.resize(imgW, imgH);
    plot.show();
    QApplication::processEvents();

    for (const QString& kind : patterns) {
        QVector< double > x, y;
        makePattern(kind, n, 2000, x, y);
        addSpikes(y, { 2500, 7777, 12345, 16666 }, 2.4);
        y[ 5000 ] -= 3.1;  // deep negative spike
        if (kind == "extremum") {
            // NaN out the true global max/min of the finite signal
            int iMax = -1, iMin = -1;
            double best = -DBL_MAX, worst = DBL_MAX;
            for (int i = 0; i < n; i++) {
                if (!std::isfinite(y[ i ]))
                    continue;
                if (y[ i ] > best) { best = y[ i ]; iMax = i; }
                if (y[ i ] < worst) { worst = y[ i ]; iMin = i; }
            }
            if (iMax >= 0)
                y[ iMax ] = qQNaN();
            if (iMin >= 0)
                y[ iMin ] = qQNaN();
        }

        double xs1, xs2, ys1, ys2;
        yScaleFromData(x, y, xs1, xs2, ys1, ys2);
        const double ym = (ys2 - ys1) * 0.02;
        ys1 -= ym;
        ys2 += ym;

        // --- finite segments for the ideal reference rendering ---
        struct Seg
        {
            int from, to;
        };
        std::vector< Seg > segs;
        int segStart = -1;
        for (int i = 0; i <= n; i++) {
            const bool finite = (i < n) && std::isfinite(x[ i ]) && std::isfinite(y[ i ]);
            if (finite && segStart < 0) {
                segStart = i;
            } else if (!finite && segStart >= 0) {
                segs.push_back(Seg { segStart, i });
                segStart = -1;
            }
        }
        const int idealSegs = (int) segs.size();

        // --- actual: single curve with NaN, one render per mode ---
        QwtPlotCurve* curve = new QwtPlotCurve();
        curve->setSamples(x, y);
        curve->setPen(QPen(QColor("#1f77b4"), 1.5));
        curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
        curve->attach(&plot);
        plot.setAxisScale(QwtAxis::XBottom, xs1, xs2);
        plot.setAxisScale(QwtAxis::YLeft, ys1, ys2);

        for (const QString& mode : modeNames) {
            // ideal reference: one curve per finite segment, SAME mode
            // attributes as the actual curve ("split at NaN, else unchanged")
            QImage idealImg;
            {
                curve->detach();
                std::vector< QwtPlotCurve* > idealCurves;
                for (const Seg& s : segs) {
                    QVector< double > sx(x.begin() + s.from, x.begin() + s.to);
                    QVector< double > sy(y.begin() + s.from, y.begin() + s.to);
                    QwtPlotCurve* c = new QwtPlotCurve();
                    c->setSamples(sx, sy);
                    c->setPen(QPen(QColor("#1f77b4"), 1.5));
                    c->setRenderHint(QwtPlotItem::RenderAntialiased, true);
                    applyModeToCurve(c, mode);
                    c->attach(&plot);
                    idealCurves.push_back(c);
                }
                plot.replot();
                QApplication::processEvents();
                idealImg = renderPlot(&plot, imgW, imgH);
                for (QwtPlotCurve* c : idealCurves) {
                    c->detach();
                    delete c;
                }
                curve->attach(&plot);
            }

            applyModeToCurve(curve, mode);
            plot.replot();
            QApplication::processEvents();
            const QImage actual = renderPlot(&plot, imgW, imgH);

            long long diffPx = 0, maxDev = 0;
            int bbL = imgW, bbT = imgH, bbR = -1, bbB = -1;
            const QRgb* a = reinterpret_cast< const QRgb* >(actual.constBits());
            const QRgb* b = reinterpret_cast< const QRgb* >(idealImg.constBits());
            const int total = imgW * imgH;
            for (int px = 0; px < total; px++) {
                if (a[ px ] == b[ px ])
                    continue;
                diffPx++;
                const int dr = qAbs(qRed(a[ px ]) - qRed(b[ px ]));
                const int dg = qAbs(qGreen(a[ px ]) - qGreen(b[ px ]));
                const int db = qAbs(qBlue(a[ px ]) - qBlue(b[ px ]));
                maxDev = qMax((long long) qMax(dr, qMax(dg, db)), maxDev);
                const int cx = px % imgW, cy = px / imgW;
                bbL = qMin(bbL, cx);
                bbR = qMax(bbR, cx);
                bbT = qMin(bbT, cy);
                bbB = qMax(bbB, cy);
            }
            std::printf("VIS %s %s %d %lld %d %lld %d %d %d %d\n", qPrintable(kind), qPrintable(mode),
                        idealSegs, diffPx, total, maxDev, bbL, bbT, bbR, bbB);
            std::fflush(stdout);

            if (kind == "run" && !outdir.isEmpty()) {
                actual.save(QStringLiteral("%1/nan_vis_run_%2.png").arg(outdir).arg(mode));
                if (mode == "lttb")
                    idealImg.save(QStringLiteral("%1/nan_vis_run_ideal.png").arg(outdir));
            }
            if (kind == "xyinter" && !outdir.isEmpty())
                actual.save(QStringLiteral("%1/nan_vis_xyinter_%2.png").arg(outdir).arg(mode));
        }

        curve->detach();
        delete curve;
    }
}

// ---------------------------------------------------------------- probes

static bool probePainted(const QImage& img, const QPoint& center, int radius)
{
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            const QRgb px = img.pixel(center.x() + dx, center.y() + dy);
            if (qRed(px) < 200 || qGreen(px) < 200 || qBlue(px) < 200)
                return true;  // something darker than white was painted
        }
    }
    return false;
}

static void runProbes()
{
    // --- boundingRect edge cases (task d) ---
    {
        const int n = 100;
        QVector< double > x(n), y(n);
        for (int i = 0; i < n; i++) {
            x[ i ] = i;
            y[ i ] = qQNaN();
        }
        QwtPointArrayData< double > allNan(x, y);
        const QRectF r1 = allNan.boundingRect();
        std::printf("PROBE boundingRect_allNan_valid %d\n", (int) r1.isValid());
        std::printf("PROBE boundingRect_allNan_x %.17g\n", r1.x());
        std::printf("PROBE boundingRect_allNan_width %.17g\n", r1.width());

        QVector< double > ex, ey;
        QwtPointArrayData< double > empty(ex, ey);
        const QRectF r2 = empty.boundingRect();
        std::printf("PROBE boundingRect_empty_valid %d\n", (int) r2.isValid());

        QVector< double > y2(n);
        for (int i = 0; i < n; i++)
            y2[ i ] = (i % 10 == 5) ? qQNaN() : std::sin(i * 0.1);
        QwtPointArrayData< double > mixed(x, y2);
        const QRectF r3 = mixed.boundingRect();
        std::printf("PROBE boundingRect_mixed_finite %d\n",
                    (int) (std::isfinite(r3.left()) && std::isfinite(r3.right())
                           && std::isfinite(r3.top()) && std::isfinite(r3.bottom())));
    }

    // --- autoscale with an all-NaN curve: what do the axes show? ---
    {
        QwtPlot plot;
        plot.setAutoReplot(false);
        const int n = 100;
        QVector< double > x(n), y(n);
        for (int i = 0; i < n; i++) {
            x[ i ] = i;
            y[ i ] = qQNaN();
        }
        QwtPlotCurve* curve = new QwtPlotCurve();
        curve->setSamples(x, y);
        curve->attach(&plot);
        plot.resize(400, 300);
        plot.show();
        plot.replot();
        QApplication::processEvents();
        const QwtScaleDiv xd = plot.axisScaleDiv(QwtAxis::XBottom);
        const QwtScaleDiv yd = plot.axisScaleDiv(QwtAxis::YLeft);
        std::printf("PROBE autoscale_allNan_x_lower %.17g\n", xd.lowerBound());
        std::printf("PROBE autoscale_allNan_x_upper %.17g\n", xd.upperBound());
        std::printf("PROBE autoscale_allNan_y_lower %.17g\n", yd.lowerBound());
        std::printf("PROBE autoscale_allNan_y_upper %.17g\n", yd.upperBound());
        curve->detach();
        delete curve;
    }

    // --- qRound(NaN) on this platform (documents the garbage coordinate) ---
    std::printf("PROBE qround_qt_nan %d\n", qRound(qQNaN()));
    std::printf("PROBE qround_qt_pos_inf %d\n", qRound(std::numeric_limits< double >::infinity()));
    std::printf("PROBE qround_qt_neg_inf %d\n", qRound(-std::numeric_limits< double >::infinity()));

    // --- CurveStyle probes: Steps bridging, Sticks/Dots skipping ---
    // Coordinates come from canvasMap + a frameless canvas grab, so probe
    // pixels are 1:1 with the transformed data coordinates.
    {
        QwtPlot plot;
        plot.setCanvasBackground(Qt::white);
        static_cast< QwtPlotCanvas* >(plot.canvas())->setFrameStyle(QFrame::NoFrame);
        static_cast< QwtPlotCanvas* >(plot.canvas())->setPaintAttribute(QwtPlotCanvas::ImmediatePaint, true);
        plot.setAutoReplot(false);
        plot.resize(480, 320);

        const int n = 8;
        QVector< double > x(n), y(n);
        for (int i = 0; i < n; i++)
            x[ i ] = i;
        y = { 2, 2, 2, qQNaN(), qQNaN(), 3, 3, 3 };

        QwtPlotCurve* curve = new QwtPlotCurve();
        curve->setSamples(x, y);
        curve->setPen(QPen(Qt::black, 1));
        curve->setRenderHint(QwtPlotItem::RenderAntialiased, false);
        curve->attach(&plot);
        plot.setAxisScale(QwtAxis::XBottom, 0, 7);
        plot.setAxisScale(QwtAxis::YLeft, 0, 4);
        plot.show();
        QApplication::processEvents();

        auto canvasPos = [&](double sx, double sy) -> QPoint {
            const QwtScaleMap xm = plot.canvasMap(QwtAxis::XBottom);
            const QwtScaleMap ym = plot.canvasMap(QwtAxis::YLeft);
            return QPoint(qRound(xm.transform(sx)), qRound(ym.transform(sy)));
        };
        auto grabCanvas = [&]() -> QImage {
            plot.replot();
            QApplication::processEvents();
            return plot.canvas()->grab().toImage();
        };

        // Steps (default vertical orientation => "inverted" corner layout):
        // if NaN is skipped instead of splitting, the gap x in (2,5) shows
        //   a) a vertical riser at x=2 from y=2 to y=3 (jump at the PRE-gap x)
        //   b) a horizontal tread at y=3 spanning the whole gap
        // Ideal split semantics would paint neither inside the gap.
        curve->setStyle(QwtPlotCurve::Steps);
        QImage img = grabCanvas();
        const bool stepsTread = probePainted(img, canvasPos(3.5, 3.0), 2);
        const bool stepsRiser = probePainted(img, canvasPos(2.0, 2.5), 2);
        const bool stepsPreLevel = probePainted(img, canvasPos(3.5, 2.0), 2);
        std::printf("PROBE steps_bridge_tread_upper %d\n", (int) stepsTread);
        std::printf("PROBE steps_bridge_riser %d\n", (int) stepsRiser);
        std::printf("PROBE steps_gap_at_pre_level %d\n", (int) stepsPreLevel);

        // Sticks: no stick may be painted at the NaN sample positions.
        curve->setStyle(QwtPlotCurve::Sticks);
        curve->setBaseline(0.0);
        img = grabCanvas();
        const bool stickNanPos = probePainted(img, canvasPos(3.0, 1.0), 1);
        const bool stickValidPos = probePainted(img, canvasPos(2.0, 1.0), 1);
        std::printf("PROBE sticks_at_nan %d\n", (int) stickNanPos);
        std::printf("PROBE sticks_at_valid %d\n", (int) stickValidPos);

        // Dots (MinimizeMemory direct loop): dot at finite, none at NaN.
        curve->setStyle(QwtPlotCurve::Dots);
        curve->setPaintAttribute(QwtPlotCurve::MinimizeMemory, true);
        img = grabCanvas();
        const bool dotNanPos = probePainted(img, canvasPos(3.0, 2.0), 2);
        const bool dotValidPos = probePainted(img, canvasPos(2.0, 2.0), 2);
        std::printf("PROBE dots_minmem_at_nan %d\n", (int) dotNanPos);
        std::printf("PROBE dots_minmem_at_valid %d\n", (int) dotValidPos);

        curve->detach();
        delete curve;
    }

    // --- x-NaN hazards in the MinMax raw-pointer path ---
    {
        const int n = 8000;
        QVector< double > x(n), y(n);
        for (int i = 0; i < n; i++) {
            x[ i ] = i;
            y[ i ] = exactSignal(i, 2000);
        }
        // isolated x-NaN whose y is a local maximum -> it can win a bucket
        const int k = 4000;
        y[ k ] = 10.0;   // make it the bucket extremum for sure
        x[ k ] = qQNaN();

        // y=10.0 (the x-NaN spike) maps to this pixel row on the 900x500 canvas
        const QwtScaleMap yMap = makeMap(-1.5, 10.5, 499.0, 0.0);
        const double sySpike = qRound(yMap.transform(10.0));

        for (const ModeDef& m : modes()) {
            const QPolygonF poly = runMapper(m, x, y, 0.0, (double) (n - 1), -1.5, 10.5, 900, 500);
            long long garbage = 0, fakeLeft = 0;
            for (const QPointF& p : poly) {
                if (!std::isfinite(p.x()) || !std::isfinite(p.y()) || std::fabs(p.x()) > 1e8)
                    garbage++;
                // phantom vertex: the x-NaN spike reappearing at the left border
                if (std::fabs(p.x()) <= 1.0 && std::fabs(p.y() - sySpike) <= 1.0)
                    fakeLeft++;
            }
            std::printf("PROBE xnan_%s_garbage %lld\n", m.name, garbage);
            std::printf("PROBE xnan_%s_fakeLeft %lld\n", m.name, fakeLeft);
            if (std::strcmp(m.name, "lttb") == 0 || std::strcmp(m.name, "lttbgen") == 0)
                emitPolygon("xnan_probe", m.name, poly);
        }
    }

    // --- qwtFindVisibleRange robustness with an isolated x-NaN ---
    // full coverage (axes = data range) and zoomed (axes = 30000..60000):
    // a NaN x must not truncate the mapped output.
    {
        const int n = 100000;
        for (int k : { 12345, 50000, 77777 }) {
            QVector< double > x(n), y(n);
            for (int i = 0; i < n; i++) {
                x[ i ] = i;
                y[ i ] = exactSignal(i, 5000);
            }
            x[ k ] = qQNaN();
            for (const char* modeName : { "pixel", "lttb" }) {
                ModeDef m = (std::strcmp(modeName, "pixel") == 0) ? modes()[ 3 ] : modes()[ 4 ];
                const QPolygonF poly = runMapper(m, x, y, 0.0, (double) (n - 1), -1.5, 1.5, 900, 500);
                double maxX = -DBL_MAX, minX = DBL_MAX;
                for (const QPointF& p : poly) {
                    if (std::isfinite(p.x())) {
                        maxX = qMax(maxX, p.x());
                        minX = qMin(minX, p.x());
                    }
                }
                std::printf("PROBE visrange_x%d_%s_maxx %.4f\n", k, modeName, maxX);

                const QPolygonF polyZ = runMapper(m, x, y, 30000.0, 60000.0, -1.5, 1.5, 900, 500);
                double maxXz = -DBL_MAX, minXz = DBL_MAX;
                for (const QPointF& p : polyZ) {
                    if (std::isfinite(p.x())) {
                        maxXz = qMax(maxXz, p.x());
                        minXz = qMin(minXz, p.x());
                    }
                }
                std::printf("PROBE visrangezoom_x%d_%s_minmax %.4f %.4f\n", k, modeName, minXz, maxXz);
                (void) minX;
            }
        }
    }
}

// ---------------------------------------------------------------- main

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    const QString cmd = argc > 1 ? argv[ 1 ] : "matrix";
    if (cmd == "matrix") {
        runMatrix();
    } else if (cmd == "random") {
        const int count = argc > 2 ? std::atoi(argv[ 2 ]) : 250;
        const uint64_t seed = argc > 3 ? std::strtoull(argv[ 3 ], nullptr, 10) : 20261012ULL;
        runRandom(count, seed);
    } else if (cmd == "visual") {
        const QString outdir = argc > 2 ? argv[ 2 ] : QString();
        runVisual(outdir);
    } else if (cmd == "probes") {
        runProbes();
    } else {
        std::fprintf(stderr, "unknown subcommand: %s\n", qPrintable(cmd));
        return 1;
    }
    return 0;
}
