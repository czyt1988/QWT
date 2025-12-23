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

#ifndef QWT_SCALE_DIV_H
#define QWT_SCALE_DIV_H

#include "qwt_global.h"
#include <qlist.h>

class QwtInterval;

/**
 * @brief A class representing a scale division/表示刻度划分的类
 *
 * A Qwt scale is defined by its boundaries and 3 list for the positions of the major, medium and minor ticks.
 *
 * The upperBound() might be smaller than the lowerBound() to indicate inverted scales.
 *
 * Scale divisions can be calculated from a QwtScaleEngine.
 *
 * Qwt 刻度由其边界以及分别表示主刻度、中刻度和次刻度位置的三个列表定义。
 *
 * upperBound() 可能小于 lowerBound()，以此表示刻度是反向的。
 *
 * 刻度划分可通过 QwtScaleEngine 计算得出。
 *
 * @sa QwtScaleEngine::divideScale(), QwtPlot::setAxisScaleDiv(), QwtAbstractSlider::setScaleDiv()
 */
class QWT_EXPORT QwtScaleDiv
{
public:
    //! Scale tick types
    enum TickType
    {
        //! No ticks
        NoTick = -1,

        //! Minor ticks
        MinorTick,

        //! Medium ticks
        MediumTick,

        //! Major ticks
        MajorTick,

        //! Number of valid tick types
        NTickTypes
    };

    explicit QwtScaleDiv(double lowerBound = 0.0, double upperBound = 0.0);

    explicit QwtScaleDiv(const QwtInterval&, QList< double >[ NTickTypes ]);

    explicit QwtScaleDiv(double lowerBound, double upperBound, QList< double >[ NTickTypes ]);

    explicit QwtScaleDiv(double lowerBound,
                         double upperBound,
                         const QList< double >& minorTicks,
                         const QList< double >& mediumTicks,
                         const QList< double >& majorTicks);

    bool operator==(const QwtScaleDiv&) const;
    bool operator!=(const QwtScaleDiv&) const;
    bool fuzzyCompare(const QwtScaleDiv& other) const;

    void setInterval(double lowerBound, double upperBound);
    void setInterval(const QwtInterval&);
    QwtInterval interval() const;

    void setLowerBound(double);
    double lowerBound() const;

    void setUpperBound(double);
    double upperBound() const;

    double range() const;

    bool contains(double value) const;

    void setTicks(int tickType, const QList< double >&);
    QList< double > ticks(int tickType) const;

    bool isEmpty() const;
    bool isIncreasing() const;

    void invert();
    QwtScaleDiv inverted() const;

    QwtScaleDiv bounded(double lowerBound, double upperBound) const;

private:
    double m_lowerBound;
    double m_upperBound;
    QList< double > m_ticks[ NTickTypes ];
};

Q_DECLARE_TYPEINFO(QwtScaleDiv, Q_MOVABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
QWT_EXPORT QDebug operator<<(QDebug, const QwtScaleDiv&);
#endif

#endif
