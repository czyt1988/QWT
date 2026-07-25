/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *
 * Modified by ChenZongYan in 2024 <czy.t@163.com>
 *   Summary of major modifications (see ChangeLog.md for full history):
 *   1. CMake build system & C++11 throughout.
 *   2. Core panner/ zoomer refactored:
 *        - QwtPanner → QwtCachePanner (pixmap-cache version)
 *        - New real-time QwtPlotPanner derived from QwtPicker.
 *   3. Zoomer supports multi-axis.
 *   4. Parasite-plot framework:
 *        - QwtFigure, QwtPlotParasiteLayout, QwtPlotTransparentCanvas,
 *        - QwtPlotScaleEventDispatcher, built-in pan/zoom on axis.
 *   5. New picker: QwtPlotSeriesDataPicker (works with date axis).
 *   6. Raster & color-map extensions:
 *        - QwtGridRasterData (2-D table + interpolation)
 *        - QwtLinearColorMap::stopColors(), stopPos() API rename.
 *   7. Bar-chart: expose pen/brush control.
 *   8. Amalgamated build: single QwtPlot.h / QwtPlot.cpp pair in src-amalgamate.
 *****************************************************************************/
#include <QDebug>
#include <QtMath>
// stl
#include <algorithm>
// qwt
#include "qwt_plot.h"
#include "qwt_plot_dict.h"
#include "qwt_plot_layout.h"
#include "qwt_scale_widget.h"
#include "qwt_scale_engine.h"
#include "qwt_scale_map.h"
#include "qwt_text_label.h"
#include "qwt_legend.h"
#include "qwt_legend_data.h"
#include "qwt_plot_canvas.h"
#include "qwt_math.h"
#include "qwt_interval.h"
#include "qwt_date_scale_engine.h"
#include "qwt_date_scale_draw.h"
#include "qwt_plot_transparent_canvas.h"
#include "qwt_parasite_plot_layout.h"
#include "qwt_plot_scale_event_dispatcher.h"
#include "qwt_painter.h"
#include "qwt_scale_draw.h"
#include "qwt_color_cycle.h"
// qt
#include <qpainter.h>
#include <qpointer.h>
#include <qapplication.h>
#include <qcoreevent.h>
#include <QTimer>
#include <QUuid>

#ifndef QwtPlot_DEBUG_PRINT
#define QwtPlot_DEBUG_PRINT 0
#endif

static inline void qwtEnableLegendItems(QwtPlot* plot, bool on)
{
    // Old-style SIGNAL/SLOT required: updateLegendItems is a private slot,
    // and this is a free function that cannot access private members.
    const char* sig  = SIGNAL(legendDataChanged(QVariant, QList< QwtLegendData >));
    const char* slot = SLOT(updateLegendItems(QVariant, QList< QwtLegendData >));

    if (on)
        QObject::connect(plot, sig, plot, slot);
    else
        QObject::disconnect(plot, sig, plot, slot);
}

static void qwtSetTabOrder(QWidget* first, QWidget* second, bool withChildren)
{
    QList< QWidget* > tabChain;
    tabChain += first;
    tabChain += second;

    if (withChildren) {
        QList< QWidget* > children = second->findChildren< QWidget* >();

        QWidget* w = second->nextInFocusChain();
        while (children.contains(w)) {
            children.removeAll(w);

            tabChain += w;
            w = w->nextInFocusChain();
        }
    }

    for (int i = 0; i < tabChain.size() - 1; i++) {
        QWidget* from = tabChain[ i ];
        QWidget* to   = tabChain[ i + 1 ];

        const Qt::FocusPolicy policy1 = from->focusPolicy();
        const Qt::FocusPolicy policy2 = to->focusPolicy();

        QWidget* proxy1 = from->focusProxy();
        QWidget* proxy2 = to->focusProxy();

        from->setFocusPolicy(Qt::TabFocus);
        from->setFocusProxy(nullptr);

        to->setFocusPolicy(Qt::TabFocus);
        to->setFocusProxy(nullptr);

        QWidget::setTabOrder(from, to);

        from->setFocusPolicy(policy1);
        from->setFocusProxy(proxy1);

        to->setFocusPolicy(policy2);
        to->setFocusProxy(proxy2);
    }
}

class QwtPlot::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlot)
public:
    PrivateData(QwtPlot* p);
    QPointer< QwtTextLabel > titleLabel;
    QPointer< QwtTextLabel > footerLabel;
    QPointer< QWidget > canvas;
    QPointer< QwtAbstractLegend > legend;
    QwtPlotLayout* layout;
    QwtPlotScaleEventDispatcher* scaleEventDispatcher { nullptr };

    bool autoReplot { true };
    bool autoReplotTemp { true };  ///< Used to temporarily store the autoReplot state

    bool isParasitePlot { false };  ///< Marks this plot as a parasite plot
    QMetaObject::Connection shareConn[ QwtAxis::AxisPositions ];  // Records signal-slot connections for syncing parasite axes with host axes, only relevant for parasite axes
    QString plotId;

    // NEW: tick direction for each axis
    TickDirection tickDirection[ QwtAxis::AxisPositions ] = { TickOutside, TickOutside, TickOutside, TickOutside };

    // Color cycle for automatic item coloring
    QwtColorCycle colorCycle;
    int colorCycleCounters[ 20 ] = {};  // per-rtti counter for built-in types
};

QwtPlot::PrivateData::PrivateData(QwtPlot* p) : q_ptr(p)
{
}

//----------------------------------------------------
// QwtPlot
//----------------------------------------------------

/**
 * @brief Constructor
 * @param[in] parent Parent widget
 * @details Creates a QwtPlot widget with an empty title.
 */
QwtPlot::QwtPlot(QWidget* parent) : QFrame(parent), QWT_PIMPL_CONSTRUCT
{
    initPlot(QwtText());
}

/**
 * @brief Constructor with title
 * @param[in] title Title text
 * @param[in] parent Parent widget
 * @details Creates a QwtPlot widget with the specified title.
 */
QwtPlot::QwtPlot(const QwtText& title, QWidget* parent) : QFrame(parent), QWT_PIMPL_CONSTRUCT
{
    initPlot(title);
}

/**
 * @brief Destructor
 * @details Detaches all plot items and deletes the layout and axis data.
 */
QwtPlot::~QwtPlot()
{
    setAutoReplot(false);
    detachItems(QwtPlotItem::Rtti_PlotItem, autoDelete());
    delete m_data->layout;
    deleteAxesData();
}

/*!
   @brief Initializes a QwtPlot instance
   @param title Title text
 */
void QwtPlot::initPlot(const QwtText& title)
{
    m_data->layout         = new QwtPlotLayout;
    m_data->autoReplot     = true;
    m_data->autoReplotTemp = true;

    // title
    m_data->titleLabel = new QwtTextLabel(this);
    m_data->titleLabel->setObjectName("QwtPlotTitle");
    m_data->titleLabel->setFont(QFont(fontInfo().family(), 14, QFont::Bold));

    QwtText text(title);
    text.setRenderFlags(Qt::AlignCenter | Qt::TextWordWrap);
    m_data->titleLabel->setText(text);

    // footer
    m_data->footerLabel = new QwtTextLabel(this);
    m_data->footerLabel->setObjectName("QwtPlotFooter");

    QwtText footer;
    footer.setRenderFlags(Qt::AlignCenter | Qt::TextWordWrap);
    m_data->footerLabel->setText(footer);

    // legend
    m_data->legend = nullptr;

    // axes
    initAxesData();

    // canvas
    m_data->canvas = new QwtPlotCanvas(this);
    m_data->canvas->setObjectName("QwtPlotCanvas");
    m_data->canvas->installEventFilter(this);
    setCanvasBackground(QBrush(Qt::white));

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    // create uuid
    m_data->plotId = QUuid::createUuid().toString();

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    resize(200, 200);

    using namespace QwtAxis;

    QList< QWidget* > focusChain;
    focusChain << this << m_data->titleLabel << axisWidget(XTop) << axisWidget(YLeft) << m_data->canvas
               << axisWidget(YRight) << axisWidget(XBottom) << m_data->footerLabel;

    for (int i = 0; i < focusChain.size() - 1; i++)
        qwtSetTabOrder(focusChain[ i ], focusChain[ i + 1 ], false);

    qwtEnableLegendItems(this, true);
    // Install a default event dispatcher
    setupScaleEventDispatcher(new QwtPlotScaleEventDispatcher(this, this));
}

/**
 * @brief The topmost parasite plot triggers the host plot to call updateAllAxisEdgeMargin
 *
 * The purpose of this function is that when a plot has parasite plots, since parasite plots are
 * children of the host plot, when the host calls updateAllAxisEdgeMargin, the positions of the
 * parasite plots may not yet be determined. In this case, the topmost parasite plot needs to
 * trigger the host's updateAllAxisEdgeMargin in certain situations. A typical use case is
 * when the plot is first displayed.
 *
 * @note If there are no parasite plots, this function does nothing.
 */
void QwtPlot::topParasiteTriggerHostUpdateAxisMargins()
{
    if (isParasitePlot()) {
        // Host explicitly calls the host's update
        if (isTopParasitePlot()) {
            QwtPlot* host = hostPlot();
            if (host) {
                host->updateAllAxisEdgeMargin();
            }
        }
    }
}

/*!
   @brief Set the drawing canvas of the plot widget

   QwtPlot invokes methods of the canvas as meta methods ( see QMetaObject ).
   In opposite to using conventional C++ techniques like virtual methods
   they allow to use canvas implementations that are derived from
   QWidget or QGLWidget.

   The following meta methods could be implemented:

   - replot()
    When the canvas doesn't offer a replot method, QwtPlot calls
    update() instead.

   - borderPath()
    The border path is necessary to clip the content of the canvas
    When the canvas doesn't have any special border ( f.e rounded corners )
    it is o.k. not to implement this method.

   The default canvas is a QwtPlotCanvas

   @param canvas Canvas Widget
   @sa canvas()
 */
