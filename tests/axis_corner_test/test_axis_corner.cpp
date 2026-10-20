#include "test_axis_corner.h"

#include <qwt_axis.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_renderer.h>
#include <qwt_scale_div.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_widget.h>

#include <QApplication>
#include <QImage>
#include <QMargins>
#include <QPainter>
#include <QRect>
#include <QtTest>

namespace
{
bool isDark(const QImage& img, int x, int y)
{
    return qGray(img.pixel(x, y)) < 128;
}

bool someDark(const QImage& img, int x, int y0, int y1)
{
    for (int y = y0; y <= y1; y++) {
        if (isDark(img, x, y))
            return true;
    }
    return false;
}

/*
    Map a logical (plot) coordinate to the device pixel grid of an image
    rendered at its device pixel ratio. The painters of the plot widgets map
    logical coordinates through translate(widgetPos) * scale(dpr) with integer
    widget positions, so the device position of a logical position p is
    round(p * dpr).
 */
int toDevice(const QImage& img, qreal logicalPos)
{
    return qRound(logicalPos * img.devicePixelRatio());
}

/*
    The device width of the scale pens: the pixel contract of QwtScaleDraw
    rounds the pen width to whole device pixels without scaling it by the
    device pixel ratio.
 */
int devicePenWidth(qreal penWidthF)
{
    return qMax(1, qRound(penWidthF + 1e-6));
}

/*
    A widget painting 20 pixels beyond its right border: the control for the
    compose fixture below.
 */
class OverpaintWidget : public QWidget
{
public:
    explicit OverpaintWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setPen(QPen(Qt::black, 1));
        painter.drawLine(0, height() / 2, width() + 20, height() / 2);
    }
};

/*
    Compose an image of the plot by rendering every widget on its own.

    Each widget is rendered clipped to its own rectangle - exactly like on
    screen, where the backing store never lets a widget paint outside of its
    geometry. The corner assertions need this clipping: the defect they guard
    against is a backbone miter clipped away at the border of its axis widget.
    Composing the widgets one by one makes the clipping explicit instead of
    relying on how grab() of the whole tree happens to treat child widgets.

    The widgets are rendered with DrawChildren only: on screen the plot paints
    its background once and the (non autofilling) children paint on top of it
    and of each other where they overlap. The default render flags would fill
    every widget with its window background instead, erasing the pixels of
    previously rendered widgets in the overlap.
 */
QImage composePlot(QwtPlot& plot, const QList<QWidget*>& extraWidgets = QList<QWidget*>())
{
    // grab() runs the layout of the still hidden widget tree, so all
    // geometries are current when the widgets are rendered below
    plot.grab();

    const qreal dpr = plot.devicePixelRatioF();
    QImage img(qRound(plot.width() * dpr), qRound(plot.height() * dpr), QImage::Format_ARGB32_Premultiplied);
    img.setDevicePixelRatio(dpr);
    img.fill(Qt::white);

    const auto renderWidget = [&img, &plot](QWidget* w) {
        w->render(&img, w->mapTo(&plot, QPoint(0, 0)), QRegion(), QWidget::DrawChildren);
    };

    renderWidget(plot.canvas());

    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++) {
        const QwtAxisId axisId(axisPos);
        if (!plot.isAxisVisible(axisId))
            continue;

        renderWidget(plot.axisWidget(axisId));
    }

    for (QWidget* w : extraWidgets)
        renderWidget(w);

    return img;
}

QRect canvasRectInPlot(const QwtPlot& plot)
{
    const QPoint topLeft = plot.canvas()->mapTo(&plot, QPoint(0, 0));
    return QRect(topLeft, plot.canvas()->rect().size());
}

/*
    The plot of the issue report: canvas aligned to the scales, hairline
    axis pens, no canvas frame.
 */
void setupCornerPlot(QwtPlot& plot)
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
}

