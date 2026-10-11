#include "test_rotation.h"

#include <QKeyEvent>
#include <QMouseEvent>

#include <algorithm>

namespace
{

const int PLOT_WIDTH  = 400;
const int PLOT_HEIGHT = 300;

/// Turns per widget dimension of the default mouse speed: half a turn
const double DEFAULT_TURNS = 0.5;

/**
 * @brief Sends a drag as a sequence of 1 px moves
 * @details The handlers accumulate per mouse move event, so single pixel steps
 *          are what makes the sub degree resolution observable.
 */
void drag(TestPlot& plot,
    QPoint from,
    QPoint to,
    Qt::MouseButton button,
    Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(from), QPointF(from), button, button, modifiers);
    plot.mousePressEvent(&press);

    const QPoint delta = to - from;
    const int steps    = std::max(qAbs(delta.x()), qAbs(delta.y()));
    for (int i = 1; i <= steps; ++i) {
        const QPoint p = from + QPoint(delta.x() * i / steps, delta.y() * i / steps);
        QMouseEvent move(QEvent::MouseMove, QPointF(p), QPointF(p), Qt::NoButton, button, modifiers);
        plot.mouseMoveEvent(&move);
    }

    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(to), QPointF(to), button, Qt::NoButton, modifiers);
    plot.mouseReleaseEvent(&release);
}

/// Sends one key press
void press(TestPlot& plot, int key, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    QKeyEvent event(QEvent::KeyPress, key, modifiers);
    plot.keyPressEvent(&event);
}

/// Rotation one rotation key press moves the z axis by
double keyboardZStep(TestPlot& plot)
{
    double rot   = 0.0;
    double scale = 0.0;
    double shift = 0.0;
    plot.keySpeed(rot, scale, shift);

    return rot * 360 / plot.width();
}

} // namespace

// ---------------------------------------------------------------------------
// Speed configuration
// ---------------------------------------------------------------------------

void TestRotation::defaultMouseSpeed()
{
    TestPlot plot;

    double rot   = 0.0;
    double scale = 0.0;
    double shift = 0.0;
    plot.mouseSpeed(rot, scale, shift);

    QCOMPARE(rot, DEFAULT_TURNS);
    QCOMPARE(scale, 5.0);
    QCOMPARE(shift, 2.0);
}

void TestRotation::setMouseSpeedIsApplied()
{
    TestPlot plot;
    plot.resize(PLOT_WIDTH, PLOT_HEIGHT);
    plot.setMouseSpeed(1.0, 0.0, 0.0);  // 0 keeps the current scaling/shifting

    double rot   = 0.0;
    double scale = 0.0;
    double shift = 0.0;
    plot.mouseSpeed(rot, scale, shift);
    QCOMPARE(rot, 1.0);
    QCOMPARE(scale, 5.0);
    QCOMPARE(shift, 2.0);

    // One turn per widget width: a quarter of the width rotates by 90 degrees
    drag(plot, QPoint(0, 0), QPoint(PLOT_WIDTH / 4, 0), Qt::LeftButton);
    QCOMPARE(plot.zRotation(), 90.0);
}

void TestRotation::setMouseSpeedIgnoresNonPositive()
{
    TestPlot plot;
    plot.resize(PLOT_WIDTH, PLOT_HEIGHT);
    plot.setMouseSpeed(-1.0, -1.0, -1.0);

    double rot   = 0.0;
    double scale = 0.0;
    double shift = 0.0;
    plot.mouseSpeed(rot, scale, shift);

    QCOMPARE(rot, DEFAULT_TURNS);
    QCOMPARE(scale, 5.0);
    QCOMPARE(shift, 2.0);
}

// ---------------------------------------------------------------------------
// Drag response
// ---------------------------------------------------------------------------

void TestRotation::dragGainIsHalfATurnPerWidget()
{
    TestPlot plot;
    plot.resize(PLOT_WIDTH, PLOT_HEIGHT);

    // The default is half a turn per widget dimension
    drag(plot, QPoint(0, 0), QPoint(PLOT_WIDTH, 0), Qt::LeftButton);
    QCOMPARE(plot.zRotation(), 0.5 * 360.0);

    plot.setRotation(0, 0, 0);
    drag(plot, QPoint(0, 0), QPoint(0, PLOT_HEIGHT), Qt::LeftButton);
    QCOMPARE(plot.xRotation(), 0.5 * 360.0);
}

void TestRotation::verticalDragChangesElevationOnly()
{
    TestPlot plot;
    plot.resize(PLOT_WIDTH, PLOT_HEIGHT);
    plot.setRotation(45.0, 10.0, 20.0);

    // Dragging downwards pushes the near face down, so the elevation rises
    drag(plot, QPoint(0, 0), QPoint(0, 30), Qt::LeftButton);
    QCOMPARE(plot.xRotation(), 45.0 + 30.0 * DEFAULT_TURNS * 360 / PLOT_HEIGHT);
    QCOMPARE(plot.yRotation(), 10.0);
    QCOMPARE(plot.zRotation(), 20.0);
}

