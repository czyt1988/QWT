#include "BenchCommon.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

namespace chartbench
{

const char* toString(Backend backend)
{
    return backend == Backend::OpenGL ? "opengl" : "raster";
}

const char* toString(UpdateMode mode)
{
    switch (mode) {
    case UpdateMode::Window:
        return "window";
    case UpdateMode::Append:
        return "append";
    case UpdateMode::None:
        break;
    }
    return "none";
}

const char* toString(AxisMode mode)
{
    return mode == AxisMode::Rescale ? "rescale" : "fixed";
}

namespace
{

//! RFC 4180 style quoting: the note column carries free text with commas.
QString csvField(const QString& value)
{
    if (!value.contains(QLatin1Char(',')) && !value.contains(QLatin1Char('"'))
        && !value.contains(QLatin1Char('\n'))) {
        return value;
    }
    QString quoted = value;
    quoted.replace(QLatin1Char('"'), QLatin1String("\"\""));
    return QLatin1Char('"') + quoted + QLatin1Char('"');
}

} // namespace

// ---------------------------------------------------------------------------
// ResultWriter
// ---------------------------------------------------------------------------

QString ResultWriter::backendTag(Backend backend)
{
    return QString::fromLatin1(toString(backend));
}

bool ResultWriter::open(const QString& directory, const QString& library, Backend backend)
{
    QDir dir(directory);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        qWarning("chartbench: cannot create output directory %s", qPrintable(directory));
        return false;
    }

    const QString stem = QStringLiteral("%1_%2").arg(library, backendTag(backend));
    m_framePath = dir.filePath(QStringLiteral("frames_%1.csv").arg(stem));
    m_configPath = dir.filePath(QStringLiteral("configs_%1.csv").arg(stem));

    QFile frameFile(m_framePath);
    if (!frameFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning("chartbench: cannot write %s", qPrintable(m_framePath));
        return false;
    }
    QTextStream frameStream(&frameFile);
    frameStream << "suite,library,variant,backend,curves,points_per_curve,total_points,"
                   "canvas_w,canvas_h,frame_idx,update_ms,render_ms,total_ms,private_mb,ws_mb\n";
    frameFile.close();
    m_frameHeaderWritten = true;

    QFile configFile(m_configPath);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning("chartbench: cannot write %s", qPrintable(m_configPath));
        return false;
    }
    QTextStream configStream(&configFile);
    configStream << "suite,library,variant,backend,curves,points_per_curve,total_points,"
                    "canvas_w,canvas_h,frames,aborted,setup_ms,create_ms,handover_ms,rebuild_ms,"
                    "baseline_private_mb,private_mb,peak_private_mb,ws_mb,peak_ws_mb,gpu_sync_ms,"
                    "note\n";
    configFile.close();
    m_configHeaderWritten = true;

    return true;
}

void ResultWriter::writeFrame(const FrameSample& sample)
{
    if (m_framePath.isEmpty())
        return;

    QFile file(m_framePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << sample.suite << ',' << sample.library << ',' << sample.variant << ','
        << toString(sample.backend) << ',' << sample.curves << ',' << sample.pointsPerCurve << ','
        << (sample.curves * sample.pointsPerCurve) << ',' << sample.canvasWidth << ','
        << sample.canvasHeight << ',' << sample.frameIndex << ','
        << formatNumber(sample.updateMs, 4) << ',' << formatNumber(sample.renderMs, 4) << ','
        << formatNumber(sample.totalMs, 4) << ',' << formatNumber(sample.privateMb, 2) << ','
        << formatNumber(sample.workingSetMb, 2) << '\n';
    file.close();
}

void ResultWriter::writeConfig(const ConfigSample& sample)
{
    if (m_configPath.isEmpty())
        return;

    QFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << sample.suite << ',' << sample.library << ',' << sample.variant << ','
        << toString(sample.backend) << ',' << sample.curves << ',' << sample.pointsPerCurve << ','
        << (sample.curves * sample.pointsPerCurve) << ',' << sample.canvasWidth << ','
        << sample.canvasHeight << ',' << sample.frames << ',' << (sample.aborted ? 1 : 0) << ','
        << formatNumber(sample.setupMs, 4) << ',' << formatNumber(sample.createMs, 4) << ','
        << formatNumber(sample.handoverMs, 4) << ',' << formatNumber(sample.rebuildMs, 4) << ','
        << formatNumber(sample.baselinePrivateMb, 2) << ',' << formatNumber(sample.privateMb, 2)
        << ',' << formatNumber(sample.peakPrivateMb, 2) << ',' << formatNumber(sample.workingSetMb, 2)
        << ',' << formatNumber(sample.peakWorkingSetMb, 2) << ','
        << formatNumber(sample.cpuGpuSyncMs, 4) << ',' << csvField(sample.note) << '\n';
    file.close();
}

void ResultWriter::close()
{
    m_frames.clear();
    m_configs.clear();
}

// ---------------------------------------------------------------------------
// Stopwatch
// ---------------------------------------------------------------------------

Stopwatch::Stopwatch()
    : m_start(std::chrono::steady_clock::now())
{
}

void Stopwatch::restart()
{
    m_start = std::chrono::steady_clock::now();
}

double Stopwatch::elapsedMs() const
{
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(now - m_start).count();
}

double Stopwatch::nowMs()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double, std::milli>(now).count();
}

