#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4305)
#endif

#include "qwt3d_colorlegend.h"

#include "qwt3d_plot.h"

#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

class Qwt3DColorLegend::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DColorLegend)

public:
    PrivateData(Qwt3DColorLegend* p) : q_ptr(p)
    {
        m_axisposition = Qwt3DColorLegend::Left;
        m_orientation  = Qwt3DColorLegend::BottomTop;
        m_showaxis     = true;
        m_position     = Qwt3DColorLegend::PosCustom;
        m_useAbsolute  = false;
    }

    Qwt3DLabel m_caption;
    ParallelEpiped m_pe;
    Tuple m_relMin, m_relMax;
    Qwt3DAxis m_axis;
    Qwt3DColorLegend::SCALEPOSITION m_axisposition;
    Qwt3DColorLegend::ORIENTATION m_orientation;
    bool m_showaxis;
    Qwt3DColorLegend::Position m_position;
    QRectF m_absoluteRect;
    bool m_useAbsolute;
};

Qwt3DColorLegend::Qwt3DColorLegend() : QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    d->m_axis.setNumbers(true);
    d->m_axis.setScaling(true);
    d->m_axis.setNumberColor(RGBA(0, 0, 0, 1));
    d->m_axis.setNumberAnchor(CenterRight);
    d->m_axis.setNumberFont(QFont("Courier", 8));

    d->m_caption.setFont("Courier", 10, QFont::Bold);
    d->m_caption.setColor(RGBA(0, 0, 0, 1));
    setRelPosition(Tuple(0.94, 1 - 0.36), Tuple(0.97, 1 - 0.04));
}

Qwt3DColorLegend::~Qwt3DColorLegend() = default;

void Qwt3DColorLegend::setTitleString(QString const& s)
{
    QWT_D(d);
    d->m_caption.setString(s);
}

void Qwt3DColorLegend::setTitleFont(QString const& family, int pointSize, int weight, bool italic)
{
    QWT_D(d);
    d->m_caption.setFont(family, pointSize, weight, italic);
}

void Qwt3DColorLegend::setLimits(double start, double stop)
{
    QWT_D(d);
    d->m_axis.setLimits(start, stop);
}

void Qwt3DColorLegend::setMajors(int majors)
{
    QWT_D(d);
    d->m_axis.setMajors(majors);
}

void Qwt3DColorLegend::setMinors(int minors)
{
    QWT_D(d);
    d->m_axis.setMinors(minors);
}

void Qwt3DColorLegend::setAutoScale(bool val)
{
    QWT_D(d);
    d->m_axis.setAutoScale(val);
}

void Qwt3DColorLegend::setScale(SCALETYPE val)
{
    QWT_D(d);
    d->m_axis.setScale(val);
}

void Qwt3DColorLegend::setScale(Qwt3DScale* val)
{
    QWT_D(d);
    d->m_axis.setScale(val);
}

void Qwt3DColorLegend::setOrientation(ORIENTATION orientation, SCALEPOSITION pos)
{
    QWT_D(d);
    d->m_orientation  = orientation;
    d->m_axisposition = pos;

    if (d->m_orientation == BottomTop) {
        if (d->m_axisposition == Bottom || d->m_axisposition == Top)
            d->m_axisposition = Left;
    } else {
        if (d->m_axisposition == Left || d->m_axisposition == Right)
            d->m_axisposition = Bottom;
    }
}

void Qwt3DColorLegend::setRelPosition(Tuple relMin, Tuple relMax)
{
    QWT_D(d);
    d->m_relMin = relMin;
    d->m_relMax = relMax;
    d->m_position = PosCustom;
    d->m_useAbsolute = false;
}

