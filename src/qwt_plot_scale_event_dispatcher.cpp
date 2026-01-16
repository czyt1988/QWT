#include "qwt_plot_scale_event_dispatcher.h"

// qt
#include <QMouseEvent>
#include <QApplication>
#include <QDebug>
#include <QTimer>
// qwt
#include "qwt_plot.h"
#include "qwt_scale_widget.h"
#include "qwt_qt5qt6_compat.hpp"

class QwtPlotScaleEventDispatcher::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotScaleEventDispatcher)
public:
    PrivateData(QwtPlotScaleEventDispatcher* p);
    // 缓存结构
    struct ScaleCache
    {
        QwtScaleWidget* scaleWidget;
        QRect eventRect;    // 在基准绘图坐标系中的矩形，也就是当前事件对象所在的坐标系
        QwtPlot* basePlot;  // 基准绘图，也就是当前事件过滤器对应的绘图
        bool isValid;

        ScaleCache() : scaleWidget(nullptr), basePlot(nullptr), isValid(false)
        {
        }
        ScaleCache(QwtScaleWidget* scale, QwtPlot* base) : scaleWidget(scale), basePlot(base), isValid(true)
        {
            updateRect();
        }

        void updateRect()
        {
            if (scaleWidget) {
                QRect localRect      = scaleWidget->scaleRect();
                QPoint globalTopLeft = scaleWidget->mapToGlobal(localRect.topLeft());
                eventRect            = QRect(basePlot->mapFromGlobal(globalTopLeft), localRect.size());
                isValid              = true;
            } else {
                isValid = false;
            }
        }

        bool contains(const QPoint& pos) const
        {
            return isValid && eventRect.contains(pos);
        }
    };
    ScaleCache findScaleCache(const QPoint& pos);
    // 重置状态
    void resetRecord();
    void updateCursorForMousePress(QwtPlot* p);
    void updateCursorForHover(QwtPlot* p, bool isOnScale);
    void updateCursorForMouseRelease(QwtPlot* p, Qt::MouseButton button, bool isOnScale);
    static QwtPlot* scalePlot(QwtScaleWidget* scaleWidget);

public:
    bool isEnable { true };
    QwtPlot* bindedPlot { nullptr };  ///< 绑定的绘图
    QwtPlot* currentPlot { nullptr };
    QwtScaleWidget* currentScale { nullptr };            ///< 当前正在操作的 scale widget
    QwtAxisId currentAxisId { QwtAxis::AxisPositions };  ///< 当前坐标轴id
    QPoint lastMousePos;                                 ///< 上一次鼠标位置（用于拖拽计算）
    bool isMousePressed { false };                       ///< 记录鼠标是否按下
    // 缓存相关
    QList< ScaleCache > scaleCaches;
    bool cacheDirty { true };   ///< 缓存是否需要重建
    double zoomFactor { 1.5 };  ///< 缩放比例
};

QwtPlotScaleEventDispatcher::PrivateData::PrivateData(QwtPlotScaleEventDispatcher* p) : q_ptr(p)
{
}

QwtPlotScaleEventDispatcher::PrivateData::ScaleCache QwtPlotScaleEventDispatcher::PrivateData::findScaleCache(const QPoint& pos)
{
    for (const auto& cache : qAsConst(this->scaleCaches)) {
        if (cache.contains(pos)) {
            return cache;
        }
    }
    return ScaleCache();
}

void QwtPlotScaleEventDispatcher::PrivateData::resetRecord()
{
    currentScale   = nullptr;
    currentPlot    = nullptr;
    isMousePressed = false;
    currentAxisId  = QwtAxis::AxisPositions;
}

void QwtPlotScaleEventDispatcher::PrivateData::updateCursorForMousePress(QwtPlot* p)
{
    if (!currentScale) {
        p->unsetCursor();
    }
    if (currentScale->testBuildinActions(QwtScaleWidget::ActionClickPan)) {
        p->setCursor(Qt::ClosedHandCursor);
    } else {
        p->unsetCursor();
    }
}

void QwtPlotScaleEventDispatcher::PrivateData::updateCursorForHover(QwtPlot* p, bool isOnScale)
{
    if (!currentScale) {
        p->unsetCursor();
    }
    if (currentScale->testBuildinActions(QwtScaleWidget::ActionClickPan)) {
        if (currentScale->isSelected()) {
            if (isMousePressed) {
                p->setCursor(Qt::ClosedHandCursor);
            } else {
                if (isOnScale) {
                    p->setCursor(Qt::OpenHandCursor);
                } else {
                    p->unsetCursor();
                }
            }
        } else {
            p->unsetCursor();
        }
    }
}

