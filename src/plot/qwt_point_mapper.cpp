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

#include "qwt_point_mapper.h"
#include "qwt_scale_map.h"
#include "qwt_pixel_matrix.h"
#include "qwt_series_data.h"
#include "qwt_point_data.h"
#include "qwt_math.h"
#include "qwt_simd_argminmax.h"

#include <qpolygon.h>
#include <qimage.h>
#include <qpen.h>
#include <qpainter.h>

#include <qthread.h>
#include <qfuture.h>
#include <qtconcurrentrun.h>

#if !defined(QT_NO_QFUTURE)
#define QWT_USE_THREADS 1
#endif

static QRectF qwtInvalidRect(0.0, 0.0, -1.0, -1.0);

static inline int qwtRoundValue(double value)
{
    return qRound(value);
}

static inline double qwtRoundValueF(double value)
{
#if 1
    // MS Windows and at least IRIX does not have C99's nearbyint() function
    return (value >= 0.0) ? std::floor(value + 0.5) : std::ceil(value - 0.5);
#else
    return nearbyint(value);
#endif
}

// Create a point of the output polygon from pixel positions,
// that may have fractional coordinates
template< class Point >
static Point qwtMakePointD(double x, double y);

template<>
inline QPointF qwtMakePointD< QPointF >(double x, double y)
{
    return QPointF(x, y);
}

template<>
inline QPoint qwtMakePointD< QPoint >(double x, double y)
{
    return QPoint(qRound(x), qRound(y));
}

static Qt::Orientation qwtProbeOrientation(const QwtSeriesData< QPointF >* series, int from, int to)
{
    if (to - from < 20) {
        // not enough points to "have an orientation"
        return Qt::Horizontal;
    }

    const double x0 = series->sample(from).x();
    const double xn = series->sample(to).x();

    if (x0 == xn)
        return Qt::Vertical;

    const int step          = qMax(1, (to - from) / 50);
    const bool isIncreasing = xn > x0;

    double x1 = x0;
    for (int i = from + step; i < to; i += step) {
        const double x2 = series->sample(i).x();
        if (x2 != x1) {
            if ((x2 > x1) != isIncreasing)
                return Qt::Vertical;
        }

        x1 = x2;
    }

    return Qt::Horizontal;
}

namespace
{
template< class Polygon, class Point >
class QwtPolygonQuadrupelX
{
public:
    inline void start(int x, int y)
    {
        x0 = x;
        y1 = yMin = yMax = y2 = y;
    }

    inline bool append(int x, int y)
    {
        if (x0 != x)
            return false;

        if (y < yMin)
            yMin = y;
        else if (y > yMax)
            yMax = y;

        y2 = y;

        return true;
    }

    inline void flush(Polygon& polyline)
    {
        appendTo(y1, polyline);

        if (y2 > y1)
            qSwap(yMin, yMax);

        if (yMax != y1)
            appendTo(yMax, polyline);

        if (yMin != yMax)
            appendTo(yMin, polyline);

        if (y2 != yMin)
            appendTo(y2, polyline);
    }

    // emit the collected device pixel coordinates scaled back to paint coordinates
    inline void setScale(qreal dpr)
    {
        m_inv = 1.0 / dpr;
    }

private:
    inline void appendTo(int y, Polygon& polyline)
    {
        polyline += qwtMakePointD< Point >(x0 * m_inv, y * m_inv);
    }

private:
    int x0, y1, yMin, yMax, y2;
    double m_inv = 1.0;
};

template< class Polygon, class Point >
class QwtPolygonQuadrupelY
{
public:
    inline void start(int x, int y)
    {
        y0 = y;
        x1 = xMin = xMax = x2 = x;
    }

    inline bool append(int x, int y)
    {
        if (y0 != y)
            return false;

        if (x < xMin)
            xMin = x;
        else if (x > xMax)
            xMax = x;

        x2 = x;

        return true;
    }

    inline void flush(Polygon& polyline)
    {
        appendTo(x1, polyline);

        if (x2 > x1)
            qSwap(xMin, xMax);

        if (xMax != x1)
            appendTo(xMax, polyline);

        if (xMin != xMax)
            appendTo(xMin, polyline);

        if (x2 != xMin)
            appendTo(x2, polyline);
    }

    // emit the collected device pixel coordinates scaled back to paint coordinates
    inline void setScale(qreal dpr)
    {
        m_inv = 1.0 / dpr;
    }

private:
    inline void appendTo(int x, Polygon& polyline)
    {
        polyline += qwtMakePointD< Point >(x * m_inv, y0 * m_inv);
    }

    int y0, x1, xMin, xMax, x2;
    double m_inv = 1.0;
};
}

template< class Polygon, class Point, class PolygonQuadrupel >
static Polygon qwtMapPointsQuad(const QwtScaleMap& xMap,
                                const QwtScaleMap& yMap,
                                const QwtSeriesData< QPointF >* series,
                                int from,
                                int to,
                                int reserveSize = 0,
                                qreal dpr       = 1.0)
{
    Polygon polyline;

    // find first non-NaN sample
    int realFrom    = from;
    QPointF sample0 = series->sample(from);
    while (realFrom < to && qwt_is_nan_or_inf(sample0)) {
        realFrom++;
        sample0 = series->sample(realFrom);
    }
    if (realFrom >= to && qwt_is_nan_or_inf(sample0))
        return polyline;

    if (reserveSize > 0)
        polyline.reserve(reserveSize);

    PolygonQuadrupel q;
    q.start(qwtRoundValue(xMap.transform(sample0.x()) * dpr), qwtRoundValue(yMap.transform(sample0.y()) * dpr));

    // linear scale fast path: avoid virtual transform() call per point
    const bool linearPath = xMap.isLinear() && yMap.isLinear();
    if (linearPath) {
        const double xCnv = xMap.cnv(), xOff = xMap.p1() - xMap.ts1() * xCnv;
        const double yCnv = yMap.cnv(), yOff = yMap.p1() - yMap.ts1() * yCnv;

        for (int i = realFrom; i <= to; i++) {
            const QPointF sample = series->sample(i);
            if (qwt_is_nan_or_inf(sample))
                continue;
            const int x = qRound((sample.x() * xCnv + xOff) * dpr);
            const int y = qRound((sample.y() * yCnv + yOff) * dpr);

            if (!q.append(x, y)) {
                q.flush(polyline);
                q.start(x, y);
            }
        }
    } else {
        for (int i = realFrom; i <= to; i++) {
            const QPointF sample = series->sample(i);
            if (qwt_is_nan_or_inf(sample))
                continue;
            const int x = qwtRoundValue(xMap.transform(sample.x()) * dpr);
            const int y = qwtRoundValue(yMap.transform(sample.y()) * dpr);

            if (!q.append(x, y)) {
                q.flush(polyline);
                q.start(x, y);
            }
        }
    }
    q.flush(polyline);

    return polyline;
}

template< class Polygon, class Point, class PolygonQuadrupel >
static Polygon qwtMapPointsQuad(const Polygon& polyline, int reserveSize = 0, qreal dpr = 1.0)
{
    const int numPoints = polyline.size();

    if (numPoints < 3)
        return polyline;

    const Point* points = polyline.constData();

    Polygon polylineXY;
    if (reserveSize > 0)
        polylineXY.reserve(reserveSize);

    PolygonQuadrupel q;
    q.setScale(dpr);
    q.start(points[ 0 ].x(), points[ 0 ].y());

    for (int i = 0; i < numPoints; i++) {
        const int x = points[ i ].x();
        const int y = points[ i ].y();

        if (!q.append(x, y)) {
            q.flush(polylineXY);
            q.start(x, y);
        }
    }
    q.flush(polylineXY);

    return polylineXY;
}

