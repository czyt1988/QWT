/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef TEST_FIGURE_GRID_SPLITTER_H
#define TEST_FIGURE_GRID_SPLITTER_H

#include <QEvent>
#include <QObject>
#include <QPoint>
#include <QTest>
#include <Qt>

class QwtFigure;
class QwtFigureGrid;
class QwtFigureGridSplitter;
class QwtPlot;
class QWidget;

/**
 * @brief Unit tests for QwtFigureGridSplitter
 *
 * The figure is a 400x300 QwtFigure with a 2x2 retained grid, two equal columns
 * and two equal rows. Both dividers therefore sit at 0.5 (200 and 150 pixels)
 * and the default hit width is 4 pixels.
 */
class TestFigureGridSplitter : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();

    void testBandHitRange();
    void testHoverSignalShapes();
    void testCursorAppliedToCanvasNotCell();
    void testPressOutsideBandDoesNotStartDrag();
    void testBoundaryLineIsDefault();
    void testBoundaryDragKeepsRatiosUntilRelease();
    void testBoundaryLineFollowsPointerAndClamps();
    void testBoundaryLineStaysAlignedOnFigureResize();
    void testBoundaryEscapeCancelsWithoutChange();
    void testBoundaryLinePen();
    void testLiveDragChangesOnlyColumnTracks();
    void testLiveDragRelayoutsImmediately();
    void testMinimumTrackClamp();
    void testFinishedEmittedExactlyOnceAndOnlyWhenChanged();
    void testLiveEscapeRestoresOriginalSizes();
    void testMemberRepaintsStayEnabledWhileDragging();
    void testWindowDeactivateCancelsDrag();
    void testUngrabMouseCancelsDrag();
    void testDragSurvivesConsumingCanvasFilter();
    void testEventsOutsideBandAreNotConsumed();
    void testCanvasEventsInBandAreConsumed();
    void testDisabledSplitterPassesEverything();
    void testEditableRequiresEnabledGrid();

private:
    // Sends a synthetic mouse event to receiver, returns the accepted state
    bool sendMouseEvent(QWidget* receiver,
                        QEvent::Type type,
                        const QPoint& pos,
                        Qt::MouseButton button   = Qt::NoButton,
                        Qt::MouseButtons buttons = Qt::NoButton);
    // Sends a synthetic left button press to receiver
    bool pressAt(const QPoint& pos);
    // Sends a synthetic mouse move with the left button held down
    bool dragTo(const QPoint& pos);
    // Sends a synthetic left button release to receiver
    bool releaseAt(const QPoint& pos);
    // Sends a synthetic Esc key press to receiver
    bool escape();
    // Sends a plain event of the given type to receiver
    bool sendSimpleEvent(QWidget* receiver, QEvent::Type type);

    double columnAt(int index) const;
    double rowAt(int index) const;
    // The virtual boundary line overlay of a running boundary drag, nullptr otherwise
    QWidget* boundaryLineWidget() const;

private:
    QwtFigure* m_figure { nullptr };
    QwtFigureGrid* m_grid { nullptr };
    QwtFigureGridSplitter* m_splitter { nullptr };
    QwtPlot* m_topPlot { nullptr };
    QwtPlot* m_bottomLeftPlot { nullptr };
    QwtPlot* m_bottomRightPlot { nullptr };
};

#endif  // TEST_FIGURE_GRID_SPLITTER_H
