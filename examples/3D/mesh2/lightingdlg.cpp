#include <qlayout.h>
#include <qframe.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qpushbutton.h>

#include "lightingdlg.h"

class Sphere : public Qwt3DParametricSurface
{
public:
    Sphere() : Qwt3DParametricSurface()
    {
        setMesh(41, 31);
        setDomain(0, 2 * Qwt3D_PI, 0, Qwt3D_PI);
        setPeriodic(false, false);
    }

    Triple operator()(double u, double v) override
    {
        double x, y, z;
        double r = 1;
        x = r * cos(u) * sin(v);
        y = r * sin(u) * sin(v);
        z = r * cos(v);
        return Triple(x, y, z);
    }
};

/////////////////////////////////////////////////////////////////
//
//   Plot
//
/////////////////////////////////////////////////////////////////

Plot::Plot(QWidget *parent) : Qwt3DPlot(parent)
{
    surface = new Qwt3DSurface();
    surface->attach(this);

    setTitle("A Simple SurfacePlot Demonstration");

    Sphere sphere;
    sphere.assign(*surface);
    sphere.create();

    reset();
    assignMouse(Qt::LeftButton, Qt::RightButton, Qt::LeftButton, Qt::NoButton, Qt::NoButton,
                Qt::NoButton, Qt::NoButton, Qt::NoButton, Qt::NoButton);

    stick = static_cast<Pointer*>(surface->addEnrichment(Pointer(0.05)));
    stick->setPos(0, 0, 1);
}

void Plot::reset()
{
    setRotation(0, 0, 0);
    setTitle("Use your mouse buttons and keyboard");
    setTitleFont("Arial", 8, QFont::Bold);
    setTitleColor(RGBA(0.9, 0.9, 0.9));
    surface->setSmoothMesh(true);
    setZoom(0.9);
    coordinates()->setStyle(NOCOORD);
    surface->setMeshColor(RGBA(0.6, 0.6, 0.6, 0.3));
    surface->setPlotStyle(FILLEDMESH);
    setBackgroundColor(RGBA(0, 0, 0));

    update();
}

/////////////////////////////////////////////////////////////////
//
//   Pointer
//
/////////////////////////////////////////////////////////////////

Pointer::Pointer(double rad)
{
    configure(rad);
}

Pointer::~Pointer() { }

void Pointer::configure(double rad)
{
    plot = nullptr;

    radius_ = rad;
}

// Stubs — full VBO+shader implementation deferred.
// The Pointer enrichment draws a line from the tip to the origin.
// When implemented: create a 2-vertex VBO, use plot->lineShader().
void Pointer::drawBegin()
{
}

LightingDlg::LightingDlg(QWidget *parent) : LightingBase(parent)
{
    setupUi(this);
    QGridLayout *grid = new QGridLayout(frame);

    dataPlot = nullptr;

    plot = new Plot(frame);
    plot->update();

    grid->addWidget(plot, 0, 0);

    connect(stdlight, &QPushButton::clicked, this, &LightingDlg::reset);
    connect(distSL, &QSlider::valueChanged, this, &LightingDlg::setDistance);
    connect(emissSL, &QSlider::valueChanged, this, &LightingDlg::setEmission);
    connect(ambdiffSL, &QSlider::valueChanged, this, &LightingDlg::setDiff);
    connect(specSL, &QSlider::valueChanged, this, &LightingDlg::setSpec);
    connect(shinSL, &QSlider::valueChanged, this, &LightingDlg::setShin);
    connect(plot, &Qwt3DPlot::rotationChanged, this, &LightingDlg::setRotation);
}

LightingDlg::~LightingDlg()
{
    delete plot;
}

void LightingDlg::setEmission(int val)
{
    if (!dataPlot)
        return;
    dataPlot->setMaterialComponent(GL_EMISSION, val / 100.);
    dataPlot->update();
}
void LightingDlg::setDiff(int val)
{
    if (!dataPlot)
        return;
    dataPlot->setLightComponent(GL_DIFFUSE, val / 100.);
    dataPlot->update();
}
void LightingDlg::setSpec(int val)
{
    if (!dataPlot)
        return;
    dataPlot->setMaterialComponent(GL_SPECULAR, val / 100.);
    dataPlot->update();
}
void LightingDlg::setShin(int val)
{
    if (!dataPlot)
        return;
    dataPlot->setShininess(val / 100.);
    dataPlot->update();
}

void LightingDlg::reset()
{
    plot->reset();
    if (dataPlot)
        dataPlot->update();
}

void LightingDlg::setDistance(int val)
{

    plot->stick->setPos(0, 0, val / 100.);
    plot->update();

    double drad = (dataPlot->hull().maxVertex - dataPlot->hull().minVertex).length();
    drad *= val / 20.;

    dataPlot->setLightShift(drad, drad, drad);
    dataPlot->update();
}

void LightingDlg::assign(Qwt3DPlot *pl)
{
    if (!pl)
        return;
    dataPlot = pl;
}

void LightingDlg::setRotation(double x, double y, double z)
{
    if (!dataPlot)
        return;

    setDistance(distSL->value());
    dataPlot->setLightRotation(x, y, z);
    dataPlot->update();
}
