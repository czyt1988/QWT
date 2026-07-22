#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4305)
#pragma warning(disable : 4786)
#endif

#include "qwt3d_surfaceplot_p.h"
#include "qwt3d_enrichment_std.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////////////
//
//     cell specific
//

void SurfacePlot::createDataC()
{
    QWT_D(d);
    createFloorDataC();

    if (plotStyle() == NOPLOT)
        return;

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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    int idx = 0;
    if (plotStyle() != WIREFRAME) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_QUADS);

        bool hl = (plotStyle() == HIDDENLINE);
        if (hl) {
            RGBA col = backgroundRGBAColor();
            glColor4d(col.r, col.g, col.b, col.a);
        }

        for (unsigned i = 0; i != d->m_actualDataC->cells.size(); ++i) {
            glBegin(GL_POLYGON);
            for (unsigned j = 0; j != d->m_actualDataC->cells[ i ].size(); ++j) {
                idx = d->m_actualDataC->cells[ i ][ j ];
                setColorFromVertexC(idx, hl);
                glVertex3d(
                    d->m_actualDataC->nodes[ idx ].x, d->m_actualDataC->nodes[ idx ].y, d->m_actualDataC->nodes[ idx ].z);
                glNormal3d(d->m_actualDataC->normals[ idx ].x,
                           d->m_actualDataC->normals[ idx ].y,
                           d->m_actualDataC->normals[ idx ].z);
            }
            glEnd();
        }
    }

    if (plotStyle() == FILLEDMESH || plotStyle() == WIREFRAME || plotStyle() == HIDDENLINE) {
        glColor4d(meshColor().r, meshColor().g, meshColor().b, meshColor().a);
        {
            for (unsigned i = 0; i != d->m_actualDataC->cells.size(); ++i) {
                glBegin(GL_LINE_LOOP);
                for (unsigned j = 0; j != d->m_actualDataC->cells[ i ].size(); ++j) {
                    idx = d->m_actualDataC->cells[ i ][ j ];
                    glVertex3d(d->m_actualDataC->nodes[ idx ].x,
                               d->m_actualDataC->nodes[ idx ].y,
                               d->m_actualDataC->nodes[ idx ].z);
                }
                glEnd();
            }
        }
    }
}

// ci = cell index
// cv = vertex index in cell ci
void SurfacePlot::setColorFromVertexC(int node, bool skip)
{
    QWT_D(d);
    if (skip)
        return;

    RGBA col = (*dataColor())(
        d->m_actualDataC->nodes[ node ].x, d->m_actualDataC->nodes[ node ].y, d->m_actualDataC->nodes[ node ].z);

    glColor4d(col.r, col.g, col.b, col.a);
}

void SurfacePlot::createFloorDataC()
{
    switch (floorStyle()) {
    case FLOORDATA:
        Data2FloorC();
        break;
    case FLOORISO:
        Isolines2FloorC();
        break;
    default:
        break;
    }
}

void SurfacePlot::Data2FloorC()
{
    QWT_D(d);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    double zshift = d->m_actualDataC->hull().minVertex.z;
    int idx;

    for (unsigned i = 0; i != d->m_actualDataC->cells.size(); ++i) {
        glBegin(GL_POLYGON);
        for (unsigned j = 0; j != d->m_actualDataC->cells[ i ].size(); ++j) {
            idx = d->m_actualDataC->cells[ i ][ j ];
            setColorFromVertexC(idx);
            glVertex3d(d->m_actualDataC->nodes[ idx ].x, d->m_actualDataC->nodes[ idx ].y, zshift);
        }
        glEnd();
    }
}

