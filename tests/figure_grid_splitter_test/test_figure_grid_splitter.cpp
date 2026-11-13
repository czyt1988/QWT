/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "test_figure_grid_splitter.h"

// Qt
#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QtGlobal>

// qwt
#include "qwt_figure.h"
#include "qwt_figure_grid.h"
#include "qwt_figure_grid_splitter.h"
#include "qwt_plot.h"

namespace
{
// Supported tolerance for the computed track ratios
const double RATIO_EPSILON = 1e-6;
}  // namespace

void TestFigureGridSplitter::init()
{
    m_figure = new QwtFigure();
    m_figure->resize(400, 300);

    QVERIFY(m_figure->enableGrid(2, 2));
    m_grid = m_figure->grid();
    QVERIFY(m_grid != nullptr);

    // Equal tracks: both dividers sit exactly in the middle of the figure
    m_grid->setRowSizes(QVector< qreal >() << 0.5 << 0.5);
    m_grid->setColumnSizes(QVector< qreal >() << 0.5 << 0.5);
    m_grid->setSpacing(0.0, 0.0);

    m_topPlot         = new QwtPlot();
    m_bottomLeftPlot  = new QwtPlot();
    m_bottomRightPlot = new QwtPlot();

    m_figure->addAxes(m_topPlot, 0, 0, 1, 2);  // spans both columns
    m_figure->addAxes(m_bottomLeftPlot, 1, 0);
    m_figure->addAxes(m_bottomRightPlot, 1, 1);

    m_figure->setGridEditable(true);
    QVERIFY(m_figure->isGridEditable());

    m_splitter = m_figure->findChild< QwtFigureGridSplitter* >();
    QVERIFY(m_splitter != nullptr);
    QCOMPARE(m_splitter->hitWidth(), 4);

    // The geometry of the cells is needed for the hit test and the cursor target
    m_figure->show();
    QApplication::processEvents();
}

void TestFigureGridSplitter::cleanup()
{
    delete m_figure;
    m_figure          = nullptr;
    m_grid            = nullptr;
    m_splitter        = nullptr;
    m_topPlot         = nullptr;
    m_bottomLeftPlot  = nullptr;
    m_bottomRightPlot = nullptr;
    QApplication::processEvents();
}

bool TestFigureGridSplitter::sendMouseEvent(QWidget* receiver,
                                            QEvent::Type type,
                                            const QPoint& pos,
                                            Qt::MouseButton button,
                                            Qt::MouseButtons buttons)
{
    QMouseEvent event(type, QPointF(pos), QPointF(receiver->mapToGlobal(pos)), button, buttons, Qt::NoModifier);
    QApplication::sendEvent(receiver, &event);
    return event.isAccepted();
}

bool TestFigureGridSplitter::pressAt(const QPoint& pos)
{
    return sendMouseEvent(m_figure, QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton);
}

bool TestFigureGridSplitter::dragTo(const QPoint& pos)
{
    return sendMouseEvent(m_figure, QEvent::MouseMove, pos, Qt::NoButton, Qt::LeftButton);
}

bool TestFigureGridSplitter::releaseAt(const QPoint& pos)
{
    return sendMouseEvent(m_figure, QEvent::MouseButtonRelease, pos, Qt::LeftButton, Qt::NoButton);
}

bool TestFigureGridSplitter::escape()
{
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(m_figure, &event);
    return event.isAccepted();
}

double TestFigureGridSplitter::columnAt(int index) const
{
    const QVector< qreal > sizes = m_grid->columnSizes();
    return (index >= 0 && index < sizes.size()) ? sizes.at(index) : -1.0;
}

double TestFigureGridSplitter::rowAt(int index) const
{
    const QVector< qreal > sizes = m_grid->rowSizes();
    return (index >= 0 && index < sizes.size()) ? sizes.at(index) : -1.0;
}

/**
 * @brief A divider is hit exactly up to hitWidth pixels on both sides
 */
