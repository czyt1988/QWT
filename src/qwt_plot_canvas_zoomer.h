/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef QWT_PLOT_CANVAS_ZOOMER_H
#define QWT_PLOT_CANVAS_ZOOMER_H
#include "qwt_global.h"
#include "qwt_canvas_picker.h"
#include "qwt_axis_id.h"
#include "qwt_interval.h"
#include <QPointer>
// Qt
class QSizeF;
template< typename T >
class QStack;
// Qwt
class QwtPlot;

/**
 * @brief 存储所有四个坐标轴缩放状态的结构体
 */
struct QWT_EXPORT QwtPlotCanvasZoomState
{
public:
    QwtPlotCanvasZoomState();
    QwtPlotCanvasZoomState(QwtPlot* p,
                           const QwtInterval& yLeft,
                           const QwtInterval& yRight,
                           const QwtInterval& xBottom,
                           const QwtInterval& xTop);
    // 从plot获取当前所有坐标轴的状态
    static QwtPlotCanvasZoomState fromPlot(QwtPlot* plot);

    // 将状态应用到plot
    void apply() const;

    // 判断两个状态是否相等
    bool operator==(const QwtPlotCanvasZoomState& other) const;
    bool operator!=(const QwtPlotCanvasZoomState& other) const;

    // 检查状态是否有效
    bool isValid() const;

public:
    QPointer< QwtPlot > plot;
    QwtInterval axisInterval[ QwtAxis::AxisPositions ];
};
Q_DECLARE_METATYPE(QwtPlotCanvasZoomState)

/*!
   \brief QwtCanvasZoomer provides zooming for all axes of a plot canvas

   QwtCanvasZoomer selects rectangles from user inputs (mouse or keyboard)
   and adjusts ALL axes of the plot simultaneously. Unlike QwtPlotZoomer which
   only works on two axes, this zoomer works on the entire canvas and maintains
   separate ranges for all four axes.

   The selection is supported by a rubber band and optionally by displaying
   the coordinates of the current mouse position.

   Zooming can be repeated as often as possible, limited only by
   maxStackDepth() or minZoomSize(). Each zoom state is pushed on a stack.

   支持如下鼠标快捷键设置：
   - MouseSelect2，重置回基础
   - MouseSelect3，缩放栈回退
   - MouseSelect6，缩放栈前进

   支持如下键盘快捷键设置：
   - KeyUndo，缩放栈回退
   - KeyRedo，缩放栈前进
   - KeyHome，重置回基础

   \sa QwtPlotPanner, QwtPlotMagnifier
 */
class QWT_EXPORT QwtPlotCanvasZoomer : public QwtCanvasPicker
{
    Q_OBJECT
    QWT_DECLARE_PRIVATE(QwtPlotCanvasZoomer)
public:
    explicit QwtPlotCanvasZoomer(QWidget* canvas, bool doReplot = true);
    virtual ~QwtPlotCanvasZoomer();

    // 设置缩放基准状态（当前所有坐标轴的范围），最小缩放范围会基于此基准进行计算
    virtual void setZoomBase(bool doReplot = true);

    // 设置是否自动replot,默认为true
    void setAutoReplot(bool on = true);
    bool isAutoReplot() const;
    // 获取基准缩放状态
    QList< QwtPlotCanvasZoomState > zoomBase() const;
    // 获取当前缩放状态
    QList< QwtPlotCanvasZoomState > zoomState() const;
    // zoomer记录的最大的缩放次数，如果超过这个次数，不会再让缩放,-1为不限制次数
    void setMaxStackDepth(int);
    int maxStackDepth() const;
    // 缩放栈
    const QStack< QList< QwtPlotCanvasZoomState > >& zoomStack() const;

    uint zoomStateIndex() const;

public Q_SLOTS:
    // 通过索引在缩放栈中导航
    virtual void zoom(int offset);
    virtual void appendZoom(const QList< QwtPlotCanvasZoomState >& rect);
Q_SIGNALS:
    /*!
       A signal emitted when the plot has been zoomed in or out.
       \param state Current zoom state containing all axis ranges.
     */
    void zoomed(const QList< QwtPlotCanvasZoomState >& state);

protected:
    virtual void rescale();

    virtual void widgetMouseReleaseEvent(QMouseEvent*) QWT_OVERRIDE;
    virtual void widgetKeyPressEvent(QKeyEvent*) QWT_OVERRIDE;

    virtual void begin() QWT_OVERRIDE;
    virtual bool end(bool ok = true) QWT_OVERRIDE;
    virtual bool accept(QPolygon&) const QWT_OVERRIDE;

private:
    void init(bool doReplot);

    // 将画布上的像素矩形转换为四个坐标轴的缩放状态
    QList< QwtPlotCanvasZoomState > canvasRectToZoomStateList(const QRect& pixelRect) const;
    // 将画布上的像素矩形转换为四个坐标轴的缩放状态
    QwtPlotCanvasZoomState canvasRectToZoomState(QwtPlot* plt, const QRect& pixelRect) const;
    // 移动当前缩放状态
    void moveCurrentState(double dx, double dy);
};

#endif  // QWTCANVASZOOMER_H
