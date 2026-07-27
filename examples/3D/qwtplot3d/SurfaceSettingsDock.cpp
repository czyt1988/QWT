#include "SurfaceSettingsDock.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include "qwt3d_axis.h"
#include "qwt3d_colormap_color.h"
#include "qwt3d_colorlegend.h"
#include "qwt3d_coordsys.h"
#include "qwt3d_enrichment_std.h"
#include "qwt3d_plot.h"
#include "qwt3d_surface.h"
#include "qwt3d_theme.h"
#include "qwt3d_types.h"

#include "qwt_colormap_preset.h"

// ---------------------------------------------------------------------------
// Color conversion helpers
// ---------------------------------------------------------------------------
static QColor rgbaToQColor(const RGBA& rgba)
{
    return QColor(qRound(rgba.r * 255), qRound(rgba.g * 255), qRound(rgba.b * 255), qRound(rgba.a * 255));
}

static RGBA qColorToRGBA(const QColor& c)
{
    return RGBA(c.redF(), c.greenF(), c.blueF(), c.alphaF());
}

static void styleColorButton(QPushButton* btn, const QColor& color)
{
    btn->setStyleSheet(QString("background-color: %1; border: 1px solid #888; min-height: 20px;")
                                .arg(color.name()));
}

// ---------------------------------------------------------------------------
// Axis name lookup
// ---------------------------------------------------------------------------
static const char* s_axisNames[] = {
    "X1", "Y1", "Z1", "X2", "X3", "X4", "Y4", "Y3", "Y2", "Z2", "Z4", "Z3"
};

QString SurfaceSettingsDock::axisName(int axis)
{
    if (axis >= 0 && axis < 12)
        return QString::fromLatin1(s_axisNames[axis]);
    return QStringLiteral("?");
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
SurfaceSettingsDock::SurfaceSettingsDock(QWidget* parent)
    : QDockWidget(QStringLiteral("3D Settings"), parent)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(createSurfaceTab(), QStringLiteral("Surface"));
    m_tabs->addTab(createAxesTab(), QStringLiteral("Axes"));
    m_tabs->addTab(createLegendTab(), QStringLiteral("Legend"));
    m_tabs->addTab(createViewLightTab(), QStringLiteral("View & Light"));
    m_tabs->addTab(createEnrichmentsTab(), QStringLiteral("Enrichments"));
    setWidget(m_tabs);
}

SurfaceSettingsDock::~SurfaceSettingsDock()
{
    // m_colorFunctor is owned by the surface; enrichments are owned by the surface too
}

// ---------------------------------------------------------------------------
// Set plot/surface
// ---------------------------------------------------------------------------
void SurfaceSettingsDock::setPlot(Qwt3DPlot* plot)
{
    m_plot = plot;
}

void SurfaceSettingsDock::setSurface(Qwt3DSurface* surface)
{
    m_surface = surface;

    if (m_surface && m_plot) {
        // Create and attach the color functor
        m_colorFunctor = new Qwt3DColorMapColor(m_plot, QStringLiteral("viridis"), 256);
        m_surface->setDataColor(m_colorFunctor);
    }
}

// ---------------------------------------------------------------------------
// Re-apply all settings (call after data switch)
// ---------------------------------------------------------------------------
void SurfaceSettingsDock::reapplyAll()
{
    if (!m_plot || !m_surface)
        return;

    // Surface tab
    onPlotStyleChanged(m_plotStyleCombo->currentIndex());
    onFloorStyleChanged(m_floorStyleCombo->currentIndex());
    onShadingChanged(m_shadingCombo->currentIndex());
    applyMeshColor();
    onMeshLineWidth(m_meshLineWidthSpin->value());
    onSmoothMesh(m_smoothMeshCheck->isChecked());
    onIsolines(m_isolinesSpin->value());
    onPolygonOffset(m_polygonOffsetSpin->value());
    onResolution(m_resolutionSpin->value());
    onShowNormals(m_showNormalsCheck->isChecked());
    onNormalLength(m_normalLengthSpin->value());
    onNormalQuality(m_normalQualitySpin->value());

    // Rebuild color functor with current preset.
    // applyTheme() replaces the surface's color functor (destroying the old one),
    // so m_colorFunctor may be a dangling pointer — always create a fresh one.
    m_colorFunctor = new Qwt3DColorMapColor(m_plot, m_colorPresetCombo->currentText(), 256);
    m_surface->setDataColor(m_colorFunctor);
    onColorAlpha(m_colorAlphaSlider->value());

    // Axes tab
    loadAxisValues();
    applyAxesColor();
    applyGridLinesColor();
    onAutoDecoration(m_autoDecorationCheck->isChecked());
    onTickPositionChanged(m_tickPositionCombo->currentIndex());
    onLineSmooth(m_lineSmoothCheck->isChecked());
    onGridSideToggled();

    // Legend tab
    onLegendOrientationChanged(m_legendOrientationCombo->currentIndex());
    onLegendScalePositionChanged(m_legendScalePosCombo->currentIndex());
    onLegendDrawScale(m_legendDrawScaleCheck->isChecked());
    onLegendDrawNumbers(m_legendDrawNumbersCheck->isChecked());
    onLegendAutoScale(m_legendAutoScaleCheck->isChecked());
    onLegendMajors(m_legendMajorsSpin->value());
    onLegendMinors(m_legendMinorsSpin->value());
    onLegendTitleChanged(m_legendTitleEdit->text());
    onLegendLimitStart(m_legendLimitStartSpin->value());

    // View & Light tab
    onRotationChanged();
    onShiftChanged();
    onViewportShiftChanged();
    onScaleChanged();
    onZoomChanged(m_zoomSpin->value());
    applyBackgroundColor();
    onLightingEnabled(m_lightingCheck->isChecked());
    onLightRotationChanged();
    onLightShiftChanged();
    onShininess(m_shininessSpin->value());
    onTitleChanged(m_titleEdit->text());
    applyTitleColor();
    onTitlePositionChanged();

    updatePlot();
}

