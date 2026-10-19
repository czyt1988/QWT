// RoundPoints microbenchmark for docs/tech-report/point-mapper-rounding.md.
//
// Measures QwtPointMapper::toPolygonF() with and without the RoundPoints
// flag for every reduction path of the paint pipeline:
//
//   none        no flags                       (plain transform, 1:1 output)
//   weedout     WeedOutPoints                  (dedupe mapped duplicates)
//   aggressive  WeedOutIntermediatePoints      (only runs with RoundPoints)
//   pixel       PixelColumnReduce              (min/max per pixel column)
//   lttb        LTTBReduce                     (true LTTB, 4W buckets)
//   minmax      MinMaxReduce                   (min/max per index bucket)
//   adaptive    AdaptiveColumnReduce           (density gate + ordered select)
//
// The benchmark times the mapper only, not QPainter::drawPolyline(), and
// reports the number of output vertices, so that the cost of rounding can
// be separated from the cost of drawing fewer points.
//
// Timing protocol: 3 warm-up calls, then 9 measured calls; the median of
// the 9 calls is reported together with the minimum. A call is repeated
// until it takes at least ~20 ms (calibrated once per configuration), so
// timer resolution and scheduling noise stay small.
//
// Build (MSVC 2019 x64, Qt 6.7.3, qwt built in <repo>/build):
//
//   cl /nologo /O2 /EHsc /std:c++17 /MD /permissive- ^
//      /I<repo>/src /I<repo>/src/core /I<repo>/src/plot ^
//      /I<qt>/include /I<qt>/include/QtCore /I<qt>/include/QtGui ^
//      /I<qt>/include/QtWidgets /I<qt>/include/QtConcurrent ^
//      /I<qt>/include/QtPrintSupport ^
//      roundpoints_bench.cpp /Fe:roundpoints_bench.exe ^
//      /link /LIBPATH:<repo>/build/lib/Release /LIBPATH:<qt>/lib ^
//      qwtplot.lib qwtcore.lib Qt6Widgets.lib Qt6Gui.lib Qt6Core.lib ^
//      Qt6PrintSupport.lib Qt6Concurrent.lib opengl32.lib
//
// Run with PATH containing <qt>/bin and <repo>/build/bin/Release.
// Output (stdout) is the CSV consumed by roundpoints_experiment.py:
//
//   meta,<key>,<value>
//   bench,<shape>,<algo>,<round>,<dpr>,<points>,<outPoints>,<minMs>,<medMs>,<calls>
//   geom,<shape>,<algo>,<round>,<dpr>,<outPoints>,<realSegs>,<horizSegs>,<vertSegs>,<horizPct>,<vertPct>,<axisPct>
//   sweep,<points>,<round>,<dpr>,<outPoints>,<minMs>,<medMs>,<calls>
//
// Usage: roundpoints_bench.exe [bench|sweep|all]   (default: all)
//        add "-platform offscreen" for headless runs.

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QPolygonF>
#include <QVector>
#include <QtGlobal>

