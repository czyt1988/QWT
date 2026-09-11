#ifndef LINE_SETTINGS_DOCK_H
#define LINE_SETTINGS_DOCK_H

#include <QDockWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QPushButton;
class QSpinBox;
class QSlider;

class Qwt3DColorMapColor;
class Qwt3DLine;
class Qwt3DPlot;

/**
 * @brief Dock widget with interactive settings for Qwt3DLine
 * @details Provides controls for line data regeneration (sample count,
 *          curve type, turns), line style (Lines/Tube/Dots), geometry
 *          (line width, tube radius/segments, point size, markers), and
 *          color mapping. All changes are pushed to the attached Qwt3DLine
 *          immediately and the plot is repainted.
 *
 * The dock holds a non-owning pointer to a Qwt3DColorMapColor functor that
 * is also owned by the line item (via setDataColor). reapplyAll() recreates
 * the functor because theme application replaces it.
 */
class LineSettingsDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit LineSettingsDock(QWidget* parent = nullptr);
    ~LineSettingsDock() override;

    void setPlot(Qwt3DPlot* plot);
    void setLine(Qwt3DLine* line);

    /// Re-apply all current UI values to the line (call after a theme switch)
    void reapplyAll();

private Q_SLOTS:
    void onRegenerateData();
    void onLineStyleChanged(int index);
    void onLineWidth(double val);
    void onTubeRadius(double val);
    void onTubeSegments(int val);
    void onPointSize(double val);
    void onPointVisible(bool on);
    void onPointShapeChanged(int index);
    void onColorPresetChanged(int index);
    void onColorAlpha(int val);

private:
    QWidget* createContentWidget();
    void regenerateData();
    void updatePlot();
    void setColorButton(QPushButton* btn, const QColor& color);
    QColor getColorFromButton(QPushButton* btn) const;

    Qwt3DPlot* m_plot = nullptr;
    Qwt3DLine* m_line = nullptr;
    Qwt3DColorMapColor* m_colorFunctor = nullptr;

    // Data controls
    QSpinBox* m_sampleCountSpin = nullptr;
    QComboBox* m_curveTypeCombo = nullptr;
    QDoubleSpinBox* m_helixTurnsSpin = nullptr;
    QPushButton* m_regenerateBtn = nullptr;

    // Style controls
    QComboBox* m_lineStyleCombo = nullptr;

    // Geometry controls
    QDoubleSpinBox* m_lineWidthSpin = nullptr;
    QDoubleSpinBox* m_tubeRadiusSpin = nullptr;
    QSpinBox* m_tubeSegmentsSpin = nullptr;
    QDoubleSpinBox* m_pointSizeSpin = nullptr;
    QCheckBox* m_pointVisibleCheck = nullptr;
    QComboBox* m_pointShapeCombo = nullptr;

    // Color controls
    QComboBox* m_colorPresetCombo = nullptr;
    QSlider* m_colorAlphaSlider = nullptr;
};

#endif // LINE_SETTINGS_DOCK_H