void TestRotation::subDegreeDragAccumulates()
{
    TestPlot plot;
    plot.resize(2000, PLOT_HEIGHT);  // wide widget: one pixel is 0.09 degrees

    // 20 single pixel moves must accumulate to 1.8 degrees; an implementation
    // that quantizes every step to whole degrees loses them all
    drag(plot, QPoint(0, 0), QPoint(20, 0), Qt::LeftButton);
    QCOMPARE(plot.zRotation(), 20.0 * DEFAULT_TURNS * 360 / 2000);

    // ... and it keeps accumulating in the same direction
    drag(plot, QPoint(0, 0), QPoint(20, 0), Qt::LeftButton);
    QCOMPARE(plot.zRotation(), 2.0 * 20.0 * DEFAULT_TURNS * 360 / 2000);
}

void TestRotation::dragWrapsIntoUnitRange()
{
    TestPlot plot;
    plot.resize(PLOT_WIDTH, PLOT_HEIGHT);

    // The horizontal drag drives the z rotation: 350 + 45 wraps to 35
    plot.setRotation(0.0, 0.0, 350.0);
    drag(plot, QPoint(0, 0), QPoint(PLOT_WIDTH / 4, 0), Qt::LeftButton);
    QCOMPARE(plot.zRotation(), 35.0);

    // Negative results wrap as well: 10 - 45 = -35 -> 325
    plot.setRotation(0.0, 0.0, 10.0);
    drag(plot, QPoint(PLOT_WIDTH / 4, 0), QPoint(0, 0), Qt::LeftButton);
    QCOMPARE(plot.zRotation(), 325.0);
}

void TestRotation::setRotationIsNotQuantized()
{
    TestPlot plot;

    // setRotation() stores what it is given; only the interaction handlers
    // canonicalize their results
    plot.setRotation(-30.25, 400.5, 12.75);
    QCOMPARE(plot.xRotation(), -30.25);
    QCOMPARE(plot.yRotation(), 400.5);
    QCOMPARE(plot.zRotation(), 12.75);
}

// ---------------------------------------------------------------------------
// Gesture to axis mapping
// ---------------------------------------------------------------------------

void TestRotation::shiftDragRolls()
{
    TestPlot plot;
    plot.resize(PLOT_WIDTH, PLOT_HEIGHT);
    plot.setRotation(45.0, 10.0, 20.0);

    drag(plot, QPoint(0, 0), QPoint(PLOT_WIDTH / 4, 0), Qt::LeftButton, Qt::ShiftModifier);
    QCOMPARE(plot.yRotation(), 10.0 + PLOT_WIDTH / 4.0 * DEFAULT_TURNS * 360 / PLOT_WIDTH);
    QCOMPARE(plot.xRotation(), 45.0);
    QCOMPARE(plot.zRotation(), 20.0);
}

void TestRotation::middleButtonRolls()
{
    TestPlot plot;
    plot.resize(PLOT_WIDTH, PLOT_HEIGHT);
    plot.setRotation(45.0, 10.0, 20.0);

    QCOMPARE(plot.rollMouse(), Qwt3DMouseState(Qt::MiddleButton));

    drag(plot, QPoint(0, 0), QPoint(PLOT_WIDTH / 4, 0), Qt::MiddleButton);
    QCOMPARE(plot.yRotation(), 10.0 + PLOT_WIDTH / 4.0 * DEFAULT_TURNS * 360 / PLOT_WIDTH);
    QCOMPARE(plot.xRotation(), 45.0);
    QCOMPARE(plot.zRotation(), 20.0);
}

void TestRotation::rollMouseIsConfigurable()
{
    TestPlot plot;
    plot.resize(PLOT_WIDTH, PLOT_HEIGHT);
    const double roll = PLOT_WIDTH / 4.0 * DEFAULT_TURNS * 360 / PLOT_WIDTH;

    // Turning the extra gesture off
    plot.setRollMouse(Qwt3DMouseState());
    plot.setRotation(45.0, 10.0, 20.0);
    drag(plot, QPoint(0, 0), QPoint(PLOT_WIDTH / 4, 0), Qt::MiddleButton);
    QCOMPARE(plot.yRotation(), 10.0);

    // Binding it to another button
    plot.setRollMouse(Qwt3DMouseState(Qt::RightButton));
    drag(plot, QPoint(0, 0), QPoint(PLOT_WIDTH / 4, 0), Qt::RightButton);
    QCOMPARE(plot.yRotation(), 10.0 + roll);
}

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------

void TestRotation::keyboardStepsAccumulateWithoutRounding()
{
    TestPlot plot;
    plot.resize(2000, PLOT_HEIGHT);  // one press is 0.54 degrees
    plot.setRotation(0.0, 0.0, 0.0);

    const double step = keyboardZStep(plot);
    QVERIFY(step < 1.0);  // the rounding defect only shows below one degree per press

    // 50 presses must land on 50 steps; quantizing every press to whole degrees
    // would report 50 degrees instead
    for (int i = 0; i < 50; ++i)
        press(plot, Qt::Key_Right);

    QCOMPARE(plot.zRotation(), 50.0 * step);
}

void TestRotation::keyboardWrapsIntoUnitRange()
{
    TestPlot plot;
    plot.resize(PLOT_WIDTH, PLOT_HEIGHT);

    const double step = keyboardZStep(plot);
    plot.setRotation(0.0, 0.0, 360.0 - step / 2.0);

    press(plot, Qt::Key_Right);
    QCOMPARE(plot.zRotation(), step / 2.0);

    // ... and the opposite direction wraps from 0 down to 360 - step
    plot.setRotation(0.0, 0.0, 0.0);
    press(plot, Qt::Key_Left);
    QCOMPARE(plot.zRotation(), 360.0 - step);
}

QTEST_MAIN(TestRotation)
#include "test_rotation.moc"
