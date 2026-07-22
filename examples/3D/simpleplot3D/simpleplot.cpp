//-----------------------------------------------------------------
//              simpleplot.cpp
//
//      A simple example which shows how to use Qwt3DPlot + Qwt3DSurface
//-----------------------------------------------------------------

#include <math.h>
#include <qapplication.h>
#include <qwt3d_plot.h>
#include <qwt3d_surface.h>
#include <qwt3d_function.h>

class Rosenbrock : public Qwt3DFunction
{
public:
    Rosenbrock() : Qwt3DFunction() { }

    double operator()(double x, double y) override
    {
        return log((1 - x) * (1 - x) + 100 * (y - x * x) * (y - x * x)) / 8;
    }
};

class PlotWidget : public Qwt3DPlot
{
public:
    PlotWidget();
};

PlotWidget::PlotWidget()
{
    auto* surface = new Qwt3DSurface();
    surface->attach(this);

    setTitle("A Simple SurfacePlot Demonstration");

    Rosenbrock rosenbrock;

    rosenbrock.setMesh(41, 31);
    rosenbrock.setDomain(-1.73, 1.5, -1.5, 1.5);
    rosenbrock.setMinZ(-10);

    rosenbrock.assign(*surface);
    rosenbrock.create();

    setRotation(30, 0, 15);
    setScale(1, 1, 1);
    setShift(0.15, 0, 0);
    setZoom(0.9);

    for (unsigned i = 0; i != coordinates()->axes.size(); ++i) {
        coordinates()->axes[i].setMajors(7);
        coordinates()->axes[i].setMinors(4);
    }

    coordinates()->axes[X1].setLabelString("x-axis");
    coordinates()->axes[Y1].setLabelString("y-axis");
    // coordinates()->axes[Z1].setLabelString(QChar(0x38f)); // Omega - see
    // http://www.unicode.org/charts/

    coordinates()->setStyle(BOX);

    update();
}

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    PlotWidget plot;
    plot.resize(800, 600);
    plot.show();
    return a.exec();
}
