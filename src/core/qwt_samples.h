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

#ifndef QWT_SAMPLES_H
#define QWT_SAMPLES_H

#include "qwtcore_global.h"
#include "qwt_interval.h"

#include <qvector.h>
#include <qrect.h>
#include <qnumeric.h>

/**
 * @brief Base class for statistical samples with position and range
 * @details Provides common fields for samples that have a position on one axis
 *          and a statistical range on the other axis (used by boxplots, OHLC charts).
 *
 */
class QWTCORE_EXPORT QwtStatisticalSample
{
public:
    /**
     * @brief Default constructor
     * @details All values set to 0.0
     */
    QwtStatisticalSample(double position = 0.0);

    //! Position on the "time" axis (x for vertical, y for horizontal orientation)
    double position;

    //! Lower bound of the statistical range
    double lower;

    //! Upper bound of the statistical range
    double upper;

    //! Central reference value
    double center;
};

inline QwtStatisticalSample::QwtStatisticalSample(double pos) : position(pos), lower(0.0), upper(0.0), center(0.0)
{
}

/**
 * @brief A sample of the types (x1-x2, y) or (x, y1-y2)
 * @details Used for interval-based samples where one dimension has a range
 *          instead of a single value.
 */
class QWTCORE_EXPORT QwtIntervalSample
{
public:
    QwtIntervalSample();
    QwtIntervalSample(double, const QwtInterval&);
    QwtIntervalSample(double value, double min, double max);

    bool operator==(const QwtIntervalSample&) const;
    bool operator!=(const QwtIntervalSample&) const;

    //! Value
    double value;

    //! Interval
    QwtInterval interval;
};

/**
 * @brief Default constructor
 * @details The value is set to 0.0, the interval is invalid
 */
inline QwtIntervalSample::QwtIntervalSample() : value(0.0)
{
}

/**
 * @brief Constructor with value and interval
 * @param v Value
 * @param intv Interval
 */
inline QwtIntervalSample::QwtIntervalSample(double v, const QwtInterval& intv) : value(v), interval(intv)
{
}

/**
 * @brief Constructor with value and min/max
 * @param v Value
 * @param min Minimum value
 * @param max Maximum value
 */
inline QwtIntervalSample::QwtIntervalSample(double v, double min, double max) : value(v), interval(min, max)
{
}

/**
 * @brief Equality comparison operator
 */
inline bool QwtIntervalSample::operator==(const QwtIntervalSample& other) const
{
    return value == other.value && interval == other.interval;
}

/**
 * @brief Inequality comparison operator
 */
inline bool QwtIntervalSample::operator!=(const QwtIntervalSample& other) const
{
    return !(*this == other);
}

/**
 * @brief A sample of the types (x1...xn, y) or (x, y1..yn)
 * @details Used for set-based samples where one dimension has multiple values.
 *          Commonly used for bar charts with multiple bars at each position.
 */
class QWTCORE_EXPORT QwtSetSample
{
public:
    QwtSetSample();
    explicit QwtSetSample(double, const QVector< double >& = QVector< double >());

    bool operator==(const QwtSetSample& other) const;
    bool operator!=(const QwtSetSample& other) const;

    double added() const;

    //! value
    double value;

    //! Vector of values associated to value
    QVector< double > set;
};

/**
 * @brief Default constructor
 * @details The value is set to 0.0
 */
inline QwtSetSample::QwtSetSample() : value(0.0)
{
}

/**
 * @brief Constructor with value and set
 * @param v Value
 * @param s Set of values
 */
inline QwtSetSample::QwtSetSample(double v, const QVector< double >& s) : value(v), set(s)
{
}

/**
 * @brief Equality comparison operator
 */
inline bool QwtSetSample::operator==(const QwtSetSample& other) const
{
    return value == other.value && set == other.set;
}

/**
 * @brief Inequality comparison operator
 */
inline bool QwtSetSample::operator!=(const QwtSetSample& other) const
{
    return !(*this == other);
}

/**
 * @brief Return all values of the set added together
 * @return Sum of all values in the set
 */
inline double QwtSetSample::added() const
{
    double y = 0.0;
    for (int i = 0; i < set.size(); i++)
        y += set[ i ];

    return y;
}

/**
 * @brief Open-High-Low-Close sample used in financial charts
 * @details In financial charts the movement of a price in a time interval is often
 *          represented by the opening/closing prices and the lowest/highest prices
 *          in this interval.
 * @sa QwtTradingChartData
 */
