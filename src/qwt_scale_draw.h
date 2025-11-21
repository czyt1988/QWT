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

#ifndef QWT_SCALE_DRAW_H
#define QWT_SCALE_DRAW_H

#include "qwt_global.h"
#include "qwt_abstract_scale_draw.h"

#include <qpoint.h>

class QTransform;
class QSizeF;
class QRectF;
class QRect;

/*!
   \brief A class for drawing scales

   QwtScaleDraw can be used to draw linear or logarithmic scales.
   A scale has a position, an alignment and a length, which can be specified .
   The labels can be rotated and aligned
   to the ticks using setLabelRotation() and setLabelAlignment().

   After a scale division has been specified as a QwtScaleDiv object
   using QwtAbstractScaleDraw::setScaleDiv(const QwtScaleDiv &s),
   the scale can be drawn with the QwtAbstractScaleDraw::draw() member.
 */
class QWT_EXPORT QwtScaleDraw : public QwtAbstractScaleDraw
{
  public:
    /*!
        Alignment of the scale draw
        \sa setAlignment(), alignment()
     */
    enum Alignment
    {
        //! The scale is below
        BottomScale,

        //! The scale is above
        TopScale,

        //! The scale is left
        LeftScale,

        //! The scale is right
        RightScale
    };

    QwtScaleDraw();
    virtual ~QwtScaleDraw();

    void getBorderDistHint( const QFont&, int& start, int& end ) const;
    int minLabelDist( const QFont& ) const;

    int minLength( const QFont& ) const;
    virtual double extent( const QFont& ) const QWT_OVERRIDE;

    void move( double x, double y );
    void move( const QPointF& );
    void setLength( double length );

    Alignment alignment() const;
    void setAlignment( Alignment );

    Qt::Orientation orientation() const;

    QPointF pos() const;
    double length() const;

    void setLabelAlignment( Qt::Alignment );
    Qt::Alignment labelAlignment() const;

    void setLabelRotation( double rotation );
    double labelRotation() const;

    int maxLabelHeight( const QFont& ) const;
    int maxLabelWidth( const QFont& ) const;

    QPointF labelPosition( double value ) const;

    QRectF labelRect( const QFont&, double value ) const;
    QSizeF labelSize( const QFont&, double value ) const;

    QRect boundingLabelRect( const QFont&, double value ) const;

  protected:
    QTransform labelTransformation( const QPointF&, const QSizeF& ) const;

    virtual void drawTick( QPainter*,
        double value, double len ) const QWT_OVERRIDE;

    virtual void drawBackbone( QPainter* ) const QWT_OVERRIDE;
    virtual void drawLabel( QPainter*, double value ) const QWT_OVERRIDE;

  private:
    void updateMap();

    class PrivateData;
    PrivateData* m_data;
};

/*!
   Move the position of the scale

   \param x X coordinate
   \param y Y coordinate

   \sa move(const QPointF &)
 */
inline void QwtScaleDraw::move( double x, double y )
{
    move( QPointF( x, y ) );
}

#endif
