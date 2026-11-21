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

/**
 * @brief Constructor
 * @param[in] plot Parent plot widget
 */
QwtPlotTransparentCanvas::QwtPlotTransparentCanvas(QwtPlot* plot) : QFrame(plot), QwtPlotAbstractCanvas(this)
{
    // Set transparency-related attributes
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);  // Typically needs to handle mouse events
    // Disable all attributes that may produce opaque effects
    setAttribute(Qt::WA_OpaquePaintEvent,
                 false);  // Qt will no longer erase background with background color/brush before paintEvent()
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);

    // Set transparent palette
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::transparent);
    setPalette(palette);

    // Disable border
    setLineWidth(0);
    setFrameShadow(QFrame::Plain);
    setFrameShape(QFrame::NoFrame);
}

/**
 * @brief Destructor
 */
QwtPlotTransparentCanvas::~QwtPlotTransparentCanvas()
{
}

/**
 * @brief Replot the canvas
 */
void QwtPlotTransparentCanvas::replot()
{
    update(contentsRect());
}

void QwtPlotTransparentCanvas::paintEvent(QPaintEvent* event)
{
    // only used by the debug drawing below
    Q_UNUSED(event);

    QPainter painter(this);
#if QWTPLOTTRANSPARENTCANVAS_DEBUG_DRAW
    painter.setPen(QPen(Qt::red, 1, Qt::DashLine));
    painter.drawRect(rect().adjusted(1, 1, -1, -1));
    painter.setClipRegion(event->region());
#endif

    // For transparent canvas, we do not need to fill the background
    // Draw content directly onto the canvas
    drawCanvas(&painter);
}

void QwtPlotTransparentCanvas::drawBorder(QPainter* painter)
{
    // Do not draw any border
    Q_UNUSED(painter);
}

QPainterPath QwtPlotTransparentCanvas::borderPath(const QRect& rect) const
{
    Q_UNUSED(rect);
    return QPainterPath();
}
