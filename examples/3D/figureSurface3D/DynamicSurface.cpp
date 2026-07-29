#include "DynamicSurface.h"

#include <QVBoxLayout>
#include <cmath>

#include "qwt3d_color.h"
#include "qwt3d_coordsys.h"
#include "qwt3d_types.h"

// ---- RippleFunction ----

RippleFunction::RippleFunction()
    : Qwt3DFunction()
{
    setMesh(DynamicSurfacePlot::GRID_SIZE, DynamicSurfacePlot::GRID_SIZE);
    setDomain(-DynamicSurfacePlot::DOMAIN_EXTENT, DynamicSurfacePlot::DOMAIN_EXTENT,
              -DynamicSurfacePlot::DOMAIN_EXTENT, DynamicSurfacePlot::DOMAIN_EXTENT);
}

double RippleFunction::operator()(double x, double y)
{
    return std::sin(2.0 * m_time + std::sqrt(x * x + y * y));
}

// ---- DynamicSurfacePlot ----

DynamicSurfacePlot::DynamicSurfacePlot(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_plot = new Qwt3DPlot(this);
    layout->addWidget(m_plot);

    m_surface = new Qwt3DSurface();
    m_surface->attach(m_plot);

    m_plot->setTitle("Dynamic Ripple Surface: z = sin(2t + sqrt(x^2 + y^2))");
    m_plot->setTitleFont("Arial", 12, QFont::Bold);
    m_plot->setTitleColor(RGBA(0.1, 0.1, 0.1, 1.0));

    // Configure coordinate system
    setupAxes();
    m_plot->coordinates()->setStyle(BOX);

    // Configure data color (StandardColor for z-value-driven coloring)
    auto* stdColor = new Qwt3DStandardColor();
    m_surface->setDataColor(stdColor);

    // Configure lighting
    m_plot->enableLighting(true);
    m_plot->illuminate(0);
    m_plot->setLightComponent(GL_DIFFUSE, 0.7, 0);
    m_plot->setLightComponent(GL_SPECULAR, 0.3, 0);
    m_plot->setShininess(10.0);
    m_plot->setLightRotation(20.0, 30.0, 0.0, 0);

    // Configure plot style
    m_surface->setPlotStyle(FILLED);
    m_surface->setShading(GOURAUD);
    m_surface->setFloorStyle(FLOORISO);
    m_surface->setPolygonOffset(0.8);
    m_surface->setMeshColor(RGBA(0.0, 0.0, 0.0, 0.15));
    m_surface->setMeshLineWidth(0.5);
    m_plot->setBackgroundColor(RGBA(1.0, 1.0, 1.0, 1.0));

    // Enable mouse and keyboard interaction
    m_plot->enableMouse(true);
    m_plot->enableKeyboard(true);

    // Create the initial surface data
    m_ripple.assign(*m_surface);
    m_ripple.create();

    // Set initial view
    resetView();

    // Show color legend
    m_plot->showColorLegend(true);
}

void DynamicSurfacePlot::setupAxes()
{
    for (unsigned i = 0; i < m_plot->coordinates()->axes.size(); ++i) {
        m_plot->coordinates()->axes[i].setMajors(5);
        m_plot->coordinates()->axes[i].setMinors(4);
    }

    m_plot->coordinates()->axes[X1].setLabelString("X");
    m_plot->coordinates()->axes[Y1].setLabelString("Y");
    m_plot->coordinates()->axes[Z1].setLabelString("Z");
}

void DynamicSurfacePlot::advanceTime(double dt)
{
    m_ripple.setTime(m_ripple.time() + dt);
    m_ripple.create();
    m_plot->update();
    Q_EMIT timeChanged(m_ripple.time());
}

void DynamicSurfacePlot::resetView()
{
    m_plot->setRotation(30, 0, 15);
    m_plot->setViewportShift(0.05, 0);
    m_plot->setScale(1, 1, 1);
    m_plot->setZoom(0.9);
}

void DynamicSurfacePlot::crossSectionY0(QVector<double>& xData, QVector<double>& zData) const
{
    const int n = GRID_SIZE;
    const double ext = DOMAIN_EXTENT;
    const double step = 2.0 * ext / (n - 1);
    const double t = m_ripple.time();

    xData.resize(n);
    zData.resize(n);

    for (int i = 0; i < n; ++i) {
        double x = -ext + i * step;
        xData[i] = x;
        zData[i] = std::sin(2.0 * t + std::sqrt(x * x)); // y=0
    }
}

double DynamicSurfacePlot::zAtOrigin() const
{
    return std::sin(2.0 * m_ripple.time());
}

double DynamicSurfacePlot::time() const
{
    return m_ripple.time();
}
