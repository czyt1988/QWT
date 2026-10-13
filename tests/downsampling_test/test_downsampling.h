/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef TEST_DOWNSAMPLING_H
#define TEST_DOWNSAMPLING_H

#include <QObject>
#include <QTest>
#include <QApplication>

#include <qwt_plot_curve.h>

/**
 * @brief Tests for the curve downsampling algorithms
 * @details This test class verifies the invariants of the
 *          QwtPointMapper downsampling flags LTTBReduce ( Largest
 *          Triangle Three Buckets ) and MinMaxReduce, and the wiring
 *          of the QwtPlotCurve paint attributes
 *          FilterPointsLTTB / FilterPointsMinMaxLTTB.
 */
class TestDownsampling : public QObject
{
    Q_OBJECT

private slots:
    /// LTTB emits one point per bucket and always keeps the endpoints
    void testLttbOutputSizeAndEndpoints();

    /// With RoundPoints the LTTB output is snapped to integer pixels
    void testLttbRounding();

    /// Without RoundPoints the MinMax output keeps sub pixel precision
    void testMinMaxSubPixelPrecision();

    /// The apex of Gaussian peaks survives LTTB and MinMax downsampling
    void testPeakApexPreserved();

    /// Below the activation threshold no samples are dropped
    void testBelowThresholdFallback();

    /// NaN samples are skipped by both algorithms
    void testNaNHandling();

    /// LTTB works with series, that have no raw pointer access
    void testGenericSeriesPath();

    /// LTTB and MinMax work with a logarithmic y axis
    void testNonLinearScale();

    /// The default paint attributes enable ClipPolygons | FilterPointsLTTB
    void testDefaultPaintAttributes();
};

#endif // TEST_DOWNSAMPLING_H
