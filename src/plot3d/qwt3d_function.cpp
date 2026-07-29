#include "qwt3d_function.h"

#include <vector>

/**
 * @brief Default constructor
 */
Qwt3DFunction::Qwt3DFunction() : Qwt3DGridMapping()
{
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
 * @brief Evaluates the function over the mesh grid and returns the result
 * @return Qwt3DFunctionData containing the z-value matrix and domain bounds
 * @details Allocates a z-value matrix, evaluates operator() over the
 *          mesh grid, clips values to the min/max z range, and returns
 *          the result. The caller is responsible for feeding this to
 *          Qwt3DSurface::loadFromData(). Returns an empty result
 *          (columns=0) if the mesh is too small.
 */
Qwt3DFunctionData Qwt3DFunction::create()
{
    const unsigned int um = meshU();
    const unsigned int vm = meshV();

    Qwt3DFunctionData result;
    result.columns = um;
    result.rows = vm;
    result.minx = minU();
    result.maxx = maxU();
    result.miny = minV();
    result.maxy = maxV();

    if (um <= 2 || vm <= 2) {
        result.columns = 0;
        result.rows = 0;
        return result;
    }

    result.z.resize(um);
    for (unsigned int i = 0; i < um; ++i)
        result.z[i].resize(vm);

    const double dx = (maxU() - minU()) / (um - 1);
    const double dy = (maxV() - minV()) / (vm - 1);

    for (unsigned int i = 0; i < um; ++i) {
        for (unsigned int j = 0; j < vm; ++j) {
            double val = operator()(minU() + i * dx, minV() + j * dy);

            if (val > range().maxVertex.z)
                val = range().maxVertex.z;
            else if (val < range().minVertex.z)
                val = range().minVertex.z;

            result.z[i][j] = val;
        }
    }

    return result;
}
