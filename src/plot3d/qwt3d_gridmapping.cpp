#include "qwt3d_gridmapping.h"
#include "qwt3d_surfaceplot.h"


class GridMapping::PrivateData
{
    QWT_DECLARE_PUBLIC(GridMapping)

public:
    PrivateData(GridMapping* q)
        : q_ptr(q), m_plotwidget(nullptr), m_umesh(0), m_vmesh(0), m_minu(0.0), m_maxu(0.0), m_minv(0.0), m_maxv(0.0)
    {
    }

    ParallelEpiped m_range;
    SurfacePlot* m_plotwidget;
    unsigned int m_umesh;
    unsigned int m_vmesh;
    double m_minu;
    double m_maxu;
    double m_minv;
    double m_maxv;
};

/**
 * @brief Default constructor
 * @details Initializes with no plot widget, zero mesh dimensions, zero domain,
 *          and unrestricted z range.
 */
GridMapping::GridMapping() : QWT_PIMPL_CONSTRUCT
{
    setMesh(0, 0);
    setDomain(0, 0, 0, 0);
    restrictRange(ParallelEpiped(Triple(-DBL_MAX, -DBL_MAX, -DBL_MAX), Triple(DBL_MAX, DBL_MAX, DBL_MAX)));
}

GridMapping::~GridMapping() = default;

/**
 * @brief Sets the number of mesh columns and rows
 * @param columns Number of columns (u direction)
 * @param rows Number of rows (v direction)
 */
void GridMapping::setMesh(unsigned int columns, unsigned int rows)
{
    QWT_D(d);
    d->m_umesh = columns;
    d->m_vmesh = rows;
}

/**
 * @brief Sets the domain (parameter range) for the u and v directions
 * @param minu Minimum u value
 * @param maxu Maximum u value
 * @param minv Minimum v value
 * @param maxv Maximum v value
 */
void GridMapping::setDomain(double minu, double maxu, double minv, double maxv)
{
    QWT_D(d);
    d->m_minu = minu;
    d->m_maxu = maxu;
    d->m_minv = minv;
    d->m_maxv = maxv;
}

/**
 * @brief Restricts the data range to a parallelepiped
 * @param p The parallelepiped defining the restricted range
 */
void GridMapping::restrictRange(ParallelEpiped const& p)
{
    QWT_D(d);
    d->m_range = p;
}

SurfacePlot* GridMapping::plotWidget() const
{
    QWT_DC(d);
    return d->m_plotwidget;
}

void GridMapping::setPlotWidget(SurfacePlot* pw)
{
    QWT_D(d);
    d->m_plotwidget = pw;
}

ParallelEpiped& GridMapping::range()
{
    QWT_D(d);
    return d->m_range;
}

const ParallelEpiped& GridMapping::range() const
{
    QWT_DC(d);
    return d->m_range;
}

unsigned int GridMapping::meshU() const
{
    QWT_DC(d);
    return d->m_umesh;
}

unsigned int GridMapping::meshV() const
{
    QWT_DC(d);
    return d->m_vmesh;
}

double GridMapping::minU() const
{
    QWT_DC(d);
    return d->m_minu;
}

double GridMapping::maxU() const
{
    QWT_DC(d);
    return d->m_maxu;
}

double GridMapping::minV() const
{
    QWT_DC(d);
    return d->m_minv;
}

double GridMapping::maxV() const
{
    QWT_DC(d);
    return d->m_maxv;
}
