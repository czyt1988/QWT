#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4305)
#pragma warning(disable : 4786)
#endif

#include "qwt3d_surfaceplot_p.h"
#include "qwt3d_enrichment_std.h"

using namespace std;

void SurfacePlot::createDataG()
{
    QWT_D(d);
    createFloorData();

    if (plotStyle() == NOPLOT)
        return;

    int i, j;
    RGBA col;
    int step = resolution();

    if (plotStyle() == QWT3D_POINTS) {
        createPoints();
        return;
    } else if (plotStyle() == USER) {
        if (userStyle())
            createEnrichment(*userStyle());
        return;
    }

    setDeviceLineWidth(meshLineWidth());

    GLStateBewarer sb(GL_POLYGON_OFFSET_FILL, true);
    setDevicePolygonOffset(polygonOffset(), 1.0);

    GLStateBewarer sb2(GL_LINE_SMOOTH, smoothDataMesh());
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int lastcol = d->m_actualDataG->columns();
    int lastrow = d->m_actualDataG->rows();

    if (plotStyle() != WIREFRAME) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_QUADS);

        bool hl = (plotStyle() == HIDDENLINE);
        if (hl) {
            col = backgroundRGBAColor();
            glColor4d(col.r, col.g, col.b, col.a);
        }

        for (i = 0; i < lastcol - step; i += step) {
            glBegin(GL_TRIANGLE_STRIP);
            setColorFromVertexG(i, 0, hl);
            glNormal3dv(d->m_actualDataG->normals[ i ][ 0 ]);
            glVertex3dv(d->m_actualDataG->vertices[ i ][ 0 ]);

            setColorFromVertexG(i + step, 0, hl);
            glNormal3dv(d->m_actualDataG->normals[ i + step ][ 0 ]);
            glVertex3dv(d->m_actualDataG->vertices[ i + step ][ 0 ]);

            for (j = 0; j < lastrow - step; j += step) {
                setColorFromVertexG(i, j + step, hl);
                glNormal3dv(d->m_actualDataG->normals[ i ][ j + step ]);
                glVertex3dv(d->m_actualDataG->vertices[ i ][ j + step ]);

                setColorFromVertexG(i + step, j + step, hl);
                glNormal3dv(d->m_actualDataG->normals[ i + step ][ j + step ]);
                glVertex3dv(d->m_actualDataG->vertices[ i + step ][ j + step ]);
            }
            glEnd();
        }
    }

    if (plotStyle() == FILLEDMESH || plotStyle() == WIREFRAME || plotStyle() == HIDDENLINE) {
        glColor4d(meshColor().r, meshColor().g, meshColor().b, meshColor().a);

        if (step < d->m_actualDataG->columns() && step < d->m_actualDataG->rows()) {
            glBegin(GL_LINE_LOOP);
            for (i = 0; i < d->m_actualDataG->columns() - step; i += step)
                glVertex3dv(d->m_actualDataG->vertices[ i ][ 0 ]);
            for (j = 0; j < d->m_actualDataG->rows() - step; j += step)
                glVertex3dv(d->m_actualDataG->vertices[ i ][ j ]);
            for (; i >= 0; i -= step)
                glVertex3dv(d->m_actualDataG->vertices[ i ][ j ]);
            for (; j >= 0; j -= step)
                glVertex3dv(d->m_actualDataG->vertices[ 0 ][ j ]);
            glEnd();
        }

        // weaving
        for (i = step; i < d->m_actualDataG->columns() - step; i += step) {
            glBegin(GL_LINE_STRIP);
            for (j = 0; j < d->m_actualDataG->rows(); j += step)
                glVertex3dv(d->m_actualDataG->vertices[ i ][ j ]);
            glEnd();
        }
        for (j = step; j < d->m_actualDataG->rows() - step; j += step) {
            glBegin(GL_LINE_STRIP);
            for (i = 0; i < d->m_actualDataG->columns(); i += step)
                glVertex3dv(d->m_actualDataG->vertices[ i ][ j ]);
            glEnd();
        }
    }
}

void SurfacePlot::setColorFromVertexG(int ix, int iy, bool skip)
{
    QWT_D(d);
    if (skip)
        return;

    RGBA col = (*dataColor())(d->m_actualDataG->vertices[ ix ][ iy ][ 0 ],
                              d->m_actualDataG->vertices[ ix ][ iy ][ 1 ],
                              d->m_actualDataG->vertices[ ix ][ iy ][ 2 ]);

    glColor4d(col.r, col.g, col.b, col.a);
}

