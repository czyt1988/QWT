#include "qwt_figure_widget_overlay.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QHash>
#include <QCoreApplication>
#include <QDebug>
#include <QPainter>
// std
#include <algorithm>
// qwt
#include "qwt_algorithm.hpp"
#include "qwt_figure.h"
#include "qwt_plot.h"
#include "qwt_qt5qt6_compat.hpp"

class QwtFigureWidgetOverlay::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtFigureWidgetOverlay)
public:
    PrivateData(QwtFigureWidgetOverlay* p);

public:
    QPoint mLastMousePressPos { 0, 0 };      ///< 记录最后一次窗口移动的坐标
    QBrush mContorlPointBrush { Qt::blue };  ///< 绘制chart2d在编辑模式下控制点的画刷
    QPen mBorderPen { Qt::blue };            ///< 绘制chart2d在编辑模式下的画笔
    bool mIsStartResize { false };           ///< 标定开始进行缩放
    QWidget* mActiveWidget { nullptr };      /// 标定当前激活的窗口，如果没有就为nullptr
    QRect mOldGeometry;                      ///< 保存旧的窗口位置，用于redo/undo
    QSize mControlPointSize { 8, 8 };        ///< 控制点大小
    QwtFigureWidgetOverlay::ControlType mControlType { QwtFigureWidgetOverlay::OutSide };  ///< 记录当前缩放窗口的位置情况

    bool mFigOldMouseTracking { false };  /// 记录原来fig是否设置了mousetrack
    bool mFigOldHasHoverAttr { false };   ///< 记录原来的figure是否含有Hover属性
    bool mShowPrecentText { true };       ///< 显示占比文字
};

QwtFigureWidgetOverlay::PrivateData::PrivateData(QwtFigureWidgetOverlay* p) : q_ptr(p)
{
}

//----------------------------------------------------
// QwtFigureWidgetOverlay
//----------------------------------------------------

QwtFigureWidgetOverlay::QwtFigureWidgetOverlay(QwtFigure* fig) : QwtWidgetOverlay(fig), QWT_PIMPL_CONSTRUCT
{
}

