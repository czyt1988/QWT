/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef QWT_PLOT_SERIES_DATA_PICKER_H
#define QWT_PLOT_SERIES_DATA_PICKER_H
#include <QList>
#include <QPointF>
#include "qwt_canvas_picker.h"
#include "qwt_text.h"
class QwtPlot;
class QwtPlotItem;
/**
 * @brief 这是一个绘图数据拾取显示类，用于显示当前的y值，或者显示最近点
 */
class QWT_EXPORT QwtPlotSeriesDataPicker : public QwtCanvasPicker
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
        PickNearestPoint  ///< 拾取最接近鼠标光标位置的点（此模式会比较耗时，曲线点非常多的时候谨慎使用）
    };

    /**
     * @brief The TextArea enum
     */
    enum TextPlacement
    {
        TextPlaceAuto,         ///< 自动放置（pick y的时候放置在顶部，pick nearest的时候跟随鼠标）
        TextFollowOnTop,       ///< 放在绘图区的顶部(默认）
        TextFollowOnBottom,    ///< 放在绘图区的底部
        TextFollowMouse,       ///< 跟随鼠标指针
        TextOnCanvasTopRight,  ///< 文字在画布的右上角
        TextOnCanvasTopLeft,   ///< 文字在画布的左上角
        TextOnCanvasBottomRight,  ///< 文字在画布的右下角
        TextOnCanvasBottomLeft,   ///< 文字在画布的左下角
        TextOnCanvasTopAuto,  ///< 文字在画布的上边，具体是左是右根据鼠标位置来自动识别，尽量避免不影响鼠标位置
        TextOnCanvasBottomAuto  ///< 文字在画布的下边，具体是左是右根据鼠标位置来自动识别，尽量避免不影响鼠标位置
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
    // 判断是否插值
    bool isInterpolation() const;

    // 临近点搜索窗口大小，窗口大小决定了临近点搜索的范围，避免全曲线遍历
    void setNearestSearchWindowSize(int windowSize);
    int nearestSearchWindowSize() const;

    // 是否绘制特征点,如果是，picker会把捕获的特征点绘制在曲线上
    void setEnableDrawFeaturePoint(bool on = true);
    bool isEnableDrawFeaturePoint() const;

    // 设置绘制的特征点的大小
    void setDrawFeaturePointSize(int px);
    int drawFeaturePointSize() const;

    // 设置文字的背景颜色
    void setTextBackgroundBrush(const QBrush& br);
    QBrush textBackgroundBrush() const;

    // 文字的对其方式
    void setTextAlignment(Qt::Alignment al);
    Qt::Alignment textAlignment() const;

    // 顶部矩形文字
    QwtText trackerText(const QPoint& pos) const QWT_OVERRIDE;

    // 让矩形在最顶部
    QRect trackerRect(const QFont& f) const QWT_OVERRIDE;

    // 绘制rubberband
    virtual void drawRubberBand(QPainter* painter) const QWT_OVERRIDE;

private:
    // 获取绘图区域屏幕坐标pos上，的所有可拾取的y值,返回获取的个数
    int pickYValue(const QwtPlot* plot, const QPoint& pos, bool interpolate = false);
    // 获取绘图区域屏幕坐标pos上，可拾取的最近的一个点，(基于窗口实现快速索引)
    int pickNearestPoint(const QwtPlot* plot, const QPoint& pos, int windowSize = -5);
private Q_SLOTS:
    // item删除的槽，用于更新记录
    void onPlotItemDetached(QwtPlotItem* item, bool on);

protected:
    // 生成一个item的文字内容
    virtual QString valueString(const QPointF& value, QwtPlotItem* item, size_t seriesIndex, int order) const;
    // 绘制特征点，所谓特征点就是捕获到的点
    virtual void drawFeaturePoints(QPainter* painter) const;
    // 鼠标移动
    virtual void move(const QPoint& pos) QWT_OVERRIDE;
    // 格式化为坐标轴对应的内容，针对时间轴，value是一个大浮点数，用户需要看到的是2024-10-01这样的数字
    QString formatAxisValue(double value, int axisId, QwtPlot* plot) const;
};

#endif  // QWT_PLOT_SERIES_DATA_PICKER_H
