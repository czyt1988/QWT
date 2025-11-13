/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PLOT_H
#define QWT_PLOT_H

#include "qwt_global.h"
#include "qwt_axis_id.h"
#include "qwt_plot_dict.h"

#include <qframe.h>

class QwtPlotLayout;
class QwtAbstractLegend;
class QwtScaleWidget;
class QwtScaleEngine;
class QwtScaleDiv;
class QwtScaleMap;
class QwtScaleDraw;
class QwtTextLabel;
class QwtInterval;
class QwtText;
class QwtPlotScaleEventDispatcher;

template< typename T >
class QList;

// 6.1 compatibility definitions
#define QWT_AXIS_COMPAT 1

/*!
   \brief A 2-D plotting widget

   QwtPlot is a widget for plotting two-dimensional graphs.
   An unlimited number of plot items can be displayed on
   its canvas. Plot items might be curves (QwtPlotCurve), markers
   (QwtPlotMarker), the grid (QwtPlotGrid), or anything else derived
   from QwtPlotItem.
   A plot can have up to four axes, with each plot item attached to an x- and
   a y axis. The scales at the axes can be explicitly set (QwtScaleDiv), or
   are calculated from the plot items, using algorithms (QwtScaleEngine) which
   can be configured separately for each axis.

   The simpleplot example is a good starting point to see how to set up a
   plot widget.

   \image html plot.png

   \par Example
    The following example shows (schematically) the most simple
    way to use QwtPlot. By default, only the left and bottom axes are
    visible and their scales are computed automatically.
    \code
 #include <qwt_plot.h>
 #include <qwt_plot_curve.h>

      QwtPlot *myPlot = new QwtPlot( "Two Curves", parent );

      // add curves
      QwtPlotCurve *curve1 = new QwtPlotCurve( "Curve 1" );
      QwtPlotCurve *curve2 = new QwtPlotCurve( "Curve 2" );

      // connect or copy the data to the curves
      curve1->setData( ... );
      curve2->setData( ... );

      curve1->attach( myPlot );
      curve2->attach( myPlot );

      // finally, refresh the plot
      myPlot->replot();
    \endcode
 */

class QWT_EXPORT QwtPlot : public QFrame, public QwtPlotDict
{
    Q_OBJECT

    Q_PROPERTY(QBrush canvasBackground READ canvasBackground WRITE setCanvasBackground)

    Q_PROPERTY(bool autoReplot READ autoReplot WRITE setAutoReplot)

    QWT_DECLARE_PRIVATE(QwtPlot)
public:
    /*!
        Position of the legend, relative to the canvas.

        \sa insertLegend()
     */
    enum LegendPosition
    {
        //! The legend will be left from the QwtAxis::YLeft axis.
        LeftLegend,

        //! The legend will be right from the QwtAxis::YRight axis.
        RightLegend,

        //! The legend will be below the footer
        BottomLegend,

        //! The legend will be above the title
        TopLegend
    };

    explicit QwtPlot(QWidget* = NULL);
    explicit QwtPlot(const QwtText& title, QWidget* = NULL);

    virtual ~QwtPlot();

    void setAutoReplot(bool = true);
    bool autoReplot() const;

    // Layout

    void setPlotLayout(QwtPlotLayout*);

    QwtPlotLayout* plotLayout();
    const QwtPlotLayout* plotLayout() const;

    // Title

    void setTitle(const QString&);
    void setTitle(const QwtText&);
    QwtText title() const;

    QwtTextLabel* titleLabel();
    const QwtTextLabel* titleLabel() const;

    // Footer

    void setFooter(const QString&);
    void setFooter(const QwtText&);
    QwtText footer() const;

    QwtTextLabel* footerLabel();
    const QwtTextLabel* footerLabel() const;

    // Canvas

    void setCanvas(QWidget*);

    QWidget* canvas();
    const QWidget* canvas() const;

    void setCanvasBackground(const QBrush&);
    QBrush canvasBackground() const;

    virtual QwtScaleMap canvasMap(QwtAxisId) const;

    double invTransform(QwtAxisId, double pos) const;
    double transform(QwtAxisId, double value) const;

    // Axes

    bool isAxisValid(QwtAxisId) const;

    void setAxisVisible(QwtAxisId, bool on = true);
    bool isAxisVisible(QwtAxisId) const;

    // Axes data

    QwtScaleEngine* axisScaleEngine(QwtAxisId);
    const QwtScaleEngine* axisScaleEngine(QwtAxisId) const;
    void setAxisScaleEngine(QwtAxisId, QwtScaleEngine*);

