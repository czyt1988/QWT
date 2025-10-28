#ifndef QWTFIGUREWIDGETOVERLAY_H
#define QWTFIGUREWIDGETOVERLAY_H
#include "qwt_widget_overlay.h"
// Qt
class QEvent;
class QMouseEvent;
class QHoverEvent;
class QKeyEvent;
// Qwt
class QwtFigure;
class QwtPlot;

/**
 * @brief The QwtFigureWidgetOverlay class
 *
 * @note QwtFigureWidgetOverlay并不会直接改变尺寸，因此尺寸的改变主要在管理窗口中执行，这是为了能让它有更大的自由度，例如需要做回退功能
 */
class QWT_EXPORT QwtFigureWidgetOverlay : public QwtWidgetOverlay
{
    Q_OBJECT
    QWT_DECLARE_PRIVATE(QwtFigureWidgetOverlay)
public:
    /**
     * @brief 用于标记矩形的区域
     */
    enum ControlType
    {
        ControlLineTop,
        ControlLineBottom,
        ControlLineLeft,
        ControlLineRight,
        ControlPointTopLeft,
        ControlPointTopRight,
        ControlPointBottomLeft,
        ControlPointBottomRight,
        Inner,
        OutSide
    };
    Q_ENUM(ControlType)
public:
    // 构造函数不允许传入nullptr
    explicit QwtFigureWidgetOverlay(QwtFigure* fig);
    ~QwtFigureWidgetOverlay();
    QwtFigure* figure() const;

public:
    // 根据点和矩形的关系，返回图标的样式
    static Qt::CursorShape controlTypeToCursor(ControlType rr);
    static ControlType getPositionControlType(const QPoint& pos, const QRect& region, int err = 1);
    static bool isPointInRectEdget(const QPoint& pos, const QRect& region, int err = 1);
    // 判断当前是否有激活的窗口
    bool isHaveActiveWidget() const;
    // 设置边框的画笔
    void setBorderPen(const QPen& p);
    QPen borderPen() const;
    // 控制点的填充
    void setControlPointBrush(const QBrush& b);
    QBrush controlPointBrush() const;
    // 控制点尺寸
    void setControlPointSize(const QSize& c);
    QSize controlPointSize() const;
    // 选择下一个窗口作为激活窗体
    void selectNextWidget(bool forward = true);
    // 选择下一个绘图作为激活窗体
    void selectNextPlot(bool forward = true);
    // 获取当前激活的窗口
    QWidget* currentActiveWidget() const;
    QwtPlot* currentActivePlot() const;
    // 显示占比数值
    void showPercentText(bool on = true);
public Q_SLOTS:
    // 改变激活窗口
    void setActiveWidget(QWidget* w);

protected:
    virtual void drawOverlay(QPainter* p) const override;
    virtual QRegion maskHint() const override;
    virtual bool eventFilter(QObject* obj, QEvent* event) override;
    // 绘制
    virtual void drawActiveWidget(QPainter* painter, QWidget* activeW) const;
    //
    virtual void drawResizeingControlLine(QPainter* painter, const QRectF& willSetNormRect) const;

    //
    void drawControlLine(QPainter* painter, const QRect& actualRect, const QRectF& normRect) const;
Q_SIGNALS:

    /**
     * @brief 绘图尺寸发生改变信号
     * @param w 窗口
     * @param oldGeometry 旧的位置
     * @param newGeometry 新的位置
     */
    void widgetGeometryChanged(QWidget* w, const QRect& oldGeometry, const QRect& newGeometry);
    /**
     * @brief 激活窗口发生变化的信号
     * @param oldActive 如果之前没有激活窗口，此指针有可能是nullptr
     * @param newActive 如果没有新的激活窗口，此指针有可能是nullptr
     */
    void activeWidgetChanged(QWidget* oldActive, QWidget* newActive);

private:
    bool onMouseMoveEvent(QMouseEvent* me);
    bool onMouseReleaseEvent(QMouseEvent* me);
    bool onMousePressedEvent(QMouseEvent* me);
    bool onHoverMoveEvent(QHoverEvent* me);
    bool onKeyPressedEvent(QKeyEvent* ke);
};

#endif  // QWTFIGUREWIDGETOVERLAY_H
