/*****************************************************************************
 * chartbench - QIm (ImGui / ImPlot wrapper) adapter
 *****************************************************************************/

#ifndef CHARTBENCH_QIM_ADAPTER_H
#define CHARTBENCH_QIM_ADAPTER_H

#include "PlotAdapter.h"

#include <vector>

namespace QIM
{
class QImFigureWidget;
class QImPlotNode;
class QImPlotLineItemNode;
}

namespace chartbench
{

//! QImFigureWidget with the protected render entry points exposed for timing.
//! Defined in QImAdapter.cpp; QOpenGLWidget defers plain repaint() calls to the
//! composition step, which would make every measured frame look instantaneous.
class BenchQImFigure;

//! Downsampling strategies offered by QImPlotLineItemNode.
enum class QImSampling
{
    None,
    Auto,
    LTTB,
    MinMaxLTTB
};

class QImAdapter : public PlotAdapter
{
public:
    explicit QImAdapter(Backend backend);
    ~QImAdapter() override;

    QString libraryId() const override { return QStringLiteral("qim"); }
    QString displayName() const override { return QStringLiteral("QIm 0.0.3 (ImPlot)"); }
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
    double lastGpuSyncMs() const override { return m_gpuSyncMs; }
    bool isReady() const override { return m_figure != nullptr && m_frames > 0; }

private:
    void applyPlotStyle();
    void applySampling();

    Backend m_backend;
    BenchQImFigure* m_benchFigure = nullptr;
    QIM::QImFigureWidget* m_figure = nullptr;
    QIM::QImPlotNode* m_plot = nullptr;
    std::vector<QIM::QImPlotLineItemNode*> m_lines;
    QImSampling m_sampling = QImSampling::None;
    bool m_styleApplied = false;
    double m_gpuSyncMs = 0.0;
    int m_frames = 0;
};

} // namespace chartbench

#endif // CHARTBENCH_QIM_ADAPTER_H
