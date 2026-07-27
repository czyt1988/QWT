#include "MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>

#include "SurfaceData.h"
#include "SurfaceSettingsDock.h"

#include "qwt3d_coordsys.h"
#include "qwt3d_plot.h"
#include "qwt3d_surface.h"
#include "qwt3d_theme.h"
#include "qwt3d_types.h"

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Qwt3D Plot — Comprehensive Demo"));
    resize(1280, 800);

    createWidgets();
    createMenuBar();
    createToolBar();

    // Status bar
    m_rotationLabel = new QLabel(QStringLiteral("Rotation: 30, 0, 15"));
    m_zoomLabel = new QLabel(QStringLiteral("Zoom: 0.9"));
    m_dataLabel = new QLabel(QStringLiteral("Data: Ripple"));
    statusBar()->addPermanentWidget(m_rotationLabel);
    statusBar()->addPermanentWidget(m_zoomLabel);
    statusBar()->addPermanentWidget(m_dataLabel);

    // Connect 3D plot signals to status bar
    connect(m_plot, &Qwt3DPlot::rotationChanged, this, &MainWindow::showRotation);
    connect(m_plot, &Qwt3DPlot::zoomChanged, this, &MainWindow::showZoom);

    // Animation timer
    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(30);
    connect(m_animTimer, &QTimer::timeout, this, &MainWindow::onTimerTick);

    // Load initial data
    switchData(0);

    // Apply initial theme
    m_plot->applyTheme(Qwt3DTheme::Default);
    m_plot->update();
}

MainWindow::~MainWindow() = default;

// ---------------------------------------------------------------------------
// Create widgets
// ---------------------------------------------------------------------------
void MainWindow::createWidgets()
{
    // Create the 3D plot as the central widget
    m_plot = new Qwt3DPlot(this);
    setCentralWidget(m_plot);

    // Create the surface item
    m_surface = new Qwt3DSurface();
    m_surface->attach(m_plot);

    // Configure default plot settings
    m_surface->setPlotStyle(FILLEDMESH);
    m_surface->setShading(GOURAUD);
    m_surface->setFloorStyle(FLOORISO);
    m_surface->setPolygonOffset(0.8);
    m_surface->setMeshColor(RGBA(0.0, 0.0, 0.0, 0.15));
    m_surface->setMeshLineWidth(0.5);

    // Configure lighting
    m_plot->enableLighting(true);
    m_plot->illuminate(0);
    m_plot->setLightRotation(20.0, 30.0, 0.0, 0);
    m_plot->setShininess(10.0);

    // Configure background and title
    m_plot->setBackgroundColor(RGBA(1.0, 1.0, 1.0, 1.0));
    m_plot->setTitle(QStringLiteral("Qwt3D Surface Plot"));
    m_plot->setTitleFont(QStringLiteral("Arial"), 12, QFont::Bold);
    m_plot->setTitleColor(RGBA(0.1, 0.1, 0.1, 1.0));

    // Configure coordinate system
    m_plot->coordinates()->setStyle(BOX);

    // Configure axes labels
    m_plot->coordinates()->axes[X1].setLabelString(QStringLiteral("X"));
    m_plot->coordinates()->axes[Y1].setLabelString(QStringLiteral("Y"));
    m_plot->coordinates()->axes[Z1].setLabelString(QStringLiteral("Z"));

    // Enable interaction
    m_plot->enableMouse(true);
    m_plot->enableKeyboard(true);

    // Set initial view
    m_plot->setRotation(30, 0, 15);
    m_plot->setViewportShift(0.05, 0);
    m_plot->setScale(1, 1, 1);
    m_plot->setZoom(0.9);

    // Show color legend
    m_plot->showColorLegend(true);

    // Create the settings dock
    m_dock = new SurfaceSettingsDock(this);
    m_dock->setPlot(m_plot);
    m_dock->setSurface(m_surface);
    addDockWidget(Qt::LeftDockWidgetArea, m_dock);
}