/*
    A plot whose outermost major ticks sit clear of the scale borders: the
    natural border distance hints collapse to 0 then, the axis widgets end
    flush with the scales and the corner miters have no room inside the
    widgets without the reservation of QwtPlot::doLayout(). This is the
    configuration of the HiDPI issue report, where the missing corner shows
    up as 2 blank device pixels at 200% scaling.
 */
void setupTicksInsideBorderPlot(QwtPlot& plot)
{
    plot.plotLayout()->setAlignCanvasToScales(true);
    plot.axisScaleDraw(QwtAxis::XBottom)->setPenWidthF(1.0);
    plot.axisScaleDraw(QwtAxis::YLeft)->setPenWidthF(1.0);

    QwtPlotCanvas* canvas = new QwtPlotCanvas(&plot);
    canvas->setLineWidth(0);
    plot.setCanvas(canvas);

    QVector<double> xs, ys;
    for (int i = 0; i <= 40; i++) {
        xs << i * 0.25;
        ys << qSin(xs.last());
    }
    QwtPlotCurve* curve = new QwtPlotCurve("sin");
    curve->setSamples(xs, ys);
    curve->attach(&plot);

    // explicit scale divisions whose bounds extend beyond the outermost
    // major ticks, so the ticks sit clear of the scale borders
    {
        QList<double> majors;
        for (double v = 0.0; v <= 1.0 + 0.001; v += 0.2)
            majors += v;

        plot.setAxisScaleDiv(QwtAxis::YLeft,
            QwtScaleDiv(-0.13, 1.0, QList<double>(), QList<double>(), majors));
    }
    {
        QList<double> majors;
        for (int v = 0; v <= 10; v++)
            majors += double(v);

        plot.setAxisScaleDiv(QwtAxis::XBottom,
            QwtScaleDiv(-0.7, 10.0, QList<double>(), QList<double>(), majors));
    }

    plot.resize(480, 360);
    plot.replot();
}

/*
    Device positions of the two backbone bands that form the bottom-left
    corner, taken from the geometry the axis widgets were laid out with:
    the y band column of yw and the x band row of xw. Exact for the hairline
    pens the corner tests use, and immune to canvas frames or margins that
    would confuse a pixel scan.
 */
QPoint bandCorner(const QImage& img, const QwtPlot& root, const QwtScaleWidget* yw, const QwtScaleWidget* xw)
{
    const qreal yCol = yw->mapTo(&root, QPoint(0, 0)).x() + yw->scaleDraw()->pos().x();
    const qreal xRow = xw->mapTo(&root, QPoint(0, 0)).y() + xw->scaleDraw()->pos().y();
    return QPoint(toDevice(img, yCol), toDevice(img, xRow));
}

/*
    Verify that the two backbone bands intersect at (yCol, xRow): the corner
    pixel is dark and there is no white row or column between the bands.
    cr is the canvas rectangle for a sanity check of the band positions -
    pass an invalid rect to skip it (stacked parasite axes sit outside of
    the canvas corner).
 */
void checkCornerClosed(const QImage& img, const QRect& cr, const QPoint& band, const QString& ctx)
{
    const int dpr  = qCeil(img.devicePixelRatio());
    const int yCol = band.x();
    const int xRow = band.y();

    if (cr.isValid()) {
        // sanity: the bands sit where the canvas corner is
        QVERIFY2(qAbs(yCol - toDevice(img, cr.left() - 1)) <= 8 * dpr,
            qPrintable(ctx + QString(": unexpected y band column %1").arg(yCol)));
        QVERIFY2(xRow >= toDevice(img, cr.bottom()) - 2 * dpr && xRow <= toDevice(img, cr.bottom()) + 10 * dpr,
            qPrintable(ctx + QString(": unexpected x band row %1").arg(xRow)));
    }

    // no white row between the end of the y band and the x band
    for (int r = xRow - 5 * dpr; r <= xRow; r++) {
        QVERIFY2(isDark(img, yCol, r),
            qPrintable(QString("%1: gap in the y backbone at (col=%2, row=%3), xRow=%4")
                .arg(ctx).arg(yCol).arg(r).arg(xRow)));
    }

    // the x band reaches the y band and continues into the canvas
    for (int c = yCol; c <= yCol + 5 * dpr; c++) {
        QVERIFY2(isDark(img, c, xRow),
            qPrintable(QString("%1: gap in the x backbone at (col=%2, row=%3), yCol=%4")
                .arg(ctx).arg(c).arg(xRow).arg(yCol)));
    }
}

