#ifndef TEST_COLOR_CYCLE_H
#define TEST_COLOR_CYCLE_H

#include <QObject>

class TestColorCycle : public QObject
{
    Q_OBJECT

private slots:
    void testDefaultPalette();
    void testAllPalettes();
    void testColorWrap();
    void testCustomPalette();
    void testAutoColorCurve();
    void testUserPenPreserved();
    void testAutoColorBarChart();
    void testAutoColorCurveSymbol();
    void testAutoColorIntervalCurve();
    void testUserSymbolPreserved();
    void testDetachReattach();

    // Phase B: default style modernization
    void testGridDefaultPen();
    void testCanvasDefaultBackground();
    void testSymbolDefaultStyle();
    void testColumnSymbolDefaultStyle();
    void testMarkerDefaultPen();
};

#endif
