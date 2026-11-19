#include "qwt3d_textbatch.h"

#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QPainter>

namespace
{
//! Interleaved vertex layout: position(3) + texcoord(2) + color(4)
const int VERTEX_FLOATS = 9;

const int ATLAS_MAX_HEIGHT = 2048;

//! Deletes a GL object, but only while the context that owns it is current
/*!
  When the owning context is already gone the GL name died with it, so
  dropping the handle is enough - and avoids Qt's warnings about objects
  that are not valid in the current context.
 */
template< typename T >
void releaseGLObject(T*& object, QOpenGLContext* owner)
{
    if (object == nullptr)
        return;

    if (owner != nullptr && QOpenGLContext::currentContext() == owner)
        delete object;

    object = nullptr;
}
}

Qwt3DTextBatch::Qwt3DTextBatch() = default;

Qwt3DTextBatch::~Qwt3DTextBatch()
{
    releaseGL();
}

/*!
  Drops the GL resources after the context was recreated. The atlas image and
  the packed cells are kept: the labels do not need to be rasterized or packed
  again, only the texture is uploaded anew on the next flush.
 */
void Qwt3DTextBatch::invalidate()
{
    releaseGL();
}

/*!
  Copies \a image into the atlas and returns its normalized texture rectangle.
  The image is only repacked/composited when \a stamp differs from the stamp of
  the previous call, so calling this for every label on every frame is cheap.

  @return false when the image does not fit into the atlas (yet); the caller
          should draw the label directly for this frame.
 */
bool Qwt3DTextBatch::addLabel(const void* key, const QImage& image, quint64 stamp, QRectF& uv)
{
    if (key == nullptr || image.isNull())
        return false;

    if (image.width() > m_size.width() || image.height() > m_size.height())
        return false;

    Entry& entry = m_entries[ key ];

    if (!entry.cell.isNull() && entry.cell.width() >= image.width() && entry.cell.height() >= image.height()) {
        // the cell is large enough, also after the label text became shorter
        if (entry.stamp != stamp || entry.size != image.size()) {
            entry.stamp = stamp;
            entry.size  = image.size();
            composite(QRect(entry.cell.topLeft(), entry.size), image);
        }
    } else {
        QRect cell;
        if (!pack(image.size(), cell)) {
            m_entries.remove(key);
            m_growPending = true;
            return false;
        }

        entry.cell  = cell;
        entry.size  = image.size();
        entry.stamp = stamp;
        composite(QRect(cell.topLeft(), entry.size), image);
    }

    const QRect used(entry.cell.topLeft(), entry.size);
    uv = QRectF(double(used.x()) / m_size.width(),
                double(used.y()) / m_size.height(),
                double(used.width()) / m_size.width(),
                double(used.height()) / m_size.height());
    return true;
}

/*!
  Appends one screen aligned quad as two triangles.
 */
void Qwt3DTextBatch::addQuad(const Triple& bl,
                             const Triple& tl,
                             const Triple& br,
                             const Triple& tr,
                             const QRectF& uv,
                             const RGBA& color)
{
    const float u0 = static_cast< float >(uv.left());
    const float v0 = static_cast< float >(uv.top());
    const float u1 = static_cast< float >(uv.right());
    const float v1 = static_cast< float >(uv.bottom());

    const float r = static_cast< float >(color.r);
    const float g = static_cast< float >(color.g);
    const float b = static_cast< float >(color.b);
    const float a = static_cast< float >(color.a);

    // vertex order and texture coordinates match the per label path:
    // BL -> (u0, v1), TL -> (u0, v0), BR -> (u1, v1), TR -> (u1, v0)
    auto vertex = [ & ](const Triple& p, float u, float v) {
        m_vertices << static_cast< float >(p.x) << static_cast< float >(p.y) << static_cast< float >(p.z) << u << v << r
                   << g << b << a;
    };

    vertex(bl, u0, v1);
    vertex(tl, u0, v0);
    vertex(br, u1, v1);

    vertex(tl, u0, v0);
    vertex(tr, u1, v0);
    vertex(br, u1, v1);
}

bool Qwt3DTextBatch::isEmpty() const
{
    return m_vertices.isEmpty();
}

/*!
  Draws all submitted quads with one draw call and clears the batch.
 */
