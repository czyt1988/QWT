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

#ifndef QWT_PLOT_AXIS_WHEEL_INTERACTION_H
#define QWT_PLOT_AXIS_WHEEL_INTERACTION_H

#include "qwt_global.h"
#include "qwt_axis_id.h"

#include <qobject.h>

class QWheelEvent;
class QEvent;
class QWidget;
class QwtPlot;
class QwtScaleWidget;

/**
 * @brief Configurable wheel interaction for a single plot axis
 *
 * @details QwtPlotAxisWheelInteraction installs an event filter on a
 *          QwtScaleWidget (the axis tick area) and provides two
 *          configurable wheel actions:
 *
 *          - <b>Zoom</b> (plain wheel by default): zooms the axis centered
 *            at the cursor position using QwtPlot::zoomAxis().
 *          - <b>Pan</b> (Ctrl+wheel by default): pans the axis by a fixed
 *            pixel amount per step using QwtPlot::panAxis().
 *
 *          Unlike the built-in QwtPlotScaleEventDispatcher (which hardcodes
 *          wheel=zoom and requires the axis to be clicked/selected first),
 *          this class:
 *          - Supports keyboard modifiers to switch between zoom and pan.
 *          - Works immediately without needing to select the axis first.
 *          - Is fully configurable (modifiers, factors, enable/disable).
 *          - Is designed for inheritance — override wheelZoom(),
 *            wheelPan(), or handleWheelEvent() to implement custom behavior.
 *
 *          When the modifier matches zoom or pan, the event is consumed
 *          (return true) so the dispatcher does not also process it.
 *          When the modifier matches neither, the event is passed through
 *          to the dispatcher for default handling.
 *
 *          Typical usage:
 *          @code
 *          // Plain wheel zooms XBottom at cursor; Ctrl+wheel pans it
 *          new QwtPlotAxisWheelInteraction(plot, QwtAxis::XBottom);
 *
 *          // Customize: Shift+wheel zooms YLeft, plain wheel pans
 *          auto* ix = new QwtPlotAxisWheelInteraction(plot, QwtAxis::YLeft);
 *          ix->setZoomModifiers(Qt::ShiftModifier);
 *          ix->setPanModifiers(Qt::NoModifier);
 *          @endcode
 *
 *          Inheritance example:
 *          @code
 *          class CenterZoom : public QwtPlotAxisWheelInteraction {
 *          public:
 *              using QwtPlotAxisWheelInteraction::QwtPlotAxisWheelInteraction;
 *          protected:
 *              void wheelZoom(double factor, const QPoint&) override {
 *                  // Zoom centered on axis midpoint instead of cursor
 *                  const QwtScaleMap m = plot()->canvasMap(axisId());
 *                  const int mid = static_cast<int>((m.p1() + m.p2()) / 2);
 *                  QwtPlotAxisWheelInteraction::wheelZoom(factor, QPoint(mid, 0));
 *              }
 *          };
 *          @endcode
 *
 * @sa QwtPlotScaleEventDispatcher, QwtPlot::zoomAxis(), QwtPlot::panAxis()
 */
class QWT_EXPORT QwtPlotAxisWheelInteraction : public QObject
{
    Q_OBJECT

public:
    // Construct from a plot and axis id
    explicit QwtPlotAxisWheelInteraction(QwtPlot* plot, QwtAxisId axisId);

    // Construct from a scale widget (auto-derives plot and axis id)
    explicit QwtPlotAxisWheelInteraction(QwtScaleWidget* scaleWidget);

    // Destructor
    ~QwtPlotAxisWheelInteraction() override;

    // Enable or disable the interaction
    void setEnabled(bool on);
    // Return whether the interaction is enabled
    bool isEnabled() const;

    // ---- Zoom configuration (plain wheel by default) ----

    // Set keyboard modifiers for zoom mode (default: Qt::NoModifier)
    void setZoomModifiers(Qt::KeyboardModifiers modifiers);
    // Return the zoom modifiers
    Qt::KeyboardModifiers zoomModifiers() const;

    // Set the zoom factor per wheel step (default: 1.2; >1 zoom in, <1 zoom out)
    void setZoomFactor(double factor);
    // Return the zoom factor
    double zoomFactor() const;

    // ---- Pan configuration (Ctrl+wheel by default) ----

    // Set keyboard modifiers for pan mode (default: Qt::ControlModifier)
    void setPanModifiers(Qt::KeyboardModifiers modifiers);
    // Return the pan modifiers
    Qt::KeyboardModifiers panModifiers() const;

    // Set pixels to pan per wheel step (default: 30)
    void setPanFactor(int pixelsPerStep);
    // Return the pan factor in pixels
    int panFactor() const;

    // Return the observed scale widget
    QwtScaleWidget* scaleWidget();
    // Return the observed scale widget (const version)
    const QwtScaleWidget* scaleWidget() const;

    // Return the plot that owns the axis
    QwtPlot* plot();
    // Return the plot that owns the axis (const version)
    const QwtPlot* plot() const;

    // Return the axis id
    QwtAxisId axisId() const;

protected:
    // Event filter — intercepts wheel events on the scale widget
    virtual bool eventFilter(QObject* obj, QEvent* event) override;

    // Top-level wheel handler. Override for completely custom behavior.
    // Return true if handled (event consumed), false to pass through.
    virtual bool handleWheelEvent(QWheelEvent* event);

    // Called when zoom action is triggered. Override to customize zoom.
    // @param factor Zoom factor (>1 zoom in, <1 zoom out)
    // @param cursorPos Cursor position in canvas coordinates
    virtual void wheelZoom(double factor, const QPoint& cursorPos);

    // Called when pan action is triggered. Override to customize pan.
    // @param deltaPixels Pixel offset (positive = right/down, negative = left/up)
    virtual void wheelPan(int deltaPixels);

private:
    QWT_DECLARE_PRIVATE(QwtPlotAxisWheelInteraction)
};

#endif  // QWT_PLOT_AXIS_WHEEL_INTERACTION_H
