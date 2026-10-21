/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *
 * Modified by ChenZongYan in 2024 <czy.t@163.com>
 *   Summary of major modifications (see ChangeLog.md for full history):
 *   1. CMake build system & C++11 throughout.
 *   2. Core panner/ zoomer refactored:
 *        - QwtPanner → QwtCachePanner (pixmap-cache version)
 *        - New real-time QwtPlotPanner derived from QwtPicker.
 *   3. Zoomer supports multi-axis.
 *   4. Parasite-plot framework:
 *        - QwtFigure, QwtPlotParasiteLayout, QwtPlotTransparentCanvas,
 *        - QwtPlotScaleEventDispatcher, built-in pan/zoom on axis.
 *   5. New picker: QwtPlotSeriesDataPicker (works with date axis).
 *   6. Raster & color-map extensions:
 *        - QwtGridRasterData (2-D table + interpolation)
 *        - QwtLinearColorMap::stopColors(), stopPos() API rename.
 *   7. Bar-chart: expose pen/brush control.
 *   8. Amalgamated build: single QwtPlot.h / QwtPlot.cpp pair in src-amalgamate.
 *****************************************************************************/

#include "qwt_scale_draw.h"
#include "qwt_scale_div.h"
#include "qwt_scale_map.h"
#include "qwt_math.h"
#include "qwt_text.h"

#include <qpainter.h>
#include <qmath.h>

static inline double qwtEffectivePenWidth(const QwtAbstractScaleDraw* scaleDraw)
{
    return qwtMaxF(scaleDraw->penWidthF(), 1.0);
}

namespace
{
/*
    Backbone and ticks are filled as device pixel aligned rectangles instead of
    being stroked with a pen: every edge lands on an integer device coordinate,
    so the scale renders as crisp hairlines at any device pixel ratio.

    The pen width is a device pixel contract: it is rounded to whole device
    pixels without being scaled by the device pixel ratio, so setPenWidthF(1.0)
    paints a one device pixel wide line at any scaling (0.0 maps to 1 as well).
    Tick lengths, in contrast, stay in logical pixels and are scaled by the
    device pixel ratio (see qwtDeviceTickLength()).
 */
inline int qwtDevicePenWidth(const QPainter* painter)
{
    return qMax(1, qRound(painter->pen().widthF() + 1e-6));
}

inline int qwtDeviceTickLength(qreal length, qreal deviceStep)
{
    return qMax(1, qRound(length * deviceStep));
}

/*
    Device position a corner miter reaches: the target of the perpendicular
    backbone when one is set, otherwise the historical one pixel beyond the
    end of the scale.
 */
inline int qwtCornerMiterPos(qreal target, int fallback)
{
    return qIsNaN(target) ? fallback : qRound(target);
}

/*
    Device pixels a corner miter reaches beyond the target: the target is the
    near edge of the perpendicular backbone band, and closing the corner
    square for pen widths above one needs the far edge of that band as well.
 */
inline int qwtCornerMiterWidth(qreal width)
{
    return qMax(1, qRound(width + 1e-6));
}

/*
    Offset of a border tick that snapped onto the perpendicular backbone
    band: the tick thickness is aligned into the span of that band, which
    grows away from the canvas for top/left bands. Without the alignment an
    even pen width would leave the tick off the band by up to width - 1
    device pixels; unsnapped ticks ( bandWidth 0 ) keep the historical
    centering on their value position.
 */
inline int qwtSnappedTickShift(int bandWidth, int tickWidth, bool atStart)
{
    if (bandWidth <= 0)
        return (tickWidth - 1) / 2;

    // the band of a start ( top/left ) perpendicular grows away from the
    // canvas, the band of an end ( bottom/right ) one towards it
    const int inset = qMax(0, (bandWidth - tickWidth) / 2);
    return atStart ? (bandWidth - 1) - inset : -inset;
}

inline void qwtFillDeviceRect(QPainter* painter, const QRect& deviceRect)
{
    /*
        Map the device pixel aligned rectangle back into paint coordinates:
        the edges land on integer device pixels after the paint engine has
        applied the device pixel ratio, so backbone and ticks stay crisp
        hairlines at any scaling.
     */
    painter->fillRect(painter->deviceTransform().inverted().mapRect(QRectF(deviceRect)), painter->pen().color());
}

inline qreal qwtDeviceX(const QTransform& tr, qreal x)
{
    return tr.map(QPointF(x, 0.0)).x();
}

inline qreal qwtDeviceY(const QTransform& tr, qreal y)
{
    return tr.map(QPointF(0.0, y)).y();
}

/*
    Device coordinate of the canvas side ( near ) edge of a backbone band.

    The position of a scale draw is a logical pixel: at device pixel ratios
    above one it covers several device pixels. A right/bottom band grows
    towards the canvas from the first of them - the one qRound() of the
    position yields, which is the pixel adjacent to the canvas. A left/top
    band grows away from the canvas, so its near edge is the last device
    pixel of the position instead: rounding the position itself would leave
    the band one device pixel short of the canvas per missing pixel of the
    logical pixel.
 */
inline int qwtNearBandEdgeX(const QTransform& tr, qreal x, bool growsAwayFromCanvas)
{
    return growsAwayFromCanvas ? qRound(qwtDeviceX(tr, x + 1.0)) - 1 : qRound(qwtDeviceX(tr, x));
}

inline int qwtNearBandEdgeY(const QTransform& tr, qreal y, bool growsAwayFromCanvas)
{
    return growsAwayFromCanvas ? qRound(qwtDeviceY(tr, y + 1.0)) - 1 : qRound(qwtDeviceY(tr, y));
}
}

