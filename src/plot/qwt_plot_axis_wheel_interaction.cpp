/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *
 * This file is a new addition to the Qwt 7.x maintenance branch.
 *****************************************************************************/

#include "qwt_plot_axis_wheel_interaction.h"

#include <qevent.h>
#include <qwidget.h>

#include <cmath>

#include "qwt_plot.h"
#include "qwt_qt5qt6_compat.hpp"

class QwtPlotAxisWheelInteraction::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotAxisWheelInteraction)

public:
    PrivateData(QwtPlotAxisWheelInteraction* p)
        : q_ptr(p)
        , isEnabled(false)
        , zoomAxisId(QwtAxis::XBottom)
        , zoomFactor(0.9)
        , zoomModifiers(Qt::ControlModifier)
        , panOrientation(Qt::Horizontal)
        , panModifiers(Qt::NoModifier)
        , panFactor(30)
    {
    }

    bool isEnabled;

    QwtAxisId zoomAxisId;
    double zoomFactor;
    Qt::KeyboardModifiers zoomModifiers;

    Qt::Orientation panOrientation;
    Qt::KeyboardModifiers panModifiers;
    int panFactor;
};

/**
 * @brief Constructor
 * @param[in] canvas Plot canvas to observe
 *
 * Creates a wheel interaction handler attached to the given plot canvas.
 * The interaction is enabled by default.
 */
QwtPlotAxisWheelInteraction::QwtPlotAxisWheelInteraction(QWidget* canvas)
    : QObject(canvas), QWT_PIMPL_CONSTRUCT
{
    if (canvas && canvas->focusPolicy() == Qt::NoFocus)
        canvas->setFocusPolicy(Qt::WheelFocus);

    setEnabled(true);
}

/**
 * @brief Destructor
 */
QwtPlotAxisWheelInteraction::~QwtPlotAxisWheelInteraction()
{
}

/**
 * @brief Enable or disable the interaction
 * @details When enabled, an event filter is installed on the observed canvas
 *          to intercept wheel events. When disabled, the event filter is removed.
 * @param[in] on true to enable, false to disable
 * @sa isEnabled(), eventFilter()
 */
void QwtPlotAxisWheelInteraction::setEnabled(bool on)
{
    QWT_D(d);
    if (d->isEnabled != on) {
        d->isEnabled = on;

        QObject* o = parent();
        if (o) {
            if (d->isEnabled)
                o->installEventFilter(this);
            else
                o->removeEventFilter(this);
        }
    }
}

/**
 * @brief Return whether the interaction is enabled
 * @return true when enabled, false otherwise
 * @sa setEnabled()
 */
bool QwtPlotAxisWheelInteraction::isEnabled() const
{
    QWT_DC(d);
    return d->isEnabled;
}

/**
 * @brief Set the axis to zoom in zoom mode
 * @param[in] axisId Axis identifier (default: QwtAxis::XBottom)
 * @sa zoomAxisId()
 */
void QwtPlotAxisWheelInteraction::setZoomAxisId(QwtAxisId axisId)
{
    QWT_D(d);
    d->zoomAxisId = axisId;
}

/**
 * @brief Return the axis being zoomed in zoom mode
 * @return Axis identifier
 * @sa setZoomAxisId()
 */
QwtAxisId QwtPlotAxisWheelInteraction::zoomAxisId() const
{
    QWT_DC(d);
    return d->zoomAxisId;
}

/**
 * @brief Set the zoom factor per wheel step
 * @details The zoom factor defines the ratio between the current axis range
 *          and the zoomed range for each wheel step. Values < 1 zoom in,
 *          values > 1 zoom out. The default is 0.9.
 * @param[in] factor Zoom factor
 * @sa zoomFactor()
 */
void QwtPlotAxisWheelInteraction::setZoomFactor(double factor)
{
    QWT_D(d);
    d->zoomFactor = factor;
}

/**
 * @brief Return the zoom factor
 * @return Zoom factor
 * @sa setZoomFactor()
 */
double QwtPlotAxisWheelInteraction::zoomFactor() const
{
    QWT_DC(d);
    return d->zoomFactor;
}

/**
 * @brief Set keyboard modifiers that activate zoom mode
 * @details When the wheel is rotated with these modifiers pressed, the
 *          configured axis is zoomed. The default is Qt::ControlModifier.
 * @param[in] modifiers Keyboard modifiers
 * @sa zoomModifiers()
 */
void QwtPlotAxisWheelInteraction::setZoomModifiers(Qt::KeyboardModifiers modifiers)
{
    QWT_D(d);
    d->zoomModifiers = modifiers;
}

/**
 * @brief Return the zoom modifiers
 * @return Keyboard modifiers for zoom mode
 * @sa setZoomModifiers()
 */
Qt::KeyboardModifiers QwtPlotAxisWheelInteraction::zoomModifiers() const
{
    QWT_DC(d);
    return d->zoomModifiers;
}

/**
 * @brief Set the pan orientation
 * @details When pan mode is active, the canvas is panned along the given
 *          orientation. Qt::Horizontal pans left/right; Qt::Vertical pans
 *          up/down. The default is Qt::Horizontal.
 * @param[in] orientation Pan orientation
 * @sa panOrientation()
 */
void QwtPlotAxisWheelInteraction::setPanOrientation(Qt::Orientation orientation)
{
    QWT_D(d);
    d->panOrientation = orientation;
}

/**
 * @brief Return the pan orientation
 * @return Pan orientation
 * @sa setPanOrientation()
 */
Qt::Orientation QwtPlotAxisWheelInteraction::panOrientation() const
{
    QWT_DC(d);
    return d->panOrientation;
}

