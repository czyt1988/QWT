#include "qwt_figure_widget_overlay.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QHash>
#include <QApplication>
#include <QDebug>
#include <QPainter>
// std
#include <algorithm>
// qwt
#include "qwt_algorithm.hpp"
#include "qwt_figure.h"
#include "qwt_plot.h"
#include "qwt_qt5qt6_compat.hpp"

#ifndef QwtFigureWidgetOverlay_DEBUG_PRINT
#define QwtFigureWidgetOverlay_DEBUG_PRINT 0
#endif

class QwtFigureWidgetOverlay::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtFigureWidgetOverlay)
public:
    PrivateData(QwtFigureWidgetOverlay* p);

public:
    QwtFigureWidgetOverlay::BuiltInFunctions mFuntion;  ///< 内置功能
    QPoint mLastMousePressPos { 0, 0 };                 ///< 记录最后一次窗口移动的坐标
    QBrush mContorlPointBrush { Qt::blue };             ///< 绘制chart2d在编辑模式下控制点的画刷
    QPen mBorderPen { Qt::blue };                       ///< 绘制chart2d在编辑模式下的画笔
    bool mIsStartResize { false };                      ///< 标定开始进行缩放
    QWidget* mActiveWidget { nullptr };                 /// 标定当前激活的窗口，如果没有就为nullptr
    QRectF mOldNormRect;                                ///< 保存旧的窗口位置，用于redo/undo
    QRectF mWillSetNormRect;                            ///< 将要设置的正则尺寸
    QSize mControlPointSize { 8, 8 };                   ///< 控制点大小
    QwtFigureWidgetOverlay::ControlType mControlType { QwtFigureWidgetOverlay::OutSide };  ///< 记录当前缩放窗口的位置情况

    bool mFigOldMouseTracking { false };  /// 记录原来fig是否设置了mousetrack
    bool mFigOldHasHoverAttr { false };   ///< 记录原来的figure是否含有Hover属性
    bool mShowPrecentText { true };       ///< 显示占比文字
};

QwtFigureWidgetOverlay::PrivateData::PrivateData(QwtFigureWidgetOverlay* p) : q_ptr(p)
{
    mFuntion.setFlag(FunSelectCurrentPlot, true);
    mFuntion.setFlag(FunResizePlot, true);
}

//----------------------------------------------------
// QwtFigureWidgetOverlay
//----------------------------------------------------

/**
 * @brief QwtFigureWidgetOverlay的构造函数
 *
 * @note 构造函数不允许传入nullptr
 * @param fig
 */
QwtFigureWidgetOverlay::QwtFigureWidgetOverlay(QwtFigure* fig) : QwtWidgetOverlay(fig), QWT_PIMPL_CONSTRUCT
{
    Q_ASSERT(fig);
    QWT_D(d);
    // 由于QwtWidgetOverlay是对鼠标隐藏的，因此不能直接使用mouseEvent，直接捕获QwtFigure的event
    //  QwtWidgetOverlay已经install了，因此无需再fig->installEventFilter(this);
    d->mFigOldMouseTracking = fig->hasMouseTracking();
    if (!d->mFigOldMouseTracking) {
        fig->setMouseTracking(true);
    }
    d->mFigOldHasHoverAttr = fig->testAttribute(Qt::WA_Hover);
    if (!d->mFigOldHasHoverAttr) {
        fig->setAttribute(Qt::WA_Hover, true);
    }

    QwtPlot* gca = fig->currentAxes();
    if (gca) {
        setActiveWidget(gca);
    } else {
        selectNextPlot();
        if (!currentActiveWidget()) {
            selectNextWidget();
        }
    }
}

QwtFigureWidgetOverlay::~QwtFigureWidgetOverlay()
{
    QwtFigure* fig = figure();
    Q_ASSERT(fig);
    // 如果fig原来没有mousetrack，设置回去
    if (!m_data->mFigOldMouseTracking) {
        fig->setMouseTracking(false);
    }
    if (!m_data->mFigOldHasHoverAttr) {
        fig->setAttribute(Qt::WA_Hover, false);
    }
}

