/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_curve_smoothing.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
    // Force a window size to be odd and >= 3
    inline int qwtOddWindow(int window)
    {
        if (window < 3)
            window = 3;
        else if (window % 2 == 0)
            window++;

        return window;
    }

    /*!
       Map an index outside [lo, hi) back into the interval by mirroring
       it at the interval boundaries ( no duplicated boundary point ).
     */
    inline int qwtReflectIndex(int index, int lo, int hi)
    {
        while (index < lo || index >= hi) {
            if (index < lo)
                index = 2 * lo - index;
            else
                index = 2 * (hi - 1) - index;
        }

        return index;
    }

    /*!
       Convolve the y coordinates of polyline[lo, hi) with a normalized
       kernel of odd size, using mirrored samples at the interval borders.
       The x coordinates are copied from the input polyline.
     */
    void qwtConvolveSegment(const QPolygonF& polyline, int lo, int hi,
                            const QVector< double >& kernel, QPolygonF& result)
    {
        const int m = kernel.size() / 2;

        for (int i = lo; i < hi; i++) {
            double acc = 0.0;

            for (int j = -m; j <= m; j++) {
                const int index = qwtReflectIndex(i + j, lo, hi);
                acc += kernel[j + m] * polyline[index].y();
            }

            result[i] = QPointF(polyline[i].x(), acc);
        }
    }

    /*!
       Split the polyline at points with non finite coordinates and convolve
       each segment on its own. Segments shorter than the kernel are passed
       through unchanged, so that small fragments don't get distorted.
     */
    QPolygonF qwtConvolvePolyline(const QPolygonF& polyline, const QVector< double >& kernel)
    {
        QPolygonF result = polyline;

        const int n = polyline.size();
        const int window = kernel.size();

        int lo = -1;
        for (int i = 0; i <= n; i++) {
            const bool isFinite = (i < n) && std::isfinite(polyline[i].x()) && std::isfinite(polyline[i].y());

            if (isFinite) {
                if (lo < 0)
                    lo = i;
            } else {
                if (lo >= 0 && i - lo >= window)
                    qwtConvolveSegment(polyline, lo, i, kernel, result);

                lo = -1;
            }
        }

        return result;
    }

    // Gaussian kernel, where sigma is a quarter of the window size,
    // so that the window spans about +/- 2 sigma
    QVector< double > qwtGaussianKernel(int window)
    {
        const int m = window / 2;
        const double sigma = qMax(1.0, 0.25 * window);

        QVector< double > kernel(window);
        double sum = 0.0;

        for (int j = -m; j <= m; j++) {
            const double w = std::exp(-0.5 * (j * j) / (sigma * sigma));
            kernel[j + m] = w;
            sum += w;
        }

        for (int i = 0; i < window; i++)
            kernel[i] /= sum;

        return kernel;
    }

    // Solve a n x n linear system in place, matrix is row major
    bool qwtSolveLinearSystem(double* matrix, double* rhs, int n)
    {
        for (int k = 0; k < n; k++) {
            int pivot = k;
            for (int i = k + 1; i < n; i++) {
                if (qAbs(matrix[i * n + k]) > qAbs(matrix[pivot * n + k]))
                    pivot = i;
            }

            if (qAbs(matrix[pivot * n + k]) < 1.0e-12)
                return false;

            if (pivot != k) {
                for (int j = 0; j < n; j++)
                    std::swap(matrix[k * n + j], matrix[pivot * n + j]);

                std::swap(rhs[k], rhs[pivot]);
            }

            for (int i = k + 1; i < n; i++) {
                const double factor = matrix[i * n + k] / matrix[k * n + k];

                for (int j = k; j < n; j++)
                    matrix[i * n + j] -= factor * matrix[k * n + j];

                rhs[i] -= factor * rhs[k];
            }
        }

        for (int i = n - 1; i >= 0; i--) {
            for (int j = i + 1; j < n; j++)
                rhs[i] -= matrix[i * n + j] * rhs[j];

            rhs[i] /= matrix[i * n + i];
        }

        return true;
    }

    /*!
       Convolution coefficients of a Savitzky-Golay filter, that evaluates
       the locally fitted polynomial of the given order at the window center.

       The samples are considered to be equidistant, what is approximately
       true for a polyline that has been reduced to paint coordinates.
     */
    QVector< double > qwtSavitzkyGolayKernel(int window, int order)
    {
        const int m = window / 2;
        const int np = order + 1;

        // moments[i] = sum( ( j - m )^i ), for i = 0 .. 2 * order
        QVector< double > moments(2 * order + 1, 0.0);

        for (int j = -m; j <= m; j++) {
            double v = 1.0;
            for (int i = 0; i < 2 * order + 1; i++) {
                moments[i] += v;
                v *= j;
            }
        }

        // normal equations of the local least squares fit, where the
        // solution is the polynomial coefficients of the window center
        QVector< double > matrix(np * np, 0.0);
        QVector< double > rhs(np, 0.0);
        rhs[0] = 1.0;

        for (int i = 0; i < np; i++) {
            for (int k = 0; k < np; k++)
                matrix[i * np + k] = moments[i + k];
        }

        if (!qwtSolveLinearSystem(matrix.data(), rhs.data(), np))
            return QVector< double >();

        QVector< double > kernel(window);

        for (int j = -m; j <= m; j++) {
            double v = 1.0;
            double c = 0.0;

            for (int i = 0; i < np; i++) {
                c += rhs[i] * v;
                v *= j;
            }

            kernel[j + m] = c;
        }

        return kernel;
    }

    /*!
       Moving average of the y coordinates, computed with prefix sums, so
       that the cost does not depend on the window size. The window is
       truncated ( and renormalized ) at the segment borders.
     */
    QPolygonF qwtBoxSmoothPolyline(const QPolygonF& polyline, int window)
    {
        QPolygonF result = polyline;

        const int n = polyline.size();
        const int m = window / 2;

        int lo = -1;
        for (int i = 0; i <= n; i++) {
            const bool isFinite = (i < n) && std::isfinite(polyline[i].x()) && std::isfinite(polyline[i].y());

            if (isFinite) {
                if (lo < 0)
                    lo = i;
                continue;
            }

            if (lo < 0)
                continue;

            const int hi = i;

            if (hi - lo >= window) {
                // prefix sums over the segment
                QVector< double > sums(hi - lo + 1, 0.0);
                double acc = 0.0;
                for (int k = lo; k < hi; k++) {
                    acc += polyline[k].y();
                    sums[k - lo] = acc;
                }

                for (int k = lo; k < hi; k++) {
                    const int a = qMax(lo, k - m);
                    const int b = qMin(hi - 1, k + m);

                    const double rangeSum = sums[b - lo] - (a > lo ? sums[a - 1 - lo] : 0.0);
                    result[k].setY(rangeSum / (b - a + 1));
                }
            }

            lo = -1;
        }

        return result;
    }

    /*!
       Restore the core points ( peaks and dips ) of the original polyline
       on top of a linearly smoothed polyline.

       A linear low pass filter always attenuates features that are narrower
       than its window. To avoid this distortion the deviation of the original
       points from a wide baseline ( that follows the overall shape but not
       the narrow features ) is compared with a robust estimate of the noise
       level. Points deviating by more than threshold * noise are considered
       to be real features and keep their original position and height,
       points below are taken from the smoothed polyline. The transition
       between both regions is blended.
     */
    QPolygonF qwtPreserveFeatures(const QPolygonF& polyline, const QPolygonF& smoothed,
                                  double threshold, int baselineWindow)
    {
        const int n = polyline.size();

        // two box passes approximate a Gaussian baseline, that follows the
        // overall shape but not the narrow features, in O(n)
        QPolygonF baseline = qwtBoxSmoothPolyline(polyline, baselineWindow);
        baseline = qwtBoxSmoothPolyline(baseline, baselineWindow);

        // robust noise estimate: median of the absolute deviation to the
        // baseline ( MAD ), what is not affected by the features themselves
        QVector< double > deviation(n, 0.0);
        QVector< double > absDeviation;
        absDeviation.reserve(n);

        for (int i = 0; i < n; i++) {
            if (std::isfinite(polyline[i].x()) && std::isfinite(polyline[i].y())) {
                deviation[i] = polyline[i].y() - baseline[i].y();
                absDeviation += qAbs(deviation[i]);
            }
        }

        if (absDeviation.isEmpty())
            return smoothed;

        const int mid = absDeviation.size() / 2;
        std::nth_element(absDeviation.begin(), absDeviation.begin() + mid, absDeviation.end());

        // robust scale of the noise: the median deviation of the points from
        // the baseline, what corresponds to the typical extent of the noise
        // band ( the features themselves do not influence the median )
        const double noiseScale = absDeviation[mid];
        if (noiseScale <= 0.0)
            return smoothed;

        const double lo = threshold * noiseScale;
        const double hi = 1.2 * lo;

        QPolygonF result = smoothed;

        for (int i = 0; i < n; i++) {
            const double d = qAbs(deviation[i]);
            if (d <= lo)
                continue;

            const double w = (d >= hi) ? 1.0 : (d - lo) / (hi - lo);
            result[i].setY(smoothed[i].y() + w * (polyline[i].y() - smoothed[i].y()));
        }

        return result;
    }
}