/**
 * @brief Set keyboard modifiers that activate pan mode
 * @details When the wheel is rotated with these modifiers pressed, the
 *          canvas is panned. The default is Qt::NoModifier (plain wheel).
 * @param[in] modifiers Keyboard modifiers
 * @sa panModifiers()
 */
void QwtPlotAxisWheelInteraction::setPanModifiers(Qt::KeyboardModifiers modifiers)
{
    QWT_D(d);
    d->panModifiers = modifiers;
}

/**
 * @brief Return the pan modifiers
 * @return Keyboard modifiers for pan mode
 * @sa setPanModifiers()
 */
Qt::KeyboardModifiers QwtPlotAxisWheelInteraction::panModifiers() const
{
    QWT_DC(d);
    return d->panModifiers;
}

/**
 * @brief Set the pan distance per wheel step
 * @param[in] pixelsPerStep Number of pixels to pan per wheel notch (default: 30)
 * @sa panFactor()
 */
void QwtPlotAxisWheelInteraction::setPanFactor(int pixelsPerStep)
{
    QWT_D(d);
    d->panFactor = pixelsPerStep;
}

/**
 * @brief Return the pan factor in pixels
 * @return Pixels per wheel step
 * @sa setPanFactor()
 */
int QwtPlotAxisWheelInteraction::panFactor() const
{
    QWT_DC(d);
    return d->panFactor;
}

/**
 * @brief Return the observed plot canvas
 * @return Pointer to the canvas widget
 */
QWidget* QwtPlotAxisWheelInteraction::canvas()
{
    return qobject_cast< QWidget* >(parent());
}

/**
 * @brief Return the observed plot canvas (const version)
 * @return Const pointer to the canvas widget
 */
const QWidget* QwtPlotAxisWheelInteraction::canvas() const
{
    return qobject_cast< const QWidget* >(parent());
}

/**
 * @brief Return the plot widget containing the observed canvas
 * @return Pointer to the QwtPlot widget, or nullptr if not found
 */
QwtPlot* QwtPlotAxisWheelInteraction::plot()
{
    QWidget* w = canvas();
    if (w)
        w = w->parentWidget();

    return qobject_cast< QwtPlot* >(w);
}

/**
 * @brief Return the plot widget containing the observed canvas (const version)
 * @return Const pointer to the QwtPlot widget, or nullptr if not found
 */
const QwtPlot* QwtPlotAxisWheelInteraction::plot() const
{
    const QWidget* w = canvas();
    if (w)
        w = w->parentWidget();

    return qobject_cast< const QwtPlot* >(w);
}

/**
 * @brief Event filter for wheel events on the canvas
 * @param[in] obj Object receiving the event
 * @param[in] event Event
 * @return true when the wheel event was handled (consumed), false otherwise
 * @sa handleWheelEvent()
 */
bool QwtPlotAxisWheelInteraction::eventFilter(QObject* obj, QEvent* event)
{
    if (obj && obj == parent() && event->type() == QEvent::Wheel) {
        return handleWheelEvent(static_cast< QWheelEvent* >(event));
    }
    return QObject::eventFilter(obj, event);
}

/**
 * @brief Handle a wheel event on the canvas
 * @details The wheel delta is read via qwt::compat::wheelEventDelta() for
 *          Qt5/Qt6 compatibility. When the current modifiers match the
 *          zoom modifiers, the configured axis is zoomed centered at the
 *          mouse position using QwtPlot::zoomAxis(). When they match the
 *          pan modifiers, the canvas is panned via QwtPlot::panCanvas().
 *
 *          Both modes operate on all plots returned by plotList(true),
 *          including parasite plots. A single replotAll() call is issued
 *          after all axis changes. Auto-replot is temporarily disabled
 *          during axis updates to avoid redundant repaints.
 *
 * @param[in] event Wheel event
 * @return true if the event was handled, false if modifiers matched
 *         neither zoom nor pan mode
 */
bool QwtPlotAxisWheelInteraction::handleWheelEvent(QWheelEvent* event)
{
    QWT_D(d);

    QwtPlot* hostPlot = plot();
    if (!hostPlot || hostPlot->isParasitePlot())
        return false;

    const int wheelDelta = qwt::compat::wheelEventDelta(event);

    if (event->modifiers() == d->zoomModifiers) {
        // ---- Zoom mode: zoom the configured axis at mouse position ----

        double f = std::pow(d->zoomFactor, qAbs(wheelDelta / 120.0));
        if (wheelDelta > 0)
            f = 1.0 / f;  // wheel up → zoom in

        const QPoint pos = qwt::compat::eventPos(event);

        const QList< QwtPlot* > allPlots = hostPlot->plotList(true);
        for (QwtPlot* plt : allPlots) {
            plt->saveAutoReplotState();
            plt->setAutoReplot(false);
            plt->zoomAxis(d->zoomAxisId, f, pos);
            plt->restoreAutoReplotState();
        }
        hostPlot->replotAll();
        return true;
    }

    if (event->modifiers() == d->panModifiers) {
        // ---- Pan mode: pan all axes in the configured orientation ----

        const int panPixels = static_cast< int >(wheelDelta / 120.0 * d->panFactor);
        const QPoint offset = (d->panOrientation == Qt::Horizontal)
            ? QPoint(panPixels, 0)
            : QPoint(0, panPixels);

        const QList< QwtPlot* > allPlots = hostPlot->plotList(true);
        for (QwtPlot* plt : allPlots) {
            plt->saveAutoReplotState();
            plt->setAutoReplot(false);
            plt->panCanvas(offset);
            plt->restoreAutoReplotState();
        }
        hostPlot->replotAll();
        return true;
    }

    return false;
}
