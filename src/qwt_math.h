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
// stl
#include <iterator>
#include <type_traits>
#include <algorithm>
// qt
#include <QPointF>
#include <QtMath>
// qwt
#include "qwt_global.h"
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

/**
 * @brief 检查浮点数值是否为NaN或无穷大/Check if floating point value is NaN or infinity
 *
 * This function checks whether a floating point value is either NaN (Not a Number)
 * or infinite (positive or negative infinity). It uses std::isfinite() for the check.
 *
 * 此函数检查浮点数值是否为NaN（非数字）或无穷大（正无穷大或负无穷大）。
 * 它使用std::isfinite()进行检查。
 *
 * @tparam T Floating point type (float, double, long double)/浮点数类型（float, double, long double）
 * @param value The value to check/要检查的值
 * @return true if value is NaN or infinite/如果值是NaN或无穷大返回true
 * @return false if value is finite/如果值是有限数返回false
 *
 * @note This overload is enabled only for floating point types/此重载仅对浮点数类型启用
 * @see std::isfinite()
 *
 * @par Example/示例:
 * @code
 * double nan_val = std::numeric_limits<double>::quiet_NaN();
 * double inf_val = std::numeric_limits<double>::infinity();
 * double finite_val = 3.14;
 *
 * bool is_nan_invalid = qwt_is_nan_or_inf(nan_val);     // true
 * bool is_inf_invalid = qwt_is_nan_or_inf(inf_val);     // true
 * bool is_finite_valid = qwt_is_nan_or_inf(finite_val); // false
 * @endcode
 */
template< typename T >
inline typename std::enable_if< std::is_floating_point< T >::value, bool >::type qwt_is_nan_or_inf(const T& value)
{
    return !std::isfinite(value);
}

/**
 * @brief 检查QPointF点是否包含NaN或无穷大坐标/Check if QPointF contains NaN or infinite coordinates
 *
 * This function checks whether either the x or y coordinate of a QPointF
 * is NaN (Not a Number) or infinite. Both coordinates are checked.
 *
 * 此函数检查QPointF的x或y坐标是否为NaN（非数字）或无穷大。两个坐标都会被检查。
 *
 * @param point The QPointF to check/要检查的QPointF
 * @return true if either x or y coordinate is NaN or infinite/如果x或y坐标是NaN或无穷大返回true
 * @return false if both coordinates are finite/如果两个坐标都是有限数返回false
 *
 * @par Example/示例:
 * @code
 * QPointF valid_point(1.0, 2.0);
 * QPointF nan_point(std::numeric_limits<qreal>::quiet_NaN(), 3.0);
 * QPointF inf_point(4.0, std::numeric_limits<qreal>::infinity());
 *
 * bool valid_result = qwt_is_nan_or_inf(valid_point); // false
 * bool nan_result = qwt_is_nan_or_inf(nan_point);     // true
 * bool inf_result = qwt_is_nan_or_inf(inf_point);     // true
 * @endcode
 */
inline bool qwt_is_nan_or_inf(const QPointF& point)
{
    return !std::isfinite(point.x()) || !std::isfinite(point.y());
}

// 默认检查函数 - 用于其他类型
template< typename T >
typename std::enable_if< !std::is_floating_point< T >::value && !std::is_same< T, QPointF >::value,
                         bool >::type inline qwt_is_nan_or_inf(const T& /*value*/)
{
	return false;
}

/**
 * @brief 检查指定迭代器范围内是否存在NaN或Inf值。
 *
 * 检查迭代器范围[first, last)内的元素是否包含NaN或无穷大值。
 * 支持浮点数类型、QPointF等类型。
 *
 * @tparam InputIt 输入迭代器类型
 * @param first 范围的起始迭代器
 * @param last 范围的结束迭代器
 * @return 如果范围内存在至少一个NaN或Inf值，返回true；否则返回false
 *
 * @par Example 使用示例:
 * @code
 * // 检查浮点数数组
 * std::vector<double> data = {1.0, std::numeric_limits<double>::quiet_NaN(), 3.0};
 * bool result = qwtContainsNanOrInf(data.begin(), data.end()); // 返回true
 *
 * // 检查QPointF数组
 * QVector<QPointF> points;
 * points << QPointF(1.0, 2.0) << QPointF(std::numeric_limits<qreal>::infinity(), 3.0);
 * bool result2 = qwtContainsNanOrInf(points.begin(), points.end()); // 返回true
 * @endcode
 */
template< typename InputIt >
inline bool qwtContainsNanOrInf(InputIt first, InputIt last)
{
	// 使用迭代器遍历，对每个元素调用适当的 qwt_is_nan_or_inf 函数
	for (InputIt it = first; it != last; ++it) {
		if (qwt_is_nan_or_inf(*it)) {
			return true;
		}
	}
	return false;
}

/**
 * @brief 就地清理数组，移除NaN和Inf值/In-place clean array, removing NaN and Inf values
 *
 * Alternative implementation using manual loop for maximum control.
 * 适用于需要最大控制权的替代实现。
 *
 * @tparam Container 容器类型/Container type
 * @param container 要清理的容器/Container to clean
 *
 * @par Example/示例:
 * @code
 * std::vector<float> data = {1.0f, std::numeric_limits<float>::quiet_NaN(), 3.0f};
 * qwtRemoveNanOrInf(data);
 * // data 现在包含 {1.0f, 3.0f}
 * @endcode
 */
template< typename Container >
inline std::size_t qwtRemoveNanOrInf(Container& container)
{
	// 使用 std::remove_if 算法将需要保留的元素移动到容器前部
	auto new_end = std::remove_if(container.begin(), container.end(), [](const typename Container::value_type& value) {
		return qwt_is_nan_or_inf(value);
	});

	// 计算被删除的元素数量
	std::size_t removed_count = std::distance(new_end, container.end());

	// 实际删除不需要的元素
	if (removed_count > 0) {
		container.erase(new_end, container.end());
	}

	return removed_count;
}

/**
 * @brief 从容器中删除所有 NaN 或 Inf 值（返回新容器）
 * @tparam Container 容器类型
 * @param container 要处理的容器
 * @return 返回不包含 NaN 或 Inf 值的新容器
 */
template< typename Container >
inline Container qwtRemoveNanOrInfCopy(const Container& container)
{
	Container result;
	result.reserve(container.size());  // 预分配空间以提高效率

	// 只复制不是 NaN 或 Inf 的元素
	std::copy_if(container.begin(),
                 container.end(),
                 std::back_inserter(result),
                 [](const typename Container::value_type& value) { return !qwt_is_nan_or_inf(value); });

	return result;
}
#endif
