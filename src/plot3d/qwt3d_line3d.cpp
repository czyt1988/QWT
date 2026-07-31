#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4305)
#pragma warning(disable : 4786)
#endif

#include "qwt3d_line3d_p.h"
#include "qwt3d_plot.h"
#include "qwt3d_color.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QMatrix3x3>

#include <cfloat>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Anonymous namespace: tube geometry and hull helpers
// ---------------------------------------------------------------------------

namespace
{
/// Clamps a float to [lo, hi]
float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/// Builds tube (swept circle) geometry from a polyline using parallel-transport
/// framing. Emits ring vertices (position + outward radial normal + color) and
/// side indices connecting adjacent rings.
void buildTubeGeometry(const QVector<QVector3D>& pts,
                       const QVector<QVector4D>& colors,
                       float radius, int segments,
                       QVector<LineTubeVertex>& vertices,
                       QVector<unsigned int>& indices)
{
    const int N = pts.size();
    if (N < 2 || segments < 3 || radius <= 0.0f)
        return;

    // 1. Tangents (central differences, single-sided at the ends)
    QVector<QVector3D> T(N);
    for (int i = 0; i < N; ++i) {
        QVector3D t;
        if (i == 0)
            t = pts[1] - pts[0];
        else if (i == N - 1)
            t = pts[N - 1] - pts[N - 2];
        else
            t = pts[i + 1] - pts[i - 1];
        const float len = t.length();
        if (len < 1e-6f) {
            // Duplicate point: carry a neighbor tangent to keep a frame
            t = (i > 0) ? T[i - 1] : QVector3D(1.0f, 0.0f, 0.0f);
        } else {
            t /= len;
        }
        T[i] = t;
    }

    // 2. Frames via parallel transport (robust on straight segments, unlike Frenet)
    QVector<QVector3D> Nrm(N), Bnm(N);
    {
        const QVector3D up = (std::abs(T[0].x()) < 0.9f)
            ? QVector3D(1.0f, 0.0f, 0.0f)
            : QVector3D(0.0f, 1.0f, 0.0f);
        QVector3D n0 = up - T[0] * QVector3D::dotProduct(up, T[0]);
        const float nl = n0.length();
        Nrm[0] = (nl > 1e-6f) ? n0 / nl : QVector3D(0.0f, 1.0f, 0.0f);
        Bnm[0] = QVector3D::crossProduct(T[0], Nrm[0]).normalized();
    }
    for (int i = 1; i < N; ++i) {
        QVector3D axis = QVector3D::crossProduct(T[i - 1], T[i]);
        const float cosA = clampf(QVector3D::dotProduct(T[i - 1], T[i]), -1.0f, 1.0f);
        QVector3D n;
        if (axis.lengthSquared() < 1e-12f) {
            // Parallel tangents: no rotation needed
            n = Nrm[i - 1];
        } else {
            axis.normalize();
            const float sinA = std::sqrt(std::max(0.0f, 1.0f - cosA * cosA));
            const QVector3D v = Nrm[i - 1];
            // Rodrigues rotation around axis by angle = acos(cosA)
            n = v * cosA
                + QVector3D::crossProduct(axis, v) * sinA
                + axis * (QVector3D::dotProduct(axis, v) * (1.0f - cosA));
        }
        // Re-orthogonalize against the new tangent, then compute binormal
        n = n - T[i] * QVector3D::dotProduct(n, T[i]);
        const float nl = n.length();
        Nrm[i] = (nl > 1e-6f) ? n / nl : Nrm[i - 1];
        Bnm[i] = QVector3D::crossProduct(T[i], Nrm[i]).normalized();
    }

    // 3. Ring vertices (one ring per polyline point)
    vertices.reserve(N * segments);
    for (int i = 0; i < N; ++i) {
        const QVector4D& col = colors.value(i, QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
        for (int k = 0; k < segments; ++k) {
            const float ang = float(2.0 * Qwt3D_PI * k / segments);
            const float ca = std::cos(ang);
            const float sa = std::sin(ang);
            const QVector3D radial = Nrm[i] * ca + Bnm[i] * sa;  // outward normal
            LineTubeVertex v;
            v.position = pts[i] + radial * radius;
            v.normal = radial;
            v.color = col;
            vertices.append(v);
        }
    }

    // 4. Side indices: each ring pair -> a quad strip of 'segments' quads
    indices.reserve((N - 1) * segments * 6);
    for (int i = 0; i < N - 1; ++i) {
        for (int k = 0; k < segments; ++k) {
            const int k1 = (k + 1) % segments;
            const unsigned int a = static_cast<unsigned int>(i * segments + k);
            const unsigned int b = static_cast<unsigned int>(i * segments + k1);
            const unsigned int c = static_cast<unsigned int>((i + 1) * segments + k);
            const unsigned int dd = static_cast<unsigned int>((i + 1) * segments + k1);
            // Quad (a, b, dd, c) split into two triangles
            indices.append(a);
            indices.append(b);
            indices.append(c);
            indices.append(b);
            indices.append(dd);
            indices.append(c);
        }
    }
}

/// Computes the bounding hull of a 3D point series
ParallelEpiped computeHullFromSeries(const QwtSeriesData<QwtPoint3D>* s)
{
    const size_t n = s ? s->size() : 0;
    if (n == 0)
        return ParallelEpiped(Triple(0, 0, 0), Triple(0, 0, 0));

    double minx = DBL_MAX, miny = DBL_MAX, minz = DBL_MAX;
    double maxx = -DBL_MAX, maxy = -DBL_MAX, maxz = -DBL_MAX;
    for (size_t i = 0; i < n; ++i) {
        const QwtPoint3D p = s->sample(i);
        minx = std::min(minx, p.x()); maxx = std::max(maxx, p.x());
        miny = std::min(miny, p.y()); maxy = std::max(maxy, p.y());
        minz = std::min(minz, p.z()); maxz = std::max(maxz, p.z());
    }
    return ParallelEpiped(Triple(minx, miny, minz), Triple(maxx, maxy, maxz));
}
} // namespace

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

Qwt3DLine::Qwt3DLine()
    : Qwt3DPlotItem()
    , QWT_PIMPL_CONSTRUCT
{
}

Qwt3DLine::~Qwt3DLine() = default;

// ---------------------------------------------------------------------------
// Data loading
// ---------------------------------------------------------------------------

void Qwt3DLine::setSamples(const QVector<QwtPoint3D>& samples)
{
    setSamples(new QwtPoint3DSeriesData(samples));
}

void Qwt3DLine::setSamples(const QVector<double>& x, const QVector<double>& y,
                           const QVector<double>& z)
{
    const int n = std::min({ x.size(), y.size(), z.size() });
    QVector<QwtPoint3D> samples;
    samples.reserve(n);
    for (int i = 0; i < n; ++i)
        samples.append(QwtPoint3D(x[i], y[i], z[i]));
    setSamples(samples);
}

void Qwt3DLine::setSamples(const QwtPoint3D* samples, size_t count)
{
    QVector<QwtPoint3D> v;
    v.reserve(int(count));
    for (size_t i = 0; i < count; ++i)
        v.append(samples[i]);
    setSamples(v);
}

void Qwt3DLine::setSamples(QwtSeriesData<QwtPoint3D>* data)
{
    QWT_D(d);
    if (d->m_series)
        delete d->m_series;
    d->m_series = data ? data : new QwtPoint3DSeriesData();

    d->m_hull = computeHullFromSeries(d->m_series);
    d->m_vboDirty = true;
    d->m_pointsDirty = true;
    itemChanged();
}

const QwtSeriesData<QwtPoint3D>* Qwt3DLine::data() const
{
    QWT_DC(d);
    return d->m_series;
}

size_t Qwt3DLine::dataSize() const
{
    QWT_DC(d);
    return d->m_series ? d->m_series->size() : 0;
}

// ---------------------------------------------------------------------------
// Style and geometry
// ---------------------------------------------------------------------------

Qwt3DLine::LineStyle Qwt3DLine::lineStyle() const
{
    QWT_DC(d);
    return d->m_style;
}

void Qwt3DLine::setLineStyle(LineStyle style)
{
    QWT_D(d);
    if (d->m_style == style)
        return;
    d->m_style = style;
    d->m_vboDirty = true;  // vertex layout differs between Tube and Lines/Dots
    itemChanged();
}

double Qwt3DLine::lineWidth() const
{
    QWT_DC(d);
    return d->m_lineWidth;
}

void Qwt3DLine::setLineWidth(double width)
{
    QWT_D(d);
    d->m_lineWidth = width;
    itemChanged();
}

double Qwt3DLine::tubeRadius() const
{
    QWT_DC(d);
    return d->m_tubeRadius;
}

void Qwt3DLine::setTubeRadius(double radius)
{
    QWT_D(d);
    if (d->m_tubeRadius == radius)
        return;
    d->m_tubeRadius = radius;
    d->m_vboDirty = true;
    itemChanged();
}

int Qwt3DLine::tubeSegments() const
{
    QWT_DC(d);
    return d->m_tubeSegments;
}

void Qwt3DLine::setTubeSegments(int segments)
{
    QWT_D(d);
    if (segments < 3)
        segments = 3;
    if (d->m_tubeSegments == segments)
        return;
    d->m_tubeSegments = segments;
    d->m_vboDirty = true;
    itemChanged();
}

double Qwt3DLine::pointSize() const
{
    QWT_DC(d);
    return d->m_pointSize;
}

void Qwt3DLine::setPointSize(double size)
{
    QWT_D(d);
    d->m_pointSize = size;
    itemChanged();
}

bool Qwt3DLine::pointVisible() const
{
    QWT_DC(d);
    return d->m_pointVisible;
}

void Qwt3DLine::setPointVisible(bool on)
{
    QWT_D(d);
    if (d->m_pointVisible == on)
        return;
    d->m_pointVisible = on;
    itemChanged();
}

// ---------------------------------------------------------------------------
// Color
// ---------------------------------------------------------------------------

void Qwt3DLine::setColor(RGBA color)
{
    QWT_D(d);
    d->m_solidColor = color;
    d->m_vboDirty = true;
    d->m_pointsDirty = true;
    itemChanged();
}

void Qwt3DLine::setDataColor(Qwt3DColor* color)
{
    QWT_D(d);
    if (d->m_dataColor)
        d->m_dataColor->destroy();
    d->m_dataColor = color;
    d->m_vboDirty = true;
    d->m_pointsDirty = true;
    itemChanged();
}

const Qwt3DColor* Qwt3DLine::dataColor() const
{
    QWT_DC(d);
    return d->m_dataColor;
}

void Qwt3DLine::invalidateColors()
{
    QWT_D(d);
    d->m_vboDirty = true;
    d->m_pointsDirty = true;
    itemChanged();
}

void Qwt3DLine::pushColorRange() const
{
    QWT_DC(d);
    if (!d->m_dataColor || !plot())
        return;
    const ParallelEpiped h = plot()->hull();
    d->m_dataColor->setActiveRange(h.minVertex.z, h.maxVertex.z);
}

void Qwt3DLine::populateLegendColors(ColorVector& colors) const
{
    QWT_DC(d);
    if (d->m_dataColor) {
        pushColorRange();
        d->m_dataColor->createVector(colors);
    } else {
        colors.push_back(d->m_solidColor);
    }
}

// ---------------------------------------------------------------------------
// Qwt3DPlotItem interface
// ---------------------------------------------------------------------------

ParallelEpiped Qwt3DLine::hull() const
{
    QWT_DC(d);
    return d->m_hull;
}

// ---------------------------------------------------------------------------
// GL rendering
// ---------------------------------------------------------------------------

void Qwt3DLine::buildVBO()
{
    QWT_D(d);
    d->m_vertexCount = 0;
    d->m_indexCount = 0;

    // The Lines/Dots styles render from the separate points VBO
    if (d->m_style != Tube)
        return;

    const int N = static_cast<int>(d->m_series ? d->m_series->size() : 0);
    if (N < 2)
        return;

    // Ensure a color functor exists (solid color is handled by falling back
    // to a default standard color when attached to a plot)
    if (!d->m_dataColor) {
        if (plot())
            d->m_dataColor = new Qwt3DStandardColor();
        else
            return;
    }
    pushColorRange();

    // Resolve radius (auto = 0.5% of hull diagonal)
    double radius = d->m_tubeRadius;
    if (radius <= 0.0) {
        const Triple dt = d->m_hull.maxVertex - d->m_hull.minVertex;
        const double diag = std::sqrt(dt.x * dt.x + dt.y * dt.y + dt.z * dt.z);
        radius = 0.005 * diag;
        if (radius <= 0.0)
            radius = 0.01;
    }
    const int segs = std::max(3, d->m_tubeSegments);

    // Gather points + per-sample colors
    QVector<QVector3D> pts(N);
    QVector<QVector4D> cols(N);
    for (int i = 0; i < N; ++i) {
        const QwtPoint3D p = d->m_series->sample(i);
        pts[i] = QVector3D(static_cast<float>(p.x()),
                           static_cast<float>(p.y()),
                           static_cast<float>(p.z()));
        RGBA c = (*d->m_dataColor)(p.x(), p.y(), p.z());
        cols[i] = QVector4D(static_cast<float>(c.r),
                            static_cast<float>(c.g),
                            static_cast<float>(c.b),
                            static_cast<float>(c.a));
    }

    QVector<LineTubeVertex> vertices;
    QVector<unsigned int> indices;
    buildTubeGeometry(pts, cols, static_cast<float>(radius), segs, vertices, indices);

    d->m_vertexCount = vertices.size();
    d->m_indexCount = indices.size();
    if (vertices.isEmpty())
        return;

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    if (!f)
        return;

    if (!d->m_vao.isCreated())
        d->m_vao.create();
    d->m_vao.bind();

    // VBO (position + normal + color, 40-byte stride matching the surface shader)
    if (!d->m_vertexBuffer.isCreated())
        d->m_vertexBuffer.create();
    d->m_vertexBuffer.bind();
    d->m_vertexBuffer.allocate(vertices.constData(),
                               vertices.size() * sizeof(LineTubeVertex));

    const int stride = sizeof(LineTubeVertex);
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                             reinterpret_cast<void*>(sizeof(QVector3D)));
    f->glEnableVertexAttribArray(2);
    f->glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
                             reinterpret_cast<void*>(2 * sizeof(QVector3D)));

    // EBO (side indices)
    if (!d->m_indexBuffer.isCreated())
        d->m_indexBuffer.create();
    d->m_indexBuffer.bind();
    d->m_indexBuffer.allocate(indices.constData(),
                               indices.size() * sizeof(unsigned int));

    d->m_vao.release();
}

