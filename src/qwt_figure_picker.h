#ifndef QWT_FIGURE_PICKER_H
#define QWT_FIGURE_PICKER_H
#include <QObject>
#include <QWidget>
#include <QPoint>
#include <QRect>
#include <QPen>
#include <QFont>
#include <QPainter>
#include <QString>
#include <QList>
#include <QMap>
#include <QMouseEvent>

#include "qwt_figure.h"
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
/**
 * @brief 用于QwtFigure的多绘图区域选择器，支持联动选择、跟踪和橡皮筋功能
 *
 * @ref QwtFigurePicker 和  @ref QwtFigureWidgetOverlay 不同之处在于
 * - QwtFigurePicker是QwtFigure的事件捕获器，QwtFigureWidgetOverlay是一个蒙版窗口
 * - QwtFigureWidgetOverlay直接一个透明窗口，对鼠标不透明，所有鼠标事件都会经过QwtFigureWidgetOverlay处理，而QwtFigurePicker则没有此问题
 */
class QWT_EXPORT QwtFigurePicker : public QObject
{
    Q_OBJECT
    QWT_DECLARE_PRIVATE(QwtFigurePicker)
public:
    /**
     * @brief 橡皮筋样式枚举
     */
    enum RubberBandStyle
    {
        NoRubberBand,       ///< 无橡皮筋
        VLineRubberBand,    ///< 垂直线橡皮筋
        HLineRubberBand,    ///< 水平线橡皮筋
        CrossRubberBand,    ///< 十字线橡皮筋
        RectRubberBand,     ///< 矩形橡皮筋
        EllipseRubberBand,  ///< 椭圆橡皮筋
        UserRubberBand      ///< 用户自定义橡皮筋
    };

    /**
     * @brief 跟踪模式枚举
     */
    enum TrackerMode
    {
        AlwaysOff,  ///< 总是关闭跟踪
        AlwaysOn,   ///< 总是打开跟踪
        ActiveOnly  ///< 仅在激活时跟踪
    };
    explicit QwtFigurePicker(QwtFigure* figure, QObject* par = nullptr);
    ~QwtFigurePicker();
    // 状态控制
    /**
     * @brief 设置选择器是否启用
     * @param on 启用状态
     */
    void setEnabled(bool on);

    /**
     * @brief 获取选择器是否启用
     * @return 启用状态
     */
    bool isEnabled() const;

    /**
     * @brief 设置选择器是否激活
     * @param on 激活状态
     */
    void setActive(bool on);

    /**
     * @brief 获取选择器是否激活
     * @return 激活状态
     */
    bool isActive() const;
    // 橡皮筋设置
    /**
     * @brief 设置橡皮筋样式
     * @param style 橡皮筋样式
     */
    void setRubberBand(RubberBandStyle style);

    /**
     * @brief 获取橡皮筋样式
     * @return 橡皮筋样式
     */
    RubberBandStyle rubberBand() const;

    /**
     * @brief 设置橡皮筋画笔
     * @param pen 画笔
     */
    void setRubberBandPen(const QPen& pen);

    /**
     * @brief 获取橡皮筋画笔
     * @return 画笔
     */
    QPen rubberBandPen() const;

    // 跟踪器设置
    /**
     * @brief 设置跟踪模式
     * @param mode 跟踪模式
     */
    void setTrackerMode(TrackerMode mode);

    /**
     * @brief 获取跟踪模式
     * @return 跟踪模式
     */
    TrackerMode trackerMode() const;

    /**
     * @brief 设置跟踪器字体
     * @param font 字体
     */
    void setTrackerFont(const QFont& font);

    /**
     * @brief 获取跟踪器字体
     * @return 字体
     */
    QFont trackerFont() const;

    /**
     * @brief 设置跟踪器画笔
     * @param pen 画笔
     */
    void setTrackerPen(const QPen& pen);

    /**
     * @brief 获取跟踪器画笔
     * @return 画笔
     */
    QPen trackerPen() const;

    /**
     * @brief 设置跟踪器位置相对于鼠标的偏移
     * @param offset 偏移量
     */
    void setTrackerPositionOffset(const QPoint& offset);

    /**
     * @brief 获取跟踪器位置偏移
     * @return 偏移量
     */
    QPoint trackerPositionOffset() const;

    /**
     * @brief 设置是否在所有绘图上显示参考线
     * @param show 是否显示
     */
    void setShowReferenceLines(bool show);

    /**
     * @brief 获取是否在所有绘图上显示参考线
     * @return 是否显示
     */
    bool showReferenceLines() const;

    /**
     * @brief 设置参考线画笔
     * @param pen 画笔
     */
    void setReferenceLinePen(const QPen& pen);

    /**
     * @brief 获取参考线画笔
     * @return 画笔
     */
    QPen referenceLinePen() const;

    /**
     * @brief 获取当前鼠标所在的绘图
     * @return 当前绘图指针，如果不在任何绘图上则返回nullptr
     */
    QwtPlot* currentPlot() const;

    /**
     * @brief 获取当前鼠标位置在绘图上的坐标值
     * @param plot 目标绘图
     * @return 坐标值对(x, y)，如果无法转换则返回(0,0)
     */
    QPointF plotValue(QwtPlot* plot) const;

