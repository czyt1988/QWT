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

#ifndef QWT_PLOT_PANNER_H
#define QWT_PLOT_PANNER_H

#include "qwt_global.h"
#include "qwt_cache_panner.h"
#include "qwt_axis_id.h"

class QwtPlot;

/*!
   \brief QwtPlotPanner provides panning of a plot canvas

   QwtPlotPanner is a panner for a plot canvas, that
   adjusts the scales of the axes after dropping
   the canvas on its new position.

   Together with QwtPlotZoomer and QwtPlotMagnifier powerful ways
   of navigating on a QwtPlot widget can be implemented easily.

   \note The axes are not updated, while dragging the canvas
   \sa QwtPlotZoomer, QwtPlotMagnifier
 */
class QWT_EXPORT QwtPlotPanner : public QwtCachePanner
{
    Q_OBJECT

public:
    explicit QwtPlotPanner(QWidget*);
    virtual ~QwtPlotPanner();

    QWidget* canvas();
    const QWidget* canvas() const;

    QwtPlot* plot();
    const QwtPlot* plot() const;

    void setAxisEnabled(QwtAxisId axisId, bool on);
    bool isAxisEnabled(QwtAxisId) const;

public Q_SLOTS:
    virtual void moveCanvas(int dx, int dy);

protected:
    virtual QBitmap contentsMask() const QWT_OVERRIDE;
    virtual QPixmap grab() const QWT_OVERRIDE;

private:
    class PrivateData;
    PrivateData* m_data;
};

#endif