// ---------------------------------------------------------------------------
// Process memory
// ---------------------------------------------------------------------------

MemoryInfo processMemory()
{
    MemoryInfo info;
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX counters;
    std::memset(&counters, 0, sizeof(counters));
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                             sizeof(counters))) {
        info.privateMb = counters.PrivateUsage / (1024.0 * 1024.0);
        info.peakPrivateMb = counters.PeakPagefileUsage / (1024.0 * 1024.0);
        info.workingSetMb = counters.WorkingSetSize / (1024.0 * 1024.0);
        info.peakWorkingSetMb = counters.PeakWorkingSetSize / (1024.0 * 1024.0);
    }
#endif
    return info;
}

// ---------------------------------------------------------------------------
// DataGenerator
// ---------------------------------------------------------------------------

DataGenerator::DataGenerator(quint32 seed)
    : m_state(seed ? seed : 1u)
{
}

double DataGenerator::random()
{
    // Numerical Recipes LCG: never returns exactly 1.0, identical on all platforms.
    m_state = m_state * 1664525u + 1013904223u;
    return m_state / 4294967296.0;
}

void DataGenerator::generate(int curveCount, int pointsPerCurve,
                             std::vector<std::vector<double>>& x,
                             std::vector<std::vector<double>>& y)
{
    x.assign(curveCount, std::vector<double>());
    y.assign(curveCount, std::vector<double>());

    const double step = 20.0 * M_PI / qMax(1, pointsPerCurve - 1);

    for (int c = 0; c < curveCount; ++c) {
        x[c].resize(pointsPerCurve);
        y[c].resize(pointsPerCurve);
        const double phase = 0.37 * c;
        const double amplitude = 1.0 - 0.25 * (c % 3);
        for (int i = 0; i < pointsPerCurve; ++i) {
            const double xi = i * step;
            x[c][i] = xi;
            y[c][i] = amplitude * std::sin(xi + phase) + 0.25 * std::sin(3.7 * xi + phase)
                + 0.12 * (random() - 0.5);
        }
    }
}

void DataGenerator::generatePeakySignal(int pointsPerCurve, double spikeProbability,
                                        std::vector<double>& x, std::vector<double>& y)
{
    x.resize(pointsPerCurve);
    y.resize(pointsPerCurve);
    const double step = 20.0 * M_PI / qMax(1, pointsPerCurve - 1);
    for (int i = 0; i < pointsPerCurve; ++i) {
        const double xi = i * step;
        x[i] = xi;
        double value = std::sin(xi) + 0.25 * std::sin(3.7 * xi);
        if (random() < spikeProbability) {
            // A narrow peak that a naive min/max bucket may collapse into a stroke.
            value += 0.6 + 0.4 * random();
        } else {
            value += 0.02 * (random() - 0.5);
        }
        y[i] = value;
    }
}

void DataGenerator::generateStream(int length, std::vector<double>& x, std::vector<double>& y)
{
    x.resize(length);
    y.resize(length);
    double value = 0.0;
    for (int i = 0; i < length; ++i) {
        value += 0.02 * (random() - 0.5);
        value = qBound(-1.0, value, 1.0);
        x[i] = i;
        y[i] = value;
    }
}