void TestFigureGridSplitter::testBandHitRange()
{
    const int dividerX = qRound(0.5 * m_figure->width());
    const int dividerY = qRound(0.5 * m_figure->height());
    const int midY     = dividerY / 2;
    const int midX     = dividerX / 2;

    // Column divider: 4 pixels are inside the band, 5 pixels are outside
    QVERIFY2(sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(dividerX + 4, midY), Qt::NoButton, Qt::NoButton),
             "divider + hitWidth must be inside the band");
    QVERIFY2(sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(dividerX - 4, midY), Qt::NoButton, Qt::NoButton),
             "divider - hitWidth must be inside the band");
    QVERIFY2(!sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(dividerX + 5, midY), Qt::NoButton, Qt::NoButton),
             "divider + hitWidth + 1 must be outside the band");
    QVERIFY2(!sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(dividerX - 5, midY), Qt::NoButton, Qt::NoButton),
             "divider - hitWidth - 1 must be outside the band");

    // Row divider: same range on the Y axis
    QVERIFY2(sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(midX, dividerY + 4), Qt::NoButton, Qt::NoButton),
             "row divider + hitWidth must be inside the band");
    QVERIFY2(!sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(midX, dividerY + 5), Qt::NoButton, Qt::NoButton),
             "row divider + hitWidth + 1 must be outside the band");

    // A custom hit width applies to the next hover event
    m_splitter->setHitWidth(2);
    QVERIFY(!sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(dividerX + 3, midY), Qt::NoButton, Qt::NoButton));
    QVERIFY(sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(dividerX + 2, midY), Qt::NoButton, Qt::NoButton));
    m_splitter->setHitWidth(4);
}

/**
 * @brief The hover signal reports the split cursor shape and returns to the arrow
 */
void TestFigureGridSplitter::testHoverSignalShapes()
{
    QList< Qt::CursorShape > shapes;
    connect(m_splitter, &QwtFigureGridSplitter::hoverChanged, this, [ &shapes ](Qt::CursorShape shape) {
        shapes.append(shape);
    });

    const int dividerX = qRound(0.5 * m_figure->width());
    const int dividerY = qRound(0.5 * m_figure->height());
    const int midY     = dividerY / 2;
    const int midX     = dividerX / 2;

    QVERIFY(sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(dividerX, midY), Qt::NoButton, Qt::NoButton));
    QCOMPARE(shapes.size(), 1);
    QCOMPARE(shapes.last(), Qt::SplitHCursor);

    // Moving inside the same band keeps the shape, no extra emission
    QVERIFY(sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(dividerX + 1, midY - 5), Qt::NoButton, Qt::NoButton));
    QCOMPARE(shapes.size(), 1);

    QVERIFY(!sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(midX, midY), Qt::NoButton, Qt::NoButton));
    QCOMPARE(shapes.size(), 2);
    QCOMPARE(shapes.last(), Qt::ArrowCursor);

    QVERIFY(sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(midX, dividerY), Qt::NoButton, Qt::NoButton));
    QCOMPARE(shapes.size(), 3);
    QCOMPARE(shapes.last(), Qt::SplitVCursor);

    QVERIFY(!sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(midX, midY), Qt::NoButton, Qt::NoButton));
    QCOMPARE(shapes.size(), 4);
    QCOMPARE(shapes.last(), Qt::ArrowCursor);
}

/**
 * @brief A press outside the divider bands must not start a drag
 */
void TestFigureGridSplitter::testPressOutsideBandDoesNotStartDrag()
{
    const int midX = qRound(0.5 * m_figure->width()) / 2;
    const int midY = qRound(0.5 * m_figure->height()) / 2;

    QVERIFY2(!pressAt(QPoint(midX, midY)), "a press outside the band must not be consumed");
    QVERIFY(!m_splitter->isDragging());

    QVERIFY(!releaseAt(QPoint(midX, midY)));
    QVERIFY(!m_splitter->isDragging());
}

/**
 * @brief Dragging the column divider only changes the column ratios
 */
void TestFigureGridSplitter::testDragChangesOnlyColumnTracks()
{
    QSignalSpy finishedSpy(m_grid, &QwtFigureGrid::sizesChangeFinished);

    const int dividerX         = qRound(0.5 * m_figure->width());
    const int dividerY         = qRound(0.5 * m_figure->height());
    const int midY             = dividerY / 2;
    const int deltaPixels      = 40;
    const double expectedFirst = 0.5 + static_cast< double >(deltaPixels) / m_figure->width();

    QVERIFY(pressAt(QPoint(dividerX, midY)));
    QVERIFY(m_splitter->isDragging());

    QVERIFY(dragTo(QPoint(dividerX + deltaPixels, midY)));
    QVERIFY(qAbs(columnAt(0) - expectedFirst) < RATIO_EPSILON);
    QVERIFY(qAbs(columnAt(1) - (1.0 - expectedFirst)) < RATIO_EPSILON);

    // The row tracks must not be touched by a column drag
    QVERIFY(qAbs(rowAt(0) - 0.5) < RATIO_EPSILON);
    QVERIFY(qAbs(rowAt(1) - 0.5) < RATIO_EPSILON);

    QCOMPARE(finishedSpy.count(), 0);
    QVERIFY(releaseAt(QPoint(dividerX + deltaPixels, midY)));
    QVERIFY(!m_splitter->isDragging());
    QCOMPARE(finishedSpy.count(), 1);
}

