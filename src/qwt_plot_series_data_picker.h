#ifndef QWT_PLOT_SERIES_DATA_PICKER_H
#define QWT_PLOT_SERIES_DATA_PICKER_H
#include <QList>
#include <QPointF>
#include "qwt_plot_picker.h"
#include "qwt_text.h"
class QwtPlot;
class QwtPlotItem;
/**
 * @brief 这是一个绘图数据拾取显示类，用于显示当前的y值，或者显示最近点
 */
class QWT_EXPORT QwtPlotSeriesDataPicker : public QwtPlotPicker
{
    Q_OBJECT
    QWT_DECLARE_PRIVATE(QwtPlotSeriesDataPicker)
public:
    /**
     * @brief 拾取模式
     */
    enum PickSeriesMode
    {
        PickYValue,  ///< 拾取y值（默认）
        PickNearestPoint  ///< 拾取最接近鼠标光标位置的店（此模式会比较耗时，曲线点非常多的时候谨慎使用）
    };

    /**
     * @brief The TextArea enum
     */
    enum TextPlacement
    {
        TextOnTop,       ///< 放在绘图区的顶部(默认）
        TextOnBottom,    ///< 放在绘图区的底部
        TextFollowMouse  ///< 跟随鼠标指针
    };

    /**
     * @brief 插值模式枚举
     */
    enum InterpolationMode
    {
        NoInterpolation,     ///< 不进行插值，使用最近的数据点
        LinearInterpolation  ///< 线性插值，在相邻数据点之间进行插值计算
    };

public:
    explicit QwtPlotSeriesDataPicker(QWidget* canvas);
    ~QwtPlotSeriesDataPicker();

    // 拾取模式
    void setPickMode(PickSeriesMode mode);
    PickSeriesMode pickMode() const;

    // 设置文字显示的位置
    void setTextArea(TextPlacement t);
    TextPlacement textArea() const;

    // 插值模式
    void setInterpolationMode(InterpolationMode mode);
    InterpolationMode interpolationMode() const;

    // 顶部矩形文字
    QwtText trackerText(const QPoint& pos) const override;

    // 让矩形在最顶部
    QRect trackerRect(const QFont& f) const override;

    // 获取绘图区域屏幕坐标pos上，的所有可拾取的y值
    static QList< QPair< QwtPlotItem*, QPointF > > pickYValue(const QwtPlot* plot, const QPoint& pos, bool interpolate = false);
    // 获取绘图区域屏幕坐标pos上，可拾取的最近的一个点，(基于窗口实现快速索引)
    static QPair< QwtPlotItem*, QPointF > pickNearestPoint(const QwtPlot* plot, const QPoint& pos, int windowSize = -1);

protected:
    // 生成一个item的文字内容
    virtual QString valueString(const QPointF& value, QwtPlotItem* item, int index) const;
};

#endif  // QWT_PLOT_SERIES_DATA_PICKER_H