// ---------------------------------------------------------------------------
// Options
// ---------------------------------------------------------------------------

QString BenchOptions::usage()
{
    return QStringLiteral(
        "chartbench - rendering performance comparison for Qt plotting libraries\n"
        "\n"
        "  --library=<name>      qwt | qcustomplot | qtcharts | qim (default: qwt)\n"
        "  --backend=<name>      raster | opengl (default: raster)\n"
        "  --suite=<name>        selftest | points | curves | canvas | update |\n"
        "                        downsample | jitter | memory | fidelity | all\n"
        "  --out=<dir>           directory for the CSV result tables\n"
        "  --canvas=<WxH>        canvas size in device independent pixels\n"
        "  --warmup=<n>          discarded frames per configuration (default 5)\n"
        "  --frames=<n>          measured frames per configuration (default 60)\n"
        "  --budget=<ms>         time budget per configuration (default 4000)\n"
        "  --offscreen           move the benchmark window off the visible desktop\n"
        "  --verbose             print a line per measured configuration\n"
        "  --images              write reference images (fidelity suite)\n");
}

bool BenchOptions::parse(int argc, char** argv, BenchOptions& options, QString& error)
{
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == QLatin1String("--help") || arg == QLatin1String("-h")) {
            error = usage();
            return false;
        }
        if (arg == QLatin1String("--verbose")) {
            options.verbose = true;
            continue;
        }
        if (arg == QLatin1String("--offscreen")) {
            options.offscreenWindow = true;
            continue;
        }
        if (arg == QLatin1String("--images")) {
            options.saveImages = true;
            continue;
        }

        const int eq = arg.indexOf(QLatin1Char('='));
        if (eq < 0) {
            error = QStringLiteral("unknown argument: %1\n\n%2").arg(arg, usage());
            return false;
        }
        const QString key = arg.left(eq);
        const QString value = arg.mid(eq + 1);
        bool ok = true;

        if (key == QLatin1String("--library"))
            options.library = value;
        else if (key == QLatin1String("--suite"))
            options.suites = value.split(QLatin1Char(','), Qt::SkipEmptyParts);
        else if (key == QLatin1String("--out"))
            options.outputDir = value;
        else if (key == QLatin1String("--warmup"))
            options.warmupFrames = value.toInt(&ok);
        else if (key == QLatin1String("--frames"))
            options.maxFrames = value.toInt(&ok);
        else if (key == QLatin1String("--budget"))
            options.frameBudgetMs = value.toDouble(&ok);
        else if (key == QLatin1String("--repeats"))
            options.repeats = value.toInt(&ok);
        else if (key == QLatin1String("--backend")) {
            if (value == QLatin1String("raster"))
                options.backend = Backend::Raster;
            else if (value == QLatin1String("opengl"))
                options.backend = Backend::OpenGL;
            else {
                error = QStringLiteral("unknown backend: %1").arg(value);
                return false;
            }
        } else if (key == QLatin1String("--canvas")) {
            const QStringList wh = value.split(QLatin1Char('x'));
            if (wh.size() != 2) {
                error = QStringLiteral("--canvas expects WxH, got %1").arg(value);
                return false;
            }
            options.canvasWidth = wh[0].toInt(&ok);
            options.canvasHeight = wh[1].toInt(&ok);
        } else if (key == QLatin1String("--window-pos")) {
            const QStringList xy = value.split(QLatin1Char(','));
            if (xy.size() == 2) {
                options.windowX = xy[0].toInt();
                options.windowY = xy[1].toInt();
            }
        } else {
            error = QStringLiteral("unknown argument: %1\n\n%2").arg(key, usage());
            return false;
        }

        if (!ok) {
            error = QStringLiteral("cannot parse value of %1: %2").arg(key, value);
            return false;
        }
    }

    if (options.library.isEmpty()) {
        error = QStringLiteral("--library is required\n\n%1").arg(usage());
        return false;
    }
    if (options.suites.isEmpty())
        options.suites << QStringLiteral("all");

    if (options.offscreenWindow) {
        options.windowX = -32000;
        options.windowY = -32000;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Formatting
// ---------------------------------------------------------------------------

QString formatNumber(double value, int decimals)
{
    if (!std::isfinite(value))
        return QStringLiteral("nan");
    return QString::number(value, 'f', decimals);
}

} // namespace chartbench
