/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *
 * Modified by ChenZongYan in 2024 <czy.t@163.com>
 *   Summary of major modifications (see ChangeLog.md for full history):
 *   1. CMake build system & C++11 throughout.
 *   2. Core panner/ zoomer refactored:
 *        - QwtPanner → QwtCachePanner (pixmap-cache version)
 *        - New real-time QwtPlotPanner derived from QwtPicker.
 *   3. Zoomer supports multi-axis.
 *   4. Parasite-plot framework:
 *        - QwtFigure, QwtPlotParasiteLayout, QwtPlotTransparentCanvas,
 *        - QwtPlotScaleEventDispatcher, built-in pan/zoom on axis.
 *   5. New picker: QwtPlotSeriesDataPicker (works with date axis).
 *   6. Raster & color-map extensions:
 *        - QwtGridRasterData (2-D table + interpolation)
 *        - QwtLinearColorMap::stopColors(), stopPos() API rename.
 *   7. Bar-chart: expose pen/brush control.
 *   8. Amalgamated build: single QwtPlot.h / QwtPlot.cpp pair in src-amalgamate.
 *****************************************************************************/

#include "qwt_scale_div.h"
#include "qwt_interval.h"
#include "qwt_math.h"

#include <algorithm>

/**
 * @brief Construct a division without ticks
 *
 * @param lowerBound First boundary
 * @param upperBound Second boundary
 *
 * @note lowerBound might be greater than upperBound for inverted scales
 *
 */
QwtScaleDiv::QwtScaleDiv(double lowerBound, double upperBound) : m_lowerBound(lowerBound), m_upperBound(upperBound)
{
}

/**
 * @brief Construct a scale division
 *
 * @param interval Interval
 * @param ticks List of major, medium and minor ticks
 *
 */
QwtScaleDiv::QwtScaleDiv(const QwtInterval& interval, QList< double > ticks[ NTickTypes ])
    : m_lowerBound(interval.minValue()), m_upperBound(interval.maxValue())
{
    for (int i = 0; i < NTickTypes; i++)
        m_ticks[ i ] = ticks[ i ];
}

/**
 * @brief Construct a scale division
 *
 * @param lowerBound First boundary
 * @param upperBound Second boundary
 * @param ticks List of major, medium and minor ticks
 *
 * @note lowerBound might be greater than upperBound for inverted scales
 *
 */
QwtScaleDiv::QwtScaleDiv(double lowerBound, double upperBound, QList< double > ticks[ NTickTypes ])
    : m_lowerBound(lowerBound), m_upperBound(upperBound)
{
    for (int i = 0; i < NTickTypes; i++)
        m_ticks[ i ] = ticks[ i ];
}

/**
 * @brief Construct a scale division
 *
 * @param lowerBound First boundary
 * @param upperBound Second boundary
 * @param minorTicks List of minor ticks
 * @param mediumTicks List of medium ticks
 * @param majorTicks List of major ticks
 *
 * @note lowerBound might be greater than upperBound for inverted scales
 *
 */
QwtScaleDiv::QwtScaleDiv(double lowerBound,
                         double upperBound,
                         const QList< double >& minorTicks,
                         const QList< double >& mediumTicks,
                         const QList< double >& majorTicks)
    : m_lowerBound(lowerBound), m_upperBound(upperBound)
{
    m_ticks[ MinorTick ]  = minorTicks;
    m_ticks[ MediumTick ] = mediumTicks;
    m_ticks[ MajorTick ]  = majorTicks;
}

/**
 * @brief Copy constructor
 */
QwtScaleDiv::QwtScaleDiv(const QwtScaleDiv&) = default;

/**
 * @brief Copy assignment
 */
QwtScaleDiv& QwtScaleDiv::operator=(const QwtScaleDiv&) = default;

/**
 * @brief Change the interval
 *
 * @param lowerBound First boundary
 * @param upperBound Second boundary
 *
 * @note lowerBound might be greater than upperBound for inverted scales
 *
 */
void QwtScaleDiv::setInterval(double lowerBound, double upperBound)
{
    m_lowerBound = lowerBound;
    m_upperBound = upperBound;
}

/**
 * @brief Change the interval
 *
 * @param interval Interval
 *
 */
void QwtScaleDiv::setInterval(const QwtInterval& interval)
{
    m_lowerBound = interval.minValue();
    m_upperBound = interval.maxValue();
}

/**
 * @brief Get the interval from lowerBound to upperBound
 * @return Interval from lowerBound to upperBound
 *
 */
QwtInterval QwtScaleDiv::interval() const
{
    return QwtInterval(m_lowerBound, m_upperBound);
}

/**
 * @brief Set the first boundary
 *
 * @param lowerBound First boundary
 * @sa lowerBound(), setUpperBound()
 *
 */
void QwtScaleDiv::setLowerBound(double lowerBound) noexcept
{
    m_lowerBound = lowerBound;
}

/**
 * @brief Get the first boundary
 * @return First boundary
 * @sa upperBound()
 *
 */
double QwtScaleDiv::lowerBound() const noexcept
{
    return m_lowerBound;
}

/**
 * @brief Set the second boundary
 *
 * @param upperBound Second boundary
 * @sa upperBound(), setLowerBound()
 *
 */
void QwtScaleDiv::setUpperBound(double upperBound) noexcept
{
    m_upperBound = upperBound;
}

/**
 * @brief Get the upper bound
 * @return Upper bound
 * @sa lowerBound()
 *
 */
double QwtScaleDiv::upperBound() const noexcept
{
    return m_upperBound;
}

/**
 * @brief Get the range (upperBound - lowerBound)
 * @return upperBound() - lowerBound()
 *
 */
double QwtScaleDiv::range() const noexcept
{
    return m_upperBound - m_lowerBound;
}

