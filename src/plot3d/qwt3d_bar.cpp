#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4305)
#pragma warning(disable : 4786)
#endif

#include "qwt3d_bar_p.h"
#include "qwt3d_plot.h"
#include "qwt3d_color.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QMatrix3x3>

#include <cfloat>
#include <cmath>
#include <algorithm>
#include <vector>

// ---------------------------------------------------------------------------
// Anonymous namespace: bar box geometry helpers
// ---------------------------------------------------------------------------

namespace
{
/// Number of faces, vertices and indices per bar
constexpr int kFacesPerBar = 6;
constexpr int kVertsPerFace = 4;
constexpr int kTrisPerFace = 6;   // 2 triangles = 6 indices
constexpr int kLinesPerFace = 8;  // 4 perimeter edges = 8 indices

/// Eight corners of an axis-aligned box, labeled by (x,y,z) bit flags
struct BoxCorners
{
    QVector3D c[8];
};

/// Builds the eight corners of a bar from its specification
BoxCorners barCorners(const Qwt3DBarSpec& s)
{
    const float hw = static_cast<float>(s.halfWidth);
    const float hd = static_cast<float>(s.halfDepth);
    const float cx = static_cast<float>(s.center.x);
    const float cy = static_cast<float>(s.center.y);
    const float z0 = static_cast<float>(s.baseZ);
    const float z1 = static_cast<float>(s.topZ);

    BoxCorners b;
    b.c[0] = QVector3D(cx - hw, cy - hd, z0); // (x0,y0,z0)
    b.c[1] = QVector3D(cx + hw, cy - hd, z0); // (x1,y0,z0)
    b.c[2] = QVector3D(cx + hw, cy + hd, z0); // (x1,y1,z0)
    b.c[3] = QVector3D(cx - hw, cy + hd, z0); // (x0,y1,z0)
    b.c[4] = QVector3D(cx - hw, cy - hd, z1); // (x0,y0,z1)
    b.c[5] = QVector3D(cx + hw, cy - hd, z1); // (x1,y0,z1)
    b.c[6] = QVector3D(cx + hw, cy + hd, z1); // (x1,y1,z1)
    b.c[7] = QVector3D(cx - hw, cy + hd, z1); // (x0,y1,z1)
    return b;
}

/// One face: four corner indices in perimeter order plus the outward normal
struct FaceDef
{
    int corners[kVertsPerFace];
    QVector3D normal;
};

/// The six box faces (perimeter order, outward normals)
const FaceDef& faceDef(int i)
{
    static const FaceDef faces[kFacesPerBar] = {
        { { 0, 3, 2, 1 }, QVector3D(0, 0, -1) }, // bottom (-Z)
        { { 4, 5, 6, 7 }, QVector3D(0, 0, 1) },   // top    (+Z)
        { { 0, 4, 7, 3 }, QVector3D(-1, 0, 0) }, // -X
        { { 1, 2, 6, 5 }, QVector3D(1, 0, 0) },  // +X
        { { 0, 1, 5, 4 }, QVector3D(0, -1, 0) }, // -Y
        { { 3, 7, 6, 2 }, QVector3D(0, 1, 0) }   // +Y
    };
    return faces[i];
}

/// Appends one bar (6 faces, 24 vertices) to the vertex/index vectors
void appendBar(QVector<BarVertex>& vertices,
               QVector<unsigned int>& triIndices,
               QVector<unsigned int>& lineIndices,
               const Qwt3DBarSpec& s, const QVector4D& color)
{
    const BoxCorners box = barCorners(s);
    const unsigned int base = static_cast<unsigned int>(vertices.size());

    for (int f = 0; f < kFacesPerBar; ++f) {
        const FaceDef& fd = faceDef(f);
        const unsigned int fb = base + static_cast<unsigned int>(f * kVertsPerFace);

        for (int v = 0; v < kVertsPerFace; ++v) {
            BarVertex vtx;
            vtx.position = box.c[fd.corners[v]];
            vtx.normal = fd.normal;
            vtx.color = color;
            vertices.append(vtx);
        }
        // Two triangles tiling the quad: (0,1,2) + (0,2,3)
        triIndices.append(fb + 0);
        triIndices.append(fb + 1);
        triIndices.append(fb + 2);
        triIndices.append(fb + 0);
        triIndices.append(fb + 2);
        triIndices.append(fb + 3);
        // Four perimeter edges
        for (int v = 0; v < kVertsPerFace; ++v) {
            lineIndices.append(fb + static_cast<unsigned int>(v));
            lineIndices.append(fb + static_cast<unsigned int>((v + 1) % kVertsPerFace));
        }
    }
}

/// Computes the minimum positive spacing of a sorted-unique coordinate set
double minPositiveSpacing(const std::vector<double>& coords)
{
    if (coords.size() < 2)
        return 0.0;
    std::vector<double> s(coords);
    std::sort(s.begin(), s.end());
    s.erase(std::unique(s.begin(), s.end(), [](double a, double b) { return std::abs(a - b) < 1e-12; }),
            s.end());
    double minDiff = DBL_MAX;
    for (size_t i = 1; i < s.size(); ++i) {
        const double d = s[i] - s[i - 1];
        if (d > 1e-12 && d < minDiff)
            minDiff = d;
    }
    return minDiff == DBL_MAX ? 0.0 : minDiff;
}

/// Resolves the (halfWidth, halfDepth) pair from explicit settings or auto spacing
std::pair<double, double> resolveDims(double barW, double barD,
                                      double spacingX, double spacingY)
{
    double hw = (barW > 0.0) ? barW * 0.5 : (spacingX > 0.0 ? 0.4 * spacingX : 0.5);
    double hd = (barD > 0.0) ? barD * 0.5 : (spacingY > 0.0 ? 0.4 * spacingY : hw);
    return { hw, hd };
}

/// Recomputes the cached hull from the current bar specifications
ParallelEpiped computeHull(const std::vector<Qwt3DBarSpec>& bars)
{
    if (bars.empty())
        return ParallelEpiped(Triple(0, 0, 0), Triple(0, 0, 0));

    ParallelEpiped h(Triple(DBL_MAX, DBL_MAX, DBL_MAX),
                     Triple(-DBL_MAX, -DBL_MAX, -DBL_MAX));
    for (const auto& s : bars) {
        h.minVertex.x = std::min(h.minVertex.x, s.center.x - s.halfWidth);
        h.minVertex.y = std::min(h.minVertex.y, s.center.y - s.halfDepth);
        h.minVertex.z = std::min(h.minVertex.z, s.baseZ);
        h.maxVertex.x = std::max(h.maxVertex.x, s.center.x + s.halfWidth);
        h.maxVertex.y = std::max(h.maxVertex.y, s.center.y + s.halfDepth);
        h.maxVertex.z = std::max(h.maxVertex.z, s.topZ);
    }
    return h;
}
} // namespace

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