QwtFigureWidgetOverlay::~QwtFigureWidgetOverlay()
{
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
        drawActiveWidget(p, currentActiveWidget());
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

void QwtFigureWidgetOverlay::drawActiveWidget(QPainter* painter, QWidget* activeW) const
{
    const QRect& chartRect      = activeW->frameGeometry();
    const QRectF& normalPercent = figure()->widgetNormRect(activeW);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(m_data->mBorderPen);
    QRect edgetRect = chartRect.adjusted(-1, -1, 1, 1);

    // 绘制矩形边框
    painter->drawRect(edgetRect);
    // 绘制边框到figure四周
    QPen linePen(m_data->mBorderPen);

    linePen.setStyle(Qt::DotLine);
    painter->setPen(linePen);
    QPoint center = chartRect.center();

    painter->drawLine(center.x(), 0, center.x(), chartRect.top());            // top
    painter->drawLine(center.x(), chartRect.bottom(), center.x(), height());  // bottom
    painter->drawLine(0, center.y(), chartRect.left(), center.y());           // left
    painter->drawLine(chartRect.right(), center.y(), width(), center.y());    // right
    // 绘制顶部数据
    QFontMetrics fm = painter->fontMetrics();
    // top text
    QString percentText = QString::number(normalPercent.y() * 100, 'g', 2) + "%";
    QRectF textRect     = fm.boundingRect(percentText);
    textRect.moveTopLeft(QPoint(center.x(), 0));
    painter->drawText(textRect, Qt::AlignCenter, percentText);
    // left
    percentText = QString::number(normalPercent.x() * 100, 'g', 2) + "%";
    textRect    = fm.boundingRect(percentText);
    textRect.moveBottomLeft(QPoint(0, center.y()));
    painter->drawText(textRect, Qt::AlignCenter, percentText);

    //    painter->drawText(QPointF(0, chartRect.y()), QString::number(percent.x(), 'g', 2));
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

bool QwtFigureWidgetOverlay::onMouseReleaseEvent(QMouseEvent* me)
{
    if (Qt::LeftButton == me->button()) {
        if (m_data->mIsStartResize) {
            m_data->mIsStartResize = false;
            if (m_data->mActiveWidget) {
                QRect newGeometry = m_data->mActiveWidget->geometry();
                Q_EMIT widgetGeometryChanged(m_data->mActiveWidget, m_data->mOldGeometry, newGeometry);
                return (true);  // 这里把消息截取不传递下去
            }
        }
    }
    return (true);  // 托管所有的鼠标事件
}

bool QwtFigureWidgetOverlay::onMousePressedEvent(QMouseEvent* me)
{
    if (Qt::LeftButton == me->button()) {
        // 左键点击
        QList< QwtPlot* > orderws = figure()->allAxes();
        for (auto ite = orderws.begin(); ite != orderws.end(); ++ite) {
            // 点击到了其他窗体
            if ((*ite)->geometry().contains(qwt::compat::eventPos(me))) {
                setActiveWidget(*ite);
            }
        }
        ControlType ct = getPositionControlType(qwt::compat::eventPos(me), m_data->mActiveWidget->frameGeometry(), 4);
        if (OutSide == ct) {
            // 如果点击了外部，那么久尝试变更激活窗口
            QList< QwtPlot* > ws = figure()->allAxes();
            for (QWidget* w : qAsConst(ws)) {
                if (w->frameGeometry().contains(qwt::compat::eventPos(me), true)) {
                    if (m_data->mActiveWidget != w) {
                        setActiveWidget(w);
                        updateOverlay();
                        return (true);  // 这里把消息截取不传递下去
                    }
                }
            }
        } else {
            // 点击了其他区域，就执行变换
            m_data->mOldGeometry       = m_data->mActiveWidget->geometry();
            m_data->mLastMousePressPos = qwt::compat::eventPos(me);
            m_data->mIsStartResize     = true;
            m_data->mControlType       = ct;
            return (true);  // 这里把消息截取不传递下去
        }

        // 没有点击到任何的地方就
    }
    return (true);  // 托管所有的鼠标事件
}

bool QwtFigureWidgetOverlay::onHoverMoveEvent(QHoverEvent* me)
{
    //! 注意，不要在onMouseMoveEvent进行处理，因为鼠标移动到子窗体后，
    //! onMouseMoveEvent不会触发，但onHoverMoveEvent还会继续触发
    QWidget* activeW = m_data->mActiveWidget;
    if (activeW) {
        if (m_data->mIsStartResize) {
            // 开始resize（鼠标按下左键后触发为true）
            QRect geoRect = m_data->mOldGeometry;
            switch (m_data->mControlType) {
            case ControlLineTop: {
                QPoint offset = qwt::compat::eventPos(me) - m_data->mLastMousePressPos;
                geoRect.adjust(0, offset.y(), 0, 0);
                activeW->setGeometry(geoRect);
                break;
            }

            case ControlLineBottom: {
                int resultY = qwt::compat::eventY(me);
                geoRect.adjust(0, 0, 0, resultY - geoRect.bottom());
                activeW->setGeometry(geoRect);
                break;
            }

            case ControlLineLeft: {
                int resultX = qwt::compat::eventX(me);
                geoRect.adjust(resultX - geoRect.left(), 0, 0, 0);
                activeW->setGeometry(geoRect);
                break;
            }

            case ControlLineRight: {
                int resultX = qwt::compat::eventX(me);
                geoRect.adjust(0, 0, resultX - geoRect.right(), 0);
                activeW->setGeometry(geoRect);
                break;
            }

            case ControlPointTopLeft: {
                geoRect.adjust(qwt::compat::eventX(me) - geoRect.left(), qwt::compat::eventY(me) - geoRect.top(), 0, 0);
                activeW->setGeometry(geoRect);
                break;
            }

            case ControlPointTopRight: {
                geoRect.adjust(0, qwt::compat::eventY(me) - geoRect.top(), qwt::compat::eventX(me) - geoRect.right(), 0);
                activeW->setGeometry(geoRect);
                break;
            }

            case ControlPointBottomLeft: {
                geoRect.adjust(qwt::compat::eventX(me) - geoRect.left(), 0, 0, qwt::compat::eventY(me) - geoRect.bottom());
                activeW->setGeometry(geoRect);
                break;
            }

            case ControlPointBottomRight: {
                geoRect.adjust(0, 0, qwt::compat::eventX(me) - geoRect.right(), qwt::compat::eventY(me) - geoRect.bottom());
                activeW->setGeometry(geoRect);
                break;
            }

            case Inner: {
                QPoint offset = qwt::compat::eventPos(me) - m_data->mLastMousePressPos;
                geoRect.moveTo(m_data->mOldGeometry.topLeft() + offset);
                activeW->setGeometry(geoRect);
                break;
            }

            default:
                return (false);
            }
            updateOverlay();
            return (true);  // 这里把消息截取不传递下去
        } else {
            ControlType ct = getPositionControlType(qwt::compat::eventPos(me), activeW->frameGeometry(), 4);
            if (m_data->mControlType != ct) {
                // 说明控制点变更
                Qt::CursorShape cur = controlTypeToCursor(ct);
                figure()->setCursor(cur);
                m_data->mControlType = ct;
            }
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