class QwtScaleDraw::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtScaleDraw)
public:
    PrivateData(QwtScaleDraw* p) : q_ptr(p), len(0), alignment(QwtScaleDraw::BottomScale), labelRotation(0.0)
    {
    }

    QPointF pos;
    double len;

    Alignment alignment;

    Qt::Alignment labelAlignment;
    double labelRotation;

    QwtScaleDraw::CornerMiter cornerMiter { QwtScaleDraw::NoMiter };

    /*
        Device coordinates ( y for vertical, x for horizontal scales ) of the
        near edges of the perpendicular backbone bands the miter has to reach
        at the start/end of the scale. qQNaN() falls back to extending one
        pixel beyond the end of the scale.
     */
    QwtScaleDraw::CornerTargets cornerTargets;
};

/**
 * @brief Constructor
 * @details The range of the scale is initialized to [0, 100], the position is at (0, 0)
 *          with a length of 100. The orientation is QwtAbstractScaleDraw::Bottom.
 */
QwtScaleDraw::QwtScaleDraw() : QWT_PIMPL_CONSTRUCT
{
    setLength(100);
}

//! Destructor
QwtScaleDraw::~QwtScaleDraw()
{
}

/**
 * @brief Set the corner miter flags
 * @param miter Flags specifying which ends of the backbone are extended
 *              into a corner shared with a perpendicular scale
 * @details QwtScaleWidget sets the flags from the visibility of the
 *          perpendicular axes of the plot; standalone use keeps NoMiter.
 * @sa cornerMiter()
 */
void QwtScaleDraw::setCornerMiter(CornerMiter miter)
{
    QWT_D(d);
    d->cornerMiter = miter;
}

/**
 * @brief Get the corner miter flags
 * @return Flags specifying which ends of the backbone are extended
 * @sa setCornerMiter()
 */
QwtScaleDraw::CornerMiter QwtScaleDraw::cornerMiter() const
{
    QWT_DC(d);
    return d->cornerMiter;
}

/**
 * @brief Set the corner miter target positions
 * @param targets Target positions in device coordinates
 * @details When a miter flag is set and its target is valid, the backbone is
 *          extended exactly to that position instead of one pixel beyond the
 *          end of the scale; border ticks snap to the same target. The
 *          target is the near edge of the perpendicular backbone band
 *          closest to the canvas. QwtScaleWidget and QwtPlotRenderer compute
 *          the targets from the geometry of the perpendicular axes;
 *          standalone use keeps them unset and the historical fallback.
 * @sa setCornerMiter(), cornerTargets(), CornerTargets
 */
void QwtScaleDraw::setCornerTargets(const CornerTargets& targets)
{
    QWT_D(d);
    d->cornerTargets = targets;
}

/**
 * @brief Get the corner miter target positions
 * @return Target positions in device coordinates, qQNaN() when unset
 * @sa setCornerTargets()
 */
QwtScaleDraw::CornerTargets QwtScaleDraw::cornerTargets() const
{
    QWT_DC(d);
    return d->cornerTargets;
}

/**
 * @brief Return alignment of the scale
 * @return Alignment of the scale
 * @sa setAlignment()
 */
QwtScaleDraw::Alignment QwtScaleDraw::alignment() const
{
    QWT_DC(d);
    return d->alignment;
}

