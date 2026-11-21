/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "test_figure_axis_align.h"
// Qt
#include <QApplication>
#include <QLayout>
#include <QTest>
#include <QWidget>
// Qwt
#include "qwt_axis.h"
#include "qwt_figure.h"
#include "qwt_figure_grid.h"
#include "qwt_plot.h"
#include "qwt_plot_layout.h"

//----------------------------------------------------
// Helpers
//----------------------------------------------------

/**
 * @brief Apply a fixed pixel geometry to the figure and run its layout
 * @details The layout pass also refreshes the plot area alignment of the figure.
 */
void TestFigureAxisAlign::layoutFigure(QwtFigure* figure, int width, int height)
{
    QVERIFY(figure);
    figure->resize(width, height);

    QLayout* lay = figure->layout();
    QVERIFY(lay);
    lay->setGeometry(QRect(0, 0, width, height));

    // Drain the layout requests the plots posted while they were laid out
    QTest::qWait(10);
}

namespace
{
/**
 * @brief Create a plot with deterministic tick labels
 * @param yMin/yMax Range of the left axis
 * @param xMin/xMax Range of the bottom axis
 * @param title Plot title, a '\n' makes it two lines
 * @param xTitle Title of the bottom axis, a '\n' makes it two lines
 * @return The plot, not owned by anything
 */
QwtPlot* createPlot(double yMin,
                    double yMax,
                    double xMin,
                    double xMax,
                    const QString& title  = QString(),
                    const QString& xTitle = QString())
{
    QwtPlot* plot = new QwtPlot;
    plot->setCanvasBackground(Qt::white);
    plot->setAxisScale(QwtAxis::XBottom, xMin, xMax);
    plot->setAxisScale(QwtAxis::YLeft, yMin, yMax);
    if (!title.isEmpty())
        plot->setTitle(title);
    if (!xTitle.isEmpty())
        plot->setAxisTitle(QwtAxis::XBottom, xTitle);
    plot->updateAxes();
    return plot;
}

//! Canvas rectangle in the coordinate system of the figure
QRect canvasRect(QwtFigure* figure, QwtPlot* plot)
{
    QWidget* canvas = plot->canvas();
    if (!canvas)
        return QRect();
    return QRect(canvas->mapTo(figure, QPoint(0, 0)), canvas->size());
}

//! Canvas rectangle in the coordinate system of its plot widget
QRect canvasLocalRect(QwtPlot* plot)
{
    QWidget* canvas = plot->canvas();
    if (!canvas)
        return QRect();
    return QRect(canvas->pos(), canvas->size());
}

//! Left inset of the plot area (distance between the plot widget and its canvas)
int canvasInsetLeft(QwtPlot* plot)
{
    return canvasLocalRect(plot).left();
}

//! Natural canvas inset of a side (the layout without any alignment applied)
int naturalCanvasInset(QwtPlot* plot, int axisPos)
{
    QwtPlotLayout* lay = plot->plotLayout();
    if (!lay)
        return -1;
    lay->activate(plot, plot->contentsRect());
    const QRectF canvas   = lay->naturalCanvasRect();
    const QRectF plotRect = plot->contentsRect();
    if (!canvas.isValid())
        return -1;
    switch (axisPos) {
    case QwtAxis::YLeft:
        return qRound(canvas.left() - plotRect.left());
    case QwtAxis::YRight:
        return qRound(plotRect.right() - canvas.right());
    case QwtAxis::XTop:
        return qRound(canvas.top() - plotRect.top());
    case QwtAxis::XBottom:
        return qRound(plotRect.bottom() - canvas.bottom());
    default:
        return -1;
    }
}
}  // namespace

//----------------------------------------------------
// QwtPlotLayout primitive
//----------------------------------------------------

/**
 * @brief The minimum canvas inset grows the canvas and is a no-op when smaller
 */
