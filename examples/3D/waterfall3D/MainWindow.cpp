#include "MainWindow.h"
#include "SpectrumGenerator.h"

#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QElapsedTimer>
#include <QLabel>
#include <QQueue>
#include <QPixmap>
#include <QSlider>
#include <QSplitter>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>

#include <functional>

#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"

#include "qwt3d_colormap_color.h"
#include "qwt3d_coordsys.h"
#include "qwt3d_plot.h"
#include "qwt3d_scale.h"
#include "qwt3d_theme.h"
#include "qwt3d_types.h"

namespace
{

// Timer interval. 33 ms targets ~30 fps but interacts badly with the
// compositor vsync period (a paint that misses the swap deadline stalls a
// full extra vsync, dropping the loop to ~20 fps). 16 ms lets the loop ride
// the vsync cadence; the waterfall then updates at up to 60 Hz.
const int TIMER_INTERVAL_MS = 16;

// Refresh the 2D top plot every Nth frame. Keep at 1 so both views always
// show the identical frame; raise only on machines where the 2D replot
// becomes the bottleneck.
const int TOP_PLOT_EVERY_NTH_FRAME = 1;

// Default sliding window size
const int DEFAULT_CAPACITY = 64;

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_generator(new SpectrumGenerator(256))
{
    setWindowTitle("Qwt3D Waterfall - live spectrum");
    resize(1000, 720);

    m_splitter = new QSplitter(Qt::Vertical, this);
    m_splitter->addWidget(createTopPlot());
    m_splitter->addWidget(createBottomPlot());
    m_splitter->setSizes({ 300, 700 });
    setCentralWidget(m_splitter);

    createToolbar();
    createStatusBar();

    setupWaterfallDefaults();

    m_timer = new QTimer(this);
    m_timer->setInterval(TIMER_INTERVAL_MS);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::onTick);

    // Pre-fill the window so the 3D view starts full instead of crawling
    // up from a single trace.
    fillWindow();

    m_fpsEpochMs = QDateTime::currentMSecsSinceEpoch();
    m_timer->start();
}

MainWindow::~MainWindow()
{
    delete m_generator;
}

QWidget* MainWindow::createTopPlot()
{
    m_topPlot = new QwtPlot();
    m_topPlot->setTitle("Live spectrum");
    m_topPlot->setAxisTitle(QwtPlot::xBottom, "Bin");
    m_topPlot->setAxisTitle(QwtPlot::yLeft, "Amplitude");

    auto* grid = new QwtPlotGrid();
    grid->setMajorPen(Qt::gray, 0.0, Qt::DotLine);
    grid->attach(m_topPlot);

    m_spectrumCurve = new QwtPlotCurve("Spectrum");
    m_spectrumCurve->setPen(QPen(QColor(40, 80, 220), 2.0));
    m_spectrumCurve->setBrush(QBrush(QColor(80, 120, 255, 70)));
    m_spectrumCurve->setBaseline(0.0);
    m_spectrumCurve->attach(m_topPlot);

    m_peakCurve = new QwtPlotCurve("Peak hold");
    QPen peakPen(QColor(190, 60, 60), 1.0);
    peakPen.setStyle(Qt::DashLine);
    m_peakCurve->setPen(peakPen);
    m_peakCurve->attach(m_topPlot);

    m_topPlot->setAxisScale(QwtPlot::yLeft, 0.0, 1.05);
    m_topPlot->setAxisAutoScale(QwtPlot::xBottom, true);

    return m_topPlot;
}

QWidget* MainWindow::createBottomPlot()
{
    m_plot3d = new Qwt3DPlot();
    m_plot3d->setWindowTitle("3D waterfall");

    m_waterfall = new Qwt3DWaterfall();
    m_waterfall->attach(m_plot3d);
    m_waterfall->setTraceCapacity(DEFAULT_CAPACITY);

    m_plot3d->showColorLegend(true);
    // Label the legend with the amplitude window instead of color indices
    m_plot3d->legend()->setLimits(0.0, 1.0);
    m_plot3d->enableMouse(true);
    m_plot3d->enableKeyboard(true);

    // Axis convention: x = bin, y = depth (trace index), z = amplitude
    auto* coords = m_plot3d->coordinates();
    coords->axes[ X1 ].setLabelString("Bin");
    coords->axes[ Y1 ].setLabelString("Frames ago");
    coords->axes[ Z1 ].setLabelString("Amplitude");
    for (unsigned i = 0; i < coords->axes.size(); ++i) {
        coords->axes[ i ].setMajors(5);
        coords->axes[ i ].setMinors(2);
    }

    return m_plot3d;
}

