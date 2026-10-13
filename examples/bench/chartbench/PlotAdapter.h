/*****************************************************************************
 * chartbench - rendering performance comparison of Qt plotting libraries
 *
 * Abstract adapter that hides the four libraries behind one measurement
 * interface. Every adapter exposes the same operations in the same order so
 * the suites can drive all libraries with an identical protocol:
 *
 *   create()  -> rebuild() -> setAxesRange() -> warmup frames -> measure
 *
 * The only operation that is allowed to differ between libraries is frame():
 * it must return after a *complete* frame exists - for raster painters that is
 * when QPainter is done, for GPU painters that is after glFinish().
 *****************************************************************************/

#ifndef CHARTBENCH_PLOT_ADAPTER_H
#define CHARTBENCH_PLOT_ADAPTER_H

#include "BenchCommon.h"

#include <QImage>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <memory>
#include <vector>

namespace chartbench
{

//! Everything a library needs to know before the first frame is drawn.
struct PlotSpec
{
    int curveCount = 1;
    int pointsPerCurve = 1000;
    QSize widgetSize { 1000, 700 };
    bool antialiasing = true;
    AxisMode axisMode = AxisMode::Fixed;
    double xMin = 0.0;
    double xMax = 2.0 * M_PI * 10.0;
    double yMin = -1.8;
    double yMax = 1.8;
    QString label = QStringLiteral("series");
};

class PlotAdapter
{
public:
    virtual ~PlotAdapter() = default;

    //! Identification ------------------------------------------------------
    virtual QString libraryId() const = 0;
    virtual QString displayName() const = 0;
    //! Free form description of what actually renders (QPainter, GL canvas, ...)
    virtual QString backendDescription() const = 0;
    virtual bool supportsDownsampling() const = 0;
    virtual QString downsamplingDescription() const = 0;
    //! Point reduction strategies this adapter can select, "none" always first.
    virtual QStringList downsamplingAlgorithms() const;
    //! Selects one of downsamplingAlgorithms(); returns false when unknown.
    virtual bool setDownsamplingAlgorithm(const QString& name);

    //! Lifecycle -----------------------------------------------------------
    //! Creates the plot widget below @a parent and applies @a spec.
    virtual bool create(QWidget* parent, const PlotSpec& spec, QString& error) = 0;
    virtual void destroy() = 0;
    virtual QWidget* widget() const = 0;

    //! Data ----------------------------------------------------------------
    //! Replaces all series in one call (the "open a file and show it" path).
    virtual void rebuild(const std::vector<std::vector<double>>& x,
                         const std::vector<std::vector<double>>& y,
                         const PlotSpec& spec) = 0;
    //! Pushes a window of @a count samples starting at @a offset of series @a index.
    virtual void updateWindow(int index, const std::vector<double>& x,
                              const std::vector<double>& y, int offset, int count) = 0;

    //! Axes ------------------------------------------------------------------
    virtual void setAxesRange(double xMin, double xMax, double yMin, double yMax) = 0;
    virtual void rescaleAxes() = 0;

    //! Rendering -------------------------------------------------------------
    //! Produces exactly one complete frame and returns when it is finished.
    virtual void frame() = 0;
    //! Renders one frame and returns it (used by the fidelity suite).
    virtual QImage exportWidget() = 0;

    //! Extra time the GPU synchronisation inside frame() needed. Adapters that
    //! cannot be synchronised report 0.
    virtual double lastGpuSyncMs() const { return 0.0; }

    //! True once the widget has been exposed and at least one frame finished.
    virtual bool isReady() const = 0;
};

//! Creates the adapter for @a library, or nullptr with @a error set.
std::unique_ptr<PlotAdapter> createAdapter(const QString& library, Backend backend, QString& error);

//! Libraries that are compiled into this binary.
QStringList availableLibraries();

} // namespace chartbench

#endif // CHARTBENCH_PLOT_ADAPTER_H
