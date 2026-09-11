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

#include "qwt_magnifier.h"
#include "qwt_math.h"

#include <qevent.h>
#include <qwidget.h>

class QwtMagnifier::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtMagnifier)
public:
    PrivateData(QwtMagnifier* p)
        : q_ptr(p)
        , isEnabled(false)
        , wheelFactor(0.9)
        , wheelModifiers(Qt::NoModifier)
        , mouseFactor(0.95)
        , mouseButton(Qt::RightButton)
        , mouseButtonModifiers(Qt::NoModifier)
        , keyFactor(0.9)
        , zoomInKey(Qt::Key_Plus)
        , zoomInKeyModifiers(Qt::NoModifier)
        , zoomOutKey(Qt::Key_Minus)
        , zoomOutKeyModifiers(Qt::NoModifier)
        , mousePressed(false)
        , hasMouseTracking(false)
    {
    }

    bool isEnabled;

    double wheelFactor;
    Qt::KeyboardModifiers wheelModifiers;

    double mouseFactor;

    Qt::MouseButton mouseButton;
    Qt::KeyboardModifiers mouseButtonModifiers;

    double keyFactor;

    int zoomInKey;
    Qt::KeyboardModifiers zoomInKeyModifiers;

    int zoomOutKey;
    Qt::KeyboardModifiers zoomOutKeyModifiers;

    bool mousePressed;
    bool hasMouseTracking;
    QPoint mousePos;
};

/**
 * @brief Constructor
 * @param[in] parent Widget to be magnified
 */
QwtMagnifier::QwtMagnifier(QWidget* parent) : QObject(parent), QWT_PIMPL_CONSTRUCT
{
    if (parent) {
        if (parent->focusPolicy() == Qt::NoFocus)
            parent->setFocusPolicy(Qt::WheelFocus);
    }

    setEnabled(true);
}

/**
 * @brief Destructor
 */
QwtMagnifier::~QwtMagnifier()
{
}

/**
 * @brief En/disable the magnifier
 * @details When enabled is true an event filter is installed for
 *          the observed widget, otherwise the event filter is removed.
 * @param[in] on true or false
 * @sa isEnabled(), eventFilter()
 */
void QwtMagnifier::setEnabled(bool on)
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
 * @brief Return whether the magnifier is enabled
 * @return true when enabled, false otherwise
 * @sa setEnabled(), eventFilter()
 */
bool QwtMagnifier::isEnabled() const
{
    QWT_DC(d);
    return d->isEnabled;
}

/**
 * @brief Change the wheel factor
 * @details The wheel factor defines the ratio between the current range
 *          on the parent widget and the zoomed range for each step of the wheel.
 *          Use values > 1 for magnification (i.e. 2.0) and values < 1 for
 *          scaling down (i.e. 1/2.0 = 0.5). You can use this feature for
 *          inverting the direction of the wheel.
 *          The default value is 0.9.
 * @param[in] factor Wheel factor
 * @sa wheelFactor(), setWheelButtonState(), setMouseFactor(), setKeyFactor()
 */
void QwtMagnifier::setWheelFactor(double factor)
{
    QWT_D(d);
    d->wheelFactor = factor;
}

/**
 * @brief Return the wheel factor
 * @return Wheel factor
 * @sa setWheelFactor()
 */
double QwtMagnifier::wheelFactor() const
{
    QWT_DC(d);
    return d->wheelFactor;
}

/**
 * @brief Assign keyboard modifiers for zooming in/out using the wheel
 * @details The default modifiers are Qt::NoModifiers.
 * @param[in] modifiers Keyboard modifiers
 * @sa wheelModifiers()
 */
void QwtMagnifier::setWheelModifiers(Qt::KeyboardModifiers modifiers)
{
    QWT_D(d);
    d->wheelModifiers = modifiers;
}

/**
 * @brief Return the wheel modifiers
 * @return Wheel modifiers
 * @sa setWheelModifiers()
 */
Qt::KeyboardModifiers QwtMagnifier::wheelModifiers() const
{
    QWT_DC(d);
    return d->wheelModifiers;
}

/**
 * @brief Change the mouse factor
 * @details The mouse factor defines the ratio between the current range
 *          on the parent widget and the zoomed range for each vertical mouse movement.
 *          The default value is 0.95.
 * @param[in] factor Mouse factor
 * @sa mouseFactor(), setMouseButton(), setWheelFactor(), setKeyFactor()
 */
void QwtMagnifier::setMouseFactor(double factor)
{
    QWT_D(d);
    d->mouseFactor = factor;
}

/**
 * @brief Return the mouse factor
 * @return Mouse factor
 * @sa setMouseFactor()
 */
double QwtMagnifier::mouseFactor() const
{
    QWT_DC(d);
    return d->mouseFactor;
}

