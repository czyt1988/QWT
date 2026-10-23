#include "test_axis_corner.h"

#include <qwt_axis.h>
#include <qwt_color_map.h>
#include <qwt_interval.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_renderer.h>
#include <qwt_scale_div.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_map.h>
#include <qwt_scale_widget.h>

#include <QApplication>
#include <QColor>
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
    Device coordinate of the canvas side ( near ) edge of a backbone band,
    mirroring the rule of QwtScaleDraw: a left/top band grows away from the
    canvas, so its near edge is the last device pixel of the logical pixel of
    the scale position, while a right/bottom band grows towards the canvas
    from the first one.
 */
int nearBandEdge(const QImage& img, qreal logicalPos, bool growsAwayFromCanvas)
{
    return growsAwayFromCanvas ? toDevice(img, logicalPos + 1) - 1 : toDevice(img, logicalPos);
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
QImage composePlot(QwtPlot& plot, const QList<QWidget*>& extraWidgets = QList<QWidget*>(),
    bool runLayout = true)
{
    // grab() runs the layout of the still hidden widget tree, so all
    // geometries are current when the widgets are rendered below. Callers
    // that changed paint state after the last layout pass can skip it, so
    // the rendering keeps the geometries of that pass.
    if (runLayout)
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
    return QPoint(nearBandEdge(img, yCol, true), nearBandEdge(img, xRow, false));
}

/*
    The plot of the inside tick report: all four axes with their ticks
    inside the canvas, a pen width that is not a whole device pixel and
    explicit major ticks clear of the canvas corners, so every tick can be
    measured without being clipped.
 */
void setupInsideTicksPlot(QwtPlot& plot, qreal penWidth)
{
    plot.plotLayout()->setAlignCanvasToScales(true);
    plot.setAxisVisible(QwtAxis::XTop, true);
    plot.setAxisVisible(QwtAxis::YRight, true);

    QwtPlotCanvas* canvas = new QwtPlotCanvas(&plot);
    canvas->setLineWidth(0);
    plot.setCanvas(canvas);

    QList< double > majors;
    for (int v = 1; v <= 9; v++)
        majors += double(v);

    const QwtAxisId axes[] = { QwtAxis::XBottom, QwtAxis::XTop, QwtAxis::YLeft, QwtAxis::YRight };
    for (QwtAxisId axisId : axes) {
        plot.setAxisTickDirection(axisId, QwtPlot::TickInside);
        plot.axisScaleDraw(axisId)->setPenWidthF(penWidth);
        plot.setAxisScaleDiv(axisId, QwtScaleDiv(0.0, 10.0, QList< double >(), QList< double >(), majors));
    }

    plot.resize(520, 400);
    plot.replot();
}

/*
    Length of the longest dark run on a pixel column ( vertical = true )
    or row ( vertical = false ) around center. Measuring the run instead
    of a single pixel keeps the result independent of where exactly the
    tick landed.
 */
int longestDarkRun(const QImage& img, int fixed, int center, bool vertical, int radius = 8)
{
    int best = 0;
    int run  = 0;

    for (int p = center - radius; p <= center + radius; p++) {
        const bool dark = vertical ? isDark(img, fixed, p) : isDark(img, p, fixed);
        if (dark) {
            run++;
            best = qMax(best, run);
        } else {
            run = 0;
        }
    }

    return best;
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
        QVERIFY2(qAbs(yCol - (toDevice(img, cr.left()) - 1)) <= 8 * dpr,
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
    const int yCol = toDevice(img, cr.left()) - 1;
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
    const int yCol = toDevice(img, cr.left()) - 1;
    QVERIFY(isDark(img, yCol, toDevice(img, cr.bottom())));
    QVERIFY(!isDark(img, toDevice(img, cr.left()), toDevice(img, cr.bottom())));

    // the tick at the scale minimum of xBottom sits at the canvas left column:
    // it may darken the backbone row but not the row above it
    QVERIFY(isDark(img, toDevice(img, cr.left()), toDevice(img, cr.bottom() + 1)));
    QVERIFY(!isDark(img, toDevice(img, cr.left() + 3), toDevice(img, cr.bottom())));

    // the border ticks snap onto the perpendicular backbone:
    // the x minimum tick hangs at the y backbone column,
    // the y minimum tick sits on the x backbone row
    QVERIFY(isDark(img, yCol, toDevice(img, cr.bottom() + 2)));
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
    const int yCol = toDevice(img, cr.left()) - 1;
    QVERIFY(isDark(img, yCol, toDevice(img, cr.top())));
    QVERIFY(!isDark(img, yCol, toDevice(img, cr.top()) - pw));

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
            const int inner = toDevice(img, cr.left()) - 1;
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
    At any pen width the corner square where two perpendicular backbone
    bands meet must be fully covered: every band reaches the far edge of
    its perpendicular neighbour, and all four bands sit flush with the
    canvas edges they frame.
 */
void TestAxisCorner::testCornerSquareAtAnyPenWidth()
{
    const qreal widths[] = { 1.0, 2.0, 3.0 };

    for (qreal w : widths) {
        QwtPlot plot;
        plot.plotLayout()->setAlignCanvasToScales(true);
        plot.setAxisVisible(QwtAxis::XTop, true);
        plot.setAxisVisible(QwtAxis::YRight, true);

        QwtPlotCanvas* canvas = new QwtPlotCanvas(&plot);
        canvas->setLineWidth(0);
        plot.setCanvas(canvas);

        const QwtAxisId axes[] = { QwtAxis::XBottom, QwtAxis::XTop, QwtAxis::YLeft, QwtAxis::YRight };
        for (QwtAxisId axisId : axes) {
            QwtScaleDraw* sd = plot.axisScaleDraw(axisId);
            sd->setPenWidthF(w);
            sd->enableComponent(QwtAbstractScaleDraw::Ticks, false);
            sd->enableComponent(QwtAbstractScaleDraw::Labels, false);
        }

        plot.resize(480, 360);
        plot.replot();

        const QImage img    = composePlot(plot);
        const QRect cr      = canvasRectInPlot(plot);
        const int pw        = devicePenWidth(w);
        const QString ctx   = QString("penWidthF(%1) dpr(%2)").arg(w).arg(img.devicePixelRatio());

        // near edges of the four bands, flush with the canvas
        const int leftCol   = toDevice(img, cr.left()) - 1;
        const int topRow    = toDevice(img, cr.top()) - 1;
        const int rightCol  = toDevice(img, cr.right() + 1);
        const int bottomRow = toDevice(img, cr.bottom() + 1);

        struct Square
        {
            int c0, c1, r0, r1;
            const char* name;
        };
        const Square squares[] = {
            { leftCol - pw + 1, leftCol, topRow - pw + 1, topRow, "top-left" },
            { rightCol, rightCol + pw - 1, topRow - pw + 1, topRow, "top-right" },
            { leftCol - pw + 1, leftCol, bottomRow, bottomRow + pw - 1, "bottom-left" },
            { rightCol, rightCol + pw - 1, bottomRow, bottomRow + pw - 1, "bottom-right" },
        };

        for (const Square& sq : squares) {
            for (int r = sq.r0; r <= sq.r1; r++) {
                for (int c = sq.c0; c <= sq.c1; c++) {
                    QVERIFY2(isDark(img, c, r),
                        qPrintable(ctx + QString(": %1 corner open at (%2,%3)")
                            .arg(sq.name).arg(c).arg(r)));
                }
            }
        }

        // nothing painted outside of the frame: the bands stay flush
        const int midRow = (topRow + bottomRow) / 2;
        const int midCol = (leftCol + rightCol) / 2;
        QVERIFY2(!isDark(img, leftCol - pw, midRow), qPrintable(ctx + ": left band too wide"));
        QVERIFY2(!isDark(img, rightCol + pw, midRow), qPrintable(ctx + ": right band too wide"));
        QVERIFY2(!isDark(img, midCol, topRow - pw), qPrintable(ctx + ": top band too wide"));
        QVERIFY2(!isDark(img, midCol, bottomRow + pw), qPrintable(ctx + ": bottom band too wide"));

        /*
            A backbone must not stick out beyond the perpendicular band at
            the corners: the scale span edge and the band edge fall into
            different device pixels of the same logical pixel at device
            pixel ratios above one, and the miter - not the span - defines
            where the backbone ends.
         */
        QVERIFY2(!isDark(img, leftCol - pw, topRow), qPrintable(ctx + ": top band sticks out left of the y band"));
        QVERIFY2(!isDark(img, leftCol - pw, bottomRow), qPrintable(ctx + ": bottom band sticks out left of the y band"));
        QVERIFY2(!isDark(img, rightCol + pw, topRow), qPrintable(ctx + ": top band sticks out right of the y band"));
        QVERIFY2(!isDark(img, rightCol + pw, bottomRow), qPrintable(ctx + ": bottom band sticks out right of the y band"));
        QVERIFY2(!isDark(img, leftCol, topRow - pw), qPrintable(ctx + ": left band sticks out above the x band"));
        QVERIFY2(!isDark(img, rightCol, topRow - pw), qPrintable(ctx + ": right band sticks out above the x band"));
        QVERIFY2(!isDark(img, leftCol, bottomRow + pw), qPrintable(ctx + ": left band sticks out below the x band"));
        QVERIFY2(!isDark(img, rightCol, bottomRow + pw), qPrintable(ctx + ": right band sticks out below the x band"));
    }
}

/*
    A tick at the scale border snaps onto the perpendicular backbone and
    must sit exactly on the span of that band: for even pen widths the
    band of a top/left axis grows away from the canvas, and centering the
    tick thickness on the near edge instead would leave it off the band by
    a device pixel.
 */
void TestAxisCorner::testBorderTickSitsOnBand()
{
    const qreal widths[] = { 1.0, 2.0, 3.0 };

    for (qreal w : widths) {
        QwtPlot plot;
        plot.plotLayout()->setAlignCanvasToScales(true);
        plot.setAxisVisible(QwtAxis::XTop, true);
        plot.setAxisVisible(QwtAxis::YRight, true);

        QwtPlotCanvas* canvas = new QwtPlotCanvas(&plot);
        canvas->setLineWidth(0);
        plot.setCanvas(canvas);

        const QwtAxisId axes[] = { QwtAxis::XBottom, QwtAxis::XTop, QwtAxis::YLeft, QwtAxis::YRight };
        for (QwtAxisId axisId : axes)
            plot.axisScaleDraw(axisId)->setPenWidthF(w);

        plot.resize(480, 360);
        plot.replot();

        const QImage img  = composePlot(plot);
        const QRect cr    = canvasRectInPlot(plot);
        const int pw      = devicePenWidth(w);
        const QString ctx = QString("penWidthF(%1) dpr(%2)").arg(w).arg(img.devicePixelRatio());

        const int leftCol   = toDevice(img, cr.left()) - 1;
        const int topRow    = toDevice(img, cr.top()) - 1;
        const int rightCol  = toDevice(img, cr.right() + 1);
        const int bottomRow = toDevice(img, cr.bottom() + 1);

        // a column left of the y band carrying the horizontal border ticks
        const int tickCol = leftCol - pw - 3;
        // a row below the x band carrying the vertical border ticks
        const int tickRow = bottomRow + pw + 3;

        // the y border ticks sit on the rows of the x bands
        for (int r = topRow - pw + 1; r <= topRow; r++) {
            QVERIFY2(isDark(img, tickCol, r),
                qPrintable(ctx + QString(": y border tick misses the top band row %1").arg(r)));
        }
        QVERIFY2(!isDark(img, tickCol, topRow - pw), qPrintable(ctx + ": y border tick above the top band"));
        QVERIFY2(!isDark(img, tickCol, topRow + 1), qPrintable(ctx + ": y border tick below the top band"));
        for (int r = bottomRow; r <= bottomRow + pw - 1; r++) {
            QVERIFY2(isDark(img, tickCol, r),
                qPrintable(ctx + QString(": y border tick misses the bottom band row %1").arg(r)));
        }
        QVERIFY2(!isDark(img, tickCol, bottomRow - 1), qPrintable(ctx + ": y border tick above the bottom band"));
        QVERIFY2(!isDark(img, tickCol, bottomRow + pw), qPrintable(ctx + ": y border tick below the bottom band"));

        // the x border ticks sit on the columns of the y bands
        for (int c = leftCol - pw + 1; c <= leftCol; c++) {
            QVERIFY2(isDark(img, c, tickRow),
                qPrintable(ctx + QString(": x border tick misses the left band column %1").arg(c)));
        }
        QVERIFY2(!isDark(img, leftCol - pw, tickRow), qPrintable(ctx + ": x border tick left of the left band"));
        QVERIFY2(!isDark(img, leftCol + 1, tickRow), qPrintable(ctx + ": x border tick right of the left band"));
        for (int c = rightCol; c <= rightCol + pw - 1; c++) {
            QVERIFY2(isDark(img, c, tickRow),
                qPrintable(ctx + QString(": x border tick misses the right band column %1").arg(c)));
        }
        QVERIFY2(!isDark(img, rightCol - 1, tickRow), qPrintable(ctx + ": x border tick left of the right band"));
        QVERIFY2(!isDark(img, rightCol + pw, tickRow), qPrintable(ctx + ": x border tick right of the right band"));
    }
}

/*
    Inside ticks must be rasterized with the same device pixel rules as the
    backbone and the outside ticks: every tick is qMax(1, round(penWidthF))
    device pixels thick at any device pixel ratio, and ticks at different
    subpixel positions do not end up with different thicknesses.

    Before the fix the inside ticks were stroked with a QPen in logical
    coordinates: at 200% a pen width of 2 painted 4 device pixels while the
    backbone painted 2, and a fractional width like 1.5 rasterized to 1 or
    2 pixels depending on the position of the tick.
 */
void TestAxisCorner::testInsideTicksShareBackboneWidth()
{
    const qreal widths[] = { 0.0, 1.0, 1.5, 2.0, 3.0 };

    for (qreal w : widths) {
        QwtPlot plot;
        setupInsideTicksPlot(plot, w);

        const QImage img  = composePlot(plot);
        const QRect cr    = canvasRectInPlot(plot);
        const int pw      = devicePenWidth(w);
        const QString ctx = QString("penWidthF(%1) dpr(%2)").arg(w).arg(img.devicePixelRatio());

        // sample a column/row a few pixels inside the canvas edge: all
        // inside ticks are longer than that and cross it
        struct Probe
        {
            QwtAxisId axisId;
            int sample;     // device column for vertical ticks, row for horizontal ticks
            bool vertical;  // measure the tick thickness along a column
            const char* name;
        };

        const Probe probes[] = {
            { QwtAxis::YLeft, toDevice(img, cr.left() + 2), true, "YLeft" },
            { QwtAxis::YRight, toDevice(img, cr.right() - 2), true, "YRight" },
            { QwtAxis::XTop, toDevice(img, cr.top() + 2), false, "XTop" },
            { QwtAxis::XBottom, toDevice(img, cr.bottom() - 2), false, "XBottom" },
        };

        for (const Probe& probe : probes) {
            const QwtScaleMap map       = plot.canvasMap(probe.axisId);
            const QList< double > ticks = plot.axisScaleDiv(probe.axisId).ticks(QwtScaleDiv::MajorTick);
            QVERIFY2(ticks.count() == 9, qPrintable(ctx + QString(": unexpected major tick count on %1").arg(probe.name)));

            for (double value : ticks) {
                const qreal localPos = map.transform(value);
                const int pos = probe.vertical ? toDevice(img, cr.top() + localPos) : toDevice(img, cr.left() + localPos);
                const int thickness = longestDarkRun(img, probe.sample, pos, probe.vertical);

                QVERIFY2(thickness == pw,
                         qPrintable(ctx
                                    + QString(": %1 tick at %2 is %3 device pixels thick, expected %4")
                                          .arg(probe.name)
                                          .arg(value)
                                          .arg(thickness)
                                          .arg(pw)));
            }
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

    /*
        The parasite band is stacked outside of the host band. Its y backbone
        still reaches the host x backbone row, but the host x backbone must
        not stretch out to the parasite band: extending to the outermost band
        would run the backbone through the whole axis stack and close the
        corners of unrelated bands ( the parasite example drew a box around
        the plot when both directions had stacked bands ).
     */
    const QPoint band = bandCorner(img, host,
        parasite->axisWidget(QwtAxis::YLeft), host.axisWidget(QwtAxis::XBottom));
    QVERIFY2(band.x() < hostBand.x(), "the parasite band must sit outside of the host band");
    QCOMPARE(band.y(), hostBand.y());

    const int dpr = qCeil(img.devicePixelRatio());
    QVERIFY2(band.x() + 5 * dpr < hostBand.x(),
        "the fixture needs a gap between the parasite and host bands");

    QVERIFY2(isDark(img, band.x(), hostBand.y()),
        "the parasite y backbone must reach the host x backbone row");
    QVERIFY2(!isDark(img, band.x() + 5 * dpr, hostBand.y()),
        "the host x backbone must not stretch out to the parasite band");
    QVERIFY2(!isDark(img, hostBand.x() - 5 * dpr, hostBand.y()),
        "the host x backbone must end at the host corner");
}

/*
    The corner reservation grows the axis widget rectangle at the scale
    ends so the backbone miter is not clipped. It must not grow the geometry
    derived from the contents rectangle: the color bar, the inside title and
    scaleRect() describe the scale and keep its extent.
 */
void TestAxisCorner::testReservationDoesNotExtendScaleGeometry()
{
    QwtPlot plot;
    setupTicksInsideBorderPlot(plot);

    QwtScaleWidget* yw = plot.axisWidget(QwtAxis::YLeft);
    yw->setColorBarEnabled(true);
    yw->setColorBarWidth(10);
    yw->setColorMap(QwtInterval(0.0, 1.0),
        new QwtLinearColorMap(QColor(Qt::black), QColor(Qt::blue)));

    plot.replot();

    /*
        Apply the layout once (including the corner reservation), then hide
        the x backbone for the composition: the reserved corner row of the
        y widget overlaps the x backbone, and the assertion below needs that
        row to show the background instead.
     */
    plot.grab();
    plot.axisScaleDraw(QwtAxis::XBottom)->enableComponent(QwtAbstractScaleDraw::Backbone, false);

    const QImage img = composePlot(plot, QList<QWidget*>(), false);

    // precondition: the bottom corner reservation of yLeft is active
    // (XBottom is visible, XTop is not), so the widget is taller than the
    // scale rect the layout allocated for it
    const QRect layoutRect = plot.plotLayout()->scaleRect(QwtAxis::YLeft).toRect();
    QVERIFY2(yw->geometry().bottom() > layoutRect.bottom(),
        "the corner reservation must be active for this test");

    const QwtScaleDraw* sd = yw->scaleDraw();
    const QPoint origin    = yw->mapTo(&plot, QPoint(0, 0));
    const qreal scaleStart = origin.y() + sd->pos().y();
    const qreal scaleEnd   = scaleStart + sd->length();

    // scaleRect() keeps the scale extent at both ends
    const QRect sr = yw->scaleRect();
    QCOMPARE(sr.top(), qRound(sd->pos().y()));
    QCOMPARE(sr.bottom() + 1, qRound(sd->pos().y() + sd->length()));

    // the drawn color bar ends with the scale as well: its last row is
    // inside the scale, the reserved corner row below it stays untouched
    const QRectF bar = yw->colorBarRect(yw->contentsRect());
    const int col    = toDevice(img, origin.x() + bar.center().x());

    QVERIFY2(isDark(img, col, toDevice(img, scaleEnd)),
        "the color bar must reach the end of the scale");
    QVERIFY2(!isDark(img, col, toDevice(img, scaleEnd + 1)),
        "the color bar must not grow into the reserved corner pixels");
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
