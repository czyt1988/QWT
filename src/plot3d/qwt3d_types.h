#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4786)
#endif

#ifndef QWT3D_TYPES_H
#define QWT3D_TYPES_H

#ifdef _DEBUG
#include <fstream>
#endif

#include <string>

#include "qwt3d_global.h"

#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif

#include "qwt3d_portability.h"
#include "qwt3d_helper.h"
#include <QColor>



const double Qwt3D_PI = 3.14159265358979323846264338328;

/**
 * @brief Plotting style enumeration
 */
enum PLOTSTYLE
{
    NOPLOT,      // No visible data
    WIREFRAME,   // Wireframe style
    HIDDENLINE,  // Hidden Line style
    FILLED,      // Color filled polygons w/o edges
    FILLEDMESH,  // Color filled polygons w/ separately colored edges
    QWT3D_POINTS,      // User defined style (used by Enrichments)
    USER         // User defined style (used by Enrichments)
};

/**
 * @brief Shading style enumeration
 */
enum SHADINGSTYLE
{
    FLAT,    // Flat shading (OpenGL)
    GOURAUD  // Gouraud Shading (OpenGL)
};

/**
 * @brief Style of coordinate system enumeration
 */
enum COORDSTYLE
{
    NOCOORD,  // Coordinate system is not visible
    BOX,      // Boxed
    FRAME     // Frame - 3 visible axes
};

/**
 * @brief Different types of axis scales
 */
enum SCALETYPE
{
    LINEARSCALE,  // Linear scaling
    LOG10SCALE,   // Logarithmic scaling (base 10)
    USERSCALE     // User-defined (for extensions)
};

/**
 * @brief Plotting style for floor data (projections)
 */
enum FLOORSTYLE
{
    NOFLOOR,   // Empty floor
    FLOORISO,  // Isoline projections visible
    FLOORDATA  // Projected polygons visible
};

/**
 * @brief Mesh type enumeration
 */
enum DATATYPE
{
    GRID,    // Rectangular grid
    POLYGON  // Convex polygon
};

/**
 * @brief The 12 axes
 * @image html axes.png
 */
enum AXIS
{
    X1 = 0,   // 1st x-axis
    X2 = 3,   // 2nd x-axis
    X3 = 4,   // 3rd x-axis
    X4 = 5,   // 4th x-axis
    Y1 = 1,   // 1st y-axis
    Y2 = 8,   // 2nd y-axis
    Y3 = 7,   // 3rd y-axis
    Y4 = 6,   // 4th y-axis
    Z1 = 2,   // 1st z-axis
    Z2 = 9,   // 2nd z-axis
    Z3 = 11,  // 3rd z-axis
    Z4 = 10   // 4th z-axis
};

/**
 * @brief The 6 sides of a plot box
 */
enum SIDE
{
    NOSIDEGRID = 0,
    LEFT       = 1 << 0,
    RIGHT      = 1 << 1,
    CEIL       = 1 << 2,
    FLOOR      = 1 << 3,
    FRONT      = 1 << 4,
    BACK       = 1 << 5
};

/**
 * @brief Interior grid line directions
 * @details Controls which interior grid lines are drawn through the box volume.
 *          Interior lines connect grid intersection points on opposite faces.
 */
enum INTERIOR_DIRECTION
{
    NO_INTERIOR = 0,
    X_INNER     = 1 << 0,  ///< Lines in X direction through the interior (left to right)
    Y_INNER     = 1 << 1,  ///< Lines in Y direction through the interior (front to back)
    Z_INNER     = 1 << 2   ///< Lines in Z direction through the interior (floor to ceil)
};

/**
 * @brief Tick position preference for auto-decorated axes
 * @details Controls whether ticks appear on the visually lower or upper axis
 *          when auto-decoration is enabled. In screen coordinates, y increases
 *          downward, so TICK_BOTTOM selects the axis with the largest screen y.
 */
