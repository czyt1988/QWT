/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "test_downsampling.h"

#include <QVector>
#include <QWidget>

#include <cmath>
#include <limits>

#include <qwt_point_data.h>
#include <qwt_point_mapper.h>
#include <qwt_scale_map.h>
#include <qwt_series_data.h>
#include <qwt_transform.h>

namespace
{
    // canvas geometry used by all tests: 100 x 80 pixels
    const double CanvasWidth  = 100.0;
    const double CanvasHeight = 80.0;

    // QwtPointMapper derives the bucket count from the number of
    // pixel columns: 4 buckets per column ( including both borders )
    const int NumBuckets = 4 * (int(CanvasWidth) + 1);

    // A spectrum of numPoints samples with 3 Gaussian peaks
    // ( each peak is sampled by far more than 50 samples ) on a zero
    // baseline, x in [0,10].
    void buildGaussianSpectrum(QVector< double >& x, QVector< double >& y, int numPoints = 10001)
    {
        x.clear();
        y.clear();
        x.reserve(numPoints);
        y.reserve(numPoints);

        const double dx = 10.0 / (numPoints - 1);
        for (int i = 0; i < numPoints; i++) {
            const double xi = i * dx;

            const double d1 = (xi - 2.0) / 0.1;
            const double d2 = (xi - 5.0) / 0.08;
            const double d3 = (xi - 8.0) / 0.06;

            x += xi;
            y += 0.8 * std::exp(-0.5 * d1 * d1)
               + 0.5 * std::exp(-0.5 * d2 * d2)
               + 0.3 * std::exp(-0.5 * d3 * d3);
        }
    }

    QwtScaleMap buildXMap()
    {
        QwtScaleMap map;
        map.setScaleInterval(0.0, 10.0);
        map.setPaintInterval(0.0, CanvasWidth);
        return map;
    }

    QwtScaleMap buildYMap()
    {
        QwtScaleMap map;
        map.setScaleInterval(-0.2, 1.0);
        map.setPaintInterval(0.0, CanvasHeight);
        return map;
    }

    QRectF canvasRect()
    {
        return QRectF(0.0, 0.0, CanvasWidth, CanvasHeight);
    }

    // A series, that can not be accessed by raw pointers,
    // so that the generic sample() code paths are used.
    class TestSeries: public QwtSeriesData< QPointF >
    {
    public:
        TestSeries(const QVector< double >& x, const QVector< double >& y)
            : m_x(x)
            , m_y(y)
        {
        }

        size_t size() const override
        {
            return m_x.size();
        }

        QPointF sample(size_t i) const override
        {
            return QPointF(m_x[ i ], m_y[ i ]);
        }

        QRectF boundingRect() const override
        {
            return QRectF(0.0, 0.0, 10.0, 1.0);
        }

    private:
        const QVector< double > m_x;
        const QVector< double > m_y;
    };
}

void TestDownsampling::testLttbOutputSizeAndEndpoints()
{
    QVector< double > x, y;
    buildGaussianSpectrum(x, y);
    QwtPointArrayData< double > series(x, y);

    const QwtScaleMap xMap = buildXMap();
    const QwtScaleMap yMap = buildYMap();

    QwtPointMapper mapper;
    mapper.setBoundingRect(canvasRect());
    mapper.setFlag(QwtPointMapper::LTTBReduce, true);

    const QPolygonF polyline = mapper.toPolygonF(xMap, yMap, &series, 0, x.size() - 1);

    // one point per bucket: 4 buckets per pixel column
    QCOMPARE(polyline.size(), qsizetype(NumBuckets));

    // the first and last sample are always kept
    QCOMPARE(polyline.first().x(), xMap.transform(x.first()));
    QCOMPARE(polyline.first().y(), yMap.transform(y.first()));
    QCOMPARE(polyline.last().x(), xMap.transform(x.last()));
    QCOMPARE(polyline.last().y(), yMap.transform(y.last()));
}