void SurfacePlot::createNormalsG()
{
    QWT_D(d);
    if (!normals() || d->m_actualDataG->empty())
        return;

    Qwt3DArrow arrow;
    arrow.setQuality(normalQuality());

    Triple basev, topv, norm;

    int step = resolution();

    double diag = (d->m_actualDataG->hull().maxVertex - d->m_actualDataG->hull().minVertex).length() * normalLength();

    arrow.assign(*this);
    arrow.drawBegin();
    for (int i = 0; i <= d->m_actualDataG->columns() - step; i += step) {
        for (int j = 0; j <= d->m_actualDataG->rows() - step; j += step) {
            basev = Triple(d->m_actualDataG->vertices[ i ][ j ][ 0 ],
                           d->m_actualDataG->vertices[ i ][ j ][ 1 ],
                           d->m_actualDataG->vertices[ i ][ j ][ 2 ]);
            topv  = Triple(d->m_actualDataG->vertices[ i ][ j ][ 0 ] + d->m_actualDataG->normals[ i ][ j ][ 0 ],
                          d->m_actualDataG->vertices[ i ][ j ][ 1 ] + d->m_actualDataG->normals[ i ][ j ][ 1 ],
                          d->m_actualDataG->vertices[ i ][ j ][ 2 ] + d->m_actualDataG->normals[ i ][ j ][ 2 ]);

            norm = topv - basev;
            norm.normalize();
            norm *= diag;

            arrow.setTop(basev + norm);
            arrow.setColor((*dataColor())(basev.x, basev.y, basev.z));
            arrow.draw(basev);
        }
    }
    arrow.drawEnd();
}

void SurfacePlot::readIn(Qwt3DGridData& gdata, Triple** data, unsigned int columns, unsigned int rows)
{
    gdata.setSize(columns, rows);

    ParallelEpiped range(Triple(DBL_MAX, DBL_MAX, DBL_MAX), Triple(-DBL_MAX, -DBL_MAX, -DBL_MAX));

    /* fill out the vertex array for the mesh. */
    for (unsigned i = 0; i != columns; ++i) {
        for (unsigned j = 0; j != rows; ++j) {
            gdata.vertices[ i ][ j ][ 0 ] = data[ i ][ j ].x;
            gdata.vertices[ i ][ j ][ 1 ] = data[ i ][ j ].y;
            gdata.vertices[ i ][ j ][ 2 ] = data[ i ][ j ].z;

            if (data[ i ][ j ].x > range.maxVertex.x)
                range.maxVertex.x = data[ i ][ j ].x;
            if (data[ i ][ j ].y > range.maxVertex.y)
                range.maxVertex.y = data[ i ][ j ].y;
            if (data[ i ][ j ].z > range.maxVertex.z)
                range.maxVertex.z = data[ i ][ j ].z;
            if (data[ i ][ j ].x < range.minVertex.x)
                range.minVertex.x = data[ i ][ j ].x;
            if (data[ i ][ j ].y < range.minVertex.y)
                range.minVertex.y = data[ i ][ j ].y;
            if (data[ i ][ j ].z < range.minVertex.z)
                range.minVertex.z = data[ i ][ j ].z;
        }
    }
    gdata.setHull(range);
}

void SurfacePlot::readIn(Qwt3DGridData& gdata,
                         double** data,
                         unsigned int columns,
                         unsigned int rows,
                         double minx,
                         double maxx,
                         double miny,
                         double maxy)
{
    gdata.setPeriodic(false, false);
    gdata.setSize(columns, rows);

    double dx = (maxx - minx) / (gdata.columns() - 1);
    double dy = (maxy - miny) / (gdata.rows() - 1);

    double tmin = DBL_MAX;
    double tmax = -DBL_MAX;

    /* fill out the vertex array for the mesh. */
    for (unsigned i = 0; i != columns; ++i) {
        for (unsigned j = 0; j != rows; ++j) {
            gdata.vertices[ i ][ j ][ 0 ] = minx + i * dx;
            gdata.vertices[ i ][ j ][ 1 ] = miny + j * dy;
            gdata.vertices[ i ][ j ][ 2 ] = data[ i ][ j ];

            if (data[ i ][ j ] > tmax)
                tmax = data[ i ][ j ];
            if (data[ i ][ j ] < tmin)
                tmin = data[ i ][ j ];
        }
    }
    ParallelEpiped hull = ParallelEpiped(Triple(gdata.vertices[ 0 ][ 0 ][ 0 ], gdata.vertices[ 0 ][ 0 ][ 1 ], tmin),
                                         Triple(gdata.vertices[ gdata.columns() - 1 ][ gdata.rows() - 1 ][ 0 ],
                                                gdata.vertices[ gdata.columns() - 1 ][ gdata.rows() - 1 ][ 1 ],
                                                tmax));

    gdata.setHull(hull);
}

