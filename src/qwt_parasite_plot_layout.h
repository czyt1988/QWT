#ifndef QWTPARASITEPLOTLAYOUT_H
#define QWTPARASITEPLOTLAYOUT_H
#include "qwt_global.h"
#include "qwt_plot_layout.h"

class QWT_EXPORT QwtParasitePlotLayout : public QwtPlotLayout
{
public:
    QwtParasitePlotLayout();
    ~QwtParasitePlotLayout();
	virtual void activate(const QwtPlot* plot, const QRectF& plotRect, Options options = Options()) override;

	virtual QSize minimumSizeHint(const QwtPlot* plot) const override;
    // 这个是获取寄生轴的不偏移时的坐标轴矩形
    QRectF parasiteScaleRect(QwtAxisId aid) const;

private:
    QRectF mScaleRects[ QwtAxis::AxisPositions ];
};

#endif  // QWTPLOTPARASITELAYOUT_H