void TestFigureAxisAlign::testNaturalCanvasInsets()
{
    QwtPlot plot;
    plot.resize(400, 300);
    plot.setAxisScale(QwtAxis::YLeft, 0.0, 1.0);
    plot.setAxisScale(QwtAxis::XBottom, 0.0, 1.0);
    plot.updateAxes();

    QwtPlotLayout* lay = plot.plotLayout();
    QVERIFY(lay);

    lay->resetMinCanvasInsets();
    lay->activate(&plot, plot.contentsRect());

    const QRectF natural = lay->naturalCanvasRect();
    QVERIFY(natural.isValid());
    QCOMPARE(lay->canvasRect(), natural);

    const int naturalInset = naturalCanvasInset(&plot, QwtAxis::YLeft);
    QVERIFY(naturalInset > 0);

    // An inset larger than the natural one grows the canvas inward on that side
    const int largerInset = naturalInset + 25;
    lay->setMinCanvasInset(QwtAxis::YLeft, largerInset);
    QCOMPARE(lay->minCanvasInset(QwtAxis::YLeft), qreal(largerInset));

    lay->activate(&plot, plot.contentsRect());
    QCOMPARE(qRound(lay->canvasRect().left()), qRound(natural.left()) + 25);

    // An inset smaller than the natural one has no effect
    lay->setMinCanvasInset(QwtAxis::YLeft, 1.0);
    lay->activate(&plot, plot.contentsRect());
    QCOMPARE(qRound(lay->canvasRect().left()), qRound(natural.left()));

    // A side without an inset keeps its natural rect
    QCOMPARE(lay->minCanvasInset(QwtAxis::YRight), 0.0);
    QCOMPARE(qRound(lay->canvasRect().right()), qRound(natural.right()));

    lay->resetMinCanvasInsets();
    QCOMPARE(lay->minCanvasInset(QwtAxis::YLeft), 0.0);
    lay->activate(&plot, plot.contentsRect());
    QCOMPARE(lay->canvasRect(), natural);
}

//----------------------------------------------------
// Retained grid alignment
//----------------------------------------------------

/**
 * @brief Members of a column share the left/right edges, members of a row share the top/bottom edges
 */
void TestFigureAxisAlign::testGridColumnsAndRowsAlign()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(2, 2));
    QVERIFY(figure.isGridAxisAlignment());

    // Column 0: the two plots have very different y tick label widths
    QwtPlot* p00 = createPlot(0.0, 1.0, 0.0, 1.0, QStringLiteral("row 0, column 0"));
    QwtPlot* p10 = createPlot(0.0, 1000000.0, 0.0, 1.0, QStringLiteral("row 1, column 0"));

    // Row 0: a two line title shifts the plot area of the right member down
    QwtPlot* p01 = createPlot(0.0, 1.0, 0.0, 1.0, QStringLiteral("row 0, column 1\nsecond line"));

    // Row 1: a two line x axis title shifts the plot area of the left member up
    QwtPlot* p11 = createPlot(0.0, 1.0, 0.0, 1.0, QStringLiteral("row 1, column 1"));
    p11->setAxisTitle(QwtAxis::XBottom, QStringLiteral("time [s]\nsecond line"));

    figure.addAxes(p00, 0, 0);
    figure.addAxes(p01, 0, 1);
    figure.addAxes(p10, 1, 0);
    figure.addAxes(p11, 1, 1);

    // Without the alignment the plot areas are shifted against each other
    figure.setGridAxisAlignment(false);
    layoutFigure(&figure, 800, 600);

    const QRect n00 = canvasRect(&figure, p00);
    const QRect n01 = canvasRect(&figure, p01);
    const QRect n10 = canvasRect(&figure, p10);
    const QRect n11 = canvasRect(&figure, p11);

    QVERIFY(qAbs(n00.left() - n10.left()) > 2);      // column 0: y label width
    QVERIFY(qAbs(n01.top() - n00.top()) > 2);        // row 0: title height
    QVERIFY(qAbs(n11.bottom() - n10.bottom()) > 2);  // row 1: x axis title height

    // The grid alignment equalizes the insets of the shared edges
    figure.setGridAxisAlignment(true);
    layoutFigure(&figure, 800, 600);

    const QRect a00 = canvasRect(&figure, p00);
    const QRect a01 = canvasRect(&figure, p01);
    const QRect a10 = canvasRect(&figure, p10);
    const QRect a11 = canvasRect(&figure, p11);

    QCOMPARE(a00.left(), a10.left());      // column 0, left edge
    QCOMPARE(a00.right(), a10.right());    // column 0, right edge
    QCOMPARE(a01.left(), a11.left());      // column 1, left edge
    QCOMPARE(a01.right(), a11.right());    // column 1, right edge
    QCOMPARE(a00.top(), a01.top());        // row 0, top edge
    QCOMPARE(a00.bottom(), a01.bottom());  // row 0, bottom edge
    QCOMPARE(a10.top(), a11.top());        // row 1, top edge
    QCOMPARE(a10.bottom(), a11.bottom());  // row 1, bottom edge

    // The alignment works on the plot area only: the member widgets keep their geometry
    QCOMPARE(p00->geometry(), QRect(0, 0, 400, 300));
    QCOMPARE(p01->geometry(), QRect(400, 0, 400, 300));
    QCOMPARE(p10->geometry(), QRect(0, 300, 400, 300));
    QCOMPARE(p11->geometry(), QRect(400, 300, 400, 300));

    // The applied insets are the largest content insets of the shared edges
    QCOMPARE(p00->plotLayout()->minCanvasInset(QwtAxis::YLeft), qreal(naturalCanvasInset(p10, QwtAxis::YLeft)));
}

