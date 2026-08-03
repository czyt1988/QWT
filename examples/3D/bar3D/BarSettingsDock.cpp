#include "BarSettingsDock.h"

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
#include <vector>

#include <QVector>

#include "qwt3d_bar.h"
#include "qwt3d_colormap_color.h"
#include "qwt3d_plot.h"
#include "qwt3d_theme.h"
#include "qwt3d_types.h"
#include "qwt_colormap_preset.h"
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

BarSettingsDock::BarSettingsDock(QWidget* parent)
    : QDockWidget(QStringLiteral("Bar Settings"), parent)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setWidget(createContentWidget());
    setWidget(scroll);
}

BarSettingsDock::~BarSettingsDock()
{
    // m_colorFunctor is owned by the bar item, not destroyed here
}

void BarSettingsDock::setPlot(Qwt3DPlot* plot)
{
    m_plot = plot;
}

void BarSettingsDock::setBar(Qwt3DBar* bar)
{
    m_bar = bar;
    if (!m_bar)
        return;

    // Create the initial color functor from current UI state
    m_colorFunctor = new Qwt3DColorMapColor(m_colorPresetCombo->currentText(), 256);
    m_colorFunctor->setAlpha(m_colorAlphaSlider->value() / 255.0);
    m_bar->setDataColor(m_colorFunctor);
}

// ---------------------------------------------------------------------------
// Re-apply all UI values (call after a theme switch)
// ---------------------------------------------------------------------------

void BarSettingsDock::reapplyAll()
{
    if (!m_bar)
        return;

    // Theme application replaces the data color functor, so recreate it
    m_colorFunctor = new Qwt3DColorMapColor(m_colorPresetCombo->currentText(), 256);
    m_colorFunctor->setAlpha(m_colorAlphaSlider->value() / 255.0);
    m_bar->setDataColor(m_colorFunctor);

    // Re-apply bar properties from current UI state
    m_bar->setBarStyle(static_cast<Qwt3DBar::BarStyle>(m_barStyleCombo->currentData().toInt()));
    m_bar->setBarWidth(m_barWidthSpin->value());
    m_bar->setBarDepth(m_barDepthSpin->value());
    m_bar->setBaseline(m_baselineSpin->value());
    applyMeshColor();
    m_bar->setMeshLineWidth(m_meshLineWidthSpin->value());

    // Regenerate data with current parameters
    regenerateData();

    updatePlot();
}

// ---------------------------------------------------------------------------
// Data regeneration
// ---------------------------------------------------------------------------

void BarSettingsDock::regenerateData()
{
    if (!m_bar)
        return;

    const int cols = m_gridResolutionSpin->value();
    const int rows = cols;
    const double minX = -2.0, maxX = 2.0, minY = -2.0, maxY = 2.0;

    QVector<QVector<double>> z(cols, QVector<double>(rows));
    const int funcType = m_functionTypeCombo->currentData().toInt();

    for (int i = 0; i < cols; ++i) {
        const double x = minX + (maxX - minX) * i / (cols - 1);
        for (int j = 0; j < rows; ++j) {
            const double y = minY + (maxY - minY) * j / (rows - 1);
            switch (funcType) {
                case 0: // Gaussian peak
                    z[i][j] = std::exp(-(x * x + y * y) / 1.5);
                    break;
                case 1: // Saddle
                    z[i][j] = (x * x - y * y) * 0.3;
                    break;
                case 2: { // Ridge
                    z[i][j] = std::exp(-y * y) * std::cos(x);
                    break;
                }
                case 3: { // Sombrero
                    const double r = std::sqrt(x * x + y * y);
                    z[i][j] = r > 0.01 ? std::sin(r) / r : 1.0;
                    break;
                }
                case 4: // Wave
                    z[i][j] = std::sin(x) * std::cos(y);
                    break;
                default:
                    z[i][j] = 0.0;
                    break;
            }
        }
    }

    std::vector<double*> ptrs(cols);
    for (int i = 0; i < cols; ++i)
        ptrs[i] = z[i].data();

    m_bar->setSamples(ptrs.data(), cols, rows, minX, maxX, minY, maxY);
}

