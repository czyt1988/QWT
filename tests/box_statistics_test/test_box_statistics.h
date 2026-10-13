/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef TEST_BOX_STATISTICS_H
#define TEST_BOX_STATISTICS_H

#include <QObject>
#include <QTest>
#include <QCoreApplication>

#include <qwt_box_statistics.h>

/**
 * @brief Tests for QwtBoxStatisticsCalculator
 * @details Covers the whisker conventions of every WhiskerMethod, with a
 *          regression test for the Percentile method whose whisker levels were
 *          exchanged (whiskerLower/whiskerUpper) before v7.5.1.
 */
class TestBoxStatistics : public QObject
{
    Q_OBJECT

private slots:
    /// Percentile: whiskerLower is the c-th, whiskerUpper the (100-c)-th percentile
    void percentileRegressionDataSet();

    /// Percentile: the sample stays ordered for every sane tail coefficient
    void percentileOrderingForAllCoefficients();

    /// Percentile: outlierCount matches the tail split (5%/95% -> 8 of 64)
    void percentileOutlierCountEqualsTailSplit();

    /// Percentile on linear data: quantile levels are the R-7 interpolation
    void percentileLinearData();

    /// Tukey/StandardDeviation/StandardError/MinMax keep whisker ordering
    void otherMethodsStayOrdered();

    /// MinMax has no outliers by construction
    void minMaxHasNoOutliers();

    /// calculateFromRaw drops NaN and Inf before statistics
    void rawInputFiltersNanAndInf();

    /// Empty input yields a zeroed sample, not a crash
    void emptyInputIsZeroed();

    /// The calculator fills the mean of every sample it computes
    void calculatorFillsMean();

    /// Manually assembled samples carry no mean until one is assigned
    void manualSampleHasNoMean();
};

#endif  // TEST_BOX_STATISTICS_H
