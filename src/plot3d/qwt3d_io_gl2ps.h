#ifndef QWT3D_IO_GL2PS_H
#define QWT3D_IO_GL2PS_H

#include <ctime>

#include "qwt3d_types.h"
#include "qwt3d_io.h"



/**
 * @brief Provides EPS, PS, PDF, SVG, PGF and TeX output
 * @details VectorWriter provides vector graphics output through the gl2ps library,
 *          supporting EPS, PS, PDF, SVG, PGF and TeX formats.
 */
class QWT3D_EXPORT VectorWriter : public IO::Functor
{
    friend class IO;
    QWT_DECLARE_PRIVATE(VectorWriter)

public:
    // The possible output formats for the text parts of the scene
    enum TEXTMODE
    {
        PIXEL,
        NATIVE,
        TEX
    };

    // The possible behaviour for landscape settings
    enum LANDSCAPEMODE
    {
        ON,
        OFF,
        AUTO
    };

    // The possible sorting types which are translated in gl2ps types
    enum SORTMODE
    {
        NOSORT,
        SIMPLESORT,
        BSPSORT
    };

    VectorWriter();
    ~VectorWriter() override;

    // Sets landscape mode
    void setLandscape(LANDSCAPEMODE val);
    // Returns the current landscape mode
    LANDSCAPEMODE landscape() const;

    void setTextMode(TEXTMODE val, QString fname = "");
    // Return current text output mode
    TEXTMODE textMode() const;

    // Sets one of the SORTMODE sorting modes
    void setSortMode(SORTMODE val);
    // Returns gl2ps sorting type
    SORTMODE sortMode() const;
    // Turns compressed output on or off (no effect if zlib support is not available)
    void setCompressed(bool val);
    // Returns compression mode (always false if zlib support has not been set)
    bool compressed() const;

    bool setFormat(QString const& format);

private:
    IO::Functor* clone() const override;
    bool operator()(Qwt3DPlot* plot, QString const& fname) override;
};

GLint setDeviceLineWidth(GLfloat val);
GLint setDevicePointSize(GLfloat val);
GLint drawDevicePixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
GLint drawDeviceText(const char* str,
                     const char* fontname,
                     int fontsize,
                     Triple pos,
                     RGBA rgba,
                     ANCHOR align,
                     double gap);
void setDevicePolygonOffset(GLfloat factor, GLfloat units);


#endif  // QWT3D_IO_GL2PS_H