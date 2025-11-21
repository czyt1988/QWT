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

#ifndef QWT_SPLINE_CURVE_FITTER_H
#define QWT_SPLINE_CURVE_FITTER_H

#include "qwt_curve_fitter.h"

class QwtSpline;

/*!
   \brief A curve fitter using a spline interpolation

   The default setting for the spline is a cardinal spline with
   uniform parametrization.

   \sa QwtSpline, QwtSplineLocal
 */
class QWT_EXPORT QwtSplineCurveFitter : public QwtCurveFitter
{
  public:
    QwtSplineCurveFitter();
    virtual ~QwtSplineCurveFitter();

    void setSpline( QwtSpline* );

    const QwtSpline* spline() const;
    QwtSpline* spline();

    virtual QPolygonF fitCurve( const QPolygonF& ) const QWT_OVERRIDE;
    virtual QPainterPath fitCurvePath( const QPolygonF& ) const QWT_OVERRIDE;

  private:
    QwtSpline* m_spline;
};

#endif
