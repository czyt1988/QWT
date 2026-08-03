#ifndef QWT3D_DRAWABLE_H
#define QWT3D_DRAWABLE_H

#include "qwt3d_global.h"
#include "qwt3d_render_context.h"
#include "qwt3d_types.h"

/**
 * @brief Abstract base class for Drawables
 * @details Drawables are scene-graph nodes rendered during paintGL.
 *          They no longer manage legacy GL state. Each drawable receives
 *          its rendering resources (shaders, matrices, viewport, coordinate
 *          conversion) through a Qwt3DRenderContext passed to draw(),
 *          eliminating the former Qwt3DPlot* back-pointer (deferred smell #1,
 *          resolved).
 */
class QWT3D_EXPORT Qwt3DDrawable
{
    QWT_DECLARE_PRIVATE(Qwt3DDrawable)

public:
    virtual ~Qwt3DDrawable() = 0;

    // Draws the drawable and all attached children
    virtual void draw(const Qwt3DRenderContext& ctx);

    void attach(Qwt3DDrawable*);
    void detach(Qwt3DDrawable*);
    void detachAll();

    virtual void setColor(double r, double g, double b, double a = 1);
    virtual void setColor(RGBA rgba);

protected:
    RGBA color;

    Qwt3DDrawable();
    Qwt3DDrawable(Qwt3DDrawable&& other) noexcept;
    Qwt3DDrawable& operator=(Qwt3DDrawable&& other) noexcept;
};

#endif
