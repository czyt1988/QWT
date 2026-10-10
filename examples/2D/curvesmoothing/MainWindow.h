#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QWidget>
#include <QPointF>
#include <QString>
#include <QVector>

class QwtPlot;
class QwtPlotCurve;

class QComboBox;
class QDoubleSpinBox;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QSpinBox;

/*
    Compares the render smoothing of QwtPlotCurve:

    The left plot paints a noisy spectrum ( 100,000 samples ) with the
    default settings, what means downsampling only ( FilterPointsLTTB ).
    The right plot paints the same samples with an additional smoothing
    algorithm.

    Both panels measure and display the time of their replots.
 */
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private Q_SLOTS:
    void onAlgorithmChanged(int index);
    void onWindowChanged(int value);
    void onOrderChanged(int value);
    void onPreserveChanged(double value);
    void onDownsampleChanged(int index);
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
    QHBoxLayout* buildControls();

    void applySmoothing();
    void remeasureAll();
    void replotMeasured(QwtPlot* plot, RenderStats& stats, QLabel* label);

    QwtPlot* m_plainPlot;
    QwtPlot* m_smoothPlot;
    QwtPlotCurve* m_plainCurve;
    QwtPlotCurve* m_smoothCurve;

    QLabel* m_plainTimeLabel;
    QLabel* m_smoothTimeLabel;

    QComboBox* m_algorithmBox;
    QComboBox* m_downsampleBox;
    QSpinBox* m_windowSpin;
    QSpinBox* m_orderSpin;
    QDoubleSpinBox* m_preserveSpin;
    QPushButton* m_benchmarkButton;

    QVector< QPointF > m_samples;

    RenderStats m_plainStats;
    RenderStats m_smoothStats;
};

#endif
