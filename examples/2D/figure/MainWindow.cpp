#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <cmath>
#include <limits>
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>
// qwt
#include "qwt_math.h"
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_scale_widget.h"
#include "qwt_figure.h"
#include "qwt_figure_grid.h"
#include "qwt_figure_widget_overlay.h"
#include "qwt_scale_engine.h"
#include "qwt_plot_series_data_picker.h"
#include "qwt_plot_scaleitem.h"
#include "qwt_plot_tradingcurve.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_boxchart.h"
#include "qwt_plot_barchart.h"
#include "qwt_samples.h"
//
#include "qwt_plot_series_data_picker_group.h"

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
    // m_clickInfoLabel is created inside initFigure(), added to layout there
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

    // 示例2: K线图 (QwtPlotTradingCurve) — 演示多类型 picker
    QwtPlot* plot2                   = new QwtPlot();
    QwtPlotSeriesDataPicker* picker2 = new QwtPlotSeriesDataPicker(plot2->canvas());

    QwtPlotTradingCurve* tradingCurve = new QwtPlotTradingCurve("Stock OHLC");
    QVector< QwtOHLCSample > ohlcData;
    double basePrice = 100.0;
    for (int i = 0; i < 60; ++i) {
        double open  = basePrice + (rand() % 10 - 5);
        double close = open + (rand() % 6 - 3);
        double high  = std::max(open, close) + (rand() % 3);
        double low   = std::min(open, close) - (rand() % 3);
        ohlcData.append(QwtOHLCSample(i, open, high, low, close));
        basePrice = close;
    }
    tradingCurve->setSamples(ohlcData);
    tradingCurve->setSymbolStyle(QwtPlotTradingCurve::CandleStick);
    tradingCurve->setSymbolPen(QColor(231, 76, 60), 1.0);
    tradingCurve->setSymbolBrush(QwtPlotTradingCurve::Increasing, QColor(46, 204, 113));
    tradingCurve->setSymbolBrush(QwtPlotTradingCurve::Decreasing, QColor(231, 76, 60));
    tradingCurve->attach(plot2);

    setupPlotStyle(plot2, "Trading Chart - QwtPlotTradingCurve (Top-Right)", QColor(231, 76, 60));
    figure->addAxes(plot2, 0.5, 0.0, 0.5, 0.33333333);  // 右上角
    plot2->setAxisAutoScale(QwtPlot::xBottom, true);
    plot2->setAxisAutoScale(QwtPlot::yLeft, true);
    plot2->setAxisTitle(QwtPlot::xBottom, "Day");
    plot2->setAxisTitle(QwtPlot::yLeft, "Price");
    plot2->replot();
    qDebug() << "plot2 norm rect =" << figure->axesNormRect(plot2);

    // 示例3: QwtPlotIntervalCurve (温度区间图) — 演示多类型 picker
    QwtPlot* plot3                   = new QwtPlot();
    QwtPlotSeriesDataPicker* picker3 = new QwtPlotSeriesDataPicker(plot3->canvas());

    QwtPlotIntervalCurve* intervalCurve = new QwtPlotIntervalCurve("Daily Temperature Range");
    QVector< QwtIntervalSample > intervalData;
    for (int i = 0; i < 30; ++i) {
        double base     = 15.0 + 10.0 * sin(i * 0.2);
        double lowTemp  = base - (3.0 + rand() % 4);
        double highTemp = base + (3.0 + rand() % 4);
        intervalData.append(QwtIntervalSample(i, QwtInterval(lowTemp, highTemp)));
    }
    intervalCurve->setSamples(intervalData);
    intervalCurve->setStyle(QwtPlotIntervalCurve::Tube);
    intervalCurve->setPen(QColor(46, 204, 113));
    intervalCurve->setBrush(QColor(46, 204, 113, 80));
    intervalCurve->attach(plot3);

    setupPlotStyle(plot3, "Interval Curve - QwtPlotIntervalCurve (3x2, Cell 1,0)", QColor(46, 204, 113));
    figure->addGridAxes(plot3, 3, 2, 1, 0);
    plot3->setAxisTitle(QwtPlot::xBottom, "Day");
    plot3->setAxisTitle(QwtPlot::yLeft, "Temp (°C)");
    plot3->setAxisAutoScale(QwtPlot::xBottom, true);
    plot3->setAxisAutoScale(QwtPlot::yLeft, true);
    plot3->replot();
    qDebug() << "plot3 norm rect =" << figure->axesNormRect(plot3);

    // 示例4: 箱线图 (QwtPlotBoxChart) — 演示多类型 picker
    QwtPlot* plot4                   = new QwtPlot();
    QwtPlotSeriesDataPicker* picker4 = new QwtPlotSeriesDataPicker(plot4->canvas());

    QwtPlotBoxChart* boxChart = new QwtPlotBoxChart("Test Score Distribution");
    QVector< QwtBoxSample > boxData;
    const QStringList categories = { "Math", "Physics", "Chemistry", "Biology", "English" };
    for (int i = 0; i < categories.size(); ++i) {
        double median = 60.0 + rand() % 25;
        double q1     = median - 5 - (rand() % 8);
        double q3     = median + 5 + (rand() % 8);
        double wl     = q1 - 5 - (rand() % 5);
        double wh     = q3 + 5 + (rand() % 5);
        boxData.append(QwtBoxSample(i, wl, q1, median, q3, wh));
    }
    boxChart->setSamples(boxData);
    boxChart->setPen(QColor(142, 68, 173), 1.5);
    boxChart->setBrush(QColor(142, 68, 173, 100));
    boxChart->attach(plot4);

    // Set X axis scale to match category positions
    plot4->setAxisScale(QwtPlot::xBottom, -0.5, categories.size() - 0.5);
    plot4->setAxisScaleDiv(QwtPlot::xBottom,
                           QwtScaleDiv(-0.5, categories.size() - 0.5,
                                       { 0.0, 1.0, 2.0, 3.0, 4.0 }, {}, {}));

    setupPlotStyle(plot4, "Box Chart - QwtPlotBoxChart (3x2, Cell 1,1)", QColor(142, 68, 173));
    figure->addGridAxes(plot4, 3, 2, 1, 1);
    plot4->setAxisTitle(QwtPlot::yLeft, "Score");
    plot4->setAxisAutoScale(QwtPlot::yLeft, true);
    plot4->replot();

    QwtPlot* hostPlot                = createParasitePlot();
    QwtPlotSeriesDataPicker* picker5 = new QwtPlotSeriesDataPicker(hostPlot->canvas());
    figure->addGridAxes(hostPlot, 3, 2, 2, 0, 1, 2);  // 3x2网格，第2行第0列，跨2列
    qDebug() << "plot4 norm rect =" << figure->axesNormRect(plot4);

    // 对齐坐标轴
    figure->addAxisAlignment({ plot1, plot3, hostPlot }, QwtAxis::YLeft);
    figure->addAxisAlignment({ plot2, plot4 }, QwtAxis::YLeft);

    m_pickerLinker = new QwtPlotSeriesDataPickerGroup(this);
    m_pickerLinker->addPicker(picker1);
    m_pickerLinker->addPicker(picker2);
    m_pickerLinker->addPicker(picker3);
    m_pickerLinker->addPicker(picker4);
    m_pickerLinker->addPicker(picker5);

    // Connect the group's clicked signal to display data
    connect(m_pickerLinker, &QwtPlotSeriesDataPickerGroup::clicked,
            this, &MainWindow::onPickerGroupClicked);

    // Create info label at the bottom (created once, the figure can be rebuilt)
    if (!m_clickInfoLabel) {
        m_clickInfoLabel = new QLabel("Click on a curve to see data");
        m_clickInfoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_clickInfoLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 4px; border: 1px solid #ccc; }");
        m_clickInfoLabel->setMinimumHeight(30);

        // Add the info label to the main layout (after figure)
        // Find the parent layout that contains the figure
        if (QWidget* central = this->centralWidget()) {
            if (QVBoxLayout* mainLayout = qobject_cast< QVBoxLayout* >(central->layout())) {
                mainLayout->addWidget(m_clickInfoLabel);
            }
        }
    } else {
        m_clickInfoLabel->setText("Click on a curve to see data");
    }
}

