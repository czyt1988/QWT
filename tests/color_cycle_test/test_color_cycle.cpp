#include "test_color_cycle.h"

#include "qwt_color_cycle.h"
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_grid.h"
#include "qwt_plot_marker.h"
#include "qwt_symbol.h"
#include "qwt_column_symbol.h"

#include <QTest>
#include <QVector>
#include <QColor>
#include <QPen>
#include <QBrush>
#include <QSize>

void TestColorCycle::testDefaultPalette()
{
    QwtColorCycle cc;
    QCOMPARE(cc.count(), 10);

    // Default10 first color is #1f77b4
    QCOMPARE(cc.color(0), QColor("#1f77b4"));
}

void TestColorCycle::testAllPalettes()
{
    struct PaletteInfo {
        QwtColorCycle::Palette palette;
        int expectedCount;
    };

    const QVector<PaletteInfo> palettes = {
        { QwtColorCycle::Default10, 10 },
        { QwtColorCycle::Tab10,     10 },
        { QwtColorCycle::Tab20,     20 },
        { QwtColorCycle::Set1,       9 },
        { QwtColorCycle::Set2,       8 },
        { QwtColorCycle::Set3,      12 },
        { QwtColorCycle::OkabeIto,   8 },
    };

    for (const auto& info : palettes) {
        QwtColorCycle cc(info.palette);
        QCOMPARE(cc.count(), info.expectedCount);

        // Verify all colors are valid
        for (int i = 0; i < cc.count(); i++) {
            QVERIFY(cc.color(i).isValid());
        }
    }
}

void TestColorCycle::testColorWrap()
{
    QwtColorCycle cc;
    const int n = cc.count();
    QVERIFY(n > 0);

    // Wrapping: color(n) == color(0), color(n+1) == color(1), etc.
    QCOMPARE(cc.color(n), cc.color(0));
    QCOMPARE(cc.color(n + 1), cc.color(1));
    QCOMPARE(cc.color(2 * n + 3), cc.color(3));
}

void TestColorCycle::testCustomPalette()
{
    QVector<QColor> custom = { Qt::red, Qt::green, Qt::blue };
    QwtColorCycle cc(custom);

    QCOMPARE(cc.count(), 3);
    QCOMPARE(cc.color(0), QColor(Qt::red));
    QCOMPARE(cc.color(1), QColor(Qt::green));
    QCOMPARE(cc.color(2), QColor(Qt::blue));
    QCOMPARE(cc.color(3), QColor(Qt::red)); // wraps
}

void TestColorCycle::testAutoColorCurve()
{
    QwtPlot plot;
    plot.show();

    QwtColorCycle cc;
    plot.setColorCycle(cc);

    auto* c1 = new QwtPlotCurve("Curve 1");
    c1->attach(&plot);

    auto* c2 = new QwtPlotCurve("Curve 2");
    c2->attach(&plot);

    auto* c3 = new QwtPlotCurve("Curve 3");
    c3->attach(&plot);

    // Each curve should get a different color from Default10
    QCOMPARE(c1->pen().color(), cc.color(0));
    QCOMPARE(c2->pen().color(), cc.color(1));
    QCOMPARE(c3->pen().color(), cc.color(2));

    // All colors should be different
    QVERIFY(c1->pen().color() != c2->pen().color());
    QVERIFY(c2->pen().color() != c3->pen().color());
}

void TestColorCycle::testUserPenPreserved()
{
    QwtPlot plot;
    plot.show();

    auto* c1 = new QwtPlotCurve("Manual");
    c1->setPen(QColor(Qt::red));
    c1->attach(&plot);

    // User-set pen should not be overridden
    QCOMPARE(c1->pen().color(), QColor(Qt::red));

    // Next auto-colored curve should get color(0) since counter was not incremented
    auto* c2 = new QwtPlotCurve("Auto");
    c2->attach(&plot);

    QwtColorCycle cc;
    QCOMPARE(c2->pen().color(), cc.color(0));
}

void TestColorCycle::testAutoColorBarChart()
{
    QwtPlot plot;
    plot.show();

    QwtColorCycle cc;
    plot.setColorCycle(cc);

    auto* bar = new QwtPlotBarChart("Bar");
    bar->attach(&plot);

    // Bar chart brush should be auto-assigned from color cycle
    QCOMPARE(bar->brush().color(), cc.color(0));

    // Pen should be a darker shade
    QCOMPARE(bar->pen().color(), cc.color(0).darker(150));
}

