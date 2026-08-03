#include "LineSettingsDock.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSlider>
#include <QVBoxLayout>

#include <cmath>

#include <QVector>

#include "qwt3d_colormap_color.h"
#include "qwt3d_line3d.h"
#include "qwt3d_plot.h"
#include "qwt3d_theme.h"
#include "qwt3d_types.h"
#include "qwt_colormap_preset.h"
#include "qwt_math.h"
#include "qwt_point_3d.h"

// ---------------------------------------------------------------------------
// File-local helpers (mirroring SurfaceSettingsDock.cpp)
// ---------------------------------------------------------------------------

static QColor rgbaToQColor(const RGBA& rgba)
{
    return QColor(qRound(rgba.r * 255), qRound(rgba.g * 255),
                  qRound(rgba.b * 255), qRound(rgba.a * 255));
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
// Constructor / Destructor
// ---------------------------------------------------------------------------

LineSettingsDock::LineSettingsDock(QWidget* parent)
    : QDockWidget(QStringLiteral("Line Settings"), parent)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setWidget(createContentWidget());
    setWidget(scroll);
}

LineSettingsDock::~LineSettingsDock()
{
    // m_colorFunctor is owned by the line item, not destroyed here
}

void LineSettingsDock::setPlot(Qwt3DPlot* plot)
{
    m_plot = plot;
}

void LineSettingsDock::setLine(Qwt3DLine* line)
{
    m_line = line;
    if (!m_line)
        return;

    // Create the initial color functor from current UI state
    m_colorFunctor = new Qwt3DColorMapColor(m_colorPresetCombo->currentText(), 256);
    m_colorFunctor->setAlpha(m_colorAlphaSlider->value() / 255.0);
    m_line->setDataColor(m_colorFunctor);
}

// ---------------------------------------------------------------------------
// Re-apply all UI values (call after a theme switch)
// ---------------------------------------------------------------------------

void LineSettingsDock::reapplyAll()
{
    if (!m_line)
        return;

    // Theme application replaces the data color functor, so recreate it
    m_colorFunctor = new Qwt3DColorMapColor(m_colorPresetCombo->currentText(), 256);
    m_colorFunctor->setAlpha(m_colorAlphaSlider->value() / 255.0);
    m_line->setDataColor(m_colorFunctor);

    // Re-apply line properties from current UI state
    m_line->setLineStyle(static_cast<Qwt3DLine::LineStyle>(m_lineStyleCombo->currentData().toInt()));
    m_line->setLineWidth(m_lineWidthSpin->value());
    m_line->setTubeRadius(m_tubeRadiusSpin->value());
    m_line->setTubeSegments(m_tubeSegmentsSpin->value());
    m_line->setPointSize(m_pointSizeSpin->value());
    m_line->setPointVisible(m_pointVisibleCheck->isChecked());
    m_line->setPointShape(static_cast<Qwt3DLine::PointShape>(m_pointShapeCombo->currentData().toInt()));

    // Regenerate data with current parameters
    regenerateData();

    updatePlot();
}

// ---------------------------------------------------------------------------
// Data regeneration
// ---------------------------------------------------------------------------

void LineSettingsDock::regenerateData()
{
    if (!m_line)
        return;

    const int n = m_sampleCountSpin->value();
    const double turns = m_helixTurnsSpin->value();
    const double tMin = 0.0;
    const double tMax = turns * 2.0 * M_PI;
    const int curveType = m_curveTypeCombo->currentData().toInt();

    QVector<QwtPoint3D> samples;
    samples.reserve(n);
    for (int i = 0; i < n; ++i) {
        const double t = (n > 1) ? (tMin + (tMax - tMin) * i / (n - 1)) : tMin;
        switch (curveType) {
            case 0: // Helix: (cos t, sin t, t)
                samples.append(QwtPoint3D(std::cos(t), std::sin(t), t));
                break;
            case 1: // Spiral: (t cos t, t sin t, t)
                samples.append(QwtPoint3D(t * std::cos(t), t * std::sin(t), t));
                break;
            case 2: // Lissajous: (sin 3t, sin 2t, t)
                samples.append(QwtPoint3D(std::sin(3.0 * t), std::sin(2.0 * t), t));
                break;
            default:
                samples.append(QwtPoint3D(0.0, 0.0, t));
                break;
        }
    }

    m_line->setSamples(samples);
}

