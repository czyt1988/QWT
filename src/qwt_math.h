/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_MATH_H
#define QWT_MATH_H

#include "qwt_global.h"

#include <QPointF>
#include <QtMath>
/*
   Microsoft says:

   Define _USE_MATH_DEFINES before including math.h to expose these macro
   definitions for common math constants.  These are placed under an #ifdef
   since these commonly-defined names are not part of the C/C++ standards.
 */

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#define undef_USE_MATH_DEFINES
#endif

#include <cmath>

#ifdef undef_USE_MATH_DEFINES
#undef _USE_MATH_DEFINES
#undef undef_USE_MATH_DEFINES
#endif

#ifndef M_E
#define M_E (2.7182818284590452354)
#endif

#ifndef M_LOG2E
#define M_LOG2E (1.4426950408889634074)
#endif

#ifndef M_LOG10E
#define M_LOG10E (0.43429448190325182765)
#endif

#ifndef M_LN2
#define M_LN2 (0.69314718055994530942)
#endif

#ifndef M_LN10
#define M_LN10 (2.30258509299404568402)
#endif

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#ifndef M_PI_2
#define M_PI_2 (1.57079632679489661923)
#endif

#ifndef M_PI_4
#define M_PI_4 (0.78539816339744830962)
#endif

#ifndef M_1_PI
#define M_1_PI (0.31830988618379067154)
#endif

#ifndef M_2_PI
#define M_2_PI (0.63661977236758134308)
#endif

#ifndef M_2_SQRTPI
#define M_2_SQRTPI (1.12837916709551257390)
#endif

#ifndef M_SQRT2
#define M_SQRT2 (1.41421356237309504880)
#endif

#ifndef M_SQRT1_2
#define M_SQRT1_2 (0.70710678118654752440)
#endif

#if defined(QT_WARNING_PUSH)
/*
    early Qt versions not having QT_WARNING_PUSH is full of warnings
    so that we do not care of suppressing those from below
 */
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wdouble-promotion")
QT_WARNING_DISABLE_GCC("-Wdouble-promotion")
#endif

/*
    On systems, where qreal is a float you often run into
    compiler issues with qMin/qMax.
 */

//! \return Minimum of a and b.
QWT_CONSTEXPR inline float qwtMinF(float a, float b)
{
    return (a < b) ? a : b;
}

//! \return Minimum of a and b.
QWT_CONSTEXPR inline double qwtMinF(double a, double b)
{
    return (a < b) ? a : b;
}

//! \return Minimum of a and b.
QWT_CONSTEXPR inline qreal qwtMinF(float a, double b)
{
    return (a < b) ? a : b;
}

//! \return Minimum of a and b.
QWT_CONSTEXPR inline qreal qwtMinF(double a, float b)
{
    return (a < b) ? a : b;
}

//! \return Maximum of a and b.
QWT_CONSTEXPR inline float qwtMaxF(float a, float b)
{
    return (a < b) ? b : a;
}

//! \return Maximum of a and b.
QWT_CONSTEXPR inline double qwtMaxF(double a, double b)
{
    return (a < b) ? b : a;
}

//! \return Maximum of a and b.
QWT_CONSTEXPR inline qreal qwtMaxF(float a, double b)
{
    return (a < b) ? b : a;
}

//! \return Maximum of a and b.
QWT_CONSTEXPR inline qreal qwtMaxF(double a, float b)
{
    return (a < b) ? b : a;
}

#if defined(QT_WARNING_POP)
QT_WARNING_POP
#endif

QWT_EXPORT double qwtNormalizeRadians(double radians);
QWT_EXPORT double qwtNormalizeDegrees(double degrees);
QWT_EXPORT quint32 qwtRand();

/*!
   \brief Compare 2 values, relative to an interval

   Values are "equal", when :
   \f$\cdot value2 - value1 <= abs(intervalSize * 10e^{-6})\f$

   \param value1 First value to compare
   \param value2 Second value to compare
   \param intervalSize interval size

   \return 0: if equal, -1: if value2 > value1, 1: if value1 > value2
 */
