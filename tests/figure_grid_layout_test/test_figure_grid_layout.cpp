/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "test_figure_grid_layout.h"
// Qt
#include <QApplication>
#include <QEvent>
#include <QLayout>
#include <QMetaType>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>
// Qwt
#include "qwt_figure.h"
#include "qwt_figure_grid.h"
#include "qwt_figure_layout.h"
#include "qwt_plot.h"

/**
 * @brief Apply a fixed pixel geometry to the figure and run its layout
 * @param[in,out] figure Figure to lay out
 * @param width Pixel width
 * @param height Pixel height
 */
void TestFigureGridLayout::layoutFigure(QwtFigure* figure, int width, int height)
{
    QVERIFY(figure);
    figure->resize(width, height);
    QLayout* lay = figure->layout();
    QVERIFY(lay);
    lay->setGeometry(QRect(0, 0, width, height));
}

/**
 * @brief Enabling the retained grid creates equal tracks and rejects invalid counts
 */
void TestFigureGridLayout::testEnableGrid()
{
    QwtFigure figure;
    QVERIFY(!figure.isGridEnabled());
    QVERIFY(figure.grid() == nullptr);

    QVERIFY(!figure.enableGrid(0, 2));
    QVERIFY(!figure.enableGrid(2, -1));
    QVERIFY(!figure.isGridEnabled());

    QVERIFY(figure.enableGrid(2, 3));
    QVERIFY(figure.isGridEnabled());
    QwtFigureGrid* grid = figure.grid();
    QVERIFY(grid);
    QCOMPARE(grid->rowCount(), 2);
    QCOMPARE(grid->columnCount(), 3);

    const QVector< qreal > rows = grid->rowSizes();
    QCOMPARE(rows.size(), 2);
    QVERIFY(qFuzzyCompare(rows.at(0), 0.5));
    QVERIFY(qFuzzyCompare(rows.at(1), 0.5));

    const QVector< qreal > columns = grid->columnSizes();
    QCOMPARE(columns.size(), 3);
    for (int i = 0; i < columns.size(); ++i) {
        QVERIFY(qFuzzyCompare(columns.at(i), 1.0 / 3.0));
    }

    // Re-enabling resets the tracks and drops the previous member registrations
    QwtPlot* plot = new QwtPlot;
    QVERIFY(figure.enableGrid(2, 2));
    figure.addAxes(plot, 0, 0);
    QCOMPARE(grid->items().size(), 1);
    QVERIFY(figure.enableGrid(3, 3));
    QCOMPARE(grid->rowCount(), 3);
    QCOMPARE(grid->items().size(), 0);
    figure.removeWidget(plot);
    delete plot;
}

/**
 * @brief Grid members get the rectangles derived from the tracks
 */
void TestFigureGridLayout::testGridGeometry()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(2, 2));

    QwtPlot* topLeft = new QwtPlot;
    QwtPlot* topRight = new QwtPlot;
    QwtPlot* bottomLeft = new QwtPlot;
    QwtPlot* bottomRight = new QwtPlot;
    QwtPlot* spanning = new QwtPlot;

    figure.addAxes(topLeft, 0, 0);
    figure.addAxes(topRight, 0, 1);
    figure.addAxes(bottomLeft, 1, 0);
    figure.addAxes(bottomRight, 1, 1);
    layoutFigure(&figure, 400, 300);

    QCOMPARE(topLeft->geometry(), QRect(0, 0, 200, 150));
    QCOMPARE(topRight->geometry(), QRect(200, 0, 200, 150));
    QCOMPARE(bottomLeft->geometry(), QRect(0, 150, 200, 150));
    QCOMPARE(bottomRight->geometry(), QRect(200, 150, 200, 150));

    // Spanning both columns
    figure.addAxes(spanning, 0, 0, 1, 2);
    layoutFigure(&figure, 400, 300);
    QCOMPARE(spanning->geometry(), QRect(0, 0, 400, 150));

    // The normalized rectangle reported for a grid member follows the tracks
    const QRectF normRect = figure.axesNormRect(bottomRight);
    QVERIFY(qFuzzyCompare(normRect.left(), 0.5));
    QVERIFY(qFuzzyCompare(normRect.top(), 0.5));
    QVERIFY(qFuzzyCompare(normRect.width(), 0.5));
    QVERIFY(qFuzzyCompare(normRect.height(), 0.5));

    QList< QwtPlot* > plots;
    plots << topLeft << topRight << bottomLeft << bottomRight << spanning;
    for (QwtPlot* plot : plots) {
        figure.removeWidget(plot);
        delete plot;
    }
}

