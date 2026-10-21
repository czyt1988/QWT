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

#include "qwt_scale_corner.h"

#include <qmath.h>
#include <QWidget>

#include "qwt_abstract_scale_draw.h"
#include "qwt_plot.h"
#include "qwt_plot_layout.h"
#include "qwt_scale_widget.h"

namespace
{
//! The perpendicular axes sharing the start/end corner of an axis
void cornerAxes(QwtAxisId axisId, QwtAxisId& startAxis, QwtAxisId& endAxis)
{
    if (QwtAxis::isYAxis(axisId)) {
        startAxis = QwtAxis::XTop;
        endAxis   = QwtAxis::XBottom;
    } else {
        startAxis = QwtAxis::YLeft;
        endAxis   = QwtAxis::YRight;
    }
}

bool hasBackbone(const QwtPlot* plot, QwtAxisId axisId)
{
    if (!plot || !plot->isAxisVisible(axisId))
        return false;

    const QwtScaleDraw* sd = plot->axisScaleDraw(axisId);
    return sd && sd->hasComponent(QwtAbstractScaleDraw::Backbone);
}

/*
    All plots sharing the canvas of plot: the host (or plot itself) and
    every parasite stacked on it. The perpendicular axis of a corner may
    live on any of them, and several of them can carry a band of the same
    axis position, stacked outwards from the canvas.
 */
QList<const QwtPlot*> cornerFamily(const QwtPlot* plot)
{
    QList<const QwtPlot*> family;
    if (!plot)
        return family;

    const QwtPlot* root = plot;
    while (root->hostPlot())
        root = root->hostPlot();

    family += root;

    const QList<QwtPlot*> parasites = root->parasitePlots();
    for (const QwtPlot* parasite : parasites)
        family += parasite;

    return family;
}

/*
    Logical coordinate of the near edge of the backbone band of a scale draw:
    the row for horizontal scales, the column for vertical scales. This is the
    edge closest to the canvas the band belongs to.
 */
qreal bandPosition(const QwtScaleDraw* scaleDraw)
{
    switch (scaleDraw->alignment()) {
    case QwtScaleDraw::BottomScale:
    case QwtScaleDraw::TopScale:
        return scaleDraw->pos().y();
    case QwtScaleDraw::RightScale:
    case QwtScaleDraw::LeftScale:
    default:
        return scaleDraw->pos().x();
    }
}
}

/**
 * @brief Compute the corner miter flags for an axis of a plot
 * @param plot Plot the axis belongs to
 * @param axisId Axis
 * @return Miter flags for the corners shared with visible perpendicular axes
 * @details A corner exists when a perpendicular axis is visible and paints a
 *          backbone - in the plot itself or anywhere in its family (the host
 *          and the sibling parasites sharing the canvas).
 */
QwtScaleDraw::CornerMiter QwtScaleCorner::miterFlags(const QwtPlot* plot, QwtAxisId axisId)
{
    QwtScaleDraw::CornerMiter miter = QwtScaleDraw::NoMiter;

    QwtAxisId startAxis, endAxis;
    cornerAxes(axisId, startAxis, endAxis);

    const QList<const QwtPlot*> family = cornerFamily(plot);

    for (const QwtPlot* p : family) {
        if (hasBackbone(p, startAxis))
            miter |= QwtScaleDraw::MiterAtStart;
        if (hasBackbone(p, endAxis))
            miter |= QwtScaleDraw::MiterAtEnd;
    }

    return miter;
}

/**
 * @brief Compute where the backbone miter should reach, from the axis widgets
 * @param plot Plot the axis belongs to
 * @param axisId Axis
 * @return Target positions in logical coordinates of plot
 * @details The targets are derived from the geometry and scale draw positions
 *          of the perpendicular axis widgets of the whole plot family. Per end
 *          the target is the near edge of the innermost band - the one closest
 *          to the canvas, where a border tick sits. Bands stacked farther out
 *          are not crossed. Call after the plot layout has been applied to
 *          the widgets of the family.
 */
QwtScaleCorner::Targets QwtScaleCorner::widgetTargetPositions(const QwtPlot* plot, QwtAxisId axisId)
{
    Targets targets;

    QwtAxisId startAxis, endAxis;
    cornerAxes(axisId, startAxis, endAxis);

    const QPoint plotOrigin = plot ? plot->mapToGlobal(QPoint(0, 0)) : QPoint();
    const bool vertical     = QwtAxis::isYAxis(axisId);
    const QList<const QwtPlot*> family = cornerFamily(plot);

    /*
        Keep the position of the band closest to the canvas: the largest
        coordinate at the start of the scale (top/left) and the smallest one
        at the end (bottom/right).
     */
    const auto collect = [&](QwtAxisId perpAxisId, bool atStart, qreal& target) {
        for (const QwtPlot* p : family) {
            if (!hasBackbone(p, perpAxisId))
                continue;

            const QwtScaleWidget* w = p->axisWidget(perpAxisId);
            if (!w || !w->scaleDraw() || w->width() <= 0 || w->height() <= 0)
                continue;

            qreal position = bandPosition(w->scaleDraw());

            // widget origins are integer, so the fractional part of the band
            // position survives the round trip through global coordinates
            const QPoint delta = w->mapToGlobal(QPoint(0, 0)) - plotOrigin;
            position += vertical ? delta.y() : delta.x();

            if (qIsNaN(target) || (atStart ? position > target : position < target))
                target = position;
        }
    };

    collect(startAxis, true, targets.start);
    collect(endAxis, false, targets.end);

    return targets;
}

