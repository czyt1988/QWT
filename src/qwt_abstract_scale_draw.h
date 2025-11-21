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

#ifndef QWT_ABSTRACT_SCALE_DRAW_H
#define QWT_ABSTRACT_SCALE_DRAW_H

#include "qwt_global.h"
#include "qwt_scale_div.h"

class QwtText;
class QPalette;
class QPainter;
class QFont;
class QwtTransform;
class QwtScaleMap;

/*!
   \brief A abstract base class for drawing scales

   QwtAbstractScaleDraw can be used to draw linear or logarithmic scales.

   After a scale division has been specified as a QwtScaleDiv object
   using setScaleDiv(), the scale can be drawn with the draw() member.
 */
class QWT_EXPORT QwtAbstractScaleDraw
{
public:
    /*!
       Components of a scale
       \sa enableComponent(), hasComponent
     */
    enum ScaleComponent
    {
        //! Backbone = the line where the ticks are located
        Backbone = 0x01,

        //! Ticks
        Ticks = 0x02,

        //! Labels
        Labels = 0x04
    };

    Q_DECLARE_FLAGS(ScaleComponents, ScaleComponent)

    QwtAbstractScaleDraw();
    virtual ~QwtAbstractScaleDraw();

    void setScaleDiv(const QwtScaleDiv&);
    const QwtScaleDiv& scaleDiv() const;

    void setTransformation(QwtTransform*);
    const QwtScaleMap& scaleMap() const;
    QwtScaleMap& scaleMap();

    void enableComponent(ScaleComponent, bool enable = true);
    bool hasComponent(ScaleComponent) const;

    void setTickLength(QwtScaleDiv::TickType, double length);
    double tickLength(QwtScaleDiv::TickType) const;
    double maxTickLength() const;

    void setSpacing(double);
    double spacing() const;

    void setPenWidthF(qreal width);
    qreal penWidthF() const;

    // 设置是否选中，选中状态的绘制会有一定差异
    void setSelected(bool on);
    bool isSelected() const;

    // 设置选中后画笔的宽度修正
    void setSelectedPenWidthOffset(qreal offset = 1);
    qreal selectedPenWidthOffset() const;

    virtual void draw(QPainter*, const QPalette&) const;

    virtual QwtText label(double) const;

    /*!
       Calculate the extent

       The extent is the distance from the baseline to the outermost
       pixel of the scale draw in opposite to its orientation.
       It is at least minimumExtent() pixels.

       \param font Font used for drawing the tick labels
       \return Number of pixels

       \sa setMinimumExtent(), minimumExtent()
     */
    virtual double extent(const QFont& font) const = 0;

    void setMinimumExtent(double);
    double minimumExtent() const;

    void invalidateCache();

protected:
    /*!
       Draw a tick

       \param painter Painter
       \param value Value of the tick
       \param len Length of the tick

       \sa drawBackbone(), drawLabel()
     */
    virtual void drawTick(QPainter* painter, double value, double len) const = 0;

    /*!
       Draws the baseline of the scale
       \param painter Painter

       \sa drawTick(), drawLabel()
     */
    virtual void drawBackbone(QPainter* painter) const = 0;

    /*!
        Draws the label for a major scale tick

        \param painter Painter
        \param value Value

        \sa drawTick(), drawBackbone()
     */
    virtual void drawLabel(QPainter* painter, double value) const = 0;

    const QwtText& tickLabel(const QFont&, double value) const;

private:
    Q_DISABLE_COPY(QwtAbstractScaleDraw)

    class PrivateData;
    PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtAbstractScaleDraw::ScaleComponents)

#endif
