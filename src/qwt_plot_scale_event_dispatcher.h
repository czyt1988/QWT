#ifndef QWTPLOTSCALEEVENTDISPATCHER_H
#define QWTPLOTSCALEEVENTDISPATCHER_H
#include <QObject>
#include "qwt_global.h"
class QMouseEvent;
class QWheelEvent;
class QwtPlot;
class QwtScaleWidget;
/**
 * @brief 针对寄生绘图的事件过滤器，主要处理坐标轴动作
 *
 * 由于寄生绘图属于宿主绘图的子窗口，多个寄生绘图的部件的事件无法传递到其它寄生绘图的部件上
 *
 * 例如有2个寄生绘图，寄生绘图的轴区域是重叠覆盖的，最顶层的寄生绘图的坐标轴窗口是和下面层级及宿主绘图的坐标轴窗口尺寸是一样的，
 * 也就是说，对于一个坐标轴窗口的事件，只有最顶层的寄生轴窗口能接收到，就算ignore忽略掉这个事件，只是落到了当前寄生绘图窗口，
 * 而不是下一层级的寄生绘图的坐标轴窗口，但一般希望的事情是，顶层的寄生绘图的坐标轴窗口事件如果忽略，应该落到下一层寄生绘图的坐标轴窗口，
 * 这样就能处理多坐标轴时坐标轴的动作。
 *
 * 这个类就是为了解决上述问题设计的，轴的动作都放到这里来执行，不用做事件的传递者，而做事件的执行者
 *
 */
class QWT_EXPORT QwtPlotScaleEventDispatcher : public QObject
{
    Q_OBJECT
    QWT_DECLARE_PRIVATE(QwtPlotScaleEventDispatcher)
public:
    explicit QwtPlotScaleEventDispatcher(QwtPlot* plot, QObject* par = nullptr);
    ~QwtPlotScaleEventDispatcher();
    bool isEnable() const;

public Q_SLOTS:
    void updateCache();
    // 设置可用
    void setEnable(bool on = true);

protected:
    virtual bool eventFilter(QObject* obj, QEvent* e) override;
    // 更新数据
    void rebuildCache();
    // 处理各种鼠标事件
    virtual bool handleMousePress(QwtPlot* plot, QMouseEvent* e);
    virtual bool handleMouseMove(QwtPlot* plot, QMouseEvent* e);
    virtual bool handleMouseRelease(QwtPlot* plot, QMouseEvent* e);
    virtual bool handleWheelEvent(QwtPlot* plot, QWheelEvent* e);
    // 查找应该处理事件的 scale widget
    QwtScaleWidget* findTargetOnScale(const QPoint& pos);

private:
    bool handleScaleMousePan(QwtScaleWidget* scaleWidget, QMouseEvent* e);
    bool handleScaleWheelZoom(QwtScaleWidget* scaleWidget, QWheelEvent* e);
};

#endif  // QWTPLOTSCALEEVENTDISPATCHER_H
