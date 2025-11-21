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

#ifndef QWT_PIXEL_MATRIX_H
#define QWT_PIXEL_MATRIX_H

#include "qwt_global.h"

#include <qbitarray.h>
#include <qrect.h>

/*!
   \brief A bit field corresponding to the pixels of a rectangle

   QwtPixelMatrix is intended to filter out duplicates in an
   unsorted array of points.
 */
class QWT_EXPORT QwtPixelMatrix : public QBitArray
{
  public:
    explicit QwtPixelMatrix( const QRect& rect );
    ~QwtPixelMatrix();

    void setRect( const QRect& rect );
    QRect rect() const;

    bool testPixel( int x, int y ) const;
    bool testAndSetPixel( int x, int y, bool on );

    int index( int x, int y ) const;

  private:
    QRect m_rect;
};

/*!
   \brief Test if a pixel has been set

   \param x X-coordinate
   \param y Y-coordinate

   \return true, when pos is outside of rect(), or when the pixel
          has already been set.
 */
inline bool QwtPixelMatrix::testPixel( int x, int y ) const
{
    const int idx = index( x, y );
    return ( idx >= 0 ) ? testBit( idx ) : true;
}

/*!
   \brief Set a pixel and test if a pixel has been set before

   \param x X-coordinate
   \param y Y-coordinate
   \param on Set/Clear the pixel

   \return true, when pos is outside of rect(), or when the pixel
          was set before.
 */
inline bool QwtPixelMatrix::testAndSetPixel( int x, int y, bool on )
{
    const int idx = index( x, y );
    if ( idx < 0 )
        return true;

    const bool onBefore = testBit( idx );
    setBit( idx, on );

    return onBefore;
}

/*!
   \brief Calculate the index in the bit field corresponding to a position

   \param x X-coordinate
   \param y Y-coordinate
   \return Index, when rect() contains pos - otherwise -1.
 */
inline int QwtPixelMatrix::index( int x, int y ) const
{
    const int dx = x - m_rect.x();
    if ( dx < 0 || dx >= m_rect.width() )
        return -1;

    const int dy = y - m_rect.y();
    if ( dy < 0 || dy >= m_rect.height() )
        return -1;

    return dy * m_rect.width() + dx;
}

#endif