// ---------------------------------------------------------------------------
// Tab 1: Surface
// ---------------------------------------------------------------------------
QWidget* SurfaceSettingsDock::createSurfaceTab()
{
    auto* w = new QWidget;
    auto* form = new QFormLayout(w);

    m_plotStyleCombo = new QComboBox;
    m_plotStyleCombo->addItems({QStringLiteral("Wireframe"), QStringLiteral("Filled"),
                                QStringLiteral("FilledMesh"), QStringLiteral("HiddenLine"),
                                QStringLiteral("Points")});
    m_plotStyleCombo->setCurrentIndex(2); // FILLEDMESH
    connect(m_plotStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SurfaceSettingsDock::onPlotStyleChanged);
    form->addRow(QStringLiteral("Plot Style:"), m_plotStyleCombo);

    m_floorStyleCombo = new QComboBox;
    m_floorStyleCombo->addItems({QStringLiteral("No Floor"), QStringLiteral("Floor Iso"),
                                 QStringLiteral("Floor Data")});
    m_floorStyleCombo->setCurrentIndex(1); // FLOORISO
    connect(m_floorStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SurfaceSettingsDock::onFloorStyleChanged);
    form->addRow(QStringLiteral("Floor Style:"), m_floorStyleCombo);

    m_shadingCombo = new QComboBox;
    m_shadingCombo->addItems({QStringLiteral("Flat"), QStringLiteral("Gouraud")});
    m_shadingCombo->setCurrentIndex(1); // GOURAUD
    connect(m_shadingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SurfaceSettingsDock::onShadingChanged);
    form->addRow(QStringLiteral("Shading:"), m_shadingCombo);

    // --- Mesh properties ---
    auto* meshGroup = new QGroupBox(QStringLiteral("Mesh"));
    auto* meshForm = new QFormLayout(meshGroup);

    m_meshColorBtn = new QPushButton(QStringLiteral("Choose..."));
    styleColorButton(m_meshColorBtn, QColor(0, 0, 0));
    connect(m_meshColorBtn, &QPushButton::clicked, this, &SurfaceSettingsDock::onMeshColor);
    meshForm->addRow(QStringLiteral("Mesh Color:"), m_meshColorBtn);

    m_meshLineWidthSpin = new QDoubleSpinBox;
    m_meshLineWidthSpin->setRange(0.1, 5.0);
    m_meshLineWidthSpin->setSingleStep(0.1);
    m_meshLineWidthSpin->setValue(0.5);
    connect(m_meshLineWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onMeshLineWidth);
    meshForm->addRow(QStringLiteral("Line Width:"), m_meshLineWidthSpin);

    m_smoothMeshCheck = new QCheckBox(QStringLiteral("Smooth"));
    connect(m_smoothMeshCheck, &QCheckBox::toggled, this, &SurfaceSettingsDock::onSmoothMesh);
    meshForm->addRow(QString(), m_smoothMeshCheck);

    m_isolinesSpin = new QSpinBox;
    m_isolinesSpin->setRange(0, 50);
    m_isolinesSpin->setValue(10);
    connect(m_isolinesSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onIsolines);
    meshForm->addRow(QStringLiteral("Isolines:"), m_isolinesSpin);

    m_polygonOffsetSpin = new QDoubleSpinBox;
    m_polygonOffsetSpin->setRange(0.0, 2.0);
    m_polygonOffsetSpin->setSingleStep(0.1);
    m_polygonOffsetSpin->setValue(0.8);
    connect(m_polygonOffsetSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onPolygonOffset);
    meshForm->addRow(QStringLiteral("Polygon Offset:"), m_polygonOffsetSpin);

    form->addRow(meshGroup);

    // --- Data color ---
    auto* colorGroup = new QGroupBox(QStringLiteral("Data Color"));
    auto* colorForm = new QFormLayout(colorGroup);

    m_colorPresetCombo = new QComboBox;
    m_colorPresetCombo->addItems(QwtColorMapPreset::availablePresets());
    m_colorPresetCombo->setCurrentText(QStringLiteral("viridis"));
    connect(m_colorPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SurfaceSettingsDock::onColorPresetChanged);
    colorForm->addRow(QStringLiteral("Preset:"), m_colorPresetCombo);

    auto* alphaLayout = new QHBoxLayout;
    m_colorAlphaSlider = new QSlider(Qt::Horizontal);
    m_colorAlphaSlider->setRange(0, 255);
    m_colorAlphaSlider->setValue(255);
    connect(m_colorAlphaSlider, &QSlider::valueChanged, this, &SurfaceSettingsDock::onColorAlpha);
    alphaLayout->addWidget(m_colorAlphaSlider);
    colorForm->addRow(QStringLiteral("Alpha:"), alphaLayout);

    form->addRow(colorGroup);

    // --- Resolution ---
    m_resolutionSpin = new QSpinBox;
    m_resolutionSpin->setRange(1, 20);
    m_resolutionSpin->setValue(1);
    connect(m_resolutionSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onResolution);
    form->addRow(QStringLiteral("Resolution:"), m_resolutionSpin);

    // --- Normals ---
    auto* normalsGroup = new QGroupBox(QStringLiteral("Normals"));
    auto* normalsForm = new QFormLayout(normalsGroup);

    m_showNormalsCheck = new QCheckBox(QStringLiteral("Show"));
    connect(m_showNormalsCheck, &QCheckBox::toggled, this, &SurfaceSettingsDock::onShowNormals);
    normalsForm->addRow(QString(), m_showNormalsCheck);

    m_normalLengthSpin = new QDoubleSpinBox;
    m_normalLengthSpin->setRange(0.0, 1.0);
    m_normalLengthSpin->setSingleStep(0.05);
    m_normalLengthSpin->setValue(0.1);
    connect(m_normalLengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onNormalLength);
    normalsForm->addRow(QStringLiteral("Length:"), m_normalLengthSpin);

    m_normalQualitySpin = new QSpinBox;
    m_normalQualitySpin->setRange(3, 20);
    m_normalQualitySpin->setValue(3);
    connect(m_normalQualitySpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onNormalQuality);
    normalsForm->addRow(QStringLiteral("Quality:"), m_normalQualitySpin);

    form->addRow(normalsGroup);

    return w;
}

