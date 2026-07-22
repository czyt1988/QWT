#include <qsplitter.h>
#include <qlayout.h>
#include <qcombobox.h>
#include <qaction.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qmenubar.h>
#include <qdatetime.h>

#include "axes.h"
#include "axesmainwindow.h"
#include "qwt3d_function.h"

using namespace std;

// Example function
class Rosenbrock : public Qwt3DFunction
{
public:
    Rosenbrock() : Qwt3DFunction() { }

    double operator()(double x, double y) override
    {
        return log10((1 - x) * (1 - x) + 1 * (y - x * x) * (y - x * x));
    }
};

// Main widget

AxesMainWindow::AxesMainWindow(QWidget *parent) : DummyBase(parent)
{
    setupUi(this);
    QGridLayout *grid = new QGridLayout(frame);

    plot = new Qwt3DPlot(frame);
    surface = new Qwt3DSurface();
    surface->attach(plot);
    grid->addWidget(plot, 0, 0);

    plot->setZoom(0.8);
    plot->setRotation(30, 0, 15);

    plot->coordinates()->setGridLines(true, true);
    plot->coordinates()->setStyle(BOX);

    rosenbrock = new Rosenbrock();

    rosenbrock->setMesh(31, 33);
    rosenbrock->setDomain(-1.73, 1.8, -1.9, 1.8);
    rosenbrock->setMinZ(-100);

    rosenbrock->assign(*surface);
    rosenbrock->create();

    for (unsigned i = 0; i != plot->coordinates()->axes.size(); ++i) {
        plot->coordinates()->axes[i].setMajors(5);
        plot->coordinates()->axes[i].setMinors(4);
        plot->coordinates()->axes[i].setLabelColor(RGBA(0, 0, 0.4));
    }

    surface->setMeshLineWidth(1);
    plot->coordinates()->setGridLinesColor(RGBA(0, 0, 0.5));
    plot->coordinates()->setLineWidth(1);
    plot->coordinates()->setNumberColor(RGBA(0, 0.5, 0));
    plot->coordinates()->setNumberFont("Courier", 10);
    plot->setTitleFont("Courier", 11);
    plot->coordinates()->setLabelFont("Courier", 12, QFont::Bold);
    plot->coordinates()->axes[X1].setLabelString("X1");
    plot->coordinates()->axes[Y1].setLabelString("Y1");
    plot->coordinates()->axes[Z1].setLabelString("Z1");
    plot->coordinates()->axes[X2].setLabelString("X2");
    plot->coordinates()->axes[Y2].setLabelString("Y2");
    plot->coordinates()->axes[Z2].setLabelString("Z2");
    plot->coordinates()->axes[X3].setLabelString("X3");
    plot->coordinates()->axes[Y3].setLabelString("Y3");
    plot->coordinates()->axes[Z3].setLabelString("Z3");
    plot->coordinates()->axes[X4].setLabelString("X4");
    plot->coordinates()->axes[Y4].setLabelString("Y4");
    plot->coordinates()->axes[Z4].setLabelString("Z4");

    plot->coordinates()->setLineSmooth(true);
    smoothBox->setDown(true);

    QMenu *Items = menuBar()->addMenu("Item");
    Items->addAction("&Standard", this, &AxesMainWindow::standardItems, QKeySequence("ALT+S"));
    Items->addAction("&Imaginary", this, &AxesMainWindow::complexItems, QKeySequence("ALT+I"));
    Items->addAction("&Letter", this, &AxesMainWindow::letterItems, QKeySequence("ALT+L"));
    Items->addAction("&Time", this, &AxesMainWindow::timeItems, QKeySequence("ALT+T"));
    Items->addAction("&Log", this, &AxesMainWindow::customScale, QKeySequence("ALT+C"));

    plot->update();

    connect(smoothBox, &QCheckBox::toggled, this, &AxesMainWindow::setSmoothLines);
    connect(numbergapslider, &QSlider::valueChanged, this, &AxesMainWindow::setNumberGap);
    connect(labelgapslider, &QSlider::valueChanged, this, &AxesMainWindow::setLabelGap);
    connect(ticLengthSlider, &QSlider::valueChanged, this, &AxesMainWindow::setTicLength);
    connect(ticNumberSlider, &QSlider::valueChanged, this, &AxesMainWindow::setTicNumber);

    tics = plot->coordinates()->axes[X1].majors();

    // resetTics();

    customScale();

    surface->setPolygonOffset(10);
}

