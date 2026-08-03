#include "MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QMenu>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include "qwt3d_theme.h"
#include "qwt_figure.h"
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_scale_widget.h"

#include "DynamicSurface.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Qwt3D Dynamic Surface in QwtFigure");
    resize(1280, 800);

    createWidgets();
    createToolBar();

    // Status bar
    m_rotationLabel = new QLabel("Rotation: 30, 0, 15");
    m_zoomLabel = new QLabel("Zoom: 0.9");
    m_timeLabel = new QLabel("t = 0.000");
    statusBar()->addPermanentWidget(m_rotationLabel);
    statusBar()->addPermanentWidget(m_zoomLabel);
    statusBar()->addPermanentWidget(m_timeLabel);

    // Connect 3D plot signals to status bar
    connect(m_surfacePlot->plot(), &Qwt3DPlot::rotationChanged,
            this, &MainWindow::showRotation);
    connect(m_surfacePlot->plot(), &Qwt3DPlot::zoomChanged,
            this, &MainWindow::showZoom);
    connect(m_surfacePlot, &DynamicSurfacePlot::timeChanged, this, [this](double t) {
        m_timeLabel->setText(QString("t = %1").arg(t, 0, 'f', 3));
    });

    // Animation timer for surface data update
    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, &MainWindow::onTimerTick);

    // Auto-rotation timer
    m_rotateTimer = new QTimer(this);
    connect(m_rotateTimer, &QTimer::timeout, this, [this]() {
        double rx = m_surfacePlot->plot()->xRotation();
        double ry = m_surfacePlot->plot()->yRotation();
        m_surfacePlot->plot()->setRotation(
                int(rx + 0.5) % 360,
                int(ry + 0.5) % 360,
                int(m_surfacePlot->plot()->zRotation()));
    });

    // Initial 2D plot data
    update2DPlots();
}

MainWindow::~MainWindow() = default;

void MainWindow::createWidgets()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);

    m_figure = new QwtFigure(central);
    m_figure->setFaceColor(Qt::white);
    layout->addWidget(m_figure);

    // Create the 3D surface plot (top-left 2x2 area in a 3x3 grid)
    m_surfacePlot = new DynamicSurfacePlot();
    m_figure->addWidget(m_surfacePlot, 3, 3, 0, 0, 2, 2);

    // Create 2D cross-section plot (right column, rows 0-1)
    m_crossSectionPlot = new QwtPlot();
    m_crossSectionPlot->setTitle("Cross Section (y = 0)");
    m_crossSectionPlot->setAxisTitle(QwtPlot::xBottom, "X");
    m_crossSectionPlot->setAxisTitle(QwtPlot::yLeft, "Z");
    m_crossSectionPlot->enableAxis(QwtPlot::yRight, false);
    m_crossSectionPlot->enableAxis(QwtPlot::xTop, false);

    auto* crossGrid = new QwtPlotGrid();
    crossGrid->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
    crossGrid->attach(m_crossSectionPlot);

    m_crossSectionCurve = new QwtPlotCurve("z(x, 0)");
    m_crossSectionCurve->setPen(QPen(QColor(0, 100, 200), 2));
    m_crossSectionCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    m_crossSectionCurve->attach(m_crossSectionPlot);

    m_crossSectionPlot->setAxisScale(QwtPlot::xBottom, -DynamicSurfacePlot::DOMAIN_EXTENT,
                                     DynamicSurfacePlot::DOMAIN_EXTENT);
    m_crossSectionPlot->setAxisScale(QwtPlot::yLeft, -1.5, 1.5);
    m_crossSectionPlot->replot();

    m_figure->addGridAxes(m_crossSectionPlot, 3, 3, 0, 2, 2, 1);

    // Create 2D time-series plot (bottom row, spanning all columns)
    m_timeSeriesPlot = new QwtPlot();
    m_timeSeriesPlot->setTitle("Z(0, 0) Over Time");
    m_timeSeriesPlot->setAxisTitle(QwtPlot::xBottom, "Time (t)");
    m_timeSeriesPlot->setAxisTitle(QwtPlot::yLeft, "Z");
    m_timeSeriesPlot->enableAxis(QwtPlot::yRight, false);
    m_timeSeriesPlot->enableAxis(QwtPlot::xTop, false);

    auto* tsGrid = new QwtPlotGrid();
    tsGrid->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
    tsGrid->attach(m_timeSeriesPlot);

    m_timeSeriesCurve = new QwtPlotCurve("Z(0,0)");
    m_timeSeriesCurve->setPen(QPen(QColor(200, 50, 50), 2));
    m_timeSeriesCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    m_timeSeriesCurve->attach(m_timeSeriesPlot);

    m_timeSeriesPlot->setAxisScale(QwtPlot::yLeft, -1.5, 1.5);
    m_timeSeriesPlot->replot();

    m_figure->addGridAxes(m_timeSeriesPlot, 3, 3, 2, 0, 1, 3);
}