// ---------------------------------------------------------------------------
// Tab 2: Axes
// ---------------------------------------------------------------------------
QWidget* SurfaceSettingsDock::createAxesTab()
{
    auto* w = new QWidget;
    auto* form = new QFormLayout(w);

    // --- Per-axis section ---
    auto* axisGroup = new QGroupBox(QStringLiteral("Individual Axis"));
    auto* axisForm = new QFormLayout(axisGroup);

    m_axisSelector = new QComboBox;
    for (int i = 0; i < 12; ++i)
        m_axisSelector->addItem(axisName(i));
    m_axisSelector->setCurrentIndex(2); // Z1
    connect(m_axisSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SurfaceSettingsDock::onAxisSelectionChanged);
    axisForm->addRow(QStringLiteral("Axis:"), m_axisSelector);

    m_axisLabelEdit = new QLineEdit;
    connect(m_axisLabelEdit, &QLineEdit::textChanged, this, &SurfaceSettingsDock::onAxisLabelChanged);
    axisForm->addRow(QStringLiteral("Label:"), m_axisLabelEdit);

    m_axisMajorsSpin = new QSpinBox;
    m_axisMajorsSpin->setRange(1, 20);
    m_axisMajorsSpin->setValue(5);
    connect(m_axisMajorsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onAxisMajors);
    axisForm->addRow(QStringLiteral("Majors:"), m_axisMajorsSpin);

    m_axisMinorsSpin = new QSpinBox;
    m_axisMinorsSpin->setRange(0, 20);
    m_axisMinorsSpin->setValue(4);
    connect(m_axisMinorsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onAxisMinors);
    axisForm->addRow(QStringLiteral("Minors:"), m_axisMinorsSpin);

    m_axisNumbersCheck = new QCheckBox(QStringLiteral("Show Numbers"));
    m_axisNumbersCheck->setChecked(true);
    connect(m_axisNumbersCheck, &QCheckBox::toggled, this, &SurfaceSettingsDock::onAxisNumbers);
    axisForm->addRow(QString(), m_axisNumbersCheck);

    m_axisScalingCheck = new QCheckBox(QStringLiteral("Show Scale"));
    m_axisScalingCheck->setChecked(true);
    connect(m_axisScalingCheck, &QCheckBox::toggled, this, &SurfaceSettingsDock::onAxisScaling);
    axisForm->addRow(QString(), m_axisScalingCheck);

    m_axisAutoScaleCheck = new QCheckBox(QStringLiteral("Auto Scale"));
    m_axisAutoScaleCheck->setChecked(true);
    connect(m_axisAutoScaleCheck, &QCheckBox::toggled, this, &SurfaceSettingsDock::onAxisAutoScale);
    axisForm->addRow(QString(), m_axisAutoScaleCheck);

    m_axisScaleTypeCombo = new QComboBox;
    m_axisScaleTypeCombo->addItems({QStringLiteral("Linear"), QStringLiteral("Log10")});
    connect(m_axisScaleTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SurfaceSettingsDock::onAxisScaleTypeChanged);
    axisForm->addRow(QStringLiteral("Scale Type:"), m_axisScaleTypeCombo);

    m_axisLineWidthSpin = new QDoubleSpinBox;
    m_axisLineWidthSpin->setRange(0.1, 10.0);
    m_axisLineWidthSpin->setSingleStep(0.1);
    m_axisLineWidthSpin->setValue(1.0);
    connect(m_axisLineWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onAxisLineWidth);
    axisForm->addRow(QStringLiteral("Line Width:"), m_axisLineWidthSpin);

    m_axisTicMajorSpin = new QDoubleSpinBox;
    m_axisTicMajorSpin->setRange(0.0, 1.0);
    m_axisTicMajorSpin->setSingleStep(0.01);
    m_axisTicMajorSpin->setValue(0.03);
    connect(m_axisTicMajorSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onAxisTicMajor);
    axisForm->addRow(QStringLiteral("Tic Major:"), m_axisTicMajorSpin);

    m_axisTicMinorSpin = new QDoubleSpinBox;
    m_axisTicMinorSpin->setRange(0.0, 1.0);
    m_axisTicMinorSpin->setSingleStep(0.01);
    m_axisTicMinorSpin->setValue(0.015);
    connect(m_axisTicMinorSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onAxisTicMinor);
    axisForm->addRow(QStringLiteral("Tic Minor:"), m_axisTicMinorSpin);

    m_axisSymmetricTicsCheck = new QCheckBox(QStringLiteral("Symmetric Tics"));
    connect(m_axisSymmetricTicsCheck, &QCheckBox::toggled, this, &SurfaceSettingsDock::onAxisSymmetricTics);
    axisForm->addRow(QString(), m_axisSymmetricTicsCheck);

    form->addRow(axisGroup);

    // --- Common section ---
    auto* commonGroup = new QGroupBox(QStringLiteral("Common (All Axes)"));
    auto* commonForm = new QFormLayout(commonGroup);

    m_axesColorBtn = new QPushButton(QStringLiteral("Choose..."));
    styleColorButton(m_axesColorBtn, QColor(80, 80, 80));
    connect(m_axesColorBtn, &QPushButton::clicked, this, &SurfaceSettingsDock::onAxesColor);
    commonForm->addRow(QStringLiteral("Axes Color:"), m_axesColorBtn);

    m_gridLinesColorBtn = new QPushButton(QStringLiteral("Choose..."));
    styleColorButton(m_gridLinesColorBtn, QColor(200, 200, 200));
    connect(m_gridLinesColorBtn, &QPushButton::clicked, this, &SurfaceSettingsDock::onGridLinesColor);
    commonForm->addRow(QStringLiteral("Grid Color:"), m_gridLinesColorBtn);

    auto* gridSidesLayout = new QVBoxLayout;
    const char* sideNames[] = {"Left", "Right", "Ceil", "Floor", "Front", "Back"};
    for (int i = 0; i < 6; ++i) {
        m_gridSideChecks[i] = new QCheckBox(QString::fromLatin1(sideNames[i]));
        connect(m_gridSideChecks[i], &QCheckBox::toggled, this, &SurfaceSettingsDock::onGridSideToggled);
        gridSidesLayout->addWidget(m_gridSideChecks[i]);
    }
    commonForm->addRow(QStringLiteral("Grid Sides:"), gridSidesLayout);

    m_gridMajorsCheck = new QCheckBox(QStringLiteral("Major Grid Lines"));
    connect(m_gridMajorsCheck, &QCheckBox::toggled, this, &SurfaceSettingsDock::onGridMajorsToggled);
    commonForm->addRow(QString(), m_gridMajorsCheck);

    m_gridMinorsCheck = new QCheckBox(QStringLiteral("Minor Grid Lines"));
    connect(m_gridMinorsCheck, &QCheckBox::toggled, this, &SurfaceSettingsDock::onGridMinorsToggled);
    commonForm->addRow(QString(), m_gridMinorsCheck);

    m_autoDecorationCheck = new QCheckBox(QStringLiteral("Auto Decoration"));
    m_autoDecorationCheck->setChecked(true);
    connect(m_autoDecorationCheck, &QCheckBox::toggled, this, &SurfaceSettingsDock::onAutoDecoration);
    commonForm->addRow(QString(), m_autoDecorationCheck);

    m_tickPositionCombo = new QComboBox;
    m_tickPositionCombo->addItems({QStringLiteral("Bottom"), QStringLiteral("Top")});
    connect(m_tickPositionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SurfaceSettingsDock::onTickPositionChanged);
    commonForm->addRow(QStringLiteral("Tick Position:"), m_tickPositionCombo);

    m_lineSmoothCheck = new QCheckBox(QStringLiteral("Smooth Lines"));
    m_lineSmoothCheck->setChecked(true);
    connect(m_lineSmoothCheck, &QCheckBox::toggled, this, &SurfaceSettingsDock::onLineSmooth);
    commonForm->addRow(QString(), m_lineSmoothCheck);

    form->addRow(commonGroup);

    return w;
}