void Qwt3DLine::buildPointsVBO()
{
    QWT_D(d);
    d->m_pointCount = 0;

    const int N = static_cast<int>(d->m_series ? d->m_series->size() : 0);
    if (N <= 0)
        return;

    // Lazily create a default color functor so colormap coloring works out of the box
    if (!d->m_dataColor) {
        if (plot())
            d->m_dataColor = new Qwt3DStandardColor();
        else
            return;
    }
    pushColorRange();

    QVector<LinePointVertex> vertices(N);
    for (int i = 0; i < N; ++i) {
        const QwtPoint3D p = d->m_series->sample(i);
        const RGBA c = (*d->m_dataColor)(p.x(), p.y(), p.z());
        vertices[i].position = QVector3D(static_cast<float>(p.x()),
                                          static_cast<float>(p.y()),
                                          static_cast<float>(p.z()));
        vertices[i].color = QVector4D(static_cast<float>(c.r),
                                     static_cast<float>(c.g),
                                     static_cast<float>(c.b),
                                     static_cast<float>(c.a));
    }
    d->m_pointCount = N;

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    if (!f)
        return;

    if (!d->m_pointsVAO.isCreated())
        d->m_pointsVAO.create();
    d->m_pointsVAO.bind();

    // VBO (position + color, 28-byte stride matching the line/point shaders)
    if (!d->m_pointsBuffer.isCreated())
        d->m_pointsBuffer.create();
    d->m_pointsBuffer.bind();
    d->m_pointsBuffer.allocate(vertices.constData(), N * sizeof(LinePointVertex));

    const int stride = sizeof(LinePointVertex);
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride,
                             reinterpret_cast<void*>(sizeof(QVector3D)));

    d->m_pointsVAO.release();
}