void MainWindow::setupWaterfallDefaults()
{
    m_waterfall->setPlotStyle(Qwt3DWaterfall::RibbonMesh);
    m_waterfall->setColorMode(Qwt3DWaterfall::ByAmplitude);
    m_waterfall->setColorRange(0.0, 1.0);

    onResetView();
    rebuildDepthAxisLabels();
}

void MainWindow::rebuildDepthAxisLabels()
{
    // Depth axis: Qwt3DTextScale labels are "frames ago" values at the
    // (autoscale-free) major tic positions. setScale() clones the scale and
    // consumes the passed object, so every axis needs its own instance.
    const int capacity = m_waterfall->traceCapacity();
    const int majors = 4;

    QVector<QString> labels;
    for (int k = 0; k <= majors; ++k) {
        const double tic = double(k) * (capacity - 1) / majors;
        const int framesAgo = qRound(capacity - 1 - tic);
        labels << QString::number(-framesAgo);
    }

    auto* coords = m_plot3d->coordinates();
    for (int axis : { Y1, Y2, Y3, Y4 }) {
        auto& a = coords->axes[ static_cast< size_t >(axis) ];
        a.setAutoScale(false);
        a.setMajors(majors);
        auto* scale = new Qwt3DTextScale();
        scale->setLabels(labels);
        a.setScale(scale);
    }
}

void MainWindow::fillWindow()
{
    m_generator->reset();
    m_waterfall->clear();
    for (int i = 0; i < m_waterfall->traceCapacity(); ++i)
        m_waterfall->pushTrace(m_generator->nextFrame());
}

void MainWindow::createToolbar()
{
    auto* tb = addToolBar("Controls");
    tb->setMovable(false);

    // Run / pause
    auto* runAction = tb->addAction("Pause");
    runAction->setCheckable(true);
    runAction->setChecked(true);
    connect(runAction, &QAction::toggled, this, &MainWindow::onRunToggled);

    tb->addAction("Reset", this, &MainWindow::onReset);
    tb->addSeparator();

    // Window size
    tb->addWidget(new QLabel("Window: ", tb));
    m_capacityCombo = new QComboBox(tb);
    for (int capacity : { 16, 32, 64, 128 })
        m_capacityCombo->addItem(QString::number(capacity), capacity);
    m_capacityCombo->setCurrentIndex(2);  // 64
    connect(m_capacityCombo, QOverload< int >::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::onWindowCapacityChanged);
    tb->addWidget(m_capacityCombo);

    // Geometry style
    tb->addWidget(new QLabel("Style: ", tb));
    m_styleCombo = new QComboBox(tb);
    m_styleCombo->addItem("Ribbon");
    m_styleCombo->addItem("RibbonMesh");
    m_styleCombo->addItem("Wireframe");
    m_styleCombo->addItem("Tube");
    m_styleCombo->setCurrentIndex(1);
    connect(m_styleCombo, QOverload< int >::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::onStyleChanged);
    tb->addWidget(m_styleCombo);

    // Colormap
    tb->addWidget(new QLabel("Colormap: ", tb));
    m_cmapCombo = new QComboBox(tb);
    for (const char* preset : { "viridis", "plasma", "inferno", "turbo", "jet", "hot", "cool" })
        m_cmapCombo->addItem(QString::fromLatin1(preset));
    connect(m_cmapCombo, QOverload< int >::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::onColorMapChanged);
    tb->addWidget(m_cmapCombo);

    // Color mode
    tb->addWidget(new QLabel("Color by: ", tb));
    m_colorModeCombo = new QComboBox(tb);
    m_colorModeCombo->addItem("Depth");
    m_colorModeCombo->addItem("Amplitude");
    m_colorModeCombo->addItem("Solid");
    m_colorModeCombo->setCurrentIndex(1);
    connect(m_colorModeCombo, QOverload< int >::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::onColorModeChanged);
    tb->addWidget(m_colorModeCombo);

    tb->addSeparator();

    // Opacity
    tb->addWidget(new QLabel("Opacity: ", tb));
    m_opacitySlider = new QSlider(Qt::Horizontal, tb);
    m_opacitySlider->setRange(20, 100);
    m_opacitySlider->setValue(100);
    m_opacitySlider->setMinimumWidth(120);
    connect(m_opacitySlider, &QSlider::valueChanged, this, &MainWindow::onOpacityChanged);
    tb->addWidget(m_opacitySlider);

    tb->addSeparator();

    // View
    tb->addAction("Reset view", this, &MainWindow::onResetView);
    tb->addAction("Screenshot", this, [this]() {
        const QString name = QString("waterfall3D_%1.png")
            .arg(QDateTime::currentDateTime().toString("hhmmss"));
        if (m_plot3d->grab().save(name))
            m_statusLabel->setText(QString("Saved %1").arg(name));
    });
}

