#ifndef QWT3D_COLOR_H
#define QWT3D_COLOR_H

#include <qstring.h>
#include "qwt3d_global.h"
#include "qwt3d_types.h"



class Qwt3DSurface;

/**
 * @brief Abstract base class for color functors
 * @details Use your own color model by providing an implementation of
 *          operator()(double x, double y, double z). Colors destructor has been
 *          declared protected, in order to use only heap based objects. Plot3D
 *          will handle the objects destruction. See Qwt3DStandardColor for an example.
 */
class QWT3D_EXPORT Qwt3DColor
{
public:
    // Implement your color model here
    virtual RGBA operator()(double x, double y, double z) const = 0;
    virtual RGBA operator()(Triple const& t) const
    {
        return this->operator()(t.x, t.y, t.z);
    }
    // Should create a color vector usable by Qwt3DColorLegend. The default implementation returns its argument
    virtual ColorVector& createVector(ColorVector& vec)
    {
        return vec;
    }

    void destroy() const
    {
        delete this;
    }

    /// Sets the owning surface for automatic VBO invalidation (called by Qwt3DSurface)
    void setSurface(Qwt3DSurface* surface) { m_surface = surface; }

protected:
    virtual ~Qwt3DColor()
    {
    }

    /// Called by subclasses after mutating color state; triggers VBO rebuild on the owning surface
    void notifyColorChanged();

    Qwt3DSurface* m_surface = nullptr;
};

class Qwt3DPlot;

/**
 * @brief Standard color model for Qwt3DPlot - implements the data driven operator()(double x, double y, double z)
 * @details The class has a ColorVector representing z values, which will be used by
 *          operator()(double x, double y, double z)
 */
class QWT3D_EXPORT Qwt3DStandardColor : public Qwt3DColor
{
    QWT_DECLARE_PRIVATE(Qwt3DStandardColor)

public:
    // Initializes with data and set up a ColorVector with a size of 100 z values (default)
    explicit Qwt3DStandardColor(Qwt3DPlot* data, unsigned size = 100);
    ~Qwt3DStandardColor() override;
    // Receives z-dependent color from ColorVector
    RGBA operator()(double x, double y, double z) const override;
    void setColorVector(ColorVector const& cv);
    // Resets the standard colors
    void reset(unsigned size = 100);
    // Sets unitary alpha value for all colors
    void setAlpha(double a);
    // Creates color vector for Qwt3DColorLegend - essentially a copy from the internal vector
    ColorVector& createVector(ColorVector& vec) override;

    // Set colormap from a preset name (e.g. "viridis", "plasma", "jet")
    void setPreset(const QString& presetName, unsigned size = 100);
};


#endif