/**
 * @brief Assign the mouse button that is used for zooming in/out
 * @details The default value is Qt::RightButton.
 * @param[in] button Button
 * @param[in] modifiers Keyboard modifiers
 * @sa getMouseButton()
 */
void QwtMagnifier::setMouseButton(Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
{
    QWT_D(d);
    d->mouseButton          = button;
    d->mouseButtonModifiers = modifiers;
}

/**
 * @brief Get the mouse button and modifiers used for zooming
 * @param[out] button Mouse button used for zooming
 * @param[out] modifiers Keyboard modifiers used for zooming
 * @sa setMouseButton()
 */
void QwtMagnifier::getMouseButton(Qt::MouseButton& button, Qt::KeyboardModifiers& modifiers) const
{
    QWT_DC(d);
    button    = d->mouseButton;
    modifiers = d->mouseButtonModifiers;
}

/**
 * @brief Change the key factor
 * @details The key factor defines the ratio between the current range
 *          on the parent widget and the zoomed range for each key press of
 *          the zoom in/out keys. The default value is 0.9.
 * @param[in] factor Key factor
 * @sa keyFactor(), setZoomInKey(), setZoomOutKey(), setWheelFactor(), setMouseFactor()
 */
void QwtMagnifier::setKeyFactor(double factor)
{
    QWT_D(d);
    d->keyFactor = factor;
}

/**
 * @brief Return the key factor
 * @return Key factor
 * @sa setKeyFactor()
 */
double QwtMagnifier::keyFactor() const
{
    QWT_DC(d);
    return d->keyFactor;
}

/**
 * @brief Assign the key that is used for zooming in
 * @details The default combination is Qt::Key_Plus + Qt::NoModifier.
 * @param[in] key Key code
 * @param[in] modifiers Keyboard modifiers
 * @sa getZoomInKey(), setZoomOutKey()
 */
void QwtMagnifier::setZoomInKey(int key, Qt::KeyboardModifiers modifiers)
{
    QWT_D(d);
    d->zoomInKey          = key;
    d->zoomInKeyModifiers = modifiers;
}

/**
 * @brief Retrieve the settings of the zoom in key
 * @param[out] key Key code, see Qt::Key
 * @param[out] modifiers Keyboard modifiers
 * @sa setZoomInKey()
 */
void QwtMagnifier::getZoomInKey(int& key, Qt::KeyboardModifiers& modifiers) const
{
    QWT_DC(d);
    key       = d->zoomInKey;
    modifiers = d->zoomInKeyModifiers;
}

/**
 * @brief Assign the key that is used for zooming out
 * @details The default combination is Qt::Key_Minus + Qt::NoModifier.
 * @param[in] key Key code
 * @param[in] modifiers Keyboard modifiers
 * @sa getZoomOutKey(), setZoomInKey()
 */
void QwtMagnifier::setZoomOutKey(int key, Qt::KeyboardModifiers modifiers)
{
    QWT_D(d);
    d->zoomOutKey          = key;
    d->zoomOutKeyModifiers = modifiers;
}

/**
 * @brief Retrieve the settings of the zoom out key
 * @param[out] key Key code, see Qt::Key
 * @param[out] modifiers Keyboard modifiers
 * @sa setZoomOutKey()
 */
void QwtMagnifier::getZoomOutKey(int& key, Qt::KeyboardModifiers& modifiers) const
{
    QWT_DC(d);
    key       = d->zoomOutKey;
    modifiers = d->zoomOutKeyModifiers;
}

/**
 * @brief Event filter
 * @details When isEnabled() is true, the mouse events of the
 *          observed widget are filtered.
 * @param[in] object Object to be filtered
 * @param[in] event Event
 * @return Forwarded to QObject::eventFilter()
 * @sa widgetMousePressEvent(), widgetMouseReleaseEvent(),
 *     widgetMouseMoveEvent(), widgetWheelEvent(), widgetKeyPressEvent(),
 *     widgetKeyReleaseEvent()
 */
bool QwtMagnifier::eventFilter(QObject* object, QEvent* event)
{
    if (object && object == parent()) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            widgetMousePressEvent(static_cast< QMouseEvent* >(event));
            break;
        }
        case QEvent::MouseMove: {
            widgetMouseMoveEvent(static_cast< QMouseEvent* >(event));
            break;
        }
        case QEvent::MouseButtonRelease: {
            widgetMouseReleaseEvent(static_cast< QMouseEvent* >(event));
            break;
        }
        case QEvent::Wheel: {
            if (widgetWheelEvent(static_cast< QWheelEvent* >(event))) {
                // Consume the event to stop propagation to parent widgets
                // (e.g. a QScrollArea viewport would scroll otherwise)
                event->accept();
                return true;
            }
            break;
        }
        case QEvent::KeyPress: {
            widgetKeyPressEvent(static_cast< QKeyEvent* >(event));
            break;
        }
        case QEvent::KeyRelease: {
            widgetKeyReleaseEvent(static_cast< QKeyEvent* >(event));
            break;
        }
        default:;
        }
    }
    return QObject::eventFilter(object, event);
}

