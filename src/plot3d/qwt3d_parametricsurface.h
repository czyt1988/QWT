#ifndef QWT3D_PARAMETRICSURFACE_H
#define QWT3D_PARAMETRICSURFACE_H

#include "qwt3d_gridmapping.h"
#include "qwt3d_types.h"



/**
 * @brief Abstract base class for parametric surfaces
 * @details A Qwt3DParametricSurface encapsulates a parametric surface
 *          defined by operator()(u, v). The user overrides operator() to
 *          produce a working object. setDomain, setMesh and create should
 *          be called for reasonable operating conditions.
 *
 *          create() returns a Qwt3DParametricData struct that the caller
 *          feeds to Qwt3DSurface::loadFromData(). The class no longer holds
 *          a back-pointer to the surface item (deferred smell #2, resolved).
 *
 * @code
 * class Torus : public Qwt3DParametricSurface
 * {
 * public:
 *     Triple operator()(double u, double v) override { ... }
 * };
 *
 * Torus torus;
 * torus.setMesh(40, 40);
 * torus.setDomain(0, 2*PI, 0, 2*PI);
 * torus.setPeriodic(true, true);
 * auto data = torus.create();
 * surface->loadFromData(data);
 * @endcode
 */
class QWT3D_EXPORT Qwt3DParametricSurface : public Qwt3DGridMapping
{
    QWT_DECLARE_PRIVATE(Qwt3DParametricSurface)

public:
    // Constructs Qwt3DParametricSurface object
    Qwt3DParametricSurface();
    ~Qwt3DParametricSurface() override;
    // Overwrite this
    virtual Triple operator()(double u, double v) = 0;
    // Evaluates the parametric surface over the mesh grid and returns the result
    virtual Qwt3DParametricData create();
    // Provide information about periodicity of the 'u' resp. 'v' domains
    void setPeriodic(bool u, bool v);
};

#endif