template< class Polygon, class Point >
static Polygon
qwtMapPointsQuad(const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QwtSeriesData< QPointF >* series, int from, int to, qreal dpr)
{
    Polygon polyline;
    if (from > to)
        return polyline;

    // estimate reserve sizes: 4 points per pixel dimension
    const int xPixels  = qAbs(qRound((xMap.p2() - xMap.p1()) * dpr)) + 1;
    const int yPixels  = qAbs(qRound((yMap.p2() - yMap.p1()) * dpr)) + 1;
    const int reserve1 = 4 * qMax(xPixels, yPixels) + 16;
    const int reserve2 = 4 * qMin(xPixels, yPixels) + 16;

    /*
        probing some values, to decide if it is better
        to start with x or y coordinates
     */
    const Qt::Orientation orientation = qwtProbeOrientation(series, from, to);

    if (orientation == Qt::Horizontal) {
        polyline = qwtMapPointsQuad< Polygon, Point, QwtPolygonQuadrupelY< Polygon, Point > >(
            xMap, yMap, series, from, to, reserve1, dpr);

        polyline = qwtMapPointsQuad< Polygon, Point, QwtPolygonQuadrupelX< Polygon, Point > >(polyline, reserve2, dpr);
    } else {
        polyline = qwtMapPointsQuad< Polygon, Point, QwtPolygonQuadrupelX< Polygon, Point > >(
            xMap, yMap, series, from, to, reserve1, dpr);

        polyline = qwtMapPointsQuad< Polygon, Point, QwtPolygonQuadrupelY< Polygon, Point > >(polyline, reserve2, dpr);
    }

    return polyline;
}

// Binary search for visible range in monotonic X data with linear scale.
// Returns true if the range was successfully narrowed.
static bool
qwtFindVisibleRange(const QwtScaleMap& xMap, const QwtSeriesData< QPointF >* series, int from, int to, int& visFrom, int& visTo)
{
    if (!xMap.isLinear() || to - from < 20)
        return false;

    const double x0 = series->sample(from).x();
    const double xn = series->sample(to).x();
    if (x0 == xn)
        return false;

    const bool increasing = xn > x0;

    // quick monotonicity check by sampling (up to 50 probe points)
    const int step = qMax(1, (to - from) / 50);
    double prev    = x0;
    for (int i = from + step; i < to; i += step) {
        const double xi = series->sample(i).x();
        if ((xi > prev) != increasing && xi != prev)
            return false;
        prev = xi;
    }

    const double visMin = qMin(xMap.s1(), xMap.s2());
    const double visMax = qMax(xMap.s1(), xMap.s2());

    // binary search for first visible index
    int lo = from, hi = to;
    if (increasing) {
        while (lo < hi) {
            const int mid = lo + (hi - lo) / 2;
            if (series->sample(mid).x() < visMin)
                lo = mid + 1;
            else
                hi = mid;
        }
        visFrom = qMax(from, lo - 1);

        hi = to;
        while (lo < hi) {
            const int mid = lo + (hi - lo + 1) / 2;
            if (series->sample(mid).x() > visMax)
                hi = mid - 1;
            else
                lo = mid;
        }
        visTo = qMin(to, hi + 1);
    } else {
        while (lo < hi) {
            const int mid = lo + (hi - lo) / 2;
            if (series->sample(mid).x() > visMax)
                lo = mid + 1;
            else
                hi = mid;
        }
        visFrom = qMax(from, lo - 1);

        hi = to;
        while (lo < hi) {
            const int mid = lo + (hi - lo + 1) / 2;
            if (series->sample(mid).x() < visMin)
                hi = mid - 1;
            else
                lo = mid;
        }
        visTo = qMin(to, hi + 1);
    }

    return visFrom <= visTo;
}

// Pixel-column downsampling: bin points by screen X column, keep first/min/max/last Y.
// Output: at most 4 points per pixel column.
template< class Polygon, class Point >
static Polygon
qwtPixelColumnReduce(const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QwtSeriesData< QPointF >* series, int from, int to, qreal dpr)
{
    Polygon polyline;
    if (from > to)
        return polyline;

    int visFrom = from, visTo = to;
    qwtFindVisibleRange(xMap, series, from, to, visFrom, visTo);

    // bins are indexed by device pixel columns/rows
    const int xPixels = qAbs(qRound((xMap.p2() - xMap.p1()) * dpr)) + 1;
    if (xPixels <= 0)
        return polyline;

    struct Bin
    {
        int firstY, minY, maxY, lastY;
        int count;
    };

    QVector< Bin > bins(xPixels, Bin { 0, 0, 0, 0, 0 });

    const int xMin        = qRound(xMap.p1() * dpr);
    const bool linearPath = xMap.isLinear() && yMap.isLinear();
    if (linearPath) {
        const double xCnv = xMap.cnv(), xOff = xMap.p1() - xMap.ts1() * xCnv;
        const double yCnv = yMap.cnv(), yOff = yMap.p1() - yMap.ts1() * yCnv;

        for (int i = visFrom; i <= visTo; i++) {
            const QPointF sample = series->sample(i);
            if (qwt_is_nan_or_inf(sample))
                continue;
            const int x   = qRound((sample.x() * xCnv + xOff) * dpr);
            const int y   = qRound((sample.y() * yCnv + yOff) * dpr);
            const int col = x - xMin;
            if (col < 0 || col >= xPixels)
                continue;
            Bin& bin = bins[ col ];
            if (bin.count == 0) {
                bin.firstY = bin.minY = bin.maxY = bin.lastY = y;
            } else {
                if (y < bin.minY)
                    bin.minY = y;
                else if (y > bin.maxY)
                    bin.maxY = y;
                bin.lastY = y;
            }
            bin.count++;
        }
    } else {
        for (int i = visFrom; i <= visTo; i++) {
            const QPointF sample = series->sample(i);
            if (qwt_is_nan_or_inf(sample))
                continue;
            const int x   = qwtRoundValue(xMap.transform(sample.x()) * dpr);
            const int y   = qwtRoundValue(yMap.transform(sample.y()) * dpr);
            const int col = x - xMin;
            if (col < 0 || col >= xPixels)
                continue;
            Bin& bin = bins[ col ];
            if (bin.count == 0) {
                bin.firstY = bin.minY = bin.maxY = bin.lastY = y;
            } else {
                if (y < bin.minY)
                    bin.minY = y;
                else if (y > bin.maxY)
                    bin.maxY = y;
                bin.lastY = y;
            }
            bin.count++;
        }
    }

    polyline.resize(4 * xPixels);
    Point* outPts = polyline.data();
    int n         = 0;

    /*
        The clusters are laid out as in the adaptive sampling of QCustomPlot:
        the min and max of a pixel column are placed at fixed sub pixel
        positions ( +0.25 / +0.75 ), what keeps the vertical stroke of each
        column inside its pixel and preserves the exact extremes.
        Real data points are only added at the borders of a cluster, when
        the neighbor column is empty or holds a single point.
     */
    // device pixel coordinates back to paint coordinates
    const double invDpr = 1.0 / dpr;

    for (int col = 0; col < xPixels; col++) {
        const Bin& bin = bins[ col ];
        if (bin.count == 0)
            continue;

        const double x = (xMin + col) * invDpr;

        if (bin.count == 1) {
            outPts[ n++ ] = qwtMakePointD< Point >(x, bin.firstY * invDpr);
            continue;
        }

        const bool gapBefore = (col > 0) && (bins[ col - 1 ].count <= 1);
        const bool gapAfter  = (col < xPixels - 1) && (bins[ col + 1 ].count <= 1);

        if (gapBefore)
            outPts[ n++ ] = qwtMakePointD< Point >(x + 0.2 * invDpr, bin.firstY * invDpr);

        outPts[ n++ ] = qwtMakePointD< Point >(x + 0.25 * invDpr, bin.minY * invDpr);
        outPts[ n++ ] = qwtMakePointD< Point >(x + 0.75 * invDpr, bin.maxY * invDpr);

        if (gapAfter)
            outPts[ n++ ] = qwtMakePointD< Point >(x + 0.8 * invDpr, bin.lastY * invDpr);
    }
    polyline.resize(n);

    return polyline;
}

// Try to extract raw contiguous X/Y double pointers from the series data.
// Returns {nullptr, nullptr} if the series type does not support direct access.
struct QwtRawPointData
{
    const double* x;
    const double* y;
};

static QwtRawPointData qwtTryGetRawPointData(const QwtSeriesData< QPointF >* series)
{
    if (auto* arr = dynamic_cast< const QwtPointArrayData< double >* >(series))
        return { arr->xData().constData(), arr->yData().constData() };
    if (auto* ptr = dynamic_cast< const QwtCPointerData< double >* >(series))
        return { ptr->xData(), ptr->yData() };
    if (auto* val = dynamic_cast< const QwtValuePointData< double >* >(series))
        return { nullptr, val->yData().constData() };
    if (auto* cval = dynamic_cast< const QwtCPointerValueData< double >* >(series))
        return { nullptr, cval->yData() };
    return { nullptr, nullptr };
}

