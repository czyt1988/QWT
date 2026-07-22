#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4786)
#endif

#include <ctime>
#include "qwt3d_openglhelper.h"
#include "gl2ps.h"
#include "qwt3d_io_gl2ps.h"
#include "qwt3d_plot.h"
#include "qwt_version_info.h"


class VectorWriter::PrivateData
{
    QWT_DECLARE_PUBLIC(VectorWriter)

public:
    PrivateData(VectorWriter* q)
        : q_ptr(q)
        , m_gl2psFormat(GL2PS_EPS)
        , m_formatError(false)
#ifdef GL2PS_HAVE_ZLIB
        , m_compressed(true)
#else
        , m_compressed(false)
#endif
        , m_sortMode(VectorWriter::SIMPLESORT)
        , m_landscape(VectorWriter::AUTO)
        , m_textMode(VectorWriter::PIXEL)
    {
    }

    GLint m_gl2psFormat;
    bool m_formatError;
    bool m_compressed;
    VectorWriter::SORTMODE m_sortMode;
    VectorWriter::LANDSCAPEMODE m_landscape;
    VectorWriter::TEXTMODE m_textMode;
    QString m_texFname;
};

VectorWriter::VectorWriter() : QWT_PIMPL_CONSTRUCT
{
}

VectorWriter::~VectorWriter() = default;

/**
 * @brief Provides a new VectorWriter object
 * @return A cloned copy of this VectorWriter as Functor pointer
 */
IO::Functor* VectorWriter::clone() const
{
    auto* copy = new VectorWriter();
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

/**
 * @brief Sets landscape mode
 * @param val Landscape mode (ON, OFF, or AUTO)
 */
void VectorWriter::setLandscape(LANDSCAPEMODE val)
{
    QWT_D(d);
    d->m_landscape = val;
}

/**
 * @brief Returns the current landscape mode
 */
VectorWriter::LANDSCAPEMODE VectorWriter::landscape() const
{
    QWT_DC(d);
    return d->m_landscape;
}

/**
 * @brief Sets the sorting mode
 * @param val Sort mode (NOSORT, SIMPLESORT, or BSPSORT)
 */
void VectorWriter::setSortMode(SORTMODE val)
{
    QWT_D(d);
    d->m_sortMode = val;
}

/**
 * @brief Returns the current sorting mode
 */
VectorWriter::SORTMODE VectorWriter::sortMode() const
{
    QWT_DC(d);
    return d->m_sortMode;
}

/**
 * @brief Sets the mode for text output
 * @param val The underlying format for the generated output:
 *            PIXEL - poor quality but exact positioning;
 *            NATIVE - high quality but inexact positioning;
 *            TEX - high quality and exact positioning, arbitrary TeX strings
 *            as content for the saved labels are possible. The disadvantage is
 *            the need for an additionally TeX run to get the final output.
 * @param fname Optional, used only in conjunction with TeX output; file name
 *              for the generated TeX file. If not set, a file called
 *              "OUTPUT.FOR.tex" will be generated, where "OUTPUT.FOR" describes
 *              the file name argument for IO::save().
 * @note On Linux platforms, pdflatex seems a file named 'dump_0.pdf.tex' mistakenly
 *       to identify as PDF file.
 */
void VectorWriter::setTextMode(TEXTMODE val, QString fname)
{
    QWT_D(d);
    d->m_textMode = val;
    d->m_texFname = (fname.isEmpty()) ? QString("") : fname;
}

/**
 * @brief Returns the current text output mode
 */
VectorWriter::TEXTMODE VectorWriter::textMode() const
{
    QWT_DC(d);
    return d->m_textMode;
}

#ifdef GL2PS_HAVE_ZLIB
/**
 * @brief Turns compressed output on or off
 * @param val True to enable compression, false to disable
 * @details No effect if zlib support has not been set.
 */
void VectorWriter::setCompressed(bool val)
{
    QWT_D(d);
    d->m_compressed = val;
}
#else
/**
 * @brief Turns compressed output on or off (no effect - zlib support not available)
 */
void VectorWriter::setCompressed(bool)
{
    QWT_D(d);
    d->m_compressed = false;
}
#endif

/**
 * @brief Returns compression mode
 */
bool VectorWriter::compressed() const
{
    QWT_DC(d);
    return d->m_compressed;
}

/**
 * @brief Sets output format
 * @param format Must be one of "EPS_GZ", "PS_GZ", "EPS", "PS", "PDF", "SVG" or "PGF" (case sensitive)
 * @return True on success, false for unknown format
 */
bool VectorWriter::setFormat(QString const& format)
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
 * @brief Performs actual output
 * @param plot Plot3D widget to export
 * @param fname Output file name
 * @return True on success, false on format error or file open failure
 */
bool VectorWriter::operator()(Plot3D* plot, QString const& fname)
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
    case VectorWriter::AUTO:
        if (viewport[ 2 ] - viewport[ 0 ] > viewport[ 3 ] - viewport[ 0 ])
            options |= GL2PS_LANDSCAPE;
        break;
    case VectorWriter::ON:
        options |= GL2PS_LANDSCAPE;
        break;
    default:
        break;
    }

    int sortmode = GL2PS_SIMPLE_SORT;
    switch (d->m_sortMode) {
    case VectorWriter::NOSORT:
        sortmode = GL2PS_NO_SORT;
        break;
    case VectorWriter::SIMPLESORT:
        sortmode = GL2PS_SIMPLE_SORT;
        break;
    case VectorWriter::BSPSORT:
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

    // extra TeX file
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

            plot->updateData();
            plot->update();
            state = gl2psEndPage();
        }
        fclose(fp);
    }

    Qwt3DLabel::useDeviceFonts(false);

    return true;
}

