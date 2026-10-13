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

#include "qwt_abstract_scale_draw.h"
#include "qwt_text.h"
#include "qwt_painter.h"
#include "qwt_scale_map.h"
#include "qwt_math.h"

#include <qpainter.h>
#include <qpalette.h>
#include <qmap.h>
#include <qlist.h>
#include <qlocale.h>
#include <qset.h>

class QwtAbstractScaleDraw::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtAbstractScaleDraw)
public:
    PrivateData(QwtAbstractScaleDraw* p) : q_ptr(p), spacing(4.0), penWidthF(0.0), minExtent(0.0)
    {
        components = QwtAbstractScaleDraw::Backbone | QwtAbstractScaleDraw::Ticks | QwtAbstractScaleDraw::Labels;

        tickLength[ QwtScaleDiv::MinorTick ]  = 4.0;
        tickLength[ QwtScaleDiv::MediumTick ] = 6.0;
        tickLength[ QwtScaleDiv::MajorTick ]  = 8.0;
    }

    ScaleComponents components;

    QwtScaleMap map;
    QwtScaleDiv scaleDiv;

    double spacing;
    double tickLength[ QwtScaleDiv::NTickTypes ];
    qreal penWidthF { 0.0 };
    qreal penWidthOffset { 1.0 };

    bool isSelected { false };

    double minExtent;

    mutable QMap< double, QwtText > labelCache;
};

/**
 * @brief Constructor for QwtAbstractScaleDraw
 * @details The range of the scale is initialized to [0, 100],
 *          the spacing (distance between ticks and labels) is set to 4,
 *          the tick lengths are set to 4, 6 and 8 pixels.
 */
QwtAbstractScaleDraw::QwtAbstractScaleDraw() : QWT_PIMPL_CONSTRUCT
{
}

/**
 * @brief Destructor for QwtAbstractScaleDraw
 */
QwtAbstractScaleDraw::~QwtAbstractScaleDraw()
{
}

/**
 * @brief Enable or disable a scale component
 * @param component Scale component (Backbone, Ticks, or Labels)
 * @param enable True to enable, false to disable
 * @sa hasComponent()
 */
void QwtAbstractScaleDraw::enableComponent(ScaleComponent component, bool enable)
{
    QWT_D(d);
    if (enable)
        d->components |= component;
    else
        d->components &= ~component;
}

/**
 * @brief Check if a component is enabled
 * @param component Component type
 * @return True if component is enabled
 * @sa enableComponent()
 */
bool QwtAbstractScaleDraw::hasComponent(ScaleComponent component) const
{
    QWT_DC(d);
    return (d->components & component);
}

/**
 * @brief Set the scale division
 * @param scaleDiv New scale division object
 * @details This also updates the scale map and clears the label cache.
 */
void QwtAbstractScaleDraw::setScaleDiv(const QwtScaleDiv& scaleDiv)
{
    QWT_D(d);
    d->scaleDiv = scaleDiv;
    d->map.setScaleInterval(scaleDiv.lowerBound(), scaleDiv.upperBound());
    d->labelCache.clear();
}

/**
 * @brief Set the scale transformation
 * @param transformation New scale transformation object
 */
void QwtAbstractScaleDraw::setTransformation(QwtTransform* transformation)
{
    QWT_D(d);
    d->map.setTransformation(transformation);
}

/**
 * @brief Return the scale map (const version)
 * @return Map for translating between scale and pixel values
 */
const QwtScaleMap& QwtAbstractScaleDraw::scaleMap() const
{
    QWT_DC(d);
    return d->map;
}

/**
 * @brief Return the scale map (non-const version)
 * @return Map for translating between scale and pixel values
 */
QwtScaleMap& QwtAbstractScaleDraw::scaleMap()
{
    QWT_D(d);
    return d->map;
}

/**
 * @brief Return the scale division
 * @return Current scale division object
 */
const QwtScaleDiv& QwtAbstractScaleDraw::scaleDiv() const
{
    QWT_DC(d);
    return d->scaleDiv;
}

/**
 * @brief Specify the width of the scale pen
 * @param width Pen width
 * @sa penWidth()
 */
void QwtAbstractScaleDraw::setPenWidthF(qreal width)
{
    QWT_D(d);
    if (width < 0.0)
        width = 0.0;

    d->penWidthF = width;
}

/**
 * @brief Get the scale pen width
 * @return Scale pen width
 * @sa setPenWidth()
 */
qreal QwtAbstractScaleDraw::penWidthF() const
{
    QWT_DC(d);
    return d->penWidthF;
}

/**
 * @brief Set whether the scale draw is selected
 * @param on True to select, false to deselect
 */