void TestDownsampling::testLttbRounding()
{
    QVector< double > x, y;
    buildGaussianSpectrum(x, y);
    QwtPointArrayData< double > series(x, y);

    const QwtScaleMap xMap = buildXMap();
    const QwtScaleMap yMap = buildYMap();

    QwtPointMapper mapper;
    mapper.setBoundingRect(canvasRect());
    mapper.setFlag(QwtPointMapper::LTTBReduce, true);
    mapper.setFlag(QwtPointMapper::RoundPoints, true);

    const QPolygonF polyline = mapper.toPolygonF(xMap, yMap, &series, 0, x.size() - 1);

    QVERIFY(polyline.size() <= qsizetype(NumBuckets));

    for (const QPointF& p : polyline) {
        QCOMPARE(p.x(), double(qRound(p.x())));
        QCOMPARE(p.y(), double(qRound(p.y())));
    }
}

void TestDownsampling::testMinMaxSubPixelPrecision()
{
    QVector< double > x, y;
    buildGaussianSpectrum(x, y);
    QwtPointArrayData< double > series(x, y);

    const QwtScaleMap xMap = buildXMap();
    const QwtScaleMap yMap = buildYMap();

    {
        // without RoundPoints the output keeps sub pixel precision
        QwtPointMapper mapper;
        mapper.setBoundingRect(canvasRect());
        mapper.setFlag(QwtPointMapper::MinMaxReduce, true);

        const QPolygonF polyline = mapper.toPolygonF(xMap, yMap, &series, 0, x.size() - 1);

        QVERIFY(polyline.size() > 2 * qsizetype(CanvasWidth));

        bool hasSubPixel = false;
        for (const QPointF& p : polyline) {
            if (p.x() != double(qRound(p.x())) || p.y() != double(qRound(p.y()))) {
                hasSubPixel = true;
                break;
            }
        }
        QVERIFY(hasSubPixel);
    }

    {
        // with RoundPoints the output is snapped to integer pixels
        QwtPointMapper mapper;
        mapper.setBoundingRect(canvasRect());
        mapper.setFlag(QwtPointMapper::MinMaxReduce, true);
        mapper.setFlag(QwtPointMapper::RoundPoints, true);

        const QPolygonF polyline = mapper.toPolygonF(xMap, yMap, &series, 0, x.size() - 1);

        for (const QPointF& p : polyline) {
            QCOMPARE(p.x(), double(qRound(p.x())));
            QCOMPARE(p.y(), double(qRound(p.y())));
        }
    }
}

void TestDownsampling::testPeakApexPreserved()
{
    QVector< double > x, y;
    buildGaussianSpectrum(x, y);
    QwtPointArrayData< double > series(x, y);

    const QwtScaleMap xMap = buildXMap();
    const QwtScaleMap yMap = buildYMap();

    const double peaks[ 3 ][ 2 ] = {
        { 2.0, 0.8 },  // x center, height
        { 5.0, 0.5 },
        { 8.0, 0.3 }
    };

    for (int mode = 0; mode < 2; mode++) {
        QwtPointMapper mapper;
        mapper.setBoundingRect(canvasRect());
        if (mode == 0)
            mapper.setFlag(QwtPointMapper::LTTBReduce, true);
        else
            mapper.setFlag(QwtPointMapper::MinMaxReduce, true);

        const QPolygonF polyline = mapper.toPolygonF(xMap, yMap, &series, 0, x.size() - 1);

        for (const auto& peak : peaks) {
            const double centerX = xMap.transform(peak[ 0 ]);
            const double apexY   = yMap.transform(peak[ 1 ]);

            // the highest point inside a +/- 3 pixel window around the peak
            double bestY = -1.0e12;
            for (const QPointF& p : polyline) {
                if (qAbs(p.x() - centerX) <= 3.0)
                    bestY = qMax(bestY, p.y());
            }

            // the apex of the peak must survive downsampling
            QVERIFY2(bestY > -1.0e11, "no point inside the peak window");
            QVERIFY2(qAbs(bestY - apexY) <= 2.0,
                qPrintable(QString("apex lost: %1 vs %2").arg(bestY).arg(apexY)));
        }
    }
}

void TestDownsampling::testBelowThresholdFallback()
{
    // 300 samples are below the activation threshold ( 4 * canvas width ),
    // so nothing must be dropped
    QVector< double > x, y;
    buildGaussianSpectrum(x, y, 300);
    QwtPointArrayData< double > series(x, y);

    const QwtScaleMap xMap = buildXMap();
    const QwtScaleMap yMap = buildYMap();

    for (int mode = 0; mode < 2; mode++) {
        QwtPointMapper mapper;
        mapper.setBoundingRect(canvasRect());
        if (mode == 0)
            mapper.setFlag(QwtPointMapper::LTTBReduce, true);
        else
            mapper.setFlag(QwtPointMapper::MinMaxReduce, true);

        const QPolygonF polyline = mapper.toPolygonF(xMap, yMap, &series, 0, x.size() - 1);
        QCOMPARE(polyline.size(), qsizetype(300));
    }
}