/**
 * @brief Build the 2x2 retained grid demo with draggable dividers
 * @details The layout mirrors the default signal analysis layout: one plot spanning
 *          the top row and two analysis plots below. The dividers between the tracks
 *          can be dragged once setGridEditable(true) has been called.
 */
void MainWindow::initGridDemoFigure(QwtFigure* figure)
{
    figure->setFaceColor(Qt::white);

    if (!figure->enableGrid(2, 2)) {
        return;
    }

    QwtFigureGrid* grid = figure->grid();
    // Tall waveform on top, two analysis plots below
    grid->setRowSizes(QVector< qreal >() << 0.56 << 0.44);
    grid->setColumnSizes(QVector< qreal >() << 0.50 << 0.50);
    // A visible gap between the cells makes the divider band easy to grab
    grid->setSpacing(0.01, 0.01);

    // Top row: one plot spanning both columns
    QwtPlot* wavePlot       = new QwtPlot();
    QwtPlotCurve* waveCurve = new QwtPlotCurve("Waveform");
    waveCurve->setSamples(generateSampleData(400, 1.0, 1.0));
    waveCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    waveCurve->attach(wavePlot);
    setupPlotStyle(wavePlot, "Draggable grid: waveform (row 0, columns 0-1)", QColor(52, 152, 219));
    figure->addAxes(wavePlot, 0, 0, 1, 2);

    // Bottom-left: log X sample data
    QwtPlot* logPlot       = new QwtPlot();
    QwtPlotCurve* logCurve = new QwtPlotCurve("Log X");
    logCurve->setSamples(generateLogXSampleData(200, 2.0));
    logCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    logCurve->attach(logPlot);
    setupPlotStyle(logPlot, "Draggable grid: log X (row 1, column 0)", QColor(46, 204, 113));
    figure->addAxes(logPlot, 1, 0);

    // Bottom-right: exponential sample data
    QwtPlot* expPlot       = new QwtPlot();
    QwtPlotCurve* expCurve = new QwtPlotCurve("Exponential");
    expCurve->setSamples(generateExponentialData(200, 1.0));
    expCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    expCurve->attach(expPlot);
    setupPlotStyle(expPlot, "Draggable grid: exponential (row 1, column 1)", QColor(231, 76, 60));
    figure->addAxes(expPlot, 1, 1);

    figure->replotAll();

    // The splitter is owned by the figure; it only works while the overlay does not
    // swallow the mouse events (see onActionGridDemoTriggered)
    figure->setGridEditable(true);

    if (m_clickInfoLabel) {
        m_clickInfoLabel->setText(tr("Drag the divider between the plots to resize the cells; press Esc to cancel"));
    }
}

