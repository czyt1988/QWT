#include <qlayout.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qradiobutton.h>

#include "enrichmentmainwindow.h"
#include "enrichments.h"
#include "qwt3d_function.h"

class Hat : public Qwt3DFunction
{
public:
    Hat() : Qwt3DFunction() { }

    double operator()(double x, double y) override
    {
        double ret = 1.0 / (x * x + y * y + 0.5);
        return ret;
    }
};

Hat *hat;

EnrichmentMainWindow::EnrichmentMainWindow(QWidget *parent) : DummyBase(parent)
{
    setupUi(this);
    show();
    QGridLayout *grid = new QGridLayout(frame);

    plot = new Qwt3DPlot(frame);
    surface = new Qwt3DSurface();
    surface->attach(plot);
    plot->show();
    grid->addWidget(plot, 0, 0);

    plot->setTitle("Bar Style (Vertex Enrichment)");
    plot->setTitleFont("Arial", 12);

    plot->setZoom(0.8);
    plot->setRotation(30, 0, 15);

    plot->coordinates()->setStyle(BOX);
    surface->setPlotStyle(QWT3D_POINTS);
    bar = static_cast<Bar*>(surface->addEnrichment(Bar(0.007, 0.5)));

    hat = new Hat();

    hat->setMesh(23, 21);
    hat->setDomain(-1.8, 1.7, -1.6, 1.7);

    hat->assign(*surface);
    hat->create();

    for (unsigned i = 0; i != plot->coordinates()->axes.size(); ++i) {
        plot->coordinates()->axes[i].setMajors(5);
        plot->coordinates()->axes[i].setMinors(4);
    }

    plot->coordinates()->setGridLinesColor(RGBA(0, 0, 0.5));
    plot->coordinates()->setLineWidth(1);
    plot->coordinates()->setNumberFont("Courier", 8);
    plot->coordinates()->adjustNumbers(5);

    setColor();
    plot->update();

    levelSlider->setValue(50);
    level_ = 0.5;
    width_ = 0.007;
    connect(levelSlider, &QSlider::valueChanged, this, &EnrichmentMainWindow::setLevel);
}

EnrichmentMainWindow::~EnrichmentMainWindow()
{
    delete hat;
}

void EnrichmentMainWindow::setColor()
{
    ColorVector cv;

    RGBA rgb;
    int i = 252;
    int step = 4;

    while (i >= 0) {
        rgb.r = i / 255.;
        rgb.g = (i - 60 > 0) ? (i - 60) / 255. : 0;
        rgb.b = 0;
        cv.push_back(rgb);
        if (!--step) {
            i -= 4;
            step = 4;
        }
    }
    auto *col = new Qwt3DStandardColor(plot);
    col->setColorVector(cv);

    surface->setDataColor(col);
}

void EnrichmentMainWindow::setLevel(int i)
{
    level_ = 1 - i / 100.;
    bar->configure(width_, level_);
    plot->update();
}

void EnrichmentMainWindow::setWidth(int i)
{
    width_ = i / 20000.;
    bar->configure(width_, level_);
    plot->update();
}

void EnrichmentMainWindow::barSlot()
{
    Bar b(width_, level_);
    bar = static_cast<Bar*>(surface->addEnrichment(b));
    plot->update();
}