    void setAxisAutoScale(QwtAxisId, bool on = true);
    bool axisAutoScale(QwtAxisId) const;

    void setAxisFont(QwtAxisId, const QFont&);
    QFont axisFont(QwtAxisId) const;

    void setAxisScale(QwtAxisId, double min, double max, double stepSize = 0);
    void setAxisScaleDiv(QwtAxisId, const QwtScaleDiv&);
    void setAxisScaleDraw(QwtAxisId, QwtScaleDraw*);

    double axisStepSize(QwtAxisId) const;
    QwtInterval axisInterval(QwtAxisId) const;
    const QwtScaleDiv& axisScaleDiv(QwtAxisId) const;

    const QwtScaleDraw* axisScaleDraw(QwtAxisId) const;
    QwtScaleDraw* axisScaleDraw(QwtAxisId);

    const QwtScaleWidget* axisWidget(QwtAxisId) const;
    QwtScaleWidget* axisWidget(QwtAxisId);

    // 获取一个有效的x轴，有效的定义顺序：1.可见、2.XBottom优先、3.指针有效，因此如果xBotton和xTop都可见时，先放回xBottom
    QwtAxisId visibleXAxisId() const;
    QwtAxisId visibleYAxisId() const;

    void setAxisLabelAlignment(QwtAxisId, Qt::Alignment);
    void setAxisLabelRotation(QwtAxisId, double rotation);

    void setAxisTitle(QwtAxisId, const QString&);
    void setAxisTitle(QwtAxisId, const QwtText&);
    QwtText axisTitle(QwtAxisId) const;

    void setAxisMaxMinor(QwtAxisId, int maxMinor);
    int axisMaxMinor(QwtAxisId) const;

    void setAxisMaxMajor(QwtAxisId, int maxMajor);
    int axisMaxMajor(QwtAxisId) const;

    // Legend

    void insertLegend(QwtAbstractLegend*, LegendPosition = QwtPlot::RightLegend, double ratio = -1.0);

    QwtAbstractLegend* legend();
    const QwtAbstractLegend* legend() const;

    void updateLegend();
    void updateLegend(const QwtPlotItem*);

    // Misc

    virtual QSize sizeHint() const QWT_OVERRIDE;
    virtual QSize minimumSizeHint() const QWT_OVERRIDE;

    virtual void updateLayout();
    virtual void drawCanvas(QPainter*);

    void updateAxes();
    void updateCanvasMargins();

    virtual void getCanvasMarginsHint(const QwtScaleMap maps[],
                                      const QRectF& canvasRect,
                                      double& left,
                                      double& top,
                                      double& right,
                                      double& bottom) const;

    virtual bool event(QEvent*) QWT_OVERRIDE;
    virtual bool eventFilter(QObject*, QEvent*) QWT_OVERRIDE;

    virtual void drawItems(QPainter*, const QRectF&, const QwtScaleMap maps[ QwtAxis::AxisPositions ]) const;

    virtual QVariant itemToInfo(QwtPlotItem*) const;
    virtual QwtPlotItem* infoToItem(const QVariant&) const;

    // add since v7.1.0

    // 创建一个基于此轴为宿主的寄生轴
    QwtPlot* createParasitePlot(QwtAxis::Position enableAxis);

    // 设置寄生轴共享宿主的轴是哪些，此函数仅针对寄生轴有效
    void setParasiteShareAxis(QwtAxisId axisId, bool isShare = true);

    // 获取寄生轴是和宿主轴的哪些轴共享，此函数仅针对寄生轴有效
    bool isParasiteShareAxis(QwtAxisId axisId) const;
    // Remove a parasite plot from this host plot/从此宿主绘图移除寄生绘图
    void removeParasitePlot(QwtPlot* parasite);

    // Get all parasite plots associated with this host plot/获取与此宿主绘图关联的所有寄生绘图
    QList< QwtPlot* > parasitePlots() const;
    // 返回所有绘图,包含宿主绘图，descending=false,增序返回，宿主绘图在第一个，层级越低越靠前，如果descending=true，那么降序返回，宿主在最末端
    QList< QwtPlot* > plotList(bool descending = true) const;
    // 获取第n个宿主轴
    QwtPlot* parasitePlotAt(int index) const;

    // 寄生轴的索引（层级），所谓寄生轴层级，默认是寄生轴的添加顺序，第一个添加的寄生轴为0层，第二个添加的寄生轴为1层，寄生轴层级越高，轴越靠绘图的边界
    int parasitePlotIndex(QwtPlot* parasite) const;

    // Get the host plot for this parasite plot/获取此寄生绘图的宿主绘图
    QwtPlot* hostPlot() const;

