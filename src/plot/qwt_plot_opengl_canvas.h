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

#ifndef QWT_PLOT_OPENGL_CANVAS_H
#define QWT_PLOT_OPENGL_CANVAS_H

#include "qwt_global.h"
#include "qwt_plot_abstract_canvas.h"
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtOpenGLWidgets/QOpenGLWidget>
#else
#include <QOpenGLWidget>
#endif
#include <QSurfaceFormat>
#include <QPainterPath>

class QwtPlot;

/**
 * @brief An alternative canvas for a QwtPlot derived from QOpenGLWidget
 * @details Even if QwtPlotOpenGLCanvas is not derived from QFrame it imitates
 *          its API. When using style sheets it supports the box model - beside
 *          backgrounds with rounded borders.
 *
 * @sa QwtPlot::setCanvas(), QwtPlotCanvas, QwtPlotCanvas::OpenGLBuffer
 *
 * @note Another way for getting hardware accelerated graphics is using
 *       an OpenGL offscreen buffer ( QwtPlotCanvas::OpenGLBuffer ) with QwtPlotCanvas.
 *       Performance is worse, than rendering straight to a QOpenGLWidget, but is usually
 *       better integrated into a desktop application.
 */
class QWT_EXPORT QwtPlotOpenGLCanvas : public QOpenGLWidget, public QwtPlotAbstractGLCanvas
{
    Q_OBJECT

    Q_PROPERTY(QFrame::Shadow frameShadow READ frameShadow WRITE setFrameShadow)
    Q_PROPERTY(QFrame::Shape frameShape READ frameShape WRITE setFrameShape)
    Q_PROPERTY(int lineWidth READ lineWidth WRITE setLineWidth)
    Q_PROPERTY(int midLineWidth READ midLineWidth WRITE setMidLineWidth)
    Q_PROPERTY(int frameWidth READ frameWidth)
    Q_PROPERTY(QRect frameRect READ frameRect DESIGNABLE false)

    Q_PROPERTY(double borderRadius READ borderRadius WRITE setBorderRadius)

public:
    /**
     * @brief Constructor
     */
    explicit QwtPlotOpenGLCanvas(QwtPlot* = nullptr);
    /**
     * @brief Constructor with surface format
     */
    explicit QwtPlotOpenGLCanvas(const QSurfaceFormat&, QwtPlot* = nullptr);
    /**
     * @brief Destructor
     */
    ~QwtPlotOpenGLCanvas() override;

    /**
     * @brief Invalidate the backing store
     */
    Q_INVOKABLE virtual void invalidateBackingStore() override;
    /**
     * @brief Get the border path
     */
    Q_INVOKABLE QPainterPath borderPath(const QRect&) const;

    /**
     * @brief Handle events
     */
    virtual bool event(QEvent*) override;

public Q_SLOTS:
    /**
     * @brief Replot the canvas
     */
    void replot();

protected:
    /**
     * @brief Handle paint events
     */
    virtual void paintEvent(QPaintEvent*) override;

    /**
     * @brief Initialize OpenGL
     */
    virtual void initializeGL() override;
    /**
     * @brief Paint OpenGL
     */
    virtual void paintGL() override;
    /**
     * @brief Resize OpenGL
     */
    virtual void resizeGL(int width, int height) override;

private:
    /**
     * @brief Initialize the canvas
     */
    void init(const QSurfaceFormat&);
    /**
     * @brief Clear the backing store
     */
    virtual void clearBackingStore() override;

    QWT_DECLARE_PRIVATE(QwtPlotOpenGLCanvas)
};

#endif