void QwtPlot::setCanvas(QWidget* canvas)
{
    if (canvas == m_data->canvas)
        return;

    delete m_data->canvas;
    m_data->canvas = canvas;

    if (canvas) {
        canvas->setParent(this);
        canvas->installEventFilter(this);

        if (isVisible())
            canvas->show();
    }
}

/*!
   @brief Adds handling of layout requests
   @param event Event

   @return See QFrame::event()
 */
bool QwtPlot::event(QEvent* event)
{
    bool ok = QFrame::event(event);
    switch (event->type()) {
    case QEvent::LayoutRequest:
        updateLayout();
        break;
    case QEvent::PolishRequest:
        replot();
        topParasiteTriggerHostUpdateAxisMargins();
        break;
    default:;
    }
    return ok;
}

/*!
   @brief Event filter

   The plot handles the following events for the canvas:

   - QEvent::Resize
    The canvas margins might depend on its size

   - QEvent::ContentsRectChange
    The layout needs to be recalculated

   - For parasite axes, the parasite axis filters the host axis dimensions and adjusts its own dimensions

   @param object Object to be filtered
   @param event Event

   @return See QFrame::eventFilter()

   @sa updateCanvasMargins(), updateLayout()
 */
bool QwtPlot::eventFilter(QObject* object, QEvent* e)
{
    if (object == m_data->canvas) {
        if (e->type() == QEvent::Resize) {
            updateCanvasMargins();
        } else if (e->type() == QEvent::ContentsRectChange) {
            updateLayout();
        }
    }
    return QFrame::eventFilter(object, e);
}

/**
 * @brief Replots the plot if autoReplot() is enabled
 * @details This method is called internally when plot properties change
 *          and autoReplot is enabled.
 */
void QwtPlot::autoRefresh()
{
    if (m_data->autoReplot) {
        replot();
    }
}

/*!
   @brief Set or reset the autoReplot option

   If the autoReplot option is set, the plot will be
   updated implicitly by manipulating member functions.
   Since this may be time-consuming, it is recommended
   to leave this option switched off and call replot()
   explicitly if necessary.

   The autoReplot option is set to false by default, which
   means that the user has to call replot() in order to make
   changes visible.
   @param tf \c true or \c false. Defaults to \c true.
   @sa replot()
 */
void QwtPlot::setAutoReplot(bool tf)
{
    m_data->autoReplot = tf;
}

/**
 * @brief Check if autoReplot option is enabled
 * @return true if the autoReplot option is set
 * @sa setAutoReplot()
 */
bool QwtPlot::autoReplot() const
{
    return m_data->autoReplot;
}

/**
 * @brief Plot ID
 *
 * The plot ID is provided to enable a Figure to identify individual plots during persistence.
 *
 * For example, after saving a Figure in XML format, it is necessary to know which plots have established
 * axis synchronization signals with which other plots, and which plot boundaries have been aligned.
 * These tasks require locating specific plots, which can be achieved using the plot ID.
 *
 * @return A unique UUID
 */
QString QwtPlot::plotId() const
{
    return m_data->plotId;
}

/**
 * @brief Change the plot's title
 * @param[in] title New title as a string
 */
void QwtPlot::setTitle(const QString& title)
{
    if (title != m_data->titleLabel->text().text()) {
        m_data->titleLabel->setText(title);
        updateLayout();
    }
}

/**
 * @brief Change the plot's title
 * @param[in] title New title as a QwtText
 */
void QwtPlot::setTitle(const QwtText& title)
{
    if (title != m_data->titleLabel->text()) {
        m_data->titleLabel->setText(title);
        updateLayout();
    }
}

/**
 * @brief Get the plot's title
 * @return Title of the plot
 */
QwtText QwtPlot::title() const
{
    return m_data->titleLabel->text();
}

/**
 * @brief Get the title label widget
 * @return Title label widget
 */
QwtTextLabel* QwtPlot::titleLabel()
{
    return m_data->titleLabel;
}

/**
 * @brief Get the title label widget
 * @return Title label widget (const)
 */
const QwtTextLabel* QwtPlot::titleLabel() const
{
    return m_data->titleLabel;
}

/**
 * @brief Change the footer text
 * @param[in] text New text of the footer as a string
 */
void QwtPlot::setFooter(const QString& text)
{
    if (text != m_data->footerLabel->text().text()) {
        m_data->footerLabel->setText(text);
        updateLayout();
    }
}

/**
 * @brief Change the footer text
 * @param[in] text New text of the footer as a QwtText
 */
void QwtPlot::setFooter(const QwtText& text)
{
    if (text != m_data->footerLabel->text()) {
        m_data->footerLabel->setText(text);
        updateLayout();
    }
}

/**
 * @brief Get the footer text
 * @return Text of the footer
 */
QwtText QwtPlot::footer() const
{
    return m_data->footerLabel->text();
}

/**
 * @brief Get the footer label widget
 * @return Footer label widget
 */
QwtTextLabel* QwtPlot::footerLabel()
{
    return m_data->footerLabel;
}

/**
 * @brief Get the footer label widget
 * @return Footer label widget (const)
 */
const QwtTextLabel* QwtPlot::footerLabel() const
{
    return m_data->footerLabel;
}

/*!
   @brief Assign a new plot layout

   @param layout Layout()
   @sa plotLayout()
 */
void QwtPlot::setPlotLayout(QwtPlotLayout* layout)
{
    if (layout != m_data->layout) {
        delete m_data->layout;
        m_data->layout = layout;

        updateLayout();
    }
}

/**
 * @brief Get the plot's layout
 * @return The plot's layout
 */
QwtPlotLayout* QwtPlot::plotLayout()
{
    return m_data->layout;
}

/**
 * @brief Get the plot's layout
 * @return The plot's layout (const)
 */
const QwtPlotLayout* QwtPlot::plotLayout() const
{
    return m_data->layout;
}

/**
 * @brief Get the plot's legend
 * @return The plot's legend
 * @sa insertLegend()
 */
QwtAbstractLegend* QwtPlot::legend()
{
    return m_data->legend;
}

/**
 * @brief Get the plot's legend
 * @return The plot's legend (const)
 * @sa insertLegend()
 */
const QwtAbstractLegend* QwtPlot::legend() const
{
    return m_data->legend;
}

/**
 * @brief Get the plot's canvas
 * @return The plot's canvas widget
 */
QWidget* QwtPlot::canvas()
{
    return m_data->canvas;
}

/**
 * @brief Get the plot's canvas
 * @return The plot's canvas widget (const)
 */
const QWidget* QwtPlot::canvas() const
{
    return m_data->canvas;
}

/*!
   @return Size hint for the plot widget
   @sa minimumSizeHint()
 */
QSize QwtPlot::sizeHint() const
{
    int dw = 0;
    int dh = 0;

    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++) {
        {
            const QwtAxisId axisId(axisPos);

            if (isAxisVisible(axisId)) {
                const int niceDist                = 40;
                const QwtScaleWidget* scaleWidget = axisWidget(axisId);
                const QwtScaleDiv& scaleDiv       = scaleWidget->scaleDraw()->scaleDiv();
                const int majCnt                  = scaleDiv.ticks(QwtScaleDiv::MajorTick).count();

                const QSize hint = scaleWidget->minimumSizeHint();

                if (QwtAxis::isYAxis(axisPos)) {
                    const int hDiff = (majCnt - 1) * niceDist - hint.height();
                    dh              = qMax(dh, hDiff);
                } else {
                    const int wDiff = (majCnt - 1) * niceDist - hint.width();
                    dw              = qMax(dw, wDiff);
                }
            }
        }
    }
    return minimumSizeHint() + QSize(dw, dh);
}

/*!
   @brief Return a minimum size hint
 */
QSize QwtPlot::minimumSizeHint() const
{
    QSize hint = m_data->layout->minimumSizeHint(this);
    hint += QSize(2 * frameWidth(), 2 * frameWidth());

    return hint;
}

/*!
   Resize and update internal layout
   @param e Resize event
 */
void QwtPlot::resizeEvent(QResizeEvent* e)
{
    QFrame::resizeEvent(e);
    updateLayout();
    // updateAllAxisEdgeMargin must be executed after updateLayout
    if (isHostPlot()) {
        updateAllAxisEdgeMargin();
    }
}

/*!
   @brief Redraw the plot

   If the autoReplot option is not set (which is the default)
   or if any curves are attached to raw data, the plot has to
   be refreshed explicitly in order to make changes visible.

   @sa updateAxes(), setAutoReplot()
 */
void QwtPlot::replot()
{
    saveAutoReplotState();
    setAutoReplot(false);

    updateAxes();

    /*
       Maybe the layout needs to be updated, because of changed
       axes labels. We need to process them here before painting
       to avoid that scales and canvas get out of sync.
     */
    QApplication::sendPostedEvents(this, QEvent::LayoutRequest);

    if (m_data->canvas) {
        const bool ok = QMetaObject::invokeMethod(m_data->canvas, "replot", Qt::DirectConnection);
        if (!ok) {
            // fallback, when canvas has no a replot method
            m_data->canvas->update(m_data->canvas->contentsRect());
        }
    }
    restoreAutoReplotState();
}

/**
 * @brief Replot all plots
 */
void QwtPlot::replotAll()
{
    const QList< QwtPlot* > allPlot = plotList();
    for (QwtPlot* plot : allPlot) {
        plot->replot();
    }
}

void QwtPlot::autoRefreshAll()
{
    if (m_data->autoReplot) {
        replotAll();
    }
}

/*!
   @brief Adjust plot content to its current size.
   @sa resizeEvent()
 */
void QwtPlot::updateLayout()
{
    doLayout();
}