// Smoothing the y coordinates with a Gaussian weighted moving window
QPolygonF qwtSmoothPolylineGaussian(const QPolygonF& polyline, int window, double preserveThreshold)
{
    window = qwtOddWindow(window);

    if (polyline.size() <= window)
        return polyline;

    const QPolygonF smoothed = qwtConvolvePolyline(polyline, qwtGaussianKernel(window));

    if (preserveThreshold <= 0.0)
        return smoothed;

    // the baseline must be wide enough to average out the features, that
    // shall be preserved
    return qwtPreserveFeatures(polyline, smoothed, preserveThreshold, qwtOddWindow(qMax(61, 6 * window)));
}

// Smoothing the y coordinates with a Savitzky-Golay local polynomial regression
QPolygonF qwtSmoothPolylineSavitzkyGolay(const QPolygonF& polyline, int window, int order, double preserveThreshold)
{
    window = qwtOddWindow(window);

    // an order >= window - 1 would do an exact interpolation instead of smoothing
    order = qBound(1, order, window - 2);

    if (polyline.size() <= window)
        return polyline;

    const QVector< double > kernel = qwtSavitzkyGolayKernel(window, order);
    if (kernel.isEmpty())
        return polyline;

    const QPolygonF smoothed = qwtConvolvePolyline(polyline, kernel);

    if (preserveThreshold <= 0.0)
        return smoothed;

    return qwtPreserveFeatures(polyline, smoothed, preserveThreshold, qwtOddWindow(qMax(61, 6 * window)));
}
