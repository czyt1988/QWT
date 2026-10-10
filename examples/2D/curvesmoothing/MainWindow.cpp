#include "MainWindow.h"

#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

#include <cmath>
#include <random>

#include <qwt_axis.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_item.h>

namespace
{
    const int kNumPoints = 100000;
    const int kBenchmarkIterations = 50;

    double gaussianPeak(double x, double center, double sigma)
    {
        const double d = (x - center) / sigma;
        return std::exp(-0.5 * d * d);
    }

    // A spectrum like curve: broad humps and narrow peaks on top of
    // a noise floor with white noise jitter and sparse spikes.
    // The random generator uses a fixed seed, so that both plots show
    // the same samples and benchmark runs are reproducible.
    QVector< QPointF > generateSpectrumSamples()
    {
        std::mt19937 rng(0xC0FFEE);
        std::normal_distribution< double > jitter(0.0, 0.15);
        std::uniform_real_distribution< double > uniform(0.0, 1.0);

        QVector< QPointF > samples;
        samples.reserve(kNumPoints);

        for (int i = 0; i < kNumPoints; i++) {
            const double x = i;

            // slowly varying noise floor
            double y = 1.0 + 0.3 * std::sin(x / 8000.0);

            // broad humps
            y += 3.0 * gaussianPeak(x, 30000.0, 5000.0);
            y += 2.0 * gaussianPeak(x, 70000.0, 3000.0);

            // narrow spectral peaks, that are meant to survive the smoothing
            y += 5.0 * gaussianPeak(x, 15000.0, 300.0);
            y += 3.5 * gaussianPeak(x, 55000.0, 250.0);
            y += 4.5 * gaussianPeak(x, 85000.0, 400.0);

            // white noise jitter + sparse spikes: the pixel level noise
            y += jitter(rng);
            if (uniform(rng) < 0.01)
                y += 0.4 + 0.6 * uniform(rng);

            // sparse deep dropouts toward the bottom: the pixel level noise
            // of the lower side, that must not fill up the dips
            if (uniform(rng) < 0.008)
                y -= 0.8 + 0.8 * uniform(rng);

            // a deterministic deep dip, so that the preservation of the
            // bottom side can be verified at a known position
            if (i >= 44999 && i <= 45001)
                y -= 3.0;

            samples += QPointF(x, y);
        }

        return samples;
    }
}

void MainWindow::RenderStats::reset()
{
    lastMs = avgMs = minMs = maxMs = 0.0;
    count = 0;
}

void MainWindow::RenderStats::add(double ms)
{
    if (count == 0 || ms < minMs)
        minMs = ms;
    if (count == 0 || ms > maxMs)
        maxMs = ms;

    // incremental mean
    avgMs += (ms - avgMs) / (count + 1);
    count++;

    lastMs = ms;
}

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
    , m_plainPlot(nullptr)
    , m_smoothPlot(nullptr)
    , m_plainCurve(nullptr)
    , m_smoothCurve(nullptr)
    , m_plainTimeLabel(new QLabel(QStringLiteral("last: -"), this))
    , m_smoothTimeLabel(new QLabel(QStringLiteral("last: -"), this))
    , m_algorithmBox(new QComboBox(this))
    , m_downsampleBox(new QComboBox(this))
    , m_windowSpin(new QSpinBox(this))
    , m_orderSpin(new QSpinBox(this))
    , m_preserveSpin(new QDoubleSpinBox(this))
    , m_benchmarkButton(
          new QPushButton(QStringLiteral("Run Benchmark (%1 replots each)").arg(kBenchmarkIterations), this))
{
    m_samples = generateSpectrumSamples();

    m_plainPlot  = createPlot(QStringLiteral("No Smoothing ( FilterPointsLTTB only )"));
    m_plainCurve = createCurve(m_plainPlot);

    m_smoothPlot  = createPlot(QStringLiteral("Render Smoothing: Gaussian ( window 15 )"));
    m_smoothCurve = createCurve(m_smoothPlot);

    // enable the smoothing of the right plot, matching the initial
    // values of the controls below
    m_smoothCurve->setSmoothAlgorithm(QwtPlotCurve::GaussianSmoothing);
    m_smoothCurve->setSmoothWindow(15);
    m_smoothCurve->setSmoothPolynomialOrder(3);
    m_smoothCurve->setSmoothPreserveThreshold(3.0);

    // fixed scales, so that the replot times are not affected by autoscaling
    const QRectF br = m_plainCurve->data()->boundingRect();
    const double yMargin = 0.05 * (br.bottom() - br.top());

    for (auto* plot : { m_plainPlot, m_smoothPlot })
        plot->setAxisScale(QwtAxis::YLeft, br.top() - yMargin, br.bottom() + yMargin);

    auto* layout = new QGridLayout(this);
    layout->addWidget(m_plainPlot, 0, 0);
    layout->addWidget(m_smoothPlot, 0, 1);
    layout->addWidget(m_plainTimeLabel, 1, 0);
    layout->addWidget(m_smoothTimeLabel, 1, 1);
    layout->addLayout(buildControls(), 2, 0, 1, 2);
    layout->setRowStretch(0, 1);

    setWindowTitle(QStringLiteral("curvesmoothing - QwtPlotCurve Render Smoothing"));
}

