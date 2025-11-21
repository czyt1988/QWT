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

#ifndef QWT_PLOT_CANVAS_H
#define QWT_PLOT_CANVAS_H

#include "qwt_global.h"
#include "qwt_plot_abstract_canvas.h"

#include <qframe.h>

class QwtPlot;
class QPixmap;
class QPainterPath;

/*!
   \brief Canvas of a QwtPlot.

   Canvas is the widget where all plot items are displayed

   \sa QwtPlot::setCanvas(), QwtPlotGLCanvas, QwtPlotOpenGLCanvas
 */
class QWT_EXPORT QwtPlotCanvas : public QFrame, public QwtPlotAbstractCanvas
{
    Q_OBJECT

    Q_PROPERTY(double borderRadius READ borderRadius WRITE setBorderRadius)

public:
    /**
     * @brief Paint attributes/绘制属性
     *
     * The default setting enables BackingStore and Opaque.
     *
     * 默认设置启用 BackingStore 与 Opaque
     *
     * @sa setPaintAttribute(), testPaintAttribute()
     */
    enum PaintAttribute
    {
        /**
         * @brief BackingStore
         *
         * Paint double buffered reusing the content of the pixmap buffer when possible.
         * 双缓冲绘制，尽可能复用 pixmap 缓存的内容。
         *
         * Using a backing store might improve the performance significantly,
         * when working with widget overlays (like rubber bands).
         * 使用后备存储可显著提升性能，尤其当存在部件叠加（如 rubber band）时。
         *
         * Disabling the cache might improve the performance for incremental paints
         * (using QwtPlotDirectPainter).
         * 禁用缓存可能提升增量绘制（如使用 QwtPlotDirectPainter）的性能。
         *
         * @sa backingStore(), invalidateBackingStore()
         */
        BackingStore = 1,

        /**
         * @brief Opaque
         *
         * Try to fill the complete contents rectangle of the plot canvas.
         * 尝试填充绘图画布的全部内容矩形。
         *
         * When using styled backgrounds Qt assumes that the canvas doesn't fill its area
         * completely (e.g. because of rounded borders) and fills the area below the canvas.
         * When this is done with gradients it might result in a serious performance bottleneck
         * — depending on the size.
         * 使用样式背景时，Qt 假定画布并未完全填充其区域（例如圆角边框），
         * 因而会在画布下方继续填充。若使用渐变填充，这可能成为严重性能瓶颈。
         *
         * When the Opaque attribute is enabled the canvas tries to identify the gaps
         * with some heuristics and to fill those only.
         * 启用 Opaque 后，画布会通过启发式算法识别空隙并仅填充这些区域。
         *
         * @warning Will not work for semitransparent backgrounds.
         * @warning 对半透明背景无效。
         */
        Opaque = 2,

        /**
         * @brief HackStyledBackground
         *
         * Try to improve painting of styled backgrounds.
         * 尝试优化样式背景的绘制。
         *
         * QwtPlotCanvas supports the box model attributes for customizing the layout
         * with style sheets. Unfortunately the design of Qt style sheets has no concept
         * how to handle backgrounds with rounded corners — beside of padding.
         * QwtPlotCanvas 支持盒模型属性，以便通过样式表自定义布局。
         * 遗憾的是，Qt 样式表的设计并未提供处理圆角背景的方案，除 padding 外。
         *
         * When HackStyledBackground is enabled the plot canvas tries to separate
         * the background from the background border by reverse engineering to paint
         * the background before and the border after the plot items.
         * In this order the border gets perfectly antialiased and you can avoid
         * some pixel artifacts in the corners.
         * 启用 HackStyledBackground 后，画布会通过逆向工程将背景与边框分离：
         * 先绘制背景，再绘制边框，使边框获得完美抗锯齿并避免角落像素伪影。
         */
        HackStyledBackground = 4,

        /**
         * @brief ImmediatePaint
         *
         * When ImmediatePaint is set replot() calls repaint() instead of update().
         * 当 ImmediatePaint 被设置时，replot() 将调用 repaint() 而非 update()。
         *
         * @sa replot(), QWidget::repaint(), QWidget::update()
         */
        ImmediatePaint = 8
    };

    Q_DECLARE_FLAGS(PaintAttributes, PaintAttribute)

    explicit QwtPlotCanvas(QwtPlot* = NULL);
    virtual ~QwtPlotCanvas();

    void setPaintAttribute(PaintAttribute, bool on = true);
    bool testPaintAttribute(PaintAttribute) const;

    const QPixmap* backingStore() const;
    Q_INVOKABLE void invalidateBackingStore();

    virtual bool event(QEvent*) QWT_OVERRIDE;

    Q_INVOKABLE QPainterPath borderPath(const QRect&) const;

public Q_SLOTS:
    void replot();

protected:
    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;
    virtual void resizeEvent(QResizeEvent*) QWT_OVERRIDE;

    virtual void drawBorder(QPainter*) QWT_OVERRIDE;

private:
    class PrivateData;
    PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotCanvas::PaintAttributes)

#endif
