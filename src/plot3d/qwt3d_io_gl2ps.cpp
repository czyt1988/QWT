#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4786)
#endif

#include <ctime>
#include "gl2ps.h"
#include "qwt3d_io_gl2ps.h"
#include "qwt3d_plot.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include "qwt_version_info.h"

// GL types are provided by gl2ps.h which includes <GL/gl.h>


class Qwt3DVectorWriter::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DVectorWriter)

public:
    PrivateData(Qwt3DVectorWriter* q)
        : q_ptr(q)
        , m_gl2psFormat(GL2PS_EPS)
        , m_formatError(false)
#ifdef GL2PS_HAVE_ZLIB
        , m_compressed(true)
#else
        , m_compressed(false)
#endif
        , m_sortMode(Qwt3DVectorWriter::SIMPLESORT)
        , m_landscape(Qwt3DVectorWriter::AUTO)
        , m_textMode(Qwt3DVectorWriter::PIXEL)
    {
    }

    GLint m_gl2psFormat;
    bool m_formatError;
    bool m_compressed;
    Qwt3DVectorWriter::SORTMODE m_sortMode;
    Qwt3DVectorWriter::LANDSCAPEMODE m_landscape;
    Qwt3DVectorWriter::TEXTMODE m_textMode;
    QString m_texFname;
};

Qwt3DVectorWriter::Qwt3DVectorWriter() : QWT_PIMPL_CONSTRUCT
{
}

Qwt3DVectorWriter::~Qwt3DVectorWriter() = default;

Qwt3DIO::Functor* Qwt3DVectorWriter::clone() const
{
    auto* copy = new Qwt3DVectorWriter();
    QWT_DC(d);
    auto* copyD          = copy->d_func();
    copyD->m_gl2psFormat = d->m_gl2psFormat;
    copyD->m_formatError = d->m_formatError;
    copyD->m_compressed  = d->m_compressed;
    copyD->m_sortMode    = d->m_sortMode;
    copyD->m_landscape   = d->m_landscape;
    copyD->m_textMode    = d->m_textMode;
    copyD->m_texFname    = d->m_texFname;
    return copy;
}

void Qwt3DVectorWriter::setLandscape(LANDSCAPEMODE val)
{
    QWT_D(d);
    d->m_landscape = val;
}

Qwt3DVectorWriter::LANDSCAPEMODE Qwt3DVectorWriter::landscape() const
{
    QWT_DC(d);
    return d->m_landscape;
}

void Qwt3DVectorWriter::setSortMode(SORTMODE val)
{
    QWT_D(d);
    d->m_sortMode = val;
}

Qwt3DVectorWriter::SORTMODE Qwt3DVectorWriter::sortMode() const
{
    QWT_DC(d);
    return d->m_sortMode;
}

void Qwt3DVectorWriter::setTextMode(TEXTMODE val, QString fname)
{
    QWT_D(d);
    d->m_textMode = val;
    d->m_texFname = (fname.isEmpty()) ? QString("") : fname;
}

Qwt3DVectorWriter::TEXTMODE Qwt3DVectorWriter::textMode() const
{
    QWT_DC(d);
    return d->m_textMode;
}

#ifdef GL2PS_HAVE_ZLIB
void Qwt3DVectorWriter::setCompressed(bool val)
{
    QWT_D(d);
    d->m_compressed = val;
}
#else
void Qwt3DVectorWriter::setCompressed(bool)
{
    QWT_D(d);
    d->m_compressed = false;
}
#endif

bool Qwt3DVectorWriter::compressed() const
{
    QWT_DC(d);
    return d->m_compressed;
}