void TestColorCycle::testAutoColorCurveSymbol()
{
    QwtPlot plot;
    plot.show();

    QwtColorCycle cc;
    plot.setColorCycle(cc);

    // Case A: default symbol assigned AFTER attach.
    auto* c1 = new QwtPlotCurve("Scatter 1");
    c1->attach(&plot);
    auto* sym1 = new QwtSymbol(QwtSymbol::Ellipse);
    sym1->setSize(QSize(6, 6));
    c1->setSymbol(sym1);

    // Symbol brush/pen should follow the curve pen color from the cycle.
    QCOMPARE(c1->pen().color(), cc.color(0));
    QCOMPARE(c1->symbol()->brush().color(), cc.color(0));
    QCOMPARE(c1->symbol()->pen().color(), cc.color(0).darker(150));

    // Case B: default symbol assigned BEFORE attach.
    auto* c2 = new QwtPlotCurve("Scatter 2");
    auto* sym2 = new QwtSymbol(QwtSymbol::Ellipse);
    sym2->setSize(QSize(6, 6));
    c2->setSymbol(sym2);
    c2->attach(&plot);

    QCOMPARE(c2->pen().color(), cc.color(1));
    QCOMPARE(c2->symbol()->brush().color(), cc.color(1));
    QCOMPARE(c2->symbol()->pen().color(), cc.color(1).darker(150));
}

void TestColorCycle::testAutoColorIntervalCurve()
{
    QwtPlot plot;
    plot.show();

    QwtColorCycle cc;
    plot.setColorCycle(cc);

    auto* c1 = new QwtPlotIntervalCurve("Interval 1");
    c1->attach(&plot);

    auto* c2 = new QwtPlotIntervalCurve("Interval 2");
    c2->attach(&plot);

    QCOMPARE(c1->pen().color(), cc.color(0));
    QCOMPARE(c2->pen().color(), cc.color(1));
    // brush is a semi-transparent version of the pen color
    const QColor exp(cc.color(0).red(), cc.color(0).green(), cc.color(0).blue(), 128);
    QCOMPARE(c1->brush().color(), exp);
}

void TestColorCycle::testUserSymbolPreserved()
{
    QwtPlot plot;
    plot.show();

    QwtColorCycle cc;
    plot.setColorCycle(cc);

    // User explicitly customizes the symbol brush/pen -> must be preserved.
    auto* c1 = new QwtPlotCurve("Manual Scatter");
    auto* sym1 = new QwtSymbol(QwtSymbol::Ellipse, QBrush(Qt::red), QPen(Qt::black, 1), QSize(6, 6));
    c1->setSymbol(sym1);
    c1->attach(&plot);

    QVERIFY(c1->symbol()->brush().color() != cc.color(0));
    QCOMPARE(c1->symbol()->brush().color(), QColor(Qt::red));
    QCOMPARE(c1->symbol()->pen().color(), QColor(Qt::black));
}

void TestColorCycle::testDetachReattach()
{
    QwtPlot plot;
    plot.show();

    auto* c1 = new QwtPlotCurve("Curve 1");
    c1->attach(&plot);

    const QColor originalColor = c1->pen().color();

    // Detach and reattach
    c1->detach();
    c1->attach(&plot);

    // Color should remain the same (pen is no longer default black)
    QCOMPARE(c1->pen().color(), originalColor);
}

void TestColorCycle::testGridDefaultPen()
{
    QwtPlotGrid grid;
    // Major pen: light gray #c0c0c0, width 0.5, solid
    QCOMPARE(grid.majorPen().color(), QColor("#c0c0c0"));
    QVERIFY(qFuzzyCompare(grid.majorPen().widthF(), 0.5));
    QCOMPARE(grid.majorPen().style(), Qt::SolidLine);

    // Minor pen: lighter gray #e0e0e0, width 0.5, dot
    QCOMPARE(grid.minorPen().color(), QColor("#e0e0e0"));
    QVERIFY(qFuzzyCompare(grid.minorPen().widthF(), 0.5));
    QCOMPARE(grid.minorPen().style(), Qt::DotLine);
}

void TestColorCycle::testCanvasDefaultBackground()
{
    QwtPlot plot;
    QCOMPARE(plot.canvasBackground().color(), QColor(Qt::white));
}

void TestColorCycle::testSymbolDefaultStyle()
{
    QwtSymbol sym;
    QCOMPARE(sym.brush().style(), Qt::NoBrush);
    QCOMPARE(sym.pen().color(), QColor("#555555"));
}

void TestColorCycle::testColumnSymbolDefaultStyle()
{
    QwtColumnSymbol sym(QwtColumnSymbol::Box);
    QCOMPARE(sym.brush().color(), QColor("#c0c0c0"));
    QCOMPARE(sym.pen().color(), QColor("#888888"));
}

void TestColorCycle::testMarkerDefaultPen()
{
    QwtPlotMarker marker;
    QCOMPARE(marker.linePen().color(), QColor("#555555"));
}

QTEST_MAIN(TestColorCycle)
