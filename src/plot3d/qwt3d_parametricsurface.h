#ifndef QWT3D_PARAMETRICSURFACE_H
#define QWT3D_PARAMETRICSURFACE_H

#include "qwt3d_gridmapping.h"



class Qwt3DSurface;

/**
 * @brief Abstract base class for parametric surfaces
 */
class QWT3D_EXPORT Qwt3DParametricSurface : public Qwt3DGridMapping
{
    QWT_DECLARE_PRIVATE(Qwt3DParametricSurface)

public:
    // Constructs Qwt3DParametricSurface object w/o assigned Qwt3DSurface
    Qwt3DParametricSurface();
    // Constructs Qwt3DParametricSurface object and assigns a Qwt3DSurface
    explicit Qwt3DParametricSurface(Qwt3DSurface& surface);
    // Constructs Qwt3DParametricSurface object and assigns a Qwt3DSurface
    explicit Qwt3DParametricSurface(Qwt3DSurface* surface);
    ~Qwt3DParametricSurface() override;
    // Overwrite this
    virtual Triple operator()(double u, double v) = 0;
    // Assigns a new Qwt3DSurface and creates a data representation for it
    virtual bool create(Qwt3DSurface& surface);
    // Creates data representation for the actual assigned Qwt3DSurface
    virtual bool create();
    // Assigns the object to another surface - call before create()
    void assign(Qwt3DSurface& surface);
    // Assigns the object to another surface - call before create()
    void assign(Qwt3DSurface* surface);
    // Provide information about periodicity of the 'u' resp. 'v' domains
    void setPeriodic(bool u, bool v);
};


#endif  // QWT3D_PARAMETRICSURFACE_H