void QwtPlotScaleEventDispatcher::PrivateData::updateCursorForMouseRelease(QwtPlot* p, Qt::MouseButton button, bool isOnScale)
{
    if (button == Qt::LeftButton) {
        if (currentScale && currentScale->testBuildinActions(QwtScaleWidget::ActionClickPan)) {
            p->setCursor(Qt::OpenHandCursor);
        } else {
            p->unsetCursor();
        }
    } else if (button == Qt::RightButton) {
        if (isOnScale) {
            p->unsetCursor();
        }
    }
}

QwtPlot* QwtPlotScaleEventDispatcher::PrivateData::scalePlot(QwtScaleWidget* scaleWidget)
{
    return qobject_cast< QwtPlot* >(scaleWidget->parent());
}

//----------------------------------------------------
// QwtParasitePlotEventFilter
//----------------------------------------------------

QwtPlotScaleEventDispatcher::QwtPlotScaleEventDispatcher(QwtPlot* plot, QObject* par)
    : QObject(par), QWT_PIMPL_CONSTRUCT
{
    m_data->bindedPlot = plot;
    if (!plot->hasMouseTracking()) {
        plot->setMouseTracking(true);
    }
    rebuildCache();
}

QwtPlotScaleEventDispatcher::~QwtPlotScaleEventDispatcher()
{
}

void QwtPlotScaleEventDispatcher::setEnable(bool on)
{
    m_data->isEnable = on;
}

bool QwtPlotScaleEventDispatcher::isEnable() const
{
    return m_data->isEnable;
}

/**
 * @brief 获取 QwtScaleWidget 对应的轴 ID
 * @param plot QwtPlot 指针
 * @param scaleWidget 要查找的 QwtScaleWidget
 * @return 轴 ID，如果找不到返回 QwtAxis::AxisPositions
 */
QwtAxisId QwtPlotScaleEventDispatcher::findAxisIdByScaleWidget(const QwtPlot* plot, const QwtScaleWidget* scaleWidget)
{
    if (!plot || !scaleWidget) {
        return QwtAxis::AxisPositions;
    }

    // 遍历所有可能的轴
    for (int axis = 0; axis < QwtAxis::AxisPositions; axis++) {
        if (plot->axisWidget(axis) == scaleWidget) {
            return axis;
        }
    }

    return QwtAxis::AxisPositions;  // 未找到
}

/**
 * @brief 重建所有缓存数据，在有新的寄生绘图添加时，都需要调用此函数，把绘图的坐标轴缓存
 */
void QwtPlotScaleEventDispatcher::rebuildCache()
{
    QWT_D(d);
    d->scaleCaches.clear();
    const QList< QwtPlot* > plotslist = d->bindedPlot->plotList();
    for (QwtPlot* plot : plotslist) {
        // 检查该绘图的所有 scale widget
        for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; ++axisPos) {
            QwtAxisId axisId(axisPos);
            QwtScaleWidget* scale = plot->axisWidget(axisId);
            if (scale && plot->isAxisVisible(axisId)) {
                d->scaleCaches.append(PrivateData::ScaleCache(scale, d->bindedPlot));
            }
        }
    }
    d->cacheDirty = false;
}

/**
 * @brief 更新缓存，必要时会调用rebuildCache进行重建
 */
void QwtPlotScaleEventDispatcher::updateCache()
{
    if (m_data->cacheDirty) {
        rebuildCache();
    } else {
        // 只更新现有的缓存项
        for (PrivateData::ScaleCache& cache : m_data->scaleCaches) {
            if (cache.scaleWidget) {
                cache.updateRect();
            }
        }
    }
}

bool QwtPlotScaleEventDispatcher::eventFilter(QObject* obj, QEvent* e)
{
    if (!m_data->isEnable) {
        return false;
    }
    QwtPlot* plot = qobject_cast< QwtPlot* >(obj);
    if (!plot) {
        return false;
    }
    switch (e->type()) {
    case QEvent::Resize:
    case QEvent::LayoutRequest:
    case QEvent::Polish: {
        // 处理可能影响缓存的事件
        m_data->cacheDirty = true;
        updateCache();
        break;
    }
    case QEvent::MouseButtonPress:
        return handleMousePress(plot, static_cast< QMouseEvent* >(e));
    case QEvent::MouseMove:
        return handleMouseMove(plot, static_cast< QMouseEvent* >(e));
    case QEvent::MouseButtonRelease:
        return handleMouseRelease(plot, static_cast< QMouseEvent* >(e));
    case QEvent::Wheel:
        return handleWheelEvent(plot, static_cast< QWheelEvent* >(e));
    default:
        break;
    }
    return QObject::eventFilter(obj, e);
}

