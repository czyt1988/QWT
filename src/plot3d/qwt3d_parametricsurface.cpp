#include "qwt3d_parametricsurface.h"

#include "qwt3d_surface.h"


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

Qwt3DParametricSurface::Qwt3DParametricSurface(Qwt3DSurface& pw) : Qwt3DGridMapping(), QWT_PIMPL_CONSTRUCT
{
    setSurface(&pw);
}

Qwt3DParametricSurface::Qwt3DParametricSurface(Qwt3DSurface* pw) : Qwt3DGridMapping(), QWT_PIMPL_CONSTRUCT
{
    setSurface(pw);
}

Qwt3DParametricSurface::~Qwt3DParametricSurface() = default;

void Qwt3DParametricSurface::setPeriodic(bool u, bool v)
{
    QWT_D(d);
    d->m_uperiodic = u;
    d->m_vperiodic = v;
}

void Qwt3DParametricSurface::assign(Qwt3DSurface& surface)
{
    if (&surface != this->surface())
        setSurface(&surface);
}

void Qwt3DParametricSurface::assign(Qwt3DSurface* surface)
{
    if (surface != this->surface())
        setSurface(surface);
}

/**
 * @brief Creates the parametric surface data and loads it into the surface item
 * @return True on success, false if meshU() <= 2, meshV() <= 2, or surface() is null
 * @details For surface() != nullptr the function permanently assigns her argument (In fact, assign(surface) is called)
 */
bool Qwt3DParametricSurface::create()
{
    const unsigned int um = meshU();
    const unsigned int vm = meshV();

    if ((um <= 2) || (vm <= 2) || !surface())
        return false;

    /* allocate some cache for the mesh */
    Triple** data = new Triple*[um];

    unsigned i, j;
    for (i = 0; i < um; i++) {
        data[i] = new Triple[vm];
    }

    /* get the data */

    double du = (maxU() - minU()) / (um - 1);
    double dv = (maxV() - minV()) / (vm - 1);

    for (i = 0; i < um; ++i) {
        for (j = 0; j < vm; ++j) {
            data[i][j] = operator()(minU() + i * du, minV() + j * dv);

            if (data[i][j].x > range().maxVertex.x)
                data[i][j].x = range().maxVertex.x;
            if (data[i][j].y > range().maxVertex.y)
                data[i][j].y = range().maxVertex.y;
            if (data[i][j].z > range().maxVertex.z)
                data[i][j].z = range().maxVertex.z;
            if (data[i][j].x < range().minVertex.x)
                data[i][j].x = range().minVertex.x;
            if (data[i][j].y < range().minVertex.y)
                data[i][j].y = range().minVertex.y;
            if (data[i][j].z < range().minVertex.z)
                data[i][j].z = range().minVertex.z;
        }
    }

    QWT_D(d);
    surface()->loadFromData(data, um, vm, d->m_uperiodic, d->m_vperiodic);

    for (i = 0; i < um; i++) {
        delete[] data[i];
    }

    delete[] data;

    return true;
}

bool Qwt3DParametricSurface::create(Qwt3DSurface& pl)
{
    assign(pl);
    return create();
}