// Map all samples to pixel coordinates, keeping their sub pixel precision.
// NaN/Inf samples are kept, so that gaps of the polyline are painted
// the same way as without any downsampling.
template< class Polygon, class Point >
static Polygon qwtMapPointsNoRound(const QwtScaleMap& xMap,
                                   const QwtScaleMap& yMap,
                                   const QwtSeriesData< QPointF >* series,
                                   int from, int to)
{
    Polygon polyline;
    if (from > to)
        return polyline;

    polyline.reserve(to - from + 1);

    for (int i = from; i <= to; i++) {
        const QPointF sample = series->sample(i);
        polyline += qwtMakePointD< Point >(xMap.transform(sample.x()), yMap.transform(sample.y()));
    }

    return polyline;
}

// Append the min/max pair of a bucket to the output polygon, keeping
// the sample order of both points. Points becoming identical
// ( f.e. after rounding ) are appended once only.
template< class Point >
static inline void qwtAppendMinMaxPair(Point*& outPts, int& n, bool roundPoints, qreal dpr,
                                       double x1, double y1, double x2, double y2)
{
    if (roundPoints) {
        // round to the device pixel grid
        x1 = qwtRoundValueF(x1 * dpr) / dpr;
        y1 = qwtRoundValueF(y1 * dpr) / dpr;
        x2 = qwtRoundValueF(x2 * dpr) / dpr;
        y2 = qwtRoundValueF(y2 * dpr) / dpr;
    }

    outPts[ n++ ] = qwtMakePointD< Point >(x1, y1);
    if (x1 != x2 || y1 != y2)
        outPts[ n++ ] = qwtMakePointD< Point >(x2, y2);
}

// MinMax bucket downsampling: divide data into N equal-count buckets,
// keep min-Y and max-Y from each bucket. O(n) time, O(N) output.
//
// When roundPoints is false, the mapped coordinates keep their sub pixel
// precision - what is the better choice for paint devices, where
// antialiasing is enabled.
template< class Polygon, class Point >
static Polygon
qwtMinMaxBucketReduce(const QwtScaleMap& xMap, const QwtScaleMap& yMap,
                      const QwtSeriesData< QPointF >* series, int from, int to, bool roundPoints, qreal dpr)
{
    Polygon polyline;
    if (from > to)
        return polyline;

    int visFrom = from, visTo = to;
    qwtFindVisibleRange(xMap, series, from, to, visFrom, visTo);

    const int numPoints = visTo - visFrom + 1;
    if (numPoints <= 0)
        return polyline;

    // buckets are sized in device pixels
    const int xPixels    = qAbs(qRound((xMap.p2() - xMap.p1()) * dpr)) + 1;
    const int numBuckets = qMax(2, 2 * xPixels);  // 2 points per bucket (min + max)

    if (numPoints <= numBuckets * 2) {
        // not enough points to benefit from bucketing, fall back to plain mapping
        if (roundPoints)
            return qwtMapPointsQuad< Polygon, Point >(xMap, yMap, series, visFrom, visTo, dpr);
        return qwtMapPointsNoRound< Polygon, Point >(xMap, yMap, series, visFrom, visTo);
    }

    const double bucketSize = static_cast< double >(numPoints) / numBuckets;
    const bool linearPath   = xMap.isLinear() && yMap.isLinear();

    polyline.resize(2 * numBuckets);
    Point* outPts = polyline.data();
    int n         = 0;

    const QwtRawPointData raw = qwtTryGetRawPointData(series);

    if (linearPath && raw.y != nullptr) {
        const double xCnv = xMap.cnv(), xOff = xMap.p1() - xMap.ts1() * xCnv;
        const double yCnv = yMap.cnv(), yOff = yMap.p1() - yMap.ts1() * yCnv;

        const bool hasRawX = raw.x != nullptr;

        /*
            Scalar reduction of one bucket, skipping samples with a
            non-finite x or y like the generic path does. Used for the
            whole range, when the pre-scan found non-finite y values,
            and per bucket, when a SIMD bucket extremum sits on a
            sample with a non-finite x.
         */
        auto reduceBucketScalar = [ & ](int b)
        {
            const int start = visFrom + static_cast< int >(b * bucketSize);
            const int end   = visFrom + static_cast< int >((b + 1) * bucketSize) - 1;

            double minX = 0.0, maxX = 0.0;
            double minY = DBL_MAX, maxY = -DBL_MAX;
            int minIdx = -1, maxIdx = -1;

            for (int i = start; i <= qMin(end, visTo); i++) {
                const double y = raw.y[ i ];
                if (!std::isfinite(y))
                    continue;

                const double x = hasRawX ? raw.x[ i ] : i;
                if (!std::isfinite(x))
                    continue;

                const double px = x * xCnv + xOff;
                const double py = y * yCnv + yOff;

                if (minIdx < 0) {
                    minIdx = maxIdx = i;
                    minX = maxX = px;
                    minY = maxY = py;
                } else {
                    if (py < minY) {
                        minY   = py;
                        minX   = px;
                        minIdx = i;
                    }
                    if (py > maxY) {
                        maxY   = py;
                        maxX   = px;
                        maxIdx = i;
                    }
                }
            }

            if (minIdx >= 0) {
                if (minIdx <= maxIdx)
                    qwtAppendMinMaxPair< Point >(outPts, n, roundPoints, dpr, minX, minY, maxX, maxY);
                else
                    qwtAppendMinMaxPair< Point >(outPts, n, roundPoints, dpr, maxX, maxY, minX, minY);
            }
        };

        /*
            Non-finite pre-scan of the y values only: scanning the x
            array as well would double the memory traffic of the scan
            ( ~10% frame time on 1M clean points, measured ). Samples
            with a non-finite x are caught per bucket in the SIMD path
            below instead.
         */
        bool hasNonFinite = false;
        for (int i = visFrom; i <= visTo; ++i) {
            if (!std::isfinite(raw.y[ i ])) {
                hasNonFinite = true;
                break;
            }
        }

        if (!hasNonFinite) {
            // ===== SIMD fast path: all y values are finite =====
            for (int b = 0; b < numBuckets; b++) {
                const int start = visFrom + static_cast< int >(b * bucketSize);
                const int end   = visFrom + static_cast< int >((b + 1) * bucketSize) - 1;
                const int bEnd  = qMin(end, visTo);
                const int count = bEnd - start + 1;
                if (count <= 0)
                    continue;

                const QwtArgMinMaxResult r = qwtSimdArgMinMax(raw.y + start, count);
                const int minI             = start + r.minIdx;
                const int maxI             = start + r.maxIdx;

                if (hasRawX && (!std::isfinite(raw.x[ minI ]) || !std::isfinite(raw.x[ maxI ]))) {
                    // a bucket extremum sits on a sample with NaN/Inf x:
                    // qRound(NaN) == 0 would emit a phantom vertex at
                    // column 0 - redo the bucket without those samples
                    reduceBucketScalar(b);
                    continue;
                }

                const double xMin = hasRawX ? raw.x[ minI ] * xCnv + xOff : minI * xCnv + xOff;
                const double xMax = hasRawX ? raw.x[ maxI ] * xCnv + xOff : maxI * xCnv + xOff;
                const double yMin = r.minVal * yCnv + yOff;
                const double yMax = r.maxVal * yCnv + yOff;

                if (minI <= maxI)
                    qwtAppendMinMaxPair< Point >(outPts, n, roundPoints, dpr, xMin, yMin, xMax, yMax);
                else
                    qwtAppendMinMaxPair< Point >(outPts, n, roundPoints, dpr, xMax, yMax, xMin, yMin);
            }
        } else {
            // ===== Scalar path with raw pointers (has non-finite y) =====
            for (int b = 0; b < numBuckets; b++)
                reduceBucketScalar(b);
        }
    } else if (linearPath) {
        // ===== Scalar path with virtual sample() access =====
        const double xCnv = xMap.cnv(), xOff = xMap.p1() - xMap.ts1() * xCnv;
        const double yCnv = yMap.cnv(), yOff = yMap.p1() - yMap.ts1() * yCnv;

        for (int b = 0; b < numBuckets; b++) {
            const int start = visFrom + static_cast< int >(b * bucketSize);
            const int end   = visFrom + static_cast< int >((b + 1) * bucketSize) - 1;

            double minX = 0.0, maxX = 0.0;
            double minY = DBL_MAX, maxY = -DBL_MAX;
            int minIdx = -1, maxIdx = -1;

            for (int i = start; i <= qMin(end, visTo); i++) {
                const QPointF sample = series->sample(i);
                if (qwt_is_nan_or_inf(sample))
                    continue;

                const double px = sample.x() * xCnv + xOff;
                const double py = sample.y() * yCnv + yOff;

                if (minIdx < 0) {
                    minIdx = maxIdx = i;
                    minX = maxX = px;
                    minY = maxY = py;
                } else {
                    if (py < minY) {
                        minY   = py;
                        minX   = px;
                        minIdx = i;
                    }
                    if (py > maxY) {
                        maxY   = py;
                        maxX   = px;
                        maxIdx = i;
                    }
                }
            }

            if (minIdx >= 0) {
                if (minIdx <= maxIdx)
                    qwtAppendMinMaxPair< Point >(outPts, n, roundPoints, dpr, minX, minY, maxX, maxY);
                else
                    qwtAppendMinMaxPair< Point >(outPts, n, roundPoints, dpr, maxX, maxY, minX, minY);
            }
        }
    } else {
        // ===== Scalar path for non-linear scale maps =====
        for (int b = 0; b < numBuckets; b++) {
            const int start = visFrom + static_cast< int >(b * bucketSize);
            const int end   = visFrom + static_cast< int >((b + 1) * bucketSize) - 1;

            double minX = 0.0, maxX = 0.0;
            double minY = DBL_MAX, maxY = -DBL_MAX;
            int minIdx = -1, maxIdx = -1;

            for (int i = start; i <= qMin(end, visTo); i++) {
                const QPointF sample = series->sample(i);
                if (qwt_is_nan_or_inf(sample))
                    continue;

                const double px = xMap.transform(sample.x());
                const double py = yMap.transform(sample.y());

                if (minIdx < 0) {
                    minIdx = maxIdx = i;
                    minX = maxX = px;
                    minY = maxY = py;
                } else {
                    if (py < minY) {
                        minY   = py;
                        minX   = px;
                        minIdx = i;
                    }
                    if (py > maxY) {
                        maxY   = py;
                        maxX   = px;
                        maxIdx = i;
                    }
                }
            }

            if (minIdx >= 0) {
                if (minIdx <= maxIdx)
                    qwtAppendMinMaxPair< Point >(outPts, n, roundPoints, dpr, minX, minY, maxX, maxY);
                else
                    qwtAppendMinMaxPair< Point >(outPts, n, roundPoints, dpr, maxX, maxY, minX, minY);
            }
        }
    }

    polyline.resize(n);
    return polyline;
}