// ---------------------------------------------------------------------------
// Tab 3: Legend
// ---------------------------------------------------------------------------
QWidget* SurfaceSettingsDock::createLegendTab()
{
    auto* w = new QWidget;
    auto* form = new QFormLayout(w);

    m_legendOrientationCombo = new QComboBox;
    m_legendOrientationCombo->addItems({QStringLiteral("Bottom-Top"), QStringLiteral("Left-Right")});
    connect(m_legendOrientationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SurfaceSettingsDock::onLegendOrientationChanged);
    form->addRow(QStringLiteral("Orientation:"), m_legendOrientationCombo);

    m_legendScalePosCombo = new QComboBox;
    m_legendScalePosCombo->addItems({QStringLiteral("Top"), QStringLiteral("Bottom"),
                                     QStringLiteral("Left"), QStringLiteral("Right")});
    connect(m_legendScalePosCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SurfaceSettingsDock::onLegendScalePositionChanged);
    form->addRow(QStringLiteral("Scale Position:"), m_legendScalePosCombo);

    m_legendDrawScaleCheck = new QCheckBox(QStringLiteral("Draw Scale"));
    m_legendDrawScaleCheck->setChecked(true);
    connect(m_legendDrawScaleCheck, &QCheckBox::toggled, this, &SurfaceSettingsDock::onLegendDrawScale);
    form->addRow(QString(), m_legendDrawScaleCheck);

    m_legendDrawNumbersCheck = new QCheckBox(QStringLiteral("Draw Numbers"));
    m_legendDrawNumbersCheck->setChecked(true);
    connect(m_legendDrawNumbersCheck, &QCheckBox::toggled, this, &SurfaceSettingsDock::onLegendDrawNumbers);
    form->addRow(QString(), m_legendDrawNumbersCheck);

    m_legendAutoScaleCheck = new QCheckBox(QStringLiteral("Auto Scale"));
    m_legendAutoScaleCheck->setChecked(true);
    connect(m_legendAutoScaleCheck, &QCheckBox::toggled, this, &SurfaceSettingsDock::onLegendAutoScale);
    form->addRow(QString(), m_legendAutoScaleCheck);

    m_legendMajorsSpin = new QSpinBox;
    m_legendMajorsSpin->setRange(1, 20);
    m_legendMajorsSpin->setValue(5);
    connect(m_legendMajorsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onLegendMajors);
    form->addRow(QStringLiteral("Majors:"), m_legendMajorsSpin);

    m_legendMinorsSpin = new QSpinBox;
    m_legendMinorsSpin->setRange(0, 20);
    m_legendMinorsSpin->setValue(4);
    connect(m_legendMinorsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onLegendMinors);
    form->addRow(QStringLiteral("Minors:"), m_legendMinorsSpin);

    m_legendTitleEdit = new QLineEdit;
    connect(m_legendTitleEdit, &QLineEdit::textChanged, this, &SurfaceSettingsDock::onLegendTitleChanged);
    form->addRow(QStringLiteral("Title:"), m_legendTitleEdit);

    auto* limitsLayout = new QHBoxLayout;
    m_legendLimitStartSpin = new QDoubleSpinBox;
    m_legendLimitStartSpin->setRange(-1000, 1000);
    m_legendLimitStartSpin->setDecimals(3);
    m_legendLimitStopSpin = new QDoubleSpinBox;
    m_legendLimitStopSpin->setRange(-1000, 1000);
    m_legendLimitStopSpin->setDecimals(3);
    connect(m_legendLimitStartSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onLegendLimitStart);
    connect(m_legendLimitStopSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onLegendLimitStop);
    limitsLayout->addWidget(m_legendLimitStartSpin);
    limitsLayout->addWidget(new QLabel(QStringLiteral("..")));
    limitsLayout->addWidget(m_legendLimitStopSpin);
    form->addRow(QStringLiteral("Limits:"), limitsLayout);

    return w;
}

