#ifndef __EXAMPLE_H__
#define __EXAMPLE_H__

#include <math.h>
#include "qwt3d_parametricsurface.h"
#include "qwt3d_function.h"

class Rosenbrock : public Qwt3DFunction
{
public:
    Rosenbrock() : Qwt3DFunction() { }

    double operator()(double x, double y) override
    {
        return 0.7 * log10((1 - x) * (1 - x) + 10 * (y - x * x) * (y - x * x));
    }
};

class Hat : public Qwt3DFunction
{
public:
    Hat() : Qwt3DFunction()
    {
        // setMinZ(0.3);
        setDomain(0, 10, 0, 10);
    }

    double operator()(double x, double y) override
    {
        return 1.0 / (x * x + y * y + 0.5);
    }
};

class Ripple : public Qwt3DFunction
{
public:
    Ripple() : Qwt3DFunction()
    {
        double l = 12;
        setDomain(-l, l, -l, l);
    }

    double operator()(double x, double y) override
    {
        return (cos(sqrt(x * x + y * y)
                    + cos(sqrt(((x + .913 * 2 * Qwt3D_PI) * (x + .913 * 2 * Qwt3D_PI)) + y * y))
                    + cos(sqrt(((x - .913 * 2 * Qwt3D_PI) * (x - .913 * 2 * Qwt3D_PI))
                               + (y * y))))
                * 4);
    }
};

class Saddle : public Qwt3DFunction
{
public:
    Saddle() : Qwt3DFunction()
    {
        // setMaxZ(0.8);
    }

    double operator()(double x, double y) override { return x * x - y * y; }
};

class Mex : public Qwt3DFunction
{
public:
    Mex() : Qwt3DFunction()
    {
        // setMaxZ(0.8);
    }

    double operator()(double x, double y) override
    {
        double n = sqrt(x * x + y * y);

        if (n < DBL_MIN)
            return 20;

        return 20 * sin(sqrt(x * x + y * y)) / n;
    }
};

class Torus : public Qwt3DParametricSurface
{
public:
    Torus() : Qwt3DParametricSurface()
    {
        setMesh(41, 31);
        setDomain(-2 * Qwt3D_PI, 0, -2 * Qwt3D_PI, 0);
        setPeriodic(true, true);
    }

    Triple operator()(double u, double v) override
    {
        double x, y, z;
        double c = 1.9;
        x = (c + cos(v)) * cos(u);
        y = (c + cos(v)) * sin(u);
        z = sin(v) + cos(v);
        return Triple(x, y, z);
    }
};

class Seashell : public Qwt3DParametricSurface
{
public:
    Seashell() : Qwt3DParametricSurface()
    {
        setMesh(41, 131);
        setDomain(0, 2 * Qwt3D_PI, 0, 2 * Qwt3D_PI);
        setPeriodic(true, true);
    }

    Triple operator()(double u, double v) override
    {
        double x, y, z;
        double a = 1;
        double b = 6;
        double c = 0.5;
        int n = 3;

        double f = v / (2 * Qwt3D_PI);

        x = a * (1 - f) * cos(n * v) * (1 + cos(u)) + c * cos(n * v);
        y = a * (1 - f) * sin(n * v) * (1 + cos(u)) + c * sin(n * v);
        z = b * f + a * (1 - f) * sin(u);
        return Triple(x, y, z);
    }
};

class Boy : public Qwt3DParametricSurface
{
public:
    Boy() : Qwt3DParametricSurface()
    {
        setMesh(141, 131);
        setDomain(0, Qwt3D_PI, 0, Qwt3D_PI);
        setPeriodic(true, true);
    }

    Triple operator()(double u, double v) override
    {
        double x, y, z;
        double a = 2 / 3.;
        double b = sqrt(2.);

        x = a * (cos(u) * cos(2 * v) + b * sin(u) * cos(v)) * cos(u)
                / (b - sin(2 * u) * sin(3 * v));
        y = a * (cos(u) * sin(2 * v) - b * sin(u) * sin(v)) * cos(u)
                / (b - sin(2 * u) * sin(3 * v));
        z = b * cos(u) * cos(u) / (b - sin(2 * u) * sin(2 * v));

        return Triple(x, y, z);
    }
};

class Dini : public Qwt3DParametricSurface
{
public:
    Dini() : Qwt3DParametricSurface()
    {
        setMesh(141, 35);
        setDomain(0, 5 * Qwt3D_PI, 0.001, 2);
        setPeriodic(true, true);
    }

    Triple operator()(double u, double v) override
    {
        double x, y, z;
        double a = 5;
        double b = 1;

        x = a * cos(u) * sin(v);
        y = a * sin(u) * sin(v);
        z = a * (cos(v) + log(tan(v / 2))) + b * u;

        return Triple(x, y, z);
    }
};

void createCone(TripleField &conepos, CellField &conecell);

#endif
