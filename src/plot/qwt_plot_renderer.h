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

#ifndef QWT_PLOT_RENDERER_H
#define QWT_PLOT_RENDERER_H

#include "qwt_global.h"
#include "qwt_axis_id.h"

#include <qobject.h>
#include <qsize.h>

class QwtPlot;
class QwtScaleMap;
class QRectF;
class QPainter;
class QPaintDevice;

#ifndef QT_NO_PRINTER
class QPrinter;
#endif

#ifndef QWT_NO_SVG
#ifdef QT_SVG_LIB
class QSvgGenerator;
#endif
#endif

/**
 * @brief Renderer for exporting a plot to a document, a printer
 *        or anything else, that is supported by QPainter/QPaintDevice
 */
class QWT_EXPORT QwtPlotRenderer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Discard flags
     * @details Flags to control which components of the plot are rendered
     */
    enum DiscardFlag
    {
        /// Render all components of the plot
        DiscardNone = 0x00,

        /// Don't render the background of the plot
        DiscardBackground = 0x01,

        /// Don't render the title of the plot
        DiscardTitle = 0x02,

        /// Don't render the legend of the plot
        DiscardLegend = 0x04,

        /// Don't render the background of the canvas
        DiscardCanvasBackground = 0x08,

        /// Don't render the footer of the plot
        DiscardFooter = 0x10,

        ///< Don't render the frame of the canvas
        ///< @note This flag has no effect when using style sheets, where the frame is part of the background
        DiscardCanvasFrame = 0x20

    };

    Q_DECLARE_FLAGS(DiscardFlags, DiscardFlag)

    /**
     * @brief Layout flags
     * @details Flags to control the layout of the rendered plot
     * @sa setLayoutFlag(), testLayoutFlag()
     */
    enum LayoutFlag
    {
        /// Use the default layout as on screen
        DefaultLayout = 0x00,

        ///< Instead of the scales a box is painted around the plot canvas, where the scale ticks are aligned to.
        FrameWithScales = 0x01
    };

    Q_DECLARE_FLAGS(LayoutFlags, LayoutFlag)

    // Constructor
    explicit QwtPlotRenderer(QObject* = nullptr);
    // Destructor
    ~QwtPlotRenderer() override;

    // Set a discard flag
    void setDiscardFlag(DiscardFlag flag, bool on = true);
    // Test a discard flag
    bool testDiscardFlag(DiscardFlag flag) const;

    // Set discard flags
    void setDiscardFlags(DiscardFlags flags);
    // Get discard flags
    DiscardFlags discardFlags() const;

    // Set a layout flag
    void setLayoutFlag(LayoutFlag flag, bool on = true);
    // Test a layout flag
    bool testLayoutFlag(LayoutFlag flag) const;

    // Set layout flags
    void setLayoutFlags(LayoutFlags flags);
    // Get layout flags
    LayoutFlags layoutFlags() const;

    // Render the plot to a document
    void renderDocument(QwtPlot*, const QString& fileName, const QSizeF& sizeMM, int resolution = 85);

    // Render the plot to a document with specified format
    void renderDocument(QwtPlot*, const QString& fileName, const QString& format, const QSizeF& sizeMM, int resolution = 85);

#ifndef QWT_NO_SVG
#ifdef QT_SVG_LIB
    // Render the plot to an SVG generator
    void renderTo(QwtPlot*, QSvgGenerator&) const;
#endif
#endif

#ifndef QT_NO_PRINTER
    // Render the plot to a printer
    void renderTo(QwtPlot*, QPrinter&) const;
#endif

    // Render the plot to a paint device
    void renderTo(QwtPlot*, QPaintDevice&) const;

    // Render the plot
    virtual void render(QwtPlot*, QPainter*, const QRectF& plotRect) const;

    // Render the title
    virtual void renderTitle(const QwtPlot*, QPainter*, const QRectF& titleRect) const;

    // Render the footer
    virtual void renderFooter(const QwtPlot*, QPainter*, const QRectF& footerRect) const;

    // Render a scale
    virtual void
    renderScale(const QwtPlot*, QPainter*, QwtAxisId, int startDist, int endDist, int baseDist, const QRectF& scaleRect) const;

    // Render the caption of an outside axis title (QwtScaleWidget::TitleOutside)
    virtual void renderScaleCaption(const QwtPlot*, QPainter*, QwtAxisId, const QRectF& captionRect) const;

    // Render the canvas
    virtual void renderCanvas(const QwtPlot*, QPainter*, const QRectF& canvasRect, const QwtScaleMap* maps) const;

    // Render the legend
    virtual void renderLegend(const QwtPlot*, QPainter*, const QRectF& legendRect) const;

    // Export the plot to a document
    bool exportTo(QwtPlot*, const QString& documentName, const QSizeF& sizeMM = QSizeF(300, 200), int resolution = 85);

private:
    // Build canvas maps
    void buildCanvasMaps(const QwtPlot*, const QRectF&, QwtScaleMap maps[]) const;

    // Update canvas margins
    bool updateCanvasMargins(QwtPlot*, const QRectF&, const QwtScaleMap maps[]) const;

private:
    QWT_DECLARE_PRIVATE(QwtPlotRenderer)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotRenderer::DiscardFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotRenderer::LayoutFlags)

#endif
