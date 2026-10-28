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
 *        - QwtGridRasterData (2-d table + interpolation)
 *        - QwtLinearColorMap::stopColors(), stopPos() API rename.
 *   7. Bar-chart: expose pen/brush control.
 *   8. Amalgamated build: single QwtPlot.h / QwtPlot.cpp pair in src-amalgamate.
 *****************************************************************************/

#include "qwt_plot_canvas.h"
#include "qwt_painter.h"
#include "qwt_plot.h"

#include <qpainter.h>
#include <qpainterpath.h>
#include <qevent.h>

class QwtPlotCanvas::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotCanvas)
public:
    PrivateData(QwtPlotCanvas* p) : q_ptr(p), backingStore(nullptr)
    {
    }

    ~PrivateData()
    {
        delete backingStore;
    }

    QwtPlotCanvas::PaintAttributes paintAttributes;
    QPixmap* backingStore;
};

/**
 * @brief Constructor
 * @param[in] plot Parent plot widget
 * @sa QwtPlot::setCanvas()
 */
QwtPlotCanvas::QwtPlotCanvas(QwtPlot* plot) : QFrame(plot), QwtPlotAbstractCanvas(this), QWT_PIMPL_CONSTRUCT
{
    setPaintAttribute(QwtPlotCanvas::BackingStore, true);
    setPaintAttribute(QwtPlotCanvas::Opaque, true);
    setPaintAttribute(QwtPlotCanvas::HackStyledBackground, true);

    setLineWidth(0);
    setFrameShadow(QFrame::Plain);
    setFrameShape(QFrame::Box);
}

/**
 * @brief Destructor
 */
QwtPlotCanvas::~QwtPlotCanvas()
{
}

/**
 * @brief Change the paint attributes
 * @param[in] attribute Paint attribute
 * @param[in] on On/Off
 * @sa testPaintAttribute(), backingStore()
 */
void QwtPlotCanvas::setPaintAttribute(PaintAttribute attribute, bool on)
{
    QWT_D(d);

    if (bool(d->paintAttributes & attribute) == on)
        return;

    if (on)
        d->paintAttributes |= attribute;
    else
        d->paintAttributes &= ~attribute;

    switch (attribute) {
    case BackingStore: {
        if (on) {
            if (d->backingStore == nullptr)
                d->backingStore = new QPixmap();

            if (isVisible()) {
#if QT_VERSION >= 0x050000
                *d->backingStore = grab(rect());
#else
                *d->backingStore = QPixmap::grabWidget(this, rect());
#endif
            }
        } else {
            delete d->backingStore;
            d->backingStore = nullptr;
        }
        break;
    }
    case Opaque: {
        if (on)
            setAttribute(Qt::WA_OpaquePaintEvent, true);

        break;
    }
    default: {
        break;
    }
    }
}

/**
 * @brief Test whether a paint attribute is enabled
 * @param[in] attribute Paint attribute
 * @return true when attribute is enabled
 * @sa setPaintAttribute()
 */
bool QwtPlotCanvas::testPaintAttribute(PaintAttribute attribute) const
{
    QWT_DC(d);
    return d->paintAttributes & attribute;
}

/**
 * @brief Get the backing store
 * @return Backing store pixmap, might be null
 */
const QPixmap* QwtPlotCanvas::backingStore() const
{
    QWT_DC(d);
    return d->backingStore;
}

/**
 * @brief Invalidate the internal backing store
 */
void QwtPlotCanvas::invalidateBackingStore()
{
    QWT_D(d);

    if (d->backingStore)
        *d->backingStore = QPixmap();
}

/**
 * @brief Qt event handler for QEvent::PolishRequest and QEvent::StyleChange
 * @param[in] event Qt Event
 * @return See QFrame::event()
 */
bool QwtPlotCanvas::event(QEvent* event)
{
    if (event->type() == QEvent::PolishRequest) {
        if (testPaintAttribute(QwtPlotCanvas::Opaque)) {
            // Setting a style sheet changes the
            // Qt::WA_OpaquePaintEvent attribute, but we insist
            // on painting the background.

            setAttribute(Qt::WA_OpaquePaintEvent, true);
        }
    }

    if (event->type() == QEvent::PolishRequest || event->type() == QEvent::StyleChange) {
        updateStyleSheetInfo();
    }

    return QFrame::event(event);
}

/*!
   Paint event
   @param event Paint event
 */
