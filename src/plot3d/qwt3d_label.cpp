#include "qwt3d_label.h"

#include "qwt3d_io_gl2ps.h"
#include "qwt3d_textbatch.h"

#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QGuiApplication>
#include <QScreen>

#include <algorithm>
#include <cmath>

namespace
{
bool deviceFonts = false;

//! DPI the rasterized label text currently depends on
/*!
  The text is rasterized into an offscreen QPixmap, whose paint device
  resolves a point sized font against the primary screen - the widget the
  label is finally drawn on does not take part in it. A screen change can
  therefore change the pixel size of the glyphs without touching the font,
  the string or the color of the label.
 */
qreal labelFontDpi()
{
    const QScreen* screen = QGuiApplication::primaryScreen();
    return screen != nullptr ? screen->logicalDotsPerInch() : 96.0;
}

bool sameColor(const RGBA& lhs, const RGBA& rhs)
{
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

//! Releases a label texture
/*!
  The GL name can only be deleted while the context that created it is
  current. When that context is already gone the name died with it, so
  dropping the handle is enough - and avoids Qt's warnings about textures
  that are not valid in the current context.
 */
void destroyTexture(QOpenGLTexture*& texture, QOpenGLContext* ownerContext)
{
    if (texture == nullptr)
        return;

    if (ownerContext != nullptr && QOpenGLContext::currentContext() == ownerContext)
        delete texture;

    texture = nullptr;
}
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
        , m_font()
        , m_anchor(BottomLeft)
        , m_gap(0)
        , m_flagForUpdate(true)
    {
    }

    Triple m_beg;
    Triple m_end;
    Triple m_pos;
    QImage m_buf;
    QImage m_tex;
    QFont m_font;
    QString m_text;
    ANCHOR m_anchor;
    int m_gap;
    bool m_flagForUpdate;
    float m_ndcZ                     = 0.0f;
    qreal m_fontDpi                  = 0.0;
    quint64 m_stamp                  = 0;
    QOpenGLTexture* m_texture        = nullptr;
    QOpenGLContext* m_textureContext = nullptr;
    bool m_textureDirty              = true;
};

Qwt3DLabel::Qwt3DLabel() : QWT_PIMPL_CONSTRUCT
{
    init();
}

Qwt3DLabel::Qwt3DLabel(const QString& family, int pointSize, int weight, bool italic) : QWT_PIMPL_CONSTRUCT
{
    init(family, pointSize, weight, italic);
}

Qwt3DLabel::~Qwt3DLabel()
{
    // A moved-from label has no PrivateData left (see Qwt3DDrawable::attach)
    if (!m_data)
        return;

    QWT_D(d);
    destroyTexture(d->m_texture, d->m_textureContext);
}

Qwt3DLabel::Qwt3DLabel(const Qwt3DLabel& other) : Qwt3DDrawable(), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    const PrivateData* od = other.d_func();
    d->m_beg              = od->m_beg;
    d->m_end              = od->m_end;
    d->m_pos              = od->m_pos;
    d->m_buf              = od->m_buf;
    d->m_tex              = od->m_tex;
    d->m_font             = od->m_font;
    d->m_text             = od->m_text;
    d->m_anchor           = od->m_anchor;
    d->m_gap              = od->m_gap;
    d->m_flagForUpdate    = od->m_flagForUpdate;
    d->m_fontDpi          = od->m_fontDpi;
    m_color               = other.m_color;

    // the GL texture belongs to the source label, this one builds its own
    d->m_texture        = nullptr;
    d->m_textureContext = nullptr;
    d->m_textureDirty   = true;
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
        d->m_buf              = od->m_buf;
        d->m_tex              = od->m_tex;
        d->m_font             = od->m_font;
        d->m_text             = od->m_text;
        d->m_anchor           = od->m_anchor;
        d->m_gap              = od->m_gap;
        d->m_flagForUpdate    = od->m_flagForUpdate;
        d->m_fontDpi          = od->m_fontDpi;
        m_color               = other.m_color;

        // the GL texture belongs to the source label, this one builds its own
        destroyTexture(d->m_texture, d->m_textureContext);
        d->m_textureContext = nullptr;
        d->m_textureDirty   = true;
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
    d->m_font          = QFont();
    d->m_anchor        = BottomLeft;
    d->m_gap           = 0;
    d->m_flagForUpdate = true;
}