/**
 * @brief Replace the current figure with a freshly built demo
 * @details The grid demo and the classic demo use different layout models (retained
 *          grid tracks vs. fixed normalized rectangles), so switching rebuilds the
 *          whole figure instead of re-registering the members on a used model.
 * @param gridDemo True for the 2x2 draggable grid demo, false for the classic demo
 */
void MainWindow::rebuildFigure(bool gridDemo)
{
    // The overlay is a child of the figure: drop the pointer before the figure goes away
    if (m_figureOverlay) {
        m_figureOverlay->hide();
        m_figureOverlay = nullptr;
    }
    if (m_pickerLinker) {
        delete m_pickerLinker;
        m_pickerLinker = nullptr;
    }

    QwtFigure* oldFigure = m_figure;
    m_figure             = new QwtFigure(ui->centralwidget);
    m_gridDemo           = gridDemo;

    if (gridDemo) {
        initGridDemoFigure(m_figure);
    } else {
        initFigure(m_figure);
    }

    if (QVBoxLayout* mainLayout = qobject_cast< QVBoxLayout* >(ui->centralwidget->layout())) {
        mainLayout->insertWidget(0, m_figure);
    }

    // The overlay is the editor of the normalized rectangle layout, it must not be
    // used while the retained grid tracks drive the layout
    if (m_actionResize) {
        m_actionResize->setEnabled(!gridDemo);
    }

    oldFigure->hide();
    oldFigure->deleteLater();
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
    QwtPlotScaleItem* innerScale = new QwtPlotScaleItem(QwtScaleDraw::BottomScale);
    innerScale->setScaleDivFromAxis(true);
    innerScale->attach(hostPlot);

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

    //! 在寄生绘图中添加柱状图 (QwtPlotBarChart) — 演示多类型 picker
    QwtPlotBarChart* barChart = new QwtPlotBarChart("Monthly Sales");
    QVector< QPointF > barData;
    const double sales[] = { 420, 380, 510, 470, 600, 550, 680, 720, 650, 580, 490, 530 };
    for (int i = 0; i < 12; ++i) {
        barData.append(QPointF(i, sales[ i ]));
    }
    barChart->setSamples(barData);
    barChart->setPen(QPen(QColor(52, 152, 219), 1.0));
    barChart->setBrush(QColor(52, 152, 219, 160));
    barChart->attach(parasitePlot);

    return hostPlot;
}