// moved

GLint setDeviceLineWidth(GLfloat val)
{
    if (val < 0)
        val = 0;

    GLint ret = gl2psLineWidth(val);

    GLfloat lw[ 2 ];
    glGetFloatv(GL_LINE_WIDTH_RANGE, lw);

    if (val < lw[ 0 ])
        val = lw[ 0 ];
    else if (val > lw[ 1 ])
        val = lw[ 1 ];

    glLineWidth(val);
    return ret;
}

GLint setDevicePointSize(GLfloat val)
{
    if (val < 0)
        val = 0;

    GLint ret = gl2psPointSize(val);

    GLfloat lw[ 2 ];
    glGetFloatv(GL_POINT_SIZE_RANGE, lw);

    if (val < lw[ 0 ])
        val = lw[ 0 ];
    else if (val > lw[ 1 ])
        val = lw[ 1 ];

    glPointSize(val);
    return ret;
}

GLint drawDevicePixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels)
{
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

GLint drawDeviceText(const char* str, const char* fontname, int fontsize, Triple pos, RGBA /*rgba*/, ANCHOR align, double gap)
{
    double vp[ 3 ];

    World2ViewPort(vp[ 0 ], vp[ 1 ], vp[ 2 ], pos.x, pos.y, pos.z);
    Triple start(vp[ 0 ], vp[ 1 ], vp[ 2 ]);

    GLdouble fcol[ 4 ];
    glGetDoublev(GL_CURRENT_COLOR, fcol);
    GLdouble bcol[ 4 ];
    glGetDoublev(GL_COLOR_CLEAR_VALUE, bcol);

    GLint ret = GL2PS_SUCCESS;

    GLint a = GL2PS_TEXT_BL;
    switch (align) {
    case Center:
        a = GL2PS_TEXT_C;
        break;
    case CenterLeft:
        a = GL2PS_TEXT_CL;
        start += Triple(gap, 0, 0);
        break;
    case CenterRight:
        a = GL2PS_TEXT_CR;
        start += Triple(-gap, 0, 0);
        break;
    case BottomCenter:
        a = GL2PS_TEXT_B;
        start += Triple(0, gap, 0);
        break;
    case BottomLeft:
        a = GL2PS_TEXT_BL;
        start += Triple(gap, gap, 0);
        break;
    case BottomRight:
        a = GL2PS_TEXT_BR;
        start += Triple(-gap, gap, 0);
        break;
    case TopCenter:
        a = GL2PS_TEXT_T;
        start += Triple(0, -gap, 0);
        break;
    case TopLeft:
        a = GL2PS_TEXT_TL;
        start += Triple(gap, -gap, 0);
        break;
    case TopRight:
        a = GL2PS_TEXT_TR;
        start += Triple(-gap, -gap, 0);
        break;
    default:
        break;
    }

    ViewPort2World(vp[ 0 ], vp[ 1 ], vp[ 2 ], start.x, start.y, start.z);
    Triple adjpos(vp[ 0 ], vp[ 1 ], vp[ 2 ]);

    glRasterPos3d(adjpos.x, adjpos.y, adjpos.z);
    ret = gl2psTextOpt(str, fontname, static_cast< int >(fontsize), a, 0);
    glColor4dv(fcol);
    glClearColor(bcol[ 0 ], bcol[ 1 ], bcol[ 2 ], bcol[ 3 ]);
    return ret;
}

void setDevicePolygonOffset(GLfloat factor, GLfloat units)
{
    glPolygonOffset(factor, units);
    gl2psEnable(GL2PS_POLYGON_OFFSET_FILL);
}
