/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef TEST_WHEEL_SCROLLAREA_H
#define TEST_WHEEL_SCROLLAREA_H

#include <QObject>
#include <QTest>
#include <QScrollArea>
#include <QWheelEvent>
#include <QwtPlot>
#include <QwtPlotCurve>
#include <QwtPlotMagnifier>
#include <QwtScaleWidget>

/**
 * \if ENGLISH
 * @brief Tests that wheel zoom on plot/canvas is not stolen by an enclosing QScrollArea
 * @details Reproduces the scenario: a QwtPlot inside a QScrollArea where the widget
 *          is larger than the viewport (scrollbars visible). After selecting an axis
 *          (left click) or while hovering the canvas, wheel events must be consumed
 *          by the plot (axis zoom / canvas zoom) and must NOT scroll the QScrollArea.
 * \endif
 *
 * \if CHINESE
 * @brief 测试绘图/画布上的滚轮缩放不会被外层 QScrollArea 截获
 * @details 复现场景：QwtPlot 放在 QScrollArea 中且尺寸大于视口（出现滚动条）。
 *          左键点击选中坐标轴后、或光标位于画布上时，滚轮事件必须被绘图消费
 *          （轴缩放/画布缩放），且 QScrollArea 不应发生滚动。
 * \endif
 */
class TestWheelScrollArea : public QObject
{
    Q_OBJECT

private slots:
    /// Mechanism probe: filter returning true WITHOUT accept must propagate
    /// the wheel event to the scroll area (confirms Qt propagation semantics)
    void testFilterTrueWithoutAcceptPropagates();
    /// Axis wheel zoom must not scroll the QScrollArea
    void testAxisWheelZoomDoesNotScroll();

    /// Canvas wheel zoom (QwtPlotMagnifier) must not scroll the QScrollArea
    void testCanvasWheelZoomDoesNotScroll();

    /// Wheel over the figure/plot background (no axis selected, no magnifier)
    /// should still be available for the scroll area to scroll
    void testWheelWithoutZoomStillScrolls();

    /// Regression: axis zoom actually happens when wheel is turned on selected axis
    void testAxisWheelZoomActuallyZooms();

private:
    /// Send a real wheel event to the widget under a global position
    void sendWheel(QWidget* receiver, const QPoint& globalPos, int delta);

    /// Send a real left-button press/release to select an axis
    void sendClick(QWidget* receiver, const QPoint& globalPos);

    /// Build a plot inside a scroll area whose content is bigger than the viewport
    void buildScenario(QScrollArea*& scrollArea, QwtPlot*& plot, QWidget*& container);

    /// Find a point on the bottom scale of the plot in global coordinates
    QPoint pointOnBottomScale(QwtPlot* plot, QWidget* container);

    /// Find a point in the middle of the canvas in global coordinates
    QPoint pointOnCanvasCenter(QwtPlot* plot, QWidget* container);
};

#endif  // TEST_WHEEL_SCROLLAREA_H