/**
 * @brief Resizing the figure keeps the track proportions
 */
void TestFigureGridLayout::testResizeKeepsRatios()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(2, 2));
    QwtPlot* plot = new QwtPlot;
    figure.addAxes(plot, 1, 1);

    layoutFigure(&figure, 400, 300);
    QCOMPARE(plot->geometry(), QRect(200, 150, 200, 150));

    layoutFigure(&figure, 800, 600);
    QCOMPARE(plot->geometry(), QRect(400, 300, 400, 300));

    layoutFigure(&figure, 100, 50);
    QCOMPARE(plot->geometry(), QRect(50, 25, 50, 25));

    figure.removeWidget(plot);
    delete plot;
}

/**
 * @brief Neighbouring cells share their rounded edges, so no 1 pixel seam appears
 *
 * The pixel extents are derived from the rounded accumulated edges instead of
 * being rounded independently: the right edge of a cell is the left edge of the
 * next one and the extents add up to the available width/height.
 */
void TestFigureGridLayout::testCumulativeEdgeRounding()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(1, 3));

    QwtPlot* first  = new QwtPlot;
    QwtPlot* second = new QwtPlot;
    QwtPlot* third  = new QwtPlot;
    figure.addAxes(first, 0, 0);
    figure.addAxes(second, 0, 1);
    figure.addAxes(third, 0, 2);

    // Equal 1/3 tracks over a width that is not divisible by 3
    layoutFigure(&figure, 400, 300);
    QCOMPARE(first->geometry().left(), 0);
    QCOMPARE(first->geometry().right() + 1, second->geometry().left());
    QCOMPARE(second->geometry().right() + 1, third->geometry().left());
    QCOMPARE(third->geometry().right() + 1, 400);
    QCOMPARE(first->geometry().width() + second->geometry().width() + third->geometry().width(), 400);

    // Custom non-integral ratios: the edges are still shared and the widths add up
    figure.grid()->setColumnSizes({ 0.2, 0.3, 0.5 });
    layoutFigure(&figure, 333, 300);
    QCOMPARE(first->geometry().left(), 0);
    QCOMPARE(first->geometry().right() + 1, second->geometry().left());
    QCOMPARE(second->geometry().right() + 1, third->geometry().left());
    QCOMPARE(third->geometry().right() + 1, 333);
    QCOMPARE(first->geometry().width() + second->geometry().width() + third->geometry().width(), 333);

    // The same cumulative rule applies to the Y axis
    QwtFigureLayout* lay = qobject_cast< QwtFigureLayout* >(figure.layout());
    QVERIFY(lay);
    const QRect parent(0, 0, 400, 301);
    const QRect topCell    = lay->calcActualRect(parent, QRectF(0.0, 0.0, 1.0, 1.0 / 3.0));
    const QRect middleCell = lay->calcActualRect(parent, QRectF(0.0, 1.0 / 3.0, 1.0, 1.0 / 3.0));
    const QRect bottomCell = lay->calcActualRect(parent, QRectF(0.0, 2.0 / 3.0, 1.0, 1.0 / 3.0));
    QCOMPARE(topCell.top(), 0);
    QCOMPARE(topCell.bottom() + 1, middleCell.top());
    QCOMPARE(middleCell.bottom() + 1, bottomCell.top());
    QCOMPARE(bottomCell.bottom() + 1, 301);
    // The resulting rects stay inside the parent rect
    QCOMPARE(topCell, QRect(0, 0, 400, 100));
    QCOMPARE(middleCell, QRect(0, 100, 400, 101));
    QCOMPARE(bottomCell, QRect(0, 201, 400, 100));

    figure.removeWidget(first);
    figure.removeWidget(second);
    figure.removeWidget(third);
    delete first;
    delete second;
    delete third;
}

