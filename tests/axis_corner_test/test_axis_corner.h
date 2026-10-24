#ifndef TEST_AXIS_CORNER_H
#define TEST_AXIS_CORNER_H

#include <QObject>

class TestAxisCorner : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testComposeFixtureClips();
    void testCornerIntersection();
    void testTickDoesNotCrossBackbone();
    void testNoMiterStubWithoutNeighbour();
    void testCornerWithTicksInsideBorder();
    void testPenWidthDeviceContract();
    void testCornerSquareAtAnyPenWidth();
    void testBorderTickSitsOnBand();
    void testInsideTicksShareBackboneWidth();
    void testLayoutMatrix();
    void testRendererDeterminism();
    void testParasiteCorner();
    void testReservationDoesNotExtendScaleGeometry();
    void testCanvasCoversDeviceFootprint();
};

#endif