/**
 * @brief Set the alignment of the scale
 * @param align Alignment of the scale
 * @details The default alignment is QwtScaleDraw::BottomScale
 * @sa alignment()
 */
void QwtScaleDraw::setAlignment(Alignment align)
{
    QWT_D(d);
    d->alignment = align;
}

/**
 * @brief Return the orientation
 * @details TopScale, BottomScale are horizontal (Qt::Horizontal) scales,
 *          LeftScale, RightScale are vertical (Qt::Vertical) scales.
 * @return Orientation of the scale
 * @sa alignment()
 */
Qt::Orientation QwtScaleDraw::orientation() const
{
    QWT_DC(d);
    switch (d->alignment) {
    case TopScale:
    case BottomScale:
        return Qt::Horizontal;
    case LeftScale:
    case RightScale:
    default:
        return Qt::Vertical;
    }
}

/**
 * @brief Determine the minimum border distance
 *
 * This member function returns the minimum space
 * needed to draw the mark labels at the scale's endpoints.
 *
 * @param font Font
 * @param start Start border distance
 * @param end End border distance
 *
 */
void QwtScaleDraw::getBorderDistHint(const QFont& font, int& start, int& end) const
{
    start = 0;
    end   = 1.0;

    if (!hasComponent(QwtAbstractScaleDraw::Labels))
        return;

    const QList< double >& ticks = scaleDiv().ticks(QwtScaleDiv::MajorTick);
    if (ticks.count() == 0)
        return;

    // Find the ticks, that are mapped to the borders.
    // minTick is the tick, that is mapped to the top/left-most position
    // in widget coordinates.

    double minTick = ticks[ 0 ];
    double minPos  = scaleMap().transform(minTick);
    double maxTick = minTick;
    double maxPos  = minPos;

    for (int i = 1; i < ticks.count(); i++) {
        const double tickPos = scaleMap().transform(ticks[ i ]);
        if (tickPos < minPos) {
            minTick = ticks[ i ];
            minPos  = tickPos;
        }
        if (tickPos > scaleMap().transform(maxTick)) {
            maxTick = ticks[ i ];
            maxPos  = tickPos;
        }
    }

    double e = 0.0;
    double s = 0.0;
    if (orientation() == Qt::Vertical) {
        s = -labelRect(font, minTick).top();
        s -= qAbs(minPos - qRound(scaleMap().p2()));

        e = labelRect(font, maxTick).bottom();
        e -= qAbs(maxPos - scaleMap().p1());
    } else {
        s = -labelRect(font, minTick).left();
        s -= qAbs(minPos - scaleMap().p1());

        e = labelRect(font, maxTick).right();
        e -= qAbs(maxPos - scaleMap().p2());
    }

    if (s < 0.0)
        s = 0.0;
    if (e < 0.0)
        e = 0.0;

    start = qwtCeil(s);
    end   = qwtCeil(e);
}

/**
 * @brief Determine the minimum distance between two labels
 *
 * Determine the minimum distance between two labels, that is necessary
 * that the texts don't overlap.
 *
 * @param font Font
 * @return The maximum width of a label
 *
 * @sa getBorderDistHint()
 *
 */

int QwtScaleDraw::minLabelDist(const QFont& font) const
{
    if (!hasComponent(QwtAbstractScaleDraw::Labels))
        return 0;

    const QList< double >& ticks = scaleDiv().ticks(QwtScaleDiv::MajorTick);
    if (ticks.isEmpty())
        return 0;

    const QFontMetrics fm(font);

    const bool vertical = (orientation() == Qt::Vertical);

    QRectF bRect1;
    QRectF bRect2 = labelRect(font, ticks[ 0 ]);
    if (vertical) {
        bRect2.setRect(-bRect2.bottom(), 0.0, bRect2.height(), bRect2.width());
    }

    double maxDist = 0.0;

    for (int i = 1; i < ticks.count(); i++) {
        bRect1 = bRect2;
        bRect2 = labelRect(font, ticks[ i ]);
        if (vertical) {
            bRect2.setRect(-bRect2.bottom(), 0.0, bRect2.height(), bRect2.width());
        }

        double dist = fm.leading();  // space between the labels
        if (bRect1.right() > 0)
            dist += bRect1.right();
        if (bRect2.left() < 0)
            dist += -bRect2.left();

        if (dist > maxDist)
            maxDist = dist;
    }

    double angle = qwtRadians(labelRotation());
    if (vertical)
        angle += M_PI / 2;

    const double sinA = qFastSin(angle);  // qreal -> double
    if (qFuzzyCompare(sinA + 1.0, 1.0))
        return qCeil(maxDist);

    const int fmHeight = fm.ascent() - 2;

    // The distance we need until there is
    // the height of the label font. This height is needed
    // for the neighbored label.

    double labelDist = fmHeight / qFastSin(angle) * qFastCos(angle);
    if (labelDist < 0)
        labelDist = -labelDist;

    // For text orientations close to the scale orientation

    if (labelDist > maxDist)
        labelDist = maxDist;

    // For text orientations close to the opposite of the
    // scale orientation

    if (labelDist < fmHeight)
        labelDist = fmHeight;

    return qCeil(labelDist);
}