// Largest-Triangle-Three-Buckets ( LTTB ) downsampling: divide the data
// into N equal-count buckets and keep the sample of each bucket, that
// forms the largest triangle with the previously selected point and the
// average of the next bucket. The first and last sample are always kept,
// so that the curve always spans the full data range.
// O(n) time, O(N) output.
//
// When roundPoints is false, the mapped coordinates keep their sub pixel
// precision - what is the better choice for paint devices, where
// antialiasing is enabled.
template< class Polygon, class Point >
static Polygon
qwtLttbReduce(const QwtScaleMap& xMap, const QwtScaleMap& yMap,
              const QwtSeriesData< QPointF >* series, int from, int to, bool roundPoints, qreal dpr)
{
    Polygon polyline;
    if (from > to)
        return polyline;

    int visFrom = from, visTo = to;
    qwtFindVisibleRange(xMap, series, from, to, visFrom, visTo);

    const int numPoints = visTo - visFrom + 1;
    if (numPoints <= 0)
        return polyline;

    // buckets are sized in device pixels
    const int xPixels    = qAbs(qRound((xMap.p2() - xMap.p1()) * dpr)) + 1;
    const int numBuckets = qMax(2, 4 * xPixels);  // 1 point per bucket

    if (numPoints <= numBuckets) {
        // not enough points to benefit from bucketing, fall back to plain mapping
        if (roundPoints)
            return qwtMapPointsQuad< Polygon, Point >(xMap, yMap, series, visFrom, visTo, dpr);
        return qwtMapPointsNoRound< Polygon, Point >(xMap, yMap, series, visFrom, visTo);
    }

    const double bucketSize = static_cast< double >(numPoints) / numBuckets;
    const bool linearPath   = xMap.isLinear() && yMap.isLinear();

    // fast path for linear scale maps: avoid virtual transform() calls
    const double xCnv = linearPath ? xMap.cnv() : 0.0;
    const double xOff = linearPath ? xMap.p1() - xMap.ts1() * xCnv : 0.0;
    const double yCnv = linearPath ? yMap.cnv() : 0.0;
    const double yOff = linearPath ? yMap.p1() - yMap.ts1() * yCnv : 0.0;

    const QwtRawPointData raw = qwtTryGetRawPointData(series);
    const bool useRaw         = linearPath && (raw.y != nullptr);

    // map sample i to pixel coordinates, returns false for NaN/Inf samples
    auto mapSample = [ & ](int i, double& px, double& py)->bool {
        if (useRaw) {
            const double y = raw.y[ i ];
            if (std::isnan(y))
                return false;

            const double x = (raw.x != nullptr) ? raw.x[ i ] : i;
            px             = x * xCnv + xOff;
            py             = y * yCnv + yOff;
        } else {
            const QPointF sample = series->sample(i);
            if (qwt_is_nan_or_inf(sample))
                return false;

            px = linearPath ? sample.x() * xCnv + xOff : xMap.transform(sample.x());
            py = linearPath ? sample.y() * yCnv + yOff : yMap.transform(sample.y());
        }
        return true;
    };

    // pass 1: average of the mapped coordinates per bucket
    QVector< double > avgX(numBuckets, 0.0), avgY(numBuckets, 0.0);
    QVector< int > cnt(numBuckets, 0);

    for (int b = 0; b < numBuckets; b++) {
        const int start = visFrom + static_cast< int >(b * bucketSize);
        const int end   = visFrom + static_cast< int >((b + 1) * bucketSize) - 1;

        double sx = 0.0, sy = 0.0;
        int c     = 0;

        for (int i = start; i <= qMin(end, visTo); i++) {
            double px, py;
            if (!mapSample(i, px, py))
                continue;

            sx += px;
            sy += py;
            c++;
        }

        if (c > 0) {
            avgX[ b ] = sx / c;
            avgY[ b ] = sy / c;
        }
        cnt[ b ] = c;
    }

    // index of the next bucket, that holds valid samples
    QVector< int > nextValidBucket(numBuckets, -1);
    {
        int next = -1;
        for (int b = numBuckets - 1; b >= 0; b--) {
            nextValidBucket[ b ] = next;
            if (cnt[ b ] > 0)
                next = b;
        }
    }

    polyline.resize(numBuckets);
    Point* outPts = polyline.data();
    int n         = 0;

    double lastX = 0.0, lastY = 0.0;  // coordinates of the last emitted point
    bool haveLast = false;

    auto appendPoint = [ & ](double px, double py) {
        if (roundPoints) {
            // round to the device pixel grid
            px = qwtRoundValueF(px * dpr) / dpr;
            py = qwtRoundValueF(py * dpr) / dpr;
        }
        if (haveLast && px == lastX && py == lastY)
            return;

        outPts[ n++ ] = qwtMakePointD< Point >(px, py);
        lastX         = px;
        lastY         = py;
        haveLast      = true;
    };

    // bucket 0: keep the first valid sample of the data
    double prevX = 0.0, prevY = 0.0;
    bool havePrev = false;

    for (int i = visFrom; i <= visTo; i++) {
        double px, py;
        if (mapSample(i, px, py)) {
            prevX    = px;
            prevY    = py;
            havePrev = true;
            break;
        }
    }

    if (!havePrev) {
        polyline.clear();
        return polyline;
    }

    appendPoint(prevX, prevY);

    // middle buckets: keep the sample forming the largest triangle with
    // the previous selection and the average of the next valid bucket
    for (int b = 1; b < numBuckets - 1; b++) {
        const int start = visFrom + static_cast< int >(b * bucketSize);
        const int end   = visFrom + static_cast< int >((b + 1) * bucketSize) - 1;

        const int nb = nextValidBucket[ b ];

        double bestX = 0.0, bestY = 0.0;
        double bestArea = -1.0;

        for (int i = start; i <= qMin(end, visTo); i++) {
            double px, py;
            if (!mapSample(i, px, py))
                continue;

            double area = 0.0;
            if (nb >= 0) {
                // twice the area of the triangle ( prev, point, avgNext )
                area = qAbs( (px - prevX) * (avgY[ nb ] - prevY)
                             - (py - prevY) * (avgX[ nb ] - prevX) );
            }

            if (area > bestArea) {
                bestArea = area;
                bestX    = px;
                bestY    = py;
            }
        }

        if (bestArea >= 0.0) {
            prevX = bestX;
            prevY = bestY;
            appendPoint(prevX, prevY);
        }
    }

    // last bucket: keep the last valid sample, so that the curve
    // always ends at the end of the data
    {
        const int start = visFrom + static_cast< int >((numBuckets - 1) * bucketSize);

        for (int i = visTo; i >= qMax(start, visFrom); i--) {
            double px, py;
            if (mapSample(i, px, py)) {
                appendPoint(px, py);
                break;
            }
        }
    }

    polyline.resize(n);
    return polyline;
}

