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

#ifndef QWT_SCALE_DRAW_H
#define QWT_SCALE_DRAW_H

#include "qwt_global.h"
#include "qwt_abstract_scale_draw.h"

#include <qpoint.h>

class QTransform;
class QSizeF;
class QRectF;
class QRect;

/**
 * @brief A class for drawing scales
 * @details QwtScaleDraw can be used to draw linear or logarithmic scales.
 *          A scale has a position, an alignment and a length, which can be specified.
 *          The labels can be rotated and aligned to the ticks using setLabelRotation()
 *          and setLabelAlignment().
 *
 *          After a scale division has been specified as a QwtScaleDiv object
 *          using QwtAbstractScaleDraw::setScaleDiv(const QwtScaleDiv &s),
 *          the scale can be drawn with the QwtAbstractScaleDraw::draw() member.
 */
class QWT_EXPORT QwtScaleDraw : public QwtAbstractScaleDraw
{
public:
    /**
     * @brief Alignment of the scale draw
     * @sa setAlignment(), alignment()
     */
    enum Alignment
    {
        /// The scale is below
        BottomScale,

        /// The scale is above
        TopScale,

        /// The scale is left
        LeftScale,

        /// The scale is right
        RightScale
    };

    QwtScaleDraw();
    ~QwtScaleDraw() override;

    /**
     * @brief Corner miter flags
     * @details When a miter flag is set, the backbone is extended beyond the
     *          corresponding end of the scale into a corner shared with a
     *          perpendicular scale, so that both backbones intersect instead
     *          of ending in a diagonal step. The extension reaches the near
     *          edge of the perpendicular backbone band closest to the canvas
     *          when a target has been set with setCornerTargets(), otherwise
     *          it falls back to one pixel beyond the end of the scale.
     *          For vertical scales AtStart/AtEnd refer to the top/bottom end,
     *          for horizontal scales to the left/right end.
     * @sa setCornerMiter(), cornerMiter(), setCornerTargets()
     */
    enum CornerMiterFlag
    {
        /// Do not extend the backbone
        NoMiter = 0x0,

        /// Extend the backbone beyond the start of the scale
        MiterAtStart = 0x1,

        /// Extend the backbone beyond the end of the scale
        MiterAtEnd = 0x2
    };

    Q_DECLARE_FLAGS(CornerMiter, CornerMiterFlag)

    /// Set the corner miter flags
    void setCornerMiter(CornerMiter miter);
    /// @return the corner miter flags
    CornerMiter cornerMiter() const;

    /**
     * @brief Corner miter target positions in device coordinates
     * @details Per scale end the target is the device coordinate ( a row for
     *          vertical scales, a column for horizontal scales ) of the near
     *          edge of the perpendicular backbone band: the band closest to
     *          the canvas, where a border tick sits. Bands stacked farther
     *          out ( parasite axes ) are not crossed. qQNaN() keeps the
     *          historical fallback of one pixel beyond the end of the scale.
     * @sa setCornerTargets(), cornerTargets()
     */
    struct CornerTargets
    {
        /// Target at the start of the scale
        qreal start = qQNaN();
        /// Target at the end of the scale
        qreal end = qQNaN();
    };

    /// Set the corner miter target positions (device coordinates)
    void setCornerTargets(const CornerTargets& targets);
    /// @return the corner miter target positions
    CornerTargets cornerTargets() const;

    /// Get the border distance hint
    void getBorderDistHint(const QFont&, int& start, int& end) const;
    /// Get the minimum label distance
    int minLabelDist(const QFont&) const;

    /// Get the minimum length
    int minLength(const QFont&) const;
    virtual double extent(const QFont&) const override;

    void move(double x, double y);
    void move(const QPointF&);
    void setLength(double length);

    /// @return the alignment
    Alignment alignment() const;
    /// Set the alignment
    void setAlignment(Alignment);

    /// @return the orientation
    Qt::Orientation orientation() const;

    /// @return the position
    QPointF pos() const;
    /// @return the length
    double length() const;

    /// Set the label alignment
    void setLabelAlignment(Qt::Alignment);
    /// @return the label alignment
    Qt::Alignment labelAlignment() const;

    /// Set the label rotation
    void setLabelRotation(double rotation);
    /// @return the label rotation
    double labelRotation() const;

    /// Get the maximum label height
    int maxLabelHeight(const QFont&) const;
    /// Get the maximum label width
    int maxLabelWidth(const QFont&) const;

    /// Get the label position for a value
    QPointF labelPosition(double value) const;

    /// Get the label rectangle for a value
    QRectF labelRect(const QFont&, double value) const;
    /// Get the label size for a value
    QSizeF labelSize(const QFont&, double value) const;

    /// Get the bounding label rectangle
    QRect boundingLabelRect(const QFont&, double value) const;

protected:
    QTransform labelTransformation(const QPointF&, const QSizeF&) const;

    virtual void drawTick(QPainter*, double value, double len) const override;

    virtual void drawBackbone(QPainter*) const override;
    virtual void drawLabel(QPainter*, double value) const override;

private:
    void updateMap();

    QWT_DECLARE_PRIVATE(QwtScaleDraw)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtScaleDraw::CornerMiter)

/**
 * @brief Move the position of the scale
 * @param x X coordinate
 * @param y Y coordinate
 * @sa move(const QPointF&)
 */
inline void QwtScaleDraw::move(double x, double y)
{
    move(QPointF(x, y));
}

#endif
