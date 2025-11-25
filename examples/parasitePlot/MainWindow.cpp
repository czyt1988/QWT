#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_scale_widget.h"
#include "qwt_figure.h"
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

#include "qwt_plot_series_data_picker.h"
#include "qwt_plot_panner.h"
#include "qwt_plot_canvas_zoomer.h"
#include "qwt_plot_magnifier.h"
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

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mStatusBarLabel = new QLabel();
    ui->statusbar->addWidget(mStatusBarLabel);
    // create plot
    QPalette pal = ui->centralwidget->palette();
    pal.setColor(QPalette::Window, Qt::white);
    ui->centralwidget->setPalette(pal);
    ui->centralwidget->setAutoFillBackground(true);
    QVBoxLayout* mainLayout = new QVBoxLayout(ui->centralwidget);
    m_plot                  = createPlot(ui->centralwidget);
    createToolBar();
    // 添加布局
    mainLayout->addWidget(m_plot);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QwtPlot* MainWindow::createPlot(QWidget* par)
{
    // 创建宿主绘图
    QwtPlot* hostPlot = new QwtPlot(par);
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
    // parasitePlot->enableAxis(QwtAxis::XBottom, true);
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

    // 建立其他附加工具，picker要在宿主绘图的canvas那里，否则会无法捕获到事件（寄生绘图对鼠标透明）
    m_dataPicker = new QwtPlotSeriesDataPicker(hostPlot->canvas());
    m_dataPicker->setEnabled(false);
    // panner
    m_panner = new QwtPlotPanner(hostPlot->canvas());
    m_panner->setMouseButton(Qt::MiddleButton);
    m_panner->setEnabled(false);
    // zoomer
    m_zoomer = new QwtPlotCanvasZoomer(hostPlot->canvas());
    m_zoomer->setEnabled(false);
    //
    m_magnifier = new QwtPlotMagnifier(hostPlot->canvas());
    m_magnifier->setEnabled(false);
    return hostPlot;
}

void MainWindow::createToolBar()
{
    QActionGroup* group    = new QActionGroup(this);
    QAction* actPickYValue = ui->toolBar->addAction("Pick Y Value");
    actPickYValue->setCheckable(true);
    connect(actPickYValue, &QAction::triggered, this, [ this ](bool on) {
        if (!(this->m_dataPicker->isEnabled())) {
            this->m_dataPicker->setEnabled(true);
        }
        if (on) {
            this->m_dataPicker->setPickMode(QwtPlotSeriesDataPicker::PickYValue);
        }
    });

    QAction* actPickNearestPoint = ui->toolBar->addAction("Pick Nearest Point");
    actPickNearestPoint->setCheckable(true);
    connect(actPickNearestPoint, &QAction::triggered, this, [ this ](bool on) {
        if (!(this->m_dataPicker->isEnabled())) {
            this->m_dataPicker->setEnabled(true);
        }
        if (on) {
            this->m_dataPicker->setPickMode(QwtPlotSeriesDataPicker::PickNearestPoint);
        }
    });
    group->addAction(actPickYValue);
    group->addAction(actPickNearestPoint);
    group->setExclusive(true);

    QAction* actPanner = ui->toolBar->addAction("Panner");
    actPanner->setCheckable(true);
    connect(actPanner, &QAction::triggered, this, [ this ](bool on) {
        this->m_panner->setEnabled(on);
        if (on) {
            this->mStatusBarLabel->setText(tr("Use the middle mouse button to drag the canvas"));  // cn:使用鼠标左键拖动画布
        }
    });

    QAction* actZoomer = ui->toolBar->addAction("Zoomer");
    actZoomer->setCheckable(true);
    connect(actZoomer, &QAction::triggered, this, [ this ](bool on) {
        this->m_zoomer->setEnabled(on);
        if (on) {
            mStatusBarLabel->setText(
                tr("Use the mouse to drag a selection box on the canvas to zoom into the selected area."));  // cn:使用鼠标在画布中框选要缩放的区域进行缩放
        }
    });

    QAction* actMagnifier = ui->toolBar->addAction("Magnifier");
    actMagnifier->setCheckable(true);
    connect(actMagnifier, &QAction::triggered, this, [ this ](bool on) {
        this->m_magnifier->setEnabled(on);
        if (on) {
            mStatusBarLabel->setText(tr("Use the mouse wheel to zoom the canvas.."));  // cn:使用鼠标滚轮缩放画布
        }
    });
}