/**
 * @brief Adjust plot content to its current size.
 *
 * The concrete implementation of updateLayout. Before version 7.0, there were no parasite axes.
 * Since the size information of parasite axes fully references the host axes, during the updateLayout
 * process, parasite axes should not perform any actions. Instead, after the host axis's updateLayout
 * finishes, doLayout is executed for all parasite axes.
 *
 * Therefore, all implementation of updateLayout is extracted into doLayout.
 */
void QwtPlot::doLayout()
{
    QwtPlotLayout* layout = m_data->layout;
    if (isHostPlot()) {
        layout->activate(this, contentsRect());
#if QwtPlot_DEBUG_PRINT
        qDebug() << "host plot do layout:" << contentsRect();
#endif
    } else {
        if (QwtPlot* host = hostPlot()) {
            layout->activate(this, host->contentsRect());
        }
#if QwtPlot_DEBUG_PRINT
        qDebug() << "parasite plot do layout:" << hostPlot()->contentsRect();
#endif
    }

    const QRect titleRect  = layout->titleRect().toRect();
    const QRect footerRect = layout->footerRect().toRect();
    const QRect legendRect = layout->legendRect().toRect();
    const QRect canvasRect = layout->canvasRect().toRect();

    // resize and show the visible widgets

    if (!m_data->titleLabel->text().isEmpty()) {
        m_data->titleLabel->setGeometry(titleRect);
        if (!m_data->titleLabel->isVisibleTo(this))
            m_data->titleLabel->show();
    } else
        m_data->titleLabel->hide();

    if (!m_data->footerLabel->text().isEmpty()) {
        m_data->footerLabel->setGeometry(footerRect);
        if (!m_data->footerLabel->isVisibleTo(this))
            m_data->footerLabel->show();
    } else {
        m_data->footerLabel->hide();
    }

    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++) {
        {
            const QwtAxisId axisId(axisPos);

            QwtScaleWidget* scaleWidget = axisWidget(axisId);
            if (isAxisVisible(axisId)) {

                QRect scaleRect = layout->scaleRect(axisId).toRect();
                if (scaleRect != scaleWidget->geometry()) {
                    scaleWidget->setGeometry(scaleRect);

                    int startDist = 0, endDist = 0;
                    scaleWidget->getBorderDistHint(startDist, endDist);
                    scaleWidget->setBorderDist(startDist, endDist);
                }

                if (!scaleWidget->isVisibleTo(this))
                    scaleWidget->show();
            } else {
                scaleWidget->hide();
            }
        }
    }

    if (m_data->legend) {
        if (m_data->legend->isEmpty()) {
            m_data->legend->hide();
        } else {
            m_data->legend->setGeometry(legendRect);
            m_data->legend->show();
        }
    }

    m_data->canvas->setGeometry(canvasRect);

    // After the host axis layout completes, execute doLayout for all parasite axes
    if (isHostPlot()) {
        const QList< QwtPlot* > allparasites = parasitePlots();
        // Set dimensions first, then adjust the rest
        for (QwtPlot* p : allparasites) {
            p->setGeometry(QRect(0, 0, width(), height()));
        }
    }
}

/**
 * set the plot id.
 *
 * @param id
 */
void QwtPlot::setPlotId(const QString& id)
{
    m_data->plotId = id;
}

/*!
   @brief Calculate the canvas margins

   @param maps QwtAxis::AxisCount maps, mapping between plot and paint device coordinates
   @param canvasRect Bounding rectangle where to paint
   @param left Return parameter for the left margin
   @param top Return parameter for the top margin
   @param right Return parameter for the right margin
   @param bottom Return parameter for the bottom margin

   Plot items might indicate, that they need some extra space
   at the borders of the canvas by the QwtPlotItem::Margins flag.

   updateCanvasMargins(), QwtPlotItem::getCanvasMarginHint()
 */
void QwtPlot::getCanvasMarginsHint(const QwtScaleMap maps[],
                                   const QRectF& canvasRect,
                                   double& left,
                                   double& top,
                                   double& right,
                                   double& bottom) const
{
    left = top = right = bottom = -1.0;

    const QwtPlotItemList& itmList = itemList();
    for (const QwtPlotItem* item : itmList) {
        if (item->testItemAttribute(QwtPlotItem::Margins)) {
            using namespace QwtAxis;

            double m[ AxisPositions ];
            item->getCanvasMarginHint(
                maps[ item->xAxis() ], maps[ item->yAxis() ], canvasRect, m[ YLeft ], m[ XTop ], m[ YRight ], m[ XBottom ]);

            left   = qwtMaxF(left, m[ YLeft ]);
            top    = qwtMaxF(top, m[ XTop ]);
            right  = qwtMaxF(right, m[ YRight ]);
            bottom = qwtMaxF(bottom, m[ XBottom ]);
        }
    }
}

/*!
   @brief Update the canvas margins

   Plot items might indicate, that they need some extra space
   at the borders of the canvas by the QwtPlotItem::Margins flag.

   getCanvasMarginsHint(), QwtPlotItem::getCanvasMarginHint()
 */
void QwtPlot::updateCanvasMargins()
{
    using namespace QwtAxis;

    QwtScaleMap maps[ AxisPositions ];
    for (int axisId = 0; axisId < AxisPositions; axisId++)
        maps[ axisId ] = canvasMap(axisId);

    double margins[ AxisPositions ];
    getCanvasMarginsHint(
        maps, canvas()->contentsRect(), margins[ YLeft ], margins[ XTop ], margins[ YRight ], margins[ XBottom ]);

    bool doUpdate = false;
    for (int axisPos = 0; axisPos < AxisPositions; axisPos++) {
        if (margins[ axisPos ] >= 0.0) {
            const int m = qwtCeil(margins[ axisPos ]);
            plotLayout()->setCanvasMargin(m, axisPos);
            doUpdate = true;
        }
    }

    if (doUpdate)
        updateLayout();
}

/*!
   @brief Draw a single inside tick

   @param painter Painter
   @param tickPixelPos Tick position in pixels
   @param tickLength Tick length
   @param backbonePos Backbone position (canvas edge)
   @param axisPos Axis position (YLeft, YRight, XTop, XBottom)
 */
void QwtPlot::drawSingleInsideTick(QPainter* painter, double tickPixelPos, double tickLength, double backbonePos, int axisPos) const
{
    using namespace QwtAxis;
    const double len = tickLength;

    switch (axisPos) {
    case YLeft:
        // From canvas left edge, draw toward right (inside)
        QwtPainter::drawLine(painter, backbonePos, tickPixelPos, backbonePos + len, tickPixelPos);
        break;

    case YRight:
        // From canvas right edge, draw toward left (inside)
        QwtPainter::drawLine(painter, backbonePos, tickPixelPos, backbonePos - len, tickPixelPos);
        break;

    case XTop:
        // From canvas top edge, draw toward bottom (inside)
        QwtPainter::drawLine(painter, tickPixelPos, backbonePos, tickPixelPos, backbonePos + len);
        break;

    case XBottom:
        // From canvas bottom edge, draw toward top (inside)
        QwtPainter::drawLine(painter, tickPixelPos, backbonePos, tickPixelPos, backbonePos - len);
        break;
    }
}

/*!
   @brief Draw inside ticks for axes with TickInside direction

   This method is called after drawItems() to draw tick marks that
   extend from the canvas edge toward the interior.

   @param painter Painter
   @param canvasRect Canvas rectangle
   @param maps Scale maps for all axes
 */
void QwtPlot::drawInsideTicks(QPainter* painter, const QRectF& canvasRect, const QwtScaleMap maps[ QwtAxis::AxisPositions ]) const
{
    using namespace QwtAxis;

    for (int axisPos = 0; axisPos < AxisPositions; axisPos++) {
        if (axisTickDirection(axisPos) != TickInside)
            continue;

        if (!isAxisVisible(axisPos))
            continue;

        const QwtScaleWidget* scaleWidget = axisWidget(axisPos);
        if (!scaleWidget)
            continue;

        const QwtScaleDraw* scaleDraw = scaleWidget->scaleDraw();
        if (!scaleDraw)
            continue;

        painter->save();

        // Set pen style (sync with outside ticks)
        QPen pen;
        pen.setWidthF(scaleDraw->penWidthF());
        pen.setColor(scaleWidget->scaleColor());
        painter->setPen(pen);

        // Get scale division
        const QwtScaleDiv& scaleDiv = scaleDraw->scaleDiv();
        const QwtScaleMap& map      = maps[ axisPos ];

        // Calculate backbone position (canvas edge)
        double backbonePos;
        switch (axisPos) {
        case YLeft:
            backbonePos = canvasRect.left();
            break;
        case YRight:
            backbonePos = canvasRect.right();
            break;
        case XTop:
            backbonePos = canvasRect.top();
            break;
        case XBottom:
            backbonePos = canvasRect.bottom();
            break;
        }

        // Draw all tick types
        const QList< double >& minorTicks  = scaleDiv.ticks(QwtScaleDiv::MinorTick);
        const QList< double >& mediumTicks = scaleDiv.ticks(QwtScaleDiv::MediumTick);
        const QList< double >& majorTicks  = scaleDiv.ticks(QwtScaleDiv::MajorTick);

        double minorLen  = scaleDraw->tickLength(QwtScaleDiv::MinorTick);
        double mediumLen = scaleDraw->tickLength(QwtScaleDiv::MediumTick);
        double majorLen  = scaleDraw->tickLength(QwtScaleDiv::MajorTick);

        for (const double tickValue : minorTicks) {
            double tickPixelPos = map.transform(tickValue);
            drawSingleInsideTick(painter, tickPixelPos, minorLen, backbonePos, axisPos);
        }

        for (const double tickValue : mediumTicks) {
            double tickPixelPos = map.transform(tickValue);
            drawSingleInsideTick(painter, tickPixelPos, mediumLen, backbonePos, axisPos);
        }

        for (const double tickValue : majorTicks) {
            double tickPixelPos = map.transform(tickValue);
            drawSingleInsideTick(painter, tickPixelPos, majorLen, backbonePos, axisPos);
        }

        painter->restore();
    }
}

