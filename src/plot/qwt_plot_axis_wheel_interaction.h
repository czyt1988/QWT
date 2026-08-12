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

/**
 * @brief Wheel-based axis zoom and canvas pan interaction
 *
 * @details QwtPlotAxisWheelInteraction provides two wheel-driven navigation modes
 *          on the plot canvas:
 *
 *          - <b>Zoom mode</b> (Ctrl + wheel by default): zooms a configurable axis
 *            (default: QwtAxis::XBottom) centered at the current mouse position.
 *          - <b>Pan mode</b> (plain wheel by default): pans all curves in a
 *            configurable orientation (default: horizontal) by a fixed pixel
 *            amount per wheel step.
 *
 *          The class installs an event filter on the plot canvas, similar to
 *          QwtPlotMagnifier. It handles parasite plots automatically by iterating
 *          over hostPlot->plotList() and calling replotAll().
 *
 *          Typical usage:
 *          @code
 *          // Ctrl+wheel zooms XBottom, plain wheel pans horizontally
 *          new QwtPlotAxisWheelInteraction(canvas());
 *
 *          // Customize: Ctrl+wheel zooms YLeft, plain wheel pans vertically
 *          auto* wheel = new QwtPlotAxisWheelInteraction(canvas());
 *          wheel->setZoomAxisId(QwtAxis::YLeft);
 *          wheel->setPanOrientation(Qt::Vertical);
 *
 *          // Coexist with QwtPlotMagnifier (Shift+wheel)
 *          auto* mag = new QwtPlotMagnifier(canvas());
 *          mag->setWheelModifiers(Qt::ShiftModifier);
 *          new QwtPlotAxisWheelInteraction(canvas());
 *          @endcode
 *
 * @sa QwtPlotMagnifier, QwtPlotPanner, QwtPlot::zoomAxis(), QwtPlot::panCanvas()
 */
class QWT_EXPORT QwtPlotAxisWheelInteraction : public QObject
{
    Q_OBJECT

public:
    // Constructor
    explicit QwtPlotAxisWheelInteraction(QWidget* canvas);

    // Destructor
    ~QwtPlotAxisWheelInteraction() override;

    // Enable or disable the interaction
    void setEnabled(bool on);
    // Return whether the interaction is enabled
    bool isEnabled() const;

    // ---- Zoom configuration (Ctrl+wheel by default) ----

    // Set the axis to zoom (default: QwtAxis::XBottom)
    void setZoomAxisId(QwtAxisId axisId);
    QwtAxisId zoomAxisId() const;

    // Set the zoom factor per wheel step (default: 0.9)
    void setZoomFactor(double factor);
    // Return the zoom factor
    double zoomFactor() const;

    // Set keyboard modifiers for zoom mode (default: Qt::ControlModifier)
    void setZoomModifiers(Qt::KeyboardModifiers modifiers);
    // Return the zoom modifiers
    Qt::KeyboardModifiers zoomModifiers() const;

    // ---- Pan configuration (plain wheel by default) ----

    // Set the pan orientation (default: Qt::Horizontal)
    void setPanOrientation(Qt::Orientation orientation);
    // Return the pan orientation
    Qt::Orientation panOrientation() const;

    // Set keyboard modifiers for pan mode (default: Qt::NoModifier)
    void setPanModifiers(Qt::KeyboardModifiers modifiers);
    // Return the pan modifiers
    Qt::KeyboardModifiers panModifiers() const;

    // Set pixels to pan per wheel step (default: 30)
    void setPanFactor(int pixelsPerStep);
    // Return the pan factor in pixels
    int panFactor() const;

    // Return the observed plot canvas
    QWidget* canvas();
    // Return the observed plot canvas (const version)
    const QWidget* canvas() const;

    // Return the plot widget containing the observed canvas
    QwtPlot* plot();
    // Return the plot widget containing the observed canvas (const version)
    const QwtPlot* plot() const;

protected:
    // Event filter handling wheel events on the canvas
    virtual bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QWT_DECLARE_PRIVATE(QwtPlotAxisWheelInteraction)

    bool handleWheelEvent(QWheelEvent* event);
};

#endif  // QWT_PLOT_AXIS_WHEEL_INTERACTION_H
