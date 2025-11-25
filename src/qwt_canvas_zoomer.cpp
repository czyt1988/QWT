/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/

#include "qwt_canvas_zoomer.h"
#include "qwt_plot.h"
#include "qwt_scale_div.h"
#include "qwt_scale_map.h"
#include "qwt_picker_machine.h"

#include <qstack.h>
#include <limits>

// QwtCanvasZoomState 实现
QwtCanvasZoomState QwtCanvasZoomState::fromPlot(const QwtPlot* plot)
{
    QwtCanvasZoomState state;
    if (!plot)
        return state;

    // 获取四个坐标轴的当前范围
    if (plot->isAxisVisible(QwtAxis::XBottom)) {
        const QwtScaleDiv& scaleDiv = plot->axisScaleDiv(QwtAxis::XBottom);
        state.xBottom               = QwtInterval(scaleDiv.lowerBound(), scaleDiv.upperBound());
    }

    if (plot->isAxisVisible(QwtAxis::XTop)) {
        const QwtScaleDiv& scaleDiv = plot->axisScaleDiv(QwtAxis::XTop);
        state.xTop                  = QwtInterval(scaleDiv.lowerBound(), scaleDiv.upperBound());
    }

    if (plot->isAxisVisible(QwtAxis::YLeft)) {
        const QwtScaleDiv& scaleDiv = plot->axisScaleDiv(QwtAxis::YLeft);
        state.yLeft                 = QwtInterval(scaleDiv.lowerBound(), scaleDiv.upperBound());
    }

    if (plot->isAxisVisible(QwtAxis::YRight)) {
        const QwtScaleDiv& scaleDiv = plot->axisScaleDiv(QwtAxis::YRight);
        state.yRight                = QwtInterval(scaleDiv.lowerBound(), scaleDiv.upperBound());
    }

    return state;
}

void QwtCanvasZoomState::applyToPlot(QwtPlot* plot) const
{
    if (!plot)
        return;

    // 应用四个坐标轴的范围
    if (plot->isAxisVisible(QwtAxis::XBottom) && xBottom.isValid()) {
        plot->setAxisScale(QwtAxis::XBottom, xBottom.minValue(), xBottom.maxValue());
    }

    if (plot->isAxisVisible(QwtAxis::XTop) && xTop.isValid()) {
        plot->setAxisScale(QwtAxis::XTop, xTop.minValue(), xTop.maxValue());
    }

    if (plot->isAxisVisible(QwtAxis::YLeft) && yLeft.isValid()) {
        plot->setAxisScale(QwtAxis::YLeft, yLeft.minValue(), yLeft.maxValue());
    }

    if (plot->isAxisVisible(QwtAxis::YRight) && yRight.isValid()) {
        plot->setAxisScale(QwtAxis::YRight, yRight.minValue(), yRight.maxValue());
    }
}

bool QwtCanvasZoomState::operator==(const QwtCanvasZoomState& other) const
{
    return xBottom == other.xBottom && xTop == other.xTop && yLeft == other.yLeft && yRight == other.yRight;
}

bool QwtCanvasZoomState::operator!=(const QwtCanvasZoomState& other) const
{
    return !(*this == other);
}

bool QwtCanvasZoomState::isValid() const
{
    return xBottom.isValid() || xTop.isValid() || yLeft.isValid() || yRight.isValid();
}

class QwtCanvasZoomer::PrivateData
{
public:
    uint zoomStateIndex;
    QStack< QwtCanvasZoomState > zoomStack;
    int maxStackDepth;
};

/*!
   \brief Create a zoomer for a plot canvas.

   The zoomer will zoom all axes of the plot simultaneously.

   \param canvas Plot canvas to observe, also the parent object
   \param doReplot Call QwtPlot::replot() for the attached plot before initializing
                  the zoomer with its scales.
 */
QwtCanvasZoomer::QwtCanvasZoomer(QWidget* canvas, bool doReplot) : QwtPicker(RectRubberBand, ActiveOnly, canvas)
{
    if (canvas)
        init(doReplot);
}

QwtCanvasZoomer::~QwtCanvasZoomer()
{
    delete m_data;
}

void QwtCanvasZoomer::init(bool doReplot)
{
    m_data                = new PrivateData;
    m_data->maxStackDepth = -1;

    setZoomBase(doReplot);
}