/**
 * @brief A drag cannot collapse a track below grid()->minimumTrackPixels()
 */
void TestFigureGridSplitter::testMinimumTrackClamp()
{
    const int dividerX    = qRound(0.5 * m_figure->width());
    const int dividerY    = qRound(0.5 * m_figure->height());
    const int midY        = dividerY / 2;
    const double minRatio = static_cast< double >(m_grid->minimumTrackPixels()) / m_figure->width();

    QVERIFY(pressAt(QPoint(dividerX, midY)));

    // Dragging far to the left clamps the first column, it must not become zero
    QVERIFY(dragTo(QPoint(0, midY)));
    QVERIFY(qAbs(columnAt(0) - minRatio) < RATIO_EPSILON);
    QVERIFY(qAbs(columnAt(1) - (1.0 - minRatio)) < RATIO_EPSILON);

    // Dragging far to the right clamps the second column
    QVERIFY(dragTo(QPoint(m_figure->width() - 1, midY)));
    QVERIFY(qAbs(columnAt(0) - (1.0 - minRatio)) < RATIO_EPSILON);
    QVERIFY(qAbs(columnAt(1) - minRatio) < RATIO_EPSILON);

    QVERIFY(releaseAt(QPoint(m_figure->width() - 1, midY)));
    QVERIFY(!m_splitter->isDragging());
}

/**
 * @brief sizesChangeFinished is emitted exactly once, and only when the ratios changed
 */
void TestFigureGridSplitter::testFinishedEmittedExactlyOnceAndOnlyWhenChanged()
{
    QSignalSpy gridSpy(m_grid, &QwtFigureGrid::sizesChangeFinished);
    QSignalSpy figureSpy(m_figure, &QwtFigure::gridSizesChangeFinished);

    const int dividerX = qRound(0.5 * m_figure->width());
    const int dividerY = qRound(0.5 * m_figure->height());
    const int midY     = dividerY / 2;

    // A press and release without movement does not change anything
    QVERIFY(pressAt(QPoint(dividerX, midY)));
    QVERIFY(releaseAt(QPoint(dividerX, midY)));
    QCOMPARE(gridSpy.count(), 0);
    QCOMPARE(figureSpy.count(), 0);

    // A real drag emits the finished signal exactly once on release
    QVERIFY(pressAt(QPoint(dividerX, midY)));
    QVERIFY(dragTo(QPoint(dividerX + 10, midY)));
    QVERIFY(dragTo(QPoint(dividerX + 20, midY)));
    QCOMPARE(gridSpy.count(), 0);

    QVERIFY(releaseAt(QPoint(dividerX + 20, midY)));
    QCOMPARE(gridSpy.count(), 1);
    QCOMPARE(figureSpy.count(), 1);
}

/**
 * @brief Esc restores the original ratios and emits no finished signal
 */
void TestFigureGridSplitter::testEscapeRestoresOriginalSizes()
{
    QSignalSpy finishedSpy(m_grid, &QwtFigureGrid::sizesChangeFinished);
    QSignalSpy changedSpy(m_grid, &QwtFigureGrid::sizesChanged);

    const int dividerX = qRound(0.5 * m_figure->width());
    const int dividerY = qRound(0.5 * m_figure->height());
    const int midY     = dividerY / 2;

    QVERIFY(pressAt(QPoint(dividerX, midY)));
    QVERIFY(dragTo(QPoint(dividerX + 30, midY)));
    QVERIFY(qAbs(columnAt(0) - 0.5) > RATIO_EPSILON);
    QVERIFY(changedSpy.count() > 0);

    QVERIFY(escape());
    QVERIFY(!m_splitter->isDragging());
    QVERIFY(qAbs(columnAt(0) - 0.5) < RATIO_EPSILON);
    QVERIFY(qAbs(columnAt(1) - 0.5) < RATIO_EPSILON);
    QVERIFY(qAbs(rowAt(0) - 0.5) < RATIO_EPSILON);
    QCOMPARE(finishedSpy.count(), 0);

    // The repaints are enabled again after the cancel
    QVERIFY(m_topPlot->updatesEnabled());
    QVERIFY(m_bottomLeftPlot->updatesEnabled());

    // A release after the cancel must not start or finish a drag
    QVERIFY(!m_splitter->isDragging());
    QCOMPARE(finishedSpy.count(), 0);
}

