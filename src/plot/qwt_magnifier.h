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
 *****************************************************************************/

#ifndef QWT_MAGNIFIER_H
#define QWT_MAGNIFIER_H

#include "qwt_global.h"
#include <qobject.h>

class QWidget;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;

/**
 * @brief QwtMagnifier provides zooming by magnifying in steps
 * @details Using QwtMagnifier a plot can be zoomed in/out in steps using
 *          keys, the mouse wheel or moving a mouse button in vertical direction.
 */
class QWT_EXPORT QwtMagnifier : public QObject
{
    Q_OBJECT

public:
    // Constructor
    explicit QwtMagnifier(QWidget*);
    // Destructor
    ~QwtMagnifier() override;

    // Return the parent widget (non-const version)
    QWidget* parentWidget();
    // Return the parent widget (const version)
    const QWidget* parentWidget() const;

    // Enable or disable the magnifier
    void setEnabled(bool);
    // Return whether the magnifier is enabled
    bool isEnabled() const;

    // mouse

    // Set the mouse factor for zooming
    void setMouseFactor(double);
    // Return the mouse factor
    double mouseFactor() const;

    // Set the mouse button for zooming
    void setMouseButton(Qt::MouseButton, Qt::KeyboardModifiers = Qt::NoModifier);
    // Get the mouse button and modifiers
    void getMouseButton(Qt::MouseButton&, Qt::KeyboardModifiers&) const;

    // mouse wheel

    // Set the wheel factor for zooming
    void setWheelFactor(double);
    // Return the wheel factor
    double wheelFactor() const;

    // Set the wheel modifiers
    void setWheelModifiers(Qt::KeyboardModifiers);
    // Return the wheel modifiers
    Qt::KeyboardModifiers wheelModifiers() const;

    // keyboard

    // Set the key factor for zooming
    void setKeyFactor(double);
    // Return the key factor
    double keyFactor() const;

    // Set the zoom in key and modifiers
    void setZoomInKey(int key, Qt::KeyboardModifiers = Qt::NoModifier);
    // Get the zoom in key and modifiers
    void getZoomInKey(int& key, Qt::KeyboardModifiers&) const;

    // Set the zoom out key and modifiers
    void setZoomOutKey(int key, Qt::KeyboardModifiers = Qt::NoModifier);
    // Get the zoom out key and modifiers
    void getZoomOutKey(int& key, Qt::KeyboardModifiers&) const;

    // Event filter for mouse and keyboard events
    virtual bool eventFilter(QObject*, QEvent*) override;

protected:
    // Rescale the parent widget
    virtual void rescale(double factor) = 0;

    virtual void widgetMousePressEvent(QMouseEvent*);
    virtual void widgetMouseReleaseEvent(QMouseEvent*);
    virtual void widgetMouseMoveEvent(QMouseEvent*);
    /*!
       Handle a wheel event for the observed widget.
       \return true when the event was consumed ( rescaled ), so the caller
               can accept it to stop propagation to parent widgets.
               false when the event is not handled and should propagate.
     */
    virtual bool widgetWheelEvent(QWheelEvent*);
    virtual void widgetKeyPressEvent(QKeyEvent*);
    virtual void widgetKeyReleaseEvent(QKeyEvent*);

private:
    QWT_DECLARE_PRIVATE(QwtMagnifier)
};

#endif
