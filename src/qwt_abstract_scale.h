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

#ifndef QWT_ABSTRACT_SCALE_H
#define QWT_ABSTRACT_SCALE_H

#include "qwt_global.h"
#include <qwidget.h>

class QwtScaleEngine;
class QwtAbstractScaleDraw;
class QwtScaleDiv;
class QwtScaleMap;
class QwtInterval;

/*!
   \brief An abstract base class for widgets having a scale

   The scale of an QwtAbstractScale is determined by a QwtScaleDiv
   definition, that contains the boundaries and the ticks of the scale.
   The scale is painted using a QwtScaleDraw object.

   The scale division might be assigned explicitly - but usually
   it is calculated from the boundaries using a QwtScaleEngine.

   The scale engine also decides the type of transformation of the scale
   ( linear, logarithmic ... ).
 */

class QWT_EXPORT QwtAbstractScale : public QWidget
{
    Q_OBJECT

    Q_PROPERTY( double lowerBound READ lowerBound WRITE setLowerBound )
    Q_PROPERTY( double upperBound READ upperBound WRITE setUpperBound )

    Q_PROPERTY( int scaleMaxMajor READ scaleMaxMajor WRITE setScaleMaxMajor )
    Q_PROPERTY( int scaleMaxMinor READ scaleMaxMinor WRITE setScaleMaxMinor )

    Q_PROPERTY( double scaleStepSize READ scaleStepSize WRITE setScaleStepSize )

  public:
    explicit QwtAbstractScale( QWidget* parent = NULL );
    virtual ~QwtAbstractScale();

    void setScale( double lowerBound, double upperBound );
    void setScale( const QwtInterval& );
    void setScale( const QwtScaleDiv& );

    const QwtScaleDiv& scaleDiv() const;

    void setLowerBound( double value );
    double lowerBound() const;

    void setUpperBound( double value );
    double upperBound() const;

    void setScaleStepSize( double stepSize );
    double scaleStepSize() const;

    void setScaleMaxMajor( int ticks );
    int scaleMaxMinor() const;

    void setScaleMaxMinor( int ticks );
    int scaleMaxMajor() const;

    void setScaleEngine( QwtScaleEngine* );
    const QwtScaleEngine* scaleEngine() const;
    QwtScaleEngine* scaleEngine();

    int transform( double ) const;
    double invTransform( int ) const;

    bool isInverted() const;

    double minimum() const;
    double maximum() const;

    const QwtScaleMap& scaleMap() const;

  protected:
    virtual void changeEvent( QEvent* ) QWT_OVERRIDE;

    void rescale( double lowerBound,
        double upperBound, double stepSize );

    void setAbstractScaleDraw( QwtAbstractScaleDraw* );

    const QwtAbstractScaleDraw* abstractScaleDraw() const;
    QwtAbstractScaleDraw* abstractScaleDraw();

    void updateScaleDraw();
    virtual void scaleChange();

  private:
    class PrivateData;
    PrivateData* m_data;
};

#endif
