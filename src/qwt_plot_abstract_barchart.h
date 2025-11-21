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

#ifndef QWT_PLOT_ABSTRACT_BAR_CHART_H
#define QWT_PLOT_ABSTRACT_BAR_CHART_H

#include "qwt_global.h"
#include "qwt_plot_seriesitem.h"

/*!
   \brief Abstract base class for bar chart items

   In opposite to almost all other plot items bar charts can't be
   displayed inside of their bounding rectangle and need a special
   API  how to calculate the width of the bars and how they affect
   the layout of the attached plot.
 */
class QWT_EXPORT QwtPlotAbstractBarChart : public QwtPlotSeriesItem
{
  public:
    /*!
        \brief Mode how to calculate the bar width

        setLayoutPolicy(), setLayoutHint(), barWidthHint()
     */
    enum LayoutPolicy
    {
        /*!
           The sample width is calculated by dividing the bounding rectangle
           by the number of samples. The layoutHint() is used as a minimum width
           in paint device coordinates.

           \sa boundingRectangle()
         */
        AutoAdjustSamples,

        /*!
           layoutHint() defines an interval in axis coordinates
         */
        ScaleSamplesToAxes,

        /*!
           The bar width is calculated by multiplying layoutHint()
           with the height or width of the canvas.

           \sa boundingRectangle()
         */
        ScaleSampleToCanvas,

        /*!
           layoutHint() defines a fixed width in paint device coordinates.
         */
        FixedSampleSize
    };

    explicit QwtPlotAbstractBarChart( const QwtText& title );
    virtual ~QwtPlotAbstractBarChart();

    void setLayoutPolicy( LayoutPolicy );
    LayoutPolicy layoutPolicy() const;

    void setLayoutHint( double );
    double layoutHint() const;

    void setSpacing( int );
    int spacing() const;

    void setMargin( int );
    int margin() const;

    void setBaseline( double );
    double baseline() const;

    virtual void getCanvasMarginHint(
        const QwtScaleMap& xMap, const QwtScaleMap& yMap,
        const QRectF& canvasRect, double& left, double& top,
        double& right, double& bottom) const QWT_OVERRIDE;


  protected:
    double sampleWidth( const QwtScaleMap& map,
        double canvasSize, double boundingSize,
        double value ) const;

  private:
    class PrivateData;
    PrivateData* m_data;
};

#endif
