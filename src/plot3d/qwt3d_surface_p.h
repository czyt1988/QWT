#ifndef QWT3D_SURFACE_P_H
#define QWT3D_SURFACE_P_H

#include "qwt3d_surface.h"
#include "qwt3d_types.h"

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix3x3>

#include <list>
#include <cfloat>

/**
 * @brief Vertex structure for VBO upload
 * @details Each vertex has position (3 floats), normal (3 floats), and color (4 floats).
 *          Total stride = 40 bytes, matching the shader attribute layout:
 *          location 0 = position, location 1 = normal, location 2 = color.
 */
struct SurfaceVertex
{
    QVector3D position;
    QVector3D normal;
    QVector4D color;
};

class Qwt3DSurface::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DSurface)

public:
    explicit PrivateData(Qwt3DSurface* q);
    ~PrivateData();

    // Data storage
    Qwt3DGridData* m_actualDataG;
    Qwt3DCellData* m_actualDataC;

    // Normals display
    bool m_dataNormals;
    double m_normalLength;
    int m_normalQuality;

    // Resolution (index buffer stepping)
    int m_resolution;

    // Floor projection style
    FLOORSTYLE m_floorStyle;

    // Plot style and color
    PLOTSTYLE m_plotStyle;
    SHADINGSTYLE m_shading;
    Qwt3DColor* m_dataColor;
    RGBA m_meshColor;
    double m_meshLineWidth;
    int m_isolines;
    bool m_smoothDataMesh;
    double m_polygonOffset;

    // Enrichments
    std::list<Qwt3DEnrichment*> m_enrichmentList;

    // GL resources (created lazily in draw())
    bool m_vboDirty;
    bool m_shaderInitialized;
    QOpenGLBuffer m_vertexBuffer;
    QOpenGLBuffer m_indexBuffer;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLShaderProgram m_shader;

    // Index counts (tri indices first, then line indices in the same EBO)
    int m_triIndexCount;
    int m_lineIndexCount;
    int m_vertexCount;

    // Cached hull
    ParallelEpiped m_hull;
};

inline Qwt3DSurface::PrivateData::PrivateData(Qwt3DSurface* q)
    : q_ptr(q)
    , m_actualDataG(nullptr)
    , m_actualDataC(nullptr)
    , m_dataNormals(false)
    , m_normalLength(0.02)
    , m_normalQuality(3)
    , m_resolution(1)
    , m_floorStyle(NOFLOOR)
    , m_plotStyle(FILLEDMESH)
    , m_shading(GOURAUD)
    , m_dataColor(nullptr)
    , m_meshColor(RGBA(0.0, 0.0, 0.0, 1.0))
    , m_meshLineWidth(1.0)
    , m_isolines(0)
    , m_smoothDataMesh(false)
    , m_polygonOffset(0.5)
    , m_vboDirty(true)
    , m_shaderInitialized(false)
    , m_triIndexCount(0)
    , m_lineIndexCount(0)
    , m_vertexCount(0)
    , m_hull(Triple(0, 0, 0), Triple(0, 0, 0))
{
    m_actualDataG = new Qwt3DGridData();
    m_actualDataC = new Qwt3DCellData();

    // QOpenGLBuffer with default target GL_ARRAY_BUFFER
    m_indexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
}

inline Qwt3DSurface::PrivateData::~PrivateData()
{
    // Destroy GL resources (safe to call even without current context)
    m_vertexBuffer.destroy();
    m_indexBuffer.destroy();
    m_vao.destroy();

    // Clean up data
    delete m_actualDataG;
    delete m_actualDataC;

    // Clean up color functor
    if (m_dataColor) {
        m_dataColor->destroy();
    }

    // Clean up enrichments
    for (auto* e : m_enrichmentList)
        delete e;
    m_enrichmentList.clear();
}

#endif // QWT3D_SURFACE_P_H
