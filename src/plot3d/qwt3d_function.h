#ifndef QWT3D_FUNCTION_H
#define QWT3D_FUNCTION_H

#include "qwt3d_gridmapping.h"
#include "qwt3d_types.h"



/**
 * @brief Abstract base class for mathematical functions
 * @details A Qwt3DFunction encapsulates a mathematical function with rectangular domain. The user has to
 *          adapt the pure virtual operator() to get a working object. Also, the client code should call
 *          setDomain, setMesh and create for reasonable operating conditions.
 *
 *          create() returns a Qwt3DFunctionData struct that the caller feeds to
 *          Qwt3DSurface::loadFromData(). The function no longer holds a back-pointer
 *          to the surface item (deferred smell #2, resolved).
 *
 * @code
 * class MyFunction : public Qwt3DFunction
 * {
 * public:
 *     double operator()(double x, double y) override { return std::sin(x + y); }
 * };
 *
 * MyFunction func;
 * func.setMesh(50, 50);
 * func.setDomain(-3, 3, -3, 3);
 * auto data = func.create();
 * surface->loadFromData(data);
 * @endcode
 */
class QWT3D_EXPORT Qwt3DFunction : public Qwt3DGridMapping
{

public:
    // Constructs Qwt3DFunction object
    Qwt3DFunction();
    // Overwrite this
    virtual double operator()(double x, double y) = 0;

    // Sets minimal z value
    void setMinZ(double val);
    // Sets maximal z value
    void setMaxZ(double val);

    // Evaluates the function over the mesh grid and returns the result
    virtual Qwt3DFunctionData create();
};

#endif  // QWT3D_FUNCTION_H
