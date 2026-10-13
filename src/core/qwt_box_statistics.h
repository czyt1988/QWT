/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *
 * Modified by ChenZongYan in 2024 <czy.t@163.com>
 *****************************************************************************/

#ifndef QWT_BOX_STATISTICS_H
#define QWT_BOX_STATISTICS_H

#include "qwtcore_global.h"
#include "qwt_samples.h"

#include <qvector.h>

/**
 * @brief Helper class for computing boxplot statistics from raw data
 * @details Provides static methods to calculate whisker bounds, quartiles,
 *          median, and extract outliers using various methods (Tukey, percentile, SD, SE).
 *
 */
class QWTCORE_EXPORT QwtBoxStatisticsCalculator
{
public:
    /**
     * @brief Whisker calculation method
     */
    enum WhiskerMethod
    {
        //! Tukey's method: whiskers extend to 1.5×IQR from Q1/Q3 (default)
        Tukey,

        //! Percentile method: whiskers at the c-th / (100-c)-th percentile (c = tail percentage)
        Percentile,

        //! Min/Max method: whiskers at actual data min/max
        MinMax,

        //! Standard deviation method: whiskers at mean ± coeff×SD
        StandardDeviation,

        //! Standard error method: whiskers at mean ± coeff×SE
        StandardError
    };

    //! Compute QwtBoxSample from sorted data
    static QwtBoxSample
    calculate(double position, const QVector< double >& sortedData, WhiskerMethod method = Tukey, double coefficient = 1.5);

    //! Compute QwtBoxSample from unsorted data (sorts internally)
    static QwtBoxSample calculateFromRaw(double position,
                                         const QVector< double >& rawData,
                                         WhiskerMethod method = Tukey,
                                         double coefficient   = 1.5);

    //! Extract outliers given box sample and sorted data
    static QVector< double > extractOutliers(const QwtBoxSample& sample, const QVector< double >& sortedData);

    //! Compute full statistics (sample + outliers) in one call
    static void calculateFull(double position,
                              const QVector< double >& rawData,
                              QwtBoxSample& sample,
                              QwtBoxOutlierSample& outliers,
                              WhiskerMethod method = Tukey,
                              double coefficient   = 1.5);

private:
    //! Helper: compute quantile from sorted data (p in [0,1])
    static double quantile(const QVector< double >& sorted, double p);

    //! Helper: compute median from sorted data
    static double median(const QVector< double >& sorted);

    //! Helper: compute IQR from sorted data
    static double iqr(const QVector< double >& sorted);

    //! Helper: compute mean from data
    static double mean(const QVector< double >& data);

    //! Helper: compute standard deviation
    static double standardDeviation(const QVector< double >& data);

    //! Helper: sort data (returns new sorted vector)
    static QVector< double > sortData(const QVector< double >& data);
};

#endif  // QWT_BOX_STATISTICS_H