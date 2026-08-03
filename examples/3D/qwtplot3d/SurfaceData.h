#ifndef SURFACE_DATA_H
#define SURFACE_DATA_H

#include <QString>

class Qwt3DSurface;

/**
 * @brief Abstract interface for surface data providers
 * @details Each provider generates data onto a Qwt3DSurface via create().
 *          Animated providers support advance(dt) for time-dependent updates.
 */
class SurfaceDataProvider
{
public:
    virtual ~SurfaceDataProvider() = default;

    /// Generate data onto the given surface
    virtual void create(Qwt3DSurface& surface) = 0;

    /// Advance time by dt (only meaningful for animated providers)
    virtual void advance(double /*dt*/) {}

    /// Whether this provider is time-dependent
    virtual bool isAnimated() const { return false; }

    /// Human-readable name
    virtual QString name() const = 0;
};

// z = sin(2t + sqrt(x^2 + y^2)) — animated ripple
class RippleProvider : public SurfaceDataProvider
{
public:
    RippleProvider();
    void create(Qwt3DSurface& surface) override;
    void advance(double dt) override { m_time += dt; }
    bool isAnimated() const override { return true; }
    QString name() const override { return QStringLiteral("Ripple"); }

private:
    double m_time = 0.0;
};

// z = 2 * exp(-(x^2 + y^2) / 4) — gaussian bump
class GaussianProvider : public SurfaceDataProvider
{
public:
    void create(Qwt3DSurface& surface) override;
    QString name() const override { return QStringLiteral("Gaussian"); }
};

// z = (x^2 - y^2) / 8 — saddle point
class SaddleProvider : public SurfaceDataProvider
{
public:
    void create(Qwt3DSurface& surface) override;
    QString name() const override { return QStringLiteral("Saddle"); }
};

// z = sin(sqrt(x^2 + y^2)) / max(r, 0.2) — sombrero
class SombreroProvider : public SurfaceDataProvider
{
public:
    void create(Qwt3DSurface& surface) override;
    QString name() const override { return QStringLiteral("Sombrero"); }
};

// Parametric torus: (R + r*cos(v))*cos(u), (R + r*cos(v))*sin(u), r*sin(v)
class TorusProvider : public SurfaceDataProvider
{
public:
    void create(Qwt3DSurface& surface) override;
    QString name() const override { return QStringLiteral("Torus"); }
};

#endif // SURFACE_DATA_H
