/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *
 * Regression tests for the NaN/Inf handling of the curve rendering
 * pipeline ( see docs/zh/tech-report/nan-rendering.md ):
 *
 *  - mapper level: the MinMax raw-pointer paths must skip samples with
 *    non-finite x or y like the generic path does ( no phantom vertices,
 *    no INT_MIN pixels ), all-NaN input must map to an empty polygon.
 *  - curve level: the default skip semantics bridges gaps with a chord,
 *    while the BreakAtNan attribute renders gap semantics.
 *****************************************************************************/

#include <QTest>
#include <QApplication>
#include <QImage>
#include <QPen>
#include <QBrush>
#include <QtMath>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_point_mapper.h>
#include <qwt_scale_map.h>
#include <qwt_point_data.h>
#include <qwt_series_data.h>

#include <cmath>
#include <limits>

namespace
{
/*
    A series type that is unknown to qwtTryGetRawPointData, so the mapper
    always takes the generic sample()-based path when it is fed with it.
 */
class VirtualSampleSeries : public QwtSeriesData< QPointF >
{
public:
    VirtualSampleSeries(const QVector< double >& x, const QVector< double >& y)
        : m_x(x)
        , m_y(y)
    {
    }

    virtual size_t size() const override
    {
        return static_cast< size_t >(m_x.size());
    }

    virtual QPointF sample(size_t index) const override
    {
        const int i = static_cast< int >(index);
        return QPointF(m_x[ i ], m_y[ i ]);
    }

    virtual QRectF boundingRect() const override
    {
        // union of the finite samples only, like qwtBoundingRectT
        QRectF br(1.0, 1.0, -2.0, -2.0);
        bool first = true;
        for (int i = 0; i < m_x.size(); i++) {
            if (!std::isfinite(m_x[ i ]) || !std::isfinite(m_y[ i ]))
                continue;

            if (first) {
                br    = QRectF(m_x[ i ], m_y[ i ], 0.0, 0.0);
                first = false;
            } else {
                br.setLeft(qMin(br.left(), m_x[ i ]));
                br.setRight(qMax(br.right(), m_x[ i ]));
                br.setTop(qMin(br.top(), m_y[ i ]));
                br.setBottom(qMax(br.bottom(), m_y[ i ]));
            }
        }
        return br;
    }

private:
    QVector< double > m_x;
    QVector< double > m_y;
};

// Canvas image plus the scale maps used for it, so that data
// coordinates can be probed after the plot itself is gone
struct CanvasProbe
{
    QImage image;
    QwtScaleMap xMap;
    QwtScaleMap yMap;

    QPoint pos(double sx, double sy) const
    {
        return QPoint(qRound(xMap.transform(sx)), qRound(yMap.transform(sy)));
    }
};

// true, when any pixel within radius around center is darker than white
bool probePainted(const QImage& img, const QPoint& center, int radius)
{
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            const int px = center.x() + dx;
            const int py = center.y() + dy;
            if (px < 0 || py < 0 || px >= img.width() || py >= img.height())
                continue;

            const QRgb rgb = img.pixel(px, py);
            if (qRed(rgb) < 200 || qGreen(rgb) < 200 || qBlue(rgb) < 200)
                return true;
        }
    }
    return false;
}

// true, when any pixel within radius around center matches the color
bool probeIsColor(const QImage& img, const QPoint& center, int radius, QRgb color)
{
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            const int px = center.x() + dx;
            const int py = center.y() + dy;
            if (px < 0 || py < 0 || px >= img.width() || py >= img.height())
                continue;

            if (img.pixel(px, py) == color)
                return true;
        }
    }
    return false;
}

/*
    Plateau-gap-plateau signal: high plateau, a wide NaN gap, low plateau.
    The gap is wide and the level difference is large, so a bridging chord
    crosses the gap center while gap semantics leaves it blank.
 */
void makeGapData(QVector< double >& x, QVector< double >& y, int n, int gapFrom, int gapTo)
{
    x.resize(n);
    y.resize(n);
    for (int i = 0; i < n; i++) {
        x[ i ] = i;
        y[ i ] = (i < gapFrom) ? 0.8 : ((i <= gapTo) ? qQNaN() : 0.2);
    }
}