QwtFigure* QwtFigureWidgetOverlay::figure() const
{
    return static_cast< QwtFigure* >(parent());
}

/**
 * @brief根据范围获取鼠标图标
 * @param rr
 * @return 鼠标图标
 */
Qt::CursorShape QwtFigureWidgetOverlay::controlTypeToCursor(QwtFigureWidgetOverlay::ControlType rr)
{
    switch (rr) {
    case ControlLineTop:
    case ControlLineBottom:
        return (Qt::SizeVerCursor);

    case ControlLineLeft:
    case ControlLineRight:
        return (Qt::SizeHorCursor);

    case ControlPointTopLeft:
    case ControlPointBottomRight:
        return (Qt::SizeFDiagCursor);

    case ControlPointTopRight:
    case ControlPointBottomLeft:
        return (Qt::SizeBDiagCursor);

    case Inner:
        return (Qt::SizeAllCursor);

    default:
        break;
    }
    return (Qt::ArrowCursor);
}

/**
 * @brief 根据点和矩形的关系，返回图标的样式
 * @param pos 点
 * @param region 矩形区域
 * @param err 允许误差
 * @return
 */
QwtFigureWidgetOverlay::ControlType QwtFigureWidgetOverlay::getPositionControlType(const QPoint& pos,
                                                                                   const QRect& region,
                                                                                   int err)
{
    if (!region.adjusted(-err, -err, err, err).contains(pos)) {
        return (OutSide);
    }
    if (pos.x() < (region.left() + err)) {
        if (pos.y() < region.top() + err) {
            return (ControlPointTopLeft);
        } else if (pos.y() > region.bottom() - err) {
            return (ControlPointBottomLeft);
        }
        return (ControlLineLeft);
    } else if (pos.x() > (region.right() - err)) {
        if (pos.y() < region.top() + err) {
            return (ControlPointTopRight);
        } else if (pos.y() > region.bottom() - err) {
            return (ControlPointBottomRight);
        }
        return (ControlLineRight);
    } else if (pos.y() < (region.top() + err)) {
        if (pos.x() < region.left() + err) {
            return (ControlPointTopLeft);
        } else if (pos.x() > region.right() - err) {
            return (ControlPointTopRight);
        }
        return (ControlLineTop);
    } else if (pos.y() > region.bottom() - err) {
        if (pos.x() < region.left() + err) {
            return (ControlPointBottomLeft);
        } else if (pos.x() > region.right() - err) {
            return (ControlPointBottomRight);
        }
        return (ControlLineBottom);
    }
    return (Inner);
}

/**
 * @brief 判断点是否在矩形区域的边缘
 * @param pos 点
 * @param region 矩形区域
 * @param err 允许误差
 * @return  如果符合边缘条件，返回true
 */
bool QwtFigureWidgetOverlay::isPointInRectEdget(const QPoint& pos, const QRect& region, int err)
{
    if (!region.adjusted(-err, -err, err, err).contains(pos)) {
        return (false);
    }
    if ((pos.x() < (region.left() - err)) && (pos.x() < (region.left() + err))) {
        return (true);
    } else if ((pos.x() > (region.right() - err)) && (pos.x() < (region.right() + err))) {
        return (true);
    } else if ((pos.y() > (region.top() - err)) && (pos.y() < (region.top() + err))) {
        return (true);
    } else if ((pos.y() > region.bottom() - err) && (pos.y() < region.bottom() + err)) {
        return (true);
    }
    return (false);
}

/**
 * @brief 设置内置功能的开关
 * @param flag
 * @param on
 * @sa QwtFigureWidgetOverlay::BuiltInFunctionsFlag
 */
void QwtFigureWidgetOverlay::setBuiltInFunctionsEnable(BuiltInFunctionsFlag flag, bool on)
{
    m_data->mFuntion.setFlag(flag, on);
}

