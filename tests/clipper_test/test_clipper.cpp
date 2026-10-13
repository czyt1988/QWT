/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "test_clipper.h"

#include "qwt_clipper.h"
#include "qwt_point_polar.h"

#include <QPointF>
#include <QRectF>
#include <QVector>

#include <cmath>

namespace
{

const double TWO_PI = 2.0 * M_PI;

/// Arc measure of an interval list, handling wrapped (min > max) intervals
double intervalMeasure(const QVector< QwtInterval >& intervals)
{
    double sum = 0.0;

    for (const QwtInterval& intv : intervals) {
        double span = intv.maxValue() - intv.minValue();
        if (span < 0.0)
            span += TWO_PI;

        sum += span;
    }

    return sum;
}

/// Is the circle point at the given angle part of the clipped (visible) arcs
bool visibleAt(const QVector< QwtInterval >& intervals, double angle)
{
    angle = std::fmod(std::fmod(angle, TWO_PI) + TWO_PI, TWO_PI);

    for (const QwtInterval& intv : intervals) {
        double from = intv.minValue();
        double span = intv.maxValue() - from;
        if (span < 0.0)
            span += TWO_PI;

        const double offset = std::fmod(angle - from + TWO_PI, TWO_PI);
        if (offset <= span + 1e-12)  // inclusive, tolerant against rounding at the interval ends
            return true;
    }

    return false;
}

/// Ground truth measure by dense angular sampling of the circle
double bruteForceMeasure(const QRectF& rect, const QPointF& center, double radius, int samples = 400000)
{
    int inside = 0;

    for (int i = 0; i < samples; ++i) {
        const double angle = TWO_PI * i / samples;
        if (rect.contains(qwtPolar2Pos(center, radius, angle)))
            inside++;
    }

    return TWO_PI * inside / samples;
}

void compareMeasure(const QRectF& rect, const QPointF& center, double radius,
                    double expected, double tolerance)
{
    const QVector< QwtInterval > intervals = QwtClipper::clipCircle(rect, center, radius);
    const double measure = intervalMeasure(intervals);

    QVERIFY2(std::fabs(measure - expected) <= tolerance,
             qPrintable(QString("measure %1 expected %2 (rect %3,%4 %5x%6, center %7,%8, r %9)")
                            .arg(measure, 0, 'g', 10)
                            .arg(expected, 0, 'g', 10)
                            .arg(rect.x())
                            .arg(rect.y())
                            .arg(rect.width())
                            .arg(rect.height())
                            .arg(center.x())
                            .arg(center.y())
                            .arg(radius)));

    // the same measure must follow from brute force sampling
    const double truth = bruteForceMeasure(rect, center, radius);
    QVERIFY2(std::fabs(measure - truth) <= 2e-3,
             qPrintable(QString("measure %1 vs brute force %2").arg(measure, 0, 'g', 10).arg(truth, 0, 'g', 10)));
}

} // namespace

void TestClipper::verticalEdgeLeft()
{
    // Report case: rect 10x10, center on the left edge (0, 8), r = 3.
    // Before the fix the vertical toAngle() branch returned 3*pi/2 for the
    // point above the center, flipping the pairing and reporting pi too much.
    const QRectF rect(0.0, 0.0, 10.0, 10.0);
    const QPointF center(0.0, 8.0);
    const double radius = 3.0;

    QVector< QwtInterval > intervals = QwtClipper::clipCircle(rect, center, radius);

    // The circle crosses the left edge at (0, 5) - above the center - and the
    // bottom edge at (2.2360679..., 10)
    QVERIFY(visibleAt(intervals, M_PI / 2));      // (0, 5), on the left edge
    QVERIFY(visibleAt(intervals, 0.0));           // (3, 8), inside
    QVERIFY(!visibleAt(intervals, M_PI));         // (-3, 8), outside
    QVERIFY(!visibleAt(intervals, 3 * M_PI / 2)); // (0, 11), below the rect

    const double expected = TWO_PI + M_PI / 2.0 - (TWO_PI - std::atan(2.0 / std::sqrt(5.0)));
    compareMeasure(rect, center, radius, expected, 1e-9);
}

void TestClipper::verticalEdgeRight()
{
    const QRectF rect(0.0, 0.0, 10.0, 10.0);
    const QPointF center(10.0, 3.0);
    const double radius = 4.0;

    const QVector< QwtInterval > intervals = QwtClipper::clipCircle(rect, center, radius);

    QVERIFY(visibleAt(intervals, M_PI));          // (6, 3), inside
    QVERIFY(visibleAt(intervals, 3 * M_PI / 2));  // (10, 7), on the right edge
    QVERIFY(!visibleAt(intervals, M_PI / 2));     // (10, -1), above the rect
    QVERIFY(!visibleAt(intervals, 0.0));          // (14, 3), outside

    compareMeasure(rect, center, radius, 2.4188, 1e-3);
}