/**
 * @brief Compute where the backbone miter should reach, from the plot layout
 * @param plot Plot the axis belongs to
 * @param axisId Axis
 * @return Target positions in logical coordinates of the plot layout
 * @details Variant of widgetTargetPositions() for QwtPlotRenderer: the
 *          renderer activates the plot layout for the paint device and
 *          positions the scales from the layout rectangles, so the widget
 *          geometries are not a reliable source here. Stacked parasite axes
 *          are not rendered into documents and are ignored. The plot layout
 *          must be activated when calling.
 */
QwtScaleCorner::Targets QwtScaleCorner::layoutTargetPositions(const QwtPlot* plot, QwtAxisId axisId)
{
    Targets targets;

    if (!plot)
        return targets;

    QwtAxisId startAxis, endAxis;
    cornerAxes(axisId, startAxis, endAxis);

    const auto collect = [plot](QwtAxisId perpAxisId, qreal& target) {
        if (!hasBackbone(plot, perpAxisId))
            return;

        const QwtScaleWidget* w = plot->axisWidget(perpAxisId);
        if (!w)
            return;

        const QRectF r = plot->plotLayout()->scaleRect(perpAxisId);
        if (!r.isValid())
            return;

        int baseDist = w->margin();
        if (w->isColorBarEnabled() && w->colorBarWidth() > 0)
            baseDist += w->colorBarWidth() + w->spacing();

        // backbone position like QwtPlotRenderer::renderScale()
        switch (perpAxisId) {
        case QwtAxis::YLeft:
            target = r.right() - 1.0 - baseDist;
            break;
        case QwtAxis::YRight:
            target = r.left() + baseDist;
            break;
        case QwtAxis::XTop:
            target = r.bottom() - 1.0 - baseDist;
            break;
        case QwtAxis::XBottom:
            target = r.top() + baseDist;
            break;
        default:
            target = qQNaN();
            return;
        }
    };

    collect(startAxis, targets.start);
    collect(endAxis, targets.end);

    return targets;
}

/**
 * @brief Compute how much an axis widget has to reserve for the corner miter
 * @param plot Plot the axis belongs to
 * @param axisId Axis
 * @param devicePixelRatio Device pixel ratio of the axis widget
 * @param[out] startExtension Logical pixels to reserve at the start of the scale
 * @param[out] endExtension Logical pixels to reserve at the end of the scale
 * @details QwtScaleWidget clips everything painted outside of its rectangle,
 *          so a backbone miter reaching the perpendicular backbone is only
 *          visible when the widget covers the corner pixels. The extensions
 *          are the gap between the widget borders and the targets of
 *          widgetTargetPositions(), rounded up to whole logical pixels. The
 *          caller grows the widget rectangle accordingly and passes the same
 *          values to QwtScaleWidget::setCornerExtension(), which keeps the
 *          scale span unchanged by absorbing them into the border distances.
 */
void QwtScaleCorner::widgetExtensions(const QwtPlot* plot, QwtAxisId axisId,
                                      qreal devicePixelRatio, int& startExtension, int& endExtension)
{
    startExtension = 0;
    endExtension   = 0;

    if (!plot || devicePixelRatio <= 0.0)
        return;

    const QwtScaleWidget* w = plot->axisWidget(axisId);
    if (!w)
        return;

    const Targets targets = widgetTargetPositions(plot, axisId);

    const bool vertical = QwtAxis::isYAxis(axisId);
    const QRect r       = w->geometry();

    if (!qIsNaN(targets.start)) {
        // the widget border must not sit after the first pixel of the target
        const qreal edge   = vertical ? r.top() : r.left();
        const qreal target = qRound(targets.start * devicePixelRatio) / devicePixelRatio;
        startExtension     = qMax(0, static_cast< int >(qCeil(edge - target)));
    }

    if (!qIsNaN(targets.end)) {
        const qreal edge   = (vertical ? r.bottom() : r.right()) + 1.0;
        const qreal target = (qRound(targets.end * devicePixelRatio) + 1.0) / devicePixelRatio;
        endExtension       = qMax(0, static_cast< int >(qCeil(target - edge)));
    }
}
