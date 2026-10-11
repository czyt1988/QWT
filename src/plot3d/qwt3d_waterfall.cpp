#include "qwt3d_waterfall_p.h"
#include "qwt3d_plot.h"
#include "qwt3d_color.h"
#include "qwt3d_colormap_color.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <cfloat>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Anonymous namespace: tube geometry helper
// ---------------------------------------------------------------------------

namespace
{

/// Clamps a float to [lo, hi]
float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/**
 * Builds tube (swept circle) geometry from one polyline using parallel-transport
 * framing (same algorithm as Qwt3DLine). Appends ring vertices and side indices
 * to the given containers so several traces can share one VBO/EBO.
 */
void appendTubeGeometry(const QVector<QVector3D>& pts,
                        const QVector<QVector4D>& colors,
                        const QVector3D& radius, int segments,
                        QVector<WaterfallTubeVertex>& vertices,
                        QVector<unsigned int>& indices)
{
    const int N = pts.size();
    if (N < 2 || segments < 3 || radius.x() <= 0.0f || radius.y() <= 0.0f || radius.z() <= 0.0f)
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
    const unsigned int base = static_cast< unsigned int >(vertices.size());
    for (int i = 0; i < N; ++i) {
        const QVector4D& col = colors.value(i, QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
        for (int k = 0; k < segments; ++k) {
            const float ang = float(2.0 * Qwt3D_PI * k / segments);
            const float ca = std::cos(ang);
            const float sa = std::sin(ang);
            const QVector3D radial = Nrm[i] * ca + Bnm[i] * sa;  // outward normal
            WaterfallTubeVertex v;
            // Component-wise radius: the cross section is an ellipse in data
            // space that projects to a circle under the plot's per-axis
            // normalization (an isotropic data-space radius would fuse
            // neighboring traces and stretch into slabs under anisotropic
            // view scales).
            v.position = pts[i] + QVector3D(radial.x() * radius.x(),
                                            radial.y() * radius.y(),
                                            radial.z() * radius.z());
            // Inverse-scale rule for normals under non-uniform scaling
            v.normal = QVector3D(radial.x() / radius.x(),
                                 radial.y() / radius.y(),
                                 radial.z() / radius.z()).normalized();
            v.color = col;
            vertices.append(v);
        }
    }

    // 4. Side indices: each ring pair -> a quad strip of 'segments' quads
    for (int i = 0; i < N - 1; ++i) {
        for (int k = 0; k < segments; ++k) {
            const int k1 = (k + 1) % segments;
            const unsigned int a = base + static_cast< unsigned int >(i * segments + k);
            const unsigned int b = base + static_cast< unsigned int >(i * segments + k1);
            const unsigned int c = base + static_cast< unsigned int >((i + 1) * segments + k);
            const unsigned int dd = base + static_cast< unsigned int >((i + 1) * segments + k1);
            indices.append(a);
            indices.append(b);
            indices.append(c);
            indices.append(b);
            indices.append(dd);
            indices.append(c);
        }
    }
}

} // namespace

// ---------------------------------------------------------------------------
// PrivateData helpers
// ---------------------------------------------------------------------------

/**
 * @brief Recomputes the amplitude range and the hull from the stored traces
 * @details Pure CPU, no GL dependency - safe for Qwt3DPlot::itemChanged()
 *          hull unions and for unit tests.
 *          Hull: x from the bin values (or 0..M-1), y = [0, count-1] (depth),
 *          z = [min(dataMin, baseline), dataMax] (amplitude).
 */
void Qwt3DWaterfall::PrivateData::recalculateRanges()
{
    m_dataMin = 0.0;
    m_dataMax = 0.0;

    const int M = m_samplesPerTrace;
    if (m_count > 0 && M > 0) {
        double minV = DBL_MAX;
        double maxV = -DBL_MAX;
        for (int i = 0; i < m_count; ++i) {
            const QVector<double>& trace = m_traces[ i ];
            for (int j = 0; j < M; ++j) {
                minV = std::min(minV, trace[ j ]);
                maxV = std::max(maxV, trace[ j ]);
            }
        }
        m_dataMin = minV;
        m_dataMax = maxV;
    }

    // x range from the bin values (empty bins mean 0..M-1)
    double minX = 0.0, maxX = (M > 0) ? M - 1.0 : 0.0;
    if (!m_bins.isEmpty() && M > 0) {
        minX = DBL_MAX;
        maxX = -DBL_MAX;
        for (int j = 0; j < M; ++j) {
            const double x = m_bins.value(j, double(j));
            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
        }
    }

    // y range: the depth axis, slot indices grow monotonically until the
    // window is full
    const double maxY = (m_count > 0) ? m_count - 1.0 : 0.0;

    // z range: the amplitude axis; the ribbon may extend down to a fixed
    // baseline (3D module convention: the value axis is z, like
    // Qwt3DSurface's z = f(x, y))
    const double effBaseline = m_fixedBaseline ? m_baseline : m_dataMin;
    const double minZ = std::min(m_dataMin, effBaseline);
    const double maxZ = m_dataMax;

    m_hull = ParallelEpiped(Triple(minX, 0.0, minZ), Triple(maxX, maxY, maxZ));
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

Qwt3DWaterfall::Qwt3DWaterfall()
    : Qwt3DPlotItem()
    , QWT_PIMPL_CONSTRUCT
{
}

Qwt3DWaterfall::~Qwt3DWaterfall() = default;

// ---------------------------------------------------------------------------
// Data / sliding window
// ---------------------------------------------------------------------------

/**
 * @brief Returns the sliding window size
 * @return Number of trace slots (>= 1)
 */
int Qwt3DWaterfall::traceCapacity() const
{
    QWT_DC(d);
    return d->m_capacity;
}

/**
 * @brief Sets the sliding window size
 * @param count New window size (clamped to >= 1)
 * @details When shrinking, the oldest traces are dropped so the newest
 *          min(traceCount, count) traces survive. The stored traces keep
 *          their relative order.
 */
void Qwt3DWaterfall::setTraceCapacity(int count)
{
    QWT_D(d);
    if (count < 1)
        count = 1;
    if (count == d->m_capacity)
        return;

    const int keep = std::min(d->m_count, count);
    std::vector< QVector<double> > traces(static_cast< size_t >(count));
    for (int i = 0; i < keep; ++i)
        traces[ static_cast< size_t >(i) ] = std::move(d->m_traces[ static_cast< size_t >(d->m_count - keep + i) ]);

    d->m_traces = std::move(traces);
    d->m_capacity = count;
    d->m_count = keep;

    d->recalculateRanges();
    d->m_ribbonDirty = true;
    d->m_outlineDirty = true;
    d->m_tubeDirty = true;
    itemChanged();
}

/**
 * @brief Returns how many traces are currently stored
 * @return Trace count, <= traceCapacity()
 */
int Qwt3DWaterfall::traceCount() const
{
    QWT_DC(d);
    return d->m_count;
}

/**
 * @brief Returns the bin count of the stored traces
 * @return Number of samples per trace, 0 before the first pushTrace()
 */
int Qwt3DWaterfall::samplesPerTrace() const
{
    QWT_DC(d);
    return d->m_samplesPerTrace;
}

/**
 * @brief Appends one trace as the newest window entry
 * @param values Sample values of the trace
 * @param count Number of samples in values
 * @details The first push fixes samplesPerTrace(); later traces are fitted
 *          into the established bin count (longer ones truncated, shorter
 *          ones zero-padded) so the window width (and the hull x-range)
 *          never jitters. When the window is full the oldest trace is
 *          dropped; the surviving slots are rotated by cheap QVector moves,
 *          never deep copies.
 */
void Qwt3DWaterfall::pushTrace(const double* values, int count)
{
    QWT_D(d);
    if (!values || count <= 0)
        return;

    if (d->m_count == 0)
        d->m_samplesPerTrace = count;

    // Fit the incoming frame into the established bin count: truncate longer
    // frames, zero-pad shorter ones, so every slot has the full bin width.
    const int M = d->m_samplesPerTrace;
    const int n = std::min(count, M);
    QVector<double> trace(M, 0.0);
    std::copy(values, values + n, trace.begin());

    if (d->m_count == d->m_capacity) {
        // Window full: drop the oldest slot by rotating the QVector handles
        std::rotate(d->m_traces.begin(), d->m_traces.begin() + 1, d->m_traces.begin() + d->m_count);
        d->m_traces[ static_cast< size_t >(d->m_capacity - 1) ] = std::move(trace);
    } else {
        d->m_traces[ static_cast< size_t >(d->m_count) ] = std::move(trace);
        ++d->m_count;
    }

    d->recalculateRanges();
    d->m_ribbonDirty = true;
    d->m_outlineDirty = true;
    d->m_tubeDirty = true;
    itemChanged();
}

/**
 * @brief Appends one trace as the newest window entry
 * @param values Sample values of the trace
 * @sa pushTrace(const double*, int)
 */
void Qwt3DWaterfall::pushTrace(const QVector<double>& values)
{
    pushTrace(values.constData(), values.size());
}

/**
 * @brief Clears all traces
 * @details The capacity, bin values, styles and colors are kept. The bin
 *          count is re-established by the next pushTrace().
 */
void Qwt3DWaterfall::clear()
{
    QWT_D(d);
    d->m_count = 0;
    d->m_samplesPerTrace = 0;
    d->recalculateRanges();
    d->m_ribbonDirty = true;
    d->m_outlineDirty = true;
    d->m_tubeDirty = true;
    itemChanged();
}

/**
 * @brief Sets the x value of every bin
 * @param x Bin positions; an empty vector means 0..samplesPerTrace-1
 */
void Qwt3DWaterfall::setBinValues(const QVector<double>& x)
{
    QWT_D(d);
    d->m_bins = x;
    d->recalculateRanges();
    d->m_ribbonDirty = true;
    d->m_outlineDirty = true;
    d->m_tubeDirty = true;
    itemChanged();
}

/**
 * @brief Returns the current bin values
 * @return Bin positions, empty when the bins default to 0..M-1
 */
QVector<double> Qwt3DWaterfall::binValues() const
{
    QWT_DC(d);
    return d->m_bins;
}

/**
 * @brief Returns the depth direction of the sliding window
 * @return NewestInFront (newest trace at the largest z) or NewestBehind
 */
Qwt3DWaterfall::DepthOrder Qwt3DWaterfall::depthOrder() const
{
    QWT_DC(d);
    return d->m_depthOrder;
}

/**
 * @brief Sets the depth direction of the sliding window
 * @param order NewestInFront or NewestBehind
 * @details The newest trace either sits at the largest z (the window moves
 *          away from the viewer) or at z == 0 (the window moves towards the
 *          viewer). Either way the hull z-range stays [0, count-1].
 */
void Qwt3DWaterfall::setDepthOrder(DepthOrder order)
{
    QWT_D(d);
    if (d->m_depthOrder == order)
        return;
    d->m_depthOrder = order;
    d->m_ribbonDirty = true;
    d->m_outlineDirty = true;
    d->m_tubeDirty = true;
    itemChanged();
}

// ---------------------------------------------------------------------------
// Style
// ---------------------------------------------------------------------------

/**
 * @brief Returns the geometry style
 * @return Ribbon, RibbonMesh, Wireframe or Tube
 */
Qwt3DWaterfall::PlotStyle Qwt3DWaterfall::plotStyle() const
{
    QWT_DC(d);
    return d->m_style;
}

/**
 * @brief Sets the geometry style
 * @param style The new geometry style
 * @details RibbonMesh draws its outline after the fill, regardless of the
 *          item's z value, so the outline is never hidden by its own curtain.
 */
void Qwt3DWaterfall::setPlotStyle(PlotStyle style)
{
    QWT_D(d);
    if (d->m_style == style)
        return;
    d->m_style = style;
    d->m_ribbonDirty = true;
    d->m_outlineDirty = true;
    d->m_tubeDirty = true;
    itemChanged();
}

/**
 * @brief Returns the color mode
 * @return ByDepth, ByAmplitude or Solid
 */
Qwt3DWaterfall::ColorMode Qwt3DWaterfall::colorMode() const
{
    QWT_DC(d);
    return d->m_colorMode;
}

/**
 * @brief Sets the color mode
 * @param mode The new color mode
 * @details ByAmplitude retargets a Qwt3DColorMapColor data functor to the z
 *          coordinate (via setSourceAxis(ColorFromZ)), ByDepth retargets it
 *          to y (the depth axis). A functor of a different type is left
 *          untouched; its owner decides how to interpret the (x, y, z)
 *          argument.
 */
void Qwt3DWaterfall::setColorMode(ColorMode mode)
{
    QWT_D(d);
    if (d->m_colorMode == mode)
        return;
    d->m_colorMode = mode;

    if (auto* cmap = dynamic_cast< Qwt3DColorMapColor* >(d->m_dataColor))
        cmap->setSourceAxis(mode == ByAmplitude ? ColorFromZ : ColorFromY);

    d->m_ribbonDirty = true;
    d->m_outlineDirty = true;
    d->m_tubeDirty = true;
    itemChanged();
}

/**
 * @brief Sets the solid color
 * @param color Color used by the Solid mode
 */
void Qwt3DWaterfall::setColor(RGBA color)
{
    QWT_D(d);
    d->m_solidColor = color;
    d->m_ribbonDirty = true;
    d->m_tubeDirty = true;
    itemChanged();
}

/**
 * @brief Returns the solid color
 * @return Color used by the Solid mode
 */
RGBA Qwt3DWaterfall::color() const
{
    QWT_DC(d);
    return d->m_solidColor;
}

/**
 * @brief Sets the data color functor
 * @param color The new functor (the item takes ownership)
 * @details A Qwt3DColorMapColor functor is retargeted to the source axis
 *          matching the current color mode (y for ByAmplitude, z otherwise).
 */
void Qwt3DWaterfall::setDataColor(Qwt3DColor* color)
{
    QWT_D(d);
    if (d->m_dataColor)
        d->m_dataColor->destroy();
    d->m_dataColor = color;

    if (auto* cmap = dynamic_cast< Qwt3DColorMapColor* >(d->m_dataColor))
        cmap->setSourceAxis(d->m_colorMode == ByAmplitude ? ColorFromZ : ColorFromY);

    d->m_ribbonDirty = true;
    d->m_outlineDirty = true;
    d->m_tubeDirty = true;
    itemChanged();
}

/**
 * @brief Returns the data color functor
 * @return The functor set with setDataColor(), nullptr if none was set
 */
const Qwt3DColor* Qwt3DWaterfall::dataColor() const
{
    QWT_DC(d);
    return d->m_dataColor;
}

/**
 * @brief Marks the baked per-vertex colors as stale
 * @details Colors are baked into the VBO at build time, so an in-place
 *          mutation of the data color functor needs this call (or any other
 *          setter) to take effect on the next draw.
 */
void Qwt3DWaterfall::invalidateColors()
{
    QWT_D(d);
    d->m_ribbonDirty = true;
    d->m_outlineDirty = true;
    d->m_tubeDirty = true;
    itemChanged();
}

/**
 * @brief Fixes the color range
 * @param min Lower bound of the colormap input range
 * @param max Upper bound of the colormap input range
 * @details A fixed range disables the automatic (per-frame) range derivation,
 *          which is essential for stable amplitude colors in a live waterfall.
 */
void Qwt3DWaterfall::setColorRange(double min, double max)
{
    QWT_D(d);
    d->m_fixedColorRange = true;
    d->m_colorRangeMin = std::min(min, max);
    d->m_colorRangeMax = std::max(min, max);
    d->m_ribbonDirty = true;
    d->m_outlineDirty = true;
    d->m_tubeDirty = true;
    itemChanged();
}

/**
 * @brief Returns to the automatic color range
 */
void Qwt3DWaterfall::resetColorRange()
{
    QWT_D(d);
    if (!d->m_fixedColorRange)
        return;
    d->m_fixedColorRange = false;
    d->m_ribbonDirty = true;
    d->m_outlineDirty = true;
    d->m_tubeDirty = true;
    itemChanged();
}

/**
 * @brief Returns true when a fixed color range is active
 * @return true after setColorRange(), false after resetColorRange()
 */
bool Qwt3DWaterfall::hasFixedColorRange() const
{
    QWT_DC(d);
    return d->m_fixedColorRange;
}

/**
 * @brief Returns the ribbon baseline
 * @return The fixed baseline, or the current amplitude minimum when unset
 */
double Qwt3DWaterfall::baseline() const
{
    QWT_DC(d);
    return d->m_fixedBaseline ? d->m_baseline : d->m_dataMin;
}

/**
 * @brief Sets the ribbon baseline
 * @param value The baseline y value
 */
void Qwt3DWaterfall::setBaseline(double value)
{
    QWT_D(d);
    if (d->m_fixedBaseline && d->m_baseline == value)
        return;
    d->m_fixedBaseline = true;
    d->m_baseline = value;
    d->recalculateRanges();
    d->m_ribbonDirty = true;
    itemChanged();
}

/**
 * @brief Lets the baseline follow the amplitude minimum again
 */
void Qwt3DWaterfall::resetBaseline()
{
    QWT_D(d);
    if (!d->m_fixedBaseline)
        return;
    d->m_fixedBaseline = false;
    d->recalculateRanges();
    d->m_ribbonDirty = true;
    itemChanged();
}

/**
 * @brief Returns the ribbon alpha
 * @return Alpha in [0, 1] (default 1)
 */
double Qwt3DWaterfall::opacity() const
{
    QWT_DC(d);
    return d->m_opacity;
}

/**
 * @brief Sets the ribbon alpha
 * @param alpha Alpha in [0, 1]; values outside are clamped
 * @details Alpha < 1 enables blended drawing. The slots are drawn from the
 *          oldest to the newest trace, which is the recommended front-to-back
 *          order for the default camera; view nearly parallel to the curtains
 *          can still produce interpenetration artifacts (see the docs).
 */
void Qwt3DWaterfall::setOpacity(double alpha)
{
    QWT_D(d);
    alpha = std::min(1.0, std::max(0.0, alpha));
    if (d->m_opacity == alpha)
        return;
    d->m_opacity = alpha;
    d->m_ribbonDirty = true;
    itemChanged();
}

/**
 * @brief Returns the outline color
 * @return Color of the RibbonMesh / Wireframe outline
 */
RGBA Qwt3DWaterfall::outlineColor() const
{
    QWT_DC(d);
    return d->m_outlineColor;
}

/**
 * @brief Sets the outline color
 * @param color Color of the RibbonMesh / Wireframe outline
 */
void Qwt3DWaterfall::setOutlineColor(RGBA color)
{
    QWT_D(d);
    d->m_outlineColor = color;
    d->m_outlineDirty = true;
    itemChanged();
}

/**
 * @brief Returns the outline line width
 * @return Line width in pixels (Lines-based styles only)
 */
double Qwt3DWaterfall::outlineWidth() const
{
    QWT_DC(d);
    return d->m_outlineWidth;
}

/**
 * @brief Sets the outline line width
 * @param width Line width in pixels (> 1 is not guaranteed in Core profile)
 */
void Qwt3DWaterfall::setOutlineWidth(double width)
{
    QWT_D(d);
    d->m_outlineWidth = width;
    itemChanged();
}

// ---------------------------------------------------------------------------
// Color range push / functor handling
// ---------------------------------------------------------------------------

/**
 * @brief Pushes the active color range into the data color functor
 * @details ByDepth pushes [0, traceCapacity()-1] so the colormap spans the
 *          whole window. ByAmplitude pushes the fixed range when active, the
 *          current amplitude range else. The functor stays a silent value
 *          object; the item drives it (same pattern as Qwt3DLine).
 */
void Qwt3DWaterfall::pushColorRange() const
{
    QWT_DC(d);
    if (!d->m_dataColor)
        return;

    double lo = 0.0, hi = 1.0;
    switch (d->m_colorMode) {
    case ByDepth:
        lo = 0.0;
        hi = (d->m_capacity > 1) ? d->m_capacity - 1.0 : 1.0;
        break;
    case ByAmplitude:
    case Solid:
        if (d->m_fixedColorRange) {
            lo = d->m_colorRangeMin;
            hi = d->m_colorRangeMax;
        } else {
            lo = d->m_dataMin;
            hi = d->m_dataMax;
        }
        break;
    }

    if (hi <= lo)
        hi = lo + 1.0;  // degenerate range guard for flat/empty data

    d->m_dataColor->setActiveRange(lo, hi);
}

/**
 * @brief Creates the default data color functor when none was set
 * @details ByAmplitude defaults to a Qwt3DColorMapColor reading the z
 *          coordinate (the amplitude axis), ByDepth to one reading y (the
 *          depth axis), so setColorMode(ByAmplitude) colors by amplitude with
 *          zero custom code. Solid needs no functor.
 */
void Qwt3DWaterfall::ensureDataColor()
{
    QWT_D(d);
    if (d->m_dataColor || d->m_colorMode == Solid)
        return;

    auto* cmap = new Qwt3DColorMapColor(QString("viridis"));
    // Amplitude lives on z (module convention), depth on y
    cmap->setSourceAxis(d->m_colorMode == ByAmplitude ? ColorFromZ : ColorFromY);
    d->m_dataColor = cmap;
}

// ---------------------------------------------------------------------------
// Qwt3DPlotItem interface
// ---------------------------------------------------------------------------

/**
 * @brief Returns the bounding hull of the waterfall
 * @return x from the bin values, y = [0, traceCount()-1] (depth),
 *         z = [min(amplitude, baseline), amplitude max]; degenerate to
 *         (0,0,0)-(0,0,0) when empty
 */
ParallelEpiped Qwt3DWaterfall::hull() const
{
    QWT_DC(d);
    return d->m_hull;
}

/**
 * @brief Populates the legend colors
 * @param[out] colors Legend color vector to fill
 * @details One waterfall is one item and contributes one legend gradient:
 *          the data color functor vector (with the active range pushed), or
 *          the solid color in Solid mode / before a functor exists.
 */
void Qwt3DWaterfall::populateLegendColors(ColorVector& colors) const
{
    QWT_DC(d);
    if (d->m_colorMode != Solid && d->m_dataColor) {
        pushColorRange();
        d->m_dataColor->createVector(colors);
    } else {
        colors.push_back(d->m_solidColor);
    }
}

// ---------------------------------------------------------------------------
// GL rendering
// ---------------------------------------------------------------------------

/**
 * @brief Builds the ribbon fill VBO
 * @details One GL_TRIANGLE_STRIP per trace slot: for every bin a baseline and
 *          a curve vertex, both colored with the curve value's color so each
 *          curtain column reads as one amplitude band. Slots are stored
 *          oldest first and drawn in that order (stable painter's order for
 *          the blending case).
 *
 * Upload budget: capacity * bins * 28 bytes per rebuild (every pushTrace
 * rebuilds the whole window because all slots shift). At the target workload
 * (<= 128 traces * 512 bins) this stays well below the measured budget.
 * Optimization channel (NOT implemented, measure first): when
 * capacity > 128 and bins > 1024 become a bottleneck, switch to one draw call
 * per slot with the row z provided by a private shader uniform, then only
 * glBufferSubData the newest row each frame.
 */
void Qwt3DWaterfall::buildVBO()
{
    QWT_D(d);
    d->m_ribbonVertexCount = 0;

    if (d->m_style != Ribbon && d->m_style != RibbonMesh)
        return;

    const int M = d->m_samplesPerTrace;
    if (d->m_count <= 0 || M <= 0)
        return;

    ensureDataColor();
    if (d->m_colorMode != Solid)
        pushColorRange();

    const double baseZ = d->m_fixedBaseline ? d->m_baseline : d->m_dataMin;
    const bool useFunctor = (d->m_colorMode != Solid) && d->m_dataColor;

    QVector<WaterfallVertex> vertices;
    vertices.reserve(d->m_count * 2 * M);
    for (int i = 0; i < d->m_count; ++i) {
        const float z = static_cast< float >(
            d->m_depthOrder == NewestInFront ? i : d->m_count - 1 - i);
        const QVector<double>& trace = d->m_traces[ i ];
        for (int j = 0; j < M; ++j) {
            const double x = d->m_bins.value(j, double(j));
            const double v = trace[ j ];

            QVector4D col(1.0f, 1.0f, 1.0f, 1.0f);
            if (useFunctor) {
                const RGBA c = (*d->m_dataColor)(x, z, v);
                col = QVector4D(static_cast< float >(c.r),
                                static_cast< float >(c.g),
                                static_cast< float >(c.b),
                                static_cast< float >(c.a * d->m_opacity));
            } else {
                col = QVector4D(static_cast< float >(d->m_solidColor.r),
                                static_cast< float >(d->m_solidColor.g),
                                static_cast< float >(d->m_solidColor.b),
                                static_cast< float >(d->m_solidColor.a * d->m_opacity));
            }

            WaterfallVertex bv;
            bv.position = QVector3D(static_cast< float >(x), z, static_cast< float >(baseZ));
            bv.color = col;
            vertices.append(bv);

            WaterfallVertex tv;
            tv.position = QVector3D(static_cast< float >(x), z, static_cast< float >(v));
            tv.color = col;
            vertices.append(tv);
        }
    }

    d->m_ribbonVertexCount = static_cast< int >(vertices.size());
    if (vertices.isEmpty())
        return;

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    if (!f)
        return;

    if (!d->m_ribbonVAO.isCreated())
        d->m_ribbonVAO.create();
    d->m_ribbonVAO.bind();

    // VBO (position + color, 28-byte stride matching the polygon shader)
    if (!d->m_ribbonBuffer.isCreated())
        d->m_ribbonBuffer.create();
    d->m_ribbonBuffer.bind();
    d->m_ribbonBuffer.allocate(vertices.constData(),
                               static_cast< int >(vertices.size() * sizeof(WaterfallVertex)));

    const int stride = sizeof(WaterfallVertex);
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride,
                             reinterpret_cast<void*>(sizeof(QVector3D)));

    d->m_ribbonVAO.release();
}

/**
 * @brief Builds the outline VBO
 * @details One GL_LINE_STRIP per trace slot along the curve. Used by the
 *          RibbonMesh (fill + outline) and Wireframe (outline only) styles.
 */
void Qwt3DWaterfall::buildOutlineVBO()
{
    QWT_D(d);
    d->m_outlineVertexCount = 0;

    if (d->m_style != RibbonMesh && d->m_style != Wireframe)
        return;

    const int M = d->m_samplesPerTrace;
    if (d->m_count <= 0 || M <= 0)
        return;

    const QVector4D col(static_cast< float >(d->m_outlineColor.r),
                        static_cast< float >(d->m_outlineColor.g),
                        static_cast< float >(d->m_outlineColor.b),
                        static_cast< float >(d->m_outlineColor.a));

    QVector<WaterfallVertex> vertices;
    vertices.reserve(d->m_count * M);
    for (int i = 0; i < d->m_count; ++i) {
        const float z = static_cast< float >(
            d->m_depthOrder == NewestInFront ? i : d->m_count - 1 - i);
        const QVector<double>& trace = d->m_traces[ i ];
        for (int j = 0; j < M; ++j) {
            const double x = d->m_bins.value(j, double(j));
            WaterfallVertex v;
            v.position = QVector3D(static_cast< float >(x), z,
                                   static_cast< float >(trace[ j ]));
            v.color = col;
            vertices.append(v);
        }
    }

    d->m_outlineVertexCount = static_cast< int >(vertices.size());
    if (vertices.isEmpty())
        return;

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    if (!f)
        return;

    if (!d->m_outlineVAO.isCreated())
        d->m_outlineVAO.create();
    d->m_outlineVAO.bind();

    // VBO (position + color, 28-byte stride matching the line shader)
    if (!d->m_outlineBuffer.isCreated())
        d->m_outlineBuffer.create();
    d->m_outlineBuffer.bind();
    d->m_outlineBuffer.allocate(vertices.constData(),
                                static_cast< int >(vertices.size() * sizeof(WaterfallVertex)));

    const int stride = sizeof(WaterfallVertex);
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride,
                             reinterpret_cast<void*>(sizeof(QVector3D)));

