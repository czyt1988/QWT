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

	// 创建 QwtFigure
	QwtFigure* figure = new QwtFigure(centralWidget);
	figure->setSizeInches(8, 6);      // 设置图形尺寸为8x6英寸
	figure->setFaceColor(Qt::white);  // 设置背景颜色

    createParasitePlot(figure);
	// 添加控制按钮
	QHBoxLayout* buttonLayout = new QHBoxLayout();

	QPushButton* saveButton = new QPushButton("Save Figure (300 DPI)");
	QObject::connect(saveButton, &QPushButton::clicked, [ figure ]() {
        figure->saveFig("qwt_parasite_example.png", 300);
        qDebug() << "Figure saved as 'qwt_parasite_example.png' with 300 DPI";
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

void createParasitePlot(QwtFigure* figure)
{
    //! 创建宿主绘图
    QwtPlot* hostPlot = new QwtPlot();
    hostPlot->setCanvasBackground(Qt::white);

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
    //! 把主绘图添加到figure中
    figure->addAxes(hostPlot, 1, 1, 0, 0);  //  填满整个figure

    //! 添加宿主坐标系
    QwtPlot* parasitePlot = figure->createParasiteAxes(hostPlot, QwtAxis::YLeft);

    //! 为了演示，把寄生轴坐标都显示
    parasitePlot->enableAxis(QwtAxis::YRight, true);
    parasitePlot->enableAxis(QwtAxis::XBottom, true);
    parasitePlot->enableAxis(QwtAxis::XTop, true);
    //! 为了让宿主坐标显示出来，主坐标的左边要预留空白，通过setEdgeMargin设置坐标轴和绘图边缘的空白
    hostPlot->axisWidget(QwtAxis::YLeft)->setEdgeMargin(80);
    hostPlot->axisWidget(QwtAxis::YRight)->setEdgeMargin(80);
    hostPlot->axisWidget(QwtAxis::XBottom)->setEdgeMargin(80);
    hostPlot->axisWidget(QwtAxis::XTop)->setEdgeMargin(80);
    //! 宿主坐标轴和画布偏移60像素，避免和主坐标轴重合
    parasitePlot->axisWidget(QwtAxis::YLeft)->setMargin(60);
    parasitePlot->axisWidget(QwtAxis::YRight)->setMargin(70);
    parasitePlot->axisWidget(QwtAxis::XBottom)->setMargin(60);
    parasitePlot->axisWidget(QwtAxis::XTop)->setMargin(60);
    //! 宿主坐标轴的其他设置
    parasitePlot->setAxisTitle(QwtAxis::YLeft, "Y2 Left Axis");
    parasitePlot->setAxisTitle(QwtAxis::YRight, "Y2 Right Axis");
    parasitePlot->setAxisTitle(QwtAxis::XBottom, "X2 Bottom Axis");
    parasitePlot->setAxisTitle(QwtAxis::XTop, "X2 Top Axis");
    //! 用于宿主坐标的曲线
    QColor curColor             = QColor(255, 127, 14);
    QwtPlotCurve* parasiteCurve = new QwtPlotCurve("Sine Wave 6");
    parasiteCurve->setSamples(generateSampleData(100, 2000, 2.3));
    parasiteCurve->attach(parasitePlot);
    parasiteCurve->setPen(curColor, 1.5);
    parasiteCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    //! 为了区分宿主轴，给宿主轴的坐标也增加颜色
    parasitePlot->axisWidget(QwtAxis::YLeft)->setScaleColor(curColor);
}