// Adaptive column reduce: single pass O(n), device pixel ratio aware.
//
// Density gate: when the visible samples are not denser than 2 points per
// device pixel column the data is mapped 1:1 ( no reduction ) - zooming in
// always restores full fidelity.
//
// Otherwise the visible samples are clustered by device pixel column and,
// inside a cluster, the first sample, every sample whose mapped y differs by
// at least 1 device pixel from the last kept sample and the last sample are
// kept. The samples keep their original order and their real ( sub pixel )
// coordinates, so ramps stay ramps instead of degenerating into the 1px
// staircase of vertical strokes and horizontal steps, that the min/max
// envelope per column ( qwtPixelColumnReduce ) produces for oversampled
// smooth curves. As every emitted point is a real sample the polyline
// across empty columns is exactly the chord of the unreduced rendering.
//
// When the tolerance selection of a cluster would emit more than
// K = 4 + ( cluster y span in device pixels ) points - what only happens for
// high frequency noise inside one column - the cluster falls back to an
// ordered first/min/max/last quadruple, so the output per column is bounded.
//
// When roundPoints is false, the mapped coordinates keep their sub pixel
// precision - what is the better choice for paint devices, where
// antialiasing is enabled.
template< class Polygon, class Point >
static Polygon
qwtAdaptiveColumnReduce(const QwtScaleMap& xMap, const QwtScaleMap& yMap,
                        const QwtSeriesData< QPointF >* series, int from, int to, bool roundPoints, qreal dpr)
{
    Polygon polyline;
    if (from > to)
        return polyline;

    int visFrom = from, visTo = to;
    qwtFindVisibleRange(xMap, series, from, to, visFrom, visTo);

    const int numPoints = visTo - visFrom + 1;
    if (numPoints <= 0)
        return polyline;

    // columns are device pixel columns
    const int xPixels = qAbs(qRound((xMap.p2() - xMap.p1()) * dpr)) + 1;
    if (xPixels <= 0)
        return polyline;

    const bool linearPath = xMap.isLinear() && yMap.isLinear();

    // fast path for linear scale maps: avoid virtual transform() calls
    const double xCnv = linearPath ? xMap.cnv() : 0.0;
    const double xOff = linearPath ? xMap.p1() - xMap.ts1() * xCnv : 0.0;
    const double yCnv = linearPath ? yMap.cnv() : 0.0;
    const double yOff = linearPath ? yMap.p1() - yMap.ts1() * yCnv : 0.0;

    const QwtRawPointData raw = qwtTryGetRawPointData(series);
    const bool useRaw         = linearPath && (raw.y != nullptr);
    const bool hasRawX        = raw.x != nullptr;

    // map sample i to paint coordinates, returns false for NaN/Inf samples
    auto mapSample = [ & ](int i, double& px, double& py)->bool {
        if (useRaw) {
            const double y = raw.y[ i ];
            if (!std::isfinite(y))
                return false;

            const double x = hasRawX ? raw.x[ i ] : i;
            if (hasRawX && !std::isfinite(x))
                return false;

            px = x * xCnv + xOff;
            py = y * yCnv + yOff;
        } else {
            const QPointF sample = series->sample(i);
            if (qwt_is_nan_or_inf(sample))
                return false;

            px = linearPath ? sample.x() * xCnv + xOff : xMap.transform(sample.x());
            py = linearPath ? sample.y() * yCnv + yOff : yMap.transform(sample.y());
        }
        return true;
    };

    double lastOutX = 0.0, lastOutY = 0.0;
    bool haveLast = false;

    auto appendPoint = [ & ](double px, double py) {
        if (roundPoints) {
            // round to the device pixel grid
            px = qwtRoundValueF(px * dpr) / dpr;
            py = qwtRoundValueF(py * dpr) / dpr;
        }
        if (haveLast && px == lastOutX && py == lastOutY)
            return;

        polyline += qwtMakePointD< Point >(px, py);
        lastOutX   = px;
        lastOutY   = py;
        haveLast   = true;
    };

    if (numPoints <= 2 * xPixels) {
        /*
            Density gate: at most ~2 samples per device pixel column.
            Mapping 1:1 - any reduction would only lose fidelity, f.e.
            the shape of curves being zoomed in.
         */
        polyline.reserve(numPoints);

        for (int i = visFrom; i <= visTo; i++) {
            double px, py;
            if (!mapSample(i, px, py))
                continue;

            appendPoint(px, py);
        }

        return polyline;
    }

    // vertical tolerance for the ordered selection, in device pixels
    const double tol = 1.0;

    polyline.reserve(qMin(numPoints, 8 * xPixels) + 16);

    // buffer for the selected points of the cluster being processed
    QVector< QPointF > kept;
    kept.reserve(64);

    int i = visFrom;
    while (i <= visTo) {
        double headX, headY;
        if (!mapSample(i, headX, headY)) {
            i++;
            continue;
        }

        const int col = qRound(headX * dpr);

        /*
            Pass 1: find the extent of the cluster of samples mapped into
            the same device pixel column and collect the statistics for
            the quadruple fallback: first/last and the extremes with their
            real coordinates.
         */
        int clusterEnd = i;
        double tailX = headX, tailY = headY;
        double minDevY = headY * dpr, maxDevY = minDevY;
        double minPx = headX, minPy = headY;
        double maxPx = headX, maxPy = headY;
        int count    = 1;

        int j = i + 1;
        for (; j <= visTo; j++) {
            double px, py;
            if (!mapSample(j, px, py))
                continue;

            if (qRound(px * dpr) != col)
                break;

            clusterEnd = j;
            tailX      = px;
            tailY      = py;

            const double devY = py * dpr;
            if (devY < minDevY) {
                minDevY = devY;
                minPx   = px;
                minPy   = py;
            } else if (devY > maxDevY) {
                maxDevY = devY;
                maxPx   = px;
                maxPy   = py;
            }

            count++;
        }

        const double span = maxDevY - minDevY;

        if (count == 1 || span <= tol) {
            // a single sample or a cluster that is flat inside the
            // tolerance is represented by its first sample
            appendPoint(headX, headY);
        } else {
            // upper bound for the number of emitted points of the cluster
            const int cap = 4 + qRound(span);

            /*
                Pass 2: ordered selection - keep the first sample and every
                sample, whose mapped y differs by at least tol device pixels
                from the last kept sample. The greedy selection is aborted as
                soon as the cap is exceeded: the cluster holds high frequency
                noise then, that can't be shown inside one column anyway.
             */
            kept.clear();
            kept.append(QPointF(headX, headY));

            double lastDevY = headY * dpr;
            bool overflow   = false;

            for (int k = i + 1; k <= clusterEnd; k++) {
                double px, py;
                if (!mapSample(k, px, py))
                    continue;

                const double devY = py * dpr;
                if (qAbs(devY - lastDevY) >= tol) {
                    kept.append(QPointF(px, py));
                    lastDevY = devY;

                    if (kept.size() > cap) {
                        overflow = true;
                        break;
                    }
                }
            }

            if (overflow) {
                // noise fallback: an ordered first/min/max/last quadruple
                // covering the full span of the column, so that the vertical
                // extent of the cluster is not lost
                appendPoint(headX, headY);

                if (tailY > headY) {
                    appendPoint(minPx, minPy);
                    appendPoint(maxPx, maxPy);
                } else {
                    appendPoint(maxPx, maxPy);
                    appendPoint(minPx, minPy);
                }

                appendPoint(tailX, tailY);
            } else {
                // the last sample of the cluster is always kept, so that
                // the connection to the next cluster is the real chord
                if (kept.constLast() != QPointF(tailX, tailY))
                    kept.append(QPointF(tailX, tailY));

                for (const QPointF& p : qwt_as_const(kept))
                    appendPoint(p.x(), p.y());
            }
        }

        // j is either behind the end or the index of the first sample
        // of the next cluster
        i = j;
    }

    return polyline;
}