enum TICKPOSITION
{
    TICK_BOTTOM,  // Ticks on the visually lower axis (default)
    TICK_TOP      // Ticks on the visually upper axis
};

/**
 * @brief Aspect ratio mode for the 3D coordinate box
 * @details Controls whether each axis is independently scaled to fill the
 *          viewport (AUTOFILL) or whether the original data proportions are
 *          preserved (DATARATIO).
 */
enum ASPECTRATIOMODE
{
    AUTOFILL,   // Each axis independently scaled to fill the view (default)
    DATARATIO   // Preserve data proportions (equal aspect ratio)
};

/**
 * @brief Possible anchor points for drawing operations
 */
enum ANCHOR
{
    BottomLeft,
    BottomRight,
    BottomCenter,
    TopLeft,
    TopRight,
    TopCenter,
    CenterLeft,
    CenterRight,
    Center
};

/**
 * @brief Tuple [x,y]
 * @details A 2D point/vector represented by x and y coordinates.
 */
struct QWT3D_EXPORT Tuple
{
    // Calls Tuple(0,0)
    Tuple() : x(0), y(0)
    {
    }
    // Initialize Tuple with x and y
    Tuple(double X, double Y) : x(X), y(Y)
    {
    }
    // Tuple coordinates
    double x, y;
};

/**
 * @brief Triple [x,y,z]
 * @details Consider Triples also as vectors in R^3. Provides basic vector operations
 *          including addition, subtraction, scaling, normalization, and length calculation.
 */
struct QWT3D_EXPORT Triple
{
    // Initialize Triple with x,y and z
    explicit Triple(double xv = 0, double yv = 0, double zv = 0) : x(xv), y(yv), z(zv)
    {
    }

#ifndef QWT3D_NOT_FOR_DOXYGEN
#ifdef Q_OS_IRIX
    Triple(const Triple& val)
    {
        if (&val == this)
            return;
        x = val.x;
        y = val.y;
        z = val.z;
    }
    const Triple& operator=(const Triple& val)
    {
        if (&val == this)
            return *this;
        x = val.x;
        y = val.y;
        z = val.z;
        return *this;
    }
#endif
#endif  // QWT3D_NOT_FOR_DOXYGEN

    // Triple coordinates
    double x, y, z;

    Triple& operator+=(Triple t)
    {
        x += t.x;
        y += t.y;
        z += t.z;

        return *this;
    }

    Triple& operator-=(Triple t)
    {
        x -= t.x;
        y -= t.y;
        z -= t.z;

        return *this;
    }
    Triple& operator*=(double d)
    {
        x *= d;
        y *= d;
        z *= d;

        return *this;
    }
    Triple& operator/=(double d)
    {
        x /= d;
        y /= d;
        z /= d;

        return *this;
    }
    Triple& operator*=(Triple t)  // scale
    {
        x *= t.x;
        y *= t.y;
        z *= t.z;

        return *this;
    }

    bool operator!=(Triple t) const
    {
        return !isPracticallyZero(x, t.x) || !isPracticallyZero(y, t.y) || !isPracticallyZero(z, t.z);
    }

    bool operator==(Triple t) const
    {
        return !operator!=(t);
    }

    double length() const
    {
        double l2 = x * x + y * y + z * z;
        return (isPracticallyZero(l2)) ? 0 : sqrt(l2);
    }

    void normalize()
    {
        double l = length();
        if (l)
            *this /= l;
    }
};

inline const Triple operator+(const Triple& t, const Triple& t2)
{
    return Triple(t) += t2;
}
inline const Triple operator-(const Triple& t, const Triple& t2)
{
    return Triple(t) -= t2;
}
inline const Triple operator*(double d, const Triple& t)
{
    return Triple(t) *= d;
}
inline const Triple operator*(const Triple& t, double d)
{
    return Triple(t) *= d;
}
inline const Triple operator/(double d, const Triple& t)
{
    return Triple(t) /= d;
}
inline const Triple operator/(const Triple& t, double d)
{
    return Triple(t) /= d;
}
inline const Triple operator*(const Triple& t, const Triple& t2)
{
    return Triple(t) *= t2;
}