void SurfacePlot::calcNormals(Qwt3DGridData& gdata)
{

    unsigned int rows    = gdata.rows();
    unsigned int columns = gdata.columns();

    // normals

    Triple u, v, n;  // for cross product

    for (unsigned i = 0; i != columns; ++i) {
        for (unsigned j = 0; j != rows; ++j) {
            n = Triple(0, 0, 0);

            if (i < columns - 1 && j < rows - 1) {
                /*	get two vectors to cross */
                u = Triple(gdata.vertices[ i + 1 ][ j ][ 0 ] - gdata.vertices[ i ][ j ][ 0 ],
                           gdata.vertices[ i + 1 ][ j ][ 1 ] - gdata.vertices[ i ][ j ][ 1 ],
                           gdata.vertices[ i + 1 ][ j ][ 2 ] - gdata.vertices[ i ][ j ][ 2 ]);

                v = Triple(gdata.vertices[ i ][ j + 1 ][ 0 ] - gdata.vertices[ i ][ j ][ 0 ],
                           gdata.vertices[ i ][ j + 1 ][ 1 ] - gdata.vertices[ i ][ j ][ 1 ],
                           gdata.vertices[ i ][ j + 1 ][ 2 ] - gdata.vertices[ i ][ j ][ 2 ]);
                /* get the normalized cross product */
                n += normalizedcross(u, v);  // right hand system here !
            }

            if (i > 0 && j < rows - 1) {
                u = Triple(gdata.vertices[ i ][ j + 1 ][ 0 ] - gdata.vertices[ i ][ j ][ 0 ],
                           gdata.vertices[ i ][ j + 1 ][ 1 ] - gdata.vertices[ i ][ j ][ 1 ],
                           gdata.vertices[ i ][ j + 1 ][ 2 ] - gdata.vertices[ i ][ j ][ 2 ]);
                v = Triple(gdata.vertices[ i - 1 ][ j ][ 0 ] - gdata.vertices[ i ][ j ][ 0 ],
                           gdata.vertices[ i - 1 ][ j ][ 1 ] - gdata.vertices[ i ][ j ][ 1 ],
                           gdata.vertices[ i - 1 ][ j ][ 2 ] - gdata.vertices[ i ][ j ][ 2 ]);
                n += normalizedcross(u, v);
            }

            if (i > 0 && j > 0) {
                u = Triple(gdata.vertices[ i - 1 ][ j ][ 0 ] - gdata.vertices[ i ][ j ][ 0 ],
                           gdata.vertices[ i - 1 ][ j ][ 1 ] - gdata.vertices[ i ][ j ][ 1 ],
                           gdata.vertices[ i - 1 ][ j ][ 2 ] - gdata.vertices[ i ][ j ][ 2 ]);

                v = Triple(gdata.vertices[ i ][ j - 1 ][ 0 ] - gdata.vertices[ i ][ j ][ 0 ],
                           gdata.vertices[ i ][ j - 1 ][ 1 ] - gdata.vertices[ i ][ j ][ 1 ],
                           gdata.vertices[ i ][ j - 1 ][ 2 ] - gdata.vertices[ i ][ j ][ 2 ]);
                n += normalizedcross(u, v);
            }

            if (i < columns - 1 && j > 0) {
                u = Triple(gdata.vertices[ i ][ j - 1 ][ 0 ] - gdata.vertices[ i ][ j ][ 0 ],
                           gdata.vertices[ i ][ j - 1 ][ 1 ] - gdata.vertices[ i ][ j ][ 1 ],
                           gdata.vertices[ i ][ j - 1 ][ 2 ] - gdata.vertices[ i ][ j ][ 2 ]);

                v = Triple(gdata.vertices[ i + 1 ][ j ][ 0 ] - gdata.vertices[ i ][ j ][ 0 ],
                           gdata.vertices[ i + 1 ][ j ][ 1 ] - gdata.vertices[ i ][ j ][ 1 ],
                           gdata.vertices[ i + 1 ][ j ][ 2 ] - gdata.vertices[ i ][ j ][ 2 ]);
                n += normalizedcross(u, v);
            }
            n.normalize();

            gdata.normals[ i ][ j ][ 0 ] = n.x;
            gdata.normals[ i ][ j ][ 1 ] = n.y;
            gdata.normals[ i ][ j ][ 2 ] = n.z;
        }
    }
}

