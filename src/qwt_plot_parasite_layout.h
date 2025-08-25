#ifndef QWTPLOTPARASITELAYOUT_H
#define QWTPLOTPARASITELAYOUT_H
#include "qwt_global.h"
#include "qwt_plot_layout.h"

class QWT_EXPORT QwtPlotParasiteLayout : public QwtPlotLayout
{
public:
    QwtPlotParasiteLayout();
    ~QwtPlotParasiteLayout();
    virtual void activate(const QwtPlot* plot, const QRectF& plotRect, Options options = 0) override;

    virtual QSize minimumSizeHint(const QwtPlot* plot) const override;
};

#endif  // QWTPLOTPARASITELAYOUT_H