/**
 * @brief 判断当前的功能开关
 * @param flag
 * @return
 */
bool QwtFigureWidgetOverlay::testBuiltInFunctions(BuiltInFunctionsFlag flag) const
{
    return m_data->mFuntion.testFlag(flag);
}

/**
 * @brief 判断当前是否有激活的窗口
 * @return
 */
bool QwtFigureWidgetOverlay::isHaveActiveWidget() const
{
    return (m_data->mActiveWidget != nullptr);
}

/**
 * @brief 设置边框的画笔
 * @param p
 */
void QwtFigureWidgetOverlay::setBorderPen(const QPen& p)
{
    m_data->mBorderPen = p;
}

/**
 * @brief 边框的画笔
 * @param p
 */
QPen QwtFigureWidgetOverlay::borderPen() const
{
    return m_data->mBorderPen;
}

/**
 * @brief 设置控制点的填充
 * @param b
 */
void QwtFigureWidgetOverlay::setControlPointBrush(const QBrush& b)
{
    m_data->mContorlPointBrush = b;
}

/**
 * @brief 控制点的填充
 * @param b
 */
QBrush QwtFigureWidgetOverlay::controlPointBrush() const
{
    return m_data->mContorlPointBrush;
}

/**
 * @brief 设置控制点尺寸
 * @param c
 */
void QwtFigureWidgetOverlay::setControlPointSize(const QSize& c)
{
    m_data->mControlPointSize = c;
}

/**
 * @brief 控制点尺寸
 * @default 8*8
 * @return
 */
QSize QwtFigureWidgetOverlay::controlPointSize() const
{
    return m_data->mControlPointSize;
}

/**
 * @brief 选择下一个窗口作为激活窗体
 * @param forward
 */
void QwtFigureWidgetOverlay::selectNextWidget(bool forward)
{
    QList< QWidget* > ws = figure()->findChildren< QWidget* >("", Qt::FindDirectChildrenOnly);
    ws.removeAll(this);
    if (ws.isEmpty()) {
        setActiveWidget(nullptr);
        return;
    }
    // 删除寄生轴
    auto it = std::remove_if(ws.begin(), ws.end(), [](QWidget* w) -> bool {
        if (QwtPlot* plot = qobject_cast< QwtPlot* >(w)) {
            if (plot->isParasitePlot()) {
                return true;
            }
        }
        return false;
    });
    if (it != ws.end()) {
        ws.erase(it, ws.end());  // 删除末尾的“无效”元素,也就是寄生轴都删除
    }
    // 这时ws都是可选中的窗口
    auto nextIt = qwtSelectNextIterator(ws.begin(), ws.end(), currentActiveWidget(), forward);
    setActiveWidget((nextIt != ws.end()) ? *nextIt : nullptr);
}

/**
 * @brief 选择下一个绘图作为激活窗体
 * @param forward
 */
void QwtFigureWidgetOverlay::selectNextPlot(bool forward)
{
    // 此函数不会返回寄生轴
    QList< QwtPlot* > ws = figure()->allAxes();
    if (ws.isEmpty()) {
        setActiveWidget(nullptr);
        return;
    }
    // 转换当前元素类型并获取下一个迭代器
    QwtPlot* current = qobject_cast< QwtPlot* >(currentActiveWidget());
    auto nextIt      = qwtSelectNextIterator(ws.begin(), ws.end(), current, forward);
    setActiveWidget((nextIt != ws.end()) ? *nextIt : nullptr);
}

/**
 * @brief 获取当前激活的窗体
 * @return
 */
QWidget* QwtFigureWidgetOverlay::currentActiveWidget() const
{
    return m_data->mActiveWidget;
}

/**
 * @brief 获取当前激活的绘图
 * @return
 */
QwtPlot* QwtFigureWidgetOverlay::currentActivePlot() const
{
    return qobject_cast< QwtPlot* >(m_data->mActiveWidget);
}