void QwtCanvasZoomer::setMaxStackDepth(int depth)
{
    m_data->maxStackDepth = depth;

    if (depth >= 0) {
        const int zoomOut = m_data->zoomStack.count() - 1 - depth;
        if (zoomOut > 0) {
            zoom(-zoomOut);
            for (int i = m_data->zoomStack.count() - 1; i > int(m_data->zoomStateIndex); i--) {
                (void)m_data->zoomStack.pop();
            }
        }
    }
}

int QwtCanvasZoomer::maxStackDepth() const
{
    return m_data->maxStackDepth;
}

const QStack< QwtCanvasZoomState >& QwtCanvasZoomer::zoomStack() const
{
    return m_data->zoomStack;
}

QwtCanvasZoomState QwtCanvasZoomer::zoomBase() const
{
    if (m_data->zoomStack.isEmpty())
        return QwtCanvasZoomState();
    return m_data->zoomStack[ 0 ];
}

QwtCanvasZoomState QwtCanvasZoomer::zoomState() const
{
    if (m_data->zoomStack.isEmpty())
        return QwtCanvasZoomState();
    return m_data->zoomStack[ m_data->zoomStateIndex ];
}

void QwtCanvasZoomer::setZoomBase(bool doReplot)
{
    QwtPlot* plot = qobject_cast< QwtPlot* >(parentWidget()->parentWidget());
    if (!plot)
        return;

    if (doReplot)
        plot->replot();

    m_data->zoomStack.clear();
    m_data->zoomStack.push(QwtCanvasZoomState::fromPlot(plot));
    m_data->zoomStateIndex = 0;
}

QwtCanvasZoomState QwtCanvasZoomer::canvasRectToZoomState(const QRect& pixelRect) const
{
    QwtPlot* plot = qobject_cast< QwtPlot* >(parentWidget()->parentWidget());
    if (!plot)
        return QwtCanvasZoomState();

    QwtCanvasZoomState newState;

    // 将像素矩形转换为四个坐标轴的区间
    QRect normalizedRect = pixelRect.normalized();

    // 处理X轴
    if (plot->isAxisVisible(QwtAxis::XBottom)) {
        const QwtScaleMap xMap = plot->canvasMap(QwtAxis::XBottom);
        double x1              = xMap.invTransform(normalizedRect.left());
        double x2              = xMap.invTransform(normalizedRect.right());
        if (x1 > x2)
            qSwap(x1, x2);
        newState.xBottom = QwtInterval(x1, x2);
    }

    if (plot->isAxisVisible(QwtAxis::XTop)) {
        const QwtScaleMap xMap = plot->canvasMap(QwtAxis::XTop);
        double x1              = xMap.invTransform(normalizedRect.left());
        double x2              = xMap.invTransform(normalizedRect.right());
        if (x1 > x2)
            qSwap(x1, x2);
        newState.xTop = QwtInterval(x1, x2);
    }

    // 处理Y轴
    if (plot->isAxisVisible(QwtAxis::YLeft)) {
        const QwtScaleMap yMap = plot->canvasMap(QwtAxis::YLeft);
        double y1              = yMap.invTransform(normalizedRect.top());
        double y2              = yMap.invTransform(normalizedRect.bottom());
        if (y1 > y2)
            qSwap(y1, y2);
        newState.yLeft = QwtInterval(y1, y2);
    }

    if (plot->isAxisVisible(QwtAxis::YRight)) {
        const QwtScaleMap yMap = plot->canvasMap(QwtAxis::YRight);
        double y1              = yMap.invTransform(normalizedRect.top());
        double y2              = yMap.invTransform(normalizedRect.bottom());
        if (y1 > y2)
            qSwap(y1, y2);
        newState.yRight = QwtInterval(y1, y2);
    }

    return newState;
}

void QwtCanvasZoomer::zoom(const QwtCanvasZoomState& state)
{
    if (m_data->maxStackDepth >= 0 && int(m_data->zoomStateIndex) >= m_data->maxStackDepth) {
        return;
    }

    if (state != zoomState()) {
        for (uint i = m_data->zoomStack.count() - 1; i > m_data->zoomStateIndex; i--) {
            (void)m_data->zoomStack.pop();
        }

        m_data->zoomStack.push(state);
        m_data->zoomStateIndex++;

        rescale();
        Q_EMIT zoomed(state);
    }
}

