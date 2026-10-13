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

#include "qwt_box_statistics.h"
#include "qwt_math.h"

#include <algorithm>

QVector< double > QwtBoxStatisticsCalculator::sortData(const QVector< double >& data)
{
    QVector< double > sorted = data;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

double QwtBoxStatisticsCalculator::quantile(const QVector< double >& sorted, double p)
{
    if (sorted.isEmpty())
        return 0.0;

    const int n        = sorted.size();
    const double index = p * (n - 1);
    const int lower    = static_cast< int >(std::floor(index));
    const int upper    = static_cast< int >(std::ceil(index));

    if (lower == upper)
        return sorted[ lower ];

    // Linear interpolation
    const double frac = index - lower;
    return sorted[ lower ] + frac * (sorted[ upper ] - sorted[ lower ]);
}

double QwtBoxStatisticsCalculator::median(const QVector< double >& sorted)
{
    return quantile(sorted, 0.5);
}

double QwtBoxStatisticsCalculator::iqr(const QVector< double >& sorted)
{
    return quantile(sorted, 0.75) - quantile(sorted, 0.25);
}

double QwtBoxStatisticsCalculator::mean(const QVector< double >& data)
{
    if (data.isEmpty())
        return 0.0;

    double sum = 0.0;
    int count = 0;
    for (int i = 0; i < data.size(); ++i) {
        if (!qwt_is_nan_or_inf(data[ i ])) {
            sum += data[ i ];
            ++count;
        }
    }

    return count > 0 ? sum / count : qQNaN();
}

double QwtBoxStatisticsCalculator::standardDeviation(const QVector< double >& data)
{
    if (data.size() < 2)
        return 0.0;

    double sum = 0.0;
    int count = 0;
    for (int i = 0; i < data.size(); ++i) {
        if (!qwt_is_nan_or_inf(data[ i ])) {
            sum += data[ i ];
            ++count;
        }
    }

    if (count < 2)
        return count == 0 ? qQNaN() : 0.0;

    const double m = sum / count;
    double sumSq   = 0.0;
    for (int i = 0; i < data.size(); ++i) {
        if (qwt_is_nan_or_inf(data[ i ]))
            continue;
        const double diff = data[ i ] - m;
        sumSq += diff * diff;
    }

    return std::sqrt(sumSq / (count - 1));
}

/**
 * @brief Compute boxplot statistics from sorted data
 * @details Calculates whisker bounds, quartiles, median, and outlier count
 *          using the specified whisker method.
 * @param[in] position The position value for the resulting sample
 * @param[in] sortedData Pre-sorted data vector
 * @param[in] method Whisker calculation method (default: Tukey)
 * @param[in] coefficient Whisker coefficient (default: 1.5)
 * @return QwtBoxSample containing computed statistics
 */
QwtBoxSample QwtBoxStatisticsCalculator::calculate(double position,
                                                   const QVector< double >& sortedData,
                                                   WhiskerMethod method,
                                                   double coefficient)
{
    if (sortedData.isEmpty())
        return QwtBoxSample(position);

    const double minVal = sortedData.first();
    const double maxVal = sortedData.last();
    const double med    = median(sortedData);
    const double q1     = quantile(sortedData, 0.25);
    const double q3     = quantile(sortedData, 0.75);

    double whiskerLower, whiskerUpper;

    switch (method) {
    case Tukey: {
        const double iqrVal = q3 - q1;
        whiskerLower        = qwtMaxF(minVal, q1 - coefficient * iqrVal);
        whiskerUpper        = qwtMinF(maxVal, q3 + coefficient * iqrVal);
        break;
    }
    case Percentile: {
        whiskerLower = quantile(sortedData, coefficient / 100.0);
        whiskerUpper = quantile(sortedData, 1.0 - coefficient / 100.0);
        break;
    }
    case MinMax: {
        whiskerLower = minVal;
        whiskerUpper = maxVal;
        break;
    }
    case StandardDeviation: {
        const double m  = mean(sortedData);
        const double sd = standardDeviation(sortedData);
        whiskerLower    = qwtMaxF(minVal, m - coefficient * sd);
        whiskerUpper    = qwtMinF(maxVal, m + coefficient * sd);
        break;
    }
    case StandardError: {
        const double m  = mean(sortedData);
        const double sd = standardDeviation(sortedData);
        const double se = sd / std::sqrt(static_cast< double >(sortedData.size()));
        whiskerLower    = qwtMaxF(minVal, m - coefficient * se);
        whiskerUpper    = qwtMinF(maxVal, m + coefficient * se);
        break;
    }
    default: {
        whiskerLower = minVal;
        whiskerUpper = maxVal;
        break;
    }
    }

    QwtBoxSample sample(position, whiskerLower, q1, med, q3, whiskerUpper);
    sample.mean = mean(sortedData);

    // Count outliers
    int outlierCount = 0;
    for (int i = 0; i < sortedData.size(); ++i) {
        if (qwt_is_nan_or_inf(sortedData[ i ]))
            continue;
        if (sortedData[ i ] < whiskerLower || sortedData[ i ] > whiskerUpper)
            outlierCount++;
    }
    sample.outlierCount = outlierCount;

    return sample;
}

/**
 * @brief Compute boxplot statistics from unsorted raw data
 * @details Sorts the data internally and then calculates statistics.
 * @param[in] position The position value for the resulting sample
 * @param[in] rawData Unsorted raw data vector
 * @param[in] method Whisker calculation method (default: Tukey)
 * @param[in] coefficient Whisker coefficient (default: 1.5)
 * @return QwtBoxSample containing computed statistics
 */
QwtBoxSample QwtBoxStatisticsCalculator::calculateFromRaw(double position,
                                                          const QVector< double >& rawData,
                                                          WhiskerMethod method,
                                                          double coefficient)
{
    QVector< double > filtered;
    filtered.reserve(rawData.size());
    for (int i = 0; i < rawData.size(); ++i) {
        if (!qwt_is_nan_or_inf(rawData[ i ]))
            filtered.append(rawData[ i ]);
    }

    QVector< double > sorted = sortData(filtered);
    return calculate(position, sorted, method, coefficient);
}

/**
 * @brief Extract outlier values from sorted data based on box sample
 * @details Identifies all values outside the whisker bounds as outliers.
 * @param[in] sample The box sample containing whisker bounds
 * @param[in] sortedData Pre-sorted data vector
 * @return Vector of outlier values
 */
QVector< double > QwtBoxStatisticsCalculator::extractOutliers(const QwtBoxSample& sample, const QVector< double >& sortedData)
{
    QVector< double > outliers;

    for (int i = 0; i < sortedData.size(); ++i) {
        const double val = sortedData[ i ];
        if (qwt_is_nan_or_inf(val))
            continue;
        if (val < sample.whiskerLower || val > sample.whiskerUpper)
            outliers.append(val);
    }

    return outliers;
}

/**
 * @brief Compute full boxplot statistics including outliers
 * @details Calculates both the box sample and extracts outlier values in one call.
 * @param[in] position The position value for the resulting samples
 * @param[in] rawData Unsorted raw data vector
 * @param[out] sample Output parameter for computed box statistics
 * @param[out] outliers Output parameter for extracted outlier values
 * @param[in] method Whisker calculation method (default: Tukey)
 * @param[in] coefficient Whisker coefficient (default: 1.5)
 */
void QwtBoxStatisticsCalculator::calculateFull(double position,
                                               const QVector< double >& rawData,
                                               QwtBoxSample& sample,
                                               QwtBoxOutlierSample& outliers,
                                               WhiskerMethod method,
                                               double coefficient)
{
    QVector< double > filtered;
    filtered.reserve(rawData.size());
    for (int i = 0; i < rawData.size(); ++i) {
        if (!qwt_is_nan_or_inf(rawData[ i ]))
            filtered.append(rawData[ i ]);
    }

    QVector< double > sorted = sortData(filtered);
    sample                   = calculate(position, sorted, method, coefficient);

    QVector< double > outlierValues = extractOutliers(sample, sorted);
    outliers                        = QwtBoxOutlierSample(position, std::move(outlierValues));
}