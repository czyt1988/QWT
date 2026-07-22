#ifndef QWT3D_PARAMETRICSURFACE_H
#define QWT3D_PARAMETRICSURFACE_H

#include "qwt3d_gridmapping.h"



class SurfacePlot;

/**
 * @brief Abstract base class for parametric surfaces
 */
class QWT3D_EXPORT ParametricSurface : public GridMapping
{
    QWT_DECLARE_PRIVATE(ParametricSurface)

public:
    // Constructs ParametricSurface object w/o assigned SurfacePlot
    ParametricSurface();
    // Constructs ParametricSurface object and assigns a SurfacePlot
    explicit ParametricSurface(SurfacePlot& plotWidget);
    // Constructs ParametricSurface object and assigns a SurfacePlot
    explicit ParametricSurface(SurfacePlot* plotWidget);
    ~ParametricSurface() override;
    // Overwrite this
    virtual Triple operator()(double u, double v) = 0;
    // Assigns a new SurfacePlot and creates a data representation for it
    virtual bool create(SurfacePlot& plotWidget);
    // Creates data representation for the actual assigned SurfacePlot
    virtual bool create();
    // Assigns the object to another widget - call before create()
    void assign(SurfacePlot& plotWidget);
    // Assigns the object to another widget - call before create()
    void assign(SurfacePlot* plotWidget);
    // Provide information about periodicity of the 'u' resp. 'v' domains
    void setPeriodic(bool u, bool v);
};


#endif  // QWT3D_PARAMETRICSURFACE_H