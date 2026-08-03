#ifndef BAR_SETTINGS_DOCK_H
#define BAR_SETTINGS_DOCK_H

#include <QDockWidget>

class QComboBox;
class QDoubleSpinBox;
class QPushButton;
class QSpinBox;
class QSlider;

class Qwt3DBar;
class Qwt3DColorMapColor;
class Qwt3DPlot;

/**
 * @brief Dock widget with interactive settings for Qwt3DBar
 * @details Provides controls for bar chart data regeneration, bar style,
 *          geometry (width/depth/baseline), mesh appearance, and color
 *          mapping. All changes are pushed to the attached Qwt3DBar
 *          immediately and the plot is repainted.
 *
 * The dock holds a non-owning pointer to a Qwt3DColorMapColor functor that
 * is also owned by the bar item (via setDataColor). reapplyAll() recreates
 * the functor because theme application replaces it.
 */
class BarSettingsDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit BarSettingsDock(QWidget* parent = nullptr);
    ~BarSettingsDock() override;

    void setPlot(Qwt3DPlot* plot);
    void setBar(Qwt3DBar* bar);

    /// Re-apply all current UI values to the bar (call after a theme switch)
    void reapplyAll();

private Q_SLOTS:
    void onRegenerateData();
    void onBarStyleChanged(int index);
    void onBarWidth(double val);
    void onBarDepth(double val);
    void onBaseline(double val);
    void onMeshColor();
    void onMeshLineWidth(double val);
    void onColorPresetChanged(int index);
    void onColorAlpha(int val);

private:
    QWidget* createContentWidget();
    void regenerateData();
    void applyMeshColor();
    void updatePlot();
    void setColorButton(QPushButton* btn, const QColor& color);
    QColor getColorFromButton(QPushButton* btn) const;

    Qwt3DPlot* m_plot = nullptr;
    Qwt3DBar* m_bar = nullptr;
    Qwt3DColorMapColor* m_colorFunctor = nullptr;

    // Data controls
    QSpinBox* m_gridResolutionSpin = nullptr;
    QComboBox* m_functionTypeCombo = nullptr;
    QPushButton* m_regenerateBtn = nullptr;

    // Bar style controls
    QComboBox* m_barStyleCombo = nullptr;

    // Geometry controls
    QDoubleSpinBox* m_barWidthSpin = nullptr;
    QDoubleSpinBox* m_barDepthSpin = nullptr;
    QDoubleSpinBox* m_baselineSpin = nullptr;

    // Mesh controls
    QPushButton* m_meshColorBtn = nullptr;
    QDoubleSpinBox* m_meshLineWidthSpin = nullptr;

    // Color controls
    QComboBox* m_colorPresetCombo = nullptr;
    QSlider* m_colorAlphaSlider = nullptr;
};

#endif // BAR_SETTINGS_DOCK_H