QImage renderToImage(QwtPlot& plot, qreal scale)
{
    QImage img(qRound(plot.width() * scale), qRound(plot.height() * scale), QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::white);

    QwtPlotRenderer renderer;
    renderer.renderTo(&plot, img);
    return img;
}
}

/*
    Control for the compose fixture: rendering a single widget clips at the
    widget border, like the screen does. Whatever paints beyond the border
    of its widget must not show up in the composed image.
 */
void TestAxisCorner::testComposeFixtureClips()
{
    QWidget root;
    root.resize(80, 40);

    OverpaintWidget* child = new OverpaintWidget(&root);
    child->setGeometry(0, 0, 40, 40);

    const qreal dpr = root.devicePixelRatioF();

    QImage composed(qRound(80 * dpr), qRound(40 * dpr), QImage::Format_ARGB32_Premultiplied);
    composed.setDevicePixelRatio(dpr);
    composed.fill(Qt::white);
    child->render(&composed, child->mapTo(&root, QPoint(0, 0)), QRegion(), QWidget::DrawChildren);

    const int row   = toDevice(composed, 20);
    const int slack = qCeil(2 * dpr);

    // inside the child rectangle the line is painted ...
    QVERIFY(someDark(composed, toDevice(composed, 39), row - slack, row + slack));
    // ... beyond the child border it is clipped away
    QVERIFY(!someDark(composed, toDevice(composed, 45), row - slack, row + slack));

    // grab() of the whole tree clips its children the same way (measured,
    // Qt 6.7 offscreen), so both images agree
    const QImage grabbed = root.grab().toImage();
    QVERIFY(someDark(grabbed, toDevice(grabbed, 39), row - slack, row + slack));
    QVERIFY(!someDark(grabbed, toDevice(grabbed, 45), row - slack, row + slack));
}

/*
    The backbones of two perpendicular axes must share the canvas
    corner pixel instead of ending in a diagonal step.
 */
void TestAxisCorner::testCornerIntersection()
{
    QwtPlot plot;
    setupCornerPlot(plot);

    const QImage img = composePlot(plot);
    const QRect cr   = canvasRectInPlot(plot);

    // the yLeft backbone column and the xBottom backbone row
    const int yCol = toDevice(img, cr.left() - 1);
    const int xRow = toDevice(img, cr.bottom() + 1);
    const int pw   = devicePenWidth(1.0);

    QVERIFY(isDark(img, yCol, toDevice(img, cr.bottom())));  // y backbone reaches the canvas bottom
    QVERIFY(isDark(img, toDevice(img, cr.left()), xRow));    // x backbone reaches the canvas left
    QVERIFY(isDark(img, yCol, xRow));                        // both share the corner pixel

    // hairline: nothing left of the y backbone, nothing below the x backbone
    QVERIFY(!isDark(img, yCol - pw, toDevice(img, cr.top() + 10)));
    QVERIFY(!isDark(img, toDevice(img, cr.left() + 10), xRow + pw));
}

/*
    Ticks must end at the inner edge of the backbone and never
    protrude into the canvas.
 */