// Helper class to work around the 5 parameters
// limitation of QtConcurrent::run()
class QwtDotsCommand
{
public:
    const QwtSeriesData< QPointF >* series;
    int from;
    int to;
    QRgb rgb;
};

static void
qwtRenderDots(const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QwtDotsCommand& command, const QPoint& pos, QImage* image)
{
    const QRgb rgb = command.rgb;
    QRgb* bits     = reinterpret_cast< QRgb* >(image->bits());

    const int w = image->width();
    const int h = image->height();

    const int x0 = pos.x();
    const int y0 = pos.y();

    for (int i = command.from; i <= command.to; i++) {
        const QPointF sample = command.series->sample(i);
        // check nan
        if (qwt_is_nan_or_inf(sample)) {
            continue;
        }
        const int x = static_cast< int >(xMap.transform(sample.x()) + 0.5) - x0;
        const int y = static_cast< int >(yMap.transform(sample.y()) + 0.5) - y0;

        if (x >= 0 && x < w && y >= 0 && y < h)
            bits[ y * w + x ] = rgb;
    }
}

// some functors, so that the compile can inline
struct QwtRoundI
{
    inline int operator()(double value) const
    {
        return qwtRoundValue(value);
    }
};

// rounding to the device pixel grid: integers for a ratio of 1.0
struct QwtRoundF
{
    qreal ratio = 1.0;

    inline double operator()(double value) const
    {
        return qwtRoundValueF(value * ratio) / ratio;
    }
};

struct QwtNoRoundF
{
    inline double operator()(double value) const
    {
        return value;
    }
};

// mapping points without any filtering - beside checking
// the bounding rectangle

template< class Polygon, class Point, class Round >
static inline Polygon qwtToPoints(const QRectF& boundingRect,
                                  const QwtScaleMap& xMap,
                                  const QwtScaleMap& yMap,
                                  const QwtSeriesData< QPointF >* series,
                                  int from,
                                  int to,
                                  Round round)
{
    Polygon polyline(to - from + 1);
    Point* points = polyline.data();

    int numPoints = 0;

    if (boundingRect.isValid()) {
        // iterating over all values
        // filtering out all points outside of
        // the bounding rectangle

        for (int i = from; i <= to; i++) {
            const QPointF sample = series->sample(i);
            // check nan
            if (qwt_is_nan_or_inf(sample)) {
                continue;
            }
            const double x = xMap.transform(sample.x());
            const double y = yMap.transform(sample.y());

            if (boundingRect.contains(x, y)) {
                points[ numPoints ].rx() = round(x);
                points[ numPoints ].ry() = round(y);

                numPoints++;
            }
        }

        polyline.resize(numPoints);
    } else {
        // simply iterating over all values
        // without any filtering

        for (int i = from; i <= to; i++) {
            const QPointF sample = series->sample(i);
            // check nan
            if (qwt_is_nan_or_inf(sample)) {
                continue;
            }
            const double x = xMap.transform(sample.x());
            const double y = yMap.transform(sample.y());

            points[ numPoints ].rx() = round(x);
            points[ numPoints ].ry() = round(y);

            numPoints++;
        }
        // Since NaN value checking has been added, resizing is also required here.
        polyline.resize(numPoints);
    }

    return polyline;
}

static inline QPolygon qwtToPointsI(const QRectF& boundingRect,
                                    const QwtScaleMap& xMap,
                                    const QwtScaleMap& yMap,
                                    const QwtSeriesData< QPointF >* series,
                                    int from,
                                    int to)
{
    return qwtToPoints< QPolygon, QPoint >(boundingRect, xMap, yMap, series, from, to, QwtRoundI());
}

template< class Round >
static inline QPolygonF qwtToPointsF(const QRectF& boundingRect,
                                     const QwtScaleMap& xMap,
                                     const QwtScaleMap& yMap,
                                     const QwtSeriesData< QPointF >* series,
                                     int from,
                                     int to,
                                     Round round)
{
    return qwtToPoints< QPolygonF, QPointF >(boundingRect, xMap, yMap, series, from, to, round);
}

// Mapping points with filtering out consecutive
// points mapped to the same position

template< class Polygon, class Point, class Round >
static inline Polygon qwtToPolylineFiltered(const QwtScaleMap& xMap,
                                            const QwtScaleMap& yMap,
                                            const QwtSeriesData< QPointF >* series,
                                            int from,
                                            int to,
                                            Round round)
{
    // in curves with many points consecutive points
    // are often mapped to the same position. As this might
    // result in empty lines ( or symbols hidden by others )
    // we try to filter them out

    Polygon polyline(to - from + 1);
    Point* points = polyline.data();

    int realFrom    = from;
    QPointF sample0 = series->sample(from);
    // check nan
    while (realFrom < to && qwt_is_nan_or_inf(sample0)) {
        realFrom++;
        sample0 = series->sample(realFrom);
    }

    // all samples in [from, to] are NaN/Inf: return an empty polyline,
    // so that "empty polygon == no data" holds for the callers
    if (realFrom >= to && qwt_is_nan_or_inf(sample0))
        return Polygon();

    points[ 0 ].rx() = round(xMap.transform(sample0.x()));
    points[ 0 ].ry() = round(yMap.transform(sample0.y()));

    int pos = 0;
    for (int i = realFrom + 1; i <= to; i++) {
        const QPointF sample = series->sample(i);
        // check nan
        if (qwt_is_nan_or_inf(sample)) {
            continue;
        }
        const Point p(round(xMap.transform(sample.x())), round(yMap.transform(sample.y())));

        if (points[ pos ] != p)
            points[ ++pos ] = p;
    }

    polyline.resize(pos + 1);
    return polyline;
}

static inline QPolygon qwtToPolylineFilteredI(const QwtScaleMap& xMap,
                                              const QwtScaleMap& yMap,
                                              const QwtSeriesData< QPointF >* series,
                                              int from,
                                              int to)
{
    return qwtToPolylineFiltered< QPolygon, QPoint >(xMap, yMap, series, from, to, QwtRoundI());
}

template< class Round >
static inline QPolygonF qwtToPolylineFilteredF(const QwtScaleMap& xMap,
                                               const QwtScaleMap& yMap,
                                               const QwtSeriesData< QPointF >* series,
                                               int from,
                                               int to,
                                               Round round)
{
    return qwtToPolylineFiltered< QPolygonF, QPointF >(xMap, yMap, series, from, to, round);
}

template< class Polygon, class Point >
static inline Polygon qwtToPointsFiltered(const QRectF& boundingRect,
                                          const QwtScaleMap& xMap,
                                          const QwtScaleMap& yMap,
                                          const QwtSeriesData< QPointF >* series,
                                          int from,
                                          int to)
{
    // F.e. in scatter plots ( no connecting lines ) we
    // can sort out all duplicates ( not only consecutive points )

    Polygon polygon(to - from + 1);
    Point* points = polygon.data();

    QwtPixelMatrix pixelMatrix(boundingRect.toAlignedRect());

    int numPoints = 0;
    for (int i = from; i <= to; i++) {
        const QPointF sample = series->sample(i);
        // check nan
        if (qwt_is_nan_or_inf(sample)) {
            continue;
        }
        const int x = qwtRoundValue(xMap.transform(sample.x()));
        const int y = qwtRoundValue(yMap.transform(sample.y()));

        if (pixelMatrix.testAndSetPixel(x, y, true) == false) {
            points[ numPoints ].rx() = x;
            points[ numPoints ].ry() = y;

            numPoints++;
        }
    }

    polygon.resize(numPoints);
    return polygon;
}

