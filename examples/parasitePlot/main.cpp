/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_scale_widget.h"
#include "qwt_figure.h"

// 生成示例数据
QVector< QPointF > generateSampleData(int count = 100, double amplitude = 1.0, double frequency = 1.0)
{
    QVector< QPointF > data;
    for (int i = 0; i < count; ++i) {
        double x = i * 0.1;
        double y = amplitude * sin(frequency * x);
        data.append(QPointF(x, y));
    }
    return data;
}

// 设置绘图样式
void setupPlotStyle(QwtPlot* plot, const QString& title, const QColor& color)
{
    plot->setTitle(title);
    plot->setCanvasBackground(Qt::white);

    // 添加网格
    QwtPlotGrid* grid = new QwtPlotGrid();
    grid->setPen(Qt::gray, 0.0, Qt::DotLine);
    grid->attach(plot);

    // 设置坐标轴
    plot->setAxisTitle(QwtPlot::xBottom, "X Axis");
    plot->setAxisTitle(QwtPlot::yLeft, "Y Axis");

    // 设置曲线颜色
    auto itemlist = plot->itemList();
    if (itemlist.size() > 0) {
        for (int i = 0; i < itemlist.size(); ++i) {
            if (QwtPlotCurve* curve = dynamic_cast< QwtPlotCurve* >(plot->itemList()[ i ])) {
                curve->setPen(color, 2);
            }
        }
    }

    plot->replot();
}

/**
 * @brief 创建一个寄生轴
 * @param figure
 */
void createParasitePlot(QwtFigure* figure);

// 创建示例应用程序
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 创建主窗口
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Qwt Parasite Plot Example");
    mainWindow.resize(1200, 800);

    // 创建中央部件
    QWidget* centralWidget  = new QWidget(&mainWindow);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // 创建宿主绘图
    QwtPlot* hostPlot = new QwtPlot(centralWidget);
    //! 设置宿主绘图坐标轴标题
    hostPlot->setTitle("Mult Axes Plot");
    hostPlot->setFooter("footer");
    hostPlot->setAxisTitle(QwtPlot::xBottom, "X1 Axis Bottom");
    hostPlot->setAxisTitle(QwtPlot::yLeft, "Y1 Axis Left");
    hostPlot->setAxisTitle(QwtPlot::xTop, "X1 Axis Top");
    hostPlot->setAxisTitle(QwtPlot::yRight, "Y1 Axis Right");
    hostPlot->enableAxis(QwtPlot::yRight, true);
    hostPlot->enableAxis(QwtPlot::xTop, true);

    //! 给宿主绘图添加网格
    QwtPlotGrid* grid = new QwtPlotGrid();
    grid->setPen(Qt::gray, 0.0, Qt::DotLine);
    grid->attach(hostPlot);

    //! 给宿主绘图添加曲线
    QwtPlotCurve* hostCurve = new QwtPlotCurve("Host Sine Wave");
    hostCurve->setSamples(generateSampleData(100, 1.2, 0.8));
    hostCurve->setPen(QColor(31, 119, 180), 1.5);
    hostCurve->attach(hostPlot);
    hostCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);

    ////////////////////////////////////////////////////////
    //! 添加寄生坐标系
    ////////////////////////////////////////////////////////
    QwtPlot* parasitePlot = hostPlot->createParasitePlot(QwtAxis::YLeft);
    //!  设置寄生轴1坐标的显示和共享的轴
    parasitePlot->enableAxis(QwtAxis::YRight, true);
    parasitePlot->enableAxis(QwtAxis::XTop, true);
    parasitePlot->setParasiteShareAxis(QwtAxis::XBottom);

    //! 宿主坐标轴的其他设置
    parasitePlot->setAxisTitle(QwtAxis::YLeft, "Y2 Left Axis");
    parasitePlot->setAxisTitle(QwtAxis::YRight, "Y2 Right Axis");
    parasitePlot->setAxisTitle(QwtAxis::XTop, "X2 Top Axis");
    //! 用于宿主坐标的曲线
    QColor curColor             = QColor(255, 127, 14);
    QwtPlotCurve* parasiteCurve = new QwtPlotCurve("parasite sine Wave 1");
    parasiteCurve->setSamples(generateSampleData(100, 2000, 2.3));
    parasiteCurve->attach(parasitePlot);
    parasiteCurve->setPen(curColor, 1.5);
    parasiteCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    //! 为了区分宿主轴，给宿主轴的坐标也增加颜色
    parasitePlot->axisWidget(QwtAxis::YLeft)->setScaleColor(curColor);
    parasitePlot->axisWidget(QwtAxis::YRight)->setScaleColor(curColor);
    parasitePlot->axisWidget(QwtAxis::XTop)->setScaleColor(curColor);

    ////////////////////////////////////////////////////////
    //! 添加第二个寄生坐标系
    ////////////////////////////////////////////////////////
    QwtPlot* parasitePlot2 = hostPlot->createParasitePlot(QwtAxis::YLeft);
    //! 设置寄生轴2坐标的显示和共享的轴
    parasitePlot2->enableAxis(QwtAxis::YRight, true);
    parasitePlot2->enableAxis(QwtAxis::XBottom, true);
    parasitePlot2->setParasiteShareAxis(QwtAxis::XTop);

    //! 宿主坐标轴的其他设置
    parasitePlot2->setAxisTitle(QwtAxis::YLeft, "Y3 Left Axis");
    parasitePlot2->setAxisTitle(QwtAxis::YRight, "Y3 Right Axis");
    parasitePlot2->setAxisTitle(QwtAxis::XBottom, "X3 Bottom Axis");
    //! 用于宿主坐标的曲线
    QColor curColor2             = QColor(192, 43, 149);
    QwtPlotCurve* parasiteCurve2 = new QwtPlotCurve("parasite sine Wave 2");
    parasiteCurve2->setSamples(generateSampleData(200, 1000, 4.3));
    parasiteCurve2->attach(parasitePlot2);
    parasiteCurve2->setPen(curColor2, 1);
    parasiteCurve2->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    //! 为了区分宿主轴，给宿主轴的坐标也增加颜色
    parasitePlot2->axisWidget(QwtAxis::YLeft)->setScaleColor(curColor2);
    parasitePlot2->axisWidget(QwtAxis::YRight)->setScaleColor(curColor2);
    parasitePlot2->axisWidget(QwtAxis::XBottom)->setScaleColor(curColor2);

    // 通过invokeMethod把replot投递到消息队列，让其最后执行
    QMetaObject::invokeMethod(hostPlot, &QwtPlot::replot, Qt::QueuedConnection);
    // 添加布局
    mainLayout->addWidget(hostPlot);

    // 设置中央部件
    mainWindow.setCentralWidget(centralWidget);
    mainWindow.show();

    return app.exec();
}
