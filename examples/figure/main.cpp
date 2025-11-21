#include <cmath>
#include <limits>
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QDebug>
#include <QMetaObject>
#include "qwt_math.h"
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_scale_widget.h"
#include "qwt_figure.h"
#include "qwt_figure_widget_overlay.h"
#include "qwt_scale_engine.h"

// 生成示例数据（线性坐标）
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

// 生成适合对数X轴的数据
QVector< QPointF > generateLogXSampleData(int count = 100, double amplitude = 1.0)
{
    QVector< QPointF > data;
    for (int i = 1; i <= count; ++i) {
        double x = i * 0.1;                    // 从0.1开始，避免对数坐标的0值问题
        double y = amplitude * sin(10.0 / x);  // 频率随x增大而减小的波形
        data.append(QPointF(x, y));
    }
    return data;
}

// 生成适合对数Y轴的数据（确保所有y值为正）
QVector< QPointF > generateLogYSampleData(int count = 100, double baseValue = 1.0)
{
    QVector< QPointF > data;
    for (int i = 0; i < count; ++i) {
        double x = i * 0.1;
        double y = baseValue + fabs(sin(x)) * 10.0;  // 确保y值始终为正
        data.append(QPointF(x, y));
    }
    return data;
}

// 生成指数增长数据（适合对数Y轴）
QVector< QPointF > generateExponentialData(int count = 100, double base = 1.0)
{
    QVector< QPointF > data;
    for (int i = 0; i < count; ++i) {
        double x = i * 0.1;
        double y = base * exp(0.1 * x);  // 指数增长
        data.append(QPointF(x, y));
    }
    return data;
}

