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

#ifndef QWT_SPLINE_PLEASING_H
#define QWT_SPLINE_PLEASING_H

#include "qwt_spline.h"

/*!
   \brief A spline with G1 continuity

   QwtSplinePleasing is some sort of cardinal spline, with
   non C1 continuous extra rules for narrow angles. It has a locality of 2.

   \note The algorithm is the one offered by a popular office package.
 */
class QWT_EXPORT QwtSplinePleasing : public QwtSplineG1
{
  public:
    QwtSplinePleasing();
    virtual ~QwtSplinePleasing();

    virtual uint locality() const QWT_OVERRIDE;

    virtual QPainterPath painterPath( const QPolygonF& ) const QWT_OVERRIDE;
    virtual QVector< QLineF > bezierControlLines( const QPolygonF& ) const QWT_OVERRIDE;
};

#endif
