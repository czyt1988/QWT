/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "test_wheel_scrollarea.h"
#include <QApplication>
#include <QDateTime>
#include <QEventLoop>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTimer>
#include <QPolygonF>
#include <QSpontaneKeyEvent>
#include <qwt_plot_canvas.h>
#include <qwt_plot_scale_event_dispatcher.h>

/**
 * \if ENGLISH
 * @brief Helper: send a wheel event to the widget under a global position.
 *        The event is marked spontaneous so QApplication::notify runs the
 *        parent-propagation loop used for real system wheel events;
 *        otherwise the event would be delivered to the receiver only once.
 * \endif
 * \if CHINESE
 * @brief 辅助函数：向指定全局位置发送滚轮事件。
 *        事件标记为 spontaneous，使 QApplication::notify 走真实系统滚轮事件的
 *        父窗口冒泡循环；否则事件只会投递给接收者一次，不向上传播。
 * \endif
 */
void TestWheelScrollArea::sendWheel(QWidget* receiver, const QPoint& globalPos, int delta)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QWheelEvent e(receiver->mapFromGlobal(globalPos), QPointF(globalPos),
                  QPoint(0, 0), QPoint(0, delta), Qt::NoButton, Qt::NoModifier,
                  Qt::NoScrollPhase, false);
#else
    QWheelEvent e(QPointF(receiver->mapFromGlobal(globalPos)), QPointF(globalPos),
                  QPoint(0, 0), QPoint(0, delta), Qt::NoButton, Qt::NoModifier,
                  Qt::NoScrollPhase, false, Qt::MouseEventNotSynthesized);
#endif
    e.setTimestamp(QDateTime::currentMSecsSinceEpoch());
    // 标记 spontaneous 后经 QApplication 实例的 notify 投递，使 Wheel 事件走
    // 真实系统滚轮事件的父窗口冒泡分支（sendEvent/sendSpontaneousEvent 均无法保留该标志）
    QSpontaneKeyEvent::setSpontaneous(&e);
    QApplication::instance()->notify(receiver, &e);
}

/**
 * \if ENGLISH
 * @brief Helper: send a synthetic left click (press+release) to select an axis
 * \endif
 * \if CHINESE
 * @brief 辅助函数：发送合成左键点击（按下+释放）以选中坐标轴
 * \endif
 */
