#include "qwt3d_parametricsurface.h"
#include "qwt3d_surfaceplot.h"


class ParametricSurface::PrivateData
{
    QWT_DECLARE_PUBLIC(ParametricSurface)

public:
    PrivateData(ParametricSurface* q) : q_ptr(q), m_uperiodic(false), m_vperiodic(false)
    {
    }

    bool m_uperiodic;
    bool m_vperiodic;
};

ParametricSurface::ParametricSurface() : GridMapping(), QWT_PIMPL_CONSTRUCT
{
}

ParametricSurface::ParametricSurface(SurfacePlot& pw) : GridMapping(), QWT_PIMPL_CONSTRUCT
{
    setPlotWidget(&pw);
}

ParametricSurface::ParametricSurface(SurfacePlot* pw) : GridMapping(), QWT_PIMPL_CONSTRUCT
{
    setPlotWidget(pw);
}

ParametricSurface::~ParametricSurface() = default;

void ParametricSurface::setPeriodic(bool u, bool v)
{
    QWT_D(d);
    d->m_uperiodic = u;
    d->m_vperiodic = v;
}

void ParametricSurface::assign(SurfacePlot& plotWidget)
{
    if (&plotWidget != this->plotWidget())
        setPlotWidget(&plotWidget);
}

void ParametricSurface::assign(SurfacePlot* plotWidget)
{
    if (plotWidget != this->plotWidget())
        setPlotWidget(plotWidget);
}

/**
 * @brief Creates the parametric surface data and loads it into the plot widget
 * @return True on success, false if meshU() <= 2, meshV() <= 2, or plotWidget() is null
 * @details For plotWidget() != nullptr the function permanently assigns her argument (In fact, assign(plotWidget) is called)
 */
bool ParametricSurface::create()
{
    const unsigned int um = meshU();
    const unsigned int vm = meshV();

    if ((um <= 2) || (vm <= 2) || !plotWidget())
        return false;

    /* allocate some cache for the mesh */
    Triple** data = new Triple*[ um ];

    unsigned i, j;
    for (i = 0; i < um; i++) {
        data[ i ] = new Triple[ vm ];
    }

    /* get the data */

    double du = (maxU() - minU()) / (um - 1);
    double dv = (maxV() - minV()) / (vm - 1);

    for (i = 0; i < um; ++i) {
        for (j = 0; j < vm; ++j) {
            data[ i ][ j ] = operator()(minU() + i * du, minV() + j * dv);

            if (data[ i ][ j ].x > range().maxVertex.x)
                data[ i ][ j ].x = range().maxVertex.x;
            else if (data[ i ][ j ].y > range().maxVertex.y)
                data[ i ][ j ].y = range().maxVertex.y;
            else if (data[ i ][ j ].z > range().maxVertex.z)
                data[ i ][ j ].z = range().maxVertex.z;
            else if (data[ i ][ j ].x < range().minVertex.x)
                data[ i ][ j ].x = range().minVertex.x;
            else if (data[ i ][ j ].y < range().minVertex.y)
                data[ i ][ j ].y = range().minVertex.y;
            else if (data[ i ][ j ].z < range().minVertex.z)
                data[ i ][ j ].z = range().minVertex.z;
        }
    }

    QWT_D(d);
    static_cast< SurfacePlot* >(plotWidget())->loadFromData(data, um, vm, d->m_uperiodic, d->m_vperiodic);

    for (i = 0; i < um; i++) {
        delete[] data[ i ];
    }

    delete[] data;

    return true;
}

bool ParametricSurface::create(SurfacePlot& pl)
{
    assign(pl);
    return create();
}
