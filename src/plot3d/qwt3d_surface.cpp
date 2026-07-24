#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4305)
#pragma warning(disable : 4786)
#endif

#include "qwt3d_surface_p.h"
#include "qwt3d_plot.h"
#include "qwt3d_color.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QMatrix3x3>

#include <cfloat>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Anonymous namespace: data loading helpers (ported from old gridplot/meshplot)
// ---------------------------------------------------------------------------

namespace
{

/// Reads Triple grid data into Qwt3DGridData and computes hull
void readInGridTriples(Qwt3DGridData& gdata, Triple** data,
                       unsigned int columns, unsigned int rows)
{
    gdata.setSize(columns, rows);

    ParallelEpiped range(Triple(DBL_MAX, DBL_MAX, DBL_MAX),
                         Triple(-DBL_MAX, -DBL_MAX, -DBL_MAX));

    for (unsigned i = 0; i != columns; ++i) {
        for (unsigned j = 0; j != rows; ++j) {
            gdata.vertices[i][j][0] = data[i][j].x;
            gdata.vertices[i][j][1] = data[i][j].y;
            gdata.vertices[i][j][2] = data[i][j].z;

            if (data[i][j].x > range.maxVertex.x)
                range.maxVertex.x = data[i][j].x;
            if (data[i][j].y > range.maxVertex.y)
                range.maxVertex.y = data[i][j].y;
            if (data[i][j].z > range.maxVertex.z)
                range.maxVertex.z = data[i][j].z;
            if (data[i][j].x < range.minVertex.x)
                range.minVertex.x = data[i][j].x;
            if (data[i][j].y < range.minVertex.y)
                range.minVertex.y = data[i][j].y;
            if (data[i][j].z < range.minVertex.z)
                range.minVertex.z = data[i][j].z;
        }
    }
    gdata.setHull(range);
}

/// Reads z-value matrix into Qwt3DGridData with explicit x/y domain
void readInGridZ(Qwt3DGridData& gdata, double** data,
                 unsigned int columns, unsigned int rows,
                 double minx, double maxx, double miny, double maxy)
{
    gdata.setPeriodic(false, false);
    gdata.setSize(columns, rows);

    double dx = (maxx - minx) / (gdata.columns() - 1);
    double dy = (maxy - miny) / (gdata.rows() - 1);

    double tmin = DBL_MAX;
    double tmax = -DBL_MAX;

    for (unsigned i = 0; i != columns; ++i) {
        for (unsigned j = 0; j != rows; ++j) {
            gdata.vertices[i][j][0] = minx + i * dx;
            gdata.vertices[i][j][1] = miny + j * dy;
            gdata.vertices[i][j][2] = data[i][j];

            if (data[i][j] > tmax)
                tmax = data[i][j];
            if (data[i][j] < tmin)
                tmin = data[i][j];
        }
    }

    ParallelEpiped hull(Triple(gdata.vertices[0][0][0],
                               gdata.vertices[0][0][1], tmin),
                        Triple(gdata.vertices[gdata.columns() - 1][gdata.rows() - 1][0],
                               gdata.vertices[gdata.columns() - 1][gdata.rows() - 1][1],
                               tmax));
    gdata.setHull(hull);
}

/// Calculates per-vertex normals for grid data using 4-neighbor cross products
void calcNormalsGrid(Qwt3DGridData& gdata)
{
    unsigned int rows = gdata.rows();
    unsigned int columns = gdata.columns();

    Triple u, v, n;

    for (unsigned i = 0; i != columns; ++i) {
        for (unsigned j = 0; j != rows; ++j) {
            n = Triple(0, 0, 0);

            if (i < columns - 1 && j < rows - 1) {
                u = Triple(gdata.vertices[i + 1][j][0] - gdata.vertices[i][j][0],
                           gdata.vertices[i + 1][j][1] - gdata.vertices[i][j][1],
                           gdata.vertices[i + 1][j][2] - gdata.vertices[i][j][2]);
                v = Triple(gdata.vertices[i][j + 1][0] - gdata.vertices[i][j][0],
                           gdata.vertices[i][j + 1][1] - gdata.vertices[i][j][1],
                           gdata.vertices[i][j + 1][2] - gdata.vertices[i][j][2]);
                n += normalizedcross(u, v);
            }

            if (i > 0 && j < rows - 1) {
                u = Triple(gdata.vertices[i][j + 1][0] - gdata.vertices[i][j][0],
                           gdata.vertices[i][j + 1][1] - gdata.vertices[i][j][1],
                           gdata.vertices[i][j + 1][2] - gdata.vertices[i][j][2]);
                v = Triple(gdata.vertices[i - 1][j][0] - gdata.vertices[i][j][0],
                           gdata.vertices[i - 1][j][1] - gdata.vertices[i][j][1],
                           gdata.vertices[i - 1][j][2] - gdata.vertices[i][j][2]);
                n += normalizedcross(u, v);
            }

            if (i > 0 && j > 0) {
                u = Triple(gdata.vertices[i - 1][j][0] - gdata.vertices[i][j][0],
                           gdata.vertices[i - 1][j][1] - gdata.vertices[i][j][1],
                           gdata.vertices[i - 1][j][2] - gdata.vertices[i][j][2]);
                v = Triple(gdata.vertices[i][j - 1][0] - gdata.vertices[i][j][0],
                           gdata.vertices[i][j - 1][1] - gdata.vertices[i][j][1],
                           gdata.vertices[i][j - 1][2] - gdata.vertices[i][j][2]);
                n += normalizedcross(u, v);
            }

            if (i < columns - 1 && j > 0) {
                u = Triple(gdata.vertices[i][j - 1][0] - gdata.vertices[i][j][0],
                           gdata.vertices[i][j - 1][1] - gdata.vertices[i][j][1],
                           gdata.vertices[i][j - 1][2] - gdata.vertices[i][j][2]);
                v = Triple(gdata.vertices[i + 1][j][0] - gdata.vertices[i][j][0],
                           gdata.vertices[i + 1][j][1] - gdata.vertices[i][j][1],
                           gdata.vertices[i + 1][j][2] - gdata.vertices[i][j][2]);
                n += normalizedcross(u, v);
            }
            n.normalize();

            gdata.normals[i][j][0] = n.x;
            gdata.normals[i][j][1] = n.y;
            gdata.normals[i][j][2] = n.z;
        }
    }
}

/// Sews (averages) normals at periodic boundaries
void sewPeriodicGrid(Qwt3DGridData& gdata)
{
    Triple n;

    unsigned int columns = gdata.columns();
    unsigned int rows = gdata.rows();

    if (gdata.uperiodic()) {
        for (unsigned i = 0; i != columns; ++i) {
            n = Triple(gdata.normals[i][0][0] + gdata.normals[i][rows - 1][0],
                       gdata.normals[i][0][1] + gdata.normals[i][rows - 1][1],
                       gdata.normals[i][0][2] + gdata.normals[i][rows - 1][2]);
            n.normalize();
            gdata.normals[i][0][0] = gdata.normals[i][rows - 1][0] = n.x;
            gdata.normals[i][0][1] = gdata.normals[i][rows - 1][1] = n.y;
            gdata.normals[i][0][2] = gdata.normals[i][rows - 1][2] = n.z;
        }
    }
    if (gdata.vperiodic()) {
        for (unsigned j = 0; j != rows; ++j) {
            n = Triple(gdata.normals[0][j][0] + gdata.normals[columns - 1][j][0],
                       gdata.normals[0][j][1] + gdata.normals[columns - 1][j][1],
                       gdata.normals[0][j][2] + gdata.normals[columns - 1][j][2]);
            n.normalize();
            gdata.normals[0][j][0] = gdata.normals[columns - 1][j][0] = n.x;
            gdata.normals[0][j][1] = gdata.normals[columns - 1][j][1] = n.y;
            gdata.normals[0][j][2] = gdata.normals[columns - 1][j][2] = n.z;
        }
    }
}

/// Calculates per-vertex normals for cell data
void calcNormalsCell(Qwt3DCellData& cdata)
{
    cdata.normals = TripleField(cdata.nodes.size());

    Triple n, u, v;
    for (unsigned i = 0; i < cdata.cells.size(); ++i) {
        if (cdata.cells[i].size() < 3)
            n = Triple(0, 0, 0);
        else {
            for (size_t j = 0; j < cdata.cells[i].size(); ++j) {
                size_t jj = (j + 1) % cdata.cells[i].size();
                size_t pjj = (j) ? j - 1 : cdata.cells[i].size() - 1;
                u = cdata.nodes[cdata.cells[i][jj]] - cdata.nodes[cdata.cells[i][j]];
                v = cdata.nodes[cdata.cells[i][pjj]] - cdata.nodes[cdata.cells[i][j]];
                n = normalizedcross(u, v);
                cdata.normals[cdata.cells[i][j]] += n;
            }
        }
    }
    for (unsigned i = 0; i != cdata.normals.size(); ++i) {
        cdata.normals[i].normalize();
    }

    // Compute hull
    ParallelEpiped hull(Triple(DBL_MAX, DBL_MAX, DBL_MAX),
                        Triple(-DBL_MAX, -DBL_MAX, -DBL_MAX));
    for (unsigned i = 0; i != cdata.nodes.size(); ++i) {
        if (cdata.nodes[i].x < hull.minVertex.x)
            hull.minVertex.x = cdata.nodes[i].x;
        if (cdata.nodes[i].y < hull.minVertex.y)
            hull.minVertex.y = cdata.nodes[i].y;
        if (cdata.nodes[i].z < hull.minVertex.z)
            hull.minVertex.z = cdata.nodes[i].z;
        if (cdata.nodes[i].x > hull.maxVertex.x)
            hull.maxVertex.x = cdata.nodes[i].x;
        if (cdata.nodes[i].y > hull.maxVertex.y)
            hull.maxVertex.y = cdata.nodes[i].y;
        if (cdata.nodes[i].z > hull.maxVertex.z)
            hull.maxVertex.z = cdata.nodes[i].z;
    }
    cdata.setHull(hull);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

/**
 * @brief Constructs an empty Qwt3DSurface item
 * @details Initializes data storage (empty grid and cell data) and GL resource
 *          placeholders. VBO/VAO/shader are created lazily in draw().
 */
Qwt3DSurface::Qwt3DSurface()
    : Qwt3DPlotItem()
    , QWT_PIMPL_CONSTRUCT
{
}

/**
 * @brief Destructor
 * @details GL resources (VBO, EBO, VAO) are cleaned up in PrivateData destructor.
 *          Data pointers and color functor are also deleted there.
 */
Qwt3DSurface::~Qwt3DSurface() = default;

// ---------------------------------------------------------------------------
// Data loading
// ---------------------------------------------------------------------------

/**
 * @brief Load grid data from Triple array
 * @param data 2D array of Triple values [columns][rows]
 * @param columns Number of columns in the grid
 * @param rows Number of rows in the grid
 * @param uperiodic Whether the u-direction is periodic
 * @param vperiodic Whether the v-direction is periodic
 * @details Reads vertex positions, computes normals, and marks the VBO for rebuild.
 */
void Qwt3DSurface::loadFromData(Triple** data, unsigned int columns, unsigned int rows,
                                bool uperiodic, bool vperiodic)
{
    QWT_D(d);

    d->m_actualDataC->clear();
    readInGridTriples(*d->m_actualDataG, data, columns, rows);
    calcNormalsGrid(*d->m_actualDataG);
    d->m_actualDataG->setPeriodic(uperiodic, vperiodic);
    sewPeriodicGrid(*d->m_actualDataG);

    d->m_vboDirty = true;
    itemChanged();
}

/**
 * @brief Load grid data from z-value matrix
 * @param data 2D array of z-values [columns][rows]
 * @param columns Number of columns in the grid
 * @param rows Number of rows in the grid
 * @param minx Minimum x coordinate
 * @param maxx Maximum x coordinate
 * @param miny Minimum y coordinate
 * @param maxy Maximum y coordinate
 * @details Generates x/y coordinates from the domain, reads z-values,
 *          computes normals, and marks the VBO for rebuild.
 */
void Qwt3DSurface::loadFromData(double** data, unsigned int columns, unsigned int rows,
                                double minx, double maxx, double miny, double maxy)
{
    QWT_D(d);

    d->m_actualDataC->clear();
    d->m_actualDataG->setPeriodic(false, false);
    d->m_actualDataG->setSize(columns, rows);
    readInGridZ(*d->m_actualDataG, data, columns, rows, minx, maxx, miny, maxy);
    calcNormalsGrid(*d->m_actualDataG);

    d->m_vboDirty = true;
    itemChanged();
}

/**
 * @brief Load cell (polygon) data from node coordinates and cell indices
 * @param nodes Vector of 3D node coordinates
 * @param poly Vector of cells (each cell is a vector of node indices)
 * @details Stores node positions and cell topology, computes per-vertex normals
 *          and bounding hull, and marks the VBO for rebuild.
 */
void Qwt3DSurface::loadFromData(TripleField const& nodes, CellField const& poly)
{
    QWT_D(d);

    d->m_actualDataG->clear();
    d->m_actualDataC->nodes = nodes;
    d->m_actualDataC->cells = poly;
    calcNormalsCell(*d->m_actualDataC);

    d->m_vboDirty = true;
    itemChanged();
}

// ---------------------------------------------------------------------------
// Resolution, floor style, normals
// ---------------------------------------------------------------------------

/**
 * @brief Returns the current data resolution
 * @return Resolution value (1 = full data, higher = coarser)
 */
int Qwt3DSurface::resolution() const
{
    QWT_DC(d);
    return d->m_resolution;
}

/**
 * @brief Sets the data resolution
 * @param res Resolution value (1 = full data, higher = coarser)
 * @details Resolution controls index buffer stepping: higher values skip
 *          vertices in the index buffer, reducing rendered detail without
 *          rebuilding the VBO. A value of 1 renders all data.
 */
void Qwt3DSurface::setResolution(int res)
{
    QWT_D(d);
    if (res < 1 || d->m_resolution == res)
        return;

    d->m_resolution = res;
    d->m_vboDirty = true;
    itemChanged();
}

/**
 * @brief Returns the floor projection style
 */
FLOORSTYLE Qwt3DSurface::floorStyle() const
{
    QWT_DC(d);
    return d->m_floorStyle;
}

/**
 * @brief Sets the floor projection style
 * @param style Floor projection style
 * @details TODO: Floor projection rendering will be implemented as a
 *          separate Qwt3DFloorProjection item in a future plan.
 */
void Qwt3DSurface::setFloorStyle(FLOORSTYLE style)
{
    QWT_D(d);
    d->m_floorStyle = style;
    itemChanged();
}

/**
 * @brief Returns true if normal vectors are drawn
 */
bool Qwt3DSurface::normals() const
{
    QWT_DC(d);
    return d->m_dataNormals;
}

/**
 * @brief Show or hide normal vectors at each vertex
 */
void Qwt3DSurface::showNormals(bool b)
{
    QWT_D(d);
    d->m_dataNormals = b;
    itemChanged();
}

/**
 * @brief Returns the relative length of drawn normals (0..1)
 */
double Qwt3DSurface::normalLength() const
{
    QWT_DC(d);
    return d->m_normalLength;
}

/**
 * @brief Sets the relative length of drawn normals
 * @param val Relative length (0..1, relative to hull diagonal)
 */
void Qwt3DSurface::setNormalLength(double val)
{
    QWT_D(d);
    if (val < 0 || val > 1)
        return;
    d->m_normalLength = val;
    itemChanged();
}

/**
 * @brief Returns the quality of normal arrow rendering
 */
int Qwt3DSurface::normalQuality() const
{
    QWT_DC(d);
    return d->m_normalQuality;
}

/**
 * @brief Sets the quality of normal arrow rendering
 * @param val Quality value (minimum 3)
 */
void Qwt3DSurface::setNormalQuality(int val)
{
    QWT_D(d);
    if (val < 3)
        return;
    d->m_normalQuality = val;
    itemChanged();
}

/**
 * @brief Returns the number of mesh cells in the original data
 * @return For grid data: (columns, rows). For cell data: (cells, 0).
 */
std::pair<int, int> Qwt3DSurface::facets() const
{
    QWT_DC(d);
    if (d->m_actualDataG && !d->m_actualDataG->empty())
        return std::pair<int, int>(d->m_actualDataG->columns(),
                                   d->m_actualDataG->rows());
    if (d->m_actualDataC && !d->m_actualDataC->empty())
        return std::pair<int, int>(
            static_cast<int>(d->m_actualDataC->cells.size()), 0);
    return std::pair<int, int>(0, 0);
}

/**
 * @brief Recalculates surface normals from current data
 * @details For grid data, recalculates normals using 4-neighbor cross products.
 *          For cell data, recalculates normals from cell topology.
 *          Marks the VBO for rebuild.
 */
void Qwt3DSurface::updateNormals()
{
    QWT_D(d);
    if (d->m_actualDataG && !d->m_actualDataG->empty()) {
        calcNormalsGrid(*d->m_actualDataG);
        if (d->m_actualDataG->uperiodic() || d->m_actualDataG->vperiodic())
            sewPeriodicGrid(*d->m_actualDataG);
    } else if (d->m_actualDataC && !d->m_actualDataC->empty()) {
        calcNormalsCell(*d->m_actualDataC);
    }
    d->m_vboDirty = true;
    itemChanged();
}

// ---------------------------------------------------------------------------
// Style and color
// ---------------------------------------------------------------------------

/**
 * @brief Returns the current plotting style
 */
PLOTSTYLE Qwt3DSurface::plotStyle() const
{
    QWT_DC(d);
    return d->m_plotStyle;
}

/**
 * @brief Sets the plotting style
 * @param style One of NOPLOT, WIREFRAME, HIDDENLINE, FILLED, FILLEDMESH, QWT3D_POINTS
 */
void Qwt3DSurface::setPlotStyle(PLOTSTYLE style)
{
    QWT_D(d);
    d->m_plotStyle = style;
    itemChanged();
}

/**
 * @brief Sets the data color functor
 * @param color Pointer to a Qwt3DColor subclass (item takes ownership)
 * @details The color functor is called per-vertex during VBO build to compute
 *          vertex colors. If nullptr, a default Qwt3DStandardColor is created
 *          lazily when the item is attached to a plot.
 */
void Qwt3DSurface::setDataColor(Qwt3DColor* color)
{
    QWT_D(d);
    if (d->m_dataColor)
        d->m_dataColor->destroy();
    d->m_dataColor = color;
    d->m_vboDirty = true;
    itemChanged();
}

/**
 * @brief Returns the data color functor
 */
const Qwt3DColor* Qwt3DSurface::dataColor() const
{
    QWT_DC(d);
    return d->m_dataColor;
}

void Qwt3DSurface::populateLegendColors(ColorVector& colors) const
{
    QWT_DC(d);
    if (d->m_dataColor)
        d->m_dataColor->createVector(colors);
}

/**
 * @brief Returns the mesh line color
 */
RGBA Qwt3DSurface::meshColor() const
{
    QWT_DC(d);
    return d->m_meshColor;
}

/**
 * @brief Sets the mesh line color
 */
void Qwt3DSurface::setMeshColor(RGBA color)
{
    QWT_D(d);
    d->m_meshColor = color;
    itemChanged();
}

/**
 * @brief Returns the mesh line width
 */
double Qwt3DSurface::meshLineWidth() const
{
    QWT_DC(d);
    return d->m_meshLineWidth;
}

/**
 * @brief Sets the mesh line width
 */
void Qwt3DSurface::setMeshLineWidth(double width)
{
    QWT_D(d);
    d->m_meshLineWidth = width;
    itemChanged();
}

/**
 * @brief Returns the number of isolines
 */
int Qwt3DSurface::isolines() const
{
    QWT_DC(d);
    return d->m_isolines;
}

/**
 * @brief Sets the number of isolines
 */
void Qwt3DSurface::setIsolines(int n)
{
    QWT_D(d);
    d->m_isolines = n;
    itemChanged();
}

/**
 * @brief Returns true if smooth mesh lines are enabled
 */
bool Qwt3DSurface::smoothMesh() const
{
    QWT_DC(d);
    return d->m_smoothDataMesh;
}

/**
 * @brief Enables or disables smooth mesh lines
 */
void Qwt3DSurface::setSmoothMesh(bool smooth)
{
    QWT_D(d);
    d->m_smoothDataMesh = smooth;
    itemChanged();
}

/**
 * @brief Returns the polygon offset for filled rendering
 */
double Qwt3DSurface::polygonOffset() const
{
    QWT_DC(d);
    return d->m_polygonOffset;
}

/**
 * @brief Sets the polygon offset for filled rendering
 */
void Qwt3DSurface::setPolygonOffset(double offset)
{
    QWT_D(d);
    d->m_polygonOffset = offset;
    itemChanged();
}

/**
 * @brief Returns the shading style
 */
SHADINGSTYLE Qwt3DSurface::shading() const
{
    QWT_DC(d);
    return d->m_shading;
}

/**
 * @brief Sets the shading style
 * @param style One of GOURAUD, FLAT, or SMOOTH
 * @details The shading style controls per-fragment lighting interpolation.
 *          Currently stored for shader uniform use; full implementation
 *          requires shader-level branching.
 */
void Qwt3DSurface::setShading(SHADINGSTYLE style)
{
    QWT_D(d);
    d->m_shading = style;
    itemChanged();
}

// ---------------------------------------------------------------------------
// Enrichments
// ---------------------------------------------------------------------------

/**
 * @brief Adds an enrichment to the surface
 * @param enrichment The enrichment to add (will be cloned)
 * @return Pointer to the cloned enrichment, or nullptr if type is not VERTEXENRICHMENT
 * @details The enrichment is cloned and assigned to the current plot.
 *          TODO: Enrichment rendering with VBO will be implemented in a future plan.
 */
Qwt3DEnrichment* Qwt3DSurface::addEnrichment(Qwt3DEnrichment const& enrichment)
{
    QWT_D(d);
    if (enrichment.type() != Qwt3DEnrichment::VERTEXENRICHMENT)
        return nullptr;

    Qwt3DEnrichment* en = enrichment.clone();
    if (plot())
        en->assign(*plot());
    d->m_enrichmentList.push_back(en);
    return en;
}

/**
 * @brief Removes an enrichment from the surface
 * @param enrichment Pointer to the enrichment to remove
 * @return true if the enrichment was found and removed
 */
bool Qwt3DSurface::degrade(Qwt3DEnrichment* enrichment)
{
    QWT_D(d);
    auto it = std::find(d->m_enrichmentList.begin(), d->m_enrichmentList.end(),
                        enrichment);
    if (it != d->m_enrichmentList.end()) {
        delete *it;
        d->m_enrichmentList.erase(it);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Protected data access
// ---------------------------------------------------------------------------

/**
 * @brief Returns the grid data pointer
 * @return Pointer to Qwt3DGridData, or nullptr if current data is cell-based
 */
Qwt3DGridData* Qwt3DSurface::gridData() const
{
    QWT_DC(d);
    return d->m_actualDataG;
}

/**
 * @brief Returns the cell data pointer
 * @return Pointer to Qwt3DCellData, or nullptr if current data is grid-based
 */
Qwt3DCellData* Qwt3DSurface::cellData() const
{
    QWT_DC(d);
    return d->m_actualDataC;
}

/**
 * @brief Returns true if the current data is grid-based
 */
bool Qwt3DSurface::isGridData() const
{
    QWT_DC(d);
    return d->m_actualDataG && !d->m_actualDataG->empty();
}

// ---------------------------------------------------------------------------
// Qwt3DPlotItem interface
// ---------------------------------------------------------------------------

/**
 * @brief Returns the bounding hull of the surface in 3D space
 * @return ParallelEpiped bounding the surface data, or a degenerate box if no data
 */
ParallelEpiped Qwt3DSurface::hull() const
{
    QWT_DC(d);
    if (d->m_actualDataG && !d->m_actualDataG->empty())
        return d->m_actualDataG->hull();
    if (d->m_actualDataC && !d->m_actualDataC->empty())
        return d->m_actualDataC->hull();
    return ParallelEpiped(Triple(0, 0, 0), Triple(0, 0, 0));
}

// ---------------------------------------------------------------------------
// GL rendering
// ---------------------------------------------------------------------------

/**
 * @brief Builds the VBO, EBO, and VAO from current data
 * @details Called lazily from draw() when m_vboDirty is set. This method:
 *          1. Gathers vertices (position + normal + color) into a flat array
 *          2. Generates triangle indices for filled rendering
 *          3. Generates line indices for wireframe/mesh rendering
 *          4. Creates and uploads VBO with vertex data
 *          5. Creates and uploads EBO with combined tri+line indices
 *          6. Configures VAO vertex attribute pointers
 *
 * For grid data: vertex index = column * rows + row
 * For cell data: vertex index = node index in TripleField
 *
 * Resolution > 1 skips vertices in index generation (VBO stays full).
 */
void Qwt3DSurface::buildVBO()
{
    QWT_D(d);

    QVector<SurfaceVertex> vertices;
    QVector<unsigned int> triIndices;
    QVector<unsigned int> lineIndices;

    // Ensure we have a data color functor
    if (!d->m_dataColor) {
        if (plot()) {
            d->m_dataColor = new Qwt3DStandardColor(plot());
        } else {
            // No plot attached yet — cannot create default color
            return;
        }
    }

    if (d->m_actualDataG && !d->m_actualDataG->empty()) {
        // --- Grid data ---
        int cols = d->m_actualDataG->columns();
        int rows = d->m_actualDataG->rows();
        int step = d->m_resolution;

        // Build vertex array
        for (int i = 0; i < cols; ++i) {
            for (int j = 0; j < rows; ++j) {
                SurfaceVertex v;
                v.position = QVector3D(
                    static_cast<float>(d->m_actualDataG->vertices[i][j][0]),
                    static_cast<float>(d->m_actualDataG->vertices[i][j][1]),
                    static_cast<float>(d->m_actualDataG->vertices[i][j][2]));
                v.normal = QVector3D(
                    static_cast<float>(d->m_actualDataG->normals[i][j][0]),
                    static_cast<float>(d->m_actualDataG->normals[i][j][1]),
                    static_cast<float>(d->m_actualDataG->normals[i][j][2]));
                RGBA col = (*d->m_dataColor)(
                    d->m_actualDataG->vertices[i][j][0],
                    d->m_actualDataG->vertices[i][j][1],
                    d->m_actualDataG->vertices[i][j][2]);
                v.color = QVector4D(static_cast<float>(col.r),
                                    static_cast<float>(col.g),
                                    static_cast<float>(col.b),
                                    static_cast<float>(col.a));
                vertices.append(v);
            }
        }

        // Build triangle indices: each quad -> 2 triangles
        for (int i = 0; i < cols - step; i += step) {
            for (int j = 0; j < rows - step; j += step) {
                unsigned int v0 = static_cast<unsigned int>(i * rows + j);
                unsigned int v1 = static_cast<unsigned int>((i + step) * rows + j);
                unsigned int v2 = static_cast<unsigned int>(i * rows + (j + step));
                unsigned int v3 = static_cast<unsigned int>((i + step) * rows + (j + step));

                // Triangle 1: v0, v1, v2
                triIndices.append(v0);
                triIndices.append(v1);
                triIndices.append(v2);
                // Triangle 2: v1, v3, v2
                triIndices.append(v1);
                triIndices.append(v3);
                triIndices.append(v2);
            }
        }

        // Build line indices: horizontal and vertical edges
        for (int i = 0; i < cols - step; i += step) {
            for (int j = 0; j < rows - step; j += step) {
                unsigned int v0 = static_cast<unsigned int>(i * rows + j);
                unsigned int v1 = static_cast<unsigned int>((i + step) * rows + j);
                unsigned int v2 = static_cast<unsigned int>(i * rows + (j + step));

                // Horizontal edge: v0 -> v2
                lineIndices.append(v0);
                lineIndices.append(v2);
                // Vertical edge: v0 -> v1
                lineIndices.append(v0);
                lineIndices.append(v1);
            }
        }
        // Right boundary (last column)
        for (int j = 0; j < rows - step; j += step) {
            unsigned int v0 = static_cast<unsigned int>((cols - 1) * rows + j);
            unsigned int v1 = static_cast<unsigned int>((cols - 1) * rows + (j + step));
            lineIndices.append(v0);
            lineIndices.append(v1);
        }
        // Top boundary (last row)
        for (int i = 0; i < cols - step; i += step) {
            unsigned int v0 = static_cast<unsigned int>(i * rows + (rows - 1));
            unsigned int v1 = static_cast<unsigned int>((i + step) * rows + (rows - 1));
            lineIndices.append(v0);
            lineIndices.append(v1);
        }

    } else if (d->m_actualDataC && !d->m_actualDataC->empty()) {
        // --- Cell data ---
        int nodeCount = static_cast<int>(d->m_actualDataC->nodes.size());

        // Build vertex array
        for (int i = 0; i < nodeCount; ++i) {
            SurfaceVertex v;
            v.position = QVector3D(
                static_cast<float>(d->m_actualDataC->nodes[i].x),
                static_cast<float>(d->m_actualDataC->nodes[i].y),
                static_cast<float>(d->m_actualDataC->nodes[i].z));
            v.normal = QVector3D(
                static_cast<float>(d->m_actualDataC->normals[i].x),
                static_cast<float>(d->m_actualDataC->normals[i].y),
                static_cast<float>(d->m_actualDataC->normals[i].z));
            RGBA col = (*d->m_dataColor)(
                d->m_actualDataC->nodes[i].x,
                d->m_actualDataC->nodes[i].y,
                d->m_actualDataC->nodes[i].z);
            v.color = QVector4D(static_cast<float>(col.r),
                                static_cast<float>(col.g),
                                static_cast<float>(col.b),
                                static_cast<float>(col.a));
            vertices.append(v);
        }

        // Build triangle indices: fan decomposition per cell
        for (unsigned i = 0; i < d->m_actualDataC->cells.size(); ++i) {
            const Cell& cell = d->m_actualDataC->cells[i];
            if (cell.size() < 3)
                continue;
            for (size_t j = 1; j < cell.size() - 1; ++j) {
                triIndices.append(cell[0]);
                triIndices.append(cell[j]);
                triIndices.append(cell[j + 1]);
            }
        }

        // Build line indices: line loop per cell
        for (unsigned i = 0; i < d->m_actualDataC->cells.size(); ++i) {
            const Cell& cell = d->m_actualDataC->cells[i];
            for (size_t j = 0; j < cell.size(); ++j) {
                lineIndices.append(cell[j]);
                lineIndices.append(cell[(j + 1) % cell.size()]);
            }
        }
    }

    d->m_vertexCount = vertices.size();
    d->m_triIndexCount = triIndices.size();
    d->m_lineIndexCount = lineIndices.size();

    if (vertices.isEmpty())
        return;

    // Get OpenGL functions
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    if (!f)
        return;

    // Create/bind VAO
    if (!d->m_vao.isCreated())
        d->m_vao.create();
    d->m_vao.bind();

    // Create/bind VBO and upload vertices
    if (!d->m_vertexBuffer.isCreated())
        d->m_vertexBuffer.create();
    d->m_vertexBuffer.bind();
    d->m_vertexBuffer.allocate(vertices.constData(),
                               vertices.size() * sizeof(SurfaceVertex));

    // Configure vertex attributes (layout locations match shader)
    const int stride = sizeof(SurfaceVertex);
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                             reinterpret_cast<void*>(sizeof(QVector3D)));
    f->glEnableVertexAttribArray(2);
    f->glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
                             reinterpret_cast<void*>(2 * sizeof(QVector3D)));

    // Create/bind EBO and upload combined indices (tri + line)
    if (!d->m_indexBuffer.isCreated())
        d->m_indexBuffer.create();
    d->m_indexBuffer.bind();

    int totalIndexCount = triIndices.size() + lineIndices.size();
    d->m_indexBuffer.allocate(totalIndexCount * sizeof(unsigned int));
    if (!triIndices.isEmpty())
        d->m_indexBuffer.write(0, triIndices.constData(),
                               triIndices.size() * sizeof(unsigned int));
    if (!lineIndices.isEmpty()) {
        d->m_indexBuffer.write(triIndices.size() * sizeof(unsigned int),
                               lineIndices.constData(),
                               lineIndices.size() * sizeof(unsigned int));
    }

    // Release VAO (saves VBO + EBO bindings)
    d->m_vao.release();
}

/**
 * @brief Draws the surface using VBO/VAO and GLSL shaders
 * @details Called by Qwt3DPlot::paintGL() for each visible item.
 *          The GL context is guaranteed to be current.
 *
 * Rendering strategy by plot style:
 * - FILLED: glDrawElements(GL_TRIANGLES) with vertex colors
 * - FILLEDMESH: triangles + lines (two draw calls)
 * - WIREFRAME: glDrawElements(GL_LINES) with mesh color
 * - HIDDENLINE: triangles with background color + lines
 * - QWT3D_POINTS: glDrawArrays(GL_POINTS)
 */
void Qwt3DSurface::draw()
{
    QWT_D(d);

    if (!plot() || !plot()->initializedGL())
        return;

    if (d->m_plotStyle == NOPLOT)
        return;

    // Build VBO if dirty
    if (d->m_vboDirty) {
        buildVBO();
        d->m_vboDirty = false;
    }

    if (d->m_vertexCount == 0)
        return;

    // Initialize shader on first use
    if (!d->m_shaderInitialized) {
        d->m_shader.addShaderFromSourceFile(
            QOpenGLShader::Vertex, ":/shaders/surface.vert");
        d->m_shader.addShaderFromSourceFile(
            QOpenGLShader::Fragment, ":/shaders/surface.frag");
        if (!d->m_shader.link()) {
            return;
        }
        d->m_shaderInitialized = true;
    }

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    if (!f)
        return;

    // Bind shader and set uniforms
    d->m_shader.bind();

    QMatrix4x4 mv = plot()->modelViewMatrix();
    QMatrix4x4 proj = plot()->projectionMatrix();
    QMatrix3x3 normalMatrix = mv.normalMatrix();

    d->m_shader.setUniformValue("uModelView", mv);
    d->m_shader.setUniformValue("uProjection", proj);
    d->m_shader.setUniformValue("uNormalMatrix", normalMatrix);

    // Lighting uniforms
    bool useLighting = plot()->lightingEnabled();
    d->m_shader.setUniformValue("uUseLighting", useLighting);
    if (useLighting) {
        d->m_shader.setUniformValue("uLightPos",
                                    QVector3D(0.0f, 0.0f, 10.0f));
        d->m_shader.setUniformValue("uLightColor",
                                    QVector3D(1.0f, 1.0f, 1.0f));
        d->m_shader.setUniformValue("uShininess", 32.0f);
    }

    // Bind VAO (binds VBO + EBO + vertex attribute config)
    d->m_vao.bind();

    // Render based on plot style
    const size_t triOffset = 0;
    const size_t lineOffset = static_cast<size_t>(d->m_triIndexCount) * sizeof(unsigned int);

    switch (d->m_plotStyle) {
    case FILLED:
        d->m_shader.setUniformValue("uUseOverrideColor", false);
        if (d->m_triIndexCount > 0)
            f->glDrawElements(GL_TRIANGLES, d->m_triIndexCount,
                              GL_UNSIGNED_INT, reinterpret_cast<void*>(triOffset));
        break;

    case FILLEDMESH:
        // Pass 1: filled triangles with vertex colors
        d->m_shader.setUniformValue("uUseOverrideColor", false);
        if (d->m_triIndexCount > 0) {
            f->glEnable(GL_POLYGON_OFFSET_FILL);
            f->glPolygonOffset(static_cast<float>(d->m_polygonOffset), 1.0f);
            f->glDrawElements(GL_TRIANGLES, d->m_triIndexCount,
                              GL_UNSIGNED_INT, reinterpret_cast<void*>(triOffset));
        }
        // Pass 2: mesh lines with mesh color
        d->m_shader.setUniformValue("uUseOverrideColor", true);
        d->m_shader.setUniformValue("uOverrideColor",
                                    QVector4D(static_cast<float>(d->m_meshColor.r),
                                              static_cast<float>(d->m_meshColor.g),
                                              static_cast<float>(d->m_meshColor.b),
                                              static_cast<float>(d->m_meshColor.a)));
        if (d->m_lineIndexCount > 0) {
            f->glDrawElements(GL_LINES, d->m_lineIndexCount,
                              GL_UNSIGNED_INT, reinterpret_cast<void*>(lineOffset));
        }
        break;

    case WIREFRAME:
        d->m_shader.setUniformValue("uUseOverrideColor", true);
        d->m_shader.setUniformValue("uOverrideColor",
                                    QVector4D(static_cast<float>(d->m_meshColor.r),
                                              static_cast<float>(d->m_meshColor.g),
                                              static_cast<float>(d->m_meshColor.b),
                                              static_cast<float>(d->m_meshColor.a)));
        if (d->m_lineIndexCount > 0) {
            f->glDrawElements(GL_LINES, d->m_lineIndexCount,
                              GL_UNSIGNED_INT, reinterpret_cast<void*>(lineOffset));
        }
        break;

    case HIDDENLINE: {
        // Pass 1: filled triangles with background color (hidden surface removal)
        RGBA bg = plot()->backgroundRGBAColor();
        d->m_shader.setUniformValue("uUseOverrideColor", true);
        d->m_shader.setUniformValue("uOverrideColor",
                                    QVector4D(static_cast<float>(bg.r),
                                              static_cast<float>(bg.g),
                                              static_cast<float>(bg.b),
                                              static_cast<float>(bg.a)));
        if (d->m_triIndexCount > 0) {
            f->glEnable(GL_POLYGON_OFFSET_FILL);
            f->glPolygonOffset(static_cast<float>(d->m_polygonOffset), 1.0f);
            f->glDrawElements(GL_TRIANGLES, d->m_triIndexCount,
                              GL_UNSIGNED_INT, reinterpret_cast<void*>(triOffset));
        }
        // Pass 2: mesh lines
        d->m_shader.setUniformValue("uUseOverrideColor", true);
        d->m_shader.setUniformValue("uOverrideColor",
                                    QVector4D(static_cast<float>(d->m_meshColor.r),
                                              static_cast<float>(d->m_meshColor.g),
                                              static_cast<float>(d->m_meshColor.b),
                                              static_cast<float>(d->m_meshColor.a)));
        if (d->m_lineIndexCount > 0) {
            f->glDrawElements(GL_LINES, d->m_lineIndexCount,
                              GL_UNSIGNED_INT, reinterpret_cast<void*>(lineOffset));
        }
        break;
    }

    case QWT3D_POINTS:
        d->m_shader.setUniformValue("uUseOverrideColor", false);
        f->glDrawArrays(GL_POINTS, 0, d->m_vertexCount);
        break;

    default:
        break;
    }

    d->m_vao.release();
    d->m_shader.release();

    // TODO: Enrichment rendering (normals, crosshairs, etc.) will be
    // reimplemented with VBO-based drawing in a future plan.
    // The old enrichment code used legacy GL (glBegin/glEnd) which is
    // forbidden in Core Profile.
}
