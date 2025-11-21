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

#ifndef QWT_SPLINE_BASIS_H
#define QWT_SPLINE_BASIS_H

#include "qwt_global.h"
#include "qwt_spline.h"

/*!
   \brief An approximation using a basis spline

   QwtSplineBasis approximates a set of points by a polynomials with C2 continuity
   ( = first and second derivatives are equal ) at the end points.

   The end points of the spline do not match the original points.
 */
class QWT_EXPORT QwtSplineBasis : public QwtSpline
{
  public:
    QwtSplineBasis();
    virtual ~QwtSplineBasis();

    virtual QPainterPath painterPath( const QPolygonF& ) const QWT_OVERRIDE;
    virtual uint locality() const QWT_OVERRIDE;
};

#endif