void MainWindow::createToolBar()
{
    auto* toolbar = addToolBar("Controls");
    toolbar->setMovable(false);

    // Animation toggle (auto-start deferred until event loop runs, after GL context is ready)
    auto* animAction = toolbar->addAction("Animate Surface");
    animAction->setCheckable(true);
    connect(animAction, &QAction::toggled, this, &MainWindow::onAnimationToggled);
    QTimer::singleShot(0, animAction, [animAction]() { animAction->setChecked(true); });

    // Auto-rotate toggle
    auto* rotateAction = toolbar->addAction("Auto Rotate");
    rotateAction->setCheckable(true);
    connect(rotateAction, &QAction::toggled, this, &MainWindow::onAutoRotateToggled);

    toolbar->addSeparator();

    // Theme selector
    auto* themeLabel = new QLabel("Theme:");
    toolbar->addWidget(themeLabel);
    m_themeCombo = new QComboBox();
    m_themeCombo->addItems(Qwt3DTheme::availablePresets());
    toolbar->addWidget(m_themeCombo);
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onThemeChanged);

    toolbar->addSeparator();

    // Plot style group
    auto* plotStyleGroup = createPlotStyleGroup();
    auto* plotStyleMenu = toolbar->addAction("Plot Style");
    plotStyleMenu->setMenu(new QMenu(toolbar));
    for (auto* action : plotStyleGroup->actions())
        plotStyleMenu->menu()->addAction(action);
    connect(plotStyleGroup, &QActionGroup::triggered,
            this, &MainWindow::onPlotStyleTriggered);

    // Floor style group
    auto* floorStyleGroup = createFloorStyleGroup();
    auto* floorStyleMenu = toolbar->addAction("Floor Style");
    floorStyleMenu->setMenu(new QMenu(toolbar));
    for (auto* action : floorStyleGroup->actions())
        floorStyleMenu->menu()->addAction(action);
    connect(floorStyleGroup, &QActionGroup::triggered,
            this, &MainWindow::onFloorStyleTriggered);

    // Coordinate style group
    auto* coordStyleGroup = createCoordStyleGroup();
    auto* coordStyleMenu = toolbar->addAction("Coord Style");
    coordStyleMenu->setMenu(new QMenu(toolbar));
    for (auto* action : coordStyleGroup->actions())
        coordStyleMenu->menu()->addAction(action);
    connect(coordStyleGroup, &QActionGroup::triggered,
            this, &MainWindow::onCoordStyleTriggered);

    toolbar->addSeparator();

    // Lighting toggle
    auto* lightAction = toolbar->addAction("Lighting");
    lightAction->setCheckable(true);
    lightAction->setChecked(true);
    connect(lightAction, &QAction::toggled, this, &MainWindow::onLightingToggled);

    // Legend toggle
    auto* legendAction = toolbar->addAction("Color Legend");
    legendAction->setCheckable(true);
    legendAction->setChecked(true);
    connect(legendAction, &QAction::toggled, this, &MainWindow::onLegendToggled);

    // Ortho/Perspective toggle
    auto* orthoAction = toolbar->addAction("Orthographic");
    orthoAction->setCheckable(true);
    orthoAction->setChecked(false);
    connect(orthoAction, &QAction::toggled, this, &MainWindow::onOrthoToggled);

    toolbar->addSeparator();

    // Reset view
    toolbar->addAction("Reset View", this, &MainWindow::onResetView);

    // Save figure
    toolbar->addAction("Save Figure", this, &MainWindow::onSaveFigure);
}

QActionGroup* MainWindow::createPlotStyleGroup()
{
    auto* group = new QActionGroup(this);
    group->setExclusive(true);

    auto* wireframe = group->addAction("Wireframe");
    wireframe->setCheckable(true);
    wireframe->setData(int(WIREFRAME));

    auto* filled = group->addAction("Filled");
    filled->setCheckable(true);
    filled->setChecked(true);
    filled->setData(int(FILLED));

    auto* filledMesh = group->addAction("FilledMesh");
    filledMesh->setCheckable(true);
    filledMesh->setData(int(FILLEDMESH));

    auto* hiddenLine = group->addAction("HiddenLine");
    hiddenLine->setCheckable(true);
    hiddenLine->setData(int(HIDDENLINE));

    auto* points = group->addAction("Points");
    points->setCheckable(true);
    points->setData(int(QWT3D_POINTS));

    return group;
}

