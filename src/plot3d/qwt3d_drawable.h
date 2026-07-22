#ifndef QWT3D_DRAWABLE_H
#define QWT3D_DRAWABLE_H

#include "qwt3d_global.h"
#include "qwt3d_types.h"

class Qwt3DPlot;

/**
 * @brief Abstract base class for Drawables
 * @details Drawables are scene-graph nodes rendered during paintGL.
 *          They no longer manage legacy GL state. Each drawable accesses
 *          the owning Qwt3DPlot to obtain shared shaders and coordinate
 *          conversion utilities.
 */
class QWT3D_EXPORT Qwt3DDrawable
{
    QWT_DECLARE_PRIVATE(Qwt3DDrawable)

public:
    virtual ~Qwt3DDrawable() = 0;

    // Draws the drawable and all attached children
    virtual void draw();

    void attach(Qwt3DDrawable*);
    void detach(Qwt3DDrawable*);
    void detachAll();

    virtual void setColor(double r, double g, double b, double a = 1);
    virtual void setColor(RGBA rgba);

    // Returns the owning plot (may be null)
    Qwt3DPlot* plot() const;
    // Sets the owning plot
    void setPlot(Qwt3DPlot* p);

    // Converts a relative viewport position to world coordinates
    Triple relativePosition(Triple rel) const;

protected:
    RGBA color;
    Qwt3DPlot* m_plot = nullptr;

    Qwt3DDrawable();
    Qwt3DDrawable(Qwt3DDrawable&& other) noexcept;
    Qwt3DDrawable& operator=(Qwt3DDrawable&& other) noexcept;
};

#endif
