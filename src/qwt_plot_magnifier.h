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

#ifndef QWT_PLOT_MAGNIFIER_H
#define QWT_PLOT_MAGNIFIER_H

#include "qwt_global.h"
#include "qwt_axis_id.h"
#include "qwt_magnifier.h"

class QwtPlot;

/*!
   \brief QwtPlotMagnifier provides zooming, by magnifying in steps.

   Using QwtPlotMagnifier a plot can be zoomed in/out in steps using
   keys, the mouse wheel or moving a mouse button in vertical direction.

   Together with QwtPlotZoomer and QwtPlotPanner it is possible to implement
   individual and powerful navigation of the plot canvas.

   \sa QwtPlotZoomer, QwtPlotPanner, QwtPlot
 */
class QWT_EXPORT QwtPlotMagnifier : public QwtMagnifier
{
    Q_OBJECT

  public:
    explicit QwtPlotMagnifier( QWidget* );
    virtual ~QwtPlotMagnifier();

    void setAxisEnabled( QwtAxisId, bool on );
    bool isAxisEnabled( QwtAxisId ) const;

    QWidget* canvas();
    const QWidget* canvas() const;

    QwtPlot* plot();
    const QwtPlot* plot() const;

  public Q_SLOTS:
    virtual void rescale( double factor ) QWT_OVERRIDE;

  private:
    class PrivateData;
    PrivateData* m_data;
};

#endif
