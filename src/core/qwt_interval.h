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

#ifndef QWT_INTERVAL_H
#define QWT_INTERVAL_H

#include "qwtcore_global.h"
#include <qmetatype.h>

/**
 * @brief A class representing an interval
 * @details The interval is represented by 2 doubles, the lower and the upper limit.
 */

class QWTCORE_EXPORT QwtInterval
{
public:
    /*!
       @brief Flag indicating if a border is included or excluded
       @sa setBorderFlags(), borderFlags()
     */
    enum BorderFlag
    {
        //! Min/Max values are inside the interval
        IncludeBorders = 0x00,

        //! Min value is not included in the interval
        ExcludeMinimum = 0x01,

        //! Max value is not included in the interval
        ExcludeMaximum = 0x02,

        //! Min/Max values are not included in the interval
        ExcludeBorders = ExcludeMinimum | ExcludeMaximum
    };

    //! Border flags
    Q_DECLARE_FLAGS(BorderFlags, BorderFlag)

    Q_DECL_CONSTEXPR QwtInterval() noexcept;
    Q_DECL_CONSTEXPR QwtInterval(double minValue, double maxValue, BorderFlags = IncludeBorders) noexcept;

    // Assign the limits of the interval
    void setInterval(double minValue, double maxValue, BorderFlags = IncludeBorders) noexcept;

    // Normalize the limits of the interval
    QwtInterval normalized() const;
    // Invert the interval
    QwtInterval inverted() const;
    // Limit the interval, keeping the border modes
    QwtInterval limited(double lowerBound, double upperBound) const;

    // Compare two intervals for equality
    bool operator==(const QwtInterval&) const;
    // Compare two intervals for inequality
    bool operator!=(const QwtInterval&) const;

    void setBorderFlags(BorderFlags) noexcept;
    Q_DECL_CONSTEXPR BorderFlags borderFlags() const noexcept;

    Q_DECL_CONSTEXPR double minValue() const noexcept;
    Q_DECL_CONSTEXPR double maxValue() const noexcept;
    Q_DECL_CONSTEXPR double centerValue() const noexcept;

    Q_DECL_CONSTEXPR double width() const noexcept;
    Q_DECL_CONSTEXPR long double widthL() const noexcept;

    void setMinValue(double) noexcept;
    void setMaxValue(double) noexcept;

    // Test if a value is inside the interval
    bool contains(double value) const;
    // Test if an interval is inside the interval
    bool contains(const QwtInterval&) const;

    // Test if two intervals overlap
    bool intersects(const QwtInterval&) const;
    // Intersect two intervals
    QwtInterval intersect(const QwtInterval&) const;
    // Unite two intervals
    QwtInterval unite(const QwtInterval&) const;

    // Union of two intervals
    QwtInterval operator|(const QwtInterval&) const;
    // Intersection of two intervals
    QwtInterval operator&(const QwtInterval&) const;

    // Unite this interval with the given interval
    QwtInterval& operator|=(const QwtInterval&);
    // Intersect this interval with the given interval
    QwtInterval& operator&=(const QwtInterval&);

    // Extend the interval with a value
    QwtInterval extend(double value) const;
    // Extend an interval with a value
    QwtInterval operator|(double) const;
    // Extend an interval with a value
    QwtInterval& operator|=(double);

    Q_DECL_CONSTEXPR bool isValid() const noexcept;
    Q_DECL_CONSTEXPR bool isNull() const noexcept;
    void invalidate() noexcept;

    // Symmetrize the interval around a value
    QwtInterval symmetrize(double value) const;

private:
    double m_minValue { 0.0 };
    double m_maxValue { -1.0 };
    BorderFlags m_borderFlags { IncludeBorders };
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtInterval::BorderFlags)
Q_DECLARE_METATYPE(QwtInterval)
Q_DECLARE_TYPEINFO(QwtInterval, Q_MOVABLE_TYPE);

/**
 * @brief Default Constructor
 * @details Creates an invalid interval [0.0, -1.0]
 * @sa setInterval(), isValid()
 */
inline Q_DECL_CONSTEXPR QwtInterval::QwtInterval() noexcept : m_minValue(0.0), m_maxValue(-1.0), m_borderFlags(IncludeBorders)
{
}

/**
 * @brief Constructor with min/max values
 * @param minValue Minimum value
 * @param maxValue Maximum value
 * @param borderFlags Include/Exclude borders
 */
inline Q_DECL_CONSTEXPR QwtInterval::QwtInterval(double minValue, double maxValue, BorderFlags borderFlags) noexcept
    : m_minValue(minValue), m_maxValue(maxValue), m_borderFlags(borderFlags)
{
}

/**
 * @brief Assign the limits of the interval
 * @param minValue Minimum value
 * @param maxValue Maximum value
 * @param borderFlags Include/Exclude borders
 */
inline void QwtInterval::setInterval(double minValue, double maxValue, BorderFlags borderFlags) noexcept
{
    m_minValue    = minValue;
    m_maxValue    = maxValue;
    m_borderFlags = borderFlags;
}