Qwt3DBar::Qwt3DBar()
    : Qwt3DPlotItem()
    , QWT_PIMPL_CONSTRUCT
{
}

Qwt3DBar::~Qwt3DBar() = default;

// ---------------------------------------------------------------------------
// Data loading
// ---------------------------------------------------------------------------

void Qwt3DBar::setSamples(const QVector<QwtPoint3D>& samples)
{
    QWT_D(d);
    d->m_bars.clear();

    std::vector<double> xs, ys;
    xs.reserve(samples.size());
    ys.reserve(samples.size());
    for (const auto& p : samples) {
        xs.push_back(p.x());
        ys.push_back(p.y());
    }

    const double spx = minPositiveSpacing(xs);
    const double spy = minPositiveSpacing(ys);
    auto dims = resolveDims(d->m_barWidth, d->m_barDepth, spx, spy);

    for (const auto& p : samples) {
        Qwt3DBarSpec s;
        s.center = Triple(p.x(), p.y(), 0.0);
        s.halfWidth = dims.first;
        s.halfDepth = dims.second;
        const double topZ = d->m_baseline + p.z();
        s.baseZ = std::min(d->m_baseline, topZ);
        s.topZ = std::max(d->m_baseline, topZ);
        d->m_bars.push_back(s);
    }

    d->m_hull = computeHull(d->m_bars);
    d->m_vboDirty = true;
    itemChanged();
}

