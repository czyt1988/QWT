#include "MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QSpinBox>

#include <algorithm>
#include <cmath>
#include <random>

#include <qwt_axis.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_canvas_zoomer.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_item.h>

namespace
{
    const int kNumPoints = 100000;
    const int kBenchmarkIterations = 50;

    const char* const kDownsampleNames[ MainWindow::DownsampleMethodCount ] = {
        "None ( raw samples )",
        "FilterPoints",
        "FilterPointsAggressive",
        "FilterPointsPixel",
        "FilterPointsLTTB",
        "FilterPointsMinMaxLTTB"};

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

    // Copies the y values in [pos, end) into a value vector: the same
    // result as QVector::mid(), but without the implicit sharing.
    QVector< double > copyYValues(const QVector< double >& values, int pos, int end)
    {
        QVector< double > out;
        const int count = end - pos;
        if (count <= 0)
            return out;

        out.resize(count);
        std::copy(values.constBegin() + pos, values.constBegin() + end, out.begin());
        return out;
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
    , m_plainZoomer(nullptr)
    , m_smoothZoomer(nullptr)
    , m_syncingZoom(false)
    , m_plainTimeLabel(new QLabel(QStringLiteral("last: -"), this))
    , m_smoothTimeLabel(new QLabel(QStringLiteral("last: -"), this))
    , m_filterInfoLabel(new QLabel(this))
    , m_algorithmBox(new QComboBox(this))
    , m_downsampleBox(new QComboBox(this))
    , m_windowSpin(new QSpinBox(this))
    , m_orderSpin(new QSpinBox(this))
    , m_preserveSpin(new QDoubleSpinBox(this))
    , m_zoomCheck(new QCheckBox(QStringLiteral("Zoom"), this))
    , m_autoYCheck(new QCheckBox(QStringLiteral("Auto Y"), this))
    , m_benchmarkButton(
          new QPushButton(QStringLiteral("Run Benchmark (%1 replots each)").arg(kBenchmarkIterations), this))
    , m_fullYMin(0.0)
    , m_fullYMax(0.0)
{
    m_samples = generateSpectrumSamples();

    m_yValues.reserve(m_samples.size());
    for (int i = 0; i < m_samples.size(); i++)
        m_yValues += m_samples[ i ].y();

    m_plainPlot  = createPlot(QStringLiteral("No Smoothing"));
    m_plainCurve = createCurve(m_plainPlot);

    m_smoothPlot  = createPlot(QStringLiteral("Render Smoothing"));
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

    m_fullYMin = br.top() - yMargin;
    m_fullYMax = br.bottom() + yMargin;

    for (auto* plot : { m_plainPlot, m_smoothPlot })
        plot->setAxisScale(QwtAxis::YLeft, m_fullYMin, m_fullYMax);

    auto* layout = new QGridLayout(this);
    layout->addWidget(m_plainPlot, 0, 0);
    layout->addWidget(m_smoothPlot, 0, 1);
    layout->addWidget(m_plainTimeLabel, 1, 0);
    layout->addWidget(m_smoothTimeLabel, 1, 1);
    layout->addLayout(buildControls(), 2, 0, 1, 2);
    layout->setRowStretch(0, 1);

    setupZoom();

    updateTitles();
    updateFilterInfo();

    setWindowTitle(QStringLiteral("curvesmoothing - Downsampling vs. Render Smoothing"));
}

MainWindow::~MainWindow()
{
    delete m_plainZoomer;
    delete m_smoothZoomer;
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
    auto* curve = new QwtPlotCurve(QStringLiteral("Spectrum"));
    curve->setPen(QColor("#1f77b4"), 1.0);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curve->setSamples(m_samples);

    // the default downsampling is FilterPointsLTTB, see applyDownsampleAttributes()
    curve->attach(plot);

    return curve;
}

QGridLayout* MainWindow::buildControls()
{
    auto* layout = new QGridLayout;
    layout->setHorizontalSpacing(8);

    // row 1: smoothing
    auto* smoothRow = new QHBoxLayout;
    smoothRow->addWidget(new QLabel(QStringLiteral("Algorithm:"), this));

    m_algorithmBox->addItem(QStringLiteral("Gaussian"));
    m_algorithmBox->addItem(QStringLiteral("Savitzky-Golay"));
    smoothRow->addWidget(m_algorithmBox);

    smoothRow->addWidget(new QLabel(QStringLiteral("Window:"), this));

    // odd values only
    m_windowSpin->setRange(3, 99);
    m_windowSpin->setSingleStep(2);
    m_windowSpin->setValue(15);
    smoothRow->addWidget(m_windowSpin);

    smoothRow->addWidget(new QLabel(QStringLiteral("SG Order:"), this));

    m_orderSpin->setRange(1, 5);
    m_orderSpin->setValue(3);
    m_orderSpin->setEnabled(false);
    smoothRow->addWidget(m_orderSpin);

    smoothRow->addWidget(new QLabel(QStringLiteral("Preserve:"), this));

    // feature preservation threshold in multiples of the noise level,
    // 0 disables it ( pure low pass filter )
    m_preserveSpin->setRange(0.0, 20.0);
    m_preserveSpin->setSingleStep(0.5);
    m_preserveSpin->setValue(3.0);
    m_preserveSpin->setToolTip(QStringLiteral("Multiples of the noise level above which peaks/dips "
                                              "keep their original height. 0 = pure low pass."));
    smoothRow->addWidget(m_preserveSpin);

    smoothRow->addStretch(1);
    smoothRow->addWidget(m_benchmarkButton);

    layout->addLayout(smoothRow, 0, 0);

    // row 2: downsampling
    auto* downsampleRow = new QHBoxLayout;
    downsampleRow->addWidget(new QLabel(QStringLiteral("Downsampling:"), this));

    for (int i = 0; i < DownsampleMethodCount; i++)
        m_downsampleBox->addItem(QLatin1String(kDownsampleNames[ i ]));

    m_downsampleBox->setCurrentIndex(DownsampleFilterPointsLTTB);
    m_downsampleBox->setToolTip(QStringLiteral("QwtPlotCurve::PaintAttribute, applied to both plots."));
    downsampleRow->addWidget(m_downsampleBox);

    downsampleRow->addStretch(1);
    layout->addLayout(downsampleRow, 1, 0);

    // row 3: zoom
    auto* zoomRow = new QHBoxLayout;
    zoomRow->addWidget(new QLabel(QStringLiteral("Zoom:"), this));

    m_zoomCheck->setChecked(true);
    m_zoomCheck->setToolTip(QStringLiteral("Rubber band zoom ( left button ), zoom out ( right button ),\n"
                                           "wheel zoom and undo/redo/home keys."));
    zoomRow->addWidget(m_zoomCheck);

    struct Preset
    {
        const char* text;
        double center;
        double count;
    };

    // the windows show the different regimes of the comparison: downsampling
    // active, the fallback threshold around 8 samples/pixel ( the bucket
    // based methods use 4 * canvasWidth buckets ) and single features
    const Preset presets[] = {
        {"Hump 30000", 30000.0, 30000.0},
        {"Peak 15000", 15000.0, 1200.0},
        {"Dip 45000", 45000.0, 600.0},
        {"Peak 85000", 85000.0, 400.0}};

    for (const Preset& preset : presets) {
        auto* button = new QPushButton(QLatin1String(preset.text), this);

        const double center = preset.center;
        const double count = preset.count;

        const int canvasWidth = qMax(1, m_plainPlot->canvas()->width());
        const double samplesPerPixel = count / canvasWidth;

        button->setToolTip(QStringLiteral("Zoom both plots to the window [%1, %2]\n%3 samples/pixel: %4")
                               .arg(center - 0.5 * count)
                               .arg(center + 0.5 * count)
                               .arg(samplesPerPixel, 0, 'f', 1)
                               .arg(samplesPerPixel < 8.0 ? QStringLiteral("below the filter threshold")
                                                          : QStringLiteral("downsampling active")));

        connect(button, &QPushButton::clicked, this, [ this, center, count ]() { zoomToSampleWindow(center, count); });
        zoomRow->addWidget(button);
    }

    auto* fullRangeButton = new QPushButton(QStringLiteral("Full range"), this);
    connect(fullRangeButton, &QPushButton::clicked, this, &MainWindow::zoomToFullRange);
    zoomRow->addWidget(fullRangeButton);

    auto* undoButton = new QPushButton(QStringLiteral("Undo"), this);
    undoButton->setToolTip(QStringLiteral("Zoom out ( one step of the zoom stack )"));
    connect(undoButton, &QPushButton::clicked, this, [ this ]() {
        if (m_plainZoomer)
            m_plainZoomer->zoom(-1);
    });
    zoomRow->addWidget(undoButton);

    auto* redoButton = new QPushButton(QStringLiteral("Redo"), this);
    redoButton->setToolTip(QStringLiteral("Zoom in ( one step of the zoom stack )"));
    connect(redoButton, &QPushButton::clicked, this, [ this ]() {
        if (m_plainZoomer)
            m_plainZoomer->zoom(+1);
    });
    zoomRow->addWidget(redoButton);

    auto* resetButton = new QPushButton(QStringLiteral("Reset"), this);
    resetButton->setToolTip(QStringLiteral("Back to the initial view"));
    connect(resetButton, &QPushButton::clicked, this, &MainWindow::zoomToFullRange);
    zoomRow->addWidget(resetButton);

    m_autoYCheck->setChecked(true);
    m_autoYCheck->setToolTip(QStringLiteral("Scale the y axes to the visible samples: without it the\n"
                                            "smoothing is hard to see in a zoomed window."));
    zoomRow->addWidget(m_autoYCheck);

    zoomRow->addStretch(1);
    zoomRow->addWidget(m_filterInfoLabel);

    layout->addLayout(zoomRow, 2, 0);

    connect(m_algorithmBox, QOverload< int >::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::onAlgorithmChanged);
    connect(m_windowSpin, QOverload< int >::of(&QSpinBox::valueChanged), this, &MainWindow::onWindowChanged);
    connect(m_orderSpin, QOverload< int >::of(&QSpinBox::valueChanged), this, &MainWindow::onOrderChanged);
    connect(m_preserveSpin, QOverload< double >::of(&QDoubleSpinBox::valueChanged), this,
            &MainWindow::onPreserveChanged);
    connect(m_downsampleBox, QOverload< int >::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::onDownsampleChanged);
    connect(m_zoomCheck, &QCheckBox::toggled, this, &MainWindow::onZoomEnabledChanged);
    connect(m_autoYCheck, &QCheckBox::toggled, this, &MainWindow::onAutoYChanged);
    connect(m_benchmarkButton, &QPushButton::clicked, this, &MainWindow::runBenchmark);

    return layout;
}

void MainWindow::setupZoom()
{
    // A zoomer, that can be switched off completely: the inherited
    // keyboard handling alone would zoom even when disabled.
    class Zoomer : public QwtPlotCanvasZoomer
    {
      public:
        Zoomer(QWidget* canvas) : QwtPlotCanvasZoomer(canvas) { }

      protected:
        void widgetKeyPressEvent(QKeyEvent* event) override
        {
            if (!isEnabled()) {
                event->ignore();
                return;
            }

            QwtPlotCanvasZoomer::widgetKeyPressEvent(event);
        }

        void widgetMousePressEvent(QMouseEvent* event) override
        {
            if (!isEnabled()) {
                event->ignore();
                return;
            }

            QwtPlotCanvasZoomer::widgetMousePressEvent(event);
        }
    };

    m_plainZoomer  = new Zoomer(m_plainPlot->canvas());
    m_smoothZoomer = new Zoomer(m_smoothPlot->canvas());

    for (QwtPlotCanvasZoomer* zoomer : { m_plainZoomer, m_smoothZoomer }) {
        zoomer->setRubberBand(QwtPicker::RectRubberBand);
        zoomer->setRubberBandPen(QPen(QColor("#1f77b4"), 1, Qt::DotLine));
        zoomer->setTrackerMode(QwtPicker::ActiveOnly);
        zoomer->setTrackerPen(QColor("#333333"));
        zoomer->setEnabled(m_zoomCheck->isChecked());
    }

    // both plots always show the same window
    connect(m_plainZoomer, &QwtPlotCanvasZoomer::zoomed, this,
            [ this ](const QList< QwtPlotCanvasZoomState >&) {
                mirrorZoom(m_plainZoomer, m_smoothPlot, m_smoothZoomer);
            });
    connect(m_smoothZoomer, &QwtPlotCanvasZoomer::zoomed, this,
            [ this ](const QList< QwtPlotCanvasZoomState >&) {
                mirrorZoom(m_smoothZoomer, m_plainPlot, m_plainZoomer);
            });
}

// The user zooms one canvas, the other one follows. Both canvases show the
// same data in the same axis ranges, so the zoom states can be transferred
// between them, whatever the pixel size of the canvas is.
void MainWindow::mirrorZoom(QwtPlotCanvasZoomer* source, QwtPlot* targetPlot, QwtPlotCanvasZoomer* targetZoomer)
{
    if (m_syncingZoom || targetPlot == nullptr)
        return;

    const QList< QwtPlotCanvasZoomState > states = source->zoomState();
    for (int i = 0; i < states.size(); i++) {
        const QwtPlotCanvasZoomState& state = states[ i ];
        if (state.plot != targetPlot)
            continue;

        m_syncingZoom = true;

        // the source rescale() has already been done, so the state of
        // the source canvas is the target of this mirror
        targetPlot->setAxisScale(QwtAxis::XBottom, state.axisInterval[ QwtAxis::XBottom ].minValue(),
                                 state.axisInterval[ QwtAxis::XBottom ].maxValue());

        if (m_autoYCheck->isChecked()) {
            // the same visible samples give the same y range on both plots
            targetPlot->setAxisScale(QwtAxis::YLeft, source->plot()->axisScaleDiv(QwtAxis::YLeft).interval().minValue(),
                                     source->plot()->axisScaleDiv(QwtAxis::YLeft).interval().maxValue());
        } else {
            targetPlot->setAxisScale(QwtAxis::YLeft, state.axisInterval[ QwtAxis::YLeft ].minValue(),
                                     state.axisInterval[ QwtAxis::YLeft ].maxValue());
        }

        // the stack of the mirrored zoomer starts over at the new window
        targetZoomer->setZoomBase(false);
        targetPlot->replot();

        m_syncingZoom = false;
        break;
    }

    updateFilterInfo();
}

void MainWindow::applyZoomToPlots(double xMin, double xMax)
{
    m_plainPlot->setAxisScale(QwtAxis::XBottom, xMin, xMax);
    m_smoothPlot->setAxisScale(QwtAxis::XBottom, xMin, xMax);

    if (m_autoYCheck->isChecked())
        fitYScales();

    // the zoom stack starts over at the new window
    m_plainZoomer->setZoomBase(false);
    m_smoothZoomer->setZoomBase(false);

    m_plainPlot->replotAll();

    updateFilterInfo();
}

void MainWindow::zoomToSampleWindow(double center, double count)
{
    const double xMin = qMax(0.0, center - 0.5 * count);
    const double xMax = qMin(double(kNumPoints - 1), center + 0.5 * count);

    applyZoomToPlots(xMin, xMax);
}

void MainWindow::zoomToFullRange()
{
    applyZoomToPlots(0.0, kNumPoints - 1.0);
}

// Scale the y axes to the visible samples, padded like the initial scales.
// Both plots share the same samples, so both get the same y range.
void MainWindow::fitYScales()
{
    const QwtInterval xInterval = m_plainPlot->axisScaleDiv(QwtAxis::XBottom).interval();
    const int from = qMax(0, int(std::floor(xInterval.minValue())));
    const int to   = qMin(kNumPoints - 1, int(std::ceil(xInterval.maxValue())));

    if (from > to)
        return;

    const QVector< double > values = copyYValues(m_yValues, from, to + 1);
    if (values.isEmpty())
        return;

    const auto minMax = std::minmax_element(values.constBegin(), values.constEnd());

    double yMin = *minMax.first;
    double yMax = *minMax.second;
    const double margin = 0.05 * (yMax - yMin);
    if (!(margin > 0.0))
        return;

    yMin -= margin;
    yMax += margin;

    for (auto* plot : { m_plainPlot, m_smoothPlot })
        plot->setAxisScale(QwtAxis::YLeft, yMin, yMax);
}

void MainWindow::fitYToVisibleRange()
{
    fitYScales();

    m_plainPlot->replotAll();
    updateFilterInfo();
}

void MainWindow::onAutoYChanged(bool on)
{
    if (on)
        fitYToVisibleRange();
}

void MainWindow::onZoomEnabledChanged(bool on)
{
    m_plainZoomer->setEnabled(on);
    m_smoothZoomer->setEnabled(on);
}

void MainWindow::applyDownsampleAttributes()
{
    const int method = m_downsampleBox->currentIndex();

    for (auto* curve : { m_plainCurve, m_smoothCurve }) {
        curve->setPaintAttribute(QwtPlotCurve::FilterPoints, false);
        curve->setPaintAttribute(QwtPlotCurve::FilterPointsAggressive, false);
        curve->setPaintAttribute(QwtPlotCurve::FilterPointsPixel, false);
        curve->setPaintAttribute(QwtPlotCurve::FilterPointsLTTB, false);
        curve->setPaintAttribute(QwtPlotCurve::FilterPointsMinMaxLTTB, false);

        switch (method) {
        case DownsampleNone:
            break;
        case DownsampleFilterPoints:
            curve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
            break;
        case DownsampleFilterPointsAggressive:
            curve->setPaintAttribute(QwtPlotCurve::FilterPointsAggressive, true);
            break;
        case DownsampleFilterPointsPixel:
            curve->setPaintAttribute(QwtPlotCurve::FilterPointsPixel, true);
            break;
        case DownsampleFilterPointsLTTB:
            curve->setPaintAttribute(QwtPlotCurve::FilterPointsLTTB, true);
            break;
        case DownsampleFilterPointsMinMaxLTTB:
            curve->setPaintAttribute(QwtPlotCurve::FilterPointsMinMaxLTTB, true);
            break;
        default:
            break;
        }
    }
}

void MainWindow::setDownsampleMethod(DownsampleMethod method)
{
    if (method < 0 || method >= DownsampleMethodCount)
        return;

    m_downsampleBox->setCurrentIndex(method);
}

void MainWindow::applySmoothing()
{
    const bool isSavitzkyGolay = (m_algorithmBox->currentIndex() == 1);

    m_smoothCurve->setSmoothAlgorithm(
        isSavitzkyGolay ? QwtPlotCurve::SavitzkyGolaySmoothing : QwtPlotCurve::GaussianSmoothing);
    m_smoothCurve->setSmoothWindow(m_windowSpin->value());
    m_smoothCurve->setSmoothPolynomialOrder(m_orderSpin->value());
    m_smoothCurve->setSmoothPreserveThreshold(m_preserveSpin->value());

    updateTitles();

    replotMeasured(m_smoothPlot, m_smoothStats, m_smoothTimeLabel);
}

void MainWindow::updateTitles()
{
    const QString downsample = QString::fromLatin1(kDownsampleNames[ m_downsampleBox->currentIndex() ]);

    const bool isSavitzkyGolay = (m_algorithmBox->currentIndex() == 1);
    const QString algorithmName = isSavitzkyGolay
        ? QStringLiteral("Savitzky-Golay ( order %1 )").arg(m_orderSpin->value())
        : QStringLiteral("Gaussian");

    const QString smoothing = QStringLiteral("Render Smoothing: %1 ( window %2 )")
                                  .arg(algorithmName)
                                  .arg(m_windowSpin->value());

    m_plainPlot->setTitle(QStringLiteral("No Smoothing | Downsampling: %1").arg(downsample));
    m_smoothPlot->setTitle(QStringLiteral("%1 | Downsampling: %2").arg(smoothing).arg(downsample));
}

// Reports how the visible range relates to the paint resolution: the
// downsampling methods filter, as soon as a pixel column has to hold
// more samples than the filter can keep.
void MainWindow::updateFilterInfo()
{
    const int width = m_plainPlot->canvas()->width();
    if (width <= 0)
        return;

    const QwtInterval xInterval = m_plainPlot->axisScaleDiv(QwtAxis::XBottom).interval();
    const int visible = visibleSampleCount(xInterval.minValue(), xInterval.maxValue());
    const double samplesPerPixel = double(visible) / width;

    // the bucket based methods use 4 * canvasWidth buckets, so that
    // below ~8 samples per pixel all methods fall back to plain mapping
    QString note;
    if (samplesPerPixel < 8.0)
        note = QStringLiteral("below the filter threshold: all methods paint every sample");
    else
        note = QStringLiteral("filter active: the methods differ here");

    m_filterInfoLabel->setText(QStringLiteral("visible: %L1 samples ( %2 samples/pixel ) - %3")
                                   .arg(visible)
                                   .arg(samplesPerPixel, 0, 'f', 1)
                                   .arg(note));
}

int MainWindow::visibleSampleCount(double xMin, double xMax) const
{
    const auto begin = m_samples.constBegin();
    const auto end   = m_samples.constEnd();

    const auto first = std::lower_bound(begin, end, xMin, [](const QPointF& sample, double x) {
        return sample.x() < x;
    });

    const auto last = std::upper_bound(begin, end, xMax, [](double x, const QPointF& sample) {
        return x < sample.x();
    });

    return int(last - first);
}

void MainWindow::remeasureAll()
{
    m_plainStats.reset();
    m_smoothStats.reset();

    replotMeasured(m_plainPlot, m_plainStats, m_plainTimeLabel);
    replotMeasured(m_smoothPlot, m_smoothStats, m_smoothTimeLabel);

    updateFilterInfo();
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
    Q_UNUSED(index)

    applyDownsampleAttributes();
    updateTitles();

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
