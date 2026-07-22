#ifndef QWT3D_SURFACEPLOT_H
#define QWT3D_SURFACEPLOT_H

#include "qwt3d_plot.h"



/**
 * @brief A class representing surfaces
 * @details SurfacePlot provides visualization of surface data in 3D space,
 *          supporting both grid-based and cell-based data representations.
 *
 */
class QWT3D_EXPORT SurfacePlot : public Plot3D
{
    Q_OBJECT

public:
    SurfacePlot(QWidget* parent = nullptr);
    ~SurfacePlot() override;
    // Recalculates surface normals
    void updateNormals();
    // Returns data resolution (1 means all data)
    int resolution() const;
    // Returns the number of mesh cells for the ORIGINAL data
    std::pair< int, int > facets() const;
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

Q_SIGNALS:
    /**
     * @brief Signal emitted when the resolution changes
     * @param resolution The new resolution value
     *
     */
    void resolutionChanged(int);

public Q_SLOTS:
    void setResolution(int);

protected:
    QWT_DECLARE_PRIVATE(SurfacePlot)

    void calculateHull() override;
    void createData() override;
    void createEnrichment(Enrichment& p) override;
    virtual void createFloorData();
    void createNormals();
    void createPoints();

    void readIn(Qwt3DGridData& gdata, Triple** data, unsigned int columns, unsigned int rows);
    void readIn(Qwt3DGridData& gdata,
                double** data,
                unsigned int columns,
                unsigned int rows,
                double minx,
                double maxx,
                double miny,
                double maxy);
    void calcNormals(Qwt3DGridData& gdata);
    void sewPeriodic(Qwt3DGridData& gdata);

private:
    void Data2Floor();
    void Isolines2Floor();

    virtual void createDataG();
    virtual void createFloorDataG();
    void createNormalsG();
    void Data2FloorG();
    void Isolines2FloorG();
    void setColorFromVertexG(int ix, int iy, bool skip = false);

    virtual void createDataC();
    virtual void createFloorDataC();
    void createNormalsC();
    void Data2FloorC();
    void Isolines2FloorC();
    void setColorFromVertexC(int node, bool skip = false);
};


#endif  // QWT3D_SURFACEPLOT_H