void QwtCanvasZoomer::zoom(int offset)
{
    int newIndex;

    if (offset == 0) {
        newIndex = 0;
    } else {
        newIndex = m_data->zoomStateIndex + offset;
        newIndex = qBound(0, newIndex, m_data->zoomStack.count() - 1);
    }

    if (newIndex != static_cast< int >(m_data->zoomStateIndex)) {
        m_data->zoomStateIndex = newIndex;
        rescale();
        Q_EMIT zoomed(zoomState());
    }
}

void QwtCanvasZoomer::setZoomStack(const QStack< QwtCanvasZoomState >& zoomStack, int zoomStateIndex)
{
    if (zoomStack.isEmpty())
        return;

    if (m_data->maxStackDepth >= 0 && zoomStack.count() > m_data->maxStackDepth) {
        return;
    }

    if (zoomStateIndex < 0 || zoomStateIndex > zoomStack.count())
        zoomStateIndex = zoomStack.count() - 1;

    const bool doRescale = zoomStack[ zoomStateIndex ] != zoomState();

    m_data->zoomStack      = zoomStack;
    m_data->zoomStateIndex = uint(zoomStateIndex);

    if (doRescale) {
        rescale();
        Q_EMIT zoomed(zoomState());
    }
}

void QwtCanvasZoomer::rescale()
{
    QwtPlot* plot = qobject_cast< QwtPlot* >(parentWidget()->parentWidget());
    if (!plot)
        return;

    const QwtCanvasZoomState& state = m_data->zoomStack[ m_data->zoomStateIndex ];
    state.applyToPlot(plot);
    plot->replot();
}

void QwtCanvasZoomer::moveBy(double dx, double dy)
{
    moveCurrentState(dx, dy);
}

void QwtCanvasZoomer::moveTo(const QPointF& pos)
{
    QwtPlot* plot = qobject_cast< QwtPlot* >(parentWidget()->parentWidget());
    if (!plot)
        return;

    // 计算移动量
    const QwtCanvasZoomState& current = zoomState();

    // 使用第一个可见的X轴和Y轴作为参考
    double currentX = 0, currentY = 0;
    if (plot->isAxisVisible(QwtAxis::XBottom) && current.xBottom.isValid()) {
        currentX = current.xBottom.minValue();
    } else if (plot->isAxisVisible(QwtAxis::XTop) && current.xTop.isValid()) {
        currentX = current.xTop.minValue();
    }

    if (plot->isAxisVisible(QwtAxis::YLeft) && current.yLeft.isValid()) {
        currentY = current.yLeft.minValue();
    } else if (plot->isAxisVisible(QwtAxis::YRight) && current.yRight.isValid()) {
        currentY = current.yRight.minValue();
    }

    double dx = pos.x() - currentX;
    double dy = pos.y() - currentY;

    moveCurrentState(dx, dy);
}

void QwtCanvasZoomer::moveCurrentState(double dx, double dy)
{
    QwtCanvasZoomState currentState = zoomState();
    QwtCanvasZoomState newState     = currentState;

    // 应用平移到所有坐标轴
    if (currentState.xBottom.isValid()) {
        newState.xBottom = QwtInterval(currentState.xBottom.minValue() + dx, currentState.xBottom.maxValue() + dx);
    }

    if (currentState.xTop.isValid()) {
        newState.xTop = QwtInterval(currentState.xTop.minValue() + dx, currentState.xTop.maxValue() + dx);
    }

    if (currentState.yLeft.isValid()) {
        newState.yLeft = QwtInterval(currentState.yLeft.minValue() + dy, currentState.yLeft.maxValue() + dy);
    }

    if (currentState.yRight.isValid()) {
        newState.yRight = QwtInterval(currentState.yRight.minValue() + dy, currentState.yRight.maxValue() + dy);
    }

    if (newState != currentState) {
        m_data->zoomStack[ m_data->zoomStateIndex ] = newState;
        rescale();
        Q_EMIT zoomed(newState);
    }
}

void QwtCanvasZoomer::widgetMouseReleaseEvent(QMouseEvent* me)
{
    if (mouseMatch(MouseSelect2, me))
        zoom(0);
    else if (mouseMatch(MouseSelect3, me))
        zoom(-1);
    else if (mouseMatch(MouseSelect6, me))
        zoom(+1);
    else
        QwtPicker::widgetMouseReleaseEvent(me);
}