/**
 * @brief A member spanning several tracks aligns with the first and the last track it covers
 */
void TestFigureAxisAlign::testSpanningMemberAlignment()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(2, 3));

    QwtPlot* spanning = createPlot(0.0, 1.0, 0.0, 1.0, QStringLiteral("row 0, columns 0-2"));
    QwtPlot* left     = createPlot(0.0, 100000.0, 0.0, 1.0, QStringLiteral("row 1, column 0"));
    QwtPlot* middle   = createPlot(0.0, 1.0, 0.0, 1.0, QStringLiteral("row 1, column 1"));
    QwtPlot* right    = createPlot(0.0, 1.0, 0.0, 1.0, QStringLiteral("row 1, column 2"));

    figure.addAxes(spanning, 0, 0, 1, 3);
    figure.addAxes(left, 1, 0);
    figure.addAxes(middle, 1, 1);
    figure.addAxes(right, 1, 2);
    layoutFigure(&figure, 900, 600);

    const QRect spanRect  = canvasRect(&figure, spanning);
    const QRect leftRect  = canvasRect(&figure, left);
    const QRect rightRect = canvasRect(&figure, right);

    // The left edge belongs to the group of column 0, the right edge to the
    // group of column 2 (the last track the member covers).
    QCOMPARE(spanRect.left(), leftRect.left());
    QCOMPARE(spanRect.right(), rightRect.right());
    QVERIFY(spanRect.width() > leftRect.width());
}

/**
 * @brief A manual configuration aligns plots that are not part of a retained grid
 */
void TestFigureAxisAlign::testManualAxisAlignment()
{
    QwtFigure figure;

    QwtPlot* narrow = createPlot(0.0, 1.0, 0.0, 1.0);
    QwtPlot* wide   = createPlot(0.0, 1000000.0, 0.0, 1.0);
    figure.addAxes(narrow, 0.0, 0.0, 0.5, 1.0);
    figure.addAxes(wide, 0.5, 0.0, 0.5, 1.0);
    layoutFigure(&figure, 800, 400);

    QVERIFY(canvasInsetLeft(narrow) != canvasInsetLeft(wide));

    figure.addAxisAlignment({ narrow, wide }, QwtAxis::YLeft);
    QCOMPARE(figure.axisAligmentCount(), 1);

    layoutFigure(&figure, 800, 400);
    QCOMPARE(canvasInsetLeft(narrow), canvasInsetLeft(wide));
    QVERIFY(canvasInsetLeft(narrow) >= naturalCanvasInset(wide, QwtAxis::YLeft));

    // Removing the configuration restores the natural plot area
    QVERIFY(figure.removeAxisAlignment({ narrow, wide }, QwtAxis::YLeft));
    layoutFigure(&figure, 800, 400);
    QVERIFY(canvasInsetLeft(narrow) != canvasInsetLeft(wide));

    delete narrow;
    delete wide;
}

/**
 * @brief Overlapping manual configurations of one side are merged transitively
 */