bool Qwt3DVectorWriter::setFormat(QString const& format)
{
    QWT_D(d);
    if (format == QString("EPS")) {
        d->m_gl2psFormat = GL2PS_EPS;
    } else if (format == QString("PS")) {
        d->m_gl2psFormat = GL2PS_PS;
    } else if (format == QString("PDF")) {
        d->m_gl2psFormat = GL2PS_PDF;
    } else if (format == QString("SVG")) {
        d->m_gl2psFormat = GL2PS_SVG;
    } else if (format == QString("PGF")) {
        d->m_gl2psFormat = GL2PS_PGF;
    }
#ifdef GL2PS_HAVE_ZLIB
    else if (format == QString("EPS_GZ")) {
        d->m_gl2psFormat = GL2PS_EPS;
    } else if (format == QString("PS_GZ")) {
        d->m_gl2psFormat = GL2PS_PS;
    }
#endif
    else {
        d->m_formatError = true;
        return false;
    }
    d->m_formatError = false;
    return true;
}

/**
 * @brief Performs actual vector output via gl2ps
 * @details gl2ps relies on the Compatibility Profile GL state.
 *          In the modernized renderer, the GL matrix stack is not used,
 *          so gl2ps output may not reflect the correct transformation.
 *          This is a known limitation; full gl2ps modernization requires
 *          generating vector output from VBO vertex data directly.
 */
bool Qwt3DVectorWriter::operator()(Qwt3DPlot* plot, QString const& fname)
{
    QWT_D(d);
    if (d->m_formatError)
        return false;

    plot->makeCurrent();

    GLint bufsize = 0, state = GL2PS_OVERFLOW;
    GLint viewport[ 4 ];

    glGetIntegerv(GL_VIEWPORT, viewport);

    GLint options = GL2PS_SIMPLE_LINE_OFFSET | GL2PS_SILENT | GL2PS_DRAW_BACKGROUND | GL2PS_OCCLUSION_CULL | GL2PS_BEST_ROOT;

    if (d->m_compressed)
        options |= GL2PS_COMPRESS;

    switch (d->m_landscape) {
    case Qwt3DVectorWriter::AUTO:
        if (viewport[ 2 ] - viewport[ 0 ] > viewport[ 3 ] - viewport[ 0 ])
            options |= GL2PS_LANDSCAPE;
        break;
    case Qwt3DVectorWriter::ON:
        options |= GL2PS_LANDSCAPE;
        break;
    default:
        break;
    }

    int sortmode = GL2PS_SIMPLE_SORT;
    switch (d->m_sortMode) {
    case Qwt3DVectorWriter::NOSORT:
        sortmode = GL2PS_NO_SORT;
        break;
    case Qwt3DVectorWriter::SIMPLESORT:
        sortmode = GL2PS_SIMPLE_SORT;
        break;
    case Qwt3DVectorWriter::BSPSORT:
        sortmode = GL2PS_BSP_SORT;
        break;
    default:
        break;
    }

    switch (d->m_textMode) {
    case NATIVE:
        Qwt3DLabel::useDeviceFonts(true);
        break;
    case PIXEL:
        Qwt3DLabel::useDeviceFonts(false);
        break;
    case TEX:
        options |= GL2PS_NO_PIXMAP | GL2PS_NO_TEXT;
        break;
    default:
        break;
    }

    QString version = QString::number(QWT_VERSION_MAJ) + "." + QString::number(QWT_VERSION_MIN) + "."
                      + QString::number(QWT_VERSION_PAT);

    QString producer = QString("QwtPlot3D ") + version + " (beta)";

    FILE* fp = fopen(QWT3DLOCAL8BIT(fname), "wb");
    if (!fp) {
        Qwt3DLabel::useDeviceFonts(false);
        return false;
    }
    while (state == GL2PS_OVERFLOW) {
        bufsize += 2 * 1024 * 1024;
        gl2psBeginPage("---",
                       QWT3DLOCAL8BIT(producer),
                       viewport,
                       d->m_gl2psFormat,
                       sortmode,
                       options,
                       GL_RGBA,
                       0,
                       nullptr,
                       0,
                       0,
                       0,
                       bufsize,
                       fp,
                       QWT3DLOCAL8BIT(fname));

        plot->grabFramebuffer();
        state = gl2psEndPage();
    }
    fclose(fp);

    if (d->m_textMode == TEX) {
        QString fn = (d->m_texFname.isEmpty()) ? fname + ".tex" : d->m_texFname;

        fp = fopen(QWT3DLOCAL8BIT(fn), "wb");
        if (!fp) {
            Qwt3DLabel::useDeviceFonts(false);
            return false;
        }
        Qwt3DLabel::useDeviceFonts(true);
        options &= ~GL2PS_NO_PIXMAP & ~GL2PS_NO_TEXT;
        state = GL2PS_OVERFLOW;
        while (state == GL2PS_OVERFLOW) {
            bufsize += 2 * 1024 * 1024;
            gl2psBeginPage("---",
                           QWT3DLOCAL8BIT(producer),
                           viewport,
                           GL2PS_TEX,
                           sortmode,
                           options,
                           GL_RGBA,
                           0,
                           nullptr,
                           0,
                           0,
                           0,
                           bufsize,
                           fp,
                           QWT3DLOCAL8BIT(fn));

            plot->update();
            state = gl2psEndPage();
        }
        fclose(fp);
    }

    Qwt3DLabel::useDeviceFonts(false);

    return true;
}

