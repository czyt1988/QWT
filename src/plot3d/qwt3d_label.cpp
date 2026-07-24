#include <qbitmap.h>
#include "qwt3d_label.h"

#include "qwt3d_plot.h"
#include "qwt3d_io_gl2ps.h"

#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>

namespace
{
bool deviceFonts = false;
}

class Qwt3DLabel::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DLabel)

public:
    PrivateData(Qwt3DLabel* q)
        : q_ptr(q)
        , m_beg(0.0, 0.0, 0.0)
        , m_end(0.0, 0.0, 0.0)
        , m_pos(0.0, 0.0, 0.0)
        , m_pm(0, 0)
        , m_font()
        , m_anchor(BottomLeft)
        , m_gap(0)
        , m_flagForUpdate(true)
    {
    }

    Triple m_beg;
    Triple m_end;
    Triple m_pos;
    QPixmap m_pm;
    QImage m_buf;
    QImage m_tex;
    QFont m_font;
    QString m_text;
    ANCHOR m_anchor;
    int m_gap;
    bool m_flagForUpdate;
    float m_ndcZ = 0.0f;
};

Qwt3DLabel::Qwt3DLabel() : QWT_PIMPL_CONSTRUCT
{
    init();
}

Qwt3DLabel::Qwt3DLabel(const QString& family, int pointSize, int weight, bool italic) : QWT_PIMPL_CONSTRUCT
{
    init(family, pointSize, weight, italic);
}

Qwt3DLabel::~Qwt3DLabel() = default;

Qwt3DLabel::Qwt3DLabel(const Qwt3DLabel& other) : Qwt3DDrawable(), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    const PrivateData* od = other.d_func();
    d->m_beg              = od->m_beg;
    d->m_end              = od->m_end;
    d->m_pos              = od->m_pos;
    d->m_pm               = od->m_pm;
    d->m_buf              = od->m_buf;
    d->m_tex              = od->m_tex;
    d->m_font             = od->m_font;
    d->m_text             = od->m_text;
    d->m_anchor           = od->m_anchor;
    d->m_gap              = od->m_gap;
    d->m_flagForUpdate    = od->m_flagForUpdate;
    color                 = other.color;
    m_plot                = other.m_plot;
}

Qwt3DLabel::Qwt3DLabel(Qwt3DLabel&& other) noexcept : Qwt3DDrawable(std::move(other)), m_data(std::move(other.m_data))
{
}

Qwt3DLabel& Qwt3DLabel::operator=(const Qwt3DLabel& other)
{
    if (this != &other) {
        QWT_D(d);
        const PrivateData* od = other.d_func();
        d->m_beg              = od->m_beg;
        d->m_end              = od->m_end;
        d->m_pos              = od->m_pos;
        d->m_pm               = od->m_pm;
        d->m_buf              = od->m_buf;
        d->m_tex              = od->m_tex;
        d->m_font             = od->m_font;
        d->m_text             = od->m_text;
        d->m_anchor           = od->m_anchor;
        d->m_gap              = od->m_gap;
        d->m_flagForUpdate    = od->m_flagForUpdate;
        color                 = other.color;
        m_plot                = other.m_plot;
    }
    return *this;
}

Qwt3DLabel& Qwt3DLabel::operator=(Qwt3DLabel&& other) noexcept
{
    if (this != &other) {
        Qwt3DDrawable::operator=(std::move(other));
        m_data = std::move(other.m_data);
    }
    return *this;
}

void Qwt3DLabel::init(const QString& family, int pointSize, int weight, bool italic)
{
    init();
    QWT_D(d);
    d->m_font = QFont(family, pointSize, weight, italic);
}

void Qwt3DLabel::init()
{
    QWT_D(d);
    d->m_beg = Triple(0.0, 0.0, 0.0);
    d->m_end = d->m_beg;
    d->m_pos = d->m_beg;
    setColor(0, 0, 0);
    d->m_pm            = QPixmap(0, 0);
    d->m_font          = QFont();
    d->m_anchor        = BottomLeft;
    d->m_gap           = 0;
    d->m_flagForUpdate = true;
}

void Qwt3DLabel::useDeviceFonts(bool val)
{
    deviceFonts = val;
}

void Qwt3DLabel::setFont(const QString& family, int pointSize, int weight, bool italic)
{
    QWT_D(d);
    d->m_font          = QFont(family, pointSize, weight, italic);
    d->m_flagForUpdate = true;
}

void Qwt3DLabel::setString(QString const& s)
{
    QWT_D(d);
    d->m_text          = s;
    d->m_flagForUpdate = true;
}

void Qwt3DLabel::setColor(double r, double g, double b, double a)
{
    Qwt3DDrawable::setColor(r, g, b, a);
    QWT_D(d);
    d->m_flagForUpdate = true;
}

void Qwt3DLabel::setColor(RGBA rgba)
{
    Qwt3DDrawable::setColor(rgba);
    QWT_D(d);
    d->m_flagForUpdate = true;
}