/**
 * @brief 显示占比数值
 * @param on
 */
void QwtFigureWidgetOverlay::showPercentText(bool on)
{
    m_data->mShowPrecentText = on;
    updateOverlay();
}

/**
 * @brief 设置当前激活的窗口
 * @param w 如果w和当前的activePlot一样，不做任何动作
 * @note 此函数会发射信号activeWidgetChanged
 * @sa activeWidgetChanged
 */
void QwtFigureWidgetOverlay::setActiveWidget(QWidget* w)
{
    QWidget* oldact = currentActiveWidget();
    if (w == oldact) {
        // 避免嵌套
        return;
    }
    m_data->mActiveWidget = w;
    updateOverlay();
    Q_EMIT activeWidgetChanged(oldact, w);
}

void QwtFigureWidgetOverlay::drawOverlay(QPainter* p) const
{
    if (isHaveActiveWidget()) {
        // 对于激活的窗口，绘制到四周的距离提示线
        p->save();
        if (m_data->mIsStartResize) {
            // 在resize状态，绘制控制线
            drawResizeingControlLine(p, m_data->mWillSetNormRect);
        } else {
            drawActiveWidget(p, currentActiveWidget());
        }
        p->restore();
    }
}

QRegion QwtFigureWidgetOverlay::maskHint() const
{
    return (figure()->rect());
}

/**
 * @brief DAFigureWidgetChartRubberbandEditOverlay::eventFilter
 *
 * 注意，鼠标移动事件在setMouseTracking(true)后，button永远是NoButton,需要配合press事件才能判断
 * @param obj
 * @param event
 * @return
 */
bool QwtFigureWidgetOverlay::eventFilter(QObject* obj, QEvent* event)
{
    if (isHaveActiveWidget()) {
        if (obj == figure()) {
            // 捕获DAChartWidget和DAFigure都是无法捕获正常鼠标移动的事件
            switch (event->type()) {
            case QEvent::MouseButtonPress: {
                QMouseEvent* me = static_cast< QMouseEvent* >(event);
                return (onMousePressedEvent(me));
            }

            case QEvent::MouseButtonRelease: {
                QMouseEvent* me = static_cast< QMouseEvent* >(event);
                return (onMouseReleaseEvent(me));
            }

            case QEvent::KeyPress: {
                QKeyEvent* ke = static_cast< QKeyEvent* >(event);
                return (onKeyPressedEvent(ke));
            }
            case QEvent::MouseMove: {
                QMouseEvent* e = static_cast< QMouseEvent* >(event);
                return (onMouseMoveEvent(e));
            }
            case QEvent::HoverMove: {
                QHoverEvent* e = static_cast< QHoverEvent* >(event);
                return (onHoverMoveEvent(e));
            }
            default:
                break;
            }
        }
    }

    // QwtWidgetOverlay也继承了eventFilter
    return (QwtWidgetOverlay::eventFilter(obj, event));
}

/**
 * @brief 绘制激活的窗口
 *
 * 通过继承此函数可改变绘制的方式，默认绘制会调用@ref drawControlLine 函数
 * @param painter
 * @param activeW
 */
void QwtFigureWidgetOverlay::drawActiveWidget(QPainter* painter, QWidget* activeW) const
{
    const QRect& chartRect      = activeW->frameGeometry();
    const QRectF& normalPercent = figure()->widgetNormRect(activeW);
    drawControlLine(painter, chartRect, normalPercent);
}

/**
 * @brief 绘制resize变换的橡皮筋控制线
 *
 * 通过继承此函数可改变绘制的方式，默认绘制会调用@ref drawControlLine 函数
 * @param painter
 * @param willSetNormRect
 */
void QwtFigureWidgetOverlay::drawResizeingControlLine(QPainter* painter, const QRectF& willSetNormRect) const
{
    QRect actualRect = figure()->calcActualRect(willSetNormRect);
    drawControlLine(painter, actualRect, willSetNormRect);
}

