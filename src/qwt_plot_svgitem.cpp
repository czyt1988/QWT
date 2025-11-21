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

#include "qwt_plot_svgitem.h"
#include "qwt_text.h"
#include "qwt_graphic.h"

#include <qsvgrenderer.h>

/*!
   \brief Constructor
   \param title Title
 */
QwtPlotSvgItem::QwtPlotSvgItem( const QString& title )
    : QwtPlotGraphicItem( QwtText( title ) )
{
}

/*!
   \brief Constructor
   \param title Title
 */
QwtPlotSvgItem::QwtPlotSvgItem( const QwtText& title )
    : QwtPlotGraphicItem( title )
{
}

//! Destructor
QwtPlotSvgItem::~QwtPlotSvgItem()
{
}

/*!
   Load a SVG file

   \param rect Bounding rectangle
   \param fileName SVG file name

   \return true, if the SVG file could be loaded
 */
bool QwtPlotSvgItem::loadFile( const QRectF& rect,
    const QString& fileName )
{
    QwtGraphic graphic;

    QSvgRenderer renderer;

    const bool ok = renderer.load( fileName );
    if ( ok )
    {
        QPainter p( &graphic );
        renderer.render( &p );
    }

    setGraphic( rect, graphic );

    return ok;
}

/*!
   Load SVG data

   \param rect Bounding rectangle
   \param data in SVG format

   \return true, if the SVG data could be loaded
 */
bool QwtPlotSvgItem::loadData( const QRectF& rect,
    const QByteArray& data )
{
    QwtGraphic graphic;

    QSvgRenderer renderer;

    const bool ok = renderer.load( data );
    if ( ok )
    {
        QPainter p( &graphic );
        renderer.render( &p );
    }

    setGraphic( rect, graphic );

    return ok;
}