// render the canvas of a freshly set up plot with the given curve config
CanvasProbe renderCanvas(QwtPlotCurve::CurveStyle style,
                         const QVector< double >& x,
                         const QVector< double >& y,
                         bool breakAtNan,
                         const QBrush& brush = QBrush())
{
    QwtPlot plot;
    plot.setCanvasBackground(Qt::white);
    plot.setAutoReplot(false);
    plot.resize(640, 480);

    QwtPlotCurve* curve = new QwtPlotCurve();
    curve->setSamples(x, y);
    curve->setPen(QPen(Qt::black, 2));
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, false);
    curve->setStyle(style);
    if (brush.style() != Qt::NoBrush)
        curve->setBrush(brush);
    curve->setCurveAttribute(QwtPlotCurve::BreakAtNan, breakAtNan);
    curve->attach(&plot);

    plot.setAxisScale(QwtAxis::XBottom, 0, x.size() - 1);
    plot.setAxisScale(QwtAxis::YLeft, 0.0, 1.0);

    plot.show();
    QApplication::processEvents();
    plot.replot();

    CanvasProbe probe;
    probe.image = plot.canvas()->grab().toImage();
    probe.xMap = plot.canvasMap(QwtAxis::XBottom);
    probe.yMap = plot.canvasMap(QwtAxis::YLeft);

    return probe;
}
}

class TestNanCurve : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // mapper level
    void lttb_xNan_noPhantomVertex();
    void lttb_inf_noGarbageCoordinates();
    void filterPoints_allNan_emptyPolygon();
    void filterPoints_leadingNan_singleFinite();
    void bridgeSemantics_anchor();
    // curve level
    void allNan_fill_noCrash();
    void breakAtNan_gapBlank();
    void default_bridgePresent();
    void steps_breakAtNan();
    void fill_breakAtNan();
    void breakAtNan_cleanData_identical();
    void breakAtNan_allNan_noCrash();
};

/*
    x contains NaN where y carries the bucket maximum. When the MinMax
    pre-scan misses the x-NaN samples, one of them wins a bucket extremum
    and its screen x becomes qRound(NaN) == 0: a phantom vertex pinned to
    the left canvas edge. After the fix the raw-pointer path and the
    generic path must agree bit for bit.
 */
void TestNanCurve::lttb_xNan_noPhantomVertex()
{
    const int n = 8000;
    QVector< double > x(n), y(n);
    for (int i = 0; i < n; i++) {
        x[ i ] = i;
        y[ i ] = std::sin(i * 0.01) * 0.8;
    }
    for (int i = 100; i < 130; i++) {
        x[ i ] = qQNaN();
        y[ i ] = 5.0; // far above everything else: wins the bucket maxima
    }

    QwtScaleMap xMap, yMap;
    xMap.setScaleInterval(0.0, n - 1);
    xMap.setPaintInterval(10.0, 910.0);
    yMap.setScaleInterval(-1.0, 6.0);
    yMap.setPaintInterval(490.0, 10.0);

    QwtPointMapper mapper;
    mapper.setFlag(QwtPointMapper::RoundPoints, true);
    mapper.setFlag(QwtPointMapper::MinMaxReduce, true);
    mapper.setBoundingRect(QRectF(0, 0, 900, 500));

    QwtPointArrayData< double > rawSeries(x, y);
    VirtualSampleSeries genericSeries(x, y);

    const QPolygonF rawPoly = mapper.toPolygonF(xMap, yMap, &rawSeries, 0, n - 1);
    const QPolygonF genPoly = mapper.toPolygonF(xMap, yMap, &genericSeries, 0, n - 1);

    QVERIFY(rawPoly.size() > 0);

    // no phantom vertices: x data starts at pixel 10
    for (const QPointF& p : rawPoly)
        QVERIFY2(p.x() >= 9.0, qPrintable(QString("phantom vertex at x=%1").arg(p.x())));

    // the raw-pointer path and the generic path are identical
    QCOMPARE(rawPoly.size(), genPoly.size());
    for (int i = 0; i < rawPoly.size(); i++)
        QCOMPARE(rawPoly[ i ], genPoly[ i ]);
}

/*
    +/-Inf must not pass the pre-scan into the SIMD path, where Inf wins
    the bucket extrema and maps to INT_MIN pixel coordinates.
 */