/**
 * @brief Parallelepiped spanned by 2 Triples
 * @details Please use normalized Parallelepipeds:
 *          minVertex.x <= maxVertex.x, minVertex.y <= maxVertex.y,
 *          minVertex.z <= maxVertex.z
 */
struct QWT3D_EXPORT ParallelEpiped
{
    // Construct non-initialized Parallelepiped
    ParallelEpiped()
    {
    }

    // Construct initialized Parallelepiped: minv -> minVertex, maxv -> maxVertex
    ParallelEpiped(Triple minv, Triple maxv) : minVertex(minv), maxVertex(maxv)
    {
    }

    Triple minVertex;
    Triple maxVertex;
};

/**
 * @brief Free vector
 * @details FreeVectors represent objects like normal vectors and other vector fields inside R^3.
 */
struct QWT3D_EXPORT FreeVector
{
    FreeVector()
    {
    }

    // Construct initialized vector: b -> base, e -> top
    FreeVector(Triple b, Triple t) : base(b), top(t)
    {
    }

    Triple base;
    Triple top;
};

/**
 * @brief A free vector field in R^3
 */
using FreeVectorField = std::vector< FreeVector >;

/**
 * @brief A point field in R^3
 */
using TripleField = std::vector< Triple >;

/**
 * @brief Holds indices in a TripleField interpreted as counterclockwise node numbering for a convex polygon
 */
using Cell = std::vector< unsigned >;

/**
 * @brief Vector of convex polygons. You need a TripleField as base for the node data
 */
using CellField = std::vector< Cell >;

// Returns the sum over the sizes of the single cells
unsigned tesselationSize(CellField const& t);

/**
 * @brief Red-Green-Blue-Alpha value
 */
struct QWT3D_EXPORT RGBA
{
    RGBA() : r(0), g(0), b(0), a(1)
    {
    }
    RGBA(double rr, double gg, double bb, double aa = 1) : r(rr), g(gg), b(bb), a(aa)
    {
    }
    double r, g, b, a;
};

/**
 * @brief A Color field
 */
using ColorVector = std::vector< RGBA >;

#ifndef QWT3D_NOT_FOR_DOXYGEN

// RGB -> QColor
QWT3D_EXPORT QColor GL2Qt(double r, double g, double b);
// QColor -> RGBA
QWT3D_EXPORT RGBA Qt2GL(QColor col);

using Vertex     = double*;
using DataRow    = std::vector< Vertex >;
using DataMatrix = std::vector< DataRow >;

/**
 * @brief Abstract base class for plot data
 * @details Qwt3DData provides the interface for different data representations
 *          used by 3D plot widgets.
 */
class Qwt3DData
{
    QWT_DECLARE_PRIVATE(Qwt3DData)

public:
    DATATYPE datatype;
    Qwt3DData();
    virtual ~Qwt3DData();
    // Destroy content
    virtual void clear() = 0;
    // No data
    virtual bool empty() const = 0;
    void setHull(ParallelEpiped const& h);
    ParallelEpiped const& hull() const;
};

/**
 * @brief Implements a matrix of z-Values with limit access functions
 * @details Qwt3DGridData represents data on a rectangular grid topology,
 *          providing z-values organized in a matrix with associated normals.
 */
class Qwt3DGridData : public Qwt3DData
{
    QWT_DECLARE_PRIVATE(Qwt3DGridData)

public:
    Qwt3DGridData();
    // See setSize()
    Qwt3DGridData(unsigned int columns, unsigned int rows);
    ~Qwt3DGridData() override;

    int columns() const;
    int rows() const;

    // Destroy content
    void clear();
    bool empty() const;
    // Destroys content and set new size, elements are uninitialized
    void setSize(unsigned int columns, unsigned int rows);