/**
 * @brief Retained grid members and free normalized members can coexist
 */
void TestFigureGridLayout::testGridAndFreeMembers()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(2, 2));

    QwtPlot* gridPlot = new QwtPlot;
    QwtPlot* freePlot = new QwtPlot;
    figure.addAxes(gridPlot, 0, 0);
    figure.addAxes(freePlot, QRectF(0.5, 0.0, 0.5, 1.0));

    layoutFigure(&figure, 400, 300);
    QCOMPARE(gridPlot->geometry(), QRect(0, 0, 200, 150));
    QCOMPARE(freePlot->geometry(), QRect(200, 0, 200, 300));

    // A free member keeps its normalized rectangle when the track sizes change
    figure.grid()->setColumnSizes({ 1.0, 3.0 });
    layoutFigure(&figure, 400, 300);
    QCOMPARE(gridPlot->geometry(), QRect(0, 0, 100, 150));
    QCOMPARE(freePlot->geometry(), QRect(200, 0, 200, 300));

    figure.removeWidget(gridPlot);
    figure.removeWidget(freePlot);
    delete gridPlot;
    delete freePlot;
}

/**
 * @brief Changing the track sizes immediately relayouts the members
 */
void TestFigureGridLayout::testTrackSizesRelayout()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(2, 2));
    QwtPlot* plot = new QwtPlot;
    figure.addAxes(plot, 0, 1);
    layoutFigure(&figure, 400, 300);
    QCOMPARE(plot->geometry(), QRect(200, 0, 200, 150));

    figure.grid()->setColumnSizes({ 1.0, 3.0 });
    QCOMPARE(plot->geometry(), QRect(100, 0, 300, 150));

    figure.grid()->setRowSizes({ 3.0, 1.0 });
    QCOMPARE(plot->geometry(), QRect(100, 0, 300, 225));

    figure.grid()->setSpacing(0.05, 0.10);
    const QRectF spacedRect = figure.axesNormRect(plot);
    QVERIFY(qAbs(spacedRect.left() - 0.30) < 1e-9);
    QVERIFY(qAbs(spacedRect.width() - 0.70) < 1e-9);

    figure.removeWidget(plot);
    delete plot;
}

/**
 * @brief removeWidget drops the layout item and the grid registration
 */
void TestFigureGridLayout::testRemoveWidget()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(2, 2));

    QwtPlot* plot = new QwtPlot;
    QWidget* widget = new QWidget;
    figure.addAxes(plot, 0, 0);
    QVERIFY(figure.addGridItem(widget, 1, 1));

    QCOMPARE(figure.layout()->count(), 2);
    QCOMPARE(figure.grid()->items().size(), 2);

    figure.removeWidget(plot);
    QVERIFY(!figure.hasAxes(plot));
    QVERIFY(plot->parentWidget() == nullptr);
    QCOMPARE(figure.layout()->count(), 1);
    QCOMPARE(figure.grid()->items().size(), 1);

    figure.removeWidget(widget);
    QVERIFY(widget->parentWidget() == nullptr);
    QCOMPARE(figure.layout()->count(), 0);
    QCOMPARE(figure.grid()->items().size(), 0);

    delete plot;
    delete widget;
}

/**
 * @brief A press on a grid member - delivered to its canvas like a real click - makes it the current axes
 */