    // Check if this plot is a parasite plot/检查此绘图是否为寄生绘图
    bool isParasitePlot() const;

    // Check if this plot is a host plot/检查此绘图是否为宿主绘图
    bool isHostPlot() const;

    // set Background Color/设置背景颜色
    void setBackgroundColor(const QColor& c);
    QColor backgroundColor() const;

    // Synchronize the axis ranges of the corresponding plot/同步plot对应的坐标轴范围
    void syncAxis(QwtAxisId axis, const QwtPlot* plot);
    // Rescale the axes to encompass the full range of all data items./重新缩放坐标轴以适应所有数据项的范围
    void rescaleAxes(bool onlyVisibleItems = true,
                     double marginPercent  = 0.05,
                     QwtAxisId xAxis       = QwtPlot::xBottom,
                     QwtAxisId yAxis       = QwtPlot::yLeft);

    // Set the specified axis to logarithmic scale / 将指定坐标轴设置为对数刻度
    void setAxisToLogScale(QwtAxisId axisId);

    // Set the specified axis to date-time scale / 将指定坐标轴设置为日期-时间刻度
    void setAxisToDateTime(QwtAxisId axisId, Qt::TimeSpec timeSpec = Qt::LocalTime);

    // Restore the specified axis to linear scale / 将指定坐标轴恢复为线性刻度
    void setAxisToLinearScale(QwtAxisId axisId);

    // 让寄生轴和宿主轴对齐
    void alignToHost();

    // 获取宿主轴的个数
    int parasitePlotCount() const;

    // 更新宿主轴和寄生轴的偏移
    void updateAxisEdgeMargin(QwtAxisId axisId);
    void updateAxisEdgeMargin();
    // 更新绘图上的items，让其适配scaleDiv的范围
    void updateItemsToScaleDiv();
    // 坐标轴事件使能
    void setEnableScaleBuildinActions(bool on);
    bool isEnableScaleBuildinActions() const;
    // 设置坐标轴事件转发器，这个是实现坐标轴事件的主要管理类
    void setupScaleEventDispatcher(QwtPlotScaleEventDispatcher* dispatcher);
    // 保存/恢复当前自动绘图设置的状态
    void saveAutoReplotState();
    void restoreAutoReplotState();
#if QWT_AXIS_COMPAT
    enum Axis
    {
        yLeft   = QwtAxis::YLeft,
        yRight  = QwtAxis::YRight,
        xBottom = QwtAxis::XBottom,
        xTop    = QwtAxis::XTop,

        axisCnt = QwtAxis::AxisPositions
    };

    void enableAxis(int axisId, bool on = true)
    {
        setAxisVisible(axisId, on);
    }

    bool axisEnabled(int axisId) const
    {
        return isAxisVisible(axisId);
    }
#endif

Q_SIGNALS:
    /*!
       A signal indicating, that an item has been attached/detached

       \param plotItem Plot item
       \param on Attached/Detached
     */
    void itemAttached(QwtPlotItem* plotItem, bool on);

    /*!
       A signal with the attributes how to update
       the legend entries for a plot item.

       \param itemInfo Info about a plot item, build from itemToInfo()
       \param data Attributes of the entries ( usually <= 1 ) for
                  the plot item.

       \sa itemToInfo(), infoToItem(), QwtAbstractLegend::updateLegend()
     */
    void legendDataChanged(const QVariant& itemInfo, const QList< QwtLegendData >& data);

public Q_SLOTS:
    virtual void replot();
    void autoRefresh();
    // 重绘所有绘图，包括寄生绘图或者宿主绘图
    virtual void replotAll();

protected:
    virtual void resizeEvent(QResizeEvent*) QWT_OVERRIDE;
    // Add a parasite plot to this host plot/向此宿主绘图添加寄生绘图
    void addParasitePlot(QwtPlot* parasite);
    // 初始化寄生轴的基本属性
    void initParasiteAxes(QwtPlot* parasitePlot) const;
    // updateLayout的具体实现
    void doLayout();
private Q_SLOTS:
    void updateLegendItems(const QVariant& itemInfo, const QList< QwtLegendData >& legendData);
    void yLeftRequestScaleRangeUpdate(double min, double max);
    void yRightRequestScaleRangeUpdate(double min, double max);
    void xBottomRequestScaleRangeUpdate(double min, double max);
    void xTopRequestScaleRangeUpdate(double min, double max);

private:
    friend class QwtPlotItem;
    void attachItem(QwtPlotItem*, bool);

    void initAxesData();
    void deleteAxesData();

    void initPlot(const QwtText& title);

    class ScaleData;
    ScaleData* m_scaleData;
};

#endif
