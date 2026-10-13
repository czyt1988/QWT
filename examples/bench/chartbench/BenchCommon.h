/*****************************************************************************
 * chartbench - rendering performance comparison of Qt plotting libraries
 *
 * Headless-friendly infrastructure shared by all benchmark suites: command
 * line configuration, deterministic data generation, high resolution timing,
 * process memory accounting and CSV output.
 *****************************************************************************/

#ifndef CHARTBENCH_COMMON_H
#define CHARTBENCH_COMMON_H

#include <QString>
#include <QStringList>
#include <QSize>
#include <QVector>

#include <chrono>
#include <string>
#include <vector>

namespace chartbench
{

//! Rendering backend a plot is drawn with.
enum class Backend
{
    Raster,
    OpenGL
};

//! Data refresh strategy used while a configuration is being measured.
enum class UpdateMode
{
    None,       //!< Data stays untouched, only the repaint is measured
    Window,     //!< A sliding window of the backing array is pushed to the plot
    Append      //!< One sample per curve is appended (streaming workload)
};

//! How the plot axes are handled between frames.
enum class AxisMode
{
    Fixed,      //!< Ranges are set once before the measurement
    Rescale     //!< The library rescales to the data on every frame
};

const char* toString(Backend backend);
const char* toString(UpdateMode mode);
const char* toString(AxisMode mode);

// ---------------------------------------------------------------------------
// Measurement row
// ---------------------------------------------------------------------------

//! One measured frame.
struct FrameSample
{
    QString suite;
    QString library;
    QString variant;
    Backend backend = Backend::Raster;
    int curves = 1;
    int pointsPerCurve = 0;
    int canvasWidth = 0;
    int canvasHeight = 0;
    int frameIndex = 0;
    double updateMs = 0.0;   //!< Time spent handing new data to the library
    double renderMs = 0.0;   //!< Time spent producing a complete frame
    double totalMs = 0.0;
    double privateMb = 0.0;  //!< Private bytes of the process after the frame
    double workingSetMb = 0.0;
};

//! Aggregate facts of one measured configuration.
struct ConfigSample
{
    QString suite;
    QString library;
    QString variant;
    Backend backend = Backend::Raster;
    int curves = 1;
    int pointsPerCurve = 0;
    int canvasWidth = 0;
    int canvasHeight = 0;
    int frames = 0;
    bool aborted = false;
    double setupMs = 0.0;        //!< create + handover + first frame
    double createMs = 0.0;       //!< Widget/plot construction only
    double handoverMs = 0.0;     //!< First data handover only
    double rebuildMs = 0.0;      //!< Mean data handover of the measured frames
    double baselinePrivateMb = 0.0;
    double privateMb = 0.0;
    double peakPrivateMb = 0.0;
    double workingSetMb = 0.0;
    double peakWorkingSetMb = 0.0;
    double cpuGpuSyncMs = 0.0;   //!< Extra time the glFinish() wait added
    QString note;
};

// ---------------------------------------------------------------------------
// CSV output
// ---------------------------------------------------------------------------

//! Append-only CSV writers for the two result tables.
class ResultWriter
{
public:
    bool open(const QString& directory, const QString& library, Backend backend);
    void writeFrame(const FrameSample& sample);
    void writeConfig(const ConfigSample& sample);
    void close();

    const QString& framePath() const { return m_framePath; }
    const QString& configPath() const { return m_configPath; }

    static QString backendTag(Backend backend);

private:
    QString m_framePath;
    QString m_configPath;
    QVector<FrameSample> m_frames;
    QVector<ConfigSample> m_configs;
    bool m_frameHeaderWritten = false;
    bool m_configHeaderWritten = false;
};

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------

//! Monotonic clock returning milliseconds as double.
class Stopwatch
{
public:
    Stopwatch();

    void restart();
    double elapsedMs() const;

    static double nowMs();

private:
    std::chrono::steady_clock::time_point m_start;
};

// ---------------------------------------------------------------------------
// Process memory
// ---------------------------------------------------------------------------

struct MemoryInfo
{
    double privateMb = 0.0;
    double peakPrivateMb = 0.0;
    double workingSetMb = 0.0;
    double peakWorkingSetMb = 0.0;
};

MemoryInfo processMemory();

// ---------------------------------------------------------------------------
// Deterministic data
// ---------------------------------------------------------------------------

/**
 * @brief Reproducible series generator.
 *
 * The same seed produces bit identical data on every run and in every library,
 * which is a precondition for comparing rendering cost and rendering fidelity.
 */
class DataGenerator
{
public:
    explicit DataGenerator(quint32 seed = 20260101);

    //! @param[out] x,y one vector per curve
    void generate(int curveCount, int pointsPerCurve,
                  std::vector<std::vector<double>>& x,
                  std::vector<std::vector<double>>& y);

    //! Spiky reference signal: smooth base plus narrow Gaussian peaks.
    void generatePeakySignal(int pointsPerCurve, double spikeProbability,
                             std::vector<double>& x, std::vector<double>& y);

    //! Backing array of the streaming workloads: length samples of a random walk.
    void generateStream(int length, std::vector<double>& x, std::vector<double>& y);

    //! Uniform noise in [0,1) from a small reproducible LCG.
    double random();

private:
    quint32 m_state;
};

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

struct BenchOptions
{
    QString library;              //!< qwt | qcustomplot | qtcharts | qim
    Backend backend = Backend::Raster;
    QStringList suites;
    QString outputDir = QStringLiteral(".");
    int warmupFrames = 5;
    int maxFrames = 60;
    double frameBudgetMs = 4000.0;  //!< Per configuration time budget
    int repeats = 1;
    int canvasWidth = 1000;
    int canvasHeight = 700;
    bool offscreenWindow = false;
    int windowX = 60;
    int windowY = 60;
    bool verbose = false;
    bool saveImages = false;

    static bool parse(int argc, char** argv, BenchOptions& options, QString& error);
    static QString usage();
};

// ---------------------------------------------------------------------------
// Formatting helpers
// ---------------------------------------------------------------------------

QString formatNumber(double value, int decimals = 3);

} // namespace chartbench

#endif // CHARTBENCH_COMMON_H
