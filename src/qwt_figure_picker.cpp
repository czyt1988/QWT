#include "qwt_figure_picker.h"
#include <QApplication>
#include <QPainterPath>
#include <QMouseEvent>
#include <QDebug>
#include "qwt_figure_widget_overlay.h"
#include "qwt_scale_div.h"
#include "qwt_math.h"
#include "qwt_picker_machine.h"
#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include "qwt_figure.h"

// 私有数据类
class QwtFigurePicker::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtFigurePicker)
public:
    PrivateData(QwtFigurePicker* p) : q_ptr(p), figure(nullptr)
    {
    }

    ~PrivateData()
    {
    }
    QwtPlot* currentPlot { nullptr };
    bool isOnCanvas { false };
    QwtFigure* figure;  ///< 目标QwtFigure
    QList< QPair< QwtPlot*, QRect > > canvasRects;
};

QwtFigurePicker::QwtFigurePicker(QwtFigure* fig) : QwtPicker(fig), QWT_PIMPL_CONSTRUCT
{
    m_data->figure = fig;
    init();
}

QwtFigurePicker::~QwtFigurePicker()
{
}

void QwtFigurePicker::init()
{
    QwtFigure* figure = m_data->figure;
    // 更新画布区域
    updateCanvasRects();

    // 连接QwtFigure的信号
    if (figure) {
        if (!figure->hasMouseTracking()) {
            figure->setMouseTracking(trackerMode() == AlwaysOn);
        }
        if (figure->focusPolicy() == Qt::NoFocus) {
            figure->setFocusPolicy(Qt::WheelFocus);
        }
        connect(figure, &QwtFigure::axesAdded, this, &QwtFigurePicker::onAxesAdded);
        connect(figure, &QwtFigure::axesRemoved, this, &QwtFigurePicker::onAxesRemoved);
    }
}

/**
 * @brief 获取当前鼠标所在的绘图
 * @return 当前绘图指针，如果不在任何绘图上则返回nullptr
 */
QwtPlot* QwtFigurePicker::currentPlot() const
{
    return m_data->currentPlot;
}

bool QwtFigurePicker::isOnCanvas() const
{
    return m_data->isOnCanvas;
}

/**
 * @brief 获取指定绘图的画布区域
 * @param plot 目标绘图
 * @return 画布区域
 */
QRect QwtFigurePicker::canvasRect(QwtPlot* plot) const
{
    for (const auto& p : qAsConst(m_data->canvasRects)) {
        if (p.first == plot) {
            return p.second;
        }
    }
    return QRect();
}

/**
 * @brief 获取所有绘图的画布区域
 * @return 绘图->画布区域的映射
 */
const QList< QPair< QwtPlot*, QRect > >& QwtFigurePicker::canvasRects() const
{
    return m_data->canvasRects;
}

void QwtFigurePicker::replot()
{
    if (m_data->figure) {
        m_data->figure->replotAll();
    }
    updateDisplay();
}

bool QwtFigurePicker::eventFilter(QObject* object, QEvent* event)
{
    if (object && object == parentWidget()) {
        switch (event->type()) {
        case QEvent::Resize: {
            updateCanvasRects();
            break;
        }
        default:
            break;
        }
    }
    return QwtPicker::eventFilter(object, event);
}

void QwtFigurePicker::widgetMousePressEvent(QMouseEvent* event)
{
    if (event) {
        updateCurrentPlot(event->pos());
    }
    QwtPicker::widgetMousePressEvent(event);
}

void QwtFigurePicker::widgetMouseReleaseEvent(QMouseEvent* event)
{
    if (event) {
        updateCurrentPlot(event->pos());
    }
    QwtPicker::widgetMouseReleaseEvent(event);
}

void QwtFigurePicker::widgetMouseMoveEvent(QMouseEvent* event)
{
    if (event) {
        updateCurrentPlot(event->pos());
    }
    QwtPicker::widgetMouseMoveEvent(event);
}

void QwtFigurePicker::onAxesAdded(QwtPlot* plot)
{
    Q_UNUSED(plot);
}

void QwtFigurePicker::onAxesRemoved(QwtPlot* plot)
{
    if (m_data->currentPlot == plot) {
        m_data->currentPlot = nullptr;
    }
}

void QwtFigurePicker::updateCanvasRects()
{
    m_data->canvasRects.clear();

    if (!m_data->figure) {
        return;
    }

    QList< QwtPlot* > plots = m_data->figure->allAxes();
    for (QwtPlot* plot : plots) {
        if (plot && plot->canvas()) {
            QRect canvasGeo = plot->canvas()->geometry();

            // 转换为全局坐标
            QPoint globalPos = plot->mapTo(m_data->figure, canvasGeo.topLeft());
            QRect globalRect(globalPos, canvasGeo.size());
            m_data->canvasRects.append(qMakePair(plot, globalRect));
        }
    }
}

void QwtFigurePicker::updateCurrentPlot(const QPoint& pos)
{
    m_data->currentPlot = nullptr;
    for (const auto& p : qAsConst(m_data->canvasRects)) {
        if (p.first->geometry().contains(pos)) {
            m_data->currentPlot = p.first;
            m_data->isOnCanvas  = p.second.contains(pos);
        }
    }
}