void Qwt3DColorLegend::setPosition(Position pos)
{
    QWT_D(d);
    d->m_position = pos;
    d->m_useAbsolute = false;

    // Default legend dimensions (relative to viewport)
    const double margin = 0.02;
    const double legendW = 0.03;
    const double legendH = 0.32;

    if (pos == PosCustom)
        return;

    double left, right, top, bottom;

    // Horizontal placement
    switch (pos) {
    case PosTopLeft: case PosLeftCenter: case PosBottomLeft:
        left = margin;
        right = margin + legendW;
        break;
    case PosTopCenter: case PosCenter: case PosBottomCenter:
        left = 0.5 - legendW / 2;
        right = 0.5 + legendW / 2;
        break;
    case PosTopRight: case PosRightCenter: case PosBottomRight:
        right = 1.0 - margin;
        left = right - legendW;
        break;
    default:
        return;
    }

    // Vertical placement (rel.y: 0 = top, 1 = bottom, Qt convention)
    switch (pos) {
    case PosTopLeft: case PosTopCenter: case PosTopRight:
        top = margin;
        bottom = margin + legendH;
        break;
    case PosLeftCenter: case PosCenter: case PosRightCenter:
        top = 0.5 - legendH / 2;
        bottom = 0.5 + legendH / 2;
        break;
    case PosBottomLeft: case PosBottomCenter: case PosBottomRight:
        bottom = 1.0 - margin;
        top = bottom - legendH;
        break;
    default:
        return;
    }

    d->m_relMin = Tuple(left, top);
    d->m_relMax = Tuple(right, bottom);

    if (plot())
        plot()->update();
}

void Qwt3DColorLegend::setAbsolutePosition(const QRectF& pixelRect)
{
    QWT_D(d);
    d->m_absoluteRect = pixelRect;
    d->m_useAbsolute = true;
    d->m_position = PosCustom;

    if (plot())
        plot()->update();
}

Qwt3DColorLegend::Position Qwt3DColorLegend::position() const
{
    QWT_DC(d);
    return d->m_position;
}

void Qwt3DColorLegend::setGeometryInternal()
{
    QWT_D(d);

    if (!plot())
        return;

    Tuple relMin = d->m_relMin;
    Tuple relMax = d->m_relMax;

    // Convert absolute pixel coordinates to relative on each draw
    // so the legend tracks viewport resize correctly
    if (d->m_useAbsolute) {
        QSize vp = plot()->viewportSize();
        if (vp.width() > 0 && vp.height() > 0) {
            relMin = Tuple(d->m_absoluteRect.left() / vp.width(),
                           d->m_absoluteRect.top() / vp.height());
            relMax = Tuple(d->m_absoluteRect.right() / vp.width(),
                           d->m_absoluteRect.bottom() / vp.height());
        }
    }

    d->m_pe.minVertex = relativePosition(Triple(relMin.x, relMin.y, 0.99));
    d->m_pe.maxVertex = relativePosition(Triple(relMax.x, relMax.y, 0.99));

    double diff = 0;
    Triple b;
    Triple e;

    switch (d->m_axisposition) {
    case Qwt3DColorLegend::Left:
        b   = d->m_pe.minVertex;
        e   = d->m_pe.maxVertex;
        e.x = b.x;
        d->m_axis.setTicOrientation(-1, 0, 0);
        d->m_axis.setNumberAnchor(CenterRight);
        diff = d->m_pe.maxVertex.x - d->m_pe.minVertex.x;
        break;
    case Qwt3DColorLegend::Right:
        e   = d->m_pe.maxVertex;
        b   = d->m_pe.minVertex;
        b.x = e.x;
        d->m_axis.setTicOrientation(+1, 0, 0);
        d->m_axis.setNumberAnchor(CenterLeft);
        diff = d->m_pe.maxVertex.x - d->m_pe.minVertex.x;
        break;
    case Qwt3DColorLegend::Top:
        e   = d->m_pe.maxVertex;
        b   = d->m_pe.minVertex;
        b.z = e.z;
        d->m_axis.setTicOrientation(0, 0, +1);
        d->m_axis.setNumberAnchor(BottomCenter);
        diff = d->m_pe.maxVertex.z - d->m_pe.minVertex.z;
        break;
    case Qwt3DColorLegend::Bottom:
        b   = d->m_pe.minVertex;
        e   = d->m_pe.maxVertex;
        e.z = b.z;
        d->m_axis.setTicOrientation(0, 0, -1);
        d->m_axis.setNumberAnchor(TopCenter);
        diff = d->m_pe.maxVertex.z - d->m_pe.minVertex.z;
        break;
    default:
        break;
    }

    d->m_axis.setPosition(b, e);
    diff /= 10;

    d->m_axis.setTicLength(diff, 0.6 * diff);

    Triple c;
    c.x = d->m_pe.minVertex.x + ((d->m_pe.maxVertex - d->m_pe.minVertex) / 2).x;
    c.z = d->m_pe.maxVertex.z;
    c.z += (d->m_pe.maxVertex.z - d->m_pe.minVertex.z) / 20;
    c.y = d->m_pe.maxVertex.y;

    d->m_caption.setPosition(c, BottomCenter);
}

