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

#ifndef QWT_ROUND_SCALE_DRAW_H
#define QWT_ROUND_SCALE_DRAW_H

#include "qwt_global.h"
#include "qwt_abstract_scale_draw.h"

#include <qpoint.h>

/*!
   \brief A class for drawing round scales

   QwtRoundScaleDraw can be used to draw round scales.
   The circle segment can be adjusted by setAngleRange().
   The geometry of the scale can be specified with
   moveCenter() and setRadius().

   After a scale division has been specified as a QwtScaleDiv object
   using QwtAbstractScaleDraw::setScaleDiv(const QwtScaleDiv &s),
   the scale can be drawn with the QwtAbstractScaleDraw::draw() member.
 */

class QWT_EXPORT QwtRoundScaleDraw : public QwtAbstractScaleDraw
{
  public:
    QwtRoundScaleDraw();
    virtual ~QwtRoundScaleDraw();

    void setRadius( double radius );
    double radius() const;

    void moveCenter( double x, double y );
    void moveCenter( const QPointF& );
    QPointF center() const;

    void setAngleRange( double angle1, double angle2 );

    virtual double extent( const QFont& ) const QWT_OVERRIDE;

  protected:
    virtual void drawTick( QPainter*,
        double value, double len ) const QWT_OVERRIDE;

    virtual void drawBackbone(
        QPainter* ) const QWT_OVERRIDE;

    virtual void drawLabel(
        QPainter*, double value ) const QWT_OVERRIDE;

  private:
    class PrivateData;
    PrivateData* m_data;
};

//! Move the center of the scale draw, leaving the radius unchanged
inline void QwtRoundScaleDraw::moveCenter( double x, double y )
{
    moveCenter( QPointF( x, y ) );
}

#endif