static inline QPolygon qwtToPointsFilteredI(const QRectF& boundingRect,
                                            const QwtScaleMap& xMap,
                                            const QwtScaleMap& yMap,
                                            const QwtSeriesData< QPointF >* series,
                                            int from,
                                            int to)
{
    return qwtToPointsFiltered< QPolygon, QPoint >(boundingRect, xMap, yMap, series, from, to);
}

static inline QPolygonF qwtToPointsFilteredF(const QRectF& boundingRect,
                                             const QwtScaleMap& xMap,
                                             const QwtScaleMap& yMap,
                                             const QwtSeriesData< QPointF >* series,
                                             int from,
                                             int to)
{
    return qwtToPointsFiltered< QPolygonF, QPointF >(boundingRect, xMap, yMap, series, from, to);
}

class QwtPointMapper::PrivateData
{
public:
    QWT_DECLARE_PUBLIC(QwtPointMapper)

    PrivateData(QwtPointMapper* p) : q_ptr(p), boundingRect(qwtInvalidRect)
    {
    }

    QRectF boundingRect;
    QwtPointMapper::TransformationFlags flags;
    qreal devicePixelRatio = 1.0;
};

/**
 * @brief Constructor
 *
 * @details Creates a QwtPointMapper with default settings.
 *
 */
QwtPointMapper::QwtPointMapper() : QWT_PIMPL_CONSTRUCT
{
}

/**
 * @brief Destructor
 *
 * @details Destroys the QwtPointMapper and frees all allocated resources.
 *
 */
QwtPointMapper::~QwtPointMapper()
{
}

/**
 * @brief Set the flags affecting the transformation process
 *
 * @param[in] flags Flags
 *
 * @sa flags(), setFlag()
 *
 */
void QwtPointMapper::setFlags(TransformationFlags flags)
{
    QWT_D(d);
    d->flags = flags;
}

/**
 * @brief Get the flags affecting the transformation process
 *
 * @return Flags affecting the transformation process
 *
 * @sa setFlags(), setFlag()
 *
 */
QwtPointMapper::TransformationFlags QwtPointMapper::flags() const
{
    QWT_DC(d);
    return d->flags;
}

/**
 * @brief Modify a flag affecting the transformation process
 *
 * @param[in] flag Flag type
 * @param[in] on Value
 *
 * @sa testFlag(), setFlags()
 *
 */
void QwtPointMapper::setFlag(TransformationFlag flag, bool on)
{
    QWT_D(d);
    if (on)
        d->flags |= flag;
    else
        d->flags &= ~flag;
}

/**
 * @brief Test if a flag is set
 *
 * @param[in] flag Flag type
 *
 * @return True when the flag is set
 *
 * @sa setFlag(), setFlags()
 *
 */
bool QwtPointMapper::testFlag(TransformationFlag flag) const
{
    QWT_DC(d);
    return d->flags & flag;
}

/**
 * @brief Set a bounding rectangle for the point mapping algorithm
 *
 * @details A valid bounding rectangle can be used for optimizations.
 *
 * @param[in] rect Bounding rectangle
 *
 * @sa boundingRect()
 *
 */
void QwtPointMapper::setBoundingRect(const QRectF& rect)
{
    QWT_D(d);
    d->boundingRect = rect;
}

/**
 * @brief Get the bounding rectangle
 *
 * @return Bounding rectangle
 *
 * @sa setBoundingRect()
 *
 */
QRectF QwtPointMapper::boundingRect() const
{
    QWT_DC(d);
    return d->boundingRect;
}

/**
 * @brief Set the device pixel ratio of the target paint device
 *
 * @details All pixel based reductions ( bucket counts, pixel columns ) and
 *          the rounding of RoundPoints are calculated in device pixels of
 *          the target paint device. On high DPI screens ( ratio > 1 ) this
 *          keeps the resolution of the reduction and the granularity of the
 *          rounding aligned with what the device can actually display.
 *          The mapped coordinates themselves remain in paint coordinates.
 *
 * @param[in] ratio Device pixel ratio - values <= 0 are reset to 1.0
 *
 * @sa devicePixelRatio(), QwtPainter::devicePixelRatio()
 *
 */
void QwtPointMapper::setDevicePixelRatio(qreal ratio)
{
    QWT_D(d);
    d->devicePixelRatio = (ratio > 0.0) ? ratio : 1.0;
}

/**
 * @brief Get the device pixel ratio of the target paint device
 *
 * @return Device pixel ratio
 *
 * @sa setDevicePixelRatio()
 *
 */
qreal QwtPointMapper::devicePixelRatio() const
{
    QWT_DC(d);
    return d->devicePixelRatio;
}

/**
 * @brief Translate a series of points into a QPolygonF
 *
 * @details When the WeedOutPoints flag is enabled, consecutive points
 *          that are mapped to the same position will be one point.
 *          When RoundPoints is set, all points are rounded to the device
 *          pixel grid ( see devicePixelRatio() ) but returned as PolygonF -
 *          what only makes sense when the further processing of the values
 *          need a QPolygonF.
 *          When WeedOutIntermediatePoints is enabled, an even more aggressive
 *          weeding algorithm is enabled. It rounds to device pixels on its
 *          own and does not require RoundPoints to be set.
 *
 * @param[in] xMap x map
 * @param[in] yMap y map
 * @param[in] series Series of points to be mapped
 * @param[in] from Index of the first point to be painted
 * @param[in] to Index of the last point to be painted
 *
 * @return Translated polygon
 *
 */
QPolygonF QwtPointMapper::toPolygonF(const QwtScaleMap& xMap,
                                     const QwtScaleMap& yMap,
                                     const QwtSeriesData< QPointF >* series,
                                     int from,
                                     int to) const
{
    QWT_DC(d);
    QPolygonF polyline;

    const qreal dpr   = d->devicePixelRatio;
    const QwtRoundF roundF { dpr };

    // priority: AdaptiveColumnReduce > PixelColumnReduce > LTTBReduce > MinMaxReduce > WeedOutIntermediatePoints > WeedOutPoints
    if (d->flags & AdaptiveColumnReduce) {
        // without RoundPoints the output keeps its sub pixel precision
        polyline = qwtAdaptiveColumnReduce< QPolygonF, QPointF >(
            xMap, yMap, series, from, to, d->flags & RoundPoints, dpr);
    } else if (d->flags & PixelColumnReduce) {
        polyline = qwtPixelColumnReduce< QPolygonF, QPointF >(xMap, yMap, series, from, to, dpr);
    } else if (d->flags & LTTBReduce) {
        // without RoundPoints the output keeps its sub pixel precision
        polyline = qwtLttbReduce< QPolygonF, QPointF >(
            xMap, yMap, series, from, to, d->flags & RoundPoints, dpr);
    } else if (d->flags & MinMaxReduce) {
        // without RoundPoints the output keeps its sub pixel precision
        polyline = qwtMinMaxBucketReduce< QPolygonF, QPointF >(
            xMap, yMap, series, from, to, d->flags & RoundPoints, dpr);
    } else if (d->flags & WeedOutIntermediatePoints) {
        // the quad weeding path rounds to device pixels internally,
        // so it does not depend on RoundPoints being set
        polyline = qwtMapPointsQuad< QPolygonF, QPointF >(xMap, yMap, series, from, to, dpr);
    } else if (d->flags & RoundPoints) {
        if (d->flags & WeedOutPoints) {
            polyline = qwtToPolylineFilteredF(xMap, yMap, series, from, to, roundF);
        } else {
            polyline = qwtToPointsF(qwtInvalidRect, xMap, yMap, series, from, to, roundF);
        }
    } else {
        if (d->flags & WeedOutPoints) {
            polyline = qwtToPolylineFilteredF(xMap, yMap, series, from, to, QwtNoRoundF());
        } else {
            polyline = qwtToPointsF(qwtInvalidRect, xMap, yMap, series, from, to, QwtNoRoundF());
        }
    }

    return polyline;
}

/**
 * @brief Translate a series of points into a QPolygon
 *
 * @details When the WeedOutPoints flag is enabled, consecutive points
 *          that are mapped to the same position will be one point.
 *
 * @param[in] xMap x map
 * @param[in] yMap y map
 * @param[in] series Series of points to be mapped
 * @param[in] from Index of the first point to be painted
 * @param[in] to Index of the last point to be painted
 *
 * @return Translated polygon
 *
 */
