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

#ifndef QWT_POINT_MAPPER_H
#define QWT_POINT_MAPPER_H

#include "qwt_global.h"

class QwtScaleMap;
template< typename T >
class QwtSeriesData;
class QPolygonF;
class QPointF;
class QRectF;
class QPolygon;
class QPen;
class QImage;

/**
 * @brief A helper class for translating a series of points
 *
 * @details QwtPointMapper is a collection of methods and optimizations
 *          for translating a series of points into paint device coordinates.
 *          It is used by QwtPlotCurve but might also be useful for
 *          similar plot items displaying a QwtSeriesData<QPointF>.
 *
 */
class QWT_EXPORT QwtPointMapper
{
public:
    /**
     * @brief Flags affecting the transformation process
     * @sa setFlag(), setFlags()
     *
     */
    enum TransformationFlag
    {
        //! Round points to the device pixel grid ( see devicePixelRatio() )
        RoundPoints = 0x01,

        //! Try to remove points, that are translated to the same position
        WeedOutPoints = 0x02,

        /**
         * @brief An even more aggressive weeding algorithm
         *
         * @details An even more aggressive weeding algorithm, that can be used in toPolygon().
         *          A consecutive chunk of points being mapped to the same x coordinate is reduced to 4 points:
         *          - first point
         *          - point with the minimum y coordinate
         *          - point with the maximum y coordinate
         *          - last point
         *
         *          In the worst case (first and last points are never one of the extremes)
         *          the number of points will be 4 times the width.
         *          As the algorithm is fast it can be used inside of a polyline render cycle.
         *
         */
        WeedOutIntermediatePoints = 0x04,

        /**
         * @brief Pixel-column based downsampling
         *
         * @details Allocates a bin array indexed by device pixel column
         *          ( see devicePixelRatio() ) and stores first/min/max/last Y
         *          per column. Output is at most 4 points per column.
         *          Requires a valid boundingRect() to determine the canvas width.
         *          Overrides WeedOutIntermediatePoints when both are set.
         */
        PixelColumnReduce = 0x08,

        /**
         * @brief Largest-Triangle-Three-Buckets (LTTB) downsampling
         *
         * @details Divides the visible data into equal-count buckets and keeps
         *          the sample of each bucket, that forms the largest triangle
         *          with the previously selected point and the average of the
         *          next bucket. The first and last sample are always kept.
         *          Target bucket count is 4 * boundingRect() width in device
         *          pixels ( see devicePixelRatio() ), the output is at most
         *          one point per bucket.
         *          Overrides WeedOutIntermediatePoints when both are set.
         */
        LTTBReduce = 0x20,

        /**
         * @brief MinMax bucket downsampling
         *
         * @details Divides the visible data into equal-count buckets and keeps
         *          the min-Y and max-Y point from each bucket. Target bucket count
         *          is derived from boundingRect() width in device pixels
         *          ( see devicePixelRatio() ).
         *          Overrides WeedOutIntermediatePoints when both are set.
         */
        MinMaxReduce = 0x10,

        /**
         * @brief Adaptive column reduce downsampling
         *
         * @details Single pass O(n) reduction combining a density gate with an
         *          ordered vertical tolerance selection, all pixel quantities
         *          in device pixels ( see devicePixelRatio() ):
         *
         *          - When the visible samples are not denser than 2 points per
         *            device pixel column the data is mapped 1:1 without any
         *            reduction, so zooming in always restores full fidelity.
         *          - Otherwise the samples are clustered by device pixel column.
         *            Inside a cluster the first sample, every sample whose mapped
         *            y differs by at least 1 device pixel from the last kept
         *            sample and the last sample are kept - in their original
         *            order and at their real sub pixel coordinates. In contrast
         *            to the min/max envelope per column ( PixelColumnReduce )
         *            ramps stay ramps instead of degenerating into a 1px
         *            staircase of vertical strokes and horizontal steps.
         *          - When this selection of a cluster would emit more than
         *            K = 4 + ( cluster y span in device pixels ) points
         *            ( high frequency noise inside one column ) the cluster
         *            falls back to an ordered first/min/max/last quadruple,
         *            so the output per column is bounded by K + 1 points.
         *
         *          The deviation from the original curve is bounded by 1 device
         *          pixel vertically and one column horizontally, visible
         *          features of at least 1 device pixel ( including in-column
         *          extrema and spikes ) are kept in sample order. For smooth
         *          data the output size is roughly the vertical pixel travel.
         *
         *          Overrides all other reduction flags when set.
         */
        AdaptiveColumnReduce = 0x40
    };

    Q_DECLARE_FLAGS(TransformationFlags, TransformationFlag)

    // Constructor
    QwtPointMapper();
    // Destructor
    ~QwtPointMapper();

    // Set the transformation flags
    void setFlags(TransformationFlags);
    // Get the transformation flags
    TransformationFlags flags() const;

    // Set or clear a transformation flag
    void setFlag(TransformationFlag, bool on = true);
    // Test if a transformation flag is set
    bool testFlag(TransformationFlag) const;

    // Set the bounding rectangle for mapping
    void setBoundingRect(const QRectF&);
    // Get the bounding rectangle
    QRectF boundingRect() const;

    // Set the device pixel ratio of the target paint device
    void setDevicePixelRatio(qreal ratio);
    // Get the device pixel ratio of the target paint device
    qreal devicePixelRatio() const;

    // Translate a series of points into a QPolygonF
    QPolygonF
    toPolygonF(const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QwtSeriesData< QPointF >* series, int from, int to) const;

    // Translate a series of points into a QPolygon
    QPolygon
    toPolygon(const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QwtSeriesData< QPointF >* series, int from, int to) const;

    // Translate a series of points into a QPolygon (scattered points)
    QPolygon
    toPoints(const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QwtSeriesData< QPointF >* series, int from, int to) const;

    // Translate a series of points into a QPolygonF (scattered points)
    QPolygonF
    toPointsF(const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QwtSeriesData< QPointF >* series, int from, int to) const;

    // Translate a series into a QImage
    QImage toImage(const QwtScaleMap& xMap,
                   const QwtScaleMap& yMap,
                   const QwtSeriesData< QPointF >* series,
                   int from,
                   int to,
                   const QPen&,
                   bool antialiased,
                   uint numThreads) const;

private:
    QwtPointMapper(const QwtPointMapper&)            = delete;
    QwtPointMapper& operator=(const QwtPointMapper&) = delete;

    QWT_DECLARE_PRIVATE(QwtPointMapper)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPointMapper::TransformationFlags)

#endif
