// NaN rendering performance benchmark for docs/zh/tech-report/nan-rendering.md.
//
// Mirrors the measurement decomposition of examples/2D/nanperf
// (BenchmarkRunner::measureOn): cold-cache boundingRect() scan and pure
// replot() render cost, with fixed axis scales so replot never autoscales.
// Data generation mirrors examples/2D/nanperf/NanDataGenerator.cpp exactly
// (same 8 NaN distribution cases, same sin-based signal).
//
// Timing protocol (task standard):
//   boundingRect : 5 x (setSamples untimed + timed cold scan), report min
//   replot       : 3 warmup + 7 timed frames -> min & median; if
//                  (median - min) / min > 0.30, up to 3 extra rounds of 7
//                  frames are measured and pooled (min/median over all).
//
// Sweeps:
//   A (main)  : case=Middle, points in {100k, 1M}, nanPct in {0,1,5,10,25,50},
//               5 modes
//   B (cases) : points=100k, nanPct=10, all 8 NanCase distributions, 5 modes
//   C (edges) : all-NaN and empty-data degenerate configs
//
// Output (stdout): CSV records
//   perf,<sweep>,<case>,<mode>,<points>,<nanPct>,<nanPoints>,<boundingRectMs>,<replotMinMs>,<replotMedMs>,<fpsMin>,<rounds>
//
// Usage: nan_bench.exe [sweepA|sweepB|sweepC|all]   (default: all)
//        add "-platform offscreen" for headless runs
//
// Build: see nan_mapper_check.cpp header (same recipe, same libraries).

#include <QApplication>
#include <QElapsedTimer>
#include <QVector>
#include <QtGlobal>
#include <QtMath>

#include <qwt_axis.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// ------------------------------------------------- NanDataGenerator mirror

enum class NanCase
{
    Leading, LeadingTrailing, Middle, Trailing, XyNan, XyInterleavedNan, AllNan, Baseline
};

static double signalValue(int i)
{
    const double t1 = 2.0 * M_PI * i / 2000.0;
    const double t2 = 2.0 * M_PI * i / 200.0;
    return std::sin(t1) + 0.3 * std::sin(t2);
}

static int nanCount(NanCase cs, int numPoints, double nanFraction)
{
    const int nanN = static_cast< int >(nanFraction * numPoints);
    switch (cs) {
    case NanCase::LeadingTrailing:
        return 2 * (nanN / 2);
    case NanCase::AllNan:
        return numPoints;
    case NanCase::Baseline:
        return 0;
    default:
        return nanN;
    }
}

static void generate(NanCase cs, int numPoints, double nanFraction, QVector< double >& x, QVector< double >& y)
{
    x.resize(numPoints);
    y.resize(numPoints);
    const int nanN     = static_cast< int >(nanFraction * numPoints);
    const int half     = nanN / 2;
    const int midStart = (numPoints - nanN) / 2;
    const double nan   = qQNaN();

    for (int i = 0; i < numPoints; ++i) {
        x[ i ] = static_cast< double >(i);
        bool isNan = false, xNan = false, yNan = false;
        switch (cs) {
        case NanCase::Leading:
            isNan = (i < nanN);
            yNan  = isNan;
            break;
        case NanCase::LeadingTrailing:
            isNan = (i < half) || (i >= numPoints - half);
            yNan  = isNan;
            break;
        case NanCase::Middle:
        case NanCase::XyNan:
            isNan = (i >= midStart) && (i < midStart + nanN);
            yNan  = isNan;
            if (cs == NanCase::XyNan && isNan)
                xNan = true;
            break;
        case NanCase::XyInterleavedNan: {
            const bool inRange = (i >= midStart) && (i < midStart + nanN);
            if (inRange) {
                if ((i - midStart) % 2 == 0)
                    xNan = true;
                else
                    yNan = true;
            }
            isNan = inRange;
            break;
        }
        case NanCase::Trailing:
            isNan = (i >= numPoints - nanN);
            yNan  = isNan;
            break;
        case NanCase::AllNan:
            yNan = true;
            break;
        case NanCase::Baseline:
            isNan = false;
            break;
        }
        Q_UNUSED(isNan);
        y[ i ] = yNan ? nan : signalValue(i);
        if (xNan)
            x[ i ] = nan;
    }
}

static const char* caseLabel(NanCase cs)
{
    switch (cs) {
    case NanCase::Leading: return "leading";
    case NanCase::LeadingTrailing: return "leadtrail";
    case NanCase::Middle: return "middle";
    case NanCase::Trailing: return "trailing";
    case NanCase::XyNan: return "xynan";
    case NanCase::XyInterleavedNan: return "xyinter";
    case NanCase::AllNan: return "allnan";
    case NanCase::Baseline: return "baseline";
    }
    return "?";
}

// ------------------------------------------------------------------- modes

struct Mode
{
    const char* name;
    int attr;      // QwtPlotCurve::PaintAttribute to enable (-1 = none)
};

