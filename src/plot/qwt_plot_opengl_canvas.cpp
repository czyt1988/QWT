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

#include "qwt_plot_opengl_canvas.h"
#include "qwt_plot.h"
#include "qwt_painter.h"

#include <qpainter.h>
#include <qpainterpath.h>
#include <qcoreevent.h>
#include <qopenglframebufferobject.h>
#include <qopenglpaintdevice.h>

class QwtPlotOpenGLCanvas::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotOpenGLCanvas)
public:
    PrivateData(QwtPlotOpenGLCanvas* p) : q_ptr(p), numSamples(0), isPolished(false), fboDirty(true), fbo(nullptr)
    {
    }

    ~PrivateData()
    {
        delete fbo;
    }

    int numSamples;

    bool isPolished;
    bool fboDirty;
    QOpenGLFramebufferObject* fbo;
};

/**
 * @brief Constructor
 * @param[in] plot Parent plot widget
 * @sa QwtPlot::setCanvas()
 */
QwtPlotOpenGLCanvas::QwtPlotOpenGLCanvas(QwtPlot* plot)
    : QOpenGLWidget(plot), QwtPlotAbstractGLCanvas(this), QWT_PIMPL_CONSTRUCT
{
    QSurfaceFormat fmt = format();
    fmt.setSamples(4);

    init(fmt);
}

/**
 * @brief Constructor
 * @param[in] format OpenGL surface format
 * @param[in] plot Parent plot widget
 * @sa QwtPlot::setCanvas()
 */
QwtPlotOpenGLCanvas::QwtPlotOpenGLCanvas(const QSurfaceFormat& format, QwtPlot* plot)
    : QOpenGLWidget(plot), QwtPlotAbstractGLCanvas(this), QWT_PIMPL_CONSTRUCT
{
    init(format);
}

void QwtPlotOpenGLCanvas::init(const QSurfaceFormat& format)
{
    QWT_D(d);
    d->numSamples = format.samples();

    setFormat(format);

#if 1
    setAttribute(Qt::WA_OpaquePaintEvent, true);
#endif

    // same default frame as QwtPlotCanvas: a box without a visible border
    setLineWidth(0);
    setFrameShadow(QFrame::Plain);
    setFrameShape(QFrame::Box);
}

/**
 * @brief Destructor
 */
QwtPlotOpenGLCanvas::~QwtPlotOpenGLCanvas()
{
}

/*!
   Paint event

   @param event Paint event
   @sa QwtPlot::drawCanvas()
 */
void QwtPlotOpenGLCanvas::paintEvent(QPaintEvent* event)
{
    QWT_D(d);
    if (d->isPolished)
        QOpenGLWidget::paintEvent(event);
}

/**
 * @brief Qt event handler for QEvent::PolishRequest and QEvent::StyleChange
 * @param[in] event Qt Event
 * @return See QOpenGLWidget::event()
 */
bool QwtPlotOpenGLCanvas::event(QEvent* event)
{
    QWT_D(d);
    const bool ok = QOpenGLWidget::event(event);

    if (event->type() == QEvent::PolishRequest) {
        // In opposite to non OpenGL widgets receive pointless
        // early repaints. As we always have a QEvent::PolishRequest
        // followed by QEvent::Paint, we can ignore all these repaints.

        d->isPolished = true;
    }

    if (event->type() == QEvent::PolishRequest || event->type() == QEvent::StyleChange) {
        // assuming, that we always have a styled background
        // when we have a style sheet

        setAttribute(Qt::WA_StyledBackground, testAttribute(Qt::WA_StyleSheet));
    }

    return ok;
}

/**
 * @brief Invalidate the paint cache and repaint the canvas
 * @sa invalidatePaintCache()
 */
void QwtPlotOpenGLCanvas::replot()
{
    QwtPlotAbstractGLCanvas::replot();
}

/**
 * @brief Invalidate the internal backing store
 */
void QwtPlotOpenGLCanvas::invalidateBackingStore()
{
    QWT_D(d);
    d->fboDirty = true;
}