QActionGroup* MainWindow::createFloorStyleGroup()
{
    auto* group = new QActionGroup(this);
    group->setExclusive(true);

    auto* none = group->addAction("No Floor");
    none->setCheckable(true);
    none->setData(int(NOFLOOR));

    auto* iso = group->addAction("Floor Iso");
    iso->setCheckable(true);
    iso->setChecked(true);
    iso->setData(int(FLOORISO));

    auto* data = group->addAction("Floor Data");
    data->setCheckable(true);
    data->setData(int(FLOORDATA));

    return group;
}

QActionGroup* MainWindow::createCoordStyleGroup()
{
    auto* group = new QActionGroup(this);
    group->setExclusive(true);

    auto* box = group->addAction("Box");
    box->setCheckable(true);
    box->setChecked(true);
    box->setData(int(BOX));

    auto* frame = group->addAction("Frame");
    frame->setCheckable(true);
    frame->setData(int(FRAME));

    auto* noCoord = group->addAction("None");
    noCoord->setCheckable(true);
    noCoord->setData(int(NOCOORD));

    return group;
}

void MainWindow::onAnimationToggled(bool on)
{
    if (on)
        m_animTimer->start(30);
    else
        m_animTimer->stop();
}

void MainWindow::onAutoRotateToggled(bool on)
{
    if (on)
        m_rotateTimer->start(30);
    else
        m_rotateTimer->stop();
}

void MainWindow::onTimerTick()
{
    m_surfacePlot->advanceTime(0.05);
    update2DPlots();
}

void MainWindow::update2DPlots()
{
    // Update cross-section plot
    QVector<double> xData, zData;
    m_surfacePlot->crossSectionY0(xData, zData);
    m_crossSectionCurve->setSamples(xData, zData);
    m_crossSectionPlot->replot();

    // Update time-series plot (ring buffer)
    double t = m_surfacePlot->time();
    double z0 = m_surfacePlot->zAtOrigin();

    m_timeData.append(t);
    m_zData.append(z0);

    if (m_timeData.size() > TIME_SERIES_MAX) {
        m_timeData.removeFirst();
        m_zData.removeFirst();
    }

    m_timeSeriesCurve->setSamples(m_timeData, m_zData);

    // Adjust x-axis to show sliding window
    if (!m_timeData.isEmpty()) {
        double tMin = m_timeData.first();
        double tMax = m_timeData.last();
        if (tMax - tMin < 1.0)
            tMax = tMin + 1.0;
        m_timeSeriesPlot->setAxisScale(QwtPlot::xBottom, tMin, tMax);
    }
    m_timeSeriesPlot->replot();
}

void MainWindow::onPlotStyleTriggered(QAction* action)
{
    int val = action->data().toInt();
    m_surfacePlot->surface()->setPlotStyle(static_cast<PLOTSTYLE>(val));
    m_surfacePlot->plot()->update();
}

void MainWindow::onFloorStyleTriggered(QAction* action)
{
    int val = action->data().toInt();
    m_surfacePlot->surface()->setFloorStyle(static_cast<FLOORSTYLE>(val));
    m_surfacePlot->plot()->update();
}

void MainWindow::onCoordStyleTriggered(QAction* action)
{
    int val = action->data().toInt();
    m_surfacePlot->plot()->coordinates()->setStyle(static_cast<COORDSTYLE>(val));
    m_surfacePlot->plot()->update();
}

void MainWindow::onLightingToggled(bool on)
{
    m_surfacePlot->plot()->enableLighting(on);
    m_surfacePlot->plot()->update();
}

void MainWindow::onLegendToggled(bool on)
{
    m_surfacePlot->plot()->showColorLegend(on);
    m_surfacePlot->plot()->update();
}

void MainWindow::onOrthoToggled(bool on)
{
    m_surfacePlot->plot()->setOrtho(on);
}

void MainWindow::onThemeChanged(int index)
{
    const QString name = m_themeCombo->itemText(index);
    m_surfacePlot->plot()->applyTheme(name);
    m_surfacePlot->plot()->update();
}

void MainWindow::onResetView()
{
    m_surfacePlot->resetView();
}

void MainWindow::onSaveFigure()
{
    QString fileName = QFileDialog::getSaveFileName(
            this, "Save Figure", "figure_surface_3d.png",
            "PNG Files (*.png);;JPEG Files (*.jpg);;All Files (*)");
    if (!fileName.isEmpty())
        m_figure->saveFig(fileName, 150);
}

void MainWindow::showRotation(double x, double y, double z)
{
    m_rotationLabel->setText(QString("Rotation: %1, %2, %3")
                                     .arg(x, 0, 'f', 1)
                                     .arg(y, 0, 'f', 1)
                                     .arg(z, 0, 'f', 1));
}

void MainWindow::showZoom(double z)
{
    m_zoomLabel->setText(QString("Zoom: %1").arg(z, 0, 'f', 2));
}