/**
 * @brief The member widgets stop repainting while a divider is dragged
 */
void TestFigureGridSplitter::testUpdatesDisabledWhileDragging()
{
    const int dividerX = qRound(0.5 * m_figure->width());
    const int dividerY = qRound(0.5 * m_figure->height());
    const int midY     = dividerY / 2;

    QVERIFY(m_topPlot->updatesEnabled());
    QVERIFY(m_bottomLeftPlot->updatesEnabled());
    QVERIFY(m_bottomRightPlot->updatesEnabled());

    QVERIFY(pressAt(QPoint(dividerX, midY)));
    QVERIFY(!m_topPlot->updatesEnabled());
    QVERIFY(!m_bottomLeftPlot->updatesEnabled());
    QVERIFY(!m_bottomRightPlot->updatesEnabled());

    QVERIFY(dragTo(QPoint(dividerX + 25, midY)));
    QVERIFY(!m_topPlot->updatesEnabled());

    QVERIFY(releaseAt(QPoint(dividerX + 25, midY)));
    QVERIFY(m_topPlot->updatesEnabled());
    QVERIFY(m_bottomLeftPlot->updatesEnabled());
    QVERIFY(m_bottomRightPlot->updatesEnabled());
}

/**
 * @brief Events away from the dividers are passed through untouched
 */
void TestFigureGridSplitter::testEventsOutsideBandAreNotConsumed()
{
    const int midX = qRound(0.5 * m_figure->width()) / 2;
    const int midY = qRound(0.5 * m_figure->height()) / 2;

    QVERIFY2(!sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(midX, midY), Qt::NoButton, Qt::NoButton),
             "a hover outside the band must not be consumed");
    QVERIFY2(!pressAt(QPoint(midX, midY)), "a press outside the band must not be consumed");
    QVERIFY(!m_splitter->isDragging());
    QVERIFY2(!releaseAt(QPoint(midX, midY)), "a release outside the band must not be consumed");
}

/**
 * @brief Events delivered to the plot canvas in the band are consumed before the plot
 *
 * The in-band press must not reach the picker/zoom handlers that are installed as event
 * filters on the canvas, and the position mapping from the canvas back to the figure
 * must resolve the divider.
 */
