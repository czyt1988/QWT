#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4786)
#endif

#include <cstdlib>  // qsort
#include <algorithm>
#include <cfloat>
#include "qwt3d_types.h"

#include <cmath>


#ifndef QWT3D_NOT_FOR_DOXYGEN

class Qwt3DData::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DData)

public:
    PrivateData(Qwt3DData* q) : q_ptr(q)
    {
    }

    ParallelEpiped m_hull;
};

Qwt3DData::Qwt3DData() : QWT_PIMPL_CONSTRUCT, datatype(POLYGON)
{
}

Qwt3DData::~Qwt3DData() = default;

void Qwt3DData::setHull(ParallelEpiped const& h)
{
    QWT_D(d);
    d->m_hull = h;
}

ParallelEpiped const& Qwt3DData::hull() const
{
    QWT_DC(d);
    return d->m_hull;
}

class Qwt3DGridData::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DGridData)

public:
    PrivateData(Qwt3DGridData* q) : q_ptr(q), m_uperiodic(false), m_vperiodic(false)
    {
    }

    bool m_uperiodic;
    bool m_vperiodic;
};

namespace
{
// convex hull

using coordinate_type = double;

int ccw(coordinate_type** P, int i, int j, int k)
{
    coordinate_type a = P[ i ][ 0 ] - P[ j ][ 0 ], b = P[ i ][ 1 ] - P[ j ][ 1 ], c = P[ k ][ 0 ] - P[ j ][ 0 ],
                    d = P[ k ][ 1 ] - P[ j ][ 1 ];
    return a * d - b * c <= 0; /* true if points i, j, k counterclockwise */
}

#define CMPM(c, A, B)                                                                                                  \
    v = (*static_cast< coordinate_type* const* >(A))[ c ] - (*static_cast< coordinate_type* const* >(B))[ c ];         \
    if (v > 0)                                                                                                         \
        return 1;                                                                                                      \
    if (v < 0)                                                                                                         \
        return -1;

int cmpl(const void* a, const void* b)
{
    double v;
    CMPM(0, a, b);
    CMPM(1, b, a);
    return 0;
}

int cmph(const void* a, const void* b)
{
    return cmpl(b, a);
}

int make_chain(coordinate_type** V, int n, int (*cmp)(const void*, const void*))
{
    int i, j, s = 1;
    coordinate_type* t;

    qsort(V, n, sizeof(coordinate_type*), cmp);
    for (i = 2; i < n; i++) {
        for (j = s; j >= 1 && ccw(V, i, j, j - 1); j--) { }
        s      = j + 1;
        t      = V[ s ];
        V[ s ] = V[ i ];
        V[ i ] = t;
    }
    return s;
}

int _ch2d(coordinate_type** P, int n)
{
    int u = make_chain(P, n, cmpl); /* make lower hull */
    if (!n)
        return 0;
    P[ n ] = P[ 0 ];
    return u + make_chain(P + u, n - u + 1, cmph); /* make upper hull */
}

}  // ns anon

Qwt3DGridData::Qwt3DGridData() : QWT_PIMPL_CONSTRUCT
{
    datatype = GRID;
    setSize(0, 0);
    setPeriodic(false, false);
}

Qwt3DGridData::Qwt3DGridData(unsigned int columns, unsigned int rows) : QWT_PIMPL_CONSTRUCT
{
    datatype = GRID;
    setSize(columns, rows);
    setPeriodic(false, false);
}

Qwt3DGridData::~Qwt3DGridData()
{
    clear();
}

int Qwt3DGridData::columns() const
{
    return static_cast< int >(vertices.size());
}

int Qwt3DGridData::rows() const
{
    return (empty()) ? 0 : static_cast< int >(vertices[ 0 ].size());
}

bool Qwt3DGridData::empty() const
{
    return vertices.empty();
}

void Qwt3DGridData::setPeriodic(bool u, bool v)
{
    QWT_D(d);
    d->m_uperiodic = u;
    d->m_vperiodic = v;
}

bool Qwt3DGridData::uperiodic() const
{
    QWT_DC(d);
    return d->m_uperiodic;
}

bool Qwt3DGridData::vperiodic() const
{
    QWT_DC(d);
    return d->m_vperiodic;
}