void Qwt3DLabel::setPosition(Triple pos, ANCHOR a)
{
    QWT_D(d);
    d->m_anchor = a;
    d->m_pos    = pos;
}

void Qwt3DLabel::setRelPosition(Tuple rpos, ANCHOR a)
{
    QWT_D(d);
    d->m_anchor = a;
    d->m_beg = relativePosition(Triple(rpos.x, rpos.y, 0.99));
    setPosition(d->m_beg, a);
}

void Qwt3DLabel::update()
{
    QWT_D(d);
    QPainter p;
    QFontMetrics fm(d->m_font);

    QFontInfo info(d->m_font);

    QRect r = QRect(QPoint(0, 0), fm.size(SingleLine, d->m_text));

    r.translate(0, -r.top());

    d->m_pm = QPixmap(r.width(), r.bottom());

    if (d->m_pm.isNull())
    {
        r = QRect(QPoint(0, 0), fm.size(SingleLine, QString(" ")));
        r.translate(0, -r.top());
        d->m_pm = QPixmap(r.width(), r.bottom());
    }

    QBitmap bm(d->m_pm.width(), d->m_pm.height());
    bm.fill(Qt::color0);
    p.begin(&bm);
    p.setPen(Qt::color1);
    p.setFont(d->m_font);
    p.drawText(0, r.height() - fm.descent() - 1, d->m_text);
    p.end();

    d->m_pm.setMask(bm);

    p.begin(&d->m_pm);
    p.setFont(d->m_font);
    p.setPen(Qt::SolidLine);
    p.setPen(GL2Qt(color.r, color.g, color.b));

    p.drawText(0, r.height() - fm.descent() - 1, d->m_text);
    p.end();
    d->m_buf = d->m_pm.toImage();
    d->m_tex = d->m_buf.mirrored();
}

void Qwt3DLabel::adjust(int gap)
{
    QWT_D(d);
    d->m_gap = gap;
}

void Qwt3DLabel::convert2screen()
{
    QWT_D(d);
    if (!plot())
        return;

    Qwt3DPlot* p = plot();
    QMatrix4x4 mvp = p->projectionMatrix() * p->modelViewMatrix();

    // Compute NDC z of the label position
    QVector4D posVec(static_cast< float >(d->m_pos.x),
                     static_cast< float >(d->m_pos.y),
                     static_cast< float >(d->m_pos.z),
                     1.0f);
    QVector4D clip = mvp.map(posVec);
    d->m_ndcZ = 0.0f;
    if (clip.w() != 0.0f)
        d->m_ndcZ = clip.z() / clip.w();

    QSize vp = p->viewportSize();
    if (vp.width() <= 0 || vp.height() <= 0)
        return;

    // Helper to convert screen + NDC z back to world
    auto screenToWorldZ = [&](const QPointF& s) -> Triple {
        float ndcX = 2.0f * static_cast< float >(s.x()) / vp.width() - 1.0f;
        float ndcY = 1.0f - 2.0f * static_cast< float >(s.y()) / vp.height();
        QVector4D ndc(ndcX, ndcY, d->m_ndcZ, 1.0f);
        QVector4D world = mvp.inverted().map(ndc);
        if (world.w() != 0.0f)
            return Triple(world.x() / world.w(), world.y() / world.w(), world.z() / world.w());
        return Triple(0, 0, 0);
    };

    QPointF screen = p->worldToScreen(d->m_pos);

    double w = width();
    double h = height();

    switch (d->m_anchor) {
    case BottomLeft:
        d->m_beg = d->m_pos;
        break;
    case BottomRight:
        d->m_beg = screenToWorldZ(screen - QPointF(w + d->m_gap, 0));
        break;
    case BottomCenter:
        d->m_beg = screenToWorldZ(screen - QPointF(w / 2, -d->m_gap));
        break;
    case TopRight:
        d->m_beg = screenToWorldZ(screen - QPointF(w + d->m_gap, h));
        break;
    case TopLeft:
        d->m_beg = screenToWorldZ(screen - QPointF(-d->m_gap, h));
        break;
    case TopCenter:
        d->m_beg = screenToWorldZ(screen - QPointF(w / 2, h + d->m_gap));
        break;
    case CenterLeft:
        d->m_beg = screenToWorldZ(screen - QPointF(-d->m_gap, h / 2));
        break;
    case CenterRight:
        d->m_beg = screenToWorldZ(screen - QPointF(w + d->m_gap, h / 2));
        break;
    case Center:
        d->m_beg = screenToWorldZ(screen - QPointF(w / 2, h / 2));
        break;
    default:
        break;
    }

    QPointF begScreen = p->worldToScreen(d->m_beg);
    d->m_end = screenToWorldZ(begScreen + QPointF(w, h));
}

/**
 * @brief Draws the label using a texture quad with GLSL text shader
 * @details Renders the text to a QImage, creates an OpenGL texture,
 *          and draws a textured quad using VBO + text.vert/text.frag shaders.
 *          For gl2ps vector export (deviceFonts mode), falls back to
 *          drawDeviceText.
 */
