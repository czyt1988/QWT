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
// qt
#include <qpainter.h>
#include <qpointer.h>
#include <qapplication.h>
#include <qcoreevent.h>
#include <QTimer>

#ifndef QwtPlot_DEBUG_PRINT
#define QwtPlot_DEBUG_PRINT 0
#endif

static inline void qwtEnableLegendItems(QwtPlot* plot, bool on)
{
    // gcc seems to have problems with const char sig[] in combination with certain options
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
        from->setFocusProxy(NULL);

        to->setFocusPolicy(Qt::TabFocus);
        to->setFocusProxy(NULL);

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

    bool autoReplot;
    bool autoReplotTemp;  ///< 用于暂存autoReplot状态

    bool isParasitePlot { false };                                ///< 标记这个绘图是寄生绘图
    QMetaObject::Connection shareConn[ QwtAxis::AxisPositions ];  // 记录寄生轴和宿主轴坐标同步的信号槽，仅仅针对寄生轴有用
};

QwtPlot::PrivateData::PrivateData(QwtPlot* p) : q_ptr(p)
{
}

//----------------------------------------------------
// QwtPlot
//----------------------------------------------------

/*!
   \brief Constructor
   \param parent Parent widget
 */
QwtPlot::QwtPlot(QWidget* parent) : QFrame(parent), QWT_PIMPL_CONSTRUCT
{
    initPlot(QwtText());
}

/*!
   \brief Constructor
   \param title Title text
   \param parent Parent widget
 */
QwtPlot::QwtPlot(const QwtText& title, QWidget* parent) : QFrame(parent), QWT_PIMPL_CONSTRUCT
{
    initPlot(title);
}

//! Destructor
QwtPlot::~QwtPlot()
{
    setAutoReplot(false);
    detachItems(QwtPlotItem::Rtti_PlotItem, autoDelete());
    delete m_data->layout;
    deleteAxesData();
}

/*!
   \brief Initializes a QwtPlot instance
   \param title Title text
 */
void QwtPlot::initPlot(const QwtText& title)
{
    m_data->layout     = new QwtPlotLayout;
    m_data->autoReplot = false;

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
    m_data->legend = NULL;

    // axes
    initAxesData();

    // canvas
    m_data->canvas = new QwtPlotCanvas(this);
    m_data->canvas->setObjectName("QwtPlotCanvas");
    m_data->canvas->installEventFilter(this);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    resize(200, 200);

    using namespace QwtAxis;

    QList< QWidget* > focusChain;
    focusChain << this << m_data->titleLabel << axisWidget(XTop) << axisWidget(YLeft) << m_data->canvas
               << axisWidget(YRight) << axisWidget(XBottom) << m_data->footerLabel;

    for (int i = 0; i < focusChain.size() - 1; i++)
        qwtSetTabOrder(focusChain[ i ], focusChain[ i + 1 ], false);

    qwtEnableLegendItems(this, true);
    // 默认安装一个事件转发器
    setupScaleEventDispatcher(new QwtPlotScaleEventDispatcher(this, this));
}

/**
 * @brief 最顶部的寄生绘图对宿主绘图调用updateAllAxisEdgeMargin
 *
 * 这个函数的目的是，绘图存在寄生绘图时，由于寄生绘图属于宿主的子绘图，宿主绘图调用updateAllAxisEdgeMargin函数时，
 * 寄生绘图的位置可能还没确定，这时，就需要最顶部的寄生绘图在某些情况调用宿主绘图的updateAllAxisEdgeMargin，
 * 典型的应用场景是绘图第一次显示的时候就需要用到此函数
 *
 * @note 如果没有寄生绘图，此函数没有任何动作
 */
void QwtPlot::topParasiteTriggerHostUpdateAxisMargins()
{
    if (isParasitePlot()) {
        // 宿主显示调用host的更新
        if (isTopParasitePlot()) {
            QwtPlot* host = hostPlot();
            if (host) {
                host->updateAllAxisEdgeMargin();
            }
        }
    }
}

/*!
   \brief Set the drawing canvas of the plot widget

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

   \param canvas Canvas Widget
   \sa canvas()
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
   \brief Adds handling of layout requests
   \param event Event

   \return See QFrame::event()
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
   \brief Event filter

   The plot handles the following events for the canvas:

   - QEvent::Resize
    The canvas margins might depend on its size

   - QEvent::ContentsRectChange
    The layout needs to be recalculated

   - 对于寄生轴，寄生轴会过滤宿主轴的尺寸，并调整自己的尺寸

   \param object Object to be filtered
   \param event Event

   \return See QFrame::eventFilter()

   \sa updateCanvasMargins(), updateLayout()
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

//! Replots the plot if autoReplot() is \c true.
void QwtPlot::autoRefresh()
{
    if (m_data->autoReplot) {
        replot();
    }
}

/*!
   \brief Set or reset the autoReplot option

   If the autoReplot option is set, the plot will be
   updated implicitly by manipulating member functions.
   Since this may be time-consuming, it is recommended
   to leave this option switched off and call replot()
   explicitly if necessary.

   The autoReplot option is set to false by default, which
   means that the user has to call replot() in order to make
   changes visible.
   \param tf \c true or \c false. Defaults to \c true.
   \sa replot()
 */
void QwtPlot::setAutoReplot(bool tf)
{
    m_data->autoReplot = tf;
}

/*!
   \return true if the autoReplot option is set.
   \sa setAutoReplot()
 */
bool QwtPlot::autoReplot() const
{
    return m_data->autoReplot;
}

/*!
   Change the plot's title
   \param title New title
 */
void QwtPlot::setTitle(const QString& title)
{
    if (title != m_data->titleLabel->text().text()) {
        m_data->titleLabel->setText(title);
        updateLayout();
    }
}

/*!
   Change the plot's title
   \param title New title
 */
void QwtPlot::setTitle(const QwtText& title)
{
    if (title != m_data->titleLabel->text()) {
        m_data->titleLabel->setText(title);
        updateLayout();
    }
}

//! \return Title of the plot
QwtText QwtPlot::title() const
{
    return m_data->titleLabel->text();
}

//! \return Title label widget.
QwtTextLabel* QwtPlot::titleLabel()
{
    return m_data->titleLabel;
}