void Qwt3DLabel::useDeviceFonts(bool val)
{
    deviceFonts = val;
}

/**
 * @brief Sets the label font
 * @details Assigning the font that is already active is a no-op: the axis
 *          redraw path reapplies font, color and text of every tic label on
 *          each frame, and re-rasterizing the text for unchanged properties
 *          is pure overhead (Qwt3DAxis::drawTicLabel()).
 */
void Qwt3DLabel::setFont(const QString& family, int pointSize, int weight, bool italic)
{
    QWT_D(d);

    const QFont font(family, pointSize, weight, italic);
    if (d->m_font == font)
        return;

    d->m_font          = font;
    d->m_flagForUpdate = true;
}

/**
 * @brief Sets the label text
 * @details Setting the string that is already active is a no-op, see setFont().
 */
void Qwt3DLabel::setString(QString const& s)
{
    QWT_D(d);

    if (d->m_text == s)
        return;

    d->m_text          = s;
    d->m_flagForUpdate = true;
}

/**
 * @brief 返回标签文本
 * @return 当前标签文本字符串
 */
QString Qwt3DLabel::string() const
{
    QWT_DC(d);
    return d->m_text;
}

/**
 * @brief Sets the label color
 * @details Setting the color that is already active is a no-op, see setFont().
 */
void Qwt3DLabel::setColor(double r, double g, double b, double a)
{
    if (sameColor(m_color, RGBA(r, g, b, a)))
        return;

    Qwt3DDrawable::setColor(r, g, b, a);
    QWT_D(d);
    d->m_flagForUpdate = true;
}

/**
 * @brief Sets the label color
 * @details Setting the color that is already active is a no-op, see setFont().
 */