bool QwtPlotScaleEventDispatcher::handleMousePress(QwtPlot* bindPlot, QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) {
        return false;
    }
    QWT_D(d);

    // 检查当前绘图是否有 scale widget 应该处理此事件
    QwtScaleWidget* targetScale = findTargetOnScale(e->pos());
    if (!targetScale) {
        // 没有在scale上
        if (d->currentScale) {
            // 说明已经有选中了scale，则取消选中
            d->currentScale->setSelected(false);
        }
        d->resetRecord();
        return false;
    }
    // 这里说明点击在了scale上面，首先看看点击的scale和currentScale是否一样，不一样则把currentScale的选中状态取消

    if (targetScale != d->currentScale) {
        if (d->currentScale) {
            d->currentScale->setSelected(false);
        }
        d->currentScale  = targetScale;
        d->currentPlot   = PrivateData::scalePlot(targetScale);
        d->currentAxisId = findAxisIdByScaleWidget(d->currentPlot, targetScale);
        // 记录当前选中的内容
        d->currentScale->setSelected(true);
    }
    d->lastMousePos   = e->pos();
    d->isMousePressed = true;

    d->updateCursorForMousePress(bindPlot);

    return true;  // 我们已经处理了事件转发逻辑
}

bool QwtPlotScaleEventDispatcher::handleMouseMove(QwtPlot* bindPlot, QMouseEvent* e)
{
    // 检查当前绘图是否有 scale widget 应该处理此事件
    QWT_D(d);
    if (d->currentScale) {
        if (d->isMousePressed) {
            // 说明当前已经选中了一个scale,且鼠标是按下状态，这个时候处于移动过程中
            if (d->currentScale->testBuildinActions(QwtScaleWidget::ActionClickPan)) {

                d->updateCursorForHover(bindPlot, true);
                const QPoint delta   = e->pos() - d->lastMousePos;
                const int deltaPixel = QwtAxis::isYAxis(d->currentAxisId) ? delta.y() : delta.x();
                if (deltaPixel != 0) {
                    d->currentPlot->panAxis(d->currentAxisId, deltaPixel);
                    // 需要手动触发重绘
                    d->currentPlot->replotAll();
                    d->lastMousePos = e->pos();
                    return true;
                }
            }
        } else {
            // 之前已经选中一个scael
            // 但没有按下
            QwtScaleWidget* targetScale = findTargetOnScale(e->pos());
            if (targetScale == d->currentScale) {
                d->updateCursorForHover(bindPlot, true);
            } else {
                bindPlot->unsetCursor();
            }
        }
    } else {
        // 没有选中任何scale，正常处理
        bindPlot->unsetCursor();
    }
    return false;
}

bool QwtPlotScaleEventDispatcher::handleMouseRelease(QwtPlot* bindPlot, QMouseEvent* e)
{
    QWT_D(d);
    Q_UNUSED(bindPlot);

    QwtScaleWidget* targetScale = findTargetOnScale(e->pos());
    d->updateCursorForMouseRelease(bindPlot, e->button(), targetScale == d->currentScale);
    if (e->button() == Qt::RightButton) {
        if (!targetScale) {
            return false;
        }
        // 右键：如果在当前scale区域则处理，否则忽略
        if (d->currentScale == targetScale) {
            d->isMousePressed = false;
            d->currentScale->setSelected(false);  // 此函数做了变更检查，重复设置不会重复触发信号
            return true;
        }
        return false;
    }

    if (e->button() == Qt::LeftButton) {
        d->isMousePressed = false;
        // 左键：只有之前按下且在当前scale区域才处理,这里不要联合onMyScale判断，拖曳出去 绘图区域就识别不了
        return (targetScale != nullptr);  // targetScale不为空时返回true代表截断事件，这样上层的事件才能处理，例如QwtFigureWidgetOverlay
    }
    return false;
}

bool QwtPlotScaleEventDispatcher::handleWheelEvent(QwtPlot* bindPlot, QWheelEvent* e)
{
    QWT_D(d);
    Q_UNUSED(bindPlot);
    // 检查当前绘图是否有 scale widget 应该处理此事件
    QwtScaleWidget* targetScale = findTargetOnScale(qwt::compat::eventPos(e));
    if (d->currentScale && d->currentScale == targetScale) {
        if (d->currentScale->testBuildinActions(QwtScaleWidget::ActionWheelZoom)) {
            QPoint p = e->globalPosition().toPoint();
            p        = d->currentScale->mapFromGlobal(p);
            if (qwt::compat::wheelEventDelta(e) > 0) {
                d->currentPlot->zoomAxis(d->currentAxisId, d->zoomFactor, p);
            } else {
                d->currentPlot->zoomAxis(d->currentAxisId, 1.0 / d->zoomFactor, p);
            }
            d->currentPlot->replot();
            return true;
        }
    }
    return false;
}

QwtScaleWidget* QwtPlotScaleEventDispatcher::findTargetOnScale(const QPoint& pos)
{
    // 按层级从高到低检查（缓存列表已经按层级排序）
    for (const auto& cache : qAsConst(m_data->scaleCaches)) {
        if (cache.contains(pos)) {
            return cache.scaleWidget;
        }
    }
    return nullptr;
}
