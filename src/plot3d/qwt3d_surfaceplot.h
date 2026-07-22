#ifndef QWT3D_SURFACEPLOT_H
#define QWT3D_SURFACEPLOT_H

#include "qwt3d_plot.h"



/**
 * @brief A class representing surfaces (TEMPORARY STUB)
 * @details SurfacePlot is temporarily stubbed during the Plot+Item refactor.
 *          It inherits Qwt3DPlot but no longer overrides calculateHull/createData
 *          (which have been removed from Qwt3DPlot). This class will be refactored
 *          into a Qwt3DSurface item in Plan 06.
 *
 * @warning This class is non-functional in its current state. It compiles but
 *          does not render any surface data.
 */
class QWT3D_EXPORT SurfacePlot : public Qwt3DPlot
{
    Q_OBJECT

public:
    SurfacePlot(QWidget* parent = nullptr);
    ~SurfacePlot() override;

    // Deprecated stubs — will be replaced by Qwt3DSurface item API
    bool loadFromData(Triple** data, unsigned int columns, unsigned int rows, bool uperiodic = false, bool vperiodic = false);
    bool loadFromData(double** data, unsigned int columns, unsigned int rows, double minx, double maxx, double miny, double maxy);
    bool loadFromData(TripleField const& data, CellField const& poly);

    // Deprecated - Use loadFromData instead
    bool createDataRepresentation(Triple** data,
                                  unsigned int columns,
                                  unsigned int rows,
                                  bool uperiodic = false,
                                  bool vperiodic = false)
    {
        return loadFromData(data, columns, rows, uperiodic, vperiodic);
    }
    // Deprecated - Use loadFromData instead
    bool createDataRepresentation(double** data,
                                  unsigned int columns,
                                  unsigned int rows,
                                  double minx,
                                  double maxx,
                                  double miny,
                                  double maxy)
    {
        return loadFromData(data, columns, rows, minx, maxx, miny, maxy);
    }
    // Deprecated - Use loadFromData instead
    bool createDataRepresentation(TripleField const& data, CellField const& poly)
    {
        return loadFromData(data, poly);
    }

    // Return floor style
    FLOORSTYLE floorStyle() const;
    // Sets floor style
    void setFloorStyle(FLOORSTYLE val);

    // Returns data resolution (1 means all data)
    int resolution() const;
    // Draw normals to every vertex
    void showNormals(bool);
    // Returns true, if normal drawing is on
    bool normals() const;

    // Sets length of normals in percent per hull diagonale
    void setNormalLength(double val);
    // Returns relative length of normals
    double normalLength() const;
    // Increases plotting quality of normal arrows
    void setNormalQuality(int val);
    // Returns plotting quality of normal arrows
    int normalQuality() const;

    // Returns the number of mesh cells for the ORIGINAL data
    std::pair< int, int > facets() const;

    // Recalculates surface normals
    void updateNormals();

Q_SIGNALS:
    /**
     * @brief Signal emitted when the resolution changes
     * @param resolution The new resolution value
     */
    void resolutionChanged(int);

public Q_SLOTS:
    void setResolution(int);

protected:
    QWT_DECLARE_PRIVATE(SurfacePlot)

private:
    // All rendering methods are temporarily stubbed.
    // SurfacePlot will be refactored to Qwt3DSurface item in Plan 06.
};


#endif  // QWT3D_SURFACEPLOT_H
