#ifndef QWT3D_SURFACE_H
#define QWT3D_SURFACE_H

#include "qwt3d_plotitem.h"
#include "qwt3d_types.h"
#include "qwt3d_color.h"
#include "qwt3d_enrichment.h"

#include <list>

class Qwt3DGridData;
class Qwt3DCellData;

/**
 * @brief 3D surface plot item with VBO/VAO rendering
 * @details Qwt3DSurface is a Qwt3DPlotItem that renders 3D surface data using
 *          modern OpenGL (VBO/VAO + GLSL shaders). It supports both grid-based
 *          (regular mesh) and cell-based (polygon mesh) data, with multiple
 *          rendering styles (filled, wireframe, hiddenline, points).
 *
 * The item manages its own data storage, color mapping, and GL resources
 * (VBO, VAO, EBO, shader program). Data is uploaded to GPU memory when dirty
 * and reused across frames until the data changes.
 *
 * @code
 * auto plot = new Qwt3DPlot(parent);
 *
 * auto surface = new Qwt3DSurface();
 * surface->loadFromData(data, columns, rows, minX, maxX, minY, maxY);
 * surface->setPlotStyle(FILLEDMESH);
 * surface->attach(plot);
 * @endcode
 *
 * @sa Qwt3DPlotItem, Qwt3DPlot, Qwt3DColor
 */
class QWT3D_EXPORT Qwt3DSurface : public Qwt3DPlotItem
{
    QWT_DECLARE_PRIVATE(Qwt3DSurface)

public:
    /// Constructs an empty surface item
    Qwt3DSurface();
    /// Destructor — releases GL resources and data
    ~Qwt3DSurface() override;

    // Data loading — grid data

    /// Load grid data from Triple array (x, y, z per vertex)
    void loadFromData(Triple** data, unsigned int columns, unsigned int rows,
                      bool uperiodic = false, bool vperiodic = false);
    /// Load grid data from z-value matrix with explicit x/y domain
    void loadFromData(double** data, unsigned int columns, unsigned int rows,
                      double minx, double maxx, double miny, double maxy);

    // Data loading — cell data

    /// Load cell (polygon) data from node coordinates and cell indices
    void loadFromData(TripleField const& nodes, CellField const& poly);

    // Resolution

    /// Returns data resolution (1 = all data, higher = coarser)
    int resolution() const;

    // Floor style

    /// Returns the current floor projection style
    FLOORSTYLE floorStyle() const;
    /// Sets the floor projection style
    void setFloorStyle(FLOORSTYLE style);

    // Normals

    /// Returns true if normal vectors are drawn
    bool normals() const;
    /// Show or hide normal vectors at each vertex
    void showNormals(bool b);
    /// Returns the relative length of drawn normals (0..1)
    double normalLength() const;
    /// Sets the relative length of drawn normals
    void setNormalLength(double val);
    /// Returns the quality of normal arrow rendering
    int normalQuality() const;
    /// Sets the quality of normal arrow rendering (minimum 3)
    void setNormalQuality(int val);

    /// Returns the number of mesh cells in the original data
    std::pair<int, int> facets() const;

    /// Recalculates surface normals from current data
    void updateNormals();

    // Style (moved from Plot3D)

    /// Returns the current plotting style
    PLOTSTYLE plotStyle() const;
    /// Sets the plotting style
    void setPlotStyle(PLOTSTYLE style);
    /// Sets the data color functor (takes ownership)
    void setDataColor(Qwt3DColor* color);
    /// Returns the data color functor
    const Qwt3DColor* dataColor() const;
    /// Returns the mesh line color
    RGBA meshColor() const;
    /// Sets the mesh line color
    void setMeshColor(RGBA color);
    /// Returns the mesh line width
    double meshLineWidth() const;
    /// Sets the mesh line width
    void setMeshLineWidth(double width);
    /// Returns the number of isolines
    int isolines() const;
    /// Sets the number of isolines
    void setIsolines(int n);
    /// Returns true if smooth mesh lines are enabled
    bool smoothMesh() const;
    /// Enables or disables smooth mesh lines
    void setSmoothMesh(bool smooth);
    /// Returns the polygon offset for filled rendering
    double polygonOffset() const;
    /// Sets the polygon offset for filled rendering
    void setPolygonOffset(double offset);

    // Enrichments (moved from Plot3D)

    /// Adds an enrichment to the surface (clones the argument)
    Qwt3DEnrichment* addEnrichment(Qwt3DEnrichment const& enrichment);
    /// Removes an enrichment from the surface
    bool degrade(Qwt3DEnrichment* enrichment);

    // Qwt3DPlotItem interface

    void draw() override;
    ParallelEpiped hull() const override;

public Q_SLOTS:
    /// Sets the data resolution (1 = full, higher = coarser)
    void setResolution(int res);

protected:
    /// Returns the grid data pointer (nullptr if cell data)
    Qwt3DGridData* gridData() const;
    /// Returns the cell data pointer (nullptr if grid data)
    Qwt3DCellData* cellData() const;
    /// Returns true if the current data is grid-based
    bool isGridData() const;

private:
    void buildVBO();
};

#endif // QWT3D_SURFACE_H
