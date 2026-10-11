#ifndef WATERFALL_MAINWINDOW_H
#define WATERFALL_MAINWINDOW_H

#include <QMainWindow>

#include "qwt3d_waterfall.h"

class QComboBox;
class QLabel;
class QSlider;
class QSplitter;
class QTimer;

class QwtPlot;
class QwtPlotCurve;
class Qwt3DPlot;

class SpectrumGenerator;

/**
 * @brief Live 3D waterfall demo window
 * @details Top: a 2D QwtPlot showing the live spectrum and its peak hold.
 *          Bottom: a Qwt3DPlot with a Qwt3DWaterfall sliding window fed by
 *          the same frame. Both views always display the identical frame.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private Q_SLOTS:
    void onTick();
    void onRunToggled(bool running);
    void onReset();
    void onWindowCapacityChanged(int index);
    void onStyleChanged(int index);
    void onColorMapChanged(int index);
    void onColorModeChanged(int index);
    void onOpacityChanged(int value);
    void onResetView();

public:
    /// Automation hook: cycles styles/color modes/window sizes and saves
    /// per-stage screenshots into screenshotDir, then quits the application
    void runSelfTest(const QString& screenshotDir);

    /// Returns the status bar line (running state, fps, traces, bins)
    QString statusLine() const;

private:
    QWidget* createTopPlot();
    QWidget* createBottomPlot();
    void createToolbar();
    void createStatusBar();
    void setupWaterfallDefaults();
    void rebuildDepthAxisLabels();
    void fillWindow();
    void updateStatusBar();

    SpectrumGenerator* m_generator = nullptr;

    QTimer* m_timer = nullptr;
    QSplitter* m_splitter = nullptr;

    // 2D live spectrum
    QwtPlot* m_topPlot = nullptr;
    QwtPlotCurve* m_spectrumCurve = nullptr;
    QwtPlotCurve* m_peakCurve = nullptr;

    // 3D waterfall
    Qwt3DPlot* m_plot3d = nullptr;
    Qwt3DWaterfall* m_waterfall = nullptr;

    // Toolbar controls
    QComboBox* m_capacityCombo = nullptr;
    QComboBox* m_styleCombo = nullptr;
    QComboBox* m_cmapCombo = nullptr;
    QComboBox* m_colorModeCombo = nullptr;
    QSlider* m_opacitySlider = nullptr;

    // Status bar
    QLabel* m_statusLabel = nullptr;

    // FPS accounting
    int m_frameCounter = 0;
    double m_fps = 0.0;
    qint64 m_fpsEpochMs = 0;
};

#endif // WATERFALL_MAINWINDOW_H
