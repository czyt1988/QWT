#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QPointF>
#include <QString>
#include <QVector>
#include <QWidget>

class QwtPlot;
class QwtPlotCanvasZoomer;
class QwtPlotCurve;

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGridLayout;
class QLabel;
class QPushButton;
class QSpinBox;

/*
    Compares the render smoothing of QwtPlotCurve:

    The left plot paints a noisy spectrum ( 100,000 samples ) with
    downsampling only, the right plot paints the same samples with an
    additional smoothing algorithm. Both panels measure and display the
    time of their replots.

    The downsampling combo box selects any of the downsampling algorithms
    of QwtPlotCurve::PaintAttribute for both plots, so that the interaction
    of downsampling and smoothing can be compared. As the downsampling is
    done while painting, the zoom level decides whether it is active at all:
    the bucket based methods fall back to painting every sample, when the
    visible range holds less than ~8 samples per pixel - the info label and
    the tooltips of the preset buttons report the value of the current view.

    The plots can be zoomed - by rubber band selection, by the preset
    windows or by the zoom stack buttons. The zoom is always synchronized
    between both plots, so that the same samples are inspected on both
    sides, and the y axes are scaled to the visible samples, when Auto Y
    is enabled.
 */
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    //! Downsampling algorithms offered by the combo box
    enum DownsampleMethod
    {
        DownsampleNone = 0,
        DownsampleFilterPoints,
        DownsampleFilterPointsAggressive,
        DownsampleFilterPointsPixel,
        DownsampleFilterPointsLTTB,
        DownsampleFilterPointsMinMaxLTTB,

        DownsampleMethodCount
    };

    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // Select a downsampling method ( also used by the screenshot variants )
    void setDownsampleMethod(DownsampleMethod method);

    // Zoom both plots to a window of count samples, centered at center
    void zoomToSampleWindow(double center, double count);

    // Zoom both plots to the full data range
    void zoomToFullRange();

protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private Q_SLOTS:
    void onAlgorithmChanged(int index);
    void onWindowChanged(int value);
    void onOrderChanged(int value);
    void onPreserveChanged(double value);
    void onDownsampleChanged(int index);
    void onAutoYChanged(bool on);
    void onZoomEnabledChanged(bool on);
    void fitYToVisibleRange();
    void runBenchmark();

private:
    // Rendering statistics of one plot panel
    struct RenderStats
    {
        RenderStats()
            : lastMs(0.0)
            , avgMs(0.0)
            , minMs(0.0)
            , maxMs(0.0)
            , count(0)
        {
        }

        void reset();
        void add(double ms);

        double lastMs;
        double avgMs;
        double minMs;
        double maxMs;
        int count;
    };

    QwtPlot* createPlot(const QString& title);
    QwtPlotCurve* createCurve(QwtPlot* plot);
    QGridLayout* buildControls();

    void setupZoom();
    void applySmoothing();
    void applyDownsampleAttributes();
    void updateTitles();
    void updateFilterInfo();
    void applyZoomToPlots(double xMin, double xMax);
    void fitYScales();
    void mirrorZoom(QwtPlotCanvasZoomer* source, QwtPlot* targetPlot, QwtPlotCanvasZoomer* targetZoomer);
    int visibleSampleCount(double xMin, double xMax) const;

    void remeasureAll();
    void replotMeasured(QwtPlot* plot, RenderStats& stats, QLabel* label);

    QwtPlot* m_plainPlot;
    QwtPlot* m_smoothPlot;
    QwtPlotCurve* m_plainCurve;
    QwtPlotCurve* m_smoothCurve;

    QwtPlotCanvasZoomer* m_plainZoomer;
    QwtPlotCanvasZoomer* m_smoothZoomer;
    bool m_syncingZoom;

    QLabel* m_plainTimeLabel;
    QLabel* m_smoothTimeLabel;
    QLabel* m_filterInfoLabel;

    QComboBox* m_algorithmBox;
    QComboBox* m_downsampleBox;
    QSpinBox* m_windowSpin;
    QSpinBox* m_orderSpin;
    QDoubleSpinBox* m_preserveSpin;
    QCheckBox* m_zoomCheck;
    QCheckBox* m_autoYCheck;
    QPushButton* m_benchmarkButton;

    QVector< QPointF > m_samples;
    QVector< double > m_yValues;  //!< Cached y values of m_samples

    // y range of the unzoomed plots, so that the replot times of the
    // benchmark stay reproducible
    double m_fullYMin;
    double m_fullYMax;

    RenderStats m_plainStats;
    RenderStats m_smoothStats;
};

#endif