    d->m_outlineVAO.release();
}

/**
 * @brief Builds the tube VBO/EBO
 * @details Each trace is swept with a view-circular cross-section
 *          (parallel-transport framing, per-axis radius = 0.5% of the hull
 *          extents, 6 ring segments), rendered with the surface shader (lit).
 *          Dense traces are decimated to at most 129 ring stations to bound
 *          the vertex/index count. Colors follow the color mode at the curve
 *          sample points.
 */
void Qwt3DWaterfall::buildTubeVBO()
{
    QWT_D(d);
    d->m_tubeIndexCount = 0;

    if (d->m_style != Tube)
        return;

    const int M = d->m_samplesPerTrace;
    if (d->m_count <= 0 || M < 2)
        return;

    ensureDataColor();
    if (d->m_colorMode != Solid)
        pushColorRange();

    // Per-axis radius: a common fraction c of each hull extent, so the cross
    // section reads as a circle under the plot's per-axis normalization. c is
    // capped so the depth radius stays below half a slot (the trace spacing
    // is one slot): without the cap, dense windows fuse into a single blob.
    const Triple dt = d->m_hull.maxVertex - d->m_hull.minVertex;
    const double c = qMin(0.005, 0.4 / qMax(1.0, double(d->m_count - 1)));
    const QVector3D radius(static_cast< float >(c * (dt.x > 1e-9 ? dt.x : 1.0)),
                           static_cast< float >(c * (dt.y > 1e-9 ? dt.y : 1.0)),
                           static_cast< float >(c * (dt.z > 1e-9 ? dt.z : 1.0)));
    const int segments = 6;

    // Ring stations per trace: decimate dense traces (tubes smooth out the
    // difference) so the vertex/index count - the Tube style's real cost -
    // stays bounded. The last bin is always kept.
    const int binStride = qMax(1, M / 128);

    const bool useFunctor = (d->m_colorMode != Solid) && d->m_dataColor;
    QVector<WaterfallTubeVertex> vertices;
    QVector<unsigned int> indices;
    for (int i = 0; i < d->m_count; ++i) {
        const float z = static_cast< float >(
            d->m_depthOrder == NewestInFront ? i : d->m_count - 1 - i);
        const QVector<double>& trace = d->m_traces[ i ];

        QVector<QVector3D> pts;
        QVector<QVector4D> cols;
        for (int j = 0; j < M; j += binStride) {
            const double x = d->m_bins.value(j, double(j));
            const double v = trace[ j ];
            pts.append(QVector3D(static_cast< float >(x), z, static_cast< float >(v)));
            RGBA c = d->m_solidColor;
            if (useFunctor)
                c = (*d->m_dataColor)(x, z, v);
            cols.append(QVector4D(static_cast< float >(c.r),
                                  static_cast< float >(c.g),
                                  static_cast< float >(c.b),
                                  static_cast< float >(c.a)));
        }
        if ((M - 1) % binStride != 0) {
            const double x = d->m_bins.value(M - 1, double(M - 1));
            const double v = trace[ M - 1 ];
            pts.append(QVector3D(static_cast< float >(x), z, static_cast< float >(v)));
            RGBA c = d->m_solidColor;
            if (useFunctor)
                c = (*d->m_dataColor)(x, z, v);
            cols.append(QVector4D(static_cast< float >(c.r),
                                  static_cast< float >(c.g),
                                  static_cast< float >(c.b),
                                  static_cast< float >(c.a)));
        }
        appendTubeGeometry(pts, cols, radius, segments, vertices, indices);
    }

    d->m_tubeIndexCount = static_cast< int >(indices.size());
    if (vertices.isEmpty() || indices.isEmpty())
        return;

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    if (!f)
        return;

    if (!d->m_tubeVAO.isCreated())
        d->m_tubeVAO.create();
    d->m_tubeVAO.bind();

    // VBO (position + normal + color, 40-byte stride matching the surface shader)
    if (!d->m_tubeVertexBuffer.isCreated())
        d->m_tubeVertexBuffer.create();
    d->m_tubeVertexBuffer.bind();
    d->m_tubeVertexBuffer.allocate(vertices.constData(),
                                   static_cast< int >(vertices.size() * sizeof(WaterfallTubeVertex)));

    const int stride = sizeof(WaterfallTubeVertex);
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                             reinterpret_cast<void*>(sizeof(QVector3D)));
    f->glEnableVertexAttribArray(2);
    f->glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
                             reinterpret_cast<void*>(2 * sizeof(QVector3D)));

    // EBO (side indices)
    if (!d->m_tubeIndexBuffer.isCreated())
        d->m_tubeIndexBuffer.create();
    d->m_tubeIndexBuffer.bind();
    d->m_tubeIndexBuffer.allocate(indices.constData(),
                                  static_cast< int >(indices.size() * sizeof(unsigned int)));

    d->m_tubeVAO.release();
}