/**
 * @brief Calculate the width/height that is needed for a vertical/horizontal scale
 *
 * The extent is calculated from the pen width of the backbone,
 * the major tick length, the spacing and the maximum width/height
 * of the labels.
 *
 * @param font Font used for painting the labels
 * @return Extent
 *
 * @sa minLength()
 *
 */
double QwtScaleDraw::extent(const QFont& font) const
{
    double d = 0;

    if (hasComponent(QwtAbstractScaleDraw::Labels)) {
        if (orientation() == Qt::Vertical)
            d = maxLabelWidth(font);
        else
            d = maxLabelHeight(font);

        if (d > 0)
            d += spacing();
    }

    if (hasComponent(QwtAbstractScaleDraw::Ticks)) {
        d += maxTickLength();
    }

    if (hasComponent(QwtAbstractScaleDraw::Backbone)) {
        d += qwtEffectivePenWidth(this);
    }

    d = qwtMaxF(d, minimumExtent());
    return d;
}

/**
 * @brief Calculate the minimum length that is needed to draw the scale
 *
 * @param font Font used for painting the labels
 * @return Minimum length that is needed to draw the scale
 *
 * @sa extent()
 *
 */
int QwtScaleDraw::minLength(const QFont& font) const
{
    int startDist, endDist;
    getBorderDistHint(font, startDist, endDist);

    const QwtScaleDiv& sd = scaleDiv();

    const uint minorCount = sd.ticks(QwtScaleDiv::MinorTick).count() + sd.ticks(QwtScaleDiv::MediumTick).count();
    const uint majorCount = sd.ticks(QwtScaleDiv::MajorTick).count();

    int lengthForLabels = 0;
    if (hasComponent(QwtAbstractScaleDraw::Labels))
        lengthForLabels = minLabelDist(font) * majorCount;

    int lengthForTicks = 0;
    if (hasComponent(QwtAbstractScaleDraw::Ticks)) {
        const double pw = qwtEffectivePenWidth(this);
        lengthForTicks  = qCeil((majorCount + minorCount) * (pw + 1.0));
    }

    return startDist + endDist + qMax(lengthForLabels, lengthForTicks);
}

/**
 * @brief Find the position, where to paint a label
 *
 * The position has a distance that depends on the length of the ticks
 * in direction of the alignment().
 *
 * @param value Value
 * @return Position, where to paint a label
 *
 */
QPointF QwtScaleDraw::labelPosition(double value) const
{
    QWT_DC(d);
    const double tval = scaleMap().transform(value);
    double dist       = spacing();
    if (hasComponent(QwtAbstractScaleDraw::Backbone))
        dist += qwtEffectivePenWidth(this);

    if (hasComponent(QwtAbstractScaleDraw::Ticks))
        dist += tickLength(QwtScaleDiv::MajorTick);

    double px = 0;
    double py = 0;

    switch (alignment()) {
    case RightScale: {
        px = d->pos.x() + dist;
        py = tval;
        break;
    }
    case LeftScale: {
        px = d->pos.x() - dist;
        py = tval;
        break;
    }
    case BottomScale: {
        px = tval;
        py = d->pos.y() + dist;
        break;
    }
    case TopScale: {
        px = tval;
        py = d->pos.y() - dist;
        break;
    }
    }

    return QPointF(px, py);
}

/**
 * @brief Draw a tick
 *
 * @param painter Painter
 * @param value Value of the tick
 * @param len Length of the tick
 *
 * @sa drawBackbone(), drawLabel()
 *
 */