static const std::vector< Mode >& benchModes()
{
    static const std::vector< Mode > m = {
        { "none", -1 },
        { "points", QwtPlotCurve::FilterPoints },
        { "aggressive", QwtPlotCurve::FilterPointsAggressive },
        { "pixel", QwtPlotCurve::FilterPointsPixel },
        { "lttb", QwtPlotCurve::FilterPointsLTTB },
    };
    return m;
}

static void applyMode(QwtPlotCurve* curve, const Mode& mode)
{
    curve->setPaintAttribute(QwtPlotCurve::ClipPolygons, false);
    curve->setPaintAttribute(QwtPlotCurve::FilterPoints, false);
    curve->setPaintAttribute(QwtPlotCurve::FilterPointsAggressive, false);
    curve->setPaintAttribute(QwtPlotCurve::FilterPointsPixel, false);
    curve->setPaintAttribute(QwtPlotCurve::FilterPointsLTTB, false);
    curve->setPaintAttribute(QwtPlotCurve::ClipPolygons, true);
    if (mode.attr >= 0)
        curve->setPaintAttribute(static_cast< QwtPlotCurve::PaintAttribute >(mode.attr), true);
}

// ------------------------------------------------------------------ timing

struct Measure
{
    double boundingRectMs = 0.0;
    double replotMinMs    = 0.0;
    double replotMedMs    = 0.0;
    int rounds            = 1;
};

static double medianOf(std::vector< double > v)
{
    if (v.empty())
        return 0.0;
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    return (n % 2) ? v[ n / 2 ] : 0.5 * (v[ n / 2 - 1 ] + v[ n / 2 ]);
}

static Measure measure(QwtPlot* plot, QwtPlotCurve* curve, const QVector< double >& x, const QVector< double >& y)
{
    Measure m;
    QElapsedTimer timer;

    // cold-cache boundingRect: min of 5 (setSamples invalidates the cache)
    double best = 1e30;
    for (int r = 0; r < 5; ++r) {
        curve->setSamples(x, y);
        timer.start();
        const QRectF rect = curve->data()->boundingRect();
        const double ms   = timer.nsecsElapsed() / 1.0e6;
        (void) rect;
        best = qMin(best, ms);
    }
    m.boundingRectMs = best;

    // warm the cache so replot measures pure render/mapping cost
    curve->setSamples(x, y);
    curve->data()->boundingRect();

    std::vector< double > frames;
    for (int round = 0; round < 4; ++round) {
        const int warmup = (round == 0) ? 3 : 0;
        for (int f = 0; f < warmup; ++f)
            plot->replot();
        std::vector< double > batch;
        for (int f = 0; f < 7; ++f) {
            timer.start();
            plot->replot();
            batch.push_back(timer.nsecsElapsed() / 1.0e6);
        }
        frames.insert(frames.end(), batch.begin(), batch.end());
        const double bmin = *std::min_element(batch.begin(), batch.end());
        const double bmed = medianOf(batch);
        m.rounds          = round + 1;
        if (bmin > 0.0 && (bmed - bmin) / bmin <= 0.30)
            break;  // stable batch, protocol satisfied
    }
    m.replotMinMs = *std::min_element(frames.begin(), frames.end());
    m.replotMedMs = medianOf(frames);
    return m;
}

static void emitRow(const char* sweep, const char* caseName, const char* modeName, int points, double nanPct,
                    int nanPoints, const Measure& m)
{
    const double fps = m.replotMinMs > 0.0 ? 1000.0 / m.replotMinMs : 0.0;
    std::printf("perf,%s,%s,%s,%d,%.2f,%d,%.4f,%.4f,%.4f,%.2f,%d\n", sweep, caseName, modeName, points,
                nanPct * 100.0, nanPoints, m.boundingRectMs, m.replotMinMs, m.replotMedMs, fps, m.rounds);
    std::fflush(stdout);
}

