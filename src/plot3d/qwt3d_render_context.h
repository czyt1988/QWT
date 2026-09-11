#ifndef QWT3D_RENDER_CONTEXT_H
#define QWT3D_RENDER_CONTEXT_H

#include "qwt3d_global.h"
#include "qwt3d_types.h"

#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QPointF>
#include <QSize>

/**
 * @brief Bundles all widget resources that drawables need during rendering
 * @details A value struct constructed per-call-site in Qwt3DPlot::paintGL()
 *          with the matrices, viewport, and shared shader programs current
 *          at that moment. Passed by const reference down the drawable draw
 *          chain, replacing the former Qwt3DPlot* back-pointer baked into
 *          Qwt3DDrawable (deferred smell #1, resolved).
 *
 * The coordinate-conversion methods (worldToScreen, screenToWorld,
 * relativePosition) are pure CPU math over the stored matrices and viewport
 * — no OpenGL calls — so the context is self-contained and copyable.
 *
 * @code
 * // Constructed in paintGL() per draw phase:
 * Qwt3DRenderContext ctx;
 * ctx.modelView = d->m_modelView;
 * ctx.projection = d->m_projection;
 * ctx.viewport = viewportSize();
 * ctx.lineShader = d->m_lineShader;
 * ctx.polygonShader = d->m_polygonShader;
 * ctx.textShader = d->m_textShader;
 *
 * d->m_coordinates.draw(ctx);
 * @endcode
 */
struct QWT3D_EXPORT Qwt3DRenderContext
{
    // Matrices — current at time of construction (per-call-site in paintGL)
    QMatrix4x4 modelView;
    QMatrix4x4 projection;

    // Viewport in pixels (already resolved from widget size fallback)
    QSize viewport;

    // Shared shader programs (non-owning pointers, valid during paintGL)
    QOpenGLShaderProgram* lineShader = nullptr;
    QOpenGLShaderProgram* polygonShader = nullptr;
    QOpenGLShaderProgram* textShader = nullptr;

    // Projects a world-space point to screen (viewport) pixel coordinates
    QPointF worldToScreen(const Triple& world) const;

    // Unprojects a screen (viewport) pixel point to world-space coordinates
    Triple screenToWorld(const QPointF& screen) const;

    // Converts a relative viewport position [0..1] to world coordinates
    Triple relativePosition(Triple rel) const;
};

#endif