void MainWindow::createStatusBar()
{
    m_statusLabel = new QLabel(this);
    statusBar()->addWidget(m_statusLabel);
    m_statusLabel->setText(statusLine());
}

void MainWindow::onTick()
{
    const QVector<double>& frame = m_generator->nextFrame();

    // Top: live spectrum (and its peak hold); decoupled from the 3D repaint
    if (m_frameCounter % TOP_PLOT_EVERY_NTH_FRAME == 0) {
        m_spectrumCurve->setSamples(m_generator->binValues(), frame);
        m_peakCurve->setSamples(m_generator->binValues(), m_generator->peakHold());
        m_topPlot->replot();
    }

    // Bottom: pushTrace() triggers itemChanged() -> Qwt3DPlot::update();
    // the whole window shifts, the view rotation/zoom is untouched.
    m_waterfall->pushTrace(frame);

    // FPS accounting over a one second epoch
    ++m_frameCounter;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 elapsed = now - m_fpsEpochMs;
    if (elapsed >= 1000) {
        m_fps = 1000.0 * m_frameCounter / elapsed;
        m_frameCounter = 0;
        m_fpsEpochMs = now;
    }
    updateStatusBar();
}

void MainWindow::onRunToggled(bool running)
{
    if (running)
        m_timer->start();
    else
        m_timer->stop();

    updateStatusBar();
}

void MainWindow::onReset()
{
    fillWindow();
    updateStatusBar();
}

void MainWindow::onWindowCapacityChanged(int index)
{
    const int capacity = m_capacityCombo->itemData(index).toInt();
    m_waterfall->setTraceCapacity(capacity);
    rebuildDepthAxisLabels();
    fillWindow();
    onResetView();  // the depth span changed, keep the view proportional
}

void MainWindow::onStyleChanged(int index)
{
    m_waterfall->setPlotStyle(static_cast< Qwt3DWaterfall::PlotStyle >(index));
}

void MainWindow::onColorMapChanged(int index)
{
    const QString preset = m_cmapCombo->itemText(index);
    m_waterfall->setDataColor(new Qwt3DColorMapColor(preset));
}

void MainWindow::onColorModeChanged(int index)
{
    m_waterfall->setColorMode(static_cast< Qwt3DWaterfall::ColorMode >(index));
}

void MainWindow::onOpacityChanged(int value)
{
    m_waterfall->setOpacity(value / 100.0);
}