//! \return Title label widget.
const QwtTextLabel* QwtPlot::titleLabel() const
{
    return m_data->titleLabel;
}

/*!
   Change the text the footer
   \param text New text of the footer
 */
void QwtPlot::setFooter(const QString& text)
{
    if (text != m_data->footerLabel->text().text()) {
        m_data->footerLabel->setText(text);
        updateLayout();
    }
}

/*!
   Change the text the footer
   \param text New text of the footer
 */
void QwtPlot::setFooter(const QwtText& text)
{
    if (text != m_data->footerLabel->text()) {
        m_data->footerLabel->setText(text);
        updateLayout();
    }
}

//! \return Text of the footer
QwtText QwtPlot::footer() const
{
    return m_data->footerLabel->text();
}

//! \return Footer label widget.
QwtTextLabel* QwtPlot::footerLabel()
{
    return m_data->footerLabel;
}

//! \return Footer label widget.
const QwtTextLabel* QwtPlot::footerLabel() const
{
    return m_data->footerLabel;
}

/*!
   \brief Assign a new plot layout

   \param layout Layout()
   \sa plotLayout()
 */
void QwtPlot::setPlotLayout(QwtPlotLayout* layout)
{
    if (layout != m_data->layout) {
        delete m_data->layout;
        m_data->layout = layout;

        updateLayout();
    }
}

//! \return the plot's layout
QwtPlotLayout* QwtPlot::plotLayout()
{
    return m_data->layout;
}

//! \return the plot's layout
const QwtPlotLayout* QwtPlot::plotLayout() const
{
    return m_data->layout;
}

/*!
   \return the plot's legend
   \sa insertLegend()
 */
QwtAbstractLegend* QwtPlot::legend()
{
    return m_data->legend;
}

/*!
   \return the plot's legend
   \sa insertLegend()
 */
const QwtAbstractLegend* QwtPlot::legend() const
{
    return m_data->legend;
}

/*!
   \return the plot's canvas
 */
QWidget* QwtPlot::canvas()
{
    return m_data->canvas;
}

/*!
   \return the plot's canvas
 */
const QWidget* QwtPlot::canvas() const
{
    return m_data->canvas;
}

/*!
   \return Size hint for the plot widget
   \sa minimumSizeHint()
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
   \brief Return a minimum size hint
 */
QSize QwtPlot::minimumSizeHint() const
{
    QSize hint = m_data->layout->minimumSizeHint(this);
    hint += QSize(2 * frameWidth(), 2 * frameWidth());

    return hint;
}

/*!
   Resize and update internal layout
   \param e Resize event
 */
void QwtPlot::resizeEvent(QResizeEvent* e)
{
    QFrame::resizeEvent(e);
    updateLayout();
    // updateAllAxisEdgeMargin 必须在updateLayout之后执行
    if (isHostPlot()) {
        updateAllAxisEdgeMargin();
    }
}

/*!
   \brief Redraw the plot

   If the autoReplot option is not set (which is the default)
   or if any curves are attached to raw data, the plot has to
   be refreshed explicitly in order to make changes visible.

   \sa updateAxes(), setAutoReplot()
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
 * @brief 重绘所有绘图
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
   \brief Adjust plot content to its current size.
   \sa resizeEvent()
 */
void QwtPlot::updateLayout()
{
    doLayout();
}

/**
 * @brief Adjust plot content to its current size.
 *
 * updateLayout的具体实现，7.0之前的版本没有寄生轴，由于寄生轴的尺寸信息完全参照宿主轴，
 * 因此，在updateLayout过程中，寄生轴不应该执行任何动作，而是等宿主轴的updateLayout最后在对所有寄生轴执行doLayout
 *
 * 所以把updateLayout的所有实现抽到doLayout中
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

    // 宿主轴的layout执行完成后，对所有寄生轴执行doLayout
    if (isHostPlot()) {
        const QList< QwtPlot* > allparasites = parasitePlots();
        // 先设置好尺寸再调整其他
        for (QwtPlot* p : allparasites) {
            p->setGeometry(QRect(0, 0, width(), height()));
        }
    }
}

/*!
   \brief Calculate the canvas margins

   \param maps QwtAxis::AxisCount maps, mapping between plot and paint device coordinates
   \param canvasRect Bounding rectangle where to paint
   \param left Return parameter for the left margin
   \param top Return parameter for the top margin
   \param right Return parameter for the right margin
   \param bottom Return parameter for the bottom margin

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
    for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
        const QwtPlotItem* item = *it;
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
   \brief Update the canvas margins

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
   Redraw the canvas.
   \param painter Painter used for drawing

   \warning drawCanvas calls drawItems what is also used
           for printing. Applications that like to add individual
           plot items better overload drawItems()
   \sa drawItems()
 */
void QwtPlot::drawCanvas(QPainter* painter)
{
    QwtScaleMap maps[ QwtAxis::AxisPositions ];
    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++)
        maps[ axisPos ] = canvasMap(axisPos);

    drawItems(painter, m_data->canvas->contentsRect(), maps);
}

/*!
   Redraw the canvas items.

   \param painter Painter used for drawing
   \param canvasRect Bounding rectangle where to paint
   \param maps QwtAxis::AxisCount maps, mapping between plot and paint device coordinates

   \note Usually canvasRect is contentsRect() of the plot canvas.
        Due to a bug in Qt this rectangle might be wrong for certain
        frame styles ( f.e QFrame::Box ) and it might be necessary to
        fix the margins manually using QWidget::setContentsMargins()
 */