void Qwt3DBar::setSamples(const QVector<double>& x, const QVector<double>& heights)
{
    QWT_D(d);

    const int n = std::min(x.size(), heights.size());
    QVector<QwtPoint3D> samples;
    samples.reserve(n);
    for (int i = 0; i < n; ++i)
        samples.append(QwtPoint3D(x[i], 0.0, heights[i]));
    setSamples(samples);
}

void Qwt3DBar::setSamples(double** z, int columns, int rows,
                          double minX, double maxX, double minY, double maxY)
{
    QWT_D(d);
    d->m_bars.clear();

    const double dx = (columns > 1) ? (maxX - minX) / (columns - 1) : 1.0;
    const double dy = (rows > 1) ? (maxY - minY) / (rows - 1) : 1.0;
    auto dims = resolveDims(d->m_barWidth, d->m_barDepth, dx, dy);

    for (int i = 0; i < columns; ++i) {
        for (int j = 0; j < rows; ++j) {
            Qwt3DBarSpec s;
            s.center = Triple(minX + i * dx, minY + j * dy, 0.0);
            s.halfWidth = dims.first;
            s.halfDepth = dims.second;
            const double topZ = d->m_baseline + z[i][j];
            s.baseZ = std::min(d->m_baseline, topZ);
            s.topZ = std::max(d->m_baseline, topZ);
            d->m_bars.push_back(s);
        }
    }

    d->m_hull = computeHull(d->m_bars);
    d->m_vboDirty = true;
    itemChanged();
}

void Qwt3DBar::setSamples(const Qwt3DFunctionData& data)
{
    QWT_D(d);

    if (data.columns == 0 || data.rows == 0)
        return;
    if (data.z.size() < data.columns)
        return;

    // Build a temporary pointer array from the vector-of-vectors
    std::vector<double*> ptrs(data.columns);
    for (unsigned int i = 0; i < data.columns; ++i)
        ptrs[i] = const_cast<double*>(data.z[i].data());

    setSamples(ptrs.data(), static_cast<int>(data.columns), static_cast<int>(data.rows),
               data.minx, data.maxx, data.miny, data.maxy);
}

// ---------------------------------------------------------------------------
// Bar footprint, baseline, style and color
// ---------------------------------------------------------------------------

double Qwt3DBar::barWidth() const
{
    QWT_DC(d);
    return d->m_barWidth;
}

void Qwt3DBar::setBarWidth(double w)
{
    QWT_D(d);
    if (d->m_barWidth == w)
        return;
    d->m_barWidth = w;
    d->m_vboDirty = true;
    itemChanged();
}

double Qwt3DBar::barDepth() const
{
    QWT_DC(d);
    return d->m_barDepth;
}

void Qwt3DBar::setBarDepth(double dpt)
{
    QWT_D(d);
    if (d->m_barDepth == dpt)
        return;
    d->m_barDepth = dpt;
    d->m_vboDirty = true;
    itemChanged();
}

double Qwt3DBar::baseline() const
{
    QWT_DC(d);
    return d->m_baseline;
}

void Qwt3DBar::setBaseline(double z)
{
    QWT_D(d);
    if (d->m_baseline == z)
        return;
    d->m_baseline = z;
    d->m_vboDirty = true;
    itemChanged();
}

Qwt3DBar::BarStyle Qwt3DBar::barStyle() const
{
    QWT_DC(d);
    return d->m_barStyle;
}

void Qwt3DBar::setBarStyle(BarStyle style)
{
    QWT_D(d);
    if (d->m_barStyle == style)
        return;
    d->m_barStyle = style;
    itemChanged();
}

void Qwt3DBar::setDataColor(Qwt3DColor* color)
{
    QWT_D(d);
    if (d->m_dataColor)
        d->m_dataColor->destroy();
    d->m_dataColor = color;
    d->m_vboDirty = true;
    itemChanged();
}

const Qwt3DColor* Qwt3DBar::dataColor() const
{
    QWT_DC(d);
    return d->m_dataColor;
}

void Qwt3DBar::invalidateColors()
{
    QWT_D(d);
    d->m_vboDirty = true;
    itemChanged();
}

