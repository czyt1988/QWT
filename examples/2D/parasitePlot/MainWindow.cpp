#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_scale_widget.h"
#include "qwt_plot_layout.h"
#include "qwt_figure.h"
#include "qwt_plot_histogram.h"
#include "qwt_plot_vectorfield.h"
#include "qwt_samples.h"
#include <QPen>
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QActionGroup>
#include <QAction>
#include <QIcon>
#include <QLabel>

#include "qwt_plot_series_data_picker.h"
#include "qwt_plot_panner.h"
#include "qwt_plot_canvas_zoomer.h"
#include "qwt_plot_magnifier.h"
#include "qwt_plot_axis_wheel_interaction.h"
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

    //! 给宿主绘图添加直方图 (QwtPlotHistogram) — 演示多类型 picker
    QwtPlotHistogram* histogram = new QwtPlotHistogram("Frequency Histogram");
    QVector< QwtIntervalSample > histData;
    for (int i = 0; i < 15; ++i) {
        double low  = i * 0.6;
        double high = (i + 1) * 0.6;
        double freq = 0.5 + 0.8 * fabs(sin(i * 0.5));
        histData.append(QwtIntervalSample(freq, QwtInterval(low, high)));
    }
    histogram->setSamples(histData);
    histogram->setStyle(QwtPlotHistogram::Columns);
    histogram->setPen(QColor(255, 165, 0), 1.0);
    histogram->setBrush(QColor(255, 165, 0, 100));
    histogram->attach(hostPlot);

    //! 给宿主绘图添加向量场 (QwtPlotVectorField) — 演示多类型 picker
    QwtPlotVectorField* vectorField = new QwtPlotVectorField("Wind Vectors");
    QVector< QwtVectorFieldSample > vecData;
    for (int i = 0; i < 10; ++i) {
        double x  = i * 0.9 + 0.5;
        double y  = 0.5 * sin(0.8 * x);
        double vx = 0.3 * cos(i * 0.6);
        double vy = 0.3 * sin(i * 0.6);
        vecData.append(QwtVectorFieldSample(x, y, vx, vy));
    }
    vectorField->setSamples(vecData);
    vectorField->setPen(QPen(QColor(128, 0, 128), 1.5));
    vectorField->attach(hostPlot);

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

    // Set the initial title position for every axis to TitleAtEnd so the stacked
    // YLeft titles sit at the top of each backbone and no longer overlap. The
    // toolbar "Title Pos" action group starts in the End state to match this.
    for (QwtPlot* p : hostPlot->plotList()) {
        for (int pos = 0; pos < QwtAxis::AxisPositions; pos++) {
            const QwtAxisId axisId(pos);
            if (p->isAxisValid(axisId) && p->isAxisVisible(axisId))
                p->setAxisTitlePosition(axisId, QwtScaleWidget::TitleAtEnd);
        }
    }

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
    // Axis wheel interaction on the XBottom axis:
    //   plain wheel  → zoom the axis at cursor position (no need to click-select first)
    //   Ctrl+wheel   → pan the axis left/right
    m_axisWheel = new QwtPlotAxisWheelInteraction(hostPlot, QwtAxis::XBottom);
    m_axisWheel->setEnabled(false);
    return hostPlot;
}

