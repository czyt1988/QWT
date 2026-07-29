#include "SurfaceData.h"

#include <cmath>

#include "qwt3d_function.h"
#include "qwt3d_parametricsurface.h"
#include "qwt3d_surface.h"
#include "qwt3d_types.h"

// ---------------------------------------------------------------------------
// Common domain constants
// ---------------------------------------------------------------------------
static constexpr double DOMAIN_EXTENT = 3.0;
static constexpr int GRID_SIZE = 50;

// ---------------------------------------------------------------------------
// RippleProvider — z = sin(2t + sqrt(x^2 + y^2))
// ---------------------------------------------------------------------------

class RippleFunction : public Qwt3DFunction
{
public:
    RippleFunction()
        : Qwt3DFunction()
    {
        setMesh(GRID_SIZE, GRID_SIZE);
        setDomain(-DOMAIN_EXTENT, DOMAIN_EXTENT, -DOMAIN_EXTENT, DOMAIN_EXTENT);
    }

    double operator()(double x, double y) override
    {
        return std::sin(2.0 * m_time + std::sqrt(x * x + y * y));
    }

    void setTime(double t) { m_time = t; }

private:
    double m_time = 0.0;
};

RippleProvider::RippleProvider() = default;

void RippleProvider::create(Qwt3DSurface& surface)
{
    RippleFunction ripple;
    ripple.setTime(m_time);
    surface.loadFromData(ripple.create());
}

// ---------------------------------------------------------------------------
// GaussianProvider — z = 2 * exp(-(x^2 + y^2) / 4)
// ---------------------------------------------------------------------------

class GaussianFunction : public Qwt3DFunction
{
public:
    GaussianFunction()
        : Qwt3DFunction()
    {
        setMesh(GRID_SIZE, GRID_SIZE);
        setDomain(-DOMAIN_EXTENT, DOMAIN_EXTENT, -DOMAIN_EXTENT, DOMAIN_EXTENT);
    }

    double operator()(double x, double y) override
    {
        return 2.0 * std::exp(-(x * x + y * y) / 4.0);
    }
};

void GaussianProvider::create(Qwt3DSurface& surface)
{
    GaussianFunction gaussian;
    surface.loadFromData(gaussian.create());
}

// ---------------------------------------------------------------------------
// SaddleProvider — z = (x^2 - y^2) / 8
// ---------------------------------------------------------------------------

class SaddleFunction : public Qwt3DFunction
{
public:
    SaddleFunction()
        : Qwt3DFunction()
    {
        setMesh(GRID_SIZE, GRID_SIZE);
        setDomain(-DOMAIN_EXTENT, DOMAIN_EXTENT, -DOMAIN_EXTENT, DOMAIN_EXTENT);
    }

    double operator()(double x, double y) override
    {
        return (x * x - y * y) / 8.0;
    }
};

void SaddleProvider::create(Qwt3DSurface& surface)
{
    SaddleFunction saddle;
    surface.loadFromData(saddle.create());
}

// ---------------------------------------------------------------------------
// SombreroProvider — z = sin(sqrt(x^2 + y^2)) / max(r, 0.2)
// ---------------------------------------------------------------------------

class SombreroFunction : public Qwt3DFunction
{
public:
    SombreroFunction()
        : Qwt3DFunction()
    {
        setMesh(GRID_SIZE, GRID_SIZE);
        setDomain(-DOMAIN_EXTENT, DOMAIN_EXTENT, -DOMAIN_EXTENT, DOMAIN_EXTENT);
    }

    double operator()(double x, double y) override
    {
        double r = std::sqrt(x * x + y * y);
        if (r < 0.2)
            r = 0.2;
        return std::sin(r) / r;
    }
};

void SombreroProvider::create(Qwt3DSurface& surface)
{
    SombreroFunction sombrero;
    surface.loadFromData(sombrero.create());
}

// ---------------------------------------------------------------------------
// TorusProvider — parametric torus
// ---------------------------------------------------------------------------

class TorusSurface : public Qwt3DParametricSurface
{
public:
    TorusSurface()
        : Qwt3DParametricSurface()
    {
        setMesh(40, 40);
        setDomain(0, 2.0 * Qwt3D_PI, 0, 2.0 * Qwt3D_PI);
        setPeriodic(true, true);
    }

    Triple operator()(double u, double v) override
    {
        const double R = 2.0; // major radius
        const double r = 0.7; // minor radius
        return Triple(
            (R + r * std::cos(v)) * std::cos(u),
            (R + r * std::cos(v)) * std::sin(u),
            r * std::sin(v));
    }
};

void TorusProvider::create(Qwt3DSurface& surface)
{
    TorusSurface torus;
    surface.loadFromData(torus.create());
}