void MainWindow::onResetView()
{
    // Default waterfall view: tilted so curtains stay readable.
    //
    // DATARATIO keeps true data proportions, and the raw ranges are strongly
    // anisotropic (bins: 0..255, amplitude: 0..1, depth: 0..capacity-1), so
    // the per-axis scales normalize x to the depth span and exaggerate the
    // amplitude axis - the classic waterfall look. The z (depth) axis is
    // compressed to about a third of the x span.
    // AUTOFILL normalizes every axis to the largest data range BEFORE the
    // projection radius is derived, which is what strongly anisotropic
    // waterfall data (bins 0..255, amplitude 0..1, depth 0..capacity-1)
    // needs; under DATARATIO the per-axis setScale() below would shrink the
    // geometry away from a projection sized for the raw ranges. The scales
    // then shape the normalized cube: full bin width, a depth compressed to
    // about a third and 60% amplitude height - the classic waterfall
    // proportions (x = bin, y = depth, z = amplitude).
    m_plot3d->setRotation(55.0, 0.0, 25.0);
    m_plot3d->setScale(1.0, 0.35, 0.6);
    m_plot3d->setZoom(1.1);
    m_plot3d->setViewportShift(0.0, 0.0);
    m_plot3d->setAspectRatioMode(AUTOFILL);
}

void MainWindow::runSelfTest(const QString& screenshotDir)
{
    // Automation hook: cycle through styles, color modes, window sizes,
    // opacity, pause/resume and reset, saving a screenshot of every stage.
    // Exits the application when done - used by docs/CI pipelines.
    struct Stage
    {
        const char* name;
        std::function<void()> apply;
    };

    QQueue< Stage > stages;
    const auto shot = [this, screenshotDir](const QString& name) {
        if (!screenshotDir.isEmpty())
            grab().save(QString("%1/waterfall3D_%2.png").arg(screenshotDir, name));
    };

    stages.enqueue({ "ribbon", [this]() {
        m_styleCombo->setCurrentIndex(0);
    } });
    stages.enqueue({ "wireframe", [this]() {
        m_styleCombo->setCurrentIndex(2);
    } });
    stages.enqueue({ "tube", [this]() {
        m_styleCombo->setCurrentIndex(3);
    } });
    stages.enqueue({ "bydepth", [this]() {
        m_styleCombo->setCurrentIndex(1);
        m_colorModeCombo->setCurrentIndex(0);
    } });
    stages.enqueue({ "solid", [this]() {
        m_colorModeCombo->setCurrentIndex(2);
    } });
    stages.enqueue({ "amplitude", [this]() {
        m_colorModeCombo->setCurrentIndex(1);
    } });
    stages.enqueue({ "win16", [this]() {
        m_capacityCombo->setCurrentIndex(0);
    } });
    stages.enqueue({ "win128", [this]() {
        m_capacityCombo->setCurrentIndex(3);
    } });
    stages.enqueue({ "opacity50", [this]() {
        m_opacitySlider->setValue(50);
    } });
    stages.enqueue({ "pause", [this]() {
        // Pause / resume toggling through the toolbar action
        auto* tb = findChild< QToolBar* >();
        if (tb) {
            for (QAction* a : tb->actions()) {
                if (a->isCheckable() && !a->text().isEmpty()) {
                    a->setChecked(false);
                    a->setChecked(true);
                }
            }
        }
        onReset();
    } });

    auto* driver = new QTimer(this);
    const int warmupMs = 4000;   // fill the window and let the fps meter settle
    const int stageMs = 2500;
    int elapsed = 0;
    driver->setInterval(1000);
    connect(driver, &QTimer::timeout, this, [=]() mutable {
        elapsed += 1000;
        if (elapsed < warmupMs)
            return;
        if (stages.isEmpty()) {
            driver->stop();
            driver->deleteLater();
            QApplication::quit();
            return;
        }
        const Stage stage = stages.dequeue();
        stage.apply();
        QTimer::singleShot(stageMs, this, [shot, name = stage.name]() { shot(QString::fromLatin1(name)); });
    });
    driver->start();
}

QString MainWindow::statusLine() const
{
    if (!m_timer || !m_waterfall)
        return QString();

    const bool running = m_timer->isActive();
    return QString("%1   %2 fps   %3/%4 traces   %5 bins")
        .arg(running ? "Running" : "Paused")
        .arg(QString::number(m_fps, 'f', 1))
        .arg(m_waterfall->traceCount())
        .arg(m_waterfall->traceCapacity())
        .arg(m_waterfall->samplesPerTrace());
}

void MainWindow::updateStatusBar()
{
    m_statusLabel->setText(statusLine());
}
