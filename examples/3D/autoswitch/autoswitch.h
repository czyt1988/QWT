#ifndef AUTOSWITCH_H__
#define AUTOSWITCH_H__

#include <qtimer.h>
#include "qwt3d_plot.h"
#include "qwt3d_surface.h"
#include "qwt3d_function.h"

class Saddle : public Qwt3DFunction
{
public:
    Saddle() : Qwt3DFunction()
    {
        setDomain(-2.5, 2.5, -2.5, 2.5);
        setMaxZ(1.5);
        setMinZ(-1.5);
        setMesh(31, 31);
    }

    double operator()(double x, double y) override { return x * y; }
};

class Hat : public Qwt3DFunction
{
public:
    Hat() : Qwt3DFunction()
    {
        setDomain(-1.5, 1.5, -1.5, 1.5);
        setMesh(41, 41);
    }

    double operator()(double x, double y) override { return 1.0 / (x * x + y * y + 0.3); }
};

class Plot : public Qwt3DPlot
{
    Q_OBJECT

public:
    Plot(QWidget *pw, int updateinterval);

public slots:
    void rotate();
};

#endif
