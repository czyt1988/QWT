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

#include "qwt_pixel_matrix.h"

/*!
   \brief Constructor

   \param rect Bounding rectangle for the matrix
 */
QwtPixelMatrix::QwtPixelMatrix( const QRect& rect )
    : QBitArray( qMax( rect.width() * rect.height(), 0 ) )
    , m_rect( rect )
{
}

//! Destructor
QwtPixelMatrix::~QwtPixelMatrix()
{
}

/*!
    Set the bounding rectangle of the matrix

    \param rect Bounding rectangle

    \note All bits are cleared
 */
void QwtPixelMatrix::setRect( const QRect& rect )
{
    if ( rect != m_rect )
    {
        m_rect = rect;
        const int sz = qMax( rect.width() * rect.height(), 0 );
        resize( sz );
    }

    fill( false );
}

//! \return Bounding rectangle
QRect QwtPixelMatrix::rect() const
{
    return m_rect;
}
