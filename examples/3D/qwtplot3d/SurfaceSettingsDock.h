#ifndef SURFACE_SETTINGS_DOCK_H
#define SURFACE_SETTINGS_DOCK_H

#include <QDockWidget>

#include <QList>
#include <QMap>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QPushButton;
class QSpinBox;
class QLineEdit;
class QListWidget;
class QSlider;
class QTabWidget;

class Qwt3DColorMapColor;
class Qwt3DEnrichment;
class Qwt3DPlot;
class Qwt3DSurface;
class Qwt3DTheme;

/**
 * @brief Dock widget with tabbed settings for all Qwt3DPlot and Qwt3DSurface properties.
 * @details Holds pointers to the plot and surface and applies changes directly.
 *          Five tabs: Surface, Axes, Legend, View & Light, Enrichments.
 */
class SurfaceSettingsDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit SurfaceSettingsDock(QWidget* parent = nullptr);
    ~SurfaceSettingsDock() override;

    void setPlot(Qwt3DPlot* plot);
    void setSurface(Qwt3DSurface* surface);

    /// Re-apply all current UI values to the plot/surface (call after data switch)
    void reapplyAll();

    /// Sync all UI controls to match the given theme (call before reapplyAll after a theme switch)
    void syncFromTheme(const Qwt3DTheme& theme);

private Q_SLOTS:
    // Surface tab
    void onPlotStyleChanged(int index);
    void onFloorStyleChanged(int index);
    void onShadingChanged(int index);
    void onMeshColor();
    void onMeshLineWidth(double val);
    void onSmoothMesh(bool on);
    void onIsolines(int val);
    void onPolygonOffset(double val);
    void onResolution(int val);
    void onColorPresetChanged(int index);
    void onColorAlpha(int val);
    void onShowNormals(bool on);
    void onNormalLength(double val);
    void onNormalQuality(int val);

    // Axes tab
    void onAxisSelectionChanged(int index);
    void onAxisLabelChanged(const QString& text);
    void onAxisMajors(int val);
    void onAxisMinors(int val);
    void onAxisNumbers(bool on);
    void onAxisScaling(bool on);
    void onAxisAutoScale(bool on);
    void onAxisScaleTypeChanged(int index);
    void onAxisLineWidth(double val);
    void onAxisTicMajor(double val);
    void onAxisTicMinor(double val);
    void onAxisSymmetricTics(bool on);
    void onAxesColor();
    void onGridLinesColor();
    void onGridSideToggled();
    void onGridMajorsToggled(bool on);
    void onGridMinorsToggled(bool on);
    void onAutoDecoration(bool on);
    void onTickPositionChanged(int index);
    void onLineSmooth(bool on);

    // Legend tab
    void onLegendOrientationChanged(int index);
    void onLegendScalePositionChanged(int index);
    void onLegendDrawScale(bool on);
    void onLegendDrawNumbers(bool on);
    void onLegendAutoScale(bool on);
    void onLegendMajors(int val);
    void onLegendMinors(int val);
    void onLegendTitleChanged(const QString& text);
    void onLegendLimitStart(double val);
    void onLegendLimitStop(double val);

    // View & Light tab
    void onRotationChanged();
    void onShiftChanged();
    void onViewportShiftChanged();
    void onScaleChanged();
    void onZoomChanged(double val);
    void onBackgroundColor();
    void onLightingEnabled(bool on);
    void onIlluminateLight(int idx);
    void onLightRotationChanged();
    void onLightShiftChanged();
    void onShininess(double val);
    void onTitleChanged(const QString& text);
    void onTitleColor();
    void onTitlePositionChanged();

    // Enrichments tab
    void onAddEnrichment();
    void onRemoveEnrichment();