#include <qwt_point_data.h>
#include <qwt_point_mapper.h>
#include <qwt_scale_map.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace
{

const int kCanvasWidth  = 1200;
const int kCanvasHeight = 800;

enum AlgoId
{
    AlgoNone = 0,
    AlgoWeedOut,
    AlgoAggressive,
    AlgoPixel,
    AlgoLttb,
    AlgoMinMax,
    AlgoAdaptive,
    AlgoCount
};

struct AlgoInfo
{
    const char* key;
    QwtPointMapper::TransformationFlags flags;
};

// Every reduction path of toPolygonF(), one flag set at a time.
const AlgoInfo kAlgos[ AlgoCount ] = {
    { "none", QwtPointMapper::TransformationFlags() },
    { "weedout", QwtPointMapper::WeedOutPoints },
    { "aggressive", QwtPointMapper::WeedOutPoints | QwtPointMapper::WeedOutIntermediatePoints },
    { "pixel", QwtPointMapper::PixelColumnReduce },
    { "lttb", QwtPointMapper::LTTBReduce },
    { "minmax", QwtPointMapper::MinMaxReduce },
    { "adaptive", QwtPointMapper::AdaptiveColumnReduce },
};

// Two signal shapes:
//
//   smooth  two incommensurate sines whose periods are far longer than the
//           canvas ( ~750 and ~440 pixel columns for 1M samples over 1200
//           columns ) - the curve advances only a few pixel rows per column,
//           so every reduction path can compress it heavily
//   rough   the same plus a high-frequency term whose period is shorter than
//           one pixel column - the worst case for the vertical-tolerance
//           selection, because a single column genuinely spans many rows
void makeSignal(int count, bool rough, QVector< double >& x, QVector< double >& y)
{
    x.resize(count);
    y.resize(count);

    for (int i = 0; i < count; i++) {
        x[ i ] = i;
        y[ i ] = std::sin(i * 1.0e-5) + 0.35 * std::sin(i * 1.7e-5);
        if (rough)
            y[ i ] += 0.05 * std::sin(i * 0.21);
    }
}

double median(std::vector< double >& v)
{
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    return (n % 2) ? v[ n / 2 ] : 0.5 * (v[ n / 2 - 1 ] + v[ n / 2 ]);
}

struct Measurement
{
    int outPoints = 0;
    double minMs  = 0.0;
    double medMs  = 0.0;
    int calls     = 1;
};

// Time one configuration. `calls` is calibrated so that a single timed run
// lasts at least ~20 ms; the reported value is the per-call time.
Measurement measure(const QwtScaleMap& xMap, const QwtScaleMap& yMap,
                    const QwtPointArrayData< double >& data,
                    QwtPointMapper::TransformationFlags flags, bool roundPoints, qreal dpr)
{
    QwtPointMapper mapper;
    mapper.setDevicePixelRatio(dpr);
    mapper.setFlags(flags);
    mapper.setFlag(QwtPointMapper::RoundPoints, roundPoints);

    const int count = static_cast< int >(data.size());

    auto call = [ & ]() -> int
    {
        const QPolygonF polygon = mapper.toPolygonF(xMap, yMap, &data, 0, count - 1);
        return polygon.size();
    };

    Measurement m;

    // calibration: grow the repetition count until one run takes >= 20 ms
    m.calls = 1;
    for (;;) {
        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < m.calls; i++)
            m.outPoints = call();
        const double elapsed = timer.nsecsElapsed() / 1e6;
        if (elapsed >= 20.0 || m.calls >= 1024)
            break;
        m.calls *= 2;
    }

    for (int i = 0; i < 3; i++)
        call();

    std::vector< double > times;
    times.reserve(9);

    for (int r = 0; r < 9; r++) {
        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < m.calls; i++)
            m.outPoints = call();
        times.push_back(timer.nsecsElapsed() / 1e6 / m.calls);
    }

    m.minMs = *std::min_element(times.begin(), times.end());
    m.medMs = median(times);
    return m;
}

void printMeta(const char* key, const char* value)
{
    std::printf("meta,%s,%s\n", key, value);
}

void runBench(int pointCount)
{
    for (int shape = 0; shape < 2; shape++) {
        const bool rough = (shape == 1);

        QVector< double > x, y;
        makeSignal(pointCount, rough, x, y);
        const QwtPointArrayData< double > data(x, y);

        double yMin = y[ 0 ], yMax = y[ 0 ];
        for (int i = 1; i < pointCount; i++) {
            yMin = qMin(yMin, y[ i ]);
            yMax = qMax(yMax, y[ i ]);
        }

        QwtScaleMap xMap;
        xMap.setScaleInterval(0.0, pointCount - 1.0);
        xMap.setPaintInterval(0.0, kCanvasWidth);

        QwtScaleMap yMap;
        yMap.setScaleInterval(yMin, yMax);
        yMap.setPaintInterval(kCanvasHeight, 0.0);

        char buf[ 256 ];

        std::snprintf(buf, sizeof(buf), "%d", pointCount);
        printMeta("points", buf);
        std::snprintf(buf, sizeof(buf), "%dx%d", kCanvasWidth, kCanvasHeight);
        printMeta("canvas", buf);
        printMeta("signal", rough ? "smooth + 0.05 sin(0.21 i)" : "smooth (two long-period sines)");

        for (int dprIndex = 0; dprIndex < 2; dprIndex++) {
            const qreal dpr = dprIndex == 0 ? 1.0 : 2.0;

            for (int a = 0; a < AlgoCount; a++) {
                for (int r = 0; r < 2; r++) {
                    const bool roundPoints = (r == 1);
                    const Measurement m = measure(xMap, yMap, data, kAlgos[ a ].flags, roundPoints, dpr);

                    std::printf("bench,%s,%s,%d,%.1f,%d,%d,%.4f,%.4f,%d\n",
                                rough ? "rough" : "smooth", kAlgos[ a ].key, roundPoints ? 1 : 0,
                                dpr, pointCount, m.outPoints, m.minMs, m.medMs, m.calls);
                    std::fflush(stdout);
                }
            }
        }
    }
}

