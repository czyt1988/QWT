/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_CURVE_SMOOTHING_H
#define QWT_CURVE_SMOOTHING_H

#include <qpolygon.h>

/*
    Internal helpers implementing the render smoothing algorithms
    of QwtPlotCurve.

    They operate on the polyline that has already been translated to paint
    coordinates and has been downscaled ( f.e. by the FilterPointsLTTB mode ).
    Only the y coordinates are modified, the x coordinates are copied, so that
    the horizontal position of the curve stays untouched.

    Both algorithms combine a linear low pass filter with a feature
    preservation step: points that deviate from a wide baseline by more
    than preserveThreshold times the estimated noise level ( peaks, dips )
    keep their original position and height, so that the smoothing removes
    the pixel noise without distorting the significant features.
    A preserveThreshold <= 0 disables the feature preservation.

    The functions are stateless and reentrant. They are not part of the
    public API - the public interface is QwtPlotCurve::setSmoothAlgorithm().
 */

// Smooth the y coordinates with a Gaussian weighted moving window
QPolygonF qwtSmoothPolylineGaussian(const QPolygonF& polyline, int window, double preserveThreshold);

// Smooth the y coordinates with a Savitzky-Golay local polynomial regression
QPolygonF qwtSmoothPolylineSavitzkyGolay(const QPolygonF& polyline, int window, int order, double preserveThreshold);

#endif