/**
 * @brief Emits interleaved vertices as an immediate mode primitive for the vector export
 * @details See the declaration in the header for the motivation. The vertices
 *          are transformed by the same matrices the shaders use, but on the
 *          CPU, and submitted through the fixed-function pipeline - whose
 *          matrices stay at their defaults, so glVertex4f() receives clip
 *          coordinates and the feedback buffer records exactly the window
 *          coordinates the shader based pass would have produced.
 *
 *          GL_CULL_FACE and GL_LIGHTING are disabled for the capture (the
 *          feedback replay must see every triangle, and the per vertex color
 *          is the final one) and restored afterwards.
 */
void qwt3dEmitVerticesForVectorExport(unsigned int mode,
                                      const float* vertices,
                                      int vertexCount,
                                      const unsigned int* indices,
                                      int indexCount,
                                      const float* overrideColor,
                                      const float* modelView,
                                      const float* projection)
{
    if (!vertices || vertexCount <= 0)
        return;

    // projection * modelView in column major layout
    float mvp[ 16 ];
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k)
                sum += projection[ k * 4 + row ] * modelView[ col * 4 + k ];
            mvp[ col * 4 + row ] = sum;
        }
    }

    // the fixed-function pipeline must be active (the shader path never
    // publishes its colors to the feedback buffer). <GL/gl.h> as pulled in by
    // gl2ps.h only declares GL 1.1, so the wrapper is used for the two
    // non-1.1 entry points.
    if (QOpenGLContext* context = QOpenGLContext::currentContext()) {
        QOpenGLFunctions* f = context->functions();
        f->glUseProgram(0);
        for (unsigned int i = 0; i < 3; ++i)
            f->glDisableVertexAttribArray(i);
    }

    const GLboolean cullFace = glIsEnabled(GL_CULL_FACE);
    const GLboolean lighting = glIsEnabled(GL_LIGHTING);
    if (cullFace)
        glDisable(GL_CULL_FACE);
    if (lighting)
        glDisable(GL_LIGHTING);

    const int count = (indices && indexCount > 0) ? indexCount : vertexCount;

    glBegin(mode);
    for (int i = 0; i < count; ++i) {
        int vi = indices ? static_cast< int >(indices[ i ]) : i;
        if (vi < 0 || vi >= vertexCount)
            continue;

        const float* v = vertices + 7 * vi;
        const float x = v[ 0 ], y = v[ 1 ], z = v[ 2 ];

        const float cx = mvp[ 0 ] * x + mvp[ 4 ] * y + mvp[ 8 ] * z + mvp[ 12 ];
        const float cy = mvp[ 1 ] * x + mvp[ 5 ] * y + mvp[ 9 ] * z + mvp[ 13 ];
        const float cz = mvp[ 2 ] * x + mvp[ 6 ] * y + mvp[ 10 ] * z + mvp[ 14 ];
        const float cw = mvp[ 3 ] * x + mvp[ 7 ] * y + mvp[ 11 ] * z + mvp[ 15 ];

        if (overrideColor)
            glColor4f(overrideColor[ 0 ], overrideColor[ 1 ], overrideColor[ 2 ], overrideColor[ 3 ]);
        else
            glColor4f(v[ 3 ], v[ 4 ], v[ 5 ], v[ 6 ]);

        glVertex4f(cx, cy, cz, cw);
    }
    glEnd();

    if (cullFace)
        glEnable(GL_CULL_FACE);
    if (lighting)
        glEnable(GL_LIGHTING);
}

