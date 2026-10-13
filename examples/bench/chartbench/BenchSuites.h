/*****************************************************************************
 * chartbench - measurement suites
 *****************************************************************************/

#ifndef CHARTBENCH_SUITES_H
#define CHARTBENCH_SUITES_H

#include "BenchCommon.h"
#include "PlotAdapter.h"

#include <QString>
#include <QStringList>

#include <memory>
#include <vector>

class QVBoxLayout;
class QWidget;

namespace chartbench
{

//! Request for one measured configuration. The data stays owned by the suite.
struct MeasureRequest
{
    QString suite;
    QString variant;
    PlotSpec spec;
    const std::vector<std::vector<double>>* x = nullptr;
    const std::vector<std::vector<double>>* y = nullptr;
    QString sampling = QStringLiteral("none");  //!< point reduction strategy
    UpdateMode updateMode = UpdateMode::None;
    //! Data the sliding window is taken from; falls back to x/y when null.
    const std::vector<std::vector<double>>* updateX = nullptr;
    const std::vector<std::vector<double>>* updateY = nullptr;
    int updateWindow = 0;   //!< samples pushed per update
    int updateStride = 1;   //!< samples the window advances per frame
    //! Follow the window with the x axis, the way a streaming plot does.
    bool trackAxes = false;
    int maxFrames = 0;      //!< 0 = use the global option
    double budgetMs = 0.0;  //!< 0 = use the global option
    bool measureUpdateOnly = false;
    QString note;
};

//! One library measured with one protocol.
class BenchSuites
{
public:
    BenchSuites(const BenchOptions& options, ResultWriter& writer);
    ~BenchSuites();

    bool initialize(QString& error);
    bool run(const QStringList& suites);

    //! Renders @a spec and stores the frame as PNG below <out>/images.
    bool exportImage(const QString& suite, const QString& variant, const PlotSpec& spec,
                     const QString& sampling,
                     const std::vector<std::vector<double>>& x,
                     const std::vector<std::vector<double>>& y);

private:
    void warmUpProcess();
    bool isRequested(const QString& suite) const;
    void measure(const MeasureRequest& request);
    void runPoints();
    void runCurves();
    void runCanvas();
    void runUpdate();
    void runDownsample();
    void runJitter();
    void runMemory();
    void runFidelity();
    void runSelftest();

    bool prepareAdapter(const PlotSpec& spec, QString& error, double* createMs = nullptr);
    void releaseAdapter();
    void syncAdapterToSpec(const PlotSpec& spec);

    const BenchOptions& m_options;
    ResultWriter& m_writer;
    std::unique_ptr<PlotAdapter> m_adapter;
    DataGenerator m_generator;
    QWidget* m_window = nullptr;
    QVBoxLayout* m_layout = nullptr;
    QStringList m_requested;
    int m_failures = 0;
};

} // namespace chartbench

#endif // CHARTBENCH_SUITES_H
