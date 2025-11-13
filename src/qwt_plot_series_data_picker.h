#ifndef QWT_PLOT_SERIES_DATA_PICKER_H
#define QWT_PLOT_SERIES_DATA_PICKER_H
#include "qwt_plot_picker.h"
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
        PickClosePoint  ///< 拾取最接近鼠标光标位置的店（此模式会比较耗时，曲线点非常多的时候谨慎使用）
    };

public:
    explicit QwtPlotSeriesDataPicker(QWidget* canvas);
    ~QwtPlotSeriesDataPicker();

    // 拾取模式
    void setPickSeriesMode(PickSeriesMode mode);
    PickSeriesMode pickSeriesMode() const;
};

#endif  // QWT_PLOT_SERIES_DATA_PICKER_H