void TestWheelScrollArea::sendClick(QWidget* receiver, const QPoint& globalPos)
{
    const QPoint local = receiver->mapFromGlobal(globalPos);
    QMouseEvent press(QEvent::MouseButtonPress, local, globalPos,
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(receiver, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, local, globalPos,
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(receiver, &release);
}

/**
 * \if ENGLISH
 * @brief Build a QwtPlot (600x400) inside a QScrollArea with a 300x300 viewport
 *        so both scrollbars are visible, exactly like DAFigureWidget does
 * \endif
 * \if CHINESE
 * @brief 构建场景：600x400 的 QwtPlot 放入 300x300 视口的 QScrollArea，
 *        与 DAFigureWidget 的用法一致，此时两个滚动条均可见
 * \endif
 */
void TestWheelScrollArea::buildScenario(QScrollArea*& scrollArea, QwtPlot*& plot, QWidget*& container)
{
    container = new QWidget();
    container->resize(320, 320);

    scrollArea = new QScrollArea(container);
    scrollArea->setGeometry(0, 0, 300, 300);
    scrollArea->setWidgetResizable(false);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setAlignment(Qt::AlignCenter);

    plot = new QwtPlot();
    plot->resize(600, 400);
    scrollArea->setWidget(plot);

    // 一条简单曲线，确保坐标轴刻度有效
    QwtPlotCurve* curve = new QwtPlotCurve("test");
    QPolygonF samples;
    for (int i = 0; i <= 10; ++i) {
        samples << QPointF(i, i * 1.0);
    }
    curve->setSamples(samples);
    curve->attach(plot);
    plot->replot();

    container->show();
    // 等待布局与 paint 完成（scale cache 需要 rect 有效）
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();
    QApplication::processEvents();

    // 滚动到中间，保证滚动条有滚动空间
    scrollArea->horizontalScrollBar()->setValue(0);
    scrollArea->verticalScrollBar()->setValue(0);
}

QPoint TestWheelScrollArea::pointOnBottomScale(QwtPlot* plot, QWidget* container)
{
    Q_UNUSED(container)
    QwtScaleWidget* bottomScale = plot->axisWidget(QwtAxis::XBottom);
    // 取轴刻度区域中心（略偏下方以命中刻度数字区）
    QPoint local = bottomScale->rect().center() + QPoint(0, bottomScale->rect().height() / 4);
    return bottomScale->mapToGlobal(local);
}

QPoint TestWheelScrollArea::pointOnCanvasCenter(QwtPlot* plot, QWidget* container)
{
    Q_UNUSED(container)
    QWidget* canvas = plot->canvas();
    return canvas->mapToGlobal(canvas->rect().center());
}

/**
 * \if ENGLISH
 * @brief Select the bottom axis by left click, then wheel on the axis:
 *        the scroll area must not scroll
 * \endif
 * \if CHINESE
 * @brief 左键点击选中底轴后，在轴上滚动滚轮：QScrollArea 不应滚动
 * \endif
 */
void TestWheelScrollArea::testAxisWheelZoomDoesNotScroll()
{
    QScrollArea* scrollArea = nullptr;
    QwtPlot* plot = nullptr;
    QWidget* container = nullptr;
    buildScenario(scrollArea, plot, container);

    QVERIFY2(scrollArea->verticalScrollBar()->maximum() > 0, "vertical scrollbar should be scrollable");
    QVERIFY2(scrollArea->horizontalScrollBar()->maximum() > 0, "horizontal scrollbar should be scrollable");

    // 左键点击选中底轴
    QPoint scalePos = pointOnBottomScale(plot, container);
    QwtScaleWidget* scaleWidget = plot->axisWidget(QwtAxis::XBottom);
    sendClick(scaleWidget, scalePos);
    QApplication::processEvents();
    QVERIFY2(plot->axisWidget(QwtAxis::XBottom)->isSelected(), "bottom axis should be selected after click");

    // 在轴上滚动滚轮（只用 -120：这是会使未消费事件滚离最小值的方向，
    // 避免 +120/-120 连续滚动相互抵消导致断言平凡通过）
    const int vBefore = scrollArea->verticalScrollBar()->value();
    const int hBefore = scrollArea->horizontalScrollBar()->value();

    sendWheel(scaleWidget, scalePos, -120);
    QApplication::processEvents();

    QCOMPARE(scrollArea->verticalScrollBar()->value(), vBefore);
    QCOMPARE(scrollArea->horizontalScrollBar()->value(), hBefore);

    delete container;
}

/**
 * \if ENGLISH
 * @brief With a QwtPlotMagnifier installed on the canvas, wheel over the canvas
 *        must zoom and must not scroll the QScrollArea
 * \endif
 * \if CHINESE
 * @brief 画布上安装 QwtPlotMagnifier 后，画布上的滚轮应触发缩放且
 *        QScrollArea 不应滚动
 * \endif
 */
void TestWheelScrollArea::testCanvasWheelZoomDoesNotScroll()
{
    QScrollArea* scrollArea = nullptr;
    QwtPlot* plot = nullptr;
    QWidget* container = nullptr;
    buildScenario(scrollArea, plot, container);

    // 注意：QwtPlotMagnifier 的 QObject parent 是 canvas，必须堆分配
    // （canvas 析构时会 delete 子对象，栈对象会被二次销毁而崩溃）
    QwtPlotMagnifier* magnifier = new QwtPlotMagnifier(plot->canvas());
    magnifier->setEnabled(true);

    QWidget* canvas = plot->canvas();
    QPoint canvasPos = pointOnCanvasCenter(plot, container);

    const double xMinBefore = plot->axisScaleDiv(QwtAxis::XBottom).lowerBound();
    const double xMaxBefore = plot->axisScaleDiv(QwtAxis::XBottom).upperBound();

    const int vBefore = scrollArea->verticalScrollBar()->value();
    const int hBefore = scrollArea->horizontalScrollBar()->value();

    // 向下滚（-120）：若事件未被消费，滚动条一定滚动（value 从最小值增大）
    sendWheel(canvas, canvasPos, -120);
    QApplication::processEvents();

    // 画布滚轮缩放应生效（x 范围变化）
    const double xMinAfter = plot->axisScaleDiv(QwtAxis::XBottom).lowerBound();
    const double xMaxAfter = plot->axisScaleDiv(QwtAxis::XBottom).upperBound();
    QVERIFY2(!qFuzzyCompare(xMinBefore, xMinAfter) || !qFuzzyCompare(xMaxBefore, xMaxAfter),
             "canvas wheel zoom should change the axis scale");

    // 滚动条不应滚动
    QCOMPARE(scrollArea->verticalScrollBar()->value(), vBefore);
    QCOMPARE(scrollArea->horizontalScrollBar()->value(), hBefore);

    delete container;
}

/**
 * \if ENGLISH
 * @brief Wheel over the canvas WITHOUT any magnifier and without a selected axis
 *        should leave the event unconsumed, so the scroll area can scroll
 * \endif
 * \if CHINESE
 * @brief 未安装 magnifier 且未选中轴时，画布上的滚轮事件不应被消费，
 *        QScrollArea 应能正常滚动
 * \endif
 */
void TestWheelScrollArea::testWheelWithoutZoomStillScrolls()
{
    QScrollArea* scrollArea = nullptr;
    QwtPlot* plot = nullptr;
    QWidget* container = nullptr;
    buildScenario(scrollArea, plot, container);

    QWidget* canvas = plot->canvas();
    QPoint canvasPos = pointOnCanvasCenter(plot, container);

    const int vBefore = scrollArea->verticalScrollBar()->value();
    sendWheel(canvas, canvasPos, -120);
    QApplication::processEvents();

    QVERIFY2(scrollArea->verticalScrollBar()->value() != vBefore,
             "scroll area should scroll when wheel is not consumed by the plot");

    delete container;
}

/**
 * \if ENGLISH
 * @brief Regression: wheel on the selected axis must actually zoom the axis
 * \endif
 * \if CHINESE
 * @brief 回归验证：选中轴上滚轮必须真正缩放该轴
 * \endif
 */
void TestWheelScrollArea::testAxisWheelZoomActuallyZooms()
{
    QScrollArea* scrollArea = nullptr;
    QwtPlot* plot = nullptr;
    QWidget* container = nullptr;
    buildScenario(scrollArea, plot, container);

    QPoint scalePos = pointOnBottomScale(plot, container);
    QwtScaleWidget* scaleWidget = plot->axisWidget(QwtAxis::XBottom);
    sendClick(scaleWidget, scalePos);
    QApplication::processEvents();
    QVERIFY2(scaleWidget->isSelected(), "axis should be selected");

    const double xMinBefore = plot->axisScaleDiv(QwtAxis::XBottom).lowerBound();
    const double xMaxBefore = plot->axisScaleDiv(QwtAxis::XBottom).upperBound();
    const double rangeBefore = xMaxBefore - xMinBefore;

    sendWheel(scaleWidget, scalePos, 120);
    QApplication::processEvents();

    const double xMinAfter = plot->axisScaleDiv(QwtAxis::XBottom).lowerBound();
    const double xMaxAfter = plot->axisScaleDiv(QwtAxis::XBottom).upperBound();
    const double rangeAfter = xMaxAfter - xMinAfter;

    QVERIFY2(rangeAfter < rangeBefore, "wheel zoom-in should shrink the axis range");

    delete container;
}

/**
 * \if ENGLISH
 * @brief Mechanism probe: a plain QLabel inside the scroll area with an event
 *        filter that returns true but never accepts the wheel event.
 *        If Qt propagation works as analyzed ( res && eventAccepted ), the
 *        event must still reach the scroll area and scroll it. This test
 *        documents/validates the propagation semantics the fix relies on.
 * \endif
 * \if CHINESE
 * @brief 机制探针：滚动区内放一个 QLabel，其事件过滤器返回 true 但从不 accept
 *        滚轮事件。若 Qt 传播机制如分析所述（res && eventAccepted 双条件），
 *        事件必须仍到达滚动区并使其滚动。本测试验证修复所依赖的传播语义。
 * \endif
 */
void TestWheelScrollArea::testFilterTrueWithoutAcceptPropagates()
{
    QWidget container;
    container.resize(320, 320);
    QScrollArea* scrollArea = new QScrollArea(&container);
    scrollArea->setGeometry(0, 0, 300, 300);
    scrollArea->setWidgetResizable(false);
    QWidget* inner = new QWidget();
    inner->resize(600, 400);
    scrollArea->setWidget(inner);
    container.show();
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();
    QApplication::processEvents();

    // 过滤器：拦截（return true）但不 accept
    struct TrueNoAcceptFilter : QObject {
        bool eventFilter(QObject*, QEvent* e) override
        {
            if (e->type() == QEvent::Wheel)
                return true;  // 不调用 e->accept()
            return false;
        }
    } filter;
    inner->installEventFilter(&filter);

    const int vBefore = scrollArea->verticalScrollBar()->value();
    sendWheel(inner, inner->mapToGlobal(inner->rect().center()), -120);
    QApplication::processEvents();

    QVERIFY2(scrollArea->verticalScrollBar()->value() != vBefore,
             "filter returning true without accept must let the wheel event propagate");
}

QTEST_MAIN(TestWheelScrollArea)
#include "test_wheel_scrollarea.moc"
