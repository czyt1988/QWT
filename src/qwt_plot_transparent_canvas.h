#ifndef QWT_PLOT_TRANSPARENT_CANVAS_H
#define QWT_PLOT_TRANSPARENT_CANVAS_H

#include "qwt_global.h"
#include "qwt_plot_abstract_canvas.h"
#include <qframe.h>

class QWT_EXPORT QwtPlotTransparentCanvas : public QFrame, public QwtPlotAbstractCanvas
{
    Q_OBJECT
public:
    explicit QwtPlotTransparentCanvas(QwtPlot* plot = nullptr);
    virtual ~QwtPlotTransparentCanvas();
public Q_SLOTS:
    virtual void replot();

protected:
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void drawBorder(QPainter* painter) override;
    virtual QPainterPath borderPath(const QRect& rect) const;
};

#endif  // QWT_PLOT_TRANSPARENT_CANVAS_H