/*!
   Handle a mouse press event for the observed widget.

   @param mouseEvent Mouse event
   @sa eventFilter(), widgetMouseReleaseEvent(), widgetMouseMoveEvent()
 */
void QwtMagnifier::widgetMousePressEvent(QMouseEvent* mouseEvent)
{
    QWT_D(d);
    if (parentWidget() == nullptr)
        return;

    if ((mouseEvent->button() != d->mouseButton) || (mouseEvent->modifiers() != d->mouseButtonModifiers)) {
        return;
    }

    d->hasMouseTracking = parentWidget()->hasMouseTracking();

    parentWidget()->setMouseTracking(true);
    d->mousePos     = mouseEvent->pos();
    d->mousePressed = true;
}

/*!
   Handle a mouse release event for the observed widget.

   @param mouseEvent Mouse event

   @sa eventFilter(), widgetMousePressEvent(), widgetMouseMoveEvent(),
 */
void QwtMagnifier::widgetMouseReleaseEvent(QMouseEvent* mouseEvent)
{
    QWT_D(d);
    Q_UNUSED(mouseEvent);

    if (d->mousePressed && parentWidget()) {
        d->mousePressed = false;
        parentWidget()->setMouseTracking(d->hasMouseTracking);
    }
}

/*!
   Handle a mouse move event for the observed widget.

   @param mouseEvent Mouse event
   @sa eventFilter(), widgetMousePressEvent(), widgetMouseReleaseEvent(),
 */
void QwtMagnifier::widgetMouseMoveEvent(QMouseEvent* mouseEvent)
{
    QWT_D(d);
    if (!d->mousePressed)
        return;

    const int dy = mouseEvent->pos().y() - d->mousePos.y();
    if (dy != 0) {
        double f = d->mouseFactor;
        if (dy < 0)
            f = 1 / f;

        rescale(f);
    }

    d->mousePos = mouseEvent->pos();
}

/*!
   Handle a wheel event for the observed widget.

   @param wheelEvent Wheel event
   @return true when the event was consumed ( rescaled ), false otherwise
   @sa eventFilter()
 */
bool QwtMagnifier::widgetWheelEvent(QWheelEvent* wheelEvent)
{
    QWT_D(d);
    if (wheelEvent->modifiers() != d->wheelModifiers) {
        return false;
    }

    if (d->wheelFactor != 0.0) {
#if QT_VERSION < 0x050000
        const int wheelDelta = wheelEvent->delta();
#else
        const QPoint delta   = wheelEvent->angleDelta();
        const int wheelDelta = (qAbs(delta.x()) > qAbs(delta.y())) ? delta.x() : delta.y();
#endif

        /*
            A positive delta indicates that the wheel was
            rotated forwards away from the user; a negative
            value indicates that the wheel was rotated
            backwards toward the user.
            Most mouse types work in steps of 15 degrees,
            in which case the delta value is a multiple
            of 120 (== 15 * 8).
         */
        double f = std::pow(d->wheelFactor, qAbs(wheelDelta / 120.0));

        if (wheelDelta > 0)
            f = 1 / f;

        rescale(f);
        return true;
    }
    return false;
}

/*!
   Handle a key press event for the observed widget.

   @param keyEvent Key event
   @sa eventFilter(), widgetKeyReleaseEvent()
 */
void QwtMagnifier::widgetKeyPressEvent(QKeyEvent* keyEvent)
{
    QWT_D(d);
    if (keyEvent->key() == d->zoomInKey && keyEvent->modifiers() == d->zoomInKeyModifiers) {
        rescale(d->keyFactor);
    } else if (keyEvent->key() == d->zoomOutKey && keyEvent->modifiers() == d->zoomOutKeyModifiers) {
        rescale(1.0 / d->keyFactor);
    }
}

/*!
   Handle a key release event for the observed widget.

   @param keyEvent Key event
   @sa eventFilter(), widgetKeyReleaseEvent()
 */
void QwtMagnifier::widgetKeyReleaseEvent(QKeyEvent* keyEvent)
{
    Q_UNUSED(keyEvent);
}

/**
 * @brief Return the parent widget where the rescaling happens
 * @return Parent widget
 */
QWidget* QwtMagnifier::parentWidget()
{
    return qobject_cast< QWidget* >(parent());
}

/**
 * @brief Return the parent widget where the rescaling happens
 * @return Parent widget (const)
 */
const QWidget* QwtMagnifier::parentWidget() const
{
    return qobject_cast< const QWidget* >(parent());
}