void QwtAbstractScaleDraw::setSelected(bool on)
{
    QWT_D(d);
    d->isSelected = on;
}

/**
 * @brief Check if the scale draw is selected
 * @return True if selected, false otherwise
 */
bool QwtAbstractScaleDraw::isSelected() const
{
    QWT_DC(d);
    return d->isSelected;
}

/**
 * @brief Set the pen width offset for the axis when it is in selected state
 * @details When an axis (e.g., X-axis or Y-axis) is selected by the user, the pen width used
 *          for drawing will be increased by this offset value, achieving a visual highlighting effect.
 * @param offset The additional width value to be added when selected (unit: pixels).
 *               This value should be non-negative. If it is 0, the line width in selected state
 *               will be the same as in normal state.
 * @sa selectedPenWidthOffset()
 */
void QwtAbstractScaleDraw::setSelectedPenWidthOffset(qreal offset)
{
    QWT_D(d);
    d->penWidthOffset = offset;
}

/**
 * @brief Get the current pen width offset for the axis when it is in selected state
 * @return The current width offset value
 * @sa setSelectedPenWidthOffset()
 */
qreal QwtAbstractScaleDraw::selectedPenWidthOffset() const
{
    QWT_DC(d);
    return d->penWidthOffset;
}

/**
 * @brief Draw the scale
 * @param painter The painter
 * @param palette Palette, text color is used for the labels, foreground color for ticks and backbone
 */
void QwtAbstractScaleDraw::draw(QPainter* painter, const QPalette& palette) const
{
    QWT_DC(d);
    painter->save();

    QPen pen = painter->pen();
    pen.setWidthF(d->penWidthF);
    if (isSelected()) {
        if (qFuzzyIsNull(d->penWidthF)) {
            pen.setWidthF(1.0 + d->penWidthOffset);
        } else {
            pen.setWidthF(d->penWidthF + d->penWidthOffset);
        }
    }
    painter->setPen(pen);

    if (hasComponent(QwtAbstractScaleDraw::Labels)) {
        painter->save();
        painter->setPen(palette.color(QPalette::Text));  // ignore pen style

        const QList< double >& majorTicks = d->scaleDiv.ticks(QwtScaleDiv::MajorTick);

        for (int i = 0; i < majorTicks.count(); i++) {
            const double v = majorTicks[ i ];
            if (d->scaleDiv.contains(v))
                drawLabel(painter, v);
        }

        painter->restore();
    }

    if (hasComponent(QwtAbstractScaleDraw::Ticks)) {
        painter->save();

        pen = painter->pen();
        pen.setColor(palette.color(QPalette::WindowText));
        pen.setCapStyle(Qt::FlatCap);

        painter->setPen(pen);

        // The tick lists of different types can contain the same value: the
        // date engine collapses rounded minor ticks onto major ticks, for
        // example. Paint every position only once - a tick that sits on a
        // tick of a longer type is skipped, so the position keeps the length
        // of the longest type.
        QSet< double > mediumAndMajorValues;
        QSet< double > majorValues;

        for (int tickType = QwtScaleDiv::MediumTick; tickType <= QwtScaleDiv::MajorTick; tickType++) {
            const QList< double >& ticks = d->scaleDiv.ticks(tickType);
            for (int i = 0; i < ticks.count(); i++) {
                if (d->scaleDiv.contains(ticks[ i ])) {
                    mediumAndMajorValues.insert(ticks[ i ]);
                    if (tickType == QwtScaleDiv::MajorTick)
                        majorValues.insert(ticks[ i ]);
                }
            }
        }

        for (int tickType = QwtScaleDiv::MinorTick; tickType < QwtScaleDiv::NTickTypes; tickType++) {
            const double tickLen = d->tickLength[ tickType ];
            if (tickLen <= 0.0)
                continue;

            const QSet< double >& covered =
                (tickType == QwtScaleDiv::MinorTick) ? mediumAndMajorValues : majorValues;

            const QList< double >& ticks = d->scaleDiv.ticks(tickType);
            for (int i = 0; i < ticks.count(); i++) {
                const double v = ticks[ i ];
                if (!d->scaleDiv.contains(v))
                    continue;
                if (tickType != QwtScaleDiv::MajorTick && covered.contains(v))
                    continue;  // a longer tick is drawn at this position

                drawTick(painter, v, tickLen);
            }
        }

        painter->restore();
    }

    if (hasComponent(QwtAbstractScaleDraw::Backbone)) {
        painter->save();

        pen = painter->pen();
        pen.setColor(palette.color(QPalette::WindowText));
        pen.setCapStyle(Qt::FlatCap);

        painter->setPen(pen);

        drawBackbone(painter);

        painter->restore();
    }

    painter->restore();
}