void MainWindow::createToolBar()
{
    // -----------------------------------------------------------------------
    // 1. Picker mode (exclusive group)
    // -----------------------------------------------------------------------
    QActionGroup* group    = new QActionGroup(this);
    QAction* actPickYValue = ui->toolBar->addAction(QIcon(":/icons/pick-y.svg"), "Pick Y Value");
    actPickYValue->setCheckable(true);
    connect(actPickYValue, &QAction::triggered, this, [ this ](bool on) {
        if (!(this->m_dataPicker->isEnabled())) {
            this->m_dataPicker->setEnabled(true);
        }
        if (on) {
            this->m_dataPicker->setPickMode(QwtPlotSeriesDataPicker::PickYValue);
        }
    });

    QAction* actPickNearestPoint = ui->toolBar->addAction(QIcon(":/icons/pick-point.svg"), "Pick Nearest Point");
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

    ui->toolBar->addSeparator();

    // -----------------------------------------------------------------------
    // 2. Canvas interactions
    // -----------------------------------------------------------------------
    QAction* actPanner = ui->toolBar->addAction(QIcon(":/icons/panner.svg"), "Panner");
    actPanner->setCheckable(true);
    connect(actPanner, &QAction::triggered, this, [ this ](bool on) {
        this->m_panner->setEnabled(on);
        if (on) {
            mStatusBarLabel->setText(tr("Use the middle mouse button to drag the canvas"));  // cn:使用鼠标左键拖动画布
        }
    });

    QAction* actZoomer = ui->toolBar->addAction(QIcon(":/icons/zoomer.svg"), "Zoomer");
    actZoomer->setCheckable(true);
    connect(actZoomer, &QAction::triggered, this, [ this ](bool on) {
        this->m_zoomer->setEnabled(on);
        if (on) {
            m_zoomer->setZoomBase(false);
            mStatusBarLabel->setText(
                tr("Use the mouse to drag a selection box on the canvas to zoom into the selected area.")
            );  // cn:使用鼠标在画布中框选要缩放的区域进行缩放
        }
    });

    QAction* actMagnifier = ui->toolBar->addAction(QIcon(":/icons/magnifier.svg"), "Magnifier");
    actMagnifier->setCheckable(true);
    connect(actMagnifier, &QAction::triggered, this, [ this ](bool on) {
        this->m_magnifier->setEnabled(on);
        if (on) {
            mStatusBarLabel->setText(tr("Use the mouse wheel to zoom the canvas.."));  // cn:使用鼠标滚轮缩放画布
        }
    });

    QAction* actAxisWheel = ui->toolBar->addAction(QIcon(":/icons/axis-wheel.svg"), "Axis Wheel");
    actAxisWheel->setCheckable(true);
    connect(actAxisWheel, &QAction::triggered, this, [ this ](bool on) {
        this->m_axisWheel->setEnabled(on);
        if (on) {
            mStatusBarLabel->setText(tr("Move the mouse over the XBottom axis: "
                                        "plain wheel = zoom at cursor; "
                                        "Ctrl+wheel = pan the axis"));  // cn:鼠标移到X轴上: 纯滚轮以鼠标为中心缩放轴, Ctrl+滚轮平移轴
        }
    });

    ui->toolBar->addSeparator();

    // -----------------------------------------------------------------------
    // 3. Title position / alignment verification
    //    Three exclusive action groups: target axis, position, alignment.
    //    Selecting any action re-applies the current combo to the host + every
    //    parasite plot, so all 4 sides x 3 positions x 3 alignments can be
    //    inspected live.
    // -----------------------------------------------------------------------

    // 3a. Target axis
    m_titleAxisGroup = new QActionGroup(this);
    m_titleAxisGroup->setExclusive(true);
    auto addAxisAction = [ this ](const QString& text, const QIcon& icon, QwtAxisId axisId) {
        QAction* a = ui->toolBar->addAction(icon, text);
        a->setCheckable(true);
        a->setData(QVariant::fromValue(axisId));
        connect(a, &QAction::triggered, this, [ this ]() { applyTitleSettings(); });
        m_titleAxisGroup->addAction(a);
        return a;
    };
    QAction* actAxisAll    = addAxisAction("Title Axis: All", QIcon(":/icons/axis-all.svg"), QwtAxisId(-1));
    QAction* actAxisYLeft  = addAxisAction("Title Axis: YLeft", QIcon(":/icons/axis-yleft.svg"), QwtAxis::YLeft);
    QAction* actAxisYRight = addAxisAction("Title Axis: YRight", QIcon(":/icons/axis-yright.svg"), QwtAxis::YRight);
    QAction* actAxisXBottom = addAxisAction("Title Axis: XBottom", QIcon(":/icons/axis-xbottom.svg"), QwtAxis::XBottom);
    QAction* actAxisXTop   = addAxisAction("Title Axis: XTop", QIcon(":/icons/axis-xtop.svg"), QwtAxis::XTop);
    actAxisAll->setChecked(true);

    ui->toolBar->addSeparator();

    // 3b. Title position
    m_titlePosGroup = new QActionGroup(this);
    m_titlePosGroup->setExclusive(true);
    auto addPosAction = [ this ](const QString& text, const QIcon& icon, QwtScaleWidget::TitlePosition pos) {
        QAction* a = ui->toolBar->addAction(icon, text);
        a->setCheckable(true);
        a->setData(static_cast< int >(pos));
        connect(a, &QAction::triggered, this, [ this ]() { applyTitleSettings(); });
        m_titlePosGroup->addAction(a);
        return a;
    };
    QAction* actPosCentered = addPosAction("Title Pos: Center", QIcon(":/icons/pos-centered.svg"), QwtScaleWidget::TitleCentered);
    QAction* actPosStart    = addPosAction("Title Pos: Start", QIcon(":/icons/pos-start.svg"), QwtScaleWidget::TitleAtStart);
    QAction* actPosEnd      = addPosAction("Title Pos: End", QIcon(":/icons/pos-end.svg"), QwtScaleWidget::TitleAtEnd);
    actPosEnd->setChecked(true);  // matches the initial state set in createPlot()

    ui->toolBar->addSeparator();

    // 3c. Title text alignment
    m_titleAlignGroup = new QActionGroup(this);
    m_titleAlignGroup->setExclusive(true);
    auto addAlignAction = [ this ](const QString& text, const QIcon& icon, Qt::Alignment align) {
        QAction* a = ui->toolBar->addAction(icon, text);
        a->setCheckable(true);
        a->setData(static_cast< int >(align));
        connect(a, &QAction::triggered, this, [ this ]() { applyTitleSettings(); });
        m_titleAlignGroup->addAction(a);
        return a;
    };
    QAction* actAlignLeft   = addAlignAction("Title Align: Left", QIcon(":/icons/align-left.svg"), Qt::AlignLeft);
    QAction* actAlignCenter = addAlignAction("Title Align: Center", QIcon(":/icons/align-center.svg"), Qt::AlignHCenter);
    QAction* actAlignRight  = addAlignAction("Title Align: Right", QIcon(":/icons/align-right.svg"), Qt::AlignRight);
    actAlignCenter->setChecked(true);

    ui->toolBar->addSeparator();

    // -----------------------------------------------------------------------
    // 4. Fixed-canvas / long-label demo
    // -----------------------------------------------------------------------
    QAction* actFixedCanvas = ui->toolBar->addAction(QIcon(":/icons/fixed-canvas.svg"), "Fixed Canvas");
    actFixedCanvas->setCheckable(true);
    connect(actFixedCanvas, &QAction::triggered, this, [ this ](bool on) {
        m_plot->plotLayout()->setFixedCanvasSize(QwtAxis::YLeft, on);
        if (!on)
            m_plot->plotLayout()->resetFixedCanvasSize();
        m_plot->replot();
        mStatusBarLabel->setText(
            on ? tr("Fixed canvas ON: Y-axis labels that grow will overflow "
                    "and be clipped instead of shrinking the canvas.")
               : tr("Fixed canvas OFF: canvas shrinks with label growth."));
    });

    QAction* actLongLabels = ui->toolBar->addAction(QIcon(":/icons/long-labels.svg"), "Long Y Labels");
    actLongLabels->setCheckable(true);
    connect(actLongLabels, &QAction::triggered, this, [ this ](bool on) {
        if (on)
            m_plot->setAxisScale(QwtAxis::YLeft, -1000000.0, 1000000.0);
        else
            m_plot->setAxisScale(QwtAxis::YLeft, -1.5, 1.5);
        m_plot->replot();
        mStatusBarLabel->setText(
            on ? tr("YLeft labels switched to large values (long text).")
               : tr("YLeft labels switched back to short values."));
    });
}