void SurfacePlot::Isolines2FloorC()
{
    QWT_D(d);
    Qwt3DData* data = actualData();
    if (isolines() <= 0 || !data || data->empty())
        return;

    double step = (data->hull().maxVertex.z - data->hull().minVertex.z) / isolines();

    RGBA col;

    double zshift = data->hull().minVertex.z;

    TripleField nodes;
    TripleField intersection;

    double lambda = 0;

    GLStateBewarer sb2(GL_LINE_SMOOTH, false);

    for (int k = 0; k != isolines(); ++k) {
        double val = zshift + k * step;

        for (unsigned i = 0; i != d->m_actualDataC->cells.size(); ++i) {
            nodes.clear();
            size_t cellnodes = d->m_actualDataC->cells[ i ].size();
            for (size_t j = 0; j < cellnodes; ++j) {
                nodes.push_back(d->m_actualDataC->nodes[ d->m_actualDataC->cells[ i ][ j ] ]);
            }

            double diff = 0;
            for (unsigned m = 0; m != cellnodes; ++m) {
                unsigned mm = (m + 1) % cellnodes;
                if ((val >= nodes[ m ].z && val <= nodes[ mm ].z) || (val >= nodes[ mm ].z && val <= nodes[ m ].z)) {
                    diff = nodes[ mm ].z - nodes[ m ].z;

                    if (isPracticallyZero(diff))  // degenerated
                    {
                        intersection.push_back(nodes[ m ]);
                        intersection.push_back(nodes[ mm ]);
                        continue;
                    }

                    lambda = (val - nodes[ m ].z) / diff;
                    intersection.push_back(Triple(nodes[ m ].x + lambda * (nodes[ mm ].x - nodes[ m ].x),
                                                  nodes[ m ].y + lambda * (nodes[ mm ].y - nodes[ m ].y),
                                                  val));
                }
            }

            if (!intersection.empty()) {
                col = (*dataColor())(nodes[ 0 ].x, nodes[ 0 ].y, nodes[ 0 ].z);
                glColor4d(col.r, col.g, col.b, col.a);
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

void SurfacePlot::createNormalsC()
{
    QWT_D(d);
    Qwt3DData* data = actualData();
    if (!normals() || !data || data->empty())
        return;

    if (d->m_actualDataC->nodes.size() != d->m_actualDataC->normals.size())
        return;
    Arrow arrow;
    arrow.setQuality(normalQuality());

    Triple basev, topv, norm;

    double diag = (data->hull().maxVertex - data->hull().minVertex).length() * normalLength();

    RGBA col;
    arrow.assign(*this);
    arrow.drawBegin();
    for (unsigned i = 0; i != d->m_actualDataC->normals.size(); ++i) {
        basev = d->m_actualDataC->nodes[ i ];
        topv  = basev + d->m_actualDataC->normals[ i ];

        norm = topv - basev;
        norm.normalize();
        norm *= diag;

        arrow.setTop(basev + norm);
        arrow.setColor((*dataColor())(basev.x, basev.y, basev.z));
        arrow.draw(basev);
    }
    arrow.drawEnd();
}

/*!
        Convert user (non-rectangular) mesh based data to internal structure.
        See also TripleField and CellField
*/
bool SurfacePlot::loadFromData(TripleField const& data, CellField const& poly)
{
    QWT_D(d);
    d->m_actualDataG->clear();
    setActualData(d->m_actualDataC);

    d->m_actualDataC->nodes   = data;
    d->m_actualDataC->cells   = poly;
    d->m_actualDataC->normals = TripleField(d->m_actualDataC->nodes.size());

    unsigned i;

    //  normals for the moment
    Triple n, u, v;
    for (i = 0; i < poly.size(); ++i) {
        if (poly[ i ].size() < 3)
            n = Triple(0, 0, 0);
        else {
            for (size_t j = 0; j < poly[ i ].size(); ++j) {
                size_t jj  = (j + 1) % poly[ i ].size();
                size_t pjj = (j) ? j - 1 : poly[ i ].size() - 1;
                u          = d->m_actualDataC->nodes[ poly[ i ][ jj ] ] - d->m_actualDataC->nodes[ poly[ i ][ j ] ];
                v          = d->m_actualDataC->nodes[ poly[ i ][ pjj ] ] - d->m_actualDataC->nodes[ poly[ i ][ j ] ];
                n          = normalizedcross(u, v);
                d->m_actualDataC->normals[ poly[ i ][ j ] ] += n;
            }
        }
    }
    for (i = 0; i != d->m_actualDataC->normals.size(); ++i) {
        d->m_actualDataC->normals[ i ].normalize();
    }

    ParallelEpiped hull(Triple(DBL_MAX, DBL_MAX, DBL_MAX), Triple(-DBL_MAX, -DBL_MAX, -DBL_MAX));

    for (i = 0; i != data.size(); ++i) {
        if (data[ i ].x < hull.minVertex.x)
            hull.minVertex.x = data[ i ].x;
        if (data[ i ].y < hull.minVertex.y)
            hull.minVertex.y = data[ i ].y;
        if (data[ i ].z < hull.minVertex.z)
            hull.minVertex.z = data[ i ].z;

        if (data[ i ].x > hull.maxVertex.x)
            hull.maxVertex.x = data[ i ].x;
        if (data[ i ].y > hull.maxVertex.y)
            hull.maxVertex.y = data[ i ].y;
        if (data[ i ].z > hull.maxVertex.z)
            hull.maxVertex.z = data[ i ].z;
    }

    d->m_actualDataC->setHull(hull);

    updateData();
    updateNormals();
    createCoordinateSystem();

    return true;
}
