#ifndef QWT3D_FUNCTION_H
#define QWT3D_FUNCTION_H

#include "qwt3d_gridmapping.h"



class Qwt3DSurface;

/**
 * @brief Abstract base class for mathematical functions
 * @details A Qwt3DFunction encapsulates a mathematical function with rectangular domain. The user has to
 *          adapt the pure virtual operator() to get a working object. Also, the client code should call
 *          setDomain, setMesh and create for reasonable operating conditions.
 */
class QWT3D_EXPORT Qwt3DFunction : public Qwt3DGridMapping
{

public:
    // Constructs Qwt3DFunction object w/o assigned Qwt3DSurface
    Qwt3DFunction();
    // Constructs Qwt3DFunction object and assigns a Qwt3DSurface
    explicit Qwt3DFunction(Qwt3DSurface& surface);
    // Constructs Qwt3DFunction object and assigns a Qwt3DSurface
    explicit Qwt3DFunction(Qwt3DSurface* surface);
    // Overwrite this
    virtual double operator()(double x, double y) = 0;

    // Sets minimal z value
    void setMinZ(double val);
    // Sets maximal z value
    void setMaxZ(double val);

    // Assigns a new Qwt3DSurface and creates a data representation for it
    virtual bool create(Qwt3DSurface& surface);
    // Creates data representation for the actual assigned Qwt3DSurface
    virtual bool create();
    // Assigns the object to another surface - call before create()
    void assign(Qwt3DSurface& surface);
    // Assigns the object to another surface - call before create()
    void assign(Qwt3DSurface* surface);
};


#endif  // QWT3D_FUNCTION_H
