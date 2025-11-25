/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/

#include "qwt_plot_canvas_zoomer.h"
#include "qwt_plot.h"
#include "qwt_scale_div.h"
#include "qwt_scale_map.h"
#include "qwt_picker_machine.h"

#include <qstack.h>
#include <limits>

// QwtCanvasZoomState 实现
QwtPlotCanvasZoomState::QwtPlotCanvasZoomState()
{
}

QwtPlotCanvasZoomState::QwtPlotCanvasZoomState(QwtPlot* p,
                                               const QwtInterval& yLeft,
                                               const QwtInterval& yRight,
                                               const QwtInterval& xBottom,
                                               const QwtInterval& xTop)
    : plot(p)
{
    axisInterval[ QwtAxis::YLeft ]   = yLeft;
    axisInterval[ QwtAxis::YRight ]  = yRight;
    axisInterval[ QwtAxis::XBottom ] = xBottom;
    axisInterval[ QwtAxis::XTop ]    = xTop;
}

QwtPlotCanvasZoomState QwtPlotCanvasZoomState::fromPlot(QwtPlot* plot)
{
    QwtPlotCanvasZoomState state;
    if (!plot) {
        return state;
    }
    state.plot = plot;
    // 获取四个坐标轴的当前范围
    for (QwtAxisId axisId = 0; axisId < QwtAxis::AxisPositions; ++axisId) {
        const QwtScaleDiv& scaleDiv  = plot->axisScaleDiv(axisId);
        state.axisInterval[ axisId ] = QwtInterval(scaleDiv.lowerBound(), scaleDiv.upperBound());
    }

    return state;
}

void QwtPlotCanvasZoomState::apply() const
{
    if (!plot) {
        return;
    }

    // 应用四个坐标轴的范围
    for (QwtAxisId axisId = 0; axisId < QwtAxis::AxisPositions; ++axisId) {
        plot->setAxisScale(axisId, axisInterval[ axisId ].minValue(), axisInterval[ axisId ].maxValue());
    }
}

bool QwtPlotCanvasZoomState::operator==(const QwtPlotCanvasZoomState& other) const
{
    if (plot != other.plot) {
        return false;
    }
    for (QwtAxisId axisId = 0; axisId < QwtAxis::AxisPositions; ++axisId) {
        if (axisInterval[ axisId ] != other.axisInterval[ axisId ]) {
            return false;
        }
    }
    return true;
}

bool QwtPlotCanvasZoomState::operator!=(const QwtPlotCanvasZoomState& other) const
{
    return !(*this == other);
}

bool QwtPlotCanvasZoomState::isValid() const
{
    return (!plot.isNull());
}
//----------------------------------------------------
// QwtCanvasZoomer::PrivateData
//----------------------------------------------------
class QwtPlotCanvasZoomer::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotCanvasZoomer)
public:
    PrivateData(QwtPlotCanvasZoomer* p);
    uint zoomStateIndex;
    QStack< QList< QwtPlotCanvasZoomState > > zoomStack;
    int maxStackDepth { -1 };  ///< zoomer记录的最大的缩放次数，如果超过这个次数，不会再让缩放
    bool replot { true };      ///< 是否进行重绘
};

QwtPlotCanvasZoomer::PrivateData::PrivateData(QwtPlotCanvasZoomer* p) : q_ptr(p)
{
}

//----------------------------------------------------
// QwtCanvasZoomer
//----------------------------------------------------

/*!
   \brief Create a zoomer for a plot canvas.

   The zoomer will zoom all axes of the plot simultaneously.

   \param canvas Plot canvas to observe, also the parent object
   \param doReplot Call QwtPlot::replot() for the attached plot before initializing
                  the zoomer with its scales.
 */
QwtPlotCanvasZoomer::QwtPlotCanvasZoomer(QWidget* canvas, bool doReplot) : QwtCanvasPicker(canvas), QWT_PIMPL_CONSTRUCT
{
    if (canvas) {
        init(doReplot);
    }
}

QwtPlotCanvasZoomer::~QwtPlotCanvasZoomer()
{
}

void QwtPlotCanvasZoomer::init(bool doReplot)
{
    m_data->maxStackDepth = -1;
    m_data->replot        = doReplot;
    setTrackerMode(ActiveOnly);
    setRubberBand(RectRubberBand);
    setStateMachine(new QwtPickerDragRectMachine());
    setZoomBase(doReplot);
}

/**
 * @brief 获取所有绘图的缩放区域
 * @param pixelRect
 * @return
 */
QList< QwtPlotCanvasZoomState > QwtPlotCanvasZoomer::canvasRectToZoomStateList(const QRect& pixelRect) const
{
    const QwtPlot* plt = plot();
    if (!plt) {
        return QList< QwtPlotCanvasZoomState >();
    }
    QList< QwtPlotCanvasZoomState > states;
    // 宿主轴放到最后，一般寄生轴会绑定宿主轴的某个轴，宿主轴最后变更可以把寄生轴重新设置回和宿主轴同步
    const QList< QwtPlot* > plts = plt->plotList(true);
    for (QwtPlot* p : plts) {
        QwtPlotCanvasZoomState s = canvasRectToZoomState(p, pixelRect);
        states.append(s);
    }
    return states;
}

