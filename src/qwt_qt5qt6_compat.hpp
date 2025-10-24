#ifndef QWT_QT5QT6_COMPAT_HPP
#define QWT_QT5QT6_COMPAT_HPP
#include <QtCore/QtGlobal>
#include <QtCore/QObject>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QFontMetrics>
#include <QtGui/QFontMetricsF>

namespace qwt
{

/**
 * @brief 处理Qt5与Qt6中的差异
 *
 */
namespace compat
{

/**
 * @brief 获取事件的位置（QPoint）
 * @tparam EventType 事件类型（需支持pos()或position()方法）
 * @param event 事件指针
 * @return 事件位置的QPoint表示
 */
template< typename EventType >
inline QPoint eventPos(EventType* event)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return event->pos();
#else
    return event->position().toPoint();
#endif
}

/**
 * @brief 获取事件的x坐标
 * @tparam EventType 事件类型
 * @param event 事件指针
 * @return x坐标（整数）
 */
template< typename EventType >
inline int eventX(EventType* event)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return event->pos().x();
#else
    return static_cast< int >(event->position().x());
#endif
}

/**
 * @brief 获取事件的y坐标
 * @tparam EventType 事件类型
 * @param event 事件指针
 * @return y坐标（整数）
 */
template< typename EventType >
inline int eventY(EventType* event)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return event->pos().y();
#else
    return static_cast< int >(event->position().y());
#endif
}

/**
 * @brief 计算字符串的水平宽度（整数版本）
 * @param fm QFontMetrics对象
 * @param str 目标字符串
 * @return 宽度（整数）
 */
inline int horizontalAdvance(const QFontMetrics& fm, const QString& str)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)
    return fm.width(str);
#else
    return fm.horizontalAdvance(str);
#endif
}

/**
 * @brief 计算字符串的水平宽度（浮点数版本）
 * @param fm QFontMetricsF对象
 * @param str 目标字符串
 * @return 宽度（浮点数）
 */
inline qreal horizontalAdvanceF(const QFontMetricsF& fm, const QString& str)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)
    return fm.width(str);
#else
    return fm.horizontalAdvance(str);
#endif
}

}  // namespace   compat
}  // namespace   qwt
#endif  // QWT_QT5QT6_COMPAT_HPP
