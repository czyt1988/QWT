/*****************************************************************************
 * chartbench - Qwt adapter
 *****************************************************************************/

#ifndef CHARTBENCH_QWT_ADAPTER_H
#define CHARTBENCH_QWT_ADAPTER_H

#include "PlotAdapter.h"

#include <qwt_plot.h>   // QwtPlot is needed for the QWidget upcast in widget()

#include <vector>

class QwtPlotCurve;
class QWidget;

// Qwt only builds its OpenGL canvas when QWT_CONFIG_QWTOPENGL is on; the header
// is the reliable indicator.
#if __has_include(<qwt_plot_opengl_canvas.h>)
#define CHARTBENCH_HAVE_QWT_OPENGL 1
namespace chartbench
{
class BenchGlCanvas;   // defined in QwtAdapter.cpp
}
#endif

namespace chartbench
{

//! Downsampling strategies of QwtPlotCurve that are exercised by the suites.
enum class QwtFilter
{
    None,
    FilterPoints,
    FilterPointsAggressive,
    FilterPointsPixel,
    FilterPointsLTTB,
    FilterPointsMinMaxLTTB
};

class QwtAdapter : public PlotAdapter
{
public:
    explicit QwtAdapter(Backend backend);
    ~QwtAdapter() override;

    QString libraryId() const override { return QStringLiteral("qwt"); }
    QString displayName() const override { return QStringLiteral("Qwt 7.5"); }
    QString backendDescription() const override;
    bool supportsDownsampling() const override { return true; }
    QString downsamplingDescription() const override;
    QStringList downsamplingAlgorithms() const override;
    bool setDownsamplingAlgorithm(const QString& name) override;

    bool create(QWidget* parent, const PlotSpec& spec, QString& error) override;
    void destroy() override;
    QWidget* widget() const override { return m_plot; }

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
    bool isReady() const override { return m_plot != nullptr && m_frames > 0; }

private:
    void setImmediatePaint();
    void applyFilterAttribute();

    Backend m_backend;
    QwtPlot* m_plot = nullptr;
    QWidget* m_canvas = nullptr;
#ifdef CHARTBENCH_HAVE_QWT_OPENGL
    BenchGlCanvas* m_glCanvas = nullptr;
#endif
    std::vector<QwtPlotCurve*> m_curves;
    QwtFilter m_filter = QwtFilter::None;
    double m_gpuSyncMs = 0.0;
    int m_frames = 0;
};

} // namespace chartbench

#endif // CHARTBENCH_QWT_ADAPTER_H