QPolygon QwtPointMapper::toPolygon(const QwtScaleMap& xMap,
                                   const QwtScaleMap& yMap,
                                   const QwtSeriesData< QPointF >* series,
                                   int from,
                                   int to) const
{
    QWT_DC(d);
    QPolygon polyline;

    const qreal dpr = d->devicePixelRatio;

    if (d->flags & AdaptiveColumnReduce) {
        // integer points are always rounded
        polyline = qwtAdaptiveColumnReduce< QPolygon, QPoint >(xMap, yMap, series, from, to, true, dpr);
    } else if (d->flags & PixelColumnReduce) {
        polyline = qwtPixelColumnReduce< QPolygon, QPoint >(xMap, yMap, series, from, to, dpr);
    } else if (d->flags & LTTBReduce) {
        // integer points are always rounded
        polyline = qwtLttbReduce< QPolygon, QPoint >(xMap, yMap, series, from, to, true, dpr);
    } else if (d->flags & MinMaxReduce) {
        // integer points are always rounded
        polyline = qwtMinMaxBucketReduce< QPolygon, QPoint >(xMap, yMap, series, from, to, true, dpr);
    } else if (d->flags & WeedOutIntermediatePoints) {
        polyline = qwtMapPointsQuad< QPolygon, QPoint >(xMap, yMap, series, from, to, dpr);
    } else if (d->flags & WeedOutPoints) {
        polyline = qwtToPolylineFilteredI(xMap, yMap, series, from, to);
    } else {
        polyline = qwtToPointsI(qwtInvalidRect, xMap, yMap, series, from, to);
    }

    return polyline;
}

/**
 * @brief Translate a series into a QPolygonF (scattered points)
 *
 * @details
 * - WeedOutPoints & RoundPoints & boundingRect().isValid():
 *   All points that are mapped to the same position will be one point.
 *   Points outside of the bounding rectangle are ignored.
 * - WeedOutPoints & RoundPoints & !boundingRect().isValid():
 *   All consecutive points that are mapped to the same position will one point.
 * - WeedOutPoints & !RoundPoints:
 *   All consecutive points that are mapped to the same position will one point.
 * - !WeedOutPoints & boundingRect().isValid():
 *   Points outside of the bounding rectangle are ignored.
 *
 *   When RoundPoints is set, all points are rounded to the device pixel
 *   grid ( see devicePixelRatio() ) but returned as PolygonF - what only
 *   makes sense when the further processing of the values need a QPolygonF.
 *
 * @param[in] xMap x map
 * @param[in] yMap y map
 * @param[in] series Series of points to be mapped
 * @param[in] from Index of the first point to be painted
 * @param[in] to Index of the last point to be painted
 *
 * @return Translated polygon
 *
 */
QPolygonF QwtPointMapper::toPointsF(const QwtScaleMap& xMap,
                                    const QwtScaleMap& yMap,
                                    const QwtSeriesData< QPointF >* series,
                                    int from,
                                    int to) const
{
    QWT_DC(d);
    QPolygonF points;

    const QwtRoundF roundF { d->devicePixelRatio };

    if (d->flags & WeedOutPoints) {
        if (d->flags & RoundPoints) {
            if (d->boundingRect.isValid()) {
                points = qwtToPointsFilteredF(d->boundingRect, xMap, yMap, series, from, to);
            } else {
                // without a bounding rectangle all we can
                // do is to filter out duplicates of
                // consecutive points

                points = qwtToPolylineFilteredF(xMap, yMap, series, from, to, roundF);
            }
        } else {
            // when rounding is not allowed we can't use
            // qwtToPointsFilteredF

            points = qwtToPolylineFilteredF(xMap, yMap, series, from, to, QwtNoRoundF());
        }
    } else {
        if (d->flags & RoundPoints) {
            points = qwtToPointsF(d->boundingRect, xMap, yMap, series, from, to, roundF);
        } else {
            points = qwtToPointsF(d->boundingRect, xMap, yMap, series, from, to, QwtNoRoundF());
        }
    }

    return points;
}

/**
 * @brief Translate a series of points into a QPolygon (scattered points)
 *
 * @details
 * - WeedOutPoints & boundingRect().isValid():
 *   All points that are mapped to the same position will be one point.
 *   Points outside of the bounding rectangle are ignored.
 * - WeedOutPoints & !boundingRect().isValid():
 *   All consecutive points that are mapped to the same position will one point.
 * - !WeedOutPoints & boundingRect().isValid():
 *   Points outside of the bounding rectangle are ignored.
 *
 * @param[in] xMap x map
 * @param[in] yMap y map
 * @param[in] series Series of points to be mapped
 * @param[in] from Index of the first point to be painted
 * @param[in] to Index of the last point to be painted
 *
 * @return Translated polygon
 *
 */
QPolygon QwtPointMapper::toPoints(const QwtScaleMap& xMap,
                                  const QwtScaleMap& yMap,
                                  const QwtSeriesData< QPointF >* series,
                                  int from,
                                  int to) const
{
    QWT_DC(d);
    QPolygon points;

    if (d->flags & WeedOutPoints) {
        if (d->boundingRect.isValid()) {
            points = qwtToPointsFilteredI(d->boundingRect, xMap, yMap, series, from, to);
        } else {
            // when we don't have the bounding rectangle all
            // we can do is to filter out consecutive duplicates

            points = qwtToPolylineFilteredI(xMap, yMap, series, from, to);
        }
    } else {
        points = qwtToPointsI(d->boundingRect, xMap, yMap, series, from, to);
    }

    return points;
}

/**
 * @brief Translate a series into a QImage
 *
 * @param[in] xMap x map
 * @param[in] yMap y map
 * @param[in] series Series of points to be mapped
 * @param[in] from Index of the first point to be painted
 * @param[in] to Index of the last point to be painted
 * @param[in] pen Pen used for drawing a point of the image, where a point is mapped to
 * @param[in] antialiased True when the dots should be displayed antialiased
 * @param[in] numThreads Number of threads to be used for rendering.
 *                       If numThreads is set to 0, the system specific ideal thread count is used.
 *
 * @return Image displaying the series
 *
 */
QImage QwtPointMapper::toImage(const QwtScaleMap& xMap,
                               const QwtScaleMap& yMap,
                               const QwtSeriesData< QPointF >* series,
                               int from,
                               int to,
                               const QPen& pen,
                               bool antialiased,
                               uint numThreads) const
{
    QWT_DC(d);
    Q_UNUSED(antialiased)

#if QWT_USE_THREADS
    if (numThreads == 0)
        numThreads = QThread::idealThreadCount();

    if (numThreads <= 0)
        numThreads = 1;
#else
    Q_UNUSED(numThreads)
#endif

    // a very special optimization for scatter plots
    // where every sample is mapped to one pixel only.

    const QRect rect = d->boundingRect.toAlignedRect();

    QImage image(rect.size(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    if (pen.width() <= 1 && pen.color().alpha() == 255) {
        QwtDotsCommand command;
        command.series = series;
        command.rgb    = pen.color().rgba();

#if QWT_USE_THREADS
        const int numPoints = (to - from + 1) / numThreads;

        QList< QFuture< void > > futures;
        for (uint i = 0; i < numThreads; i++) {
            const QPoint pos = rect.topLeft();

            const int index0 = from + i * numPoints;
            if (i == numThreads - 1) {
                command.from = index0;
                command.to   = to;

                qwtRenderDots(xMap, yMap, command, pos, &image);
            } else {
                command.from = index0;
                command.to   = index0 + numPoints - 1;

                futures += QtConcurrent::run(&qwtRenderDots, xMap, yMap, command, pos, &image);
            }
        }
        for (int i = 0; i < futures.size(); i++)
            futures[ i ].waitForFinished();
#else
        command.from = from;
        command.to   = to;

        qwtRenderDots(xMap, yMap, command, rect.topLeft(), &image);
#endif
    } else {
        // fallback implementation: to be replaced later by
        // setting the pixels of the image like above, TODO ...

        QPainter painter(&image);
        painter.setPen(pen);
        painter.setRenderHint(QPainter::Antialiasing, antialiased);

        const int chunkSize = 1000;
        for (int i = from; i <= to; i += chunkSize) {
            const int indexTo     = qMin(i + chunkSize - 1, to);
            const QPolygon points = toPoints(xMap, yMap, series, i, indexTo);

            painter.drawPoints(points);
        }
    }

    return image;
}
