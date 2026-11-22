/*****************************************************************************
 * chartbench - Qt Charts adapter
 *****************************************************************************/

#ifndef CHARTBENCH_QTCHARTS_ADAPTER_H
#define CHARTBENCH_QTCHARTS_ADAPTER_H

#include "PlotAdapter.h"

#include <QtGlobal>

#include <vector>

// Qt 5 keeps the Charts classes in the QtCharts namespace while Qt 6 moved them
// to the global namespace, so the forward declarations have to follow suit.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtCharts/QChartGlobal>
QT_CHARTS_BEGIN_NAMESPACE
#endif
class QChart;
class QChartView;
class QValueAxis;
class QLineSeries;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QT_CHARTS_END_NAMESPACE
QT_CHARTS_USE_NAMESPACE
#endif

namespace chartbench
{

class QtChartsAdapter : public PlotAdapter
{
public:
    explicit QtChartsAdapter(Backend backend);
    ~QtChartsAdapter() override;

    QString libraryId() const override { return QStringLiteral("qtcharts"); }
    QString displayName() const override { return QStringLiteral("Qt Charts 6.7"); }
    QString backendDescription() const override;
    bool supportsDownsampling() const override { return false; }
    QString downsamplingDescription() const override { return QStringLiteral("none (no built-in)"); }

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
    bool isReady() const override { return m_view != nullptr && m_frames > 0; }

private:
    Backend m_backend;
    QChartView* m_view = nullptr;
    QChart* m_chart = nullptr;
    QValueAxis* m_axisX = nullptr;
    QValueAxis* m_axisY = nullptr;
    std::vector<QLineSeries*> m_series;
    QImage m_target;    //!< reusable render target
    bool m_useOpenGl = false;
    double m_gpuSyncMs = 0.0;
    int m_frames = 0;
};

} // namespace chartbench

#endif // CHARTBENCH_QTCHARTS_ADAPTER_H