// -------------------------------------------------------------------- main

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    const QString sweep = argc > 1 && argv[ 1 ][ 0 ] != '-' ? argv[ 1 ] : "all";

    QwtPlot plot;
    plot.setCanvasBackground(Qt::white);
    QwtPlotCanvas* canvas = static_cast< QwtPlotCanvas* >(plot.canvas());
    canvas->setPaintAttribute(QwtPlotCanvas::ImmediatePaint, true);
    plot.setAutoReplot(false);

    QwtPlotCurve* curve = new QwtPlotCurve("bench");
    curve->setPen(QColor("#1f77b4"), 1.5);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curve->attach(&plot);

    // size the widget so the canvas is exactly 900x500
    plot.resize(900, 500);
    plot.show();
    QApplication::processEvents();
    for (int i = 0; i < 3; ++i) {
        const QSize cs = canvas->size();
        if (cs.width() == 900 && cs.height() == 500)
            break;
        plot.resize(plot.width() + (900 - cs.width()), plot.height() + (500 - cs.height()));
        QApplication::processEvents();
    }
    std::printf("# canvas %dx%d (widget %dx%d)\n", canvas->width(), canvas->height(), plot.width(), plot.height());
    std::printf("# protocol: boundingRect min-of-5 cold; replot warmup3+min/median-of-7, extra rounds while (med-min)/min>0.30 (max 4)\n");

    auto runConfig = [&](const char* sweepName, NanCase cs, int points, double nanPct) {
        QVector< double > x, y;
        generate(cs, points, nanPct, x, y);
        const int nNan = nanCount(cs, points, nanPct);
        plot.setAxisScale(QwtAxis::XBottom, 0.0, qMax(1, points - 1));
        plot.setAxisScale(QwtAxis::YLeft, -1.5, 1.5);
        for (const Mode& mode : benchModes()) {
            applyMode(curve, mode);
            const Measure m = measure(&plot, curve, x, y);
            emitRow(sweepName, caseLabel(cs), mode.name, points, nanPct, nNan, m);
        }
    };

    if (sweep == "sweepA" || sweep == "all") {
        for (int points : { 100000, 1000000 })
            for (double nanPct : { 0.0, 0.01, 0.05, 0.10, 0.25, 0.50 })
                runConfig("A", NanCase::Middle, points, nanPct);
    }

    if (sweep == "sweepB" || sweep == "all") {
        const std::vector< NanCase > cases = {
            NanCase::Baseline, NanCase::Leading, NanCase::LeadingTrailing, NanCase::Middle,
            NanCase::Trailing, NanCase::XyNan, NanCase::XyInterleavedNan, NanCase::AllNan
        };
        for (NanCase cs : cases)
            runConfig("B", cs, 100000, 0.10);
    }

    if (sweep == "single") {
        // re-measurement of individual configs with a stricter protocol:
        //   nan_bench.exe single <points> <nanPct%> [case]
        const int points = argc > 2 ? std::atoi(argv[ 2 ]) : 100000;
        const double nanPct = argc > 3 ? std::atof(argv[ 3 ]) / 100.0 : 0.0;
        NanCase cs = NanCase::Middle;
        if (argc > 4 && std::string(argv[ 4 ]) != "-platform") {
            const std::string c = argv[ 4 ];
            if (c == "baseline") cs = NanCase::Baseline;
            else if (c == "leading") cs = NanCase::Leading;
            else if (c == "leadtrail") cs = NanCase::LeadingTrailing;
            else if (c == "trailing") cs = NanCase::Trailing;
            else if (c == "xynan") cs = NanCase::XyNan;
            else if (c == "xyinter") cs = NanCase::XyInterleavedNan;
            else if (c == "allnan") cs = NanCase::AllNan;
        }
        QVector< double > x, y;
        generate(cs, points, nanPct, x, y);
        plot.setAxisScale(QwtAxis::XBottom, 0.0, qMax(1, points - 1));
        plot.setAxisScale(QwtAxis::YLeft, -1.5, 1.5);
        for (const Mode& mode : benchModes()) {
            applyMode(curve, mode);
            // stricter: up to 6 rounds, stop after two consecutive stable batches
            Measure m;
            QElapsedTimer timer;
            double best = 1e30;
            for (int r = 0; r < 5; ++r) {
                curve->setSamples(x, y);
                timer.start();
                const QRectF rect = curve->data()->boundingRect();
                best = qMin(best, timer.nsecsElapsed() / 1.0e6);
                (void) rect;
            }
            m.boundingRectMs = best;
            curve->setSamples(x, y);
            curve->data()->boundingRect();
            std::vector< double > frames;
            int stable = 0;
            for (int round = 0; round < 6; ++round) {
                if (round == 0)
                    for (int f = 0; f < 3; ++f)
                        plot.replot();
                std::vector< double > batch;
                for (int f = 0; f < 7; ++f) {
                    timer.start();
                    plot.replot();
                    batch.push_back(timer.nsecsElapsed() / 1.0e6);
                }
                frames.insert(frames.end(), batch.begin(), batch.end());
                m.rounds = round + 1;
                const double bmin = *std::min_element(batch.begin(), batch.end());
                const double bmed = medianOf(batch);
                if (bmin > 0.0 && (bmed - bmin) / bmin <= 0.15)
                    stable++;
                else
                    stable = 0;
                if (stable >= 2)
                    break;
            }
            m.replotMinMs = *std::min_element(frames.begin(), frames.end());
            m.replotMedMs = medianOf(frames);
            emitRow("S", caseLabel(cs), mode.name, points, nanPct, nanCount(cs, points, nanPct), m);
        }
    }

    if (sweep == "sweepC" || sweep == "all") {
        // all-NaN and empty data degenerates at 100k
        runConfig("C", NanCase::AllNan, 100000, 1.0);
        {
            QVector< double > ex, ey;
            plot.setAxisScale(QwtAxis::XBottom, 0.0, 1.0);
            plot.setAxisScale(QwtAxis::YLeft, -1.5, 1.5);
            for (const Mode& mode : benchModes()) {
                applyMode(curve, mode);
                const Measure m = measure(&plot, curve, ex, ey);
                emitRow("C", "empty", mode.name, 0, 0.0, 0, m);
            }
        }
    }

    return 0;
}
