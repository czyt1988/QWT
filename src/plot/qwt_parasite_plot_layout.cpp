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

/**
 * @brief Construct a parasite plot layout
 */
QwtParasitePlotLayout::QwtParasitePlotLayout() : QwtPlotLayout()
{
}

/**
 * @brief Destructor
 */
QwtParasitePlotLayout::~QwtParasitePlotLayout()
{
}

/**
 * @brief Activate the layout for a parasite plot
 *
 * Copies all layout attributes from the host plot to maintain visual consistency.
 * If no host plot is found, uses default layout behavior.
 *
 * @param plot Parasite plot to activate layout for
 * @param plotRect Rectangle for the plot area
 * @param options Layout options
 */
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
    // Copy all parasite axis components from the host
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
        // Caption rects for outside axis titles depend on the scale rects and
        // the own layer offsets (margin/edgeMargin), so they have to be
        // recomputed from the copied host rects. The bands have already been
        // reserved by the host layout (it aggregates the caption heights of
        // all parasite plots).
        updateScaleCaptionRects(plot);
    }
}

/**
 * @brief Get minimum size hint for a parasite plot
 *
 * Returns the minimum size from the host plot's layout if available,
 * otherwise uses default QwtPlotLayout behavior.
 *
 * @param plot Parasite plot to get size hint for
 * @return Minimum size hint
 */
QSize QwtParasitePlotLayout::minimumSizeHint(const QwtPlot* plot) const
{
    // For parasite axes, minimum size is mainly determined by the enabled axes
    QwtPlot* hostPlot = plot->hostPlot();
    if (hostPlot) {
        QwtPlotLayout* lay = hostPlot->plotLayout();
        if (lay) {
            return lay->minimumSizeHint(hostPlot);
        }
    }
    return QwtPlotLayout::minimumSizeHint(plot);
}

/**
 * @brief Get the parasite axis scale rect without offset
 *
 * Returns the scale rectangle for a parasite axis before it was copied
 * from the host plot layout. This is the original unmodified position.
 *
 * @param aid Axis ID to get scale rect for
 * @return Scale rectangle for the axis
 */
QRectF QwtParasitePlotLayout::parasiteScaleRect(QwtAxisId aid) const
{
    return mScaleRects[ aid ];
}