/**
 * @brief 绘制控制线
 * @param painter
 * @param actualRect 真实尺寸
 * @param normRect 归一化尺寸
 */
void QwtFigureWidgetOverlay::drawControlLine(QPainter* painter, const QRect& actualRect, const QRectF& normRect) const
{
    painter->setBrush(Qt::NoBrush);
    painter->setPen(m_data->mBorderPen);
    QRect edgetRect = actualRect.adjusted(-1, -1, 1, 1);

    // 绘制矩形边框
    painter->drawRect(edgetRect);
    // 绘制边框到figure四周
    QPen linePen(m_data->mBorderPen);

    linePen.setStyle(Qt::DotLine);
    painter->setPen(linePen);
    QPoint center = actualRect.center();

    painter->drawLine(center.x(), 0, center.x(), actualRect.top());            // top
    painter->drawLine(center.x(), actualRect.bottom(), center.x(), height());  // bottom
    painter->drawLine(0, center.y(), actualRect.left(), center.y());           // left
    painter->drawLine(actualRect.right(), center.y(), width(), center.y());    // right
    // 绘制顶部数据
    QFontMetrics fm = painter->fontMetrics();
    // top text
    QString percentText = QString::number(normRect.y() * 100, 'g', 2) + "%";
    QRectF textRect     = fm.boundingRect(percentText);
    textRect.moveTopLeft(QPoint(center.x(), 0));
    painter->drawText(textRect, Qt::AlignCenter, percentText);
    // left
    percentText = QString::number(normRect.x() * 100, 'g', 2) + "%";
    textRect    = fm.boundingRect(percentText);
    textRect.moveBottomLeft(QPoint(0, center.y()));
    painter->drawText(textRect, Qt::AlignCenter, percentText);

    //    painter->drawText(QPointF(0, actualRect.y()), QString::number(percent.x(), 'g', 2));
    // 绘制四个角落
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_data->mContorlPointBrush);
    QRect connerRect(0, 0, m_data->mControlPointSize.width(), m_data->mControlPointSize.height());
    QPoint offset = QPoint(m_data->mControlPointSize.width() / 2, m_data->mControlPointSize.height() / 2);
    connerRect.moveTo(edgetRect.topLeft() - offset);
    painter->drawRect(connerRect);
    connerRect.moveTo(edgetRect.topRight() - offset);
    painter->drawRect(connerRect);
    connerRect.moveTo(edgetRect.bottomLeft() - offset);
    painter->drawRect(connerRect);
    connerRect.moveTo(edgetRect.bottomRight() - offset);
    painter->drawRect(connerRect);
}

bool QwtFigureWidgetOverlay::onMouseMoveEvent(QMouseEvent* me)
{
    Q_UNUSED(me);
    QWT_D(d);
    if (d->mActiveWidget) {
        if (d->mIsStartResize) {
            return (true);  // 托管所有的鼠标事件
        }
    }
    return (false);
}

bool QwtFigureWidgetOverlay::onMouseReleaseEvent(QMouseEvent* me)
{
#if QwtFigureWidgetOverlay_DEBUG_PRINT
    qDebug() << "QwtFigureWidgetOverlay::onMouseReleaseEvent" << me->pos();
#endif
    if (Qt::LeftButton == me->button()) {
        QWT_D(d);
        if (!d->mWillSetNormRect.isValid()) {
            // 这种就是点击一下
            d->mIsStartResize = false;
            return (true);  // 托管所有的鼠标事件
        }
        if (d->mIsStartResize) {
            d->mIsStartResize = false;
            if (d->mActiveWidget) {
                QwtFigure* fig = figure();
                Q_ASSERT(fig);
                // QRect newGeometry = d->mActiveWidget->geometry();
                fig->setWidgetNormPos(d->mActiveWidget, d->mWillSetNormRect);
#if QwtFigureWidgetOverlay_DEBUG_PRINT
                qDebug() << "fig->setWidgetNormPos(d->mActiveWidget, " << d->mWillSetNormRect << ")";
#endif
                Q_EMIT widgetNormGeometryChanged(d->mActiveWidget, d->mOldNormRect, d->mWillSetNormRect);
                return (true);  // 这里把消息截取不传递下去
            }
        }
    }
    return (true);  // 托管所有的鼠标事件
}

