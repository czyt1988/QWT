#ifndef TEST_AXIS_CORNER_H
#define TEST_AXIS_CORNER_H

#include <QObject>

class TestAxisCorner : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCornerIntersection();
    void testTickDoesNotCrossBackbone();
    void testNoMiterStubWithoutNeighbour();
};

#endif