void Qwt3DLabel::draw()
{
    QWT_D(d);
    if (d->m_flagForUpdate) {
        update();
        d->m_flagForUpdate = false;
    }

    if (d->m_buf.isNull())
        return;

    convert2screen();

    if (!plot())
        return;

    // gl2ps vector export path: use device text for vector output
#ifdef QWT3D_ENABLE_GL2PS
    if (deviceFonts) {
        drawDeviceText(QWT3DLOCAL8BIT(d->m_text), "Courier", d->m_font.pointSize(),
                       d->m_pos, color, d->m_anchor, d->m_gap);
        return;
    }
#endif

    auto* shader = plot()->textShader();
    if (!shader)
        return;

    auto* f = QOpenGLContext::currentContext()->functions();

    // Create texture from the text image (alpha channel is used by shader)
    QOpenGLTexture texture(d->m_tex);
    texture.setMinificationFilter(QOpenGLTexture::Linear);
    texture.setMagnificationFilter(QOpenGLTexture::Linear);
    texture.setWrapMode(QOpenGLTexture::ClampToEdge);

    // Compute all four quad corners via screen-to-world conversion.
    // m_beg and m_end are two diagonal corners (BL, TR) in world space,
    // but the other two corners cannot be synthesized by mixing x/y/z
    // components — they must be computed independently from screen space.
    Qwt3DPlot* p = plot();
    QMatrix4x4 mvp = p->projectionMatrix() * p->modelViewMatrix();
    QSize vp = p->viewportSize();

    QPointF begScreen = p->worldToScreen(d->m_beg);
    QPointF endScreen = p->worldToScreen(d->m_end);

    auto screenToWorldZ = [&](const QPointF& s) -> Triple {
        if (vp.width() <= 0 || vp.height() <= 0)
            return Triple(0, 0, 0);
        float ndcX = 2.0f * static_cast< float >(s.x()) / vp.width() - 1.0f;
        float ndcY = 1.0f - 2.0f * static_cast< float >(s.y()) / vp.height();
        QVector4D ndc(ndcX, ndcY, d->m_ndcZ, 1.0f);
        QVector4D world = mvp.inverted().map(ndc);
        if (world.w() != 0.0f)
            return Triple(world.x() / world.w(), world.y() / world.w(), world.z() / world.w());
        return Triple(0, 0, 0);
    };

    Triple bl = d->m_beg;
    Triple tr = d->m_end;
    Triple tl = screenToWorldZ(QPointF(begScreen.x(), endScreen.y()));
    Triple br = screenToWorldZ(QPointF(endScreen.x(), begScreen.y()));

    // Build quad vertices: position(3) + texcoord(2) = 5 floats per vertex
    // Triangle strip order: BL, TL, BR, TR
    QVector<float> verts;
    // Bottom-left
    verts << static_cast< float >(bl.x) << static_cast< float >(bl.y) << static_cast< float >(bl.z)
          << 0.0f << 1.0f;
    // Top-left
    verts << static_cast< float >(tl.x) << static_cast< float >(tl.y) << static_cast< float >(tl.z)
          << 0.0f << 0.0f;
    // Bottom-right
    verts << static_cast< float >(br.x) << static_cast< float >(br.y) << static_cast< float >(br.z)
          << 1.0f << 1.0f;
    // Top-right
    verts << static_cast< float >(tr.x) << static_cast< float >(tr.y) << static_cast< float >(tr.z)
          << 1.0f << 0.0f;

    QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(verts.constData(), verts.size() * sizeof(float));

    shader->bind();
    shader->setUniformValue("uModelView", plot()->modelViewMatrix());
    shader->setUniformValue("uProjection", plot()->projectionMatrix());
    shader->setUniformValue("uTextTexture", 0);
    shader->setUniformValue("uTextColor",
                            QVector4D(static_cast< float >(color.r),
                                      static_cast< float >(color.g),
                                      static_cast< float >(color.b),
                                      static_cast< float >(color.a)));

    texture.bind(0);

    int stride = 5 * sizeof(float);
    shader->enableAttributeArray(0);
    shader->setAttributeBuffer(0, GL_FLOAT, 0, 3, stride);
    shader->enableAttributeArray(1);
    shader->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 2, stride);

    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    f->glDisable(GL_DEPTH_TEST);
    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    f->glEnable(GL_DEPTH_TEST);

    shader->disableAttributeArray(0);
    shader->disableAttributeArray(1);
    shader->release();
    texture.release();
    vbo.release();
    vbo.destroy();
}

double Qwt3DLabel::width() const
{
    QWT_DC(d);
    return d->m_pm.width();
}

double Qwt3DLabel::height() const
{
    QWT_DC(d);
    return d->m_pm.height();
}

double Qwt3DLabel::gap() const
{
    QWT_DC(d);
    return d->m_gap;
}

Triple Qwt3DLabel::first() const
{
    QWT_DC(d);
    return d->m_beg;
}

Triple Qwt3DLabel::second() const
{
    QWT_DC(d);
    return d->m_end;
}

ANCHOR Qwt3DLabel::anchor() const
{
    QWT_DC(d);
    return d->m_anchor;
}