ParallelEpiped Qwt3DColorLegend::geometry() const
{
    QWT_DC(d);
    return d->m_pe;
}

void Qwt3DColorLegend::drawScale(bool val)
{
    QWT_D(d);
    d->m_showaxis = val;
}

void Qwt3DColorLegend::drawNumbers(bool val)
{
    QWT_D(d);
    d->m_axis.setNumbers(val);
}

/**
 * @brief Draws the color legend using VBO + polygon/line shaders
 * @details Renders the color bar as a set of quads using VBO + polygon shader,
 *          the border outline using VBO + line shader, then delegates axis
 *          and caption drawing to their respective draw() methods.
 */
void Qwt3DColorLegend::draw()
{
    if (colors.empty())
        return;

    QWT_D(d);

    setGeometryInternal();

    // Ensure axis and caption have plot pointer
    d->m_axis.setPlot(plot());
    d->m_caption.setPlot(plot());

    Triple one = d->m_pe.minVertex;
    Triple two = d->m_pe.maxVertex;

    double h = (d->m_orientation == Qwt3DColorLegend::BottomTop) ? (two - one).z / colors.size()
                                                            : (two - one).x / colors.size();

    // --- Draw color bar quads using VBO + polygon shader ---
    if (plot()) {
        auto* polyShader = plot()->polygonShader();
        auto* lineShader = plot()->lineShader();
        auto* f = QOpenGLContext::currentContext()->functions();

        if (polyShader) {
            // Build quad vertices: position(3) + color(4) = 7 floats per vertex
            QVector<float> verts;
            size_t size = colors.size();

            if (d->m_orientation == Qwt3DColorLegend::BottomTop) {
                for (unsigned i = 1; i <= size; ++i) {
                    const RGBA& rgb = colors[ i - 1 ];
                    float cr = static_cast< float >(rgb.r);
                    float cg = static_cast< float >(rgb.g);
                    float cb = static_cast< float >(rgb.b);
                    float ca = static_cast< float >(rgb.a);

                    // Quad: BL, TL, BR, TR (triangle strip)
                    // BL
                    verts << static_cast< float >(one.x) << static_cast< float >(one.y) << static_cast< float >(one.z + (i - 1) * h)
                          << cr << cg << cb << ca;
                    // TL
                    verts << static_cast< float >(one.x) << static_cast< float >(one.y) << static_cast< float >(one.z + i * h)
                          << cr << cg << cb << ca;
                    // BR
                    verts << static_cast< float >(two.x) << static_cast< float >(one.y) << static_cast< float >(one.z + (i - 1) * h)
                          << cr << cg << cb << ca;
                    // TR
                    verts << static_cast< float >(two.x) << static_cast< float >(one.y) << static_cast< float >(one.z + i * h)
                          << cr << cg << cb << ca;
                }
            } else {
                for (unsigned i = 1; i <= size; ++i) {
                    const RGBA& rgb = colors[ i - 1 ];
                    float cr = static_cast< float >(rgb.r);
                    float cg = static_cast< float >(rgb.g);
                    float cb = static_cast< float >(rgb.b);
                    float ca = static_cast< float >(rgb.a);

                    // BL
                    verts << static_cast< float >(one.x + (i - 1) * h) << static_cast< float >(one.y) << static_cast< float >(one.z)
                          << cr << cg << cb << ca;
                    // TL
                    verts << static_cast< float >(one.x + (i - 1) * h) << static_cast< float >(one.y) << static_cast< float >(two.z)
                          << cr << cg << cb << ca;
                    // BR
                    verts << static_cast< float >(one.x + i * h) << static_cast< float >(one.y) << static_cast< float >(one.z)
                          << cr << cg << cb << ca;
                    // TR
                    verts << static_cast< float >(one.x + i * h) << static_cast< float >(one.y) << static_cast< float >(two.z)
                          << cr << cg << cb << ca;
                }
            }

            QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
            vbo.create();
            vbo.bind();
            vbo.allocate(verts.constData(), verts.size() * sizeof(float));

            polyShader->bind();
            polyShader->setUniformValue("uModelView", plot()->modelViewMatrix());
            polyShader->setUniformValue("uProjection", plot()->projectionMatrix());
            polyShader->setUniformValue("uAlpha", 1.0f);

            int stride = 7 * sizeof(float);
            polyShader->enableAttributeArray(0);
            polyShader->setAttributeBuffer(0, GL_FLOAT, 0, 3, stride);
            polyShader->enableAttributeArray(1);
            polyShader->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 4, stride);

            // Draw each quad as a triangle strip
            int vertsPerQuad = 4;
            for (int i = 0; i < static_cast< int >(colors.size()); ++i) {
                f->glDrawArrays(GL_TRIANGLE_STRIP, i * vertsPerQuad, vertsPerQuad);
            }

            polyShader->disableAttributeArray(0);
            polyShader->disableAttributeArray(1);
            polyShader->release();
            vbo.release();
            vbo.destroy();
        }

        // --- Draw border outline using VBO + line shader ---
        if (lineShader) {
            float br = 0.0f, bg = 0.0f, bb = 0.0f, ba = 1.0f;  // black border
            QVector<float> lineVerts;
            // BL -> TL
            lineVerts << static_cast< float >(one.x) << static_cast< float >(one.y) << static_cast< float >(one.z) << br << bg << bb << ba;
            lineVerts << static_cast< float >(one.x) << static_cast< float >(one.y) << static_cast< float >(two.z) << br << bg << bb << ba;
            // TL -> TR
            lineVerts << static_cast< float >(one.x) << static_cast< float >(one.y) << static_cast< float >(two.z) << br << bg << bb << ba;
            lineVerts << static_cast< float >(two.x) << static_cast< float >(one.y) << static_cast< float >(two.z) << br << bg << bb << ba;
            // TR -> BR
            lineVerts << static_cast< float >(two.x) << static_cast< float >(one.y) << static_cast< float >(two.z) << br << bg << bb << ba;
            lineVerts << static_cast< float >(two.x) << static_cast< float >(one.y) << static_cast< float >(one.z) << br << bg << bb << ba;
            // BR -> BL
            lineVerts << static_cast< float >(two.x) << static_cast< float >(one.y) << static_cast< float >(one.z) << br << bg << bb << ba;
            lineVerts << static_cast< float >(one.x) << static_cast< float >(one.y) << static_cast< float >(one.z) << br << bg << bb << ba;

            QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
            vbo.create();
            vbo.bind();
            vbo.allocate(lineVerts.constData(), lineVerts.size() * sizeof(float));

            lineShader->bind();
            lineShader->setUniformValue("uModelView", plot()->modelViewMatrix());
            lineShader->setUniformValue("uProjection", plot()->projectionMatrix());

            int stride = 7 * sizeof(float);
            lineShader->enableAttributeArray(0);
            lineShader->setAttributeBuffer(0, GL_FLOAT, 0, 3, stride);
            lineShader->enableAttributeArray(1);
            lineShader->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 4, stride);

            f->glLineWidth(1.0f);
            f->glDrawArrays(GL_LINES, 0, lineVerts.size() / 7);

            lineShader->disableAttributeArray(0);
            lineShader->disableAttributeArray(1);
            lineShader->release();
            vbo.release();
            vbo.destroy();
        }
    }

    // Draw axis and caption
    if (d->m_showaxis)
        d->m_axis.draw();

    d->m_caption.draw();
}