void TestFigureGridLayout::testClickActivatesCurrentAxes()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(2, 2));

    QwtPlot* first = new QwtPlot;
    QwtPlot* second = new QwtPlot;
    figure.addAxes(first, 0, 0);
    figure.addAxes(second, 0, 1);
    figure.setCurrentAxes(second);

    // QSignalSpy needs the metatype to store the signal argument
    qRegisterMetaType< QwtPlot* >("QwtPlot*");
    QSignalSpy currentSpy(&figure, &QwtFigure::currentAxesChanged);
    QVERIFY(currentSpy.isValid());

    // Real presses are delivered to the canvas of the plot, not to the plot itself
    QWidget* firstCanvas  = first->canvas();
    QWidget* secondCanvas = second->canvas();
    QVERIFY(firstCanvas != nullptr);
    QVERIFY(secondCanvas != nullptr);

    QMouseEvent press(QEvent::MouseButtonPress,
                      QPointF(10, 10),
                      QPointF(10, 10),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);
    QApplication::sendEvent(firstCanvas, &press);
    QCOMPARE(figure.currentAxes(), first);
    QCOMPARE(currentSpy.count(), 1);

    // Pressing the already active plot does not emit again
    QApplication::sendEvent(firstCanvas, &press);
    QCOMPARE(currentSpy.count(), 1);

    QApplication::sendEvent(secondCanvas, &press);
    QCOMPARE(figure.currentAxes(), second);
    QCOMPARE(currentSpy.count(), 2);

    // A press delivered to the plot itself still activates it
    QApplication::sendEvent(first, &press);
    QCOMPARE(figure.currentAxes(), first);
    QCOMPARE(currentSpy.count(), 3);

    figure.removeWidget(first);
    figure.removeWidget(second);
    delete first;
    delete second;
}

/**
 * @brief Members added through the legacy addWidget() grid overload activate on a canvas press,
 *        and the activation ends with removeWidget()
 */
void TestFigureGridLayout::testClickActivatesAxesFromLegacyAddWidget()
{
    QwtFigure figure;
    QwtPlot* first  = new QwtPlot;
    QwtPlot* second = new QwtPlot;
    figure.addWidget(first, 2, 1, 0, 0);
    figure.addWidget(second, 2, 1, 1, 0);
    figure.setCurrentAxes(first);

    qRegisterMetaType< QwtPlot* >("QwtPlot*");
    QSignalSpy currentSpy(&figure, &QwtFigure::currentAxesChanged);
    QVERIFY(currentSpy.isValid());

    QWidget* canvas = second->canvas();
    QVERIFY(canvas != nullptr);
    QMouseEvent press(QEvent::MouseButtonPress,
                      QPointF(10, 10),
                      QPointF(10, 10),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);
    QApplication::sendEvent(canvas, &press);
    QCOMPARE(figure.currentAxes(), second);
    QCOMPARE(currentSpy.count(), 1);

    // A removed member no longer reacts to presses on its canvas
    figure.removeWidget(second);
    QCOMPARE(currentSpy.count(), 2);  // removing the current axes activates the remaining one
    QApplication::sendEvent(canvas, &press);
    QCOMPARE(figure.currentAxes(), first);
    QCOMPARE(currentSpy.count(), 2);

    figure.removeWidget(first);
    delete first;
    delete second;
}

/**
 * @brief Grid size signals are forwarded by the figure
 */
void TestFigureGridLayout::testGridSizeSignals()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(2, 2));

    QSignalSpy changedSpy(&figure, &QwtFigure::gridSizesChanged);
    QSignalSpy finishedSpy(&figure, &QwtFigure::gridSizesChangeFinished);
    QVERIFY(changedSpy.isValid());
    QVERIFY(finishedSpy.isValid());

    figure.grid()->setRowSizes({ 0.5, 0.5 });  // unchanged
    QCOMPARE(changedSpy.count(), 0);
    figure.grid()->setRowSizes({ 1.0, 3.0 });
    QCOMPARE(changedSpy.count(), 1);

    figure.grid()->finishChange();
    QCOMPARE(finishedSpy.count(), 1);
}

