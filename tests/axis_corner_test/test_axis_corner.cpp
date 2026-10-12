#include "test_axis_corner.h"

#include <qwt_plot.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_draw.h>
#include <qwt_axis.h>

#include <QApplication>
#include <QImage>
#include <QRect>

#include <qtest.h>

namespace
{
bool isDark(const QImage& img, int x, int y)
{
    return qGray(img.pixel(x, y)) < 128;
}

/*
    The plot of the issue report: canvas aligned to the scales,
    hairline axis pens, no canvas frame.
 */
QImage renderCornerPlot(QwtPlot& plot)
{
    plot.plotLayout()->setAlignCanvasToScales(true);
    plot.axisScaleDraw(QwtAxis::XBottom)->setPenWidthF(1.0);
    plot.axisScaleDraw(QwtAxis::YLeft)->setPenWidthF(1.0);

    QwtPlotCanvas* canvas = new QwtPlotCanvas(&plot);
    canvas->setLineWidth(0);
    plot.setCanvas(canvas);

    QVector<double> xs, ys;
    for (int i = 0; i <= 40; i++) {
        xs << i * 0.25 - 5.0;
        ys << qSin(xs.last());
    }
    QwtPlotCurve* curve = new QwtPlotCurve("sin");
    curve->setSamples(xs, ys);
    curve->attach(&plot);

    plot.resize(480, 360);
    plot.replot();

    return plot.grab().toImage();
}

QRect canvasRectInPlot(const QwtPlot& plot)
{
    const QPoint topLeft = plot.canvas()->mapTo(&plot, QPoint(0, 0));
    return QRect(topLeft, plot.canvas()->rect().size());
}
}

/*
    The backbones of two perpendicular axes must share the canvas
    corner pixel instead of ending in a diagonal step.
 */
void TestAxisCorner::testCornerIntersection()
{
    QwtPlot plot;
    const QImage img = renderCornerPlot(plot);
    if (img.devicePixelRatio() != 1.0)
        QSKIP("pixel assertions require a device pixel ratio of 1");

    const QRect cr = canvasRectInPlot(plot);

    // the yLeft backbone column and the xBottom backbone row
    const int yCol = cr.left() - 1;
    const int xRow = cr.bottom() + 1;

    QVERIFY(isDark(img, yCol, cr.bottom()));      // y backbone reaches the canvas bottom
    QVERIFY(isDark(img, cr.left(), xRow));         // x backbone reaches the canvas left
    QVERIFY(isDark(img, yCol, xRow));              // both share the corner pixel

    // hairline: nothing left of the y backbone, nothing below the x backbone
    QVERIFY(!isDark(img, yCol - 1, cr.top() + 10));
    QVERIFY(!isDark(img, cr.left() + 10, xRow + 1));
}

/*
    Ticks must end at the inner edge of the backbone and never
    protrude into the canvas.
 */
void TestAxisCorner::testTickDoesNotCrossBackbone()
{
    QwtPlot plot;
    const QImage img = renderCornerPlot(plot);
    if (img.devicePixelRatio() != 1.0)
        QSKIP("pixel assertions require a device pixel ratio of 1");

    const QRect cr = canvasRectInPlot(plot);

    // the tick at the scale minimum sits at the canvas bottom row:
    // it may darken the backbone column but not the first canvas column
    QVERIFY(isDark(img, cr.left() - 1, cr.bottom()));
    QVERIFY(!isDark(img, cr.left(), cr.bottom()));

    // the tick at the scale minimum of xBottom sits at the canvas left column:
    // it may darken the backbone row but not the row above it
    QVERIFY(isDark(img, cr.left(), cr.bottom() + 1));
    QVERIFY(!isDark(img, cr.left() + 3, cr.bottom()));
}

/*
    Without a perpendicular neighbour axis the backbone must not
    grow a stub beyond the canvas corner.
 */
void TestAxisCorner::testNoMiterStubWithoutNeighbour()
{
    QwtPlot plot;
    const QImage img = renderCornerPlot(plot);
    if (img.devicePixelRatio() != 1.0)
        QSKIP("pixel assertions require a device pixel ratio of 1");

    const QRect cr = canvasRectInPlot(plot);

    // xTop is invisible: the yLeft backbone starts at the canvas top row
    QVERIFY(isDark(img, cr.left() - 1, cr.top()));
    QVERIFY(!isDark(img, cr.left() - 1, cr.top() - 1));

    // yRight is invisible: the xBottom backbone ends at the canvas right column
    QVERIFY(isDark(img, cr.right(), cr.bottom() + 1));
    QVERIFY(!isDark(img, cr.right() + 1, cr.bottom() + 1));
}

QTEST_MAIN(TestAxisCorner)
#include "test_axis_corner.moc"