    /**
     * @brief 获取所有绘图的画布区域
     * @return 绘图->画布区域的映射
     */
    QMap< QwtPlot*, QRect > canvasRects() const;

    /**
     * @brief 获取指定绘图的画布区域
     * @param plot 目标绘图
     * @return 画布区域
     */
    QRect canvasRect(QwtPlot* plot) const;

    /**
     * @brief 更新橡皮筋绘制
     * @param painter 绘制器
     * @param rect 绘制区域
     */
    virtual void paintRubberBand(QPainter* painter, const QRect& rect);

    /**
     * @brief 更新跟踪器文本
     * @param pos 鼠标位置
     * @return 跟踪器文本
     */
    virtual QString trackerText(const QPoint& pos) const;

    /**
     * @brief 获取跟踪器矩形
     * @param text 跟踪器文本
     * @param pos 鼠标位置
     * @return 跟踪器矩形
     */
    virtual QRect trackerRect(const QString& text, const QPoint& pos) const;

    /**
     * @brief 绘制跟踪器
     * @param painter 绘制器
     * @param rect 绘制区域
     * @param text 跟踪器文本
     */
    virtual void drawTracker(QPainter* painter, const QRect& rect, const QString& text) const;

    /**
     * @brief 开始选择
     */
    void begin();

    /**
     * @brief 结束选择
     */
    void end();

    /**
     * @brief 重绘所有选择元素
     */
    void replot();

Q_SIGNALS:
    /**
     * @brief 鼠标移动信号
     * @param pos 鼠标位置（相对于QwtFigure）
     * @param plot 鼠标所在的绘图
     * @param value 在绘图上的坐标值
     */
    void moved(const QPoint& pos, QwtPlot* plot, const QPointF& value);

    /**
     * @brief 选择完成信号
     * @param rect 选择区域（相对于QwtFigure）
     * @param plots 涉及到的绘图列表
     */
    void selected(const QRect& rect, const QList< QwtPlot* >& plots);

    /**
     * @brief 选择激活状态改变信号
     * @param active 激活状态
     */
    void activated(bool active);

    /**
     * @brief 选择器启用状态改变信号
     * @param enabled 启用状态
     */
    void enabled(bool enabled);

protected:
    /**
     * @brief 事件过滤器
     * @param watched 监视对象
     * @param event 事件
     * @return 是否处理事件
     */
    virtual bool eventFilter(QObject* watched, QEvent* event) override;

    /**
     * @brief 处理鼠标移动事件
     * @param event 鼠标事件
     * @param plot 事件发生的绘图
     */
    virtual void mouseMoveEvent(QMouseEvent* event, QwtPlot* plot);

    /**
     * @brief 处理鼠标按下事件
     * @param event 鼠标事件
     * @param plot 事件发生的绘图
     */
    virtual void mousePressEvent(QMouseEvent* event, QwtPlot* plot);

    /**
     * @brief 处理鼠标释放事件
     * @param event 鼠标事件
     * @param plot 事件发生的绘图
     */
    virtual void mouseReleaseEvent(QMouseEvent* event, QwtPlot* plot);

    /**
     * @brief 处理鼠标进入事件
     * @param event 鼠标事件
     * @param plot 事件发生的绘图
     */
    virtual void enterEvent(QEvent* event, QwtPlot* plot);

    /**
     * @brief 处理鼠标离开事件
     * @param event 鼠标事件
     * @param plot 事件发生的绘图
     */
    virtual void leaveEvent(QEvent* event, QwtPlot* plot);

    /**
     * @brief 绘制参考线
     * @param painter 绘制器
     * @param pos 鼠标位置
     * @param plot 当前绘图
     */
    virtual void paintReferenceLines(QPainter* painter, const QPoint& pos, QwtPlot* plot);

    /**
     * @brief 更新所有绘图的参考线
     * @param x 全局X坐标值
     * @param y 全局Y坐标值
     */
    void updateReferenceLines(double x, double y);

    /**
     * @brief 清除所有参考线
     */
    void clearReferenceLines();

private:
    /**
     * @brief 初始化
     */
    void init();

    /**
     * @brief 安装事件过滤器到所有绘图
     */
    void installFilters();

    /**
     * @brief 移除所有事件过滤器
     */
    void removeFilters();

    /**
     * @brief 更新画布区域缓存
     */
    void updateCanvasRects();

    /**
     * @brief 获取绘图在鼠标位置处的坐标值
     * @param plot 目标绘图
     * @param pos 鼠标位置（相对于绘图）
     * @return 坐标值
     */
    QPointF toPlotValue(QwtPlot* plot, const QPoint& pos) const;

    /**
     * @brief 将坐标值转换为绘图坐标
     * @param plot 目标绘图
     * @param value 坐标值
     * @return 绘图坐标
     */
    QPoint toPlotPosition(QwtPlot* plot, const QPointF& value) const;

    /**
     * @brief 获取全局画布坐标
     * @param plot 目标绘图
     * @param plotPos 绘图坐标
     * @return 全局坐标
     */
    QPoint toGlobalPosition(QwtPlot* plot, const QPoint& plotPos) const;
};

#endif  // QWT_FIGURE_PICKER_H