void BarSettingsDock::onRegenerateData()
{
    regenerateData();
    updatePlot();
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void BarSettingsDock::onBarStyleChanged(int index)
{
    if (!m_bar)
        return;
    m_bar->setBarStyle(static_cast<Qwt3DBar::BarStyle>(m_barStyleCombo->itemData(index).toInt()));
    updatePlot();
}

void BarSettingsDock::onBarWidth(double val)
{
    if (!m_bar)
        return;
    m_bar->setBarWidth(val);
    updatePlot();
}

void BarSettingsDock::onBarDepth(double val)
{
    if (!m_bar)
        return;
    m_bar->setBarDepth(val);
    updatePlot();
}

void BarSettingsDock::onBaseline(double val)
{
    if (!m_bar)
        return;
    m_bar->setBaseline(val);
    updatePlot();
}

void BarSettingsDock::onMeshColor()
{
    QColor c = QColorDialog::getColor(getColorFromButton(m_meshColorBtn), this,
                                      QStringLiteral("Mesh Color"),
                                      QColorDialog::ShowAlphaChannel);
    if (!c.isValid())
        return;
    setColorButton(m_meshColorBtn, c);
    applyMeshColor();
    updatePlot();
}

void BarSettingsDock::onMeshLineWidth(double val)
{
    if (!m_bar)
        return;
    m_bar->setMeshLineWidth(val);
    updatePlot();
}

void BarSettingsDock::onColorPresetChanged(int index)
{
    if (!m_bar)
        return;
    Q_UNUSED(index);
    m_colorFunctor = new Qwt3DColorMapColor(m_colorPresetCombo->currentText(), 256);
    m_colorFunctor->setAlpha(m_colorAlphaSlider->value() / 255.0);
    m_bar->setDataColor(m_colorFunctor);
    updatePlot();
}

void BarSettingsDock::onColorAlpha(int val)
{
    if (!m_colorFunctor)
        return;
    m_colorFunctor->setAlpha(val / 255.0);
    if (m_bar)
        m_bar->invalidateColors();
    updatePlot();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void BarSettingsDock::applyMeshColor()
{
    if (!m_bar)
        return;
    m_bar->setMeshColor(qColorToRGBA(getColorFromButton(m_meshColorBtn)));
}

void BarSettingsDock::updatePlot()
{
    if (m_plot)
        m_plot->update();
}

void BarSettingsDock::setColorButton(QPushButton* btn, const QColor& color)
{
    styleColorButton(btn, color);
}

QColor BarSettingsDock::getColorFromButton(QPushButton* btn) const
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

QWidget* BarSettingsDock::createContentWidget()
{
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(6);

    // --- Data group ---
    {
        auto* group = new QGroupBox(QStringLiteral("Data"));
        auto* form = new QFormLayout(group);

        m_gridResolutionSpin = new QSpinBox;
        m_gridResolutionSpin->setRange(5, 30);
        m_gridResolutionSpin->setValue(9);
        form->addRow(QStringLiteral("Grid Resolution:"), m_gridResolutionSpin);

        m_functionTypeCombo = new QComboBox;
        m_functionTypeCombo->addItem(QStringLiteral("Gaussian"), 0);
        m_functionTypeCombo->addItem(QStringLiteral("Saddle"), 1);
        m_functionTypeCombo->addItem(QStringLiteral("Ridge"), 2);
        m_functionTypeCombo->addItem(QStringLiteral("Sombrero"), 3);
        m_functionTypeCombo->addItem(QStringLiteral("Wave"), 4);
        form->addRow(QStringLiteral("Function Type:"), m_functionTypeCombo);

        m_regenerateBtn = new QPushButton(QStringLiteral("Regenerate"));
        connect(m_regenerateBtn, &QPushButton::clicked, this, &BarSettingsDock::onRegenerateData);
        auto* regenLayout = new QHBoxLayout;
        regenLayout->addStretch();
        regenLayout->addWidget(m_regenerateBtn);
        form->addRow(QString(), regenLayout);

        layout->addWidget(group);
    }

    // --- Bar style group ---
    {
        auto* group = new QGroupBox(QStringLiteral("Bar Style"));
        auto* form = new QFormLayout(group);

        m_barStyleCombo = new QComboBox;
        m_barStyleCombo->addItem(QStringLiteral("Filled"), int(Qwt3DBar::Filled));
        m_barStyleCombo->addItem(QStringLiteral("FilledMesh"), int(Qwt3DBar::FilledMesh));
        m_barStyleCombo->addItem(QStringLiteral("Wireframe"), int(Qwt3DBar::Wireframe));
        m_barStyleCombo->setCurrentIndex(1);
        connect(m_barStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &BarSettingsDock::onBarStyleChanged);
        form->addRow(QStringLiteral("Style:"), m_barStyleCombo);

        layout->addWidget(group);
    }

    // --- Geometry group ---
    {
        auto* group = new QGroupBox(QStringLiteral("Geometry"));
        auto* form = new QFormLayout(group);

        m_barWidthSpin = new QDoubleSpinBox;
        m_barWidthSpin->setRange(0.0, 2.0);
        m_barWidthSpin->setSingleStep(0.05);
        m_barWidthSpin->setValue(0.0);
        m_barWidthSpin->setToolTip(QStringLiteral("0 means auto (80% of spacing)"));
        connect(m_barWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &BarSettingsDock::onBarWidth);
        form->addRow(QStringLiteral("Bar Width (0=auto):"), m_barWidthSpin);

        m_barDepthSpin = new QDoubleSpinBox;
        m_barDepthSpin->setRange(0.0, 2.0);
        m_barDepthSpin->setSingleStep(0.05);
        m_barDepthSpin->setValue(0.0);
        m_barDepthSpin->setToolTip(QStringLiteral("0 means auto (80% of spacing)"));
        connect(m_barDepthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &BarSettingsDock::onBarDepth);
        form->addRow(QStringLiteral("Bar Depth (0=auto):"), m_barDepthSpin);

        m_baselineSpin = new QDoubleSpinBox;
        m_baselineSpin->setRange(-2.0, 2.0);
        m_baselineSpin->setSingleStep(0.1);
        m_baselineSpin->setValue(0.0);
        connect(m_baselineSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &BarSettingsDock::onBaseline);
        form->addRow(QStringLiteral("Baseline:"), m_baselineSpin);

        layout->addWidget(group);
    }

    // --- Mesh group ---
    {
        auto* group = new QGroupBox(QStringLiteral("Mesh"));
        auto* form = new QFormLayout(group);

        m_meshColorBtn = new QPushButton(QStringLiteral("Choose..."));
        styleColorButton(m_meshColorBtn, rgbaToQColor(RGBA(0.1, 0.1, 0.1, 0.4)));
        connect(m_meshColorBtn, &QPushButton::clicked, this, &BarSettingsDock::onMeshColor);
        form->addRow(QStringLiteral("Mesh Color:"), m_meshColorBtn);

        m_meshLineWidthSpin = new QDoubleSpinBox;
        m_meshLineWidthSpin->setRange(0.1, 5.0);
        m_meshLineWidthSpin->setSingleStep(0.1);
        m_meshLineWidthSpin->setValue(1.0);
        connect(m_meshLineWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &BarSettingsDock::onMeshLineWidth);
        form->addRow(QStringLiteral("Line Width:"), m_meshLineWidthSpin);

        layout->addWidget(group);
    }

    // --- Color group ---
    {
        auto* group = new QGroupBox(QStringLiteral("Color"));
        auto* form = new QFormLayout(group);

        m_colorPresetCombo = new QComboBox;
        m_colorPresetCombo->addItems(QwtColorMapPreset::availablePresets());
        m_colorPresetCombo->setCurrentText(QStringLiteral("viridis"));
        connect(m_colorPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &BarSettingsDock::onColorPresetChanged);
        form->addRow(QStringLiteral("Preset:"), m_colorPresetCombo);

        m_colorAlphaSlider = new QSlider(Qt::Horizontal);
        m_colorAlphaSlider->setRange(0, 255);
        m_colorAlphaSlider->setValue(255);
        connect(m_colorAlphaSlider, &QSlider::valueChanged, this, &BarSettingsDock::onColorAlpha);
        form->addRow(QStringLiteral("Alpha:"), m_colorAlphaSlider);

        layout->addWidget(group);
    }

    layout->addStretch();
    return content;
}
