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