void TestAxisCorner::testTickDoesNotCrossBackbone()
{
    QwtPlot plot;
    setupCornerPlot(plot);

    const QImage img = composePlot(plot);
    const QRect cr   = canvasRectInPlot(plot);

    // the tick at the scale minimum sits at the canvas bottom row:
    // it may darken the backbone column but not the first canvas column
    QVERIFY(isDark(img, toDevice(img, cr.left() - 1), toDevice(img, cr.bottom())));
    QVERIFY(!isDark(img, toDevice(img, cr.left()), toDevice(img, cr.bottom())));

    // the tick at the scale minimum of xBottom sits at the canvas left column:
    // it may darken the backbone row but not the row above it
    QVERIFY(isDark(img, toDevice(img, cr.left()), toDevice(img, cr.bottom() + 1)));
    QVERIFY(!isDark(img, toDevice(img, cr.left() + 3), toDevice(img, cr.bottom())));

    // the border ticks snap onto the perpendicular backbone:
    // the x minimum tick hangs at the y backbone column,
    // the y minimum tick sits on the x backbone row
    QVERIFY(isDark(img, toDevice(img, cr.left() - 1), toDevice(img, cr.bottom() + 2)));
    QVERIFY(isDark(img, toDevice(img, cr.left() - 3), toDevice(img, cr.bottom() + 1)));
}

/*
    Without a perpendicular neighbour axis the backbone must not
    grow a stub beyond the canvas corner.
 */
void TestAxisCorner::testNoMiterStubWithoutNeighbour()
{
    QwtPlot plot;
    setupCornerPlot(plot);

    const QImage img = composePlot(plot);
    const QRect cr   = canvasRectInPlot(plot);
    const int pw     = devicePenWidth(1.0);

    // xTop is invisible: the yLeft backbone starts at the canvas top row
    QVERIFY(isDark(img, toDevice(img, cr.left() - 1), toDevice(img, cr.top())));
    QVERIFY(!isDark(img, toDevice(img, cr.left() - 1), toDevice(img, cr.top()) - pw));

    // yRight is invisible: the xBottom backbone ends at the canvas right column
    QVERIFY(isDark(img, toDevice(img, cr.right()), toDevice(img, cr.bottom() + 1)));
    QVERIFY(!isDark(img, toDevice(img, cr.right()) + pw, toDevice(img, cr.bottom() + 1)));
}

/*
    The corner must also close when the border distance hints collapse
    to 0 and the axis widgets end flush with the scales - the widget
    reservation and the exact miter targets have to kick in then.
 */
void TestAxisCorner::testCornerWithTicksInsideBorder()
{
    QwtPlot plot;
    setupTicksInsideBorderPlot(plot);

    const QImage img = composePlot(plot);
    const QRect cr   = canvasRectInPlot(plot);

    // precondition: the hints really collapsed to 0 (composePlot has run
    // the layout of the hidden widget tree, so the distances are current)
    QCOMPARE(plot.axisWidget(QwtAxis::YLeft)->endBorderDist(), 0);
    QCOMPARE(plot.axisWidget(QwtAxis::XBottom)->startBorderDist(), 0);

    // the reservation: the axis widgets cover the corner pixels
    {
        const QwtScaleWidget* yw = plot.axisWidget(QwtAxis::YLeft);
        const QRect yr(yw->mapTo(&plot, QPoint(0, 0)), yw->size());
        QVERIFY2(yr.bottom() >= cr.bottom() + 1, "the y widget must cover the x backbone row");

        const QwtScaleWidget* xw = plot.axisWidget(QwtAxis::XBottom);
        const QRect xr(xw->mapTo(&plot, QPoint(0, 0)), xw->size());
        QVERIFY2(xr.left() <= cr.left() - 1, "the x widget must cover the y backbone column");
    }

    // the reservation must not shift the scale spans: with the canvas
    // aligned to the scales they still cover exactly the canvas
    {
        const QwtScaleWidget* yw = plot.axisWidget(QwtAxis::YLeft);
        const QwtScaleDraw* ysd  = yw->scaleDraw();
        const qreal yEnd         = yw->mapTo(&plot, QPoint(0, 0)).y() + ysd->pos().y() + ysd->length();
        QVERIFY2(qFuzzyCompare(yEnd, qreal(cr.bottom())),
            qPrintable(QString("y scale end %1 != canvas bottom %2").arg(yEnd).arg(cr.bottom())));

        const QwtScaleWidget* xw = plot.axisWidget(QwtAxis::XBottom);
        const QwtScaleDraw* xsd  = xw->scaleDraw();
        const qreal xStart       = xw->mapTo(&plot, QPoint(0, 0)).x() + xsd->pos().x();
        QVERIFY2(qFuzzyCompare(xStart, qreal(cr.left())),
            qPrintable(QString("x scale start %1 != canvas left %2").arg(xStart).arg(cr.left())));
    }

    const QPoint band = bandCorner(img, plot, plot.axisWidget(QwtAxis::YLeft), plot.axisWidget(QwtAxis::XBottom));
    checkCornerClosed(img, cr, band, "ticksInsideBorder");
}

