/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef QWTCANVASZOOMER_H
#define QWTCANVASZOOMER_H
#include "qwt_global.h"
#include "qwt_picker.h"
#include "qwt_axis_id.h"
#include "qwt_interval.h"
// Qt
class QSizeF;
template< typename T >
class QStack;
// Qwt
class QwtPlot;

/**
 * @brief 存储所有四个坐标轴缩放状态的结构体
 */
struct QWT_EXPORT QwtCanvasZoomState
{
    QwtInterval xBottom;
    QwtInterval xTop;
    QwtInterval yLeft;
    QwtInterval yRight;

    QwtCanvasZoomState() = default;

    // 从plot获取当前所有坐标轴的状态
    static QwtCanvasZoomState fromPlot(const QwtPlot* plot);

    // 将状态应用到plot
    void applyToPlot(QwtPlot* plot) const;

    // 判断两个状态是否相等
    bool operator==(const QwtCanvasZoomState& other) const;
    bool operator!=(const QwtCanvasZoomState& other) const;

    // 检查状态是否有效
    bool isValid() const;
};

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

   \sa QwtPlotPanner, QwtPlotMagnifier
 */
class QWT_EXPORT QwtCanvasZoomer : public QwtPicker
{
    Q_OBJECT

public:
    explicit QwtCanvasZoomer(QWidget* canvas, bool doReplot = true);
    virtual ~QwtCanvasZoomer();

    // 设置缩放基准状态（当前所有坐标轴的范围）
    virtual void setZoomBase(bool doReplot = true);

    // 获取基准缩放状态
    QwtCanvasZoomState zoomBase() const;

    // 获取当前缩放状态
    QwtCanvasZoomState zoomState() const;

    void setMaxStackDepth(int);
    int maxStackDepth() const;

    const QStack< QwtCanvasZoomState >& zoomStack() const;
    void setZoomStack(const QStack< QwtCanvasZoomState >&, int zoomStateIndex = -1);

    uint zoomStateIndex() const;

public Q_SLOTS:
    // 移动当前缩放区域
    void moveBy(double dx, double dy);
    virtual void moveTo(const QPointF& pos);

    // 通过缩放状态进行缩放
    virtual void zoom(const QwtCanvasZoomState& state);

    // 通过索引在缩放栈中导航
    virtual void zoom(int offset);

Q_SIGNALS:
    /*!
       A signal emitted when the plot has been zoomed in or out.
       \param state Current zoom state containing all axis ranges.
     */
    void zoomed(const QwtCanvasZoomState& state);

protected:
    virtual void rescale();
    virtual QSizeF minZoomSize() const;

    virtual void widgetMouseReleaseEvent(QMouseEvent*) QWT_OVERRIDE;
    virtual void widgetKeyPressEvent(QKeyEvent*) QWT_OVERRIDE;

    virtual void begin() QWT_OVERRIDE;
    virtual bool end(bool ok = true) QWT_OVERRIDE;
    virtual bool accept(QPolygon&) const QWT_OVERRIDE;

private:
    void init(bool doReplot);

    // 将画布上的像素矩形转换为四个坐标轴的缩放状态
    QwtCanvasZoomState canvasRectToZoomState(const QRect& pixelRect) const;

    // 移动当前缩放状态
    void moveCurrentState(double dx, double dy);

    class PrivateData;
    PrivateData* m_data;
};

#endif  // QWTCANVASZOOMER_H