// ---------------------------------------------------------------------------
// Tab 4: View & Light
// ---------------------------------------------------------------------------
QWidget* SurfaceSettingsDock::createViewLightTab()
{
    auto* w = new QWidget;
    auto* form = new QFormLayout(w);

    // --- View ---
    auto* viewGroup = new QGroupBox(QStringLiteral("View"));
    auto* viewForm = new QFormLayout(viewGroup);

    auto* rotLayout = new QHBoxLayout;
    m_rotXSpin = new QDoubleSpinBox; m_rotXSpin->setRange(-360, 360); m_rotXSpin->setValue(30);
    m_rotYSpin = new QDoubleSpinBox; m_rotYSpin->setRange(-360, 360); m_rotYSpin->setValue(0);
    m_rotZSpin = new QDoubleSpinBox; m_rotZSpin->setRange(-360, 360); m_rotZSpin->setValue(15);
    rotLayout->addWidget(new QLabel("X")); rotLayout->addWidget(m_rotXSpin);
    rotLayout->addWidget(new QLabel("Y")); rotLayout->addWidget(m_rotYSpin);
    rotLayout->addWidget(new QLabel("Z")); rotLayout->addWidget(m_rotZSpin);
    auto* rotApplyBtn = new QPushButton(QStringLiteral("Apply"));
    connect(rotApplyBtn, &QPushButton::clicked, this, &SurfaceSettingsDock::onRotationChanged);
    rotLayout->addWidget(rotApplyBtn);
    viewForm->addRow(QStringLiteral("Rotation:"), rotLayout);

    auto* shiftLayout = new QHBoxLayout;
    m_shiftXSpin = new QDoubleSpinBox; m_shiftXSpin->setRange(-1, 1); m_shiftXSpin->setSingleStep(0.05);
    m_shiftYSpin = new QDoubleSpinBox; m_shiftYSpin->setRange(-1, 1); m_shiftYSpin->setSingleStep(0.05);
    m_shiftZSpin = new QDoubleSpinBox; m_shiftZSpin->setRange(-1, 1); m_shiftZSpin->setSingleStep(0.05);
    shiftLayout->addWidget(new QLabel("X")); shiftLayout->addWidget(m_shiftXSpin);
    shiftLayout->addWidget(new QLabel("Y")); shiftLayout->addWidget(m_shiftYSpin);
    shiftLayout->addWidget(new QLabel("Z")); shiftLayout->addWidget(m_shiftZSpin);
    auto* shiftApplyBtn = new QPushButton(QStringLiteral("Apply"));
    connect(shiftApplyBtn, &QPushButton::clicked, this, &SurfaceSettingsDock::onShiftChanged);
    shiftLayout->addWidget(shiftApplyBtn);
    viewForm->addRow(QStringLiteral("Shift:"), shiftLayout);

    auto* vpShiftLayout = new QHBoxLayout;
    m_vpShiftXSpin = new QDoubleSpinBox; m_vpShiftXSpin->setRange(-1, 1); m_vpShiftXSpin->setSingleStep(0.05); m_vpShiftXSpin->setValue(0.05);
    m_vpShiftYSpin = new QDoubleSpinBox; m_vpShiftYSpin->setRange(-1, 1); m_vpShiftYSpin->setSingleStep(0.05);
    vpShiftLayout->addWidget(new QLabel("X")); vpShiftLayout->addWidget(m_vpShiftXSpin);
    vpShiftLayout->addWidget(new QLabel("Y")); vpShiftLayout->addWidget(m_vpShiftYSpin);
    auto* vpApplyBtn = new QPushButton(QStringLiteral("Apply"));
    connect(vpApplyBtn, &QPushButton::clicked, this, &SurfaceSettingsDock::onViewportShiftChanged);
    vpShiftLayout->addWidget(vpApplyBtn);
    viewForm->addRow(QStringLiteral("Viewport Shift:"), vpShiftLayout);

    auto* scaleLayout = new QHBoxLayout;
    m_scaleXSpin = new QDoubleSpinBox; m_scaleXSpin->setRange(0.01, 10); m_scaleXSpin->setValue(1);
    m_scaleYSpin = new QDoubleSpinBox; m_scaleYSpin->setRange(0.01, 10); m_scaleYSpin->setValue(1);
    m_scaleZSpin = new QDoubleSpinBox; m_scaleZSpin->setRange(0.01, 10); m_scaleZSpin->setValue(1);
    scaleLayout->addWidget(new QLabel("X")); scaleLayout->addWidget(m_scaleXSpin);
    scaleLayout->addWidget(new QLabel("Y")); scaleLayout->addWidget(m_scaleYSpin);
    scaleLayout->addWidget(new QLabel("Z")); scaleLayout->addWidget(m_scaleZSpin);
    auto* scaleApplyBtn = new QPushButton(QStringLiteral("Apply"));
    connect(scaleApplyBtn, &QPushButton::clicked, this, &SurfaceSettingsDock::onScaleChanged);
    scaleLayout->addWidget(scaleApplyBtn);
    viewForm->addRow(QStringLiteral("Scale:"), scaleLayout);

    m_zoomSpin = new QDoubleSpinBox;
    m_zoomSpin->setRange(0.01, 10);
    m_zoomSpin->setSingleStep(0.05);
    m_zoomSpin->setValue(0.9);
    connect(m_zoomSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onZoomChanged);
    viewForm->addRow(QStringLiteral("Zoom:"), m_zoomSpin);

    m_bgColorBtn = new QPushButton(QStringLiteral("Choose..."));
    styleColorButton(m_bgColorBtn, Qt::white);
    connect(m_bgColorBtn, &QPushButton::clicked, this, &SurfaceSettingsDock::onBackgroundColor);
    viewForm->addRow(QStringLiteral("Background:"), m_bgColorBtn);

    form->addRow(viewGroup);

    // --- Lighting ---
    auto* lightGroup = new QGroupBox(QStringLiteral("Lighting"));
    auto* lightForm = new QFormLayout(lightGroup);

    m_lightingCheck = new QCheckBox(QStringLiteral("Enabled"));
    m_lightingCheck->setChecked(true);
    connect(m_lightingCheck, &QCheckBox::toggled, this, &SurfaceSettingsDock::onLightingEnabled);
    lightForm->addRow(QString(), m_lightingCheck);

    auto* lightsLayout = new QHBoxLayout;
    for (int i = 0; i < 8; ++i) {
        m_lightButtons[i] = new QPushButton(QString::number(i));
        m_lightButtons[i]->setCheckable(true);
        m_lightButtons[i]->setMaximumWidth(30);
        if (i == 0) m_lightButtons[i]->setChecked(true);
        connect(m_lightButtons[i], &QPushButton::clicked, this, [this, i]() { onIlluminateLight(i); });
        lightsLayout->addWidget(m_lightButtons[i]);
    }
    lightForm->addRow(QStringLiteral("Lights:"), lightsLayout);

    auto* lightRotLayout = new QHBoxLayout;
    m_lightRotXSpin = new QDoubleSpinBox; m_lightRotXSpin->setRange(-360, 360); m_lightRotXSpin->setValue(20);
    m_lightRotYSpin = new QDoubleSpinBox; m_lightRotYSpin->setRange(-360, 360); m_lightRotYSpin->setValue(30);
    m_lightRotZSpin = new QDoubleSpinBox; m_lightRotZSpin->setRange(-360, 360);
    lightRotLayout->addWidget(new QLabel("X")); lightRotLayout->addWidget(m_lightRotXSpin);
    lightRotLayout->addWidget(new QLabel("Y")); lightRotLayout->addWidget(m_lightRotYSpin);
    lightRotLayout->addWidget(new QLabel("Z")); lightRotLayout->addWidget(m_lightRotZSpin);
    auto* lightRotBtn = new QPushButton(QStringLiteral("Apply"));
    connect(lightRotBtn, &QPushButton::clicked, this, &SurfaceSettingsDock::onLightRotationChanged);
    lightRotLayout->addWidget(lightRotBtn);
    lightForm->addRow(QStringLiteral("Light Rotation:"), lightRotLayout);

    auto* lightShiftLayout = new QHBoxLayout;
    m_lightShiftXSpin = new QDoubleSpinBox; m_lightShiftXSpin->setRange(-10, 10);
    m_lightShiftYSpin = new QDoubleSpinBox; m_lightShiftYSpin->setRange(-10, 10);
    m_lightShiftZSpin = new QDoubleSpinBox; m_lightShiftZSpin->setRange(-10, 10);
    lightShiftLayout->addWidget(new QLabel("X")); lightShiftLayout->addWidget(m_lightShiftXSpin);
    lightShiftLayout->addWidget(new QLabel("Y")); lightShiftLayout->addWidget(m_lightShiftYSpin);
    lightShiftLayout->addWidget(new QLabel("Z")); lightShiftLayout->addWidget(m_lightShiftZSpin);
    auto* lightShiftBtn = new QPushButton(QStringLiteral("Apply"));
    connect(lightShiftBtn, &QPushButton::clicked, this, &SurfaceSettingsDock::onLightShiftChanged);
    lightShiftLayout->addWidget(lightShiftBtn);
    lightForm->addRow(QStringLiteral("Light Shift:"), lightShiftLayout);

    m_shininessSpin = new QDoubleSpinBox;
    m_shininessSpin->setRange(0, 128);
    m_shininessSpin->setValue(10);
    connect(m_shininessSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SurfaceSettingsDock::onShininess);
    lightForm->addRow(QStringLiteral("Shininess:"), m_shininessSpin);

    form->addRow(lightGroup);

    // --- Title ---
    auto* titleGroup = new QGroupBox(QStringLiteral("Title"));
    auto* titleForm = new QFormLayout(titleGroup);

    m_titleEdit = new QLineEdit;
    m_titleEdit->setText(QStringLiteral("Qwt3D Surface Plot"));
    connect(m_titleEdit, &QLineEdit::textChanged, this, &SurfaceSettingsDock::onTitleChanged);
    titleForm->addRow(QStringLiteral("Text:"), m_titleEdit);

    m_titleColorBtn = new QPushButton(QStringLiteral("Choose..."));
    styleColorButton(m_titleColorBtn, QColor(30, 30, 30));
    connect(m_titleColorBtn, &QPushButton::clicked, this, &SurfaceSettingsDock::onTitleColor);
    titleForm->addRow(QStringLiteral("Color:"), m_titleColorBtn);

    auto* titlePosLayout = new QHBoxLayout;
    m_titleRelYSpin = new QDoubleSpinBox; m_titleRelYSpin->setRange(0, 1); m_titleRelYSpin->setSingleStep(0.05);
    m_titleRelXSpin = new QDoubleSpinBox; m_titleRelXSpin->setRange(0, 1); m_titleRelXSpin->setSingleStep(0.05); m_titleRelXSpin->setValue(0.5);
    titlePosLayout->addWidget(new QLabel("Y")); titlePosLayout->addWidget(m_titleRelYSpin);
    titlePosLayout->addWidget(new QLabel("X")); titlePosLayout->addWidget(m_titleRelXSpin);
    m_titleAnchorCombo = new QComboBox;
    m_titleAnchorCombo->addItems({QStringLiteral("BottomLeft"), QStringLiteral("BottomRight"),
                                  QStringLiteral("BottomCenter"), QStringLiteral("TopLeft"),
                                  QStringLiteral("TopRight"), QStringLiteral("TopCenter"),
                                  QStringLiteral("CenterLeft"), QStringLiteral("CenterRight"),
                                  QStringLiteral("Center")});
    m_titleAnchorCombo->setCurrentIndex(5); // TopCenter
    titlePosLayout->addWidget(new QLabel("Anchor"));
    titlePosLayout->addWidget(m_titleAnchorCombo);
    auto* titlePosBtn = new QPushButton(QStringLiteral("Apply"));
    connect(titlePosBtn, &QPushButton::clicked, this, &SurfaceSettingsDock::onTitlePositionChanged);
    titlePosLayout->addWidget(titlePosBtn);
    titleForm->addRow(QStringLiteral("Position:"), titlePosLayout);

    form->addRow(titleGroup);

    return w;
}