void QwtPlot::drawItems(QPainter* painter, const QRectF& canvasRect, const QwtScaleMap maps[ QwtAxis::AxisPositions ]) const
{
    const QwtPlotItemList& itmList = itemList();
    for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
        QwtPlotItem* item = *it;
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
   \param axisId Axis
   \return Map for the axis on the canvas. With this map pixel coordinates can
          translated to plot coordinates and vice versa.
   \sa QwtScaleMap, transform(), invTransform()
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
   \brief Change the background of the plotting area

   Sets brush to QPalette::Window of all color groups of
   the palette of the canvas. Using canvas()->setPalette()
   is a more powerful way to set these colors.

   \param brush New background brush
   \sa canvasBackground()
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

   \return Background brush of the plotting area.
   \sa setCanvasBackground()
 */
QBrush QwtPlot::canvasBackground() const
{
    return canvas()->palette().brush(QPalette::Normal, QPalette::Window);
}

/*!
   \brief Insert a legend

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

   \param legend Legend
   \param pos The legend's position. For top/left position the number
             of columns will be limited to 1, otherwise it will be set to
             unlimited.

   \param ratio Ratio between legend and the bounding rectangle
               of title, canvas and axes. The legend will be shrunk
               if it would need more space than the given ratio.
               The ratio is limited to ]0.0 .. 1.0]. In case of <= 0.0
               it will be reset to the default ratio.
               The default vertical/horizontal ratio is 0.33/0.5.

   \sa legend(), QwtPlotLayout::legendPosition(),
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

            QwtLegend* lgd = qobject_cast< QwtLegend* >(legend);
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

            QWidget* previousInChain = NULL;
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

   \sa QwtPlotItem::legendData(), legendDataChanged()
 */
void QwtPlot::updateLegend()
{
    const QwtPlotItemList& itmList = itemList();
    for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
        updateLegend(*it);
    }
}

/*!
   Emit legendDataChanged() for a plot item

   \param plotItem Plot item
   \sa QwtPlotItem::legendData(), legendDataChanged()
 */
void QwtPlot::updateLegend(const QwtPlotItem* plotItem)
{
    if (plotItem == NULL)
        return;

    QList< QwtLegendData > legendData;

    if (plotItem->testItemAttribute(QwtPlotItem::Legend))
        legendData = plotItem->legendData();

    const QVariant itemInfo = itemToInfo(const_cast< QwtPlotItem* >(plotItem));
    Q_EMIT legendDataChanged(itemInfo, legendData);
}

/*!
   \brief Update all plot items interested in legend attributes

   Call QwtPlotItem::updateLegend(), when the QwtPlotItem::LegendInterest
   flag is set.

   \param itemInfo Info about the plot item
   \param legendData Entries to be displayed for the plot item ( usually 1 )

   \sa QwtPlotItem::LegendInterest,
      QwtPlotLegendItem, QwtPlotItem::updateLegend()
 */
void QwtPlot::updateLegendItems(const QVariant& itemInfo, const QList< QwtLegendData >& legendData)
{
    QwtPlotItem* plotItem = infoToItem(itemInfo);
    if (plotItem) {
        const QwtPlotItemList& itmList = itemList();
        for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
            QwtPlotItem* item = *it;
            if (item->testItemInterest(QwtPlotItem::LegendInterest))
                item->updateLegend(plotItem, legendData);
        }
    }
}

/**
 * @brief Create parasite axes for this plot/创建一个基于此轴为宿主的寄生轴
 *
 * This method creates a parasite axes that shares the same plotting area as the host plot
 * but with independent axis scaling and labeling. The parasite axes will be positioned
 * exactly on top of the host plot and will automatically synchronize its geometry.
 *
 * 此方法创建一个寄生轴，它与宿主绘图共享相同的绘图区域，但具有独立的轴缩放和标签。
 * 寄生轴将精确定位在宿主绘图之上，并自动同步其几何形状。
 *
 * @param enableAxis The axis position to enable on the parasite axes/在寄生轴上启用的轴位置
 * @return Pointer to the created parasite QwtPlot/指向创建的寄生QwtPlot的指针
 * @retval nullptr if hostPlot is invalid or not in the figure/如果hostPlot无效或不在图形中则返回nullptr
 *
 * @note 一个绘图不会既是寄生绘图也是宿主绘图的情况，也就是说，
 * 如果绘图是寄生绘图，那么他自身不允许再创建寄生绘图，寄生绘图调用次函数将返回nullptr
 */