void Qwt3DLine::draw()
{
    QWT_D(d);

    if (!plot() || !plot()->initializedGL())
        return;

    const int N = static_cast<int>(d->m_series ? d->m_series->size() : 0);
    if (N == 0)
        return;

    if (d->m_vboDirty) {
        buildVBO();
        d->m_vboDirty = false;
    }
    if (d->m_pointsDirty) {
        buildPointsVBO();
        d->m_pointsDirty = false;
    }

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    if (!f)
        return;

    const QMatrix4x4 mv = plot()->modelViewMatrix();
    const QMatrix4x4 proj = plot()->projectionMatrix();

    // Draws point markers from the points VBO using the shared point shader
    auto drawPointMarkers = [&]() {
        if (d->m_pointCount <= 0)
            return;
        QOpenGLShaderProgram* sh = plot()->pointShader();
        if (!sh)
            return;
        f->glEnable(GL_PROGRAM_POINT_SIZE);
        sh->bind();
        sh->setUniformValue("uModelView", mv);
        sh->setUniformValue("uProjection", proj);
        sh->setUniformValue("uPointSize", static_cast<float>(d->m_pointSize));
        d->m_pointsVAO.bind();
        f->glDrawArrays(GL_POINTS, 0, d->m_pointCount);
        d->m_pointsVAO.release();
        sh->release();
        f->glDisable(GL_PROGRAM_POINT_SIZE);
    };

    switch (d->m_style) {
    case Tube: {
        if (d->m_vertexCount == 0 || d->m_indexCount == 0)
            break;  // not enough points for a tube

        if (!d->m_tubeShaderInitialized) {
            d->m_tubeShader.addShaderFromSourceFile(
                QOpenGLShader::Vertex, ":/shaders/surface.vert");
            d->m_tubeShader.addShaderFromSourceFile(
                QOpenGLShader::Fragment, ":/shaders/surface.frag");
            if (!d->m_tubeShader.link())
                break;
            d->m_tubeShaderInitialized = true;
        }

        d->m_tubeShader.bind();
        d->m_tubeShader.setUniformValue("uModelView", mv);
        d->m_tubeShader.setUniformValue("uProjection", proj);
        d->m_tubeShader.setUniformValue("uNormalMatrix", mv.normalMatrix());

        const bool useLighting = plot()->lightingEnabled();
        d->m_tubeShader.setUniformValue("uUseLighting", useLighting);
        if (useLighting) {
            d->m_tubeShader.setUniformValue("uLightPos", QVector3D(0.0f, 0.0f, 10.0f));
            d->m_tubeShader.setUniformValue("uLightColor", QVector3D(1.0f, 1.0f, 1.0f));
            d->m_tubeShader.setUniformValue("uShininess", 32.0f);
        }
        d->m_tubeShader.setUniformValue("uUseOverrideColor", false);

        d->m_vao.bind();
        f->glDrawElements(GL_TRIANGLES, d->m_indexCount, GL_UNSIGNED_INT, nullptr);
        d->m_vao.release();
        d->m_tubeShader.release();
        break;
    }

    case Lines: {
        if (d->m_pointCount < 2)
            break;
        QOpenGLShaderProgram* sh = plot()->lineShader();
        if (!sh)
            break;
        sh->bind();
        sh->setUniformValue("uModelView", mv);
        sh->setUniformValue("uProjection", proj);
        f->glLineWidth(static_cast<float>(d->m_lineWidth));
        d->m_pointsVAO.bind();
        f->glDrawArrays(GL_LINE_STRIP, 0, d->m_pointCount);
        d->m_pointsVAO.release();
        sh->release();
        break;
    }

    case Dots:
        drawPointMarkers();
        break;
    }

    // Optional point-marker overlay on top of the Lines/Tube styles
    if (d->m_pointVisible && d->m_style != Dots)
        drawPointMarkers();
}