// ---------------------------------------------------------------------------
// Tab 5: Enrichments
// ---------------------------------------------------------------------------
QWidget* SurfaceSettingsDock::createEnrichmentsTab()
{
    auto* w = new QWidget;
    auto* form = new QFormLayout(w);

    m_enrichmentTypeCombo = new QComboBox;
    m_enrichmentTypeCombo->addItems({QStringLiteral("CrossHair"), QStringLiteral("Dot"),
                                     QStringLiteral("Cone"), QStringLiteral("Arrow")});
    form->addRow(QStringLiteral("Type:"), m_enrichmentTypeCombo);

    m_addEnrichmentBtn = new QPushButton(QStringLiteral("Add"));
    connect(m_addEnrichmentBtn, &QPushButton::clicked, this, &SurfaceSettingsDock::onAddEnrichment);
    form->addRow(QString(), m_addEnrichmentBtn);

    m_enrichmentList = new QListWidget;
    form->addRow(QStringLiteral("Active:"), m_enrichmentList);

    m_removeEnrichmentBtn = new QPushButton(QStringLiteral("Remove Selected"));
    connect(m_removeEnrichmentBtn, &QPushButton::clicked, this, &SurfaceSettingsDock::onRemoveEnrichment);
    form->addRow(QString(), m_removeEnrichmentBtn);

    auto* hint = new QLabel(QStringLiteral("Enrichments are vertex-dependent visual objects\n"
                                          "drawn at each data point on the surface."));
    hint->setWordWrap(true);
    form->addRow(QString(), hint);

    return w;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
void SurfaceSettingsDock::updatePlot()
{
    if (m_plot)
        m_plot->update();
}

void SurfaceSettingsDock::setColorButton(QPushButton* btn, const QColor& color)
{
    styleColorButton(btn, color);
}

QColor SurfaceSettingsDock::getColorFromButton(QPushButton* btn) const
{
    // Parse the stylesheet to get the color
    QString style = btn->styleSheet();
    int idx = style.indexOf("background-color: ");
    if (idx >= 0) {
        idx += QString("background-color: ").length();
        int end = style.indexOf(';', idx);
        QString colorName = style.mid(idx, end - idx);
        return QColor(colorName);
    }
    return Qt::black;
}

int SurfaceSettingsDock::currentAxisIndex() const
{
    return m_axisSelector->currentIndex();
}

int SurfaceSettingsDock::computeGridSides() const
{
    int sides = NOSIDEGRID;
    if (m_gridSideChecks[0]->isChecked()) sides |= LEFT;
    if (m_gridSideChecks[1]->isChecked()) sides |= RIGHT;
    if (m_gridSideChecks[2]->isChecked()) sides |= CEIL;
    if (m_gridSideChecks[3]->isChecked()) sides |= FLOOR;
    if (m_gridSideChecks[4]->isChecked()) sides |= FRONT;
    if (m_gridSideChecks[5]->isChecked()) sides |= BACK;
    return sides;
}

void SurfaceSettingsDock::loadAxisValues()
{
    if (!m_plot)
        return;

    int idx = currentAxisIndex();
    auto& axis = m_plot->coordinates()->axes[idx];

    // Qwt3DAxis does not expose a label-string getter, so leave the edit as-is.

    m_axisMajorsSpin->blockSignals(true);
    m_axisMinorsSpin->blockSignals(true);
    m_axisMajorsSpin->setValue(axis.majors());
    m_axisMinorsSpin->setValue(axis.minors());
    m_axisMajorsSpin->blockSignals(false);
    m_axisMinorsSpin->blockSignals(false);

    m_axisNumbersCheck->blockSignals(true);
    m_axisNumbersCheck->setChecked(axis.numbers());
    m_axisNumbersCheck->blockSignals(false);

    m_axisScalingCheck->blockSignals(true);
    m_axisScalingCheck->setChecked(axis.scaling());
    m_axisScalingCheck->blockSignals(false);

    m_axisAutoScaleCheck->blockSignals(true);
    m_axisAutoScaleCheck->setChecked(axis.autoScale());
    m_axisAutoScaleCheck->blockSignals(false);
}

void SurfaceSettingsDock::applyAxisValues()
{
    if (!m_plot)
        return;

    int idx = currentAxisIndex();
    auto& axis = m_plot->coordinates()->axes[idx];

    axis.setLabelString(m_axisLabelEdit->text());
    axis.setMajors(m_axisMajorsSpin->value());
    axis.setMinors(m_axisMinorsSpin->value());
    axis.setNumbers(m_axisNumbersCheck->isChecked());
    axis.setScaling(m_axisScalingCheck->isChecked());
    axis.setAutoScale(m_axisAutoScaleCheck->isChecked());

    double major, minor;
    axis.ticLength(major, minor);
    axis.setTicLength(m_axisTicMajorSpin->value(), m_axisTicMinorSpin->value());
    axis.setLineWidth(m_axisLineWidthSpin->value());
    axis.setSymmetricTics(m_axisSymmetricTicsCheck->isChecked());

    updatePlot();
}

// ---------------------------------------------------------------------------
// Surface tab slots
// ---------------------------------------------------------------------------
void SurfaceSettingsDock::onPlotStyleChanged(int index)
{
    if (!m_surface)
        return;
    static const PLOTSTYLE styles[] = {WIREFRAME, FILLED, FILLEDMESH, HIDDENLINE, QWT3D_POINTS};
    m_surface->setPlotStyle(styles[index]);
    updatePlot();
}

void SurfaceSettingsDock::onFloorStyleChanged(int index)
{
    if (!m_surface)
        return;
    static const FLOORSTYLE styles[] = {NOFLOOR, FLOORISO, FLOORDATA};
    m_surface->setFloorStyle(styles[index]);
    updatePlot();
}

void SurfaceSettingsDock::onShadingChanged(int index)
{
    if (!m_surface)
        return;
    m_surface->setShading(index == 0 ? FLAT : GOURAUD);
    updatePlot();
}

void SurfaceSettingsDock::applyMeshColor()
{
    if (m_surface)
        m_surface->setMeshColor(qColorToRGBA(getColorFromButton(m_meshColorBtn)));
}

void SurfaceSettingsDock::onMeshColor()
{
    QColor c = QColorDialog::getColor(getColorFromButton(m_meshColorBtn), this, "Mesh Color");
    if (!c.isValid())
        return;
    styleColorButton(m_meshColorBtn, c);
    applyMeshColor();
    updatePlot();
}

void SurfaceSettingsDock::onMeshLineWidth(double val)
{
    if (m_surface)
        m_surface->setMeshLineWidth(val);
    updatePlot();
}

void SurfaceSettingsDock::onSmoothMesh(bool on)
{
    if (m_surface)
        m_surface->setSmoothMesh(on);
    updatePlot();
}

void SurfaceSettingsDock::onIsolines(int val)
{
    if (m_surface)
        m_surface->setIsolines(val);
    updatePlot();
}

void SurfaceSettingsDock::onPolygonOffset(double val)
{
    if (m_surface)
        m_surface->setPolygonOffset(val);
    updatePlot();
}

void SurfaceSettingsDock::onResolution(int val)
{
    if (m_surface)
        m_surface->setResolution(val);
    updatePlot();
}

void SurfaceSettingsDock::onColorPresetChanged(int /*index*/)
{
    if (!m_colorFunctor)
        return;
    QString preset = m_colorPresetCombo->currentText();
    m_colorFunctor->setColorMap(QwtColorMapPreset::create(preset).release());
    updatePlot();
}

void SurfaceSettingsDock::onColorAlpha(int val)
{
    if (!m_colorFunctor)
        return;
    m_colorFunctor->setAlpha(val / 255.0);
    updatePlot();
}

void SurfaceSettingsDock::onShowNormals(bool on)
{
    if (m_surface)
        m_surface->showNormals(on);
    updatePlot();
}

void SurfaceSettingsDock::onNormalLength(double val)
{
    if (m_surface)
        m_surface->setNormalLength(val);
    updatePlot();
}

void SurfaceSettingsDock::onNormalQuality(int val)
{
    if (m_surface)
        m_surface->setNormalQuality(val);
    updatePlot();
}

// ---------------------------------------------------------------------------
// Axes tab slots
// ---------------------------------------------------------------------------
void SurfaceSettingsDock::onAxisSelectionChanged(int /*index*/)
{
    loadAxisValues();
}

void SurfaceSettingsDock::onAxisLabelChanged(const QString& /*text*/)
{
    applyAxisValues();
}

void SurfaceSettingsDock::onAxisMajors(int /*val*/)
{
    applyAxisValues();
}

void SurfaceSettingsDock::onAxisMinors(int /*val*/)
{
    applyAxisValues();
}

void SurfaceSettingsDock::onAxisNumbers(bool /*on*/)
{
    applyAxisValues();
}

void SurfaceSettingsDock::onAxisScaling(bool /*on*/)
{
    applyAxisValues();
}

void SurfaceSettingsDock::onAxisAutoScale(bool /*on*/)
{
    applyAxisValues();
}

void SurfaceSettingsDock::onAxisScaleTypeChanged(int index)
{
    if (!m_plot)
        return;
    int idx = currentAxisIndex();
    auto& axis = m_plot->coordinates()->axes[idx];
    axis.setScale(index == 0 ? LINEARSCALE : LOG10SCALE);
    updatePlot();
}

void SurfaceSettingsDock::onAxisLineWidth(double /*val*/)
{
    applyAxisValues();
}

void SurfaceSettingsDock::onAxisTicMajor(double /*val*/)
{
    applyAxisValues();
}

void SurfaceSettingsDock::onAxisTicMinor(double /*val*/)
{
    applyAxisValues();
}

void SurfaceSettingsDock::onAxisSymmetricTics(bool /*on*/)
{
    applyAxisValues();
}

void SurfaceSettingsDock::applyAxesColor()
{
    if (m_plot)
        m_plot->coordinates()->setAxesColor(qColorToRGBA(getColorFromButton(m_axesColorBtn)));
}

void SurfaceSettingsDock::onAxesColor()
{
    QColor c = QColorDialog::getColor(getColorFromButton(m_axesColorBtn), this, "Axes Color");
    if (!c.isValid())
        return;
    styleColorButton(m_axesColorBtn, c);
    applyAxesColor();
    updatePlot();
}

void SurfaceSettingsDock::applyGridLinesColor()
{
    if (m_plot)
        m_plot->coordinates()->setGridLinesColor(qColorToRGBA(getColorFromButton(m_gridLinesColorBtn)));
}

void SurfaceSettingsDock::onGridLinesColor()
{
    QColor c = QColorDialog::getColor(getColorFromButton(m_gridLinesColorBtn), this, "Grid Lines Color");
    if (!c.isValid())
        return;
    styleColorButton(m_gridLinesColorBtn, c);
    applyGridLinesColor();
    updatePlot();
}

void SurfaceSettingsDock::onGridSideToggled()
{
    if (!m_plot)
        return;
    int sides = computeGridSides();
    bool majors = m_gridMajorsCheck->isChecked();
    bool minors = m_gridMinorsCheck->isChecked();
    m_plot->coordinates()->setGridLines(majors, minors, sides);
    updatePlot();
}

void SurfaceSettingsDock::onGridMajorsToggled(bool)
{
    onGridSideToggled();
}

void SurfaceSettingsDock::onGridMinorsToggled(bool)
{
    onGridSideToggled();
}

void SurfaceSettingsDock::onAutoDecoration(bool on)
{
    if (m_plot)
        m_plot->coordinates()->setAutoDecoration(on);
    updatePlot();
}

void SurfaceSettingsDock::onTickPositionChanged(int index)
{
    if (m_plot)
        m_plot->coordinates()->setTickPosition(index == 0 ? TICK_BOTTOM : TICK_TOP);
    updatePlot();
}

void SurfaceSettingsDock::onLineSmooth(bool on)
{
    if (m_plot)
        m_plot->coordinates()->setLineSmooth(on);
    updatePlot();
}

// ---------------------------------------------------------------------------
// Legend tab slots
// ---------------------------------------------------------------------------
void SurfaceSettingsDock::onLegendOrientationChanged(int index)
{
    if (!m_plot || !m_plot->legend())
        return;
    // BottomTop=0, LeftRight=1
    // ScalePosition: Top=0, Bottom=1, Left=2, Right=3
    Qwt3DColorLegend::ORIENTATION orient = (index == 0)
        ? Qwt3DColorLegend::BottomTop : Qwt3DColorLegend::LeftRight;
    Qwt3DColorLegend::SCALEPOSITION sp = static_cast<Qwt3DColorLegend::SCALEPOSITION>(m_legendScalePosCombo->currentIndex());
    m_plot->legend()->setOrientation(orient, sp);
    updatePlot();
}

void SurfaceSettingsDock::onLegendScalePositionChanged(int index)
{
    if (!m_plot || !m_plot->legend())
        return;
    Qwt3DColorLegend::ORIENTATION orient = (m_legendOrientationCombo->currentIndex() == 0)
        ? Qwt3DColorLegend::BottomTop : Qwt3DColorLegend::LeftRight;
    Qwt3DColorLegend::SCALEPOSITION sp = static_cast<Qwt3DColorLegend::SCALEPOSITION>(index);
    m_plot->legend()->setOrientation(orient, sp);
    updatePlot();
}

void SurfaceSettingsDock::onLegendDrawScale(bool on)
{
    if (m_plot && m_plot->legend())
        m_plot->legend()->drawScale(on);
    updatePlot();
}

void SurfaceSettingsDock::onLegendDrawNumbers(bool on)
{
    if (m_plot && m_plot->legend())
        m_plot->legend()->drawNumbers(on);
    updatePlot();
}

void SurfaceSettingsDock::onLegendAutoScale(bool on)
{
    if (m_plot && m_plot->legend())
        m_plot->legend()->setAutoScale(on);
    updatePlot();
}

void SurfaceSettingsDock::onLegendMajors(int val)
{
    if (m_plot && m_plot->legend())
        m_plot->legend()->setMajors(val);
    updatePlot();
}

void SurfaceSettingsDock::onLegendMinors(int val)
{
    if (m_plot && m_plot->legend())
        m_plot->legend()->setMinors(val);
    updatePlot();
}

void SurfaceSettingsDock::onLegendTitleChanged(const QString& text)
{
    if (m_plot && m_plot->legend())
        m_plot->legend()->setTitleString(text);
    updatePlot();
}

void SurfaceSettingsDock::onLegendLimitStart(double /*val*/)
{
    if (m_plot && m_plot->legend())
        m_plot->legend()->setLimits(m_legendLimitStartSpin->value(), m_legendLimitStopSpin->value());
    updatePlot();
}

void SurfaceSettingsDock::onLegendLimitStop(double /*val*/)
{
    if (m_plot && m_plot->legend())
        m_plot->legend()->setLimits(m_legendLimitStartSpin->value(), m_legendLimitStopSpin->value());
    updatePlot();
}

// ---------------------------------------------------------------------------
// View & Light tab slots
// ---------------------------------------------------------------------------
void SurfaceSettingsDock::onRotationChanged()
{
    if (m_plot)
        m_plot->setRotation(m_rotXSpin->value(), m_rotYSpin->value(), m_rotZSpin->value());
}

void SurfaceSettingsDock::onShiftChanged()
{
    if (m_plot)
        m_plot->setShift(m_shiftXSpin->value(), m_shiftYSpin->value(), m_shiftZSpin->value());
}

void SurfaceSettingsDock::onViewportShiftChanged()
{
    if (m_plot)
        m_plot->setViewportShift(m_vpShiftXSpin->value(), m_vpShiftYSpin->value());
}

void SurfaceSettingsDock::onScaleChanged()
{
    if (m_plot)
        m_plot->setScale(m_scaleXSpin->value(), m_scaleYSpin->value(), m_scaleZSpin->value());
}

void SurfaceSettingsDock::onZoomChanged(double val)
{
    if (m_plot)
        m_plot->setZoom(val);
}

void SurfaceSettingsDock::applyBackgroundColor()
{
    if (m_plot)
        m_plot->setBackgroundColor(qColorToRGBA(getColorFromButton(m_bgColorBtn)));
}

void SurfaceSettingsDock::onBackgroundColor()
{
    QColor c = QColorDialog::getColor(getColorFromButton(m_bgColorBtn), this, "Background Color");
    if (!c.isValid())
        return;
    styleColorButton(m_bgColorBtn, c);
    applyBackgroundColor();
    updatePlot();
}

void SurfaceSettingsDock::onLightingEnabled(bool on)
{
    if (m_plot)
        m_plot->enableLighting(on);
    updatePlot();
}

void SurfaceSettingsDock::onIlluminateLight(int idx)
{
    if (!m_plot)
        return;
    if (m_lightButtons[idx]->isChecked())
        m_plot->illuminate(idx);
    else
        m_plot->blowout(idx);
    updatePlot();
}

void SurfaceSettingsDock::onLightRotationChanged()
{
    if (m_plot)
        m_plot->setLightRotation(m_lightRotXSpin->value(), m_lightRotYSpin->value(), m_lightRotZSpin->value(), 0);
    updatePlot();
}

void SurfaceSettingsDock::onLightShiftChanged()
{
    if (m_plot)
        m_plot->setLightShift(m_lightShiftXSpin->value(), m_lightShiftYSpin->value(), m_lightShiftZSpin->value(), 0);
    updatePlot();
}

void SurfaceSettingsDock::onShininess(double val)
{
    if (m_plot)
        m_plot->setShininess(val);
    updatePlot();
}

void SurfaceSettingsDock::onTitleChanged(const QString& text)
{
    if (m_plot)
        m_plot->setTitle(text);
    updatePlot();
}

void SurfaceSettingsDock::applyTitleColor()
{
    if (m_plot)
        m_plot->setTitleColor(qColorToRGBA(getColorFromButton(m_titleColorBtn)));
}

void SurfaceSettingsDock::onTitleColor()
{
    QColor c = QColorDialog::getColor(getColorFromButton(m_titleColorBtn), this, "Title Color");
    if (!c.isValid())
        return;
    styleColorButton(m_titleColorBtn, c);
    applyTitleColor();
    updatePlot();
}

void SurfaceSettingsDock::onTitlePositionChanged()
{
    if (!m_plot)
        return;
    m_plot->setTitlePosition(m_titleRelYSpin->value(), m_titleRelXSpin->value(),
                             static_cast<ANCHOR>(m_titleAnchorCombo->currentIndex()));
    updatePlot();
}

// ---------------------------------------------------------------------------
// Enrichments tab slots
// ---------------------------------------------------------------------------
void SurfaceSettingsDock::onAddEnrichment()
{
    if (!m_surface)
        return;

    int type = m_enrichmentTypeCombo->currentIndex();
    Qwt3DEnrichment* added = nullptr;

    switch (type) {
    case 0: { // CrossHair
        Qwt3DCrossHair ch(0.1, 1.0, true, false);
        added = m_surface->addEnrichment(ch);
        break;
    }
    case 1: { // Dot
        Qwt3DDot dot(5.0, true);
        added = m_surface->addEnrichment(dot);
        break;
    }
    case 2: { // Cone
        Qwt3DCone cone(0.1, 5);
        added = m_surface->addEnrichment(cone);
        break;
    }
    case 3: { // Arrow
        Qwt3DArrow arrow;
        arrow.configure(10, 0.3, 0.2, 0.1);
        added = m_surface->addEnrichment(arrow);
        break;
    }
    }

    if (added) {
        m_enrichments.append(added);
        m_enrichmentList->addItem(m_enrichmentTypeCombo->currentText() + QStringLiteral(" #") +
                                  QString::number(m_enrichments.size()));
        updatePlot();
    }
}

void SurfaceSettingsDock::onRemoveEnrichment()
{
    if (!m_surface)
        return;

    int row = m_enrichmentList->currentRow();
    if (row < 0 || row >= m_enrichments.size())
        return;

    Qwt3DEnrichment* e = m_enrichments.takeAt(row);
    m_surface->degrade(e);
    delete m_enrichmentList->takeItem(row);
    updatePlot();
}
