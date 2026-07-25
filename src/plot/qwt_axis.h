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

#ifndef QWT_AXIS_H
#define QWT_AXIS_H

#include "qwt_global.h"

/**
 * @brief Enums and methods for axes
 *
 */
namespace QwtAxis
{
/**
 * @brief Axis position
 *
 */
enum Position
{
    //! Y axis left of the canvas
    YLeft = 0,

    //! Y axis right of the canvas
    YRight = 1,

    //! X axis below the canvas
    XBottom = 2,

    //! X axis above the canvas
    XTop = 3
};

/**
 * @brief Number of axis positions
 *
 */
enum
{
    AxisPositions = XTop + 1
};

Q_DECL_CONSTEXPR bool isValid(int axisPos) noexcept;
Q_DECL_CONSTEXPR bool isYAxis(int axisPos) noexcept;
Q_DECL_CONSTEXPR bool isXAxis(int axisPos) noexcept;
}

// Return true, when axisPos is in the valid range [ YLeft, XTop ]
inline Q_DECL_CONSTEXPR bool QwtAxis::isValid(int axisPos) noexcept
{
    return (axisPos >= 0 && axisPos < AxisPositions);
}

// Return true, when axisPos is XBottom or XTop
inline Q_DECL_CONSTEXPR bool QwtAxis::isXAxis(int axisPos) noexcept
{
    return (axisPos == XBottom) || (axisPos == XTop);
}

// Return true, when axisPos is YLeft or YRight
inline Q_DECL_CONSTEXPR bool QwtAxis::isYAxis(int axisPos) noexcept
{
    return (axisPos == YLeft) || (axisPos == YRight);
}

#endif
