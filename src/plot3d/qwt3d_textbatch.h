#ifndef QWT3D_TEXTBATCH_H
#define QWT3D_TEXTBATCH_H

#include "qwt3d_global.h"
#include "qwt3d_types.h"

#include <QHash>
#include <QImage>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QVector>

class QMatrix4x4;
class QOpenGLBuffer;
class QOpenGLContext;
class QOpenGLShaderProgram;
class QOpenGLTexture;

/**
 * @brief Collects the screen aligned label quads of one frame into a single draw call
 * @details Every 3D label is a screen aligned textured quad. Drawing them one by
 *          one costs a vertex buffer upload, a texture bind and a draw call per
 *          label, which dominates the frame time of text heavy plots.
 *
 *          The batch packs all label images into one atlas texture and collects
 *          the quads while the drawables are traversed, so Qwt3DPlot can flush
 *          them with a single draw call per draw phase.
 *
 *          Labels own their image and rasterize it themselves; the batch only
 *          takes a copy of the pixels into the atlas. Images are cached by key
 *          (the label) together with a stamp: the expensive repack/composite
 *          only happens when the label reports a new stamp, i.e. when its text
 *          was rasterized again.
 *
 * @code
 * // in Qwt3DPlot::paintGL()
 * ctx.textBatch = &d->m_textBatch;
 * d->m_coordinates.draw(ctx);
 * d->m_textBatch.flush(d->m_textShader.get(), ctx.modelView, ctx.projection);
 * @endcode
 */
class QWT3D_EXPORT Qwt3DTextBatch
{
public:
    Qwt3DTextBatch();
    ~Qwt3DTextBatch();

    Qwt3DTextBatch(const Qwt3DTextBatch&)            = delete;
    Qwt3DTextBatch& operator=(const Qwt3DTextBatch&) = delete;

    // Drops all GL resources; call when the GL context was (re)created
    void invalidate();

    // Copies the label image into the atlas and returns its normalized texture rect
    bool addLabel(const void* key, const QImage& image, quint64 stamp, QRectF& uv);

    // Appends one screen aligned quad (triangle list) to the batch
    void addQuad(const Triple& bl, const Triple& tl, const Triple& br, const Triple& tr, const QRectF& uv, const RGBA& color);

    // True while no quad has been submitted since the last flush
    bool isEmpty() const;

    // Draws all submitted quads in one call and clears the batch
    void flush(QOpenGLShaderProgram* shader, const QMatrix4x4& modelView, const QMatrix4x4& projection);

private:
    struct Entry
    {
        QRect cell;  //!< allocated area in the atlas (pixels)
        QSize size;  //!< area the label image actually uses
        quint64 stamp = 0;
    };

    struct Shelf
    {
        int y      = 0;
        int height = 0;
        int used   = 0;
    };

    bool pack(const QSize& size, QRect& rect);
    void grow();
    void composite(const QRect& rect, const QImage& image);
    bool ensureTexture();
    void releaseGL();

    QImage m_image;
    QSize m_size = QSize(1024, 256);
    QHash< const void*, Entry > m_entries;
    QVector< Shelf > m_shelves;
    int m_usedHeight = 0;

    QVector< float > m_vertices;
    bool m_imageDirty  = false;
    bool m_growPending = false;

    QOpenGLTexture* m_texture = nullptr;
    QOpenGLBuffer* m_vbo      = nullptr;
    QOpenGLContext* m_context = nullptr;
};

#endif  // QWT3D_TEXTBATCH_H