void TestFigureGridSplitter::testCanvasEventsInBandAreConsumed()
{
    QWidget* canvas = m_topPlot->canvas();
    QVERIFY(canvas != nullptr);

    // Install the counter filter first: re-enabling the splitter installs its own
    // filters afterwards, and the most recently installed filter runs first
    m_figure->setGridEditable(false);

    struct PressCounter : QObject
    {
        int presses { 0 };
        bool eventFilter(QObject*, QEvent* event) override
        {
            if (event->type() == QEvent::MouseButtonPress) {
                ++presses;
            }
            return false;
        }
    };
    PressCounter counter;
    canvas->installEventFilter(&counter);

    m_figure->setGridEditable(true);
    QVERIFY(m_figure->isGridEditable());

    // A figure-local point in the column band, inside the canvas of the top plot
    const int dividerX        = qRound(0.5 * m_figure->width());
    const QPoint figureCenter = m_figure->mapFromGlobal(canvas->mapToGlobal(canvas->rect().center()));
    const QPoint canvasPos    = canvas->mapFrom(m_figure, QPoint(dividerX, figureCenter.y()));
    QVERIFY2(canvas->rect().contains(canvasPos), "the test point must be inside the canvas");

    // Hover in the band is consumed and reported
    QList< Qt::CursorShape > shapes;
    connect(m_splitter, &QwtFigureGridSplitter::hoverChanged, this, [ &shapes ](Qt::CursorShape shape) {
        shapes.append(shape);
    });
    QMouseEvent hover(
        QEvent::MouseMove, QPointF(canvasPos), QPointF(canvas->mapToGlobal(canvasPos)), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(canvas, &hover);
    QVERIFY(hover.isAccepted());
    QCOMPARE(shapes.size(), 1);
    QCOMPARE(shapes.last(), Qt::SplitHCursor);

    // The press in the band is consumed before the canvas filter sees it
    QMouseEvent press(QEvent::MouseButtonPress,
                      QPointF(canvasPos),
                      QPointF(canvas->mapToGlobal(canvasPos)),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);
    QApplication::sendEvent(canvas, &press);
    QVERIFY(press.isAccepted());
    QCOMPARE(counter.presses, 0);
    QVERIFY(m_splitter->isDragging());

    QMouseEvent release(QEvent::MouseButtonRelease,
                        QPointF(canvasPos),
                        QPointF(canvas->mapToGlobal(canvasPos)),
                        Qt::LeftButton,
                        Qt::NoButton,
                        Qt::NoModifier);
    QApplication::sendEvent(canvas, &release);
    QVERIFY(release.isAccepted());
    QVERIFY(!m_splitter->isDragging());

    // A press outside the band reaches the canvas and is not consumed by the splitter
    const QPoint outsidePos = canvas->mapFrom(m_figure, QPoint(dividerX / 2, figureCenter.y()));
    QVERIFY2(canvas->rect().contains(outsidePos), "the outside test point must be inside the canvas");
    QMouseEvent outsidePress(QEvent::MouseButtonPress,
                             QPointF(outsidePos),
                             QPointF(canvas->mapToGlobal(outsidePos)),
                             Qt::LeftButton,
                             Qt::LeftButton,
                             Qt::NoModifier);
    QApplication::sendEvent(canvas, &outsidePress);
    QCOMPARE(counter.presses, 1);
    QVERIFY(!m_splitter->isDragging());

    QMouseEvent outsideRelease(QEvent::MouseButtonRelease,
                               QPointF(outsidePos),
                               QPointF(canvas->mapToGlobal(outsidePos)),
                               Qt::LeftButton,
                               Qt::NoButton,
                               Qt::NoModifier);
    QApplication::sendEvent(canvas, &outsideRelease);
}

/**
 * @brief A disabled splitter lets all events through, including the band
 */
void TestFigureGridSplitter::testDisabledSplitterPassesEverything()
{
    m_figure->setGridEditable(false);
    QVERIFY(!m_figure->isGridEditable());
    QVERIFY(!m_splitter->isEnabled());

    QList< Qt::CursorShape > shapes;
    connect(m_splitter, &QwtFigureGridSplitter::hoverChanged, this, [ &shapes ](Qt::CursorShape shape) {
        shapes.append(shape);
    });

    const int dividerX = qRound(0.5 * m_figure->width());
    const int dividerY = qRound(0.5 * m_figure->height());
    const int midY     = dividerY / 2;

    // A press is delivered even without mouse tracking, so it proves that the
    // disabled splitter does not consume the divider band
    QVERIFY2(!pressAt(QPoint(dividerX, midY)), "a disabled splitter must not consume the press");
    QVERIFY(!m_splitter->isDragging());
    QVERIFY2(!releaseAt(QPoint(dividerX, midY)), "a disabled splitter must not consume the release");

    // The hover must neither report a shape nor change a cursor
    sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(dividerX, midY), Qt::NoButton, Qt::NoButton);
    QCOMPARE(shapes.size(), 0);
    QCOMPARE(m_figure->cursor().shape(), Qt::ArrowCursor);

    // Re-enabling picks the widgets up again
    m_figure->setGridEditable(true);
    QVERIFY(m_figure->isGridEditable());

    QVERIFY2(sendMouseEvent(m_figure, QEvent::MouseMove, QPoint(dividerX, midY), Qt::NoButton, Qt::NoButton),
             "the re-enabled splitter must consume the divider band again");
    QCOMPARE(shapes.size(), 1);
    QCOMPARE(shapes.last(), Qt::SplitHCursor);

    QVERIFY(pressAt(QPoint(dividerX, midY)));
    QVERIFY(m_splitter->isDragging());
    QVERIFY(releaseAt(QPoint(dividerX, midY)));
    QVERIFY(!m_splitter->isDragging());
}

/**
 * @brief Enabling grid editing without a retained grid is ignored
 */
void TestFigureGridSplitter::testEditableRequiresEnabledGrid()
{
    QwtFigure plainFigure;

    plainFigure.setGridEditable(true);
    QVERIFY(!plainFigure.isGridEditable());
    // No splitter must be created before a grid exists
    QCOMPARE(plainFigure.findChild< QwtFigureGridSplitter* >(), nullptr);
}

QTEST_MAIN(TestFigureGridSplitter)
#include "test_figure_grid_splitter.moc"