void TestFigureAxisAlign::testManualAxisAlignmentComponents()
{
    QwtFigure figure;

    QwtPlot* first  = createPlot(0.0, 1.0, 0.0, 1.0);
    QwtPlot* second = createPlot(0.0, 1000.0, 0.0, 1.0);
    QwtPlot* third  = createPlot(0.0, 1000000.0, 0.0, 1.0);

    figure.addAxes(first, 0.0, 0.0, 0.34, 1.0);
    figure.addAxes(second, 0.33, 0.0, 0.34, 1.0);
    figure.addAxes(third, 0.66, 0.0, 0.34, 1.0);

    // Chains: first-second and second-third => all three share one inset
    figure.addAxisAlignment({ first, second }, QwtAxis::YLeft);
    figure.addAxisAlignment({ second, third }, QwtAxis::YLeft);

    layoutFigure(&figure, 900, 400);

    QCOMPARE(canvasInsetLeft(first), canvasInsetLeft(second));
    QCOMPARE(canvasInsetLeft(second), canvasInsetLeft(third));
    QVERIFY(canvasInsetLeft(third) >= naturalCanvasInset(third, QwtAxis::YLeft));

    figure.clearAxisAlignment();
    layoutFigure(&figure, 900, 400);
    QVERIFY(canvasInsetLeft(first) != canvasInsetLeft(third));

    delete first;
    delete second;
    delete third;
}

/**
 * @brief Removing the member with the widest labels shrinks the shared inset again
 */
void TestFigureAxisAlign::testRemoveMemberRealigns()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(3, 1));

    QwtPlot* first = createPlot(0.0, 1.0, 0.0, 1.0);
    QwtPlot* wide  = createPlot(0.0, 1000000.0, 0.0, 1.0);
    QwtPlot* third = createPlot(0.0, 1.0, 0.0, 1.0);

    figure.addAxes(first, 0, 0);
    figure.addAxes(wide, 1, 0);
    figure.addAxes(third, 2, 0);
    layoutFigure(&figure, 400, 600);

    QCOMPARE(canvasInsetLeft(first), canvasInsetLeft(third));
    const int wideInset = canvasInsetLeft(first);

    // Removing the widest member has to shrink the inset of the remaining ones
    figure.removeWidget(wide);
    delete wide;
    layoutFigure(&figure, 400, 600);

    QCOMPARE(canvasInsetLeft(first), canvasInsetLeft(third));
    QVERIFY(canvasInsetLeft(first) < wideInset);

    // Re-adding a wide member grows the shared inset again
    QwtPlot* wideAgain = createPlot(0.0, 500000.0, 0.0, 1.0);
    figure.addAxes(wideAgain, 1, 0);
    layoutFigure(&figure, 400, 600);

    QCOMPARE(canvasInsetLeft(first), canvasInsetLeft(wideAgain));
    QVERIFY(canvasInsetLeft(first) >= naturalCanvasInset(wideAgain, QwtAxis::YLeft));

    figure.removeWidget(first);
    figure.removeWidget(third);
    figure.removeWidget(wideAgain);
    delete first;
    delete third;
    delete wideAgain;
}

/**
 * @brief Leaving the grid restores the natural plot areas
 */
void TestFigureAxisAlign::testDisableGridRestoresNatural()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(2, 1));

    QwtPlot* narrow = createPlot(0.0, 1.0, 0.0, 1.0);
    QwtPlot* wide   = createPlot(0.0, 1000000.0, 0.0, 1.0);
    figure.addAxes(narrow, 0, 0);
    figure.addAxes(wide, 1, 0);
    layoutFigure(&figure, 400, 600);

    QCOMPARE(canvasInsetLeft(narrow), canvasInsetLeft(wide));
    QVERIFY(canvasInsetLeft(narrow) > naturalCanvasInset(narrow, QwtAxis::YLeft));

    // After leaving the grid the plots return to their natural plot areas
    QVERIFY(figure.disableGrid());
    QVERIFY(!figure.isGridEnabled());
    layoutFigure(&figure, 400, 600);

    QCOMPARE(canvasInsetLeft(narrow), naturalCanvasInset(narrow, QwtAxis::YLeft));
    QCOMPARE(canvasInsetLeft(wide), naturalCanvasInset(wide, QwtAxis::YLeft));
    QVERIFY(canvasInsetLeft(narrow) != canvasInsetLeft(wide));

    delete narrow;
    delete wide;
}