// Device helper functions for gl2ps vector export.
// These functions bridge between the modern shader-based renderer and
// the legacy gl2ps library which requires Compatibility Profile GL calls.

int setDeviceLineWidth(float val)
{
    if (val < 0)
        val = 0;

    GLint ret = gl2psLineWidth(val);

    // TODO: glLineWidth is not guaranteed > 1.0 in Core Profile (Plan B)
    glLineWidth(val);
    return ret;
}

int setDevicePointSize(float val)
{
    if (val < 0)
        val = 0;

    GLint ret = gl2psPointSize(val);

    glPointSize(val);
    return ret;
}

int drawDevicePixels(int width, int height, unsigned int format, unsigned int type, const void* pixels)
{
    // Legacy glDrawPixels for gl2ps capture
    glDrawPixels(width, height, format, type, pixels);

    if (format != GL_RGBA || type != GL_UNSIGNED_BYTE)
        return GL2PS_ERROR;

    GLfloat* convertedpixel = static_cast< GLfloat* >(malloc(3 * width * height * sizeof(GLfloat)));
    if (!convertedpixel)
        return GL2PS_ERROR;

    const GLubyte* px = reinterpret_cast< const GLubyte* >(pixels);
    for (int i = 0; i != 3 * width * height; i += 3) {
        int pxi                 = (4 * i) / 3;
        convertedpixel[ i ]     = px[ pxi ] / float(255);
        convertedpixel[ i + 1 ] = px[ pxi + 1 ] / float(255);
        convertedpixel[ i + 2 ] = px[ pxi + 2 ] / float(255);
    }
    GLint ret = gl2psDrawPixels(width, height, 0, 0, GL_RGB, GL_FLOAT, convertedpixel);
    free(convertedpixel);
    return ret;
}

int drawDeviceText(const char* str, const char* fontname, int fontsize, Triple pos, RGBA /*rgba*/, ANCHOR align, double gap)
{
    // Use the world position directly for gl2ps text output.
    // The pixel-space gap/anchor adjustment is simplified since the
    // GL matrix stack is no longer used for view transformation.
    GLdouble fcol[ 4 ];
    glGetDoublev(GL_CURRENT_COLOR, fcol);

    GLint ret = GL2PS_SUCCESS;

    GLint a = GL2PS_TEXT_BL;
    switch (align) {
    case Center:
        a = GL2PS_TEXT_C;
        break;
    case CenterLeft:
        a = GL2PS_TEXT_CL;
        break;
    case CenterRight:
        a = GL2PS_TEXT_CR;
        break;
    case BottomCenter:
        a = GL2PS_TEXT_B;
        break;
    case BottomLeft:
        a = GL2PS_TEXT_BL;
        break;
    case BottomRight:
        a = GL2PS_TEXT_BR;
        break;
    case TopCenter:
        a = GL2PS_TEXT_T;
        break;
    case TopLeft:
        a = GL2PS_TEXT_TL;
        break;
    case TopRight:
        a = GL2PS_TEXT_TR;
        break;
    default:
        break;
    }

    (void)gap;

    glRasterPos3d(pos.x, pos.y, pos.z);
    ret = gl2psTextOpt(str, fontname, static_cast< int >(fontsize), a, 0);
    glColor4dv(fcol);
    return ret;
}

void setDevicePolygonOffset(float factor, float units)
{
    glPolygonOffset(factor, units);
    gl2psEnable(GL2PS_POLYGON_OFFSET_FILL);
}
