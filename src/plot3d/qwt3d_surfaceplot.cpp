#include "qwt3d_surfaceplot_p.h"

using namespace std;

SurfacePlot::PrivateData::PrivateData(SurfacePlot* q)
    : q_ptr(q)
    , m_dataNormals(false)
    , m_normalLength(0.02)
    , m_normalQuality(3)
    , m_resolution(1)
    , m_floorStyle(NOFLOOR)
    , m_actualDataG(nullptr)
    , m_actualDataC(nullptr)
{
}

/**
 * @brief Constructs a SurfacePlot widget
 * @param[in] parent Parent widget
 * @details Initializes with dataNormals()==false, NOFLOOR, resolution() == 1
 *
 */
SurfacePlot::SurfacePlot(QWidget* parent) : Plot3D(parent), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    d->m_actualDataG = new Qwt3DGridData();
    d->m_actualDataC = new Qwt3DCellData();

    setActualData(d->m_actualDataG);
}

/**
 * @brief Destructor
 *
 */
SurfacePlot::~SurfacePlot()
{
    QWT_D(d);
    delete d->m_actualDataG;
    delete d->m_actualDataC;
}

int SurfacePlot::resolution() const
{
    QWT_DC(d);
    return d->m_resolution;
}

FLOORSTYLE SurfacePlot::floorStyle() const
{
    QWT_DC(d);
    return d->m_floorStyle;
}

void SurfacePlot::setFloorStyle(FLOORSTYLE val)
{
    QWT_D(d);
    d->m_floorStyle = val;
}

bool SurfacePlot::normals() const
{
    QWT_DC(d);
    return d->m_dataNormals;
}

double SurfacePlot::normalLength() const
{
    QWT_DC(d);
    return d->m_normalLength;
}

int SurfacePlot::normalQuality() const
{
    QWT_DC(d);
    return d->m_normalQuality;
}

/**
 * @brief Shows or hides data normals
 * @param[in] b True to show normals, false to hide
 *
 */
void SurfacePlot::showNormals(bool b)
{
    QWT_D(d);
    d->m_dataNormals = b;
}

/**
 * @brief Sets the normal vector length
 * @param[in] val Normal length value (0.0 to 1.0). Values < 0 or > 1 are ignored.
 *
 */
void SurfacePlot::setNormalLength(double val)
{
    QWT_D(d);
    if (val < 0 || val > 1)
        return;
    d->m_normalLength = val;
}

/**
 * @brief Sets the normal vector quality (number of arrow segments)
 * @param[in] val Quality value. Values < 3 are ignored.
 *
 */
void SurfacePlot::setNormalQuality(int val)
{
    QWT_D(d);
    if (val < 3)
        return;
    d->m_normalQuality = val;
}

/**
 * @brief Calculates the smallest x-y-z parallelepiped enclosing the data
 * @details It can be accessed by hull();
 *
 */
void SurfacePlot::calculateHull()
{
    Qwt3DData* data = actualData();
    if (!data || data->empty())
        return;
    setHull(data->hull());
}

/**
 * @brief Sets data resolution and updates widget
 * @param[in] res Resolution multiplier (res == 1 means original resolution). If res < 1, the function does nothing.
 *
 */
void SurfacePlot::setResolution(int res)
{
    QWT_D(d);
    Qwt3DData* data = actualData();
    if (!data || data->datatype == POLYGON)
        return;

    if ((d->m_resolution == res) || res < 1)
        return;

    d->m_resolution = res;
    updateNormals();
    updateData();
    if (initializedGL())
        update();

    emit resolutionChanged(res);
}

void SurfacePlot::updateNormals()
{
    QWT_D(d);
    Qwt3DData* data         = actualData();
    std::vector< GLuint >& dl = displayLists();

    SaveGlDeleteLists(dl[ NormalObject ], 1);

    if ((plotStyle() == NOPLOT && !normals()) || !data)
        return;

    dl[ NormalObject ] = glGenLists(1);
    glNewList(dl[ NormalObject ], GL_COMPILE);

    if (data->datatype == POLYGON)
        createNormalsC();
    else if (data->datatype == GRID)
        createNormalsG();

    glEndList();
}

void SurfacePlot::createData()
{
    Qwt3DData* data = actualData();
    if (!data)
        return;
    if (data->datatype == POLYGON)
        createDataC();
    else if (data->datatype == GRID)
        createDataG();
}

void SurfacePlot::createFloorData()
{
    Qwt3DData* data = actualData();
    if (!data)
        return;
    if (data->datatype == POLYGON)
        createFloorDataC();
    else if (data->datatype == GRID)
        createFloorDataG();
}

/**
 * @brief Returns the number of facets in the data
 * @return (columns,rows) for grid data, (number of cells,1) for polygon data, (0,0) otherwise
 * @details The returned value is not affected by resolution().
 *
 */
pair< int, int > SurfacePlot::facets() const
{
    QWT_DC(d);
    if (!hasData())
        return pair< int, int >(0, 0);

    Qwt3DData* data = actualData();
    if (data->datatype == POLYGON)
        return pair< int, int >(int(d->m_actualDataC->cells.size()), 1);
    else if (data->datatype == GRID)
        return pair< int, int >(d->m_actualDataG->columns(), d->m_actualDataG->rows());
    else
        return pair< int, int >(0, 0);
}

void SurfacePlot::createPoints()
{
    Dot pt;
    createEnrichment(pt);
}

void SurfacePlot::createEnrichment(Enrichment& p)
{
    QWT_D(d);
    Qwt3DData* data = actualData();
    if (!data)
        return;

    // todo future work
    if (p.type() != Enrichment::VERTEXENRICHMENT)
        return;

    p.assign(*this);
    p.drawBegin();

    VertexEnrichment* ve = static_cast< VertexEnrichment* >(&p);
    if (data->datatype == POLYGON) {
        for (unsigned i = 0; i != d->m_actualDataC->normals.size(); ++i)
            ve->draw(d->m_actualDataC->nodes[ i ]);
    } else if (data->datatype == GRID) {
        int step = resolution();
        for (int i = 0; i <= d->m_actualDataG->columns() - step; i += step)
            for (int j = 0; j <= d->m_actualDataG->rows() - step; j += step)
                ve->draw(Triple(d->m_actualDataG->vertices[ i ][ j ][ 0 ],
                                d->m_actualDataG->vertices[ i ][ j ][ 1 ],
                                d->m_actualDataG->vertices[ i ][ j ][ 2 ]));
    }
    p.drawEnd();
}
