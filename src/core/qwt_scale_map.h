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

#ifndef QWT_SCALE_MAP_H
#define QWT_SCALE_MAP_H

#include "qwtcore_global.h"
#include "qwt_transform.h"

class QPointF;
class QRectF;

/**
 * @brief A scale map
 * @details QwtScaleMap offers transformations from the coordinate system
 *          of a scale into the linear coordinate system of a paint device
 *          and vice versa.
 * @sa QwtTransform, QwtScaleDiv
 */
class QWTCORE_EXPORT QwtScaleMap
{
public:
    //! Default constructor
    QwtScaleMap();
    //! Copy constructor
    QwtScaleMap(const QwtScaleMap&);
    //! Move constructor
    QwtScaleMap(QwtScaleMap&&);

    //! Destructor
    ~QwtScaleMap();

    //! Copy assignment operator
    QwtScaleMap& operator=(const QwtScaleMap&);
    //! Move assignment operator
    QwtScaleMap& operator=(QwtScaleMap&&);

    //! Set the transformation (takes ownership)
    void setTransformation(QwtTransform*);
    //! Return the transformation
    const QwtTransform* transformation() const;

    //! Set the paint device interval boundaries
    void setPaintInterval(double p1, double p2);
    //! Set the scale interval boundaries
    void setScaleInterval(double s1, double s2);

    //! Transform a scale value to paint device coordinate
    double transform(double s) const;
    //! Transform a paint device coordinate to scale value
    double invTransform(double p) const;

    //! Return first border of paint interval
    Q_DECL_CONSTEXPR double p1() const noexcept { return m_p1; }
    //! Return second border of paint interval
    Q_DECL_CONSTEXPR double p2() const noexcept { return m_p2; }

    //! Return first border of scale interval
    Q_DECL_CONSTEXPR double s1() const noexcept { return m_s1; }
    //! Return second border of scale interval
    Q_DECL_CONSTEXPR double s2() const noexcept { return m_s2; }

    //! Return distance between paint interval boundaries
    double pDist() const { return qAbs(m_p2 - m_p1); }
    //! Return distance between scale interval boundaries
    double sDist() const { return qAbs(m_s2 - m_s1); }

    //! Transform a rectangle from scale to paint coordinates
    static QRectF transform(const QwtScaleMap&, const QwtScaleMap&, const QRectF&);

    //! Transform a rectangle from paint to scale coordinates
    static QRectF invTransform(const QwtScaleMap&, const QwtScaleMap&, const QRectF&);

    //! Transform a point from scale to paint coordinates
    static QPointF transform(const QwtScaleMap&, const QwtScaleMap&, const QPointF&);

    //! Transform a point from paint to scale coordinates
    static QPointF invTransform(const QwtScaleMap&, const QwtScaleMap&, const QPointF&);

    //! Check if the scale is linear (no transformation)
    static bool isLinerScale(const QwtScaleMap& sm);

    //! Check if this scale has no nonlinear transformation
    Q_DECL_CONSTEXPR bool isLinear() const noexcept { return m_transform == nullptr; }

    //! Check if the mapping direction is inverted
    Q_DECL_CONSTEXPR bool isInverting() const noexcept { return ((m_p1 < m_p2) != (m_s1 < m_s2)); }

    //! Conversion factor for linear fast-path: result = p1() + (value - ts1()) * cnv()
    Q_DECL_CONSTEXPR double cnv() const noexcept { return m_cnv; }

    //! Transformed scale origin for linear fast-path
    Q_DECL_CONSTEXPR double ts1() const noexcept { return m_ts1; }

protected:
    void swap(QwtScaleMap& other) noexcept;  // helper
private:
    void updateFactor();

    double m_s1 { 0.0 }, m_s2 { 1.0 };  // scale interval boundaries
    double m_p1 { 0.0 }, m_p2 { 1.0 };  // paint device interval boundaries

    double m_cnv { 1.0 };  // conversion factor
    double m_ts1 { 0.0 };

    QwtTransform* m_transform { nullptr };
};

/**
 * @brief Transform a point related to the scale interval into a point related to the paint device interval
 * @param s Value relative to the coordinates of the scale
 * @return Transformed value
 * @sa invTransform()
 */
inline double QwtScaleMap::transform(double s) const
{
    if (m_transform)
        s = m_transform->transform(s);

    return m_p1 + (s - m_ts1) * m_cnv;
}

/**
 * @brief Transform a paint device value into a value in the interval of the scale
 * @param p Value relative to the coordinates of the paint device
 * @return Transformed value
 * @sa transform()
 */
inline double QwtScaleMap::invTransform(double p) const
{
    double s = m_ts1 + (p - m_p1) / m_cnv;
    if (m_transform)
        s = m_transform->invTransform(s);

    return s;
}

#ifndef QT_NO_DEBUG_STREAM
QWTCORE_EXPORT QDebug operator<<(QDebug, const QwtScaleMap&);
#endif

#endif
