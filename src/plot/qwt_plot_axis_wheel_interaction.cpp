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

#include "qwt_math.h"
#include "qwt_plot.h"
#include "qwt_scale_widget.h"
#include "qwt_plot_scale_event_dispatcher.h"
#include "qwt_qt5qt6_compat.hpp"

class QwtPlotAxisWheelInteraction::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotAxisWheelInteraction)

public:
    PrivateData(QwtPlotAxisWheelInteraction* p)
        : q_ptr(p)
        , isEnabled(false)
        , plot(nullptr)
        , scaleWidget(nullptr)
        , axisId(QwtAxis::AxisPositions)
        , zoomFactor(1.2)
        , zoomModifiers(Qt::NoModifier)
        , panModifiers(Qt::ControlModifier)
        , panFactor(30)
    {
    }

    bool isEnabled;
    QwtPlot* plot;
    QwtScaleWidget* scaleWidget;
    QwtAxisId axisId;

    double zoomFactor;
    Qt::KeyboardModifiers zoomModifiers;

    Qt::KeyboardModifiers panModifiers;
    int panFactor;
};

/**
 * @brief Constructor from plot and axis id
 * @param[in] plot The plot that owns the axis
 * @param[in] axisId The axis to attach the interaction to
 *
 * The event filter is installed on the QwtScaleWidget for the given axis.
 * The interaction is enabled by default.
 */
QwtPlotAxisWheelInteraction::QwtPlotAxisWheelInteraction(QwtPlot* plot, QwtAxisId axisId)
    : QObject(plot), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    d->plot = plot;
    d->axisId = axisId;
    if (plot)
        d->scaleWidget = plot->axisWidget(axisId);

    setEnabled(true);
}

/**
 * @brief Constructor from a scale widget
 * @param[in] scaleWidget The axis scale widget to observe
 *
 * The plot and axis id are auto-derived from the scale widget's parent
 * (which must be a QwtPlot) using QwtPlotScaleEventDispatcher::findAxisIdByScaleWidget().
 */
QwtPlotAxisWheelInteraction::QwtPlotAxisWheelInteraction(QwtScaleWidget* scaleWidget)
    : QObject(scaleWidget), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    d->scaleWidget = scaleWidget;
    if (scaleWidget) {
        d->plot = qobject_cast< QwtPlot* >(scaleWidget->parentWidget());
        if (d->plot) {
            d->axisId = QwtPlotScaleEventDispatcher::findAxisIdByScaleWidget(
                d->plot, scaleWidget);
        }
    }

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
 * @details When enabled, an event filter is installed on the observed
 *          scale widget to intercept wheel events before they reach the
 *          QwtPlotScaleEventDispatcher. When disabled, the filter is removed
 *          and the dispatcher regains full control.
 * @param[in] on true to enable, false to disable
 * @sa isEnabled(), eventFilter()
 */