QwtPlot* MainWindow::createPlot(const QString& title)
{
    auto* plot = new QwtPlot(this);
    plot->setTitle(title);
    plot->setCanvasBackground(Qt::white);

    // synchronous painting, so that the replot timing is accurate
    auto* canvas = static_cast< QwtPlotCanvas* >(plot->canvas());
    canvas->setPaintAttribute(QwtPlotCanvas::ImmediatePaint, true);

    auto* grid = new QwtPlotGrid();
    grid->setPen(Qt::gray, 0.0, Qt::DotLine);
    grid->attach(plot);

    plot->setAxisScale(QwtAxis::XBottom, 0.0, kNumPoints - 1.0);
    plot->setAxisTitle(QwtAxis::XBottom, QStringLiteral("Frequency Bin"));
    plot->setAxisTitle(QwtAxis::YLeft, QStringLiteral("Amplitude"));

    return plot;
}

QwtPlotCurve* MainWindow::createCurve(QwtPlot* plot)
{
    // the default paint attributes ( ClipPolygons | FilterPointsLTTB )
    // are left untouched: the comparison is about smoothing only
    auto* curve = new QwtPlotCurve(QStringLiteral("Spectrum"));
    curve->setPen(QColor("#1f77b4"), 1.0);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curve->setSamples(m_samples);
    curve->attach(plot);

    return curve;
}

QHBoxLayout* MainWindow::buildControls()
{
    auto* layout = new QHBoxLayout;

    layout->addWidget(new QLabel(QStringLiteral("Algorithm:"), this));

    m_algorithmBox->addItem(QStringLiteral("Gaussian"));
    m_algorithmBox->addItem(QStringLiteral("Savitzky-Golay"));
    layout->addWidget(m_algorithmBox);

    layout->addWidget(new QLabel(QStringLiteral("Window:"), this));

    // odd values only
    m_windowSpin->setRange(3, 99);
    m_windowSpin->setSingleStep(2);
    m_windowSpin->setValue(15);
    layout->addWidget(m_windowSpin);

    layout->addWidget(new QLabel(QStringLiteral("SG Order:"), this));

    m_orderSpin->setRange(1, 5);
    m_orderSpin->setValue(3);
    m_orderSpin->setEnabled(false);
    layout->addWidget(m_orderSpin);

    layout->addWidget(new QLabel(QStringLiteral("Preserve:"), this));

    // feature preservation threshold in multiples of the noise level,
    // 0 disables it ( pure low pass filter )
    m_preserveSpin->setRange(0.0, 20.0);
    m_preserveSpin->setSingleStep(0.5);
    m_preserveSpin->setValue(3.0);
    m_preserveSpin->setToolTip(QStringLiteral("Multiples of the noise level above which peaks/dips "
                                                "keep their original height. 0 = pure low pass."));
    layout->addWidget(m_preserveSpin);

    layout->addWidget(new QLabel(QStringLiteral("Downsampling:"), this));

    m_downsampleBox->addItem(QStringLiteral("LTTB (default)"));
    m_downsampleBox->addItem(QStringLiteral("Pixel (QCustomPlot style)"));
    layout->addWidget(m_downsampleBox);

    layout->addWidget(m_benchmarkButton);
    layout->addStretch(1);

    layout->addWidget(
        new QLabel(QStringLiteral("%L1 samples | default LTTB downsampling on both plots").arg(kNumPoints), this));

    connect(m_algorithmBox, QOverload< int >::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::onAlgorithmChanged);
    connect(m_windowSpin, QOverload< int >::of(&QSpinBox::valueChanged), this, &MainWindow::onWindowChanged);
    connect(m_orderSpin, QOverload< int >::of(&QSpinBox::valueChanged), this, &MainWindow::onOrderChanged);
    connect(m_preserveSpin, QOverload< double >::of(&QDoubleSpinBox::valueChanged), this,
            &MainWindow::onPreserveChanged);
    connect(m_downsampleBox, QOverload< int >::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::onDownsampleChanged);
    connect(m_benchmarkButton, &QPushButton::clicked, this, &MainWindow::runBenchmark);

    return layout;
}