void TestNanCurve::lttb_inf_noGarbageCoordinates()
{
    const int n = 8000;
    QVector< double > x(n), y(n);
    for (int i = 0; i < n; i++) {
        x[ i ] = i;
        y[ i ] = std::sin(i * 0.01) * 0.8;
    }
    y[ 200 ] = std::numeric_limits< double >::infinity();
    y[ 400 ] = -std::numeric_limits< double >::infinity();
    y[ 600 ] = std::numeric_limits< double >::quiet_NaN();

    QwtScaleMap xMap, yMap;
    xMap.setScaleInterval(0.0, n - 1);
    xMap.setPaintInterval(10.0, 910.0);
    yMap.setScaleInterval(-1.0, 1.0);
    yMap.setPaintInterval(490.0, 10.0);

    QwtPointMapper mapper;
    mapper.setFlag(QwtPointMapper::RoundPoints, true);
    mapper.setFlag(QwtPointMapper::MinMaxReduce, true);
    mapper.setBoundingRect(QRectF(0, 0, 900, 500));

    QwtPointArrayData< double > rawSeries(x, y);
    VirtualSampleSeries genericSeries(x, y);

    const QPolygonF rawPoly = mapper.toPolygonF(xMap, yMap, &rawSeries, 0, n - 1);
    const QPolygonF genPoly = mapper.toPolygonF(xMap, yMap, &genericSeries, 0, n - 1);

    // no garbage coordinates: everything maps inside the paint area
    for (const QPointF& p : rawPoly) {
        QVERIFY(p.x() > -1000.0 && p.x() < 2000.0);
        QVERIFY(p.y() > -1000.0 && p.y() < 2000.0);
    }

    QCOMPARE(rawPoly.size(), genPoly.size());
    for (int i = 0; i < rawPoly.size(); i++)
        QCOMPARE(rawPoly[ i ], genPoly[ i ]);
}

/*
    All-NaN input must result in an empty polygon, so that
    "empty polygon == no data" holds for the callers.
 */
void TestNanCurve::filterPoints_allNan_emptyPolygon()
{
    QwtScaleMap xMap, yMap;
    xMap.setScaleInterval(0.0, 15.0);
    xMap.setPaintInterval(0.0, 15.0);
    yMap.setScaleInterval(0.0, 1.0);
    yMap.setPaintInterval(7.0, 0.0);

    QwtPointMapper mapper;
    mapper.setFlag(QwtPointMapper::RoundPoints, true);
    mapper.setFlag(QwtPointMapper::WeedOutPoints, true);
    mapper.setBoundingRect(QRectF(0, 0, 16, 8));

    {
        const int n = 16;
        QVector< double > x(n), y(n);
        for (int i = 0; i < n; i++) {
            x[ i ] = i;
            y[ i ] = qQNaN();
        }

        QwtPointArrayData< double > series(x, y);
        const QPolygonF poly = mapper.toPolygonF(xMap, yMap, &series, 0, n - 1);
        QCOMPARE(poly.size(), 0);
    }

    { // single NaN sample
        QVector< double > x = { 0.0 };
        QVector< double > y = { qQNaN() };

        QwtPointArrayData< double > series(x, y);
        const QPolygonF poly = mapper.toPolygonF(xMap, yMap, &series, 0, 0);
        QCOMPARE(poly.size(), 0);
    }
}

/*
    Leading NaN samples followed by finite samples: the first finite sample
    defines the start of the polyline, no NaN coordinate may leak into it.
 */
void TestNanCurve::filterPoints_leadingNan_singleFinite()
{
    QVector< double > x = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    QVector< double > y = { qQNaN(), qQNaN(), qQNaN(), qQNaN(), 1, 1, 2, 2, 3, 3 };

    QwtScaleMap xMap, yMap;
    xMap.setScaleInterval(0.0, 9.0);
    xMap.setPaintInterval(0.0, 9.0);
    yMap.setScaleInterval(0.0, 3.0);
    yMap.setPaintInterval(3.0, 0.0);

    QwtPointMapper mapper;
    mapper.setFlag(QwtPointMapper::RoundPoints, true);
    mapper.setFlag(QwtPointMapper::WeedOutPoints, true);
    mapper.setBoundingRect(QRectF(0, 0, 10, 4));

    QwtPointArrayData< double > series(x, y);
    const QPolygonF poly = mapper.toPolygonF(xMap, yMap, &series, 0, x.size() - 1);

    for (const QPointF& p : poly) {
        QVERIFY(std::isfinite(p.x()));
        QVERIFY(std::isfinite(p.y()));
    }

    // samples 4..9 map 1:1 to pixels ( x scale == paint scale ), so the
    // weeding filter keeps all of them: (4,2) (5,2) (6,1) (7,1) (8,0) (9,0)
    const QPolygonF expected = QPolygonF() << QPointF(4, 2) << QPointF(5, 2) << QPointF(6, 1)
                                           << QPointF(7, 1) << QPointF(8, 0) << QPointF(9, 0);

    QCOMPARE(poly.size(), expected.size());
    for (int i = 0; i < expected.size(); i++)
        QCOMPARE(poly[ i ], expected[ i ]);

    // the polyline starts at the first finite sample, no NaN leaked in
    QCOMPARE(poly.first(), QPointF(4, 2));
}