void QwtPlotAxisWheelInteraction::setEnabled(bool on)
{
    QWT_D(d);
    if (d->isEnabled != on) {
        d->isEnabled = on;

        QObject* o = d->scaleWidget;
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
 * @brief Set keyboard modifiers for zoom mode
 * @details When the wheel is rotated with these modifiers on the scale
 *          widget, the axis is zoomed. The default is Qt::NoModifier
 *          (plain wheel).
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
 * @brief Set the zoom factor per wheel step
 * @details Values > 1 zoom in (range shrinks), values < 1 zoom out
 *          (range expands). The default is 1.2. For multi-step wheels
 *          the factor is compounded: pow(zoomFactor, abs(delta/120)).
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
 * @brief Set keyboard modifiers for pan mode
 * @details When the wheel is rotated with these modifiers on the scale
 *          widget, the axis is panned. The default is Qt::ControlModifier.
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
 * @brief Return the observed scale widget
 * @return Pointer to the scale widget
 */
QwtScaleWidget* QwtPlotAxisWheelInteraction::scaleWidget()
{
    QWT_DC(d);
    return d->scaleWidget;
}

/**
 * @brief Return the observed scale widget (const version)
 * @return Const pointer to the scale widget
 */
const QwtScaleWidget* QwtPlotAxisWheelInteraction::scaleWidget() const
{
    QWT_DC(d);
    return d->scaleWidget;
}

/**
 * @brief Return the plot that owns the axis
 * @return Pointer to the QwtPlot widget
 */
QwtPlot* QwtPlotAxisWheelInteraction::plot()
{
    QWT_DC(d);
    return d->plot;
}

/**
 * @brief Return the plot that owns the axis (const version)
 * @return Const pointer to the QwtPlot widget
 */
const QwtPlot* QwtPlotAxisWheelInteraction::plot() const
{
    QWT_DC(d);
    return d->plot;
}

/**
 * @brief Return the axis id
 * @return Axis identifier
 */
QwtAxisId QwtPlotAxisWheelInteraction::axisId() const
{
    QWT_DC(d);
    return d->axisId;
}

/**
 * @brief Event filter for wheel events on the scale widget
 * @param[in] obj Object receiving the event (should be the scale widget)
 * @param[in] event Event
 * @return true when the wheel event was handled (consumed), false otherwise
 * @sa handleWheelEvent()
 */
bool QwtPlotAxisWheelInteraction::eventFilter(QObject* obj, QEvent* event)
{
    QWT_D(d);
    if (obj && obj == d->scaleWidget && event->type() == QEvent::Wheel) {
        return handleWheelEvent(static_cast< QWheelEvent* >(event));
    }
    return QObject::eventFilter(obj, event);
}

/**
 * @brief Handle a wheel event on the scale widget
 * @details The wheel delta is read via qwt::compat::wheelEventDelta() for
 *          Qt5/Qt6 compatibility. When the current modifiers match the
 *          zoom modifiers, wheelZoom() is called. When they match the
 *          pan modifiers, wheelPan() is called. Otherwise the event is
 *          passed through (return false) so the
 *          QwtPlotScaleEventDispatcher can handle it.
 *
 * @param[in] event Wheel event
 * @return true if the event was handled, false if modifiers matched
 *         neither zoom nor pan mode
 */
bool QwtPlotAxisWheelInteraction::handleWheelEvent(QWheelEvent* event)
{
    QWT_D(d);
    if (!d->plot || !d->scaleWidget)
        return false;

    const int wheelDelta = qwt::compat::wheelEventDelta(event);

    if (event->modifiers() == d->zoomModifiers) {
        // ---- Zoom mode: zoom the axis at cursor position ----
        // factor > 1 → zoom in (range shrinks); factor < 1 → zoom out (range expands)
        const double steps = qAbs(wheelDelta / 120.0);
        double factor = std::pow(d->zoomFactor, steps);
        if (wheelDelta < 0)
            factor = 1.0 / factor;  // wheel down → zoom out

        // Convert the event position from scale-widget coordinates to canvas coordinates
        const QPoint scalePos = qwt::compat::eventPos(event);
        const QPoint globalPos = d->scaleWidget->mapToGlobal(scalePos);
        const QPoint canvasPos = d->plot->canvas()->mapFromGlobal(globalPos);

        wheelZoom(factor, canvasPos);
        return true;
    }

    if (event->modifiers() == d->panModifiers) {
        // ---- Pan mode: pan the axis ----
        const int deltaPixels = static_cast< int >(wheelDelta / 120.0 * d->panFactor);
        wheelPan(deltaPixels);
        return true;
    }

    return false;  // Modifiers matched neither mode — let the dispatcher handle it
}

/**
 * @brief Perform a zoom on the axis
 * @details Default implementation calls QwtPlot::zoomAxis() with the given
 *          factor and cursor position, then replotAll(). Auto-replot is
 *          temporarily disabled to avoid redundant repaints.
 *
 *          Override this method to customize zoom behavior, e.g. zoom
 *          centered on the axis midpoint instead of the cursor position.
 *
 * @param[in] factor Zoom factor (>1 zoom in, <1 zoom out)
 * @param[in] cursorPos Cursor position in canvas coordinates
 */
void QwtPlotAxisWheelInteraction::wheelZoom(double factor, const QPoint& cursorPos)
{
    QWT_D(d);
    if (!d->plot)
        return;

    d->plot->saveAutoReplotState();
    d->plot->setAutoReplot(false);
    d->plot->zoomAxis(d->axisId, factor, cursorPos);
    d->plot->restoreAutoReplotState();
    d->plot->replotAll();
}

/**
 * @brief Perform a pan on the axis
 * @details Default implementation calls QwtPlot::panAxis() with the given
 *          pixel delta, then replotAll(). Auto-replot is temporarily
 *          disabled to avoid redundant repaints.
 *
 *          Override this method to customize pan behavior, e.g. invert
 *          direction or add momentum.
 *
 * @param[in] deltaPixels Pixel offset (positive = right/down, negative = left/up)
 */
void QwtPlotAxisWheelInteraction::wheelPan(int deltaPixels)
{
    QWT_D(d);
    if (!d->plot || deltaPixels == 0)
        return;

    d->plot->saveAutoReplotState();
    d->plot->setAutoReplot(false);
    d->plot->panAxis(d->axisId, deltaPixels);
    d->plot->restoreAutoReplotState();
    d->plot->replotAll();
}
