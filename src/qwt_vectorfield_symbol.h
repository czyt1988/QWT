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

#ifndef QWT_VECTOR_FIELD_SYMBOL_H
#define QWT_VECTOR_FIELD_SYMBOL_H

#include "qwt_global.h"

class QPainter;
class QPainterPath;

/*!
    Defines abstract interface for arrow drawing routines.

    Arrow needs to be drawn horizontally with arrow tip at coordinate 0,0.
    arrowLength() shall return the entire length of the arrow (needed
    to translate the arrow for tail/centered alignment).
    setArrowLength() defines arror length in pixels (screen coordinates). It
    can be implemented to adjust other geometric properties such as
    the head size and width of the arrow. It is _always_ called before
    paint().

    A new arrow implementation can be set with QwtPlotVectorField::setArrowSymbol(), whereby
    ownership is transferred to the plot field.
 */
class QWT_EXPORT QwtVectorFieldSymbol
{
  public:
    QwtVectorFieldSymbol();
    virtual ~QwtVectorFieldSymbol();

    /*!
        Set the length of the symbol/arrow
        \sa length()
     */
    virtual void setLength( qreal length ) = 0;

    /*!
        \return length of the symbol/arrow
        \sa setLength()
     */
    virtual qreal length() const = 0;

    //! Draw the symbol/arrow
    virtual void paint( QPainter* ) const = 0;

  private:
    Q_DISABLE_COPY(QwtVectorFieldSymbol)
};

/*!
    Arrow implementation that draws a filled arrow with outline, using
    a triangular head of constant width.
 */
class QWT_EXPORT QwtVectorFieldArrow : public QwtVectorFieldSymbol
{
  public:
    QwtVectorFieldArrow( qreal headWidth = 6.0, qreal tailWidth = 1.0 );
    virtual ~QwtVectorFieldArrow() QWT_OVERRIDE;

    virtual void setLength( qreal length ) QWT_OVERRIDE;
    virtual qreal length() const QWT_OVERRIDE;

    virtual void paint( QPainter* ) const QWT_OVERRIDE;

  private:
    class PrivateData;
    PrivateData* m_data;
};

/*!
    Arrow implementation that only used lines, with optionally a filled arrow or only
    lines.
 */
class QWT_EXPORT QwtVectorFieldThinArrow : public QwtVectorFieldSymbol
{
  public:
    QwtVectorFieldThinArrow( qreal headWidth = 6.0 );
    virtual ~QwtVectorFieldThinArrow() QWT_OVERRIDE;

    virtual void setLength( qreal length ) QWT_OVERRIDE;
    virtual qreal length() const QWT_OVERRIDE;

    virtual void paint( QPainter* ) const QWT_OVERRIDE;

  private:
    class PrivateData;
    PrivateData* m_data;
};

#endif