void QwtScaleDraw::drawTick(QPainter* painter, double value, double len) const
{
    if (len <= 0.0)
        return;

    QWT_DC(d);
    const double tval = scaleMap().transform(value);

    const QTransform tr = painter->deviceTransform();
    const qreal sx      = qAbs(tr.m11());
    const qreal sy      = qAbs(tr.m22());

    const bool hasBackbone = hasComponent(QwtAbstractScaleDraw::Backbone);

    switch (d->alignment) {
    case LeftScale:
    case RightScale: {
        const int pw      = hasBackbone ? qwtDevicePenWidth(painter) : 0;
        const int inner   = qwtNearBandEdgeX(tr, d->pos.x(), d->alignment == LeftScale);
        const int tickLen = qwtDeviceTickLength(len, sx);

        // the tick ends at the inner edge of the backbone band, never crossing it
        int x0, w;
        if (d->alignment == LeftScale) {
            x0 = hasBackbone ? inner - pw + 1 - tickLen : inner - tickLen + 1;
            w  = tickLen + pw;
        } else {
            x0 = inner;
            w  = tickLen + pw;
        }

        const int th = qwtDevicePenWidth(painter);
        int row      = qRound(qwtDeviceY(tr, tval));

        // a tick at the scale border sits on the perpendicular backbone
        const int first = qRound(qwtDeviceY(tr, d->pos.y()));
        const int last  = qRound(qwtDeviceY(tr, d->pos.y() + d->len));
        int bandWidth   = 0;
        bool atStart    = false;
        if (row == first && (d->cornerMiter & MiterAtStart)) {
            row       = qwtCornerMiterPos(d->cornerTargets.start, qRound(qwtDeviceY(tr, d->pos.y() - 1.0)));
            bandWidth = qwtCornerMiterWidth(d->cornerTargets.startWidth);
            atStart   = true;
        } else if (row == last && (d->cornerMiter & MiterAtEnd)) {
            row       = qwtCornerMiterPos(d->cornerTargets.end, qRound(qwtDeviceY(tr, d->pos.y() + d->len + 1.0)));
            bandWidth = qwtCornerMiterWidth(d->cornerTargets.endWidth);
        }

        qwtFillDeviceRect(painter, QRect(x0, row - qwtSnappedTickShift(bandWidth, th, atStart), w, th));
        break;
    }
    case TopScale:
    case BottomScale: {
        const int pw      = hasBackbone ? qwtDevicePenWidth(painter) : 0;
        const int inner   = qwtNearBandEdgeY(tr, d->pos.y(), d->alignment == TopScale);
        const int tickLen = qwtDeviceTickLength(len, sy);

        int y0, h;
        if (d->alignment == TopScale) {
            y0 = hasBackbone ? inner - pw + 1 - tickLen : inner - tickLen + 1;
            h  = tickLen + pw;
        } else {
            y0 = inner;
            h  = tickLen + pw;
        }

        const int th = qwtDevicePenWidth(painter);
        int col      = qRound(qwtDeviceX(tr, tval));

        // a tick at the scale border sits on the perpendicular backbone
        const int first = qRound(qwtDeviceX(tr, d->pos.x()));
        const int last  = qRound(qwtDeviceX(tr, d->pos.x() + d->len));
        int bandWidth   = 0;
        bool atStart    = false;
        if (col == first && (d->cornerMiter & MiterAtStart)) {
            col       = qwtCornerMiterPos(d->cornerTargets.start, qRound(qwtDeviceX(tr, d->pos.x() - 1.0)));
            bandWidth = qwtCornerMiterWidth(d->cornerTargets.startWidth);
            atStart   = true;
        } else if (col == last && (d->cornerMiter & MiterAtEnd)) {
            col       = qwtCornerMiterPos(d->cornerTargets.end, qRound(qwtDeviceX(tr, d->pos.x() + d->len + 1.0)));
            bandWidth = qwtCornerMiterWidth(d->cornerTargets.endWidth);
        }

        qwtFillDeviceRect(painter, QRect(col - qwtSnappedTickShift(bandWidth, th, atStart), y0, th, h));
        break;
    }
    }
}

/**
 * @brief Draws the baseline of the scale
 *
 * @param painter Painter
 *
 * @sa drawTick(), drawLabel()
 *
 */
