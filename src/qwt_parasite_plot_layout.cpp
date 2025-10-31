#include "qwt_parasite_plot_layout.h"
// Qt
#include <QDebug>

// qwt
#include "qwt_plot.h"
#include "qwt_scale_widget.h"
#include "qwt_plot_canvas.h"
#include "qwt_plot_layout_engine.h"
#include "qwt_abstract_legend.h"
#include "qwt_math.h"
#include "qwt_plot_layout_engine.h"

#ifndef QWTPLOTPARASITELAYOUT_DEBUG_PRINT
#define QWTPLOTPARASITELAYOUT_DEBUG_PRINT 1
#endif

QwtParasitePlotLayout::QwtParasitePlotLayout() : QwtPlotLayout()
{
}

QwtParasitePlotLayout::~QwtParasitePlotLayout()
{
}

void QwtParasitePlotLayout::activate(const QwtPlot* plot, const QRectF& plotRect, QwtPlotLayout::Options options)
{
    QwtPlot* hostPlot = plot->hostPlot();
    if (!hostPlot) {
        // qDebug() << "QwtPlotParasiteLayout: No host plot found! Using default layout.";
        QwtPlotLayout::activate(plot, plotRect, options);
        return;
    }
    invalidate();
    doActivate(plot, plotRect, options);
    mScaleRects[ QwtAxis::YLeft ]   = scaleRect(QwtAxis::YLeft);
    mScaleRects[ QwtAxis::YRight ]  = scaleRect(QwtAxis::YRight);
    mScaleRects[ QwtAxis::XBottom ] = scaleRect(QwtAxis::XBottom);
    mScaleRects[ QwtAxis::XTop ]    = scaleRect(QwtAxis::XTop);
    // 寄生轴所有部件复制宿主轴
    if (QwtPlotLayout* hostLayout = hostPlot->plotLayout()) {
        setCanvasRect(hostLayout->canvasRect());
        setSpacing(hostLayout->spacing());
        setTitleRect(hostLayout->titleRect());
        setFooterRect(hostLayout->footerRect());
        setLegendRect(hostLayout->legendRect());
        for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; ++axisPos) {
            setCanvasMargin(hostLayout->canvasMargin(axisPos), axisPos);
            setScaleRect(axisPos, hostLayout->scaleRect(axisPos));
        }
    }
}

QSize QwtParasitePlotLayout::minimumSizeHint(const QwtPlot* plot) const
{
	// 对于寄生轴，最小尺寸主要由启用的轴决定
	QwtPlot* hostPlot = plot->hostPlot();
	if (hostPlot) {
		QwtPlotLayout* lay = hostPlot->plotLayout();
		if (lay) {
			return lay->minimumSizeHint(hostPlot);
		}
	}
    return QwtPlotLayout::minimumSizeHint(plot);
}

QRectF QwtParasitePlotLayout::parasiteScaleRect(QwtAxisId aid) const
{
    return mScaleRects[ aid ];
}
