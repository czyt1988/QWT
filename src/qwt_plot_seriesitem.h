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

#ifndef QWT_PLOT_SERIES_ITEM_H
#define QWT_PLOT_SERIES_ITEM_H

#include "qwt_global.h"
#include "qwt_plot_item.h"
#include "qwt_series_store.h"

#include <qstring.h>

class QwtScaleDiv;

/*!
   \brief Base class for plot items representing a series of samples
 */
class QWT_EXPORT QwtPlotSeriesItem : public QwtPlotItem,
    public virtual QwtAbstractSeriesStore
{
  public:
    explicit QwtPlotSeriesItem( const QString& title = QString() );
    explicit QwtPlotSeriesItem( const QwtText& title );

    virtual ~QwtPlotSeriesItem();

    void setOrientation( Qt::Orientation );
    Qt::Orientation orientation() const;

    virtual void draw( QPainter*,
        const QwtScaleMap& xMap, const QwtScaleMap& yMap,
        const QRectF& canvasRect ) const QWT_OVERRIDE;

    /*!
       Draw a subset of the samples

       \param painter Painter
       \param xMap Maps x-values into pixel coordinates.
       \param yMap Maps y-values into pixel coordinates.
       \param canvasRect Contents rectangle of the canvas
       \param from Index of the first point to be painted
       \param to Index of the last point to be painted. If to < 0 the
             curve will be painted to its last point.
     */
    virtual void drawSeries( QPainter* painter,
        const QwtScaleMap& xMap, const QwtScaleMap& yMap,
        const QRectF& canvasRect, int from, int to ) const = 0;

    virtual QRectF boundingRect() const QWT_OVERRIDE;

    virtual void updateScaleDiv(
        const QwtScaleDiv&, const QwtScaleDiv& ) QWT_OVERRIDE;

  protected:
    virtual void dataChanged() QWT_OVERRIDE;

  private:
    class PrivateData;
    PrivateData* m_data;
};

#endif