/*!
   Redraw the canvas.
   @param painter Painter used for drawing

   @warning drawCanvas calls drawItems what is also used
           for printing. Applications that like to add individual
           plot items better overload drawItems()
   @sa drawItems()
 */
void QwtPlot::drawCanvas(QPainter* painter)
{
    QwtScaleMap maps[ QwtAxis::AxisPositions ];
    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++)
        maps[ axisPos ] = canvasMap(axisPos);

    drawItems(painter, m_data->canvas->contentsRect(), maps);

    // Draw inside ticks after items
    drawInsideTicks(painter, m_data->canvas->contentsRect(), maps);
}

/*!
   Redraw the canvas items.

   @param painter Painter used for drawing
   @param canvasRect Bounding rectangle where to paint
   @param maps QwtAxis::AxisCount maps, mapping between plot and paint device coordinates

   @note Usually canvasRect is contentsRect() of the plot canvas.
        Due to a bug in Qt this rectangle might be wrong for certain
        frame styles ( f.e QFrame::Box ) and it might be necessary to
        fix the margins manually using QWidget::setContentsMargins()
 */

void QwtPlot::drawItems(QPainter* painter, const QRectF& canvasRect, const QwtScaleMap maps[ QwtAxis::AxisPositions ]) const
{
    const QwtPlotItemList& itmList = itemList();
    for (QwtPlotItem* item : itmList) {
        if (item && item->isVisible()) {
            const QwtAxisId xAxis = item->xAxis();
            const QwtAxisId yAxis = item->yAxis();

            painter->save();

            painter->setRenderHint(QPainter::Antialiasing, item->testRenderHint(QwtPlotItem::RenderAntialiased));

#if QT_VERSION < 0x050100
            painter->setRenderHint(QPainter::HighQualityAntialiasing, item->testRenderHint(QwtPlotItem::RenderAntialiased));
#endif

            item->draw(painter, maps[ xAxis ], maps[ yAxis ], canvasRect);

            painter->restore();
        }
    }
}

/*!
   @param axisId Axis
   @return Map for the axis on the canvas. With this map pixel coordinates can
          translated to plot coordinates and vice versa.
   @sa QwtScaleMap, transform(), invTransform()
 */
QwtScaleMap QwtPlot::canvasMap(QwtAxisId axisId) const
{
    QwtScaleMap map;
    if (!m_data->canvas)
        return map;

    map.setTransformation(axisScaleEngine(axisId)->transformation());

    const QwtScaleDiv& sd = axisScaleDiv(axisId);
    map.setScaleInterval(sd.lowerBound(), sd.upperBound());

    if (isAxisVisible(axisId)) {
        const QwtScaleWidget* s = axisWidget(axisId);
        if (QwtAxis::isYAxis(axisId)) {
            double y = s->y() + s->startBorderDist() - m_data->canvas->y();
            double h = s->height() - s->startBorderDist() - s->endBorderDist();
            map.setPaintInterval(y + h, y);
        } else {
            double x = s->x() + s->startBorderDist() - m_data->canvas->x();
            double w = s->width() - s->startBorderDist() - s->endBorderDist();
            map.setPaintInterval(x, x + w);
        }
    } else {
        using namespace QwtAxis;

        const QRect& canvasRect = m_data->canvas->contentsRect();
        if (isYAxis(axisId)) {
            int top = 0;
            if (!plotLayout()->alignCanvasToScale(XTop))
                top = plotLayout()->canvasMargin(XTop);

            int bottom = 0;
            if (!plotLayout()->alignCanvasToScale(XBottom))
                bottom = plotLayout()->canvasMargin(XBottom);

            map.setPaintInterval(canvasRect.bottom() - bottom, canvasRect.top() + top);
        } else {
            int left = 0;
            if (!plotLayout()->alignCanvasToScale(YLeft))
                left = plotLayout()->canvasMargin(YLeft);

            int right = 0;
            if (!plotLayout()->alignCanvasToScale(YRight))
                right = plotLayout()->canvasMargin(YRight);

            map.setPaintInterval(canvasRect.left() + left, canvasRect.right() - right);
        }
    }

    return map;
}

/*!
   @brief Change the background of the plotting area

   Sets brush to QPalette::Window of all color groups of
   the palette of the canvas. Using canvas()->setPalette()
   is a more powerful way to set these colors.

   @param brush New background brush
   @sa canvasBackground()
 */
void QwtPlot::setCanvasBackground(const QBrush& brush)
{
    QPalette pal = m_data->canvas->palette();
    pal.setBrush(QPalette::Window, brush);

    canvas()->setPalette(pal);
}

/*!
   Nothing else than: canvas()->palette().brush(
        QPalette::Normal, QPalette::Window);

   @return Background brush of the plotting area.
   @sa setCanvasBackground()
 */
QBrush QwtPlot::canvasBackground() const
{
    return canvas()->palette().brush(QPalette::Normal, QPalette::Window);
}

/**
 * @brief Set the color cycle used for automatic item coloring
 * @details When plot items (curves, bar charts, etc.) are attached without
 *          a user-specified pen or brush, they automatically receive a color
 *          from this color cycle.
 * @param colorCycle Color cycle object
 * @sa colorCycle(), nextColorForItem()
 */
void QwtPlot::setColorCycle(const QwtColorCycle& colorCycle)
{
    QWT_D(d);
    d->colorCycle = colorCycle;
}

/**
 * @brief Get the current color cycle
 * @return Current color cycle
 * @sa setColorCycle()
 */
QwtColorCycle QwtPlot::colorCycle() const
{
    QWT_DC(d);
    return d->colorCycle;
}

/**
 * @brief Get the next auto-assigned color for a plot item type
 * @details Returns the next color from the color cycle for the given rtti type.
 *          Each rtti type maintains an independent counter, so curves and bar
 *          charts each start from the first palette color.
 * @param rtti The rtti value of the plot item (e.g. QwtPlotItem::Rtti_PlotCurve)
 * @return Next color from the cycle
 * @sa colorCycle(), QwtPlotItem::rtti()
 */
QColor QwtPlot::nextColorForItem(int rtti)
{
    QWT_D(d);
    int idx = 0;
    if (rtti >= 0 && rtti < 20)
        idx = d->colorCycleCounters[ rtti ]++;

    return d->colorCycle.color(idx);
}

/*!
   @brief Insert a legend

   If the position legend is \c QwtPlot::LeftLegend or \c QwtPlot::RightLegend
   the legend will be organized in one column from top to down.
   Otherwise the legend items will be placed in a table
   with a best fit number of columns from left to right.

   insertLegend() will set the plot widget as parent for the legend.
   The legend will be deleted in the destructor of the plot or when
   another legend is inserted.

   Legends, that are not inserted into the layout of the plot widget
   need to connect to the legendDataChanged() signal. Calling updateLegend()
   initiates this signal for an initial update. When the application code
   wants to implement its own layout this also needs to be done for
   rendering plots to a document ( see QwtPlotRenderer ).

   @param legend Legend
   @param pos The legend's position. For top/left position the number
             of columns will be limited to 1, otherwise it will be set to
             unlimited.

   @param ratio Ratio between legend and the bounding rectangle
               of title, canvas and axes. The legend will be shrunk
               if it would need more space than the given ratio.
               The ratio is limited to ]0.0 .. 1.0]. In case of <= 0.0
               it will be reset to the default ratio.
               The default vertical/horizontal ratio is 0.33/0.5.

   @sa legend(), QwtPlotLayout::legendPosition(),
      QwtPlotLayout::setLegendPosition()
 */
void QwtPlot::insertLegend(QwtAbstractLegend* legend, QwtPlot::LegendPosition pos, double ratio)
{
    m_data->layout->setLegendPosition(pos, ratio);

    if (legend != m_data->legend) {
        if (m_data->legend && m_data->legend->parent() == this)
            delete m_data->legend;

        m_data->legend = legend;

        if (m_data->legend) {
            connect(this, &QwtPlot::legendDataChanged, m_data->legend, &QwtAbstractLegend::updateLegend);
            if (m_data->legend->parent() != this)
                m_data->legend->setParent(this);

            qwtEnableLegendItems(this, false);
            updateLegend();
            qwtEnableLegendItems(this, true);

            auto* lgd = qobject_cast< QwtLegend* >(legend);
            if (lgd) {
                switch (m_data->layout->legendPosition()) {
                case LeftLegend:
                case RightLegend: {
                    if (lgd->maxColumns() == 0)
                        lgd->setMaxColumns(1);  // 1 column: align vertical
                    break;
                }
                case TopLegend:
                case BottomLegend: {
                    lgd->setMaxColumns(0);  // unlimited
                    break;
                }
                default:
                    break;
                }
            }

            QWidget* previousInChain = nullptr;
            switch (m_data->layout->legendPosition()) {
            case LeftLegend: {
                const QwtAxisId axisId(QwtAxis::XTop);
                previousInChain = axisWidget(axisId);
                break;
            }
            case TopLegend: {
                previousInChain = this;
                break;
            }
            case RightLegend: {
                const QwtAxisId axisId(QwtAxis::YRight);
                previousInChain = axisWidget(axisId);
                break;
            }
            case BottomLegend: {
                previousInChain = footerLabel();
                break;
            }
            }

            if (previousInChain)
                qwtSetTabOrder(previousInChain, legend, true);
        }
    }

    updateLayout();
}

/*!
   Emit legendDataChanged() for all plot item

   @sa QwtPlotItem::legendData(), legendDataChanged()
 */
void QwtPlot::updateLegend()
{
    const QwtPlotItemList& itmList = itemList();
    for (const QwtPlotItem* item : itmList) {
        updateLegend(item);
    }
}

/*!
   Emit legendDataChanged() for a plot item

   @param plotItem Plot item
   @sa QwtPlotItem::legendData(), legendDataChanged()
 */