/**
 * @brief The old addGridAxes() conversion is unchanged and does not enable the retained grid
 */
void TestFigureGridLayout::testOldAddGridAxesUnchanged()
{
    QwtFigure figure;
    QwtPlot* plot = new QwtPlot;
    figure.addGridAxes(plot, 2, 2, 1, 0, 1, 2, 0.0, 0.0);

    QVERIFY(figure.hasAxes(plot));
    QVERIFY(!figure.isGridEnabled());
    QVERIFY(figure.grid() == nullptr);

    const QRectF normRect = figure.axesNormRect(plot);
    QVERIFY(qFuzzyCompare(normRect.left(), 0.0));
    QVERIFY(qFuzzyCompare(normRect.top(), 0.5));
    QVERIFY(qFuzzyCompare(normRect.width(), 1.0));
    QVERIFY(qFuzzyCompare(normRect.height(), 0.5));

    layoutFigure(&figure, 400, 300);
    QCOMPARE(plot->geometry(), QRect(0, 150, 400, 150));

    figure.removeWidget(plot);
    delete plot;
}

/**
 * @brief Leaving the grid keeps the current member geometry
 *
 * The normalized rectangles are frozen from the geometry the layout produced, so the
 * sub plots neither move nor shrink when the tracks disappear. Afterwards they behave
 * like any free member: a resize of the figure scales them by their frozen ratio.
 */
void TestFigureGridLayout::testDisableGridKeepsGeometry()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(2, 2));

    QwtPlot* spanning    = new QwtPlot;
    QwtPlot* bottomLeft  = new QwtPlot;
    QwtPlot* bottomRight = new QwtPlot;
    figure.addAxes(spanning, 0, 0, 1, 2);
    figure.addAxes(bottomLeft, 1, 0);
    figure.addAxes(bottomRight, 1, 1);

    // Non-integral ratios: the frozen geometry has to come from the tracks actually
    // used by the layout, not from equal cells.
    figure.grid()->setColumnSizes({ 0.3, 0.7 });
    layoutFigure(&figure, 400, 300);

    const QRect spanningGeo    = spanning->geometry();
    const QRect bottomLeftGeo  = bottomLeft->geometry();
    const QRect bottomRightGeo = bottomRight->geometry();
    QCOMPARE(spanningGeo, QRect(0, 0, 400, 150));
    QCOMPARE(bottomLeftGeo, QRect(0, 150, 120, 150));
    QCOMPARE(bottomRightGeo, QRect(120, 150, 280, 150));

    QVERIFY(figure.disableGrid());
    QVERIFY(!figure.isGridEnabled());
    QVERIFY(figure.grid() != nullptr);  // the model object survives, it is just disabled
    QVERIFY(!figure.grid()->isEnabled());

    // No jump right after the conversion
    layoutFigure(&figure, 400, 300);
    QCOMPARE(spanning->geometry(), spanningGeo);
    QCOMPARE(bottomLeft->geometry(), bottomLeftGeo);
    QCOMPARE(bottomRight->geometry(), bottomRightGeo);

    // Free members: the frozen ratios are what the figure scales now
    layoutFigure(&figure, 800, 600);
    QCOMPARE(spanning->geometry(), QRect(0, 0, 800, 300));
    QCOMPARE(bottomLeft->geometry(), QRect(0, 300, 240, 300));
    QCOMPARE(bottomRight->geometry(), QRect(240, 300, 560, 300));

    const QRectF leftNorm = figure.axesNormRect(bottomLeft);
    QVERIFY(qAbs(leftNorm.left() - 0.0) < 1e-6);
    QVERIFY(qAbs(leftNorm.top() - 0.5) < 1e-6);
    QVERIFY(qAbs(leftNorm.width() - 0.3) < 1e-6);
    QVERIFY(qAbs(leftNorm.height() - 0.5) < 1e-6);

    figure.removeWidget(spanning);
    figure.removeWidget(bottomLeft);
    figure.removeWidget(bottomRight);
    delete spanning;
    delete bottomLeft;
    delete bottomRight;
}