void QwtPlotOpenGLCanvas::clearBackingStore()
{
    QWT_D(d);
    delete d->fbo;
    d->fbo = nullptr;
}

/**
 * @brief Calculate the painter path for a styled or rounded border
 * @details When the canvas has no styled background or rounded borders
 *          the painter path is empty.
 * @param[in] rect Bounding rectangle of the canvas
 * @return Painter path, that can be used for clipping
 */
QPainterPath QwtPlotOpenGLCanvas::borderPath(const QRect& rect) const
{
    return canvasBorderPath(rect);
}

//! No operation - reserved for some potential use in the future
void QwtPlotOpenGLCanvas::initializeGL()
{
}

//! Paint the plot
void QwtPlotOpenGLCanvas::paintGL()
{
    QWT_D(d);
    const bool hasFocusIndicator = hasFocus() && focusIndicator() == CanvasFocusIndicator;

    QPainter painter;

    if (testPaintAttribute(QwtPlotOpenGLCanvas::BackingStore) && QOpenGLFramebufferObject::hasOpenGLFramebufferBlit()) {
        // the ratio of the screen this canvas lives on - not the maximum
        // over all screens, what QwtPainter::devicePixelRatio( nullptr ) returns
        const qreal pixelRatio = devicePixelRatioF();
        const QSize fboSize    = size() * pixelRatio;

        if (hasFocusIndicator)
            painter.begin(this);

        /*
           QOpenGLWidget has its own internal FBO, that is used to restore
           its content without having to repaint. This works fine when f.e
           a rubberband is moving on top, but there are still situations,
           where we can repaint without an potentially expensive replot:

               - when having the focus the top level window gets activated/deactivated
               - ???
         */

        if (d->fbo) {
            if (d->fbo->size() != fboSize) {
                delete d->fbo;
                d->fbo = nullptr;
            }
        }

        if (d->fbo == nullptr) {
            QOpenGLFramebufferObjectFormat fboFormat;
            fboFormat.setSamples(d->numSamples);
            fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);

            d->fbo      = new QOpenGLFramebufferObject(fboSize, fboFormat);
            d->fboDirty = true;
        }

        if (d->fboDirty) {
            d->fbo->bind();

            QOpenGLPaintDevice pd(fboSize);

            QPainter fboPainter(&pd);
            fboPainter.scale(pixelRatio, pixelRatio);

            /*
                The FBO is sized to round( size * ratio ) device pixels, which
                can exceed the logical canvas rect by a fraction of a device
                pixel at fractional ratios. The canvas background covers the
                logical rect only, so the remaining column/row would keep
                uninitialized content. Fill the remainder with the canvas
                background, like QwtPlotCanvas does for its backing store.
             */
            const QBrush canvasBrush = palette().brush(backgroundRole()).isOpaque()
                                           ? palette().brush(backgroundRole())
                                           : palette().brush(QPalette::Window);

            const qreal logicalWidth  = fboSize.width() / pixelRatio;
            const qreal logicalHeight = fboSize.height() / pixelRatio;

            if (logicalWidth > width() || logicalHeight > height()) {
                fboPainter.setPen(Qt::NoPen);
                fboPainter.setBrush(canvasBrush.isOpaque() ? canvasBrush : QBrush(Qt::transparent));
                fboPainter.setCompositionMode(QPainter::CompositionMode_Source);

                if (logicalWidth > width())
                    fboPainter.drawRect(QRectF(width(), 0.0, logicalWidth - width(), logicalHeight));
                if (logicalHeight > height())
                    fboPainter.drawRect(QRectF(0.0, height(), width(), logicalHeight - height()));

                fboPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            }

            draw(&fboPainter);
            fboPainter.end();

            d->fboDirty = false;
        }

        QOpenGLFramebufferObject::blitFramebuffer(nullptr, d->fbo);
    } else {
        painter.begin(this);
        draw(&painter);
    }

    if (hasFocusIndicator)
        drawFocusIndicator(&painter);
}

//! No operation - reserved for some potential use in the future
void QwtPlotOpenGLCanvas::resizeGL(int, int)
{
    // nothing to do
}
