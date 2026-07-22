#include "qwt3d_function.h"

#include "qwt3d_surface.h"


/**
 * @brief Default constructor
 */
Qwt3DFunction::Qwt3DFunction() : Qwt3DGridMapping()
{
}

/**
 * @brief Constructs a Qwt3DFunction object and assigns a Qwt3DSurface
 * @param pw Reference to a Qwt3DSurface item
 */
Qwt3DFunction::Qwt3DFunction(Qwt3DSurface& pw) : Qwt3DGridMapping()
{
    setSurface(&pw);
}

/**
 * @brief Constructs a Qwt3DFunction object and assigns a Qwt3DSurface
 * @param pw Pointer to a Qwt3DSurface item
 */
Qwt3DFunction::Qwt3DFunction(Qwt3DSurface* pw) : Qwt3DGridMapping()
{
    setSurface(pw);
}

/**
 * @brief Assigns the object to another surface - call before create()
 * @param surface Reference to a Qwt3DSurface item
 */
void Qwt3DFunction::assign(Qwt3DSurface& surface)
{
    if (&surface != this->surface())
        setSurface(&surface);
}

/**
 * @brief Assigns the object to another surface - call before create()
 * @param surface Pointer to a Qwt3DSurface item
 */
void Qwt3DFunction::assign(Qwt3DSurface* surface)
{
    if (surface != this->surface())
        setSurface(surface);
}

/**
 * @brief Sets minimum z value for the function
 * @param val Minimum z value
 */
void Qwt3DFunction::setMinZ(double val)
{
    range().minVertex.z = val;
}

/**
 * @brief Sets maximum z value for the function
 * @param val Maximum z value
 */
void Qwt3DFunction::setMaxZ(double val)
{
    range().maxVertex.z = val;
}

/**
 * @brief Creates data representation for the actual assigned Qwt3DSurface
 * @return True on success, false if mesh is too small or no surface assigned
 * @details Allocates data arrays, evaluates the function operator() over the
 *          mesh grid, clips values to the min/max z range, and loads data
 *          into the assigned Qwt3DSurface.
 */
bool Qwt3DFunction::create()
{
    const unsigned int um = meshU();
    const unsigned int vm = meshV();

    if ((um <= 2) || (vm <= 2) || !surface())
        return false;

    /* allocate some space for the mesh */
    double** data = new double*[um];

    unsigned i, j;
    for (i = 0; i < um; i++) {
        data[i] = new double[vm];
    }

    /* get the data */

    double dx = (maxU() - minU()) / (um - 1);
    double dy = (maxV() - minV()) / (vm - 1);

    for (i = 0; i < um; ++i) {
        for (j = 0; j < vm; ++j) {
            data[i][j] = operator()(minU() + i * dx, minV() + j * dy);

            if (data[i][j] > range().maxVertex.z)
                data[i][j] = range().maxVertex.z;
            else if (data[i][j] < range().minVertex.z)
                data[i][j] = range().minVertex.z;
        }
    }

    Q_ASSERT(surface());
    if (!surface()) {
        fprintf(stderr, "Qwt3DFunction: no valid Qwt3DSurface assigned");
    } else {
        surface()->loadFromData(data, um, vm, minU(), maxU(), minV(), maxV());
    }

    for (i = 0; i < um; i++) {
        delete[] data[i];
    }

    delete[] data;

    return true;
}

/**
 * @brief Assigns a new Qwt3DSurface and creates a data representation for it
 * @param pl Reference to a Qwt3DSurface item
 * @return True on success
 */
bool Qwt3DFunction::create(Qwt3DSurface& pl)
{
    assign(pl);
    return create();
}