void QwtScaleDraw::drawBackbone(QPainter* painter) const
{
    QWT_DC(d);
    const QTransform tr = painter->deviceTransform();

    switch (d->alignment) {
    case LeftScale:
    case RightScale: {
        const int pw    = qwtDevicePenWidth(painter);
        const int inner = qwtNearBandEdgeX(tr, d->pos.x(), d->alignment == LeftScale);
        const int x0    = (d->alignment == LeftScale) ? inner - pw + 1 : inner;

        int y0 = qRound(qwtDeviceY(tr, d->pos.y()));
        int y1 = qRound(qwtDeviceY(tr, d->pos.y() + d->len));
        /*
            Where a corner miter exists the backbone ends exactly at the
            perpendicular band: clamping against the scale span would leave
            the first device pixel of the span sticking out beyond the band
            at device pixel ratios above one, where the span edge and the
            band edge fall into different device pixels of the same logical
            pixel.
         */
        if (d->cornerMiter & MiterAtStart)
            y0 = qwtCornerMiterPos(d->cornerTargets.start, qRound(qwtDeviceY(tr, d->pos.y() - 1.0)))
                - (qwtCornerMiterWidth(d->cornerTargets.startWidth) - 1);
        if (d->cornerMiter & MiterAtEnd)
            y1 = qwtCornerMiterPos(d->cornerTargets.end, qRound(qwtDeviceY(tr, d->pos.y() + d->len + 1.0)))
                + (qwtCornerMiterWidth(d->cornerTargets.endWidth) - 1);

        // the length can be negative (QwtPolarGrid)
        qwtFillDeviceRect(painter, QRect(x0, qMin(y0, y1), pw, qAbs(y1 - y0) + 1));
        break;
    }
    case TopScale:
    case BottomScale: {
        const int pw    = qwtDevicePenWidth(painter);
        const int inner = qwtNearBandEdgeY(tr, d->pos.y(), d->alignment == TopScale);
        const int y0    = (d->alignment == TopScale) ? inner - pw + 1 : inner;

        int x0 = qRound(qwtDeviceX(tr, d->pos.x()));
        int x1 = qRound(qwtDeviceX(tr, d->pos.x() + d->len));
        // see the vertical branch: the miter ends the backbone at the
        // perpendicular band instead of clamping it against the scale span
        if (d->cornerMiter & MiterAtStart)
            x0 = qwtCornerMiterPos(d->cornerTargets.start, qRound(qwtDeviceX(tr, d->pos.x() - 1.0)))
                - (qwtCornerMiterWidth(d->cornerTargets.startWidth) - 1);
        if (d->cornerMiter & MiterAtEnd)
            x1 = qwtCornerMiterPos(d->cornerTargets.end, qRound(qwtDeviceX(tr, d->pos.x() + d->len + 1.0)))
                + (qwtCornerMiterWidth(d->cornerTargets.endWidth) - 1);

        // the length can be negative (QwtPolarGrid)
        qwtFillDeviceRect(painter, QRect(qMin(x0, x1), y0, qAbs(x1 - x0) + 1, pw));
        break;
    }
    }
}

/**
 * @brief Move the position of the scale
 *
 * The meaning of the parameter pos depends on the alignment:
 * - QwtScaleDraw::LeftScale: The origin is the topmost point of the
 *   backbone. The backbone is a vertical line.
 *   Scale marks and labels are drawn at the left of the backbone.
 * - QwtScaleDraw::RightScale: The origin is the topmost point of the
 *   backbone. The backbone is a vertical line.
 *   Scale marks and labels are drawn at the right of the backbone.
 * - QwtScaleDraw::TopScale: The origin is the leftmost point of the
 *   backbone. The backbone is a horizontal line.
 *   Scale marks and labels are drawn above the backbone.
 * - QwtScaleDraw::BottomScale: The origin is the leftmost point of the
 *   backbone. The backbone is a horizontal line.
 *   Scale marks and labels are drawn below the backbone.
 *
 * @param pos Origin of the scale
 *
 * @sa pos(), setLength()
 *
 */
void QwtScaleDraw::move(const QPointF& pos)
{
    QWT_D(d);
    d->pos = pos;
    updateMap();
}

/**
 * @brief Get origin of the scale
 * @return Origin of the scale
 * @sa move(), length()
 *
 */
QPointF QwtScaleDraw::pos() const
{
    QWT_DC(d);
    return d->pos;
}

/**
 * @brief Set the length of the backbone
 *
 * The length doesn't include the space needed for
 * overlapping labels.
 *
 * @param length Length of the backbone
 *
 * @sa move(), minLabelDist()
 *
 */
