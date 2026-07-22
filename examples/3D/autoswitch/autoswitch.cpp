#include <qapplication.h>
#include <qsplitter.h>
#include <qtimer.h>

#include "autoswitch.h"

//--------------------------------------------------------------------
//              autoswitch.cpp
//
//      Demonstrates autoswitching axes with a cutted saddle as data
//--------------------------------------------------------------------

Plot::Plot(QWidget *pw, int updateinterval) : Qwt3DPlot(pw)
{
    auto* surface = new Qwt3DSurface();
    surface->attach(this);

    setRotation(30, 0, 15);
    setShift(0.1, 0, 0);
    setZoom(0.8);
    coordinates()->setNumberFont("Courier", 8);

    for (unsigned i = 0; i != coordinates()->axes.size(); ++i) {
        coordinates()->axes[i].setMajors(7);
        coordinates()->axes[i].setMinors(4);
    }

    coordinates()->axes[X1].setLabelString("x");
    coordinates()->axes[Y1].setLabelString("y");
    coordinates()->axes[Z1].setLabelString("z");
    coordinates()->axes[X2].setLabelString("x");
    coordinates()->axes[Y2].setLabelString("y");
    coordinates()->axes[Z2].setLabelString("z");
    coordinates()->axes[X3].setLabelString("x");
    coordinates()->axes[Y3].setLabelString("y");
    coordinates()->axes[Z3].setLabelString("z");
    coordinates()->axes[X4].setLabelString("x");
    coordinates()->axes[Y4].setLabelString("y");
    coordinates()->axes[Z4].setLabelString("z");

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Plot::rotate);

    timer->start(updateinterval);
}

void Plot::rotate()
{
    int prec = 3;

    setRotation((int(prec * xRotation() + 2) % (360 * prec)) / double(prec),
                (int(prec * yRotation() + 2) % (360 * prec)) / double(prec),
                (int(prec * zRotation() + 2) % (360 * prec)) / double(prec));
}

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    QSplitter *spl = new QSplitter(Qt::Horizontal);

    // Plot 1: Saddle with iso floor
    Plot *plot1 = new Plot(spl, 30);
    // Get the surface item for plot1
    Qwt3DSurface *surface1 = nullptr;
    for (auto* item : plot1->itemList()) {
        surface1 = dynamic_cast<Qwt3DSurface*>(item);
        if (surface1) break;
    }
    if (surface1) {
        surface1->setFloorStyle(FLOORISO);
        Saddle saddle;
        saddle.assign(*surface1);
        saddle.create();
    }
    plot1->setTitle("Autoswitching axes");
    plot1->setBackgroundColor(RGBA(1, 1, 157. / 255));
    plot1->update();

    // Plot 2: Hat with hiddenline style
    Plot *plot2 = new Plot(spl, 80);
    plot2->setZoom(0.8);
    Qwt3DSurface *surface2 = nullptr;
    for (auto* item : plot2->itemList()) {
        surface2 = dynamic_cast<Qwt3DSurface*>(item);
        if (surface2) break;
    }
    if (surface2) {
        Hat hat;
        hat.assign(*surface2);
        hat.create();
        surface2->setPlotStyle(HIDDENLINE);
        surface2->setFloorStyle(FLOORDATA);
    }
    plot2->coordinates()->setStyle(FRAME);
    plot2->setBackgroundColor(RGBA(1, 1, 157. / 255));
    plot2->update();

    spl->resize(800, 400);
    spl->show();
    return a.exec();
}
