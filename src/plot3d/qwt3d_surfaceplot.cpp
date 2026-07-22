#include "qwt3d_surfaceplot_p.h"


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
 * @brief Constructs a SurfacePlot widget (TEMPORARY STUB)
 * @param parent Parent widget
 * @details SurfacePlot is temporarily stubbed during the Plot+Item refactor.
 *          It will be refactored into a Qwt3DSurface item in Plan 06.
 */
SurfacePlot::SurfacePlot(QWidget* parent) : Qwt3DPlot(parent), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    d->m_actualDataG = new Qwt3DGridData();
    d->m_actualDataC = new Qwt3DCellData();
}

/**
 * @brief Destructor
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

void SurfacePlot::showNormals(bool b)
{
    QWT_D(d);
    d->m_dataNormals = b;
}

void SurfacePlot::setNormalLength(double val)
{
    QWT_D(d);
    if (val < 0 || val > 1)
        return;
    d->m_normalLength = val;
}

void SurfacePlot::setNormalQuality(int val)
{
    QWT_D(d);
    if (val < 3)
        return;
    d->m_normalQuality = val;
}

std::pair< int, int > SurfacePlot::facets() const
{
    // Stub: returns (0, 0) since data rendering is disabled during refactor
    return std::pair< int, int >(0, 0);
}

void SurfacePlot::setResolution(int res)
{
    QWT_D(d);
    if ((d->m_resolution == res) || res < 1)
        return;

    d->m_resolution = res;
    emit resolutionChanged(res);
}

void SurfacePlot::updateNormals()
{
    // Stub: normals recalculation disabled during refactor
    // Will be reimplemented in Qwt3DSurface item (Plan 06)
}

// Stub implementations of loadFromData — data loading is disabled during refactor.
// These will be reimplemented in Qwt3DSurface item (Plan 06).

bool SurfacePlot::loadFromData(Triple** data, unsigned int columns, unsigned int rows, bool, bool)
{
    // Stub: data loading disabled during refactor
    (void)data;
    (void)columns;
    (void)rows;
    return false;
}

bool SurfacePlot::loadFromData(double** data, unsigned int columns, unsigned int rows, double, double, double, double)
{
    // Stub: data loading disabled during refactor
    (void)data;
    (void)columns;
    (void)rows;
    return false;
}

bool SurfacePlot::loadFromData(TripleField const& data, CellField const& poly)
{
    // Stub: data loading disabled during refactor
    (void)data;
    (void)poly;
    return false;
}