/**
 * @brief Equality operator
 * @return true if this instance is equal to other
 *
 */
bool QwtScaleDiv::operator==(const QwtScaleDiv& other) const
{
    if (m_lowerBound != other.m_lowerBound || m_upperBound != other.m_upperBound) {
        return false;
    }

    for (int i = 0; i < NTickTypes; i++) {
        if (m_ticks[ i ] != other.m_ticks[ i ])
            return false;
    }

    return true;
}

/**
 * @brief Inequality operator
 * @return true if this instance is not equal to other
 *
 */
bool QwtScaleDiv::operator!=(const QwtScaleDiv& other) const
{
    return (!(*this == other));
}

/**
 * @brief Fuzzy comparison
 * @param other Other scale division
 * @return true if this instance is approximately equal to other
 *
 */
bool QwtScaleDiv::fuzzyCompare(const QwtScaleDiv& other) const
{
    if (qFuzzyCompare(m_lowerBound, other.m_lowerBound) && qFuzzyCompare(m_upperBound, other.m_upperBound)) {
        for (int i = 0; i < NTickTypes; i++) {
            if (!fuzzyRangeEqual(
                    m_ticks[ i ].begin(), m_ticks[ i ].end(), other.m_ticks[ i ].begin(), other.m_ticks[ i ].end())) {
                return false;
            }
        }
        return true;
    }
    return false;
}

/**
 * @brief Check if the scale division is empty
 * @return true if lowerBound() == upperBound()
 *
 */
bool QwtScaleDiv::isEmpty() const
{
    return (m_lowerBound == m_upperBound);
}

/**
 * @brief Check if the scale division is increasing
 * @return true if lowerBound() <= upperBound()
 *
 */
bool QwtScaleDiv::isIncreasing() const
{
    return m_lowerBound <= m_upperBound;
}

/**
 * @brief Check if a value is between lowerBound() and upperBound()
 *
 * @param value Value
 * @return true if value is within bounds
 *
 */
bool QwtScaleDiv::contains(double value) const
{
    const double min = qMin(m_lowerBound, m_upperBound);
    const double max = qMax(m_lowerBound, m_upperBound);

    return value >= min && value <= max;
}

/**
 * @brief Invert the scale division
 * @sa inverted()
 *
 */
void QwtScaleDiv::invert()
{
    qSwap(m_lowerBound, m_upperBound);

    for (int i = 0; i < NTickTypes; i++) {
        QList< double >& ticks = m_ticks[ i ];

        const int size  = ticks.count();
        const int size2 = size / 2;

        for (int j = 0; j < size2; j++)
            qSwap(ticks[ j ], ticks[ size - 1 - j ]);
    }
}

/**
 * @brief Get a scale division with inverted boundaries and ticks
 * @return A scale division with inverted boundaries and ticks
 * @sa invert()
 *
 */
QwtScaleDiv QwtScaleDiv::inverted() const
{
    QwtScaleDiv other = *this;
    other.invert();

    return other;
}

/**
 * @brief Return a scale division with an interval [lowerBound, upperBound]
 *        where all ticks outside this interval are removed
 *
 * @param lowerBound Lower bound
 * @param upperBound Upper bound
 *
 * @return Scale division with all ticks inside of the given interval
 *
 * @note lowerBound might be greater than upperBound for inverted scales
 *
 */
QwtScaleDiv QwtScaleDiv::bounded(double lowerBound, double upperBound) const
{
    const double min = qMin(lowerBound, upperBound);
    const double max = qMax(lowerBound, upperBound);

    QwtScaleDiv sd;
    sd.setInterval(lowerBound, upperBound);

    for (int tickType = 0; tickType < QwtScaleDiv::NTickTypes; tickType++) {
        const QList< double >& ticks = m_ticks[ tickType ];

        QList< double > boundedTicks;
        for (int i = 0; i < ticks.size(); i++) {
            const double tick = ticks[ i ];
            if (tick >= min && tick <= max)
                boundedTicks += tick;
        }

        sd.setTicks(tickType, boundedTicks);
    }

    return sd;
}

/**
 * @brief Assign ticks
 *
 * @param tickType MinorTick, MediumTick or MajorTick
 * @param ticks Values of the tick positions
 *
 */
void QwtScaleDiv::setTicks(int tickType, const QList< double >& ticks)
{
    if (tickType >= 0 && tickType < NTickTypes) {
        // Normalize: the tick lists are sorted and duplicate free. Engines can
        // produce duplicated values (the date engine collapsing rounded minor
        // ticks), which would otherwise be painted - and queried by snapping
        // code - twice.
        QList< double > normalized = ticks;
        std::sort(normalized.begin(), normalized.end());
        normalized.erase(std::unique(normalized.begin(), normalized.end()), normalized.end());

        m_ticks[ tickType ] = normalized;
    }
}

/**
 * @brief Return a list of ticks
 *
 * @param tickType MinorTick, MediumTick or MajorTick
 * @return Tick list
 *
 */
QList< double > QwtScaleDiv::ticks(int tickType) const
{
    if (tickType >= 0 && tickType < NTickTypes)
        return m_ticks[ tickType ];

    return QList< double >();
}

#ifndef QT_NO_DEBUG_STREAM

#include <qdebug.h>

QDebug operator<<(QDebug debug, const QwtScaleDiv& scaleDiv)
{
    debug << scaleDiv.lowerBound() << "<->" << scaleDiv.upperBound();
    debug << "Major: " << scaleDiv.ticks(QwtScaleDiv::MajorTick);
    debug << "Medium: " << scaleDiv.ticks(QwtScaleDiv::MediumTick);
    debug << "Minor: " << scaleDiv.ticks(QwtScaleDiv::MinorTick);

    return debug;
}

#endif