void SurfacePlot::sewPeriodic(Qwt3DGridData& gdata)
{
    // sewing

    Triple n;

    unsigned int columns = gdata.columns();
    unsigned int rows    = gdata.rows();

    if (gdata.uperiodic()) {
        for (unsigned i = 0; i != columns; ++i) {
            n = Triple(gdata.normals[ i ][ 0 ][ 0 ] + gdata.normals[ i ][ rows - 1 ][ 0 ],
                       gdata.normals[ i ][ 0 ][ 1 ] + gdata.normals[ i ][ rows - 1 ][ 1 ],
                       gdata.normals[ i ][ 0 ][ 2 ] + gdata.normals[ i ][ rows - 1 ][ 2 ]);

            n.normalize();
            gdata.normals[ i ][ 0 ][ 0 ] = gdata.normals[ i ][ rows - 1 ][ 0 ] = n.x;
            gdata.normals[ i ][ 0 ][ 1 ] = gdata.normals[ i ][ rows - 1 ][ 1 ] = n.y;
            gdata.normals[ i ][ 0 ][ 2 ] = gdata.normals[ i ][ rows - 1 ][ 2 ] = n.z;
        }
    }
    if (gdata.vperiodic()) {
        for (unsigned j = 0; j != rows; ++j) {
            n = Triple(gdata.normals[ 0 ][ j ][ 0 ] + gdata.normals[ columns - 1 ][ j ][ 0 ],
                       gdata.normals[ 0 ][ j ][ 1 ] + gdata.normals[ columns - 1 ][ j ][ 1 ],
                       gdata.normals[ 0 ][ j ][ 2 ] + gdata.normals[ columns - 1 ][ j ][ 2 ]);

            n.normalize();
            gdata.normals[ 0 ][ j ][ 0 ] = gdata.normals[ columns - 1 ][ j ][ 0 ] = n.x;
            gdata.normals[ 0 ][ j ][ 1 ] = gdata.normals[ columns - 1 ][ j ][ 1 ] = n.y;
            gdata.normals[ 0 ][ j ][ 2 ] = gdata.normals[ columns - 1 ][ j ][ 2 ] = n.z;
        }
    }
}

/*!
        Convert user grid data to internal vertex structure.
        See also NativeReader::read() and Function::create()
*/
bool SurfacePlot::loadFromData(Triple** data, unsigned int columns, unsigned int rows, bool uperiodic, bool vperiodic)
{
    QWT_D(d);
    d->m_actualDataC->clear();
    setActualData(d->m_actualDataG);

    readIn(*d->m_actualDataG, data, columns, rows);
    calcNormals(*d->m_actualDataG);
    d->m_actualDataG->setPeriodic(uperiodic, vperiodic);
    sewPeriodic(*d->m_actualDataG);

    updateData();
    updateNormals();
    createCoordinateSystem();

    return true;
}

/*!
        Convert user grid data to internal vertex structure.
        See also NativeReader::read() and Function::create()
*/
bool SurfacePlot::loadFromData(double** data, unsigned int columns, unsigned int rows, double minx, double maxx, double miny, double maxy)
{
    QWT_D(d);
    d->m_actualDataC->clear();
    setActualData(d->m_actualDataG);

    d->m_actualDataG->setPeriodic(false, false);
    d->m_actualDataG->setSize(columns, rows);
    readIn(*d->m_actualDataG, data, columns, rows, minx, maxx, miny, maxy);
    calcNormals(*d->m_actualDataG);

    updateData();
    updateNormals();
    createCoordinateSystem();

    return true;
}

void SurfacePlot::createFloorDataG()
{
    switch (floorStyle()) {
    case FLOORDATA:
        Data2FloorG();
        break;
    case FLOORISO:
        Isolines2FloorG();
        break;
    default:
        break;
    }
}

void SurfacePlot::Data2FloorG()
{
    QWT_D(d);
    Qwt3DData* data = actualData();
    if (!data || data->empty())
        return;

    int step = resolution();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_QUADS);

    double zshift = data->hull().minVertex.z;
    for (int i = 0; i < d->m_actualDataG->columns() - step; i += step) {
        glBegin(GL_TRIANGLE_STRIP);
        setColorFromVertexG(i, 0);
        glVertex3d(d->m_actualDataG->vertices[ i ][ 0 ][ 0 ], d->m_actualDataG->vertices[ i ][ 0 ][ 1 ], zshift);

        setColorFromVertexG(i + step, 0);
        glVertex3d(d->m_actualDataG->vertices[ i + step ][ 0 ][ 0 ], d->m_actualDataG->vertices[ i + step ][ 0 ][ 1 ], zshift);
        for (int j = 0; j < d->m_actualDataG->rows() - step; j += step) {
            setColorFromVertexG(i, j + step);
            glVertex3d(d->m_actualDataG->vertices[ i ][ j + step ][ 0 ],
                       d->m_actualDataG->vertices[ i ][ j + step ][ 1 ],
                       zshift);

            setColorFromVertexG(i + step, j + step);
            glVertex3d(d->m_actualDataG->vertices[ i + step ][ j + step ][ 0 ],
                       d->m_actualDataG->vertices[ i + step ][ j + step ][ 1 ],
                       zshift);
        }
        glEnd();
    }
}