/**
 * @brief Change the border flags
 * @param borderFlags Or'd BorderMode flags
 * @sa borderFlags()
 */
inline void QwtInterval::setBorderFlags(BorderFlags borderFlags) noexcept
{
    m_borderFlags = borderFlags;
}

/**
 * @return Border flags
 * @sa setBorderFlags()
 */
inline Q_DECL_CONSTEXPR QwtInterval::BorderFlags QwtInterval::borderFlags() const noexcept
{
    return m_borderFlags;
}

/**
 * @brief Assign the lower limit of the interval
 * @param minValue Minimum value
 */
inline void QwtInterval::setMinValue(double minValue) noexcept
{
    m_minValue = minValue;
}

/**
 * @brief Assign the upper limit of the interval
 * @param maxValue Maximum value
 */
inline void QwtInterval::setMaxValue(double maxValue) noexcept
{
    m_maxValue = maxValue;
}

/**
 * @return Minimum value of the interval
 * @sa setMinValue()
 */
inline Q_DECL_CONSTEXPR double QwtInterval::minValue() const noexcept
{
    return m_minValue;
}

/**
 * @return Maximum value of the interval
 * @sa setMaxValue()
 */
inline Q_DECL_CONSTEXPR double QwtInterval::maxValue() const noexcept
{
    return m_maxValue;
}

/**
 * @return Center of the interval
 * @sa width()
 */
inline Q_DECL_CONSTEXPR double QwtInterval::centerValue() const noexcept
{
    return isValid() ? (m_minValue + (m_maxValue - m_minValue) * 0.5) : 0.0;
}
/**
 * @brief Check if the interval is valid
 * @details A interval is valid when minValue() <= maxValue().
 * In case of QwtInterval::ExcludeBorders it is true
 * when minValue() < maxValue()
 * @return True, if the interval is valid
 * @sa isValid()
 */
inline Q_DECL_CONSTEXPR bool QwtInterval::isValid() const noexcept
{
    return ((m_borderFlags & ExcludeBorders) == 0) ? (m_minValue <= m_maxValue) : (m_minValue < m_maxValue);
}

/**
 * @brief Return the width of an interval
 * @details The width of invalid intervals is 0.0, otherwise the result is maxValue() - minValue().
 * @return Interval width
 * @sa isValid()
 */
inline Q_DECL_CONSTEXPR double QwtInterval::width() const noexcept
{
    return isValid() ? (m_maxValue - m_minValue) : 0.0;
}

/**
 * @brief Return the width of an interval as long double
 * @details The width of invalid intervals is 0.0, otherwise the result is
 *          maxValue() - minValue().
 * @return Interval width
 * @sa isValid()
 */
inline Q_DECL_CONSTEXPR long double QwtInterval::widthL() const noexcept
{
    return isValid() ? static_cast< long double >(m_maxValue) - static_cast< long double >(m_minValue) : 0.0L;
}

/**
 * @brief Intersection of two intervals
 * @param[in] other Interval to intersect with
 * @return Intersection of this and other
 * @sa intersect()
 */
inline QwtInterval QwtInterval::operator&(const QwtInterval& other) const
{
    return intersect(other);
}

/**
 * @brief Union of two intervals
 * @param[in] other Interval to unite with
 * @return Union of this and other
 * @sa unite()
 */
inline QwtInterval QwtInterval::operator|(const QwtInterval& other) const
{
    return unite(other);
}

/**
 * @brief Compare two intervals
 * @param[in] other Interval to compare with
 * @return True when this and other are equal
 */
inline bool QwtInterval::operator==(const QwtInterval& other) const
{
    return (m_minValue == other.m_minValue) && (m_maxValue == other.m_maxValue) && (m_borderFlags == other.m_borderFlags);
}

/**
 * @brief Compare two intervals
 * @param[in] other Interval to compare with
 * @return True when this and other are not equal
 */
inline bool QwtInterval::operator!=(const QwtInterval& other) const
{
    return (!(*this == other));
}

/**
 * @brief Extend an interval
 * @param[in] value Value to extend with
 * @return Extended interval
 * @sa extend()
 */
inline QwtInterval QwtInterval::operator|(double value) const
{
    return extend(value);
}

/**
 * @brief Check if interval is null
 * @return True if isValid() && (minValue() >= maxValue())
 */
inline Q_DECL_CONSTEXPR bool QwtInterval::isNull() const noexcept
{
    return isValid() && m_minValue >= m_maxValue;
}

/**
 * @brief Invalidate the interval
 * @details The limits are set to interval [0.0, -1.0]
 * @sa isValid()
 */
inline void QwtInterval::invalidate() noexcept
{
    m_minValue = 0.0;
    m_maxValue = -1.0;
}

#ifndef QT_NO_DEBUG_STREAM
QWTCORE_EXPORT QDebug operator<<(QDebug, const QwtInterval&);
#endif

#endif