/**
 * @brief 获取绘图对应矩形像素区域的坐标范围
 * @param pixelRect 矩形像素区域
 * @return
 */
QwtPlotCanvasZoomState QwtPlotCanvasZoomer::canvasRectToZoomState(QwtPlot* plt, const QRect& pixelRect) const
{
    if (!plt) {
        return QwtPlotCanvasZoomState();
    }
    // 将像素矩形转换为四个坐标轴的区间
    QRect normalizedRect = pixelRect.normalized();
    QwtPlotCanvasZoomState state;
    state.plot = plt;
    for (QwtAxisId axisId = 0; axisId < QwtAxis::AxisPositions; ++axisId) {
        const QwtScaleMap& scaleMap = plt->canvasMap(axisId);
        double v1 { 0.0 }, v2 { 0.0 };
        if (QwtAxis::isXAxis(axisId)) {
            v1 = scaleMap.invTransform(normalizedRect.left());
            v2 = scaleMap.invTransform(normalizedRect.right());
        } else {
            v1 = scaleMap.invTransform(normalizedRect.bottom());
            v2 = scaleMap.invTransform(normalizedRect.top());
        }
        if (const QwtTransform* transform = scaleMap.transformation()) {
            // 如果存在变换，要确认变换的范围,bounded实际是qBound( LogMin, value, LogMax );
            v1 = transform->bounded(v1);
            v2 = transform->bounded(v2);
        }
        state.axisInterval[ axisId ].setInterval(v1, v2);
    }
    return state;
}

void QwtPlotCanvasZoomer::setMaxStackDepth(int depth)
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

int QwtPlotCanvasZoomer::maxStackDepth() const
{
    return m_data->maxStackDepth;
}

const QStack< QList< QwtPlotCanvasZoomState > >& QwtPlotCanvasZoomer::zoomStack() const
{
    return m_data->zoomStack;
}

QList< QwtPlotCanvasZoomState > QwtPlotCanvasZoomer::zoomBase() const
{
    if (m_data->zoomStack.isEmpty()) {
        return QList< QwtPlotCanvasZoomState >();
    }
    return m_data->zoomStack[ 0 ];
}

QList< QwtPlotCanvasZoomState > QwtPlotCanvasZoomer::zoomState() const
{
    if (m_data->zoomStack.isEmpty()) {
        return QList< QwtPlotCanvasZoomState >();
    }
    return m_data->zoomStack[ m_data->zoomStateIndex ];
}

void QwtPlotCanvasZoomer::setZoomBase(bool doReplot)
{
    QwtPlot* plt = plot();
    if (!plt) {
        return;
    }

    if (doReplot) {
        plt->replotAll();
    }

    m_data->zoomStack.clear();
    const QList< QwtPlot* > plts = plt->plotList(true);
    QList< QwtPlotCanvasZoomState > base;
    for (QwtPlot* p : plts) {
        base.append(QwtPlotCanvasZoomState::fromPlot(p));
    }
    m_data->zoomStack.push(base);
    m_data->zoomStateIndex = 0;
}

void QwtPlotCanvasZoomer::zoom(int offset)
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

void QwtPlotCanvasZoomer::appendZoom(const QList< QwtPlotCanvasZoomState >& rect)
{
    m_data->zoomStack.push(rect);
    m_data->zoomStateIndex = m_data->zoomStack.size() - 1;
    rescale();
    Q_EMIT zoomed(rect);
}

void QwtPlotCanvasZoomer::rescale()
{
    const QList< QwtPlotCanvasZoomState >& states = m_data->zoomStack[ m_data->zoomStateIndex ];
    for (const QwtPlotCanvasZoomState& state : states) {
        state.apply();
    }
    if (m_data->replot) {
        QwtPlot* plt = plot();
        plt->replotAll();
    }
}

void QwtPlotCanvasZoomer::widgetMouseReleaseEvent(QMouseEvent* me)
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

void QwtPlotCanvasZoomer::widgetKeyPressEvent(QKeyEvent* ke)
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

bool QwtPlotCanvasZoomer::accept(QPolygon& pa) const
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

void QwtPlotCanvasZoomer::begin()
{
    if (m_data->maxStackDepth >= 0) {
        if (m_data->zoomStateIndex >= uint(m_data->maxStackDepth))
            return;
    }

    QwtPicker::begin();
}

bool QwtPlotCanvasZoomer::end(bool ok)
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
    QList< QwtPlotCanvasZoomState > newState = canvasRectToZoomStateList(rect);

    appendZoom(newState);
    return true;
}