// Geometry of the polyline the mapper returns: how many segments are exactly
// horizontal or exactly vertical in device pixels. Snapping a dense curve to
// the pixel grid collapses its diagonal geometry into axis-aligned steps,
// so this fraction is the direct geometric signature of the staircase.
void runGeometry(int pointCount)
{
    for (int shape = 0; shape < 2; shape++) {
        const bool rough = (shape == 1);

        QVector< double > x, y;
        makeSignal(pointCount, rough, x, y);
        const QwtPointArrayData< double > data(x, y);

        double yMin = y[ 0 ], yMax = y[ 0 ];
        for (int i = 1; i < pointCount; i++) {
            yMin = qMin(yMin, y[ i ]);
            yMax = qMax(yMax, y[ i ]);
        }

        QwtScaleMap xMap;
        xMap.setScaleInterval(0.0, pointCount - 1.0);
        xMap.setPaintInterval(0.0, kCanvasWidth);

        QwtScaleMap yMap;
        yMap.setScaleInterval(yMin, yMax);
        yMap.setPaintInterval(kCanvasHeight, 0.0);

        for (int a = 0; a < AlgoCount; a++) {
            for (int r = 0; r < 2; r++) {
                const bool roundPoints = (r == 1);

                QwtPointMapper mapper;
                mapper.setDevicePixelRatio(1.0);
                mapper.setFlags(kAlgos[ a ].flags);
                mapper.setFlag(QwtPointMapper::RoundPoints, roundPoints);

                const QPolygonF polygon = mapper.toPolygonF(xMap, yMap, &data, 0, pointCount - 1);
                const int n             = polygon.size();

                int horiz = 0, vert = 0, real = 0;
                for (int i = 1; i < n; i++) {
                    const QPointF a0 = polygon[ i - 1 ];
                    const QPointF b0 = polygon[ i ];
                    // exact comparison: with RoundPoints the coordinates are
                    // integral, so an axis-aligned segment has dx == 0.0 or
                    // dy == 0.0 exactly; without it only true duplicates do.
                    const double dx = b0.x() - a0.x();
                    const double dy = b0.y() - a0.y();

                    if (dx == 0.0 && dy == 0.0)
                        continue;  // duplicate vertex, not a real segment
                    real++;
                    if (dy == 0.0)
                        horiz++;
                    if (dx == 0.0)
                        vert++;
                }

                std::printf("geom,%s,%s,%d,1.0,%d,%d,%d,%d,%.4f,%.4f,%.4f\n",
                            rough ? "rough" : "smooth", kAlgos[ a ].key, roundPoints ? 1 : 0,
                            n, real, horiz, vert,
                            real ? double(horiz) / real : 0.0,
                            real ? double(vert) / real : 0.0,
                            real ? double(horiz + vert) / real : 0.0);
                std::fflush(stdout);
            }
        }
    }
}

void runSweep()
{
    static const int sizes[] = { 100000, 250000, 500000, 1000000, 2000000 };
    const int sizeCount      = int(sizeof(sizes) / sizeof(sizes[0]));

    for (int s = 0; s < sizeCount; s++) {
        const int pointCount = sizes[ s ];

        QVector< double > x, y;
        makeSignal(pointCount, false, x, y);
        const QwtPointArrayData< double > data(x, y);

        double yMin = y[ 0 ], yMax = y[ 0 ];
        for (int i = 1; i < pointCount; i++) {
            yMin = qMin(yMin, y[ i ]);
            yMax = qMax(yMax, y[ i ]);
        }

        QwtScaleMap xMap;
        xMap.setScaleInterval(0.0, pointCount - 1.0);
        xMap.setPaintInterval(0.0, kCanvasWidth);

        QwtScaleMap yMap;
        yMap.setScaleInterval(yMin, yMax);
        yMap.setPaintInterval(kCanvasHeight, 0.0);

        for (int r = 0; r < 2; r++) {
            const bool roundPoints = (r == 1);
            const Measurement m     = measure(xMap, yMap, data, QwtPointMapper::TransformationFlags(),
                                             roundPoints, 1.0);

            std::printf("sweep,%d,%d,1.0,%d,%.4f,%.4f,%d\n",
                        pointCount, roundPoints ? 1 : 0, m.outPoints, m.minMs, m.medMs, m.calls);
            std::fflush(stdout);
        }
    }
}

}  // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    bool doBench = true;
    bool doSweep = true;
    bool doGeom  = true;

    if (argc > 1) {
        const char* mode = argv[ 1 ];
        doBench          = (std::strcmp(mode, "sweep") != 0) && (std::strcmp(mode, "geom") != 0);
        doSweep          = (std::strcmp(mode, "bench") != 0) && (std::strcmp(mode, "geom") != 0);
        doGeom           = (std::strcmp(mode, "bench") != 0) && (std::strcmp(mode, "sweep") != 0);
    }

    printMeta("qwt", QWT_VERSION_STR);
    printMeta("qt", QT_VERSION_STR);

    if (doBench)
        runBench(1000000);
    if (doGeom)
        runGeometry(1000000);
    if (doSweep)
        runSweep();

    return 0;
}
