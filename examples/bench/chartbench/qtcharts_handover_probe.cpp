/*****************************************************************************
 * chartbench - Qt Charts handover attribution probe (standalone)
 *
 * The harness measures the handover step as a whole (building the QList<QPointF>
 * that QXYSeries::replace() requires, plus the replace() call and the axis
 * range updates). This probe splits that step so that the report can say which
 * part is expensive.
 *
 * Result on the reference machine (Qt 6.7.3, MSVC 2019, Release):
 *
 *   n=  100000  list=  0.734  replace= 48.650  setRange= 25.545  total= 74.930 ms
 *   n= 1000000  list=  6.616  replace=420.678  setRange=269.912  total=697.206 ms
 *
 * Note that replace() costs 1.9 ms for the same data when the chart is not in a
 * live view - the price is the item/scene update path, not the data copy.
 *
 * Build (MSVC, from a Developer Command Prompt, adjust the Qt path):
 *   cl /nologo /std:c++17 /Zc:__cplusplus /permissive- /O2 /EHsc /MD /DNDEBUG  *      qtcharts_handover_probe.cpp  *      /I <Qt>/include /I <Qt>/include/QtCore /I <Qt>/include/QtGui  *      /I <Qt>/include/QtWidgets /I <Qt>/include/QtCharts  *      /link /LIBPATH:<Qt>/lib Qt6Charts.lib Qt6Widgets.lib Qt6Gui.lib Qt6Core.lib
 *   set QT_QPA_PLATFORM=offscreen && qtcharts_handover_probe.exe
 *****************************************************************************/

// same operations, to attribute the handover cost measured by the harness.
#include <QApplication>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QElapsedTimer>
#include <cstdio>
#include <cmath>
#include <vector>

static void bench(int n, int repeats)
{
    std::vector<double> xs(n), ys(n);
    for (int i = 0; i < n; ++i) { xs[i] = i * 0.001; ys[i] = std::sin(i * 0.001); }

    QChart* chart = new QChart();
    chart->legend()->hide();
    QValueAxis* ax = new QValueAxis();
    QValueAxis* ay = new QValueAxis();
    ax->setRange(0, xs.back());
    ay->setRange(-1.5, 1.5);
    chart->addAxis(ax, Qt::AlignBottom);
    chart->addAxis(ay, Qt::AlignLeft);

    QLineSeries* series = new QLineSeries();
    chart->addSeries(series);
    series->attachAxis(ax);
    series->attachAxis(ay);

    QChartView view(chart);
    view.resize(1000, 700);
    view.show();
    QApplication::processEvents();

    double listMs = 0, replaceMs = 0, rangeMs = 0;
    QElapsedTimer timer;
    timer.start();
    for (int r = 0; r < repeats; ++r) {
        const double offset = r * 1.0;

        QElapsedTimer t1; t1.start();
        QList<QPointF> points;
        points.reserve(n);
        for (int i = 0; i < n; ++i) points.append(QPointF(xs[i], ys[i]));
        listMs += t1.nsecsElapsed() / 1e6;

        QElapsedTimer t2; t2.start();
        series->replace(points);
        replaceMs += t2.nsecsElapsed() / 1e6;

        QElapsedTimer t3; t3.start();
        ax->setRange(offset, offset + xs.back());
        ay->setRange(-1.5, 1.5);
        rangeMs += t3.nsecsElapsed() / 1e6;
    }
    const double total = timer.nsecsElapsed() / 1e6;
    std::printf("n=%8d  list=%8.3f  replace=%9.3f  setRange=%9.3f  total=%9.3f ms\n",
                n, listMs / repeats, replaceMs / repeats, rangeMs / repeats, total / repeats);
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    for (int n : {100000, 1000000}) bench(n, 5);
    return 0;
}