void MainWindow::applySmoothing()
{
    const bool isSavitzkyGolay = (m_algorithmBox->currentIndex() == 1);

    m_smoothCurve->setSmoothAlgorithm(
        isSavitzkyGolay ? QwtPlotCurve::SavitzkyGolaySmoothing : QwtPlotCurve::GaussianSmoothing);
    m_smoothCurve->setSmoothWindow(m_windowSpin->value());
    m_smoothCurve->setSmoothPolynomialOrder(m_orderSpin->value());
    m_smoothCurve->setSmoothPreserveThreshold(m_preserveSpin->value());

    const QString algorithmName = isSavitzkyGolay
        ? QStringLiteral("Savitzky-Golay ( order %1 )").arg(m_orderSpin->value())
        : QStringLiteral("Gaussian");

    m_smoothPlot->setTitle(
        QStringLiteral("Render Smoothing: %1 ( window %2 )").arg(algorithmName).arg(m_windowSpin->value()));

    replotMeasured(m_smoothPlot, m_smoothStats, m_smoothTimeLabel);
}

void MainWindow::remeasureAll()
{
    m_plainStats.reset();
    m_smoothStats.reset();

    replotMeasured(m_plainPlot, m_plainStats, m_plainTimeLabel);
    replotMeasured(m_smoothPlot, m_smoothStats, m_smoothTimeLabel);
}

void MainWindow::replotMeasured(QwtPlot* plot, RenderStats& stats, QLabel* label)
{
    QElapsedTimer timer;
    timer.start();

    plot->replot();

    stats.add(timer.nsecsElapsed() / 1.0e6);

    label->setText(QStringLiteral("last: %1 ms | avg: %2 ms | min: %3 ms | max: %4 ms | replots: %5")
                       .arg(stats.lastMs, 0, 'f', 2)
                       .arg(stats.avgMs, 0, 'f', 2)
                       .arg(stats.minMs, 0, 'f', 2)
                       .arg(stats.maxMs, 0, 'f', 2)
                       .arg(stats.count));
}

void MainWindow::onAlgorithmChanged(int index)
{
    m_orderSpin->setEnabled(index == 1);
    applySmoothing();
}

void MainWindow::onWindowChanged(int value)
{
    Q_UNUSED(value)

    applySmoothing();
}

void MainWindow::onOrderChanged(int value)
{
    Q_UNUSED(value)

    applySmoothing();
}

void MainWindow::onPreserveChanged(double value)
{
    Q_UNUSED(value)

    applySmoothing();
}

void MainWindow::onDownsampleChanged(int index)
{
    const bool usePixel = (index == 1);

    for (auto* curve : { m_plainCurve, m_smoothCurve }) {
        curve->setPaintAttribute(QwtPlotCurve::FilterPointsLTTB, !usePixel);
        curve->setPaintAttribute(QwtPlotCurve::FilterPointsPixel, usePixel);
    }

    // the downsampling affects both plots: start a fresh comparison
    remeasureAll();
}

void MainWindow::runBenchmark()
{
    m_benchmarkButton->setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);

    m_plainStats.reset();
    m_smoothStats.reset();

    for (int i = 0; i < kBenchmarkIterations; i++) {
        replotMeasured(m_plainPlot, m_plainStats, m_plainTimeLabel);
        replotMeasured(m_smoothPlot, m_smoothStats, m_smoothTimeLabel);
    }

    QApplication::restoreOverrideCursor();
    m_benchmarkButton->setEnabled(true);
}

void MainWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    // the plots have their final sizes when being shown
    remeasureAll();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    if (isVisible())
        remeasureAll();
}
