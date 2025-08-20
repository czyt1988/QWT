#ifndef QWTPLOT_AMALGAM_H
#define QWTPLOT_AMALGAM_H

/*** Start of inlined file: QWTAmalgamTemplatePublicHeaders.h ***/
// 通用定义

/*** Start of inlined file: qwt_global.h ***/
#ifndef QWT_GLOBAL_H
#define QWT_GLOBAL_H

#include <qglobal.h>

// QWT_VERSION is (major << 16) + (minor << 8) + patch.

#define QWT_VERSION 0x070001
#define QWT_VERSION_STR "7.0.1"

#if defined(_MSC_VER) /* MSVC Compiler */
/* template-class specialization 'identifier' is already instantiated */
#pragma warning(disable : 4660)
/* inherits via dominance */
#pragma warning(disable : 4250)
#endif  // _MSC_VER

#ifdef QWT_DLL

#if defined(QWT_MAKEDLL)  // create a Qwt DLL library
#define QWT_EXPORT Q_DECL_EXPORT
#else  // use a Qwt DLL library
#define QWT_EXPORT Q_DECL_IMPORT
#endif

#endif  // QWT_DLL

#ifndef QWT_EXPORT
#define QWT_EXPORT
#endif

#define QWT_CONSTEXPR Q_DECL_CONSTEXPR

#if QT_VERSION >= 0x050000
#define QWT_OVERRIDE Q_DECL_OVERRIDE
#define QWT_FINAL Q_DECL_FINAL
#endif

#ifndef QWT_CONSTEXPR
#define QWT_CONSTEXPR
#endif

#ifndef QWT_OVERRIDE
#define QWT_OVERRIDE
#endif

#ifndef QWT_FINAL
#define QWT_FINAL
#endif

#endif

/*** End of inlined file: qwt_global.h ***/

/*** Start of inlined file: qwt.h ***/
#ifndef QWT_H
#define QWT_H

class QSize;

/*!
   Some constants for use within Qwt.
 */
namespace Qwt
{
}

QWT_EXPORT QSize qwtExpandedToGlobalStrut(const QSize&);

#endif

/*** End of inlined file: qwt.h ***/

/*** Start of inlined file: qwt_polar.h ***/
#ifndef QWT_POLAR_H
#define QWT_POLAR_H

namespace QwtPolar
{
//! Unit of an angle
enum AngleUnit
{
    //! 0.0 -> 2_M_PI
    Radians,

    //! 0.0 -> 360.0
    Degrees,

    //! 0.0 - 400.0
    Gradians,

    //! 0.0 - 1.0
    Turns
};

//! An enum, that identifies the type of a coordinate
enum Coordinate
{
    //! Azimuth
    Azimuth,

    //! Radius
    Radius
};

/*!
   Indices used to identify an axis.
   \sa Scale
 */
enum Axis
{
    //! Azimuth axis
    AxisAzimuth,

    //! Left axis
    AxisLeft,

    //! Right axis
    AxisRight,

    //! Top axis
    AxisTop,

    //! Bottom axis
    AxisBottom,

    //! Number of available axis
    AxesCount
};

/*!
   Indices used to identify a scale.
   \sa Axis
 */
enum Scale
{
    //! Azimuth scale
    ScaleAzimuth = Azimuth,

    //! Radial scale
    ScaleRadius = Radius,

    //! Number of scales
    ScaleCount
};
}

#endif

/*** End of inlined file: qwt_polar.h ***/

/*** Start of inlined file: qwt_math.h ***/
#ifndef QWT_MATH_H
#define QWT_MATH_H

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

/*** End of inlined file: qwt_math.h ***/

/*** Start of inlined file: qwt_interval.h ***/
#ifndef QWT_INTERVAL_H
#define QWT_INTERVAL_H

#include <qmetatype.h>

/*!
   \brief A class representing an interval

   The interval is represented by 2 doubles, the lower and the upper limit.
 */

class QWT_EXPORT QwtInterval
{
public:
	/*!
	   Flag indicating if a border is included or excluded
	   \sa setBorderFlags(), borderFlags()
	 */
	enum BorderFlag
	{
		//! Min/Max values are inside the interval
		IncludeBorders = 0x00,

		//! Min value is not included in the interval
		ExcludeMinimum = 0x01,

		//! Max value is not included in the interval
		ExcludeMaximum = 0x02,

		//! Min/Max values are not included in the interval
		ExcludeBorders = ExcludeMinimum | ExcludeMaximum
	};

	//! Border flags
    Q_DECLARE_FLAGS(BorderFlags, BorderFlag)

	QwtInterval();
    QwtInterval(double minValue, double maxValue, BorderFlags = IncludeBorders);

    void setInterval(double minValue, double maxValue, BorderFlags = IncludeBorders);

	QwtInterval normalized() const;
	QwtInterval inverted() const;
    QwtInterval limited(double lowerBound, double upperBound) const;

    bool operator==(const QwtInterval&) const;
    bool operator!=(const QwtInterval&) const;

    void setBorderFlags(BorderFlags);
	BorderFlags borderFlags() const;

	double minValue() const;
	double maxValue() const;

	double width() const;
	long double widthL() const;

    void setMinValue(double);
    void setMaxValue(double);

    bool contains(double value) const;
    bool contains(const QwtInterval&) const;

    bool intersects(const QwtInterval&) const;
    QwtInterval intersect(const QwtInterval&) const;
    QwtInterval unite(const QwtInterval&) const;

    QwtInterval operator|(const QwtInterval&) const;
    QwtInterval operator&(const QwtInterval&) const;

    QwtInterval& operator|=(const QwtInterval&);
    QwtInterval& operator&=(const QwtInterval&);

    QwtInterval extend(double value) const;
    QwtInterval operator|(double) const;
    QwtInterval& operator|=(double);

	bool isValid() const;
	bool isNull() const;
	void invalidate();

    QwtInterval symmetrize(double value) const;

private:
	double m_minValue;
	double m_maxValue;
	BorderFlags m_borderFlags;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtInterval::BorderFlags)
Q_DECLARE_METATYPE(QwtInterval)
Q_DECLARE_TYPEINFO(QwtInterval, Q_MOVABLE_TYPE);

/*!
   \brief Default Constructor

   Creates an invalid interval [0.0, -1.0]
   \sa setInterval(), isValid()
 */
inline QwtInterval::QwtInterval() : m_minValue(0.0), m_maxValue(-1.0), m_borderFlags(IncludeBorders)
{
}

/*!
   Constructor

   Build an interval with from min/max values

   \param minValue Minimum value
   \param maxValue Maximum value
   \param borderFlags Include/Exclude borders
 */
inline QwtInterval::QwtInterval(double minValue, double maxValue, BorderFlags borderFlags)
    : m_minValue(minValue), m_maxValue(maxValue), m_borderFlags(borderFlags)
{
}

/*!
   Assign the limits of the interval

   \param minValue Minimum value
   \param maxValue Maximum value
   \param borderFlags Include/Exclude borders
 */
inline void QwtInterval::setInterval(double minValue, double maxValue, BorderFlags borderFlags)
{
    m_minValue    = minValue;
    m_maxValue    = maxValue;
	m_borderFlags = borderFlags;
}

/*!
   Change the border flags

   \param borderFlags Or'd BorderMode flags
   \sa borderFlags()
 */
inline void QwtInterval::setBorderFlags(BorderFlags borderFlags)
{
	m_borderFlags = borderFlags;
}

/*!
   \return Border flags
   \sa setBorderFlags()
 */
inline QwtInterval::BorderFlags QwtInterval::borderFlags() const
{
	return m_borderFlags;
}

/*!
   Assign the lower limit of the interval

   \param minValue Minimum value
 */
inline void QwtInterval::setMinValue(double minValue)
{
	m_minValue = minValue;
}

/*!
   Assign the upper limit of the interval

   \param maxValue Maximum value
 */
inline void QwtInterval::setMaxValue(double maxValue)
{
	m_maxValue = maxValue;
}

//! \return Lower limit of the interval
inline double QwtInterval::minValue() const
{
	return m_minValue;
}

//! \return Upper limit of the interval
inline double QwtInterval::maxValue() const
{
	return m_maxValue;
}

/*!
   A interval is valid when minValue() <= maxValue().
   In case of QwtInterval::ExcludeBorders it is true
   when minValue() < maxValue()

   \return True, when the interval is valid
 */
inline bool QwtInterval::isValid() const
{
    if ((m_borderFlags & ExcludeBorders) == 0)
		return m_minValue <= m_maxValue;
	else
		return m_minValue < m_maxValue;
}

/*!
   \brief Return the width of an interval

   The width of invalid intervals is 0.0, otherwise the result is
   maxValue() - minValue().

   \return Interval width
   \sa isValid()
 */
inline double QwtInterval::width() const
{
    return isValid() ? (m_maxValue - m_minValue) : 0.0;
}

/*!
   \brief Return the width of an interval as long double

   The width of invalid intervals is 0.0, otherwise the result is
   maxValue() - minValue().

   \return Interval width
   \sa isValid()
 */
inline long double QwtInterval::widthL() const
{
    if (!isValid())
		return 0.0;

    return static_cast< long double >(m_maxValue) - static_cast< long double >(m_minValue);
}

/*!
   \brief Intersection of two intervals

   \param other Interval to intersect with
   \return Intersection of this and other

   \sa intersect()
 */
inline QwtInterval QwtInterval::operator&(const QwtInterval& other) const
{
    return intersect(other);
}

/*!
   Union of two intervals

   \param other Interval to unite with
   \return Union of this and other

   \sa unite()
 */
inline QwtInterval QwtInterval::operator|(const QwtInterval& other) const
{
    return unite(other);
}

/*!
   \brief Compare two intervals

   \param other Interval to compare with
   \return True, when this and other are equal
 */
inline bool QwtInterval::operator==(const QwtInterval& other) const
{
    return (m_minValue == other.m_minValue) && (m_maxValue == other.m_maxValue) && (m_borderFlags == other.m_borderFlags);
}
/*!
   \brief Compare two intervals

   \param other Interval to compare with
   \return True, when this and other are not equal
 */
inline bool QwtInterval::operator!=(const QwtInterval& other) const
{
    return (!(*this == other));
}

/*!
   Extend an interval

   \param value Value
   \return Extended interval
   \sa extend()
 */
inline QwtInterval QwtInterval::operator|(double value) const
{
    return extend(value);
}

//! \return true, if isValid() && (minValue() >= maxValue())
inline bool QwtInterval::isNull() const
{
	return isValid() && m_minValue >= m_maxValue;
}

/*!
   Invalidate the interval

   The limits are set to interval [0.0, -1.0]
   \sa isValid()
 */
inline void QwtInterval::invalidate()
{
	m_minValue = 0.0;
	m_maxValue = -1.0;
}

#ifndef QT_NO_DEBUG_STREAM
QWT_EXPORT QDebug operator<<(QDebug, const QwtInterval&);
#endif

#endif

/*** End of inlined file: qwt_interval.h ***/

/*** Start of inlined file: qwt_color_map.h ***/
#ifndef QWT_COLOR_MAP_H
#define QWT_COLOR_MAP_H

#include <qcolor.h>

class QwtInterval;

#if QT_VERSION < 0x060000
template< typename T >
class QVector;
#endif

/*!
   \brief QwtColorMap is used to map values into colors.

   For displaying 3D data on a 2D plane the 3rd dimension is often
   displayed using colors, like f.e in a spectrogram.

   Each color map is optimized to return colors for only one of the
   following image formats:

   - QImage::Format_Indexed8\n
   - QImage::Format_ARGB32\n

   \sa QwtPlotSpectrogram, QwtScaleWidget
 */

class QWT_EXPORT QwtColorMap
{
public:
	/*!
        Format for color mapping
        \sa rgb(), colorIndex(), colorTable()
	 */

	enum Format
	{
		//! The map is intended to map into RGB values.
		RGB,

		/*!
		   Map values into 8 bit values, that
		   are used as indexes into the color table.

		   Indexed color maps are used to generate QImage::Format_Indexed8
		   images. The calculation of the color index is usually faster
		   and the resulting image has a lower memory footprint.

		   \sa colorIndex(), colorTable()
		 */
		Indexed
	};

	explicit QwtColorMap(Format = QwtColorMap::RGB);
	virtual ~QwtColorMap();

	void setFormat(Format);
	Format format() const;

	/*!
	   Map a value of a given interval into a RGB value.

	   \param interval Range for the values
	   \param value Value
	   \return RGB value, corresponding to value
	 */
	virtual QRgb rgb(const QwtInterval& interval, double value) const = 0;

	virtual uint colorIndex(int numColors, const QwtInterval& interval, double value) const;

	QColor color(const QwtInterval&, double value) const;
	virtual QVector< QRgb > colorTable(int numColors) const;
	virtual QVector< QRgb > colorTable256() const;

private:
	Q_DISABLE_COPY(QwtColorMap)

	Format m_format;
};

/*!
   \brief QwtLinearColorMap builds a color map from color stops.

   A color stop is a color at a specific position. The valid
   range for the positions is [0.0, 1.0]. When mapping a value
   into a color it is translated into this interval according to mode().
 */
class QWT_EXPORT QwtLinearColorMap : public QwtColorMap
{
public:
	/*!
	   Mode of color map
	   \sa setMode(), mode()
	 */
	enum Mode
	{
		//! Return the color from the next lower color stop
		FixedColors,

		//! Interpolating the colors of the adjacent stops.
		ScaledColors
	};

	explicit QwtLinearColorMap(QwtColorMap::Format = QwtColorMap::RGB);

	QwtLinearColorMap(const QColor& from, const QColor& to, QwtColorMap::Format = QwtColorMap::RGB);

	virtual ~QwtLinearColorMap();

	void setMode(Mode);
	Mode mode() const;

	void setColorInterval(const QColor& color1, const QColor& color2);
	void addColorStop(double value, const QColor&);
	QVector< double > stopPos() const;
	QVector< QColor > stopColors() const;
	QColor color1() const;
	QColor color2() const;

	virtual QRgb rgb(const QwtInterval&, double value) const QWT_OVERRIDE;

	virtual uint colorIndex(int numColors, const QwtInterval&, double value) const QWT_OVERRIDE;

	class ColorStops;

private:
	class PrivateData;
	PrivateData* m_data;
};

/*!
   \brief QwtAlphaColorMap varies the alpha value of a color
 */
class QWT_EXPORT QwtAlphaColorMap : public QwtColorMap
{
public:
	explicit QwtAlphaColorMap(const QColor& = QColor(Qt::gray));
	virtual ~QwtAlphaColorMap();

	void setAlphaInterval(int alpha1, int alpha2);

	int alpha1() const;
	int alpha2() const;

	void setColor(const QColor&);
	QColor color() const;

	virtual QRgb rgb(const QwtInterval&, double value) const QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

/*!
   \brief QwtHueColorMap varies the hue value of the HSV color model.

   QwtHueColorMap can be used to set up a color map easily, that runs cyclic over
   all colors. Each cycle has 360 different steps.

   The values for value and saturation are in the range of 0 to 255 and doesn't
   depend on the data value to be mapped.

   \sa QwtSaturationValueColorMap
 */
class QWT_EXPORT QwtHueColorMap : public QwtColorMap
{
public:
	explicit QwtHueColorMap(QwtColorMap::Format = QwtColorMap::RGB);
	virtual ~QwtHueColorMap();

	void setHueInterval(int hue1, int hue2);  // direction ?
	void setSaturation(int saturation);
	void setValue(int value);
	void setAlpha(int alpha);

	int hue1() const;
	int hue2() const;
	int saturation() const;
	int value() const;
	int alpha() const;

	virtual QRgb rgb(const QwtInterval&, double value) const QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

/*!
   \brief QwtSaturationValueColorMap varies the saturation and/or value for a given
         hue in the HSV color model.

   Value and saturation are in the range of 0 to 255 while hue is in the range
   of 0 to 259.

   \sa QwtHueColorMap
 */
class QWT_EXPORT QwtSaturationValueColorMap : public QwtColorMap
{
public:
	QwtSaturationValueColorMap();
	virtual ~QwtSaturationValueColorMap();

	void setHue(int hue);
	void setSaturationInterval(int sat1, int sat2);
	void setValueInterval(int value1, int value2);
	void setAlpha(int alpha);

	int hue() const;
	int saturation1() const;
	int saturation2() const;
	int value1() const;
	int value2() const;
	int alpha() const;

	virtual QRgb rgb(const QwtInterval&, double value) const QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

/*!
   Map a value into a color

   \param interval Valid interval for values
   \param value Value

   \return Color corresponding to value
 */
inline QColor QwtColorMap::color(const QwtInterval& interval, double value) const
{
	return QColor::fromRgba(rgb(interval, value));
}

/*!
   \return Intended format of the color map
   \sa Format
 */
inline QwtColorMap::Format QwtColorMap::format() const
{
	return m_format;
}

#endif

/*** End of inlined file: qwt_color_map.h ***/

/*** Start of inlined file: qwt_pixel_matrix.h ***/
#ifndef QWT_PIXEL_MATRIX_H
#define QWT_PIXEL_MATRIX_H

#include <qbitarray.h>
#include <qrect.h>

/*!
   \brief A bit field corresponding to the pixels of a rectangle

   QwtPixelMatrix is intended to filter out duplicates in an
   unsorted array of points.
 */
class QWT_EXPORT QwtPixelMatrix : public QBitArray
{
public:
    explicit QwtPixelMatrix(const QRect& rect);
	~QwtPixelMatrix();

    void setRect(const QRect& rect);
	QRect rect() const;

    bool testPixel(int x, int y) const;
    bool testAndSetPixel(int x, int y, bool on);

    int index(int x, int y) const;

private:
	QRect m_rect;
};

/*!
   \brief Test if a pixel has been set

   \param x X-coordinate
   \param y Y-coordinate

   \return true, when pos is outside of rect(), or when the pixel
          has already been set.
 */
inline bool QwtPixelMatrix::testPixel(int x, int y) const
{
    const int idx = index(x, y);
    return (idx >= 0) ? testBit(idx) : true;
}

/*!
   \brief Set a pixel and test if a pixel has been set before

   \param x X-coordinate
   \param y Y-coordinate
   \param on Set/Clear the pixel

   \return true, when pos is outside of rect(), or when the pixel
          was set before.
 */
inline bool QwtPixelMatrix::testAndSetPixel(int x, int y, bool on)
{
    const int idx = index(x, y);
    if (idx < 0)
		return true;

    const bool onBefore = testBit(idx);
    setBit(idx, on);

	return onBefore;
}

/*!
   \brief Calculate the index in the bit field corresponding to a position

   \param x X-coordinate
   \param y Y-coordinate
   \return Index, when rect() contains pos - otherwise -1.
 */
inline int QwtPixelMatrix::index(int x, int y) const
{
	const int dx = x - m_rect.x();
    if (dx < 0 || dx >= m_rect.width())
		return -1;

	const int dy = y - m_rect.y();
    if (dy < 0 || dy >= m_rect.height())
		return -1;

	return dy * m_rect.width() + dx;
}

#endif

/*** End of inlined file: qwt_pixel_matrix.h ***/

/*** Start of inlined file: qwt_transform.h ***/
#ifndef QWT_TRANSFORM_H
#define QWT_TRANSFORM_H

/*!
   \brief A transformation between coordinate systems

   QwtTransform manipulates values, when being mapped between
   the scale and the paint device coordinate system.

   A transformation consists of 2 methods:

   - transform
   - invTransform

   where one is is the inverse function of the other.

   When p1, p2 are the boundaries of the paint device coordinates
   and s1, s2 the boundaries of the scale, QwtScaleMap uses the
   following calculations:

   - p = p1 + ( p2 - p1 ) * ( T( s ) - T( s1 ) / ( T( s2 ) - T( s1 ) );
   - s = invT ( T( s1 ) + ( T( s2 ) - T( s1 ) ) * ( p - p1 ) / ( p2 - p1 ) );
 */
class QWT_EXPORT QwtTransform
{
public:
	QwtTransform();
	virtual ~QwtTransform();

	/*!
	   Modify value to be a valid value for the transformation.
	   The default implementation does nothing.
	 */
    virtual double bounded(double value) const;

	/*!
        Transformation function

        \param value Value
        \return Modified value

        \sa invTransform()
	 */
    virtual double transform(double value) const = 0;

	/*!
        Inverse transformation function

        \param value Value
        \return Modified value

        \sa transform()
	 */
    virtual double invTransform(double value) const = 0;

	//! Virtualized copy operation
	virtual QwtTransform* copy() const = 0;

private:
	Q_DISABLE_COPY(QwtTransform)
};

/*!
   \brief Null transformation

   QwtNullTransform returns the values unmodified.

 */
class QWT_EXPORT QwtNullTransform : public QwtTransform
{
public:
	QwtNullTransform();
	virtual ~QwtNullTransform();

    virtual double transform(double value) const QWT_OVERRIDE;
    virtual double invTransform(double value) const QWT_OVERRIDE;

	virtual QwtTransform* copy() const QWT_OVERRIDE;
};
/*!
   \brief Logarithmic transformation

   QwtLogTransform modifies the values using log() and exp().

   \note In the calculations of QwtScaleMap the base of the log function
         has no effect on the mapping. So QwtLogTransform can be used
         for log2(), log10() or any other logarithmic scale.
 */
class QWT_EXPORT QwtLogTransform : public QwtTransform
{
public:
	QwtLogTransform();
	virtual ~QwtLogTransform();

    virtual double transform(double value) const QWT_OVERRIDE;
    virtual double invTransform(double value) const QWT_OVERRIDE;

    virtual double bounded(double value) const QWT_OVERRIDE;

	virtual QwtTransform* copy() const QWT_OVERRIDE;

	static const double LogMin;
	static const double LogMax;
};

/*!
   \brief A transformation using pow()

   QwtPowerTransform preserves the sign of a value.
   F.e. a transformation with a factor of 2
   transforms a value of -3 to -9 and v.v. Thus QwtPowerTransform
   can be used for scales including negative values.
 */
class QWT_EXPORT QwtPowerTransform : public QwtTransform
{
public:
    explicit QwtPowerTransform(double exponent);
	virtual ~QwtPowerTransform();

    virtual double transform(double value) const QWT_OVERRIDE;
    virtual double invTransform(double value) const QWT_OVERRIDE;

	virtual QwtTransform* copy() const QWT_OVERRIDE;

private:
	const double m_exponent;
};

#endif

/*** End of inlined file: qwt_transform.h ***/

/*** Start of inlined file: qwt_scale_map.h ***/
#ifndef QWT_SCALE_MAP_H
#define QWT_SCALE_MAP_H

class QPointF;
class QRectF;

/*!
   \brief A scale map

   QwtScaleMap offers transformations from the coordinate system
   of a scale into the linear coordinate system of a paint device
   and vice versa.
 */
class QWT_EXPORT QwtScaleMap
{
public:
	QwtScaleMap();
    QwtScaleMap(const QwtScaleMap&);

	~QwtScaleMap();

    QwtScaleMap& operator=(const QwtScaleMap&);

    void setTransformation(QwtTransform*);
	const QwtTransform* transformation() const;

    void setPaintInterval(double p1, double p2);
    void setScaleInterval(double s1, double s2);

    double transform(double s) const;
    double invTransform(double p) const;

	double p1() const;
	double p2() const;

	double s1() const;
	double s2() const;

	double pDist() const;
	double sDist() const;

    static QRectF transform(const QwtScaleMap&, const QwtScaleMap&, const QRectF&);

    static QRectF invTransform(const QwtScaleMap&, const QwtScaleMap&, const QRectF&);

    static QPointF transform(const QwtScaleMap&, const QwtScaleMap&, const QPointF&);

    static QPointF invTransform(const QwtScaleMap&, const QwtScaleMap&, const QPointF&);

	bool isInverting() const;

private:
	void updateFactor();

    double m_s1, m_s2;  // scale interval boundaries
    double m_p1, m_p2;  // paint device interval boundaries

    double m_cnv;  // conversion factor
	double m_ts1;

	QwtTransform* m_transform;
};

/*!
    \return First border of the scale interval
 */
inline double QwtScaleMap::s1() const
{
	return m_s1;
}

/*!
    \return Second border of the scale interval
 */
inline double QwtScaleMap::s2() const
{
	return m_s2;
}

/*!
    \return First border of the paint interval
 */
inline double QwtScaleMap::p1() const
{
	return m_p1;
}

/*!
    \return Second border of the paint interval
 */
inline double QwtScaleMap::p2() const
{
	return m_p2;
}

/*!
    \return qwtAbs(p2() - p1())
 */
inline double QwtScaleMap::pDist() const
{
    return qAbs(m_p2 - m_p1);
}

/*!
    \return qwtAbs(s2() - s1())
 */
inline double QwtScaleMap::sDist() const
{
    return qAbs(m_s2 - m_s1);
}

/*!
   Transform a point related to the scale interval into an point
   related to the interval of the paint device

   \param s Value relative to the coordinates of the scale
   \return Transformed value

   \sa invTransform()
 */
inline double QwtScaleMap::transform(double s) const
{
    if (m_transform)
        s = m_transform->transform(s);

    return m_p1 + (s - m_ts1) * m_cnv;
}

/*!
   Transform an paint device value into a value in the
   interval of the scale.

   \param p Value relative to the coordinates of the paint device
   \return Transformed value

   \sa transform()
 */
inline double QwtScaleMap::invTransform(double p) const
{
    double s = m_ts1 + (p - m_p1) / m_cnv;
    if (m_transform)
        s = m_transform->invTransform(s);

	return s;
}

//! \return True, when ( p1() < p2() ) != ( s1() < s2() )
inline bool QwtScaleMap::isInverting() const
{
    return ((m_p1 < m_p2) != (m_s1 < m_s2));
}

#ifndef QT_NO_DEBUG_STREAM
QWT_EXPORT QDebug operator<<(QDebug, const QwtScaleMap&);
#endif

#endif

/*** End of inlined file: qwt_scale_map.h ***/

/*** Start of inlined file: qwt_dyngrid_layout.h ***/
#ifndef QWT_DYNGRID_LAYOUT_H
#define QWT_DYNGRID_LAYOUT_H

#include <qlayout.h>

template< typename T >
class QList;

/*!
   \brief The QwtDynGridLayout class lays out widgets in a grid,
         adjusting the number of columns and rows to the current size.

   QwtDynGridLayout takes the space it gets, divides it up into rows and
   columns, and puts each of the widgets it manages into the correct cell(s).
   It lays out as many number of columns as possible (limited by maxColumns()).
 */

class QWT_EXPORT QwtDynGridLayout : public QLayout
{
	Q_OBJECT
public:
    explicit QwtDynGridLayout(QWidget*, int margin = 0, int spacing = -1);
    explicit QwtDynGridLayout(int spacing = -1);

	virtual ~QwtDynGridLayout();

	virtual void invalidate() QWT_OVERRIDE;

    void setMaxColumns(uint maxColumns);
	uint maxColumns() const;

    uint numRows() const;
    uint numColumns() const;

    virtual void addItem(QLayoutItem*) QWT_OVERRIDE;

    virtual QLayoutItem* itemAt(int index) const QWT_OVERRIDE;
    virtual QLayoutItem* takeAt(int index) QWT_OVERRIDE;
	virtual int count() const QWT_OVERRIDE;

    void setExpandingDirections(Qt::Orientations);
	virtual Qt::Orientations expandingDirections() const QWT_OVERRIDE;
    QList< QRect > layoutItems(const QRect&, uint numColumns) const;

	virtual int maxItemWidth() const;

    virtual void setGeometry(const QRect&) QWT_OVERRIDE;

	virtual bool hasHeightForWidth() const QWT_OVERRIDE;
    virtual int heightForWidth(int) const QWT_OVERRIDE;

	virtual QSize sizeHint() const QWT_OVERRIDE;

	virtual bool isEmpty() const QWT_OVERRIDE;
	uint itemCount() const;

    virtual uint columnsForWidth(int width) const;

protected:
    void layoutGrid(uint numColumns, QVector< int >& rowHeight, QVector< int >& colWidth) const;

    void stretchGrid(const QRect& rect, uint numColumns, QVector< int >& rowHeight, QVector< int >& colWidth) const;

private:
	void init();
    int maxRowWidth(int numColumns) const;

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_dyngrid_layout.h ***/

/*** Start of inlined file: qwt_weeding_curve_fitter.h ***/
#ifndef QWT_WEEDING_CURVE_FITTER_H
#define QWT_WEEDING_CURVE_FITTER_H

/*** Start of inlined file: qwt_curve_fitter.h ***/
#ifndef QWT_CURVE_FITTER_H
#define QWT_CURVE_FITTER_H

class QPainterPath;
class QPolygonF;

/*!
   \brief Abstract base class for a curve fitter
 */
class QWT_EXPORT QwtCurveFitter
{
public:
	/*!
	   \brief Preferred mode of the fitting algorithm

	   Even if a QPainterPath can always be created from a QPolygonF
	   the overhead of the conversion can be avoided by indicating
	   the preference of the implementation to the application
	   code.
	 */
	enum Mode
	{
		/*!
		   The fitting algorithm creates a polygon - the implementation
		   of fitCurvePath() simply wraps the polygon into a path.

		   \sa QwtWeedingCurveFitter
		 */
		Polygon,

		/*!
		   The fitting algorithm creates a painter path - the implementation
		   of fitCurve() extracts a polygon from the path.

		   \sa QwtSplineCurveFitter
		 */
		Path
	};

	virtual ~QwtCurveFitter();

	Mode mode() const;

	/*!
        Find a curve which has the best fit to a series of data points

        \param polygon Series of data points
        \return Curve points

        \sa fitCurvePath()
	 */
    virtual QPolygonF fitCurve(const QPolygonF& polygon) const = 0;

	/*!
        Find a curve path which has the best fit to a series of data points

        \param polygon Series of data points
        \return Curve path

        \sa fitCurve()
	 */
    virtual QPainterPath fitCurvePath(const QPolygonF& polygon) const = 0;

protected:
    explicit QwtCurveFitter(Mode mode);

private:
	Q_DISABLE_COPY(QwtCurveFitter)

	const Mode m_mode;
};

#endif

/*** End of inlined file: qwt_curve_fitter.h ***/

/*!
   \brief A curve fitter implementing Douglas and Peucker algorithm

   The purpose of the Douglas and Peucker algorithm is that given a 'curve'
   composed of line segments to find a curve not too dissimilar but that
   has fewer points. The algorithm defines 'too dissimilar' based on the
   maximum distance (tolerance) between the original curve and the
   smoothed curve.

   The runtime of the algorithm increases non linear ( worst case O( n*n ) )
   and might be very slow for huge polygons. To avoid performance issues
   it might be useful to split the polygon ( setChunkSize() ) and to run the algorithm
   for these smaller parts. The disadvantage of having no interpolation
   at the borders is for most use cases irrelevant.

   The smoothed curve consists of a subset of the points that defined the
   original curve.

   In opposite to QwtSplineCurveFitter the Douglas and Peucker algorithm reduces
   the number of points. By adjusting the tolerance parameter according to the
   axis scales QwtSplineCurveFitter can be used to implement different
   level of details to speed up painting of curves of many points.
 */
class QWT_EXPORT QwtWeedingCurveFitter : public QwtCurveFitter
{
public:
    explicit QwtWeedingCurveFitter(double tolerance = 1.0);
	virtual ~QwtWeedingCurveFitter();

    void setTolerance(double);
	double tolerance() const;

    void setChunkSize(uint);
	uint chunkSize() const;

    virtual QPolygonF fitCurve(const QPolygonF&) const QWT_OVERRIDE;
    virtual QPainterPath fitCurvePath(const QPolygonF&) const QWT_OVERRIDE;

private:
    virtual QPolygonF simplify(const QPolygonF&) const;

	class Line;

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_weeding_curve_fitter.h ***/

/*** Start of inlined file: qwt_samples.h ***/
#ifndef QWT_SAMPLES_H
#define QWT_SAMPLES_H

#include <qvector.h>
#include <qrect.h>

//! \brief A sample of the types (x1-x2, y) or (x, y1-y2)
class QWT_EXPORT QwtIntervalSample
{
public:
	QwtIntervalSample();
    QwtIntervalSample(double, const QwtInterval&);
    QwtIntervalSample(double value, double min, double max);

    bool operator==(const QwtIntervalSample&) const;
    bool operator!=(const QwtIntervalSample&) const;

	//! Value
	double value;

	//! Interval
	QwtInterval interval;
};

/*!
   Constructor
   The value is set to 0.0, the interval is invalid
 */
inline QwtIntervalSample::QwtIntervalSample() : value(0.0)
{
}

//! Constructor
inline QwtIntervalSample::QwtIntervalSample(double v, const QwtInterval& intv) : value(v), interval(intv)
{
}

//! Constructor
inline QwtIntervalSample::QwtIntervalSample(double v, double min, double max) : value(v), interval(min, max)
{
}

//! Compare operator
inline bool QwtIntervalSample::operator==(const QwtIntervalSample& other) const
{
	return value == other.value && interval == other.interval;
}

//! Compare operator
inline bool QwtIntervalSample::operator!=(const QwtIntervalSample& other) const
{
    return !(*this == other);
}

//! \brief A sample of the types (x1...xn, y) or (x, y1..yn)
class QWT_EXPORT QwtSetSample
{
public:
	QwtSetSample();
    explicit QwtSetSample(double, const QVector< double >& = QVector< double >());

    bool operator==(const QwtSetSample& other) const;
    bool operator!=(const QwtSetSample& other) const;

	double added() const;

	//! value
	double value;

	//! Vector of values associated to value
	QVector< double > set;
};

/*!
   Constructor
   The value is set to 0.0
 */
inline QwtSetSample::QwtSetSample() : value(0.0)
{
}

/*!
   Constructor

   \param v Value
   \param s Set of values
 */
inline QwtSetSample::QwtSetSample(double v, const QVector< double >& s) : value(v), set(s)
{
}

//! Compare operator
inline bool QwtSetSample::operator==(const QwtSetSample& other) const
{
	return value == other.value && set == other.set;
}

//! Compare operator
inline bool QwtSetSample::operator!=(const QwtSetSample& other) const
{
    return !(*this == other);
}

//! \return All values of the set added
inline double QwtSetSample::added() const
{
	double y = 0.0;
    for (int i = 0; i < set.size(); i++)
        y += set[ i ];

	return y;
}

/*!
   \brief Open-High-Low-Close sample used in financial charts

   In financial charts the movement of a price in a time interval is often
   represented by the opening/closing prices and the lowest/highest prices
   in this interval.

   \sa QwtTradingChartData
 */
class QWT_EXPORT QwtOHLCSample
{
public:
    QwtOHLCSample(double time = 0.0, double open = 0.0, double high = 0.0, double low = 0.0, double close = 0.0);

	QwtInterval boundingInterval() const;

	bool isValid() const;

	/*!
	   Time of the sample, usually a number representing
	   a specific interval - like a day.
	 */
	double time;

	//! Opening price
	double open;

	//! Highest price
	double high;

	//! Lowest price
	double low;

	//! Closing price
	double close;
};

/*!
   Constructor

   \param t Time value
   \param o Open value
   \param h High value
   \param l Low value
   \param c Close value
 */
inline QwtOHLCSample::QwtOHLCSample(double t, double o, double h, double l, double c)
    : time(t), open(o), high(h), low(l), close(c)
{
}

/*!
   \brief Check if a sample is valid

   A sample is valid, when all of the following checks are true:

   - low <= high
   - low <= open <= high
   - low <= close <= high

   \return True, when the sample is valid
 */
inline bool QwtOHLCSample::isValid() const
{
    return (low <= high) && (open >= low) && (open <= high) && (close >= low) && (close <= high);
}

/*!
   \brief Calculate the bounding interval of the OHLC values

   For valid samples the limits of this interval are always low/high.

   \return Bounding interval
   \sa isValid()
 */
inline QwtInterval QwtOHLCSample::boundingInterval() const
{
	double minY = open;
    minY        = qMin(minY, high);
    minY        = qMin(minY, low);
    minY        = qMin(minY, close);

	double maxY = open;
    maxY        = qMax(maxY, high);
    maxY        = qMax(maxY, low);
    maxY        = qMax(maxY, close);

    return QwtInterval(minY, maxY);
}

/*!
   \brief Sample used in vector fields

   A vector field sample is a position and a vector - usually
   representing some direction and magnitude - attached to this position.

   \sa QwtVectorFieldData
 */
class QWT_EXPORT QwtVectorFieldSample
{
public:
    QwtVectorFieldSample(double x = 0.0, double y = 0.0, double vx = 0.0, double vy = 0.0);

    QwtVectorFieldSample(const QPointF& pos, double vx = 0.0, double vy = 0.0);

	QPointF pos() const;

	bool isNull() const;

	//! x coordinate of the position
	double x;

	//! y coordinate of the position
	double y;

	//! x coordinate of the vector
	double vx;

	//! y coordinate of the vector
	double vy;
};

/*!
   \brief Constructor

   \param posX x coordinate of the position
   \param posY y coordinate of the position
   \param vectorX x coordinate of the vector
   \param vectorY y coordinate of the vector
 */
inline QwtVectorFieldSample::QwtVectorFieldSample(double posX, double posY, double vectorX, double vectorY)
    : x(posX), y(posY), vx(vectorX), vy(vectorY)
{
}

/*!
   \brief Constructor

   \param pos Position
   \param vectorX x coordinate of the vector
   \param vectorY y coordinate of the vector
 */
inline QwtVectorFieldSample::QwtVectorFieldSample(const QPointF& pos, double vectorX, double vectorY)
    : x(pos.x()), y(pos.y()), vx(vectorX), vy(vectorY)
{
}

//! \return x/y coordinates as QPointF
inline QPointF QwtVectorFieldSample::pos() const
{
    return QPointF(x, y);
}

//! \return true, if vx and vy are 0
inline bool QwtVectorFieldSample::isNull() const
{
    return (vx == 0.0) && (vy == 0.0);
}

#endif

/*** End of inlined file: qwt_samples.h ***/

/*** Start of inlined file: qwt_point_3d.h ***/
/*! \file */
#ifndef QWT_POINT_3D_H
#define QWT_POINT_3D_H

#include <qpoint.h>
#include <qmetatype.h>

/*!
   \brief QwtPoint3D class defines a 3D point in double coordinates
 */

class QWT_EXPORT QwtPoint3D
{
public:
	QwtPoint3D();
    QwtPoint3D(double x, double y, double z);
    QwtPoint3D(const QPointF&);

    bool isNull() const;

	double x() const;
	double y() const;
	double z() const;

	double& rx();
	double& ry();
	double& rz();

    void setX(double x);
    void setY(double y);
    void setZ(double y);

	QPointF toPoint() const;

    bool operator==(const QwtPoint3D&) const;
    bool operator!=(const QwtPoint3D&) const;

private:
	double m_x;
	double m_y;
	double m_z;
};

Q_DECLARE_TYPEINFO(QwtPoint3D, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(QwtPoint3D);

#ifndef QT_NO_DEBUG_STREAM
QWT_EXPORT QDebug operator<<(QDebug, const QwtPoint3D&);
#endif

/*!
    Constructs a null point.
    \sa isNull()
 */
inline QwtPoint3D::QwtPoint3D() : m_x(0.0), m_y(0.0), m_z(0.0)
{
}

//! Constructs a point with coordinates specified by x, y and z.
inline QwtPoint3D::QwtPoint3D(double x, double y, double z = 0.0) : m_x(x), m_y(y), m_z(z)
{
}

/*!
    Constructs a point with x and y coordinates from a 2D point,
    and a z coordinate of 0.
 */
inline QwtPoint3D::QwtPoint3D(const QPointF& other) : m_x(other.x()), m_y(other.y()), m_z(0.0)
{
}

/*!
    \return True if the point is null; otherwise returns false.

    A point is considered to be null if x, y and z-coordinates
    are equal to zero.
 */
inline bool QwtPoint3D::isNull() const
{
	return m_x == 0.0 && m_y == 0.0 && m_z == 0.0;
}

//! \return The x-coordinate of the point.
inline double QwtPoint3D::x() const
{
	return m_x;
}

//! \return The y-coordinate of the point.
inline double QwtPoint3D::y() const
{
	return m_y;
}

//! \return The z-coordinate of the point.
inline double QwtPoint3D::z() const
{
	return m_z;
}

//! \return A reference to the x-coordinate of the point.
inline double& QwtPoint3D::rx()
{
	return m_x;
}

//! \return A reference to the y-coordinate of the point.
inline double& QwtPoint3D::ry()
{
	return m_y;
}

//! \return A reference to the z-coordinate of the point.
inline double& QwtPoint3D::rz()
{
	return m_z;
}

//! Sets the x-coordinate of the point to the value specified by x.
inline void QwtPoint3D::setX(double x)
{
	m_x = x;
}

//! Sets the y-coordinate of the point to the value specified by y.
inline void QwtPoint3D::setY(double y)
{
	m_y = y;
}

//! Sets the z-coordinate of the point to the value specified by z.
inline void QwtPoint3D::setZ(double z)
{
	m_z = z;
}

/*!
   \return 2D point, where the z coordinate is dropped.
 */
inline QPointF QwtPoint3D::toPoint() const
{
    return QPointF(m_x, m_y);
}

//! \return True, if this point and other are equal; otherwise returns false.
inline bool QwtPoint3D::operator==(const QwtPoint3D& other) const
{
    return (m_x == other.m_x) && (m_y == other.m_y) && (m_z == other.m_z);
}

//! \return True if this rect and other are different; otherwise returns false.
inline bool QwtPoint3D::operator!=(const QwtPoint3D& other) const
{
    return !operator==(other);
}

#endif

/*** End of inlined file: qwt_point_3d.h ***/

/*** Start of inlined file: qwt_series_data.h ***/
#ifndef QWT_SERIES_DATA_H
#define QWT_SERIES_DATA_H

#include <qvector.h>
#include <qrect.h>

class QwtPointPolar;

/*!
   \brief Abstract interface for iterating over samples

   Qwt offers several implementations of the QwtSeriesData API,
   but in situations, where data of an application specific format
   needs to be displayed, without having to copy it, it is recommended
   to implement an individual data access.

   A subclass of QwtSeriesData<QPointF> must implement:

   - size()\n
     Should return number of data points.

   - sample()\n
     Should return values x and y values of the sample at specific position
     as QPointF object.

   - boundingRect()\n
     Should return the bounding rectangle of the data series.
     It is used for autoscaling and might help certain algorithms for displaying
     the data. You can use qwtBoundingRect() for an implementation
     but often it is possible to implement a more efficient algorithm
     depending on the characteristics of the series.
     The member cachedBoundingRect is intended for caching the calculated rectangle.

 */
template< typename T >
class QwtSeriesData
{
public:
	//! Constructor
	QwtSeriesData();

	//! Destructor
	virtual ~QwtSeriesData();

#ifndef QWT_PYTHON_WRAPPER

	//! \return Number of samples
	virtual size_t size() const = 0;

	/*!
	   Return a sample
	   \param i Index
	   \return Sample at position i
	 */
	virtual T sample(size_t i) const = 0;

	/*!
	   Calculate the bounding rect of all samples

	   The bounding rect is necessary for autoscaling and can be used
	   for a couple of painting optimizations.

	   qwtBoundingRect(...) offers slow implementations iterating
	   over the samples. For large sets it is recommended to implement
	   something faster f.e. by caching the bounding rectangle.

	   \return Bounding rectangle
	 */
	virtual QRectF boundingRect() const = 0;

#else
	// Needed for generating the python bindings, but not for using them !
	virtual size_t size() const
	{
		return 0;
	}
	virtual T sample(size_t i) const
	{
		return T();
	}
	virtual QRectF boundingRect() const
	{
		return cachedBoundingRect;
	}
#endif

	/*!
	   Set a the "rect of interest"

	   QwtPlotSeriesItem defines the current area of the plot canvas
	   as "rectangle of interest" ( QwtPlotSeriesItem::updateScaleDiv() ).
	   It can be used to implement different levels of details.

	   The default implementation does nothing.

	   \param rect Rectangle of interest
	 */
	virtual void setRectOfInterest(const QRectF& rect);

protected:
	//! Can be used to cache a calculated bounding rectangle
	mutable QRectF cachedBoundingRect;

private:
	QwtSeriesData< T >& operator=(const QwtSeriesData< T >&);
};

template< typename T >
QwtSeriesData< T >::QwtSeriesData() : cachedBoundingRect(0.0, 0.0, -1.0, -1.0)
{
}

template< typename T >
QwtSeriesData< T >::~QwtSeriesData()
{
}

template< typename T >
void QwtSeriesData< T >::setRectOfInterest(const QRectF&)
{
}

/*!
   \brief Template class for data, that is organized as QVector

   QVector uses implicit data sharing and can be
   passed around as argument efficiently.
 */
template< typename T >
class QwtArraySeriesData : public QwtSeriesData< T >
{
public:
	//! Constructor
	QwtArraySeriesData();

	/*!
	   Constructor
	   \param samples Array of samples
	 */
	explicit QwtArraySeriesData(const QVector< T >& samples);
	explicit QwtArraySeriesData(QVector< T >&& samples);
	/*!
	   Assign an array of samples
	   \param samples Array of samples
	 */
	void setSamples(const QVector< T >& samples);
	void setSamples(QVector< T >&& samples);

	//! \return Array of samples
	const QVector< T > samples() const;

	//! \return Number of samples
	virtual size_t size() const QWT_OVERRIDE;

	/*!
	   \return Sample at a specific position

	   \param index Index
	   \return Sample at position index
	 */
	virtual T sample(size_t index) const QWT_OVERRIDE;

protected:
	//! Vector of samples
	QVector< T > m_samples;
};

template< typename T >
QwtArraySeriesData< T >::QwtArraySeriesData()
{
}

template< typename T >
QwtArraySeriesData< T >::QwtArraySeriesData(const QVector< T >& samples) : m_samples(samples)
{
}

template< typename T >
QwtArraySeriesData< T >::QwtArraySeriesData(QVector< T >&& samples)
{
	m_samples = std::move(samples);
}

template< typename T >
void QwtArraySeriesData< T >::setSamples(const QVector< T >& samples)
{
	QwtSeriesData< T >::cachedBoundingRect = QRectF(0.0, 0.0, -1.0, -1.0);
	m_samples                              = samples;
}

template< typename T >
void QwtArraySeriesData< T >::setSamples(QVector< T >&& samples)
{
	QwtSeriesData< T >::cachedBoundingRect = QRectF(0.0, 0.0, -1.0, -1.0);
	m_samples                              = std::move(samples);
}

template< typename T >
const QVector< T > QwtArraySeriesData< T >::samples() const
{
	return m_samples;
}

template< typename T >
size_t QwtArraySeriesData< T >::size() const
{
	return m_samples.size();
}

template< typename T >
T QwtArraySeriesData< T >::sample(size_t i) const
{
	return m_samples[ static_cast< int >(i) ];
}

//! Interface for iterating over an array of points
class QWT_EXPORT QwtPointSeriesData : public QwtArraySeriesData< QPointF >
{
public:
	QwtPointSeriesData(const QVector< QPointF >& = QVector< QPointF >());

	virtual QRectF boundingRect() const QWT_OVERRIDE;
};

//! Interface for iterating over an array of 3D points
class QWT_EXPORT QwtPoint3DSeriesData : public QwtArraySeriesData< QwtPoint3D >
{
public:
	QwtPoint3DSeriesData(const QVector< QwtPoint3D >& = QVector< QwtPoint3D >());

	virtual QRectF boundingRect() const QWT_OVERRIDE;
};

//! Interface for iterating over an array of intervals
class QWT_EXPORT QwtIntervalSeriesData : public QwtArraySeriesData< QwtIntervalSample >
{
public:
	QwtIntervalSeriesData(const QVector< QwtIntervalSample >& = QVector< QwtIntervalSample >());

	virtual QRectF boundingRect() const QWT_OVERRIDE;
};

//! Interface for iterating over an array of samples
class QWT_EXPORT QwtSetSeriesData : public QwtArraySeriesData< QwtSetSample >
{
public:
	QwtSetSeriesData(const QVector< QwtSetSample >& = QVector< QwtSetSample >());

	virtual QRectF boundingRect() const QWT_OVERRIDE;
};

//! Interface for iterating over an array of vector field samples
class QWT_EXPORT QwtVectorFieldData : public QwtArraySeriesData< QwtVectorFieldSample >
{
public:
	QwtVectorFieldData(const QVector< QwtVectorFieldSample >& = QVector< QwtVectorFieldSample >());

	virtual QRectF boundingRect() const QWT_OVERRIDE;
};

/*!
    Interface for iterating over an array of OHLC samples
 */
class QWT_EXPORT QwtTradingChartData : public QwtArraySeriesData< QwtOHLCSample >
{
public:
	QwtTradingChartData(const QVector< QwtOHLCSample >& = QVector< QwtOHLCSample >());

	virtual QRectF boundingRect() const QWT_OVERRIDE;
};

QWT_EXPORT QRectF qwtBoundingRect(const QwtSeriesData< QPointF >&, int from = 0, int to = -1);

QWT_EXPORT QRectF qwtBoundingRect(const QwtSeriesData< QwtPoint3D >&, int from = 0, int to = -1);

QWT_EXPORT QRectF qwtBoundingRect(const QwtSeriesData< QwtPointPolar >&, int from = 0, int to = -1);

QWT_EXPORT QRectF qwtBoundingRect(const QwtSeriesData< QwtIntervalSample >&, int from = 0, int to = -1);

QWT_EXPORT QRectF qwtBoundingRect(const QwtSeriesData< QwtSetSample >&, int from = 0, int to = -1);

QWT_EXPORT QRectF qwtBoundingRect(const QwtSeriesData< QwtOHLCSample >&, int from = 0, int to = -1);

QWT_EXPORT QRectF qwtBoundingRect(const QwtSeriesData< QwtVectorFieldSample >&, int from = 0, int to = -1);

/**
 * Binary search for a sorted series of samples
 *
 * qwtUpperSampleIndex returns the index of sample that is the upper bound
 * of value. Is the the value smaller than the smallest value the return
 * value will be 0. Is the value greater or equal than the largest
 * value the return value will be -1.
 *
 * @par Example
 * The following example shows finds a point of curve from an x
 * coordinate
 * @code
 * #include <qwt_series_data.h>
 * #include <qwt_plot_curve.h>
 *
 *   struct compareX
 *   {
 *       inline bool operator()( const double x, const QPointF &pos ) const
 *       {
 *           return ( x < pos.x() );
 *       }
 *   };
 *
 *   QLineF curveLineAt( const QwtPlotCurve *curve, double x )
 *   {
 *       int index = qwtUpperSampleIndex<QPointF>(*curve->data(), x, compareX() );
 *
 *       if ( index == -1 &&
 *           x == curve->sample( curve->dataSize() - 1 ).x() )
 *       {
 *           // the last sample is excluded from qwtUpperSampleIndex
 *           index = curve->dataSize() - 1;
 *       }
 *
 *       QLineF line; // invalid
 *       if ( index > 0 )
 *       {
 *           line.setP1( curve->sample( index - 1 ) );
 *           line.setP2( curve->sample( index ) );
 *       }
 *
 *       return line;
 *   }
 *
 * @endcode
 * @endpar
 *
 * @param series Series of samples
 * @param value Value
 * @param lessThan Compare operation
 * @note The samples must be sorted according to the order specified
 *        by the lessThan object
 */
template< typename T, typename LessThan >
inline int qwtUpperSampleIndex(const QwtSeriesData< T >& series, double value, LessThan lessThan)
{
	const int indexMax = series.size() - 1;

	if (indexMax < 0 || !lessThan(value, series.sample(indexMax)))
		return -1;

	int indexMin = 0;
	int n        = indexMax;

	while (n > 0) {
		const int half     = n >> 1;
		const int indexMid = indexMin + half;

		if (lessThan(value, series.sample(indexMid))) {
			n = half;
		} else {
			indexMin = indexMid + 1;
			n -= half + 1;
		}
	}

	return indexMin;
}

#endif

/*** End of inlined file: qwt_series_data.h ***/

/*** Start of inlined file: qwt_raster_data.h ***/
#ifndef QWT_RASTER_DATA_H
#define QWT_RASTER_DATA_H

#include <qnamespace.h>

class QwtInterval;
class QPolygonF;
class QRectF;
class QSize;
template< typename T >
class QList;
template< class Key, class T >
class QMap;

/*!
   \brief QwtRasterData defines an interface to any type of raster data.

   QwtRasterData is an abstract interface, that is used by
   QwtPlotRasterItem to find the values at the pixels of its raster.

   Gaps inside the bounding rectangle of the data can be indicated by NaN
   values ( when WithoutGaps is disabled ).

   Often a raster item is used to display values from a matrix. Then the
   derived raster data class needs to implement some sort of resampling,
   that maps the raster of the matrix into the requested raster of
   the raster item ( depending on resolution and scales of the canvas ).

   QwtMatrixRasterData implements raster data, that returns values from
   a given 2D matrix.

   \sa QwtMatrixRasterData
 */
class QWT_EXPORT QwtRasterData
{
public:
	//! Contour lines
	typedef QMap< double, QPolygonF > ContourLines;

	/*!
	   \brief Raster data attributes

	   Additional information that is used to improve processing
	   of the data.
	 */
	enum Attribute
	{
		/*!
		   The bounding rectangle of the data is spanned by
		   the interval(Qt::XAxis) and interval(Qt::YAxis).

		   WithoutGaps indicates, that the data has no gaps
		   ( unknown values ) in this area and the result of
		   value() does not need to be checked for NaN values.

		   Enabling this flag will have an positive effect on
		   the performance of rendering a QwtPlotSpectrogram.

		   The default setting is false.

		   \note NaN values indicate an undefined value
		 */
		WithoutGaps = 0x01
	};

    Q_DECLARE_FLAGS(Attributes, Attribute)

	//! Flags to modify the contour algorithm
	enum ConrecFlag
	{
		//! Ignore all vertices on the same level
		IgnoreAllVerticesOnLevel = 0x01,

		//! Ignore all values, that are out of range
		IgnoreOutOfRange = 0x02
	};

    Q_DECLARE_FLAGS(ConrecFlags, ConrecFlag)

	QwtRasterData();
	virtual ~QwtRasterData();

    void setAttribute(Attribute, bool on = true);
    bool testAttribute(Attribute) const;

	/*!
	   \return Bounding interval for an axis
	   \sa setInterval
	 */
    virtual QwtInterval interval(Qt::Axis) const = 0;

    virtual QRectF pixelHint(const QRectF&) const;

    virtual void initRaster(const QRectF&, const QSize& raster);
	virtual void discardRaster();

	/*!
	   \return the value at a raster position
	   \param x X value in plot coordinates
	   \param y Y value in plot coordinates
	 */
    virtual double value(double x, double y) const = 0;

    virtual ContourLines contourLines(const QRectF& rect, const QSize& raster, const QList< double >& levels, ConrecFlags) const;

	class Contour3DPoint;
	class ContourPlane;

private:
	Q_DISABLE_COPY(QwtRasterData)

	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtRasterData::ConrecFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QwtRasterData::Attributes)

#endif

/*** End of inlined file: qwt_raster_data.h ***/

/*** Start of inlined file: qwt_grid_data.hpp ***/
#ifndef QWT_GRID_DATA_HPP
#define QWT_GRID_DATA_HPP
#include <vector>
#include <algorithm>
#include <limits>
#include <cmath>
#include <cassert>
#include <stdexcept>

/**
 * @class QwtGridData
 *
 * @brief A generic container class for storing 2D grid data and providing resampling methods.
 *
 * @details This template class encapsulates a 2D table with x-axis and y-axis data.
 * It supports three resampling methods: Nearest Neighbor, Bilinear Interpolation, and Bicubic Interpolation.
 * The container types for the axes and data are templated to allow flexibility.
 *
 * 此模板类封装了一个二维表格以及对应的 x 轴和 y 轴数据。
 * 它支持三种插值方法：最近邻插值、双线性插值和双三次插值。
 * 轴和数据的容器类型被模板化以提供灵活性。
 *
 * ↓Y轴方向 →X轴方向 n行 m列
 *
 * |column[0]|column[1]| ... |column[m]|
 * +---------+---------+-----+---------+
 * | [x0,yn] | [x1,yn] | ... | [xm,yn] | → yAxis[n] 对应行
 * +---------+---------+-----+---------+
 * |   ...   |   ...   | ... |   ...   |
 * +---------+---------+-----+---------+
 * | [x0,y1] | [x1,y1] | ... | [xm,y1] | → yAxis[1] 对应行
 * +---------+---------+-----+---------+
 * | [x0,y0] | [x1,y0] | ... | [xm,y0] | → yAxis[0] 对应行
 * +---------+---------+-----+---------+
 *      ↑          ↑      ↑       ↑
 *  xAxis[0]   xAxis[1]  ...   xAxis[m]
 *
 * **Example Usage**:
 *
 * Example using `std::vector`:
 * @code
 * std::vector<double> xAxis = {0, 1, 2, 3};
 * std::vector<double> yAxis = {0, 1, 2};
 * std::vector<std::vector<double>> data = {
 *     {1, 2, 3},
 *     {4, 5, 6},
 *     {7, 8, 9},
 *     {10, 11, 12}
 * };
 *
 * // Create an instance with bilinear interpolation
 * QwtGridData<double> gridData(xAxis, yAxis, data, QwtGridData<double>::BilinearInterpolation);
 *
 * // Query the value at (1.5, 1.5)
 * double value = gridData(1.5, 1.5);
 * std::cout << "Value at (1.5, 1.5): " << value << std::endl;
 * @endcode
 *
 * Example using `QVector`:
 * @code
 * QVector<double> xAxis = {0, 1, 2, 3};
 * QVector<double> yAxis = {0, 1, 2};
 * QVector<QVector<double>> data = {
 *     {1, 2, 3},
 *     {4, 5, 6},
 *     {7, 8, 9},
 *     {10, 11, 12}
 * };
 *
 * // Create an instance with bicubic interpolation
 * QwtGridData<double, QVector> gridData(xAxis, yAxis, data, QwtGridData<double, QVector>::BicubicInterpolation);
 *
 * // Query the value at (1.5, 1.5)
 * double value = gridData(1.5, 1.5);
 * qDebug() << "Value at (1.5, 1.5):" << value;
 * @endcode
 *
 * @tparam T The type of the values stored in the grid.
 * @tparam XContainer The container type for the x-axis values (default: std::vector).
 * @tparam YContainer The container type for the y-axis values (default: same as XContainer).
 * @tparam DataColumn The container type for a single column in the data matrix (default: same as XContainer).
 * @tparam DataContainer The container type for the entire data matrix (default: same as XContainer).
 */
template< typename T,
          template< typename > class XContainer    = std::vector,
          template< typename > class YContainer    = XContainer,
          template< typename > class DataColumn    = XContainer,
          template< typename > class DataContainer = XContainer >
class QwtGridData
{
public:
	using value_type       = T;                ///< @brief Type of the stored values / 存储值的类型
	using x_container_type = XContainer< T >;  ///< @brief Type of the x-axis container / x 轴容器的类型
	using y_container_type = YContainer< T >;  ///< @brief Type of the y-axis container / y 轴容器的类型
	using data_column_type = DataColumn< T >;  ///< @brief Type of a single column in the data matrix / 数据矩阵中单列的类型
	using data_container_type = DataContainer< data_column_type >;  ///< @brief Type of the data matrix / 数据矩阵的类型
	using size_type = typename DataColumn< T >::size_type;
	/**
	 * @brief Enumeration for resampling methods.
	 *
	 * Defines the available resampling methods:
	 * - NearestNeighbour: Nearest neighbor interpolation.
	 * - BilinearInterpolation: Bilinear interpolation.
	 * - BicubicInterpolation: Hermite bicubic interpolation.
	 *
	 * 定义可用的抽样方法：
	 * - NearestNeighbour: 最近邻插值。
	 * - BilinearInterpolation: 双线性插值。
	 * - BicubicInterpolation: Hermite 双三次插值。
	 */
	enum ResampleMode
	{
		NearestNeighbour,
		BilinearInterpolation,
		BicubicInterpolation
	};

	/**
	 * @brief Default constructor.
	 *
	 * 初始化一个空的 QwtGridData 对象。
	 */
	QwtGridData()
        : m_mode(NearestNeighbour), m_xMin(0.0), m_xMax(0.0), m_yMin(0.0), m_yMax(0.0), m_dataMax(0.0), m_dataMin(0.0)
	{
	}

	/**
	 * @brief Constructor with initial data.
	 *
	 * Initializes the object with x-axis, y-axis, and data matrix.
	 *
	 * 使用 x 轴、y 轴和数据矩阵初始化对象。
	 *
	 * @param xAxis The x-axis values. / x 轴值。
	 * @param yAxis The y-axis values. / y 轴值。
	 * @param data The 2D data matrix. / 二维数据矩阵。
	 * @param mode The resampling mode to use. / 要使用的抽样方法。
	 */
	QwtGridData(const XContainer< T >& xAxis,
                const YContainer< T >& yAxis,
                const DataContainer< DataColumn< T > >& data,
                ResampleMode mode = NearestNeighbour)
        : m_xAxis(xAxis), m_yAxis(yAxis), m_data(data), m_mode(mode)
	{
		validate();
		findValueRange();
		m_xMin = xAxis.front();
		m_xMax = xAxis.back();
		m_yMin = yAxis.front();
		m_yMax = yAxis.back();
	}

	/**
	 * @brief Set new x-axis, y-axis, and data matrix.
	 *
	 * data matrix is look like that:
	 *
	 * |column[0]|column[1]| ... |column[m]|
	 * +---------+---------+-----+---------+
	 * | [x0,yn] | [x1,yn] | ... | [xm,yn] | → yAxis[n] 对应行
	 * +---------+---------+-----+---------+
	 * |   ...   |   ...   | ... |   ...   |
	 * +---------+---------+-----+---------+
	 * | [x0,y1] | [x1,y1] | ... | [xm,y1] | → yAxis[1] 对应行
	 * +---------+---------+-----+---------+
	 * | [x0,y0] | [x1,y0] | ... | [xm,y0] | → yAxis[0] 对应行
	 * +---------+---------+-----+---------+
	 *      ↑          ↑      ↑       ↑
	 *  xAxis[0]   xAxis[1]  ...   xAxis[m]
	 *
	 *  so (data matrix).size = xAxis.size,(data matrix).at(n).size = yAxis.szie
	 *
	 * 设置新的 x 轴、y 轴和数据矩阵。
	 * 数据矩阵是一个vector<vector> ,数据矩阵.size = xAxis.size,数据矩阵.at(n).size = yAxis.size
	 *
	 * @param xAxis The x-axis values. / x 轴值。
	 * @param yAxis The y-axis values. / y 轴值。
	 * @param data The 2D data matrix. / 二维数据矩阵。
	 */
	void setValue(const XContainer< T >& xAxis, const YContainer< T >& yAxis, const DataContainer< DataColumn< T > >& data)
	{
		m_xAxis = xAxis;
		m_yAxis = yAxis;
		m_data  = data;
		validate();
		findValueRange();
		m_xMin = xAxis.front();
		m_xMax = xAxis.back();
		m_yMin = yAxis.front();
		m_yMax = yAxis.back();
	}

	/**
	 * @brief Operator to query value at (x, y).
	 *
	 * 根据给定的 x 和 y 坐标查询值。
	 *
	 * @param x The x-coordinate. / x 坐标。
	 * @param y The y-coordinate. / y 坐标。
	 * @return The interpolated or nearest value. / 插值或最近邻值。
	 */
	T operator()(T x, T y) const
	{
		return value(x, y);
	}

	/**
	 * @brief operator []
	 *
	 * 根据给定的 x 和 y 坐标查询值。
	 *
	 * @param xy,std::pair<x,y>
	 * @return The interpolated or nearest value. / 插值或最近邻值。
	 */
	T operator[](const std::pair< T, T >& xy) const
	{
		return value(xy.first, xy.second);
	}
	/**
	 * @brief Query value at (x, y).
	 *
	 * 根据给定的 x 和 y 坐标查询值。
	 *
	 * @param x The x-coordinate. / x 坐标。
	 * @param y The y-coordinate. / y 坐标。
	 * @return The interpolated or nearest value. / 插值或最近邻值。
	 */
	T value(T x, T y) const
	{
		switch (m_mode) {
		case NearestNeighbour:
			return nearestNeighbour(x, y);
		case BilinearInterpolation:
			return bilinearInterpolation(x, y);
		case BicubicInterpolation:
			return bicubicInterpolation(x, y);
		default:
			throw std::runtime_error("Unknown resampling mode.");
		}
	}

	/**
	 * @brief Set the resampling mode.
	 *
	 * 设置查询值时使用的抽样方法。
	 *
	 * @param mode The resampling mode to use. / 要使用的抽样方法。
	 */
	void setResampleMode(ResampleMode mode)
	{
		m_mode = mode;
	}

	/**
	 * @brief Get the current resampling mode.
	 *
	 * 返回当前激活的抽样方法。
	 *
	 * @return The current resampling mode. / 当前抽样方法。
	 */
	ResampleMode resampleMode() const
	{
		return m_mode;
	}

	/**
	 * @brief x的尺寸
	 * @return
	 */
	size_type xSize() const
	{
		return m_xAxis.size();
	}

	/**
	 * @brief y的尺寸
	 * @return
	 */
	size_type ySize() const
	{
		return m_yAxis.size();
	}

	/**
	 * @brief value矩阵的尺寸
	 * @return <xsize,ysize>
	 */
	std::pair< size_type, size_type > valueSize() const
	{
		return std::make_pair(xSize(), ySize());
	}

	/**
	 * @brief x 值对应的内容
	 * @param ix
	 * @return
	 */
	T atX(size_type ix) const
	{
		return m_xAxis.at(ix);
	}

	/**
	 * @brief y值对应的内容
	 * @param ix
	 * @return
	 */
	T atY(size_type iy) const
	{
		return m_yAxis.at(iy);
	}

	/**
	 * @brief value值对应的内容
	 * @param ix
	 * @return
	 */
	T atValue(size_type ix, size_type iy) const
	{
		return m_data.at(ix).at(iy);
	}

	/**
	 * @brief Get the x-axis values.
	 *
	 * 获取 x 轴值。
	 *
	 * @return The x-axis values. / x 轴值。
	 */
	const XContainer< T >& xAxis() const
	{
		return m_xAxis;
	}

	/**
	 * @brief Get the y-axis values.
	 *
	 * 获取 y 轴值。
	 *
	 * @return The y-axis values. / y 轴值。
	 */
	const YContainer< T >& yAxis() const
	{
		return m_yAxis;
	}

	/**
	 * @brief Get the data matrix.
	 *
	 * 获取数据矩阵。
	 *
	 * @return The data matrix. / 数据矩阵。
	 */
	const DataContainer< DataColumn< T > >& data() const
	{
		return m_data;
	}

	/**
	 * @brief Check if the object is valid.
	 *
	 * 判断对象是否有效。
	 *
	 * @return True if valid, false otherwise. / 如果有效则返回 true，否则返回 false。
	 */
	bool valid() const
	{
		if (m_xAxis.empty() || m_yAxis.empty() || m_data.empty()) {
			return false;
		}
		if (m_xAxis.size() != m_data.size()) {
			return false;
		}
		for (const auto& column : m_data) {
			if (column.size() != m_yAxis.size()) {
				return false;
			}
		}
		if (!std::is_sorted(m_xAxis.begin(), m_xAxis.end())) {
			return false;
		}
		if (!std::is_sorted(m_yAxis.begin(), m_yAxis.end())) {
			return false;
		}
		return true;
	}

	/**
	 * @brief Validate the data.
	 *
	 * 验证数据的有效性。
	 */
	void validate()
	{
		if (m_xAxis.empty() || m_yAxis.empty() || m_data.empty()) {
			throw std::invalid_argument("Axes or data cannot be empty.");
		}
		if (m_data.size() != m_xAxis.size()) {
			throw std::invalid_argument("Number of columns in data must match x-axis size.");
		}
		for (const auto& column : m_data) {
			if (column.size() != m_yAxis.size()) {
				throw std::invalid_argument("Number of rows in data must match y-axis size.");
			}
		}
		if (!std::is_sorted(m_xAxis.begin(), m_xAxis.end())) {
			std::sort(m_xAxis.begin(), m_xAxis.end());
		}
		if (!std::is_sorted(m_yAxis.begin(), m_yAxis.end())) {
			std::sort(m_yAxis.begin(), m_yAxis.end());
		}
	}

	T xMin() const
	{
		return m_xMin;
	}
	T xMax() const
	{
		return m_xMax;
	}
	T yMin() const
	{
		return m_yMin;
	}
	T yMax() const
	{
		return m_yMax;
	}
	T dataMin() const
	{
		return m_dataMin;
	}
	T dataMax() const
	{
		return m_dataMax;
	}

public:
	// static pulic function
	/**
	 * @brief Find the closest index in a sorted array.
	 *
	 * 在排序数组中查找最接近的索引。
	 *
	 * @param arr The sorted array. / 排序数组。
	 * @param val The target value. / 目标值。
	 * @return The index of the closest value. / 最接近值的索引。
	 */
	template< typename Container >
	static size_type findClosestIndex(const Container& arr, T val)
	{
		auto it = std::lower_bound(arr.begin(), arr.end(), val);
		if (it == arr.begin())
			return 0;
		if (it == arr.end())
			return arr.size() - 1;
		size_type idx = std::distance(arr.begin(), it);
		return (std::abs(arr[ idx ] - val) < std::abs(arr[ idx - 1 ] - val)) ? idx : idx - 1;
	}

	/**
	 * @brief Find the lower bound index in a sorted array.
	 *
	 * 在排序数组中查找下界索引。
	 *
	 * @param arr The sorted array. / 排序数组。
	 * @param val The target value. / 目标值。
	 * @return The lower bound index. / 下界索引。
	 */
	template< typename Container >
	static size_type findLowerIndex(const Container& arr, T val)
	{
		auto it = std::lower_bound(arr.begin(), arr.end(), val);
		if (it == arr.begin())
			return 0;
		return std::distance(arr.begin(), it) - 1;
	}

	template< typename V >
	static const V& clamp(const V& value, const V& lo, const V& hi)
	{
		return (value < lo) ? lo : (hi < value) ? hi : value;
	}

protected:
	/**
	 * @brief Get the minimum & maximum value in the data matrix.
	 *
	 * 查找并返回二维数据矩阵中的最小和最大值。
	 *
	 * @return std::pair<The minimum value, The maximum value> in the data matrix. / 数据矩阵中的最小和最大值。
	 */
	void findValueRange()
	{
		m_dataMin = std::numeric_limits< T >::max();
		m_dataMax = std::numeric_limits< T >::lowest();
		for (const auto& column : m_data) {
			for (const auto& val : column) {
				m_dataMin = std::min(m_dataMin, val);
				m_dataMax = std::max(m_dataMax, val);
			}
		}
	}

	/**
	 * @brief Nearest neighbor interpolation.
	 *
	 * 最近邻插值。
	 *
	 * @param x The x-coordinate. / x 坐标。
	 * @param y The y-coordinate. / y 坐标。
	 * @return The nearest value. / 最近邻值。
	 */
	T nearestNeighbour(T x, T y) const
	{
		size_type xIdx = findClosestIndex(m_xAxis, x);
		size_type yIdx = findClosestIndex(m_yAxis, y);
		return m_data[ xIdx ][ yIdx ];
	}

	/**
	 * @brief Bilinear interpolation.
	 *
	 * 双线性插值。
	 *
	 * @param x The x-coordinate. / x 坐标。
	 * @param y The y-coordinate. / y 坐标。
	 * @return The interpolated value. / 插值结果。
	 */
	T bilinearInterpolation(T x, T y) const
	{
		size_type x0Idx = findLowerIndex(m_xAxis, x);
		size_type x1Idx = x0Idx + 1;
		size_type y0Idx = findLowerIndex(m_yAxis, y);
		size_type y1Idx = y0Idx + 1;

		T x0 = m_xAxis[ x0Idx ], x1 = m_xAxis[ x1Idx ];
		T y0 = m_yAxis[ y0Idx ], y1 = m_yAxis[ y1Idx ];

		T f00 = m_data[ x0Idx ][ y0Idx ];
		T f10 = m_data[ x1Idx ][ y0Idx ];
		T f01 = m_data[ x0Idx ][ y1Idx ];
		T f11 = m_data[ x1Idx ][ y1Idx ];

		T dx = (x - x0) / (x1 - x0);
		T dy = (y - y0) / (y1 - y0);

		return (1 - dx) * (1 - dy) * f00 + dx * (1 - dy) * f10 + (1 - dx) * dy * f01 + dx * dy * f11;
	}

	/**
	 * @brief Perform bicubic interpolation.
	 *
	 * @details Returns the bicubically interpolated value at the specified (x, y) position.
	 * This implementation uses the Hermite bicubic interpolation method.
	 *
	 * 返回指定 (x, y) 位置的双三次插值结果。
	 * 此实现使用 Hermite 双三次插值方法。
	 *
	 * @param x The x-coordinate / x 坐标
	 * @param y The y-coordinate / y 坐标
	 * @return The bicubically interpolated value / 双三次插值结果
	 */
	T bicubicInterpolation(T x, T y) const
	{
		// Find surrounding x indices
		auto xIt = std::lower_bound(m_xAxis.begin(), m_xAxis.end(), x);
		size_type x0, x1, x2, x3;
		T xWeight;

		if (xIt == m_xAxis.begin()) {
			x0      = 0;
			x1      = 0;
			x2      = 1;
			x3      = 2;
			xWeight = 0.0;
		} else if (xIt == m_xAxis.end()) {
			x0      = m_xAxis.size() - 3;
			x1      = m_xAxis.size() - 2;
			x2      = m_xAxis.size() - 1;
			x3      = m_xAxis.size() - 1;
			xWeight = 1.0;
		} else {
			x0 = xIt - 2 - m_xAxis.begin();
			if (x0 < 0)
				x0 = 0;
			x1 = xIt - 1 - m_xAxis.begin();
			x2 = xIt - m_xAxis.begin();
			x3 = xIt + 1 - m_xAxis.begin();
			if (x3 >= m_xAxis.size())
				x3 = m_xAxis.size() - 1;
			xWeight = static_cast< T >(x - m_xAxis[ x1 ]) / (m_xAxis[ x2 ] - m_xAxis[ x1 ]);
		}

		// Find surrounding y indices
		auto yIt = std::lower_bound(m_yAxis.begin(), m_yAxis.end(), y);
		size_type y0, y1, y2, y3;
		T yWeight;

		if (yIt == m_yAxis.begin()) {
			y0      = 0;
			y1      = 0;
			y2      = 1;
			y3      = 2;
			yWeight = 0.0;
		} else if (yIt == m_yAxis.end()) {
			y0      = m_yAxis.size() - 3;
			y1      = m_yAxis.size() - 2;
			y2      = m_yAxis.size() - 1;
			y3      = m_yAxis.size() - 1;
			yWeight = 1.0;
		} else {
			y0 = yIt - 2 - m_yAxis.begin();
			if (y0 < 0)
				y0 = 0;
			y1 = yIt - 1 - m_yAxis.begin();
			y2 = yIt - m_yAxis.begin();
			y3 = yIt + 1 - m_yAxis.begin();
			if (y3 >= m_yAxis.size())
				y3 = m_yAxis.size() - 1;
			yWeight = static_cast< T >(y - m_yAxis[ y1 ]) / (m_yAxis[ y2 ] - m_yAxis[ y1 ]);
		}

		// Hermite basis functions
		auto h00 = [](T t) { return (1 + 2 * t) * (1 - t) * (1 - t); };
		auto h10 = [](T t) { return t * (1 - t) * (1 - t); };
		auto h01 = [](T t) { return t * t * (3 - 2 * t); };
		auto h11 = [](T t) { return t * t * (t - 1); };

		// Interpolate in x direction for each y position
		T values[ 4 ][ 4 ];
		for (int i = 0; i < 4; ++i) {
			// Get the four y values for current x positions
			T v[ 4 ];
			v[ 0 ] = m_data[ x0 ][ y0 + i ];
			v[ 1 ] = m_data[ x1 ][ y0 + i ];
			v[ 2 ] = m_data[ x2 ][ y0 + i ];
			v[ 3 ] = m_data[ x3 ][ y0 + i ];

			// Interpolate in x direction
			values[ 0 ][ i ] = h00(xWeight) * v[ 1 ]
                               + h10(xWeight) * (m_xAxis[ x2 ] - m_xAxis[ x1 ])
                                     * ((v[ 2 ] - v[ 1 ]) / (m_xAxis[ x2 ] - m_xAxis[ x1 ])
                                        + (v[ 2 ] - v[ 1 ] - (v[ 1 ] - v[ 0 ]) / (m_xAxis[ x1 ] - m_xAxis[ x0 ]))
                                              / (m_xAxis[ x2 ] - m_xAxis[ x0 ]) * (m_xAxis[ x2 ] - m_xAxis[ x1 ]))
                               + h01(xWeight) * v[ 2 ]
                               + h11(xWeight) * (m_xAxis[ x2 ] - m_xAxis[ x1 ])
                                     * ((v[ 2 ] - v[ 1 ]) / (m_xAxis[ x2 ] - m_xAxis[ x1 ])
                                        + (v[ 3 ] - v[ 2 ] - (v[ 2 ] - v[ 1 ]) / (m_xAxis[ x2 ] - m_xAxis[ x1 ]))
                                              / (m_xAxis[ x3 ] - m_xAxis[ x1 ]) * (m_xAxis[ x2 ] - m_xAxis[ x1 ]));
		}

		// Interpolate in y direction
		T v[ 4 ];
		v[ 0 ] = values[ 0 ][ 0 ];
		v[ 1 ] = values[ 0 ][ 1 ];
		v[ 2 ] = values[ 0 ][ 2 ];
		v[ 3 ] = values[ 0 ][ 3 ];

		return h00(yWeight) * v[ 1 ]
               + h10(yWeight) * (m_yAxis[ y2 ] - m_yAxis[ y1 ])
                     * ((v[ 2 ] - v[ 1 ]) / (m_yAxis[ y2 ] - m_yAxis[ y1 ])
                        + (v[ 2 ] - v[ 1 ] - (v[ 1 ] - v[ 0 ]) / (m_yAxis[ y1 ] - m_yAxis[ y0 ]))
                              / (m_yAxis[ y2 ] - m_yAxis[ y0 ]) * (m_yAxis[ y2 ] - m_yAxis[ y1 ]))
               + h01(yWeight) * v[ 2 ]
               + h11(yWeight) * (m_yAxis[ y2 ] - m_yAxis[ y1 ])
                     * ((v[ 2 ] - v[ 1 ]) / (m_yAxis[ y2 ] - m_yAxis[ y1 ])
                        + (v[ 3 ] - v[ 2 ] - (v[ 2 ] - v[ 1 ]) / (m_yAxis[ y2 ] - m_yAxis[ y1 ]))
                              / (m_yAxis[ y3 ] - m_yAxis[ y1 ]) * (m_yAxis[ y2 ] - m_yAxis[ y1 ]));
	}

private:
	XContainer< T > m_xAxis;  ///< @brief x-axis values / x 轴值
	YContainer< T > m_yAxis;  ///< @brief y-axis values / y 轴值
	/**
	 * @brief The 2D data matrix / 二维数据矩阵
	 *
	 * |column[0]|column[1]| ... |column[m]|
	 * +---------+---------+-----+---------+
	 * | [x0,yn] | [x1,yn] | ... | [xm,yn] | → yAxis[n] 对应行
	 * +---------+---------+-----+---------+
	 * |   ...   |   ...   | ... |   ...   |
	 * +---------+---------+-----+---------+
	 * | [x0,y1] | [x1,y1] | ... | [xm,y1] | → yAxis[1] 对应行
	 * +---------+---------+-----+---------+
	 * | [x0,y0] | [x1,y0] | ... | [xm,y0] | → yAxis[0] 对应行
	 * +---------+---------+-----+---------+
	 *      ↑          ↑      ↑       ↑
	 *  xAxis[0]   xAxis[1]  ...   xAxis[m]
	 */
	DataContainer< data_column_type > m_data;
	ResampleMode m_mode;                                     ///< @brief Current resampling mode / 当前抽样方法
	T m_xMin, m_xMax, m_yMin, m_yMax, m_dataMin, m_dataMax;  ///< @brief Bounds of the grid / 网格的边界
};

#endif  // QWT_GRID_DATA_HPP

/*** End of inlined file: qwt_grid_data.hpp ***/

/*** Start of inlined file: qwt_grid_raster_data.h ***/
#ifndef QWT_GRID_RASTER_DATA_H
#define QWT_GRID_RASTER_DATA_H

#if QT_VERSION < 0x060000
template< typename T >
class QVector;
#endif

/**
 * \class QwtGridRasterData
 * \brief A class that encapsulates grid data and provides interpolation methods.
 *
 * This class inherits from QwtRasterData and is used to represent 2D grid data.
 * It supports various interpolation methods such as nearest neighbor and bilinear interpolation.
 *
 * 此类继承自 QwtRasterData，用于表示二维网格数据。
 * 它支持多种插值方法，例如最近邻插值和双线性插值。
 */
class QWT_EXPORT QwtGridRasterData : public QwtRasterData
{
public:
	/*!
	   \brief Resampling algorithm
	   The default setting is NearestNeighbour;
	 */
	enum ResampleMode
	{
		/*!
		   Return the value from the matrix, that is nearest to the
		   the requested position.
		 */
		NearestNeighbour,

		/*!
		   Interpolate the value from the distances and values of the
		   4 surrounding values in the matrix,
		 */
		BilinearInterpolation,

		/*!
		   Interpolate the value from the 16 surrounding values in the
		   matrix using hermite bicubic interpolation
		 */
		BicubicInterpolation
	};

public:
	QwtGridRasterData();
	virtual ~QwtGridRasterData();

	//  Set the resampling algorithm
	void setResampleMode(ResampleMode mode);
	ResampleMode resampleMode() const;

	virtual QwtInterval interval(Qt::Axis axis) const QWT_OVERRIDE QWT_FINAL;

	/**
	 * @brief Set new x-axis, y-axis, and data matrix.
	 *
	 * data matrix is look like that:
	 *
	 * |column[0]|column[1]| ... |column[m]|
	 * +---------+---------+-----+---------+
	 * | [x0,yn] | [x1,yn] | ... | [xm,yn] | → yAxis[n] 对应行
	 * +---------+---------+-----+---------+
	 * |   ...   |   ...   | ... |   ...   |
	 * +---------+---------+-----+---------+
	 * | [x0,y1] | [x1,y1] | ... | [xm,y1] | → yAxis[1] 对应行
	 * +---------+---------+-----+---------+
	 * | [x0,y0] | [x1,y0] | ... | [xm,y0] | → yAxis[0] 对应行
	 * +---------+---------+-----+---------+
	 *      ↑          ↑      ↑       ↑
	 *  xAxis[0]   xAxis[1]  ...   xAxis[m]
	 *
	 *  so (data matrix).size = xAxis.size,(data matrix).at(n).size = yAxis.szie
	 *
	 * 设置新的 x 轴、y 轴和数据矩阵。
	 * 数据矩阵是一个vector<vector> ,数据矩阵.size = xAxis.size,数据矩阵.at(n).size = yAxis.size
	 *
	 * @param xAxis The x-axis values. / x 轴值。
	 * @param yAxis The y-axis values. / y 轴值。
	 * @param data The 2D data matrix. / 二维数据矩阵。
	 */
	void setValue(const QVector< double >& x, const QVector< double >& y, const QVector< QVector< double > >& v);
	virtual double value(double x, double y) const QWT_OVERRIDE;

	virtual QRectF pixelHint(const QRectF&) const QWT_OVERRIDE;

	// 获取尺寸
	int xSize() const;
	int ySize() const;
	std::pair< int, int > valueSize() const;

	// 获取value矩阵对应位置的值
	double atValue(int xIndex, int yIndex) const;

	// 获取x,y在索引位置对应的值
	double atX(int xIndex) const;
	double atY(int yIndex) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif  // QWTGRIDRASTERDATA_H

/*** End of inlined file: qwt_grid_raster_data.h ***/

/*** Start of inlined file: qwt_matrix_raster_data.h ***/
#ifndef QWT_MATRIX_RASTER_DATA_H
#define QWT_MATRIX_RASTER_DATA_H

#if QT_VERSION < 0x060000
template< typename T >
class QVector;
#endif

/*!
   \brief A class representing a matrix of values as raster data

   QwtMatrixRasterData implements an interface for a matrix of
   equidistant values, that can be used by a QwtPlotRasterItem.
   It implements a couple of resampling algorithms, to provide
   values for positions, that or not on the value matrix.
 */
class QWT_EXPORT QwtMatrixRasterData : public QwtRasterData
{
public:
	/*!
	   \brief Resampling algorithm
	   The default setting is NearestNeighbour;
	 */
	enum ResampleMode
	{
		/*!
		   Return the value from the matrix, that is nearest to the
		   the requested position.
		 */
		NearestNeighbour,

		/*!
		   Interpolate the value from the distances and values of the
		   4 surrounding values in the matrix,
		 */
		BilinearInterpolation,

		/*!
		   Interpolate the value from the 16 surrounding values in the
		   matrix using hermite bicubic interpolation
		 */
		BicubicInterpolation
	};

	QwtMatrixRasterData();
	virtual ~QwtMatrixRasterData();

	void setResampleMode(ResampleMode mode);
	ResampleMode resampleMode() const;

    void setInterval(Qt::Axis, const QwtInterval&);
    virtual QwtInterval interval(Qt::Axis axis) const QWT_OVERRIDE QWT_FINAL;

    void setValueMatrix(const QVector< double >& values, int numColumns);
	const QVector< double > valueMatrix() const;

    void setValue(int row, int col, double value);

	int numColumns() const;
	int numRows() const;

    virtual QRectF pixelHint(const QRectF&) const QWT_OVERRIDE;

    virtual double value(double x, double y) const QWT_OVERRIDE;

private:
	void update();

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_matrix_raster_data.h ***/

/*** Start of inlined file: qwt_date.h ***/
#ifndef QWT_DATE_H
#define QWT_DATE_H

#include <qdatetime.h>

/*!
   \brief A collection of methods around date/time values

   Qt offers convenient classes for dealing with date/time values,
   but Qwt uses coordinate systems that are based on doubles.
   QwtDate offers methods to translate from QDateTime to double and v.v.

   A double is interpreted as the number of milliseconds since
   1970-01-01T00:00:00 Universal Coordinated Time - also known
   as "The Epoch".

   While the range of the Julian day in Qt4 is limited to [0, MAX_INT],
   Qt5 stores it as qint64 offering a huge range of valid dates.
   As the significance of a double is below this ( assuming a
   fraction of 52 bits ) the translation is not
   bijective with rounding errors for dates very far from Epoch.
   For a resolution of 1 ms those start to happen for dates above the
   year 144683.

   An axis for a date/time interval is expected to be aligned
   and divided in time/date units like seconds, minutes, ...
   QwtDate offers several algorithms that are needed to
   calculate these axes.

   \sa QwtDateScaleEngine, QwtDateScaleDraw, QDate, QTime
 */
class QWT_EXPORT QwtDate
{
public:
	/*!
	   How to identify the first week of year differs between
	   countries.
	 */
	enum Week0Type
	{
		/*!
		   According to ISO 8601 the first week of a year is defined
		   as "the week with the year's first Thursday in it".

		   FirstThursday corresponds to the numbering that is
		   implemented in QDate::weekNumber().
		 */
		FirstThursday,

		/*!
            "The week with January 1.1 in it."

            In the U.S. this definition is more common than
            FirstThursday.
		 */
		FirstDay
	};

	/*!
	   Classification of an time interval

	   Time intervals needs to be classified to decide how to
	   align and divide it.
	 */
	enum IntervalType
	{
		//! The interval is related to milliseconds
		Millisecond,

		//! The interval is related to seconds
		Second,

		//! The interval is related to minutes
		Minute,

		//! The interval is related to hours
		Hour,

		//! The interval is related to days
		Day,

		//! The interval is related to weeks
		Week,

		//! The interval is related to months
		Month,

		//! The interval is related to years
		Year
	};

	enum
	{
		//! The Julian day of "The Epoch"
		JulianDayForEpoch = 2440588
	};

	static QDate minDate();
	static QDate maxDate();

    static QDateTime toDateTime(double value, Qt::TimeSpec = Qt::UTC);

    static double toDouble(const QDateTime&);

    static QDateTime ceil(const QDateTime&, IntervalType);
    static QDateTime floor(const QDateTime&, IntervalType);

    static QDate dateOfWeek0(int year, Week0Type);
    static int weekNumber(const QDate&, Week0Type);

    static int utcOffset(const QDateTime&);

    static QString toString(const QDateTime&, const QString& format, Week0Type);
};

#endif

/*** End of inlined file: qwt_date.h ***/

/*** Start of inlined file: qwt_axis.h ***/
#ifndef QWT_AXIS_H
#define QWT_AXIS_H

/*!
   Enums and methods for axes
 */
namespace QwtAxis
{
//! \brief Axis position
enum Position
{
    //! Y axis left of the canvas
    YLeft,

    //! Y axis right of the canvas
    YRight,

    //! X axis below the canvas
    XBottom,

    //! X axis above the canvas
    XTop
};

//! \brief Number of axis positions
enum
{
    AxisPositions = XTop + 1
};

bool isValid(int axisPos);
bool isYAxis(int axisPos);
bool isXAxis(int axisPos);
}

//! \return true, when axisPos is in the valid range [ YLeft, XTop ]
inline bool QwtAxis::isValid(int axisPos)
{
    return (axisPos >= 0 && axisPos < AxisPositions);
}

//! \return true, when axisPos is XBottom or XTop
inline bool QwtAxis::isXAxis(int axisPos)
{
    return (axisPos == XBottom) || (axisPos == XTop);
}

//! \return true, when axisPos is YLeft or YRight
inline bool QwtAxis::isYAxis(int axisPos)
{
    return (axisPos == YLeft) || (axisPos == YRight);
}

#endif

/*** End of inlined file: qwt_axis.h ***/

/*** Start of inlined file: qwt_axis_id.h ***/
#ifndef QWT_AXIS_ID_H
#define QWT_AXIS_ID_H

/*!
    \brief Axis identifier

    An axis id is one of values of QwtAxis::Position.

    QwtAxisId is a placeholder for future releases ( -> multiaxes branch ),
    where it is possible to have more than one axis at each side of a plot.

    \sa QwtAxis
 */
typedef int QwtAxisId;

#endif

/*** End of inlined file: qwt_axis_id.h ***/

/*** Start of inlined file: qwt_bezier.h ***/
#ifndef QWT_BEZIER_H
#define QWT_BEZIER_H

class QPointF;
class QPolygonF;

/*!
   \brief An implementation of the de Casteljau’s Algorithm for interpolating
         Bézier curves

   The flatness criterion for terminating the subdivision is based on
   "Piecewise Linear Approximation of Bézier Curves" by
   Roger Willcocks ( http://www.rops.org )

   This article explains the maths behind in a very nice way:
   https://jeremykun.com/2013/05/11/bezier-curves-and-picasso
 */
class QWT_EXPORT QwtBezier
{
public:
    QwtBezier(double tolerance = 0.5);
	~QwtBezier();

    void setTolerance(double tolerance);
	double tolerance() const;

    QPolygonF toPolygon(const QPointF& p1, const QPointF& cp1, const QPointF& cp2, const QPointF& p2) const;

    void appendToPolygon(const QPointF& p1, const QPointF& cp1, const QPointF& cp2, const QPointF& p2, QPolygonF& polygon) const;

    static QPointF pointAt(const QPointF& p1, const QPointF& cp1, const QPointF& cp2, const QPointF& p2, double t);

private:
	double m_tolerance;
	double m_flatness;
};

/*!
   \return Tolerance, that is used as criterion for the subdivision
   \sa setTolerance()
 */
inline double QwtBezier::tolerance() const
{
	return m_tolerance;
}

#endif

/*** End of inlined file: qwt_bezier.h ***/

/*** Start of inlined file: qwt_point_polar.h ***/
/*! \file */
#ifndef QWT_POINT_POLAR_H
#define QWT_POINT_POLAR_H

#include <qpoint.h>
#include <qmetatype.h>
#include <qmath.h>

/*!
   \brief A point in polar coordinates

   In polar coordinates a point is determined by an angle and a distance.
   See http://en.wikipedia.org/wiki/Polar_coordinate_system
 */

class QWT_EXPORT QwtPointPolar
{
public:
	QwtPointPolar();
    QwtPointPolar(double azimuth, double radius);
    QwtPointPolar(const QPointF&);

    void setPoint(const QPointF&);
	QPointF toPoint() const;

	bool isValid() const;
	bool isNull() const;

	double radius() const;
	double azimuth() const;

	double& rRadius();
	double& rAzimuth();

    void setRadius(double);
    void setAzimuth(double);

    bool operator==(const QwtPointPolar&) const;
    bool operator!=(const QwtPointPolar&) const;

	QwtPointPolar normalized() const;

private:
	double m_azimuth;
	double m_radius;
};

Q_DECLARE_TYPEINFO(QwtPointPolar, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(QwtPointPolar);

#ifndef QT_NO_DEBUG_STREAM
QWT_EXPORT QDebug operator<<(QDebug, const QwtPointPolar&);
#endif

/*!
    Constructs a null point, with a radius and azimuth set to 0.0.
    \sa QPointF::isNull()
 */
inline QwtPointPolar::QwtPointPolar() : m_azimuth(0.0), m_radius(0.0)
{
}

/*!
   Constructs a point with coordinates specified by radius and azimuth.

   \param azimuth Azimuth
   \param radius Radius
 */
inline QwtPointPolar::QwtPointPolar(double azimuth, double radius) : m_azimuth(azimuth), m_radius(radius)
{
}

//! Returns true if radius() >= 0.0
inline bool QwtPointPolar::isValid() const
{
	return m_radius >= 0.0;
}

//! Returns true if radius() >= 0.0
inline bool QwtPointPolar::isNull() const
{
	return m_radius == 0.0;
}

//! Returns the radius.
inline double QwtPointPolar::radius() const
{
	return m_radius;
}

//! Returns the azimuth.
inline double QwtPointPolar::azimuth() const
{
	return m_azimuth;
}

//! Returns the radius.
inline double& QwtPointPolar::rRadius()
{
	return m_radius;
}

//! Returns the azimuth.
inline double& QwtPointPolar::rAzimuth()
{
	return m_azimuth;
}

//! Sets the radius to radius.
inline void QwtPointPolar::setRadius(double radius)
{
	m_radius = radius;
}

//! Sets the azimuth to azimuth.
inline void QwtPointPolar::setAzimuth(double azimuth)
{
	m_azimuth = azimuth;
}

inline QPoint qwtPolar2Pos(const QPoint& pole, double radius, double angle)
{
    const double x = pole.x() + radius * std::cos(angle);
    const double y = pole.y() - radius * std::sin(angle);

    return QPoint(qRound(x), qRound(y));
}

inline QPoint qwtDegree2Pos(const QPoint& pole, double radius, double angle)
{
    return qwtPolar2Pos(pole, radius, angle / 180.0 * M_PI);
}

inline QPointF qwtPolar2Pos(const QPointF& pole, double radius, double angle)
{
    const double x = pole.x() + radius * std::cos(angle);
    const double y = pole.y() - radius * std::sin(angle);

    return QPointF(x, y);
}

inline QPointF qwtDegree2Pos(const QPointF& pole, double radius, double angle)
{
    return qwtPolar2Pos(pole, radius, angle / 180.0 * M_PI);
}

inline QPointF qwtFastPolar2Pos(const QPointF& pole, double radius, double angle)
{
    const double x = pole.x() + radius * qFastCos(angle);
    const double y = pole.y() - radius * qFastSin(angle);

    return QPointF(x, y);
}

inline QPointF qwtFastDegree2Pos(const QPointF& pole, double radius, double angle)
{
    return qwtFastPolar2Pos(pole, radius, angle / 180.0 * M_PI);
}

inline QwtPointPolar qwtFastPos2Polar(const QPointF& pos)
{
    return QwtPointPolar(qwtFastAtan2(pos.y(), pos.x()), qSqrt(qwtSqr(pos.x()) + qwtSqr(pos.y())));
}

#endif

/*** End of inlined file: qwt_point_polar.h ***/

/*** Start of inlined file: qwt_clipper.h ***/
#ifndef QWT_CLIPPER_H
#define QWT_CLIPPER_H

class QwtInterval;
class QPointF;
class QRect;
class QRectF;
class QPolygon;
class QPolygonF;

#if QT_VERSION < 0x060000
template< typename T >
class QVector;
#endif

/*!
   \brief Some clipping algorithms
 */

namespace QwtClipper
{
QWT_EXPORT void clipPolygon(const QRect&, QPolygon&, bool closePolygon = false);

QWT_EXPORT void clipPolygon(const QRectF&, QPolygon&, bool closePolygon = false);

QWT_EXPORT void clipPolygonF(const QRectF&, QPolygonF&, bool closePolygon = false);

QWT_EXPORT QPolygon clippedPolygon(const QRect&, const QPolygon&, bool closePolygon = false);

QWT_EXPORT QPolygon clippedPolygon(const QRectF&, const QPolygon&, bool closePolygon = false);

QWT_EXPORT QPolygonF clippedPolygonF(const QRectF&, const QPolygonF&, bool closePolygon = false);

QWT_EXPORT QVector< QwtInterval > clipCircle(const QRectF&, const QPointF&, double radius);
};

#endif

/*** End of inlined file: qwt_clipper.h ***/

/*** Start of inlined file: qwt_null_paintdevice.h ***/
#ifndef QWT_NULL_PAINT_DEVICE_H
#define QWT_NULL_PAINT_DEVICE_H

#include <qpaintdevice.h>
#include <qpaintengine.h>

/*!
   \brief A null paint device doing nothing

   Sometimes important layout/rendering geometries are not
   available or changeable from the public Qt class interface.
   ( f.e hidden in the style implementation ).

   QwtNullPaintDevice can be used to manipulate or filter out
   this information by analyzing the stream of paint primitives.

   F.e. QwtNullPaintDevice is used by QwtPlotCanvas to identify
   styled backgrounds with rounded corners.
 */

class QWT_EXPORT QwtNullPaintDevice : public QPaintDevice
{
public:
	/*!
	   \brief Render mode

	   \sa setMode(), mode()
	 */
	enum Mode
	{
		/*!
		   All vector graphic primitives are painted by
		   the corresponding draw methods
		 */
		NormalMode,

		/*!
		   Vector graphic primitives ( beside polygons ) are mapped to a QPainterPath
		   and are painted by drawPath. In PathMode mode
		   only a few draw methods are called:

		   - drawPath()
		   - drawPixmap()
		   - drawImage()
		   - drawPolygon()
		 */
		PolygonPathMode,

		/*!
		   Vector graphic primitives are mapped to a QPainterPath
		   and are painted by drawPath. In PathMode mode
		   only a few draw methods are called:

		   - drawPath()
		   - drawPixmap()
		   - drawImage()
		 */
		PathMode
	};

	QwtNullPaintDevice();
	virtual ~QwtNullPaintDevice();

    void setMode(Mode);
	Mode mode() const;

	virtual QPaintEngine* paintEngine() const QWT_OVERRIDE;

    virtual int metric(PaintDeviceMetric) const QWT_OVERRIDE;

    virtual void drawRects(const QRect*, int);
    virtual void drawRects(const QRectF*, int);

    virtual void drawLines(const QLine*, int);
    virtual void drawLines(const QLineF*, int);

	virtual void drawEllipse(const QRectF&);
	virtual void drawEllipse(const QRect&);

	virtual void drawPath(const QPainterPath&);

    virtual void drawPoints(const QPointF*, int);
    virtual void drawPoints(const QPoint*, int);

    virtual void drawPolygon(const QPointF*, int, QPaintEngine::PolygonDrawMode);

    virtual void drawPolygon(const QPoint*, int, QPaintEngine::PolygonDrawMode);

    virtual void drawPixmap(const QRectF&, const QPixmap&, const QRectF&);

	virtual void drawTextItem(const QPointF&, const QTextItem&);

    virtual void drawTiledPixmap(const QRectF&, const QPixmap&, const QPointF&);

    virtual void drawImage(const QRectF&, const QImage&, const QRectF&, Qt::ImageConversionFlags);

    virtual void updateState(const QPaintEngineState&);

protected:
	//! \return Size needed to implement metric()
	virtual QSize sizeMetrics() const = 0;

private:
	class PaintEngine;
	PaintEngine* m_engine;

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_null_paintdevice.h ***/

/*** Start of inlined file: qwt_stylesheet_recorder.h ***/
#ifndef QWTSTYLESHEETRECORDER_H
#define QWTSTYLESHEETRECORDER_H

#include <QList>
#include <QVector>
#include <QRectF>
#include <QRectF>
#include <QPainterPath>
#include <QBrush>
#include <QPointF>

class QWT_EXPORT QwtStyleSheetRecorder QWT_FINAL : public QwtNullPaintDevice
{
public:
	explicit QwtStyleSheetRecorder(const QSize& size);
	virtual void updateState(const QPaintEngineState& state) QWT_OVERRIDE;
	virtual void drawRects(const QRectF* rects, int count) QWT_OVERRIDE;
	virtual void drawRects(const QRect* rects, int count) QWT_OVERRIDE;
	virtual void drawPath(const QPainterPath& path) QWT_OVERRIDE;
	void setCornerRects(const QPainterPath& path);

protected:
	virtual QSize sizeMetrics() const QWT_OVERRIDE;

private:
	void alignCornerRects(const QRectF& rect);

public:
	QVector< QRectF > clipRects;

	struct Border
	{
		QList< QPainterPath > pathList;
		QList< QRectF > rectList;
		QRegion clipRegion;
	} border;

	struct Background
	{
		QPainterPath path;
		QBrush brush;
		QPointF origin;
	} background;

private:
	const QSize m_size;

	QPen m_pen;
	QBrush m_brush;
	QPointF m_origin;
};

#endif  // QWTSTYLESHEETRECORDER_H

/*** End of inlined file: qwt_stylesheet_recorder.h ***/

/*** Start of inlined file: qwt_painter_command.h ***/
#ifndef QWT_PAINTER_COMMAND_H
#define QWT_PAINTER_COMMAND_H

#include <qpaintengine.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qpolygon.h>
#include <qpainterpath.h>

class QPainterPath;

/*!
   QwtPainterCommand represents the attributes of a paint operation
   how it is used between QPainter and QPaintDevice

   It is used by QwtGraphic to record and replay paint operations

   \sa QwtGraphic::commands()
 */

class QWT_EXPORT QwtPainterCommand
{
public:
	//! Type of the paint command
	enum Type
	{
		//! Invalid command
		Invalid = -1,

		//! Draw a QPainterPath
		Path,

		//! Draw a QPixmap
		Pixmap,

		//! Draw a QImage
		Image,

		//! QPainter state change
		State
	};

	//! Attributes how to paint a QPixmap
	struct PixmapData
	{
		QRectF rect;
		QPixmap pixmap;
		QRectF subRect;
	};

	//! Attributes how to paint a QImage
	struct ImageData
	{
		QRectF rect;
		QImage image;
		QRectF subRect;
		Qt::ImageConversionFlags flags;
	};

	//! Attributes of a state change
	struct StateData
	{
		QPaintEngine::DirtyFlags flags;

		QPen pen;
		QBrush brush;
		QPointF brushOrigin;
		QBrush backgroundBrush;
		Qt::BGMode backgroundMode;
		QFont font;
		QTransform transform;

		Qt::ClipOperation clipOperation;
		QRegion clipRegion;
		QPainterPath clipPath;
		bool isClipEnabled;

		QPainter::RenderHints renderHints;
		QPainter::CompositionMode compositionMode;
		qreal opacity;
	};

	QwtPainterCommand();
	QwtPainterCommand(const QwtPainterCommand&);

    explicit QwtPainterCommand(const QPainterPath&);

    QwtPainterCommand(const QRectF& rect, const QPixmap&, const QRectF& subRect);

    QwtPainterCommand(const QRectF& rect, const QImage&, const QRectF& subRect, Qt::ImageConversionFlags);

    explicit QwtPainterCommand(const QPaintEngineState&);

	~QwtPainterCommand();

    QwtPainterCommand& operator=(const QwtPainterCommand&);

	Type type() const;

	QPainterPath* path();
	const QPainterPath* path() const;

	PixmapData* pixmapData();
	const PixmapData* pixmapData() const;

	ImageData* imageData();
	const ImageData* imageData() const;

	StateData* stateData();
	const StateData* stateData() const;

private:
    void copy(const QwtPainterCommand&);
	void reset();

	Type m_type;

    union {
		QPainterPath* m_path;
		PixmapData* m_pixmapData;
		ImageData* m_imageData;
		StateData* m_stateData;
	};
};

//! \return Type of the command
inline QwtPainterCommand::Type QwtPainterCommand::type() const
{
	return m_type;
}

//! \return Painter path to be painted
inline const QPainterPath* QwtPainterCommand::path() const
{
	return m_path;
}

//! \return Attributes how to paint a QPixmap
inline const QwtPainterCommand::PixmapData* QwtPainterCommand::pixmapData() const
{
	return m_pixmapData;
}

//! \return Attributes how to paint a QImage
inline const QwtPainterCommand::ImageData* QwtPainterCommand::imageData() const
{
	return m_imageData;
}

//! \return Attributes of a state change
inline const QwtPainterCommand::StateData* QwtPainterCommand::stateData() const
{
	return m_stateData;
}

#endif

/*** End of inlined file: qwt_painter_command.h ***/

/*** Start of inlined file: qwt_graphic.h ***/
#ifndef QWT_GRAPHIC_H
#define QWT_GRAPHIC_H

#include <qmetatype.h>

class QwtPainterCommand;
class QPixmap;
class QImage;

/*!
    \brief A paint device for scalable graphics

    QwtGraphic is the representation of a graphic that is tailored for
    scalability. Like QPicture it will be initialized by QPainter
    operations and can be replayed later to any target paint device.

    While the usual image representations QImage and QPixmap are not
    scalable Qt offers two paint devices, that might be candidates
    for representing a vector graphic:

    - QPicture\n
      Unfortunately QPicture had been forgotten, when Qt4
      introduced floating point based render engines. Its API
      is still on integers, what make it unusable for proper scaling.

    - QSvgRenderer/QSvgGenerator\n
      Unfortunately QSvgRenderer hides to much information about
      its nodes in internal APIs, that are necessary for proper
      layout calculations. Also it is derived from QObject and
      can't be copied like QImage/QPixmap.

    QwtGraphic maps all scalable drawing primitives to a QPainterPath
    and stores them together with the painter state changes
    ( pen, brush, transformation ... ) in a list of QwtPaintCommands.
    For being a complete QPaintDevice it also stores pixmaps or images,
    what is somehow against the idea of the class, because these objects
    can't be scaled without a loss in quality.

    The main issue about scaling a QwtGraphic object are the pens used for
    drawing the outlines of the painter paths. While non cosmetic pens
    ( QPen::isCosmetic() ) are scaled with the same ratio as the path,
    cosmetic pens have a fixed width. A graphic might have paths with
    different pens - cosmetic and non-cosmetic.

    QwtGraphic caches 2 different rectangles:

    - control point rectangle\n
      The control point rectangle is the bounding rectangle of all
      control point rectangles of the painter paths, or the target
      rectangle of the pixmaps/images.

    - bounding rectangle\n
      The bounding rectangle extends the control point rectangle by
      what is needed for rendering the outline with an unscaled pen.

    Because the offset for drawing the outline depends on the shape
    of the painter path ( the peak of a triangle is different than the flat side )
    scaling with a fixed aspect ratio always needs to be calculated from the
    control point rectangle.

    \sa QwtPainterCommand
 */
class QWT_EXPORT QwtGraphic : public QwtNullPaintDevice
{
public:
	/*!
        Hint how to render a graphic
        \sa setRenderHint(), testRenderHint()
	 */
	enum RenderHint
	{
		/*!
		   When rendering a QwtGraphic a specific scaling between
		   the controlPointRect() and the coordinates of the target rectangle
		   is set up internally in render().

		   When RenderPensUnscaled is set this specific scaling is applied
		   for the control points only, but not for the pens.
		   All other painter transformations ( set up by application code )
		   are supposed to work like usual.

		   \sa render();
		 */
		RenderPensUnscaled = 0x1
	};

    Q_DECLARE_FLAGS(RenderHints, RenderHint)

	/*!
	   Indicator if the graphic contains a specific type of painter command
	   \sa CommandTypes, commandTypes();
	 */
	enum CommandType
	{
		//! The graphic contains scalable vector data
        VectorData = 1 << 0,

		//! The graphic contains raster data ( QPixmap or QImage )
        RasterData = 1 << 1,

		//! The graphic contains transformations beyond simple translations
		Transformation = 1 << 2
	};

    Q_DECLARE_FLAGS(CommandTypes, CommandType)

	QwtGraphic();
    QwtGraphic(const QwtGraphic&);

	virtual ~QwtGraphic();

    QwtGraphic& operator=(const QwtGraphic&);

	void reset();

	bool isNull() const;
	bool isEmpty() const;

	CommandTypes commandTypes() const;

    void render(QPainter*) const;

    void render(QPainter*, const QSizeF&, Qt::AspectRatioMode = Qt::IgnoreAspectRatio) const;

    void render(QPainter*, const QPointF&, Qt::Alignment = Qt::AlignTop | Qt::AlignLeft) const;

    void render(QPainter*, const QRectF&, Qt::AspectRatioMode = Qt::IgnoreAspectRatio) const;

    QPixmap toPixmap(qreal devicePixelRatio = 0.0) const;

    QPixmap toPixmap(const QSize&, Qt::AspectRatioMode = Qt::IgnoreAspectRatio, qreal devicePixelRatio = 0.0) const;

    QImage toImage(qreal devicePixelRatio = 0.0) const;

    QImage toImage(const QSize&, Qt::AspectRatioMode = Qt::IgnoreAspectRatio, qreal devicePixelRatio = 0.0) const;

    QRectF scaledBoundingRect(qreal sx, qreal sy) const;

	QRectF boundingRect() const;
	QRectF controlPointRect() const;

	const QVector< QwtPainterCommand >& commands() const;
    void setCommands(const QVector< QwtPainterCommand >&);

    void setDefaultSize(const QSizeF&);
	QSizeF defaultSize() const;

    qreal heightForWidth(qreal width) const;
    qreal widthForHeight(qreal height) const;

    void setRenderHint(RenderHint, bool on = true);
    bool testRenderHint(RenderHint) const;

	RenderHints renderHints() const;

protected:
	virtual QSize sizeMetrics() const QWT_OVERRIDE;

    virtual void drawPath(const QPainterPath&) QWT_OVERRIDE;

    virtual void drawPixmap(const QRectF&, const QPixmap&, const QRectF&) QWT_OVERRIDE;

    virtual void drawImage(const QRectF&, const QImage&, const QRectF&, Qt::ImageConversionFlags) QWT_OVERRIDE;

    virtual void updateState(const QPaintEngineState&) QWT_OVERRIDE;

private:
    void renderGraphic(QPainter*, QTransform*) const;

    void updateBoundingRect(const QRectF&);
    void updateControlPointRect(const QRectF&);

	class PathInfo;

	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtGraphic::RenderHints)
Q_DECLARE_OPERATORS_FOR_FLAGS(QwtGraphic::CommandTypes)
Q_DECLARE_METATYPE(QwtGraphic)

#endif

/*** End of inlined file: qwt_graphic.h ***/

/*** Start of inlined file: qwt_event_pattern.h ***/
#ifndef QWT_EVENT_PATTERN
#define QWT_EVENT_PATTERN

#include <qnamespace.h>
#include <qvector.h>

class QMouseEvent;
class QKeyEvent;

/*!
   \brief A collection of event patterns

   QwtEventPattern introduces an level of indirection for mouse and
   keyboard inputs. Those are represented by symbolic names, so
   the application code can be configured by individual mappings.

   \sa QwtPicker, QwtPickerMachine, QwtPlotZoomer
 */
class QWT_EXPORT QwtEventPattern
{
public:
	/*!
	   \brief Symbolic mouse input codes

	   QwtEventPattern implements 3 different settings for
	   mice with 1, 2, or 3 buttons that can be activated
	   using initMousePattern(). The default setting is for
	   3 button mice.

	   Individual settings can be configured using setMousePattern().

	   \sa initMousePattern(), setMousePattern(), setKeyPattern()
	 */
	enum MousePatternCode
	{
		/*!
		   The default setting for 1, 2 and 3 button mice is:

		   - Qt::LeftButton
		   - Qt::LeftButton
		   - Qt::LeftButton
		 */
		MouseSelect1,

		/*!
		   The default setting for 1, 2 and 3 button mice is:

		   - Qt::LeftButton + Qt::ControlModifier
		   - Qt::RightButton
		   - Qt::RightButton
		 */
		MouseSelect2,

		/*!
		   The default setting for 1, 2 and 3 button mice is:

		   - Qt::LeftButton + Qt::AltModifier
		   - Qt::LeftButton + Qt::AltModifier
		   - Qt::MidButton
		 */
		MouseSelect3,

		/*!
		   The default setting for 1, 2 and 3 button mice is:

		   - Qt::LeftButton + Qt::ShiftModifier
		   - Qt::LeftButton + Qt::ShiftModifier
		   - Qt::LeftButton + Qt::ShiftModifier
		 */
		MouseSelect4,

		/*!
		   The default setting for 1, 2 and 3 button mice is:

		   - Qt::LeftButton + Qt::ControlButton | Qt::ShiftModifier
		   - Qt::RightButton + Qt::ShiftModifier
		   - Qt::RightButton + Qt::ShiftModifier
		 */
		MouseSelect5,

		/*!
		   The default setting for 1, 2 and 3 button mice is:

		   - Qt::LeftButton + Qt::AltModifier + Qt::ShiftModifier
		   - Qt::LeftButton + Qt::AltModifier | Qt::ShiftModifier
		   - Qt::MidButton + Qt::ShiftModifier
		 */
		MouseSelect6,

		//! Number of mouse patterns
		MousePatternCount
	};

	/*!
	   \brief Symbolic keyboard input codes

	   Individual settings can be configured using setKeyPattern()

	   \sa setKeyPattern(), setMousePattern()
	 */
	enum KeyPatternCode
	{
		//! Qt::Key_Return
		KeySelect1,

		//! Qt::Key_Space
		KeySelect2,

		//! Qt::Key_Escape
		KeyAbort,

		//! Qt::Key_Left
		KeyLeft,

		//! Qt::Key_Right
		KeyRight,

		//! Qt::Key_Up
		KeyUp,

		//! Qt::Key_Down
		KeyDown,

		//! Qt::Key_Plus
		KeyRedo,

		//! Qt::Key_Minus
		KeyUndo,

		//! Qt::Key_Escape
		KeyHome,

		//! Number of key patterns
		KeyPatternCount
	};

	//! A pattern for mouse events
	class MousePattern
	{
    public:
		//! Constructor
        MousePattern(Qt::MouseButton btn = Qt::NoButton, Qt::KeyboardModifiers modifierCodes = Qt::NoModifier)
            : button(btn), modifiers(modifierCodes)
		{
		}

		//! Button
		Qt::MouseButton button;

		//! Keyboard modifier
		Qt::KeyboardModifiers modifiers;
	};

	//! A pattern for key events
	class KeyPattern
	{
    public:
		//! Constructor
        KeyPattern(int keyCode = Qt::Key_unknown, Qt::KeyboardModifiers modifierCodes = Qt::NoModifier)
            : key(keyCode), modifiers(modifierCodes)
		{
		}

		//! Key code
		int key;

		//! Modifiers
		Qt::KeyboardModifiers modifiers;
	};

	QwtEventPattern();
	virtual ~QwtEventPattern();

    void initMousePattern(int numButtons);
	void initKeyPattern();

    void setMousePattern(MousePatternCode, Qt::MouseButton button, Qt::KeyboardModifiers = Qt::NoModifier);

    void setKeyPattern(KeyPatternCode, int key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    void setMousePattern(const QVector< MousePattern >&);
    void setKeyPattern(const QVector< KeyPattern >&);

	const QVector< MousePattern >& mousePattern() const;
	const QVector< KeyPattern >& keyPattern() const;

	QVector< MousePattern >& mousePattern();
	QVector< KeyPattern >& keyPattern();

    bool mouseMatch(MousePatternCode, const QMouseEvent*) const;
    bool keyMatch(KeyPatternCode, const QKeyEvent*) const;

protected:
    virtual bool mouseMatch(const MousePattern&, const QMouseEvent*) const;
    virtual bool keyMatch(const KeyPattern&, const QKeyEvent*) const;

private:
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
	QVector< MousePattern > m_mousePattern;
	QVector< KeyPattern > m_keyPattern;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
};

//! Compare operator
inline bool operator==(QwtEventPattern::MousePattern b1, QwtEventPattern::MousePattern b2)
{
	return b1.button == b2.button && b1.modifiers == b2.modifiers;
}

//! Compare operator
inline bool operator==(QwtEventPattern::KeyPattern b1, QwtEventPattern::KeyPattern b2)
{
	return b1.key == b2.key && b1.modifiers == b2.modifiers;
}

#endif

/*** End of inlined file: qwt_event_pattern.h ***/

/*** Start of inlined file: qwt_painter.h ***/
#ifndef QWT_PAINTER_H
#define QWT_PAINTER_H

#include <qpalette.h>
#include <qpolygon.h>
#include <qpen.h>

class QwtScaleMap;
class QwtColorMap;
class QwtInterval;

class QPainter;
class QBrush;
class QWidget;
class QImage;
class QPixmap;
class QFontMetrics;
class QFontMetricsF;
class QTextDocument;
class QPainterPath;

/*!
   \brief A collection of QPainter workarounds
 */
class QWT_EXPORT QwtPainter
{
public:
	static void setPolylineSplitting(bool);
	static bool polylineSplitting();

	static void setRoundingAlignment(bool);
	static bool roundingAlignment();
	static bool roundingAlignment(const QPainter*);

	static void drawText(QPainter*, qreal x, qreal y, const QString&);
	static void drawText(QPainter*, const QPointF&, const QString&);
	static void drawText(QPainter*, qreal x, qreal y, qreal w, qreal h, int flags, const QString&);
	static void drawText(QPainter*, const QRectF&, int flags, const QString&);

#ifndef QT_NO_RICHTEXT
	static void drawSimpleRichText(QPainter*, const QRectF&, int flags, const QTextDocument&);
#endif

	static void drawRect(QPainter*, qreal x, qreal y, qreal w, qreal h);
	static void drawRect(QPainter*, const QRectF& rect);
	static void fillRect(QPainter*, const QRectF&, const QBrush&);

	static void drawEllipse(QPainter*, const QRectF&);
	static void drawPie(QPainter*, const QRectF& r, int a, int alen);

	static void drawLine(QPainter*, qreal x1, qreal y1, qreal x2, qreal y2);
	static void drawLine(QPainter*, const QPointF& p1, const QPointF& p2);
	static void drawLine(QPainter*, const QLineF&);

	static void drawPolygon(QPainter*, const QPolygonF&);
	static void drawPolyline(QPainter*, const QPolygonF&);
	static void drawPolyline(QPainter*, const QPointF*, int pointCount);

	static void drawPolygon(QPainter*, const QPolygon&);
	static void drawPolyline(QPainter*, const QPolygon&);
	static void drawPolyline(QPainter*, const QPoint*, int pointCount);

	static void drawPoint(QPainter*, const QPoint&);
	static void drawPoints(QPainter*, const QPolygon&);
	static void drawPoints(QPainter*, const QPoint*, int pointCount);

	static void drawPoint(QPainter*, qreal x, qreal y);
	static void drawPoint(QPainter*, const QPointF&);
	static void drawPoints(QPainter*, const QPolygonF&);
	static void drawPoints(QPainter*, const QPointF*, int pointCount);

	static void drawPath(QPainter*, const QPainterPath&);
	static void drawImage(QPainter*, const QRectF&, const QImage&);
	static void drawPixmap(QPainter*, const QRectF&, const QPixmap&);

	static void drawRoundFrame(QPainter*, const QRectF&, const QPalette&, int lineWidth, int frameStyle);

	static void drawRoundedFrame(QPainter*, const QRectF&, qreal xRadius, qreal yRadius, const QPalette&, int lineWidth, int frameStyle);

	static void drawFrame(QPainter*,
                          const QRectF& rect,
                          const QPalette& palette,
                          QPalette::ColorRole foregroundRole,
                          int lineWidth,
                          int midLineWidth,
                          int frameStyle);

	static void drawFocusRect(QPainter*, const QWidget*);
	static void drawFocusRect(QPainter*, const QWidget*, const QRect&);

	static void drawColorBar(QPainter*, const QwtColorMap&, const QwtInterval&, const QwtScaleMap&, Qt::Orientation, const QRectF&);

	static bool isAligning(const QPainter*);
	static bool isX11GraphicsSystem();

	static void fillPixmap(const QWidget*, QPixmap&, const QPoint& offset = QPoint());
	static void fillRegion(QPainter* painter, const QRegion& region);
	static void fillBackground(QPainter* painter, QWidget* widget, const QVector< QRectF >& fillRects);
	static void fillBackground(QPainter* painter, QWidget* canvas);

	static void drawBackgound(QPainter*, const QRectF&, const QWidget*);
	// 绘制widget的背景
	static void drawCanvasBackgound(QPainter* painter, QWidget* canvas);

	static void drawStyledBackground(QWidget* w, QPainter* painter);

	static QPixmap backingStore(QWidget*, const QSize&);
	static qreal devicePixelRatio(const QPaintDevice*);

	static qreal effectivePenWidth(const QPen&);

	static int horizontalAdvance(const QFontMetrics&, const QString&);
	static qreal horizontalAdvance(const QFontMetricsF&, const QString&);

	static int horizontalAdvance(const QFontMetrics&, QChar);
	static qreal horizontalAdvance(const QFontMetricsF&, QChar);

	static QFont scaledFont(const QFont&, const QPaintDevice* = nullptr);

	static QWidget* findBackgroundWidget(QWidget* w);

private:
	static bool m_polylineSplitting;
	static bool m_roundingAlignment;
};

//!  Wrapper for QPainter::drawPoint()
inline void QwtPainter::drawPoint(QPainter* painter, qreal x, qreal y)
{
	QwtPainter::drawPoint(painter, QPointF(x, y));
}

//! Wrapper for QPainter::drawPoints()
inline void QwtPainter::drawPoints(QPainter* painter, const QPolygon& polygon)
{
	drawPoints(painter, polygon.data(), polygon.size());
}

//! Wrapper for QPainter::drawPoints()
inline void QwtPainter::drawPoints(QPainter* painter, const QPolygonF& polygon)
{
	drawPoints(painter, polygon.data(), polygon.size());
}

//!  Wrapper for QPainter::drawLine()
inline void QwtPainter::drawLine(QPainter* painter, qreal x1, qreal y1, qreal x2, qreal y2)
{
	QwtPainter::drawLine(painter, QPointF(x1, y1), QPointF(x2, y2));
}

//!  Wrapper for QPainter::drawLine()
inline void QwtPainter::drawLine(QPainter* painter, const QLineF& line)
{
	QwtPainter::drawLine(painter, line.p1(), line.p2());
}

/*!
   \return True, when line splitting for the raster paint engine is enabled.
   \sa setPolylineSplitting()
 */
inline bool QwtPainter::polylineSplitting()
{
	return m_polylineSplitting;
}

/*!
   Check whether coordinates should be rounded, before they are painted
   to a paint engine that rounds to integer values. For other paint engines
   ( PDF, SVG ), this flag has no effect.

   \return True, when rounding is enabled
   \sa setRoundingAlignment(), isAligning()
 */
inline bool QwtPainter::roundingAlignment()
{
	return m_roundingAlignment;
}

/*!
   \return roundingAlignment() && isAligning(painter);
   \param painter Painter
 */
inline bool QwtPainter::roundingAlignment(const QPainter* painter)
{
	return m_roundingAlignment && isAligning(painter);
}

/*!
   \return pen.widthF() expanded to at least 1.0
   \param pen Pen
 */
inline qreal QwtPainter::effectivePenWidth(const QPen& pen)
{
	const qreal width = pen.widthF();
	return (width < 1.0) ? 1.0 : width;
}

#endif

/*** End of inlined file: qwt_painter.h ***/

/*** Start of inlined file: qwt_text_engine.h ***/
#ifndef QWT_TEXT_ENGINE_H
#define QWT_TEXT_ENGINE_H

#include <qsize.h>

class QFont;
class QRectF;
class QString;
class QPainter;

/*!
   \brief Abstract base class for rendering text strings

   A text engine is responsible for rendering texts for a
   specific text format. They are used by QwtText to render a text.

   \sa QwtText::setTextEngine()
 */

class QWT_EXPORT QwtTextEngine
{
public:
	virtual ~QwtTextEngine();

	/*!
	   Find the height for a given width

	   \param font Font of the text
	   \param flags Bitwise OR of the flags used like in QPainter::drawText
	   \param text Text to be rendered
	   \param width Width

	   \return Calculated height
	 */
    virtual double heightForWidth(const QFont& font, int flags, const QString& text, double width) const = 0;

	/*!
	   Returns the size, that is needed to render text

	   \param font Font of the text
	   \param flags Bitwise OR of the flags like in for QPainter::drawText
	   \param text Text to be rendered

	   \return Calculated size
	 */
    virtual QSizeF textSize(const QFont& font, int flags, const QString& text) const = 0;

	/*!
	   Test if a string can be rendered by this text engine

	   \param text Text to be tested
	   \return true, if it can be rendered
	 */
    virtual bool mightRender(const QString& text) const = 0;

	/*!
	   Return margins around the texts

	   The textSize might include margins around the
	   text, like QFontMetrics::descent(). In situations
	   where texts need to be aligned in detail, knowing
	   these margins might improve the layout calculations.

	   \param font Font of the text
	   \param text Text to be rendered
	   \param left Return value for the left margin
	   \param right Return value for the right margin
	   \param top Return value for the top margin
	   \param bottom Return value for the bottom margin
	 */
    virtual void textMargins(const QFont& font,
                             const QString& text,
                             double& left,
                             double& right,
                             double& top,
                             double& bottom) const = 0;

	/*!
	   Draw the text in a clipping rectangle

	   \param painter Painter
	   \param rect Clipping rectangle
	   \param flags Bitwise OR of the flags like in for QPainter::drawText()
	   \param text Text to be rendered
	 */
    virtual void draw(QPainter* painter, const QRectF& rect, int flags, const QString& text) const = 0;

protected:
	QwtTextEngine();

private:
	Q_DISABLE_COPY(QwtTextEngine)
};

/*!
   \brief A text engine for plain texts

   QwtPlainTextEngine renders texts using the basic Qt classes
   QPainter and QFontMetrics.
 */
class QWT_EXPORT QwtPlainTextEngine : public QwtTextEngine
{
public:
	QwtPlainTextEngine();
	virtual ~QwtPlainTextEngine();

    virtual double heightForWidth(const QFont& font, int flags, const QString& text, double width) const QWT_OVERRIDE;

    virtual QSizeF textSize(const QFont& font, int flags, const QString& text) const QWT_OVERRIDE;

    virtual void draw(QPainter*, const QRectF& rect, int flags, const QString& text) const QWT_OVERRIDE;

    virtual bool mightRender(const QString&) const QWT_OVERRIDE;

    virtual void textMargins(const QFont&, const QString&, double& left, double& right, double& top, double& bottom) const QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

#ifndef QT_NO_RICHTEXT

/*!
   \brief A text engine for Qt rich texts

   QwtRichTextEngine renders Qt rich texts using the classes
   of the Scribe framework of Qt.
 */
class QWT_EXPORT QwtRichTextEngine : public QwtTextEngine
{
public:
	QwtRichTextEngine();

    virtual double heightForWidth(const QFont& font, int flags, const QString& text, double width) const QWT_OVERRIDE;

    virtual QSizeF textSize(const QFont& font, int flags, const QString& text) const QWT_OVERRIDE;

    virtual void draw(QPainter*, const QRectF& rect, int flags, const QString& text) const QWT_OVERRIDE;

    virtual bool mightRender(const QString&) const QWT_OVERRIDE;

    virtual void textMargins(const QFont&, const QString&, double& left, double& right, double& top, double& bottom) const QWT_OVERRIDE;

private:
    QString taggedText(const QString&, int flags) const;
};

#endif  // !QT_NO_RICHTEXT

#endif

/*** End of inlined file: qwt_text_engine.h ***/

/*** Start of inlined file: qwt_text.h ***/
#ifndef QWT_TEXT_H
#define QWT_TEXT_H

#include <qmetatype.h>

class QFont;
class QString;
class QColor;
class QPen;
class QBrush;
class QSizeF;
class QRectF;
class QPainter;
class QwtTextEngine;

/*!
   \brief A class representing a text

   A QwtText is a text including a set of attributes how to render it.

   - Format\n
    A text might include control sequences (f.e tags) describing
    how to render it. Each format (f.e MathML, TeX, Qt Rich Text)
    has its own set of control sequences, that can be handles by
    a special QwtTextEngine for this format.
   - Background\n
    A text might have a background, defined by a QPen and QBrush
    to improve its visibility. The corners of the background might
    be rounded.
   - Font\n
    A text might have an individual font.
   - Color\n
    A text might have an individual color.
   - Render Flags\n
    Flags from Qt::AlignmentFlag and Qt::TextFlag used like in
    QPainter::drawText().

   \sa QwtTextEngine, QwtTextLabel
 */

class QWT_EXPORT QwtText
{
public:
	/*!
	   \brief Text format

	   The text format defines the QwtTextEngine, that is used to render
	   the text.

	   \sa QwtTextEngine, setTextEngine()
	 */

	enum TextFormat
	{
		/*!
		   The text format is determined using QwtTextEngine::mightRender() for
		   all available text engines in increasing order > PlainText.
		   If none of the text engines can render the text is rendered
		   like QwtText::PlainText.
		 */
		AutoText = 0,

		//! Draw the text as it is, using a QwtPlainTextEngine.
		PlainText,

		//! Use the Scribe framework (Qt Rich Text) to render the text.
		RichText,

		/*!
		   Use a MathML (http://en.wikipedia.org/wiki/MathML) render engine
		   to display the text. In earlier versions of Qwt such an engine
		   was included - since Qwt 6.2 it can be found here:
		   https://github.com/uwerat/qwt-mml-dev

		   To enable MathML support the following code needs to be added to the
		   application:

		   \code
            QwtText::setTextEngine( QwtText::MathMLText, new QwtMathMLTextEngine() );
		   \endcode
		 */
		MathMLText,

		/*!
		   Use a TeX (http://en.wikipedia.org/wiki/TeX) render engine
		   to display the text ( not implemented yet ).
		 */
		TeXText,

		/*!
		   The number of text formats can be extended using setTextEngine.
		   Formats >= QwtText::OtherFormat are not used by Qwt.
		 */
		OtherFormat = 100
	};

	/*!
	   \brief Paint Attributes

	   Font and color and background are optional attributes of a QwtText.
	   The paint attributes hold the information, if they are set.
	 */
	enum PaintAttribute
	{
		//! The text has an individual font.
		PaintUsingTextFont = 0x01,

		//! The text has an individual color.
		PaintUsingTextColor = 0x02,

		//! The text has an individual background.
		PaintBackground = 0x04
	};

	Q_DECLARE_FLAGS(PaintAttributes, PaintAttribute)

	/*!
	   \brief Layout Attributes
	   The layout attributes affects some aspects of the layout of the text.
	 */
	enum LayoutAttribute
	{
		/*!
		   Layout the text without its margins. This mode is useful if a
		   text needs to be aligned accurately, like the tick labels of a scale.
		   If QwtTextEngine::textMargins is not implemented for the format
		   of the text, MinimumLayout has no effect.
		 */
		MinimumLayout = 0x01
	};

	Q_DECLARE_FLAGS(LayoutAttributes, LayoutAttribute)

	QwtText();
	QwtText(const QString&, TextFormat textFormat = AutoText);
	QwtText(const QwtText&);

	~QwtText();

	QwtText& operator=(const QwtText&);

	bool operator==(const QwtText&) const;
	bool operator!=(const QwtText&) const;

	void setText(const QString&, QwtText::TextFormat textFormat = AutoText);
	QString text() const;

	bool isNull() const;
	bool isEmpty() const;

	void setFont(const QFont&);
	QFont font() const;

	QFont usedFont(const QFont&) const;

	// get the current format
	TextFormat format() const;

	void setRenderFlags(int);
	int renderFlags() const;

	void setColor(const QColor&);
	QColor color() const;

	QColor usedColor(const QColor&) const;

	void setBorderRadius(double);
	double borderRadius() const;

	void setBorderPen(const QPen&);
	QPen borderPen() const;

	void setBackgroundBrush(const QBrush&);
	QBrush backgroundBrush() const;

	void setPaintAttribute(PaintAttribute, bool on = true);
	bool testPaintAttribute(PaintAttribute) const;

	void setLayoutAttribute(LayoutAttribute, bool on = true);
	bool testLayoutAttribute(LayoutAttribute) const;

	double heightForWidth(double width) const;
	double heightForWidth(double width, const QFont&) const;

	QSizeF textSize() const;
	QSizeF textSize(const QFont&) const;

	void draw(QPainter* painter, const QRectF& rect) const;

	static const QwtTextEngine* textEngine(const QString& text, QwtText::TextFormat = AutoText);

	static const QwtTextEngine* textEngine(QwtText::TextFormat);
	static void setTextEngine(QwtText::TextFormat, QwtTextEngine*);

private:
	class PrivateData;
	PrivateData* m_data;

	class LayoutCache;
	LayoutCache* m_layoutCache;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtText::PaintAttributes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QwtText::LayoutAttributes)

Q_DECLARE_METATYPE(QwtText)

#endif

/*** End of inlined file: qwt_text.h ***/

/*** Start of inlined file: qwt_text_label.h ***/
#ifndef QWT_TEXT_LABEL_H
#define QWT_TEXT_LABEL_H

#include <qframe.h>

class QString;
class QPaintEvent;
class QPainter;

/*!
   \brief A Widget which displays a QwtText
 */

class QWT_EXPORT QwtTextLabel : public QFrame
{
	Q_OBJECT

    Q_PROPERTY(int indent READ indent WRITE setIndent)
    Q_PROPERTY(int margin READ margin WRITE setMargin)
    Q_PROPERTY(QString plainText READ plainText WRITE setPlainText)

public:
    explicit QwtTextLabel(QWidget* parent = NULL);
    explicit QwtTextLabel(const QwtText&, QWidget* parent = NULL);
	virtual ~QwtTextLabel();

    void setPlainText(const QString&);
	QString plainText() const;

public Q_SLOTS:
    void setText(const QString&, QwtText::TextFormat textFormat = QwtText::AutoText);
    virtual void setText(const QwtText&);

	void clear();

public:
	const QwtText& text() const;

	int indent() const;
    void setIndent(int);

	int margin() const;
    void setMargin(int);

	virtual QSize sizeHint() const QWT_OVERRIDE;
	virtual QSize minimumSizeHint() const QWT_OVERRIDE;
    virtual int heightForWidth(int) const QWT_OVERRIDE;

	QRect textRect() const;

    virtual void drawText(QPainter*, const QRectF&);

protected:
    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;
    virtual void drawContents(QPainter*);

private:
	void init();
	int defaultIndent() const;

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_text_label.h ***/

/*** Start of inlined file: qwt_widget_overlay.h ***/
#ifndef QWT_WIDGET_OVERLAY_H
#define QWT_WIDGET_OVERLAY_H

#include <qwidget.h>

class QPainter;
class QRegion;

/*!
   \brief An overlay for a widget

   The main use case of an widget overlay is to avoid
   heavy repaint operation of the widget below.

   F.e. in combination with the plot canvas an overlay
   avoid replots as the content of the canvas can be restored from
   its backing store.

   QwtWidgetOverlay is an abstract base class. Deriving classes are
   supposed to reimplement the following methods:

   - drawOverlay()
   - maskHint()

   Internally QwtPlotPicker uses overlays for displaying
   the rubber band and the tracker text.

   \sa QwtPlotCanvas::BackingStore
 */
class QWT_EXPORT QwtWidgetOverlay : public QWidget
{
	Q_OBJECT
public:
	/*!
	   \brief Mask mode

	   When using masks the widget below gets paint events for
	   the masked regions of the overlay only. Otherwise
	   Qt triggers full repaints. On less powerful hardware
	   ( f.e embedded systems ) - or when using the raster paint
	   engine on a remote desktop - bit blitting is a noticeable
	   operation, that needs to be avoided.

	   If and how to mask depends on how expensive the calculation
	   of the mask is and how many pixels can be excluded by the mask.

	   The default setting is MaskHint.

	   \sa setMaskMode(), maskMode()
	 */
	enum MaskMode
	{
		//! Don't use a mask.
		NoMask,

		/*!
		   \brief Use maskHint() as mask

		   For many situations a fast approximation is good enough
		   and it is not necessary to build a more detailed mask
		   ( f.e the bounding rectangle of a text ).
		 */
		MaskHint,

		/*!
		   \brief Calculate a mask by checking the alpha values

		   Sometimes it is not possible to give a fast approximation
		   and the mask needs to be calculated by drawing the overlay
		   and testing the result.

		   When a valid maskHint() is available
		   only pixels inside this approximation are checked.
		 */
		AlphaMask
	};

	/*!
	   \brief Render mode

	   For calculating the alpha mask the overlay has already
	   been painted to a temporary QImage. Instead of rendering
	   the overlay twice this buffer can be copied for drawing
	   the overlay.

	   On graphic systems using the raster paint engine ( QWS, Windows )
	   it means usually copying some memory only. On X11 it results in an
	   expensive operation building a pixmap and for simple overlays
	   it might not be recommended.

	   \note The render mode has no effect, when maskMode() != AlphaMask.
	 */
	enum RenderMode
	{
		//! Copy the buffer, when using the raster paint engine.
		AutoRenderMode,

		//! Always copy the buffer
		CopyAlphaMask,

		//! Never copy the buffer
		DrawOverlay
	};

	explicit QwtWidgetOverlay(QWidget*);
	virtual ~QwtWidgetOverlay();

	void setMaskMode(MaskMode);
	MaskMode maskMode() const;

	void setRenderMode(RenderMode);
	RenderMode renderMode() const;

	virtual bool eventFilter(QObject*, QEvent*) QWT_OVERRIDE;

public Q_SLOTS:
	void updateOverlay();

protected:
	virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;
	virtual void resizeEvent(QResizeEvent*) QWT_OVERRIDE;

	virtual QRegion maskHint() const;

	/*!
	   Draw the widget overlay
	   \param painter Painter
	 */
	virtual void drawOverlay(QPainter* painter) const = 0;

private:
	void updateMask();
	void draw(QPainter*) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_widget_overlay.h ***/

/*** Start of inlined file: qwt_legend_data.h ***/
#ifndef QWT_LEGEND_DATA_H
#define QWT_LEGEND_DATA_H

#include <qvariant.h>
#include <qmap.h>

class QwtText;
class QwtGraphic;

/*!
   \brief Attributes of an entry on a legend

   QwtLegendData is an abstract container ( like QAbstractModel )
   to exchange attributes, that are only known between to
   the plot item and the legend.

   By overloading QwtPlotItem::legendData() any other set of attributes
   could be used, that can be handled by a modified ( or completely
   different ) implementation of a legend.

   \sa QwtLegend, QwtPlotLegendItem
   \note The stockchart example implements a legend as a tree
        with checkable items
 */
class QWT_EXPORT QwtLegendData
{
public:
	//! Mode defining how a legend entry interacts
	enum Mode
	{
		//! The legend item is not interactive, like a label
		ReadOnly,

		//! The legend item is clickable, like a push button
		Clickable,

		//! The legend item is checkable, like a checkable button
		Checkable
	};

	//! Identifier how to interpret a QVariant
	enum Role
	{
		// The value is a Mode
		ModeRole,

		// The value is a title
		TitleRole,

		// The value is an icon
		IconRole,

		// Values < UserRole are reserved for internal use
        UserRole = 32
	};

	QwtLegendData();
	~QwtLegendData();

    void setValues(const QMap< int, QVariant >&);
	const QMap< int, QVariant >& values() const;

    void setValue(int role, const QVariant&);
    QVariant value(int role) const;

    bool hasRole(int role) const;
	bool isValid() const;

	QwtGraphic icon() const;
	QwtText title() const;
	Mode mode() const;

private:
	QMap< int, QVariant > m_map;
};

#endif

/*** End of inlined file: qwt_legend_data.h ***/

/*** Start of inlined file: qwt_legend_label.h ***/
#ifndef QWT_LEGEND_LABEL_H
#define QWT_LEGEND_LABEL_H

class QwtText;

/*!
   \brief A widget representing something on a QwtLegend.
 */
class QWT_EXPORT QwtLegendLabel : public QwtTextLabel
{
	Q_OBJECT
public:
    explicit QwtLegendLabel(QWidget* parent = 0);
	virtual ~QwtLegendLabel();

    void setData(const QwtLegendData&);
	const QwtLegendData& data() const;

    void setItemMode(QwtLegendData::Mode);
	QwtLegendData::Mode itemMode() const;

    void setSpacing(int spacing);
	int spacing() const;

    virtual void setText(const QwtText&) QWT_OVERRIDE;

    void setIcon(const QPixmap&);
	QPixmap icon() const;

	virtual QSize sizeHint() const QWT_OVERRIDE;

	bool isChecked() const;

public Q_SLOTS:
    void setChecked(bool on);

Q_SIGNALS:
	//! Signal, when the legend item has been clicked
	void clicked();

	//! Signal, when the legend item has been pressed
	void pressed();

	//! Signal, when the legend item has been released
	void released();

	//! Signal, when the legend item has been toggled
    void checked(bool);

protected:
    void setDown(bool);
	bool isDown() const;

    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent*) QWT_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent*) QWT_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent*) QWT_OVERRIDE;
    virtual void keyReleaseEvent(QKeyEvent*) QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_legend_label.h ***/

/*** Start of inlined file: qwt_legend.h ***/
#ifndef QWT_LEGEND_H
#define QWT_LEGEND_H

/*** Start of inlined file: qwt_abstract_legend.h ***/
#ifndef QWT_ABSTRACT_LEGEND_H
#define QWT_ABSTRACT_LEGEND_H

#include <qframe.h>

class QwtLegendData;
template< typename T >
class QList;
class QVariant;

/*!
   \brief Abstract base class for legend widgets

   Legends, that need to be under control of the QwtPlot layout system
   need to be derived from QwtAbstractLegend.

   \note Other type of legends can be implemented by connecting to
        the QwtPlot::legendDataChanged() signal. But as these legends
        are unknown to the plot layout system the layout code
        ( on screen and for QwtPlotRenderer ) need to be organized
        in application code.

   \sa QwtLegend
 */
class QWT_EXPORT QwtAbstractLegend : public QFrame
{
	Q_OBJECT

public:
    explicit QwtAbstractLegend(QWidget* parent = NULL);
	virtual ~QwtAbstractLegend();

	/*!
	   Render the legend into a given rectangle.

	   \param painter Painter
	   \param rect Bounding rectangle
	   \param fillBackground When true, fill rect with the widget background

	   \sa renderLegend() is used by QwtPlotRenderer
	 */
    virtual void renderLegend(QPainter* painter, const QRectF& rect, bool fillBackground) const = 0;

	//! \return True, when no plot item is inserted
	virtual bool isEmpty() const = 0;

    virtual int scrollExtent(Qt::Orientation) const;

public Q_SLOTS:

	/*!
	   \brief Update the entries for a plot item

	   \param itemInfo Info about an item
	   \param data List of legend entry attributes for the  item
	 */
    virtual void updateLegend(const QVariant& itemInfo, const QList< QwtLegendData >& data) = 0;
};

#endif

/*** End of inlined file: qwt_abstract_legend.h ***/

#include <qvariant.h>

class QScrollBar;

/*!
   \brief The legend widget

   The QwtLegend widget is a tabular arrangement of legend items. Legend
   items might be any type of widget, but in general they will be
   a QwtLegendLabel.

   \sa QwtLegendLabel, QwtPlotItem, QwtPlot
 */

class QWT_EXPORT QwtLegend : public QwtAbstractLegend
{
	Q_OBJECT

public:
    explicit QwtLegend(QWidget* parent = NULL);
	virtual ~QwtLegend();

    void setMaxColumns(uint numColums);
	uint maxColumns() const;

    void setDefaultItemMode(QwtLegendData::Mode);
	QwtLegendData::Mode defaultItemMode() const;

	QWidget* contentsWidget();
	const QWidget* contentsWidget() const;

    QWidget* legendWidget(const QVariant&) const;
    QList< QWidget* > legendWidgets(const QVariant&) const;

    QVariant itemInfo(const QWidget*) const;

    virtual bool eventFilter(QObject*, QEvent*) QWT_OVERRIDE;

	virtual QSize sizeHint() const QWT_OVERRIDE;
    virtual int heightForWidth(int w) const QWT_OVERRIDE;

	QScrollBar* horizontalScrollBar() const;
	QScrollBar* verticalScrollBar() const;

    virtual void renderLegend(QPainter*, const QRectF&, bool fillBackground) const QWT_OVERRIDE;

    virtual void renderItem(QPainter*, const QWidget*, const QRectF&, bool fillBackground) const;

	virtual bool isEmpty() const QWT_OVERRIDE;
    virtual int scrollExtent(Qt::Orientation) const QWT_OVERRIDE;

Q_SIGNALS:
	/*!
	   A signal which is emitted when the user has clicked on
	   a legend label, which is in QwtLegendData::Clickable mode.

	   \param itemInfo Info for the item item of the
                      selected legend item
	   \param index Index of the legend label in the list of widgets
                   that are associated with the plot item

	   \note clicks are disabled as default
	   \sa setDefaultItemMode(), defaultItemMode(), QwtPlot::itemToInfo()
	 */
    void clicked(const QVariant& itemInfo, int index);

	/*!
	   A signal which is emitted when the user has clicked on
	   a legend label, which is in QwtLegendData::Checkable mode

	   \param itemInfo Info for the item of the
                      selected legend label
	   \param index Index of the legend label in the list of widgets
                   that are associated with the plot item
	   \param on True when the legend label is checked

	   \note clicks are disabled as default
	   \sa setDefaultItemMode(), defaultItemMode(), QwtPlot::itemToInfo()
	 */
    void checked(const QVariant& itemInfo, bool on, int index);

public Q_SLOTS:
    virtual void updateLegend(const QVariant&, const QList< QwtLegendData >&) QWT_OVERRIDE;

protected Q_SLOTS:
	void itemClicked();
    void itemChecked(bool);

protected:
    virtual QWidget* createWidget(const QwtLegendData&) const;
    virtual void updateWidget(QWidget*, const QwtLegendData&);

private:
	void updateTabOrder();

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_legend.h ***/

/*** Start of inlined file: qwt_arrow_button.h ***/
#ifndef QWT_ARROW_BUTTON_H
#define QWT_ARROW_BUTTON_H

#include <qpushbutton.h>

/*!
   \brief Arrow Button

   A push button with one or more filled triangles on its front.
   An Arrow button can have 1 to 3 arrows in a row, pointing
   up, down, left or right.
 */
class QWT_EXPORT QwtArrowButton : public QPushButton
{
public:
    explicit QwtArrowButton(int num, Qt::ArrowType, QWidget* parent = NULL);
	virtual ~QwtArrowButton();

	Qt::ArrowType arrowType() const;
	int num() const;

	virtual QSize sizeHint() const QWT_OVERRIDE;
	virtual QSize minimumSizeHint() const QWT_OVERRIDE;

protected:
    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent*) QWT_OVERRIDE;

    virtual void drawButtonLabel(QPainter*);
    virtual void drawArrow(QPainter*, const QRect&, Qt::ArrowType) const;
	virtual QRect labelRect() const;
    virtual QSize arrowSize(Qt::ArrowType, const QSize& boundingSize) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_arrow_button.h ***/

/*** Start of inlined file: qwt_compass_rose.h ***/
#ifndef QWT_COMPASS_ROSE_H
#define QWT_COMPASS_ROSE_H

#include <qpalette.h>

class QPainter;

/*!
   \brief Abstract base class for a compass rose
 */
class QWT_EXPORT QwtCompassRose
{
public:
	QwtCompassRose();
	virtual ~QwtCompassRose();

    virtual void setPalette(const QPalette&);
	const QPalette& palette() const;

	/*!
        Draw the rose

        \param painter Painter
        \param center Center point
        \param radius Radius of the rose
        \param north Position
        \param colorGroup Color group
	 */
    virtual void draw(QPainter* painter,
                      const QPointF& center,
                      double radius,
                      double north,
                      QPalette::ColorGroup colorGroup = QPalette::Active) const = 0;

private:
	Q_DISABLE_COPY(QwtCompassRose)

	QPalette m_palette;
};

/*!
   \brief A simple rose for QwtCompass
 */
class QWT_EXPORT QwtSimpleCompassRose : public QwtCompassRose
{
public:
    QwtSimpleCompassRose(int numThorns = 8, int numThornLevels = -1);
	virtual ~QwtSimpleCompassRose();

    void setWidth(double);
	double width() const;

    void setNumThorns(int);
	int numThorns() const;

    void setNumThornLevels(int);
	int numThornLevels() const;

    void setShrinkFactor(double factor);
	double shrinkFactor() const;

    virtual void draw(QPainter*,
                      const QPointF& center,
                      double radius,
                      double north,
                      QPalette::ColorGroup = QPalette::Active) const QWT_OVERRIDE;

    static void drawRose(QPainter*,
                         const QPalette&,
                         const QPointF& center,
                         double radius,
                         double north,
                         double width,
                         int numThorns,
                         int numThornLevels,
                         double shrinkFactor);

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_compass_rose.h ***/

/*** Start of inlined file: qwt_counter.h ***/
#ifndef QWT_COUNTER_H
#define QWT_COUNTER_H

#include <qwidget.h>

/*!
   \brief The Counter Widget

   A Counter consists of a label displaying a number and
   one ore more (up to three) push buttons on each side
   of the label which can be used to increment or decrement
   the counter's value.

   A counter has a range from a minimum value to a maximum value
   and a step size. When the wrapping property is set
   the counter is circular.

   The number of steps by which a button increments or decrements the value
   can be specified using setIncSteps(). The number of buttons can be
   changed with setNumButtons().

   Example:
   \code

   QwtCounter *counter = new QwtCounter(parent);

   counter->setRange(0.0, 100.0);                  // From 0.0 to 100
   counter->setSingleStep( 1.0 );                  // Step size 1.0
   counter->setNumButtons(2);                      // Two buttons each side
   counter->setIncSteps(QwtCounter::Button1, 1);   // Button 1 increments 1 step
   counter->setIncSteps(QwtCounter::Button2, 20);  // Button 2 increments 20 steps

   connect(counter, SIGNAL(valueChanged(double)), myClass, SLOT(newValue(double)));
   \endcode
 */

class QWT_EXPORT QwtCounter : public QWidget
{
	Q_OBJECT

    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged USER true)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)

    Q_PROPERTY(int numButtons READ numButtons WRITE setNumButtons)
    Q_PROPERTY(int stepButton1 READ stepButton1 WRITE setStepButton1)
    Q_PROPERTY(int stepButton2 READ stepButton2 WRITE setStepButton2)
    Q_PROPERTY(int stepButton3 READ stepButton3 WRITE setStepButton3)

    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(bool wrapping READ wrapping WRITE setWrapping)

public:
	//! Button index
	enum Button
	{
		//! Button intended for minor steps
		Button1,

		//! Button intended for medium steps
		Button2,

		//! Button intended for large steps
		Button3,

		//! Number of buttons
		ButtonCnt
	};

    explicit QwtCounter(QWidget* parent = NULL);
	virtual ~QwtCounter();

    void setValid(bool);
	bool isValid() const;

    void setWrapping(bool);
	bool wrapping() const;

	bool isReadOnly() const;
    void setReadOnly(bool);

    void setNumButtons(int);
	int numButtons() const;

    void setIncSteps(QwtCounter::Button, int numSteps);
    int incSteps(QwtCounter::Button) const;

	virtual QSize sizeHint() const QWT_OVERRIDE;

	double singleStep() const;
    void setSingleStep(double stepSize);

    void setRange(double min, double max);

	double minimum() const;
    void setMinimum(double);

	double maximum() const;
    void setMaximum(double);

    void setStepButton1(int nSteps);
	int stepButton1() const;

    void setStepButton2(int nSteps);
	int stepButton2() const;

    void setStepButton3(int nSteps);
	int stepButton3() const;

	double value() const;

public Q_SLOTS:
    void setValue(double);

Q_SIGNALS:
	/*!
        This signal is emitted when a button has been released
        \param value The new value
	 */
    void buttonReleased(double value);

	/*!
        This signal is emitted when the counter's value has changed
        \param value The new value
	 */
    void valueChanged(double value);

protected:
    virtual bool event(QEvent*) QWT_OVERRIDE;
    virtual void wheelEvent(QWheelEvent*) QWT_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent*) QWT_OVERRIDE;

private Q_SLOTS:
	void btnReleased();
	void btnClicked();
	void textChanged();

private:
    void incrementValue(int numSteps);
	void initCounter();
	void updateButtons();
    void showNumber(double);

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_counter.h ***/

/*** Start of inlined file: qwt_polar_fitter.h ***/
#ifndef QWT_POLAR_FITTER_H
#define QWT_POLAR_FITTER_H

/*!
   \brief A simple curve fitter for polar points

   QwtPolarFitter adds equidistant points between 2 curve points,
   so that the connection gets rounded according to the nature of
   a polar plot.

   \sa QwtPolarCurve::setCurveFitter()
 */
class QWT_EXPORT QwtPolarFitter : public QwtCurveFitter
{
public:
    QwtPolarFitter(int stepCount = 5);
	virtual ~QwtPolarFitter();

    void setStepCount(int size);
	int stepCount() const;

    virtual QPolygonF fitCurve(const QPolygonF&) const QWT_OVERRIDE;
    virtual QPainterPath fitCurvePath(const QPolygonF&) const QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_polar_fitter.h ***/

/*** Start of inlined file: qwt_sampling_thread.h ***/
#ifndef QWT_SAMPLING_THREAD_H
#define QWT_SAMPLING_THREAD_H

#include <qthread.h>

/*!
   \brief A thread collecting samples at regular intervals.

   Continuous signals are converted into a discrete signal by
   collecting samples at regular intervals. A discrete signal
   can be displayed by a QwtPlotSeriesItem on a QwtPlot widget.

   QwtSamplingThread starts a thread calling periodically sample(),
   to collect and store ( or emit ) a single sample.

   \sa QwtPlotCurve, QwtPlotSeriesItem
 */
class QWT_EXPORT QwtSamplingThread : public QThread
{
	Q_OBJECT

public:
	virtual ~QwtSamplingThread();

	double interval() const;
	double elapsed() const;

public Q_SLOTS:
    void setInterval(double interval);
	void stop();

protected:
    explicit QwtSamplingThread(QObject* parent = NULL);

	virtual void run() QWT_OVERRIDE;

	/*!
	   Collect a sample

	   \param elapsed Time since the thread was started in seconds
	   \note Due to a bug in previous version elapsed was passed as
             seconds instead of miliseconds. To avoid breaking existing
             code we stay with seconds for now.
	 */
    virtual void sample(double elapsed) = 0;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_sampling_thread.h ***/

/*** Start of inlined file: qwt_scale_div.h ***/
#ifndef QWT_SCALE_DIV_H
#define QWT_SCALE_DIV_H

#include <qlist.h>

class QwtInterval;

/*!
   \brief A class representing a scale division

   A Qwt scale is defined by its boundaries and 3 list
   for the positions of the major, medium and minor ticks.

   The upperBound() might be smaller than the lowerBound()
   to indicate inverted scales.

   Scale divisions can be calculated from a QwtScaleEngine.

   \sa QwtScaleEngine::divideScale(), QwtPlot::setAxisScaleDiv(),
      QwtAbstractSlider::setScaleDiv()
 */

class QWT_EXPORT QwtScaleDiv
{
public:
	//! Scale tick types
	enum TickType
	{
		//! No ticks
		NoTick = -1,

		//! Minor ticks
		MinorTick,

		//! Medium ticks
		MediumTick,

		//! Major ticks
		MajorTick,

		//! Number of valid tick types
		NTickTypes
	};

    explicit QwtScaleDiv(double lowerBound = 0.0, double upperBound = 0.0);

    explicit QwtScaleDiv(const QwtInterval&, QList< double >[ NTickTypes ]);

    explicit QwtScaleDiv(double lowerBound, double upperBound, QList< double >[ NTickTypes ]);

    explicit QwtScaleDiv(double lowerBound,
                         double upperBound,
                         const QList< double >& minorTicks,
                         const QList< double >& mediumTicks,
                         const QList< double >& majorTicks);

    bool operator==(const QwtScaleDiv&) const;
    bool operator!=(const QwtScaleDiv&) const;

    void setInterval(double lowerBound, double upperBound);
    void setInterval(const QwtInterval&);
	QwtInterval interval() const;

    void setLowerBound(double);
	double lowerBound() const;

    void setUpperBound(double);
	double upperBound() const;

	double range() const;

    bool contains(double value) const;

    void setTicks(int tickType, const QList< double >&);
    QList< double > ticks(int tickType) const;

	bool isEmpty() const;
	bool isIncreasing() const;

	void invert();
	QwtScaleDiv inverted() const;

    QwtScaleDiv bounded(double lowerBound, double upperBound) const;

private:
	double m_lowerBound;
	double m_upperBound;
    QList< double > m_ticks[ NTickTypes ];
};

Q_DECLARE_TYPEINFO(QwtScaleDiv, Q_MOVABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
QWT_EXPORT QDebug operator<<(QDebug, const QwtScaleDiv&);
#endif

#endif

/*** End of inlined file: qwt_scale_div.h ***/

/*** Start of inlined file: qwt_scale_draw.h ***/
#ifndef QWT_SCALE_DRAW_H
#define QWT_SCALE_DRAW_H

/*** Start of inlined file: qwt_abstract_scale_draw.h ***/
#ifndef QWT_ABSTRACT_SCALE_DRAW_H
#define QWT_ABSTRACT_SCALE_DRAW_H

class QwtText;
class QPalette;
class QPainter;
class QFont;
class QwtTransform;
class QwtScaleMap;

/*!
   \brief A abstract base class for drawing scales

   QwtAbstractScaleDraw can be used to draw linear or logarithmic scales.

   After a scale division has been specified as a QwtScaleDiv object
   using setScaleDiv(), the scale can be drawn with the draw() member.
 */
class QWT_EXPORT QwtAbstractScaleDraw
{
public:
	/*!
	   Components of a scale
	   \sa enableComponent(), hasComponent
	 */
	enum ScaleComponent
	{
		//! Backbone = the line where the ticks are located
		Backbone = 0x01,

		//! Ticks
		Ticks = 0x02,

		//! Labels
		Labels = 0x04
	};

    Q_DECLARE_FLAGS(ScaleComponents, ScaleComponent)

	QwtAbstractScaleDraw();
	virtual ~QwtAbstractScaleDraw();

    void setScaleDiv(const QwtScaleDiv&);
	const QwtScaleDiv& scaleDiv() const;

    void setTransformation(QwtTransform*);
	const QwtScaleMap& scaleMap() const;
	QwtScaleMap& scaleMap();

    void enableComponent(ScaleComponent, bool enable = true);
    bool hasComponent(ScaleComponent) const;

    void setTickLength(QwtScaleDiv::TickType, double length);
    double tickLength(QwtScaleDiv::TickType) const;
	double maxTickLength() const;

    void setSpacing(double);
	double spacing() const;

    void setPenWidthF(qreal width);
	qreal penWidthF() const;

    virtual void draw(QPainter*, const QPalette&) const;

    virtual QwtText label(double) const;

	/*!
	   Calculate the extent

	   The extent is the distance from the baseline to the outermost
	   pixel of the scale draw in opposite to its orientation.
	   It is at least minimumExtent() pixels.

	   \param font Font used for drawing the tick labels
	   \return Number of pixels

	   \sa setMinimumExtent(), minimumExtent()
	 */
    virtual double extent(const QFont& font) const = 0;

    void setMinimumExtent(double);
	double minimumExtent() const;

	void invalidateCache();

protected:
	/*!
	   Draw a tick

	   \param painter Painter
	   \param value Value of the tick
	   \param len Length of the tick

	   \sa drawBackbone(), drawLabel()
	 */
    virtual void drawTick(QPainter* painter, double value, double len) const = 0;

	/*!
	   Draws the baseline of the scale
	   \param painter Painter

	   \sa drawTick(), drawLabel()
	 */
    virtual void drawBackbone(QPainter* painter) const = 0;

	/*!
        Draws the label for a major scale tick

        \param painter Painter
        \param value Value

        \sa drawTick(), drawBackbone()
	 */
    virtual void drawLabel(QPainter* painter, double value) const = 0;

    const QwtText& tickLabel(const QFont&, double value) const;

private:
	Q_DISABLE_COPY(QwtAbstractScaleDraw)

	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtAbstractScaleDraw::ScaleComponents)

#endif

/*** End of inlined file: qwt_abstract_scale_draw.h ***/

#include <qpoint.h>

class QTransform;
class QSizeF;
class QRectF;
class QRect;

/*!
   \brief A class for drawing scales

   QwtScaleDraw can be used to draw linear or logarithmic scales.
   A scale has a position, an alignment and a length, which can be specified .
   The labels can be rotated and aligned
   to the ticks using setLabelRotation() and setLabelAlignment().

   After a scale division has been specified as a QwtScaleDiv object
   using QwtAbstractScaleDraw::setScaleDiv(const QwtScaleDiv &s),
   the scale can be drawn with the QwtAbstractScaleDraw::draw() member.
 */
class QWT_EXPORT QwtScaleDraw : public QwtAbstractScaleDraw
{
public:
	/*!
        Alignment of the scale draw
        \sa setAlignment(), alignment()
	 */
	enum Alignment
	{
		//! The scale is below
		BottomScale,

		//! The scale is above
		TopScale,

		//! The scale is left
		LeftScale,

		//! The scale is right
		RightScale
	};

	QwtScaleDraw();
	virtual ~QwtScaleDraw();

    void getBorderDistHint(const QFont&, int& start, int& end) const;
    int minLabelDist(const QFont&) const;

    int minLength(const QFont&) const;
    virtual double extent(const QFont&) const QWT_OVERRIDE;

    void move(double x, double y);
    void move(const QPointF&);
    void setLength(double length);

	Alignment alignment() const;
    void setAlignment(Alignment);

	Qt::Orientation orientation() const;

	QPointF pos() const;
	double length() const;

    void setLabelAlignment(Qt::Alignment);
	Qt::Alignment labelAlignment() const;

    void setLabelRotation(double rotation);
	double labelRotation() const;

    int maxLabelHeight(const QFont&) const;
    int maxLabelWidth(const QFont&) const;

    QPointF labelPosition(double value) const;

    QRectF labelRect(const QFont&, double value) const;
    QSizeF labelSize(const QFont&, double value) const;

    QRect boundingLabelRect(const QFont&, double value) const;

protected:
    QTransform labelTransformation(const QPointF&, const QSizeF&) const;

    virtual void drawTick(QPainter*, double value, double len) const QWT_OVERRIDE;

    virtual void drawBackbone(QPainter*) const QWT_OVERRIDE;
    virtual void drawLabel(QPainter*, double value) const QWT_OVERRIDE;

private:
	void updateMap();

	class PrivateData;
	PrivateData* m_data;
};

/*!
   Move the position of the scale

   \param x X coordinate
   \param y Y coordinate

   \sa move(const QPointF &)
 */
inline void QwtScaleDraw::move(double x, double y)
{
    move(QPointF(x, y));
}

#endif

/*** End of inlined file: qwt_scale_draw.h ***/

/*** Start of inlined file: qwt_scale_engine.h ***/
#ifndef QWT_SCALE_ENGINE_H
#define QWT_SCALE_ENGINE_H

class QwtInterval;
class QwtTransform;

/*!
   \brief Arithmetic including a tolerance
 */
class QWT_EXPORT QwtScaleArithmetic
{
public:
    static double ceilEps(double value, double intervalSize);
    static double floorEps(double value, double intervalSize);

    static double divideEps(double intervalSize, double numSteps);

    static double divideInterval(double intervalSize, int numSteps, uint base);
};

/*!
   \brief Base class for scale engines.

   A scale engine tries to find "reasonable" ranges and step sizes
   for scales.

   The layout of the scale can be varied with setAttribute().

   Qwt offers implementations for logarithmic and linear scales.
 */

class QWT_EXPORT QwtScaleEngine
{
public:
	/*!
	   Layout attributes
	   \sa setAttribute(), testAttribute(), reference(),
           lowerMargin(), upperMargin()
	 */

	enum Attribute
	{
		//! No attributes
		NoAttribute = 0x00,

		//! Build a scale which includes the reference() value.
		IncludeReference = 0x01,

		//! Build a scale which is symmetric to the reference() value.
		Symmetric = 0x02,

		/*!
		   The endpoints of the scale are supposed to be equal the
		   outmost included values plus the specified margins
		   (see setMargins()).
		   If this attribute is *not* set, the endpoints of the scale will
		   be integer multiples of the step size.
		 */
		Floating = 0x04,

		//! Turn the scale upside down.
		Inverted = 0x08
	};

    Q_DECLARE_FLAGS(Attributes, Attribute)

    explicit QwtScaleEngine(uint base = 10);
	virtual ~QwtScaleEngine();

    void setBase(uint base);
	uint base() const;

    void setAttribute(Attribute, bool on = true);
    bool testAttribute(Attribute) const;

    void setAttributes(Attributes);
	Attributes attributes() const;

    void setReference(double);
	double reference() const;

    void setMargins(double lower, double upper);
	double lowerMargin() const;
	double upperMargin() const;

	/*!
	   Align and divide an interval

	   \param maxNumSteps Max. number of steps
	   \param x1 First limit of the interval (In/Out)
	   \param x2 Second limit of the interval (In/Out)
	   \param stepSize Step size (Return value)
	 */
    virtual void autoScale(int maxNumSteps, double& x1, double& x2, double& stepSize) const = 0;

	/*!
	   \brief Calculate a scale division

	   \param x1 First interval limit
	   \param x2 Second interval limit
	   \param maxMajorSteps Maximum for the number of major steps
	   \param maxMinorSteps Maximum number of minor steps
	   \param stepSize Step size. If stepSize == 0.0, the scaleEngine
                   calculates one.

	   \return Calculated scale division
	 */
    virtual QwtScaleDiv divideScale(double x1, double x2, int maxMajorSteps, int maxMinorSteps, double stepSize = 0.0) const = 0;

    void setTransformation(QwtTransform*);
	QwtTransform* transformation() const;

protected:
    bool contains(const QwtInterval&, double value) const;
    QList< double > strip(const QList< double >&, const QwtInterval&) const;

    double divideInterval(double intervalSize, int numSteps) const;

    QwtInterval buildInterval(double value) const;

private:
	Q_DISABLE_COPY(QwtScaleEngine)

	class PrivateData;
	PrivateData* m_data;
};

/*!
   \brief A scale engine for linear scales

   The step size will fit into the pattern
   \f$\left\{ 1,2,5\right\} \cdot 10^{n}\f$, where n is an integer.
 */

class QWT_EXPORT QwtLinearScaleEngine : public QwtScaleEngine
{
public:
    explicit QwtLinearScaleEngine(uint base = 10);
	virtual ~QwtLinearScaleEngine();

    virtual void autoScale(int maxNumSteps, double& x1, double& x2, double& stepSize) const QWT_OVERRIDE;

    virtual QwtScaleDiv divideScale(double x1, double x2, int maxMajorSteps, int maxMinorSteps, double stepSize = 0.0) const QWT_OVERRIDE;

protected:
    QwtInterval align(const QwtInterval&, double stepSize) const;

    void buildTicks(const QwtInterval&, double stepSize, int maxMinorSteps, QList< double > ticks[ QwtScaleDiv::NTickTypes ]) const;

    QList< double > buildMajorTicks(const QwtInterval& interval, double stepSize) const;

    void buildMinorTicks(const QList< double >& majorTicks,
                         int maxMinorSteps,
                         double stepSize,
                         QList< double >& minorTicks,
                         QList< double >& mediumTicks) const;
};

/*!
   \brief A scale engine for logarithmic scales

   The step size is measured in *decades*
   and the major step size will be adjusted to fit the pattern
   \f$\left\{ 1,2,3,5\right\} \cdot 10^{n}\f$, where n is a natural number
   including zero.

   \warning the step size as well as the margins are measured in *decades*.
 */

class QWT_EXPORT QwtLogScaleEngine : public QwtScaleEngine
{
public:
    explicit QwtLogScaleEngine(uint base = 10);
	virtual ~QwtLogScaleEngine();

    virtual void autoScale(int maxNumSteps, double& x1, double& x2, double& stepSize) const QWT_OVERRIDE;

    virtual QwtScaleDiv divideScale(double x1, double x2, int maxMajorSteps, int maxMinorSteps, double stepSize = 0.0) const QWT_OVERRIDE;

protected:
    QwtInterval align(const QwtInterval&, double stepSize) const;

    void buildTicks(const QwtInterval&, double stepSize, int maxMinorSteps, QList< double > ticks[ QwtScaleDiv::NTickTypes ]) const;

    QList< double > buildMajorTicks(const QwtInterval& interval, double stepSize) const;

    void buildMinorTicks(const QList< double >& majorTicks,
                         int maxMinorSteps,
                         double stepSize,
                         QList< double >& minorTicks,
                         QList< double >& mediumTicks) const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtScaleEngine::Attributes)

#endif

/*** End of inlined file: qwt_scale_engine.h ***/

/*** Start of inlined file: qwt_scale_widget.h ***/
#ifndef QWT_SCALE_WIDGET_H
#define QWT_SCALE_WIDGET_H

#include <qwidget.h>
#include <qfont.h>
#include <qcolor.h>
#include <qstring.h>

class QPainter;
class QwtTransform;
class QwtScaleDiv;
class QwtColorMap;

/*!
   \brief A Widget which contains a scale

   This Widget can be used to decorate composite widgets with
   a scale.
 */

class QWT_EXPORT QwtScaleWidget : public QWidget
{
	Q_OBJECT

public:
	//! Layout flags of the title
	enum LayoutFlag
	{
		/*!
		   The title of vertical scales is painted from top to bottom.
		   Otherwise it is painted from bottom to top.
		 */
		TitleInverted = 1
	};

    Q_DECLARE_FLAGS(LayoutFlags, LayoutFlag)

    explicit QwtScaleWidget(QWidget* parent = NULL);
    explicit QwtScaleWidget(QwtScaleDraw::Alignment, QWidget* parent = NULL);
	virtual ~QwtScaleWidget();

Q_SIGNALS:
	//! Signal emitted, whenever the scale division changes
	void scaleDivChanged();

public:
    void setTitle(const QString& title);
    void setTitle(const QwtText& title);
	QwtText title() const;

    void setLayoutFlag(LayoutFlag, bool on);
    bool testLayoutFlag(LayoutFlag) const;

    void setBorderDist(int dist1, int dist2);
	int startBorderDist() const;
	int endBorderDist() const;

    void getBorderDistHint(int& start, int& end) const;

    void getMinBorderDist(int& start, int& end) const;
    void setMinBorderDist(int start, int end);

    void setMargin(int);
	int margin() const;

    void setSpacing(int);
	int spacing() const;

    void setScaleDiv(const QwtScaleDiv&);
    void setTransformation(QwtTransform*);

    void setScaleDraw(QwtScaleDraw*);
	const QwtScaleDraw* scaleDraw() const;
	QwtScaleDraw* scaleDraw();

    void setLabelAlignment(Qt::Alignment);
    void setLabelRotation(double rotation);

    void setColorBarEnabled(bool);
	bool isColorBarEnabled() const;

    void setColorBarWidth(int);
	int colorBarWidth() const;

    void setColorMap(const QwtInterval&, QwtColorMap*);

	QwtInterval colorBarInterval() const;
	const QwtColorMap* colorMap() const;

	virtual QSize sizeHint() const QWT_OVERRIDE;
	virtual QSize minimumSizeHint() const QWT_OVERRIDE;

    int titleHeightForWidth(int width) const;
    int dimForLength(int length, const QFont& scaleFont) const;

    void drawColorBar(QPainter*, const QRectF&) const;
    void drawTitle(QPainter*, QwtScaleDraw::Alignment, const QRectF& rect) const;

    void setAlignment(QwtScaleDraw::Alignment);
	QwtScaleDraw::Alignment alignment() const;

    QRectF colorBarRect(const QRectF&) const;

protected:
    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;
    virtual void resizeEvent(QResizeEvent*) QWT_OVERRIDE;
    virtual void changeEvent(QEvent*) QWT_OVERRIDE;

    void draw(QPainter*) const;

	void scaleChange();
    void layoutScale(bool update_geometry = true);

private:
    void initScale(QwtScaleDraw::Alignment);

	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtScaleWidget::LayoutFlags)

#endif

/*** End of inlined file: qwt_scale_widget.h ***/

/*** Start of inlined file: qwt_abstract_scale.h ***/
#ifndef QWT_ABSTRACT_SCALE_H
#define QWT_ABSTRACT_SCALE_H

#include <qwidget.h>

class QwtScaleEngine;
class QwtAbstractScaleDraw;
class QwtScaleDiv;
class QwtScaleMap;
class QwtInterval;

/*!
   \brief An abstract base class for widgets having a scale

   The scale of an QwtAbstractScale is determined by a QwtScaleDiv
   definition, that contains the boundaries and the ticks of the scale.
   The scale is painted using a QwtScaleDraw object.

   The scale division might be assigned explicitly - but usually
   it is calculated from the boundaries using a QwtScaleEngine.

   The scale engine also decides the type of transformation of the scale
   ( linear, logarithmic ... ).
 */

class QWT_EXPORT QwtAbstractScale : public QWidget
{
	Q_OBJECT

    Q_PROPERTY(double lowerBound READ lowerBound WRITE setLowerBound)
    Q_PROPERTY(double upperBound READ upperBound WRITE setUpperBound)

    Q_PROPERTY(int scaleMaxMajor READ scaleMaxMajor WRITE setScaleMaxMajor)
    Q_PROPERTY(int scaleMaxMinor READ scaleMaxMinor WRITE setScaleMaxMinor)

    Q_PROPERTY(double scaleStepSize READ scaleStepSize WRITE setScaleStepSize)

public:
    explicit QwtAbstractScale(QWidget* parent = NULL);
	virtual ~QwtAbstractScale();

    void setScale(double lowerBound, double upperBound);
    void setScale(const QwtInterval&);
    void setScale(const QwtScaleDiv&);

	const QwtScaleDiv& scaleDiv() const;

    void setLowerBound(double value);
	double lowerBound() const;

    void setUpperBound(double value);
	double upperBound() const;

    void setScaleStepSize(double stepSize);
	double scaleStepSize() const;

    void setScaleMaxMajor(int ticks);
	int scaleMaxMinor() const;

    void setScaleMaxMinor(int ticks);
	int scaleMaxMajor() const;

    void setScaleEngine(QwtScaleEngine*);
	const QwtScaleEngine* scaleEngine() const;
	QwtScaleEngine* scaleEngine();

    int transform(double) const;
    double invTransform(int) const;

	bool isInverted() const;

	double minimum() const;
	double maximum() const;

	const QwtScaleMap& scaleMap() const;

protected:
    virtual void changeEvent(QEvent*) QWT_OVERRIDE;

    void rescale(double lowerBound, double upperBound, double stepSize);

    void setAbstractScaleDraw(QwtAbstractScaleDraw*);

	const QwtAbstractScaleDraw* abstractScaleDraw() const;
	QwtAbstractScaleDraw* abstractScaleDraw();

	void updateScaleDraw();
	virtual void scaleChange();

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_abstract_scale.h ***/

/*** Start of inlined file: qwt_abstract_slider.h ***/
#ifndef QWT_ABSTRACT_SLIDER_H
#define QWT_ABSTRACT_SLIDER_H

/*!
   \brief An abstract base class for slider widgets with a scale

   A slider widget displays a value according to a scale.
   The class is designed as a common super class for widgets like
   QwtKnob, QwtDial and QwtSlider.

   When the slider is nor readOnly() its value can be modified
   by keyboard, mouse and wheel inputs.

   The range of the slider is divided into a number of steps from
   which the value increments according to user inputs depend.
   Only for linear scales the number of steps correspond with
   a fixed step size.
 */

class QWT_EXPORT QwtAbstractSlider : public QwtAbstractScale
{
	Q_OBJECT

    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged USER true)

    Q_PROPERTY(uint totalSteps READ totalSteps WRITE setTotalSteps)
    Q_PROPERTY(uint singleSteps READ singleSteps WRITE setSingleSteps)
    Q_PROPERTY(uint pageSteps READ pageSteps WRITE setPageSteps)
    Q_PROPERTY(bool stepAlignment READ stepAlignment WRITE setStepAlignment)

    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(bool tracking READ isTracking WRITE setTracking)
    Q_PROPERTY(bool wrapping READ wrapping WRITE setWrapping)

    Q_PROPERTY(bool invertedControls READ invertedControls WRITE setInvertedControls)

public:
    explicit QwtAbstractSlider(QWidget* parent = NULL);
	virtual ~QwtAbstractSlider();

    void setValid(bool);
	bool isValid() const;

	double value() const;

    void setWrapping(bool);
	bool wrapping() const;

    void setTotalSteps(uint);
	uint totalSteps() const;

    void setSingleSteps(uint);
	uint singleSteps() const;

    void setPageSteps(uint);
	uint pageSteps() const;

    void setStepAlignment(bool);
	bool stepAlignment() const;

    void setTracking(bool);
	bool isTracking() const;

    void setReadOnly(bool);
	bool isReadOnly() const;

    void setInvertedControls(bool);
	bool invertedControls() const;

public Q_SLOTS:
    void setValue(double value);

Q_SIGNALS:

	/*!
	   \brief Notify a change of value.

	   When tracking is enabled (default setting),
	   this signal will be emitted every time the value changes.

	   \param value New value

	   \sa setTracking(), sliderMoved()
	 */
    void valueChanged(double value);

	/*!
	   This signal is emitted when the user presses the
	   movable part of the slider.
	 */
	void sliderPressed();

	/*!
	   This signal is emitted when the user releases the
	   movable part of the slider.
	 */
	void sliderReleased();

	/*!
	   This signal is emitted when the user moves the
	   slider with the mouse.

	   \param value New value

	   \sa valueChanged()
	 */
    void sliderMoved(double value);

protected:
    virtual void mousePressEvent(QMouseEvent*) QWT_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent*) QWT_OVERRIDE;
    virtual void mouseMoveEvent(QMouseEvent*) QWT_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent*) QWT_OVERRIDE;
    virtual void wheelEvent(QWheelEvent*) QWT_OVERRIDE;

	/*!
	   \brief Determine what to do when the user presses a mouse button.

	   \param pos Mouse position

	   \retval True, when pos is a valid scroll position
	   \sa scrolledTo()
	 */
    virtual bool isScrollPosition(const QPoint& pos) const = 0;

	/*!
	   \brief Determine the value for a new position of the
             movable part of the slider

	   \param pos Mouse position

	   \return Value for the mouse position
	   \sa isScrollPosition()
	 */
    virtual double scrolledTo(const QPoint& pos) const = 0;

    void incrementValue(int stepCount);

	virtual void scaleChange() QWT_OVERRIDE;

protected:
	virtual void sliderChange();

    double incrementedValue(double value, int stepCount) const;

private:
    double alignedValue(double) const;
    double boundedValue(double) const;

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_abstract_slider.h ***/

/*** Start of inlined file: qwt_thermo.h ***/
#ifndef QWT_THERMO_H
#define QWT_THERMO_H

class QwtScaleDraw;
class QwtColorMap;

/*!
   \brief The Thermometer Widget

   QwtThermo is a widget which displays a value in an interval. It supports:
   - a horizontal or vertical layout;
   - a range;
   - a scale;
   - an alarm level.

   \image html sysinfo.png

   The fill colors might be calculated from an optional color map
   If no color map has been assigned QwtThermo uses the
   following colors/brushes from the widget palette:

   - QPalette::Base
    Background of the pipe
   - QPalette::ButtonText
    Fill brush below the alarm level
   - QPalette::Highlight
    Fill brush for the values above the alarm level
   - QPalette::WindowText
    For the axis of the scale
   - QPalette::Text
    For the labels of the scale
 */
class QWT_EXPORT QwtThermo : public QwtAbstractScale
{
	Q_OBJECT

    Q_ENUMS(ScalePosition)
    Q_ENUMS(OriginMode)

    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
    Q_PROPERTY(ScalePosition scalePosition READ scalePosition WRITE setScalePosition)
    Q_PROPERTY(OriginMode originMode READ originMode WRITE setOriginMode)

    Q_PROPERTY(bool alarmEnabled READ alarmEnabled WRITE setAlarmEnabled)
    Q_PROPERTY(double alarmLevel READ alarmLevel WRITE setAlarmLevel)
    Q_PROPERTY(double origin READ origin WRITE setOrigin)
    Q_PROPERTY(int spacing READ spacing WRITE setSpacing)
    Q_PROPERTY(int borderWidth READ borderWidth WRITE setBorderWidth)
    Q_PROPERTY(int pipeWidth READ pipeWidth WRITE setPipeWidth)
    Q_PROPERTY(double value READ value WRITE setValue USER true)

public:
    /*!
	   Position of the scale
	   \sa setScalePosition(), setOrientation()
	 */
	enum ScalePosition
	{
		//! The slider has no scale
		NoScale,

		//! The scale is right of a vertical or below of a horizontal slider
		LeadingScale,

		//! The scale is left of a vertical or above of a horizontal slider
		TrailingScale
	};

	/*!
	   Origin mode. This property specifies where the beginning of the liquid
	   is placed.

	   \sa setOriginMode(), setOrigin()
	 */
	enum OriginMode
	{
		//! The origin is the minimum of the scale
		OriginMinimum,

		//! The origin is the maximum of the scale
		OriginMaximum,

		//! The origin is specified using the origin() property
		OriginCustom
	};

    explicit QwtThermo(QWidget* parent = NULL);
	virtual ~QwtThermo();

    void setOrientation(Qt::Orientation);
	Qt::Orientation orientation() const;

    void setScalePosition(ScalePosition);
	ScalePosition scalePosition() const;

    void setSpacing(int);
	int spacing() const;

    void setBorderWidth(int);
	int borderWidth() const;

    void setOriginMode(OriginMode);
	OriginMode originMode() const;

    void setOrigin(double);
	double origin() const;

    void setFillBrush(const QBrush&);
	QBrush fillBrush() const;

    void setAlarmBrush(const QBrush&);
	QBrush alarmBrush() const;

    void setAlarmLevel(double);
	double alarmLevel() const;

    void setAlarmEnabled(bool);
	bool alarmEnabled() const;

    void setColorMap(QwtColorMap*);
	QwtColorMap* colorMap();
	const QwtColorMap* colorMap() const;

    void setPipeWidth(int);
	int pipeWidth() const;

    void setRangeFlags(QwtInterval::BorderFlags);
	QwtInterval::BorderFlags rangeFlags() const;

	double value() const;

	virtual QSize sizeHint() const QWT_OVERRIDE;
	virtual QSize minimumSizeHint() const QWT_OVERRIDE;

    void setScaleDraw(QwtScaleDraw*);
	const QwtScaleDraw* scaleDraw() const;

public Q_SLOTS:
    virtual void setValue(double);

protected:
    virtual void drawLiquid(QPainter*, const QRect&) const;
	virtual void scaleChange() QWT_OVERRIDE;

    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;
    virtual void resizeEvent(QResizeEvent*) QWT_OVERRIDE;
    virtual void changeEvent(QEvent*) QWT_OVERRIDE;

	QwtScaleDraw* scaleDraw();

	QRect pipeRect() const;
    QRect fillRect(const QRect&) const;
    QRect alarmRect(const QRect&) const;

private:
    void layoutThermo(bool);

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_thermo.h ***/

/*** Start of inlined file: qwt_system_clock.h ***/
#ifndef QWT_SYSTEM_CLOCK_H
#define QWT_SYSTEM_CLOCK_H

#include <qelapsedtimer.h>

/*!
   \brief QwtSystemClock provides high resolution clock time functions.

   Precision and time intervals are multiples of milliseconds (ms).

   ( QwtSystemClock is deprecated as QElapsedTimer offers the same precision )
 */

class QWT_EXPORT QwtSystemClock
{
public:
	bool isNull() const;

	void start();
	double restart();
	double elapsed() const;

private:
	QElapsedTimer m_timer;
};

#endif

/*** End of inlined file: qwt_system_clock.h ***/

/*** Start of inlined file: qwt_symbol.h ***/
#ifndef QWT_SYMBOL_H
#define QWT_SYMBOL_H

#include <qpolygon.h>
#include <qpen.h>
#include <qbrush.h>

class QPainter;
class QSize;
class QBrush;
class QPen;
class QColor;
class QPointF;
class QPainterPath;
class QPixmap;
class QByteArray;
class QwtGraphic;

//! A class for drawing symbols
class QWT_EXPORT QwtSymbol
{
public:
	/*!
	   Symbol Style
	   \sa setStyle(), style()
	 */
	enum Style
	{
		//! No Style. The symbol cannot be drawn.
		NoSymbol = -1,

		//! Ellipse or circle
		Ellipse,

		//! Rectangle
		Rect,

		//!  Diamond
		Diamond,

		//! Triangle pointing upwards
		Triangle,

		//! Triangle pointing downwards
		DTriangle,

		//! Triangle pointing upwards
		UTriangle,

		//! Triangle pointing left
		LTriangle,

		//! Triangle pointing right
		RTriangle,

		//! Cross (+)
		Cross,

		//! Diagonal cross (X)
		XCross,

		//! Horizontal line
		HLine,

		//! Vertical line
		VLine,

		//! X combined with +
		Star1,

		//! Six-pointed star
		Star2,

		//! Hexagon
		Hexagon,

		/*!
		   The symbol is represented by a painter path, where the
		   origin ( 0, 0 ) of the path coordinate system is mapped to
		   the position of the symbol.

		   \sa setPath(), path()
		 */
		Path,

		/*!
		   The symbol is represented by a pixmap. The pixmap is centered
		   or aligned to its pin point.

		   \sa setPinPoint()
		 */
		Pixmap,

		/*!
		   The symbol is represented by a graphic. The graphic is centered
		   or aligned to its pin point.

		   \sa setPinPoint()
		 */
		Graphic,

		/*!
		   The symbol is represented by a SVG graphic. The graphic is centered
		   or aligned to its pin point.

		   \sa setPinPoint()
		 */
		SvgDocument,

		/*!
		   Styles >= QwtSymbol::UserSymbol are reserved for derived
		   classes of QwtSymbol that overload drawSymbols() with
		   additional application specific symbol types.
		 */
		UserStyle = 1000
	};

	/*!
	   Depending on the render engine and the complexity of the
	   symbol shape it might be faster to render the symbol
	   to a pixmap and to paint this pixmap.

	   F.e. the raster paint engine is a pure software renderer
	   where in cache mode a draw operation usually ends in
	   raster operation with the the backing store, that are usually
	   faster, than the algorithms for rendering polygons.
	   But the opposite can be expected for graphic pipelines
	   that can make use of hardware acceleration.

	   The default setting is AutoCache

	   \sa setCachePolicy(), cachePolicy()

	   \note The policy has no effect, when the symbol is painted
            to a vector graphics format ( PDF, SVG ).
	   \warning Since Qt 4.8 raster is the default backend on X11
	 */

	enum CachePolicy
	{
		//! Don't use a pixmap cache
		NoCache,

		//! Always use a pixmap cache
		Cache,

		/*!
		   Use a cache when one of the following conditions is true:

		   - The symbol is rendered with the software
             renderer ( QPaintEngine::Raster )
		 */
		AutoCache
	};

public:
    explicit QwtSymbol(Style = NoSymbol);
    QwtSymbol(Style, const QBrush&, const QPen&, const QSize&);
    QwtSymbol(const QPainterPath&, const QBrush&, const QPen&);

	virtual ~QwtSymbol();

    void setCachePolicy(CachePolicy);
	CachePolicy cachePolicy() const;

    void setSize(const QSize&);
    void setSize(int width, int height = -1);
	const QSize& size() const;

    void setPinPoint(const QPointF& pos, bool enable = true);
	QPointF pinPoint() const;

    void setPinPointEnabled(bool);
	bool isPinPointEnabled() const;

    virtual void setColor(const QColor&);

    void setBrush(const QBrush&);
	const QBrush& brush() const;

    void setPen(const QColor&, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine);
    void setPen(const QPen&);
	const QPen& pen() const;

    void setStyle(Style);
	Style style() const;

    void setPath(const QPainterPath&);
	const QPainterPath& path() const;

    void setPixmap(const QPixmap&);
	const QPixmap& pixmap() const;

    void setGraphic(const QwtGraphic&);
	const QwtGraphic& graphic() const;

#ifndef QWT_NO_SVG
    void setSvgDocument(const QByteArray&);
#endif

    void drawSymbol(QPainter*, const QRectF&) const;
    void drawSymbol(QPainter*, const QPointF&) const;
    void drawSymbols(QPainter*, const QPolygonF&) const;
    void drawSymbols(QPainter*, const QPointF*, int numPoints) const;

	virtual QRect boundingRect() const;
	void invalidateCache();

protected:
    virtual void renderSymbols(QPainter*, const QPointF*, int numPoints) const;

private:
	Q_DISABLE_COPY(QwtSymbol)

	class PrivateData;
	PrivateData* m_data;
};

/*!
   \brief Draw the symbol at a specified position

   \param painter Painter
   \param pos Position of the symbol in screen coordinates
 */
inline void QwtSymbol::drawSymbol(QPainter* painter, const QPointF& pos) const
{
    drawSymbols(painter, &pos, 1);
}

/*!
   \brief Draw symbols at the specified points

   \param painter Painter
   \param points Positions of the symbols in screen coordinates
 */

inline void QwtSymbol::drawSymbols(QPainter* painter, const QPolygonF& points) const
{
    drawSymbols(painter, points.data(), points.size());
}

#endif

/*** End of inlined file: qwt_symbol.h ***/

/*** Start of inlined file: qwt_interval_symbol.h ***/
#ifndef QWT_INTERVAL_SYMBOL_H
#define QWT_INTERVAL_SYMBOL_H

#include <qnamespace.h>

class QPainter;
class QPen;
class QBrush;
class QPointF;
class QColor;

/*!
   \brief A drawing primitive for displaying an interval like an error bar

   \sa QwtPlotIntervalCurve
 */
class QWT_EXPORT QwtIntervalSymbol
{
public:
	//! Symbol style
	enum Style
	{
		//! No Style. The symbol cannot be drawn.
		NoSymbol = -1,

		/*!
		   The symbol displays a line with caps at the beginning/end.
		   The size of the caps depends on the symbol width().
		 */
		Bar,

		/*!
		   The symbol displays a plain rectangle using pen() and brush().
		   The size of the rectangle depends on the translated interval and
		   the width(),
		 */
		Box,

		/*!
		   Styles >= UserSymbol are reserved for derived
		   classes of QwtIntervalSymbol that overload draw() with
		   additional application specific symbol types.
		 */
		UserSymbol = 1000
	};

public:
    explicit QwtIntervalSymbol(Style = NoSymbol);
    QwtIntervalSymbol(const QwtIntervalSymbol&);

	virtual ~QwtIntervalSymbol();

    QwtIntervalSymbol& operator=(const QwtIntervalSymbol&);
    bool operator==(const QwtIntervalSymbol&) const;
    bool operator!=(const QwtIntervalSymbol&) const;

    void setWidth(int);
	int width() const;

    void setBrush(const QBrush&);
	const QBrush& brush() const;

    void setPen(const QColor&, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine);
    void setPen(const QPen&);
	const QPen& pen() const;

    void setStyle(Style);
	Style style() const;

    virtual void draw(QPainter*, Qt::Orientation, const QPointF& from, const QPointF& to) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_interval_symbol.h ***/

/*** Start of inlined file: qwt_column_symbol.h ***/
#ifndef QWT_COLUMN_SYMBOL_H
#define QWT_COLUMN_SYMBOL_H

#include <qnamespace.h>

class QPainter;
class QPalette;
class QRectF;

/*!
    \brief Directed rectangle representing bounding rectangle and orientation
    of a column.
 */
class QWT_EXPORT QwtColumnRect
{
public:
	//! Direction of the column
	enum Direction
	{
		//! From left to right
		LeftToRight,

		//! From right to left
		RightToLeft,

		//! From bottom to top
		BottomToTop,

		//! From top to bottom
		TopToBottom
	};

	//! Build an rectangle with invalid intervals directed BottomToTop.
	QwtColumnRect() : direction(BottomToTop)
	{
	}

	//! \return A normalized QRect built from the intervals
	QRectF toRect() const;

	//! \return Orientation
	Qt::Orientation orientation() const
	{
		if (direction == LeftToRight || direction == RightToLeft)
			return Qt::Horizontal;

		return Qt::Vertical;
	}

	//! Interval for the horizontal coordinates
	QwtInterval hInterval;

	//! Interval for the vertical coordinates
	QwtInterval vInterval;

	//! Direction
	Direction direction;
};

//! A drawing primitive for columns
class QWT_EXPORT QwtColumnSymbol
{
public:
	/*!
	   Style
	   \sa setStyle(), style()
	 */
	enum Style
	{
		//! No Style, the symbol draws nothing
		NoStyle = -1,

		/*!
		   The column is painted with a frame depending on the frameStyle()
		   and lineWidth() using the palette().
		 */
		Box,

		/*!
		   Styles >= QwtColumnSymbol::UserStyle are reserved for derived
		   classes of QwtColumnSymbol that overload draw() with
		   additional application specific symbol types.
		 */
		UserStyle = 1000
	};

	/*!
	   Frame Style used in Box style().
	   \sa Style, setFrameStyle(), frameStyle(), setStyle(), setPalette()
	 */
	enum FrameStyle
	{
		//! No frame
		NoFrame,

		//! A plain frame style
		Plain,

		//! A raised frame style
		Raised
	};

public:
	explicit QwtColumnSymbol(Style = NoStyle);
	virtual ~QwtColumnSymbol();

	void setFrameStyle(FrameStyle);
	FrameStyle frameStyle() const;

	void setLineWidth(int width);
	int lineWidth() const;

	void setStyle(Style);
	Style style() const;

	void setPen(const QPen& pen);
	QPen pen() const;

	void setBrush(const QBrush& b);
	QBrush brush() const;

	virtual void draw(QPainter*, const QwtColumnRect&) const;

protected:
	void drawBox(QPainter*, const QwtColumnRect&) const;

private:
	Q_DISABLE_COPY(QwtColumnSymbol)

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_column_symbol.h ***/

/*** Start of inlined file: qwt_wheel.h ***/
#ifndef QWT_WHEEL_H
#define QWT_WHEEL_H

#include <qwidget.h>

/*!
   \brief The Wheel Widget

   The wheel widget can be used to change values over a very large range
   in very small steps. Using the setMass() member, it can be configured
   as a flying wheel.

   The default range of the wheel is [0.0, 100.0]

   \sa The radio example.
 */
class QWT_EXPORT QwtWheel : public QWidget
{
	Q_OBJECT

    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)

    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged USER true)

    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)

    Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(int pageStepCount READ pageStepCount WRITE setPageStepCount)
    Q_PROPERTY(bool stepAlignment READ stepAlignment WRITE setStepAlignment)

    Q_PROPERTY(bool tracking READ isTracking WRITE setTracking)
    Q_PROPERTY(bool wrapping READ wrapping WRITE setWrapping)
    Q_PROPERTY(bool inverted READ isInverted WRITE setInverted)

    Q_PROPERTY(double mass READ mass WRITE setMass)
    Q_PROPERTY(int updateInterval READ updateInterval WRITE setUpdateInterval)

    Q_PROPERTY(double totalAngle READ totalAngle WRITE setTotalAngle)
    Q_PROPERTY(double viewAngle READ viewAngle WRITE setViewAngle)
    Q_PROPERTY(int tickCount READ tickCount WRITE setTickCount)
    Q_PROPERTY(int wheelWidth READ wheelWidth WRITE setWheelWidth)
    Q_PROPERTY(int borderWidth READ borderWidth WRITE setBorderWidth)
    Q_PROPERTY(int wheelBorderWidth READ wheelBorderWidth WRITE setWheelBorderWidth)

public:
    explicit QwtWheel(QWidget* parent = NULL);
	virtual ~QwtWheel();

	double value() const;

    void setOrientation(Qt::Orientation);
	Qt::Orientation orientation() const;

	double totalAngle() const;
	double viewAngle() const;

    void setTickCount(int);
	int tickCount() const;

    void setWheelWidth(int);
	int wheelWidth() const;

    void setWheelBorderWidth(int);
	int wheelBorderWidth() const;

    void setBorderWidth(int);
	int borderWidth() const;

    void setInverted(bool);
	bool isInverted() const;

    void setWrapping(bool);
	bool wrapping() const;

    void setSingleStep(double);
	double singleStep() const;

    void setPageStepCount(int);
	int pageStepCount() const;

    void setStepAlignment(bool on);
	bool stepAlignment() const;

    void setRange(double min, double max);

    void setMinimum(double);
	double minimum() const;

    void setMaximum(double);
	double maximum() const;

    void setUpdateInterval(int);
	int updateInterval() const;

    void setTracking(bool);
	bool isTracking() const;

	double mass() const;

public Q_SLOTS:
    void setValue(double);
    void setTotalAngle(double);
    void setViewAngle(double);
    void setMass(double);

Q_SIGNALS:

	/*!
	   \brief Notify a change of value.

	   When tracking is enabled this signal will be emitted every
	   time the value changes.

	   \param value new value
	   \sa setTracking()
	 */
    void valueChanged(double value);

	/*!
	   This signal is emitted when the user presses the
	   the wheel with the mouse
	 */
	void wheelPressed();

	/*!
	   This signal is emitted when the user releases the mouse
	 */
	void wheelReleased();

	/*!
	   This signal is emitted when the user moves the
	   wheel with the mouse.

	   \param value new value
	 */
    void wheelMoved(double value);

protected:
    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent*) QWT_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent*) QWT_OVERRIDE;
    virtual void mouseMoveEvent(QMouseEvent*) QWT_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent*) QWT_OVERRIDE;
    virtual void wheelEvent(QWheelEvent*) QWT_OVERRIDE;
    virtual void timerEvent(QTimerEvent*) QWT_OVERRIDE;

	void stopFlying();

	QRect wheelRect() const;

	virtual QSize sizeHint() const QWT_OVERRIDE;
	virtual QSize minimumSizeHint() const QWT_OVERRIDE;

    virtual void drawTicks(QPainter*, const QRectF&);
    virtual void drawWheelBackground(QPainter*, const QRectF&);

    virtual double valueAt(const QPoint&) const;

private:
    double alignedValue(double) const;
    double boundedValue(double) const;

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_wheel.h ***/

/*** Start of inlined file: qwt_vectorfield_symbol.h ***/
#ifndef QWT_VECTOR_FIELD_SYMBOL_H
#define QWT_VECTOR_FIELD_SYMBOL_H

class QPainter;
class QPainterPath;

/*!
    Defines abstract interface for arrow drawing routines.

    Arrow needs to be drawn horizontally with arrow tip at coordinate 0,0.
    arrowLength() shall return the entire length of the arrow (needed
    to translate the arrow for tail/centered alignment).
    setArrowLength() defines arror length in pixels (screen coordinates). It
    can be implemented to adjust other geometric properties such as
    the head size and width of the arrow. It is _always_ called before
    paint().

    A new arrow implementation can be set with QwtPlotVectorField::setArrowSymbol(), whereby
    ownership is transferred to the plot field.
 */
class QWT_EXPORT QwtVectorFieldSymbol
{
public:
	QwtVectorFieldSymbol();
	virtual ~QwtVectorFieldSymbol();

	/*!
        Set the length of the symbol/arrow
        \sa length()
	 */
    virtual void setLength(qreal length) = 0;

	/*!
        \return length of the symbol/arrow
        \sa setLength()
	 */
	virtual qreal length() const = 0;

	//! Draw the symbol/arrow
    virtual void paint(QPainter*) const = 0;

private:
	Q_DISABLE_COPY(QwtVectorFieldSymbol)
};

/*!
    Arrow implementation that draws a filled arrow with outline, using
    a triangular head of constant width.
 */
class QWT_EXPORT QwtVectorFieldArrow : public QwtVectorFieldSymbol
{
public:
    QwtVectorFieldArrow(qreal headWidth = 6.0, qreal tailWidth = 1.0);
	virtual ~QwtVectorFieldArrow() QWT_OVERRIDE;

    virtual void setLength(qreal length) QWT_OVERRIDE;
	virtual qreal length() const QWT_OVERRIDE;

    virtual void paint(QPainter*) const QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

/*!
    Arrow implementation that only used lines, with optionally a filled arrow or only
    lines.
 */
class QWT_EXPORT QwtVectorFieldThinArrow : public QwtVectorFieldSymbol
{
public:
    QwtVectorFieldThinArrow(qreal headWidth = 6.0);
	virtual ~QwtVectorFieldThinArrow() QWT_OVERRIDE;

    virtual void setLength(qreal length) QWT_OVERRIDE;
	virtual qreal length() const QWT_OVERRIDE;

    virtual void paint(QPainter*) const QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_vectorfield_symbol.h ***/

/*** Start of inlined file: qwt_spline_polynomial.h ***/
#ifndef QWT_SPLINE_POLYNOMIAL_H
#define QWT_SPLINE_POLYNOMIAL_H

#include <qpoint.h>
#include <qmetatype.h>

/*!
   \brief A cubic polynomial without constant term

   QwtSplinePolynomial is a 3rd degree polynomial
   of the form: y = c3 * x³ + c2 * x² + c1 * x;

   QwtSplinePolynomial is usually used in combination with polygon
   interpolation, where it is not necessary to store a constant term ( c0 ),
   as the translation is known from the corresponding polygon points.

   \sa QwtSplineC1
 */
class QWT_EXPORT QwtSplinePolynomial
{
public:
    QwtSplinePolynomial(double c3 = 0.0, double c2 = 0.0, double c1 = 0.0);

    bool operator==(const QwtSplinePolynomial&) const;
    bool operator!=(const QwtSplinePolynomial&) const;

    double valueAt(double x) const;
    double slopeAt(double x) const;
    double curvatureAt(double x) const;

    static QwtSplinePolynomial fromSlopes(const QPointF& p1, double m1, const QPointF& p2, double m2);

    static QwtSplinePolynomial fromSlopes(double x, double y, double m1, double m2);

    static QwtSplinePolynomial fromCurvatures(const QPointF& p1, double cv1, const QPointF& p2, double cv2);

    static QwtSplinePolynomial fromCurvatures(double dx, double dy, double cv1, double cv2);

public:
	//! coefficient of the cubic summand
	double c3;

	//! coefficient of the quadratic summand
	double c2;

	//! coefficient of the linear summand
	double c1;
};

Q_DECLARE_TYPEINFO(QwtSplinePolynomial, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(QwtSplinePolynomial)

/*!
   \brief Constructor

   \param a3 Coefficient of the cubic summand
   \param a2 Coefficient of the quadratic summand
   \param a1 Coefficient of the linear summand
 */
inline QwtSplinePolynomial::QwtSplinePolynomial(double a3, double a2, double a1) : c3(a3), c2(a2), c1(a1)
{
}

/*!
   \param other Other polynomial
   \return true, when both polynomials have the same coefficients
 */
inline bool QwtSplinePolynomial::operator==(const QwtSplinePolynomial& other) const
{
    return (c3 == other.c3) && (c2 == other.c2) && (c1 == other.c1);
}

/*!
   \param other Other polynomial
   \return true, when the polynomials have different coefficients
 */
inline bool QwtSplinePolynomial::operator!=(const QwtSplinePolynomial& other) const
{
    return (!(*this == other));
}

/*!
   Calculate the value of a polynomial for a given x

   \param x Parameter
   \return Value at x
 */
inline double QwtSplinePolynomial::valueAt(double x) const
{
    return (((c3 * x) + c2) * x + c1) * x;
}

/*!
   Calculate the value of the first derivate of a polynomial for a given x

   \param x Parameter
   \return Slope at x
 */
inline double QwtSplinePolynomial::slopeAt(double x) const
{
    return (3.0 * c3 * x + 2.0 * c2) * x + c1;
}

/*!
   Calculate the value of the second derivate of a polynomial for a given x

   \param x Parameter
   \return Curvature at x
 */
inline double QwtSplinePolynomial::curvatureAt(double x) const
{
	return 6.0 * c3 * x + 2.0 * c2;
}

/*!
   Find the coefficients for the polynomial including 2 points with
   specific values for the 1st derivates at these points.

   \param p1 First point
   \param m1 Value of the first derivate at p1
   \param p2 Second point
   \param m2 Value of the first derivate at p2

   \return Coefficients of the polynomials
   \note The missing constant term of the polynomial is p1.y()
 */
inline QwtSplinePolynomial QwtSplinePolynomial::fromSlopes(const QPointF& p1, double m1, const QPointF& p2, double m2)
{
    return fromSlopes(p2.x() - p1.x(), p2.y() - p1.y(), m1, m2);
}

/*!
   Find the coefficients for the polynomial from the offset between 2 points
   and specific values for the 1st derivates at these points.

   \param dx X-offset
   \param dy Y-offset
   \param m1 Value of the first derivate at p1
   \param m2 Value of the first derivate at p2

   \return Coefficients of the polynomials
 */
inline QwtSplinePolynomial QwtSplinePolynomial::fromSlopes(double dx, double dy, double m1, double m2)
{
    const double c2 = (3.0 * dy / dx - 2 * m1 - m2) / dx;
    const double c3 = ((m2 - m1) / dx - 2.0 * c2) / (3.0 * dx);

    return QwtSplinePolynomial(c3, c2, m1);
}

/*!
   Find the coefficients for the polynomial including 2 points with
   specific values for the 2nd derivates at these points.

   \param p1 First point
   \param cv1 Value of the second derivate at p1
   \param p2 Second point
   \param cv2 Value of the second derivate at p2

   \return Coefficients of the polynomials
   \note The missing constant term of the polynomial is p1.y()
 */
inline QwtSplinePolynomial QwtSplinePolynomial::fromCurvatures(const QPointF& p1, double cv1, const QPointF& p2, double cv2)
{
    return fromCurvatures(p2.x() - p1.x(), p2.y() - p1.y(), cv1, cv2);
}

/*!
   Find the coefficients for the polynomial from the offset between 2 points
   and specific values for the 2nd derivates at these points.

   \param dx X-offset
   \param dy Y-offset
   \param cv1 Value of the second derivate at p1
   \param cv2 Value of the second derivate at p2

   \return Coefficients of the polynomials
 */
inline QwtSplinePolynomial QwtSplinePolynomial::fromCurvatures(double dx, double dy, double cv1, double cv2)
{
    const double c3 = (cv2 - cv1) / (6.0 * dx);
	const double c2 = 0.5 * cv1;
    const double c1 = dy / dx - (c3 * dx + c2) * dx;

    return QwtSplinePolynomial(c3, c2, c1);
}

#ifndef QT_NO_DEBUG_STREAM

class QDebug;
QWT_EXPORT QDebug operator<<(QDebug, const QwtSplinePolynomial&);

#endif

#endif

/*** End of inlined file: qwt_spline_polynomial.h ***/

/*** Start of inlined file: qwt_spline_parametrization.h ***/
#ifndef QWT_SPLINE_PARAMETRIZATION_H
#define QWT_SPLINE_PARAMETRIZATION_H

#include <qpoint.h>

/*!
   \brief Curve parametrization used for a spline interpolation

   Parametrization is the process of finding a parameter value for
   each curve point - usually related to some physical quantity
   ( distance, time ... ).

   Often accumulating the curve length is the intended way of parametrization,
   but as the interpolated curve is not known in advance an approximation
   needs to be used.

   The values are calculated by cumulating increments, that are provided
   by QwtSplineParametrization. As the curve parameters need to be
   montonically increasing, each increment need to be positive.

   - t[0] = 0;
   - t[i] = t[i-1] + valueIncrement( point[i-1], p[i] );

   QwtSplineParametrization provides the most common used type of
   parametrizations and offers an interface to inject custom implementations.

   \note The most relevant types of parametrization are trying to provide an
        approximation of the curve length.

   \sa QwtSpline::setParametrization()
 */
class QWT_EXPORT QwtSplineParametrization
{
public:
	//! Parametrization type
	enum Type
	{
		/*!
		   No parametrization: t[i] = x[i]
		   \sa valueIncrementX()
		 */
		ParameterX,

		/*!
		   No parametrization: t[i] = y[i]
		   \sa valueIncrementY()
		 */
		ParameterY,

		/*!
		   Uniform parametrization: t[i] = i;

		   A very fast parametrization, with good results, when the geometry
		   of the control points is somehow "equidistant". F.e. when
		   recording the position of a body, that is moving with constant
		   speed every n seconds.

		   \sa valueIncrementUniform()
		 */
		ParameterUniform,

		/*!
		   Parametrization using the chordal length between two control points

		   The chordal length is the most commonly used approximation for
		   the curve length.

		   \sa valueIncrementChordal()
		 */
		ParameterChordal,

		/*!
		   Centripetal parametrization

		   Based on the square root of the chordal length.

		   Its name stems from the physical observations regarding
		   the centripetal force, of a body moving along the curve.

		   \sa valueIncrementCentripetal()
		 */
		ParameterCentripetal,

		/*!
		   Parametrization using the manhattan length between two control points

		   Approximating the curve length by the manhattan length is faster
		   than the chordal length, but usually gives worse results.

		   \sa valueIncrementManhattan()
		 */
		ParameterManhattan
	};

    explicit QwtSplineParametrization(int type);
	virtual ~QwtSplineParametrization();

	int type() const;

    virtual double valueIncrement(const QPointF&, const QPointF&) const;

    static double valueIncrementX(const QPointF&, const QPointF&);
    static double valueIncrementY(const QPointF&, const QPointF&);
    static double valueIncrementUniform(const QPointF&, const QPointF&);
    static double valueIncrementChordal(const QPointF&, const QPointF&);
    static double valueIncrementCentripetal(const QPointF&, const QPointF&);
    static double valueIncrementManhattan(const QPointF&, const QPointF&);

private:
	const int m_type;
};

/*!
   \brief Calculate the ParameterX value increment for 2 points

   \param point1 First point
   \param point2 Second point

   \return point2.x() - point1.x();
 */
inline double QwtSplineParametrization::valueIncrementX(const QPointF& point1, const QPointF& point2)
{
	return point2.x() - point1.x();
}

/*!
   \brief Calculate the ParameterY value increment for 2 points

   \param point1 First point
   \param point2 Second point

   \return point2.y() - point1.y();
 */
inline double QwtSplineParametrization::valueIncrementY(const QPointF& point1, const QPointF& point2)
{
	return point2.y() - point1.y();
}

/*!
   \brief Calculate the ParameterUniform value increment

   \param point1 First point
   \param point2 Second point

   \return 1.0
 */
inline double QwtSplineParametrization::valueIncrementUniform(const QPointF& point1, const QPointF& point2)
{
    Q_UNUSED(point1)
    Q_UNUSED(point2)

	return 1.0;
}

/*!
   \brief Calculate the ParameterChordal value increment for 2 points

   \param point1 First point
   \param point2 Second point

   \return qSqrt( dx * dx + dy * dy );
 */
inline double QwtSplineParametrization::valueIncrementChordal(const QPointF& point1, const QPointF& point2)
{
	const double dx = point2.x() - point1.x();
	const double dy = point2.y() - point1.y();

    return std::sqrt(dx * dx + dy * dy);
}

/*!
   \brief Calculate the ParameterCentripetal value increment for 2 points

   \param point1 First point
   \param point2 Second point

   \return The square root of a chordal increment
 */
inline double QwtSplineParametrization::valueIncrementCentripetal(const QPointF& point1, const QPointF& point2)
{
    return std::sqrt(valueIncrementChordal(point1, point2));
}

/*!
   \brief Calculate the ParameterManhattan value increment for 2 points

   \param point1 First point
   \param point2 Second point

   \return | point2.x() - point1.x() | + | point2.y() - point1.y() |
 */
inline double QwtSplineParametrization::valueIncrementManhattan(const QPointF& point1, const QPointF& point2)
{
    return qAbs(point2.x() - point1.x()) + qAbs(point2.y() - point1.y());
}

#endif

/*** End of inlined file: qwt_spline_parametrization.h ***/

/*** Start of inlined file: qwt_spline_pleasing.h ***/
#ifndef QWT_SPLINE_PLEASING_H
#define QWT_SPLINE_PLEASING_H

/*** Start of inlined file: qwt_spline.h ***/
#ifndef QWT_SPLINE_H
#define QWT_SPLINE_H

class QwtSplineParametrization;
class QwtSplinePolynomial;
class QPainterPath;
class QLineF;
class QPolygonF;

#if QT_VERSION < 0x060000
template< typename T >
class QVector;
#endif

/*!
   \brief Base class for all splines

   A spline is a curve represented by a sequence of polynomials. Spline approximation
   is the process of finding polynomials for a given set of points.
   When the algorithm preserves the initial points it is called interpolating.

   Splines can be classified according to conditions of the polynomials that
   are met at the start/endpoints of the pieces:

   - Geometric Continuity

    - G0: polynomials are joined
    - G1: first derivatives are proportional at the join point
          The curve tangents thus have the same direction, but not necessarily the
          same magnitude. i.e., C1'(1) = (a,b,c) and C2'(0) = (k*a, k*b, k*c).
    - G2: first and second derivatives are proportional at join point

   - Parametric Continuity

    - C0: curves are joined
    - C1: first derivatives equal
    - C2: first and second derivatives are equal

   Geometric continuity requires the geometry to be continuous, while parametric
   continuity requires that the underlying parameterization be continuous as well.
   Parametric continuity of order n implies geometric continuity of order n,
   but not vice-versa.

   QwtSpline is the base class for spline approximations of any continuity.
 */
class QWT_EXPORT QwtSpline
{
public:
	/*!
	   Boundary type specifying the spline at its endpoints

	   \sa setBoundaryType(), boundaryType()
	 */
	enum BoundaryType
	{
		/*!
		   The polynomials at the start/endpoint depend on specific conditions

		   \sa QwtSpline::BoundaryCondition
		 */
		ConditionalBoundaries,

		/*!
		   The polynomials at the start/endpoint are found by using
		   imaginary additional points. Additional points at the end
		   are found by translating points from the beginning or v.v.
		 */
		PeriodicPolygon,

		/*!
		   ClosedPolygon is similar to PeriodicPolygon beside, that
		   the interpolation includes the connection between the last
		   and the first control point.

		   \note Only works for parametrizations, where the parameter increment
                for the the final closing line is positive.
                This excludes QwtSplineParametrization::ParameterX and
                QwtSplineParametrization::ParameterY
		 */

		ClosedPolygon
	};

	/*!
	   position of a boundary condition
	   \sa boundaryCondition(), boundaryValue()
	 */
	enum BoundaryPosition
	{
		//! the condition is at the beginning of the polynomial
		AtBeginning,

		//! the condition is at the end of the polynomial
		AtEnd
	};

	/*!
	   \brief Boundary condition

	   A spline algorithm calculates polynomials by looking
	   a couple of points back/ahead ( locality() ). At the ends
	   additional rules are necessary to compensate the missing
	   points.

	   \sa boundaryCondition(), boundaryValue()
	   \sa QwtSplineC2::BoundaryConditionC2
	 */
	enum BoundaryCondition
	{
		/*!
		   The first derivative at the end point is given
		   \sa boundaryValue()
		 */
		Clamped1,

		/*!
		   The second derivative at the end point is given

		   \sa boundaryValue()
		   \note a condition having a second derivative of 0
                is also called "natural".
		 */
		Clamped2,

		/*!
		   The third derivative at the end point is given

		   \sa boundaryValue()
		   \note a condition having a third derivative of 0
                is also called "parabolic runout".
		 */
		Clamped3,

		/*!
		   The first derivate at the endpoint is related to the first derivative
		   at its neighbour by the boundary value. F,e when the boundary
		   value at the end is 1.0 then the slope at the last 2 points is
		   the same.

		   \sa boundaryValue().
		 */
		LinearRunout
	};

	QwtSpline();
	virtual ~QwtSpline();

    void setParametrization(int type);
    void setParametrization(QwtSplineParametrization*);
	const QwtSplineParametrization* parametrization() const;

    void setBoundaryType(BoundaryType);
	BoundaryType boundaryType() const;

    void setBoundaryValue(BoundaryPosition, double value);
    double boundaryValue(BoundaryPosition) const;

    void setBoundaryCondition(BoundaryPosition, int condition);
    int boundaryCondition(BoundaryPosition) const;

    void setBoundaryConditions(int condition, double valueBegin = 0.0, double valueEnd = 0.0);

    virtual QPolygonF polygon(const QPolygonF&, double tolerance) const;
    virtual QPainterPath painterPath(const QPolygonF&) const = 0;

	virtual uint locality() const;

private:
	Q_DISABLE_COPY(QwtSpline)

	class PrivateData;
	PrivateData* m_data;
};

/*!
   \brief Base class for a spline interpolation

   Spline interpolation is the process of interpolating a set of points
   piecewise with polynomials. The initial set of points is preserved.
 */
class QWT_EXPORT QwtSplineInterpolating : public QwtSpline
{
public:
	QwtSplineInterpolating();
	virtual ~QwtSplineInterpolating();

    virtual QPolygonF equidistantPolygon(const QPolygonF&, double distance, bool withNodes) const;

    virtual QPolygonF polygon(const QPolygonF&, double tolerance) const QWT_OVERRIDE;

    virtual QPainterPath painterPath(const QPolygonF&) const QWT_OVERRIDE;
    virtual QVector< QLineF > bezierControlLines(const QPolygonF&) const = 0;

private:
	Q_DISABLE_COPY(QwtSplineInterpolating)
};

/*!
   \brief Base class for spline interpolations providing a
         first order geometric continuity ( G1 ) between adjoining curves
 */
class QWT_EXPORT QwtSplineG1 : public QwtSplineInterpolating
{
public:
	QwtSplineG1();
	virtual ~QwtSplineG1();
};

/*!
   \brief Base class for spline interpolations providing a
         first order parametric continuity ( C1 ) between adjoining curves

   All interpolations with C1 continuity are based on rules for finding
   the 1. derivate at some control points.

   In case of non parametric splines those points are the curve points, while
   for parametric splines the calculation is done twice using a parameter value t.

   \sa QwtSplineParametrization
 */
class QWT_EXPORT QwtSplineC1 : public QwtSplineG1
{
public:
	QwtSplineC1();
	virtual ~QwtSplineC1();

    virtual QPainterPath painterPath(const QPolygonF&) const QWT_OVERRIDE;
    virtual QVector< QLineF > bezierControlLines(const QPolygonF&) const QWT_OVERRIDE;

    virtual QPolygonF equidistantPolygon(const QPolygonF&, double distance, bool withNodes) const QWT_OVERRIDE;

	// these methods are the non parametric part
    virtual QVector< QwtSplinePolynomial > polynomials(const QPolygonF&) const;
    virtual QVector< double > slopes(const QPolygonF&) const = 0;

    virtual double slopeAtBeginning(const QPolygonF&, double slopeNext) const;
    virtual double slopeAtEnd(const QPolygonF&, double slopeBefore) const;
};

/*!
   \brief Base class for spline interpolations providing a
         second order parametric continuity ( C2 ) between adjoining curves

   All interpolations with C2 continuity are based on rules for finding
   the 2. derivate at some control points.

   In case of non parametric splines those points are the curve points, while
   for parametric splines the calculation is done twice using a parameter value t.

   \sa QwtSplineParametrization
 */
class QWT_EXPORT QwtSplineC2 : public QwtSplineC1
{
public:
	/*!
	   Boundary condition that requires C2 continuity

	   \sa QwtSpline::boundaryCondition, QwtSpline::BoundaryCondition
	 */
	enum BoundaryConditionC2
	{
		/*!
		   The second derivate at the endpoint is related to the second derivatives
		   at the 2 neighbours: cv[0] := 2.0 * cv[1] - cv[2].

		   \note boundaryValue() is ignored
		 */
		CubicRunout = LinearRunout + 1,

		/*!
		   The 3rd derivate at the endpoint matches the 3rd derivate at its neighbours.
		   Or in other words: the first/last curve segment extents the polynomial of its
		   neighboured polynomial

		   \note boundaryValue() is ignored
		 */
		NotAKnot
	};

	QwtSplineC2();
	virtual ~QwtSplineC2();

    virtual QPainterPath painterPath(const QPolygonF&) const QWT_OVERRIDE;
    virtual QVector< QLineF > bezierControlLines(const QPolygonF&) const QWT_OVERRIDE;

    virtual QPolygonF equidistantPolygon(const QPolygonF&, double distance, bool withNodes) const QWT_OVERRIDE;

	// calculating the parametric equations
    virtual QVector< QwtSplinePolynomial > polynomials(const QPolygonF&) const QWT_OVERRIDE;
    virtual QVector< double > slopes(const QPolygonF&) const QWT_OVERRIDE;
    virtual QVector< double > curvatures(const QPolygonF&) const = 0;
};

#endif

/*** End of inlined file: qwt_spline.h ***/

/*!
   \brief A spline with G1 continuity

   QwtSplinePleasing is some sort of cardinal spline, with
   non C1 continuous extra rules for narrow angles. It has a locality of 2.

   \note The algorithm is the one offered by a popular office package.
 */
class QWT_EXPORT QwtSplinePleasing : public QwtSplineG1
{
public:
	QwtSplinePleasing();
	virtual ~QwtSplinePleasing();

	virtual uint locality() const QWT_OVERRIDE;

    virtual QPainterPath painterPath(const QPolygonF&) const QWT_OVERRIDE;
    virtual QVector< QLineF > bezierControlLines(const QPolygonF&) const QWT_OVERRIDE;
};

#endif

/*** End of inlined file: qwt_spline_pleasing.h ***/

/*** Start of inlined file: qwt_spline_basis.h ***/
#ifndef QWT_SPLINE_BASIS_H
#define QWT_SPLINE_BASIS_H

/*!
   \brief An approximation using a basis spline

   QwtSplineBasis approximates a set of points by a polynomials with C2 continuity
   ( = first and second derivatives are equal ) at the end points.

   The end points of the spline do not match the original points.
 */
class QWT_EXPORT QwtSplineBasis : public QwtSpline
{
public:
	QwtSplineBasis();
	virtual ~QwtSplineBasis();

    virtual QPainterPath painterPath(const QPolygonF&) const QWT_OVERRIDE;
	virtual uint locality() const QWT_OVERRIDE;
};

#endif

/*** End of inlined file: qwt_spline_basis.h ***/

/*** Start of inlined file: qwt_spline_cubic.h ***/
#ifndef QWT_SPLINE_CUBIC_H
#define QWT_SPLINE_CUBIC_H

/*!
   \brief A cubic spline

   A cubic spline is a spline with C2 continuity at all control points.
   It is a non local spline, what means that all polynomials are changing
   when one control point has changed.

   The implementation is based on the fact, that the continuity condition
   means an equation with 3 unknowns for 3 adjacent points. The equation
   system can be resolved by defining start/end conditions, that allow
   substituting of one of the unknowns for the start/end equations.

   Resolving the equation system is a 2 pass algorithm, requiring more CPU costs
   than all other implemented type of splines.

   \todo The implementation is not numerical stable
 */
class QWT_EXPORT QwtSplineCubic : public QwtSplineC2
{
public:
	QwtSplineCubic();
	virtual ~QwtSplineCubic();

	virtual uint locality() const QWT_OVERRIDE;

    virtual QPainterPath painterPath(const QPolygonF&) const QWT_OVERRIDE;
    virtual QVector< QLineF > bezierControlLines(const QPolygonF& points) const QWT_OVERRIDE;

	// calculating the parametric equations
    virtual QVector< QwtSplinePolynomial > polynomials(const QPolygonF&) const QWT_OVERRIDE;
    virtual QVector< double > slopes(const QPolygonF&) const QWT_OVERRIDE;
    virtual QVector< double > curvatures(const QPolygonF&) const QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_spline_cubic.h ***/

/*** Start of inlined file: qwt_spline_local.h ***/
#ifndef QWT_SPLINE_LOCAL_H
#define QWT_SPLINE_LOCAL_H

/*!
   \brief A spline with C1 continuity

   QwtSplineLocal offers several standard algorithms for interpolating
   a curve with polynomials having C1 continuity at the control points.
   All algorithms are local in a sense, that changing one control point
   only few polynomials.
 */
class QWT_EXPORT QwtSplineLocal : public QwtSplineC1
{
public:
	/*!
        \brief Spline interpolation type

        All type of spline interpolations are lightweight algorithms
        calculating the slopes at a point by looking 1 or 2 points back
        and ahead.
	 */
	enum Type
	{
		/*!
		   A cardinal spline

		   The cardinal spline interpolation is a very cheap calculation with
		   a locality of 1.
		 */
		Cardinal,

		/*!
		   Parabolic blending is a cheap calculation with a locality of 1. Sometimes
		   it is also called Cubic Bessel interpolation.
		 */
		ParabolicBlending,

		/*!
		   The algorithm of H.Akima is a calculation with a locality of 2.
		 */
		Akima,

		/*!
		   Piecewise Cubic Hermite Interpolating Polynomial (PCHIP) is an algorithm
		   that is popular because of being offered by MATLAB.

		   It preserves the shape of the data and respects monotonicity. It has a
		   locality of 1.
		 */
		PChip
	};

    QwtSplineLocal(Type type);
	virtual ~QwtSplineLocal();

	Type type() const;

	virtual uint locality() const QWT_OVERRIDE;

    virtual QPainterPath painterPath(const QPolygonF&) const QWT_OVERRIDE;
    virtual QVector< QLineF > bezierControlLines(const QPolygonF&) const QWT_OVERRIDE;

	// calculating the parametric equations
    virtual QVector< QwtSplinePolynomial > polynomials(const QPolygonF&) const QWT_OVERRIDE;
    virtual QVector< double > slopes(const QPolygonF&) const QWT_OVERRIDE;

private:
	const Type m_type;
};

#endif

/*** End of inlined file: qwt_spline_local.h ***/

/*** Start of inlined file: qwt_spline_curve_fitter.h ***/
#ifndef QWT_SPLINE_CURVE_FITTER_H
#define QWT_SPLINE_CURVE_FITTER_H

class QwtSpline;

/*!
   \brief A curve fitter using a spline interpolation

   The default setting for the spline is a cardinal spline with
   uniform parametrization.

   \sa QwtSpline, QwtSplineLocal
 */
class QWT_EXPORT QwtSplineCurveFitter : public QwtCurveFitter
{
public:
	QwtSplineCurveFitter();
	virtual ~QwtSplineCurveFitter();

    void setSpline(QwtSpline*);

	const QwtSpline* spline() const;
	QwtSpline* spline();

    virtual QPolygonF fitCurve(const QPolygonF&) const QWT_OVERRIDE;
    virtual QPainterPath fitCurvePath(const QPolygonF&) const QWT_OVERRIDE;

private:
	QwtSpline* m_spline;
};

#endif

/*** End of inlined file: qwt_spline_curve_fitter.h ***/

/*** Start of inlined file: qwt_date_scale_draw.h ***/
#ifndef QWT_DATE_SCALE_DRAW_H
#define QWT_DATE_SCALE_DRAW_H

/*!
   \brief A class for drawing datetime scales

   QwtDateScaleDraw displays values as datetime labels.
   The format of the labels depends on the alignment of
   the major tick labels.

   The default format strings are:

   - Millisecond\n
    "hh:mm:ss:zzz\nddd dd MMM yyyy"
   - Second\n
    "hh:mm:ss\nddd dd MMM yyyy"
   - Minute\n
    "hh:mm\nddd dd MMM yyyy"
   - Hour\n
    "hh:mm\nddd dd MMM yyyy"
   - Day\n
    "ddd dd MMM yyyy"
   - Week\n
    "Www yyyy"
   - Month\n
    "MMM yyyy"
   - Year\n
    "yyyy"

   The format strings can be modified using setDateFormat()
   or individually for each tick label by overloading dateFormatOfDate(),

   Usually QwtDateScaleDraw is used in combination with
   QwtDateScaleEngine, that calculates scales for datetime
   intervals.

   \sa QwtDateScaleEngine, QwtPlot::setAxisScaleDraw()
 */
class QWT_EXPORT QwtDateScaleDraw : public QwtScaleDraw
{
public:
    explicit QwtDateScaleDraw(Qt::TimeSpec = Qt::LocalTime);
	virtual ~QwtDateScaleDraw();

    void setDateFormat(QwtDate::IntervalType, const QString&);
    QString dateFormat(QwtDate::IntervalType) const;

    void setTimeSpec(Qt::TimeSpec);
	Qt::TimeSpec timeSpec() const;

    void setUtcOffset(int seconds);
	int utcOffset() const;

    void setWeek0Type(QwtDate::Week0Type);
	QwtDate::Week0Type week0Type() const;

    virtual QwtText label(double) const QWT_OVERRIDE;

    QDateTime toDateTime(double) const;

protected:
    virtual QwtDate::IntervalType intervalType(const QwtScaleDiv&) const;

    virtual QString dateFormatOfDate(const QDateTime&, QwtDate::IntervalType) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_date_scale_draw.h ***/

/*** Start of inlined file: qwt_date_scale_engine.h ***/
#ifndef QWT_DATE_SCALE_ENGINE_H
#define QWT_DATE_SCALE_ENGINE_H

/*!
   \brief A scale engine for date/time values

   QwtDateScaleEngine builds scales from a time intervals.
   Together with QwtDateScaleDraw it can be used for
   axes according to date/time values.

   Years, months, weeks, days, hours and minutes are organized
   in steps with non constant intervals. QwtDateScaleEngine
   classifies intervals and aligns the boundaries and tick positions
   according to this classification.

   QwtDateScaleEngine supports representations depending
   on Qt::TimeSpec specifications. The valid range for scales
   is limited by the range of QDateTime, that differs
   between Qt4 and Qt5.

   Datetime values are expected as the number of milliseconds since
   1970-01-01T00:00:00 Universal Coordinated Time - also known
   as "The Epoch", that can be converted to QDateTime using
   QwtDate::toDateTime().

   \sa QwtDate, QwtPlot::setAxisScaleEngine(),
      QwtAbstractScale::setScaleEngine()
 */
class QWT_EXPORT QwtDateScaleEngine : public QwtLinearScaleEngine
{
public:
    explicit QwtDateScaleEngine(Qt::TimeSpec = Qt::LocalTime);
	virtual ~QwtDateScaleEngine();

    void setTimeSpec(Qt::TimeSpec);
	Qt::TimeSpec timeSpec() const;

    void setUtcOffset(int seconds);
	int utcOffset() const;

    void setWeek0Type(QwtDate::Week0Type);
	QwtDate::Week0Type week0Type() const;

    void setMaxWeeks(int);
	int maxWeeks() const;

    virtual void autoScale(int maxNumSteps, double& x1, double& x2, double& stepSize) const QWT_OVERRIDE;

    virtual QwtScaleDiv divideScale(double x1, double x2, int maxMajorSteps, int maxMinorSteps, double stepSize = 0.0) const QWT_OVERRIDE;

    virtual QwtDate::IntervalType intervalType(const QDateTime&, const QDateTime&, int maxSteps) const;

    QDateTime toDateTime(double) const;

protected:
    virtual QDateTime alignDate(const QDateTime&, double stepSize, QwtDate::IntervalType, bool up) const;

private:
    QwtScaleDiv buildScaleDiv(const QDateTime&, const QDateTime&, int maxMajorSteps, int maxMinorSteps, QwtDate::IntervalType) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_date_scale_engine.h ***/

/*** Start of inlined file: qwt_round_scale_draw.h ***/
#ifndef QWT_ROUND_SCALE_DRAW_H
#define QWT_ROUND_SCALE_DRAW_H

#include <qpoint.h>

/*!
   \brief A class for drawing round scales

   QwtRoundScaleDraw can be used to draw round scales.
   The circle segment can be adjusted by setAngleRange().
   The geometry of the scale can be specified with
   moveCenter() and setRadius().

   After a scale division has been specified as a QwtScaleDiv object
   using QwtAbstractScaleDraw::setScaleDiv(const QwtScaleDiv &s),
   the scale can be drawn with the QwtAbstractScaleDraw::draw() member.
 */

class QWT_EXPORT QwtRoundScaleDraw : public QwtAbstractScaleDraw
{
public:
	QwtRoundScaleDraw();
	virtual ~QwtRoundScaleDraw();

    void setRadius(double radius);
	double radius() const;

    void moveCenter(double x, double y);
    void moveCenter(const QPointF&);
	QPointF center() const;

    void setAngleRange(double angle1, double angle2);

    virtual double extent(const QFont&) const QWT_OVERRIDE;

protected:
    virtual void drawTick(QPainter*, double value, double len) const QWT_OVERRIDE;

    virtual void drawBackbone(QPainter*) const QWT_OVERRIDE;

    virtual void drawLabel(QPainter*, double value) const QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

//! Move the center of the scale draw, leaving the radius unchanged
inline void QwtRoundScaleDraw::moveCenter(double x, double y)
{
    moveCenter(QPointF(x, y));
}

#endif

/*** End of inlined file: qwt_round_scale_draw.h ***/

/*** Start of inlined file: qwt_series_store.h ***/
#ifndef QWT_SERIES_STORE_H
#define QWT_SERIES_STORE_H

/*!
   \brief Bridge between QwtSeriesStore and QwtPlotSeriesItem

   QwtAbstractSeriesStore is an abstract interface only
   to make it possible to isolate the template based methods ( QwtSeriesStore )
   from the regular methods ( QwtPlotSeriesItem ) to make it possible
   to derive from QwtPlotSeriesItem without any hassle with templates.
 */
class QwtAbstractSeriesStore
{
public:
	//! Destructor
	virtual ~QwtAbstractSeriesStore()
	{
	}

protected:
#ifndef QWT_PYTHON_WRAPPER
	//! dataChanged() indicates, that the series has been changed.
	virtual void dataChanged() = 0;

	/*!
	   Set a the "rectangle of interest" for the stored series
	   \sa QwtSeriesData<T>::setRectOfInterest()
	 */
	virtual void setRectOfInterest(const QRectF&) = 0;

	//! \return Bounding rectangle of the stored series
	virtual QRectF dataRect() const = 0;

	//! \return Number of samples
	virtual size_t dataSize() const = 0;
#else
	// Needed for generating the python bindings, but not for using them !
	virtual void dataChanged()
	{
	}
	virtual void setRectOfInterest(const QRectF&)
	{
	}
	virtual QRectF dataRect() const
	{
		return QRectF(0.0, 0.0, -1.0, -1.0);
	}
	virtual size_t dataSize() const
	{
		return 0;
	}
#endif
};

/*!
   \brief Class storing a QwtSeriesData object

   QwtSeriesStore and QwtPlotSeriesItem are intended as base classes for all
   plot items iterating over a series of samples. Both classes share
   a virtual base class ( QwtAbstractSeriesStore ) to bridge between them.

   QwtSeriesStore offers the template based part for the plot item API, so
   that QwtPlotSeriesItem can be derived without any hassle with templates.
 */
template< typename T >
class QwtSeriesStore : public virtual QwtAbstractSeriesStore
{
public:
	/*!
	   \brief Constructor
	   The store contains no series
	 */
	explicit QwtSeriesStore();

	//! Destructor
	~QwtSeriesStore();

	/*!
	   Assign a series of samples

	   \param series Data
	   \warning The item takes ownership of the data object, deleting
               it when its not used anymore.
	 */
	void setData(QwtSeriesData< T >* series);

	//! \return the the series data
	QwtSeriesData< T >* data();

	//! \return the the series data
	const QwtSeriesData< T >* data() const;

	/*!
        \param index Index
        \return Sample at position index
	 */
	T sample(int index) const;

	/*!
	   \return Number of samples of the series
	   \sa setData(), QwtSeriesData<T>::size()
	 */
	virtual size_t dataSize() const QWT_OVERRIDE;

	/*!
	   \return Bounding rectangle of the series
              or an invalid rectangle, when no series is stored

	   \sa QwtSeriesData<T>::boundingRect()
	 */
	virtual QRectF dataRect() const QWT_OVERRIDE;

	/*!
	   Set a the "rect of interest" for the series

	   \param rect Rectangle of interest
	   \sa QwtSeriesData<T>::setRectOfInterest()
	 */
	virtual void setRectOfInterest(const QRectF& rect) QWT_OVERRIDE;

	/*!
	   Replace a series without deleting the previous one

	   \param series New series
	   \return Previously assigned series
	 */
	QwtSeriesData< T >* swapData(QwtSeriesData< T >* series);

private:
	QwtSeriesData< T >* m_series;
};

template< typename T >
QwtSeriesStore< T >::QwtSeriesStore() : m_series(NULL)
{
}

template< typename T >
QwtSeriesStore< T >::~QwtSeriesStore()
{
	delete m_series;
}

template< typename T >
inline QwtSeriesData< T >* QwtSeriesStore< T >::data()
{
	return m_series;
}

template< typename T >
inline const QwtSeriesData< T >* QwtSeriesStore< T >::data() const
{
	return m_series;
}

template< typename T >
inline T QwtSeriesStore< T >::sample(int index) const
{
	return m_series ? m_series->sample(index) : T();
}

template< typename T >
void QwtSeriesStore< T >::setData(QwtSeriesData< T >* series)
{
	if (m_series != series) {
		delete m_series;
		m_series = series;
		dataChanged();
	}
}

template< typename T >
size_t QwtSeriesStore< T >::dataSize() const
{
	if (m_series == NULL)
		return 0;

	return m_series->size();
}

template< typename T >
QRectF QwtSeriesStore< T >::dataRect() const
{
	if (m_series == NULL)
		return QRectF(1.0, 1.0, -2.0, -2.0);  // invalid

	return m_series->boundingRect();
}

template< typename T >
void QwtSeriesStore< T >::setRectOfInterest(const QRectF& rect)
{
	if (m_series)
		m_series->setRectOfInterest(rect);
}

template< typename T >
QwtSeriesData< T >* QwtSeriesStore< T >::swapData(QwtSeriesData< T >* series)
{
	QwtSeriesData< T >* swappedSeries = m_series;
	m_series                          = series;

	return swappedSeries;
}

#endif

/*** End of inlined file: qwt_series_store.h ***/

/*** Start of inlined file: qwt_point_data.h ***/
#ifndef QWT_POINT_DATA_H
#define QWT_POINT_DATA_H

#include <cstring>

/*!
   \brief Interface for iterating over two QVector<T> objects.
 */
template< typename T >
class QwtPointArrayData : public QwtPointSeriesData
{
public:
	QwtPointArrayData(const QVector< T >& x, const QVector< T >& y);
	QwtPointArrayData(QVector< T >&& x, QVector< T >&& y);
	QwtPointArrayData(const T* x, const T* y, size_t size);

	virtual size_t size() const QWT_OVERRIDE;
	virtual QPointF sample(size_t index) const QWT_OVERRIDE;

	const QVector< T >& xData() const;
	const QVector< T >& yData() const;

private:
	QVector< T > m_x;
	QVector< T > m_y;
};

/*!
   \brief Data class containing two pointers to memory blocks of T.
 */
template< typename T >
class QwtCPointerData : public QwtPointSeriesData
{
public:
	QwtCPointerData(const T* x, const T* y, size_t size);

	virtual size_t size() const QWT_OVERRIDE;
	virtual QPointF sample(size_t index) const QWT_OVERRIDE;

	const T* xData() const;
	const T* yData() const;

private:
	const T* m_x;
	const T* m_y;
	size_t m_size;
};

/*!
   \brief Interface for iterating over a QVector<T>.

   The memory contains the y coordinates, while the index is
   interpreted as x coordinate.
 */
template< typename T >
class QwtValuePointData : public QwtPointSeriesData
{
public:
	QwtValuePointData(const QVector< T >& y);
	QwtValuePointData(const T* y, size_t size);

	virtual size_t size() const QWT_OVERRIDE;
	virtual QPointF sample(size_t index) const QWT_OVERRIDE;

	const QVector< T >& yData() const;

private:
	QVector< T > m_y;
};

/*!
   \brief Data class containing a pointer to memory of y coordinates

   The memory contains the y coordinates, while the index is
   interpreted as x coordinate.
 */
template< typename T >
class QwtCPointerValueData : public QwtPointSeriesData
{
public:
	QwtCPointerValueData(const T* y, size_t size);

	virtual size_t size() const QWT_OVERRIDE;
	virtual QPointF sample(size_t index) const QWT_OVERRIDE;

	const T* yData() const;

private:
	const T* m_y;
	size_t m_size;
};

/**
 *   @brief Synthetic point data
 *
 *   QwtSyntheticPointData provides a fixed number of points for an interval.
 *   The points are calculated in equidistant steps in x-direction.
 *
 *   If the interval is invalid, the points are calculated for
 *   the "rectangle of interest", what normally is the displayed area on the
 *   plot canvas. In this mode you get different levels of detail, when
 *   zooming in/out.
 *
 *   @par Example
 *
 *   The following example shows how to implement a sinus curve.
 *
 *   @code
 *   #include <cmath>
 *   #include <qwt_series_data.h>
 *   #include <qwt_plot_curve.h>
 *   #include <qwt_plot.h>
 *   #include <qapplication.h>
 *
 *   class SinusData: public QwtSyntheticPointData
 *   {
 *   public:
 *    SinusData():
 *        QwtSyntheticPointData( 100 )
 *    {
 *    }
 *
 *    virtual double y( double x ) const
 *    {
 *        return qSin( x );
 *    }
 *   };
 *
 *   int main(int argc, char **argv)
 *   {
 *    QApplication a( argc, argv );
 *
 *    QwtPlot plot;
 *    plot.setAxisScale( QwtAxis::XBottom, 0.0, 10.0 );
 *    plot.setAxisScale( QwtAxis::YLeft, -1.0, 1.0 );
 *
 *    QwtPlotCurve *curve = new QwtPlotCurve( "y = sin(x)" );
 *    curve->setData( new SinusData() );
 *    curve->attach( &plot );
 *
 *    plot.show();
 *    return a.exec();
 *   }
 *   @endcode
 */
class QWT_EXPORT QwtSyntheticPointData : public QwtPointSeriesData
{
public:
	QwtSyntheticPointData(size_t size, const QwtInterval& = QwtInterval());

	void setSize(size_t size);
	virtual size_t size() const QWT_OVERRIDE;

	void setInterval(const QwtInterval&);
	QwtInterval interval() const;

	virtual QRectF boundingRect() const QWT_OVERRIDE;
	virtual QPointF sample(size_t index) const QWT_OVERRIDE;

	/*!
	   Calculate a y value for a x value

	   \param x x value
	   \return Corresponding y value
	 */
	virtual double y(double x) const = 0;
	virtual double x(uint index) const;

	virtual void setRectOfInterest(const QRectF&) QWT_OVERRIDE;
	QRectF rectOfInterest() const;

private:
	size_t m_size;
	QwtInterval m_interval;
	QRectF m_rectOfInterest;
	QwtInterval m_intervalOfInterest;
};

/*!
   Constructor

   \param x Array of x values
   \param y Array of y values

   \sa QwtPlotCurve::setData(), QwtPlotCurve::setSamples()
 */
template< typename T >
QwtPointArrayData< T >::QwtPointArrayData(const QVector< T >& x, const QVector< T >& y) : m_x(x), m_y(y)
{
}

/*!
   Constructor

   \param x Array of x values
   \param y Array of y values

   \sa QwtPlotCurve::setData(), QwtPlotCurve::setSamples()
 */
template< typename T >
QwtPointArrayData< T >::QwtPointArrayData(QVector< T >&& x, QVector< T >&& y)
{
	m_x = std::move(x);
	m_y = std::move(y);
}

/*!
   Constructor

   \param x Array of x values
   \param y Array of y values
   \param size Size of the x and y arrays
   \sa QwtPlotCurve::setData(), QwtPlotCurve::setSamples()
 */
template< typename T >
QwtPointArrayData< T >::QwtPointArrayData(const T* x, const T* y, size_t size)
{
	m_x.resize(size);
	std::memcpy(m_x.data(), x, size * sizeof(T));

	m_y.resize(size);
	std::memcpy(m_y.data(), y, size * sizeof(T));
}

//! \return Size of the data set
template< typename T >
size_t QwtPointArrayData< T >::size() const
{
	return qMin(m_x.size(), m_y.size());
}

/*!
   Return the sample at position i

   \param index Index
   \return Sample at position i
 */
template< typename T >
QPointF QwtPointArrayData< T >::sample(size_t index) const
{
	return QPointF(m_x[ int(index) ], m_y[ int(index) ]);
}

//! \return Array of the x-values
template< typename T >
const QVector< T >& QwtPointArrayData< T >::xData() const
{
	return m_x;
}

//! \return Array of the y-values
template< typename T >
const QVector< T >& QwtPointArrayData< T >::yData() const
{
	return m_y;
}

/*!
   Constructor

   \param y Array of y values

   \sa QwtPlotCurve::setData(), QwtPlotCurve::setSamples()
 */
template< typename T >
QwtValuePointData< T >::QwtValuePointData(const QVector< T >& y) : m_y(y)
{
}

/*!
   Constructor

   \param y Array of y values
   \param size Size of the x and y arrays
   \sa QwtPlotCurve::setData(), QwtPlotCurve::setSamples()
 */
template< typename T >
QwtValuePointData< T >::QwtValuePointData(const T* y, size_t size)
{
	m_y.resize(size);
	std::memcpy(m_y.data(), y, size * sizeof(T));
}

//! \return Size of the data set
template< typename T >
size_t QwtValuePointData< T >::size() const
{
	return m_y.size();
}

/*!
   Return the sample at position i

   \param index Index
   \return Sample at position i
 */
template< typename T >
QPointF QwtValuePointData< T >::sample(size_t index) const
{
	return QPointF(index, m_y[ int(index) ]);
}

//! \return Array of the y-values
template< typename T >
const QVector< T >& QwtValuePointData< T >::yData() const
{
	return m_y;
}

/*!
   Constructor

   \param x Array of x values
   \param y Array of y values
   \param size Size of the x and y arrays

   \warning The programmer must assure that the memory blocks referenced
           by the pointers remain valid during the lifetime of the
           QwtPlotCPointer object.

   \sa QwtPlotCurve::setData(), QwtPlotCurve::setRawSamples()
 */

template< typename T >
QwtCPointerData< T >::QwtCPointerData(const T* x, const T* y, size_t size) : m_x(x), m_y(y), m_size(size)
{
}

//! \return Size of the data set
template< typename T >
size_t QwtCPointerData< T >::size() const
{
	return m_size;
}

/*!
   Return the sample at position i

   \param index Index
   \return Sample at position i
 */
template< typename T >
QPointF QwtCPointerData< T >::sample(size_t index) const
{
	return QPointF(m_x[ int(index) ], m_y[ int(index) ]);
}

//! \return Array of the x-values
template< typename T >
const T* QwtCPointerData< T >::xData() const
{
	return m_x;
}

//! \return Array of the y-values
template< typename T >
const T* QwtCPointerData< T >::yData() const
{
	return m_y;
}

/*!
   Constructor

   \param y Array of y values
   \param size Size of the x and y arrays

   \warning The programmer must assure that the memory blocks referenced
           by the pointers remain valid during the lifetime of the
           QwtCPointerValueData object.

   \sa QwtPlotCurve::setData(), QwtPlotCurve::setRawSamples()
 */

template< typename T >
QwtCPointerValueData< T >::QwtCPointerValueData(const T* y, size_t size) : m_y(y), m_size(size)
{
}

//! \return Size of the data set
template< typename T >
size_t QwtCPointerValueData< T >::size() const
{
	return m_size;
}

/*!
   Return the sample at position i

   \param index Index
   \return Sample at position i
 */
template< typename T >
QPointF QwtCPointerValueData< T >::sample(size_t index) const
{
	return QPointF(index, m_y[ int(index) ]);
}

//! \return Array of the y-values
template< typename T >
const T* QwtCPointerValueData< T >::yData() const
{
	return m_y;
}

#endif

/*** End of inlined file: qwt_point_data.h ***/

/*** Start of inlined file: qwt_point_mapper.h ***/
#ifndef QWT_POINT_MAPPER_H
#define QWT_POINT_MAPPER_H

class QwtScaleMap;
template< typename T >
class QwtSeriesData;
class QPolygonF;
class QPointF;
class QRectF;
class QPolygon;
class QPen;
class QImage;

/*!
   \brief A helper class for translating a series of points

   QwtPointMapper is a collection of methods and optimizations
   for translating a series of points into paint device coordinates.
   It is used by QwtPlotCurve but might also be useful for
   similar plot items displaying a QwtSeriesData<QPointF>.
 */
class QWT_EXPORT QwtPointMapper
{
public:
	/*!
	   \brief Flags affecting the transformation process
	   \sa setFlag(), setFlags()
	 */
	enum TransformationFlag
	{
		//! Round points to integer values
		RoundPoints = 0x01,

		/*!
		   Try to remove points, that are translated to the
		   same position.
		 */
		WeedOutPoints = 0x02,

		/*!
		   An even more aggressive weeding algorithm, that
		   can be used in toPolygon().

		   A consecutive chunk of points being mapped to the
		   same x coordinate is reduced to 4 points:

              - first point
              - point with the minimum y coordinate
              - point with the maximum y coordinate
              - last point

		   In the worst case ( first and last points are never one of the extremes )
		   the number of points will be 4 times the width.

		   As the algorithm is fast it can be used inside of
		   a polyline render cycle.
		 */
		WeedOutIntermediatePoints = 0x04
	};

    Q_DECLARE_FLAGS(TransformationFlags, TransformationFlag)

	QwtPointMapper();
	~QwtPointMapper();

    void setFlags(TransformationFlags);
	TransformationFlags flags() const;

    void setFlag(TransformationFlag, bool on = true);
    bool testFlag(TransformationFlag) const;

    void setBoundingRect(const QRectF&);
	QRectF boundingRect() const;

    QPolygonF toPolygonF(const QwtScaleMap& xMap,
                         const QwtScaleMap& yMap,
                         const QwtSeriesData< QPointF >* series,
                         int from,
                         int to) const;

    QPolygon toPolygon(const QwtScaleMap& xMap,
                       const QwtScaleMap& yMap,
                       const QwtSeriesData< QPointF >* series,
                       int from,
                       int to) const;

    QPolygon toPoints(const QwtScaleMap& xMap,
                      const QwtScaleMap& yMap,
                      const QwtSeriesData< QPointF >* series,
                      int from,
                      int to) const;

    QPolygonF toPointsF(const QwtScaleMap& xMap,
                        const QwtScaleMap& yMap,
                        const QwtSeriesData< QPointF >* series,
                        int from,
                        int to) const;

    QImage toImage(const QwtScaleMap& xMap,
                   const QwtScaleMap& yMap,
                   const QwtSeriesData< QPointF >* series,
                   int from,
                   int to,
                   const QPen&,
                   bool antialiased,
                   uint numThreads) const;

private:
	Q_DISABLE_COPY(QwtPointMapper)

	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPointMapper::TransformationFlags)

#endif

/*** End of inlined file: qwt_point_mapper.h ***/

/*** Start of inlined file: qwt_dial_needle.h ***/
#ifndef QWT_DIAL_NEEDLE_H
#define QWT_DIAL_NEEDLE_H

#include <qpalette.h>

class QPainter;

/*!
   \brief Base class for needles that can be used in a QwtDial.

   QwtDialNeedle is a pointer that indicates a value by pointing
   to a specific direction.

   \sa QwtDial, QwtCompass
 */

class QWT_EXPORT QwtDialNeedle
{
public:
	QwtDialNeedle();
	virtual ~QwtDialNeedle();

    virtual void setPalette(const QPalette&);
	const QPalette& palette() const;

    virtual void draw(QPainter*,
                      const QPointF& center,
                      double length,
                      double direction,
                      QPalette::ColorGroup = QPalette::Active) const;

protected:
	/*!
	   \brief Draw the needle

	   The origin of the needle is at position (0.0, 0.0 )
	   pointing in direction 0.0 ( = east ).

	   The painter is already initialized with translation and
	   rotation.

	   \param painter Painter
	   \param length Length of the needle
	   \param colorGroup Color group, used for painting

	   \sa setPalette(), palette()
	 */
    virtual void drawNeedle(QPainter* painter, double length, QPalette::ColorGroup colorGroup) const = 0;

    virtual void drawKnob(QPainter*, double width, const QBrush&, bool sunken) const;

private:
	Q_DISABLE_COPY(QwtDialNeedle)

	QPalette m_palette;
};

/*!
   \brief A needle for dial widgets

   The following colors are used:

   - QPalette::Mid\n
    Pointer
   - QPalette::Base\n
    Knob

   \sa QwtDial, QwtCompass
 */

class QWT_EXPORT QwtDialSimpleNeedle : public QwtDialNeedle
{
public:
	//! Style of the needle
	enum Style
	{
		//! Arrow
		Arrow,

		//! A straight line from the center
		Ray
	};

    QwtDialSimpleNeedle(Style, bool hasKnob = true, const QColor& mid = Qt::gray, const QColor& base = Qt::darkGray);

    void setWidth(double width);
	double width() const;

protected:
    virtual void drawNeedle(QPainter*, double length, QPalette::ColorGroup) const QWT_OVERRIDE;

private:
	Style m_style;
	bool m_hasKnob;
	double m_width;
};

/*!
   \brief A magnet needle for compass widgets

   A magnet needle points to two opposite directions indicating
   north and south.

   The following colors are used:
   - QPalette::Light\n
    Used for pointing south
   - QPalette::Dark\n
    Used for pointing north
   - QPalette::Base\n
    Knob (ThinStyle only)

   \sa QwtDial, QwtCompass
 */

class QWT_EXPORT QwtCompassMagnetNeedle : public QwtDialNeedle
{
public:
	//! Style of the needle
	enum Style
	{
		//! A needle with a triangular shape
		TriangleStyle,

		//! A thin needle
		ThinStyle
	};

    QwtCompassMagnetNeedle(Style = TriangleStyle, const QColor& light = Qt::white, const QColor& dark = Qt::red);

protected:
    virtual void drawNeedle(QPainter*, double length, QPalette::ColorGroup) const QWT_OVERRIDE;

private:
	Style m_style;
};

/*!
   \brief An indicator for the wind direction

   QwtCompassWindArrow shows the direction where the wind comes from.

   - QPalette::Light\n
    Used for Style1, or the light half of Style2
   - QPalette::Dark\n
    Used for the dark half of Style2

   \sa QwtDial, QwtCompass
 */

class QWT_EXPORT QwtCompassWindArrow : public QwtDialNeedle
{
public:
	//! Style of the arrow
	enum Style
	{
		//! A needle pointing to the center
		Style1,

		//! A needle pointing to the center
		Style2
	};

    QwtCompassWindArrow(Style, const QColor& light = Qt::white, const QColor& dark = Qt::gray);

protected:
    virtual void drawNeedle(QPainter*, double length, QPalette::ColorGroup) const QWT_OVERRIDE;

private:
	Style m_style;
};

#endif

/*** End of inlined file: qwt_dial_needle.h ***/

/*** Start of inlined file: qwt_dial.h ***/
#ifndef QWT_DIAL_H
#define QWT_DIAL_H

#include <qframe.h>
#include <qpalette.h>

class QwtDialNeedle;
class QwtRoundScaleDraw;
class QwtAbstractScaleDraw;

/*!
   \brief QwtDial class provides a rounded range control.

   QwtDial is intended as base class for dial widgets like
   speedometers, compass widgets, clocks ...

   \image html dials2.png

   A dial contains a scale and a needle indicating the current value
   of the dial. Depending on Mode one of them is fixed and the
   other is rotating. If not isReadOnly() the
   dial can be rotated by dragging the mouse or using keyboard inputs
   (see QwtAbstractSlider::keyPressEvent()). A dial might be wrapping, what means
   a rotation below/above one limit continues on the other limit (f.e compass).
   The scale might cover any arc of the dial, its values are related to
   the origin() of the dial.

   Often dials have to be updated very often according to values from external
   devices. For these high refresh rates QwtDial caches as much as possible.
   For derived classes it might be necessary to clear these caches manually
   according to attribute changes using invalidateCache().

   \sa QwtCompass, QwtAnalogClock, QwtDialNeedle
   \note The controls and dials examples shows different types of dials.
   \note QDial is more similar to QwtKnob than to QwtDial
 */

class QWT_EXPORT QwtDial : public QwtAbstractSlider
{
	Q_OBJECT

    Q_ENUMS(Shadow Mode Direction)

    Q_PROPERTY(int lineWidth READ lineWidth WRITE setLineWidth)
    Q_PROPERTY(Shadow frameShadow READ frameShadow WRITE setFrameShadow)
    Q_PROPERTY(Mode mode READ mode WRITE setMode)
    Q_PROPERTY(double origin READ origin WRITE setOrigin)
    Q_PROPERTY(double minScaleArc READ minScaleArc WRITE setMinScaleArc)
    Q_PROPERTY(double maxScaleArc READ maxScaleArc WRITE setMaxScaleArc)

public:
    /*!
        \brief Frame shadow

         Unfortunately it is not possible to use QFrame::Shadow
         as a property of a widget that is not derived from QFrame.
         The following enum is made for the designer only. It is safe
         to use QFrame::Shadow instead.
	 */
	enum Shadow
	{
		//! QFrame::Plain
		Plain = QFrame::Plain,

		//! QFrame::Raised
		Raised = QFrame::Raised,

		//! QFrame::Sunken
		Sunken = QFrame::Sunken
	};

	//! Mode controlling whether the needle or the scale is rotating
	enum Mode
	{
		//! The needle is rotating
		RotateNeedle,

		//! The needle is fixed, the scales are rotating
		RotateScale
	};

    explicit QwtDial(QWidget* parent = NULL);
	virtual ~QwtDial();

    void setFrameShadow(Shadow);
	Shadow frameShadow() const;

    void setLineWidth(int);
	int lineWidth() const;

    void setMode(Mode);
	Mode mode() const;

    void setScaleArc(double minArc, double maxArc);

    void setMinScaleArc(double);
	double minScaleArc() const;

    void setMaxScaleArc(double);
	double maxScaleArc() const;

    virtual void setOrigin(double);
	double origin() const;

    void setNeedle(QwtDialNeedle*);
	const QwtDialNeedle* needle() const;
	QwtDialNeedle* needle();

	QRect boundingRect() const;
	QRect innerRect() const;

	virtual QRect scaleInnerRect() const;

	virtual QSize sizeHint() const QWT_OVERRIDE;
	virtual QSize minimumSizeHint() const QWT_OVERRIDE;

    void setScaleDraw(QwtRoundScaleDraw*);

	QwtRoundScaleDraw* scaleDraw();
	const QwtRoundScaleDraw* scaleDraw() const;

protected:
    virtual void wheelEvent(QWheelEvent*) QWT_OVERRIDE;
    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;
    virtual void changeEvent(QEvent*) QWT_OVERRIDE;

    virtual void drawFrame(QPainter*);
    virtual void drawContents(QPainter*) const;
    virtual void drawFocusIndicator(QPainter*) const;

	void invalidateCache();

    virtual void drawScale(QPainter*, const QPointF& center, double radius) const;

    virtual void drawScaleContents(QPainter* painter, const QPointF& center, double radius) const;

    virtual void drawNeedle(QPainter*, const QPointF&, double radius, double direction, QPalette::ColorGroup) const;

    virtual double scrolledTo(const QPoint&) const QWT_OVERRIDE;
    virtual bool isScrollPosition(const QPoint&) const QWT_OVERRIDE;

	virtual void sliderChange() QWT_OVERRIDE;
	virtual void scaleChange() QWT_OVERRIDE;

private:
    void setAngleRange(double angle, double span);
    void drawNeedle(QPainter*) const;

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_dial.h ***/

/*** Start of inlined file: qwt_compass.h ***/
#ifndef QWT_COMPASS_H
#define QWT_COMPASS_H

class QwtCompassRose;
class QString;
template< class Key, class T >
class QMap;

/*!
   \brief A special scale draw made for QwtCompass

   QwtCompassScaleDraw maps values to strings using
   a special map, that can be modified by the application

   The default map consists of the labels N, NE, E, SE, S, SW, W, NW.

   \sa QwtCompass
 */
class QWT_EXPORT QwtCompassScaleDraw : public QwtRoundScaleDraw
{
public:
	explicit QwtCompassScaleDraw();
    explicit QwtCompassScaleDraw(const QMap< double, QString >& map);

	virtual ~QwtCompassScaleDraw();

    void setLabelMap(const QMap< double, QString >& map);
	QMap< double, QString > labelMap() const;

    virtual QwtText label(double value) const QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

/*!
   \brief A Compass Widget

   QwtCompass is a widget to display and enter directions. It consists
   of a scale, an optional needle and rose.

   \image html dials1.png

   \note The examples/dials example shows how to use QwtCompass.
 */

class QWT_EXPORT QwtCompass : public QwtDial
{
	Q_OBJECT

public:
    explicit QwtCompass(QWidget* parent = NULL);
	virtual ~QwtCompass();

    void setRose(QwtCompassRose* rose);
	const QwtCompassRose* rose() const;
	QwtCompassRose* rose();

protected:
    virtual void drawRose(QPainter*, const QPointF& center, double radius, double north, QPalette::ColorGroup) const;

    virtual void drawScaleContents(QPainter*, const QPointF& center, double radius) const QWT_OVERRIDE;

    virtual void keyPressEvent(QKeyEvent*) QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_compass.h ***/

/*** Start of inlined file: qwt_knob.h ***/
#ifndef QWT_KNOB_H
#define QWT_KNOB_H

class QwtRoundScaleDraw;

/*!
   \brief The Knob Widget

   The QwtKnob widget imitates look and behavior of a volume knob on a radio.
   It looks similar to QDial - not to QwtDial.

   The value range of a knob might be divided into several turns.

   The layout of the knob depends on the knobWidth().

   - width > 0
    The diameter of the knob is fixed and the knob is aligned
    according to the alignment() flags inside of the contentsRect().

   - width <= 0
    The knob is extended to the minimum of width/height of the contentsRect()
    and aligned in the other direction according to alignment().

   Setting a fixed knobWidth() is helpful to align several knobs with different
   scale labels.

   \image html knob.png
 */

class QWT_EXPORT QwtKnob : public QwtAbstractSlider
{
	Q_OBJECT

    Q_ENUMS(KnobStyle MarkerStyle)

    Q_PROPERTY(KnobStyle knobStyle READ knobStyle WRITE setKnobStyle)
    Q_PROPERTY(int knobWidth READ knobWidth WRITE setKnobWidth)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
    Q_PROPERTY(double totalAngle READ totalAngle WRITE setTotalAngle)
    Q_PROPERTY(int numTurns READ numTurns WRITE setNumTurns)
    Q_PROPERTY(MarkerStyle markerStyle READ markerStyle WRITE setMarkerStyle)
    Q_PROPERTY(int markerSize READ markerSize WRITE setMarkerSize)
    Q_PROPERTY(int borderWidth READ borderWidth WRITE setBorderWidth)

public:
	/*!
	   \brief Style of the knob surface

	   Depending on the KnobStyle the surface of the knob is
	   filled from the brushes of the widget palette().

	   \sa setKnobStyle(), knobStyle()
	 */
	enum KnobStyle
	{
		//! Fill the knob with a brush from QPalette::Button.
		Flat,

		//! Build a gradient from QPalette::Midlight and QPalette::Button
		Raised,

		/*!
		   Build a gradient from QPalette::Midlight, QPalette::Button
		   and QPalette::Midlight
		 */
		Sunken,

		/*!
		   Build a radial gradient from QPalette::Button
		   like it is used for QDial in various Qt styles.
		 */
		Styled
	};

	/*!
        \brief Marker type

        The marker indicates the current value on the knob
        The default setting is a Notch marker.

        \sa setMarkerStyle(), setMarkerSize()
	 */
	enum MarkerStyle
	{
		//! Don't paint any marker
		NoMarker = -1,

		//! Paint a single tick in QPalette::ButtonText color
		Tick,

		//! Paint a triangle in QPalette::ButtonText color
		Triangle,

		//! Paint a circle in QPalette::ButtonText color
		Dot,

		/*!
		   Draw a raised ellipse with a gradient build from
		   QPalette::Light and QPalette::Mid
		 */
		Nub,

		/*!
		   Draw a sunken ellipse with a gradient build from
		   QPalette::Light and QPalette::Mid
		 */
		Notch
	};

    explicit QwtKnob(QWidget* parent = NULL);
	virtual ~QwtKnob();

    void setAlignment(Qt::Alignment);
	Qt::Alignment alignment() const;

    void setKnobWidth(int);
	int knobWidth() const;

    void setNumTurns(int);
	int numTurns() const;

    void setTotalAngle(double angle);
	double totalAngle() const;

    void setKnobStyle(KnobStyle);
	KnobStyle knobStyle() const;

    void setBorderWidth(int);
	int borderWidth() const;

    void setMarkerStyle(MarkerStyle);
	MarkerStyle markerStyle() const;

    void setMarkerSize(int);
	int markerSize() const;

	virtual QSize sizeHint() const QWT_OVERRIDE;
	virtual QSize minimumSizeHint() const QWT_OVERRIDE;

    void setScaleDraw(QwtRoundScaleDraw*);

	const QwtRoundScaleDraw* scaleDraw() const;
	QwtRoundScaleDraw* scaleDraw();

	QRect knobRect() const;

protected:
    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;
    virtual void changeEvent(QEvent*) QWT_OVERRIDE;

    virtual void drawKnob(QPainter*, const QRectF&) const;

    virtual void drawFocusIndicator(QPainter*) const;

    virtual void drawMarker(QPainter*, const QRectF&, double angle) const;

    virtual double scrolledTo(const QPoint&) const QWT_OVERRIDE;
    virtual bool isScrollPosition(const QPoint&) const QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_knob.h ***/

/*** Start of inlined file: qwt_analog_clock.h ***/
#ifndef QWT_ANALOG_CLOCK_H
#define QWT_ANALOG_CLOCK_H

class QwtDialNeedle;

/*!
   \brief An analog clock

   \image html analogclock.png

   \par Example
   \code

   QwtAnalogClock *clock = new QwtAnalogClock(...);
   clock->scaleDraw()->setPenWidth(3);
   clock->setLineWidth(6);
   clock->setFrameShadow(QwtDial::Sunken);
   clock->setTime();

   // update the clock every second
   QTimer *timer = new QTimer(clock);
   timer->connect(timer, SIGNAL(timeout()), clock, SLOT(setCurrentTime()));
   timer->start(1000);

   \endcode

   \note The examples/dials example shows how to use QwtAnalogClock.
 */

class QWT_EXPORT QwtAnalogClock : public QwtDial
{
	Q_OBJECT

public:
	/*!
        Hand type
        \sa setHand(), hand()
	 */
	enum Hand
	{
		//! Needle displaying the seconds
		SecondHand,

		//! Needle displaying the minutes
		MinuteHand,

		//! Needle displaying the hours
		HourHand,

		//! Number of needles
		NHands
	};

    explicit QwtAnalogClock(QWidget* parent = NULL);
	virtual ~QwtAnalogClock();

    void setHand(Hand, QwtDialNeedle*);

    const QwtDialNeedle* hand(Hand) const;
    QwtDialNeedle* hand(Hand);

public Q_SLOTS:
	void setCurrentTime();
    void setTime(const QTime&);

protected:
    virtual void drawNeedle(QPainter*, const QPointF&, double radius, double direction, QPalette::ColorGroup) const QWT_OVERRIDE;

    virtual void drawHand(QPainter*, Hand, const QPointF&, double radius, double direction, QPalette::ColorGroup) const;

private:
	// use setHand instead
    void setNeedle(QwtDialNeedle*);

    QwtDialNeedle* m_hand[ NHands ];
};

#endif

/*** End of inlined file: qwt_analog_clock.h ***/

/*** Start of inlined file: qwt_magnifier.h ***/
#ifndef QWT_MAGNIFIER_H
#define QWT_MAGNIFIER_H

#include <qobject.h>

class QWidget;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;

/*!
   \brief QwtMagnifier provides zooming, by magnifying in steps.

   Using QwtMagnifier a plot can be zoomed in/out in steps using
   keys, the mouse wheel or moving a mouse button in vertical direction.
 */
class QWT_EXPORT QwtMagnifier : public QObject
{
	Q_OBJECT

public:
    explicit QwtMagnifier(QWidget*);
	virtual ~QwtMagnifier();

	QWidget* parentWidget();
	const QWidget* parentWidget() const;

    void setEnabled(bool);
	bool isEnabled() const;

	// mouse
    void setMouseFactor(double);
	double mouseFactor() const;

    void setMouseButton(Qt::MouseButton, Qt::KeyboardModifiers = Qt::NoModifier);
    void getMouseButton(Qt::MouseButton&, Qt::KeyboardModifiers&) const;

	// mouse wheel
    void setWheelFactor(double);
	double wheelFactor() const;

    void setWheelModifiers(Qt::KeyboardModifiers);
	Qt::KeyboardModifiers wheelModifiers() const;

	// keyboard
    void setKeyFactor(double);
	double keyFactor() const;

    void setZoomInKey(int key, Qt::KeyboardModifiers = Qt::NoModifier);
    void getZoomInKey(int& key, Qt::KeyboardModifiers&) const;

    void setZoomOutKey(int key, Qt::KeyboardModifiers = Qt::NoModifier);
    void getZoomOutKey(int& key, Qt::KeyboardModifiers&) const;

    virtual bool eventFilter(QObject*, QEvent*) QWT_OVERRIDE;

protected:
	/*!
	   Rescale the parent widget
	   \param factor Scale factor
	 */
    virtual void rescale(double factor) = 0;

    virtual void widgetMousePressEvent(QMouseEvent*);
    virtual void widgetMouseReleaseEvent(QMouseEvent*);
    virtual void widgetMouseMoveEvent(QMouseEvent*);
    virtual void widgetWheelEvent(QWheelEvent*);
    virtual void widgetKeyPressEvent(QKeyEvent*);
    virtual void widgetKeyReleaseEvent(QKeyEvent*);

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_magnifier.h ***/

/*** Start of inlined file: qwt_picker_machine.h ***/
#ifndef QWT_PICKER_MACHINE
#define QWT_PICKER_MACHINE

class QwtEventPattern;
class QEvent;
template< typename T >
class QList;

/*!
   \brief A state machine for QwtPicker selections

   QwtPickerMachine accepts key and mouse events and translates them
   into selection commands.

   \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
 */

class QWT_EXPORT QwtPickerMachine
{
public:
	/*!
	   Type of a selection.
	   \sa selectionType()
	 */
	enum SelectionType
	{
		//! The state machine not usable for any type of selection.
		NoSelection = -1,

		//! The state machine is for selecting a single point.
		PointSelection,

		//! The state machine is for selecting a rectangle (2 points).
		RectSelection,

		//! The state machine is for selecting a polygon (many points).
		PolygonSelection
	};

	//! Commands - the output of a state machine
	enum Command
	{
		Begin,
		Append,
		Move,
		Remove,
		End
	};

    explicit QwtPickerMachine(SelectionType);
	virtual ~QwtPickerMachine();

	//! Transition
    virtual QList< Command > transition(const QwtEventPattern&, const QEvent*) = 0;
	void reset();

	int state() const;
    void setState(int);

	SelectionType selectionType() const;

private:
	const SelectionType m_selectionType;
	int m_state;
};

/*!
   \brief A state machine for indicating mouse movements

   QwtPickerTrackerMachine supports displaying information
   corresponding to mouse movements, but is not intended for
   selecting anything. Begin/End are related to Enter/Leave events.
 */
class QWT_EXPORT QwtPickerTrackerMachine : public QwtPickerMachine
{
public:
	QwtPickerTrackerMachine();

    virtual QList< Command > transition(const QwtEventPattern&, const QEvent*) QWT_OVERRIDE;
};

/*!
   \brief A state machine for point selections

   Pressing QwtEventPattern::MouseSelect1 or
   QwtEventPattern::KeySelect1 selects a point.

   \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
 */
class QWT_EXPORT QwtPickerClickPointMachine : public QwtPickerMachine
{
public:
	QwtPickerClickPointMachine();

    virtual QList< Command > transition(const QwtEventPattern&, const QEvent*) QWT_OVERRIDE;
};

/*!
   \brief A state machine for point selections

   Pressing QwtEventPattern::MouseSelect1 or QwtEventPattern::KeySelect1
   starts the selection, releasing QwtEventPattern::MouseSelect1 or
   a second press of QwtEventPattern::KeySelect1 terminates it.
 */
class QWT_EXPORT QwtPickerDragPointMachine : public QwtPickerMachine
{
public:
	QwtPickerDragPointMachine();

    virtual QList< Command > transition(const QwtEventPattern&, const QEvent*) QWT_OVERRIDE;
};

/*!
   \brief A state machine for rectangle selections

   Pressing QwtEventPattern::MouseSelect1 starts
   the selection, releasing it selects the first point. Pressing it
   again selects the second point and terminates the selection.
   Pressing QwtEventPattern::KeySelect1 also starts the
   selection, a second press selects the first point. A third one selects
   the second point and terminates the selection.

   \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
 */

class QWT_EXPORT QwtPickerClickRectMachine : public QwtPickerMachine
{
public:
	QwtPickerClickRectMachine();

    virtual QList< Command > transition(const QwtEventPattern&, const QEvent*) QWT_OVERRIDE;
};

/*!
   \brief A state machine for rectangle selections

   Pressing QwtEventPattern::MouseSelect1 selects
   the first point, releasing it the second point.
   Pressing QwtEventPattern::KeySelect1 also selects the
   first point, a second press selects the second point and terminates
   the selection.

   \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
 */

class QWT_EXPORT QwtPickerDragRectMachine : public QwtPickerMachine
{
public:
	QwtPickerDragRectMachine();

    virtual QList< Command > transition(const QwtEventPattern&, const QEvent*) QWT_OVERRIDE;
};

/*!
   \brief A state machine for line selections

   Pressing QwtEventPattern::MouseSelect1 selects
   the first point, releasing it the second point.
   Pressing QwtEventPattern::KeySelect1 also selects the
   first point, a second press selects the second point and terminates
   the selection.

   A common use case of QwtPickerDragLineMachine are pickers for
   distance measurements.

   \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
 */

class QWT_EXPORT QwtPickerDragLineMachine : public QwtPickerMachine
{
public:
	QwtPickerDragLineMachine();

    virtual QList< Command > transition(const QwtEventPattern&, const QEvent*) QWT_OVERRIDE;
};

/*!
   \brief A state machine for polygon selections

   Pressing QwtEventPattern::MouseSelect1 or QwtEventPattern::KeySelect1
   starts the selection and selects the first point, or appends a point.
   Pressing QwtEventPattern::MouseSelect2 or QwtEventPattern::KeySelect2
   appends the last point and terminates the selection.

   \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
 */

class QWT_EXPORT QwtPickerPolygonMachine : public QwtPickerMachine
{
public:
	QwtPickerPolygonMachine();

    virtual QList< Command > transition(const QwtEventPattern&, const QEvent*) QWT_OVERRIDE;
};

#endif

/*** End of inlined file: qwt_picker_machine.h ***/

/*** Start of inlined file: qwt_picker.h ***/
#ifndef QWT_PICKER
#define QWT_PICKER

#include <qobject.h>
#include <QPen>
#include <QFont>
#include <QPainterPath>
#include <QPoint>
#include <QPolygon>
#include <QSize>
#include <QRect>
#include <QRegion>
class QwtPickerMachine;
class QwtWidgetOverlay;
class QwtText;
class QWidget;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class QPainter;

/*!
   \brief QwtPicker provides selections on a widget

   QwtPicker filters all enter, leave, mouse and keyboard events of a widget
   and translates them into an array of selected points.

   The way how the points are collected depends on type of state machine
   that is connected to the picker. Qwt offers a couple of predefined
   state machines for selecting:

   - Nothing\n
    QwtPickerTrackerMachine
   - Single points\n
    QwtPickerClickPointMachine, QwtPickerDragPointMachine
   - Rectangles\n
    QwtPickerClickRectMachine, QwtPickerDragRectMachine
   - Polygons\n
    QwtPickerPolygonMachine

   While these state machines cover the most common ways to collect points
   it is also possible to implement individual machines as well.

   QwtPicker translates the picked points into a selection using the
   adjustedPoints() method. adjustedPoints() is intended to be reimplemented
   to fix up the selection according to application specific requirements.
   (F.e. when an application accepts rectangles of a fixed aspect ratio only.)

   Optionally QwtPicker support the process of collecting points by a
   rubber band and tracker displaying a text for the current mouse
   position.

   \par Example
   \code

    QwtPicker *picker = new QwtPicker(widget);
    picker->setStateMachine(new QwtPickerDragRectMachine);
    picker->setTrackerMode(QwtPicker::ActiveOnly);
    picker->setRubberBand(QwtPicker::RectRubberBand);
   \endcode

   The state machine triggers the following commands:

   - begin()\n
    Activate/Initialize the selection.
   - append()\n
    Add a new point
   - move() \n
    Change the position of the last point.
   - remove()\n
    Remove the last point.
   - end()\n
    Terminate the selection and call accept to validate the picked points.

   The picker is active (isActive()), between begin() and end().
   In active state the rubber band is displayed, and the tracker is visible
   in case of trackerMode is ActiveOnly or AlwaysOn.

   The cursor can be moved using the arrow keys. All selections can be aborted
   using the abort key. (QwtEventPattern::KeyPatternCode)

   \warning In case of QWidget::NoFocus the focus policy of the observed
           widget is set to QWidget::WheelFocus and mouse tracking
           will be manipulated while the picker is active,
           or if trackerMode() is AlwayOn.
 */

class QWT_EXPORT QwtPicker : public QObject, public QwtEventPattern
{
	Q_OBJECT

	Q_ENUMS(RubberBand DisplayMode ResizeMode)

	Q_PROPERTY(bool isEnabled READ isEnabled WRITE setEnabled)
	Q_PROPERTY(ResizeMode resizeMode READ resizeMode WRITE setResizeMode)

	Q_PROPERTY(DisplayMode trackerMode READ trackerMode WRITE setTrackerMode)
	Q_PROPERTY(QPen trackerPen READ trackerPen WRITE setTrackerPen)
	Q_PROPERTY(QFont trackerFont READ trackerFont WRITE setTrackerFont)

	Q_PROPERTY(RubberBand rubberBand READ rubberBand WRITE setRubberBand)
	Q_PROPERTY(QPen rubberBandPen READ rubberBandPen WRITE setRubberBandPen)

public:
	/*!
	   Rubber band style

	   The default value is QwtPicker::NoRubberBand.
	   \sa setRubberBand(), rubberBand()
	 */

	enum RubberBand
	{
		//! No rubberband.
		NoRubberBand = 0,

		//! A horizontal line ( only for QwtPickerMachine::PointSelection )
		HLineRubberBand,

		//! A vertical line ( only for QwtPickerMachine::PointSelection )
		VLineRubberBand,

		//! A crosshair ( only for QwtPickerMachine::PointSelection )
		CrossRubberBand,

		//! A rectangle ( only for QwtPickerMachine::RectSelection )
		RectRubberBand,

		//! An ellipse ( only for QwtPickerMachine::RectSelection )
		EllipseRubberBand,

		//! A polygon ( only for QwtPickerMachine::PolygonSelection )
		PolygonRubberBand,

		/*!
		   Values >= UserRubberBand can be used to define additional
		   rubber bands.
		 */
		UserRubberBand = 100
	};

	/*!
	   \brief Display mode
	   \sa setTrackerMode(), trackerMode(), isActive()
	 */
	enum DisplayMode
	{
		//! Display never
		AlwaysOff,

		//! Display always
		AlwaysOn,

		//! Display only when the selection is active
		ActiveOnly
	};

	/*!
	   Controls what to do with the selected points of an active
         selection when the observed widget is resized.

	   The default value is QwtPicker::Stretch.
	   \sa setResizeMode()
	 */

	enum ResizeMode
	{
		//! All points are scaled according to the new size,
		Stretch,

		//! All points remain unchanged.
		KeepSize
	};

	explicit QwtPicker(QWidget* parent);
	explicit QwtPicker(RubberBand rubberBand, DisplayMode trackerMode, QWidget*);

	virtual ~QwtPicker();

	void setStateMachine(QwtPickerMachine*);
	const QwtPickerMachine* stateMachine() const;
	QwtPickerMachine* stateMachine();

	void setRubberBand(RubberBand);
	RubberBand rubberBand() const;

	void setTrackerMode(DisplayMode);
	DisplayMode trackerMode() const;

	void setResizeMode(ResizeMode);
	ResizeMode resizeMode() const;

	void setRubberBandPen(const QPen&);
	QPen rubberBandPen() const;

	void setTrackerPen(const QPen&);
	QPen trackerPen() const;

	void setTrackerFont(const QFont&);
	QFont trackerFont() const;

	bool isEnabled() const;
	bool isActive() const;

	virtual bool eventFilter(QObject*, QEvent*) QWT_OVERRIDE;

	QWidget* parentWidget();
	const QWidget* parentWidget() const;

	virtual QPainterPath pickArea() const;

	virtual void drawRubberBand(QPainter*) const;
	virtual void drawTracker(QPainter*) const;

	virtual QRegion trackerMask() const;
	virtual QRegion rubberBandMask() const;

	virtual QwtText trackerText(const QPoint& pos) const;
	QPoint trackerPosition() const;
	virtual QRect trackerRect(const QFont&) const;

	QPolygon selection() const;

public Q_SLOTS:
	void setEnabled(bool);

Q_SIGNALS:
	/*!
	   A signal indicating, when the picker has been activated.
	   Together with setEnabled() it can be used to implement
	   selections with more than one picker.

	   \param on True, when the picker has been activated
	 */
	void activated(bool on);

	/*!
	   A signal emitting the selected points,
	   at the end of a selection.

	   \param polygon Selected points
	 */
	void selected(const QPolygon& polygon);

	/*!
	   A signal emitted when a point has been appended to the selection

	   \param pos Position of the appended point.
	   \sa append(). moved()
	 */
	void appended(const QPoint& pos);

	/*!
	   A signal emitted whenever the last appended point of the
	   selection has been moved.

	   \param pos Position of the moved last point of the selection.
	   \sa move(), appended()
	 */
	void moved(const QPoint& pos);

	/*!
	   A signal emitted whenever the last appended point of the
	   selection has been removed.

	   \param pos Position of the point, that has been removed
	   \sa remove(), appended()
	 */
	void removed(const QPoint& pos);
	/*!
	   A signal emitted when the active selection has been changed.
	   This might happen when the observed widget is resized.

	   \param selection Changed selection
	   \sa stretchSelection()
	 */
	void changed(const QPolygon& selection);

protected:
	virtual QPolygon adjustedPoints(const QPolygon&) const;

	virtual void transition(const QEvent*);

	virtual void begin();
	virtual void append(const QPoint&);
	virtual void move(const QPoint&);
	virtual void remove();
	virtual bool end(bool ok = true);

	virtual bool accept(QPolygon&) const;
	virtual void reset();

	virtual void widgetMousePressEvent(QMouseEvent*);
	virtual void widgetMouseReleaseEvent(QMouseEvent*);
	virtual void widgetMouseDoubleClickEvent(QMouseEvent*);
	virtual void widgetMouseMoveEvent(QMouseEvent*);
	virtual void widgetWheelEvent(QWheelEvent*);
	virtual void widgetKeyPressEvent(QKeyEvent*);
	virtual void widgetKeyReleaseEvent(QKeyEvent*);
	virtual void widgetEnterEvent(QEvent*);
	virtual void widgetLeaveEvent(QEvent*);

	virtual void stretchSelection(const QSize& oldSize, const QSize& newSize);

	virtual void updateDisplay();

	const QwtWidgetOverlay* rubberBandOverlay() const;
	const QwtWidgetOverlay* trackerOverlay() const;

	const QPolygon& pickedPoints() const;

private:
	void init(QWidget*, RubberBand rubberBand, DisplayMode trackerMode);

	void setMouseTracking(bool);

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_picker.h ***/

/*** Start of inlined file: qwt_panner.h ***/
#ifndef QWT_PANNER_H
#define QWT_PANNER_H

#include <qwidget.h>

class QCursor;
class QPixmap;

/*!
   \brief QwtPanner provides panning of a widget

   QwtPanner grabs the contents of a widget, that can be dragged
   in all directions. The offset between the start and the end position
   is emitted by the panned signal.

   QwtPanner grabs the content of the widget into a pixmap and moves
   the pixmap around, without initiating any repaint events for the widget.
   Areas, that are not part of content are not painted  while panning.
   This makes panning fast enough for widgets, where
   repaints are too slow for mouse movements.

   For widgets, where repaints are very fast it might be better to
   implement panning manually by mapping mouse events into paint events.
 */
class QWT_EXPORT QwtPanner : public QWidget
{
	Q_OBJECT

public:
    explicit QwtPanner(QWidget* parent);
	virtual ~QwtPanner();

    void setEnabled(bool);
	bool isEnabled() const;

    void setMouseButton(Qt::MouseButton, Qt::KeyboardModifiers = Qt::NoModifier);
    void getMouseButton(Qt::MouseButton& button, Qt::KeyboardModifiers&) const;

    void setAbortKey(int key, Qt::KeyboardModifiers = Qt::NoModifier);
    void getAbortKey(int& key, Qt::KeyboardModifiers&) const;

    void setCursor(const QCursor&);
	const QCursor cursor() const;

    void setOrientations(Qt::Orientations);
	Qt::Orientations orientations() const;

    bool isOrientationEnabled(Qt::Orientation) const;

    virtual bool eventFilter(QObject*, QEvent*) QWT_OVERRIDE;

Q_SIGNALS:
	/*!
	   Signal emitted, when panning is done

	   \param dx Offset in horizontal direction
	   \param dy Offset in vertical direction
	 */
    void panned(int dx, int dy);

	/*!
	   Signal emitted, while the widget moved, but panning
	   is not finished.

	   \param dx Offset in horizontal direction
	   \param dy Offset in vertical direction
	 */
    void moved(int dx, int dy);

protected:
    virtual void widgetMousePressEvent(QMouseEvent*);
    virtual void widgetMouseReleaseEvent(QMouseEvent*);
    virtual void widgetMouseMoveEvent(QMouseEvent*);
    virtual void widgetKeyPressEvent(QKeyEvent*);
    virtual void widgetKeyReleaseEvent(QKeyEvent*);

    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;

	virtual QBitmap contentsMask() const;
	virtual QPixmap grab() const;

private:
#ifndef QT_NO_CURSOR
    void showCursor(bool);
#endif

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_panner.h ***/

/*** Start of inlined file: qwt_plot_renderer.h ***/
#ifndef QWT_PLOT_RENDERER_H
#define QWT_PLOT_RENDERER_H

#include <qobject.h>
#include <qsize.h>

class QwtPlot;
class QwtScaleMap;
class QRectF;
class QPainter;
class QPaintDevice;

#ifndef QT_NO_PRINTER
class QPrinter;
#endif

#ifndef QWT_NO_SVG
#ifdef QT_SVG_LIB
class QSvgGenerator;
#endif
#endif

/*!
    \brief Renderer for exporting a plot to a document, a printer
           or anything else, that is supported by QPainter/QPaintDevice
 */
class QWT_EXPORT QwtPlotRenderer : public QObject
{
	Q_OBJECT

public:
	//! Discard flags
	enum DiscardFlag
	{
		//! Render all components of the plot
        DiscardNone = 0x00,

		//! Don't render the background of the plot
        DiscardBackground = 0x01,

		//! Don't render the title of the plot
        DiscardTitle = 0x02,

		//! Don't render the legend of the plot
        DiscardLegend = 0x04,

		//! Don't render the background of the canvas
		DiscardCanvasBackground = 0x08,

		//! Don't render the footer of the plot
        DiscardFooter = 0x10,

		/*!
            Don't render the frame of the canvas

            \note This flag has no effect when using
                  style sheets, where the frame is part
                  of the background
		 */
        DiscardCanvasFrame = 0x20

	};

    Q_DECLARE_FLAGS(DiscardFlags, DiscardFlag)

	/*!
	   \brief Layout flags
	   \sa setLayoutFlag(), testLayoutFlag()
	 */
	enum LayoutFlag
	{
		//! Use the default layout as on screen
        DefaultLayout = 0x00,

		/*!
		   Instead of the scales a box is painted around the plot canvas,
		   where the scale ticks are aligned to.
		 */
		FrameWithScales = 0x01
	};

    Q_DECLARE_FLAGS(LayoutFlags, LayoutFlag)

    explicit QwtPlotRenderer(QObject* = NULL);
	virtual ~QwtPlotRenderer();

    void setDiscardFlag(DiscardFlag flag, bool on = true);
    bool testDiscardFlag(DiscardFlag flag) const;

    void setDiscardFlags(DiscardFlags flags);
	DiscardFlags discardFlags() const;

    void setLayoutFlag(LayoutFlag flag, bool on = true);
    bool testLayoutFlag(LayoutFlag flag) const;

    void setLayoutFlags(LayoutFlags flags);
	LayoutFlags layoutFlags() const;

    void renderDocument(QwtPlot*, const QString& fileName, const QSizeF& sizeMM, int resolution = 85);

    void renderDocument(QwtPlot*, const QString& fileName, const QString& format, const QSizeF& sizeMM, int resolution = 85);

#ifndef QWT_NO_SVG
#ifdef QT_SVG_LIB
    void renderTo(QwtPlot*, QSvgGenerator&) const;
#endif
#endif

#ifndef QT_NO_PRINTER
    void renderTo(QwtPlot*, QPrinter&) const;
#endif

    void renderTo(QwtPlot*, QPaintDevice&) const;

    virtual void render(QwtPlot*, QPainter*, const QRectF& plotRect) const;

    virtual void renderTitle(const QwtPlot*, QPainter*, const QRectF& titleRect) const;

    virtual void renderFooter(const QwtPlot*, QPainter*, const QRectF& footerRect) const;

    virtual void renderScale(const QwtPlot*,
                             QPainter*,
                             QwtAxisId,
                             int startDist,
                             int endDist,
                             int baseDist,
                             const QRectF& scaleRect) const;

    virtual void renderCanvas(const QwtPlot*, QPainter*, const QRectF& canvasRect, const QwtScaleMap* maps) const;

    virtual void renderLegend(const QwtPlot*, QPainter*, const QRectF& legendRect) const;

    bool exportTo(QwtPlot*, const QString& documentName, const QSizeF& sizeMM = QSizeF(300, 200), int resolution = 85);

private:
    void buildCanvasMaps(const QwtPlot*, const QRectF&, QwtScaleMap maps[]) const;

    bool updateCanvasMargins(QwtPlot*, const QRectF&, const QwtScaleMap maps[]) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotRenderer::DiscardFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotRenderer::LayoutFlags)

#endif

/*** End of inlined file: qwt_plot_renderer.h ***/

/*** Start of inlined file: qwt_polar_renderer.h ***/
#ifndef QWT_POLAR_RENDERER_H
#define QWT_POLAR_RENDERER_H

#include <qobject.h>
#include <qsize.h>

class QwtPolarPlot;
class QRectF;
class QPainter;
class QPaintDevice;

#ifndef QT_NO_PRINTER
class QPrinter;
#endif

#ifndef QWT_NO_SVG
#ifdef QT_SVG_LIB
class QSvgGenerator;
#endif
#endif

/*!
   \brief Renderer for exporting a polar plot to a document, a printer
         or anything else, that is supported by QPainter/QPaintDevice
 */
class QWT_EXPORT QwtPolarRenderer : public QObject
{
	Q_OBJECT

public:
    explicit QwtPolarRenderer(QObject* parent = NULL);
	virtual ~QwtPolarRenderer();

    void renderDocument(QwtPolarPlot*, const QString& format, const QSizeF& sizeMM, int resolution = 85);

    void renderDocument(QwtPolarPlot*, const QString& title, const QString& format, const QSizeF& sizeMM, int resolution = 85);

#ifndef QWT_NO_SVG
#ifdef QT_SVG_LIB
    void renderTo(QwtPolarPlot*, QSvgGenerator&) const;
#endif
#endif

#ifndef QT_NO_PRINTER
    void renderTo(QwtPolarPlot*, QPrinter&) const;
#endif

    void renderTo(QwtPolarPlot*, QPaintDevice&) const;

    virtual void render(QwtPolarPlot*, QPainter*, const QRectF& rect) const;

    bool exportTo(QwtPolarPlot*, const QString& documentName, const QSizeF& sizeMM = QSizeF(200, 200), int resolution = 85);

    virtual void renderTitle(QPainter*, const QRectF&) const;

    virtual void renderLegend(const QwtPolarPlot*, QPainter*, const QRectF&) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_polar_renderer.h ***/

// plot items

/*** Start of inlined file: qwt_plot_abstract_canvas.h ***/
#ifndef QWT_PLOT_ABSTRACT_CANVAS_H
#define QWT_PLOT_ABSTRACT_CANVAS_H

#include <qframe.h>

class QwtPlot;

/*!
   \brief Base class for all type of plot canvases
 */
class QWT_EXPORT QwtPlotAbstractCanvas
{
public:
	/*!
	   \brief Focus indicator
	   The default setting is NoFocusIndicator
	   \sa setFocusIndicator(), focusIndicator(), drawFocusIndicator()
	 */

	enum FocusIndicator
	{
		//! Don't paint a focus indicator
		NoFocusIndicator,

		/*!
		   The focus is related to the complete canvas.
		   Paint the focus indicator using drawFocusIndicator()
		 */
		CanvasFocusIndicator,

		/*!
		   The focus is related to an item (curve, point, ...) on
		   the canvas. It is up to the application to display a
		   focus indication using f.e. highlighting.
		 */
		ItemFocusIndicator
	};

    explicit QwtPlotAbstractCanvas(QWidget* canvasWidget);
	virtual ~QwtPlotAbstractCanvas();

	QwtPlot* plot();
	const QwtPlot* plot() const;

    void setFocusIndicator(FocusIndicator);
	FocusIndicator focusIndicator() const;

    void setBorderRadius(double);
	double borderRadius() const;

protected:
	QWidget* canvasWidget();
	const QWidget* canvasWidget() const;

    virtual void drawFocusIndicator(QPainter*);
    virtual void drawBorder(QPainter*);
    virtual void drawBackground(QPainter*);

    void fillBackground(QPainter*);
    void drawCanvas(QPainter*);
    void drawStyled(QPainter*, bool);
    void drawUnstyled(QPainter*);

    QPainterPath canvasBorderPath(const QRect& rect) const;
	void updateStyleSheetInfo();

private:
	Q_DISABLE_COPY(QwtPlotAbstractCanvas)

	class PrivateData;
	PrivateData* m_data;
};

/*!
   \brief Base class of QwtPlotOpenGLCanvas and QwtPlotGLCanvas
 */
class QWT_EXPORT QwtPlotAbstractGLCanvas : public QwtPlotAbstractCanvas
{
public:
	/*!
	   \brief Paint attributes

	   The default setting enables BackingStore and Opaque.

	   \sa setPaintAttribute(), testPaintAttribute()
	 */
	enum PaintAttribute
	{
		/*!
		   \brief Paint double buffered reusing the content
                 of the pixmap buffer when possible.

		   Using a backing store might improve the performance
		   significantly, when working with widget overlays ( like rubber bands ).
		   Disabling the cache might improve the performance for
		   incremental paints (using QwtPlotDirectPainter ).

		   \sa backingStore(), invalidateBackingStore()
		 */
		BackingStore = 1,

		/*!
		   When ImmediatePaint is set replot() calls repaint()
		   instead of update().

		   \sa replot(), QWidget::repaint(), QWidget::update()
		 */
		ImmediatePaint = 8,
	};

	//! Paint attributes
    Q_DECLARE_FLAGS(PaintAttributes, PaintAttribute)

    explicit QwtPlotAbstractGLCanvas(QWidget* canvasWidget);
	virtual ~QwtPlotAbstractGLCanvas();

    void setPaintAttribute(PaintAttribute, bool on = true);
    bool testPaintAttribute(PaintAttribute) const;

    void setFrameStyle(int style);
	int frameStyle() const;

    void setFrameShadow(QFrame::Shadow);
	QFrame::Shadow frameShadow() const;

    void setFrameShape(QFrame::Shape);
	QFrame::Shape frameShape() const;

    void setLineWidth(int);
	int lineWidth() const;

    void setMidLineWidth(int);
	int midLineWidth() const;

	int frameWidth() const;
	QRect frameRect() const;

	//! Invalidate the internal backing store
	virtual void invalidateBackingStore() = 0;

protected:
	void replot();
    void draw(QPainter*);

private:
	virtual void clearBackingStore() = 0;

	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotAbstractGLCanvas::PaintAttributes)

#endif

/*** End of inlined file: qwt_plot_abstract_canvas.h ***/

/*** Start of inlined file: qwt_plot_canvas.h ***/
#ifndef QWT_PLOT_CANVAS_H
#define QWT_PLOT_CANVAS_H

#include <qframe.h>

class QwtPlot;
class QPixmap;
class QPainterPath;

/*!
   \brief Canvas of a QwtPlot.

   Canvas is the widget where all plot items are displayed

   \sa QwtPlot::setCanvas(), QwtPlotGLCanvas, QwtPlotOpenGLCanvas
 */
class QWT_EXPORT QwtPlotCanvas : public QFrame, public QwtPlotAbstractCanvas
{
	Q_OBJECT

    Q_PROPERTY(double borderRadius READ borderRadius WRITE setBorderRadius)

public:
	/*!
	   \brief Paint attributes

	   The default setting enables BackingStore and Opaque.

	   \sa setPaintAttribute(), testPaintAttribute()
	 */
	enum PaintAttribute
	{
		/*!
		   \brief Paint double buffered reusing the content
                 of the pixmap buffer when possible.

		   Using a backing store might improve the performance
		   significantly, when working with widget overlays ( like rubber bands ).
		   Disabling the cache might improve the performance for
		   incremental paints (using QwtPlotDirectPainter ).

		   \sa backingStore(), invalidateBackingStore()
		 */
		BackingStore = 1,

		/*!
		   \brief Try to fill the complete contents rectangle
                 of the plot canvas

		   When using styled backgrounds Qt assumes, that the
		   canvas doesn't fill its area completely
		   ( f.e because of rounded borders ) and fills the area
		   below the canvas. When this is done with gradients it might
		   result in a serious performance bottleneck - depending on the size.

		   When the Opaque attribute is enabled the canvas tries to
		   identify the gaps with some heuristics and to fill those only.

		   \warning Will not work for semitransparent backgrounds
		 */
        Opaque = 2,

		/*!
		   \brief Try to improve painting of styled backgrounds

		   QwtPlotCanvas supports the box model attributes for
		   customizing the layout with style sheets. Unfortunately
		   the design of Qt style sheets has no concept how to
		   handle backgrounds with rounded corners - beside of padding.

		   When HackStyledBackground is enabled the plot canvas tries
		   to separate the background from the background border
		   by reverse engineering to paint the background before and
		   the border after the plot items. In this order the border
		   gets perfectly antialiased and you can avoid some pixel
		   artifacts in the corners.
		 */
		HackStyledBackground = 4,

		/*!
		   When ImmediatePaint is set replot() calls repaint()
		   instead of update().

		   \sa replot(), QWidget::repaint(), QWidget::update()
		 */
		ImmediatePaint = 8
	};

    Q_DECLARE_FLAGS(PaintAttributes, PaintAttribute)

    explicit QwtPlotCanvas(QwtPlot* = NULL);
	virtual ~QwtPlotCanvas();

    void setPaintAttribute(PaintAttribute, bool on = true);
    bool testPaintAttribute(PaintAttribute) const;

	const QPixmap* backingStore() const;
	Q_INVOKABLE void invalidateBackingStore();

    virtual bool event(QEvent*) QWT_OVERRIDE;

    Q_INVOKABLE QPainterPath borderPath(const QRect&) const;

public Q_SLOTS:
	void replot();

protected:
    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;
    virtual void resizeEvent(QResizeEvent*) QWT_OVERRIDE;

    virtual void drawBorder(QPainter*) QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotCanvas::PaintAttributes)

#endif

/*** End of inlined file: qwt_plot_canvas.h ***/

/*** Start of inlined file: qwt_plot_glcanvas.h ***/
#ifndef QWT_PLOT_GLCANVAS_H
#define QWT_PLOT_GLCANVAS_H

#include <qgl.h>

class QwtPlot;

/*!
   \brief An alternative canvas for a QwtPlot derived from QGLWidget

   QwtPlotGLCanvas implements the very basics to act as canvas
   inside of a QwtPlot widget. It might be extended to a full
   featured alternative to QwtPlotCanvas in a future version of Qwt.

   Even if QwtPlotGLCanvas is not derived from QFrame it imitates
   its API. When using style sheets it supports the box model - beside
   backgrounds with rounded borders.

   Since Qt 5.4 QOpenGLWidget is available, that is used by QwtPlotOpenGLCanvas.

   \sa QwtPlot::setCanvas(), QwtPlotCanvas, QwtPlotOpenGLCanvas

   \note With Qt4 you might want to use the QPaintEngine::OpenGL paint engine
        ( see QGL::setPreferredPaintEngine() ). On a Linux test system
        QPaintEngine::OpenGL2 shows very basic problems like translated
        geometries.

   \note Another way for getting hardware accelerated graphics is using
        an OpenGL offscreen buffer ( QwtPlotCanvas::OpenGLBuffer ) with QwtPlotCanvas.
        Performance is worse, than rendering straight to a QGLWidget, but is usually
        better integrated into a desktop application.
 */
class QWT_EXPORT QwtPlotGLCanvas : public QGLWidget, public QwtPlotAbstractGLCanvas
{
	Q_OBJECT

    Q_PROPERTY(QFrame::Shadow frameShadow READ frameShadow WRITE setFrameShadow)
    Q_PROPERTY(QFrame::Shape frameShape READ frameShape WRITE setFrameShape)
    Q_PROPERTY(int lineWidth READ lineWidth WRITE setLineWidth)
    Q_PROPERTY(int midLineWidth READ midLineWidth WRITE setMidLineWidth)
    Q_PROPERTY(int frameWidth READ frameWidth)
    Q_PROPERTY(QRect frameRect READ frameRect DESIGNABLE false)

    Q_PROPERTY(double borderRadius READ borderRadius WRITE setBorderRadius)

public:
    explicit QwtPlotGLCanvas(QwtPlot* = NULL);
    explicit QwtPlotGLCanvas(const QGLFormat&, QwtPlot* = NULL);
	virtual ~QwtPlotGLCanvas();

	Q_INVOKABLE virtual void invalidateBackingStore() QWT_OVERRIDE;
    Q_INVOKABLE QPainterPath borderPath(const QRect&) const;

    virtual bool event(QEvent*) QWT_OVERRIDE;

public Q_SLOTS:
	void replot();

protected:
    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;

	virtual void initializeGL() QWT_OVERRIDE;
	virtual void paintGL() QWT_OVERRIDE;
    virtual void resizeGL(int width, int height) QWT_OVERRIDE;

private:
	void init();
	virtual void clearBackingStore() QWT_OVERRIDE;

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_glcanvas.h ***/

/*** Start of inlined file: qwt_plot_opengl_canvas.h ***/
#ifndef QWT_PLOT_OPENGL_CANVAS_H
#define QWT_PLOT_OPENGL_CANVAS_H

#include <QOpenGLWidget>
#include <QSurfaceFormat>

class QwtPlot;

/*!
   \brief An alternative canvas for a QwtPlot derived from QOpenGLWidget

   Even if QwtPlotOpenGLCanvas is not derived from QFrame it imitates
   its API. When using style sheets it supports the box model - beside
   backgrounds with rounded borders.

   \sa QwtPlot::setCanvas(), QwtPlotCanvas, QwtPlotCanvas::OpenGLBuffer

   \note Another way for getting hardware accelerated graphics is using
        an OpenGL offscreen buffer ( QwtPlotCanvas::OpenGLBuffer ) with QwtPlotCanvas.
        Performance is worse, than rendering straight to a QOpenGLWidget, but is usually
        better integrated into a desktop application.
 */
class QWT_EXPORT QwtPlotOpenGLCanvas : public QOpenGLWidget, public QwtPlotAbstractGLCanvas
{
	Q_OBJECT

    Q_PROPERTY(QFrame::Shadow frameShadow READ frameShadow WRITE setFrameShadow)
    Q_PROPERTY(QFrame::Shape frameShape READ frameShape WRITE setFrameShape)
    Q_PROPERTY(int lineWidth READ lineWidth WRITE setLineWidth)
    Q_PROPERTY(int midLineWidth READ midLineWidth WRITE setMidLineWidth)
    Q_PROPERTY(int frameWidth READ frameWidth)
    Q_PROPERTY(QRect frameRect READ frameRect DESIGNABLE false)

    Q_PROPERTY(double borderRadius READ borderRadius WRITE setBorderRadius)

public:
    explicit QwtPlotOpenGLCanvas(QwtPlot* = NULL);
    explicit QwtPlotOpenGLCanvas(const QSurfaceFormat&, QwtPlot* = NULL);
	virtual ~QwtPlotOpenGLCanvas();

	Q_INVOKABLE virtual void invalidateBackingStore() QWT_OVERRIDE;
    Q_INVOKABLE QPainterPath borderPath(const QRect&) const;

    virtual bool event(QEvent*) QWT_OVERRIDE;

public Q_SLOTS:
	void replot();

protected:
    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;

	virtual void initializeGL() QWT_OVERRIDE;
	virtual void paintGL() QWT_OVERRIDE;
    virtual void resizeGL(int width, int height) QWT_OVERRIDE;

private:
    void init(const QSurfaceFormat&);
	virtual void clearBackingStore() QWT_OVERRIDE;

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_opengl_canvas.h ***/

/*** Start of inlined file: qwt_plot_item.h ***/
#ifndef QWT_PLOT_ITEM_H
#define QWT_PLOT_ITEM_H

#include <qmetatype.h>

class QwtScaleMap;
class QwtScaleDiv;
class QwtPlot;
class QwtText;
class QwtGraphic;
class QwtLegendData;
class QRectF;
class QPainter;
class QString;
template< typename T >
class QList;

/*!
   \brief Base class for items on the plot canvas

   A plot item is "something", that can be painted on the plot canvas,
   or only affects the scales of the plot widget. They can be categorized as:

   - Representator\n
    A "Representator" is an item that represents some sort of data
    on the plot canvas. The different representator classes are organized
    according to the characteristics of the data:
    - QwtPlotMarker
      Represents a point or a horizontal/vertical coordinate
    - QwtPlotCurve
      Represents a series of points
    - QwtPlotSpectrogram ( QwtPlotRasterItem )
      Represents raster data
    - ...

   - Decorators\n
    A "Decorator" is an item, that displays additional information, that
    is not related to any data:
    - QwtPlotGrid
    - QwtPlotScaleItem
    - QwtPlotSvgItem
    - ...

   Depending on the QwtPlotItem::ItemAttribute flags, an item is included
   into autoscaling or has an entry on the legend.

   Before misusing the existing item classes it might be better to
   implement a new type of plot item
   ( don't implement a watermark as spectrogram ).
   Deriving a new type of QwtPlotItem primarily means to implement
   the YourPlotItem::draw() method.

   \sa The cpuplot example shows the implementation of additional plot items.
 */

class QWT_EXPORT QwtPlotItem
{
public:
	/*!
        \brief Runtime type information

        RttiValues is used to cast plot items, without
        having to enable runtime type information of the compiler.
	 */
	enum RttiValues
	{
		//! Unspecific value, that can be used, when it doesn't matter
		Rtti_PlotItem = 0,

		//! For QwtPlotGrid
		Rtti_PlotGrid,

		//! For QwtPlotScaleItem
		Rtti_PlotScale,

		//! For QwtPlotLegendItem
		Rtti_PlotLegend,

		//! For QwtPlotMarker
		Rtti_PlotMarker,

		//! For QwtPlotCurve
		Rtti_PlotCurve,

		//! For QwtPlotSpectroCurve
		Rtti_PlotSpectroCurve,

		//! For QwtPlotIntervalCurve
		Rtti_PlotIntervalCurve,

		//! For QwtPlotHistogram
		Rtti_PlotHistogram,

		//! For QwtPlotSpectrogram
		Rtti_PlotSpectrogram,

		//! For QwtPlotGraphicItem, QwtPlotSvgItem
		Rtti_PlotGraphic,

		//! For QwtPlotTradingCurve
		Rtti_PlotTradingCurve,

		//! For QwtPlotBarChart
		Rtti_PlotBarChart,

		//! For QwtPlotMultiBarChart
		Rtti_PlotMultiBarChart,

		//! For QwtPlotShapeItem
		Rtti_PlotShape,

		//! For QwtPlotTextLabel
		Rtti_PlotTextLabel,

		//! For QwtPlotZoneItem
		Rtti_PlotZone,

		//! For QwtPlotVectorField
		Rtti_PlotVectorField,

		/*!
		   Values >= Rtti_PlotUserItem are reserved for plot items
		   not implemented in the Qwt library.
		 */
		Rtti_PlotUserItem = 1000
	};

	/*!
	   \brief Plot Item Attributes

	   Various aspects of a plot widget depend on the attributes of
	   the attached plot items. If and how a single plot item
	   participates in these updates depends on its attributes.

	   \sa setItemAttribute(), testItemAttribute(), ItemInterest
	 */
	enum ItemAttribute
	{
		//! The item is represented on the legend.
		Legend = 0x01,

		/*!
		   The boundingRect() of the item is included in the
		   autoscaling calculation as long as its width or height
		   is >= 0.0.
		 */
		AutoScale = 0x02,

		/*!
		   The item needs extra space to display something outside
		   its bounding rectangle.
		   \sa getCanvasMarginHint()
		 */
		Margins = 0x04
	};

    Q_DECLARE_FLAGS(ItemAttributes, ItemAttribute)

	/*!
	   \brief Plot Item Interests

	   Plot items might depend on the situation of the corresponding
	   plot widget. By enabling an interest the plot item will be
	   notified, when the corresponding attribute of the plot widgets
	   has changed.

	   \sa setItemAttribute(), testItemAttribute(), ItemInterest
	 */
	enum ItemInterest
	{
		/*!
		   The item is interested in updates of the scales
		   \sa updateScaleDiv()
		 */
		ScaleInterest = 0x01,

		/*!
		   The item is interested in updates of the legend ( of other items )
		   This flag is intended for items, that want to implement a legend
		   for displaying entries of other plot item.

		   \note If the plot item wants to be represented on a legend
                 enable QwtPlotItem::Legend instead.

		   \sa updateLegend()
		 */
		LegendInterest = 0x02
	};

    Q_DECLARE_FLAGS(ItemInterests, ItemInterest)

	//! Render hints
	enum RenderHint
	{
		//! Enable antialiasing
		RenderAntialiased = 0x1
	};

    Q_DECLARE_FLAGS(RenderHints, RenderHint)

	explicit QwtPlotItem();
    explicit QwtPlotItem(const QString& title);
    explicit QwtPlotItem(const QwtText& title);

	virtual ~QwtPlotItem();

    void attach(QwtPlot* plot);
	void detach();

	QwtPlot* plot() const;

    void setTitle(const QString& title);
    void setTitle(const QwtText& title);
	const QwtText& title() const;

	virtual int rtti() const;

    void setItemAttribute(ItemAttribute, bool on = true);
    bool testItemAttribute(ItemAttribute) const;

    void setItemInterest(ItemInterest, bool on = true);
    bool testItemInterest(ItemInterest) const;

    void setRenderHint(RenderHint, bool on = true);
    bool testRenderHint(RenderHint) const;

    void setRenderThreadCount(uint numThreads);
	uint renderThreadCount() const;

    void setLegendIconSize(const QSize&);
	QSize legendIconSize() const;

	double z() const;
    void setZ(double z);

	void show();
	void hide();
    virtual void setVisible(bool);
    bool isVisible() const;

    void setAxes(QwtAxisId xAxis, QwtAxisId yAxis);

    void setXAxis(QwtAxisId);
	QwtAxisId xAxis() const;

    void setYAxis(QwtAxisId);
	QwtAxisId yAxis() const;

	virtual void itemChanged();
	virtual void legendChanged();

	/*!
	   \brief Draw the item

	   \param painter Painter
	   \param xMap Maps x-values into pixel coordinates.
	   \param yMap Maps y-values into pixel coordinates.
	   \param canvasRect Contents rect of the canvas in painter coordinates
	 */
    virtual void draw(QPainter* painter, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect) const = 0;

	virtual QRectF boundingRect() const;

    virtual void getCanvasMarginHint(const QwtScaleMap& xMap,
                                     const QwtScaleMap& yMap,
                                     const QRectF& canvasRect,
                                     double& left,
                                     double& top,
                                     double& right,
                                     double& bottom) const;

    virtual void updateScaleDiv(const QwtScaleDiv&, const QwtScaleDiv&);

    virtual void updateLegend(const QwtPlotItem*, const QList< QwtLegendData >&);

    QRectF scaleRect(const QwtScaleMap&, const QwtScaleMap&) const;
    QRectF paintRect(const QwtScaleMap&, const QwtScaleMap&) const;

	virtual QList< QwtLegendData > legendData() const;

    virtual QwtGraphic legendIcon(int index, const QSizeF&) const;

protected:
    QwtGraphic defaultIcon(const QBrush&, const QSizeF&) const;

private:
	Q_DISABLE_COPY(QwtPlotItem)

	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotItem::ItemAttributes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotItem::ItemInterests)
Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotItem::RenderHints)

Q_DECLARE_METATYPE(QwtPlotItem*)

#endif

/*** End of inlined file: qwt_plot_item.h ***/

/*** Start of inlined file: qwt_plot_dict.h ***/
#ifndef QWT_PLOT_DICT
#define QWT_PLOT_DICT

#include <qlist.h>

typedef QList< QwtPlotItem* > QwtPlotItemList;
typedef QList< QwtPlotItem* >::ConstIterator QwtPlotItemIterator;

/*!
   \brief A dictionary for plot items

   QwtPlotDict organizes plot items in increasing z-order.
   If autoDelete() is enabled, all attached items will be deleted
   in the destructor of the dictionary.
   QwtPlotDict can be used to get access to all QwtPlotItem items - or all
   items of a specific type -  that are currently on the plot.

   \sa QwtPlotItem::attach(), QwtPlotItem::detach(), QwtPlotItem::z()
 */
class QWT_EXPORT QwtPlotDict
{
public:
	explicit QwtPlotDict();
	virtual ~QwtPlotDict();

    void setAutoDelete(bool);
	bool autoDelete() const;

	const QwtPlotItemList& itemList() const;
    QwtPlotItemList itemList(int rtti) const;

    void detachItems(int rtti = QwtPlotItem::Rtti_PlotItem, bool autoDelete = true);

protected:
    void insertItem(QwtPlotItem*);
    void removeItem(QwtPlotItem*);

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_dict.h ***/

/*** Start of inlined file: qwt_plot_legenditem.h ***/
#ifndef QWT_PLOT_LEGEND_ITEM_H
#define QWT_PLOT_LEGEND_ITEM_H

class QFont;

/*!
   \brief A class which draws a legend inside the plot canvas

   QwtPlotLegendItem can be used to draw a inside the plot canvas.
   It can be used together with a QwtLegend or instead of it
   to have more space for the plot canvas.

   In opposite to QwtLegend the legend item is not interactive.
   To identify mouse clicks on a legend item an event filter
   needs to be installed catching mouse events ob the plot canvas.
   The geometries of the legend items are available using
   legendGeometries().

   The legend item is aligned to plot canvas according to
   its alignment() flags. It might have a background for the
   complete legend ( usually semi transparent ) or for
   each legend item.

   \note An external QwtLegend with a transparent background
        on top the plot canvas might be another option
        with a similar effect.
 */

class QWT_EXPORT QwtPlotLegendItem : public QwtPlotItem
{
public:
	/*!
	   \brief Background mode

	   Depending on the mode the complete legend or each item
	   might have an background.

	   The default setting is LegendBackground.

	   \sa setBackgroundMode(), setBackgroundBrush(), drawBackground()
	 */
	enum BackgroundMode
	{
		//! The legend has a background
		LegendBackground,

		//! Each item has a background
		ItemBackground
	};

	explicit QwtPlotLegendItem();
	virtual ~QwtPlotLegendItem();

	virtual int rtti() const QWT_OVERRIDE;

    void setAlignmentInCanvas(Qt::Alignment);
	Qt::Alignment alignmentInCanvas() const;

    void setOffsetInCanvas(Qt::Orientations, int numPixels);
    int offsetInCanvas(Qt::Orientation) const;

    void setMaxColumns(uint);
	uint maxColumns() const;

    void setMargin(int);
	int margin() const;

    void setSpacing(int);
	int spacing() const;

    void setItemMargin(int);
	int itemMargin() const;

    void setItemSpacing(int);
	int itemSpacing() const;

    void setFont(const QFont&);
	QFont font() const;

    void setBorderRadius(double);
	double borderRadius() const;

    void setBorderPen(const QPen&);
	QPen borderPen() const;

    void setBackgroundBrush(const QBrush&);
	QBrush backgroundBrush() const;

    void setBackgroundMode(BackgroundMode);
	BackgroundMode backgroundMode() const;

    void setTextPen(const QPen&);
	QPen textPen() const;

    virtual void draw(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect) const QWT_OVERRIDE;

	void clearLegend();

    virtual void updateLegend(const QwtPlotItem*, const QList< QwtLegendData >&) QWT_OVERRIDE;

    virtual QRect geometry(const QRectF& canvasRect) const;

    virtual QSize minimumSize(const QwtLegendData&) const;
    virtual int heightForWidth(const QwtLegendData&, int width) const;

	QList< const QwtPlotItem* > plotItems() const;
    QList< QRect > legendGeometries(const QwtPlotItem*) const;

protected:
    virtual void drawLegendData(QPainter*, const QwtPlotItem*, const QwtLegendData&, const QRectF&) const;

    virtual void drawBackground(QPainter*, const QRectF& rect) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_legenditem.h ***/

/*** Start of inlined file: qwt_plot_seriesitem.h ***/
#ifndef QWT_PLOT_SERIES_ITEM_H
#define QWT_PLOT_SERIES_ITEM_H

#include <qstring.h>

class QwtScaleDiv;

/*!
   \brief Base class for plot items representing a series of samples
 */
class QWT_EXPORT QwtPlotSeriesItem : public QwtPlotItem, public virtual QwtAbstractSeriesStore
{
public:
    explicit QwtPlotSeriesItem(const QString& title = QString());
    explicit QwtPlotSeriesItem(const QwtText& title);

	virtual ~QwtPlotSeriesItem();

    void setOrientation(Qt::Orientation);
	Qt::Orientation orientation() const;

    virtual void draw(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect) const QWT_OVERRIDE;

	/*!
	   Draw a subset of the samples

	   \param painter Painter
	   \param xMap Maps x-values into pixel coordinates.
	   \param yMap Maps y-values into pixel coordinates.
	   \param canvasRect Contents rectangle of the canvas
	   \param from Index of the first point to be painted
	   \param to Index of the last point to be painted. If to < 0 the
             curve will be painted to its last point.
	 */
    virtual void drawSeries(QPainter* painter,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QRectF& canvasRect,
                            int from,
                            int to) const = 0;

	virtual QRectF boundingRect() const QWT_OVERRIDE;

    virtual void updateScaleDiv(const QwtScaleDiv&, const QwtScaleDiv&) QWT_OVERRIDE;

protected:
	virtual void dataChanged() QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_seriesitem.h ***/

/*** Start of inlined file: qwt_plot_abstract_barchart.h ***/
#ifndef QWT_PLOT_ABSTRACT_BAR_CHART_H
#define QWT_PLOT_ABSTRACT_BAR_CHART_H

/*!
   \brief Abstract base class for bar chart items

   In opposite to almost all other plot items bar charts can't be
   displayed inside of their bounding rectangle and need a special
   API  how to calculate the width of the bars and how they affect
   the layout of the attached plot.
 */
class QWT_EXPORT QwtPlotAbstractBarChart : public QwtPlotSeriesItem
{
public:
	/*!
        \brief Mode how to calculate the bar width

        setLayoutPolicy(), setLayoutHint(), barWidthHint()
	 */
	enum LayoutPolicy
	{
		/*!
		   The sample width is calculated by dividing the bounding rectangle
		   by the number of samples. The layoutHint() is used as a minimum width
		   in paint device coordinates.

		   \sa boundingRectangle()
		 */
		AutoAdjustSamples,

		/*!
		   layoutHint() defines an interval in axis coordinates
		 */
		ScaleSamplesToAxes,

		/*!
		   The bar width is calculated by multiplying layoutHint()
		   with the height or width of the canvas.

		   \sa boundingRectangle()
		 */
		ScaleSampleToCanvas,

		/*!
		   layoutHint() defines a fixed width in paint device coordinates.
		 */
		FixedSampleSize
	};

    explicit QwtPlotAbstractBarChart(const QwtText& title);
	virtual ~QwtPlotAbstractBarChart();

    void setLayoutPolicy(LayoutPolicy);
	LayoutPolicy layoutPolicy() const;

    void setLayoutHint(double);
	double layoutHint() const;

    void setSpacing(int);
	int spacing() const;

    void setMargin(int);
	int margin() const;

    void setBaseline(double);
	double baseline() const;

    virtual void getCanvasMarginHint(const QwtScaleMap& xMap,
                                     const QwtScaleMap& yMap,
                                     const QRectF& canvasRect,
                                     double& left,
                                     double& top,
                                     double& right,
                                     double& bottom) const QWT_OVERRIDE;

protected:
    double sampleWidth(const QwtScaleMap& map, double canvasSize, double boundingSize, double value) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_abstract_barchart.h ***/

/*** Start of inlined file: qwt_plot_barchart.h ***/
#ifndef QWT_PLOT_BAR_CHART_H
#define QWT_PLOT_BAR_CHART_H

class QwtColumnRect;
template< typename T >
class QwtSeriesData;

/*!
   \brief QwtPlotBarChart displays a series of a values as bars.

   Each bar might be customized individually by implementing
   a specialSymbol(). Otherwise it is rendered using a default symbol.

   Depending on its orientation() the bars are displayed horizontally
   or vertically. The bars cover the interval between the baseline()
   and the value.

   By activating the LegendBarTitles mode each sample will have
   its own entry on the legend.

   The most common use case of a bar chart is to display a
   list of y coordinates, where the x coordinate is simply the index
   in the list. But for other situations ( f.e. when values are related
   to dates ) it is also possible to set x coordinates explicitly.

   \sa QwtPlotMultiBarChart, QwtPlotHistogram, QwtPlotCurve::Sticks,
      QwtPlotSeriesItem::orientation(), QwtPlotAbstractBarChart::baseline()
 */
class QWT_EXPORT QwtPlotBarChart : public QwtPlotAbstractBarChart, public QwtSeriesStore< QPointF >
{
public:
	/*!
	   \brief Legend modes.

	   The default setting is QwtPlotBarChart::LegendChartTitle.
	   \sa setLegendMode(), legendMode()
	 */
	enum LegendMode
	{
		/*!
		   One entry on the legend showing the default symbol
		   and the title() of the chart

		   \sa QwtPlotItem::title()
		 */
		LegendChartTitle,

		/*!
		   One entry for each value showing the individual symbol
		   of the corresponding bar and the bar title.

		   \sa specialSymbol(), barTitle()
		 */
		LegendBarTitles
	};

	explicit QwtPlotBarChart(const QString& title = QString());
	explicit QwtPlotBarChart(const QwtText& title);

	virtual ~QwtPlotBarChart();

	virtual int rtti() const QWT_OVERRIDE;

	void setSamples(const QVector< QPointF >&);
	void setSamples(const QVector< double >&);
	void setSamples(QwtSeriesData< QPointF >*);

	void setSymbol(QwtColumnSymbol*);
	const QwtColumnSymbol* symbol() const;

	void setPen(const QPen& p);
	QPen pen() const;

	void setBrush(const QBrush& b);
	QBrush brush() const;

	void setFrameStyle(QwtColumnSymbol::FrameStyle f);
	QwtColumnSymbol::FrameStyle frameStyle() const;

	void setLegendMode(LegendMode);
	LegendMode legendMode() const;

	virtual void drawSeries(QPainter*,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QRectF& canvasRect,
                            int from,
                            int to) const QWT_OVERRIDE;

	virtual QRectF boundingRect() const QWT_OVERRIDE;

	virtual QwtColumnSymbol* specialSymbol(int sampleIndex, const QPointF&) const;

	virtual QwtText barTitle(int sampleIndex) const;

protected:
	virtual void drawSample(QPainter* painter,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QRectF& canvasRect,
                            const QwtInterval& boundingInterval,
                            int index,
                            const QPointF& sample) const;

	virtual void drawBar(QPainter*, int sampleIndex, const QPointF& sample, const QwtColumnRect&) const;

	QwtColumnRect columnRect(const QwtScaleMap& xMap,
                             const QwtScaleMap& yMap,
                             const QRectF& canvasRect,
                             const QwtInterval& boundingInterval,
                             const QPointF& sample) const;

	QList< QwtLegendData > legendData() const QWT_OVERRIDE;
	QwtGraphic legendIcon(int index, const QSizeF&) const QWT_OVERRIDE;

private:
	void init();

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_barchart.h ***/

/*** Start of inlined file: qwt_plot_curve.h ***/
#ifndef QWT_PLOT_CURVE_H
#define QWT_PLOT_CURVE_H

#include <qstring.h>

class QwtScaleMap;
class QwtSymbol;
class QwtCurveFitter;
template< typename T >
class QwtSeriesData;
class QwtText;
class QPainter;
class QPolygonF;
class QPen;

/*!
   \brief A plot item, that represents a series of points

   A curve is the representation of a series of points in the x-y plane.
   It supports different display styles, interpolation ( f.e. spline )
   and symbols.

   \par Usage
   <dl><dt>a) Assign curve properties</dt>
   <dd>When a curve is created, it is configured to draw black solid lines
   with in QwtPlotCurve::Lines style and no symbols.
   You can change this by calling
   setPen(), setStyle() and setSymbol().</dd>
   <dt>b) Connect/Assign data.</dt>
   <dd>QwtPlotCurve gets its points using a QwtSeriesData object offering
   a bridge to the real storage of the points ( like QAbstractItemModel ).
   There are several convenience classes derived from QwtSeriesData, that also store
   the points inside ( like QStandardItemModel ). QwtPlotCurve also offers
   a couple of variations of setSamples(), that build QwtSeriesData objects from
   arrays internally.</dd>
   <dt>c) Attach the curve to a plot</dt>
   <dd>See QwtPlotItem::attach()
   </dd></dl>

   \par Example:
   see examples/bode

   \sa QwtPointSeriesData, QwtSymbol, QwtScaleMap
 */
class QWT_EXPORT QwtPlotCurve : public QwtPlotSeriesItem, public QwtSeriesStore< QPointF >
{
public:
	/*!
        Curve styles.
        \sa setStyle(), style()
	 */
	enum CurveStyle
	{
		/*!
		   Don't draw a curve. Note: This doesn't affect the symbols.
		 */
		NoCurve = -1,

		/*!
		   Connect the points with straight lines. The lines might
		   be interpolated depending on the 'Fitted' attribute. Curve
		   fitting can be configured using setCurveFitter().
		 */
		Lines,

		/*!
		   Draw vertical or horizontal sticks ( depending on the
		   orientation() ) from a baseline which is defined by setBaseline().
		 */
		Sticks,

		/*!
		   Connect the points with a step function. The step function
		   is drawn from the left to the right or vice versa,
		   depending on the QwtPlotCurve::Inverted attribute.
		 */
		Steps,

		/*!
		   Draw dots at the locations of the data points. Note:
		   This is different from a dotted line (see setPen()), and faster
		   as a curve in QwtPlotCurve::NoStyle style and a symbol
		   painting a point.
		 */
		Dots,

		/*!
		   Styles >= QwtPlotCurve::UserCurve are reserved for derived
		   classes of QwtPlotCurve that overload drawCurve() with
		   additional application specific curve types.
		 */
		UserCurve = 100
	};

	/*!
	   Attribute for drawing the curve
	   \sa setCurveAttribute(), testCurveAttribute(), curveFitter()
	 */
	enum CurveAttribute
	{
		/*!
		   For QwtPlotCurve::Steps only.
		   Draws a step function from the right to the left.
		 */
		Inverted = 0x01,

		/*!
		   Only in combination with QwtPlotCurve::Lines
		   A QwtCurveFitter tries to
		   interpolate/smooth the curve, before it is painted.

		   \note Curve fitting requires temporary memory
		   for calculating coefficients and additional points.
		   If painting in QwtPlotCurve::Fitted mode is slow it might be better
		   to fit the points, before they are passed to QwtPlotCurve.
		 */
		Fitted = 0x02
	};

	Q_DECLARE_FLAGS(CurveAttributes, CurveAttribute)

	/*!
        Attributes how to represent the curve on the legend

        \sa setLegendAttribute(), testLegendAttribute(),
            QwtPlotItem::legendData(), legendIcon()
	 */

	enum LegendAttribute
	{
		/*!
		   QwtPlotCurve tries to find a color representing the curve
		   and paints a rectangle with it.
		 */
		LegendNoAttribute = 0x00,

		/*!
		   If the style() is not QwtPlotCurve::NoCurve a line
		   is painted with the curve pen().
		 */
		LegendShowLine = 0x01,

		/*!
		   If the curve has a valid symbol it is painted.
		 */
		LegendShowSymbol = 0x02,

		/*!
		   If the curve has a brush a rectangle filled with the
		   curve brush() is painted.
		 */
		LegendShowBrush = 0x04
	};

	Q_DECLARE_FLAGS(LegendAttributes, LegendAttribute)

	/*!
        Attributes to modify the drawing algorithm.
        The default setting enables ClipPolygons | FilterPoints

        \sa setPaintAttribute(), testPaintAttribute()
	 */
	enum PaintAttribute
	{
		/*!
		   Clip polygons before painting them. In situations, where points
		   are far outside the visible area (f.e when zooming deep) this
		   might be a substantial improvement for the painting performance
		 */
		ClipPolygons = 0x01,

		/*!
		   Tries to reduce the data that has to be painted, by sorting out
		   duplicates, or paintings outside the visible area. Might have a
		   notable impact on curves with many close points.
		   Only a couple of very basic filtering algorithms are implemented.
		 */
		FilterPoints = 0x02,

		/*!
		   Minimize memory usage that is temporarily needed for the
		   translated points, before they get painted.
		   This might slow down the performance of painting
		 */
		MinimizeMemory = 0x04,

		/*!
		   Render the points to a temporary image and paint the image.
		   This is a very special optimization for Dots style, when
		   having a huge amount of points.
		   With a reasonable number of points QPainter::drawPoints()
		   will be faster.
		 */
		ImageBuffer = 0x08,

		/*!
		   More aggressive point filtering trying to filter out
		   intermediate points, accepting minor visual differences.

		   Has only an effect, when drawing the curve to a paint device
		   in integer coordinates ( f.e. all widgets on screen ) using the fact,
		   that consecutive points are often mapped to the same x or y coordinate.
		   Each chunk of samples mapped to the same coordinate can be reduced to
		   4 points ( first, min, max last ).

		   In the worst case the polygon to be rendered will be 4 times the width
		   of the plot canvas.

		   The algorithm is very fast and effective for huge datasets, and can be used
		   inside a replot cycle.

		   \note Implemented for QwtPlotCurve::Lines only
		   \note As this algo replaces many small lines by a long one
                a nasty bug of the raster paint engine ( Qt 4.8, Qt 5.1 - 5.3 )
                becomes more dominant. For these versions the bug can be
                worked around by enabling the QwtPainter::polylineSplitting() mode.
		 */
		FilterPointsAggressive = 0x10,
	};

	Q_DECLARE_FLAGS(PaintAttributes, PaintAttribute)

	explicit QwtPlotCurve(const QString& title = QString());
	explicit QwtPlotCurve(const QwtText& title);

	virtual ~QwtPlotCurve();

	virtual int rtti() const QWT_OVERRIDE;

	void setPaintAttribute(PaintAttribute, bool on = true);
	bool testPaintAttribute(PaintAttribute) const;

	void setLegendAttribute(LegendAttribute, bool on = true);
	bool testLegendAttribute(LegendAttribute) const;

	void setLegendAttributes(LegendAttributes);
	LegendAttributes legendAttributes() const;

	void setRawSamples(const double* xData, const double* yData, int size);
	void setRawSamples(const float* xData, const float* yData, int size);

	void setRawSamples(const double* yData, int size);
	void setRawSamples(const float* yData, int size);

	void setSamples(const double* xData, const double* yData, int size);
	void setSamples(const float* xData, const float* yData, int size);

	void setSamples(const double* yData, int size);
	void setSamples(const float* yData, int size);

	void setSamples(const QVector< double >& yData);
	void setSamples(const QVector< float >& yData);

	void setSamples(const QVector< double >& xData, const QVector< double >& yData);
	void setSamples(const QVector< float >& xData, const QVector< float >& yData);
	void setSamples(QVector< double >&& xData, QVector< double >&& yData);
	void setSamples(QVector< float >&& xData, QVector< float >&& yData);

	void setSamples(QVector< QPointF >&&);
	void setSamples(const QVector< QPointF >&);
	void setSamples(QwtSeriesData< QPointF >*);

	virtual int closestPoint(const QPointF& pos, double* dist = NULL) const;

	double minXValue() const;
	double maxXValue() const;
	double minYValue() const;
	double maxYValue() const;

	void setCurveAttribute(CurveAttribute, bool on = true);
	bool testCurveAttribute(CurveAttribute) const;

	void setPen(const QColor&, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine);
	void setPen(const QPen&);
	const QPen& pen() const;

	void setBrush(const QBrush&);
	const QBrush& brush() const;

	void setBaseline(double);
	double baseline() const;

	void setStyle(CurveStyle style);
	CurveStyle style() const;

	void setSymbol(QwtSymbol*);
	const QwtSymbol* symbol() const;

	void setCurveFitter(QwtCurveFitter*);
	QwtCurveFitter* curveFitter() const;

	virtual void drawSeries(QPainter*,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QRectF& canvasRect,
                            int from,
                            int to) const QWT_OVERRIDE;

	virtual QwtGraphic legendIcon(int index, const QSizeF&) const QWT_OVERRIDE;

protected:
	void init();

	virtual void drawCurve(QPainter*,
                           int style,
                           const QwtScaleMap& xMap,
                           const QwtScaleMap& yMap,
                           const QRectF& canvasRect,
                           int from,
                           int to) const;

	virtual void drawSymbols(QPainter*,
                             const QwtSymbol&,
                             const QwtScaleMap& xMap,
                             const QwtScaleMap& yMap,
                             const QRectF& canvasRect,
                             int from,
                             int to) const;

    virtual void drawLines(QPainter*,
                           const QwtScaleMap& xMap,
                           const QwtScaleMap& yMap,
                           const QRectF& canvasRect,
                           int from,
                           int to) const;

    virtual void drawSticks(QPainter*,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QRectF& canvasRect,
                            int from,
                            int to) const;

    virtual void drawDots(QPainter*,
                          const QwtScaleMap& xMap,
                          const QwtScaleMap& yMap,
                          const QRectF& canvasRect,
                          int from,
                          int to) const;

    virtual void drawSteps(QPainter*,
                           const QwtScaleMap& xMap,
                           const QwtScaleMap& yMap,
                           const QRectF& canvasRect,
                           int from,
                           int to) const;

	virtual void fillCurve(QPainter*, const QwtScaleMap&, const QwtScaleMap&, const QRectF& canvasRect, QPolygonF&) const;

	void closePolyline(QPainter*, const QwtScaleMap&, const QwtScaleMap&, QPolygonF&) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

//! boundingRect().left()
inline double QwtPlotCurve::minXValue() const
{
	return boundingRect().left();
}

//! boundingRect().right()
inline double QwtPlotCurve::maxXValue() const
{
	return boundingRect().right();
}

//! boundingRect().top()
inline double QwtPlotCurve::minYValue() const
{
	return boundingRect().top();
}

//! boundingRect().bottom()
inline double QwtPlotCurve::maxYValue() const
{
	return boundingRect().bottom();
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotCurve::PaintAttributes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotCurve::LegendAttributes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotCurve::CurveAttributes)

#endif

/*** End of inlined file: qwt_plot_curve.h ***/

/*** Start of inlined file: qwt_plot_graphicitem.h ***/
#ifndef QWT_PLOT_GRAPHIC_ITEM_H
#define QWT_PLOT_GRAPHIC_ITEM_H

#include <qstring.h>

/*!
   \brief A plot item, which displays
         a recorded sequence of QPainter commands

   QwtPlotGraphicItem renders a sequence of recorded painter commands
   into a specific plot area. Recording of painter commands can be
   done manually by QPainter or e.g. QSvgRenderer.

   \sa QwtPlotShapeItem, QwtPlotSvgItem
 */

class QWT_EXPORT QwtPlotGraphicItem : public QwtPlotItem
{
public:
    explicit QwtPlotGraphicItem(const QString& title = QString());
    explicit QwtPlotGraphicItem(const QwtText& title);

	virtual ~QwtPlotGraphicItem();

    void setGraphic(const QRectF& rect, const QwtGraphic&);
	QwtGraphic graphic() const;

	virtual QRectF boundingRect() const QWT_OVERRIDE;

    virtual void draw(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect) const QWT_OVERRIDE;

	virtual int rtti() const QWT_OVERRIDE;

private:
	void init();

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_graphicitem.h ***/

/*** Start of inlined file: qwt_plot_grid.h ***/
#ifndef QWT_PLOT_GRID_H
#define QWT_PLOT_GRID_H

class QPainter;
class QPen;
class QwtScaleMap;
class QwtScaleDiv;

/*!
   \brief A class which draws a coordinate grid

   The QwtPlotGrid class can be used to draw a coordinate grid.
   A coordinate grid consists of major and minor vertical
   and horizontal grid lines. The locations of the grid lines
   are determined by the X and Y scale divisions which can
   be assigned with setXDiv() and setYDiv().
   The draw() member draws the grid within a bounding
   rectangle.
 */

class QWT_EXPORT QwtPlotGrid : public QwtPlotItem
{
public:
	explicit QwtPlotGrid();
	virtual ~QwtPlotGrid();

	virtual int rtti() const QWT_OVERRIDE;

    void enableX(bool);
	bool xEnabled() const;

    void enableY(bool);
	bool yEnabled() const;

    void enableXMin(bool);
	bool xMinEnabled() const;

    void enableYMin(bool);
	bool yMinEnabled() const;

    void setXDiv(const QwtScaleDiv&);
	const QwtScaleDiv& xScaleDiv() const;

    void setYDiv(const QwtScaleDiv&);
	const QwtScaleDiv& yScaleDiv() const;

    void setPen(const QColor&, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine);

    void setPen(const QPen&);

    void setMajorPen(const QColor&, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine);

    void setMajorPen(const QPen&);
	const QPen& majorPen() const;

    void setMinorPen(const QColor&, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine);
    void setMinorPen(const QPen&);
	const QPen& minorPen() const;

    virtual void draw(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect) const QWT_OVERRIDE;

    virtual void updateScaleDiv(const QwtScaleDiv& xScaleDiv, const QwtScaleDiv& yScaleDiv) QWT_OVERRIDE;

private:
    void drawLines(QPainter*, const QRectF&, Qt::Orientation, const QwtScaleMap&, const QList< double >&) const;

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_grid.h ***/

/*** Start of inlined file: qwt_plot_directpainter.h ***/
#ifndef QWT_PLOT_DIRECT_PAINTER_H
#define QWT_PLOT_DIRECT_PAINTER_H

#include <qobject.h>

class QRegion;
class QwtPlotSeriesItem;

/*!
    \brief Painter object trying to paint incrementally

    Often applications want to display samples while they are
    collected. When there are too many samples complete replots
    will be expensive to be processed in a collection cycle.

    QwtPlotDirectPainter offers an API to paint
    subsets ( f.e all additions points ) without erasing/repainting
    the plot canvas.

    On certain environments it might be important to calculate a proper
    clip region before painting. F.e. for Qt Embedded only the clipped part
    of the backing store will be copied to a ( maybe unaccelerated )
    frame buffer.

    \warning Incremental painting will only help when no replot is triggered
             by another operation ( like changing scales ) and nothing needs
             to be erased.
 */
class QWT_EXPORT QwtPlotDirectPainter : public QObject
{
public:
	/*!
	   \brief Paint attributes
	   \sa setAttribute(), testAttribute(), drawSeries()
	 */
	enum Attribute
	{
		/*!
		   Initializing a QPainter is an expensive operation.
		   When AtomicPainter is set each call of drawSeries() opens/closes
		   a temporary QPainter. Otherwise QwtPlotDirectPainter tries to
		   use the same QPainter as long as possible.
		 */
		AtomicPainter = 0x01,

		/*!
		   When FullRepaint is set the plot canvas is explicitly repainted
		   after the samples have been rendered.
		 */
		FullRepaint = 0x02,

		/*!
		   When QwtPlotCanvas::BackingStore is enabled the painter
		   has to paint to the backing store and the widget. In certain
		   situations/environments it might be faster to paint to
		   the backing store only and then copy the backing store to the canvas.
		   This flag can also be useful for settings, where Qt fills the
		   the clip region with the widget background.
		 */
		CopyBackingStore = 0x04
	};

    Q_DECLARE_FLAGS(Attributes, Attribute)

    explicit QwtPlotDirectPainter(QObject* parent = NULL);
	virtual ~QwtPlotDirectPainter();

    void setAttribute(Attribute, bool on);
    bool testAttribute(Attribute) const;

    void setClipping(bool);
	bool hasClipping() const;

    void setClipRegion(const QRegion&);
	QRegion clipRegion() const;

    void drawSeries(QwtPlotSeriesItem*, int from, int to);
	void reset();

    virtual bool eventFilter(QObject*, QEvent*) QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotDirectPainter::Attributes)

#endif

/*** End of inlined file: qwt_plot_directpainter.h ***/

/*** Start of inlined file: qwt_plot_histogram.h ***/
#ifndef QWT_PLOT_HISTOGRAM_H
#define QWT_PLOT_HISTOGRAM_H

class QwtColumnSymbol;
class QwtColumnRect;
class QColor;
class QPolygonF;

#if QT_VERSION < 0x060000
template< typename T >
class QVector;
#endif

/*!
   \brief QwtPlotHistogram represents a series of samples, where an interval
         is associated with a value ( \f$y = f([x1,x2])\f$ ).

   The representation depends on the style() and an optional symbol()
   that is displayed for each interval.

   \note The term "histogram" is used in a different way in the areas of
        digital image processing and statistics. Wikipedia introduces the
        terms "image histogram" and "color histogram" to avoid confusions.
        While "image histograms" can be displayed by a QwtPlotCurve there
        is no applicable plot item for a "color histogram" yet.

   \sa QwtPlotBarChart, QwtPlotMultiBarChart
 */

class QWT_EXPORT QwtPlotHistogram : public QwtPlotSeriesItem, public QwtSeriesStore< QwtIntervalSample >
{
public:
	/*!
        Histogram styles.
        The default style is QwtPlotHistogram::Columns.

        \sa setStyle(), style(), setSymbol(), symbol(), setBaseline()
	 */
	enum HistogramStyle
	{
		/*!
		   Draw an outline around the area, that is build by all intervals
		   using the pen() and fill it with the brush(). The outline style
		   requires, that the intervals are in increasing order and
		   not overlapping.
		 */
		Outline,

		/*!
		   Draw a column for each interval. When a symbol() has been set
		   the symbol is used otherwise the column is displayed as
		   plain rectangle using pen() and brush().
		 */
		Columns,

		/*!
		   Draw a simple line using the pen() for each interval.
		 */
		Lines,

		/*!
		   Styles >= UserStyle are reserved for derived
		   classes that overload drawSeries() with
		   additional application specific ways to display a histogram.
		 */
		UserStyle = 100
	};

    explicit QwtPlotHistogram(const QString& title = QString());
    explicit QwtPlotHistogram(const QwtText& title);
	virtual ~QwtPlotHistogram();

	virtual int rtti() const QWT_OVERRIDE;

    void setPen(const QColor&, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine);

    void setPen(const QPen&);
	const QPen& pen() const;

    void setBrush(const QBrush&);
	const QBrush& brush() const;

    void setSamples(const QVector< QwtIntervalSample >&);
    void setSamples(QwtSeriesData< QwtIntervalSample >*);

    void setBaseline(double);
	double baseline() const;

    void setStyle(HistogramStyle style);
	HistogramStyle style() const;

    void setSymbol(const QwtColumnSymbol*);
	const QwtColumnSymbol* symbol() const;

    virtual void drawSeries(QPainter*,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QRectF& canvasRect,
                            int from,
                            int to) const QWT_OVERRIDE;

	virtual QRectF boundingRect() const QWT_OVERRIDE;

    virtual QwtGraphic legendIcon(int index, const QSizeF&) const QWT_OVERRIDE;

protected:
    virtual QwtColumnRect columnRect(const QwtIntervalSample&, const QwtScaleMap&, const QwtScaleMap&) const;

    virtual void drawColumn(QPainter*, const QwtColumnRect&, const QwtIntervalSample&) const;

    void drawColumns(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, int from, int to) const;

    void drawOutline(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, int from, int to) const;

    void drawLines(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, int from, int to) const;

private:
	void init();
    void flushPolygon(QPainter*, double baseLine, QPolygonF&) const;

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_histogram.h ***/

/*** Start of inlined file: qwt_plot_intervalcurve.h ***/
#ifndef QWT_PLOT_INTERVAL_CURVE_H
#define QWT_PLOT_INTERVAL_CURVE_H

class QwtIntervalSymbol;
template< typename T >
class QwtSeriesData;

/*!
   \brief QwtPlotIntervalCurve represents a series of samples, where each value
         is associated with an interval ( \f$[y1,y2] = f(x)\f$ ).

   The representation depends on the style() and an optional symbol()
   that is displayed for each interval. QwtPlotIntervalCurve might be used
   to display error bars or the area between 2 curves.
 */
class QWT_EXPORT QwtPlotIntervalCurve : public QwtPlotSeriesItem, public QwtSeriesStore< QwtIntervalSample >
{
public:
	/*!
        \brief Curve styles.
        The default setting is QwtPlotIntervalCurve::Tube.

        \sa setStyle(), style()
	 */
	enum CurveStyle
	{
		/*!
		   Don't draw a curve. Note: This doesn't affect the symbols.
		 */
		NoCurve,

		/*!
		   Build 2 curves from the upper and lower limits of the intervals
		   and draw them with the pen(). The area between the curves is
		   filled with the brush().
		 */
		Tube,

		/*!
		   Styles >= QwtPlotIntervalCurve::UserCurve are reserved for derived
		   classes that overload drawSeries() with
		   additional application specific curve types.
		 */
		UserCurve = 100
	};

	/*!
        Attributes to modify the drawing algorithm.
        \sa setPaintAttribute(), testPaintAttribute()
	 */
	enum PaintAttribute
	{
		/*!
		   Clip polygons before painting them. In situations, where points
		   are far outside the visible area (f.e when zooming deep) this
		   might be a substantial improvement for the painting performance.
		 */
		ClipPolygons = 0x01,

		//! Check if a symbol is on the plot canvas before painting it.
        ClipSymbol = 0x02
	};

    Q_DECLARE_FLAGS(PaintAttributes, PaintAttribute)

    explicit QwtPlotIntervalCurve(const QString& title = QString());
    explicit QwtPlotIntervalCurve(const QwtText& title);

	virtual ~QwtPlotIntervalCurve();

	virtual int rtti() const QWT_OVERRIDE;

    void setPaintAttribute(PaintAttribute, bool on = true);
    bool testPaintAttribute(PaintAttribute) const;

    void setSamples(const QVector< QwtIntervalSample >&);
    void setSamples(QwtSeriesData< QwtIntervalSample >*);

    void setPen(const QColor&, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine);

    void setPen(const QPen&);
	const QPen& pen() const;

    void setBrush(const QBrush&);
	const QBrush& brush() const;

    void setStyle(CurveStyle style);
	CurveStyle style() const;

    void setSymbol(const QwtIntervalSymbol*);
	const QwtIntervalSymbol* symbol() const;

    virtual void drawSeries(QPainter*,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QRectF& canvasRect,
                            int from,
                            int to) const QWT_OVERRIDE;

	virtual QRectF boundingRect() const QWT_OVERRIDE;

    virtual QwtGraphic legendIcon(int index, const QSizeF&) const QWT_OVERRIDE;

protected:
	void init();

    virtual void drawTube(QPainter*,
                          const QwtScaleMap& xMap,
                          const QwtScaleMap& yMap,
                          const QRectF& canvasRect,
                          int from,
                          int to) const;

    virtual void drawSymbols(QPainter*,
                             const QwtIntervalSymbol&,
                             const QwtScaleMap& xMap,
                             const QwtScaleMap& yMap,
                             const QRectF& canvasRect,
                             int from,
                             int to) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotIntervalCurve::PaintAttributes)

#endif

/*** End of inlined file: qwt_plot_intervalcurve.h ***/

/*** Start of inlined file: qwt_plot_magnifier.h ***/
#ifndef QWT_PLOT_MAGNIFIER_H
#define QWT_PLOT_MAGNIFIER_H

class QwtPlot;

/*!
   \brief QwtPlotMagnifier provides zooming, by magnifying in steps.

   Using QwtPlotMagnifier a plot can be zoomed in/out in steps using
   keys, the mouse wheel or moving a mouse button in vertical direction.

   Together with QwtPlotZoomer and QwtPlotPanner it is possible to implement
   individual and powerful navigation of the plot canvas.

   \sa QwtPlotZoomer, QwtPlotPanner, QwtPlot
 */
class QWT_EXPORT QwtPlotMagnifier : public QwtMagnifier
{
	Q_OBJECT

public:
    explicit QwtPlotMagnifier(QWidget*);
	virtual ~QwtPlotMagnifier();

    void setAxisEnabled(QwtAxisId, bool on);
    bool isAxisEnabled(QwtAxisId) const;

	QWidget* canvas();
	const QWidget* canvas() const;

	QwtPlot* plot();
	const QwtPlot* plot() const;

public Q_SLOTS:
    virtual void rescale(double factor) QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_magnifier.h ***/

/*** Start of inlined file: qwt_plot_marker.h ***/
#ifndef QWT_PLOT_MARKER_H
#define QWT_PLOT_MARKER_H

class QString;
class QRectF;
class QwtText;
class QwtSymbol;

/*!
   \brief A class for drawing markers

   A marker can be a horizontal line, a vertical line,
   a symbol, a label or any combination of them, which can
   be drawn around a center point inside a bounding rectangle.

   The setSymbol() member assigns a symbol to the marker.
   The symbol is drawn at the specified point.

   With setLabel(), a label can be assigned to the marker.
   The setLabelAlignment() member specifies where the label is
   drawn. All the Align*-constants in Qt::AlignmentFlags (see Qt documentation)
   are valid. The interpretation of the alignment depends on the marker's
   line style. The alignment refers to the center point of
   the marker, which means, for example, that the label would be printed
   left above the center point if the alignment was set to
   Qt::AlignLeft | Qt::AlignTop.

   \note QwtPlotTextLabel is intended to align a text label
        according to the geometry of canvas
        ( unrelated to plot coordinates )
 */

class QWT_EXPORT QwtPlotMarker : public QwtPlotItem
{
public:
	/*!
        Line styles.
        \sa setLineStyle(), lineStyle()
	 */
	enum LineStyle
	{
		//! No line
		NoLine,

		//! A horizontal line
		HLine,

		//! A vertical line
		VLine,

		//! A crosshair
		Cross
	};

	explicit QwtPlotMarker();
    explicit QwtPlotMarker(const QString& title);
    explicit QwtPlotMarker(const QwtText& title);

	virtual ~QwtPlotMarker();

	virtual int rtti() const QWT_OVERRIDE;

	double xValue() const;
	double yValue() const;
	QPointF value() const;

    void setXValue(double);
    void setYValue(double);
    void setValue(double, double);
    void setValue(const QPointF&);

    void setLineStyle(LineStyle);
	LineStyle lineStyle() const;

    void setLinePen(const QColor&, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine);
    void setLinePen(const QPen&);
	const QPen& linePen() const;

    void setSymbol(const QwtSymbol*);
	const QwtSymbol* symbol() const;

    void setLabel(const QwtText&);
	QwtText label() const;

    void setLabelAlignment(Qt::Alignment);
	Qt::Alignment labelAlignment() const;

    void setLabelOrientation(Qt::Orientation);
	Qt::Orientation labelOrientation() const;

    void setSpacing(int);
	int spacing() const;

    virtual void draw(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF&) const QWT_OVERRIDE;

	virtual QRectF boundingRect() const QWT_OVERRIDE;

    virtual QwtGraphic legendIcon(int index, const QSizeF&) const QWT_OVERRIDE;

protected:
    virtual void drawLines(QPainter*, const QRectF&, const QPointF&) const;

    virtual void drawSymbol(QPainter*, const QRectF&, const QPointF&) const;

    virtual void drawLabel(QPainter*, const QRectF&, const QPointF&) const;

private:
    class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_marker.h ***/

/*** Start of inlined file: qwt_plot_multi_barchart.h ***/
#ifndef QWT_PLOT_MULTI_BAR_CHART_H
#define QWT_PLOT_MULTI_BAR_CHART_H

class QwtColumnRect;
class QwtColumnSymbol;
template< typename T >
class QwtSeriesData;

/*!
   \brief QwtPlotMultiBarChart displays a series of a samples that consist
         each of a set of values.

   Each value is displayed as a bar, the bars of each set can be organized
   side by side or accumulated.

   Each bar of a set is rendered by a QwtColumnSymbol, that is set by setSymbol().
   The bars of different sets use the same symbols. Exceptions are possible
   by overloading specialSymbol() or overloading drawBar().

   Depending on its orientation() the bars are displayed horizontally
   or vertically. The bars cover the interval between the baseline()
   and the value.

   In opposite to most other plot items, QwtPlotMultiBarChart returns more
   than one entry for the legend - one for each symbol.

   \sa QwtPlotBarChart, QwtPlotHistogram
      QwtPlotSeriesItem::orientation(), QwtPlotAbstractBarChart::baseline()
 */
class QWT_EXPORT QwtPlotMultiBarChart : public QwtPlotAbstractBarChart, public QwtSeriesStore< QwtSetSample >
{
public:
	/*!
        \brief Chart styles.

        The default setting is QwtPlotMultiBarChart::Grouped.
        \sa setStyle(), style()
	 */
	enum ChartStyle
	{
		//! The bars of a set are displayed side by side
		Grouped,

		/*!
            The bars are displayed on top of each other accumulating
            to a single bar. All values of a set need to have the same
            sign.
		 */
		Stacked
	};

    explicit QwtPlotMultiBarChart(const QString& title = QString());
    explicit QwtPlotMultiBarChart(const QwtText& title);

	virtual ~QwtPlotMultiBarChart();

	virtual int rtti() const QWT_OVERRIDE;

    void setBarTitles(const QList< QwtText >&);
	QList< QwtText > barTitles() const;

    void setSamples(const QVector< QwtSetSample >&);
    void setSamples(const QVector< QVector< double > >&);
    void setSamples(QwtSeriesData< QwtSetSample >*);

    void setStyle(ChartStyle style);
	ChartStyle style() const;

    void setSymbol(int valueIndex, QwtColumnSymbol*);
    const QwtColumnSymbol* symbol(int valueIndex) const;

	void resetSymbolMap();

    virtual void drawSeries(QPainter*,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QRectF& canvasRect,
                            int from,
                            int to) const QWT_OVERRIDE;

	virtual QRectF boundingRect() const QWT_OVERRIDE;

	virtual QList< QwtLegendData > legendData() const QWT_OVERRIDE;

    virtual QwtGraphic legendIcon(int index, const QSizeF&) const QWT_OVERRIDE;

protected:
    QwtColumnSymbol* symbol(int valueIndex);

    virtual QwtColumnSymbol* specialSymbol(int sampleIndex, int valueIndex) const;

    virtual void drawSample(QPainter*,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QRectF& canvasRect,
                            const QwtInterval& boundingInterval,
                            int index,
                            const QwtSetSample&) const;

    virtual void drawBar(QPainter*, int sampleIndex, int valueIndex, const QwtColumnRect&) const;

    void drawStackedBars(QPainter*,
                         const QwtScaleMap& xMap,
                         const QwtScaleMap& yMap,
                         const QRectF& canvasRect,
                         int index,
                         double sampleWidth,
                         const QwtSetSample&) const;

    void drawGroupedBars(QPainter*,
                         const QwtScaleMap& xMap,
                         const QwtScaleMap& yMap,
                         const QRectF& canvasRect,
                         int index,
                         double sampleWidth,
                         const QwtSetSample&) const;

private:
	void init();

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_multi_barchart.h ***/

/*** Start of inlined file: qwt_plot_panner.h ***/
#ifndef QWT_PLOT_PANNER_H
#define QWT_PLOT_PANNER_H

class QwtPlot;

/*!
   \brief QwtPlotPanner provides panning of a plot canvas

   QwtPlotPanner is a panner for a plot canvas, that
   adjusts the scales of the axes after dropping
   the canvas on its new position.

   Together with QwtPlotZoomer and QwtPlotMagnifier powerful ways
   of navigating on a QwtPlot widget can be implemented easily.

   \note The axes are not updated, while dragging the canvas
   \sa QwtPlotZoomer, QwtPlotMagnifier
 */
class QWT_EXPORT QwtPlotPanner : public QwtPanner
{
	Q_OBJECT

public:
    explicit QwtPlotPanner(QWidget*);
	virtual ~QwtPlotPanner();

	QWidget* canvas();
	const QWidget* canvas() const;

	QwtPlot* plot();
	const QwtPlot* plot() const;

    void setAxisEnabled(QwtAxisId axisId, bool on);
    bool isAxisEnabled(QwtAxisId) const;

public Q_SLOTS:
    virtual void moveCanvas(int dx, int dy);

protected:
	virtual QBitmap contentsMask() const QWT_OVERRIDE;
	virtual QPixmap grab() const QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_panner.h ***/

/*** Start of inlined file: qwt_plot_picker.h ***/
#ifndef QWT_PLOT_PICKER_H
#define QWT_PLOT_PICKER_H

class QwtPlot;
class QPointF;
class QRectF;

#if QT_VERSION < 0x060000
template< typename T >
class QVector;
#endif

/*!
   \brief QwtPlotPicker provides selections on a plot canvas

   QwtPlotPicker is a QwtPicker tailored for selections on
   a plot canvas. It is set to a x-Axis and y-Axis and
   translates all pixel coordinates into this coordinate system.
 */

class QWT_EXPORT QwtPlotPicker : public QwtPicker
{
	Q_OBJECT

public:
    explicit QwtPlotPicker(QWidget* canvas);
	virtual ~QwtPlotPicker();

    explicit QwtPlotPicker(QwtAxisId xAxisId, QwtAxisId yAxisId, QWidget*);

    explicit QwtPlotPicker(QwtAxisId xAxisId, QwtAxisId yAxisId, RubberBand rubberBand, DisplayMode trackerMode, QWidget*);

    virtual void setAxes(QwtAxisId xAxisId, QwtAxisId yAxisId);

	QwtAxisId xAxis() const;
	QwtAxisId yAxis() const;

	QwtPlot* plot();
	const QwtPlot* plot() const;

	QWidget* canvas();
	const QWidget* canvas() const;

Q_SIGNALS:

	/*!
	   A signal emitted in case of QwtPickerMachine::PointSelection.
	   \param pos Selected point
	 */
    void selected(const QPointF& pos);

	/*!
	   A signal emitted in case of QwtPickerMachine::RectSelection.
	   \param rect Selected rectangle
	 */
    void selected(const QRectF& rect);

	/*!
	   A signal emitting the selected points,
	   at the end of a selection.

	   \param pa Selected points
	 */
    void selected(const QVector< QPointF >& pa);

	/*!
	   A signal emitted when a point has been appended to the selection

	   \param pos Position of the appended point.
	   \sa append(). moved()
	 */
    void appended(const QPointF& pos);

	/*!
	   A signal emitted whenever the last appended point of the
	   selection has been moved.

	   \param pos Position of the moved last point of the selection.
	   \sa move(), appended()
	 */
    void moved(const QPointF& pos);

protected:
	QRectF scaleRect() const;

    QRectF invTransform(const QRect&) const;
    QRect transform(const QRectF&) const;

    QPointF invTransform(const QPoint&) const;
    QPoint transform(const QPointF&) const;

    virtual QwtText trackerText(const QPoint&) const QWT_OVERRIDE;
    virtual QwtText trackerTextF(const QPointF&) const;

    virtual void move(const QPoint&) QWT_OVERRIDE;
    virtual void append(const QPoint&) QWT_OVERRIDE;
    virtual bool end(bool ok = true) QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_picker.h ***/

/*** Start of inlined file: qwt_plot_rasteritem.h ***/
#ifndef QWT_PLOT_RASTERITEM_H
#define QWT_PLOT_RASTERITEM_H

#include <qstring.h>

class QwtInterval;

/*!
   \brief A class, which displays raster data

   Raster data is a grid of pixel values, that can be represented
   as a QImage. It is used for many types of information like
   spectrograms, cartograms, geographical maps ...

   Often a plot has several types of raster data organized in layers.
   ( f.e a geographical map, with weather statistics ).
   Using setAlpha() raster items can be stacked easily.

   QwtPlotRasterItem is only implemented for images of the following formats:
   QImage::Format_Indexed8, QImage::Format_ARGB32.

   \sa QwtPlotSpectrogram
 */

class QWT_EXPORT QwtPlotRasterItem : public QwtPlotItem
{
public:
	/*!
	   \brief Cache policy
	   The default policy is NoCache
	 */
	enum CachePolicy
	{
		/*!
		   renderImage() is called each time the item has to be repainted
		 */
		NoCache,

		/*!
		   renderImage() is called, whenever the image cache is not valid,
		   or the scales, or the size of the canvas has changed.

		   This type of cache is useful for improving the performance
		   of hide/show operations or manipulations of the alpha value.
		   All other situations are handled by the canvas backing store.
		 */
		PaintCache
	};

	/*!
        Attributes to modify the drawing algorithm.
        \sa setPaintAttribute(), testPaintAttribute()
	 */
	enum PaintAttribute
	{
		/*!
		   When the image is rendered according to the data pixels
		   ( QwtRasterData::pixelHint() ) it can be expanded to paint
		   device resolution before it is passed to QPainter.
		   The expansion algorithm rounds the pixel borders in the same
		   way as the axis ticks, what is usually better than the
		   scaling algorithm implemented in Qt.
		   Disabling this flag might make sense, to reduce the size of a
		   document/file. If this is possible for a document format
		   depends on the implementation of the specific QPaintEngine.
		 */

		PaintInDeviceResolution = 1
	};

    Q_DECLARE_FLAGS(PaintAttributes, PaintAttribute)

    explicit QwtPlotRasterItem(const QString& title = QString());
    explicit QwtPlotRasterItem(const QwtText& title);
	virtual ~QwtPlotRasterItem();

    void setPaintAttribute(PaintAttribute, bool on = true);
    bool testPaintAttribute(PaintAttribute) const;

    void setAlpha(int alpha);
	int alpha() const;

    void setCachePolicy(CachePolicy);
	CachePolicy cachePolicy() const;

	void invalidateCache();

    virtual void draw(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect) const QWT_OVERRIDE;

    virtual QRectF pixelHint(const QRectF&) const;

	virtual QwtInterval interval(Qt::Axis) const;
	virtual QRectF boundingRect() const QWT_OVERRIDE;

protected:
	/*!
	   \brief Render an image

	   An implementation of render() might iterate over all
	   pixels of imageRect. Each pixel has to be translated into
	   the corresponding position in scale coordinates using the maps.
	   This position can be used to look up a value in a implementation
	   specific way and to map it into a color.

	   \param xMap X-Scale Map
	   \param yMap Y-Scale Map
	   \param area Requested area for the image in scale coordinates
	   \param imageSize Requested size of the image

	   \return Rendered image
	 */
    virtual QImage renderImage(const QwtScaleMap& xMap,
                               const QwtScaleMap& yMap,
                               const QRectF& area,
                               const QSize& imageSize) const = 0;

    virtual QwtScaleMap imageMap(Qt::Orientation,
                                 const QwtScaleMap& map,
                                 const QRectF& area,
                                 const QSize& imageSize,
                                 double pixelSize) const;

private:
    explicit QwtPlotRasterItem(const QwtPlotRasterItem&);
    QwtPlotRasterItem& operator=(const QwtPlotRasterItem&);

	void init();

    QImage compose(const QwtScaleMap&,
                   const QwtScaleMap&,
                   const QRectF& imageArea,
                   const QRectF& paintRect,
                   const QSize& imageSize,
                   bool doCache) const;

	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotRasterItem::PaintAttributes)

#endif

/*** End of inlined file: qwt_plot_rasteritem.h ***/

/*** Start of inlined file: qwt_plot_scaleitem.h ***/
#ifndef QWT_PLOT_SCALE_ITEM_H
#define QWT_PLOT_SCALE_ITEM_H

class QPalette;

/*!
   \brief A class which draws a scale inside the plot canvas

   QwtPlotScaleItem can be used to draw an axis inside the plot canvas.
   It might by synchronized to one of the axis of the plot, but can
   also display its own ticks and labels.

   It is allowed to synchronize the scale item with a disabled axis.
   In plots with vertical and horizontal scale items, it might be
   necessary to remove ticks at the intersections, by overloading
   updateScaleDiv().

   The scale might be at a specific position (f.e 0.0) or it might be
   aligned to a canvas border.

   \par Example
    The following example shows how to replace the left axis, by a scale item
    at the x position 0.0.
    \code
      QwtPlotScaleItem *scaleItem = new QwtPlotScaleItem( QwtScaleDraw::RightScale, 0.0 );
      scaleItem->setFont( plot->axisWidget( QwtAxis::YLeft )->font() );
      scaleItem->attach(plot);

      plot->setAxisVisible( QwtAxis::YLeft, false );
    \endcode
 */

class QWT_EXPORT QwtPlotScaleItem : public QwtPlotItem
{
public:
    explicit QwtPlotScaleItem(QwtScaleDraw::Alignment = QwtScaleDraw::BottomScale, const double pos = 0.0);

	virtual ~QwtPlotScaleItem();

	virtual int rtti() const QWT_OVERRIDE;

    void setScaleDiv(const QwtScaleDiv&);
	const QwtScaleDiv& scaleDiv() const;

    void setScaleDivFromAxis(bool on);
	bool isScaleDivFromAxis() const;

    void setPalette(const QPalette&);
	QPalette palette() const;

    void setFont(const QFont&);
	QFont font() const;

    void setScaleDraw(QwtScaleDraw*);

	const QwtScaleDraw* scaleDraw() const;
	QwtScaleDraw* scaleDraw();

    void setPosition(double pos);
	double position() const;

    void setBorderDistance(int);
	int borderDistance() const;

    void setAlignment(QwtScaleDraw::Alignment);

    virtual void draw(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect) const QWT_OVERRIDE;

    virtual void updateScaleDiv(const QwtScaleDiv&, const QwtScaleDiv&) QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_scaleitem.h ***/

/*** Start of inlined file: qwt_plot_shapeitem.h ***/
#ifndef QWT_PLOT_SHAPE_ITEM_H
#define QWT_PLOT_SHAPE_ITEM_H

#include <qstring.h>

class QPainterPath;
class QPolygonF;

/*!
   \brief A plot item, which displays any graphical shape,
         that can be defined by a QPainterPath

   A QPainterPath is a shape composed from intersecting and uniting
   regions, rectangles, ellipses or irregular areas defined by lines, and curves.
   QwtPlotShapeItem displays a shape with a pen and brush.

   QwtPlotShapeItem offers a couple of optimizations like clipping or weeding.
   These algorithms need to convert the painter path into polygons that might be
   less performant for paths built from curves and ellipses.

   More complex shapes, that can't be expressed by a QPainterPath can be displayed
   using QwtPlotGraphicItem.

   \sa QwtPlotZone, QwtPlotGraphicItem
 */
class QWT_EXPORT QwtPlotShapeItem : public QwtPlotItem
{
public:
	/*!
        Attributes to modify the drawing algorithm.
        The default disables all attributes

        \sa setPaintAttribute(), testPaintAttribute()
	 */
	enum PaintAttribute
	{
		/*!
		   Clip polygons before painting them. In situations, where points
		   are far outside the visible area (f.e when zooming deep) this
		   might be a substantial improvement for the painting performance

		   But polygon clipping will convert the painter path into
		   polygons what might introduce a negative impact on the
		   performance of paths composed from curves or ellipses.
		 */
		ClipPolygons = 0x01,
	};

    Q_DECLARE_FLAGS(PaintAttributes, PaintAttribute)

	//! Mode how to display the item on the legend
	enum LegendMode
	{
		//! Display a scaled down version of the shape
		LegendShape,

		//! Display a filled rectangle
		LegendColor
	};

    explicit QwtPlotShapeItem(const QString& title = QString());
    explicit QwtPlotShapeItem(const QwtText& title);

	virtual ~QwtPlotShapeItem();

    void setPaintAttribute(PaintAttribute, bool on = true);
    bool testPaintAttribute(PaintAttribute) const;

    void setLegendMode(LegendMode);
	LegendMode legendMode() const;

    void setRect(const QRectF&);
    void setPolygon(const QPolygonF&);

    void setShape(const QPainterPath&);
	QPainterPath shape() const;

    void setPen(const QColor&, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine);
    void setPen(const QPen&);
	QPen pen() const;

    void setBrush(const QBrush&);
	QBrush brush() const;

    void setRenderTolerance(double);
	double renderTolerance() const;

	virtual QRectF boundingRect() const QWT_OVERRIDE;

    virtual void draw(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect) const QWT_OVERRIDE;

    virtual QwtGraphic legendIcon(int index, const QSizeF&) const QWT_OVERRIDE;

	virtual int rtti() const QWT_OVERRIDE;

private:
	void init();

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_shapeitem.h ***/

/*** Start of inlined file: qwt_plot_spectrocurve.h ***/
#ifndef QWT_PLOT_CURVE_3D_H
#define QWT_PLOT_CURVE_3D_H

class QwtColorMap;

/*!
    \brief Curve that displays 3D points as dots, where the z coordinate is
           mapped to a color.
 */
class QWT_EXPORT QwtPlotSpectroCurve : public QwtPlotSeriesItem, public QwtSeriesStore< QwtPoint3D >
{
public:
	//! Paint attributes
	enum PaintAttribute
	{
		//! Clip points outside the canvas rectangle
		ClipPoints = 1
	};

    Q_DECLARE_FLAGS(PaintAttributes, PaintAttribute)

    explicit QwtPlotSpectroCurve(const QString& title = QString());
    explicit QwtPlotSpectroCurve(const QwtText& title);

	virtual ~QwtPlotSpectroCurve();

	virtual int rtti() const QWT_OVERRIDE;

    void setPaintAttribute(PaintAttribute, bool on = true);
    bool testPaintAttribute(PaintAttribute) const;

    void setSamples(const QVector< QwtPoint3D >&);
    void setSamples(QwtSeriesData< QwtPoint3D >*);

    void setColorMap(QwtColorMap*);
	const QwtColorMap* colorMap() const;

    void setColorRange(const QwtInterval&);
	QwtInterval& colorRange() const;

    virtual void drawSeries(QPainter*,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QRectF& canvasRect,
                            int from,
                            int to) const QWT_OVERRIDE;

    void setPenWidth(double);
	double penWidth() const;

protected:
    virtual void drawDots(QPainter*,
                          const QwtScaleMap& xMap,
                          const QwtScaleMap& yMap,
                          const QRectF& canvasRect,
                          int from,
                          int to) const;

private:
	void init();

	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotSpectroCurve::PaintAttributes)

#endif

/*** End of inlined file: qwt_plot_spectrocurve.h ***/

/*** Start of inlined file: qwt_plot_spectrogram.h ***/
#ifndef QWT_PLOT_SPECTROGRAM_H
#define QWT_PLOT_SPECTROGRAM_H

class QwtColorMap;
template< typename T >
class QList;

/*!
   \brief A plot item, which displays a spectrogram

   A spectrogram displays 3-dimensional data, where the 3rd dimension
   ( the intensity ) is displayed using colors. The colors are calculated
   from the values using a color map.

   On multi-core systems the performance of the image composition
   can often be improved by dividing the area into tiles - each of them
   rendered in a different thread ( see QwtPlotItem::setRenderThreadCount() ).

   In ContourMode contour lines are painted for the contour levels.

   \sa QwtRasterData, QwtColorMap, QwtPlotItem::setRenderThreadCount()
 */

class QWT_EXPORT QwtPlotSpectrogram : public QwtPlotRasterItem
{
public:
	/*!
	   The display mode controls how the raster data will be represented.
	   \sa setDisplayMode(), testDisplayMode()
	 */

	enum DisplayMode
	{
		//! The values are mapped to colors using a color map.
		ImageMode = 0x01,

		//! The data is displayed using contour lines
		ContourMode = 0x02
	};

	Q_DECLARE_FLAGS(DisplayModes, DisplayMode)

	explicit QwtPlotSpectrogram(const QString& title = QString());
	virtual ~QwtPlotSpectrogram();

	void setDisplayMode(DisplayMode, bool on = true);
	bool testDisplayMode(DisplayMode) const;

	void setData(QwtRasterData* data);
	const QwtRasterData* data() const;
	QwtRasterData* data();

	void setColorMap(QwtColorMap*);
	const QwtColorMap* colorMap() const;

	void setColorTableSize(int numColors);
	int colorTableSize() const;

	virtual QwtInterval interval(Qt::Axis) const QWT_OVERRIDE;
	virtual QRectF pixelHint(const QRectF&) const QWT_OVERRIDE;

	void setDefaultContourPen(const QColor&, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine);
	void setDefaultContourPen(const QPen&);
	QPen defaultContourPen() const;

	virtual QPen contourPen(double level) const;

	void setConrecFlag(QwtRasterData::ConrecFlag, bool on);
	bool testConrecFlag(QwtRasterData::ConrecFlag) const;

	void setContourLevels(const QList< double >&);
	QList< double > contourLevels() const;

	virtual int rtti() const QWT_OVERRIDE;

	virtual void draw(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect) const QWT_OVERRIDE;

protected:
	virtual QImage renderImage(const QwtScaleMap& xMap,
                               const QwtScaleMap& yMap,
                               const QRectF& area,
                               const QSize& imageSize) const QWT_OVERRIDE;

	virtual QSize contourRasterSize(const QRectF&, const QRect&) const;

	virtual QwtRasterData::ContourLines renderContourLines(const QRectF& rect, const QSize& raster) const;

    virtual void drawContourLines(QPainter*,
                                  const QwtScaleMap& xMap,
                                  const QwtScaleMap& yMap,
                                  const QwtRasterData::ContourLines&) const;

	void renderTile(const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRect& tile, QImage*) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotSpectrogram::DisplayModes)

#endif

/*** End of inlined file: qwt_plot_spectrogram.h ***/

/*** Start of inlined file: qwt_plot_svgitem.h ***/
#ifndef QWT_PLOT_SVG_ITEM_H
#define QWT_PLOT_SVG_ITEM_H

class QByteArray;

/*!
   \brief A plot item, which displays
         data in Scalable Vector Graphics (SVG) format.

   SVG images are often used to display maps

   QwtPlotSvgItem is only a small convenience wrapper class for
   QwtPlotGraphicItem, that creates a QwtGraphic from SVG data.
 */

class QWT_EXPORT QwtPlotSvgItem : public QwtPlotGraphicItem
{
public:
    explicit QwtPlotSvgItem(const QString& title = QString());
    explicit QwtPlotSvgItem(const QwtText& title);
	virtual ~QwtPlotSvgItem();

    bool loadFile(const QRectF&, const QString& fileName);
    bool loadData(const QRectF&, const QByteArray&);
};

#endif

/*** End of inlined file: qwt_plot_svgitem.h ***/

/*** Start of inlined file: qwt_plot_textlabel.h ***/
#ifndef QWT_PLOT_TEXT_LABEL_H
#define QWT_PLOT_TEXT_LABEL_H

class QwtText;

/*!
   \brief A plot item, which displays a text label

   QwtPlotTextLabel displays a text label aligned to the plot canvas.

   In opposite to QwtPlotMarker the position of the label is unrelated to
   plot coordinates.

   As drawing a text is an expensive operation the label is cached
   in a pixmap to speed up replots.

   \par Example
    The following code shows how to add a title.
    \code
      QwtText title( "Plot Title" );
      title.setRenderFlags( Qt::AlignHCenter | Qt::AlignTop );

      QFont font;
      font.setBold( true );
      title.setFont( font );

      QwtPlotTextLabel *titleItem = new QwtPlotTextLabel();
      titleItem->setText( title );
      titleItem->attach( plot );
    \endcode

   \sa QwtPlotMarker
 */

class QWT_EXPORT QwtPlotTextLabel : public QwtPlotItem
{
public:
	QwtPlotTextLabel();
	virtual ~QwtPlotTextLabel();

	virtual int rtti() const QWT_OVERRIDE;

    void setText(const QwtText&);
	QwtText text() const;

    void setMargin(int margin);
	int margin() const;

    virtual QRectF textRect(const QRectF&, const QSizeF&) const;

protected:
    virtual void draw(QPainter*, const QwtScaleMap&, const QwtScaleMap&, const QRectF&) const QWT_OVERRIDE;

	void invalidateCache();

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_textlabel.h ***/

/*** Start of inlined file: qwt_plot_tradingcurve.h ***/
#ifndef QWT_PLOT_TRADING_CURVE_H
#define QWT_PLOT_TRADING_CURVE_H

/*!
   \brief QwtPlotTradingCurve illustrates movements in the price of a
         financial instrument over time.

   QwtPlotTradingCurve supports candlestick or bar ( OHLC ) charts
   that are used in the domain of technical analysis.

   While the length ( height or width depending on orientation() )
   of each symbol depends on the corresponding OHLC sample the size
   of the other dimension can be controlled using:

   - setSymbolExtent()
   - setSymbolMinWidth()
   - setSymbolMaxWidth()

   The extent is a size in scale coordinates, so that the symbol width
   is increasing when the plot is zoomed in. Minimum/Maximum width
   is in widget coordinates independent from the zoom level.
   When setting the minimum and maximum to the same value, the width of
   the symbol is fixed.
 */
class QWT_EXPORT QwtPlotTradingCurve : public QwtPlotSeriesItem, public QwtSeriesStore< QwtOHLCSample >
{
public:
	/*!
        \brief Symbol styles.

        The default setting is QwtPlotSeriesItem::CandleStick.
        \sa setSymbolStyle(), symbolStyle()
	 */
	enum SymbolStyle
	{
		//! Nothing is displayed
		NoSymbol = -1,

		/*!
		   A line on the chart shows the price range (the highest and lowest
		   prices) over one unit of time, e.g. one day or one hour.
		   Tick marks project from each side of the line indicating the
		   opening and closing price.
		 */
		Bar,

		/*!
		   The range between opening/closing price are displayed as
		   a filled box. The fill brush depends on the direction of the
		   price movement. The box is connected to the highest/lowest
		   values by lines.
		 */
		CandleStick,

		/*!
		   SymbolTypes >= UserSymbol are displayed by drawUserSymbol(),
		   that needs to be overloaded and implemented in derived
		   curve classes.

		   \sa drawUserSymbol()
		 */
		UserSymbol = 100
	};

	/*!
        \brief Direction of a price movement
	 */
	enum Direction
	{
		//! The closing price is higher than the opening price
		Increasing,

		//! The closing price is lower than the opening price
		Decreasing
	};

	/*!
        Attributes to modify the drawing algorithm.
        \sa setPaintAttribute(), testPaintAttribute()
	 */
	enum PaintAttribute
	{
		//! Check if a symbol is on the plot canvas before painting it.
        ClipSymbols = 0x01
	};

    Q_DECLARE_FLAGS(PaintAttributes, PaintAttribute)

    explicit QwtPlotTradingCurve(const QString& title = QString());
    explicit QwtPlotTradingCurve(const QwtText& title);

	virtual ~QwtPlotTradingCurve();

	virtual int rtti() const QWT_OVERRIDE;

    void setPaintAttribute(PaintAttribute, bool on = true);
    bool testPaintAttribute(PaintAttribute) const;

    void setSamples(const QVector< QwtOHLCSample >&);
    void setSamples(QwtSeriesData< QwtOHLCSample >*);

    void setSymbolStyle(SymbolStyle style);
	SymbolStyle symbolStyle() const;

    void setSymbolPen(const QColor&, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine);
    void setSymbolPen(const QPen&);
	QPen symbolPen() const;

    void setSymbolBrush(Direction, const QBrush&);
    QBrush symbolBrush(Direction) const;

    void setSymbolExtent(double);
	double symbolExtent() const;

    void setMinSymbolWidth(double);
	double minSymbolWidth() const;

    void setMaxSymbolWidth(double);
	double maxSymbolWidth() const;

    virtual void drawSeries(QPainter*,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QRectF& canvasRect,
                            int from,
                            int to) const QWT_OVERRIDE;

	virtual QRectF boundingRect() const QWT_OVERRIDE;

    virtual QwtGraphic legendIcon(int index, const QSizeF&) const QWT_OVERRIDE;

protected:
	void init();

    virtual void drawSymbols(QPainter*,
                             const QwtScaleMap& xMap,
                             const QwtScaleMap& yMap,
                             const QRectF& canvasRect,
                             int from,
                             int to) const;

    virtual void drawUserSymbol(QPainter*, SymbolStyle, const QwtOHLCSample&, Qt::Orientation, bool inverted, double symbolWidth) const;

    void drawBar(QPainter*, const QwtOHLCSample&, Qt::Orientation, bool inverted, double width) const;

    void drawCandleStick(QPainter*, const QwtOHLCSample&, Qt::Orientation, double width) const;

    virtual double scaledSymbolWidth(const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotTradingCurve::PaintAttributes)

#endif

/*** End of inlined file: qwt_plot_tradingcurve.h ***/

/*** Start of inlined file: qwt_plot_vectorfield.h ***/
#ifndef QWT_PLOT_VECTOR_FIELD_H
#define QWT_PLOT_VECTOR_FIELD_H

class QwtVectorFieldSymbol;
class QwtColorMap;
class QPen;
class QBrush;

/*!
    \brief A plot item, that represents a vector field

    A vector field is a representation of a points with a given magnitude and direction
    as arrows. While the direction affects the direction of the arrow, the magnitude
    might be represented as a color or by the length of the arrow.

    \sa QwtVectorFieldSymbol, QwtVectorFieldSample
 */
class QWT_EXPORT QwtPlotVectorField : public QwtPlotSeriesItem, public QwtSeriesStore< QwtVectorFieldSample >
{
public:
	/*!
        Depending on the origin the indicator symbol ( usually an arrow )
        will be to the position of the corresponding sample.
	 */
	enum IndicatorOrigin
	{
		//! symbol points to the sample position
		OriginHead,

		//! The arrow starts at the sample position
		OriginTail,

		//! The arrow is centered at the sample position
		OriginCenter
	};

	/*!
        Attributes to modify the rendering
        \sa setPaintAttribute(), testPaintAttribute()
	 */
	enum PaintAttribute
	{
		/*
            FilterVectors calculates an average sample from all samples
            that lie in the same cell of a grid that is determined by
            setting the rasterSize().

            \sa setRasterSize()
		 */
        FilterVectors = 0x01
	};

    Q_DECLARE_FLAGS(PaintAttributes, PaintAttribute)

	/*!
        Depending on the MagnitudeMode the magnitude component will have
        an impact on the attributes of the symbol/arrow.

        \sa setMagnitudeMode()
	 */
	enum MagnitudeMode
	{
		/*!
		   The magnitude will be mapped to a color using a color map
		   \sa magnitudeRange(), colorMap()
		 */
		MagnitudeAsColor = 0x01,

		/*!
		   The magnitude will have an impact on the length of the arrow/symbol
		   \sa arrowLength(), magnitudeScaleFactor()
		 */
		MagnitudeAsLength = 0x02
	};

    Q_DECLARE_FLAGS(MagnitudeModes, MagnitudeMode)

    explicit QwtPlotVectorField(const QString& title = QString());
    explicit QwtPlotVectorField(const QwtText& title);

	virtual ~QwtPlotVectorField();

    void setPaintAttribute(PaintAttribute, bool on = true);
    bool testPaintAttribute(PaintAttribute) const;

    void setMagnitudeMode(MagnitudeMode, bool on = true);
    bool testMagnitudeMode(MagnitudeMode) const;

    void setSymbol(QwtVectorFieldSymbol*);
	const QwtVectorFieldSymbol* symbol() const;

    void setPen(const QPen&);
	QPen pen() const;

    void setBrush(const QBrush&);
	QBrush brush() const;

    void setRasterSize(const QSizeF&);
	QSizeF rasterSize() const;

    void setIndicatorOrigin(IndicatorOrigin);
	IndicatorOrigin indicatorOrigin() const;

    void setSamples(const QVector< QwtVectorFieldSample >&);
    void setSamples(QwtVectorFieldData*);

    void setColorMap(QwtColorMap*);
	const QwtColorMap* colorMap() const;

    void setMagnitudeRange(const QwtInterval&);
	QwtInterval magnitudeRange() const;

    void setMinArrowLength(double);
	double minArrowLength() const;

    void setMaxArrowLength(double);
	double maxArrowLength() const;

    virtual double arrowLength(double magnitude) const;

	virtual QRectF boundingRect() const QWT_OVERRIDE;

    virtual void drawSeries(QPainter*,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QRectF& canvasRect,
                            int from,
                            int to) const QWT_OVERRIDE;

	virtual int rtti() const QWT_OVERRIDE;

    virtual QwtGraphic legendIcon(int index, const QSizeF&) const QWT_OVERRIDE;

    void setMagnitudeScaleFactor(double factor);
	double magnitudeScaleFactor() const;

protected:
    virtual void drawSymbols(QPainter*,
                             const QwtScaleMap& xMap,
                             const QwtScaleMap& yMap,
                             const QRectF& canvasRect,
                             int from,
                             int to) const;

    virtual void drawSymbol(QPainter*, double x, double y, double vx, double vy) const;

	virtual void dataChanged() QWT_OVERRIDE;

private:
	void init();

	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotVectorField::PaintAttributes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotVectorField::MagnitudeModes)

#endif

/*** End of inlined file: qwt_plot_vectorfield.h ***/

/*** Start of inlined file: qwt_plot_zoneitem.h ***/
#ifndef QWT_PLOT_ZONE_ITEM_H
#define QWT_PLOT_ZONE_ITEM_H

#include <qnamespace.h>

class QwtInterval;
class QPen;
class QBrush;

/*!
   \brief A plot item, which displays a zone

   A horizontal zone highlights an interval of the y axis - a vertical
   zone an interval of the x axis - and is unbounded in the opposite direction.
   It is filled with a brush and its border lines are optionally displayed with a pen.

   \note For displaying an area that is bounded for x and y coordinates
        use QwtPlotShapeItem
 */

class QWT_EXPORT QwtPlotZoneItem : public QwtPlotItem
{
public:
	explicit QwtPlotZoneItem();
	virtual ~QwtPlotZoneItem();

	virtual int rtti() const QWT_OVERRIDE;

    void setOrientation(Qt::Orientation);
	Qt::Orientation orientation() const;

    void setInterval(double min, double max);
    void setInterval(const QwtInterval&);
	QwtInterval interval() const;

    void setPen(const QColor&, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine);
    void setPen(const QPen&);
	const QPen& pen() const;

    void setBrush(const QBrush&);
	const QBrush& brush() const;

    virtual void draw(QPainter*, const QwtScaleMap&, const QwtScaleMap&, const QRectF& canvasRect) const QWT_OVERRIDE;

	virtual QRectF boundingRect() const QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_zoneitem.h ***/

/*** Start of inlined file: qwt_plot_zoomer.h ***/
#ifndef QWT_PLOT_ZOOMER_H
#define QWT_PLOT_ZOOMER_H

class QSizeF;
template< typename T >
class QStack;

/*!
   \brief QwtPlotZoomer provides stacked zooming for a plot widget

   QwtPlotZoomer selects rectangles from user inputs ( mouse or keyboard )
   translates them into plot coordinates and adjusts the axes to them.
   The selection is supported by a rubber band and optionally by displaying
   the coordinates of the current mouse position.

   Zooming can be repeated as often as possible, limited only by
   maxStackDepth() or minZoomSize().  Each rectangle is pushed on a stack.

   The default setting how to select rectangles is
   a QwtPickerDragRectMachine with the following bindings:

   - QwtEventPattern::MouseSelect1\n
    The first point of the zoom rectangle is selected by a mouse press,
    the second point from the position, where the mouse is released.

   - QwtEventPattern::KeySelect1\n
    The first key press selects the first, the second key press
    selects the second point.

   - QwtEventPattern::KeyAbort\n
    Discard the selection in the state, where the first point
    is selected.

   To traverse the zoom stack the following bindings are used:

   - QwtEventPattern::MouseSelect3, QwtEventPattern::KeyUndo\n
    Zoom out one position on the zoom stack

   - QwtEventPattern::MouseSelect6, QwtEventPattern::KeyRedo\n
    Zoom in one position on the zoom stack

   - QwtEventPattern::MouseSelect2, QwtEventPattern::KeyHome\n
    Zoom to the zoom base

   The setKeyPattern() and setMousePattern() functions can be used
   to configure the zoomer actions. The following example
   shows, how to configure the 'I' and 'O' keys for zooming in and out
   one position on the zoom stack. The "Home" key is used to
   "unzoom" the plot.

   \code
   zoomer = new QwtPlotZoomer( plot );
   zoomer->setKeyPattern( QwtEventPattern::KeyRedo, Qt::Key_I, Qt::ShiftModifier );
   zoomer->setKeyPattern( QwtEventPattern::KeyUndo, Qt::Key_O, Qt::ShiftModifier );
   zoomer->setKeyPattern( QwtEventPattern::KeyHome, Qt::Key_Home );
   \endcode

   QwtPlotZoomer is tailored for plots with one x and y axis, but it is
   allowed to attach a second QwtPlotZoomer ( without rubber band and tracker )
   for the other axes.

   \note The realtime example includes an derived zoomer class that adds
        scrollbars to the plot canvas.

   \sa QwtPlotPanner, QwtPlotMagnifier
 */

class QWT_EXPORT QwtPlotZoomer : public QwtPlotPicker
{
	Q_OBJECT
public:
    explicit QwtPlotZoomer(QWidget*, bool doReplot = true);
    explicit QwtPlotZoomer(QwtAxisId xAxis, QwtAxisId yAxis, QWidget*, bool doReplot = true);

	virtual ~QwtPlotZoomer();

    virtual void setZoomBase(bool doReplot = true);
    virtual void setZoomBase(const QRectF&);

	QRectF zoomBase() const;
	QRectF zoomRect() const;

    virtual void setAxes(QwtAxisId xAxis, QwtAxisId yAxis) QWT_OVERRIDE;

    void setMaxStackDepth(int);
	int maxStackDepth() const;

	const QStack< QRectF >& zoomStack() const;
    void setZoomStack(const QStack< QRectF >&, int zoomRectIndex = -1);

	uint zoomRectIndex() const;

public Q_SLOTS:
    void moveBy(double dx, double dy);
    virtual void moveTo(const QPointF&);

    virtual void zoom(const QRectF&);
    virtual void zoom(int offset);

Q_SIGNALS:
	/*!
	   A signal emitting the zoomRect(), when the plot has been
	   zoomed in or out.

	   \param rect Current zoom rectangle.
	 */

    void zoomed(const QRectF& rect);

protected:
	virtual void rescale();

	virtual QSizeF minZoomSize() const;

    virtual void widgetMouseReleaseEvent(QMouseEvent*) QWT_OVERRIDE;
    virtual void widgetKeyPressEvent(QKeyEvent*) QWT_OVERRIDE;

	virtual void begin() QWT_OVERRIDE;
    virtual bool end(bool ok = true) QWT_OVERRIDE;
    virtual bool accept(QPolygon&) const QWT_OVERRIDE;

private:
    void init(bool doReplot);

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_zoomer.h ***/

// polar items

/*** Start of inlined file: qwt_polar_canvas.h ***/
#ifndef QWT_POLAR_CANVAS_H
#define QWT_POLAR_CANVAS_H

#include <qframe.h>

class QPainter;
class QwtPolarPlot;

/*!
   \brief Canvas of a QwtPolarPlot.

   The canvas is the widget, where all polar items are painted to.

   \note In opposite to QwtPlot all axes are painted on the canvas.
   \sa QwtPolarPlot
 */
class QWT_EXPORT QwtPolarCanvas : public QFrame
{
	Q_OBJECT

public:
	/*!
	   \brief Paint attributes

	   The default setting enables BackingStore

	   \sa setPaintAttribute(), testPaintAttribute(), backingStore()
	 */

	enum PaintAttribute
	{
		/*!
		   Paint double buffered and reuse the content of the pixmap buffer
		   for some spontaneous repaints that happen when a plot gets unhidden,
		   deiconified or changes the focus.
		 */
		BackingStore = 0x01
	};

    Q_DECLARE_FLAGS(PaintAttributes, PaintAttribute)

    explicit QwtPolarCanvas(QwtPolarPlot*);
	virtual ~QwtPolarCanvas();

	QwtPolarPlot* plot();
	const QwtPolarPlot* plot() const;

    void setPaintAttribute(PaintAttribute, bool on = true);
    bool testPaintAttribute(PaintAttribute) const;

	const QPixmap* backingStore() const;
	void invalidateBackingStore();

    QwtPointPolar invTransform(const QPoint&) const;
    QPoint transform(const QwtPointPolar&) const;

protected:
    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;
    virtual void resizeEvent(QResizeEvent*) QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPolarCanvas::PaintAttributes)

#endif

/*** End of inlined file: qwt_polar_canvas.h ***/

/*** Start of inlined file: qwt_polar_item.h ***/
#ifndef QWT_POLAR_ITEM_H
#define QWT_POLAR_ITEM_H

class QString;
class QRect;
class QPointF;
class QPainter;
class QwtPolarPlot;
class QwtScaleMap;
class QwtScaleDiv;

/*!
   \brief Base class for items on a polar plot

   A QwtPolarItem is "something that can be painted on the canvas".
   It is connected to the QwtPolar framework by a couple of virtual
   methods, that are individually implemented in derived item classes.

   QwtPolar offers an implementation of the most common types of items,
   but deriving from QwtPolarItem makes it easy to implement additional
   types of items.
 */
class QWT_EXPORT QwtPolarItem
{
public:
	/*!
        \brief Runtime type information

        RttiValues is used to cast plot items, without
        having to enable runtime type information of the compiler.
	 */
	enum RttiValues
	{
		//! Unspecific value, that can be used, when it doesn't matter
		Rtti_PolarItem = 0,

		//! For QwtPolarGrid
		Rtti_PolarGrid,

		//! For QwtPolarMarker
		Rtti_PolarMarker,

		//! For QwtPolarCurve
		Rtti_PolarCurve,

		//! For QwtPolarSpectrogram
		Rtti_PolarSpectrogram,

		/*!
		   Values >= Rtti_PolarUserItem are reserved for plot items
		   not implemented in the QwtPolar library.
		 */
		Rtti_PolarUserItem = 1000
	};

	/*!
	   \brief Plot Item Attributes
	   \sa setItemAttribute(), testItemAttribute()
	 */
	enum ItemAttribute
	{
		//! The item is represented on the legend.
        Legend = 0x01,

		/*!
		   The boundingRect() of the item is included in the
		   autoscaling calculation.
		 */
		AutoScale = 0x02
	};

    Q_DECLARE_FLAGS(ItemAttributes, ItemAttribute)

	/*!
	   \brief Render hints
	   \sa setRenderHint(), testRenderHint()
	 */
	enum RenderHint
	{
		//! Enable antialiasing
		RenderAntialiased = 0x01
	};

    Q_DECLARE_FLAGS(RenderHints, RenderHint)

    explicit QwtPolarItem(const QwtText& title = QwtText());
	virtual ~QwtPolarItem();

    void attach(QwtPolarPlot* plot);
	void detach();

	QwtPolarPlot* plot() const;

    void setTitle(const QString& title);
    void setTitle(const QwtText& title);
	const QwtText& title() const;

	virtual int rtti() const;

    void setItemAttribute(ItemAttribute, bool on = true);
    bool testItemAttribute(ItemAttribute) const;

    void setRenderHint(RenderHint, bool on = true);
    bool testRenderHint(RenderHint) const;

    void setRenderThreadCount(uint numThreads);
	uint renderThreadCount() const;

	double z() const;
    void setZ(double z);

	void show();
	void hide();
    virtual void setVisible(bool);
    bool isVisible() const;

	virtual void itemChanged();
	virtual void legendChanged();

	/*!
	   \brief Draw the item

	   \param painter Painter
	   \param azimuthMap Maps azimuth values to values related to 0.0, M_2PI
	   \param radialMap Maps radius values into painter coordinates.
	   \param pole Position of the pole in painter coordinates
	   \param radius Radius of the complete plot area in painter coordinates
	   \param canvasRect Contents rect of the canvas in painter coordinates
	 */
    virtual void draw(QPainter* painter,
                      const QwtScaleMap& azimuthMap,
                      const QwtScaleMap& radialMap,
                      const QPointF& pole,
                      double radius,
                      const QRectF& canvasRect) const = 0;

    virtual QwtInterval boundingInterval(int scaleId) const;

    virtual void updateScaleDiv(const QwtScaleDiv&, const QwtScaleDiv&, const QwtInterval&);

	virtual int marginHint() const;

    void setLegendIconSize(const QSize&);
	QSize legendIconSize() const;

	virtual QList< QwtLegendData > legendData() const;
    virtual QwtGraphic legendIcon(int index, const QSizeF&) const;

private:
    Q_DISABLE_COPY(QwtPolarItem)

	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPolarItem::ItemAttributes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPolarItem::RenderHints)

Q_DECLARE_METATYPE(QwtPolarItem*)

#endif

/*** End of inlined file: qwt_polar_item.h ***/

/*** Start of inlined file: qwt_polar_itemdict.h ***/
#ifndef QWT_POLAR_ITEMDICT_H
#define QWT_POLAR_ITEMDICT_H

#include <qlist.h>

typedef QList< QwtPolarItem* >::ConstIterator QwtPolarItemIterator;
typedef QList< QwtPolarItem* > QwtPolarItemList;

/*!
   \brief A dictionary for polar plot items

   QwtPolarItemDict organizes polar plot items in increasing z-order.
   If autoDelete() is enabled, all attached items will be deleted
   in the destructor of the dictionary.

   \sa QwtPolarItem::attach(), QwtPolarItem::detach(), QwtPolarItem::z()
 */
class QWT_EXPORT QwtPolarItemDict
{
public:
	explicit QwtPolarItemDict();
	~QwtPolarItemDict();

    void setAutoDelete(bool);
	bool autoDelete() const;

	const QwtPolarItemList& itemList() const;

    void detachItems(int rtti = QwtPolarItem::Rtti_PolarItem, bool autoDelete = true);

protected:
    void insertItem(QwtPolarItem*);
    void removeItem(QwtPolarItem*);

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_polar_itemdict.h ***/

/*** Start of inlined file: qwt_polar_curve.h ***/
#ifndef QWT_POLAR_CURVE_H
#define QWT_POLAR_CURVE_H

class QPainter;
class QwtSymbol;
class QwtCurveFitter;

/*!
   \brief An item, that represents a series of points

   A curve is the representation of a series of points in polar coordinates.
   The points are connected to the curve using the abstract QwtData interface.

   \sa QwtPolarPlot, QwtSymbol, QwtScaleMap
 */

class QWT_EXPORT QwtPolarCurve : public QwtPolarItem
{
public:
	/*!
        Curve styles.
        \sa setStyle(), style()
	 */
	enum CurveStyle
	{
		//! Don't draw a curve. Note: This doesn't affect the symbols.
		NoCurve,

		/*!
		   Connect the points with straight lines. The lines might
		   be interpolated depending on the 'Fitted' attribute. Curve
		   fitting can be configured using setCurveFitter().
		 */
		Lines,

		//! Values > 100 are reserved for user specific curve styles
		UserCurve = 100
	};

	/*!
        \brief Attributes how to represent the curve on the legend

        If none of the flags is activated QwtPlotCurve tries to find
        a color representing the curve and paints a rectangle with it.
        In the default setting all attributes are off.

        \sa setLegendAttribute(), testLegendAttribute()
	 */

	enum LegendAttribute
	{
		/*!
		   If the curveStyle() is not NoCurve a line is painted with the
		   curvePen().
		 */
		LegendShowLine = 0x01,

		//! If the curve has a valid symbol it is painted.
		LegendShowSymbol = 0x02
	};

    Q_DECLARE_FLAGS(LegendAttributes, LegendAttribute)

	explicit QwtPolarCurve();
    explicit QwtPolarCurve(const QwtText& title);
    explicit QwtPolarCurve(const QString& title);

	virtual ~QwtPolarCurve();

	virtual int rtti() const QWT_OVERRIDE;

    void setLegendAttribute(LegendAttribute, bool on = true);
    bool testLegendAttribute(LegendAttribute) const;

    void setData(QwtSeriesData< QwtPointPolar >* data);
	const QwtSeriesData< QwtPointPolar >* data() const;

	size_t dataSize() const;
    QwtPointPolar sample(int i) const;

    void setPen(const QPen&);
	const QPen& pen() const;

    void setStyle(CurveStyle style);
	CurveStyle style() const;

    void setSymbol(QwtSymbol*);
	const QwtSymbol* symbol() const;

    void setCurveFitter(QwtCurveFitter*);
	QwtCurveFitter* curveFitter() const;

    virtual void draw(QPainter* p,
                      const QwtScaleMap& azimuthMap,
                      const QwtScaleMap& radialMap,
                      const QPointF& pole,
                      double radius,
                      const QRectF& canvasRect) const QWT_OVERRIDE;

    virtual void draw(QPainter* p,
                      const QwtScaleMap& azimuthMap,
                      const QwtScaleMap& radialMap,
                      const QPointF& pole,
                      int from,
                      int to) const;

    virtual QwtInterval boundingInterval(int scaleId) const QWT_OVERRIDE;

    virtual QwtGraphic legendIcon(int index, const QSizeF&) const QWT_OVERRIDE;

protected:
    void init();

    virtual void drawCurve(QPainter*,
                           int style,
                           const QwtScaleMap& azimuthMap,
                           const QwtScaleMap& radialMap,
                           const QPointF& pole,
                           int from,
                           int to) const;

    virtual void drawSymbols(QPainter*,
                             const QwtSymbol&,
                             const QwtScaleMap& azimuthMap,
                             const QwtScaleMap& radialMap,
                             const QPointF& pole,
                             int from,
                             int to) const;

    void drawLines(QPainter*, const QwtScaleMap& azimuthMap, const QwtScaleMap& radialMap, const QPointF& pole, int from, int to) const;

private:
	QwtSeriesData< QwtPointPolar >* m_series;

	class PrivateData;
	PrivateData* m_data;
};

//! \return the the curve data
inline const QwtSeriesData< QwtPointPolar >* QwtPolarCurve::data() const
{
	return m_series;
}

/*!
    \param i index
    \return point at position i
 */
inline QwtPointPolar QwtPolarCurve::sample(int i) const
{
    return m_series->sample(i);
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPolarCurve::LegendAttributes)

#endif

/*** End of inlined file: qwt_polar_curve.h ***/

/*** Start of inlined file: qwt_polar_magnifier.h ***/
#ifndef QWT_POLAR_MAGNIFIER_H
#define QWT_POLAR_MAGNIFIER_H

class QwtPolarPlot;
class QwtPolarCanvas;

/*!
   \brief QwtPolarMagnifier provides zooming, by magnifying in steps.

   Using QwtPlotMagnifier a plot can be zoomed in/out in steps using
   keys, the mouse wheel or moving a mouse button in vertical direction.

   Together with QwtPolarPanner it is possible to implement
   an individual navigation of the plot canvas.

   \sa QwtPolarPanner, QwtPolarPlot, QwtPolarCanvas
 */

class QWT_EXPORT QwtPolarMagnifier : public QwtMagnifier
{
	Q_OBJECT

public:
    explicit QwtPolarMagnifier(QwtPolarCanvas*);
	virtual ~QwtPolarMagnifier();

    void setUnzoomKey(int key, int modifiers);
    void getUnzoomKey(int& key, int& modifiers) const;

	QwtPolarPlot* plot();
	const QwtPolarPlot* plot() const;

	QwtPolarCanvas* canvas();
	const QwtPolarCanvas* canvas() const;

public Q_SLOTS:
    virtual void rescale(double factor) QWT_OVERRIDE;
	void unzoom();

protected:
    virtual void widgetKeyPressEvent(QKeyEvent*) QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_polar_magnifier.h ***/

/*** Start of inlined file: qwt_polar_marker.h ***/
#ifndef QWT_POLAR_MARKER_H
#define QWT_POLAR_MARKER_H

class QRect;
class QwtText;
class QwtSymbol;

/*!
   \brief A class for drawing markers

   A marker can be a a symbol, a label or a combination of them, which can
   be drawn around a center point inside a bounding rectangle.

   The setSymbol() member assigns a symbol to the marker.
   The symbol is drawn at the specified point.

   With setLabel(), a label can be assigned to the marker.
   The setLabelAlignment() member specifies where the label is
   drawn. All the Align*-constants in Qt::AlignmentFlags (see Qt documentation)
   are valid. The alignment refers to the center point of
   the marker, which means, for example, that the label would be painted
   left above the center point if the alignment was set to AlignLeft|AlignTop.
 */
class QWT_EXPORT QwtPolarMarker : public QwtPolarItem
{
public:
	explicit QwtPolarMarker();
	virtual ~QwtPolarMarker();

	virtual int rtti() const QWT_OVERRIDE;

    void setPosition(const QwtPointPolar&);
	QwtPointPolar position() const;

    void setSymbol(const QwtSymbol* s);
	const QwtSymbol* symbol() const;

    void setLabel(const QwtText&);
	QwtText label() const;

    void setLabelAlignment(Qt::Alignment);
	Qt::Alignment labelAlignment() const;

    virtual void draw(QPainter* painter,
                      const QwtScaleMap& azimuthMap,
                      const QwtScaleMap& radialMap,
                      const QPointF& pole,
                      double radius,
                      const QRectF& canvasRect) const QWT_OVERRIDE;

    virtual QwtInterval boundingInterval(int scaleId) const QWT_OVERRIDE;

private:
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_polar_marker.h ***/

/*** Start of inlined file: qwt_polar_panner.h ***/
#ifndef QWT_POLAR_PANNER_H
#define QWT_POLAR_PANNER_H

class QwtPolarPlot;
class QwtPolarCanvas;

/*!
   \brief QwtPolarPanner provides panning of a polar plot canvas

   QwtPolarPanner is a panner for a QwtPolarCanvas, that
   adjusts the visible area after dropping
   the canvas on its new position.

   Together with QwtPolarMagnifier individual ways
   of navigating on a QwtPolarPlot widget can be implemented easily.

   \sa QwtPolarMagnifier
 */
class QWT_EXPORT QwtPolarPanner : public QwtPanner
{
	Q_OBJECT

public:
    explicit QwtPolarPanner(QwtPolarCanvas*);
	virtual ~QwtPolarPanner();

	QwtPolarPlot* plot();
	const QwtPolarPlot* plot() const;

	QwtPolarCanvas* canvas();
	const QwtPolarCanvas* canvas() const;

public Q_SLOTS:
    virtual void movePlot(int dx, int dy);

protected:
    virtual void widgetMousePressEvent(QMouseEvent*) QWT_OVERRIDE;
};

#endif

/*** End of inlined file: qwt_polar_panner.h ***/

/*** Start of inlined file: qwt_polar_picker.h ***/
#ifndef QWT_POLAR_PICKER_H
#define QWT_POLAR_PICKER_H

#include <qvector.h>
#include <qpainterpath.h>

class QwtPolarPlot;
class QwtPolarCanvas;
class QwtPointPolar;

/*!
   \brief QwtPolarPicker provides selections on a plot canvas

   QwtPolarPicker is a QwtPicker tailored for selections on
   a polar plot canvas.
 */
class QWT_EXPORT QwtPolarPicker : public QwtPicker
{
	Q_OBJECT

public:
    explicit QwtPolarPicker(QwtPolarCanvas*);
	virtual ~QwtPolarPicker();

    explicit QwtPolarPicker(RubberBand rubberBand, DisplayMode trackerMode, QwtPolarCanvas*);

	QwtPolarPlot* plot();
	const QwtPolarPlot* plot() const;

	QwtPolarCanvas* canvas();
	const QwtPolarCanvas* canvas() const;

	virtual QRect pickRect() const;

Q_SIGNALS:

	/*!
	   A signal emitted in case of selectionFlags() & PointSelection.
	   \param pos Selected point
	 */
    void selected(const QwtPointPolar& pos);

	/*!
	   A signal emitting the selected points,
	   at the end of a selection.

	   \param points Selected points
	 */
    void selected(const QVector< QwtPointPolar >& points);

	/*!
	   A signal emitted when a point has been appended to the selection

	   \param pos Position of the appended point.
	   \sa append(). moved()
	 */
    void appended(const QwtPointPolar& pos);

	/*!
	   A signal emitted whenever the last appended point of the
	   selection has been moved.

	   \param pos Position of the moved last point of the selection.
	   \sa move(), appended()
	 */
    void moved(const QwtPointPolar& pos);

protected:
    QwtPointPolar invTransform(const QPoint&) const;

    virtual QwtText trackerText(const QPoint&) const QWT_OVERRIDE;
    virtual QwtText trackerTextPolar(const QwtPointPolar&) const;

    virtual void move(const QPoint&) QWT_OVERRIDE;
    virtual void append(const QPoint&) QWT_OVERRIDE;
    virtual bool end(bool ok = true) QWT_OVERRIDE;

private:
	virtual QPainterPath pickArea() const QWT_OVERRIDE;

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_polar_picker.h ***/

/*** Start of inlined file: qwt_polar_spectrogram.h ***/
#ifndef QWT_POLAR_SPECTROGRAM_H
#define QWT_POLAR_SPECTROGRAM_H

#include <qimage.h>

class QwtRasterData;
class QwtColorMap;

/*!
   \brief An item, which displays a spectrogram

   A spectrogram displays 3-dimensional data, where the 3rd dimension
   ( the intensity ) is displayed using colors. The colors are calculated
   from the values using a color map.

   \sa QwtRasterData, QwtColorMap
 */
class QWT_EXPORT QwtPolarSpectrogram : public QwtPolarItem
{
public:
	/*!
        Attributes to modify the drawing algorithm.
        The default setting disables ApproximatedAtan

        \sa setPaintAttribute(), testPaintAttribute()
	 */
	enum PaintAttribute
	{
		/*!
		   Use qwtFastAtan2 instead of atan2 for translating
		   widget into polar coordinates.
		 */

		ApproximatedAtan = 0x01
	};

    Q_DECLARE_FLAGS(PaintAttributes, PaintAttribute)

	explicit QwtPolarSpectrogram();
	virtual ~QwtPolarSpectrogram();

    void setData(QwtRasterData* data);
	const QwtRasterData* data() const;

    void setColorMap(QwtColorMap*);
	const QwtColorMap* colorMap() const;

    void setPaintAttribute(PaintAttribute, bool on = true);
    bool testPaintAttribute(PaintAttribute) const;

	virtual int rtti() const QWT_OVERRIDE;

    virtual void draw(QPainter* painter,
                      const QwtScaleMap& azimuthMap,
                      const QwtScaleMap& radialMap,
                      const QPointF& pole,
                      double radius,
                      const QRectF& canvasRect) const QWT_OVERRIDE;

    virtual QwtInterval boundingInterval(int scaleId) const QWT_OVERRIDE;

protected:
    virtual QImage renderImage(const QwtScaleMap& azimuthMap,
                               const QwtScaleMap& radialMap,
                               const QPointF& pole,
                               const QRect& rect) const;

    virtual void renderTile(const QwtScaleMap& azimuthMap,
                            const QwtScaleMap& radialMap,
                            const QPointF& pole,
                            const QPoint& imagePos,
                            const QRect& tile,
                            QImage* image) const;

private:
	class TileInfo;
    void renderTileInfo(const QwtScaleMap&, const QwtScaleMap&, const QPointF& pole, TileInfo*) const;

	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPolarSpectrogram::PaintAttributes)

#endif

/*** End of inlined file: qwt_polar_spectrogram.h ***/

// plot widget

/*** Start of inlined file: qwt_plot.h ***/
#ifndef QWT_PLOT_H
#define QWT_PLOT_H

#include <qframe.h>

class QwtPlotLayout;
class QwtAbstractLegend;
class QwtScaleWidget;
class QwtScaleEngine;
class QwtScaleDiv;
class QwtScaleMap;
class QwtScaleDraw;
class QwtTextLabel;
class QwtInterval;
class QwtText;
template< typename T >
class QList;

// 6.1 compatibility definitions
#define QWT_AXIS_COMPAT 1

/*!
   \brief A 2-D plotting widget

   QwtPlot is a widget for plotting two-dimensional graphs.
   An unlimited number of plot items can be displayed on
   its canvas. Plot items might be curves (QwtPlotCurve), markers
   (QwtPlotMarker), the grid (QwtPlotGrid), or anything else derived
   from QwtPlotItem.
   A plot can have up to four axes, with each plot item attached to an x- and
   a y axis. The scales at the axes can be explicitly set (QwtScaleDiv), or
   are calculated from the plot items, using algorithms (QwtScaleEngine) which
   can be configured separately for each axis.

   The simpleplot example is a good starting point to see how to set up a
   plot widget.

   \image html plot.png

   \par Example
    The following example shows (schematically) the most simple
    way to use QwtPlot. By default, only the left and bottom axes are
    visible and their scales are computed automatically.
    \code

      QwtPlot *myPlot = new QwtPlot( "Two Curves", parent );

      // add curves
      QwtPlotCurve *curve1 = new QwtPlotCurve( "Curve 1" );
      QwtPlotCurve *curve2 = new QwtPlotCurve( "Curve 2" );

      // connect or copy the data to the curves
      curve1->setData( ... );
      curve2->setData( ... );

      curve1->attach( myPlot );
      curve2->attach( myPlot );

      // finally, refresh the plot
      myPlot->replot();
    \endcode
 */

class QWT_EXPORT QwtPlot : public QFrame, public QwtPlotDict
{
	Q_OBJECT

    Q_PROPERTY(QBrush canvasBackground READ canvasBackground WRITE setCanvasBackground)

    Q_PROPERTY(bool autoReplot READ autoReplot WRITE setAutoReplot)

public:
	/*!
        Position of the legend, relative to the canvas.

        \sa insertLegend()
	 */
	enum LegendPosition
	{
		//! The legend will be left from the QwtAxis::YLeft axis.
		LeftLegend,

		//! The legend will be right from the QwtAxis::YRight axis.
		RightLegend,

		//! The legend will be below the footer
		BottomLegend,

		//! The legend will be above the title
		TopLegend
	};

    explicit QwtPlot(QWidget* = NULL);
    explicit QwtPlot(const QwtText& title, QWidget* = NULL);

	virtual ~QwtPlot();

    void setAutoReplot(bool = true);
	bool autoReplot() const;

	// Layout

    void setPlotLayout(QwtPlotLayout*);

	QwtPlotLayout* plotLayout();
	const QwtPlotLayout* plotLayout() const;

	// Title

    void setTitle(const QString&);
    void setTitle(const QwtText&);
	QwtText title() const;

	QwtTextLabel* titleLabel();
	const QwtTextLabel* titleLabel() const;

	// Footer

    void setFooter(const QString&);
    void setFooter(const QwtText&);
	QwtText footer() const;

	QwtTextLabel* footerLabel();
	const QwtTextLabel* footerLabel() const;

	// Canvas

    void setCanvas(QWidget*);

	QWidget* canvas();
	const QWidget* canvas() const;

    void setCanvasBackground(const QBrush&);
	QBrush canvasBackground() const;

    virtual QwtScaleMap canvasMap(QwtAxisId) const;

    double invTransform(QwtAxisId, double pos) const;
    double transform(QwtAxisId, double value) const;

	// Axes

    bool isAxisValid(QwtAxisId) const;

    void setAxisVisible(QwtAxisId, bool on = true);
    bool isAxisVisible(QwtAxisId) const;

	// Axes data

    QwtScaleEngine* axisScaleEngine(QwtAxisId);
    const QwtScaleEngine* axisScaleEngine(QwtAxisId) const;
    void setAxisScaleEngine(QwtAxisId, QwtScaleEngine*);

    void setAxisAutoScale(QwtAxisId, bool on = true);
    bool axisAutoScale(QwtAxisId) const;

    void setAxisFont(QwtAxisId, const QFont&);
    QFont axisFont(QwtAxisId) const;

    void setAxisScale(QwtAxisId, double min, double max, double stepSize = 0);
    void setAxisScaleDiv(QwtAxisId, const QwtScaleDiv&);
    void setAxisScaleDraw(QwtAxisId, QwtScaleDraw*);

    double axisStepSize(QwtAxisId) const;
    QwtInterval axisInterval(QwtAxisId) const;
    const QwtScaleDiv& axisScaleDiv(QwtAxisId) const;

    const QwtScaleDraw* axisScaleDraw(QwtAxisId) const;
    QwtScaleDraw* axisScaleDraw(QwtAxisId);

    const QwtScaleWidget* axisWidget(QwtAxisId) const;
    QwtScaleWidget* axisWidget(QwtAxisId);

    void setAxisLabelAlignment(QwtAxisId, Qt::Alignment);
    void setAxisLabelRotation(QwtAxisId, double rotation);

    void setAxisTitle(QwtAxisId, const QString&);
    void setAxisTitle(QwtAxisId, const QwtText&);
    QwtText axisTitle(QwtAxisId) const;

    void setAxisMaxMinor(QwtAxisId, int maxMinor);
    int axisMaxMinor(QwtAxisId) const;

    void setAxisMaxMajor(QwtAxisId, int maxMajor);
    int axisMaxMajor(QwtAxisId) const;

	// Legend

    void insertLegend(QwtAbstractLegend*, LegendPosition = QwtPlot::RightLegend, double ratio = -1.0);

	QwtAbstractLegend* legend();
	const QwtAbstractLegend* legend() const;

	void updateLegend();
    void updateLegend(const QwtPlotItem*);

	// Misc

	virtual QSize sizeHint() const QWT_OVERRIDE;
	virtual QSize minimumSizeHint() const QWT_OVERRIDE;

	virtual void updateLayout();
    virtual void drawCanvas(QPainter*);

	void updateAxes();
	void updateCanvasMargins();

    virtual void getCanvasMarginsHint(const QwtScaleMap maps[],
                                      const QRectF& canvasRect,
                                      double& left,
                                      double& top,
                                      double& right,
                                      double& bottom) const;

    virtual bool event(QEvent*) QWT_OVERRIDE;
    virtual bool eventFilter(QObject*, QEvent*) QWT_OVERRIDE;

    virtual void drawItems(QPainter*, const QRectF&, const QwtScaleMap maps[ QwtAxis::AxisPositions ]) const;

    virtual QVariant itemToInfo(QwtPlotItem*) const;
    virtual QwtPlotItem* infoToItem(const QVariant&) const;

#if QWT_AXIS_COMPAT
    enum Axis { yLeft   = QwtAxis::YLeft,
                yRight  = QwtAxis::YRight,
                xBottom = QwtAxis::XBottom,
                xTop    = QwtAxis::XTop,

                axisCnt = QwtAxis::AxisPositions };

    void enableAxis(int axisId, bool on = true)
	{
        setAxisVisible(axisId, on);
	}

    bool axisEnabled(int axisId) const
	{
        return isAxisVisible(axisId);
	}
#endif

Q_SIGNALS:
	/*!
	   A signal indicating, that an item has been attached/detached

	   \param plotItem Plot item
	   \param on Attached/Detached
	 */
    void itemAttached(QwtPlotItem* plotItem, bool on);

	/*!
	   A signal with the attributes how to update
	   the legend entries for a plot item.

	   \param itemInfo Info about a plot item, build from itemToInfo()
	   \param data Attributes of the entries ( usually <= 1 ) for
                  the plot item.

	   \sa itemToInfo(), infoToItem(), QwtAbstractLegend::updateLegend()
	 */
    void legendDataChanged(const QVariant& itemInfo, const QList< QwtLegendData >& data);

public Q_SLOTS:
	virtual void replot();
	void autoRefresh();

protected:
    virtual void resizeEvent(QResizeEvent*) QWT_OVERRIDE;

private Q_SLOTS:
    void updateLegendItems(const QVariant& itemInfo, const QList< QwtLegendData >& legendData);

private:
	friend class QwtPlotItem;
    void attachItem(QwtPlotItem*, bool);

	void initAxesData();
	void deleteAxesData();
	void updateScaleDiv();

    void initPlot(const QwtText& title);

	class ScaleData;
	ScaleData* m_scaleData;

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot.h ***/

/*** Start of inlined file: qwt_plot_layout.h ***/
#ifndef QWT_PLOT_LAYOUT_H
#define QWT_PLOT_LAYOUT_H

/*!
   \brief Layout engine for QwtPlot.

   It is used by the QwtPlot widget to organize its internal widgets
   or by QwtPlot::print() to render its content to a QPaintDevice like
   a QPrinter, QPixmap/QImage or QSvgRenderer.

   \sa QwtPlot::setPlotLayout()
 */

class QWT_EXPORT QwtPlotLayout
{
public:
	/*!
	   Options to configure the plot layout engine
	   \sa activate(), QwtPlotRenderer
	 */
	enum Option
	{
		//! Unused
		AlignScales = 0x01,

		/*!
		   Ignore the dimension of the scrollbars. There are no
		   scrollbars, when the plot is not rendered to widgets.
		 */
		IgnoreScrollbars = 0x02,

		//! Ignore all frames.
		IgnoreFrames = 0x04,

		//! Ignore the legend.
		IgnoreLegend = 0x08,

		//! Ignore the title.
		IgnoreTitle = 0x10,

		//! Ignore the footer.
		IgnoreFooter = 0x20
	};

    Q_DECLARE_FLAGS(Options, Option)

	explicit QwtPlotLayout();
	virtual ~QwtPlotLayout();

    void setCanvasMargin(int margin, int axis = -1);
    int canvasMargin(int axisId) const;

    void setAlignCanvasToScales(bool);

    void setAlignCanvasToScale(int axisId, bool);
    bool alignCanvasToScale(int axisId) const;

    void setSpacing(int);
	int spacing() const;

    void setLegendPosition(QwtPlot::LegendPosition pos, double ratio);
    void setLegendPosition(QwtPlot::LegendPosition pos);
	QwtPlot::LegendPosition legendPosition() const;

    void setLegendRatio(double ratio);
	double legendRatio() const;

    virtual QSize minimumSizeHint(const QwtPlot*) const;

    virtual void activate(const QwtPlot*, const QRectF& plotRect, Options options = Options());

	virtual void invalidate();

	QRectF titleRect() const;
	QRectF footerRect() const;
	QRectF legendRect() const;
    QRectF scaleRect(QwtAxisId) const;
	QRectF canvasRect() const;

protected:
    void setTitleRect(const QRectF&);
    void setFooterRect(const QRectF&);
    void setLegendRect(const QRectF&);
    void setScaleRect(QwtAxisId, const QRectF&);
    void setCanvasRect(const QRectF&);

private:
	Q_DISABLE_COPY(QwtPlotLayout)

	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotLayout::Options)

#endif

/*** End of inlined file: qwt_plot_layout.h ***/

/*** Start of inlined file: qwt_plot_rescaler.h ***/
#ifndef QWT_PLOT_RESCALER_H
#define QWT_PLOT_RESCALER_H

#include <qobject.h>

class QwtPlot;
class QwtInterval;
class QResizeEvent;

/*!
    \brief QwtPlotRescaler takes care of fixed aspect ratios for plot scales

    QwtPlotRescaler auto adjusts the axes of a QwtPlot according
    to fixed aspect ratios.
 */

class QWT_EXPORT QwtPlotRescaler : public QObject
{
	Q_OBJECT

public:
	/*!
	   The rescale policy defines how to rescale the reference axis and
	   their depending axes.

	   \sa ExpandingDirection, setIntervalHint()
	 */
	enum RescalePolicy
	{
		/*!
		   The interval of the reference axis remains unchanged, when the
		   geometry of the canvas changes. All other axes
		   will be adjusted according to their aspect ratio.
		 */
		Fixed,

		/*!
		   The interval of the reference axis will be shrunk/expanded,
		   when the geometry of the canvas changes. All other axes
		   will be adjusted according to their aspect ratio.

		   The interval, that is represented by one pixel is fixed.

		 */
		Expanding,

		/*!
		   The intervals of the axes are calculated, so that all axes include
		   their interval hint.
		 */
		Fitting
	};

	/*!
	   When rescalePolicy() is set to Expanding its direction depends
	   on ExpandingDirection
	 */
	enum ExpandingDirection
	{
		//! The upper limit of the scale is adjusted
		ExpandUp,

		//! The lower limit of the scale is adjusted
		ExpandDown,

		//! Both limits of the scale are adjusted
		ExpandBoth
	};

    explicit QwtPlotRescaler(QWidget* canvas, QwtAxisId referenceAxis = QwtAxis::XBottom, RescalePolicy = Expanding);

	virtual ~QwtPlotRescaler();

    void setEnabled(bool);
	bool isEnabled() const;

    void setRescalePolicy(RescalePolicy);
	RescalePolicy rescalePolicy() const;

    void setExpandingDirection(ExpandingDirection);
    void setExpandingDirection(QwtAxisId, ExpandingDirection);
    ExpandingDirection expandingDirection(QwtAxisId) const;

    void setReferenceAxis(QwtAxisId);
	QwtAxisId referenceAxis() const;

    void setAspectRatio(double ratio);
    void setAspectRatio(QwtAxisId, double ratio);
    double aspectRatio(QwtAxisId) const;

    void setIntervalHint(QwtAxisId, const QwtInterval&);
    QwtInterval intervalHint(QwtAxisId) const;

	QWidget* canvas();
	const QWidget* canvas() const;

	QwtPlot* plot();
	const QwtPlot* plot() const;

    virtual bool eventFilter(QObject*, QEvent*) QWT_OVERRIDE;

	void rescale() const;

protected:
    virtual void canvasResizeEvent(QResizeEvent*);

    virtual void rescale(const QSize& oldSize, const QSize& newSize) const;
    virtual QwtInterval expandScale(QwtAxisId, const QSize& oldSize, const QSize& newSize) const;

    virtual QwtInterval syncScale(QwtAxisId, const QwtInterval& reference, const QSize& size) const;

    virtual void updateScales(QwtInterval intervals[ QwtAxis::AxisPositions ]) const;

    Qt::Orientation orientation(QwtAxisId) const;
    QwtInterval interval(QwtAxisId) const;
    QwtInterval expandInterval(const QwtInterval&, double width, ExpandingDirection) const;

private:
    double pixelDist(QwtAxisId, const QSize&) const;

	class AxisData;
	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_plot_rescaler.h ***/

/*** Start of inlined file: qwt_polar_plot.h ***/
#ifndef QWT_POLAR_PLOT_H
#define QWT_POLAR_PLOT_H

#include <qframe.h>

class QwtRoundScaleDraw;
class QwtScaleEngine;
class QwtScaleDiv;
class QwtTextLabel;
class QwtPolarCanvas;
class QwtPolarLayout;
class QwtAbstractLegend;

/*!
   \brief A plotting widget, displaying a polar coordinate system

   An unlimited number of plot items can be displayed on
   its canvas. Plot items might be curves (QwtPolarCurve), markers
   (QwtPolarMarker), the grid (QwtPolarGrid), or anything else derived
   from QwtPolarItem.

   The coordinate system is defined by a radial and a azimuth scale.
   The scales at the axes can be explicitly set (QwtScaleDiv), or
   are calculated from the plot items, using algorithms (QwtScaleEngine) which
   can be configured separately for each axis. Autoscaling is supported
   for the radial scale.

   In opposite to QwtPlot the scales might be different from the
   view, that is displayed on the canvas. The view can be changed by
   zooming - f.e. by using QwtPolarPanner or QwtPolarMaginfier.
 */
class QWT_EXPORT QwtPolarPlot : public QFrame, public QwtPolarItemDict
{
	Q_OBJECT

    Q_PROPERTY(QBrush plotBackground READ plotBackground WRITE setPlotBackground)
    Q_PROPERTY(double azimuthOrigin READ azimuthOrigin WRITE setAzimuthOrigin)

public:
	/*!
        Position of the legend, relative to the canvas.
        \sa insertLegend()
	 */
	enum LegendPosition
	{
		//! The legend will be left from the canvas.
		LeftLegend,

		//! The legend will be right from the canvas.
		RightLegend,

		//! The legend will be below the canvas.
		BottomLegend,

		//! The legend will be between canvas and title.
		TopLegend,

		/*!
		   External means that only the content of the legend
		   will be handled by QwtPlot, but not its geometry.
		   This might be interesting if an application wants to
		   have a legend in an external window ( or on the canvas ).

		   \note The legend is not painted by QwtPolarRenderer
		 */
		ExternalLegend
	};

    explicit QwtPolarPlot(QWidget* parent = NULL);
    QwtPolarPlot(const QwtText& title, QWidget* parent = NULL);

	virtual ~QwtPolarPlot();

    void setTitle(const QString&);
    void setTitle(const QwtText&);

	QwtText title() const;

	QwtTextLabel* titleLabel();
	const QwtTextLabel* titleLabel() const;

    void setAutoReplot(bool tf = true);
	bool autoReplot() const;

    void setAutoScale(int scaleId);
    bool hasAutoScale(int scaleId) const;

    void setScaleMaxMinor(int scaleId, int maxMinor);
    int scaleMaxMinor(int scaleId) const;

    int scaleMaxMajor(int scaleId) const;
    void setScaleMaxMajor(int scaleId, int maxMajor);

    QwtScaleEngine* scaleEngine(int scaleId);
    const QwtScaleEngine* scaleEngine(int scaleId) const;
    void setScaleEngine(int scaleId, QwtScaleEngine*);

    void setScale(int scaleId, double min, double max, double step = 0);

    void setScaleDiv(int scaleId, const QwtScaleDiv&);
    const QwtScaleDiv* scaleDiv(int scaleId) const;
    QwtScaleDiv* scaleDiv(int scaleId);

    QwtScaleMap scaleMap(int scaleId, double radius) const;
    QwtScaleMap scaleMap(int scaleId) const;

    void updateScale(int scaleId);

	double azimuthOrigin() const;

    void zoom(const QwtPointPolar&, double factor);
	void unzoom();

	QwtPointPolar zoomPos() const;
	double zoomFactor() const;

	// Canvas

	QwtPolarCanvas* canvas();
	const QwtPolarCanvas* canvas() const;

    void setPlotBackground(const QBrush& c);
	const QBrush& plotBackground() const;

    virtual void drawCanvas(QPainter*, const QRectF&) const;

	// Legend

    void insertLegend(QwtAbstractLegend*, LegendPosition = RightLegend, double ratio = -1.0);

	QwtAbstractLegend* legend();
	const QwtAbstractLegend* legend() const;

	void updateLegend();
    void updateLegend(const QwtPolarItem*);

	// Layout
	QwtPolarLayout* plotLayout();
	const QwtPolarLayout* plotLayout() const;

	QwtInterval visibleInterval() const;
	QRectF plotRect() const;
    QRectF plotRect(const QRectF&) const;

	int plotMarginHint() const;

    virtual QVariant itemToInfo(QwtPolarItem*) const;
    virtual QwtPolarItem* infoToItem(const QVariant&) const;

Q_SIGNALS:
	/*!
	   A signal indicating, that an item has been attached/detached

	   \param plotItem Plot item
	   \param on Attached/Detached
	 */
    void itemAttached(QwtPolarItem* plotItem, bool on);

	/*!
	   A signal with the attributes how to update
	   the legend entries for a plot item.

	   \param itemInfo Info about a plot, build from itemToInfo()
	   \param data Attributes of the entries ( usually <= 1 ) for the plot item.

	   \sa itemToInfo(), infoToItem(), QwtAbstractLegend::updateLegend()
	 */
    void legendDataChanged(const QVariant& itemInfo, const QList< QwtLegendData >& data);

	/*!
	   A signal that is emitted, whenever the layout of the plot
	   has been recalculated.
	 */
	void layoutChanged();

public Q_SLOTS:
	virtual void replot();
	void autoRefresh();
    void setAzimuthOrigin(double);

protected:
    virtual bool event(QEvent*) QWT_OVERRIDE;
    virtual void resizeEvent(QResizeEvent*) QWT_OVERRIDE;

	virtual void updateLayout();

    virtual void drawItems(QPainter* painter,
                           const QwtScaleMap& radialMap,
                           const QwtScaleMap& azimuthMap,
                           const QPointF& pole,
                           double radius,
                           const QRectF& canvasRect) const;

private:
	friend class QwtPolarItem;
    void attachItem(QwtPolarItem*, bool);

    void initPlot(const QwtText&);

	class PrivateData;
	PrivateData* m_data;
};

#endif

/*** End of inlined file: qwt_polar_plot.h ***/

/*** Start of inlined file: qwt_polar_grid.h ***/
#ifndef QWT_POLAR_GRID_H
#define QWT_POLAR_GRID_H

class QPainter;
class QPen;
class QwtScaleMap;
class QwtScaleDiv;
class QwtRoundScaleDraw;
class QwtScaleDraw;

/*!
   \brief An item which draws scales and grid lines on a polar plot.

   The QwtPolarGrid class can be used to draw a coordinate grid.
   A coordinate grid consists of major and minor gridlines.
   The locations of the gridlines are determined by the azimuth and radial
   scale divisions.

   QwtPolarGrid is also responsible for drawing the axis representing the
   scales. It is possible to display 4 radial and one azimuth axis.

   Whenever the scale divisions of the plot widget changes the grid
   is synchronized by updateScaleDiv().

   \sa QwtPolarPlot, QwtPolar::Axis
 */

class QWT_EXPORT QwtPolarGrid : public QwtPolarItem
{
public:
	/*!
	   Mysterious flags trying to avoid conflicts, when painting the
	   scales and grid lines.

	   The default setting enables all flags.

	   \sa setDisplayFlag(), testDisplayFlag()
	 */
	enum DisplayFlag
	{
		/*!
		   Try to avoid situations, where the label of the origin is
		   painted over another axis.
		 */
		SmartOriginLabel = 1,

		/*!
		   Often the outermost tick of the radial scale is close to the
		   canvas border. With HideMaxRadiusLabel enabled it is not painted.
		 */
		HideMaxRadiusLabel = 2,

		/*!
		   The tick labels of the radial scales might be hard to read, when
		   they are painted on top of the radial grid lines ( or on top
		   of a curve/spectrogram ). When ClipAxisBackground the bounding rect
		   of each label is added to the clip region.
		 */
		ClipAxisBackground = 4,

		/*!
		   Don't paint the backbone of the radial axes, when they are very close
		   to a line of the azimuth grid.
		 */
		SmartScaleDraw = 8,

		/*!
		   All grid lines are clipped against the plot area before being painted.
		   When the plot is zoomed in this will have an significant impact
		   on the performance of the painting code.
		 */
		ClipGridLines = 16
	};

    Q_DECLARE_FLAGS(DisplayFlags, DisplayFlag)

	/*!
	   \brief Grid attributes
	   \sa setGridAttributes(), testGridAttributes()
	 */
	enum GridAttribute
	{
		/*!
		   When AutoScaling is enabled, the radial axes will be adjusted
		   to the interval, that is currently visible on the canvas plot.
		 */
		AutoScaling = 0x01
	};

    Q_DECLARE_FLAGS(GridAttributes, GridAttribute)

	explicit QwtPolarGrid();
	virtual ~QwtPolarGrid();

	virtual int rtti() const QWT_OVERRIDE;

    void setDisplayFlag(DisplayFlag, bool on = true);
    bool testDisplayFlag(DisplayFlag) const;

    void setGridAttribute(GridAttribute, bool on = true);
    bool testGridAttribute(GridAttribute) const;

    void showGrid(int scaleId, bool show = true);
    bool isGridVisible(int scaleId) const;

    void showMinorGrid(int scaleId, bool show = true);
    bool isMinorGridVisible(int scaleId) const;

    void showAxis(int axisId, bool show = true);
    bool isAxisVisible(int axisId) const;

    void setPen(const QPen& p);
    void setFont(const QFont&);

    void setMajorGridPen(const QPen& p);
    void setMajorGridPen(int scaleId, const QPen& p);
    QPen majorGridPen(int scaleId) const;

    void setMinorGridPen(const QPen& p);
    void setMinorGridPen(int scaleId, const QPen& p);
    QPen minorGridPen(int scaleId) const;

    void setAxisPen(int axisId, const QPen& p);
    QPen axisPen(int axisId) const;

    void setAxisFont(int axisId, const QFont& p);
    QFont axisFont(int axisId) const;

    void setScaleDraw(int axisId, QwtScaleDraw*);
    const QwtScaleDraw* scaleDraw(int axisId) const;
    QwtScaleDraw* scaleDraw(int axisId);

    void setAzimuthScaleDraw(QwtRoundScaleDraw*);
	const QwtRoundScaleDraw* azimuthScaleDraw() const;
	QwtRoundScaleDraw* azimuthScaleDraw();

    virtual void draw(QPainter* p,
                      const QwtScaleMap& azimuthMap,
                      const QwtScaleMap& radialMap,
                      const QPointF& pole,
                      double radius,
                      const QRectF& rect) const QWT_OVERRIDE;

    virtual void updateScaleDiv(const QwtScaleDiv& azimuthMap, const QwtScaleDiv& radialMap, const QwtInterval&) QWT_OVERRIDE;

	virtual int marginHint() const QWT_OVERRIDE;

protected:
    void drawRays(QPainter*,
                  const QRectF&,
                  const QPointF& pole,
                  double radius,
                  const QwtScaleMap& azimuthMap,
                  const QList< double >&) const;
    void drawCircles(QPainter*, const QRectF&, const QPointF& pole, const QwtScaleMap& radialMap, const QList< double >&) const;

    void drawAxis(QPainter*, int axisId) const;

private:
    void updateScaleDraws(const QwtScaleMap& azimuthMap,
                          const QwtScaleMap& radialMap,
                          const QPointF& pole,
                          const double radius) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPolarGrid::DisplayFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPolarGrid::GridAttributes)

#endif

/*** End of inlined file: qwt_polar_grid.h ***/

/*** Start of inlined file: qwt_polar_layout.h ***/
#ifndef QWT_POLAR_LAYOUT_H
#define QWT_POLAR_LAYOUT_H

/*!
   \brief Layout class for QwtPolarPlot.

   Organizes the geometry for the different QwtPolarPlot components.
   It is used by the QwtPolar widget to organize its internal widgets
   or by QwtPolarRnderer to render its content to a QPaintDevice like
   a QPrinter, QPixmap/QImage or QSvgRenderer.
 */
class QWT_EXPORT QwtPolarLayout
{
public:
	//! \brief Options to configure the plot layout engine
	enum Option
	{
		//! Ignore the dimension of the scrollbars.
		IgnoreScrollbars = 0x01,

		//! Ignore all frames.
        IgnoreFrames = 0x02,

		//! Ignore the title.
        IgnoreTitle = 0x04,

		//! Ignore the legend.
        IgnoreLegend = 0x08
	};

    Q_DECLARE_FLAGS(Options, Option)

	explicit QwtPolarLayout();
	virtual ~QwtPolarLayout();

    void setLegendPosition(QwtPolarPlot::LegendPosition pos, double ratio);
    void setLegendPosition(QwtPolarPlot::LegendPosition pos);
	QwtPolarPlot::LegendPosition legendPosition() const;

    void setLegendRatio(double ratio);
	double legendRatio() const;

    virtual void activate(const QwtPolarPlot*, const QRectF& rect, Options options = Options());

	virtual void invalidate();

	const QRectF& titleRect() const;
	const QRectF& legendRect() const;
	const QRectF& canvasRect() const;

	class LayoutData;

protected:
    QRectF layoutLegend(Options options, QRectF&) const;

private:
	class PrivateData;
	PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPolarLayout::Options)

#endif

/*** End of inlined file: qwt_polar_layout.h ***/

/*** Start of inlined file: qwt_figure_layout.h ***/
#ifndef QWT_FIGURE_LAYOUT_H
#define QWT_FIGURE_LAYOUT_H
// stl
#include <memory>

// Qt
#include <QLayout>

// qwt

/**
 * @class QwtFigureLayout
 * @brief Custom layout manager for QwtFigureWidget that handles both normalized coordinates and grid layouts
 *
 * 自定义布局管理器，用于QwtFigureWidget，支持归一化坐标和网格布局
 */
class QWT_EXPORT QwtFigureLayout : public QLayout
{
	Q_OBJECT
public:
	QwtFigureLayout();
	explicit QwtFigureLayout(QWidget* parent);
	virtual ~QwtFigureLayout();

	virtual void addItem(QLayoutItem* item) override;
	virtual QLayoutItem* itemAt(int index) const override;
	virtual QLayoutItem* takeAt(int index) override;
	virtual int count() const override;
	virtual QSize sizeHint() const override;
	virtual QSize minimumSize() const override;
	virtual void setGeometry(const QRect& rect) override;

	// Add a widget with normalized coordinates/使用归一化坐标添加窗口部件
	void addAxes(QWidget* widget, const QRectF& rect);

	// Add a widget with normalized coordinates using separate parameters/使用分离参数和归一化坐标添加窗口部件
	void addAxes(QWidget* widget, qreal left, qreal top, qreal width, qreal height);

	// Add a widget by grid layout/添加窗口部件到网格布局
	void addAxes(QWidget* widget,
                 int rowCnt,
                 int colCnt,
                 int row,
                 int col,
                 int rowSpan  = 1,
                 int colSpan  = 1,
                 qreal wspace = 0.0,
                 qreal hspace = 0.0);

	// Update layout parameters/更新布局参数
	void adjustLayout(qreal left, qreal bottom, qreal right, qreal top);

	// Get the normalized rectangle for a widget/获取窗口部件的归一化矩形
	QRectF widgetNormRect(QWidget* widget) const;

protected:
	// calc the normalized rectangle for a grid cell/获取网格单元格的归一化矩形
	QRectF calcGridRect(int rowCnt,
                        int colCnt,
                        int row,
                        int col,
                        int rowSpan  = 1,
                        int colSpan  = 1,
                        qreal wspace = 0.0,
                        qreal hspace = 0.0) const;

private:
	class PrivateData;
	std::unique_ptr< PrivateData > m_data;
};

#endif  // QWT_FIGURE_LAYOUT_H

/*** End of inlined file: qwt_figure_layout.h ***/

/*** Start of inlined file: qwt_figure.h ***/
#ifndef QWT_FIGURE_H
#define QWT_FIGURE_H
// stl
#include <memory>

// Qt
#include <QFrame>
class QPaintEvent;
// qwt

class QwtPlot;

/**
 * @class QwtFigure
 * @brief A figure container for organizing Qwt plots with flexible layout options
 * @brief 用于组织Qwt绘图的图形容器，提供灵活的布局选项
 *
 * This class provides a figure-like container similar to matplotlib's Figure class,
 * supporting both normalized coordinate positioning and grid layouts for Qwt plots.
 * It uses Qt's standard top-left coordinate system for intuitive positioning.
 *
 * 此类提供类似于matplotlib的Figure类的图形容器，支持Qwt绘图的归一化坐标定位和网格布局。
 * 它使用Qt的标准左上角坐标系，使定位更加直观。
 *
 * @code
 * // Example usage:
 * // 使用示例：
 * QwtFigure figure;
 *
 * // Add a plot using normalized coordinates (Qt top-left coordinate system)
 * // 使用归一化坐标添加绘图（Qt左上角坐标系）
 * QwtPlot* plot1 = new QwtPlot;
 * figure.addAxes(plot1, QRectF(0.1, 0.1, 0.8, 0.4)); // Left: 10%, Top: 10%, Width: 80%, Height: 40%
 *
 * // Add plots using grid layout
 * // 使用网格布局添加绘图
 * // Create a 2x2 grid:
 * // 创建2x2网格：
 * // +-------------------+-------------------+
 * // |                   |                   |
 * // |      (0,0)        |       (0,1)       |
 * // |                   |                   |
 * // +-------------------+-------------------+
 * // |                   |                   |
 * // |      (1,0)        |       (1,1)       |
 * // |                   |                   |
 * // +-------------------+-------------------+
 *
 * QwtPlot* plot2 = new QwtPlot;
 * figure.addAxes(plot2, 2, 2, 0, 1); // 2x2 grid, row 0, column 1
 * // Result:
 * // 结果：
 * // +-------------------+-------------------+
 * // |                   |                   |
 * // |                   |      plot2        |
 * // |                   |                   |
 * // +-------------------+-------------------+
 * // |                   |                   |
 * // |                   |                   |
 * // |                   |                   |
 * // +-------------------+-------------------+
 *
 * QwtPlot* plot3 = new QwtPlot;
 * figure.addAxes(plot3, 2, 2, 1, 0, 1, 2); // 2x2 grid, row 1, columns 0-1 (span 2 columns)
 * // Result:
 * // 结果：
 * // +-------------------+-------------------+
 * // |                   |                   |
 * // |                   |      plot2        |
 * // |                   |                   |
 * // +---------------------------------------+
 * // |                                       |
 * // |              plot3 (span 2 cols)      |
 * // |                                       |
 * // +---------------------------------------+
 *
 * // Adjust layout parameters
 * // 调整布局参数
 * figure.adjustLayout(0.1, 0.1, 0.9, 0.9, 0.2, 0.2);
 *
 * // Save the figure
 * // 保存图形
 * figure.saveFig("output.png", 300);
 * @endcode
 */
class QWT_EXPORT QwtFigure : public QFrame
{
	Q_OBJECT
public:
	QwtFigure(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	virtual ~QwtFigure();
	// Add a plot with normalized coordinates/使用归一化坐标添加绘图
	void addAxes(QwtPlot* plot, const QRectF& rect);

	// Add a plot with normalized coordinates using separate parameters/使用分离参数和归一化坐标添加窗口部件
	void addAxes(QwtPlot* plot, qreal left, qreal top, qreal width, qreal height);

	// Add a plot by grid layout/添加窗口部件到网格布局
	void addAxes(QwtPlot* plot,
                 int rowCnt,
                 int colCnt,
                 int row,
                 int col,
                 int rowSpan  = 1,
                 int colSpan  = 1,
                 qreal wspace = 0.0,
                 qreal hspace = 0.0);

	// Update layout parameters/更新布局参数
	void adjustLayout(qreal left, qreal bottom, qreal right, qreal top);

	// Get all axes (plots) in the figure/获取图形中的所有坐标轴（绘图）
	QList< QwtPlot* > allAxes() const;

	// Check if the figure has any axes/检查图形是否有任意绘图
	bool hasAxes() const;

	// Check if the figure has plot/检查图形是否存在plot
	bool hasAxes(QwtPlot* plot) const;

	// Remove a specific axes (plot) from the figure/从图形中移除特定的坐标轴（绘图）/This function does not destroy the QwtPlot object
	void removeAxes(QwtPlot* plot);

	//  Take a specific axes (plot) from the figure without deleting it/从图形中取出特定的坐标轴（绘图）但不删除它
	bool takeAxes(QwtPlot* plot);

	// Clear all axes from the figure/清除图形中的所有坐标轴
	void clear();

	//  Get the size of the figure in inches/获取图形的英寸尺寸
	QSize getSizeInches() const;

	// Set the size of the figure in inches/设置图形的英寸尺寸
	void setSizeInches(float width, float height);
	void setSizeInches(const QSizeF& size);

	//  Set/Get the face color of the figure/设置图形的背景颜色
	void setFaceColor(const QColor& color);
	QColor faceColor() const;

	// Set/Get the face brush of the figure/设置图形的背景画刷
	void setFaceBrush(const QBrush& brush);
	QBrush faceBrush() const;

	// Set/Get the edge color of the figure/设置图形的边缘颜色
	void setEdgeColor(const QColor& color);
	QColor edgeColor() const;

	// Set/Get the edge line width of the figure/设置图形的边缘线宽
	void setEdgeLineWidth(int width);
	int edgeLineWidth() const;

	// Save methods / 保存方法
	// Save the figure to a QPixmap with specified DPI/使用指定DPI将图形保存为QPixmap
	QPixmap saveFig(int dpi = -1) const;

	// Save the figure to a QPixmap with specified size in inches/使用指定英寸尺寸将图形保存为QPixmap
	QPixmap saveFig(QSizeF& inchesSize) const;

	// Save the figure to a file with specified DPI/使用指定DPI将图形保存到文件
	bool saveFig(const QString& filename, int dpi = -1) const;

	// Set the current axes (plot)/设置当前坐标轴（绘图）
	void setCurrentAxes(QwtPlot* plot);
	void sca(QwtPlot* plot);

	// Get the current axes (plot)/获取当前坐标轴（绘图）
	QwtPlot* currentAxes() const;
	QwtPlot* gca() const;

	// Get the normalized rectangle for a axes/获取绘图的归一化矩形
	QRectF axesNormRect(QwtPlot* plot) const;
Q_SIGNALS:
	/**
	 * @brief Signal emitted when axes are added to the figure/当坐标轴添加到图形时发出的信号
	 * @param newAxes Pointer to the newly added QwtPlot / 指向新添加的QwtPlot的指针
	 */
	void axesAdded(QwtPlot* newAxes);

	/**
	 * @brief Signal emitted when axes are removed from the figure/当坐标轴从图形中移除时发出的信号
	 * @param removedAxes Pointer to the removed QwtPlot / 指向被移除的QwtPlot的指针
	 */
	void axesRemoved(QwtPlot* removedAxes);

	/**
	 * @brief Signal emitted when the figure is cleared/当图形被清除时发出的信号
	 */
	void figureCleared();

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	class PrivateData;
	std::unique_ptr< PrivateData > m_data;
};

#endif  // QWT_FIGURE_H

/*** End of inlined file: qwt_figure.h ***/

/*** End of inlined file: QWTAmalgamTemplatePublicHeaders.h ***/

#endif  // QWTPLOT_AMALGAM_H
