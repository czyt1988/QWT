#include "qwt_canvas_picker.h"
#include <QWidget>
#include "qwt_plot.h"

QwtCanvasPicker::QwtCanvasPicker(QWidget* canvas) : QwtPicker(canvas)
{
}

QwtCanvasPicker::~QwtCanvasPicker()
{
}

QwtPlot* QwtCanvasPicker::plot()
{
    QWidget* w = canvas();
    if (w) {
        w = w->parentWidget();
    }

    return qobject_cast< QwtPlot* >(w);
}

const QwtPlot* QwtCanvasPicker::plot() const
{
    const QWidget* w = canvas();
    if (w) {
        w = w->parentWidget();
    }

    return qobject_cast< const QwtPlot* >(w);
}

QWidget* QwtCanvasPicker::canvas()
{
    return parentWidget();
}

const QWidget* QwtCanvasPicker::canvas() const
{
    return parentWidget();
}