// ---------------------------------------------------------------------------
// Create menu bar
// ---------------------------------------------------------------------------
void MainWindow::createMenuBar()
{
    // --- File menu ---
    auto* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));

    fileMenu->addAction(QStringLiteral("Save Pixmap..."), this, &MainWindow::onSavePixmap);

    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("E&xit"), this, &QWidget::close, QKeySequence::Quit);

    // --- View menu ---
    auto* viewMenu = menuBar()->addMenu(QStringLiteral("&View"));

    viewMenu->addAction(QStringLiteral("Reset View"), this, &MainWindow::onResetView);

    auto* mouseAction = viewMenu->addAction(QStringLiteral("Mouse Input"));
    mouseAction->setCheckable(true);
    mouseAction->setChecked(true);
    connect(mouseAction, &QAction::toggled, this, &MainWindow::onToggleMouse);

    auto* keyboardAction = viewMenu->addAction(QStringLiteral("Keyboard Input"));
    keyboardAction->setCheckable(true);
    keyboardAction->setChecked(true);
    connect(keyboardAction, &QAction::toggled, this, &MainWindow::onToggleKeyboard);

    auto* orthoAction = viewMenu->addAction(QStringLiteral("Orthographic Projection"));
    orthoAction->setCheckable(true);
    connect(orthoAction, &QAction::toggled, this, &MainWindow::onOrthoToggled);

    // --- Help menu ---
    auto* helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));
    helpMenu->addAction(QStringLiteral("About Qwt3D"), [this]() {
        statusBar()->showMessage(QStringLiteral("Qwt3D comprehensive property demo — all settable APIs exposed"), 5000);
    });
}

// ---------------------------------------------------------------------------
// Create toolbar
// ---------------------------------------------------------------------------
void MainWindow::createToolBar()
{
    auto* toolbar = addToolBar(QStringLiteral("Controls"));
    toolbar->setMovable(false);

    // --- Data source ---
    toolbar->addWidget(new QLabel(QStringLiteral("Data: ")));
    m_dataCombo = new QComboBox;
    m_dataCombo->addItems({QStringLiteral("Ripple"), QStringLiteral("Gaussian"),
                           QStringLiteral("Saddle"), QStringLiteral("Sombrero"),
                           QStringLiteral("Torus")});
    connect(m_dataCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onDataChanged);
    toolbar->addWidget(m_dataCombo);

    // Animate toggle
    m_animateAction = toolbar->addAction(QStringLiteral("Animate"));
    m_animateAction->setCheckable(true);
    connect(m_animateAction, &QAction::toggled, this, &MainWindow::onAnimateToggled);

    toolbar->addSeparator();

    // --- Coord style ---
    auto* coordGroup = new QActionGroup(this);
    coordGroup->setExclusive(true);

    auto* boxAction = coordGroup->addAction(QStringLiteral("Box"));
    boxAction->setCheckable(true);
    boxAction->setChecked(true);
    boxAction->setData(int(BOX));

    auto* frameAction = coordGroup->addAction(QStringLiteral("Frame"));
    frameAction->setCheckable(true);
    frameAction->setData(int(FRAME));

    auto* noneAction = coordGroup->addAction(QStringLiteral("None"));
    noneAction->setCheckable(true);
    noneAction->setData(int(NOCOORD));

    toolbar->addAction(boxAction);
    toolbar->addAction(frameAction);
    toolbar->addAction(noneAction);
    connect(coordGroup, &QActionGroup::triggered, this, &MainWindow::onCoordStyleTriggered);

    toolbar->addSeparator();

    // --- Theme ---
    toolbar->addWidget(new QLabel(QStringLiteral("Theme: ")));
    m_themeCombo = new QComboBox;
    m_themeCombo->addItems(Qwt3DTheme::availablePresets());
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onThemeChanged);
    toolbar->addWidget(m_themeCombo);

    toolbar->addSeparator();

    // --- Toggles ---
    auto* lightAction = toolbar->addAction(QStringLiteral("Lighting"));
    lightAction->setCheckable(true);
    lightAction->setChecked(true);
    connect(lightAction, &QAction::toggled, this, &MainWindow::onLightingToggled);

    auto* legendAction = toolbar->addAction(QStringLiteral("Legend"));
    legendAction->setCheckable(true);
    legendAction->setChecked(true);
    connect(legendAction, &QAction::toggled, this, &MainWindow::onLegendToggled);

    auto* orthoAction = toolbar->addAction(QStringLiteral("Ortho"));
    orthoAction->setCheckable(true);
    connect(orthoAction, &QAction::toggled, this, &MainWindow::onOrthoToggled);

    auto* gridAction = toolbar->addAction(QStringLiteral("Grid"));
    gridAction->setCheckable(true);
    connect(gridAction, &QAction::toggled, this, &MainWindow::onGridToggled);

    toolbar->addSeparator();

    // --- Actions ---
    toolbar->addAction(QStringLiteral("Reset View"), this, &MainWindow::onResetView);
    toolbar->addAction(QStringLiteral("Save..."), this, &MainWindow::onSavePixmap);
}