void SurfacePlot::Isolines2FloorG()
{
    QWT_D(d);
    Qwt3DData* data = actualData();
    if (isolines() <= 0 || !data || data->empty())
        return;

    double count = (data->hull().maxVertex.z - data->hull().minVertex.z) / isolines();

    RGBA col;

    int step = resolution();

    double zshift = data->hull().minVertex.z;

    int cols = d->m_actualDataG->columns();
    int rows = d->m_actualDataG->rows();

    Triple t[ 4 ];
    vector< Triple > intersection;

    double lambda = 0;

    GLStateBewarer sb2(GL_LINE_SMOOTH, false);

    for (int k = 0; k != isolines(); ++k) {
        double val = zshift + k * count;

        for (int i = 0; i < cols - step; i += step) {
            for (int j = 0; j < rows - step; j += step) {
                t[ 0 ] = Triple(d->m_actualDataG->vertices[ i ][ j ][ 0 ],
                                d->m_actualDataG->vertices[ i ][ j ][ 1 ],
                                d->m_actualDataG->vertices[ i ][ j ][ 2 ]);

                col = (*dataColor())(t[ 0 ].x, t[ 0 ].y, t[ 0 ].z);
                glColor4d(col.r, col.g, col.b, col.a);

                t[ 1 ] = Triple(d->m_actualDataG->vertices[ i + step ][ j ][ 0 ],
                                d->m_actualDataG->vertices[ i + step ][ j ][ 1 ],
                                d->m_actualDataG->vertices[ i + step ][ j ][ 2 ]);
                t[ 2 ] = Triple(d->m_actualDataG->vertices[ i + step ][ j + step ][ 0 ],
                                d->m_actualDataG->vertices[ i + step ][ j + step ][ 1 ],
                                d->m_actualDataG->vertices[ i + step ][ j + step ][ 2 ]);
                t[ 3 ] = Triple(d->m_actualDataG->vertices[ i ][ j + step ][ 0 ],
                                d->m_actualDataG->vertices[ i ][ j + step ][ 1 ],
                                d->m_actualDataG->vertices[ i ][ j + step ][ 2 ]);

                double diff = 0;
                for (int m = 0; m != 4; ++m) {
                    int mm = (m + 1) % 4;
                    if ((val >= t[ m ].z && val <= t[ mm ].z) || (val >= t[ mm ].z && val <= t[ m ].z)) {
                        diff = t[ mm ].z - t[ m ].z;

                        if (isPracticallyZero(diff))  // degenerated
                        {
                            intersection.push_back(t[ m ]);
                            intersection.push_back(t[ mm ]);
                            continue;
                        }

                        lambda = (val - t[ m ].z) / diff;
                        intersection.push_back(Triple(
                            t[ m ].x + lambda * (t[ mm ].x - t[ m ].x), t[ m ].y + lambda * (t[ mm ].y - t[ m ].y), val));
                    }
                }

                if (!intersection.empty()) {
                    if (intersection.size() > 2) {
                        glBegin(GL_LINE_STRIP);
                        for (unsigned dd = 0; dd != intersection.size(); ++dd) {
                            glVertex3d(intersection[ dd ].x, intersection[ dd ].y, zshift);
                        }
                        glEnd();
                        glBegin(GL_POINTS);
                        glVertex3d(intersection[ 0 ].x, intersection[ 0 ].y, zshift);
                        glEnd();
                    } else if (intersection.size() == 2) {
                        glBegin(GL_LINES);
                        glVertex3d(intersection[ 0 ].x, intersection[ 0 ].y, zshift);
                        glVertex3d(intersection[ 1 ].x, intersection[ 1 ].y, zshift);

                        // small pixel gap problem (see OpenGL spec.)
                        glVertex3d(intersection[ 1 ].x, intersection[ 1 ].y, zshift);
                        glVertex3d(intersection[ 0 ].x, intersection[ 0 ].y, zshift);
                        glEnd();
                    }

                    intersection.clear();
                }
            }
        }
    }
}