inline int qwtFuzzyCompare(double value1, double value2, double intervalSize)
{
    const double eps = qAbs(1.0e-6 * intervalSize);

    if (value2 - value1 > eps)
        return -1;

    if (value1 - value2 > eps)
        return 1;

    return 0;
}

//! Return the sign
inline int qwtSign(double x)
{
    if (x > 0.0)
        return 1;
    else if (x < 0.0)
        return (-1);
    else
        return 0;
}

//! Return the square of a number
inline double qwtSqr(double x)
{
    return x * x;
}

//! Approximation of arc tangent ( error below 0,005 radians )
inline double qwtFastAtan(double x)
{
    if (x < -1.0)
        return -M_PI_2 - x / (x * x + 0.28);

    if (x > 1.0)
        return M_PI_2 - x / (x * x + 0.28);

    return x / (1.0 + x * x * 0.28);
}

//! Approximation of arc tangent ( error below 0,005 radians )
inline double qwtFastAtan2(double y, double x)
{
    if (x > 0)
        return qwtFastAtan(y / x);

    if (x < 0) {
        const double d = qwtFastAtan(y / x);
        return (y >= 0) ? d + M_PI : d - M_PI;
    }

    if (y < 0.0)
        return -M_PI_2;

    if (y > 0.0)
        return M_PI_2;

    return 0.0;
}

/* !
   \brief Calculate a value of a cubic polynomial

   \param x Value
   \param a Cubic coefficient
   \param b Quadratic coefficient
   \param c Linear coefficient
   \param d Constant offset

   \return Value of the polyonom for x
 */
inline double qwtCubicPolynomial(double x, double a, double b, double c, double d)
{
    return (((a * x) + b) * x + c) * x + d;
}

//! Translate degrees into radians
inline double qwtRadians(double degrees)
{
    return degrees * M_PI / 180.0;
}

//! Translate radians into degrees
inline double qwtDegrees(double degrees)
{
    return degrees * 180.0 / M_PI;
}

/*!
    The same as qCeil, but avoids including qmath.h
    \return Ceiling of value.
 */
inline int qwtCeil(qreal value)
{
    using std::ceil;
    return int(ceil(value));
}
/*!
    The same as qFloor, but avoids including qmath.h
    \return Floor of value.
 */
inline int qwtFloor(qreal value)
{
    using std::floor;
    return int(floor(value));
}

/**
 * @brief 验证并调整数组索引范围
 *
 * 确保给定的索引范围在有效范围内，并返回实际有效的元素个数。
 * 该函数会自动修正无效的索引值，确保i1 <= i2。
 *
 * 这个函数通常用于数组或容器的范围检查，确保访问不会越界
 *
 * @param size 数组的总大小
 * @param i1 起始索引（会被修正到有效范围内）
 * @param i2 结束索引（会被修正到有效范围内）
 * @return 返回有效范围内的元素个数，如果数组大小无效则返回0
 *
 * @details 处理逻辑如下：
 * 1. 如果数组大小小于1，直接返回0
 * 2. 使用qBound将i1和i2限制在[0, size-1]范围内
 * 3. 如果i1 > i2，则交换两个值确保i1 <= i2
 * 4. 返回范围内的元素个数(i2 - i1 + 1)
 *
 *
 *
 */
inline int qwtVerifyRange(int size, int& i1, int& i2)
{
    if (size < 1)
        return 0;

    i1 = qBound(0, i1, size - 1);
    i2 = qBound(0, i2, size - 1);

    if (i1 > i2)
        qSwap(i1, i2);

    return (i2 - i1 + 1);
}

/**
 * @brief 求距离
 * @param p1
 * @param p2
 * @return
 */
inline double qwtDistance(const QPointF& p1, const QPointF& p2)
{
    double dx = p2.x() - p1.x();
    double dy = p2.y() - p1.y();
    return qSqrt(dx * dx + dy * dy);
}
#endif