QwtPlot* QwtPlot::createParasitePlot(QwtAxis::Position enableAxis)
{
    if (isParasitePlot()) {
        qWarning() << "can not create parasite plot on parasite plot";
        return nullptr;
    }
    QwtPlot* parasitePlot = new QwtPlot(this);
    initParasiteAxes(parasitePlot);

    // 根据位置设置轴可见性
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
 * @brief 设置寄生轴是否共享宿主的指定轴，此函数仅针对寄生轴有效
 * @param axisId 指定要共享的轴 ID（如 QwtAxis::YLeft 等）
 * @param isShare 是否启用共享
 * @sa isParasiteShareAxis
 */
void QwtPlot::setParasiteShareAxis(QwtAxisId axisId, bool isShare)
{
    QWT_D(d);
    if (axisId < 0 || axisId >= QwtAxis::AxisPositions) {
        return;  // 非法轴，直接忽略
    }

    QwtPlot* host = hostPlot();
    if (!host) {  // 没有宿主，无法共享
        return;
    }

    // 1. 无论是否要共享，先断开旧连接，避免重复
    if (d->shareConn[ axisId ]) {
        disconnect(d->shareConn[ axisId ]);
    }

    // 2. 按需建立新连接
    if (isShare) {
        QwtScaleWidget* hostAxisWidget = host->axisWidget(axisId);
        if (!hostAxisWidget) {  // 宿主本身没这条轴
            return;
        }
        d->shareConn[ axisId ] = connect(hostAxisWidget, &QwtScaleWidget::scaleDivChanged, this, [ this, axisId ]() {
            syncAxis(axisId, hostPlot());
        });
    }
}

/**
 * @brief 查询寄生轴是否共享宿主的指定轴
 * @param axisId 要查询的轴 ID
 * @return true 表示当前寄生轴共享了宿主对应轴；false 否则
 * @sa setParasiteShareAxis
 */
bool QwtPlot::isParasiteShareAxis(QwtAxisId axisId) const
{
    QWT_DC(d);
    if (axisId < 0 || axisId >= QwtAxis::AxisPositions) {
        return false;  // 非法轴视为“未共享”
    }
    return bool(d->shareConn[ axisId ]);
}

/**
 * @brief Add a parasite plot to this host plot/向此宿主绘图添加寄生绘图
 *
 * This method establishes a parasite relationship where the specified plot will
 * be treated as a parasite of this host plot. The parasite plot will automatically
 * synchronize its geometry with the host plot.
 *
 * 此方法建立一个寄生关系，指定的绘图将被视为此宿主绘图的寄生绘图。
 * 寄生绘图将自动同步其几何形状与宿主绘图。
 *
 * @param parasite Pointer to the parasite QwtPlot/指向寄生QwtPlot的指针
 *
 * @note This method is typically called internally by QwtFigure::createParasiteAxes().
 *       此方法通常由QwtFigure::createParasiteAxes()内部调用。
 * @note The parasite plot should have a transparent background to avoid obscuring the host plot.
 *       寄生绘图应具有透明背景以避免遮挡宿主绘图。
 *
 * @code
 * // Manually create a parasite relationship
 * // 手动创建寄生关系
 * QwtPlot* hostPlot = new QwtPlot;
 * QwtPlot* parasitePlot = new QwtPlot;
 *
 * // Configure parasite plot
 * // 配置寄生绘图
 * parasitePlot->setAutoFillBackground(false);
 * parasitePlot->canvas()->setAutoFillBackground(false);
 * parasitePlot->enableAxis(QwtAxis::YRight, true);
 *
 * // Add parasite to host
 * // 将寄生绘图添加到宿主
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
    // 设定为寄生绘图
    parasite->m_data->isParasitePlot = true;

    // 设置后对寄生轴要进行一次布局
    updateLayout();
}

/**
 * @brief 初始化寄生轴的基本属性
 * @param parasitePlot
 */
void QwtPlot::initParasiteAxes(QwtPlot* parasitePlot) const
{
    // 确保禁用不透明绘制属性
    parasitePlot->setAttribute(Qt::WA_OpaquePaintEvent, false);

    // 禁用样式背景
    parasitePlot->setAttribute(Qt::WA_StyledBackground, false);

    // 禁用自动填充背景
    parasitePlot->setAutoFillBackground(false);

    // 寄生轴绘图和canvas都对宿主透明，让鼠标事件最终都传递到宿主处理
    parasitePlot->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    if (QWidget* c = parasitePlot->canvas()) {
        c->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }

    // 设置透明背景
    QPalette palette = parasitePlot->palette();
    palette.setColor(QPalette::Window, Qt::transparent);
    parasitePlot->setPalette(palette);
    QwtPlotTransparentCanvas* canvas = new QwtPlotTransparentCanvas(parasitePlot);
    parasitePlot->setCanvas(canvas);

    // 调整布局边距
    parasitePlot->setPlotLayout(new QwtParasitePlotLayout());
}

/**
 * @brief Remove a parasite plot from this host plot/从此宿主绘图移除寄生绘图
 *
 * This method removes the parasite relationship between this host plot and the specified plot.
 *
 * 此方法移除此宿主绘图与指定绘图之间的寄生关系。
 *
 * @param parasite Pointer to the parasite QwtPlot to remove/要移除的寄生QwtPlot指针
 *
 * @note This method does not delete the parasite plot, it only removes the relationship.
 *       此方法不会删除寄生绘图，仅移除关系。
 *
 * @code
 * // Remove a parasite plot
 * // 移除寄生绘图
 * hostPlot->removeParasitePlot(parasitePlot);
 *
 * // Now the plot is independent and can be used elsewhere
 * // 现在该绘图是独立的，可以在其他地方使用
 * @endcode
 *
 * @see addParasitePlot(), parasitePlots()
 */
void QwtPlot::removeParasitePlot(QwtPlot* parasite)
{
    if (!parasite) {
        return;
    }
    // 移除时，把绘图的寄生标记设置为false；
    parasite->m_data->isParasitePlot = false;
    updateLayout();
    updateAllAxisEdgeMargin();
}

/**
 * @brief Get all parasite plots associated with this host plot/获取与此宿主绘图关联的所有寄生绘图
 *
 * This method returns a list of all parasite plots that are associated with this host plot.
 *
 * 此方法返回与此宿主绘图关联的所有寄生绘图的列表。
 *
 * @return List of parasite QwtPlot pointers/寄生QwtPlot指针列表
 *
 * @code
 * // Get all parasite plots
 * // 获取所有寄生绘图
 * const QList<QwtPlot*> parasites = hostPlot->parasitePlots();
 *
 * // Perform an operation on all parasite plots
 * // 对所有寄生绘图执行操作
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
 * @brief 返回所有绘图,包含宿主绘图
 *
 * descending=false,增序返回，宿主绘图在第一个，层级越低越靠前，如果descending=true，那么降序返回，宿主在最末端
 * @param descending
 * @return
 */
QList< QwtPlot* > QwtPlot::plotList(bool descending) const
{
    QList< QwtPlot* > plotsByOrder;
    QwtPlot* host = hostPlot();
    if (!host) {
        // 说明当前是宿主
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
 * @brief 获取第n个宿主轴
 * @param index 索引
 * @return 如果超出范围返回nullptr，如果自身是寄生轴，此函数返回空
 */
QwtPlot* QwtPlot::parasitePlotAt(int index) const
{
    const QList< QwtPlot* > ps = parasitePlots();
    return ps.value(index, nullptr);
}

/**
 * @brief 寄生轴的索引（层级）
 *
 * 所谓寄生轴层级，默认是寄生轴的添加顺序，第一个添加的寄生轴为0层，第二个添加的寄生轴为1层，寄生轴层级越高，轴越靠绘图的边界
 * @param parasite 寄生轴
 * @return 如果为-1，说明是无效索引
 * @note 如果传入的寄生轴不是此绘图的寄生轴，返回-1
 * @note 此函数仅对宿主轴有效，如果是寄生轴调用，也将返回-1
 */
int QwtPlot::parasitePlotIndex(QwtPlot* parasite) const
{
    const QList< QwtPlot* > ps = parasitePlots();
    return ps.indexOf(parasite);
}

/**
 * @brief Get the host plot for this parasite plot/获取此寄生绘图的宿主绘图
 *
 * This method returns the host plot of this parasite plot, or nullptr if this plot is not a parasite.
 *
 * 此方法返回此寄生绘图的宿主绘图，如果此绘图不是寄生绘图则返回nullptr。
 *
 * @return Pointer to the host QwtPlot/指向宿主QwtPlot的指针
 * @retval nullptr if this plot is not a parasite plot/如果此绘图不是寄生绘图则返回nullptr
 *
 * @see setHostPlot(), isParasitePlot()
 */
QwtPlot* QwtPlot::hostPlot() const
{
    if (isParasitePlot()) {
        return qobject_cast< QwtPlot* >(parentWidget());
    }
    return nullptr;
}

/**
 * @brief Check if this plot is a parasite plot/检查此绘图是否为寄生绘图
 *
 * This method returns true if this plot is a parasite of another plot.
 *
 * 如果此绘图是另一个绘图的寄生绘图，则此方法返回true。
 *
 * @return true if this plot is a parasite plot/如果此绘图是寄生绘图则返回true
 * @return false if this plot is not a parasite plot/如果此绘图不是寄生绘图则返回false
 *
 * @see isHostPlot(), hostPlot()
 */
bool QwtPlot::isParasitePlot() const
{
    return (m_data->isParasitePlot);
}

/**
 * @brief 是否是最顶部的宿主绘图，最顶部的宿主绘图坐标轴处于最外围，且一般是最后进行更新
 * @return
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
 * @brief Check if this plot is a host plot/检查此绘图是否为宿主绘图
 *
 * This method returns true if this plot has one or more parasite plots.
 *
 * 如果此绘图有一个或多个寄生绘图，则此方法返回true。
 *
 * 只有此绘图持有寄生绘图才会认为是宿主绘图
 *
 * @note 一个绘图不会出现既是寄生绘图也是宿主绘图的情况，也就是宿主自身不允许寄生在其它绘图上
 *
 * @return true if this plot has parasite plots/如果此绘图有寄生绘图则返回true
 * @return false if this plot has no parasite plots/如果此绘图没有寄生绘图则返回false
 *
 * @see isParasitePlot(), parasitePlots()
 */
bool QwtPlot::isHostPlot() const
{
    return !(m_data->isParasitePlot);
}

/**
 * @brief set Background Color/设置背景颜色
 * @param c
 */
void QwtPlot::setBackgroundColor(const QColor& c)
{
    QPalette p = palette();
    p.setColor(QPalette::Window, c);
    setPalette(p);

    setAutoFillBackground(true);
}

/**
 * @brief Background Color/背景颜色
 * @return
 */
QColor QwtPlot::backgroundColor() const
{
    return palette().color(QPalette::Window);
}

/**
 * @brief Synchronize the axis ranges of the corresponding plot/同步plot绘图对应的坐标轴范围到此绘图
 * @param axis
 * @param plot
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
 * @brief Rescale/重新缩放坐标轴以适应所有数据项的范围
 *
 * This function automatically adjusts the axis ranges to fit all visible data items
 * in the plot. It calculates the bounding rectangle of all plot items and sets
 * appropriate axis scales with optional margins.
 *
 * 此函数自动调整坐标轴范围以适应绘图中所有可见数据项。它计算所有绘图项的边界矩形，
 * 并设置适当的坐标轴刻度，可选择添加边距。
 *
 * @param onlyVisibleItems If true, only visible items are considered/如果为true，只考虑可见的绘图项目
 * @param marginPercent Percentage of margin to add around the data range/在数据范围周围添加的边距百分比
 * @param xAxis The x-axis to rescale (default: QwtPlot::xBottom)/需要重新缩放的x轴（默认：QwtPlot::xBottom）
 * @param yAxis The y-axis to rescale (default: QwtPlot::yLeft)/需要重新缩放的y轴（默认：QwtPlot::yLeft）
 *
 *
 * Basic usage/基本用法
 * @code
 * // Rescale to fit all visible items with default 5% margin
 * // 重新缩放以适应所有可见项，默认5%边距
 * rescaleAxes();
 * @endcode
 *
 * Custom margin/自定义边距
 * @code
 * // Rescale with 10% margin around data
 * // 使用10%边距重新缩放
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

    // 遍历所有绘图项/Iterate through all plot items
    const QwtPlotItemList& items = itemList();
    for (QwtPlotItemIterator it = items.begin(); it != items.end(); ++it) {
        QwtPlotItem* item = *it;

        // 如果只处理可见项/If only processing visible items
        if (onlyVisibleItems && !item->isVisible()) {
            continue;
        }

        // 获取该项的边界矩形/Get bounding rectangle of the item
        QRectF boundingRect = item->boundingRect();

        // 检查是否有效/Check if valid
        // 检查边界矩形的有效性，包括 NaN 和无穷大
        if (boundingRect.isValid() && !boundingRect.isEmpty() && std::isfinite(boundingRect.left())
            && std::isfinite(boundingRect.right()) && std::isfinite(boundingRect.top())
            && std::isfinite(boundingRect.bottom())) {

            minX    = std::min(minX, boundingRect.left());
            maxX    = std::max(maxX, boundingRect.right());
            minY    = std::min(minY, boundingRect.top());
            maxY    = std::max(maxY, boundingRect.bottom());
            hasData = true;
        }
    }
    // 如果有数据，则设置坐标范围/If there is data, set axis ranges
    if (hasData) {
        double xMargin = 0;
        double yMargin = 0;
        if (marginPercent > 1) {
        } else {
            // 添加边距/Add margins
            xMargin = (maxX - minX) * marginPercent;
            yMargin = (maxY - minY) * marginPercent;

            setAxisScale(xAxis, minX - xMargin, maxX + xMargin);
            setAxisScale(yAxis, minY - yMargin, maxY + yMargin);
        }
    }
}

/**
 * @brief Set the specified axis to logarithmic scale / 将指定坐标轴设置为对数刻度
 *
 * This method replaces the current scale engine of the axis with QwtLogScaleEngine,
 * enabling logarithmic scaling. All data values must be greater than zero.
 *
 * 此方法将坐标轴当前的刻度引擎替换为 QwtLogScaleEngine，启用对数刻度。所有数据值必须大于零。
 *
 * @param axisId Axis identifier, e.g., QwtPlot::xBottom, QwtPlot::yLeft / 坐标轴标识符，如 QwtPlot::xBottom、QwtPlot::yLeft
 *
 * @note This method deletes the previous scale engine automatically. Data <= 0 will cause undefined behavior.
 *       此方法会自动删除先前的刻度引擎。数据 ≤ 0 将导致未定义行为。
 *
 * @code
 * // Set Y axis to logarithmic scale
 * // 将 Y 轴设置为对数刻度
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
    // setAxisScaleEngine会自动删除旧的 ScaleEngine
    setAxisScaleEngine(axisId, new QwtLogScaleEngine());
}

/**
 * @brief Set the specified axis to date-time scale / 将指定坐标轴设置为日期-时间刻度
 *
 * This method configures the axis to display date-time formatted labels using QwtDateScaleEngine
 * and QwtDateScaleDraw. Data should be provided as milliseconds since epoch (QDateTime::toMSecsSinceEpoch).
 *
 * 此方法使用 QwtDateScaleEngine 和 QwtDateScaleDraw 配置坐标轴以显示日期-时间格式的标签。数据应以自纪元以来的毫秒数提供（QDateTime::toMSecsSinceEpoch）。
 *
 * @param axisId Axis identifier, e.g., QwtPlot::xBottom, QwtPlot::yLeft / 坐标轴标识符，如 QwtPlot::xBottom、QwtPlot::yLeft
 * @param timeSpec Time zone specification, defaults to Qt::LocalTime / 时区规范，默认为 Qt::LocalTime
 *
 * @code
 * // Set X axis to UTC date-time scale
 * // 将 X 轴设置为 UTC 日期-时间刻度
 * plot->setAxisToDateTime(QwtPlot::xBottom, Qt::UTC);
 *
 * QDateTime start = QDateTime::currentDateTime().addSecs(-3600);
 * QVector<double> timestamps, values;
 * for (int i = 0; i < 60; ++i) {
 *     timestamps << start.addSecs(i * 60).toMSecsSinceEpoch(); // per minute / 每分钟
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
 * @brief Restore the specified axis to linear scale / 将指定坐标轴恢复为线性刻度
 *
 * This method replaces the current scale engine and draw with default linear versions.
 * Useful to revert from logarithmic or date-time scales.
 *
 * 此方法将当前刻度引擎和绘制器替换为默认的线性版本。适用于从对数或日期-时间刻度恢复。
 *
 * @param axisId Axis identifier, e.g., QwtPlot::xBottom, QwtPlot::yLeft / 坐标轴标识符，如 QwtPlot::xBottom、QwtPlot::yLeft
 *
 * @note Previous scale engine and draw are deleted automatically.
 *       先前的刻度引擎和绘制器将被自动删除。
 *
 * @code
 * // Switch back to linear scale after using log scale
 * // 在使用对数刻度后切换回线性刻度
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
    setAxisScaleDraw(axisId, new QwtScaleDraw());  // 恢复默认绘制器
}

/**
 * @brief 让寄生轴和宿主轴对齐
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

    // 计算host的应该设置的minbordhint
    // 保证和host的borderhint一致
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
 * @brief 获取宿主轴的个数
 * @return 如果自身就是寄生轴，返回0，否则返回宿主轴的个数
 */
int QwtPlot::parasitePlotCount() const
{
    const QList< QwtPlot* > ps = parasitePlots();
    return ps.size();
}

/**
 * @brief 根据层级顺序重新计算并下发所有层（宿主+寄生）轴的 edgeMargin 与 margin
 *
 * 在 Qwt 多轴体系里，宿主 plot 可以挂载任意数量的寄生 plot，每个寄生 plot
 * 与宿主的画布位置时一样的，但拥有自己独立的坐标轴。为了避免轴层之间重叠，
 * 需要为每条轴动态计算两个偏移量：
 *
 * 1. edgeMargin —— 当前轴到画布边框的距离，由“层级比它高的所有轴”的理论尺寸累加而成。
 * 2. margin —— 当前轴到绘图区的距离，由“层级比它低的所有轴”的理论尺寸累加而成。
 *
 * 这里定义的层级规则：
 * - 宿主始终处于第 0 层（最底层）；
 * - 寄生 plot 按挂载顺序依次构成第 1、2、3... 层，数字越大越靠近画布外侧。
 *
 * 计算流程：
 * 1. 收集宿主及所有可见寄生轴的“净”矩形（已剔除旧的 edgeMargin 与 margin）；
 * 2. 对每i层：
 *    - margin     = 0 ~ i-1 层净矩形尺寸之和；
 *    - edgeMargin = i+1 ~ 末层净矩形尺寸之和；
 * 3. 将新值下发给对应轴的 QwtScaleWidget；
 * 4. 宿主的 margin 予以保留（不覆盖用户可能手工设置的值）。
 *
 * 注意：
 * - 若当前轴不可见或寄生 plot 未使用 QwtParasitePlotLayout，则自动跳过；
 * - 函数内部所有矩形尺寸均按轴方向取宽或高：
 *   Y 轴（YLeft/YRight）取 width，X 轴（XBottom/XTop）取 height。
 *
 * @param axisId 要处理的轴 ID
 *
 * @note 本函数仅修改几何偏移，不会触发布局或重绘；调用方可在必要时
 *       随后调用 hostPlot->updateLayout()。
 *
 * @see QwtPlot::updateAxisEdgeMargin(), QwtScaleWidget::setEdgeMargin(), QwtScaleWidget::setMargin(),
 *      QwtParasitePlotLayout::parasiteScaleRect()
 *
 * @since 7.0.4
 */
void QwtPlot::updateAxisEdgeMargin(QwtAxisId axisId)
{
    // --------------- 1. 收集本次需要处理的所有 plot（含宿主） ---------------
    QwtPlot* host = isHostPlot() ? this : hostPlot();
    if (!host) {
        return;
    }
    if (host->parasitePlotCount() == 0) {
        // 没有寄生绘图不需要处理
        return;
    }
    struct AxisLayer
    {
        QwtPlot* plot = nullptr;
        QRectF scaleRect;  // 已去掉旧 edgeMargin/margin 的“净”矩形
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
    // 宿主总是第 0 层
    QRectF hostScaleRect = host->plotLayout()->scaleRect(axisId);
    // 宿主的矩形修正，宿主只修正edgeMargin作为原始矩形
    hostScaleRect = shrinkRect(hostScaleRect, host->axisWidget(axisId)->edgeMargin(), axisId);
    layers.append({ host, hostScaleRect });
    // 寄生轴按加入顺序构成 1,2,… 层
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

        layers.append({ p, r });
    }
    if (layers.isEmpty()) {
        return;
    }

    for (int i = 0; i < layers.size(); ++i) {
#if QwtPlot_DEBUG_PRINT
        qDebug() << "   layers[" << i << "] scaleRect=" << layers[ i ].scaleRect;
#endif
    }

    // --------------- 2. 计算每层的新 edgeMargin / margin ---------------
    const auto accumulateSize = [ & ](int low, int high) -> int {
        int sum = 0;
        for (int i = low; i < high; ++i) {
            const QRectF& rc = layers[ i ].scaleRect;
            sum += QwtAxis::isYAxis(axisId) ? rc.width() : rc.height();
        }
        return sum;
    };

    for (int i = 0; i < layers.size(); ++i) {
        const int margin     = accumulateSize(0, i);                  // 比我低的层
        const int edgeMargin = accumulateSize(i + 1, layers.size());  // 比我高的层

        QwtScaleWidget* axisWidget = layers[ i ].plot->axisWidget(axisId);
        axisWidget->setEdgeMargin(edgeMargin);
        if (i != 0) {  // 宿主不强制设 margin，保留用户值
            axisWidget->setMargin(margin);
        }
#if QwtPlot_DEBUG_PRINT
        qDebug() << "    [" << i << "]setEdgeMargin(" << edgeMargin << ")";
        qDebug() << "    [" << i << "]setMargin(" << margin << ")";
#endif
    }
}

/**
 * @brief 批量更新所有轴位置的边缘偏移
 *
 * 对当前绘图实例的所有轴位置（YLeft、YRight、XBottom、XTop）依次调用
 * updateAllAxisEdgeMargin(QwtAxisId)，自动完成宿主与所有寄生轴的 edgeMargin
 * 与 margin 同步，保证多轴场景下各层轴之间不重叠且绘图区对齐。
 *
 * 典型调用时机：
 * - 寄生 plot 挂载或移除后；
 * - 轴可见性、标签字体、刻度长度等影响尺寸的属性变更后；
 * - 宿主或寄生轴数据范围变化导致轴标签宽度/高度显著改变时。
 * @see updateAllAxisEdgeMargin(QwtAxisId)
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
        // updateAllAxisEdgeMargin之后更新updateCache
        m_data->scaleEventDispatcher->updateCache();
    }
}

/*!
   \brief Attach/Detach a plot item

   \param plotItem Plot item
   \param on When true attach the item, otherwise detach it
 */
void QwtPlot::attachItem(QwtPlotItem* plotItem, bool on)
{
    if (plotItem->testItemInterest(QwtPlotItem::LegendInterest)) {
        // plotItem is some sort of legend

        const QwtPlotItemList& itmList = itemList();
        for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
            QwtPlotItem* item = *it;

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
   \brief Build an information, that can be used to identify
         a plot item on the legend.

   The default implementation simply wraps the plot item
   into a QVariant object. When overloading itemToInfo()
   usually infoToItem() needs to reimplemeted too.

   \param plotItem Plot item
   \return Plot item embedded in a QVariant
   \sa infoToItem()
 */
QVariant QwtPlot::itemToInfo(QwtPlotItem* plotItem) const
{
    return QVariant::fromValue(plotItem);
}

/*!
   \brief Identify the plot item according to an item info object,
         that has bee generated from itemToInfo().

   The default implementation simply tries to unwrap a QwtPlotItem
   pointer:

   \code
    if ( itemInfo.canConvert<QwtPlotItem *>() )
        return qvariant_cast<QwtPlotItem *>( itemInfo );
   \endcode
   \param itemInfo Plot item
   \return A plot item, when successful, otherwise a NULL pointer.
   \sa itemToInfo()
 */
QwtPlotItem* QwtPlot::infoToItem(const QVariant& itemInfo) const
{
    if (itemInfo.canConvert< QwtPlotItem* >())
        return qvariant_cast< QwtPlotItem* >(itemInfo);

    return NULL;
}

/**
 * @brief 设置坐标轴事件是否可用
 *
 * 坐标轴事件是坐标轴内置的几个事件动作，主要是点击移动坐标轴，鼠标滚轮缩放等功能
 *
 * @param on
 */
void QwtPlot::setEnableScaleBuildinActions(bool on)
{
    m_data->scaleEventDispatcher->setEnable(on);
}

/**
 * @brief 判断坐标轴缩放事件
 * @return
 */
bool QwtPlot::isEnableScaleBuildinActions() const
{
    return m_data->scaleEventDispatcher->isEnable();
}

/**
 * @brief 安装坐标轴事件转发器
 * @param dispatcher
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
 * @brief 保存当前自动绘图设置的状态
 */
void QwtPlot::saveAutoReplotState()
{
    m_data->autoReplotTemp = m_data->autoReplot;
}

/**
 * @brief 恢复当前自动绘图设置的状态
 */
void QwtPlot::restoreAutoReplotState()
{
    m_data->autoReplot = m_data->autoReplotTemp;
}

/**
 * @brief 按像素平移指定坐标轴
 * @param axis 坐标轴ID (QwtPlot::xBottom, QwtPlot::yLeft 等)
 * @param deltaPixels 移动的像素数
 *
 * 正数表示向右/下移动，负数表示向左/上移动
 * 对于对数坐标轴会自动处理坐标变换
 *
 * @note 注意，此函数不会进行重绘，需要调用者手动调用@ref replot
 */
void QwtPlot::panAxis(QwtAxisId axisId, int deltaPixels)
{
    if (!QwtAxis::isValid(axisId)) {
        qWarning() << "invalid axis id:" << axisId;
        return;
    }

    // 获取坐标轴的映射和当前范围
    const QwtScaleDraw* sd        = axisScaleDraw(axisId);
    const QwtScaleMap& map        = sd->scaleMap();
    const QwtScaleDiv& currentDiv = sd->scaleDiv();
    double currentMin             = currentDiv.lowerBound();
    double currentMax             = currentDiv.upperBound();
    //! panAxis分两种情况，第一种，对于线性坐标，只要以点来变换即可，
    //! 但对于对数坐标，为了保证平移之后，在观察者的角度是“平移”，需要轴的前后端同步移动同样像素
    bool isLiner = QwtScaleMap::isLinerScale(map);

    if (isLiner) {
        // 线性坐标轴，变换简单，只需处理一个点的偏移，两个端点同步偏移即可
        // 对于垂直轴，需要考虑坐标方向
        // 数学坐标系：向上移动应该是正值，但屏幕坐标系向下移动是正值
        // 所以需要取反
        // 水平轴向右移动，实际是刻度在减，也是负值，因此这里都是负值
        const double valueDist = map.invTransform(-deltaPixels) - map.invTransform(0);
        setAxisScale(axisId, currentMin + valueDist, currentMax + valueDist);
    } else {
        // 非线性，则要计算两段
        //  获取画布尺寸
        double scalePixelsLength = sd->length();
        if (scalePixelsLength <= 0) {
            return;
        }
        double valueDistMin = 0;
        double valueDistMax = 0;
        if (QwtAxis::isXAxis(axisId)) {
            // 水平轴：向右移动为正，数据向左移动
            valueDistMin = map.invTransform(-deltaPixels) - map.invTransform(0);
            valueDistMax = map.invTransform(scalePixelsLength - deltaPixels) - map.invTransform(scalePixelsLength);
            // 对于对数坐标轴，非常容易移动到非常小的情况，这时不要让它再移动，会导致显示异常
            if (qFuzzyCompare(valueDistMin, valueDistMax)) {
                return;
            }

        } else {
            // 垂直轴：向下移动为正，数据向上移动
            // 注意：垂直轴的像素坐标是从上到下的，与数据坐标方向相反
            // 所以计算方式需要调整
            valueDistMin = map.invTransform(scalePixelsLength) - map.invTransform(scalePixelsLength + deltaPixels);
            valueDistMax = map.invTransform(0) - map.invTransform(deltaPixels);
        }
        if (qFuzzyIsNull(qAbs(valueDistMax - valueDistMin))) {
            return;
        }
        setAxisScale(axisId, currentMin + valueDistMin, currentMax + valueDistMax);
    }
}

/**
 * @brief 按像素偏移平移整个画布
 * @param offset 像素偏移量
 *
 * 该方法会将所有的坐标轴（不管是否已启用）按照指定的像素偏移量进行平移，
 * 实现整个画布的同步移动效果。
 * 水平方向：正数向右移动，负数向左移动
 * 垂直方向：正数向下移动，负数向上移动
 *
 * @note 注意，此函数不会进行重绘，需要调用者手动调用@ref replot
 */
void QwtPlot::panCanvas(const QPoint& offset)
{
    if (offset.isNull()) {
        return;  // 偏移量为零，无需处理
    }

    // 平移所有启用的坐标轴
    for (int axis = 0; axis < QwtPlot::axisCnt; axis++) {
        // 根据坐标轴类型选择相应的偏移分量
        if (QwtAxis::isXAxis(axis)) {
            // 水平轴使用x偏移量
            panAxis(axis, offset.x());
        } else {
            // 垂直轴使用y偏移量（注意方向处理）
            panAxis(axis, offset.y());
        }
    }
}

/**
 * @brief 以指定像素位置为中心缩放坐标轴
 * @param axisId 坐标轴ID
 * @param factor 缩放因子 (大于1表示放大，小于1表示缩小)
 * @param centerPosPixels 缩放中心的像素位置（相对于画布）
 *
 * 缩放原理：
 * - 线性坐标：以鼠标位置为中心进行线性缩放
 * - 对数坐标：以鼠标位置为中心进行对数域的缩放
 *
 * @note 注意，此函数不会进行重绘，需要调用者手动调用@ref replot
 */
void QwtPlot::zoomAxis(QwtAxisId axisId, double factor, const QPoint& centerPosPixels)
{
    if (!QwtAxis::isValid(axisId)) {
        return;
    }
    const QwtScaleDraw* sd      = axisScaleDraw(axisId);
    const QwtScaleMap& scaleMap = sd->scaleMap();
    double currentMin           = scaleMap.s1();  // s1,s2是当前实际点的值
    double currentMax           = scaleMap.s2();
    double center               = QwtAxis::isXAxis(axisId) ? centerPosPixels.x() : centerPosPixels.y();
    // 判断是否为线性坐标
    bool isLinear = QwtScaleMap::isLinerScale(scaleMap);
    if (!isLinear) {
        // 非线性坐标，把数据转换到屏幕坐标系上，这样就能绝对线性
        currentMin = scaleMap.transform(currentMin);
        currentMax = scaleMap.transform(currentMax);
    } else {
        // 对于线性轴，把屏幕的中心点转换为数据的中心点
        center = scaleMap.transform(center);
    }

    qDebug() << "center=" << center << ",factor=" << factor << ",currentMin=" << currentMin << ",currentMax=" << currentMax;
    currentMin = center - (center - currentMin) / factor;
    currentMax = center + (currentMax - center) / factor;
    qDebug() << "after currentMin=" << currentMin << ",currentMax=" << currentMax;
    // 边界检查
    if (currentMin >= currentMax) {
        return;  // 无效范围
    }

    if (!isLinear) {
        currentMin = scaleMap.invTransform(currentMin);
        currentMax = scaleMap.invTransform(currentMax);
    }
    setAxisScale(axisId, currentMin, currentMax);

    return;
    double centerValue { 0 };
    if (isLinear) {
        // 线性坐标缩放
        // 计算缩放后的范围
        // 原理：以鼠标位置为中心，将距离缩放 factor 倍
        // 计算新的最小值和最大值，保持中心点不变
        double newMin = centerValue - (centerValue - currentMin) / factor;
        double newMax = centerValue + (currentMax - centerValue) / factor;

        // 边界检查
        if (newMin >= newMax) {
            return;  // 无效范围
        }

        setAxisScale(axisId, newMin, newMax);
    } else {
        // 对数坐标缩放
        // 将对数坐标转换为线性域进行计算
        double logMin = log10(currentMin);
        double logMax = log10(currentMax);
        // 计算中心点的对数值
        double logCenter = log10(centerValue);

        // 在对数域进行缩放计算
        double newLogMin = logCenter - (logCenter - logMin) / factor;
        double newLogMax = logCenter + (logMax - logCenter) / factor;

        // 转换回数据域
        double newMin = pow(10.0, newLogMin);
        double newMax = pow(10.0, newLogMax);

        // 边界检查
        if (newMin <= 0 || newMax <= 0 || newMin >= newMax) {
            return;  // 无效范围
        }

        setAxisScale(axisId, newMin, newMax);
    }
}
