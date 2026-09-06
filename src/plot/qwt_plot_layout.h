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
 *        - QwtPanner -> QwtCachePanner (pixmap-cache version)
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
 ******************************************************************************/

#ifndef QWT_PLOT_LAYOUT_H
#define QWT_PLOT_LAYOUT_H

#include "qwt_global.h"
#include "qwt_plot.h"
#include "qwt_axis_id.h"
#include <QSize>
class QwtPlotLayoutEngine;

/**
 * @brief Layout engine for QwtPlot
 * @details It is used by the QwtPlot widget to organize its internal widgets
 *          or by QwtPlot::print() to render its content to a QPaintDevice like
 *          a QPrinter, QPixmap/QImage or QSvgRenderer.
 * @sa QwtPlot::setPlotLayout()
 */
class QWT_EXPORT QwtPlotLayout
{
public:
    /**
     * @brief Options to configure the plot layout engine
     * @sa activate(), QwtPlotRenderer
     */
    enum Option
    {
        /**
         * @brief Unused
         */
        AlignScales = 0x01,

        /**
         * @brief Ignore the dimension of the scrollbars.
         * @details There are no scrollbars when the plot is not rendered to widgets.
         */
        IgnoreScrollbars = 0x02,

        /**
         * @brief Ignore all frames
         */
        IgnoreFrames = 0x04,

        /**
         * @brief Ignore the legend
         */
        IgnoreLegend = 0x08,

        /**
         * @brief Ignore the title
         */
        IgnoreTitle = 0x10,

        /**
         * @brief Ignore the footer
         */
        IgnoreFooter = 0x20
    };

    Q_DECLARE_FLAGS(Options, Option)

    explicit QwtPlotLayout();
    virtual ~QwtPlotLayout();

    void setCanvasMargin(int margin, int axis = -1);
    int canvasMargin(int axisId) const;

    void setAlignCanvasToScales(bool);
    void setAlignCanvasToScale(int axisId, bool);
    bool alignCanvasToScale(int axisId) const;

    // Enable fixed canvas size for an axis direction (auto-lock the current
    // dimension on the next layout). YLeft/YRight fix width; XBottom/XTop fix height.
    void setFixedCanvasSize(int axisPos, bool on);
    // Check if fixed canvas size is enabled for a given axis position
    bool isFixedCanvasSize(int axisPos) const;

    // Set a manual fixed canvas size, overriding the auto-captured value.
    // A component < 0 means "use auto-capture" for that direction.
    void setFixedCanvasSize(const QSize& size);
    // Get the manual fixed canvas size (-1 component = auto-capture)
    QSize fixedCanvasSize() const;

    // Clear all locked canvas sizes (captured offsets and manual override),
    // re-capturing from the current layout on the next activate(). Does not
    // change the enabled state of each axis direction.
    void resetFixedCanvasSize();

    void setSpacing(int);
    int spacing() const;

    void setLegendPosition(QwtPlot::LegendPosition pos, double ratio);
    void setLegendPosition(QwtPlot::LegendPosition pos);
    QwtPlot::LegendPosition legendPosition() const;

    void setLegendRatio(double ratio);
    double legendRatio() const;

    virtual QSize minimumSizeHint(const QwtPlot*) const;
    virtual void activate(const QwtPlot* plot, const QRectF& plotRect, Options options = Options());
    virtual void invalidate();

    QRectF titleRect() const;
    QRectF footerRect() const;
    QRectF legendRect() const;
    QRectF scaleRect(QwtAxisId) const;

    // Geometry of the caption strip for an outside axis title
    // (QwtScaleWidget::TitleOutside). The strip is adjacent to the scale rect:
    // below it for YLeft/YRight/XBottom, above it for XTop. Empty if the axis
    // has no outside title. Caption strips of all parasite layers are placed
    // within the bands reserved by the host plot layout.
    QRectF scaleCaptionRect(QwtAxisId) const;

    QRectF canvasRect() const;

protected:
    void setTitleRect(const QRectF&);
    void setFooterRect(const QRectF&);
    void setLegendRect(const QRectF&);
    void setScaleRect(QwtAxisId, const QRectF&);

    // Set the geometry of the caption strip for an outside axis title
    void setScaleCaptionRect(QwtAxisId, const QRectF&);

    // Recompute all caption rects from the current scale rects and the
    // title state of the scale widgets. Called at the end of doActivate().
    // Derived layouts that replace scale rects after doActivate() (like
    // QwtParasitePlotLayout copying the host rects) must call it again.
    void updateScaleCaptionRects(const QwtPlot* plot);

    void setCanvasRect(const QRectF&);
    QwtPlotLayoutEngine* layoutEngine();
    void doActivate(const QwtPlot* plot, const QRectF& plotRect, Options options = Options());

private:
    QwtPlotLayout(const QwtPlotLayout&)            = delete;
    QwtPlotLayout& operator=(const QwtPlotLayout&) = delete;

    // Pin the canvas rect to its locked dimension(s) after the natural layout
    // (innerRect) has been computed. Captures the offsets on first use.
    void applyFixedCanvas(QRectF& canvasRect, const QRectF& rect);

    QWT_DECLARE_PRIVATE(QwtPlotLayout)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotLayout::Options)

#endif
