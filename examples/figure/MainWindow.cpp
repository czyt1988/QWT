#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <cmath>
#include <limits>
#include <QVBoxLayout>
#include <QDebug>
// qwt
#include "qwt_math.h"
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_scale_widget.h"
#include "qwt_figure.h"
#include "qwt_figure_widget_overlay.h"
#include "qwt_scale_engine.h"
#include "qwt_plot_series_data_picker.h"
//
#include "PickLinker.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(ui->centralwidget);
    // 创建 QwtFigure
    QwtFigure* figure = new QwtFigure(ui->centralwidget);
    m_figure          = figure;
    initFigure(figure);
    createToolBar();
    // 添加布局
    mainLayout->addWidget(figure);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initFigure(QwtFigure* figure)
{

    figure->setFaceColor(Qt::white);  // 设置背景颜色

    // 示例1: 横轴为对数轴
    QwtPlot* plot1                   = new QwtPlot();
    QwtPlotSeriesDataPicker* picker1 = new QwtPlotSeriesDataPicker(plot1->canvas());

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
    QwtPlotSeriesDataPicker* picker2 = new QwtPlotSeriesDataPicker(plot2->canvas());

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
    QwtPlot* plot3                   = new QwtPlot();
    QwtPlotSeriesDataPicker* picker3 = new QwtPlotSeriesDataPicker(plot3->canvas());
    QwtPlotCurve* curve3             = new QwtPlotCurve("Sine Wave 3");
    // 生成带nan和inf值的曲线
    auto series = generateSampleDataWithNan();
    //    if (qwtContainsNanOrInf(series.begin(), series.end())) {
    //        qwtRemoveNanOrInf(series);
    //    }
    curve3->setSamples(series);
    curve3->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curve3->attach(plot3);
    setupPlotStyle(plot3, "Linear Axes (3x2, Cell 1,0) sample with nan and inf", Qt::green);
    figure->addGridAxes(plot3, 3, 2, 1, 0);  // 3x2网格，第1行第0列（0base）
    plot3->rescaleAxes();
    qDebug() << "plot3 norm rect =" << figure->axesNormRect(plot3);

    // 示例4: 双对数坐标
    QwtPlot* plot4                   = new QwtPlot();
    QwtPlotSeriesDataPicker* picker4 = new QwtPlotSeriesDataPicker(plot4->canvas());
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

    QwtPlot* hostPlot                = createParasitePlot();
    QwtPlotSeriesDataPicker* picker5 = new QwtPlotSeriesDataPicker(hostPlot->canvas());
    figure->addGridAxes(hostPlot, 3, 2, 2, 0, 1, 2);  // 3x2网格，第2行第0列，跨2列
    qDebug() << "plot4 norm rect =" << figure->axesNormRect(plot4);

    // 对齐坐标轴
    figure->addAxisAlignment({ plot1, plot3, hostPlot }, QwtAxis::YLeft);
    figure->addAxisAlignment({ plot2, plot4 }, QwtAxis::YLeft);

    m_pickerLinker = new PickLinker(this);
    m_pickerLinker->addPicker(picker1);
    m_pickerLinker->addPicker(picker2);
    m_pickerLinker->addPicker(picker3);
    m_pickerLinker->addPicker(picker4);
    m_pickerLinker->addPicker(picker5);
}

void MainWindow::setupPlotStyle(QwtPlot* plot, const QString& title, const QColor& color)
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

QwtPlot* MainWindow::createParasitePlot()
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

    //! 添加宿主坐标系
    QwtPlot* parasitePlot = hostPlot->createParasitePlot(QwtAxis::YLeft);
    parasitePlot->setParasiteShareAxis(QwtAxis::XBottom);
    //! 宿主坐标轴的其他设置
    parasitePlot->setAxisTitle(QwtPlot::yLeft, "Y2 Axis");

    //! 用于宿主坐标的曲线
    QwtPlotCurve* parasiteCurve = new QwtPlotCurve("Sine Wave 6");
    parasiteCurve->setSamples(generateSampleData(100, 2000, 2.3));
    parasiteCurve->attach(parasitePlot);
    return hostPlot;
}

void MainWindow::createToolBar()
{
    QAction* actionSave = new QAction(tr("Save"), this);
    connect(actionSave, &QAction::triggered, this, [ this ]() {
        m_figure->saveFig("qwt_figure_log_example.png", 300);
        qDebug() << "Figure saved as 'qwt_figure_log_example.png' with 300 DPI";
    });

    QAction* actionClear = new QAction(tr("Clear All"), this);
    connect(actionClear, &QAction::triggered, this, [ this ]() {
        m_figure->clear();
        qDebug() << "All plots cleared";
    });

    m_figureOverlay = new QwtFigureWidgetOverlay(m_figure);
    connect(m_figureOverlay,
            &QwtFigureWidgetOverlay::widgetNormGeometryChanged,
            this,
            [ this ](QWidget* w, const QRectF& oldNormGeo, const QRectF& newNormGeo) {
                Q_UNUSED(oldNormGeo);
                qDebug() << "setWidgetNormPos:" << newNormGeo;
                m_figure->setWidgetNormPos(w, newNormGeo);
            });
    QAction* actionResize = new QAction(tr("Resize"), this);
    actionResize->setCheckable(true);
    connect(actionResize, &QAction::triggered, this, [ this ](bool on) {
        if (on) {
            m_figureOverlay->show();
            m_figureOverlay->raise();
        } else {
            if (m_figureOverlay) {
                m_figureOverlay->hide();
            }
        }
        qDebug() << "enable figure overlay:" << on;
    });

    ui->toolBar->addAction(actionSave);
    ui->toolBar->addAction(actionClear);
    ui->toolBar->addAction(actionResize);
}

QVector< QPointF > MainWindow::generateSampleData(int count, double amplitude, double frequency)
{
    QVector< QPointF > data;
    for (int i = 0; i < count; ++i) {
        double x = i * 0.1;
        double y = amplitude * sin(frequency * x);
        data.append(QPointF(x, y));
    }
    return data;
}

QVector< QPointF > MainWindow::generateLogXSampleData(int count, double amplitude)
{
    QVector< QPointF > data;
    for (int i = 1; i <= count; ++i) {
        double x = i * 0.1;                    // 从0.1开始，避免对数坐标的0值问题
        double y = amplitude * sin(10.0 / x);  // 频率随x增大而减小的波形
        data.append(QPointF(x, y));
    }
    return data;
}

QVector< QPointF > MainWindow::generateLogYSampleData(int count, double baseValue)
{
    QVector< QPointF > data;
    for (int i = 0; i < count; ++i) {
        double x = i * 0.1;
        double y = baseValue + fabs(sin(x)) * 10.0;  // 确保y值始终为正
        data.append(QPointF(x, y));
    }
    return data;
}

QVector< QPointF > MainWindow::generateExponentialData(int count, double base)
{
    QVector< QPointF > data;
    for (int i = 0; i < count; ++i) {
        double x = i * 0.1;
        double y = base * exp(0.1 * x);  // 指数增长
        data.append(QPointF(x, y));
    }
    return data;
}

QVector< QPointF > MainWindow::generateSampleDataWithNan()
{
    QVector< QPointF > data;
    double amplitude = 10000.0;
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