void LineSettingsDock::onRegenerateData()
{
    regenerateData();
    updatePlot();
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void LineSettingsDock::onLineStyleChanged(int index)
{
    if (!m_line)
        return;
    m_line->setLineStyle(static_cast<Qwt3DLine::LineStyle>(m_lineStyleCombo->itemData(index).toInt()));
    updatePlot();
}

void LineSettingsDock::onLineWidth(double val)
{
    if (!m_line)
        return;
    m_line->setLineWidth(val);
    updatePlot();
}

void LineSettingsDock::onTubeRadius(double val)
{
    if (!m_line)
        return;
    m_line->setTubeRadius(val);
    updatePlot();
}

void LineSettingsDock::onTubeSegments(int val)
{
    if (!m_line)
        return;
    m_line->setTubeSegments(val);
    updatePlot();
}

void LineSettingsDock::onPointSize(double val)
{
    if (!m_line)
        return;
    m_line->setPointSize(val);
    updatePlot();
}

void LineSettingsDock::onPointVisible(bool on)
{
    if (!m_line)
        return;
    m_line->setPointVisible(on);
    updatePlot();
}

void LineSettingsDock::onPointShapeChanged(int index)
{
    if (!m_line)
        return;
    m_line->setPointShape(static_cast<Qwt3DLine::PointShape>(m_pointShapeCombo->itemData(index).toInt()));
    updatePlot();
}

void LineSettingsDock::onColorPresetChanged(int index)
{
    if (!m_line)
        return;
    Q_UNUSED(index);
    m_colorFunctor = new Qwt3DColorMapColor(m_colorPresetCombo->currentText(), 256);
    m_colorFunctor->setAlpha(m_colorAlphaSlider->value() / 255.0);
    m_line->setDataColor(m_colorFunctor);
    updatePlot();
}

void LineSettingsDock::onColorAlpha(int val)
{
    if (!m_colorFunctor)
        return;
    m_colorFunctor->setAlpha(val / 255.0);
    if (m_line)
        m_line->invalidateColors();
    updatePlot();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void LineSettingsDock::updatePlot()
{
    if (m_plot)
        m_plot->update();
}

void LineSettingsDock::setColorButton(QPushButton* btn, const QColor& color)
{
    styleColorButton(btn, color);
}

QColor LineSettingsDock::getColorFromButton(QPushButton* btn) const
{
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

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

QWidget* LineSettingsDock::createContentWidget()
{
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(6);

    // --- Data group ---
    {
        auto* group = new QGroupBox(QStringLiteral("Data"));
        auto* form = new QFormLayout(group);

        m_sampleCountSpin = new QSpinBox;
        m_sampleCountSpin->setRange(10, 500);
        m_sampleCountSpin->setValue(240);
        form->addRow(QStringLiteral("Sample Count:"), m_sampleCountSpin);

        m_curveTypeCombo = new QComboBox;
        m_curveTypeCombo->addItem(QStringLiteral("Helix"), 0);
        m_curveTypeCombo->addItem(QStringLiteral("Spiral"), 1);
        m_curveTypeCombo->addItem(QStringLiteral("Lissajous"), 2);
        form->addRow(QStringLiteral("Curve Type:"), m_curveTypeCombo);

        m_helixTurnsSpin = new QDoubleSpinBox;
        m_helixTurnsSpin->setRange(1.0, 10.0);
        m_helixTurnsSpin->setSingleStep(0.5);
        m_helixTurnsSpin->setValue(4.0);
        m_helixTurnsSpin->setToolTip(QStringLiteral("Number of turns (controls t range)"));
        form->addRow(QStringLiteral("Turns:"), m_helixTurnsSpin);

        m_regenerateBtn = new QPushButton(QStringLiteral("Regenerate"));
        connect(m_regenerateBtn, &QPushButton::clicked, this, &LineSettingsDock::onRegenerateData);
        auto* regenLayout = new QHBoxLayout;
        regenLayout->addStretch();
        regenLayout->addWidget(m_regenerateBtn);
        form->addRow(QString(), regenLayout);

        layout->addWidget(group);
    }

    // --- Style group ---
    {
        auto* group = new QGroupBox(QStringLiteral("Style"));
        auto* form = new QFormLayout(group);

        m_lineStyleCombo = new QComboBox;
        m_lineStyleCombo->addItem(QStringLiteral("Lines"), int(Qwt3DLine::Lines));
        m_lineStyleCombo->addItem(QStringLiteral("Tube"), int(Qwt3DLine::Tube));
        m_lineStyleCombo->addItem(QStringLiteral("Dots"), int(Qwt3DLine::Dots));
        m_lineStyleCombo->setCurrentIndex(0);
        connect(m_lineStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &LineSettingsDock::onLineStyleChanged);
        form->addRow(QStringLiteral("Line Style:"), m_lineStyleCombo);

        layout->addWidget(group);
    }

    // --- Geometry group ---
    {
        auto* group = new QGroupBox(QStringLiteral("Geometry"));
        auto* form = new QFormLayout(group);

        m_lineWidthSpin = new QDoubleSpinBox;
        m_lineWidthSpin->setRange(0.5, 5.0);
        m_lineWidthSpin->setSingleStep(0.1);
        m_lineWidthSpin->setValue(1.0);
        m_lineWidthSpin->setToolTip(QStringLiteral("Line width (Lines style only; >1 not guaranteed in Core)"));
        connect(m_lineWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &LineSettingsDock::onLineWidth);
        form->addRow(QStringLiteral("Line Width:"), m_lineWidthSpin);

        m_tubeRadiusSpin = new QDoubleSpinBox;
        m_tubeRadiusSpin->setRange(0.0, 0.5);
        m_tubeRadiusSpin->setSingleStep(0.01);
        m_tubeRadiusSpin->setValue(0.05);
        m_tubeRadiusSpin->setToolTip(QStringLiteral("0 means auto (0.5% of hull diagonal)"));
        connect(m_tubeRadiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &LineSettingsDock::onTubeRadius);
        form->addRow(QStringLiteral("Tube Radius (0=auto):"), m_tubeRadiusSpin);

        m_tubeSegmentsSpin = new QSpinBox;
        m_tubeSegmentsSpin->setRange(3, 32);
        m_tubeSegmentsSpin->setValue(10);
        connect(m_tubeSegmentsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &LineSettingsDock::onTubeSegments);
        form->addRow(QStringLiteral("Tube Segments:"), m_tubeSegmentsSpin);

        m_pointSizeSpin = new QDoubleSpinBox;
        m_pointSizeSpin->setRange(1.0, 30.0);
        m_pointSizeSpin->setSingleStep(0.5);
        m_pointSizeSpin->setValue(10.0);
        connect(m_pointSizeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &LineSettingsDock::onPointSize);
        form->addRow(QStringLiteral("Point Size:"), m_pointSizeSpin);

        m_pointVisibleCheck = new QCheckBox(QStringLiteral("Draw markers on top of line/tube"));
        connect(m_pointVisibleCheck, &QCheckBox::toggled, this, &LineSettingsDock::onPointVisible);
        form->addRow(QString(), m_pointVisibleCheck);

        m_pointShapeCombo = new QComboBox;
        m_pointShapeCombo->addItem(QStringLiteral("Dot (GL_POINTS)"), int(Qwt3DLine::Dot));
        m_pointShapeCombo->addItem(QStringLiteral("Cube"), int(Qwt3DLine::Cube));
        m_pointShapeCombo->addItem(QStringLiteral("Tetrahedron"), int(Qwt3DLine::Tetrahedron));
        m_pointShapeCombo->addItem(QStringLiteral("Octahedron"), int(Qwt3DLine::Octahedron));
        m_pointShapeCombo->addItem(QStringLiteral("Sphere"), int(Qwt3DLine::Sphere));
        connect(m_pointShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &LineSettingsDock::onPointShapeChanged);
        form->addRow(QStringLiteral("Marker Shape:"), m_pointShapeCombo);

        layout->addWidget(group);
    }

    // --- Color group ---
    {
        auto* group = new QGroupBox(QStringLiteral("Color"));
        auto* form = new QFormLayout(group);

        m_colorPresetCombo = new QComboBox;
        m_colorPresetCombo->addItems(QwtColorMapPreset::availablePresets());
        m_colorPresetCombo->setCurrentText(QStringLiteral("plasma"));
        connect(m_colorPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &LineSettingsDock::onColorPresetChanged);
        form->addRow(QStringLiteral("Preset:"), m_colorPresetCombo);

        m_colorAlphaSlider = new QSlider(Qt::Horizontal);
        m_colorAlphaSlider->setRange(0, 255);
        m_colorAlphaSlider->setValue(255);
        connect(m_colorAlphaSlider, &QSlider::valueChanged, this, &LineSettingsDock::onColorAlpha);
        form->addRow(QStringLiteral("Alpha:"), m_colorAlphaSlider);

        layout->addWidget(group);
    }

    layout->addStretch();
    return content;
}
