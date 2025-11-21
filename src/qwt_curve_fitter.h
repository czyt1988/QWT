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

#ifndef QWT_CURVE_FITTER_H
#define QWT_CURVE_FITTER_H

#include "qwt_global.h"

class QPainterPath;
class QPolygonF;

/*!
   \brief Abstract base class for a curve fitter
 */
class QWT_EXPORT QwtCurveFitter
{
  public:
    /*!
       \brief Preferred mode of the fitting algorithm

       Even if a QPainterPath can always be created from a QPolygonF
       the overhead of the conversion can be avoided by indicating
       the preference of the implementation to the application
       code.
     */
    enum Mode
    {
        /*!
           The fitting algorithm creates a polygon - the implementation
           of fitCurvePath() simply wraps the polygon into a path.

           \sa QwtWeedingCurveFitter
         */
        Polygon,

        /*!
           The fitting algorithm creates a painter path - the implementation
           of fitCurve() extracts a polygon from the path.

           \sa QwtSplineCurveFitter
         */
        Path
    };

    virtual ~QwtCurveFitter();

    Mode mode() const;

    /*!
        Find a curve which has the best fit to a series of data points

        \param polygon Series of data points
        \return Curve points

        \sa fitCurvePath()
     */
    virtual QPolygonF fitCurve( const QPolygonF& polygon ) const = 0;

    /*!
        Find a curve path which has the best fit to a series of data points

        \param polygon Series of data points
        \return Curve path

        \sa fitCurve()
     */
    virtual QPainterPath fitCurvePath( const QPolygonF& polygon ) const = 0;

  protected:
    explicit QwtCurveFitter( Mode mode );

  private:
    Q_DISABLE_COPY(QwtCurveFitter)

    const Mode m_mode;
};

#endif
