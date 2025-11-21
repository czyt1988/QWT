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

#ifndef QWT_ARROW_BUTTON_H
#define QWT_ARROW_BUTTON_H

#include "qwt_global.h"
#include <qpushbutton.h>

/*!
   \brief Arrow Button

   A push button with one or more filled triangles on its front.
   An Arrow button can have 1 to 3 arrows in a row, pointing
   up, down, left or right.
 */
class QWT_EXPORT QwtArrowButton : public QPushButton
{
  public:
    explicit QwtArrowButton ( int num, Qt::ArrowType, QWidget* parent = NULL );
    virtual ~QwtArrowButton();

    Qt::ArrowType arrowType() const;
    int num() const;

    virtual QSize sizeHint() const QWT_OVERRIDE;
    virtual QSize minimumSizeHint() const QWT_OVERRIDE;

  protected:
    virtual void paintEvent( QPaintEvent*) QWT_OVERRIDE;
    virtual void keyPressEvent( QKeyEvent* ) QWT_OVERRIDE;

    virtual void drawButtonLabel( QPainter* );
    virtual void drawArrow( QPainter*,
        const QRect&, Qt::ArrowType ) const;
    virtual QRect labelRect() const;
    virtual QSize arrowSize( Qt::ArrowType,
        const QSize& boundingSize ) const;

  private:
    class PrivateData;
    PrivateData* m_data;
};

#endif