void QwtScaleDraw::setLength(double length)
{
    QWT_D(d);

    if (length >= 0 && length < 10)
        length = 10;

    // Negative lengths are used intentionally ( f.e. QwtPolarGrid for
    // left aligned scales ), so only clamp the range, but do not
    // turn them into positive values.
    if (length < 0 && length > -10)
        length = -10;

    d->len = length;
    updateMap();
}

/**
 * @brief Get the length of the backbone
 * @return the length of the backbone
 * @sa setLength(), pos()
 *
 */
double QwtScaleDraw::length() const
{
    QWT_DC(d);
    return d->len;
}

/**
 * @brief Draws the label for a major scale tick
 *
 * @param painter Painter
 * @param value Value
 *
 * @sa drawTick(), drawBackbone(), boundingLabelRect()
 *
 */
void QwtScaleDraw::drawLabel(QPainter* painter, double value) const
{
    QwtText lbl = tickLabel(painter->font(), value);
    if (lbl.isEmpty())
        return;

    QPointF pos = labelPosition(value);

    QSizeF labelSize = lbl.textSize(painter->font());

    const QTransform transform = labelTransformation(pos, labelSize);

    painter->save();
    painter->setWorldTransform(transform, true);

    lbl.draw(painter, QRect(QPoint(0, 0), labelSize.toSize()));

    painter->restore();
}

/**
 * @brief Find the bounding rectangle for the label
 *
 * The coordinates of the rectangle are absolute ( calculated from pos() )
 * in direction of the tick.
 *
 * @param font Font used for painting
 * @param value Value
 *
 * @return Bounding rectangle
 * @sa labelRect()
 *
 */
QRect QwtScaleDraw::boundingLabelRect(const QFont& font, double value) const
{
    QwtText lbl = tickLabel(font, value);
    if (lbl.isEmpty())
        return QRect();

    const QPointF pos = labelPosition(value);
    QSizeF labelSize  = lbl.textSize(font);

    const QTransform transform = labelTransformation(pos, labelSize);
    return transform.mapRect(QRect(QPoint(0, 0), labelSize.toSize()));
}

/**
 * @brief Calculate the transformation that is needed to paint a label
 *
 * Calculate the transformation that is needed to paint a label
 * depending on its alignment and rotation.
 *
 * @param pos Position where to paint the label
 * @param size Size of the label
 *
 * @return Transformation matrix
 * @sa setLabelAlignment(), setLabelRotation()
 *
 */
QTransform QwtScaleDraw::labelTransformation(const QPointF& pos, const QSizeF& size) const
{
    QTransform transform;
    transform.translate(pos.x(), pos.y());
    transform.rotate(labelRotation());

    int flags = labelAlignment();
    if (flags == 0) {
        switch (alignment()) {
        case RightScale: {
            if (flags == 0)
                flags = Qt::AlignRight | Qt::AlignVCenter;
            break;
        }
        case LeftScale: {
            if (flags == 0)
                flags = Qt::AlignLeft | Qt::AlignVCenter;
            break;
        }
        case BottomScale: {
            if (flags == 0)
                flags = Qt::AlignHCenter | Qt::AlignBottom;
            break;
        }
        case TopScale: {
            if (flags == 0)
                flags = Qt::AlignHCenter | Qt::AlignTop;
            break;
        }
        }
    }

    double x, y;

    if (flags & Qt::AlignLeft)
        x = -size.width();
    else if (flags & Qt::AlignRight)
        x = 0.0;
    else  // Qt::AlignHCenter
        x = -(0.5 * size.width());

    if (flags & Qt::AlignTop)
        y = -size.height();
    else if (flags & Qt::AlignBottom)
        y = 0;
    else  // Qt::AlignVCenter
        y = -(0.5 * size.height());

    transform.translate(x, y);

    return transform;
}

/**
 * @brief Find the bounding rectangle for the label
 *
 * The coordinates of the rectangle are relative to spacing + tick length
 * from the backbone in direction of the tick.
 *
 * @param font Font used for painting
 * @param value Value
 *
 * @return Bounding rectangle that is needed to draw a label
 *
 */
QRectF QwtScaleDraw::labelRect(const QFont& font, double value) const
{
    QwtText lbl = tickLabel(font, value);
    if (lbl.isEmpty())
        return QRectF(0.0, 0.0, 0.0, 0.0);

    const QPointF pos = labelPosition(value);

    const QSizeF labelSize     = lbl.textSize(font);
    const QTransform transform = labelTransformation(pos, labelSize);

    QRectF br = transform.mapRect(QRectF(QPointF(0, 0), labelSize));
    br.translate(-pos.x(), -pos.y());

    return br;
}