AxesMainWindow::~AxesMainWindow()
{
    delete rosenbrock;
}

void AxesMainWindow::setNumberGap(int gap)
{
    plot->coordinates()->adjustNumbers(gap);
    plot->update();
}

void AxesMainWindow::setLabelGap(int gap)
{
    plot->coordinates()->adjustLabels(gap);
    plot->update();
}

void AxesMainWindow::setSmoothLines(bool val)
{
    plot->coordinates()->setLineSmooth(val);
    plot->update();
}

void AxesMainWindow::setTicLength(int val)
{
    double majl = (plot->coordinates()->second() - plot->coordinates()->first()).length() / 1000.;
    majl = majl * val;
    plot->coordinates()->setTicLength(majl, 0.6 * majl);
    plot->update();
}

void AxesMainWindow::setTicNumber(int degree)
{
    plot->coordinates()->axes[X1].setMajors(tics + degree);
    plot->update();
}

void AxesMainWindow::resetTics()
{
    ticNumberSlider->setEnabled(false);
    plot->setTitle("");
    plot->coordinates()->axes[X1].setMajors(5);
    plot->coordinates()->setAutoScale(true);
    plot->coordinates()->setStandardScale();
    plot->coordinates()->axes[Z2].setLabelString("Z4");
    plot->coordinates()->setGridLines(false, false, BACK);
}

void AxesMainWindow::standardItems()
{
    resetTics();
    plot->update();
}

void AxesMainWindow::letterItems()
{
    resetTics();
    ticNumberSlider->setEnabled(true);
    plot->coordinates()->axes[X1].setAutoScale(false);
    plot->coordinates()->setStandardScale();
    plot->coordinates()->axes[X1].setScale(new Letter);
    plot->coordinates()->axes[X2].setScale(new Letter);
    plot->coordinates()->axes[X3].setScale(new Letter);
    plot->coordinates()->axes[X4].setScale(new Letter);
    plot->coordinates()->axes[Y1].setScale(new Letter(false));
    plot->coordinates()->axes[Y2].setScale(new Letter(false));
    plot->coordinates()->axes[Y3].setScale(new Letter(false));
    plot->coordinates()->axes[Y4].setScale(new Letter(false));
    plot->setTitle("Use the tics slider for this example!");
    plot->update();
}

void AxesMainWindow::complexItems()
{
    resetTics();
    plot->coordinates()->axes[Y1].setScale(new Imaginary);
    plot->coordinates()->axes[Y2].setScale(new Imaginary);
    plot->coordinates()->axes[Y3].setScale(new Imaginary);
    plot->coordinates()->axes[Y4].setScale(new Imaginary);
    plot->update();
}

void AxesMainWindow::timeItems()
{
    resetTics();
    plot->coordinates()->axes[Z1].setScale(new TimeItems);
    plot->coordinates()->axes[Z2].setScale(new TimeItems);
    plot->coordinates()->axes[Z3].setScale(new TimeItems);
    plot->coordinates()->axes[Z4].setScale(new TimeItems);
    plot->update();
}

void AxesMainWindow::customScale()
{
    resetTics();
    plot->coordinates()->axes[Z1].setScale(LOG10SCALE);
    plot->coordinates()->axes[Z3].setScale(LOG10SCALE);
    plot->coordinates()->axes[Z4].setScale(LOG10SCALE);
    plot->coordinates()->axes[Z2].setLabelString("log10(z)");
    //  plot->coordinates()->axes[Z4].setScale(new Qwt3DLogScale);
    //  plot->coordinates()->axes[Z1].setAutoScale(false);
    //  plot->coordinates()->axes[Z2].setAutoScale(false);
    //  plot->coordinates()->axes[Z3].setAutoScale(false);
    //  plot->coordinates()->axes[Z4].setAutoScale(false);

    plot->coordinates()->setGridLines(true, true, BACK);

    plot->update();
}
