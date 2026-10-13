/*****************************************************************************
 * chartbench - QCustomPlot adapter
 *****************************************************************************/

#ifndef CHARTBENCH_QCUSTOMPLOT_ADAPTER_H
#define CHARTBENCH_QCUSTOMPLOT_ADAPTER_H

#include "PlotAdapter.h"

#include <vector>

class QCustomPlot;
class QCPGraph;

namespace chartbench
{

//! Downsampling strategies of QCustomPlot.
enum class QcpSampling
{
    None,       //!< setAdaptiveSampling(false)
    Adaptive    //!< QCustomPlot's pixel based adaptive sampling
};

class QCustomPlotAdapter : public PlotAdapter
{
public:
    explicit QCustomPlotAdapter(Backend backend);
    ~QCustomPlotAdapter() override;

    QString libraryId() const override { return QStringLiteral("qcustomplot"); }
    QString displayName() const override { return QStringLiteral("QCustomPlot 2.1.0"); }
    QString backendDescription() const override;
    bool supportsDownsampling() const override { return true; }
    QString downsamplingDescription() const override;
    QStringList downsamplingAlgorithms() const override;
    bool setDownsamplingAlgorithm(const QString& name) override;

    bool create(QWidget* parent, const PlotSpec& spec, QString& error) override;
    void destroy() override;
    QWidget* widget() const override;

    void rebuild(const std::vector<std::vector<double>>& x,
                 const std::vector<std::vector<double>>& y,
                 const PlotSpec& spec) override;
    void updateWindow(int index, const std::vector<double>& x,
                      const std::vector<double>& y, int offset, int count) override;

    void setAxesRange(double xMin, double xMax, double yMin, double yMax) override;
    void rescaleAxes() override;

    void frame() override;
    QImage exportWidget() override;
    bool isReady() const override { return m_plot != nullptr && m_frames > 0; }

private:
    void applySampling();

    Backend m_backend;
    QCustomPlot* m_plot = nullptr;
    std::vector<QCPGraph*> m_graphs;
    QcpSampling m_sampling = QcpSampling::None;
    int m_frames = 0;
};

} // namespace chartbench

#endif // CHARTBENCH_QCUSTOMPLOT_ADAPTER_H