void QwtCanvasZoomer::widgetKeyPressEvent(QKeyEvent* ke)
{
    if (!isActive()) {
        if (keyMatch(KeyUndo, ke))
            zoom(-1);
        else if (keyMatch(KeyRedo, ke))
            zoom(+1);
        else if (keyMatch(KeyHome, ke))
            zoom(0);
    }

    QwtPicker::widgetKeyPressEvent(ke);
}

bool QwtCanvasZoomer::accept(QPolygon& pa) const
{
    if (pa.count() < 2)
        return false;

    QRect rect = QRect(pa.first(), pa.last());
    rect       = rect.normalized();

    const int minSize = 2;
    if (rect.width() < minSize && rect.height() < minSize)
        return false;

    const int minZoomSize = 11;

    const QPoint center = rect.center();
    rect.setSize(rect.size().expandedTo(QSize(minZoomSize, minZoomSize)));
    rect.moveCenter(center);

    pa.resize(2);
    pa[ 0 ] = rect.topLeft();
    pa[ 1 ] = rect.bottomRight();

    return true;
}

QSizeF QwtCanvasZoomer::minZoomSize() const
{
    QwtPlot* plot = qobject_cast< QwtPlot* >(parentWidget()->parentWidget());
    if (!plot)
        return QSizeF(1.0, 1.0);

    // 计算最小缩放尺寸基于画布大小和当前缩放范围
    const QwtCanvasZoomState& base = zoomBase();

    double minWidth  = std::numeric_limits< double >::max();
    double minHeight = std::numeric_limits< double >::max();

    if (base.xBottom.isValid()) {
        minWidth = qMin(minWidth, base.xBottom.width() / 100.0);
    }
    if (base.xTop.isValid()) {
        minWidth = qMin(minWidth, base.xTop.width() / 100.0);
    }
    if (base.yLeft.isValid()) {
        minHeight = qMin(minHeight, base.yLeft.width() / 100.0);
    }
    if (base.yRight.isValid()) {
        minHeight = qMin(minHeight, base.yRight.width() / 100.0);
    }

    if (minWidth > std::numeric_limits< double >::max() / 2)
        minWidth = 1.0;
    if (minHeight > std::numeric_limits< double >::max() / 2)
        minHeight = 1.0;

    return QSizeF(minWidth, minHeight);
}

void QwtCanvasZoomer::begin()
{
    if (m_data->maxStackDepth >= 0) {
        if (m_data->zoomStateIndex >= uint(m_data->maxStackDepth))
            return;
    }

    QwtPicker::begin();
}

bool QwtCanvasZoomer::end(bool ok)
{
    ok = QwtPicker::end(ok);
    if (!ok)
        return false;

    QwtPlot* plot = qobject_cast< QwtPlot* >(parentWidget()->parentWidget());
    if (!plot)
        return false;

    const QPolygon& pa = selection();
    if (pa.count() < 2)
        return false;

    QRect rect = QRect(pa.first(), pa.last());
    rect       = rect.normalized();

    // 直接将画布矩形转换为四个坐标轴的缩放状态
    QwtCanvasZoomState newState = canvasRectToZoomState(rect);

    if (!newState.isValid())
        return false;

    // 应用最小缩放尺寸检查
    QSizeF minSize       = minZoomSize();
    bool needsAdjustment = false;

    // 检查X轴范围
    if (newState.xBottom.isValid() && newState.xBottom.width() < minSize.width()) {
        double center    = newState.xBottom.centerValue();
        newState.xBottom = QwtInterval(center - minSize.width() / 2, center + minSize.width() / 2);
        needsAdjustment  = true;
    }

    if (newState.xTop.isValid() && newState.xTop.width() < minSize.width()) {
        double center   = newState.xTop.centerValue();
        newState.xTop   = QwtInterval(center - minSize.width() / 2, center + minSize.width() / 2);
        needsAdjustment = true;
    }

    // 检查Y轴范围
    if (newState.yLeft.isValid() && newState.yLeft.width() < minSize.height()) {
        double center   = newState.yLeft.centerValue();
        newState.yLeft  = QwtInterval(center - minSize.height() / 2, center + minSize.height() / 2);
        needsAdjustment = true;
    }

    if (newState.yRight.isValid() && newState.yRight.width() < minSize.height()) {
        double center   = newState.yRight.centerValue();
        newState.yRight = QwtInterval(center - minSize.height() / 2, center + minSize.height() / 2);
        needsAdjustment = true;
    }

    zoom(newState);
    return true;
}
