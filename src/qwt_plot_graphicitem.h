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

#ifndef QWT_PLOT_GRAPHIC_ITEM_H
#define QWT_PLOT_GRAPHIC_ITEM_H

#include "qwt_global.h"
#include "qwt_plot_item.h"

#include <qstring.h>

/*!
   \brief A plot item, which displays
         a recorded sequence of QPainter commands

   QwtPlotGraphicItem renders a sequence of recorded painter commands
   into a specific plot area. Recording of painter commands can be
   done manually by QPainter or e.g. QSvgRenderer.

   \sa QwtPlotShapeItem, QwtPlotSvgItem
 */

class QWT_EXPORT QwtPlotGraphicItem : public QwtPlotItem
{
  public:
    explicit QwtPlotGraphicItem( const QString& title = QString() );
    explicit QwtPlotGraphicItem( const QwtText& title );

    virtual ~QwtPlotGraphicItem();

    void setGraphic( const QRectF& rect, const QwtGraphic& );
    QwtGraphic graphic() const;

    virtual QRectF boundingRect() const QWT_OVERRIDE;

    virtual void draw( QPainter*,
        const QwtScaleMap& xMap, const QwtScaleMap& yMap,
        const QRectF& canvasRect ) const QWT_OVERRIDE;

    virtual int rtti() const QWT_OVERRIDE;

  private:
    void init();

    class PrivateData;
    PrivateData* m_data;
};

#endif