class QWTCORE_EXPORT QwtOHLCSample
{
public:
    QwtOHLCSample(double time = 0.0, double open = 0.0, double high = 0.0, double low = 0.0, double close = 0.0);

    QwtInterval boundingInterval() const;

    bool isValid() const;

    /*!
       Time of the sample, usually a number representing
       a specific interval - like a day.
     */
    double time;

    //! Opening price
    double open;

    //! Highest price
    double high;

    //! Lowest price
    double low;

    //! Closing price
    double close;
};

/**
 * @brief Constructor with all OHLC values
 * @param t Time value
 * @param o Open value
 * @param h High value
 * @param l Low value
 * @param c Close value
 */
inline QwtOHLCSample::QwtOHLCSample(double t, double o, double h, double l, double c)
    : time(t), open(o), high(h), low(l), close(c)
{
}

/**
 * @brief Check if a sample is valid
 * @details A sample is valid, when all of the following checks are true:
 *          - low <= high
 *          - low <= open <= high
 *          - low <= close <= high
 * @return True, when the sample is valid
 */
inline bool QwtOHLCSample::isValid() const
{
    return (low <= high) && (open >= low) && (open <= high) && (close >= low) && (close <= high);
}

/**
 * @brief Calculate the bounding interval of the OHLC values
 * @details For valid samples the limits of this interval are always low/high.
 * @return Bounding interval
 * @sa isValid()
 */
inline QwtInterval QwtOHLCSample::boundingInterval() const
{
    double minY = open;
    minY        = qMin(minY, high);
    minY        = qMin(minY, low);
    minY        = qMin(minY, close);

    double maxY = open;
    maxY        = qMax(maxY, high);
    maxY        = qMax(maxY, low);
    maxY        = qMax(maxY, close);

    return QwtInterval(minY, maxY);
}

/**
 * @brief Sample used in vector fields
 * @details A vector field sample is a position and a vector - usually
 *          representing some direction and magnitude - attached to this position.
 * @sa QwtVectorFieldData
 */
class QWTCORE_EXPORT QwtVectorFieldSample
{
public:
    QwtVectorFieldSample(double x = 0.0, double y = 0.0, double vx = 0.0, double vy = 0.0);

    QwtVectorFieldSample(const QPointF& pos, double vx = 0.0, double vy = 0.0);

    QPointF pos() const;

    bool isNull() const;

    //! x coordinate of the position
    double x;

    //! y coordinate of the position
    double y;

    //! x coordinate of the vector
    double vx;

    //! y coordinate of the vector
    double vy;
};

/**
 * @brief Constructor with position and vector coordinates
 * @param posX x coordinate of the position
 * @param posY y coordinate of the position
 * @param vectorX x coordinate of the vector
 * @param vectorY y coordinate of the vector
 */
inline QwtVectorFieldSample::QwtVectorFieldSample(double posX, double posY, double vectorX, double vectorY)
    : x(posX), y(posY), vx(vectorX), vy(vectorY)
{
}

/**
 * @brief Constructor with QPointF position and vector coordinates
 * @param pos Position as QPointF
 * @param vectorX x coordinate of the vector
 * @param vectorY y coordinate of the vector
 */
inline QwtVectorFieldSample::QwtVectorFieldSample(const QPointF& pos, double vectorX, double vectorY)
    : x(pos.x()), y(pos.y()), vx(vectorX), vy(vectorY)
{
}

/**
 * @brief Return position as QPointF
 * @return x/y coordinates as QPointF
 */
inline QPointF QwtVectorFieldSample::pos() const
{
    return QPointF(x, y);
}

/**
 * @brief Check if the vector is null
 * @return true, if vx and vy are 0
 */
inline bool QwtVectorFieldSample::isNull() const
{
    return (vx == 0.0) && (vy == 0.0);
}

/**
 * @brief Sample for box-and-whisker plot (boxplot) visualization
 * @details Contains all statistical values needed to render a boxplot:
 *          whisker endpoints, quartiles, median, mean and outlier count.
 *          Actual outlier values are stored separately in QwtBoxOutlierSample.
 *
 */
class QWTCORE_EXPORT QwtBoxSample : public QwtStatisticalSample
{
public:
    /**
     * @brief Default constructor
     * @details All values set to 0.0, the mean to NaN (unknown)
     */
    QwtBoxSample(double position = 0.0);

    /**
     * @brief Full constructor with all statistical values
     * @param position Position on the axis
     * @param whiskerLower Lower whisker endpoint
     * @param q1 First quartile (25th percentile)
     * @param median Median value (50th percentile)
     * @param q3 Third quartile (75th percentile)
     * @param whiskerUpper Upper whisker endpoint
     * @details The mean is not part of the statistics of this constructor;
     *          it stays set to NaN, so hasMean() returns false until a value
     *          is assigned to the mean member.
     */
    QwtBoxSample(double position, double whiskerLower, double q1, double median, double q3, double whiskerUpper);