RGBA Qwt3DBar::meshColor() const
{
    QWT_DC(d);
    return d->m_meshColor;
}

void Qwt3DBar::setMeshColor(RGBA color)
{
    QWT_D(d);
    d->m_meshColor = color;
    itemChanged();
}

double Qwt3DBar::meshLineWidth() const
{
    QWT_DC(d);
    return d->m_meshLineWidth;
}

void Qwt3DBar::setMeshLineWidth(double width)
{
    QWT_D(d);
    d->m_meshLineWidth = width;
    itemChanged();
}

void Qwt3DBar::pushColorRange() const
{
    QWT_DC(d);
    if (!d->m_dataColor || !plot())
        return;
    const ParallelEpiped h = plot()->hull();
    d->m_dataColor->setActiveRange(h.minVertex.z, h.maxVertex.z);
}

void Qwt3DBar::populateLegendColors(ColorVector& colors) const
{
    QWT_DC(d);
    if (d->m_dataColor) {
        pushColorRange();
        d->m_dataColor->createVector(colors);
    }
}

// ---------------------------------------------------------------------------
// Qwt3DPlotItem interface
// ---------------------------------------------------------------------------

ParallelEpiped Qwt3DBar::hull() const
{
    QWT_DC(d);
    return d->m_hull;
}

// ---------------------------------------------------------------------------
// GL rendering
// ---------------------------------------------------------------------------

void Qwt3DBar::buildVBO()
{
    QWT_D(d);

    QVector<BarVertex> vertices;
    QVector<unsigned int> triIndices;
    QVector<unsigned int> lineIndices;

    // Ensure we have a data color functor
    if (!d->m_dataColor) {
        if (plot()) {
            d->m_dataColor = new Qwt3DStandardColor();
        } else {
            return;
        }
    }

    // Push the plot's union z-range into the color functor before querying it
    pushColorRange();

    for (const auto& s : d->m_bars) {
        RGBA col = (*d->m_dataColor)(s.center.x, s.center.y, s.topZ);
        const QVector4D color(static_cast<float>(col.r),
                              static_cast<float>(col.g),
                              static_cast<float>(col.b),
                              static_cast<float>(col.a));
        appendBar(vertices, triIndices, lineIndices, s, color);
    }

    d->m_vertexCount = vertices.size();
    d->m_triIndexCount = triIndices.size();
    d->m_lineIndexCount = lineIndices.size();

    if (vertices.isEmpty())
        return;

    // Get OpenGL functions
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    if (!f)
        return;

    // Create/bind VAO
    if (!d->m_vao.isCreated())
        d->m_vao.create();
    d->m_vao.bind();

    // Create/bind VBO and upload vertices
    if (!d->m_vertexBuffer.isCreated())
        d->m_vertexBuffer.create();
    d->m_vertexBuffer.bind();
    d->m_vertexBuffer.allocate(vertices.constData(),
                               vertices.size() * sizeof(BarVertex));

    // Configure vertex attributes (layout locations match the surface shader)
    const int stride = sizeof(BarVertex);
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                             reinterpret_cast<void*>(sizeof(QVector3D)));
    f->glEnableVertexAttribArray(2);
    f->glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
                             reinterpret_cast<void*>(2 * sizeof(QVector3D)));

    // Create/bind EBO and upload combined indices (tri + line)
    if (!d->m_indexBuffer.isCreated())
        d->m_indexBuffer.create();
    d->m_indexBuffer.bind();

    const int totalIndexCount = triIndices.size() + lineIndices.size();
    d->m_indexBuffer.allocate(totalIndexCount * sizeof(unsigned int));
    if (!triIndices.isEmpty())
        d->m_indexBuffer.write(0, triIndices.constData(),
                               triIndices.size() * sizeof(unsigned int));
    if (!lineIndices.isEmpty()) {
        d->m_indexBuffer.write(triIndices.size() * sizeof(unsigned int),
                               lineIndices.constData(),
                               lineIndices.size() * sizeof(unsigned int));
    }

    d->m_vao.release();
}