    // Mesh vertices
    DataMatrix vertices;
    // Mesh normals
    DataMatrix normals;
    void setPeriodic(bool u, bool v);
    bool uperiodic() const;
    bool vperiodic() const;
};

/**
 * @brief Implements a graph-like cell structure with limit access functions
 * @details Qwt3DCellData represents data as a collection of convex polygon cells
 *          with associated node coordinates and normals.
 */
class Qwt3DCellData : public Qwt3DData
{
public:
    Qwt3DCellData()
    {
        datatype = POLYGON;
    }
    ~Qwt3DCellData()
    {
        clear();
    }

    // Destroy content
    void clear();
    bool empty() const
    {
        return cells.empty();
    }

    Triple const& operator()(unsigned cellnumber, unsigned vertexnumber);

    // Polygon/cell mesh
    CellField cells;
    TripleField nodes;
    // Mesh normals
    TripleField normals;
};

inline Triple normalizedcross(Triple const& u, Triple const& v)
{
    Triple n;

    /* compute the cross product (u x v for right-handed [ccw]) */
    n.x = u.y * v.z - u.z * v.y;
    n.y = u.z * v.x - u.x * v.z;
    n.z = u.x * v.y - u.y * v.x;

    /* normalize */
    double l = n.length();
    if (l) {
        n /= l;
    } else {
        n = Triple(0, 0, 0);
    }

    return n;
}

inline double dotProduct(Triple const& u, Triple const& v)
{
    return u.x * v.x + u.y * v.y + u.z * v.z;
}

void convexhull2d(std::vector< unsigned >& idx, const std::vector< Tuple >& src);

#endif  // QWT3D_NOT_FOR_DOXYGEN

/**
 * @brief Result of evaluating a Qwt3DFunction over its grid
 * @details Contains the z-value matrix and x/y domain bounds produced by
 *          Qwt3DFunction::create(). Feed this to Qwt3DSurface::loadFromData().
 */
struct QWT3D_EXPORT Qwt3DFunctionData
{
    /// z[i][j] = z-value at column i, row j
    std::vector<std::vector<double>> z;
    unsigned int columns = 0;
    unsigned int rows = 0;
    double minx = 0.0;
    double maxx = 0.0;
    double miny = 0.0;
    double maxy = 0.0;
};

/**
 * @brief Result of evaluating a Qwt3DParametricSurface over its grid
 * @details Contains the xyz triple matrix and periodicity flags produced by
 *          Qwt3DParametricSurface::create(). Feed this to Qwt3DSurface::loadFromData().
 */
struct QWT3D_EXPORT Qwt3DParametricData
{
    /// vertices[i][j] = xyz position at column i, row j
    std::vector<std::vector<Triple>> vertices;
    unsigned int columns = 0;
    unsigned int rows = 0;
    bool uperiodic = false;
    bool vperiodic = false;
};

/**
 * @brief Runtime type identification values for 3D plot items
 * @details Mirrors the 2D QwtPlotItem::RttiValues pattern. Concrete subclass
 *          values start at 1001 to avoid collision with 2D concrete subclass
 *          values (which range from 0 to ~19, with Rtti_PlotUserItem = 1000 as
 *          the user-defined boundary). The base class value 0 mirrors 2D's
 *          Rtti_PlotItem = 0, which is safe because 2D and 3D items are never
 *          type-checked through the same pointer. Use rtti() to query an item's
 *          type at runtime for safe downcasting, setting panel factory routing,
 *          and serialization dispatch.
 */
enum Rtti3DValues
{
    /// Unspecific value for base Qwt3DPlotItem, can be used when type doesn't matter
    Rtti_Plot3DItem = 0,

    /// For Qwt3DSurface
    Rtti_Plot3DSurface = 1001,

    /// For Qwt3DBar
    Rtti_Plot3DBar = 1002,

    /// For Qwt3DLine
    Rtti_Plot3DLine = 1003
};


#endif