void QwtPlot::updateLegend(const QwtPlotItem* plotItem)
{
    if (plotItem == nullptr)
        return;

    QList< QwtLegendData > legendData;

    if (plotItem->testItemAttribute(QwtPlotItem::Legend))
        legendData = plotItem->legendData();

    const QVariant itemInfo = itemToInfo(const_cast< QwtPlotItem* >(plotItem));
    Q_EMIT legendDataChanged(itemInfo, legendData);
}

/*!
   @brief Update all plot items interested in legend attributes

   Call QwtPlotItem::updateLegend(), when the QwtPlotItem::LegendInterest
   flag is set.

   @param itemInfo Info about the plot item
   @param legendData Entries to be displayed for the plot item ( usually 1 )

   @sa QwtPlotItem::LegendInterest,
      QwtPlotLegendItem, QwtPlotItem::updateLegend()
 */
void QwtPlot::updateLegendItems(const QVariant& itemInfo, const QList< QwtLegendData >& legendData)
{
    QwtPlotItem* plotItem = infoToItem(itemInfo);
    if (plotItem) {
        const QwtPlotItemList& itmList = itemList();
        for (QwtPlotItem* item : itmList) {
            if (item->testItemInterest(QwtPlotItem::LegendInterest))
                item->updateLegend(plotItem, legendData);
        }
    }
}

/**
 * @brief Create parasite axes for this plot
 *
 * This method creates a parasite axes that shares the same plotting area as the host plot
 * but with independent axis scaling and labeling. The parasite axes will be positioned
 * exactly on top of the host plot and will automatically synchronize its geometry.
 *
 * @param enableAxis The axis position to enable on the parasite axes
 * @return Pointer to the created parasite QwtPlot
 * @retval nullptr if hostPlot is invalid or not in the figure
 *
 * @note A plot cannot be both a parasite and a host at the same time. That is, if a plot
 * is a parasite plot, it is not allowed to create further parasite plots. Calling this
 * function on a parasite plot will return nullptr.
 */
QwtPlot* QwtPlot::createParasitePlot(QwtAxis::Position enableAxis)
{
    if (isParasitePlot()) {
        qWarning() << "can not create parasite plot on parasite plot";
        return nullptr;
    }
    QwtPlot* parasitePlot = new QwtPlot(this);
    initParasiteAxes(parasitePlot);

    // Set axis visibility based on position
    switch (enableAxis) {
    case QwtAxis::XTop:
        parasitePlot->enableAxis(QwtAxis::XTop, true);
        parasitePlot->enableAxis(QwtAxis::XBottom, false);
        parasitePlot->enableAxis(QwtAxis::YLeft, false);
        parasitePlot->enableAxis(QwtAxis::YRight, false);
        break;
    case QwtAxis::YRight:
        parasitePlot->enableAxis(QwtAxis::XTop, false);
        parasitePlot->enableAxis(QwtAxis::XBottom, false);
        parasitePlot->enableAxis(QwtAxis::YLeft, false);
        parasitePlot->enableAxis(QwtAxis::YRight, true);
        break;
    case QwtAxis::XBottom:
        parasitePlot->enableAxis(QwtAxis::XTop, false);
        parasitePlot->enableAxis(QwtAxis::XBottom, true);
        parasitePlot->enableAxis(QwtAxis::YLeft, false);
        parasitePlot->enableAxis(QwtAxis::YRight, false);
        break;
    case QwtAxis::YLeft:
        parasitePlot->enableAxis(QwtAxis::XTop, false);
        parasitePlot->enableAxis(QwtAxis::XBottom, false);
        parasitePlot->enableAxis(QwtAxis::YLeft, true);
        parasitePlot->enableAxis(QwtAxis::YRight, false);

        break;
    default:
        break;
    }
    addParasitePlot(parasitePlot);
    return parasitePlot;
}

/**
 * @brief Set whether the parasite axis shares the host's specified axis, only valid for parasite axes
 * @param axisId The axis ID to share (e.g., QwtAxis::YLeft)
 * @param isShare Whether to enable sharing
 * @sa isParasiteShareAxis
 */
void QwtPlot::setParasiteShareAxis(QwtAxisId axisId, bool isShare)
{
    QWT_D(d);
    if (axisId < 0 || axisId >= QwtAxis::AxisPositions) {
        return;  // Invalid axis, ignore
    }

    QwtPlot* host = hostPlot();
    if (!host) {  // No host, cannot share
        return;
    }

    // 1. Regardless of whether sharing is needed, disconnect the old connection first to avoid duplicates
    if (d->shareConn[ axisId ]) {
        disconnect(d->shareConn[ axisId ]);
    }

    // 2. Establish new connection as needed
    if (isShare) {
        QwtScaleWidget* hostAxisWidget = host->axisWidget(axisId);
        if (!hostAxisWidget) {  // The host does not have this axis
            return;
        }
        d->shareConn[ axisId ] = connect(hostAxisWidget, &QwtScaleWidget::scaleDivChanged, this, [ this, axisId ]() {
            syncAxis(axisId, hostPlot());
        });
    }
}

/**
 * @brief Query whether the parasite axis shares the host's specified axis
 * @param axisId The axis ID to query
 * @return true if the current parasite axis shares the corresponding host axis; false otherwise
 * @sa setParasiteShareAxis
 */
bool QwtPlot::isParasiteShareAxis(QwtAxisId axisId) const
{
    QWT_DC(d);
    if (axisId < 0 || axisId >= QwtAxis::AxisPositions) {
        return false;  // Invalid axis treated as "not shared"
    }
    return bool(d->shareConn[ axisId ]);
}

/**
 * @brief Add a parasite plot to this host plot
 *
 * This method establishes a parasite relationship where the specified plot will
 * be treated as a parasite of this host plot. The parasite plot will automatically
 * synchronize its geometry with the host plot.
 *
 * @param parasite Pointer to the parasite QwtPlot
 *
 * @note This method is typically called internally by QwtFigure::createParasiteAxes().
 * @note The parasite plot should have a transparent background to avoid obscuring the host plot.
 *
 * @code
 * // Manually create a parasite relationship
 * QwtPlot* hostPlot = new QwtPlot;
 * QwtPlot* parasitePlot = new QwtPlot;
 *
 * // Configure parasite plot
 * parasitePlot->setAutoFillBackground(false);
 * parasitePlot->canvas()->setAutoFillBackground(false);
 * parasitePlot->enableAxis(QwtAxis::YRight, true);
 *
 * // Add parasite to host
 * hostPlot->addParasitePlot(parasitePlot);
 * @endcode
 *
 * @see removeParasitePlot(), parasitePlots()
 */
void QwtPlot::addParasitePlot(QwtPlot* parasite)
{
    if (!parasite) {
        return;
    }
    if (parasite->parentWidget() != this) {
        parasite->setParent(this);
    }
    // Set as parasite plot
    parasite->m_data->isParasitePlot = true;

    // After setting, perform a layout for the parasite axis
    updateLayout();

    Q_EMIT parasitePlotAttached(parasite, true);
}

/**
 * @brief Initialize basic properties of the parasite axes
 * @param parasitePlot The parasite plot to initialize
 */
void QwtPlot::initParasiteAxes(QwtPlot* parasitePlot) const
{
    // Disable opaque paint event attribute
    parasitePlot->setAttribute(Qt::WA_OpaquePaintEvent, false);

    // Disable styled background
    parasitePlot->setAttribute(Qt::WA_StyledBackground, false);

    // Disable auto-fill background
    parasitePlot->setAutoFillBackground(false);

    // Make parasite plot and canvas transparent to host, so mouse events pass through to the host
    parasitePlot->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    if (QWidget* c = parasitePlot->canvas()) {
        c->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }

    // Set transparent background
    QPalette palette = parasitePlot->palette();
    palette.setColor(QPalette::Window, Qt::transparent);
    parasitePlot->setPalette(palette);
    QwtPlotTransparentCanvas* canvas = new QwtPlotTransparentCanvas(parasitePlot);
    parasitePlot->setCanvas(canvas);

    // Adjust layout margins
    parasitePlot->setPlotLayout(new QwtParasitePlotLayout());
}

/**
 * @brief Remove a parasite plot from this host plot
 *
 * This method removes the parasite relationship between this host plot and the specified plot.
 *
 * @param parasite Pointer to the parasite QwtPlot to remove
 *
 * @note This method does not delete the parasite plot, it only removes the relationship.
 *
 * @code
 * // Remove a parasite plot
 * hostPlot->removeParasitePlot(parasitePlot);
 *
 * // Now the plot is independent and can be used elsewhere
 * @endcode
 *
 * @see addParasitePlot(), parasitePlots()
 */
void QwtPlot::removeParasitePlot(QwtPlot* parasite)
{
    if (!parasite) {
        return;
    }
    // When removing, set the parasite flag of the plot to false
    parasite->m_data->isParasitePlot = false;
    updateLayout();
    updateAllAxisEdgeMargin();
    Q_EMIT parasitePlotAttached(parasite, false);
}

/**
 * @brief Get all parasite plots associated with this host plot
 *
 * This method returns a list of all parasite plots that are associated with this host plot.
 *
 * @return List of parasite QwtPlot pointers
 *
 * @code
 * // Get all parasite plots
 * const QList<QwtPlot*> parasites = hostPlot->parasitePlots();
 *
 * // Perform an operation on all parasite plots
 * for (QwtPlot* parasite : parasites) {
 *     parasite->replot();
 * }
 * @endcode
 *
 * @see addParasitePlot(), removeParasitePlot()
 */
QList< QwtPlot* > QwtPlot::parasitePlots() const
{
    return findChildren< QwtPlot* >(QString(), Qt::FindDirectChildrenOnly);
}

/**
 * @brief Return all plots, including the host plot
 *
 * descending=false: ascending order, host plot first, lower levels first.
 * descending=true: descending order, host at the end.
 * @param descending Sort order
 * @return Ordered list of all plots
 */
