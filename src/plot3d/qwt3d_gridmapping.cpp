#include "qwt3d_gridmapping.h"
#include "qwt3d_surface.h"


class Qwt3DGridMapping::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DGridMapping)

public:
    PrivateData(Qwt3DGridMapping* q)
        : q_ptr(q), m_surface(nullptr), m_umesh(0), m_vmesh(0), m_minu(0.0), m_maxu(0.0), m_minv(0.0), m_maxv(0.0)
    {
    }

    ParallelEpiped m_range;
    Qwt3DSurface* m_surface;
    unsigned int m_umesh;
    unsigned int m_vmesh;
    double m_minu;
    double m_maxu;
    double m_minv;
    double m_maxv;
};

/**
 * @brief Default constructor
 * @details Initializes with no surface, zero mesh dimensions, zero domain,
 *          and unrestricted z range.
 */
Qwt3DGridMapping::Qwt3DGridMapping() : QWT_PIMPL_CONSTRUCT
{
    setMesh(0, 0);
    setDomain(0, 0, 0, 0);
    restrictRange(ParallelEpiped(Triple(-DBL_MAX, -DBL_MAX, -DBL_MAX), Triple(DBL_MAX, DBL_MAX, DBL_MAX)));
}

Qwt3DGridMapping::~Qwt3DGridMapping() = default;

/**
 * @brief Sets the number of mesh columns and rows
 * @param columns Number of columns (u direction)
 * @param rows Number of rows (v direction)
 */
void Qwt3DGridMapping::setMesh(unsigned int columns, unsigned int rows)
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
void Qwt3DGridMapping::setDomain(double minu, double maxu, double minv, double maxv)
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
void Qwt3DGridMapping::restrictRange(ParallelEpiped const& p)
{
    QWT_D(d);
    d->m_range = p;
}

Qwt3DSurface* Qwt3DGridMapping::surface() const
{
    QWT_DC(d);
    return d->m_surface;
}

void Qwt3DGridMapping::setSurface(Qwt3DSurface* surface)
{
    QWT_D(d);
    d->m_surface = surface;
}

ParallelEpiped& Qwt3DGridMapping::range()
{
    QWT_D(d);
    return d->m_range;
}

const ParallelEpiped& Qwt3DGridMapping::range() const
{
    QWT_DC(d);
    return d->m_range;
}

unsigned int Qwt3DGridMapping::meshU() const
{
    QWT_DC(d);
    return d->m_umesh;
}

unsigned int Qwt3DGridMapping::meshV() const
{
    QWT_DC(d);
    return d->m_vmesh;
}

double Qwt3DGridMapping::minU() const
{
    QWT_DC(d);
    return d->m_minu;
}

double Qwt3DGridMapping::maxU() const
{
    QWT_DC(d);
    return d->m_maxu;
}

double Qwt3DGridMapping::minV() const
{
    QWT_DC(d);
    return d->m_minv;
}

double Qwt3DGridMapping::maxV() const
{
    QWT_DC(d);
    return d->m_maxv;
}