// 生成带nan值和inf值的示例数据
QVector< QPointF > generateSampleDataWithNan()
{
    QVector< QPointF > data;
    double amplitude = 100.0;
    double frequency = 30.0;
    for (int i = 0; i < 50; ++i) {
        double x = i * 0.1;
        double y = amplitude * sin(frequency * x);
        if (i == 5) {
            x = std::numeric_limits< double >::quiet_NaN();
        }
        if (i == 10) {
            y = std::numeric_limits< double >::quiet_NaN();
        }
        if (i == 15) {
            x = std::numeric_limits< double >::infinity();
        }
        if (i == 20) {
            y = std::numeric_limits< double >::infinity();
        }
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
void createGrid32_parasitePlot(QwtFigure* figure);

// 创建示例应用程序
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 创建主窗口
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("QwtFigure Layout Example with Log Scales");
    mainWindow.resize(1200, 800);

    // 创建中央部件
    QWidget* centralWidget  = new QWidget(&mainWindow);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // 创建 QwtFigure
    QwtFigure* figure = new QwtFigure(centralWidget);
    figure->setSizeInches(8, 6);      // 设置图形尺寸为8x6英寸
    figure->setFaceColor(Qt::white);  // 设置背景颜色

    // 示例1: 横轴为对数轴
    QwtPlot* plot1 = new QwtPlot();

    // 设置X轴为对数坐标
    plot1->setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine);

    QwtPlotCurve* curve1 = new QwtPlotCurve("Log X Wave");
    curve1->setSamples(generateLogXSampleData(200, 2.0));  // 使用适合对数X轴的数据
    curve1->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curve1->attach(plot1);

    setupPlotStyle(plot1, "Logarithmic X-Axis (Top-Left)", Qt::blue);
    figure->addAxes(plot1, 0.0, 0.0, 0.5, 0.3333333);  // 左上角
    plot1->setAxisAutoScale(QwtPlot::xBottom, true);
    plot1->setAxisAutoScale(QwtPlot::yLeft, true);
    plot1->replot();
    qDebug() << "plot1 norm rect =" << figure->axesNormRect(plot1);

    // 示例2: 纵轴为对数轴
    QwtPlot* plot2 = new QwtPlot();

    // 设置Y轴为对数坐标
    plot2->setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine);

    QwtPlotCurve* curve2 = new QwtPlotCurve("Log Y Wave");
    curve2->setSamples(generateExponentialData(100, 0.1));  // 使用指数增长数据，适合对数Y轴
    curve2->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curve2->attach(plot2);

    setupPlotStyle(plot2, "Logarithmic Y-Axis (Top-Right)", Qt::red);
    figure->addAxes(plot2, 0.5, 0.0, 0.5, 0.33333333);  // 右上角
    plot2->setAxisAutoScale(QwtPlot::xBottom, true);
    plot2->setAxisAutoScale(QwtPlot::yLeft, true);
    plot2->replot();
    qDebug() << "plot2 norm rect =" << figure->axesNormRect(plot2);

    // 示例3: 线性坐标（保持不变）
    QwtPlot* plot3       = new QwtPlot();
    QwtPlotCurve* curve3 = new QwtPlotCurve("Sine Wave 3");
    // 生成带nan和inf值的曲线
    auto series = generateSampleDataWithNan();
    if (qwtContainsNanOrInf(series.begin(), series.end())) {
        qwtRemoveNanOrInf(series);
    }
    curve3->setSamples(series);
    curve3->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curve3->attach(plot3);
    setupPlotStyle(plot3, "Linear Axes (3x2, Cell 1,0) sample with nan and inf", Qt::green);
    figure->addGridAxes(plot3, 3, 2, 1, 0);  // 3x2网格，第1行第0列（0base）
    plot3->rescaleAxes();
    qDebug() << "plot3 norm rect =" << figure->axesNormRect(plot3);

    // 示例4: 双对数坐标
    QwtPlot* plot4 = new QwtPlot();

    // 设置双对数坐标
    plot4->setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine);
    plot4->setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine);

    QwtPlotCurve* curve4 = new QwtPlotCurve("Log-Log Wave");

    // 生成适合双对数坐标的数据（幂律关系）
    QVector< QPointF > logLogData;
    for (int i = 1; i <= 100; ++i) {
        double x = i * 0.1;
        double y = 0.5 * pow(x, 2.0);  // 幂律关系 y = 0.5 * x^2
        logLogData.append(QPointF(x, y));
    }

    curve4->setSamples(logLogData);
    curve4->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curve4->attach(plot4);
    setupPlotStyle(plot4, "Log-Log Axes (3x2, Cell 1,1)", Qt::magenta);
    figure->addGridAxes(plot4, 3, 2, 1, 1);  // 2x2网格，第1行第1列（0base）
    plot4->setAxisAutoScale(QwtPlot::xBottom, true);
    plot4->setAxisAutoScale(QwtPlot::yLeft, true);
    plot4->replot();
    qDebug() << "plot4 norm rect =" << figure->axesNormRect(plot4);

    createGrid32_parasitePlot(figure);

    // 添加控制按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    QPushButton* saveButton = new QPushButton("Save Figure (300 DPI)");
    QObject::connect(saveButton, &QPushButton::clicked, [ figure ]() {
        figure->saveFig("qwt_figure_log_example.png", 300);
        qDebug() << "Figure saved as 'qwt_figure_log_example.png' with 300 DPI";
    });

    QPushButton* clearButton = new QPushButton("Clear All");
    QObject::connect(clearButton, &QPushButton::clicked, [ figure ]() {
        figure->clear();
        qDebug() << "All plots cleared";
    });

    QPushButton* resizeButton = new QPushButton("Resize");
    resizeButton->setCheckable(true);
    QwtFigureWidgetOverlay* overlay = new QwtFigureWidgetOverlay(figure);
    QObject::connect(resizeButton, &QPushButton::clicked, [ overlay ](bool on) {
        if (on) {
            overlay->show();
            overlay->raise();
        } else {
            if (overlay) {
                overlay->hide();
            }
        }
        qDebug() << "enable figure overlay:" << on;
    });
    QObject::connect(overlay,
                     &QwtFigureWidgetOverlay::widgetNormGeometryChanged,
                     [ figure ](QWidget* w, const QRectF& oldNormGeo, const QRectF& newNormGeo) {
                         Q_UNUSED(oldNormGeo);
                         qDebug() << "setWidgetNormPos:" << newNormGeo;
                         figure->setWidgetNormPos(w, newNormGeo);
                     });

    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addWidget(resizeButton);
    buttonLayout->addStretch();

    // 添加布局
    mainLayout->addWidget(figure);
    mainLayout->addLayout(buttonLayout);

    // 设置中央部件
    mainWindow.setCentralWidget(centralWidget);
    mainWindow.show();
    return app.exec();
}

void createGrid32_parasitePlot(QwtFigure* figure)
{
    //! 创建宿主绘图
    QwtPlot* hostPlot = new QwtPlot();
    hostPlot->setCanvasBackground(Qt::white);

    //! 设置宿主绘图坐标轴标题
    hostPlot->setTitle("Grid Layout (3x2, Cell 2,0-1)");
    hostPlot->setAxisTitle(QwtPlot::xBottom, "X Axis");
    hostPlot->setAxisTitle(QwtPlot::yLeft, "Y1 Axis");
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
    hostCurve->setPen(Qt::darkCyan, 0.5);
    hostCurve->attach(hostPlot);

    //! 把主绘图添加到figure中
    figure->addGridAxes(hostPlot, 3, 2, 2, 0, 1, 2);  // 3x2网格，第2行第0列，跨2列

    //! 添加宿主坐标系
    QwtPlot* parasitePlot = figure->createParasiteAxes(hostPlot, QwtAxis::YLeft);
    parasitePlot->setParasiteShareAxis(QwtAxis::XBottom);
    //! 宿主坐标轴的其他设置
    parasitePlot->setAxisTitle(QwtPlot::yLeft, "Y2 Axis");

    //! 用于宿主坐标的曲线
    QwtPlotCurve* parasiteCurve = new QwtPlotCurve("Sine Wave 6");
    parasiteCurve->setSamples(generateSampleData(100, 2000, 2.3));
    parasiteCurve->attach(parasitePlot);
}