void Qwt3DBar::draw()
{
    QWT_D(d);

    if (!plot() || !plot()->initializedGL())
        return;

    // Build VBO if dirty
    if (d->m_vboDirty) {
        buildVBO();
        d->m_vboDirty = false;
    }

    if (d->m_vertexCount == 0)
        return;

    // Initialize shader on first use (reuses the lit surface shader)
    if (!d->m_shaderInitialized) {
        d->m_shader.addShaderFromSourceFile(
            QOpenGLShader::Vertex, ":/shaders/surface.vert");
        d->m_shader.addShaderFromSourceFile(
            QOpenGLShader::Fragment, ":/shaders/surface.frag");
        if (!d->m_shader.link())
            return;
        d->m_shaderInitialized = true;
    }

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    if (!f)
        return;

    // Bind shader and set uniforms
    d->m_shader.bind();

    const QMatrix4x4 mv = plot()->modelViewMatrix();
    const QMatrix4x4 proj = plot()->projectionMatrix();
    const QMatrix3x3 normalMatrix = mv.normalMatrix();

    d->m_shader.setUniformValue("uModelView", mv);
    d->m_shader.setUniformValue("uProjection", proj);
    d->m_shader.setUniformValue("uNormalMatrix", normalMatrix);

    const bool useLighting = plot()->lightingEnabled();
    d->m_shader.setUniformValue("uUseLighting", useLighting);
    if (useLighting) {
        d->m_shader.setUniformValue("uLightPos", QVector3D(0.0f, 0.0f, 10.0f));
        d->m_shader.setUniformValue("uLightColor", QVector3D(1.0f, 1.0f, 1.0f));
        d->m_shader.setUniformValue("uShininess", 32.0f);
    }

    // Bind VAO (binds VBO + EBO + vertex attribute config)
    d->m_vao.bind();

    const size_t triOffset = 0;
    const size_t lineOffset = static_cast<size_t>(d->m_triIndexCount) * sizeof(unsigned int);

    switch (d->m_barStyle) {
    case Filled:
        d->m_shader.setUniformValue("uUseOverrideColor", false);
        if (d->m_triIndexCount > 0)
            f->glDrawElements(GL_TRIANGLES, d->m_triIndexCount,
                              GL_UNSIGNED_INT, reinterpret_cast<void*>(triOffset));
        break;

    case FilledMesh:
        // Pass 1: filled bars with vertex colors
        d->m_shader.setUniformValue("uUseOverrideColor", false);
        if (d->m_triIndexCount > 0) {
            f->glEnable(GL_POLYGON_OFFSET_FILL);
            f->glPolygonOffset(static_cast<float>(d->m_polygonOffset), 1.0f);
            f->glDrawElements(GL_TRIANGLES, d->m_triIndexCount,
                              GL_UNSIGNED_INT, reinterpret_cast<void*>(triOffset));
        }
        // Pass 2: edge lines with mesh color
        d->m_shader.setUniformValue("uUseOverrideColor", true);
        d->m_shader.setUniformValue("uOverrideColor",
                                    QVector4D(static_cast<float>(d->m_meshColor.r),
                                              static_cast<float>(d->m_meshColor.g),
                                              static_cast<float>(d->m_meshColor.b),
                                              static_cast<float>(d->m_meshColor.a)));
        if (d->m_lineIndexCount > 0) {
            f->glLineWidth(static_cast<float>(d->m_meshLineWidth));
            f->glDrawElements(GL_LINES, d->m_lineIndexCount,
                              GL_UNSIGNED_INT, reinterpret_cast<void*>(lineOffset));
        }
        break;

    case Wireframe:
        d->m_shader.setUniformValue("uUseOverrideColor", true);
        d->m_shader.setUniformValue("uOverrideColor",
                                    QVector4D(static_cast<float>(d->m_meshColor.r),
                                              static_cast<float>(d->m_meshColor.g),
                                              static_cast<float>(d->m_meshColor.b),
                                              static_cast<float>(d->m_meshColor.a)));
        if (d->m_lineIndexCount > 0) {
            f->glLineWidth(static_cast<float>(d->m_meshLineWidth));
            f->glDrawElements(GL_LINES, d->m_lineIndexCount,
                              GL_UNSIGNED_INT, reinterpret_cast<void*>(lineOffset));
        }
        break;
    }

    d->m_vao.release();
    d->m_shader.release();
}