QList< QwtPlot* > QwtPlot::plotList(bool descending) const
{
    QList< QwtPlot* > plotsByOrder;
    QwtPlot* host = hostPlot();
    if (!host) {
        // This is the host
        host = const_cast< QwtPlot* >(this);
    }
    plotsByOrder.append(host);
    plotsByOrder += host->parasitePlots();
    if (descending) {
        std::reverse(plotsByOrder.begin(), plotsByOrder.end());
    }
    return plotsByOrder;
}

/**
 * @brief Get the nth parasite plot
 * @param index Index
 * @return nullptr if out of range or if this plot is a parasite plot
 */
QwtPlot* QwtPlot::parasitePlotAt(int index) const
{
    const QList< QwtPlot* > ps = parasitePlots();
    return ps.value(index, nullptr);
}

/**
 * @brief Parasite plot index (level)
 *
 * The parasite level is determined by the order of addition. The first parasite added is at level 0,
 * the second at level 1, and so on. The higher the level, the closer the axis is to the plot boundary.
 * @param parasite Parasite plot
 * @return -1 if invalid index
 * @note If the passed parasite is not a parasite of this plot, returns -1
 * @note This function is only valid for host plots; calling it on a parasite plot also returns -1
 */
int QwtPlot::parasitePlotIndex(QwtPlot* parasite) const
{
    const QList< QwtPlot* > ps = parasitePlots();
    return ps.indexOf(parasite);
}

/**
 * @brief Get the host plot for this parasite plot
 *
 * This method returns the host plot of this parasite plot, or nullptr if this plot is not a parasite.
 *
 * @return Pointer to the host QwtPlot
 * @retval nullptr if this plot is not a parasite plot
 *
 * @see isParasitePlot()
 */
QwtPlot* QwtPlot::hostPlot() const
{
    if (isParasitePlot()) {
        return qobject_cast< QwtPlot* >(parentWidget());
    }
    return nullptr;
}

/**
 * @brief Check if this plot is a parasite plot
 *
 * This method returns true if this plot is a parasite of another plot.
 *
 * @return true if this plot is a parasite plot
 * @return false if this plot is not a parasite plot
 *
 * @see isHostPlot(), hostPlot()
 */
bool QwtPlot::isParasitePlot() const
{
    return (m_data->isParasitePlot);
}

/**
 * @brief Whether this is the topmost parasite plot; the topmost parasite's axes are at the outermost layer and are typically updated last
 * @return true if this is the topmost parasite plot
 */
bool QwtPlot::isTopParasitePlot() const
{
    if (!m_data->isParasitePlot) {
        return false;
    }
    QwtPlot* host = hostPlot();
    if (!host) {
        return false;
    }
    auto parasites = host->parasitePlots();
    if (parasites.empty()) {
        return false;
    }
    return (parasites.back() == this);
}

/**
 * @brief Check if this plot is a host plot
 *
 * This method returns true if this plot has one or more parasite plots.
 *
 * A plot is only considered a host if it holds parasite plots.
 *
 * @note A plot cannot be both a parasite and a host at the same time, meaning a host itself is not allowed to be parasitic on another plot.
 *
 * @return true if this plot has parasite plots
 * @return false if this plot has no parasite plots
 *
 * @see isParasitePlot(), parasitePlots()
 */
bool QwtPlot::isHostPlot() const
{
    return !(m_data->isParasitePlot);
}

/**
 * @brief Set background color
 * @param c Color
 */
void QwtPlot::setBackgroundColor(const QColor& c)
{
    QPalette p = palette();
    p.setColor(backgroundRole(), c);
    setPalette(p);

    setAutoFillBackground(true);
}

/**
 * @brief Get background color
 * @return Background color
 */
QColor QwtPlot::backgroundColor() const
{
    return palette().color(backgroundRole());
}

/**
 * @brief Synchronize the axis ranges from the given plot to this plot
 * @param axis Axis ID
 * @param plot Source plot to sync from
 */
void QwtPlot::syncAxis(QwtAxisId axis, const QwtPlot* plot)
{
    if (!plot) {
        return;
    }
    QwtInterval inv = plot->axisInterval(axis);
    setAxisScale(axis, inv.minValue(), inv.maxValue(), plot->axisStepSize(axis));
}

/**
 * @brief Rescale axes to fit all data items
 *
 * This function automatically adjusts the axis ranges to fit all visible data items
 * in the plot. It calculates the bounding rectangle of all plot items and sets
 * appropriate axis scales with optional margins.
 *
 * @param onlyVisibleItems If true, only visible items are considered
 * @param marginPercent Percentage of margin to add around the data range
 * @param xAxis The x-axis to rescale (default: QwtPlot::xBottom)
 * @param yAxis The y-axis to rescale (default: QwtPlot::yLeft)
 *
 *
 * Basic usage:
 * @code
 * // Rescale to fit all visible items with default 5% margin
 * rescaleAxes();
 * @endcode
 *
 * Custom margin:
 * @code
 * // Rescale with 10% margin around data
 * rescaleAxes(true, 0.1);
 * @endcode
 *
 * @see QwtPlotItem::boundingRect()
 * @see QwtPlot::setAxisScale()
 */
void QwtPlot::rescaleAxes(bool onlyVisibleItems, double marginPercent, QwtAxisId xAxis, QwtAxisId yAxis)
{
    double minX  = std::numeric_limits< double >::max();
    double maxX  = std::numeric_limits< double >::lowest();
    double minY  = std::numeric_limits< double >::max();
    double maxY  = std::numeric_limits< double >::lowest();
    bool hasData = false;

    // Iterate through all plot items
    const QwtPlotItemList& items = itemList();
    for (QwtPlotItem* item : items) {
        // If only processing visible items
        if (onlyVisibleItems && !item->isVisible()) {
            continue;
        }

        // Get bounding rectangle of the item
        QRectF boundingRect = item->boundingRect();

        // Check if valid, including NaN and infinity checks
        if (boundingRect.isValid() && !boundingRect.isEmpty() && !qwt_is_nan_or_inf(boundingRect.left())
            && !qwt_is_nan_or_inf(boundingRect.right()) && !qwt_is_nan_or_inf(boundingRect.top())
            && !qwt_is_nan_or_inf(boundingRect.bottom())) {

            minX    = std::min(minX, boundingRect.left());
            maxX    = std::max(maxX, boundingRect.right());
            minY    = std::min(minY, boundingRect.top());
            maxY    = std::max(maxY, boundingRect.bottom());
            hasData = true;
        }
    }
    // If there is data, set axis ranges
    if (hasData) {
        double xMargin = 0;
        double yMargin = 0;
        if (marginPercent > 1) {
        } else {
            // Add margins
            xMargin = (maxX - minX) * marginPercent;
            yMargin = (maxY - minY) * marginPercent;

            setAxisScale(xAxis, minX - xMargin, maxX + xMargin);
            setAxisScale(yAxis, minY - yMargin, maxY + yMargin);
        }
    }
}

/**
 * @brief Set the specified axis to logarithmic scale
 *
 * This method replaces the current scale engine of the axis with QwtLogScaleEngine,
 * enabling logarithmic scaling. All data values must be greater than zero.
 *
 * @param axisId Axis identifier, e.g., QwtPlot::xBottom, QwtPlot::yLeft
 *
 * @note This method deletes the previous scale engine automatically. Data <= 0 will cause undefined behavior.
 *
 * @code
 * // Set Y axis to logarithmic scale
 * plot->setAxisToLogScale(QwtPlot::yLeft);
 *
 * QVector<double> x = {1, 10, 100, 1000};
 * QVector<double> y = {1, 100, 10000, 1e6};
 * QwtPlotCurve *curve = new QwtPlotCurve();
 * curve->setSamples(x, y);
 * curve->attach(plot);
 * plot->replot();
 * @endcode
 *
 * @see setAxisToDateTime(), setAxisToLinearScale(), QwtLogScaleEngine
 */
void QwtPlot::setAxisToLogScale(QwtAxisId axisId)
{
    if (!isAxisValid(axisId)) {
        return;
    }
    // setAxisScaleEngine will automatically delete the old ScaleEngine
    setAxisScaleEngine(axisId, new QwtLogScaleEngine());
}

/**
 * @brief Set the specified axis to date-time scale
 *
 * This method configures the axis to display date-time formatted labels using QwtDateScaleEngine
 * and QwtDateScaleDraw. Data should be provided as milliseconds since epoch (QDateTime::toMSecsSinceEpoch).
 *
 * @param axisId Axis identifier, e.g., QwtPlot::xBottom, QwtPlot::yLeft
 * @param timeSpec Time zone specification, defaults to Qt::LocalTime
 *
 * @code
 * // Set X axis to UTC date-time scale
 * plot->setAxisToDateTime(QwtPlot::xBottom, Qt::UTC);
 *
 * QDateTime start = QDateTime::currentDateTime().addSecs(-3600);
 * QVector<double> timestamps, values;
 * for (int i = 0; i < 60; ++i) {
 *     timestamps << start.addSecs(i * 60).toMSecsSinceEpoch(); // per minute
 *     values << 1.0 + qAbs(qSin(i * 0.2)) * 100;
 * }
 *
 * QwtPlotCurve *curve = new QwtPlotCurve("Data");
 * curve->setSamples(timestamps, values);
 * curve->attach(plot);
 * plot->setAxisScale(QwtPlot::xBottom, timestamps.first(), timestamps.last());
 * plot->updateAxes();
 * plot->replot();
 * @endcode
 *
 * @see setAxisToLogScale(), setAxisToLinearScale(), QwtDateScaleEngine, QDateTime::toMSecsSinceEpoch()
 */
