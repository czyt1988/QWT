#ifndef QWT3D_COLOR_H
#define QWT3D_COLOR_H

#include <qstring.h>

#include "qwt3d_global.h"
#include "qwt3d_types.h"

/**
 * @brief Abstract base class for color functors
 * @details Use your own color model by providing an implementation of
 *          operator()(double x, double y, double z). Colors destructor has been
 *          declared protected, in order to use only heap based objects. Plot3D
 *          will handle the objects destruction. See Qwt3DStandardColor for an example.
 *
 * Qwt3DColor is a pure value object: it holds no back-pointer to its owner and
 * never notifies anyone when its state mutates. The owning Qwt3DPlotItem is
 * responsible for triggering a rebuild after an in-place mutation (see
 * Qwt3DSurface::invalidateColors). The active z-range used for color
 * normalization is pushed in by the owner via setActiveRange() before the
 * functor is queried. This mirrors the 2D module, where QwtColorMap is a silent
 * value object and QwtPlotSpectrogram::setColorMap() drives the notification.
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

    /// Sets the active z-range used for color normalization (pushed by the owning Qwt3DPlotItem before VBO/legend build)
    void setActiveRange(double zMin, double zMax)
    {
        m_zMin = zMin;
        m_zMax = zMax;
    }

protected:
    virtual ~Qwt3DColor()
    {
    }

    /// Lower bound of the active z-range (default 0.0)
    double activeZMin() const { return m_zMin; }
    /// Upper bound of the active z-range (default 1.0)
    double activeZMax() const { return m_zMax; }

private:
    double m_zMin = 0.0;
    double m_zMax = 1.0;
};

/**
 * @brief Standard color model for Qwt3DPlot - implements the data driven operator()(double x, double y, double z)
 * @details The class has a ColorVector representing z values, which will be used by
 *          operator()(double x, double y, double z)
 */
class QWT3D_EXPORT Qwt3DStandardColor : public Qwt3DColor
{
    QWT_DECLARE_PRIVATE(Qwt3DStandardColor)

public:
    // Initializes with a ColorVector of the given size sampled from the viridis colormap (default)
    explicit Qwt3DStandardColor(unsigned size = 100);
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

    // Returns the current preset name (empty if set via setColorVector)
    QString presetName() const;
    // Returns the number of colors in the color vector
    unsigned colorCount() const;
    // Returns the alpha value applied by the last setAlpha() call (default 1.0)
    double alpha() const;
};


#endif
