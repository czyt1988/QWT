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

#include "qwt_math.h"
#if QT_VERSION >= 0x050a00
#include <qrandom.h>
#endif

/*!
   \brief Normalize an angle to be int the range [0.0, 2 * PI[
   \param radians Angle in radians
   \return Normalized angle in radians
 */
double qwtNormalizeRadians( double radians )
{
    double a = std::fmod( radians, 2.0 * M_PI );
    if ( a < 0.0 )
        a += 2.0 * M_PI;

    return a;

}

/*!
   \brief Normalize an angle to be int the range [0.0, 360.0[
   \param radians Angle in degrees
   \return Normalized angle in degrees
 */
double qwtNormalizeDegrees( double degrees )
{
    double a = std::fmod( degrees, 360.0 );
    if ( a < 0.0 )
        a += 360.0;

    return a;
}

/*!
   \brief Uses QRandomGenerator for Qt >= 5.10 and qRand() otherwise
   \return A 32-bit random quantity
 */
quint32 qwtRand()
{
#if QT_VERSION >= 0x050a00
    return QRandomGenerator::global()->generate();
#else
    return static_cast< quint32 >( qrand() ); // [0, RAND_MAX ]
#endif
}