void Qwt3DLabel::setColor(RGBA rgba)
{
    if (sameColor(m_color, rgba))
        return;

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

void Qwt3DLabel::setRelPosition(Tuple rpos, ANCHOR a, const Qwt3DRenderContext& ctx)
{
    QWT_D(d);
    d->m_anchor = a;
    d->m_beg    = ctx.relativePosition(Triple(rpos.x, rpos.y, 0.99));
    setPosition(d->m_beg, a);
}

/**
 * @brief Renders the label text into the internal texture image
 * @details The text is painted with antialiasing into a transparent image.
 *          The alpha channel of that image is what the text shader samples,
 *          so no 1 bit mask is involved: a mask rasterizes the glyphs in
 *          monochrome, which does not match the antialiased text and made the
 *          labels look doubled and aliased.
 */
void Qwt3DLabel::update()
{
    QWT_D(d);

    d->m_fontDpi      = labelFontDpi();
    d->m_textureDirty = true;
    ++d->m_stamp;

    if (d->m_text.isEmpty()) {
        d->m_buf = QImage();
        d->m_tex = QImage();
        return;
    }

    QFontMetrics fm(d->m_font);

    QRect r = QRect(QPoint(0, 0), fm.size(SingleLine, d->m_text));
    r.translate(0, -r.top());

    const int w = std::max(1, r.width());
    const int h = std::max(1, r.height());

    QImage canvas(w, h, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::transparent);

    // QPixmap keeps the font metrics / DPI behavior of the label, the
    // transparent base image gives it the alpha channel.
    QPixmap pm = QPixmap::fromImage(canvas);

    QPainter p(&pm);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setFont(d->m_font);
    p.setPen(GL2Qt(m_color.r, m_color.g, m_color.b));
    p.drawText(0, r.height() - fm.descent() - 1, d->m_text);
    p.end();

    d->m_buf = pm.toImage();
    d->m_tex = d->m_buf.mirrored();
}

void Qwt3DLabel::adjust(int gap)
{
    QWT_D(d);
    d->m_gap = gap;
}

void Qwt3DLabel::convert2screen(const Qwt3DRenderContext& ctx)
{
    QWT_D(d);

    const QMatrix4x4 mvp = ctx.projection * ctx.modelView;

    // Compute NDC z of the label position
    QVector4D posVec(
        static_cast< float >(d->m_pos.x), static_cast< float >(d->m_pos.y), static_cast< float >(d->m_pos.z), 1.0f);
    QVector4D clip = mvp.map(posVec);
    d->m_ndcZ      = 0.0f;
    if (clip.w() != 0.0f)
        d->m_ndcZ = clip.z() / clip.w();

    if (ctx.viewport.width() <= 0 || ctx.viewport.height() <= 0)
        return;

    const QMatrix4x4 invMvp = mvp.inverted();

    // Helper to convert screen + NDC z back to world
    auto screenToWorldZ = [ & ](const QPointF& s) -> Triple {
        float ndcX = 2.0f * static_cast< float >(s.x()) / ctx.viewport.width() - 1.0f;
        float ndcY = 1.0f - 2.0f * static_cast< float >(s.y()) / ctx.viewport.height();
        QVector4D ndc(ndcX, ndcY, d->m_ndcZ, 1.0f);
        QVector4D world = invMvp.map(ndc);
        if (world.w() != 0.0f)
            return Triple(world.x() / world.w(), world.y() / world.w(), world.z() / world.w());
        return Triple(0, 0, 0);
    };

    const double w = width();
    const double h = height();

    QPointF beg = ctx.worldToScreen(d->m_pos);

    switch (d->m_anchor) {
    case BottomLeft:
        break;
    case BottomRight:
        beg -= QPointF(w + d->m_gap, 0);
        break;
    case BottomCenter:
        beg -= QPointF(w / 2, -d->m_gap);
        break;
    case TopRight:
        beg -= QPointF(w + d->m_gap, h);
        break;
    case TopLeft:
        beg -= QPointF(-d->m_gap, h);
        break;
    case TopCenter:
        beg -= QPointF(w / 2, h + d->m_gap);
        break;
    case CenterLeft:
        beg -= QPointF(-d->m_gap, h / 2);
        break;
    case CenterRight:
        beg -= QPointF(w + d->m_gap, h / 2);
        break;
    case Center:
        beg -= QPointF(w / 2, h / 2);
        break;
    default:
        break;
    }

    // The label is a screen aligned textured quad drawn 1:1 in device pixels.
    // Take the label rectangle as integral pixel rectangle, otherwise the
    // bilinear texture filter spreads every glyph edge over two pixels, which
    // reads as blurred, doubled text.
    beg = QPointF(std::round(beg.x()), std::round(beg.y()));

    d->m_beg = screenToWorldZ(beg);
    d->m_end = screenToWorldZ(beg + QPointF(w, h));
}

/**
 * @brief Draws the label using a texture quad with GLSL text shader
 * @param ctx Render context providing shaders, matrices, and coordinate conversion
 * @details Renders the text to a QImage, creates an OpenGL texture,
 *          and draws a textured quad using VBO + text.vert/text.frag shaders.
 *          For gl2ps vector export (deviceFonts mode), falls back to
 *          drawDeviceText.
 */
void Qwt3DLabel::draw(const Qwt3DRenderContext& ctx)
{
    QWT_D(d);
    if (d->m_flagForUpdate || d->m_fontDpi != labelFontDpi()) {
        update();
        d->m_flagForUpdate = false;
    }

    if (d->m_buf.isNull())
        return;

    convert2screen(ctx);

    // gl2ps vector export path: use device text for vector output
    if (deviceFonts) {
        drawDeviceText(QWT3DLOCAL8BIT(d->m_text), "Courier", d->m_font.pointSize(), d->m_pos, m_color, d->m_anchor, d->m_gap);
        return;
    }

    // Compute all four quad corners via screen-to-world conversion.
    // m_beg and m_end are two diagonal corners (BL, TR) in world space,
    // but the other two corners cannot be synthesized by mixing x/y/z
    // components - they must be computed independently from screen space.
    // The rectangle is rebuilt from the pixel snapped origin of m_beg, so the
    // quad always covers exactly width() x height() device pixels.
    const QMatrix4x4 mvp    = ctx.projection * ctx.modelView;
    const QMatrix4x4 invMvp = mvp.inverted();
    const QSize vp          = ctx.viewport;

    auto screenToWorldZ = [ & ](const QPointF& s) -> Triple {
        if (vp.width() <= 0 || vp.height() <= 0)
            return Triple(0, 0, 0);
        float ndcX = 2.0f * static_cast< float >(s.x()) / vp.width() - 1.0f;
        float ndcY = 1.0f - 2.0f * static_cast< float >(s.y()) / vp.height();
        QVector4D ndc(ndcX, ndcY, d->m_ndcZ, 1.0f);
        QVector4D world = invMvp.map(ndc);
        if (world.w() != 0.0f)
            return Triple(world.x() / world.w(), world.y() / world.w(), world.z() / world.w());
        return Triple(0, 0, 0);
    };

    const QPointF begWorld = ctx.worldToScreen(d->m_beg);
    const QPointF begScreen(std::round(begWorld.x()), std::round(begWorld.y()));
    const QPointF endScreen = begScreen + QPointF(width(), height());

    const Triple bl = screenToWorldZ(begScreen);
    const Triple tr = screenToWorldZ(endScreen);
    const Triple tl = screenToWorldZ(QPointF(begScreen.x(), endScreen.y()));
    const Triple br = screenToWorldZ(QPointF(endScreen.x(), begScreen.y()));

    // Batched path: the quad is collected by the plot and drawn together with
    // all other labels in one call. Falls back to the per label path when the
    // atlas has no room left - it grows for the next frame then.
    if (ctx.textBatch != nullptr) {
        QRectF uv;
        if (ctx.textBatch->addLabel(this, d->m_tex, d->m_stamp, uv)) {
            ctx.textBatch->addQuad(bl, tl, br, tr, uv, m_color);
            return;
        }
    }

    auto* shader = ctx.textShader;
    if (!shader)
        return;

    auto* context = QOpenGLContext::currentContext();
    if (context == nullptr)
        return;

    auto* f = context->functions();

    // The texture is only rebuilt when the text image or the GL context
    // changed: creating and uploading a texture for every label on every
    // frame was the dominant cost of the 3D text rendering.
    if (d->m_texture == nullptr || d->m_textureContext != context || d->m_textureDirty) {
        destroyTexture(d->m_texture, d->m_textureContext);

        d->m_texture = new QOpenGLTexture(d->m_tex);
        d->m_texture->setMinificationFilter(QOpenGLTexture::Linear);
        d->m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        d->m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);

        d->m_textureContext = context;
        d->m_textureDirty   = false;
    }
    QOpenGLTexture* texture = d->m_texture;

    // Build quad vertices: position(3) + texcoord(2) = 5 floats per vertex
    // Triangle strip order: BL, TL, BR, TR
    QVector< float > verts;
    // Bottom-left
    verts << static_cast< float >(bl.x) << static_cast< float >(bl.y) << static_cast< float >(bl.z) << 0.0f << 1.0f;
    // Top-left
    verts << static_cast< float >(tl.x) << static_cast< float >(tl.y) << static_cast< float >(tl.z) << 0.0f << 0.0f;
    // Bottom-right
    verts << static_cast< float >(br.x) << static_cast< float >(br.y) << static_cast< float >(br.z) << 1.0f << 1.0f;
    // Top-right
    verts << static_cast< float >(tr.x) << static_cast< float >(tr.y) << static_cast< float >(tr.z) << 1.0f << 0.0f;

    QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(verts.constData(), verts.size() * sizeof(float));

    shader->bind();
    shader->setUniformValue("uModelView", ctx.modelView);
    shader->setUniformValue("uProjection", ctx.projection);
    shader->setUniformValue("uTextTexture", 0);

    // the color is a vertex attribute; a single label has the same color for
    // all of its vertices, so a generic attribute value is enough
    shader->disableAttributeArray(2);
    shader->setAttributeValue(2,
                              QVector4D(static_cast< float >(m_color.r),
                                        static_cast< float >(m_color.g),
                                        static_cast< float >(m_color.b),
                                        static_cast< float >(m_color.a)));

    texture->bind(0);

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
    texture->release();
    vbo.release();
    vbo.destroy();
}

double Qwt3DLabel::width() const
{
    QWT_DC(d);
    return d->m_buf.width();
}

double Qwt3DLabel::height() const
{
    QWT_DC(d);
    return d->m_buf.height();
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
