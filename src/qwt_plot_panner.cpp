#include "qwt_plot_panner.h"
#include "qwt_plot.h"
#include "qwt_scale_map.h"
#include "qwt_picker_machine.h"
#include "qwt_plot_canvas.h"
#include "qwt_scale_div.h"
#include "qwt_transform.h"

#include <qevent.h>
#include <qpainter.h>

#include <QDebug>

class QwtPlotPanner::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotPanner)
public:
    PrivateData(QwtPlotPanner* p) : q_ptr(p), orientations(Qt::Vertical | Qt::Horizontal), initialPos(-1, -1)
    {
    }

    Qt::Orientations orientations;

    QPoint beginPos;    ///< 记录begin事件时的位置，在移动过程中不会更新
    QPoint initialPos;  ///< 记录上次移动时的位置
    QPoint currentPos;  ///< 记录当前位置，当前位置-initialPos=当前画布偏移，当前位置-beginPos=总体偏移
};

QwtPlotPanner::QwtPlotPanner(QWidget* canvas) : QwtPicker(canvas), QWT_PIMPL_CONSTRUCT
{
    if (canvas) {
        init();
    }
}

QwtPlotPanner::~QwtPlotPanner()
{
}

void QwtPlotPanner::init()
{
    setStateMachine(new QwtPickerDragPointMachine);
    setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton);
    setTrackerMode(QwtPicker::AlwaysOff);
    setRubberBand(QwtPicker::NoRubberBand);
    setEnabled(true);
}

QWidget* QwtPlotPanner::canvas()
{
    QWidget* w = parentWidget();
    if (w && w->inherits("QwtPlotCanvas"))
        return w;
    return nullptr;
}

const QWidget* QwtPlotPanner::canvas() const
{
    const QWidget* w = parentWidget();
    if (w && w->inherits("QwtPlotCanvas"))
        return w;
    return nullptr;
}

QwtPlot* QwtPlotPanner::plot()
{
    QWidget* w = canvas();
    if (w)
        w = w->parentWidget();

    if (w && w->inherits("QwtPlot"))
        return static_cast< QwtPlot* >(w);

    return nullptr;
}

const QwtPlot* QwtPlotPanner::plot() const
{
    const QWidget* w = canvas();
    if (w)
        w = w->parentWidget();

    if (w && w->inherits("QwtPlot"))
        return static_cast< const QwtPlot* >(w);

    return nullptr;
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
    // 配置事件模式 - 使用左键拖拽
    setMousePattern(QwtEventPattern::MouseSelect1, button, modifiers);
}

void QwtPlotPanner::getMouseButton(Qt::MouseButton& button, Qt::KeyboardModifiers& modifiers) const
{
    const QVector< MousePattern >& mp = mousePattern();
    button                            = mp[ QwtEventPattern::MouseSelect1 ].button;
    modifiers                         = mp[ QwtEventPattern::MouseSelect1 ].modifiers;
}

void QwtPlotPanner::move(const QPoint& pos)
{
    if (!isActive()) {
        return;
    }
    QWT_D(d);
    d->currentPos = pos;

    int dx = pos.x() - d->initialPos.x();
    int dy = pos.y() - d->initialPos.y();

    if (!isOrientationEnabled(Qt::Horizontal)) {
        dx = 0;
    }
    if (!isOrientationEnabled(Qt::Vertical)) {
        dy = 0;
    }

    if (dx != 0 || dy != 0) {
        // 实时移动画布
        moveCanvas(dx, dy);

        // 更新初始位置为当前位置，实现连续移动
        d->initialPos = pos;
    }
}

bool QwtPlotPanner::end(bool ok)
{
    if (!isActive()) {
        return QwtPicker::end(ok);
    }
    QWT_D(d);
    int wholeDx = d->currentPos.x() - d->beginPos.x();
    int wholeDy = d->currentPos.y() - d->beginPos.y();

    if (!isOrientationEnabled(Qt::Horizontal)) {
        wholeDx = 0;
    }
    if (!isOrientationEnabled(Qt::Vertical)) {
        wholeDy = 0;
    }

    if (wholeDx != 0 || wholeDy != 0) {
        Q_EMIT panned(wholeDx, wholeDy);
    }

    d->initialPos = QPoint();
    d->currentPos = QPoint();
    d->beginPos   = QPoint();
    return QwtPicker::end(ok);
}

void QwtPlotPanner::moveCanvas(int dx, int dy)
{
    QwtPlot* plt = plot();
    if (!plt) {
        return;
    }
    //! 这里有个问题要注意，对于寄生轴，如果这个轴是共享了宿主的某个轴，那么不应该响应pan，
    //! 因此，这里要求确保宿主绘图最后pan,这样宿主绘图的更新会同步更新给寄生绘图的共享轴
    const QList< QwtPlot* > allPlots = plt->plotList(true);  // 倒序获取，宿主最后更新
    for (auto plot : allPlots) {
        plot->panCanvas(QPoint(dx, dy));
    }
    plt->replotAll();
}

void QwtPlotPanner::widgetMousePressEvent(QMouseEvent* mouseEvent)
{
    QWT_D(d);

    //! 注：
    //! 这里不使用begin来记录，因为在setTrackerMode(QwtPicker::AlwaysOff);模式下，是不会记录鼠标位置
    //! 在begin中，通过trackerPosition()是无法获取点击的位置，只能在此获取
    if (mouseMatch(QwtEventPattern::MouseSelect1,
                   static_cast< const QMouseEvent* >(mouseEvent))) {  // 记录初始位置
        d->beginPos = d->initialPos = d->currentPos = mouseEvent->pos();
    }

    QwtPicker::widgetMousePressEvent(mouseEvent);
}