/**
 * @brief A figure mixing grid and free members ends up with free members only
 */
void TestFigureGridLayout::testDisableGridMixedMembers()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(2, 2));

    QwtPlot* gridPlot = new QwtPlot;
    QwtPlot* freePlot = new QwtPlot;
    figure.addAxes(gridPlot, 0, 0);
    figure.addAxes(freePlot, QRectF(0.5, 0.0, 0.5, 1.0));

    figure.setGridEditable(true);
    QVERIFY(figure.isGridEditable());

    layoutFigure(&figure, 400, 300);
    const QRect gridGeo = gridPlot->geometry();
    const QRect freeGeo = freePlot->geometry();
    QCOMPARE(gridGeo, QRect(0, 0, 200, 150));
    QCOMPARE(freeGeo, QRect(200, 0, 200, 300));

    QVERIFY(figure.disableGrid());
    // The divider interaction has to be stopped as well: the splitter grabs the mouse
    // and filters the member events.
    QVERIFY(!figure.isGridEditable());

    layoutFigure(&figure, 400, 300);
    QCOMPARE(gridPlot->geometry(), gridGeo);
    QCOMPARE(freePlot->geometry(), freeGeo);

    // Both members are free now: the track ratios no longer drive any geometry
    figure.grid()->setColumnSizes({ 1.0, 3.0 });
    layoutFigure(&figure, 400, 300);
    QCOMPARE(gridPlot->geometry(), gridGeo);
    QCOMPARE(freePlot->geometry(), freeGeo);

    // A former grid member can be moved through its normalized rectangle again
    figure.setWidgetNormPos(gridPlot, QRectF(0.5, 0.0, 0.5, 1.0));
    layoutFigure(&figure, 400, 300);
    QCOMPARE(gridPlot->geometry(), freeGeo);

    figure.removeWidget(gridPlot);
    figure.removeWidget(freePlot);
    delete gridPlot;
    delete freePlot;
}

/**
 * @brief Leaving the grid twice is a no-op, and grid members are rejected afterwards
 */
void TestFigureGridLayout::testDisableGridIdempotent()
{
    QwtFigure figure;
    QVERIFY(!figure.disableGrid());  // nothing to leave

    QVERIFY(figure.enableGrid(1, 2));
    QwtPlot* left  = new QwtPlot;
    QwtPlot* right = new QwtPlot;
    figure.addAxes(left, 0, 0);
    figure.addAxes(right, 0, 1);
    layoutFigure(&figure, 200, 100);

    const QRect leftGeo  = left->geometry();
    const QRect rightGeo = right->geometry();
    QCOMPARE(leftGeo, QRect(0, 0, 100, 100));
    QCOMPARE(rightGeo, QRect(100, 0, 100, 100));

    QVERIFY(figure.disableGrid());
    QVERIFY(!figure.disableGrid());  // repeated call reports nothing to do

    layoutFigure(&figure, 200, 100);
    QCOMPARE(left->geometry(), leftGeo);
    QCOMPARE(right->geometry(), rightGeo);

    // The grid overloads reject new members instead of silently creating bogus cells
    QwtPlot* extra = new QwtPlot;
    figure.addAxes(extra, 0, 0);
    QVERIFY(!figure.hasAxes(extra));
    QCOMPARE(figure.layout()->count(), 2);
    delete extra;

    // Enabling the grid again starts from equal tracks and no registrations
    QVERIFY(figure.enableGrid(2, 2));
    QVERIFY(figure.grid()->isEnabled());
    QCOMPARE(figure.grid()->rowSizes().size(), 2);
    QCOMPARE(figure.grid()->items().size(), 0);

    figure.removeWidget(left);
    figure.removeWidget(right);
    delete left;
    delete right;
}

QTEST_MAIN(TestFigureGridLayout)