void Qwt3DWaterfall::draw()
{
    QWT_D(d);

    if (!plot() || !plot()->initializedGL())
        return;

    const int M = d->m_samplesPerTrace;
    if (d->m_count <= 0 || M <= 0)
        return;

    if (d->m_ribbonDirty) {
        buildVBO();
        d->m_ribbonDirty = false;
    }
    if (d->m_outlineDirty) {
        buildOutlineVBO();
        d->m_outlineDirty = false;
    }
    if (d->m_tubeDirty) {
        buildTubeVBO();
        d->m_tubeDirty = false;
    }

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    if (!f)
        return;

    const QMatrix4x4 mv = plot()->modelViewMatrix();
    const QMatrix4x4 proj = plot()->projectionMatrix();

    const bool hasFill = (d->m_style == Ribbon || d->m_style == RibbonMesh);
    const bool hasOutline = (d->m_style == RibbonMesh || d->m_style == Wireframe);
    const bool blending = hasFill && d->m_opacity < 1.0;

    // 1. Ribbon fill (oldest slot first: stable painter's order when blending)
    if (hasFill && d->m_ribbonVertexCount > 0) {
        QOpenGLShaderProgram* sh = plot()->polygonShader();
        if (sh) {
            if (blending) {
                f->glEnable(GL_BLEND);
                f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            sh->bind();
            sh->setUniformValue("uModelView", mv);
            sh->setUniformValue("uProjection", proj);
            d->m_ribbonVAO.bind();
            const int perSlot = 2 * M;
            for (int i = 0; i < d->m_count; ++i)
                f->glDrawArrays(GL_TRIANGLE_STRIP, i * perSlot, perSlot);
            d->m_ribbonVAO.release();
            sh->release();
            if (blending)
                f->glDisable(GL_BLEND);
        }
    }

    // 2. Tube (lit, opaque)
    if (d->m_style == Tube && d->m_tubeIndexCount > 0) {
        if (!d->m_tubeShaderInitialized) {
            d->m_tubeShader.addShaderFromSourceFile(
                QOpenGLShader::Vertex, ":/shaders/surface.vert");
            d->m_tubeShader.addShaderFromSourceFile(
                QOpenGLShader::Fragment, ":/shaders/surface.frag");
            if (!d->m_tubeShader.link())
                return;
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
        d->m_tubeVAO.bind();
        f->glDrawElements(GL_TRIANGLES, d->m_tubeIndexCount, GL_UNSIGNED_INT, nullptr);
        d->m_tubeVAO.release();
        d->m_tubeShader.release();
    }

    // 3. Outline on top (fixed order, independent of the item z value)
    if (hasOutline && d->m_outlineVertexCount > 0) {
        QOpenGLShaderProgram* sh = plot()->lineShader();
        if (sh) {
            sh->bind();
            sh->setUniformValue("uModelView", mv);
            sh->setUniformValue("uProjection", proj);
            f->glLineWidth(static_cast< float >(d->m_outlineWidth));
            d->m_outlineVAO.bind();
            for (int i = 0; i < d->m_count; ++i)
                f->glDrawArrays(GL_LINE_STRIP, i * M, M);
            d->m_outlineVAO.release();
            sh->release();
        }
    }
}

/**
 * @brief Runtime type information
 * @return Rtti_Plot3DWaterfall (= 1004)
 */
int Qwt3DWaterfall::rtti() const
{
    return Rtti_Plot3DWaterfall;
}