void MainWindow::applyTitleSettings()
{
    if (!m_titleAxisGroup || !m_titlePosGroup || !m_titleAlignGroup)
        return;

    QAction* axisAction = m_titleAxisGroup->checkedAction();
    QAction* posAction = m_titlePosGroup->checkedAction();
    QAction* alignAction = m_titleAlignGroup->checkedAction();
    if (!axisAction || !posAction || !alignAction)
        return;

    const QwtAxisId targetAxis = axisAction->data().value< QwtAxisId >();
    const QwtScaleWidget::TitlePosition pos = static_cast< QwtScaleWidget::TitlePosition >(posAction->data().toInt());
    const Qt::Alignment align = static_cast< Qt::Alignment >(alignAction->data().toInt());

    // Apply to the host and every parasite plot. targetAxis == -1 means "all".
    for (QwtPlot* p : m_plot->plotList()) {
        if (targetAxis == QwtAxisId(-1)) {
            for (int axPos = 0; axPos < QwtAxis::AxisPositions; axPos++) {
                const QwtAxisId axisId(axPos);
                if (p->isAxisValid(axisId) && p->isAxisVisible(axisId)) {
                    p->setAxisTitlePosition(axisId, pos);
                    p->setAxisTitleAlignment(axisId, align);
                }
            }
        } else {
            if (p->isAxisValid(targetAxis) && p->isAxisVisible(targetAxis)) {
                p->setAxisTitlePosition(targetAxis, pos);
                p->setAxisTitleAlignment(targetAxis, align);
            }
        }
        p->replot();
    }

    mStatusBarLabel->setText(tr("Title applied — axis: %1 | pos: %2 | align: %3")
        .arg(axisAction->text(), posAction->text(), alignAction->text()));
}