void Qwt3DTextBatch::flush(QOpenGLShaderProgram* shader, const QMatrix4x4& modelView, const QMatrix4x4& projection)
{
    if (m_vertices.isEmpty()) {
        if (m_growPending)
            grow();

        return;
    }

    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (context == nullptr || shader == nullptr) {
        m_vertices.clear();
        return;
    }

    if (m_context != context) {
        releaseGL();
        m_context = context;
    }

    if (!ensureTexture()) {
        m_vertices.clear();
        return;
    }

    if (m_vbo == nullptr) {
        m_vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        m_vbo->create();
    }

    auto* f = context->functions();

    m_vbo->bind();
    m_vbo->allocate(m_vertices.constData(), static_cast< int >(m_vertices.size() * sizeof(float)));

    shader->bind();
    shader->setUniformValue("uModelView", modelView);
    shader->setUniformValue("uProjection", projection);
    shader->setUniformValue("uTextTexture", 0);

    m_texture->bind(0);

    const int stride = VERTEX_FLOATS * sizeof(float);
    shader->enableAttributeArray(0);
    shader->setAttributeBuffer(0, GL_FLOAT, 0, 3, stride);
    shader->enableAttributeArray(1);
    shader->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 2, stride);
    shader->enableAttributeArray(2);
    shader->setAttributeBuffer(2, GL_FLOAT, 5 * sizeof(float), 4, stride);

    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    f->glDisable(GL_DEPTH_TEST);
    f->glDrawArrays(GL_TRIANGLES, 0, static_cast< int >(m_vertices.size() / VERTEX_FLOATS));
    f->glEnable(GL_DEPTH_TEST);

    shader->disableAttributeArray(0);
    shader->disableAttributeArray(1);
    shader->disableAttributeArray(2);
    shader->release();
    m_texture->release();
    m_vbo->release();

    m_vertices.clear();

    // the atlas was full: grow it for the next frame, the labels that did not
    // fit were drawn directly and are packed again then
    if (m_growPending)
        grow();
}

////////////////////////////////////////////////////////////////////////////////
// atlas internals
////////////////////////////////////////////////////////////////////////////////

bool Qwt3DTextBatch::pack(const QSize& size, QRect& rect)
{
    for (int i = 0; i < m_shelves.size(); ++i) {
        Shelf& shelf = m_shelves[ i ];
        if (shelf.height >= size.height() && shelf.used + size.width() <= m_size.width()) {
            rect = QRect(shelf.used, shelf.y, size.width(), size.height());
            shelf.used += size.width();
            return true;
        }
    }

    if (m_usedHeight + size.height() > m_size.height())
        return false;

    Shelf shelf;
    shelf.y      = m_usedHeight;
    shelf.height = size.height();
    shelf.used   = size.width();
    m_shelves.append(shelf);
    m_usedHeight += size.height();

    rect = QRect(0, shelf.y, size.width(), size.height());
    return true;
}

void Qwt3DTextBatch::grow()
{
    m_growPending = false;

    if (m_size.height() >= ATLAS_MAX_HEIGHT)
        return;

    m_size.setHeight(qMin(ATLAS_MAX_HEIGHT, m_size.height() * 2));

    // cells are re-packed from the label images of the next frame
    m_entries.clear();
    m_shelves.clear();
    m_usedHeight = 0;

    m_image = QImage(m_size, QImage::Format_ARGB32_Premultiplied);
    m_image.fill(Qt::transparent);
    m_imageDirty = true;
}

void Qwt3DTextBatch::composite(const QRect& rect, const QImage& image)
{
    if (m_image.size() != m_size) {
        m_image = QImage(m_size, QImage::Format_ARGB32_Premultiplied);
        m_image.fill(Qt::transparent);
    }

    QPainter painter(&m_image);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawImage(rect.topLeft(), image);
    painter.end();

    m_imageDirty = true;
}

bool Qwt3DTextBatch::ensureTexture()
{
    if (m_image.size() != m_size) {
        m_image = QImage(m_size, QImage::Format_ARGB32_Premultiplied);
        m_image.fill(Qt::transparent);
        m_imageDirty = true;
    }

    const bool sizeChanged = m_texture != nullptr
                             && (m_texture->width() != m_size.width() || m_texture->height() != m_size.height());

    if (m_texture == nullptr || sizeChanged || m_imageDirty) {
        delete m_texture;
        m_texture = new QOpenGLTexture(m_image);
        m_texture->setMinificationFilter(QOpenGLTexture::Linear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
        m_imageDirty = false;
    }

    return m_texture != nullptr && m_texture->isCreated();
}

void Qwt3DTextBatch::releaseGL()
{
    releaseGLObject(m_texture, m_context);
    releaseGLObject(m_vbo, m_context);
    m_context = nullptr;
}
