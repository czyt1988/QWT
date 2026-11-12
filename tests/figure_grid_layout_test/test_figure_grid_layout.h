#ifndef TEST_FIGURE_GRID_LAYOUT_H
#define TEST_FIGURE_GRID_LAYOUT_H

// Qt
#include <QObject>

// Qwt
class QwtFigure;
class QwtPlot;

/**
 * @brief Tests for the retained grid layout integration of QwtFigure
 */
class TestFigureGridLayout : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testEnableGrid();
    void testGridGeometry();
    void testResizeKeepsRatios();
    void testGridAndFreeMembers();
    void testTrackSizesRelayout();
    void testRemoveWidget();
    void testClickActivatesCurrentAxes();
    void testGridSizeSignals();
    void testOldAddGridAxesUnchanged();

private:
    // Apply a fixed pixel geometry to the figure and run its layout.
    static void layoutFigure(QwtFigure* figure, int width, int height);
};

#endif  // TEST_FIGURE_GRID_LAYOUT_H
