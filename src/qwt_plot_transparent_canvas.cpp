#include "qwt_plot_transparent_canvas.h"
// qt
#include <qpainter.h>
#include <qpainterpath.h>
#include <qevent.h>
// qwt
#include "qwt_painter.h"
#include "qwt_plot.h"

#ifndef QWTPLOTTRANSPARENTCANVAS_DEBUG_DRAW
#define QWTPLOTTRANSPARENTCANVAS_DEBUG_DRAW 0
#endif

QwtPlotTransparentCanvas::QwtPlotTransparentCanvas(QwtPlot* plot) : QFrame(plot), QwtPlotAbstractCanvas(this)
{
    // 设置透明相关属性
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);  // 通常需要处理鼠标事件
    // 禁用所有可能产生不透明效果的属性
    setAttribute(Qt::WA_OpaquePaintEvent, false);  // Qt 不会再在 paintEvent() 之前用背景色/刷擦除背景。
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);

    // 设置透明调色板
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::transparent);
    setPalette(palette);

    // 禁用边框
    setLineWidth(0);
    setFrameShadow(QFrame::Plain);
    setFrameShape(QFrame::NoFrame);
}

QwtPlotTransparentCanvas::~QwtPlotTransparentCanvas()
{
}

void QwtPlotTransparentCanvas::replot()
{
    update(contentsRect());
}

void QwtPlotTransparentCanvas::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
#if QWTPLOTTRANSPARENTCANVAS_DEBUG_DRAW
    painter.setPen(QPen(Qt::red, 1, Qt::DashLine));
    painter.drawRect(rect().adjusted(1, 1, -1, -1));
    painter.setClipRegion(event->region());
#endif

    // 对于透明画布，我们不需要填充背景
    // 直接绘制内容到画布
    drawCanvas(&painter);
}

void QwtPlotTransparentCanvas::drawBorder(QPainter* painter)
{
    // 不绘制任何边框
    Q_UNUSED(painter);
}

QPainterPath QwtPlotTransparentCanvas::borderPath(const QRect& rect) const
{
    Q_UNUSED(rect);
    return QPainterPath();
}
