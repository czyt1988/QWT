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

#ifndef QWT_SLIDER_H
#define QWT_SLIDER_H

#include "qwt_global.h"
#include "qwt_abstract_slider.h"

class QwtScaleDraw;

/*!
   \brief The Slider Widget

   QwtSlider is a slider widget which operates on an interval
   of type double. Its position is related to a scale showing
   the current value.

   The slider can be customized by having a through, a groove - or both.

   \image html sliders.png
 */

class QWT_EXPORT QwtSlider : public QwtAbstractSlider
{
    Q_OBJECT

    Q_ENUMS( ScalePosition BackgroundStyle )

    Q_PROPERTY( Qt::Orientation orientation
        READ orientation WRITE setOrientation )
    Q_PROPERTY( ScalePosition scalePosition READ scalePosition
        WRITE setScalePosition )

    Q_PROPERTY( bool trough READ hasTrough WRITE setTrough )
    Q_PROPERTY( bool groove READ hasGroove WRITE setGroove )

    Q_PROPERTY( QSize handleSize READ handleSize WRITE setHandleSize )
    Q_PROPERTY( int borderWidth READ borderWidth WRITE setBorderWidth )
    Q_PROPERTY( int spacing READ spacing WRITE setSpacing )

  public:

    /*!
       Position of the scale
       \sa QwtSlider(), setScalePosition(), setOrientation()
     */
    enum ScalePosition
    {
        //! The slider has no scale
        NoScale,

        //! The scale is right of a vertical or below a horizontal slider
        LeadingScale,

        //! The scale is left of a vertical or above a horizontal slider
        TrailingScale
    };

    explicit QwtSlider( QWidget* parent = NULL );
    explicit QwtSlider( Qt::Orientation, QWidget* parent = NULL );

    virtual ~QwtSlider();

    void setOrientation( Qt::Orientation );
    Qt::Orientation orientation() const;

    void setScalePosition( ScalePosition );
    ScalePosition scalePosition() const;

    void setTrough( bool );
    bool hasTrough() const;

    void setGroove( bool );
    bool hasGroove() const;

    void setHandleSize( const QSize& );
    QSize handleSize() const;

    void setBorderWidth( int );
    int borderWidth() const;

    void setSpacing( int );
    int spacing() const;

    virtual QSize sizeHint() const QWT_OVERRIDE;
    virtual QSize minimumSizeHint() const QWT_OVERRIDE;

    void setScaleDraw( QwtScaleDraw* );
    const QwtScaleDraw* scaleDraw() const;

    void setUpdateInterval( int );
    int updateInterval() const;

  protected:
    virtual double scrolledTo( const QPoint& ) const QWT_OVERRIDE;
    virtual bool isScrollPosition( const QPoint& ) const QWT_OVERRIDE;

    virtual void drawSlider ( QPainter*, const QRect& ) const;
    virtual void drawHandle( QPainter*, const QRect&, int pos ) const;

    virtual void mousePressEvent( QMouseEvent* ) QWT_OVERRIDE;
    virtual void mouseReleaseEvent( QMouseEvent* ) QWT_OVERRIDE;
    virtual void resizeEvent( QResizeEvent* ) QWT_OVERRIDE;
    virtual void paintEvent ( QPaintEvent* ) QWT_OVERRIDE;
    virtual void changeEvent( QEvent* ) QWT_OVERRIDE;
    virtual void timerEvent( QTimerEvent* ) QWT_OVERRIDE;

    virtual bool event( QEvent* ) QWT_OVERRIDE;

    virtual void scaleChange() QWT_OVERRIDE;

    QRect sliderRect() const;
    QRect handleRect() const;

  private:
    QwtScaleDraw* scaleDraw();

    void layoutSlider( bool );
    void initSlider( Qt::Orientation );

    class PrivateData;
    PrivateData* m_data;
};

#endif
