#include "qwt3d_parametricsurface.h"

#include <vector>

class Qwt3DParametricSurface::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DParametricSurface)

public:
    PrivateData(Qwt3DParametricSurface* q) : q_ptr(q), m_uperiodic(false), m_vperiodic(false)
    {
    }

    bool m_uperiodic;
    bool m_vperiodic;
};

Qwt3DParametricSurface::Qwt3DParametricSurface() : Qwt3DGridMapping(), QWT_PIMPL_CONSTRUCT
{
}

Qwt3DParametricSurface::~Qwt3DParametricSurface() = default;

void Qwt3DParametricSurface::setPeriodic(bool u, bool v)
{
    QWT_D(d);
    d->m_uperiodic = u;
    d->m_vperiodic = v;
}

/**
 * @brief Evaluates the parametric surface over the mesh grid and returns the result
 * @return Qwt3DParametricData containing the xyz triple matrix and periodicity flags
 * @details Allocates a triple matrix, evaluates operator()(u, v) over the
 *          mesh grid, clips values to the range bounds, and returns the result.
 *          The caller is responsible for feeding this to
 *          Qwt3DSurface::loadFromData(). Returns an empty result
 *          (columns=0) if the mesh is too small.
 */
Qwt3DParametricData Qwt3DParametricSurface::create()
{
    const unsigned int um = meshU();
    const unsigned int vm = meshV();

    Qwt3DParametricData result;
    result.columns = um;
    result.rows = vm;

    QWT_D(d);
    result.uperiodic = d->m_uperiodic;
    result.vperiodic = d->m_vperiodic;

    if (um <= 2 || vm <= 2) {
        result.columns = 0;
        result.rows = 0;
        return result;
    }

    result.vertices.resize(um);
    for (unsigned int i = 0; i < um; ++i)
        result.vertices[i].resize(vm);

    const double du = (maxU() - minU()) / (um - 1);
    const double dv = (maxV() - minV()) / (vm - 1);

    for (unsigned int i = 0; i < um; ++i) {
        for (unsigned int j = 0; j < vm; ++j) {
            Triple val = operator()(minU() + i * du, minV() + j * dv);

            if (val.x > range().maxVertex.x)
                val.x = range().maxVertex.x;
            if (val.y > range().maxVertex.y)
                val.y = range().maxVertex.y;
            if (val.z > range().maxVertex.z)
                val.z = range().maxVertex.z;
            if (val.x < range().minVertex.x)
                val.x = range().minVertex.x;
            if (val.y < range().minVertex.y)
                val.y = range().minVertex.y;
            if (val.z < range().minVertex.z)
                val.z = range().minVertex.z;

            result.vertices[i][j] = val;
        }
    }

    return result;
}
