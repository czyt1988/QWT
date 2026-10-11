#ifndef QWT3D_WATERFALL_P_H
#define QWT3D_WATERFALL_P_H

#include "qwt3d_waterfall.h"
#include "qwt3d_types.h"
#include "qwt3d_color.h"

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QVector3D>
#include <QVector4D>

#include <vector>

#include <cfloat>

/**
 * @brief Vertex for the Ribbon/RibbonMesh/Wireframe styles (position + color, 28 bytes)
 * @details Matches the polygon/line shader attribute layout: location 0 =
 *          position, location 1 = color.
 */
struct WaterfallVertex
{
    QVector3D position;
    QVector4D color;
};

/**
 * @brief Vertex for the Tube style (position + normal + color, 40 bytes)
 * @details Matches the surface shader attribute layout: location 0 = position,
 *          location 1 = normal, location 2 = color.
 */
struct WaterfallTubeVertex
{
    QVector3D position;
    QVector3D normal;
    QVector4D color;
};

class Qwt3DWaterfall::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DWaterfall)

public:
    explicit PrivateData(Qwt3DWaterfall* q);
    ~PrivateData();

    // Data / sliding window
    //
    // Traces are stored at the front indices [0, m_count), ordered oldest to
    // newest. When the window is full a push rotates all slots down by one
    // (O(capacity) cheap QVector moves, no deep copy) and writes the new trace
    // into the last slot. The depth z of a trace is its slot index (possibly
    // mirrored by the depth order), so the hull z-range only grows.
    std::vector< QVector<double> > m_traces;
    int m_capacity;
    int m_count;
    int m_samplesPerTrace;
    QVector<double> m_bins;

    // Cached amplitude range over the stored traces
    double m_dataMin;
    double m_dataMax;

    Qwt3DWaterfall::DepthOrder m_depthOrder;

    // Style
    Qwt3DWaterfall::PlotStyle m_style;
    Qwt3DWaterfall::ColorMode m_colorMode;
    double m_opacity;
    bool m_fixedBaseline;
    double m_baseline;
    RGBA m_solidColor;
    RGBA m_outlineColor;
    double m_outlineWidth;

    // Fixed color range (ByAmplitude mode)
    bool m_fixedColorRange;
    double m_colorRangeMin;
    double m_colorRangeMax;

    // Color functor (owned)
    Qwt3DColor* m_dataColor;

    // GL resources (created lazily in draw())
    bool m_ribbonDirty;
    bool m_outlineDirty;
    bool m_tubeDirty;
    bool m_tubeShaderInitialized;
    QOpenGLBuffer m_ribbonBuffer;
    QOpenGLVertexArrayObject m_ribbonVAO;
    int m_ribbonVertexCount;   // total vertices, 2 * bins per slot
    QOpenGLBuffer m_outlineBuffer;
    QOpenGLVertexArrayObject m_outlineVAO;
    int m_outlineVertexCount;  // total vertices, bins per slot
    QOpenGLBuffer m_tubeVertexBuffer;
    QOpenGLBuffer m_tubeIndexBuffer;
    QOpenGLVertexArrayObject m_tubeVAO;
    QOpenGLShaderProgram m_tubeShader;  // Tube style reuses the surface shader
    int m_tubeIndexCount;

    // Cached hull
    ParallelEpiped m_hull;

    // Recomputes m_dataMin/m_dataMax and m_hull from the stored traces
    void recalculateRanges();
};

inline Qwt3DWaterfall::PrivateData::PrivateData(Qwt3DWaterfall* q)
    : q_ptr(q)
    , m_capacity(64)
    , m_count(0)
    , m_samplesPerTrace(0)
    , m_dataMin(0.0)
    , m_dataMax(0.0)
    , m_depthOrder(Qwt3DWaterfall::NewestInFront)
    , m_style(Qwt3DWaterfall::Ribbon)
    , m_colorMode(Qwt3DWaterfall::ByDepth)
    , m_opacity(1.0)
    , m_fixedBaseline(false)
    , m_baseline(0.0)
    , m_solidColor(RGBA(0.65, 0.65, 0.75, 1.0))
    , m_outlineColor(RGBA(0.15, 0.15, 0.15, 1.0))
    , m_outlineWidth(1.0)
    , m_fixedColorRange(false)
    , m_colorRangeMin(0.0)
    , m_colorRangeMax(1.0)
    , m_dataColor(nullptr)
    , m_ribbonDirty(true)
    , m_outlineDirty(true)
    , m_tubeDirty(true)
    , m_tubeShaderInitialized(false)
    , m_ribbonVertexCount(0)
    , m_outlineVertexCount(0)
    , m_tubeIndexCount(0)
    , m_hull(Triple(0, 0, 0), Triple(0, 0, 0))
{
    m_traces.resize(m_capacity);
    m_tubeIndexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
}

inline Qwt3DWaterfall::PrivateData::~PrivateData()
{
    m_ribbonBuffer.destroy();
    m_ribbonVAO.destroy();
    m_outlineBuffer.destroy();
    m_outlineVAO.destroy();
    m_tubeVertexBuffer.destroy();
    m_tubeIndexBuffer.destroy();
    m_tubeVAO.destroy();

    if (m_dataColor)
        m_dataColor->destroy();
}

#endif // QWT3D_WATERFALL_P_H