/**
 * @brief The outermost axis of a host with parasite axes aligns with the axis of a plain plot
 */
void TestFigureAxisAlign::testParasiteAxisAlignment()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(2, 1));

    QwtPlot* host  = createPlot(0.0, 1.0, 0.0, 1.0);
    QwtPlot* plain = createPlot(0.0, 1.0, 0.0, 1.0);
    plain->enableAxis(QwtAxis::YRight, true);
    plain->setAxisScale(QwtAxis::YRight, 0.0, 1.0);
    plain->updateAxes();

    figure.addAxes(host, 0, 0);
    figure.addAxes(plain, 1, 0);

    // The parasite axis adds a second band on the right of the host
    QwtPlot* parasite = figure.createParasiteAxes(host, QwtAxis::YRight);
    QVERIFY(parasite);
    parasite->setAxisScale(QwtAxis::YRight, 0.0, 250000.0);
    parasite->setAxisTitle(QwtAxis::YRight, QStringLiteral("secondary"));
    parasite->replot();

    figure.setGridAxisAlignment(false);
    layoutFigure(&figure, 500, 600);
    QVERIFY(qAbs(canvasLocalRect(host).right() - canvasLocalRect(plain).right()) > 2);

    figure.setGridAxisAlignment(true);
    layoutFigure(&figure, 500, 600);
    QCOMPARE(canvasLocalRect(host).right(), canvasLocalRect(plain).right());
}

/**
 * @brief A changed axis scale changes the label width and the alignment follows
 */
void TestFigureAxisAlign::testAlignmentFollowsAxisScaleChange()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(2, 1));

    QwtPlot* first  = createPlot(0.0, 1.0, 0.0, 1.0);
    QwtPlot* second = createPlot(0.0, 1000.0, 0.0, 1.0);
    figure.addAxes(first, 0, 0);
    figure.addAxes(second, 1, 0);
    layoutFigure(&figure, 400, 600);

    QCOMPARE(canvasInsetLeft(first), canvasInsetLeft(second));
    const int narrowInset = canvasInsetLeft(first);

    // A very wide range on the first plot makes it the widest member of the column
    first->setAxisScale(QwtAxis::YLeft, -5000000.0, 5000000.0);
    first->updateAxes();
    layoutFigure(&figure, 400, 600);

    QCOMPARE(canvasInsetLeft(first), canvasInsetLeft(second));
    QVERIFY(canvasInsetLeft(first) > narrowInset);

    delete first;
    delete second;
}

/**
 * @brief A hidden member is not aligned and its insets are dropped
 */
void TestFigureAxisAlign::testHiddenMemberIsSkipped()
{
    QwtFigure figure;
    QVERIFY(figure.enableGrid(2, 1));

    QwtPlot* visible = createPlot(0.0, 1.0, 0.0, 1.0);
    QwtPlot* hidden  = createPlot(0.0, 1000000.0, 0.0, 1.0);
    figure.addAxes(visible, 0, 0);
    figure.addAxes(hidden, 1, 0);
    layoutFigure(&figure, 400, 600);

    QCOMPARE(canvasInsetLeft(visible), canvasInsetLeft(hidden));
    QVERIFY(canvasInsetLeft(visible) > naturalCanvasInset(visible, QwtAxis::YLeft));

    hidden->hide();
    layoutFigure(&figure, 400, 600);

    QCOMPARE(canvasInsetLeft(visible), naturalCanvasInset(visible, QwtAxis::YLeft));
    QCOMPARE(hidden->plotLayout()->minCanvasInset(QwtAxis::YLeft), 0.0);

    // Showing the member again puts it back into the group of the column
    hidden->show();
    layoutFigure(&figure, 400, 600);

    QCOMPARE(canvasInsetLeft(visible), canvasInsetLeft(hidden));
    QVERIFY(canvasInsetLeft(visible) > naturalCanvasInset(visible, QwtAxis::YLeft));

    delete visible;
    delete hidden;
}

QTEST_MAIN(TestFigureAxisAlign)
#include "test_figure_axis_align.moc"
