#ifndef QWT_FIGURE_PICKER_H
#define QWT_FIGURE_PICKER_H
#include <QObject>
#include <QWidget>
#include "qwt_picker.h"
class QMouseEvent;
class QwtFigure;
class QwtPlot;
/**
 * @brief 用于QwtFigure的多绘图区域选择器，支持联动选择、跟踪和橡皮筋功能
 *
 * @ref QwtFigurePicker 和  @ref QwtFigureWidgetOverlay 不同之处在于
 * - QwtFigurePicker是QwtFigure的事件捕获器，QwtFigureWidgetOverlay是一个蒙版窗口
 * - QwtFigureWidgetOverlay直接一个透明窗口，对鼠标不透明，所有鼠标事件都会经过QwtFigureWidgetOverlay处理，而QwtFigurePicker则没有此问题
 */
class QWT_EXPORT QwtFigurePicker : public QwtPicker
{
    Q_OBJECT
    QWT_DECLARE_PRIVATE(QwtFigurePicker)
public:
    explicit QwtFigurePicker(QwtFigure* fig);
    ~QwtFigurePicker();

    // 获取当前鼠标所在的绘图,不在任何绘图返回nullptr
    QwtPlot* currentPlot() const;
    // 确认当前是否在绘图的canvas上面，此函数返回true，currentPlot必定返回指针
    bool isOnCanvas() const;

    // 获取所有绘图的画布区域
    const QList< QPair< QwtPlot*, QRect > >& canvasRects() const;
    QRect canvasRect(QwtPlot* plot) const;

    // 重绘
    void replot();

protected:
    virtual bool eventFilter(QObject* object, QEvent* event) override;
    //
    virtual void widgetMousePressEvent(QMouseEvent* event) override;
    virtual void widgetMouseReleaseEvent(QMouseEvent* event) override;
    virtual void widgetMouseMoveEvent(QMouseEvent* event) override;
private Q_SLOTS:
    void onAxesAdded(QwtPlot* plot);
    void onAxesRemoved(QwtPlot* plot);

protected:
    /**
     * @brief 初始化
     */
    void init();

    /**
     * @brief 更新画布区域缓存
     */
    void updateCanvasRects();

private:
    // 更新当前绘图
    void updateCurrentPlot(const QPoint& pos);
};

#endif  // QWT_FIGURE_PICKER_H