void MainWindow::onPickerGroupClicked(QwtPlotSeriesDataPicker* picker, const QPoint& pos)
{
    Q_UNUSED(pos);
    QList< QwtPlotSeriesDataPicker::FeaturePoint > fps = picker->featurePoints();
    if (fps.isEmpty()) {
        m_clickInfoLabel->setText("No data at click position");
        return;
    }
    QString info;
    for (const auto& fp : fps) {
        QString itemName = fp.item ? fp.item->title().text() : "Unknown";
        // Format type-specific detail from sampleData
        QString detail;
        if (fp.sampleData.isValid()) {
            if (fp.sampleData.canConvert< QwtOHLCSample >()) {
                auto s = fp.sampleData.value< QwtOHLCSample >();
                detail = QString("O:%1 H:%2 L:%3 C:%4")
                             .arg(QString::number(s.open, 'f', 2), QString::number(s.high, 'f', 2),
                                  QString::number(s.low, 'f', 2), QString::number(s.close, 'f', 2));
            } else if (fp.sampleData.canConvert< QwtIntervalSample >()) {
                auto s = fp.sampleData.value< QwtIntervalSample >();
                detail = QString("[%1, %2]").arg(QString::number(s.interval.minValue(), 'f', 1),
                                                  QString::number(s.interval.maxValue(), 'f', 1));
            } else if (fp.sampleData.canConvert< QwtBoxSample >()) {
                auto s = fp.sampleData.value< QwtBoxSample >();
                detail = QString("Med:%1 Q1:%2 Q3:%3 [%4-%5]")
                             .arg(QString::number(s.median, 'f', 1), QString::number(s.q1, 'f', 1),
                                  QString::number(s.q3, 'f', 1), QString::number(s.whiskerLower, 'f', 1),
                                  QString::number(s.whiskerUpper, 'f', 1));
            } else {
                detail = QString("(%1, %2)")
                             .arg(fp.feature.x(), 0, 'f', 2)
                             .arg(fp.feature.y(), 0, 'f', 2);
            }
        } else {
            detail = QString("(%1, %2)")
                         .arg(fp.feature.x(), 0, 'f', 2)
                         .arg(fp.feature.y(), 0, 'f', 2);
        }
        info += QString("%1: %2 [idx=%3]  ").arg(itemName, detail).arg(fp.index);
    }
    m_clickInfoLabel->setText(info);
}

void MainWindow::onActionResizeTriggered(bool on)
{
    if (!m_figureOverlay) {
        m_figureOverlay = new QwtFigureWidgetOverlay(m_figure);
        connect(m_figureOverlay,
                &QwtFigureWidgetOverlay::widgetNormGeometryChanged,
                this,
                [ this ](QWidget* w, const QRectF& oldNormGeo, const QRectF& newNormGeo) {
                    Q_UNUSED(oldNormGeo);
                    m_figure->setWidgetNormPos(w, newNormGeo);
                });
        connect(m_figureOverlay, &QwtFigureWidgetOverlay::activeWidgetChanged, this, [ this ](QWidget* oldWid, QWidget* newWid) {
            m_figure->setCurrentAxes(qobject_cast< QwtPlot* >(newWid));
        });
    }
    m_figureOverlay->setEnabled(on);
    auto active = m_figure->currentAxes();
    if (active) {
        qDebug() << "active plot is " << active->title().text();
    } else {
        qDebug() << "no active plot";
    }
    m_figureOverlay->setActiveWidget(m_figure->currentAxes());
    if (on) {
        m_figureOverlay->show();
        m_figureOverlay->raise();
    } else {
        m_figureOverlay->hide();
    }
    qDebug() << "enable figure overlay:" << on << "geo = " << m_figureOverlay->geometry()
             << ",isTransparentForMouseEvents:" << m_figureOverlay->isTransparentForMouseEvents()
             << ",isVisible:" << m_figureOverlay->isVisible() << ",isEnable:" << m_figureOverlay->isEnabled();
}

void MainWindow::onActionGridDemoTriggered(bool on)
{
    if (on) {
        // The overlay covers the whole figure and would swallow the events of the
        // plots, so the grid splitter could never see the divider bands
        if (m_actionResize && m_actionResize->isChecked()) {
            m_actionResize->setChecked(false);
            onActionResizeTriggered(false);
        }
        if (m_figureOverlay) {
            m_figureOverlay->setTransparentForMouseEvents(true);
        }
    }
    rebuildFigure(on);
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

    m_actionResize = new QAction(tr("Resize"), this);
    m_actionResize->setCheckable(true);
    connect(m_actionResize, &QAction::triggered, this, &MainWindow::onActionResizeTriggered);

    m_actionGridDemo = new QAction(tr("Grid layout (draggable)"), this);
    m_actionGridDemo->setCheckable(true);
    connect(m_actionGridDemo, &QAction::triggered, this, &MainWindow::onActionGridDemoTriggered);

    ui->toolBar->addAction(actionSave);
    ui->toolBar->addAction(actionClear);
    ui->toolBar->addAction(m_actionResize);
    ui->toolBar->addAction(m_actionGridDemo);
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
