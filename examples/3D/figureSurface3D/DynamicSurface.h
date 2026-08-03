#ifndef DYNAMIC_SURFACE_H
#define DYNAMIC_SURFACE_H

#include <QObject>
#include <QTimer>
#include <QWidget>

#include "qwt3d_function.h"
#include "qwt3d_plot.h"
#include "qwt3d_surface.h"

// Ripple function: z = sin(2t + sqrt(x^2 + y^2))
class RippleFunction : public Qwt3DFunction
{
public:
    RippleFunction();

    double operator()(double x, double y) override;

    void setTime(double t) { m_time = t; }
    double time() const { return m_time; }

private:
    double m_time = 0.0;
};

// Dynamic surface widget using composition: Qwt3DPlot + Qwt3DSurface
class DynamicSurfacePlot : public QWidget
{
    Q_OBJECT

public:
    explicit DynamicSurfacePlot(QWidget* parent = nullptr);

    // Compute cross-section at y=0, filling xData/zData arrays
    void crossSectionY0(QVector<double>& xData, QVector<double>& zData) const;

    // Get z value at origin (0, 0)
    double zAtOrigin() const;

    // Current time parameter
    double time() const;

    // Domain half-extent
    static constexpr double DOMAIN_EXTENT = 3.0;
    // Grid resolution
    static constexpr int GRID_SIZE = 60;

    // Access the underlying Qwt3DPlot for view control
    Qwt3DPlot* plot() const { return m_plot; }
    // Access the underlying Qwt3DSurface for data/style control
    Qwt3DSurface* surface() const { return m_surface; }

public Q_SLOTS:
    // Advance time by dt and regenerate surface data
    void advanceTime(double dt);

    // Reset to standard view
    void resetView();

Q_SIGNALS:
    void timeChanged(double t);

private:
    void setupAxes();

    Qwt3DPlot* m_plot = nullptr;
    Qwt3DSurface* m_surface = nullptr;
    RippleFunction m_ripple;
};

#endif // DYNAMIC_SURFACE_H
