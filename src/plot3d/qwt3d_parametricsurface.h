#ifndef QWT3D_PARAMETRICSURFACE_H
#define QWT3D_PARAMETRICSURFACE_H

#include "qwt3d_gridmapping.h"



class Qwt3DSurface;

/**
 * @brief Abstract base class for parametric surfaces
 */
class QWT3D_EXPORT ParametricSurface : public GridMapping
{
    QWT_DECLARE_PRIVATE(ParametricSurface)

public:
    // Constructs ParametricSurface object w/o assigned Qwt3DSurface
    ParametricSurface();
    // Constructs ParametricSurface object and assigns a Qwt3DSurface
    explicit ParametricSurface(Qwt3DSurface& plotWidget);
    // Constructs ParametricSurface object and assigns a Qwt3DSurface
    explicit ParametricSurface(Qwt3DSurface* plotWidget);
    ~ParametricSurface() override;
    // Overwrite this
    virtual Triple operator()(double u, double v) = 0;
    // Assigns a new Qwt3DSurface and creates a data representation for it
    virtual bool create(Qwt3DSurface& plotWidget);
    // Creates data representation for the actual assigned Qwt3DSurface
    virtual bool create();
    // Assigns the object to another surface - call before create()
    void assign(Qwt3DSurface& plotWidget);
    // Assigns the object to another surface - call before create()
    void assign(Qwt3DSurface* plotWidget);
    // Provide information about periodicity of the 'u' resp. 'v' domains
    void setPeriodic(bool u, bool v);
};


#endif  // QWT3D_PARAMETRICSURFACE_H