/**
 * @brief Calculate the size that is needed to draw a label
 *
 * @param font Label font
 * @param value Value
 *
 * @return Size that is needed to draw a label
 *
 */
QSizeF QwtScaleDraw::labelSize(const QFont& font, double value) const
{
    return labelRect(font, value).size();
}

/**
 * @brief Rotate all labels
 *
 * When changing the rotation, it might be necessary to
 * adjust the label flags too. Finding a useful combination is
 * often the result of try and error.
 *
 * @param rotation Angle in degrees. When changing the label rotation,
 *                 the label flags often needs to be adjusted too.
 *
 * @sa setLabelAlignment(), labelRotation(), labelAlignment()
 *
 */
void QwtScaleDraw::setLabelRotation(double rotation)
{
    QWT_D(d);
    d->labelRotation = rotation;
}

/**
 * @brief Get the label rotation
 * @return the label rotation
 * @sa setLabelRotation(), labelAlignment()
 *
 */
double QwtScaleDraw::labelRotation() const
{
    QWT_DC(d);
    return d->labelRotation;
}

/**
 * @brief Change the label flags
 *
 * Labels are aligned to the point tick length + spacing away from the backbone.
 *
 * The alignment is relative to the orientation of the label text.
 * In case of an flags of 0 the label will be aligned
 * depending on the orientation of the scale:
 *
 * - QwtScaleDraw::TopScale: Qt::AlignHCenter | Qt::AlignTop
 * - QwtScaleDraw::BottomScale: Qt::AlignHCenter | Qt::AlignBottom
 * - QwtScaleDraw::LeftScale: Qt::AlignLeft | Qt::AlignVCenter
 * - QwtScaleDraw::RightScale: Qt::AlignRight | Qt::AlignVCenter
 *
 * Changing the alignment is often necessary for rotated labels.
 *
 * @param alignment Or'd Qt::AlignmentFlags see <qnamespace.h>
 *
 * @sa setLabelRotation(), labelRotation(), labelAlignment()
 * @warning The various alignments might be confusing.
 *          The alignment of the label is not the alignment
 *          of the scale and is not the alignment of the flags
 *          ( QwtText::flags() ) returned from QwtAbstractScaleDraw::label().
 *
 */

void QwtScaleDraw::setLabelAlignment(Qt::Alignment alignment)
{
    QWT_D(d);
    d->labelAlignment = alignment;
}

/**
 * @brief Get the label flags
 * @return the label flags
 * @sa setLabelAlignment(), labelRotation()
 *
 */
Qt::Alignment QwtScaleDraw::labelAlignment() const
{
    QWT_DC(d);
    return d->labelAlignment;
}

/**
 * @brief Get the maximum width of a label
 * @param font Font
 * @return the maximum width of a label
 *
 */
int QwtScaleDraw::maxLabelWidth(const QFont& font) const
{
    double maxWidth = 0.0;

    const QList< double >& ticks = scaleDiv().ticks(QwtScaleDiv::MajorTick);
    for (int i = 0; i < ticks.count(); i++) {
        const double v = ticks[ i ];
        if (scaleDiv().contains(v)) {
            const double w = labelSize(font, ticks[ i ]).width();
            if (w > maxWidth)
                maxWidth = w;
        }
    }

    return qCeil(maxWidth);
}

/**
 * @brief Get the maximum height of a label
 * @param font Font
 * @return the maximum height of a label
 *
 */
int QwtScaleDraw::maxLabelHeight(const QFont& font) const
{
    double maxHeight = 0.0;

    const QList< double >& ticks = scaleDiv().ticks(QwtScaleDiv::MajorTick);
    for (int i = 0; i < ticks.count(); i++) {
        const double v = ticks[ i ];
        if (scaleDiv().contains(v)) {
            const double h = labelSize(font, ticks[ i ]).height();
            if (h > maxHeight)
                maxHeight = h;
        }
    }

    return qCeil(maxHeight);
}

/**
 * @brief Update the scale map
 *
 */
void QwtScaleDraw::updateMap()
{
    QWT_D(d);
    const QPointF pos = d->pos;
    double len        = d->len;

    QwtScaleMap& sm = scaleMap();
    if (orientation() == Qt::Vertical)
        sm.setPaintInterval(pos.y() + len, pos.y());
    else
        sm.setPaintInterval(pos.x(), pos.x() + len);
}