void QwtPlot::setAxisToDateTime(QwtAxisId axisId, Qt::TimeSpec timeSpec)
{
    if (!isAxisValid(axisId)) {
        return;
    }
    QwtDateScaleEngine* dateEngine = new QwtDateScaleEngine(timeSpec);
    QwtDateScaleDraw* dateDraw     = new QwtDateScaleDraw(timeSpec);
    // Set the scale draw and engine
    setAxisScaleDraw(axisId, dateDraw);
    setAxisScaleEngine(axisId, dateEngine);
}

/**
 * @brief Restore the specified axis to linear scale
 *
 * This method replaces the current scale engine and draw with default linear versions.
 * Useful to revert from logarithmic or date-time scales.
 *
 * @param axisId Axis identifier, e.g., QwtPlot::xBottom, QwtPlot::yLeft
 *
 * @note Previous scale engine and draw are deleted automatically.
 *
 * @code
 * // Switch back to linear scale after using log scale
 * plot->setAxisToLinearScale(QwtPlot::yLeft);
 * plot->updateAxes();
 * plot->replot();
 * @endcode
 *
 * @see setAxisToLogScale(), setAxisToDateTime(), QwtLinearScaleEngine
 */
void QwtPlot::setAxisToLinearScale(QwtAxisId axisId)
{
    if (!isAxisValid(axisId)) {
        return;
    }
    setAxisScaleEngine(axisId, new QwtLinearScaleEngine());
    setAxisScaleDraw(axisId, new QwtScaleDraw());  // Restore default draw
}

/*!
   @brief Set the tick direction for an axis

   When set to TickInside, the tick marks are drawn from the canvas edge
   toward the interior. The outside tick component is disabled on the
   QwtScaleWidget, and inside ticks are drawn in drawCanvas.

   @param axisId Axis identifier
   @param direction Tick direction (TickOutside or TickInside)

   @sa axisTickDirection()
 */
void QwtPlot::setAxisTickDirection(QwtAxisId axisId, TickDirection direction)
{
    if (!isAxisValid(axisId))
        return;

    QWT_D(d);
    if (d->tickDirection[ axisId ] != direction) {
        d->tickDirection[ axisId ] = direction;

        // When ticks are inside, disable the outside tick component
        QwtScaleWidget* scaleWidget = axisWidget(axisId);
        if (scaleWidget) {
            QwtScaleDraw* scaleDraw = scaleWidget->scaleDraw();
            if (scaleDraw) {
                if (direction == TickInside) {
                    // Disable outside ticks, keep backbone and labels
                    scaleDraw->enableComponent(QwtAbstractScaleDraw::Ticks, false);
                } else {
                    // Restore outside ticks
                    scaleDraw->enableComponent(QwtAbstractScaleDraw::Ticks, true);
                }
            }

            // Trigger scale widget to recalculate layout and redraw
            // This is necessary because enableComponent() only changes internal flags
            // without triggering any widget update
            scaleWidget->layoutScale();
        }

        autoRefresh();
    }
}

/*!
   @brief Get the tick direction for an axis

   @param axisId Axis identifier
   @return Current tick direction

   @sa setAxisTickDirection()
 */
QwtPlot::TickDirection QwtPlot::axisTickDirection(QwtAxisId axisId) const
{
    if (!isAxisValid(axisId))
        return TickOutside;

    QWT_DC(d);
    return d->tickDirection[ axisId ];
}

/**
 * @brief Align the parasite plot to the host plot
 */
void QwtPlot::alignToHost()
{
    if (isHostPlot()) {
        return;
    }
    QwtPlot* host = hostPlot();
    if (!host) {
        return;
    }

    // Calculate the minBorderDist that should be set for the host
    // Ensure consistency with the host's border hint
    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++) {
        const QwtAxisId axisId(axisPos);
        QwtScaleWidget* scaleWidget = axisWidget(axisId);
        if (isAxisVisible(axisId)) {
            int start = 0, end = 0;
            QwtScaleWidget* hostScaleWidget = host->axisWidget(axisId);
            hostScaleWidget->getMinBorderDist(start, end);
            scaleWidget->setMinBorderDist(start, end);
            hostScaleWidget->getBorderDistHint(start, end);
            scaleWidget->setBorderDist(start, end);
        }
    }
    updateAllAxisEdgeMargin();
}

/**
 * @brief Get the number of parasite plots
 * @return 0 if this plot is a parasite plot, otherwise the count of parasite plots
 */
int QwtPlot::parasitePlotCount() const
{
    const QList< QwtPlot* > ps = parasitePlots();
    return ps.size();
}

/**
 * @brief Recalculate and assign edgeMargin and margin for all layers (host + parasite) axes based on level order
 *
 * In the Qwt multi-axis system, a host plot can mount any number of parasite plots. Each parasite plot
 * shares the same canvas position as the host but has its own independent axes. To avoid overlap between
 * axis layers, two offsets need to be dynamically calculated for each axis:
 *
 * 1. edgeMargin -- The distance from the current axis to the canvas border, accumulated from the
 *    theoretical sizes of all axes at higher levels.
 * 2. margin -- The distance from the current axis to the plot area, accumulated from the
 *    theoretical sizes of all axes at lower levels.
 *
 * Level rules defined here:
 * - The host is always at level 0 (innermost layer);
 * - Parasite plots form levels 1, 2, 3... in the order they are mounted, with larger numbers
 *   being closer to the outside of the canvas.
 *
 * Calculation flow:
 * 1. Collect the "net" rectangles of the host and all visible parasite axes (with old edgeMargin and margin stripped);
 * 2. For each level i:
 *    - margin     = sum of net rectangle sizes from level 0 to i-1;
 *    - edgeMargin = sum of net rectangle sizes from level i+1 to the last level;
 * 3. Assign the new values to the corresponding axis's QwtScaleWidget;
 * 4. The host's margin is preserved (not overwriting values that users may have manually set).
 *
 * Notes:
 * - If the current axis is not visible or the parasite plot does not use QwtParasitePlotLayout, it is skipped automatically;
 * - All rectangle dimensions within the function are taken as width or height based on axis direction:
 *   Y axes (YLeft/YRight) use width, X axes (XBottom/XTop) use height.
 *
 * @param axisId The axis ID to process
 *
 * @note This function only modifies geometric offsets and does not trigger layout or repaint; the caller can
 *       subsequently call hostPlot->updateLayout() if needed.
 *
 * @see QwtScaleWidget::setEdgeMargin(), QwtScaleWidget::setMargin(),
 *      QwtParasitePlotLayout::parasiteScaleRect()
 *
 * @since 7.0.4
 */
void QwtPlot::updateAxisEdgeMargin(QwtAxisId axisId)
{
    // --------------- 1. Collect all plots to process (including host) ---------------
    QwtPlot* host = isHostPlot() ? this : hostPlot();
    if (!host) {
        return;
    }
    if (host->parasitePlotCount() == 0) {
        // No parasite plots, no processing needed
        return;
    }
    struct AxisLayer
    {
        QwtPlot* plot = nullptr;
        QRectF scaleRect;  // "Net" rectangle with old edgeMargin/margin removed
    };
    const auto shrinkRect = [](QRectF r, int delta, QwtAxisId id) -> QRectF {
        if (delta == 0)
            return r;
        switch (id) {
        case QwtAxis::YLeft:
            r.adjust(0, 0, -delta, 0);
            break;
        case QwtAxis::YRight:
            r.adjust(delta, 0, 0, 0);
            break;
        case QwtAxis::XBottom:
            r.adjust(0, delta, 0, 0);
            break;
        case QwtAxis::XTop:
            r.adjust(0, 0, 0, -delta);
            break;
        default:
            break;
        }
        return r;
    };

    const QList< QwtPlot* > parasites = host->parasitePlots();
    QVector< AxisLayer > layers;
    layers.reserve(1 + host->parasitePlots().size());
    // Host is always at level 0
    QRectF hostScaleRect = host->plotLayout()->scaleRect(axisId);
    // Host rectangle correction: only correct edgeMargin as the original rectangle
    hostScaleRect = shrinkRect(hostScaleRect, host->axisWidget(axisId)->edgeMargin(), axisId);
    AxisLayer hostLayer;
    hostLayer.plot      = host;
    hostLayer.scaleRect = hostScaleRect;
    layers.append(hostLayer);
    // Parasite axes form levels 1, 2, ... in order of addition
    for (QwtPlot* p : parasites) {
        if (!p || !p->isAxisVisible(axisId)) {
            continue;
        }
        QwtParasitePlotLayout* play = dynamic_cast< QwtParasitePlotLayout* >(p->plotLayout());
        if (!play)
            continue;

        QRectF r          = play->parasiteScaleRect(axisId);
        const int oldEdge = p->axisWidget(axisId)->edgeMargin();
        const int oldMarg = p->axisWidget(axisId)->margin();
        r                 = shrinkRect(r, oldEdge + oldMarg, axisId);

        AxisLayer pLayer;
        pLayer.plot      = p;
        pLayer.scaleRect = r;
        layers.append(pLayer);
    }
    if (layers.isEmpty()) {
        return;
    }

    for (int i = 0; i < layers.size(); ++i) {
#if QwtPlot_DEBUG_PRINT
        qDebug() << " layers[" << i << "] scaleRect =" << layers[ i ].scaleRect;
#endif
    }

    // --------------- 2. Calculate new edgeMargin / margin for each layer ---------------
    const auto accumulateSize = [ & ](int low, int high) -> int {
        int sum = 0;
        for (int i = low; i < high; ++i) {
            const QRectF& rc = layers[ i ].scaleRect;
            sum += QwtAxis::isYAxis(axisId) ? rc.width() : rc.height();
        }
        return sum;
    };

    for (int i = 0; i < layers.size(); ++i) {
        const int margin     = accumulateSize(0, i);                  // Layers lower than this one
        const int edgeMargin = accumulateSize(i + 1, layers.size());  // Layers higher than this one

        QwtScaleWidget* axisWidget = layers[ i ].plot->axisWidget(axisId);
        axisWidget->setEdgeMargin(edgeMargin);
        if (i != 0) {  // Host does not force-set margin, preserve user value
            axisWidget->setMargin(margin);
        }
#if QwtPlot_DEBUG_PRINT
        qDebug() << " [" << i << "] setEdgeMargin(" << edgeMargin << ")";
        qDebug() << " [" << i << "] setMargin(" << margin << ")";
#endif
    }
}

