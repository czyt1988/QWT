#include "qwt3d_render_context.h"

/**
 * @brief Projects a world-space point to screen pixel coordinates
 * @param world World-space Triple
 * @return Screen-space QPointF (pixel coordinates)
 * @details Uses the CPU-side modelView and projection matrices to transform
 *          world coordinates to normalized device coordinates, then maps
 *          to viewport pixels.
 */
QPointF Qwt3DRenderContext::worldToScreen(const Triple& world) const
{
    QVector4D worldVec(static_cast< float >(world.x),
                        static_cast< float >(world.y),
                        static_cast< float >(world.z), 1.0f);
    QVector4D clipVec = projection.map(modelView.map(worldVec));

    if (clipVec.w() == 0.0f)
        return QPointF(0, 0);

    const float ndcX = clipVec.x() / clipVec.w();
    const float ndcY = clipVec.y() / clipVec.w();

    const float screenX = (ndcX + 1.0f) * 0.5f * viewport.width();
    const float screenY = (1.0f - (ndcY + 1.0f) * 0.5f) * viewport.height();

    return QPointF(screenX, screenY);
}

/**
 * @brief Unprojects screen pixel coordinates to a world-space point
 * @param screen Screen-space point (pixel coordinates)
 * @return World-space Triple
 * @details Uses the inverse of the CPU-side modelView and projection matrices
 *          to unproject screen coordinates. The z-component is determined
 *          by the near plane (z=0 in NDC).
 */
Triple Qwt3DRenderContext::screenToWorld(const QPointF& screen) const
{
    if (viewport.width() <= 0 || viewport.height() <= 0)
        return Triple(0, 0, 0);

    const float ndcX = 2.0f * static_cast< float >(screen.x()) / viewport.width() - 1.0f;
    const float ndcY = 1.0f - 2.0f * static_cast< float >(screen.y()) / viewport.height();

    QVector3D clipVec(ndcX, ndcY, 0.0f);

    QMatrix4x4 invMVP = (projection * modelView).inverted();
    QVector3D worldVec = invMVP.map(clipVec);

    return Triple(worldVec.x(), worldVec.y(), worldVec.z());
}

/**
 * @brief Converts a relative viewport position to world coordinates
 * @param rel Relative position in viewport coordinates [0..1]
 * @return Corresponding world coordinates
 * @details Scales the relative position by the viewport dimensions to obtain
 *          screen pixel coordinates, then unprojects to world space.
 */
Triple Qwt3DRenderContext::relativePosition(Triple rel) const
{
    if (viewport.width() <= 0 || viewport.height() <= 0)
        return Triple(0, 0, 0);

    QPointF screen(rel.x * viewport.width(), rel.y * viewport.height());
    return screenToWorld(screen);
}