void QwtPlotCanvas::paintEvent(QPaintEvent* event)
{
    QWT_D(d);

    QPainter painter(this);
    painter.setClipRegion(event->region());

    if (testPaintAttribute(QwtPlotCanvas::BackingStore) && d->backingStore != nullptr) {
        QPixmap& bs = *d->backingStore;

        /*
            At a fractional device pixel ratio the device pixels covered by the
            canvas depend on its position, not only on its size: both edges of
            the paint region are rounded independently. A pixmap of
            qRound( size * ratio ) pixels can be a column/row short of that
            footprint, and as the canvas is opaque nothing else paints the
            uncovered pixels - they keep stale content, f.e. the backbone of
            the right axis before a resize. So the pixmap is sized to the
            footprint and blitted 1:1 onto it.
         */
        const QTransform deviceTransform = painter.deviceTransform();
        const qreal ratio                = deviceTransform.m11();
        const bool deviceAligned         = deviceTransform.type() <= QTransform::TxScale && ratio > 0.0
                                           && qFuzzyCompare(ratio, deviceTransform.m22());

        QRect deviceRect;
        if (deviceAligned) {
            const QPointF topLeft     = deviceTransform.map(QPointF(0.0, 0.0));
            const QPointF bottomRight = deviceTransform.map(QPointF(width(), height()));
            deviceRect                = QRect(QPoint(qRound(topLeft.x()), qRound(topLeft.y())),
                                              QPoint(qRound(bottomRight.x()) - 1, qRound(bottomRight.y()) - 1));
        }

        const bool outdated = deviceAligned
                                  ? (bs.size() != deviceRect.size() || !qFuzzyCompare(bs.devicePixelRatio(), ratio))
                                  : bs.size() != size() * QwtPainter::devicePixelRatio(&bs);
        if (outdated) {
            if (deviceAligned) {
                bs = QPixmap(deviceRect.size());
                bs.setDevicePixelRatio(ratio);
            } else {
                bs = QwtPainter::backingStore(this, size());
            }
            // Initialize with full transparency first
            bs.fill(Qt::transparent);

            /*
                The pixmap is sized to the device footprint of the canvas,
                which can exceed the logical canvas rect by a fraction of a
                device pixel at fractional device pixel ratios. Backgrounds
                painted by a style ( styled backgrounds, rounded borders )
                cover the logical rect only, so the remaining columns/rows
                would stay transparent. The canvas is opaque: Qt assigns
                those pixels to it and nothing else paints them, and the
                transparent blit would let stale window content shine
                through. Fill the remainder with the canvas background.
             */
            const QBrush canvasBrush = palette().brush(backgroundRole()).isOpaque()
                                           ? palette().brush(backgroundRole())
                                           : palette().brush(QPalette::Window);
            if (canvasBrush.isOpaque()) {
                const qreal pixmapRatio   = bs.devicePixelRatio();
                const qreal logicalWidth  = bs.width() / pixmapRatio;
                const qreal logicalHeight = bs.height() / pixmapRatio;

                if (logicalWidth > width() || logicalHeight > height()) {
                    QPainter p(&bs);
                    p.setPen(Qt::NoPen);
                    p.setBrush(canvasBrush);
                    if (logicalWidth > width())
                        p.drawRect(QRectF(width(), 0.0, logicalWidth - width(), logicalHeight));
                    if (logicalHeight > height())
                        p.drawRect(QRectF(0.0, height(), width(), logicalHeight - height()));
                }
            }

            if (testAttribute(Qt::WA_StyledBackground)) {
                QPainter p(&bs);
                drawStyled(&p, testPaintAttribute(HackStyledBackground));
            } else {
                QPainter p;
                if (borderRadius() <= 0.0) {
                    QwtPainter::fillPixmap(this, bs);
                    p.begin(&bs);
                    drawCanvas(&p);
                } else {
                    p.begin(&bs);
                    drawUnstyled(&p);
                }

                if (frameWidth() > 0)
                    drawBorder(&p);
            }
        }
        if (deviceAligned) {
            painter.save();
            painter.setWorldTransform(deviceTransform.inverted(), true);
            painter.drawPixmap(deviceRect, bs, bs.rect());
            painter.restore();
        } else {
            painter.drawPixmap(0, 0, bs);
        }
    } else {
        if (testAttribute(Qt::WA_StyledBackground)) {
            if (testAttribute(Qt::WA_OpaquePaintEvent)) {
                drawStyled(&painter, testPaintAttribute(HackStyledBackground));
            } else {
                drawCanvas(&painter);
            }
        } else {
            if (testAttribute(Qt::WA_OpaquePaintEvent)) {
                if (autoFillBackground()) {
                    fillBackground(&painter);
                    drawBackground(&painter);
                }
            } else {
                if (borderRadius() > 0.0) {
                    QPainterPath clipPath;
                    clipPath.addRect(rect());
                    clipPath = clipPath.subtracted(borderPath(rect()));

                    painter.save();

                    painter.setClipPath(clipPath, Qt::IntersectClip);
                    fillBackground(&painter);
                    drawBackground(&painter);

                    painter.restore();
                }
            }

            drawCanvas(&painter);

            if (frameWidth() > 0)
                drawBorder(&painter);
        }
    }

    if (hasFocus() && focusIndicator() == CanvasFocusIndicator)
        drawFocusIndicator(&painter);
}

/*!
   Draw the border of the plot canvas

   @param painter Painter
   @sa setBorderRadius()
 */
void QwtPlotCanvas::drawBorder(QPainter* painter)
{
    if (borderRadius() <= 0) {
        drawFrame(painter);
        return;
    }

    QwtPlotAbstractCanvas::drawBorder(painter);
}

/*!
   Resize event
   @param event Resize event
 */
void QwtPlotCanvas::resizeEvent(QResizeEvent* event)
{
    QFrame::resizeEvent(event);
    updateStyleSheetInfo();
}

/**
 * @brief Invalidate the paint cache and repaint the canvas
 * @sa invalidatePaintCache()
 */
void QwtPlotCanvas::replot()
{
    invalidateBackingStore();

    if (testPaintAttribute(QwtPlotCanvas::ImmediatePaint))
        repaint(contentsRect());
    else
        update(contentsRect());
}

/**
 * @brief Calculate the painter path for a styled or rounded border
 * @param[in] rect Bounding rectangle of the canvas
 * @return Painter path that can be used for clipping
 * @details When the canvas has no styled background or rounded borders,
 *          the painter path is empty.
 */
QPainterPath QwtPlotCanvas::borderPath(const QRect& rect) const
{
    return canvasBorderPath(rect);
}