/*
    setPenWidthF() is a device pixel contract: the backbone is painted
    qMax(1, round(width)) device pixels wide at any device pixel ratio.
 */
void TestAxisCorner::testPenWidthDeviceContract()
{
    const qreal widths[] = { 0.0, 1.0, 2.0, 3.0 };

    for (qreal w : widths) {
        QwtPlot plot;
        plot.plotLayout()->setAlignCanvasToScales(true);

        QwtPlotCanvas* canvas = new QwtPlotCanvas(&plot);
        canvas->setLineWidth(0);
        plot.setCanvas(canvas);

        const QwtAxisId axes[] = { QwtAxis::XBottom, QwtAxis::YLeft };
        for (QwtAxisId axisId : axes) {
            QwtScaleDraw* sd = plot.axisScaleDraw(axisId);
            sd->setPenWidthF(w);
            sd->enableComponent(QwtAbstractScaleDraw::Ticks, false);
            sd->enableComponent(QwtAbstractScaleDraw::Labels, false);
        }

        plot.resize(480, 360);
        plot.replot();

        const QImage img = composePlot(plot);
        const QRect cr   = canvasRectInPlot(plot);
        const int pw     = devicePenWidth(w);

        // width of the dark run around the inner edge of the y backbone
        {
            const int inner = toDevice(img, cr.left() - 1);
            const int row   = toDevice(img, cr.top() + cr.height() * 0.5);

            int c0 = inner;
            while (c0 - 1 >= 0 && isDark(img, c0 - 1, row))
                c0--;
            int c1 = inner;
            while (c1 + 1 < img.width() && isDark(img, c1 + 1, row))
                c1++;

            QVERIFY2(c1 - c0 + 1 == pw,
                qPrintable(QString("penWidthF(%1): y backbone is %2 device pixels wide, expected %3")
                    .arg(w).arg(c1 - c0 + 1).arg(pw)));
        }

        // width of the dark run around the inner edge of the x backbone
        {
            const int inner = toDevice(img, cr.bottom() + 1);
            const int col   = toDevice(img, cr.left() + cr.width() * 0.5);

            int r0 = inner;
            while (r0 - 1 >= 0 && isDark(img, col, r0 - 1))
                r0--;
            int r1 = inner;
            while (r1 + 1 < img.height() && isDark(img, col, r1 + 1))
                r1++;

            QVERIFY2(r1 - r0 + 1 == pw,
                qPrintable(QString("penWidthF(%1): x backbone is %2 device pixels wide, expected %3")
                    .arg(w).arg(r1 - r0 + 1).arg(pw)));
        }
    }
}

/*
    The corner must close in every combination of the layout switches that
    move the axis widgets relative to the canvas.
 */