bool QwtFigureWidgetOverlay::onMousePressedEvent(QMouseEvent* me)
{
    if (me->button() != Qt::LeftButton) {  // 只关心左键
        return false;
    }

    // 左键点击
    const QList< QwtPlot* > plots = figure()->allAxes(true);  // 传入true，按z序列最高到低排序
    if (plots.empty()) {
        return false;
    }

    // 注意，hitplot有可能是nullptr，因为点击变换边缘时，会判断为外围
    QWidget* hitPlot  = nullptr;
    const QPoint gPos = qwt::compat::eventPos(me);
    for (QWidget* w : plots) {
        if (w->frameGeometry().contains(gPos, true)) {
            hitPlot = w;
            break;
        }
    }

    QWT_D(d);
    // 如果还没激活任何窗口，或者点到新窗口，直接切换激活
    // 注意，这里不能直接判断有hitPlot就切换activeWidget，因为实际在变换的时候，点击的位置会找active的外围一点
    // 通过getPositionControlType可以真实反映出是否超过了activeWidget的变换范围
    if (!d->mActiveWidget) {
        if (hitPlot) {
            setActiveWidget(hitPlot);
            updateOverlay();
            return true;
        }
        return false;
    }

    // 有激活窗口，判断点击位置
    ControlType ct = getPositionControlType(qwt::compat::eventPos(me), d->mActiveWidget->frameGeometry(), 4);
    if (OutSide == ct) {
        // 点击在空白处
        if (hitPlot) {
            setActiveWidget(hitPlot);
            updateOverlay();
            return true;
        }
        return false;
    }
    // 点击了变换区域，就执行变换
    QwtFigure* fig = figure();
    Q_ASSERT(fig);
    d->mOldNormRect       = fig->widgetNormRect(d->mActiveWidget);
    d->mLastMousePressPos = qwt::compat::eventPos(me);
    d->mIsStartResize     = true;
    d->mControlType       = ct;
    d->mWillSetNormRect   = QRectF();
    return (true);  // 这里把消息截取不传递下去
}

