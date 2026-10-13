#ifndef QWT3D_IO_GL2PS_H
#define QWT3D_IO_GL2PS_H

#include <ctime>

#include "qwt3d_types.h"
#include "qwt3d_io.h"

/**
 * @brief Provides EPS, PS, PDF, SVG, PGF and TeX output
 * @details Qwt3DVectorWriter provides vector graphics output through the gl2ps library,
 *          supporting EPS, PS, PDF, SVG, PGF and TeX formats.
 *
 * @note gl2ps relies on the Compatibility Profile GL state. In the modernized
 *       Core Profile renderer, vector export may not reflect the correct
 *       transformation. This is a known limitation; full modernization requires
 *       generating vector output from VBO vertex data directly.
 */
class QWT3D_EXPORT Qwt3DVectorWriter : public Qwt3DIO::Functor
{
    friend class Qwt3DIO;
    QWT_DECLARE_PRIVATE(Qwt3DVectorWriter)

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

    Qwt3DVectorWriter();
    ~Qwt3DVectorWriter() override;

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
    Qwt3DIO::Functor* clone() const override;
    bool operator()(Qwt3DPlot* plot, QString const& fname) override;
};

/**
 * @brief Emits interleaved vertices as an immediate mode primitive for the vector export
 * @param[in] mode Primitive type (GL_TRIANGLES, GL_LINES, GL_POINTS, ...)
 * @param[in] vertices Interleaved vertex data: x y z r g b a (7 floats per vertex)
 * @param[in] vertexCount Number of vertices in @p vertices
 * @param[in] indices Optional index list; when null the vertices are used in order
 * @param[in] indexCount Number of indices to emit
 * @param[in] overrideColor Optional RGBA overriding the per-vertex colors
 * @param[in] modelView Column major modelview matrix (as uploaded to the shaders)
 * @param[in] projection Column major projection matrix
 *
 * @details gl2ps captures primitives from the GL_FEEDBACK buffer, where the
 *          color of every vertex is the fixed-function current color - the
 *          GLSL render path of this module never writes it, so a feedback
 *          replay of the shader based scene produces fully transparent
 *          primitives (the surfaces were invisible in PDF/SVG exports). This
 *          function feeds the captured geometry explicitly: the vertices are
 *          transformed on the CPU by the same matrices the shaders use and
 *          submitted with glColor4f()/glVertex4f() (the fixed-function
 *          matrices stay at their defaults, so the vertex positions are the
 *          clip coordinates).
 *
 *          Only meaningful while the GL render mode is GL_FEEDBACK.
 */
void qwt3dEmitVerticesForVectorExport(unsigned int mode,
                                      const float* vertices,
                                      int vertexCount,
                                      const unsigned int* indices,
                                      int indexCount,
                                      const float* overrideColor,
                                      const float* modelView,
                                      const float* projection);

int setDeviceLineWidth(float val);
int setDevicePointSize(float val);
int drawDevicePixels(int width, int height, unsigned int format, unsigned int type, const void* pixels);
int drawDeviceText(const char* str,
                   const char* fontname,
                   int fontsize,
                   Triple pos,
                   RGBA rgba,
                   ANCHOR align,
                   double gap);
void setDevicePolygonOffset(float factor, float units);

#endif  // QWT3D_IO_GL2PS_H
