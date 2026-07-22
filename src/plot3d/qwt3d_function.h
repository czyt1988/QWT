#ifndef QWT3D_FUNCTION_H
#define QWT3D_FUNCTION_H

#include "qwt3d_gridmapping.h"



class Qwt3DSurface;

/**
 * @brief Abstract base class for mathematical functions
 * @details A Function encapsulates a mathematical function with rectangular domain. The user has to
 *          adapt the pure virtual operator() to get a working object. Also, the client code should call
 *          setDomain, setMesh and create for reasonable operating conditions.
 */
class QWT3D_EXPORT Function : public GridMapping
{

public:
    // Constructs Function object w/o assigned Qwt3DSurface
    Function();
    // Constructs Function object and assigns a Qwt3DSurface
    explicit Function(Qwt3DSurface& plotWidget);
    // Constructs Function object and assigns a Qwt3DSurface
    explicit Function(Qwt3DSurface* plotWidget);
    // Overwrite this
    virtual double operator()(double x, double y) = 0;

    // Sets minimal z value
    void setMinZ(double val);
    // Sets maximal z value
    void setMaxZ(double val);

    // Assigns a new Qwt3DSurface and creates a data representation for it
    virtual bool create(Qwt3DSurface& plotWidget);
    // Creates data representation for the actual assigned Qwt3DSurface
    virtual bool create();
    // Assigns the object to another surface - call before create()
    void assign(Qwt3DSurface& plotWidget);
    // Assigns the object to another surface - call before create()
    void assign(Qwt3DSurface* plotWidget);
};


#endif  // QWT3D_FUNCTION_H
