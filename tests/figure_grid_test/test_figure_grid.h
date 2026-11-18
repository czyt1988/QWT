#ifndef TEST_FIGURE_GRID_H
#define TEST_FIGURE_GRID_H

#include <QObject>

class TestFigureGrid : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testSetupEqualTracks();
    void testSetupInvalidCounts();
    void testMinimumTrackPixels();
    void testCustomRatiosAreNormalized();
    void testInvalidSizesRejected();
    void testNormRectSingleCells();
    void testNormRectSpans();
    void testNormRectSpacing();
    void testSpacingGetters();
    void testSpacingUpperBoundRejected();
    void testNormRectInvalid();
    void testAddRemoveItems();
    void testItemDestroyed();
    void testDividers();
    void testDividersWithSpacing();
    void testDividersDisabled();
    void testDividersWithinUnitRange();
    void testSizesChangedSignal();
    void testFinishChange();
};

#endif  // TEST_FIGURE_GRID_H