void TestDownsampling::testNaNHandling()
{
    QVector< double > x, y;
    buildGaussianSpectrum(x, y);

    // a block of NaN samples inside the data
    for (int i = 4000; i < 4200; i++)
        y[ i ] = std::numeric_limits< double >::quiet_NaN();

    QwtPointArrayData< double > series(x, y);

    const QwtScaleMap xMap = buildXMap();
    const QwtScaleMap yMap = buildYMap();

    for (int mode = 0; mode < 2; mode++) {
        QwtPointMapper mapper;
        mapper.setBoundingRect(canvasRect());
        if (mode == 0)
            mapper.setFlag(QwtPointMapper::LTTBReduce, true);
        else
            mapper.setFlag(QwtPointMapper::MinMaxReduce, true);

        const QPolygonF polyline = mapper.toPolygonF(xMap, yMap, &series, 0, x.size() - 1);

        QVERIFY(polyline.size() > 0);

        for (const QPointF& p : polyline) {
            QVERIFY(!std::isnan(p.x()));
            QVERIFY(!std::isnan(p.y()));
        }
    }
}

void TestDownsampling::testGenericSeriesPath()
{
    QVector< double > x, y;
    buildGaussianSpectrum(x, y);
    TestSeries series(x, y);

    const QwtScaleMap xMap = buildXMap();
    const QwtScaleMap yMap = buildYMap();

    QwtPointMapper mapper;
    mapper.setBoundingRect(canvasRect());
    mapper.setFlag(QwtPointMapper::LTTBReduce, true);

    const QPolygonF polyline = mapper.toPolygonF(xMap, yMap, &series, 0, x.size() - 1);

    QVERIFY(polyline.size() <= qsizetype(NumBuckets));
    QVERIFY(polyline.size() > qsizetype(2 * CanvasWidth));

    QCOMPARE(polyline.first().x(), xMap.transform(x.first()));
    QCOMPARE(polyline.last().x(), xMap.transform(x.last()));
}

void TestDownsampling::testNonLinearScale()
{
    QVector< double > x, y;
    buildGaussianSpectrum(x, y);

    // shift the baseline, so that all values are positive
    for (int i = 0; i < y.size(); i++)
        y[ i ] += 0.001;

    QwtPointArrayData< double > series(x, y);

    const QwtScaleMap xMap = buildXMap();

    QwtScaleMap yMap;
    yMap.setTransformation(new QwtLogTransform());
    yMap.setScaleInterval(1.0e-3, 1.0);
    yMap.setPaintInterval(0.0, CanvasHeight);

    for (int mode = 0; mode < 2; mode++) {
        QwtPointMapper mapper;
        mapper.setBoundingRect(canvasRect());
        if (mode == 0)
            mapper.setFlag(QwtPointMapper::LTTBReduce, true);
        else
            mapper.setFlag(QwtPointMapper::MinMaxReduce, true);

        const QPolygonF polyline = mapper.toPolygonF(xMap, yMap, &series, 0, x.size() - 1);

        QVERIFY(polyline.size() > 0);

        for (const QPointF& p : polyline) {
            QVERIFY(!std::isnan(p.x()));
            QVERIFY(!std::isnan(p.y()));
        }
    }
}

void TestDownsampling::testDefaultPaintAttributes()
{
    QwtPlotCurve curve;

    // the default downsampling is the shape preserving LTTB algorithm
    QVERIFY(curve.testPaintAttribute(QwtPlotCurve::ClipPolygons));
    QVERIFY(curve.testPaintAttribute(QwtPlotCurve::FilterPointsLTTB));

    QVERIFY(!curve.testPaintAttribute(QwtPlotCurve::FilterPointsMinMaxLTTB));
    QVERIFY(!curve.testPaintAttribute(QwtPlotCurve::FilterPointsPixel));
    QVERIFY(!curve.testPaintAttribute(QwtPlotCurve::FilterPointsAggressive));
    QVERIFY(!curve.testPaintAttribute(QwtPlotCurve::FilterPoints));
}

QTEST_MAIN(TestDownsampling)
#include "test_downsampling.moc"