/**
 * @brief Set the spacing between tick and labels
 * @details The spacing is the distance between ticks and labels. The default spacing is 4 pixels.
 * @param spacing Spacing
 * @sa spacing()
 */
void QwtAbstractScaleDraw::setSpacing(double spacing)
{
    QWT_D(d);
    if (spacing < 0)
        spacing = 0;

    d->spacing = spacing;
}

/**
 * @brief Get the spacing
 * @details The spacing is the distance between ticks and labels. The default spacing is 4 pixels.
 * @return Spacing
 * @sa setSpacing()
 */
double QwtAbstractScaleDraw::spacing() const
{
    QWT_DC(d);
    return d->spacing;
}

/**
 * @brief Set a minimum for the extent
 * @details The extent is calculated from the components of the scale draw.
 *          In situations where the labels are changing and the layout depends on the extent
 *          (e.g., scrolling a scale), setting an upper limit as minimum extent will avoid jumps of the layout.
 * @param minExtent Minimum extent
 * @sa extent(), minimumExtent()
 */
void QwtAbstractScaleDraw::setMinimumExtent(double minExtent)
{
    QWT_D(d);
    if (minExtent < 0.0)
        minExtent = 0.0;

    d->minExtent = minExtent;
}

/**
 * @brief Get the minimum extent
 * @return Minimum extent
 * @sa extent(), setMinimumExtent()
 */
double QwtAbstractScaleDraw::minimumExtent() const
{
    QWT_DC(d);
    return d->minExtent;
}

/**
 * @brief Set the length of the ticks
 * @param tickType Tick type
 * @param length New length
 * @warning The length is limited to [0..1000]
 */
void QwtAbstractScaleDraw::setTickLength(QwtScaleDiv::TickType tickType, double length)
{
    QWT_D(d);
    if (tickType < QwtScaleDiv::MinorTick || tickType > QwtScaleDiv::MajorTick) {
        return;
    }

    if (length < 0.0)
        length = 0.0;

    const double maxTickLen = 1000.0;
    if (length > maxTickLen)
        length = maxTickLen;

    d->tickLength[ tickType ] = length;
}

/**
 * @brief Get the length of the ticks
 * @return Length of the ticks
 * @sa setTickLength(), maxTickLength()
 */
double QwtAbstractScaleDraw::tickLength(QwtScaleDiv::TickType tickType) const
{
    QWT_DC(d);
    if (tickType < QwtScaleDiv::MinorTick || tickType > QwtScaleDiv::MajorTick) {
        return 0;
    }

    return d->tickLength[ tickType ];
}

/**
 * @brief Get the length of the longest tick
 * @return Length of the longest tick
 * @details Useful for layout calculations
 * @sa tickLength(), setTickLength()
 */
double QwtAbstractScaleDraw::maxTickLength() const
{
    QWT_DC(d);
    double length = 0.0;
    for (int i = 0; i < QwtScaleDiv::NTickTypes; i++)
        length = qwtMaxF(length, d->tickLength[ i ]);

    return length;
}

/**
 * @brief Convert a value into its representing label
 * @details The value is converted to a plain text using QLocale().toString(value).
 *          This method is often overloaded by applications to have individual labels.
 * @param value Value
 * @return Label string
 */
QwtText QwtAbstractScaleDraw::label(double value) const
{
    return QLocale().toString(value);
}

/**
 * @brief Convert a value into its representing label and cache it
 * @details The conversion between value and label is called very often in the layout and painting code.
 *          Unfortunately the calculation of the label sizes might be slow (really slow for rich text in Qt4),
 *          so it's necessary to cache the labels.
 * @param font Font
 * @param value Value
 * @return Tick label
 */
const QwtText& QwtAbstractScaleDraw::tickLabel(const QFont& font, double value) const
{
    QWT_DC(d);
    auto it1 = d->labelCache.constFind(value);
    if (it1 != d->labelCache.constEnd())
        return *it1;

    QwtText lbl = label(value);
    lbl.setRenderFlags(0);
    lbl.setLayoutAttribute(QwtText::MinimumLayout);

    (void)lbl.textSize(font);  // initialize the internal cache

    auto it2 = d->labelCache.insert(value, lbl);
    return *it2;
}

/**
 * @brief Invalidate the cache used by tickLabel()
 * @details The cache is invalidated when a new QwtScaleDiv is set.
 *          If the labels need to be changed while the same QwtScaleDiv is set,
 *          invalidateCache() needs to be called manually.
 */
void QwtAbstractScaleDraw::invalidateCache()
{
    QWT_D(d);
    d->labelCache.clear();
}