bool QwtFigureWidgetOverlay::onHoverMoveEvent(QHoverEvent* me)
{
    //! 注意，不要在onMouseMoveEvent进行处理，因为鼠标移动到子窗体后，
    //! onMouseMoveEvent不会触发，但onHoverMoveEvent还会继续触发
    QWT_D(d);
    QWidget* activeW = d->mActiveWidget;
    if (!activeW) {
        return false;
    }
    QwtFigure* fig = figure();
    Q_ASSERT(fig);

    if (d->mIsStartResize) {
        // 开始resize（鼠标按下左键后触发为true）
        const QRectF& oldNormRect = d->mOldNormRect;
        QPoint offset             = qwt::compat::eventPos(me) - d->mLastMousePressPos;
        switch (d->mControlType) {
        case ControlLineTop: {
            //  计算offset.y()占高度比例
            qreal dh = static_cast< qreal >(offset.y()) / fig->height();
            // 要使用figure计算归一化坐标
            QRectF normRect = oldNormRect;
            normRect.setY(oldNormRect.y() + dh);
            normRect.setHeight(oldNormRect.height() - dh);
            d->mWillSetNormRect = normRect;
            break;
        }

        case ControlLineBottom: {
            //  计算offset.y()占高度比例
            qreal dh = static_cast< qreal >(offset.y()) / fig->height();
            // 要使用figure计算归一化坐标
            QRectF normRect = oldNormRect;
            normRect.setHeight(oldNormRect.height() + dh);
            d->mWillSetNormRect = normRect;
            break;
        }

        case ControlLineLeft: {
            //  计算offset.x()占宽度比例
            qreal dw        = static_cast< qreal >(offset.x()) / fig->width();
            QRectF normRect = oldNormRect;
            normRect.setX(oldNormRect.x() + dw);
            normRect.setWidth(oldNormRect.width() - dw);
            d->mWillSetNormRect = normRect;
            break;
        }

        case ControlLineRight: {
            //  计算offset.x()占宽度比例
            qreal dw        = static_cast< qreal >(offset.x()) / fig->width();
            QRectF normRect = oldNormRect;
            normRect.setWidth(oldNormRect.width() + dw);
            d->mWillSetNormRect = normRect;
            break;
        }

        case ControlPointTopLeft: {
            qreal dh = static_cast< qreal >(offset.y()) / fig->height();
            qreal dw = static_cast< qreal >(offset.x()) / fig->width();

            QRectF normRect = oldNormRect;
            normRect.setX(oldNormRect.x() + dw);
            normRect.setY(oldNormRect.y() + dh);
            normRect.setWidth(oldNormRect.width() - dw);
            normRect.setHeight(oldNormRect.height() - dh);

            d->mWillSetNormRect = normRect;
            break;
        }

        case ControlPointTopRight: {
            qreal dh = static_cast< qreal >(offset.y()) / fig->height();
            qreal dw = static_cast< qreal >(offset.x()) / fig->width();

            QRectF normRect = oldNormRect;
            normRect.setY(oldNormRect.y() + dh);
            normRect.setWidth(oldNormRect.width() + dw);
            normRect.setHeight(oldNormRect.height() - dh);

            d->mWillSetNormRect = normRect;
            break;
        }

        case ControlPointBottomLeft: {
            qreal dh = static_cast< qreal >(offset.y()) / fig->height();
            qreal dw = static_cast< qreal >(offset.x()) / fig->width();

            QRectF normRect = oldNormRect;
            normRect.setX(oldNormRect.x() + dw);
            normRect.setWidth(oldNormRect.width() - dw);
            normRect.setHeight(oldNormRect.height() + dh);

            d->mWillSetNormRect = normRect;
            break;
        }

        case ControlPointBottomRight: {
            qreal dh = static_cast< qreal >(offset.y()) / fig->height();
            qreal dw = static_cast< qreal >(offset.x()) / fig->width();

            QRectF normRect = oldNormRect;
            normRect.setWidth(oldNormRect.width() + dw);
            normRect.setHeight(oldNormRect.height() + dh);

            d->mWillSetNormRect = normRect;
            break;
        }

        case Inner: {
            qreal dh = static_cast< qreal >(offset.y()) / fig->height();
            qreal dw = static_cast< qreal >(offset.x()) / fig->width();

            QRectF normRect     = oldNormRect.adjusted(dw, dh, dw, dh);
            d->mWillSetNormRect = normRect;
            break;
        }

        default:
            return (false);
        }
        updateOverlay();
        return (true);  // 这里把消息截取不传递下去
    } else {
        ControlType ct = getPositionControlType(qwt::compat::eventPos(me), activeW->frameGeometry(), 4);
        if (d->mControlType != ct) {
            // 说明控制点变更
            Qt::CursorShape cur = controlTypeToCursor(ct);
            figure()->setCursor(cur);
            d->mControlType = ct;
        }
    }

    return (true);  // 托管所有的鼠标事件
}

bool QwtFigureWidgetOverlay::onKeyPressedEvent(QKeyEvent* ke)
{
    switch (ke->key()) {
    case Qt::Key_Return: {
        selectNextWidget(true);
    } break;

    case Qt::Key_Up:
    case Qt::Key_Left:
        selectNextWidget(true);
        break;

    case Qt::Key_Right:
    case Qt::Key_Down:
        selectNextWidget(false);
        break;

    default:
        return (false);
    }
    return (true);  // 这里把消息截取不传递下去
}
