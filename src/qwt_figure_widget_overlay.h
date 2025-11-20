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
 * @brief The QwtFigureWidgetOverlay是针对QwtFigure的操作蒙版
 *
 * 此类提供了figure窗口子对象调整大小的功能，以及改变当前绘图的功能，你可以通过继承此类实现耕地的操作
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

    /**
     * @brief 内置的功能
     */
    enum BuiltInFunctionsFlag
    {
        FunSelectCurrentPlot = 1,  ///< 此功能开启，可以改变选中的当前绘图
        FunResizePlot        = 2   ///< 此功能开启，可以改变绘图的尺寸
    };
    Q_ENUM(BuiltInFunctionsFlag)
    Q_DECLARE_FLAGS(BuiltInFunctions, BuiltInFunctionsFlag)
    Q_FLAG(BuiltInFunctions)

public:
    // 构造函数不允许传入nullptr
    explicit QwtFigureWidgetOverlay(QwtFigure* fig);
    ~QwtFigureWidgetOverlay();
    QwtFigure* figure() const;
    void setTransparentForMouseEvents(bool on);

public:
    // 根据点和矩形的关系，返回图标的样式
    static Qt::CursorShape controlTypeToCursor(ControlType rr);
    static ControlType getPositionControlType(const QPoint& pos, const QRect& region, int err = 1);
    static bool isPointInRectEdget(const QPoint& pos, const QRect& region, int err = 1);
    // 设置内置功能的开关
    void setBuiltInFunctionsEnable(BuiltInFunctionsFlag flag, bool on = true);
    bool testBuiltInFunctions(BuiltInFunctionsFlag flag) const;
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
    // virtual bool eventFilter(QObject* obj, QEvent* event) override;
    //  绘制激活的窗口
    virtual void drawActiveWidget(QPainter* painter, QWidget* activeW) const;
    // 绘制resize变换的橡皮筋控制线
    virtual void drawResizeingControlLine(QPainter* painter, const QRectF& willSetNormRect) const;
    // 绘制控制线
    virtual void drawControlLine(QPainter* painter, const QRect& actualRect, const QRectF& normRect) const;
    // 辅助函数，标记开始改变尺寸
    void startResize(ControlType controlType, const QPoint& pos);

protected:
    void mouseMoveEvent(QMouseEvent* me) override;
    void mouseReleaseEvent(QMouseEvent* me) override;
    void mousePressEvent(QMouseEvent* me) override;
    void keyPressEvent(QKeyEvent* ke) override;
Q_SIGNALS:

    /**
     * @brief 绘图尺寸发生改变信号
     * @param w 窗口
     * @param oldGeometry 旧的位置
     * @param newGeometry 新的位置
     */
    void widgetNormGeometryChanged(QWidget* w, const QRectF& oldNormGeo, const QRectF& newNormGeo);
    /**
     * @brief 激活窗口发生变化的信号
     * @param oldActive 如果之前没有激活窗口，此指针有可能是nullptr
     * @param newActive 如果没有新的激活窗口，此指针有可能是nullptr
     */
    void activeWidgetChanged(QWidget* oldActive, QWidget* newActive);

private:
};

#endif  // QWTFIGUREWIDGETOVERLAY_H