// ---------------------------------------------------------------------------
// Data switching
// ---------------------------------------------------------------------------
void MainWindow::switchData(int index)
{
    // Stop animation if previous data was animated
    if (m_animTimer->isActive())
        m_animTimer->stop();

    static RippleProvider s_ripple;
    static GaussianProvider s_gaussian;
    static SaddleProvider s_saddle;
    static SombreroProvider s_sombrero;
    static TorusProvider s_torus;

    static SurfaceDataProvider* providers[] = {
        &s_ripple, &s_gaussian, &s_saddle, &s_sombrero, &s_torus
    };

    if (index < 0 || index >= 5)
        return;

    m_currentDataIndex = index;
    auto* provider = providers[index];
    m_currentProvider = provider;
    provider->create(*m_surface);

    // Update axes labels based on data type
    if (index == 4) { // Torus
        m_plot->coordinates()->axes[X1].setLabelString(QStringLiteral("X"));
        m_plot->coordinates()->axes[Y1].setLabelString(QStringLiteral("Y"));
        m_plot->coordinates()->axes[Z1].setLabelString(QStringLiteral("Z"));
    } else {
        m_plot->coordinates()->axes[X1].setLabelString(QStringLiteral("X"));
        m_plot->coordinates()->axes[Y1].setLabelString(QStringLiteral("Y"));
        m_plot->coordinates()->axes[Z1].setLabelString(QStringLiteral("Z"));
    }

    // Re-apply dock settings after data switch
    m_dock->reapplyAll();

    // Enable/disable animate button
    m_animateAction->setEnabled(provider->isAnimated());
    if (!provider->isAnimated())
        m_animateAction->setChecked(false);

    m_dataLabel->setText(QStringLiteral("Data: %1").arg(provider->name()));
    m_plot->update();
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------
void MainWindow::onDataChanged(int index)
{
    switchData(index);
}

void MainWindow::onAnimateToggled(bool on)
{
    if (on && m_currentProvider && m_currentProvider->isAnimated())
        m_animTimer->start();
    else
        m_animTimer->stop();
}

void MainWindow::onTimerTick()
{
    if (m_currentProvider && m_currentProvider->isAnimated()) {
        m_currentProvider->advance(0.05);
        m_currentProvider->create(*m_surface);
        m_plot->update();
    }
}

void MainWindow::onCoordStyleTriggered(QAction* action)
{
    int val = action->data().toInt();
    m_plot->coordinates()->setStyle(static_cast<COORDSTYLE>(val));
    m_plot->update();
}

void MainWindow::onThemeChanged(int index)
{
    const QString name = m_themeCombo->itemText(index);
    m_plot->applyTheme(name);
    m_dock->reapplyAll();
    m_plot->update();
}

void MainWindow::onLightingToggled(bool on)
{
    m_plot->enableLighting(on);
    m_plot->update();
}

void MainWindow::onLegendToggled(bool on)
{
    m_plot->showColorLegend(on);
    m_plot->update();
}

void MainWindow::onOrthoToggled(bool on)
{
    m_plot->setOrtho(on);
}

void MainWindow::onGridToggled(bool on)
{
    int sides = LEFT | RIGHT | CEIL | FLOOR | FRONT | BACK;
    m_plot->coordinates()->setGridLines(on, false, sides);
    m_plot->update();
}

void MainWindow::onResetView()
{
    m_plot->setRotation(30, 0, 15);
    m_plot->setViewportShift(0.05, 0);
    m_plot->setScale(1, 1, 1);
    m_plot->setZoom(0.9);
}

void MainWindow::onSavePixmap()
{
    QString fileName = QFileDialog::getSaveFileName(
        this, QStringLiteral("Save Pixmap"), QStringLiteral("qwtplot3d.png"),
        QStringLiteral("PNG Files (*.png);;JPEG Files (*.jpg);;All Files (*)"));
    if (!fileName.isEmpty())
        m_plot->savePixmap(fileName, QStringLiteral("PNG"));
}

void MainWindow::onToggleMouse(bool on)
{
    m_plot->enableMouse(on);
}

void MainWindow::onToggleKeyboard(bool on)
{
    m_plot->enableKeyboard(on);
}

void MainWindow::showRotation(double x, double y, double z)
{
    m_rotationLabel->setText(QString(QStringLiteral("Rotation: %1, %2, %3"))
                                     .arg(x, 0, 'f', 1)
                                     .arg(y, 0, 'f', 1)
                                     .arg(z, 0, 'f', 1));
}

void MainWindow::showZoom(double z)
{
    m_zoomLabel->setText(QString(QStringLiteral("Zoom: %1")).arg(z, 0, 'f', 2));
}