void TestClipper::cornerPassingCircle()
{
    // A circle through all four corners of the rect (center = rect center,
    // r = half diagonal) touches the rectangle in four points: the visible
    // measure is 0. The duplicates used to flip the result between the whole
    // circle (2*pi) and four zero length arcs, depending on 1 ulp.
    {
        const QRectF rect(0.0, 0.0, 100.0, 100.0);
        const QPointF center(50.0, 50.0);
        const double radius = std::sqrt(5000.0);

        compareMeasure(rect, center, radius, 0.0, 1e-6);
        QVERIFY(QwtClipper::clipCircle(rect, center, radius).isEmpty());
    }

    // The non square variant of the same configuration
    {
        const QRectF rect(10.0, 20.0, 300.0, 200.0);
        const QPointF center(160.0, 120.0);
        const double radius = std::sqrt(150.0 * 150.0 + 100.0 * 100.0);

        compareMeasure(rect, center, radius, 0.0, 1e-6);
        QVERIFY(QwtClipper::clipCircle(rect, center, radius).isEmpty());
    }
}

void TestClipper::cornerTouchingFromInside()
{
    // Center on the rectangle corner (0, 0) of a 100x100 rect, r = 50: the
    // visible part is the quarter arc inside the rect (measure pi/2). The old
    // vertical toAngle() branch reported the opposite half and returned 3*pi/2.
    const QRectF rect(0.0, 0.0, 100.0, 100.0);
    const QPointF center(0.0, 0.0);
    const double radius = 50.0;

    const QVector< QwtInterval > intervals = QwtClipper::clipCircle(rect, center, radius);

    QVERIFY(visibleAt(intervals, 7 * M_PI / 4));   // (35.36, 35.36), inside
    QVERIFY(!visibleAt(intervals, M_PI / 4));      // (35.36, -35.36), outside
    QVERIFY(!visibleAt(intervals, 3 * M_PI / 4));  // (-35.36, -35.36), outside
    QVERIFY(!visibleAt(intervals, 5 * M_PI / 4));  // (-35.36, 35.36), outside

    compareMeasure(rect, center, radius, M_PI / 2.0, 1e-9);
}

void TestClipper::inscribedCircle()
{
    // The circle touches all four edges from inside: the strict comparison
    // produces no cutting points and the containment fallback applies
    const QRectF rect(0.0, 0.0, 100.0, 100.0);
    const QVector< QwtInterval > intervals = QwtClipper::clipCircle(rect, QPointF(50.0, 50.0), 50.0);

    QCOMPARE(intervals.size(), 1);
    QCOMPARE(intervals[ 0 ].minValue(), 0.0);
    QCOMPARE(intervals[ 0 ].maxValue(), TWO_PI);
}

void TestClipper::containmentCases()
{
    const QRectF rect(0.0, 0.0, 100.0, 100.0);

    // fully inside
    {
        const QVector< QwtInterval > intervals = QwtClipper::clipCircle(rect, QPointF(50.0, 50.0), 10.0);
        QCOMPARE(intervals.size(), 1);
        QCOMPARE(intervals[ 0 ].minValue(), 0.0);
        QCOMPARE(intervals[ 0 ].maxValue(), TWO_PI);
    }

    // fully outside
    {
        const QVector< QwtInterval > intervals = QwtClipper::clipCircle(rect, QPointF(500.0, 500.0), 10.0);
        QVERIFY(intervals.isEmpty());
    }

    // the circle contains the rectangle
    {
        const QVector< QwtInterval > intervals = QwtClipper::clipCircle(rect, QPointF(50.0, 50.0), 500.0);
        QVERIFY(intervals.isEmpty());
    }

    // radius 0: QRectF::contains rejects the zero width bounding rectangle,
    // so the documented [0, 2*pi] branch is not reached (see the report)
    {
        const QVector< QwtInterval > intervals = QwtClipper::clipCircle(rect, QPointF(50.0, 50.0), 0.0);
        QVERIFY(intervals.isEmpty());
    }
}

void TestClipper::measureMatchesBruteForce()
{
    const QRectF rect(100.0, 100.0, 200.0, 150.0);

    struct Case
    {
        QPointF center;
        double radius;
    };

    // Centers on edges, corners, inside and outside, radii below/above the
    // distance to the edges - the configurations of the report's case table
    const Case cases[] = {
        { QPointF(200.0, 175.0), 60.0 },   // generic arcs
        { QPointF(100.0, 175.0), 40.0 },   // center on the left edge
        { QPointF(300.0, 175.0), 40.0 },   // center on the right edge
        { QPointF(200.0, 100.0), 40.0 },   // center on the top edge
        { QPointF(200.0, 250.0), 40.0 },   // center on the bottom edge
        { QPointF(100.0, 100.0), 50.0 },   // center on a corner
        { QPointF(300.0, 250.0), 50.0 },   // center on the opposite corner
        { QPointF(200.0, 175.0), 1000.0 }, // contains the rectangle
        { QPointF(-500.0, -500.0), 10.0 }, // fully outside
        { QPointF(200.0, 175.0), 10.0 }    // fully inside
    };

    for (const Case& c : cases) {
        const QVector< QwtInterval > intervals = QwtClipper::clipCircle(rect, c.center, c.radius);

        const double measure = intervalMeasure(intervals);
        const double truth   = bruteForceMeasure(rect, c.center, c.radius);

        QVERIFY2(std::fabs(measure - truth) <= 2e-3,
                 qPrintable(QString("center %1,%2 r %3: measure %4 vs brute force %5")
                                .arg(c.center.x())
                                .arg(c.center.y())
                                .arg(c.radius)
                                .arg(measure, 0, 'g', 10)
                                .arg(truth, 0, 'g', 10)));
    }
}

QTEST_APPLESS_MAIN(TestClipper)
#include "test_clipper.moc"