private:
    // Tab creation
    QWidget* createSurfaceTab();
    QWidget* createAxesTab();
    QWidget* createLegendTab();
    QWidget* createViewLightTab();
    QWidget* createEnrichmentsTab();

    // Helpers
    void loadAxisValues();
    void applyAxisValues();
    void applyMeshColor();
    void applyAxesColor();
    void applyGridLinesColor();
    void applyBackgroundColor();
    void applyTitleColor();
    int currentAxisIndex() const;
    int computeGridSides() const;
    void updatePlot();
    void setColorButton(QPushButton* btn, const class QColor& color);
    QColor getColorFromButton(QPushButton* btn) const;
    static QString axisName(int axis);

    Qwt3DPlot* m_plot = nullptr;
    Qwt3DSurface* m_surface = nullptr;
    Qwt3DColorMapColor* m_colorFunctor = nullptr;

    QTabWidget* m_tabs = nullptr;

    // --- Surface tab ---
    QComboBox* m_plotStyleCombo = nullptr;
    QComboBox* m_floorStyleCombo = nullptr;
    QComboBox* m_shadingCombo = nullptr;
    QPushButton* m_meshColorBtn = nullptr;
    QDoubleSpinBox* m_meshLineWidthSpin = nullptr;
    QCheckBox* m_smoothMeshCheck = nullptr;
    QSpinBox* m_isolinesSpin = nullptr;
    QDoubleSpinBox* m_polygonOffsetSpin = nullptr;
    QSpinBox* m_resolutionSpin = nullptr;
    QComboBox* m_colorPresetCombo = nullptr;
    QSlider* m_colorAlphaSlider = nullptr;
    QCheckBox* m_showNormalsCheck = nullptr;
    QDoubleSpinBox* m_normalLengthSpin = nullptr;
    QSpinBox* m_normalQualitySpin = nullptr;

    // --- Axes tab ---
    QComboBox* m_axisSelector = nullptr;
    QLineEdit* m_axisLabelEdit = nullptr;
    QSpinBox* m_axisMajorsSpin = nullptr;
    QSpinBox* m_axisMinorsSpin = nullptr;
    QCheckBox* m_axisNumbersCheck = nullptr;
    QCheckBox* m_axisScalingCheck = nullptr;
    QCheckBox* m_axisAutoScaleCheck = nullptr;
    QComboBox* m_axisScaleTypeCombo = nullptr;
    QDoubleSpinBox* m_axisLineWidthSpin = nullptr;
    QDoubleSpinBox* m_axisTicMajorSpin = nullptr;
    QDoubleSpinBox* m_axisTicMinorSpin = nullptr;
    QCheckBox* m_axisSymmetricTicsCheck = nullptr;
    QPushButton* m_axesColorBtn = nullptr;
    QPushButton* m_gridLinesColorBtn = nullptr;
    QCheckBox* m_gridSideChecks[6] = {};
    QCheckBox* m_gridMajorsCheck = nullptr;
    QCheckBox* m_gridMinorsCheck = nullptr;
    QCheckBox* m_autoDecorationCheck = nullptr;
    QComboBox* m_tickPositionCombo = nullptr;
    QCheckBox* m_lineSmoothCheck = nullptr;

    // --- Legend tab ---
    QComboBox* m_legendOrientationCombo = nullptr;
    QComboBox* m_legendScalePosCombo = nullptr;
    QCheckBox* m_legendDrawScaleCheck = nullptr;
    QCheckBox* m_legendDrawNumbersCheck = nullptr;
    QCheckBox* m_legendAutoScaleCheck = nullptr;
    QSpinBox* m_legendMajorsSpin = nullptr;
    QSpinBox* m_legendMinorsSpin = nullptr;
    QLineEdit* m_legendTitleEdit = nullptr;
    QDoubleSpinBox* m_legendLimitStartSpin = nullptr;
    QDoubleSpinBox* m_legendLimitStopSpin = nullptr;

    // --- View & Light tab ---
    QDoubleSpinBox* m_rotXSpin = nullptr;
    QDoubleSpinBox* m_rotYSpin = nullptr;
    QDoubleSpinBox* m_rotZSpin = nullptr;
    QDoubleSpinBox* m_shiftXSpin = nullptr;
    QDoubleSpinBox* m_shiftYSpin = nullptr;
    QDoubleSpinBox* m_shiftZSpin = nullptr;
    QDoubleSpinBox* m_vpShiftXSpin = nullptr;
    QDoubleSpinBox* m_vpShiftYSpin = nullptr;
    QDoubleSpinBox* m_scaleXSpin = nullptr;
    QDoubleSpinBox* m_scaleYSpin = nullptr;
    QDoubleSpinBox* m_scaleZSpin = nullptr;
    QDoubleSpinBox* m_zoomSpin = nullptr;
    QPushButton* m_bgColorBtn = nullptr;
    QCheckBox* m_lightingCheck = nullptr;
    QPushButton* m_lightButtons[8] = {};
    QDoubleSpinBox* m_lightRotXSpin = nullptr;
    QDoubleSpinBox* m_lightRotYSpin = nullptr;
    QDoubleSpinBox* m_lightRotZSpin = nullptr;
    QDoubleSpinBox* m_lightShiftXSpin = nullptr;
    QDoubleSpinBox* m_lightShiftYSpin = nullptr;
    QDoubleSpinBox* m_lightShiftZSpin = nullptr;
    QDoubleSpinBox* m_shininessSpin = nullptr;
    QLineEdit* m_titleEdit = nullptr;
    QPushButton* m_titleColorBtn = nullptr;
    QDoubleSpinBox* m_titleRelYSpin = nullptr;
    QDoubleSpinBox* m_titleRelXSpin = nullptr;
    QComboBox* m_titleAnchorCombo = nullptr;

    // --- Enrichments tab ---
    QComboBox* m_enrichmentTypeCombo = nullptr;
    QPushButton* m_addEnrichmentBtn = nullptr;
    QListWidget* m_enrichmentList = nullptr;
    QPushButton* m_removeEnrichmentBtn = nullptr;
    QList<Qwt3DEnrichment*> m_enrichments;
};

#endif // SURFACE_SETTINGS_DOCK_H
