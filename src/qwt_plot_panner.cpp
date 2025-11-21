#include "qwt_plot_panner.h"
#include "qwt_plot.h"
#include "qwt_scale_map.h"
#include "qwt_picker_machine.h"
#include "qwt_plot_canvas.h"
#include "qwt_scale_div.h"

#include <qevent.h>
#include <qpainter.h>

class QwtPlotPanner::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotPanner)
public:
    PrivateData(QwtPlotPanner* p)
        : q_ptr(p)
        , button(Qt::LeftButton)
        , buttonModifiers(Qt::NoModifier)
        , orientations(Qt::Vertical | Qt::Horizontal)
        , isEnabled(false)
        , initialPos(-1, -1)
    {
    }

    Qt::MouseButton button;
    Qt::KeyboardModifiers buttonModifiers;
    Qt::Orientations orientations;
    bool isEnabled;

    QPoint initialPos;
    QPoint currentPos;
};

QwtPlotPanner::QwtPlotPanner(QWidget* parent) : QwtPicker(parent), QWT_PIMPL_CONSTRUCT
{
    init();
}

QwtPlotPanner::~QwtPlotPanner()
{
}

void QwtPlotPanner::init()
{
    setTrackerMode(QwtPicker::AlwaysOff);
    setRubberBand(QwtPicker::NoRubberBand);
    setStateMachine(new QwtPickerDragPointMachine);

    // 设置透明笔，这样不会绘制任何橡皮筋
    QPen pen(Qt::NoPen);
    setRubberBandPen(pen);

    m_data->isEnabled = true;
}

QWidget* QwtPlotPanner::canvas()
{
    QWidget* w = parentWidget();
    if (w && w->inherits("QwtPlotCanvas"))
        return w;
    return NULL;
}

const QWidget* QwtPlotPanner::canvas() const
{
    const QWidget* w = parentWidget();
    if (w && w->inherits("QwtPlotCanvas"))
        return w;
    return NULL;
}

QwtPlot* QwtPlotPanner::plot()
{
    QWidget* w = canvas();
    if (w)
        w = w->parentWidget();

    if (w && w->inherits("QwtPlot"))
        return static_cast< QwtPlot* >(w);

    return NULL;
}

const QwtPlot* QwtPlotPanner::plot() const
{
    const QWidget* w = canvas();
    if (w)
        w = w->parentWidget();

    if (w && w->inherits("QwtPlot"))
        return static_cast< const QwtPlot* >(w);

    return NULL;
}

void QwtPlotPanner::setOrientations(Qt::Orientations o)
{
    m_data->orientations = o;
}

Qt::Orientations QwtPlotPanner::orientations() const
{
    return m_data->orientations;
}

bool QwtPlotPanner::isOrientationEnabled(Qt::Orientation o) const
{
    return m_data->orientations & o;
}

void QwtPlotPanner::setMouseButton(Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
{
    m_data->button          = button;
    m_data->buttonModifiers = modifiers;
}

void QwtPlotPanner::getMouseButton(Qt::MouseButton& button, Qt::KeyboardModifiers& modifiers) const
{
    button    = m_data->button;
    modifiers = m_data->buttonModifiers;
}

void QwtPlotPanner::setEnabled(bool on)
{
    if (m_data->isEnabled != on) {
        m_data->isEnabled = on;
        QwtPicker::setEnabled(on);
    }
}

bool QwtPlotPanner::accept(QPolygon&) const
{
    // 我们不需要验证选择，直接接受
    return true;
}

void QwtPlotPanner::begin()
{
    if (!m_data->isEnabled)
        return;

    m_data->initialPos = m_data->currentPos = trackerPosition();
    QwtPicker::begin();
}

void QwtPlotPanner::move(const QPoint& pos)
{
    if (!m_data->isEnabled || !isActive())
        return;

    m_data->currentPos = pos;

    int dx = m_data->currentPos.x() - m_data->initialPos.x();
    int dy = m_data->currentPos.y() - m_data->initialPos.y();

    if (!isOrientationEnabled(Qt::Horizontal))
        dx = 0;
    if (!isOrientationEnabled(Qt::Vertical))
        dy = 0;

    if (dx != 0 || dy != 0) {
        // 发出移动信号，外部可以实时响应
        Q_EMIT moved(dx, dy);

        // 实时移动画布
        moveCanvas(dx, dy);

        // 更新初始位置为当前位置，实现连续移动
        m_data->initialPos = m_data->currentPos;
    }
}

bool QwtPlotPanner::end(bool ok)
{
    if (isActive()) {
        int dx = m_data->currentPos.x() - m_data->initialPos.x();
        int dy = m_data->currentPos.y() - m_data->initialPos.y();

        if (!isOrientationEnabled(Qt::Horizontal))
            dx = 0;
        if (!isOrientationEnabled(Qt::Vertical))
            dy = 0;

        if (dx != 0 || dy != 0) {
            Q_EMIT panned(dx, dy);
        }

        m_data->initialPos = QPoint(-1, -1);
        m_data->currentPos = QPoint(-1, -1);
    }

    return QwtPicker::end(ok);
}

void QwtPlotPanner::widgetMousePressEvent(QMouseEvent* mouseEvent)
{
    if ((mouseEvent->button() == m_data->button) && (mouseEvent->modifiers() == m_data->buttonModifiers)) {
        QwtPicker::widgetMousePressEvent(mouseEvent);
    }
}

void QwtPlotPanner::widgetMouseMoveEvent(QMouseEvent* mouseEvent)
{
    if (isActive()) {
        QwtPicker::widgetMouseMoveEvent(mouseEvent);
    }
}

void QwtPlotPanner::widgetMouseReleaseEvent(QMouseEvent* mouseEvent)
{
    if (mouseEvent->button() == m_data->button) {
        QwtPicker::widgetMouseReleaseEvent(mouseEvent);
    }
}

void QwtPlotPanner::moveCanvas(int dx, int dy)
{
    QwtPlot* plt = plot();
    if (!plt)
        return;

    // 实时调整坐标轴范围来实现平移效果
    for (int axis = 0; axis < QwtPlot::axisCnt; axis++) {
        if (!plt->axisEnabled(axis))
            continue;

        const QwtScaleMap map = plt->canvasMap(axis);
        if (map.isInverting())
            continue;

        double min   = plt->axisScaleDiv(axis).lowerBound();
        double max   = plt->axisScaleDiv(axis).upperBound();
        double range = max - min;

        if (axis == QwtPlot::xBottom || axis == QwtPlot::xTop) {
            if (isOrientationEnabled(Qt::Horizontal) && dx != 0) {
                double dxRange = -dx * range / map.pDist();
                plt->setAxisScale(axis, min + dxRange, max + dxRange);
            }
        } else {
            if (isOrientationEnabled(Qt::Vertical) && dy != 0) {
                double dyRange = dy * range / map.pDist();
                plt->setAxisScale(axis, min + dyRange, max + dyRange);
            }
        }
    }

    plt->replot();
}