/**
 * @brief Batch update edge offsets for all axis positions
 *
 * Sequentially calls updateAxisEdgeMargin(QwtAxisId) for all axis positions (YLeft, YRight, XBottom, XTop)
 * of the current plot instance, automatically synchronizing edgeMargin and margin between the host and all
 * parasite axes, ensuring no overlap between layers and proper alignment of plot areas in multi-axis scenarios.
 *
 * Typical invocation timing:
 * - After a parasite plot is mounted or removed;
 * - After property changes that affect dimensions, such as axis visibility, label fonts, or tick lengths;
 * - When axis label width/height changes significantly due to host or parasite axis data range changes.
 * @see updateAxisEdgeMargin(QwtAxisId)
 */
void QwtPlot::updateAllAxisEdgeMargin()
{
#if QwtPlot_DEBUG_PRINT
    static size_t s_c = 0;
    ++s_c;
    qDebug() << "QwtPlot::updateAxisEdgeMargin:" << s_c;
#endif
    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; ++axisPos) {
        updateAxisEdgeMargin(axisPos);
    }
    if (m_data->scaleEventDispatcher) {
        // Update cache after updateAllAxisEdgeMargin
        m_data->scaleEventDispatcher->updateCache();
    }
}

/*!
   @brief Attach/Detach a plot item

   @param plotItem Plot item
   @param on When true attach the item, otherwise detach it
 */
void QwtPlot::attachItem(QwtPlotItem* plotItem, bool on)
{
    if (plotItem->testItemInterest(QwtPlotItem::LegendInterest)) {
        // plotItem is some sort of legend

        const QwtPlotItemList& itmList = itemList();
        for (QwtPlotItem* item : itmList) {
            QList< QwtLegendData > legendData;
            if (on && item->testItemAttribute(QwtPlotItem::Legend)) {
                legendData = item->legendData();
                plotItem->updateLegend(item, legendData);
            }
        }
    }

    if (on)
        insertItem(plotItem);
    else
        removeItem(plotItem);

    Q_EMIT itemAttached(plotItem, on);

    if (plotItem->testItemAttribute(QwtPlotItem::Legend)) {
        // the item wants to be represented on the legend

        if (on) {
            updateLegend(plotItem);
        } else {
            const QVariant itemInfo = itemToInfo(plotItem);
            Q_EMIT legendDataChanged(itemInfo, QList< QwtLegendData >());
        }
    }

    autoRefresh();
}

/*!
   @brief Build an information, that can be used to identify
         a plot item on the legend.

   The default implementation simply wraps the plot item
   into a QVariant object. When overloading itemToInfo()
   usually infoToItem() needs to reimplemeted too.

   @param plotItem Plot item
   @return Plot item embedded in a QVariant
   @sa infoToItem()
 */
QVariant QwtPlot::itemToInfo(QwtPlotItem* plotItem) const
{
    return QVariant::fromValue(plotItem);
}

/*!
   @brief Identify the plot item according to an item info object,
         that has bee generated from itemToInfo().

   The default implementation simply tries to unwrap a QwtPlotItem
   pointer:

   @code
    if ( itemInfo.canConvert<QwtPlotItem *>() )
        return qvariant_cast<QwtPlotItem *>( itemInfo );
   @endcode
   @param itemInfo Plot item
   @return A plot item, when successful, otherwise a nullptr pointer.
   @sa itemToInfo()
 */
QwtPlotItem* QwtPlot::infoToItem(const QVariant& itemInfo) const
{
    if (itemInfo.canConvert< QwtPlotItem* >())
        return qvariant_cast< QwtPlotItem* >(itemInfo);

    return nullptr;
}

/**
 * @brief Enable or disable built-in axis event actions
 *
 * Axis events are built-in event actions for axes, primarily including click-to-move-axis, mouse wheel zoom, etc.
 *
 * @param on Enable state
 */
void QwtPlot::setEnableScaleBuildinActions(bool on)
{
    m_data->scaleEventDispatcher->setEnable(on);
}

/**
 * @brief Check if built-in axis event actions are enabled
 * @return true if enabled
 */
bool QwtPlot::isEnableScaleBuildinActions() const
{
    return m_data->scaleEventDispatcher->isEnable();
}

/**
 * @brief Install the axis event dispatcher
 * @param dispatcher The event dispatcher to install
 */
void QwtPlot::setupScaleEventDispatcher(QwtPlotScaleEventDispatcher* dispatcher)
{
    if (m_data->scaleEventDispatcher) {
        removeEventFilter(m_data->scaleEventDispatcher);
    }
    m_data->scaleEventDispatcher = dispatcher;
    if (dispatcher) {
        installEventFilter(dispatcher);
    }
}

/**
 * @brief Save the current autoReplot state
 */
void QwtPlot::saveAutoReplotState()
{
    m_data->autoReplotTemp = m_data->autoReplot;
}

/**
 * @brief Restore the previously saved autoReplot state
 */
void QwtPlot::restoreAutoReplotState()
{
    m_data->autoReplot = m_data->autoReplotTemp;
}

/**
 * @brief Pan the specified axis by a given number of pixels
 * @param axisId Axis ID (QwtPlot::xBottom, QwtPlot::yLeft, etc.)
 * @param deltaPixels Number of pixels to move
 *
 * Positive values move right/down, negative values move left/up.
 * For logarithmic axes, coordinate transformations are handled automatically.
 *
 * @note This function does not trigger a repaint; the caller must manually call @ref replot
 */
void QwtPlot::panAxis(QwtAxisId axisId, int deltaPixels)
{
    if (!QwtAxis::isValid(axisId)) {
        qWarning() << "invalid axis id:" << axisId;
        return;
    }

    // Get the axis mapping and current range
    const QwtScaleDraw* sd      = axisScaleDraw(axisId);
    const QwtScaleMap& scaleMap = sd->scaleMap();
    // p1 and p2 are the boundaries of the plot dimension
    double minValue = scaleMap.p1();
    double maxValue = scaleMap.p2();
    // Offset the plot distance by deltaPixels manually
    minValue -= deltaPixels;
    maxValue -= deltaPixels;
    // Convert back to scale coordinate system
    minValue = scaleMap.invTransform(minValue);
    maxValue = scaleMap.invTransform(maxValue);
    setAxisScale(axisId, minValue, maxValue);
}

/**
 * @brief Pan the entire canvas by a pixel offset
 * @param offset Pixel offset
 *
 * This method translates all axes (whether enabled or not) by the specified pixel offset,
 * achieving a synchronized panning effect for the entire canvas.
 * Horizontal direction: positive moves right, negative moves left
 * Vertical direction: positive moves down, negative moves up
 *
 * @note This function does not trigger a repaint; the caller must manually call @ref replot
 */
void QwtPlot::panCanvas(const QPoint& offset)
{
    if (offset.isNull()) {
        return;  // Zero offset, nothing to process
    }

    // Pan all enabled axes
    for (int axis = 0; axis < QwtPlot::axisCnt; axis++) {
        // Select the appropriate offset component based on axis type
        if (QwtAxis::isXAxis(axis)) {
            // Horizontal axis uses x offset component
            panAxis(axis, offset.x());
        } else {
            // Vertical axis uses y offset component
            panAxis(axis, offset.y());
        }
    }
}

/**
 * @brief Zoom the axis centered at the specified pixel position
 * @param axisId Axis ID
 * @param factor Zoom factor (>1 for zoom in, <1 for zoom out)
 * @param centerPosPixels Pixel position of the zoom center (relative to canvas)
 *
 * Zoom principle:
 * - Linear axes: zoom centered at the mouse position linearly
 * - Logarithmic axes: zoom centered at the mouse position in the logarithmic domain
 *
 * @note This function does not trigger a repaint; the caller must manually call @ref replot
 */
void QwtPlot::zoomAxis(QwtAxisId axisId, double factor, const QPoint& centerPosPixels)
{
    if (!QwtAxis::isValid(axisId)) {
        return;
    }
    const QwtScaleMap& scaleMap = canvasMap(axisId);
    double currentMin           = scaleMap.s1();  // s1, s2 are the values of the current actual points
    double currentMax           = scaleMap.s2();
    double center               = QwtAxis::isXAxis(axisId) ? centerPosPixels.x() : centerPosPixels.y();
    // Check if transformation is linear
    const QwtTransform* tm = scaleMap.transformation();
    if (tm) {
        // Non-linear coordinate: transform data to screen coordinates for absolute linearity
        currentMin = scaleMap.transform(currentMin);
        currentMax = scaleMap.transform(currentMax);
    } else {
        // For linear axes, convert the screen center point to the data center point (invTransform)
        center = scaleMap.invTransform(center);
    }
    // Clamp center between currentMin and currentMax; for C++11 compatibility we avoid std::clamp here.
    // If C++17 or later is explicitly required, this can be changed to center = std::clamp(center, currentMin, currentMax);
    center = std::max(currentMin, std::min(center, currentMax));

    currentMin = center - (center - currentMin) / factor;
    currentMax = center + (currentMax - center) / factor;
    // Bounds check
    if (currentMin >= currentMax) {
        return;  // Invalid range
    }

    if (tm) {
        currentMin = scaleMap.invTransform(currentMin);
        currentMax = scaleMap.invTransform(currentMax);
        // Avoid excessively small values; for log coordinates, this returns a reasonable range
        currentMin = tm->bounded(currentMin);
        currentMax = tm->bounded(currentMax);
        if (qFuzzyCompare(currentMin, currentMax)) {
            // The two points have become extremely close after zooming
            currentMax = currentMin + 1e-8;
        }
    }
    setAxisScale(axisId, currentMin, currentMax);
}