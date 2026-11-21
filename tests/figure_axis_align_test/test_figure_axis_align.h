#ifndef TEST_FIGURE_AXIS_ALIGN_H
#define TEST_FIGURE_AXIS_ALIGN_H

// Qt
#include <QObject>

// Qwt
class QwtFigure;
class QwtPlot;

/**
 * @brief Tests for the plot area (axis) alignment of QwtFigure
 *
 * The tests verify that the canvas rectangles of the sub plots line up: the
 * left/right edges of the plots sharing a grid column and the top/bottom edges
 * of the plots sharing a grid row. They also cover the spanning members, the
 * manual configurations, the parasite axes and the lifecycle hooks.
 */
class TestFigureAxisAlign : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testNaturalCanvasInsets();
    void testGridColumnsAndRowsAlign();
    void testSpanningMemberAlignment();
    void testManualAxisAlignment();
    void testManualAxisAlignmentComponents();
    void testRemoveMemberRealigns();
    void testDisableGridRestoresNatural();
    void testParasiteAxisAlignment();
    void testAlignmentFollowsAxisScaleChange();
    void testHiddenMemberIsSkipped();

private:
    // Apply a fixed pixel geometry to the figure and run its layout
    static void layoutFigure(QwtFigure* figure, int width, int height);
};

#endif  // TEST_FIGURE_AXIS_ALIGN_H
