#include "BenchSuites.h"

#include <QApplication>
#include <QDir>
#include <QScreen>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>

namespace chartbench
{

namespace
{

//! Deterministic seed so every library sees exactly the same samples.
quint32 seedFor(const QString& key)
{
    quint32 hash = 2166136261u;
    for (QChar c : key) {
        hash ^= static_cast<quint32>(c.unicode());
        hash *= 16777619u;
    }
    return hash | 1u;
}

QString dprTag()
{
    if (QScreen* screen = QApplication::primaryScreen())
        return QStringLiteral("dpr=%1").arg(screen->devicePixelRatio(), 0, 'f', 2);
    return QStringLiteral("dpr=?");
}

} // namespace

BenchSuites::BenchSuites(const BenchOptions& options, ResultWriter& writer)
    : m_options(options)
    , m_writer(writer)
{
}

BenchSuites::~BenchSuites()
{
    releaseAdapter();
    delete m_window;
    m_window = nullptr;
}

bool BenchSuites::initialize(QString& error)
{
    QString adapterError;
    m_adapter = createAdapter(m_options.library, m_options.backend, adapterError);
    if (!m_adapter) {
        error = adapterError;
        return false;
    }

    m_window = new QWidget(nullptr, Qt::Window);
    m_window->setWindowTitle(QStringLiteral("chartbench - %1").arg(m_adapter->displayName()));
    m_layout = new QVBoxLayout(m_window);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_window->resize(m_options.canvasWidth, m_options.canvasHeight);
    m_window->move(m_options.windowX, m_options.windowY);
    m_window->show();
    QApplication::processEvents();

    qInfo("chartbench: %s / %s", qPrintable(m_adapter->displayName()),
          qPrintable(m_adapter->backendDescription()));
    return true;
}

bool BenchSuites::isRequested(const QString& suite) const
{
    return m_requested.contains(QLatin1String("all")) || m_requested.contains(suite);
}

void BenchSuites::warmUpProcess()
{
    // The first configuration of a process carries one-off costs that the
    // five warm-up frames inside a configuration do not remove: GL driver and
    // shader initialisation, page faults on the library's first touch, timer
    // resolution. Measured without this, the first configuration of a process
    // is systematically slower than an identical later one (QIm: 1.30 ms
    // against 0.81 ms for the same 1000-point plot). Run one configuration and
    // discard it.
    DataGenerator generator(seedFor(QStringLiteral("process-warmup")));
    std::vector<std::vector<double>> x;
    std::vector<std::vector<double>> y;
    generator.generate(1, 1000, x, y);

    PlotSpec spec;
    spec.curveCount = 1;
    spec.pointsPerCurve = 1000;
    spec.widgetSize = QSize(m_options.canvasWidth, m_options.canvasHeight);

    QString error;
    if (!prepareAdapter(spec, error)) {
        qWarning("chartbench: process warm-up skipped: %s", qPrintable(error));
        releaseAdapter();
        return;
    }

    m_adapter->rebuild(x, y, spec);
    syncAdapterToSpec(spec);
    for (int i = 0; i < 3; ++i)
        QApplication::processEvents();
    for (int i = 0; i < 30; ++i)
        m_adapter->frame();
    releaseAdapter();
}

bool BenchSuites::run(const QStringList& suites)
{
    m_requested = suites;
    warmUpProcess();

    if (isRequested(QStringLiteral("selftest")))
        runSelftest();
    if (isRequested(QStringLiteral("points")))
        runPoints();
    if (isRequested(QStringLiteral("curves")))
        runCurves();
    if (isRequested(QStringLiteral("canvas")))
        runCanvas();
    if (isRequested(QStringLiteral("update")))
        runUpdate();
    if (isRequested(QStringLiteral("downsample")))
        runDownsample();
    if (isRequested(QStringLiteral("jitter")))
        runJitter();
    if (isRequested(QStringLiteral("memory")))
        runMemory();
    if (isRequested(QStringLiteral("fidelity")))
        runFidelity();

    return m_failures == 0;
}

// ---------------------------------------------------------------------------
// Adapter lifecycle
// ---------------------------------------------------------------------------

bool BenchSuites::prepareAdapter(const PlotSpec& spec, QString& error, double* createMs)
{
    releaseAdapter();

    if (m_window->isMinimized()) {
        error = QStringLiteral(
            "the benchmark window was minimized - frame times would be meaningless");
        return false;
    }

    // The layout of the window still carries the minimum size of the widget that
    // was just destroyed; without this a window that grew for a large canvas
    // cannot shrink again and the next configuration would be measured on the
    // wrong canvas size.
    m_layout->invalidate();
    m_window->setMinimumSize(QSize(0, 0));
    m_window->resize(spec.widgetSize);

    const Stopwatch createWatch;
    const bool created = m_adapter->create(m_window, spec, error);
    if (createMs)
        *createMs = createWatch.elapsedMs();
    if (!created)
        return false;

    QWidget* widget = m_adapter->widget();
    m_layout->addWidget(widget);
    widget->show();

    // Let the layout settle so that the widget really has the requested size.
    for (int i = 0; i < 2; ++i)
        QApplication::processEvents();

    if (widget->size() != spec.widgetSize) {
        qWarning("chartbench: canvas is %dx%d, expected %dx%d",
                 widget->width(), widget->height(), spec.widgetSize.width(),
                 spec.widgetSize.height());
    }
    return true;
}

void BenchSuites::releaseAdapter()
{
    if (!m_adapter)
        return;
    if (QWidget* widget = m_adapter->widget()) {
        m_layout->removeWidget(widget);
        widget->hide();
    }
    m_adapter->destroy();
}

void BenchSuites::syncAdapterToSpec(const PlotSpec& spec)
{
    if (spec.axisMode == AxisMode::Fixed)
        m_adapter->setAxesRange(spec.xMin, spec.xMax, spec.yMin, spec.yMax);
    else
        m_adapter->rescaleAxes();
}

// ---------------------------------------------------------------------------
// The measurement loop
// ---------------------------------------------------------------------------

void BenchSuites::measure(const MeasureRequest& request)
{
    const PlotSpec& spec = request.spec;
    const int maxFrames = request.maxFrames > 0 ? request.maxFrames : m_options.maxFrames;
    const double budgetMs = request.budgetMs > 0.0 ? request.budgetMs : m_options.frameBudgetMs;

    // The baseline is taken while the input data already exists, so the
    // reported delta is what the library itself adds.
    const MemoryInfo baseline = processMemory();

    QString error;
    const Stopwatch setupWatch;
    double createMs = 0.0;
    if (!prepareAdapter(spec, error, &createMs)) {
        ++m_failures;
        qWarning("chartbench: %s/%s skipped: %s", qPrintable(request.suite),
                 qPrintable(request.variant), qPrintable(error));
        releaseAdapter();
        return;
    }

    m_adapter->setDownsamplingAlgorithm(request.sampling);

    static const std::vector<std::vector<double>> kNoData;
    const Stopwatch handoverWatch;
    m_adapter->rebuild(request.x ? *request.x : kNoData, request.y ? *request.y : kNoData, spec);
    syncAdapterToSpec(spec);
    const double handoverMs = handoverWatch.elapsedMs();

    for (int i = 0; i < 3; ++i)
        QApplication::processEvents();

    m_adapter->frame();
    const double setupMs = setupWatch.elapsedMs();

    QWidget* widget = m_adapter->widget();
    const int canvasWidth = widget ? widget->width() : spec.widgetSize.width();
    const int canvasHeight = widget ? widget->height() : spec.widgetSize.height();

    const int totalPoints = spec.curveCount * spec.pointsPerCurve;
    const int windowSize = request.updateWindow > 0 ? request.updateWindow : spec.pointsPerCurve;
    const int stride = request.updateStride > 0 ? request.updateStride : 1;
    // The window slides over the update source, which for the streaming suites
    // is the full backing array rather than the initially visible slice.
    const std::vector<std::vector<double>>& slideSource = request.updateX ? *request.updateX
                                                                          : (request.x ? *request.x
                                                                                       : kNoData);
    const int span = slideSource.empty() ? 0 : static_cast<int>(slideSource.at(0).size());
    const int maxOffset = qMax(0, span - windowSize);

    const std::vector<std::vector<double>>& updateX = request.updateX ? *request.updateX : *request.x;
    const std::vector<std::vector<double>>& updateY = request.updateY ? *request.updateY : *request.y;

    // Warm up without recording.
    for (int i = 0; i < m_options.warmupFrames; ++i) {
        if (request.updateMode == UpdateMode::Window && maxOffset > 0) {
            const int offset = (i * stride) % maxOffset;
            m_adapter->updateWindow(0, updateX.at(0), updateY.at(0), offset, windowSize);
            if (request.trackAxes)
                m_adapter->setAxesRange(updateX.at(0)[offset], updateX.at(0)[offset + windowSize - 1],
                                        spec.yMin, spec.yMax);
        }
        m_adapter->frame();
    }

    ConfigSample config;
    config.suite = request.suite;
    config.library = m_adapter->libraryId();
    config.variant = request.variant;
    config.backend = m_options.backend;
    config.curves = spec.curveCount;
    config.pointsPerCurve = spec.pointsPerCurve;
    config.canvasWidth = canvasWidth;
    config.canvasHeight = canvasHeight;
    config.baselinePrivateMb = baseline.privateMb;
    config.note = dprTag() + QStringLiteral("; ") + m_adapter->backendDescription();
    if (canvasWidth != spec.widgetSize.width() || canvasHeight != spec.widgetSize.height()) {
        config.note += QStringLiteral("; CANVAS MISMATCH (wanted %1x%2)")
                           .arg(spec.widgetSize.width())
                           .arg(spec.widgetSize.height());
    }
    if (!request.note.isEmpty())
        config.note += QStringLiteral("; ") + request.note;

    const Stopwatch suiteWatch;
    int frames = 0;
    double gpuSyncSum = 0.0;
    double updateSum = 0.0;

    while (frames < maxFrames) {
        double updateMs = 0.0;

        if (request.updateMode == UpdateMode::Window && request.x && maxOffset > 0) {
            const int offset = (frames * stride) % maxOffset;
            const Stopwatch updateWatch;
            for (int c = 0; c < spec.curveCount; ++c)
                m_adapter->updateWindow(c, updateX.at(c), updateY.at(c), offset, windowSize);
            if (request.trackAxes)
                m_adapter->setAxesRange(updateX.at(0)[offset], updateX.at(0)[offset + windowSize - 1],
                                        spec.yMin, spec.yMax);
            updateMs = updateWatch.elapsedMs();
        }

        double renderMs = 0.0;
        if (!request.measureUpdateOnly) {
            const Stopwatch renderWatch;
            if (spec.axisMode == AxisMode::Rescale)
                m_adapter->rescaleAxes();
            m_adapter->frame();
            renderMs = renderWatch.elapsedMs();
            gpuSyncSum += m_adapter->lastGpuSyncMs();
        }

        FrameSample sample;
        sample.suite = request.suite;
        sample.library = config.library;
        sample.variant = request.variant;
        sample.backend = m_options.backend;
        sample.curves = spec.curveCount;
        sample.pointsPerCurve = spec.pointsPerCurve;
        sample.canvasWidth = canvasWidth;
        sample.canvasHeight = canvasHeight;
        sample.frameIndex = frames;
        sample.updateMs = updateMs;
        sample.renderMs = renderMs;
        sample.totalMs = updateMs + renderMs;
        if (frames == 0 || frames == maxFrames - 1) {
            const MemoryInfo info = processMemory();
            sample.privateMb = info.privateMb;
            sample.workingSetMb = info.workingSetMb;
        }
        m_writer.writeFrame(sample);
        updateSum += updateMs;

        ++frames;

        if (suiteWatch.elapsedMs() > budgetMs && frames >= 3) {
            config.aborted = true;
            break;
        }
    }

    const MemoryInfo after = processMemory();
    config.frames = frames;
    config.setupMs = setupMs;
    config.createMs = createMs;
    config.handoverMs = handoverMs;
    config.rebuildMs = frames > 0 ? updateSum / frames : 0.0;   //!< mean data handover cost
    config.privateMb = after.privateMb;
    config.peakPrivateMb = after.peakPrivateMb;
    config.workingSetMb = after.workingSetMb;
    config.peakWorkingSetMb = after.peakWorkingSetMb;
    config.cpuGpuSyncMs = frames > 0 ? gpuSyncSum / frames : 0.0;
    m_writer.writeConfig(config);

    if (m_options.verbose) {
        qInfo("  %-28s %-18s frames=%-4d setup=%7.2f ms  %s", qPrintable(request.suite),
              qPrintable(request.variant), frames, setupMs,
              config.aborted ? "ABORTED (budget)" : "");
    }

    releaseAdapter();
}

// ---------------------------------------------------------------------------
// Suites
// ---------------------------------------------------------------------------

void BenchSuites::runSelftest()
{
    const PlotSpec base = [&] {
        PlotSpec spec;
        spec.widgetSize = QSize(m_options.canvasWidth, m_options.canvasHeight);
        return spec;
    }();

    const int sizes[] = { 1000, 100000 };
    for (int n : sizes) {
        DataGenerator generator(seedFor(QStringLiteral("selftest%1").arg(n)));
        std::vector<std::vector<double>> x;
        std::vector<std::vector<double>> y;
        generator.generate(1, n, x, y);

        MeasureRequest request;
        request.suite = QStringLiteral("selftest");
        request.variant = QStringLiteral("n%1").arg(n);
        request.spec = base;
        request.spec.curveCount = 1;
        request.spec.pointsPerCurve = n;
        request.x = &x;
        request.y = &y;
        request.maxFrames = 5;
        request.budgetMs = 60000.0;
        measure(request);
    }
}

void BenchSuites::runPoints()
{
    const int sizes[] = { 1000, 5000, 10000, 50000, 100000, 250000, 500000, 1000000, 2000000, 5000000 };

    for (int n : sizes) {
        DataGenerator generator(seedFor(QStringLiteral("points%1").arg(n)));
        std::vector<std::vector<double>> x;
        std::vector<std::vector<double>> y;
        generator.generate(1, n, x, y);

        MeasureRequest request;
        request.suite = QStringLiteral("points");
        request.variant = QStringLiteral("n%1").arg(n);
        request.spec.curveCount = 1;
        request.spec.pointsPerCurve = n;
        request.spec.widgetSize = QSize(m_options.canvasWidth, m_options.canvasHeight);
        request.x = &x;
        request.y = &y;
        measure(request);
    }
}

void BenchSuites::runCurves()
{
    const int curveCounts[] = { 1, 2, 5, 10, 20, 50, 100 };
    const int pointsPerCurve = 2000;

    for (int curves : curveCounts) {
        DataGenerator generator(seedFor(QStringLiteral("curves%1").arg(curves)));
        std::vector<std::vector<double>> x;
        std::vector<std::vector<double>> y;
        generator.generate(curves, pointsPerCurve, x, y);

        MeasureRequest request;
        request.suite = QStringLiteral("curves");
        request.variant = QStringLiteral("c%1").arg(curves);
        request.spec.curveCount = curves;
        request.spec.pointsPerCurve = pointsPerCurve;
        request.spec.widgetSize = QSize(m_options.canvasWidth, m_options.canvasHeight);
        request.x = &x;
        request.y = &y;
        measure(request);
    }
}

void BenchSuites::runCanvas()
{
    const QSize sizes[] = { QSize(400, 300),  QSize(800, 600),   QSize(1200, 800),
                            QSize(1600, 1200), QSize(1920, 1080), QSize(2560, 1440) };

    for (const QSize& size : sizes) {
        // Single curve: isolates the polyline cost from the fill rate.
        {
            DataGenerator generator(seedFor(QStringLiteral("canvas-single%1").arg(size.width())));
            std::vector<std::vector<double>> x;
            std::vector<std::vector<double>> y;
            generator.generate(1, 100000, x, y);

            MeasureRequest request;
            request.suite = QStringLiteral("canvas");
            request.variant = QStringLiteral("single_%1x%2").arg(size.width()).arg(size.height());
            request.spec.curveCount = 1;
            request.spec.pointsPerCurve = 100000;
            request.spec.widgetSize = size;
            request.x = &x;
            request.y = &y;
            request.note = QStringLiteral("1 curve x 100k");
            measure(request);
        }
        // Many curves: isolates the per-series and fill-rate coupling.
        {
            DataGenerator generator(seedFor(QStringLiteral("canvas-many%1").arg(size.width())));
            std::vector<std::vector<double>> x;
            std::vector<std::vector<double>> y;
            generator.generate(20, 5000, x, y);

            MeasureRequest request;
            request.suite = QStringLiteral("canvas");
            request.variant = QStringLiteral("many_%1x%2").arg(size.width()).arg(size.height());
            request.spec.curveCount = 20;
            request.spec.pointsPerCurve = 5000;
            request.spec.widgetSize = size;
            request.x = &x;
            request.y = &y;
            request.note = QStringLiteral("20 curves x 5k");
            measure(request);
        }
    }
}

void BenchSuites::runUpdate()
{
    const int backingLength = 1000000;
    const int windows[] = { 1000, 10000, 100000 };

    for (int window : windows) {
        DataGenerator generator(seedFor(QStringLiteral("update%1").arg(window)));
        std::vector<std::vector<double>> x(1);
        std::vector<std::vector<double>> y(1);
        generator.generateStream(backingLength, x[0], y[0]);

        // The plot always shows `window` samples; the backing array is what the
        // window slides over. Without this the visible sample count would not
        // match pointsPerCurve and the x axis would not follow the window.
        std::vector<std::vector<double>> initialX(1, std::vector<double>(x[0].begin(),
                                                                        x[0].begin() + window));
        std::vector<std::vector<double>> initialY(1, std::vector<double>(y[0].begin(),
                                                                        y[0].begin() + window));

        const int stride = qMax(1, window / 10);

        // (a) pure rendering, data untouched
        {
            MeasureRequest request;
            request.suite = QStringLiteral("update");
            request.variant = QStringLiteral("render-only_w%1").arg(window);
            request.spec.curveCount = 1;
            request.spec.pointsPerCurve = window;
            request.spec.widgetSize = QSize(m_options.canvasWidth, m_options.canvasHeight);
            request.spec.xMin = initialX[0].front();
            request.spec.xMax = initialX[0].back();
            request.x = &initialX;
            request.y = &initialY;
            request.updateMode = UpdateMode::None;
            request.note = QStringLiteral("window=%1 samples").arg(window);
            measure(request);
        }
        // (b) data handover only, no repaint
        {
            MeasureRequest request;
            request.suite = QStringLiteral("update");
            request.variant = QStringLiteral("update-only_w%1").arg(window);
            request.spec.curveCount = 1;
            request.spec.pointsPerCurve = window;
            request.spec.widgetSize = QSize(m_options.canvasWidth, m_options.canvasHeight);
            request.spec.xMin = initialX[0].front();
            request.spec.xMax = initialX[0].back();
            request.x = &initialX;
            request.y = &initialY;
            request.updateX = &x;
            request.updateY = &y;
            request.updateMode = UpdateMode::Window;
            request.updateWindow = window;
            request.updateStride = stride;
            request.trackAxes = true;
            request.measureUpdateOnly = true;
            request.maxFrames = 30;
            request.note = QStringLiteral("window=%1 samples, no repaint").arg(window);
            measure(request);
        }
        // (c) streaming workload: update + repaint
        {
            MeasureRequest request;
            request.suite = QStringLiteral("update");
            request.variant = QStringLiteral("stream_w%1").arg(window);
            request.spec.curveCount = 1;
            request.spec.pointsPerCurve = window;
            request.spec.widgetSize = QSize(m_options.canvasWidth, m_options.canvasHeight);
            request.spec.xMin = initialX[0].front();
            request.spec.xMax = initialX[0].back();
            request.x = &initialX;
            request.y = &initialY;
            request.updateX = &x;
            request.updateY = &y;
            request.updateMode = UpdateMode::Window;
            request.updateWindow = window;
            request.updateStride = stride;
            request.trackAxes = true;
            request.note = QStringLiteral("window=%1 samples, sliding").arg(window);
            measure(request);
        }
    }
}

void BenchSuites::runDownsample()
{
    const int points = 1000000;
    DataGenerator generator(seedFor(QStringLiteral("downsample")));
    std::vector<std::vector<double>> x;
    std::vector<std::vector<double>> y;
    generator.generate(1, points, x, y);

    const QStringList algorithms = m_adapter->downsamplingAlgorithms();
    for (const QString& algorithm : algorithms) {
        MeasureRequest request;
        request.suite = QStringLiteral("downsample");
        request.variant = algorithm;
        request.sampling = algorithm;
        request.spec.curveCount = 1;
        request.spec.pointsPerCurve = points;
        request.spec.widgetSize = QSize(m_options.canvasWidth, m_options.canvasHeight);
        request.x = &x;
        request.y = &y;
        request.note = QStringLiteral("strategy=%1").arg(algorithm);
        measure(request);
    }
}

void BenchSuites::runJitter()
{
    const int points = 100000;
    const int window = 10000;

    DataGenerator generator(seedFor(QStringLiteral("jitter")));
    std::vector<std::vector<double>> x(1);
    std::vector<std::vector<double>> y(1);
    generator.generateStream(points, x[0], y[0]);

    std::vector<std::vector<double>> initialX(1, std::vector<double>(x[0].begin(),
                                                                    x[0].begin() + window));
    std::vector<std::vector<double>> initialY(1, std::vector<double>(y[0].begin(),
                                                                    y[0].begin() + window));

    MeasureRequest request;
    request.suite = QStringLiteral("jitter");
    request.variant = QStringLiteral("static_n%1").arg(points);
    request.spec.curveCount = 1;
    request.spec.pointsPerCurve = points;
    request.spec.widgetSize = QSize(m_options.canvasWidth, m_options.canvasHeight);
    // The stream generator uses the sample index as x, so the axis has to span
    // the array or almost every sample would be clipped away.
    request.spec.xMin = x[0].front();
    request.spec.xMax = x[0].back();
    request.x = &x;
    request.y = &y;
    request.maxFrames = 300;
    request.budgetMs = 120000.0;
    request.note = QStringLiteral("static data, 300 frames");
    measure(request);

    // The streaming configuration shows `window` samples and follows them with
    // the x axis, so the two variants differ in the visible sample count too.
    MeasureRequest streaming;
    streaming.suite = QStringLiteral("jitter");
    streaming.variant = QStringLiteral("stream_w%1").arg(window);
    streaming.spec.curveCount = 1;
    streaming.spec.pointsPerCurve = window;
    streaming.spec.widgetSize = QSize(m_options.canvasWidth, m_options.canvasHeight);
    streaming.spec.xMin = initialX[0].front();
    streaming.spec.xMax = initialX[0].back();
    streaming.x = &initialX;
    streaming.y = &initialY;
    streaming.updateX = &x;
    streaming.updateY = &y;
    streaming.updateMode = UpdateMode::Window;
    streaming.updateWindow = window;
    streaming.updateStride = window / 10;
    streaming.trackAxes = true;
    streaming.maxFrames = 300;
    streaming.budgetMs = 120000.0;
    streaming.note = QStringLiteral("sliding window of %1, 300 frames").arg(window);
    measure(streaming);
}

void BenchSuites::runMemory()
{
    const int sizes[] = { 1000, 10000, 100000, 1000000, 10000000 };

    for (int n : sizes) {
        DataGenerator generator(seedFor(QStringLiteral("memory%1").arg(n)));
        std::vector<std::vector<double>> x;
        std::vector<std::vector<double>> y;
        generator.generate(1, n, x, y);

        MeasureRequest request;
        request.suite = QStringLiteral("memory");
        request.variant = QStringLiteral("n%1").arg(n);
        request.spec.curveCount = 1;
        request.spec.pointsPerCurve = n;
        request.spec.widgetSize = QSize(m_options.canvasWidth, m_options.canvasHeight);
        request.x = &x;
        request.y = &y;
        request.maxFrames = 3;
        request.budgetMs = 120000.0;
        request.note = QStringLiteral("peak private bytes, 1 curve");
        measure(request);
    }

    // Many small series: exposes the per-series container overhead.
    {
        const int curves = 100;
        const int perCurve = 10000;
        DataGenerator generator(seedFor(QStringLiteral("memory-many")));
        std::vector<std::vector<double>> x;
        std::vector<std::vector<double>> y;
        generator.generate(curves, perCurve, x, y);

        MeasureRequest request;
        request.suite = QStringLiteral("memory");
        request.variant = QStringLiteral("c%1n%2").arg(curves).arg(perCurve);
        request.spec.curveCount = curves;
        request.spec.pointsPerCurve = perCurve;
        request.spec.widgetSize = QSize(m_options.canvasWidth, m_options.canvasHeight);
        request.x = &x;
        request.y = &y;
        request.maxFrames = 3;
        request.budgetMs = 120000.0;
        request.note = QStringLiteral("%1 curves x %2 points").arg(curves).arg(perCurve);
        measure(request);
    }
}

void BenchSuites::runFidelity()
{
    struct Case
    {
        QString name;
        int points;
        double spikeProbability;
    };

    const Case cases[] = {
        { QStringLiteral("clean_1k"), 1000, 0.0 },
        { QStringLiteral("spiky_100k"), 100000, 0.002 },
        { QStringLiteral("spiky_1m"), 1000000, 0.0002 },
    };

    for (const Case& item : cases) {
        DataGenerator generator(seedFor(QStringLiteral("fidelity%1").arg(item.points)));
        std::vector<std::vector<double>> x(1);
        std::vector<std::vector<double>> y(1);
        generator.generatePeakySignal(item.points, item.spikeProbability, x[0], y[0]);

        // Reference rendering: no point reduction at all.
        PlotSpec spec;
        spec.curveCount = 1;
        spec.pointsPerCurve = item.points;
        spec.widgetSize = QSize(m_options.canvasWidth, m_options.canvasHeight);
        exportImage(QStringLiteral("fidelity"), item.name + QStringLiteral("_full"), spec,
                    QStringLiteral("none"), x, y);

        // Same data with every point reduction strategy the library offers.
        const QStringList algorithms = m_adapter->downsamplingAlgorithms();
        for (const QString& algorithm : algorithms) {
            if (algorithm == QLatin1String("none"))
                continue;
            exportImage(QStringLiteral("fidelity"),
                        QStringLiteral("%1_%2").arg(item.name, algorithm), spec, algorithm, x, y);
        }
    }
}

// ---------------------------------------------------------------------------
// Image export
// ---------------------------------------------------------------------------

bool BenchSuites::exportImage(const QString& suite, const QString& variant, const PlotSpec& spec,
                              const QString& sampling,
                              const std::vector<std::vector<double>>& x,
                              const std::vector<std::vector<double>>& y)
{
    QString error;
    if (!prepareAdapter(spec, error)) {
        ++m_failures;
        qWarning("chartbench: %s/%s skipped: %s", qPrintable(suite), qPrintable(variant),
                 qPrintable(error));
        releaseAdapter();
        return false;
    }

    m_adapter->setDownsamplingAlgorithm(sampling);
    m_adapter->rebuild(x, y, spec);
    syncAdapterToSpec(spec);
    for (int i = 0; i < 3; ++i)
        QApplication::processEvents();

    for (int i = 0; i < 3; ++i)
        m_adapter->frame();

    const QImage image = m_adapter->exportWidget();

    QDir dir(m_options.outputDir);
    dir.mkpath(QStringLiteral("images"));
    const QString path = dir.filePath(QStringLiteral("images/%1_%2_%3.png")
                                          .arg(m_adapter->libraryId(), ResultWriter::backendTag(m_options.backend), variant));
    if (!image.save(path))
        qWarning("chartbench: cannot save %s", qPrintable(path));
    else if (m_options.verbose)
        qInfo("  image %s (%dx%d)", qPrintable(path), image.width(), image.height());

    releaseAdapter();
    return true;
}

} // namespace chartbench