/*
    Anchor of the default skip semantics: the 12 sample working example
    from the tech report. Every internal NaN run is bridged with exactly
    one chord, the polyline stays connected.
 */
void TestNanCurve::bridgeSemantics_anchor()
{
    QVector< double > x = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    QVector< double > y = { 1, 2, qQNaN(), 0, qQNaN(), qQNaN(), 3, qQNaN(), 2, 1, qQNaN(), 0 };

    QwtScaleMap xMap, yMap;
    xMap.setScaleInterval(0.0, 11.0);
    xMap.setPaintInterval(0.0, 11.0);
    yMap.setScaleInterval(0.0, 3.0);
    yMap.setPaintInterval(7.0, 0.0); // y: 0 -> 7, 1 -> 5, 2 -> 2, 3 -> 0

    QwtPointMapper mapper;
    mapper.setFlag(QwtPointMapper::RoundPoints, true);
    mapper.setFlag(QwtPointMapper::WeedOutPoints, true);
    mapper.setBoundingRect(QRectF(0, 0, 12, 8));

    QwtPointArrayData< double > series(x, y);
    const QPolygonF poly = mapper.toPolygonF(xMap, yMap, &series, 0, x.size() - 1);

    const QPolygonF expected = QPolygonF() << QPointF(0, 5) << QPointF(1, 2) << QPointF(3, 7)
                                           << QPointF(6, 0) << QPointF(8, 2) << QPointF(9, 5)
                                           << QPointF(11, 7);

    QCOMPARE(poly.size(), expected.size());
    for (int i = 0; i < expected.size(); i++)
        QCOMPARE(poly[ i ], expected[ i ]);

    // one bridging chord per internal NaN run ( indices 2 / 4-5 / 7 / 10 )
    int bridged = 0;
    for (int i = 1; i < poly.size(); i++) {
        if (poly[ i ].x() - poly[ i - 1 ].x() > 1.5)
            bridged++;
    }
    QCOMPARE(bridged, 4);
}

/*
    An all-NaN curve with a brush must render as a blank canvas without
    crashing ( empty polygon through fillCurve/closePolyline ).
 */
void TestNanCurve::allNan_fill_noCrash()
{
    const int n = 200;
    QVector< double > x(n), y(n);
    for (int i = 0; i < n; i++) {
        x[ i ] = i;
        y[ i ] = qQNaN();
    }

    QwtPlot plot;
    plot.setCanvasBackground(Qt::white);
    plot.setAutoReplot(false);
    plot.resize(640, 480);

    QwtPlotCurve* curve = new QwtPlotCurve();
    curve->setSamples(x, y);
    curve->setPen(QPen(Qt::black, 2));
    curve->setBrush(QBrush(Qt::red));
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, false);
    // force the WeedOutPoints path ( FilterPoints ) of the mapper
    curve->setPaintAttribute(QwtPlotCurve::FilterPointsLTTB, false);
    curve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
    curve->attach(&plot);

    plot.setAxisScale(QwtAxis::XBottom, 0, n - 1);
    plot.setAxisScale(QwtAxis::YLeft, 0.0, 1.0);

    plot.show();
    QApplication::processEvents();
    plot.replot();

    const QImage img = plot.canvas()->grab().toImage();
    const QwtScaleMap& xm = plot.canvasMap(QwtAxis::XBottom);
    const QwtScaleMap& ym = plot.canvasMap(QwtAxis::YLeft);
    const QPoint center(qRound(xm.transform(n / 2)), qRound(ym.transform(0.5)));

    QVERIFY(!probePainted(img, center, 2));
}

/*
    BreakAtNan renders gap semantics: the gap stays blank, the plateaus
    on both sides are painted.
 */
void TestNanCurve::breakAtNan_gapBlank()
{
    QVector< double > x, y;
    makeGapData(x, y, 200, 80, 119);

    const CanvasProbe cp = renderCanvas(QwtPlotCurve::Lines, x, y, true);

    QVERIFY(!probePainted(cp.image, cp.pos(100.0, 0.5), 2));
    QVERIFY(probePainted(cp.image, cp.pos(40.0, 0.8), 2));
    QVERIFY(probePainted(cp.image, cp.pos(160.0, 0.2), 2));
}

/*
    Without BreakAtNan the default skip semantics bridges the gap with a
    chord that crosses the gap center.
 */