void Qwt3DGridData::clear()
{
    setHull(ParallelEpiped());
    {
        for (unsigned i = 0; i != vertices.size(); ++i) {
            for (unsigned j = 0; j != vertices[ i ].size(); ++j) {
                delete[] vertices[ i ][ j ];
            }
            vertices[ i ].clear();
        }
    }

    vertices.clear();

    {
        for (unsigned i = 0; i != normals.size(); ++i) {
            for (unsigned j = 0; j != normals[ i ].size(); ++j) {
                delete[] normals[ i ][ j ];
            }
            normals[ i ].clear();
        }
    }

    normals.clear();
}

void Qwt3DGridData::setSize(unsigned int columns, unsigned int rows)
{
    this->clear();
    vertices = std::vector< DataRow >(columns);
    {
        for (unsigned int i = 0; i != vertices.size(); ++i) {
            vertices[ i ] = DataRow(rows);
            for (unsigned int j = 0; j != vertices[ i ].size(); ++j) {
                vertices[ i ][ j ] = new GLdouble[ 3 ];
            }
        }
    }
    normals = std::vector< DataRow >(columns);
    {
        for (unsigned int i = 0; i != normals.size(); ++i) {
            normals[ i ] = DataRow(rows);
            for (unsigned int j = 0; j != normals[ i ].size(); ++j) {
                normals[ i ][ j ] = new GLdouble[ 3 ];
            }
        }
    }
}

Triple const& Qwt3DCellData::operator()(unsigned cellnumber, unsigned vertexnumber)
{
    return nodes[ cells[ cellnumber ][ vertexnumber ] ];
}

void Qwt3DCellData::clear()
{
    setHull(ParallelEpiped());
    cells.clear();
    nodes.clear();
    normals.clear();
}

/**
 * @brief Converts GL double color components to Qt QColor
 * @param r Red component (0.0-1.0)
 * @param g Green component (0.0-1.0)
 * @param b Blue component (0.0-1.0)
 * @return QColor with components scaled to 0-255 range
 */
QColor GL2Qt(GLdouble r, GLdouble g, GLdouble b)
{
    return QColor(static_cast< int >(std::round(r * 255)),
                  static_cast< int >(std::round(g * 255)),
                  static_cast< int >(std::round(b * 255)));
}

/**
 * @brief Converts Qt QColor to GL RGBA structure
 * @param col Qt QColor to convert
 * @return RGBA structure with components scaled to 0.0-1.0 range
 */
RGBA Qt2GL(QColor col)
{
    QRgb qrgb = col.rgb();
    RGBA rgba;
    rgba.r = qRed(qrgb) / 255.0;
    rgba.g = qGreen(qrgb) / 255.0;
    rgba.b = qBlue(qrgb) / 255.0;
    rgba.a = qAlpha(qrgb) / 255.0;
    return rgba;
}

/**
 * @brief Calculates 2D convex hull indices from a set of points
 * @param[out] idx Output vector of indices into src forming the convex hull
 * @param src Source vector of Tuple points
 */
void convexhull2d(std::vector< unsigned >& idx, const std::vector< Tuple >& src)
{
    idx.clear();
    if (src.empty())
        return;
    if (src.size() == 1) {
        idx.push_back(0);
        return;
    }
    coordinate_type** points = new coordinate_type*[ src.size() + 1 ];
    coordinate_type* P       = new coordinate_type[ src.size() * 2 ];

    size_t i;
    for (i = 0; i < src.size(); ++i) {
        points[ i ]      = &P[ 2 * i ];
        points[ i ][ 0 ] = src[ i ].x;
        points[ i ][ 1 ] = src[ i ].y;
    }

    coordinate_type* start = points[ 0 ];
    size_t m               = _ch2d(points, static_cast< int >(src.size()));
    idx.resize(m);

    for (i = 0; i < m; ++i) {
        idx[ i ] = (points[ i ] - start) / 2;
    }
    delete[] points;
    delete[] P;
}

/**
 * @brief Returns total size of tesselation
 * @param t Cell field to measure
 * @return Sum of all cell sizes in the field
 */
unsigned tesselationSize(CellField const& t)
{
    size_t ret = 0;

    for (size_t i = 0; i < t.size(); ++i)
        ret += t[ i ].size();

    return static_cast< unsigned >(ret);
}

#endif  // QWT3D_NOT_FOR_DOXYGEN