    /**
     * @brief Check if sample has valid ordering
     * @details Returns true if whiskerLower <= q1 <= median <= q3 <= whiskerUpper
     */
    bool isValid() const;

    /**
     * @brief Check if the sample carries a mean value
     * @details QwtBoxStatisticsCalculator always fills the mean; samples that
     *          were assembled manually (or the default constructed ones) have
     *          no mean until it is assigned.
     * @return true, when mean has been set to a finite value
     */
    bool hasMean() const;

    /**
     * @brief Get bounding interval including whiskers
     * @return Interval from whiskerLower to whiskerUpper
     */
    QwtInterval boundingInterval() const;

    /**
     * @brief Get box body interval (Q1 to Q3)
     * @return Interval from q1 to q3
     */
    QwtInterval boxInterval() const;

    //! Lower whisker endpoint
    double whiskerLower;

    //! First quartile (25th percentile)
    double q1;

    //! Median (50th percentile) - also stored in inherited 'center' field
    double median;

    //! Third quartile (75th percentile)
    double q3;

    //! Upper whisker endpoint
    double whiskerUpper;

    //! Arithmetic mean - NaN, when unknown (the default and full constructor)
    double mean;

    //! Number of outliers (stored separately, this is count only)
    int outlierCount;
};

inline QwtBoxSample::QwtBoxSample(double pos)
    : QwtStatisticalSample(pos), whiskerLower(0.0), q1(0.0), median(0.0), q3(0.0), whiskerUpper(0.0),
      mean(qQNaN()), outlierCount(0)
{
}

inline QwtBoxSample::QwtBoxSample(double pos, double wl, double q1v, double med, double q3v, double wu)
    : QwtStatisticalSample(pos), whiskerLower(wl), q1(q1v), median(med), q3(q3v), whiskerUpper(wu),
      mean(qQNaN()), outlierCount(0)
{
    center = median;
}

inline bool QwtBoxSample::isValid() const
{
    return (whiskerLower <= q1) && (q1 <= median) && (median <= q3) && (q3 <= whiskerUpper);
}

inline bool QwtBoxSample::hasMean() const
{
    return qIsFinite(mean);
}

inline QwtInterval QwtBoxSample::boundingInterval() const
{
    return QwtInterval(whiskerLower, whiskerUpper);
}

inline QwtInterval QwtBoxSample::boxInterval() const
{
    return QwtInterval(q1, q3);
}

/**
 * @brief Outlier values for a single boxplot position
 * @details Contains all outlier values associated with one box position.
 *          One QwtBoxOutlierSample corresponds to one QwtBoxSample.
 *
 */
class QWTCORE_EXPORT QwtBoxOutlierSample
{
public:
    /**
     * @brief Default constructor
     */
    QwtBoxOutlierSample(double boxPosition = 0.0);

    /**
     * @brief Constructor with position and outlier values
     * @param boxPosition Position matching parent QwtBoxSample
     * @param values All outlier values for this box
     */
    QwtBoxOutlierSample(double boxPosition, const QVector< double >& values);

    /**
     * @brief Constructor with move semantics
     */
    QwtBoxOutlierSample(double boxPosition, QVector< double >&& values);

    //! Check if no outliers present
    bool isEmpty() const
    {
        return values.isEmpty();
    }

    //! Get number of outliers
    int count() const
    {
        return values.size();
    }

    //! Position of the parent box (matches QwtBoxSample.position)
    double boxPosition;

    //! All outlier values for this box
    QVector< double > values;
};

inline QwtBoxOutlierSample::QwtBoxOutlierSample(double pos) : boxPosition(pos), values()
{
}

inline QwtBoxOutlierSample::QwtBoxOutlierSample(double pos, const QVector< double >& vals)
    : boxPosition(pos), values(vals)
{
}

inline QwtBoxOutlierSample::QwtBoxOutlierSample(double pos, QVector< double >&& vals)
    : boxPosition(pos), values(std::move(vals))
{
}

Q_DECLARE_METATYPE(QwtIntervalSample)
Q_DECLARE_METATYPE(QwtOHLCSample)
Q_DECLARE_METATYPE(QwtVectorFieldSample)
Q_DECLARE_METATYPE(QwtBoxSample)
Q_DECLARE_METATYPE(QwtSetSample)

#endif
