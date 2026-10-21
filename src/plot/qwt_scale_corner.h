/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *
 * Modified by ChenZongYan in 2024 <czy.t@163.com>
 *****************************************************************************/

#ifndef QWT_SCALE_CORNER_H
#define QWT_SCALE_CORNER_H

#include "qwt_global.h"
#include "qwt_axis_id.h"
#include "qwt_scale_draw.h"

class QwtPlot;

/**
 * @brief Helpers for the corners where perpendicular scale backbones meet
 * @details When two perpendicular axes are visible, their backbones should
 *          intersect at the shared canvas corner instead of ending in a
 *          diagonal step. QwtScaleDraw extends (miters) the backbone into
 *          the corner for this purpose. These helpers answer the questions
 *          around that extension:
 *
 *          - which corners exist (miterFlags()),
 *          - where the perpendicular backbone actually is, so that the
 *            extension reaches it exactly (widgetTargetPositions() for the
 *            on-screen widgets, layoutTargetPositions() for QwtPlotRenderer,
 *            which lays out independently of the widget geometries),
 *          - how many logical pixels an axis widget has to reserve beyond
 *            its ends, so that the extension is not clipped away by the
 *            widget rectangle (widgetExtensions()).
 *
 *          Parasite plots share the canvas of their host and stack their
 *          axis bands outside of the bands of the host, so the perpendicular
 *          band adjacent to the canvas may belong to any plot of the family.
 *          The target is the near edge of that innermost band - the band a
 *          border tick sits on. Bands stacked farther out are not crossed:
 *          extending a backbone to them would draw long lines through the
 *          whole axis stack and close the corners of unrelated bands.
 *
 *          All positions are in logical coordinates of the plot the axis
 *          belongs to; qQNaN() means "no corner at this end".
 */
namespace QwtScaleCorner
{
/// @brief Target positions of the perpendicular backbones at one corner
struct QWT_EXPORT Targets
{
    /// Near edge of the innermost perpendicular band at the start of the scale
    qreal start = qQNaN();
    /// Near edge of the innermost perpendicular band at the end of the scale
    qreal end = qQNaN();

    /// @return true when any target is set
    bool isValid() const
    {
        return !qIsNaN(start) || !qIsNaN(end);
    }
};

/// @return the corner miter flags for an axis of a plot
QWT_EXPORT QwtScaleDraw::CornerMiter miterFlags(const QwtPlot* plot, QwtAxisId axisId);

/// Target positions derived from the axis widgets of the whole plot family
QWT_EXPORT Targets widgetTargetPositions(const QwtPlot* plot, QwtAxisId axisId);

/// Target positions derived from the plot layout (for QwtPlotRenderer)
QWT_EXPORT Targets layoutTargetPositions(const QwtPlot* plot, QwtAxisId axisId);

/// Logical pixels an axis widget must reserve at its ends for the miter
QWT_EXPORT void widgetExtensions(const QwtPlot* plot, QwtAxisId axisId,
                                 qreal devicePixelRatio, int& startExtension, int& endExtension);
}

#endif
