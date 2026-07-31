#ifndef QWT3D_LINE3D_P_H
#define QWT3D_LINE3D_P_H

#include "qwt3d_line3d.h"
#include "qwt3d_types.h"
#include "qwt3d_color.h"

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QVector3D>
#include <QVector4D>

#include "qwt_series_data.h"
#include "qwt_point_3d.h"

#include <cfloat>

/**
 * @brief Vertex for the Tube style (position + normal + color, 40 bytes)
 * @details Matches the surface shader attribute layout: location 0 = position,
 *          location 1 = normal, location 2 = color.
 */
struct LineTubeVertex
{
    QVector3D position;
    QVector3D normal;
    QVector4D color;
};

/**
 * @brief Vertex for the Lines/Dots styles (position + color, 28 bytes)
 * @details Matches the line/point shader attribute layout: location 0 =
 *          position, location 1 = color.
 */
struct LinePointVertex
{
    QVector3D position;
    QVector4D color;
};

class Qwt3DLine::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DLine)

public:
    explicit PrivateData(Qwt3DLine* q);
    ~PrivateData();

    // Data
    QwtSeriesData<QwtPoint3D>* m_series;

    // Style
    Qwt3DLine::LineStyle m_style;
    double m_lineWidth;
    double m_tubeRadius;  // <= 0 means auto
    int m_tubeSegments;
    double m_pointSize;
    bool m_pointVisible;
    Qwt3DLine::PointShape m_pointShape;

    // Color
    RGBA m_solidColor;
    Qwt3DColor* m_dataColor;

    // GL resources (created lazily in draw())
    bool m_vboDirty;
    bool m_pointsDirty;
    bool m_tubeShaderInitialized;
    QOpenGLBuffer m_vertexBuffer;       // tube vertices (Tube style)
    QOpenGLBuffer m_indexBuffer;        // tube side indices (Tube style)
    QOpenGLVertexArrayObject m_vao;     // tube VAO (Tube style)
    QOpenGLShaderProgram m_tubeShader;  // Tube style reuses the surface shader

    // Separate points VBO/VAO for the Dots style and the point-marker overlay
    QOpenGLBuffer m_pointsBuffer;
    QOpenGLVertexArrayObject m_pointsVAO;

    // Shape marker VBO/VAO/EBO (used when m_pointShape != Dot)
    QOpenGLBuffer m_markerVertexBuffer;
    QOpenGLBuffer m_markerIndexBuffer;
    QOpenGLVertexArrayObject m_markerVAO;
    bool m_markersDirty;
    int m_markerIndexCount;

    // Counts
    int m_vertexCount;     // ring vertices (Tube) or polyline vertices (Lines/Dots)
    int m_indexCount;      // tube side indices (Tube style only)
    int m_pointCount;      // number of polyline sample points

    // Cached hull
    ParallelEpiped m_hull;
};

inline Qwt3DLine::PrivateData::PrivateData(Qwt3DLine* q)
    : q_ptr(q)
    , m_series(new QwtPoint3DSeriesData())
    , m_style(Qwt3DLine::Lines)
    , m_lineWidth(1.0)
    , m_tubeRadius(-1.0)
    , m_tubeSegments(8)
    , m_pointSize(8.0)
    , m_pointVisible(false)
    , m_pointShape(Qwt3DLine::Dot)
    , m_solidColor(RGBA(0.9, 0.9, 0.9, 1.0))
    , m_dataColor(nullptr)
    , m_vboDirty(true)
    , m_pointsDirty(true)
    , m_tubeShaderInitialized(false)
    , m_markersDirty(true)
    , m_vertexCount(0)
    , m_indexCount(0)
    , m_pointCount(0)
    , m_markerIndexCount(0)
    , m_hull(Triple(0, 0, 0), Triple(0, 0, 0))
{
    m_indexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    m_markerIndexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
}

inline Qwt3DLine::PrivateData::~PrivateData()
{
    m_vertexBuffer.destroy();
    m_indexBuffer.destroy();
    m_vao.destroy();
    m_pointsBuffer.destroy();
    m_pointsVAO.destroy();
    m_markerVertexBuffer.destroy();
    m_markerIndexBuffer.destroy();
    m_markerVAO.destroy();

    if (m_series)
        delete m_series;
    if (m_dataColor)
        m_dataColor->destroy();
}

#endif // QWT3D_LINE3D_P_H
