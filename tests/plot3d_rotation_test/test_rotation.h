#ifndef TEST_ROTATION_H
#define TEST_ROTATION_H

#include <QtTest/QtTest>

#include "qwt3d_plot.h"

/**
 * @brief Wrapper exposing the protected input handlers of Qwt3DPlot
 * @details The tests drive the handlers with real QMouseEvent/QKeyEvent objects
 *          instead of a shown widget, so they need no GL context and no window
 *          system (the plot is never shown, initializeGL() never runs).
 */
class TestPlot : public Qwt3DPlot
{
public:
    using Qwt3DPlot::keyPressEvent;
    using Qwt3DPlot::mouseMoveEvent;
    using Qwt3DPlot::mousePressEvent;
    using Qwt3DPlot::mouseReleaseEvent;
};

/**
 * @brief Unit tests for the mouse/keyboard driven view rotation of Qwt3DPlot
 * @details Covers the drag gains, the sub degree resolution of the stored
 *          angles, the wrap into [0, 360) and the gesture to angle mapping.
 *          Rendering is not involved.
 */
class TestRotation : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // Speed configuration
    void defaultMouseSpeed();
    void setMouseSpeedIsApplied();
    void setMouseSpeedIgnoresNonPositive();

    // Drag response
    void dragGainIsHalfATurnPerWidget();
    void verticalDragChangesElevationOnly();
    void subDegreeDragAccumulates();
    void dragWrapsIntoUnitRange();
    void setRotationIsNotQuantized();

    // Gesture to axis mapping
    void shiftDragRolls();
    void middleButtonRolls();
    void rollMouseIsConfigurable();

    // Keyboard
    void keyboardStepsAccumulateWithoutRounding();
    void keyboardWrapsIntoUnitRange();
};

#endif // TEST_ROTATION_H