void TestAxisCorner::testLayoutMatrix()
{
    const bool aligns[]    = { true, false };
    const int margins[]    = { -1, 0, 1, 3 };  // -1 is the default
    const int lineWidths[] = { 0, 1, 2 };
    const QSize fixedSizes[] = { QSize(), QSize(300, 200), QSize(301, 201) };

    for (bool align : aligns) {
        for (int margin : margins) {
            for (int lineWidth : lineWidths) {
                for (const QSize& fixed : fixedSizes) {
                    QwtPlot plot;
                    plot.plotLayout()->setAlignCanvasToScales(align);
                    plot.plotLayout()->setCanvasMargin(margin);
                    if (fixed.isValid())
                        plot.plotLayout()->setFixedCanvasSize(fixed);

                    QwtPlotCanvas* canvas = new QwtPlotCanvas(&plot);
                    canvas->setLineWidth(lineWidth);
                    plot.setCanvas(canvas);

                    plot.axisScaleDraw(QwtAxis::XBottom)->setPenWidthF(1.0);
                    plot.axisScaleDraw(QwtAxis::YLeft)->setPenWidthF(1.0);

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

                    const QImage img = composePlot(plot);
                    const QRect cr   = canvasRectInPlot(plot);

                    const QString ctx = QString("align=%1 margin=%2 lineWidth=%3 fixed=%4x%5")
                        .arg(align).arg(margin).arg(lineWidth).arg(fixed.width()).arg(fixed.height());

                    if (align) {
                        // the corner reservation must not shift the scale
                        // spans away from the aligned canvas content (a
                        // canvas frame sits between the scales and the
                        // content rect)
                        const QMargins cm = plot.canvas()->contentsMargins();

                        const QwtScaleWidget* yw = plot.axisWidget(QwtAxis::YLeft);
                        const QwtScaleDraw* ysd  = yw->scaleDraw();
                        const qreal yEnd         = yw->mapTo(&plot, QPoint(0, 0)).y() + ysd->pos().y() + ysd->length();
                        QVERIFY2(qFuzzyCompare(yEnd, qreal(cr.bottom() - cm.bottom())),
                            qPrintable(ctx + QString(": y scale end %1 != canvas bottom %2").arg(yEnd).arg(cr.bottom() - cm.bottom())));

                        const QwtScaleWidget* xw = plot.axisWidget(QwtAxis::XBottom);
                        const QwtScaleDraw* xsd  = xw->scaleDraw();
                        const qreal xStart       = xw->mapTo(&plot, QPoint(0, 0)).x() + xsd->pos().x();
                        QVERIFY2(qFuzzyCompare(xStart, qreal(cr.left() + cm.left())),
                            qPrintable(ctx + QString(": x scale start %1 != canvas left %2").arg(xStart).arg(cr.left() + cm.left())));
                    }

                    const QPoint band = bandCorner(img, plot,
                        plot.axisWidget(QwtAxis::YLeft), plot.axisWidget(QwtAxis::XBottom));
                    checkCornerClosed(img, cr, band, ctx);
                }
            }
        }
    }
}

/*
    Exporting must be deterministic: the renderer sets the corner miter
    state of the shared scale draws itself, so the output does not depend
    on whether the plot was painted on screen before.
 */
void TestAxisCorner::testRendererDeterminism()
{
    QwtPlot plot;
    setupTicksInsideBorderPlot(plot);

    // determinism at the device scale: rendering in between must not
    // change a single byte of the export
    const qreal dpr   = plot.devicePixelRatioF();
    const QImage img1 = renderToImage(plot, dpr);

    // a screen paint leaves miter state on the shared scale draws
    plot.grab();

    const QImage img2 = renderToImage(plot, dpr);
    QVERIFY2(img1 == img2, "rendering before the export changed the export");

    // the corner closes in the export at 1:1, where the renderer layout
    // matches the widget layout the band positions are taken from
    const QImage img = renderToImage(plot, 1.0);
    const QPoint band = bandCorner(img, plot, plot.axisWidget(QwtAxis::YLeft), plot.axisWidget(QwtAxis::XBottom));
    checkCornerClosed(img, canvasRectInPlot(plot), band, "renderer");
}

