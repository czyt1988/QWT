#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

class QAction;
class QActionGroup;
class QComboBox;
class QLabel;
class QTimer;

class Qwt3DPlot;
class Qwt3DSurface;
class SurfaceDataProvider;
class SurfaceSettingsDock;

/**
 * @brief Main window for the qwtplot3d comprehensive demo.
 * @details Features a menu bar, top toolbar for Qwt3DPlot window-level controls,
 *          a left dock for Qwt3DSurface item-level property settings, and a
 *          center Qwt3DPlot widget. All settable 3D properties are exposed
 *          for verification.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private Q_SLOTS:
    void onDataChanged(int index);
    void onAnimateToggled(bool on);
    void onTimerTick();
    void onCoordStyleTriggered(QAction* action);
    void onThemeChanged(int index);
    void onLightingToggled(bool on);
    void onLegendToggled(bool on);
    void onOrthoToggled(bool on);
    void onGridToggled(bool on);
    void onResetView();
    void onSavePixmap();
    void onToggleMouse(bool on);
    void onToggleKeyboard(bool on);

    void showRotation(double x, double y, double z);
    void showZoom(double z);

private:
    void createWidgets();
    void createMenuBar();
    void createToolBar();
    void switchData(int index);

    Qwt3DPlot* m_plot = nullptr;
    Qwt3DSurface* m_surface = nullptr;
    SurfaceSettingsDock* m_dock = nullptr;
    SurfaceDataProvider* m_currentProvider = nullptr;

    QComboBox* m_dataCombo = nullptr;
    QAction* m_animateAction = nullptr;
    QComboBox* m_themeCombo = nullptr;

    QTimer* m_animTimer = nullptr;
    int m_currentDataIndex = 0;

    // Status bar labels
    QLabel* m_rotationLabel = nullptr;
    QLabel* m_zoomLabel = nullptr;
    QLabel* m_dataLabel = nullptr;
};

#endif // MAIN_WINDOW_H