void TestNanCurve::default_bridgePresent()
{
    QVector< double > x, y;
    makeGapData(x, y, 200, 80, 119);

    const CanvasProbe cp = renderCanvas(QwtPlotCurve::Lines, x, y, false);

    QVERIFY(probePainted(cp.image, cp.pos(100.0, 0.5), 2));
    QVERIFY(probePainted(cp.image, cp.pos(40.0, 0.8), 2));
    QVERIFY(probePainted(cp.image, cp.pos(160.0, 0.2), 2));
}

/*
    Steps + BreakAtNan: neither the early riser at the left gap edge nor
    the tread across the gap may be painted.
 */
void TestNanCurve::steps_breakAtNan()
{
    QVector< double > x, y;
    makeGapData(x, y, 200, 80, 119);

    { // default: bridging steps paint a riser at the pre-gap x and a
      // tread across the gap at the post-gap level
        const CanvasProbe cp = renderCanvas(QwtPlotCurve::Steps, x, y, false);
        QVERIFY(probePainted(cp.image, cp.pos(79.0, 0.5), 2));
        QVERIFY(probePainted(cp.image, cp.pos(100.0, 0.2), 2));
    }

    { // BreakAtNan: both are gone, the plateaus are still painted
        const CanvasProbe cp = renderCanvas(QwtPlotCurve::Steps, x, y, true);
        QVERIFY(!probePainted(cp.image, cp.pos(79.0, 0.5), 2));
        QVERIFY(!probePainted(cp.image, cp.pos(100.0, 0.2), 2));
        QVERIFY(!probePainted(cp.image, cp.pos(100.0, 0.5), 2));
        QVERIFY(probePainted(cp.image, cp.pos(40.0, 0.8), 2));
        QVERIFY(probePainted(cp.image, cp.pos(160.0, 0.2), 2));
    }
}

/*
    Fill + BreakAtNan: the fill area of each finite run closes to the
    baseline on its own, the gap area is not filled.
 */
void TestNanCurve::fill_breakAtNan()
{
    QVector< double > x, y;
    makeGapData(x, y, 200, 80, 119);

    const QBrush brush(Qt::red);

    { // default: the bridged polyline fills across the gap
        const CanvasProbe cp = renderCanvas(QwtPlotCurve::Lines, x, y, false, brush);
        QVERIFY(probeIsColor(cp.image, cp.pos(100.0, 0.4), 2, qRgb(255, 0, 0)));
        QVERIFY(probeIsColor(cp.image, cp.pos(40.0, 0.4), 2, qRgb(255, 0, 0)));
    }

    { // BreakAtNan: gap not filled, plateau fills unchanged
        const CanvasProbe cp = renderCanvas(QwtPlotCurve::Lines, x, y, true, brush);
        QVERIFY(!probePainted(cp.image, cp.pos(100.0, 0.4), 2));
        QVERIFY(probeIsColor(cp.image, cp.pos(40.0, 0.4), 2, qRgb(255, 0, 0)));
        // the right plateau lies at 0.2, its fill covers [ 0, 0.2 ]
        QVERIFY(probeIsColor(cp.image, cp.pos(160.0, 0.1), 2, qRgb(255, 0, 0)));
    }
}

/*
    Without NaN data BreakAtNan must not change the rendering at all.
 */
void TestNanCurve::breakAtNan_cleanData_identical()
{
    const int n = 200;
    QVector< double > x(n), y(n);
    for (int i = 0; i < n; i++) {
        x[ i ] = i;
        y[ i ] = 0.5 + 0.4 * std::sin(i * 0.05);
    }

    const CanvasProbe cpOff = renderCanvas(QwtPlotCurve::Lines, x, y, false);
    const CanvasProbe cpOn = renderCanvas(QwtPlotCurve::Lines, x, y, true);

    QCOMPARE(cpOn.image, cpOff.image);
}

/*
    All-NaN input with BreakAtNan: blank canvas, no crash.
 */
void TestNanCurve::breakAtNan_allNan_noCrash()
{
    const int n = 200;
    QVector< double > x(n), y(n);
    for (int i = 0; i < n; i++) {
        x[ i ] = i;
        y[ i ] = qQNaN();
    }

    const CanvasProbe cp = renderCanvas(QwtPlotCurve::Lines, x, y, true, QBrush(Qt::red));

    QVERIFY(!probePainted(cp.image, QPoint(cp.image.width() / 2, cp.image.height() / 2), 5));
}

QTEST_MAIN(TestNanCurve)

#include "tst_nan_curve.moc"