/*
    The perpendicular axis of a parasite plot corner belongs to the host:
    the miter flags and targets must be resolved through the host plot.
    The parasite YLeft widget overlays the band of the host YLeft widget
    (the parasite layout copies the host rectangles), and the parasite has
    no XBottom axis of its own - its corner is the XBottom axis of the host.
 */
void TestAxisCorner::testParasiteCorner()
{
    QwtPlot host;
    host.plotLayout()->setAlignCanvasToScales(true);

    QwtPlotCanvas* canvas = new QwtPlotCanvas(&host);
    canvas->setLineWidth(0);
    host.setCanvas(canvas);

    host.axisScaleDraw(QwtAxis::XBottom)->setPenWidthF(1.0);
    host.axisScaleDraw(QwtAxis::YLeft)->setPenWidthF(1.0);

    QVector<double> xs, ys;
    for (int i = 0; i <= 40; i++) {
        xs << i * 0.25;
        ys << qSin(xs.last());
    }
    QwtPlotCurve* curve = new QwtPlotCurve("sin");
    curve->setSamples(xs, ys);
    curve->attach(&host);

    // ticks clear of the borders on both corner axes, like the issue report
    {
        QList<double> majors;
        for (int v = 0; v <= 10; v++)
            majors += double(v);

        host.setAxisScaleDiv(QwtAxis::XBottom,
            QwtScaleDiv(-0.7, 10.0, QList<double>(), QList<double>(), majors));
    }
    {
        QList<double> majors;
        for (double v = 0.0; v <= 1.0 + 0.001; v += 0.2)
            majors += v;

        host.setAxisScaleDiv(QwtAxis::YLeft,
            QwtScaleDiv(-0.13, 1.0, QList<double>(), QList<double>(), majors));
    }

    QwtPlot* parasite = host.createParasitePlot(QwtAxis::YLeft);
    QVERIFY(parasite != nullptr);
    QVERIFY(!parasite->isAxisVisible(QwtAxis::XBottom));
    parasite->axisScaleDraw(QwtAxis::YLeft)->setPenWidthF(1.0);

    host.resize(480, 360);
    host.replot();

    QList<QWidget*> extra;
    extra << parasite->axisWidget(QwtAxis::YLeft);

    const QImage img = composePlot(host, extra);
    const QRect cr   = canvasRectInPlot(host);

    // the miter of the parasite y axis is resolved through the host x axis
    QVERIFY(parasite->axisScaleDraw(QwtAxis::YLeft)->cornerMiter().testFlag(QwtScaleDraw::MiterAtEnd));

    // the host's own corner closes as usual
    const QPoint hostBand = bandCorner(img, host,
        host.axisWidget(QwtAxis::YLeft), host.axisWidget(QwtAxis::XBottom));
    checkCornerClosed(img, cr, hostBand, "parasite-host");

    // the parasite band is stacked outside of the host band, and the host
    // x backbone reaches out to it, so its corner closes as well
    const QPoint band = bandCorner(img, host,
        parasite->axisWidget(QwtAxis::YLeft), host.axisWidget(QwtAxis::XBottom));
    QVERIFY2(band.x() < hostBand.x(), "the parasite band must sit outside of the host band");
    QCOMPARE(band.y(), hostBand.y());
    checkCornerClosed(img, QRect(), band, "parasite");
}

int main(int argc, char* argv[])
{
    /*
        The corner geometry depends on the device pixel ratio, and
        QT_SCALE_FACTOR must be set before QApplication is constructed.
        CTest registers one process per ratio (see CMakeLists.txt).
     */
    const QByteArray scaleFactor = qgetenv("QWT_TEST_SCALE_FACTOR");
    if (!scaleFactor.isEmpty())
        qputenv("QT_SCALE_FACTOR", scaleFactor);

    QApplication app(argc, argv);

    TestAxisCorner test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_axis_corner.moc"
