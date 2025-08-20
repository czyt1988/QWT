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

// 创建示例应用程序
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 创建主窗口
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("QwtFigure Layout Example");
    mainWindow.resize(1200, 800);

    // 创建中央部件
    QWidget* centralWidget  = new QWidget(&mainWindow);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // 创建 QwtFigure
    QwtFigure* figure = new QwtFigure(centralWidget);
    figure->setSizeInches(8, 6);      // 设置图形尺寸为8x6英寸
    figure->setFaceColor(Qt::white);  // 设置背景颜色

    // 示例1: 使用归一化坐标添加绘图
    QwtPlot* plot1       = new QwtPlot();
    QwtPlotCurve* curve1 = new QwtPlotCurve("Sine Wave 1");
    curve1->setSamples(generateSampleData(100, 1.0, 1.0));
    curve1->attach(plot1);
    setupPlotStyle(plot1, "Normalized Coordinates (Top-Left)", Qt::blue);
    figure->addAxes(plot1, 0.0, 0.0, 0.5, 0.3333333);  // 左上角
    qDebug() << "plot1 norm rect =" << figure->axesNormRect(plot1);

    QwtPlot* plot2       = new QwtPlot();
    QwtPlotCurve* curve2 = new QwtPlotCurve("Sine Wave 2");
    curve2->setSamples(generateSampleData(100, 1.5, 2.0));
    curve2->attach(plot2);
    setupPlotStyle(plot2, "Normalized Coordinates (Top-Right)", Qt::red);
    figure->addAxes(plot2, 0.5, 0.0, 0.5, 0.33333333);  // 右上角
    qDebug() << "plot2 norm rect =" << figure->axesNormRect(plot2);

    // 示例2: 使用网格布局添加绘图
    QwtPlot* plot3       = new QwtPlot();
    QwtPlotCurve* curve3 = new QwtPlotCurve("Sine Wave 3");
    curve3->setSamples(generateSampleData(100, 2.0, 0.5));
    curve3->attach(plot3);
    setupPlotStyle(plot3, "Grid Layout (3x2, Cell 1,0)", Qt::green);
    figure->addAxes(plot3, 3, 2, 1, 0);  // 3x2网格，第1行第0列（0base）
    qDebug() << "plot3 norm rect =" << figure->axesNormRect(plot3);

    QwtPlot* plot4       = new QwtPlot();
    QwtPlotCurve* curve4 = new QwtPlotCurve("Sine Wave 4");
    curve4->setSamples(generateSampleData(100, 0.8, 1.5));
    curve4->attach(plot4);
    setupPlotStyle(plot4, "Grid Layout (3x2, Cell 1,1)", Qt::magenta);
    figure->addAxes(plot4, 3, 2, 1, 1);  // 2x2网格，第1行第1列（0base）
    qDebug() << "plot4 norm rect =" << figure->axesNormRect(plot4);

    QwtPlot* plot5       = new QwtPlot();
    QwtPlotCurve* curve5 = new QwtPlotCurve("Sine Wave 5");
    curve5->setSamples(generateSampleData(100, 1.2, 0.8));
    curve5->attach(plot5);
    setupPlotStyle(plot5, "Grid Layout (3x2, Cell 2,0-1)", Qt::darkCyan);
    figure->addAxes(plot5, 3, 2, 2, 0, 1, 2);  // 2x2网格，第2行第0列，跨2列
    qDebug() << "plot5 norm rect =" << figure->axesNormRect(plot5);

    // 示例3: 调整布局参数
    figure->adjustLayout(0.05, 0.05, 0.05, 0.05);
    qDebug() << "after adjustLayout(0.1, 0.1, 0.05, 0.05, 0.05, 0.05)";
    qDebug() << "plot1 norm rect =" << figure->axesNormRect(plot1);
    qDebug() << "plot2 norm rect =" << figure->axesNormRect(plot2);
    qDebug() << "plot3 norm rect =" << figure->axesNormRect(plot3);
    qDebug() << "plot4 norm rect =" << figure->axesNormRect(plot4);
    qDebug() << "plot5 norm rect =" << figure->axesNormRect(plot5);
    // 添加控制按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    QPushButton* saveButton = new QPushButton("Save Figure (300 DPI)");
    QObject::connect(saveButton, &QPushButton::clicked, [ figure ]() {
        figure->saveFig("qwt_figure_example.png", 300);
        qDebug() << "Figure saved as 'qwt_figure_example.png' with 300 DPI";
    });

    QPushButton* clearButton = new QPushButton("Clear All");
    QObject::connect(clearButton, &QPushButton::clicked, [ figure ]() {
        figure->clear();
        qDebug() << "All plots cleared";
    });

    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addStretch();

    // 添加布局
    mainLayout->addWidget(figure);
    mainLayout->addLayout(buttonLayout);

    // 设置中央部件
    mainWindow.setCentralWidget(centralWidget);
    mainWindow.show();

    return app.exec();
}
