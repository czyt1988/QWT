#ifndef QWT_PLOT_SERIES_DATA_PICKER_GROUP_H
#define QWT_PLOT_SERIES_DATA_PICKER_GROUP_H
#include <QObject>
#include "qwt_global.h"
class QwtPlotSeriesDataPicker;
/**
 * \if ENGLISH
 * @brief Group manager for synchronizing multiple QwtPlotSeriesDataPicker instances across plot widgets
 *
 * This class enables coordinated interaction among multiple plot pickers, currently implementing
 * synchronized X-axis proportional positioning. When a user moves the mouse over one plot within
 * a subplot arrangement, the active picker displays its Y-value while all other pickers in the same
 * group automatically position themselves at equivalent X-axis proportional locations on their
 * respective plots, creating a unified cross-plot data inspection experience.
 *
 * Key features:
 * - Automatic synchronization of picker positions based on X-axis proportion
 * - Dynamic membership management (add/remove pickers at runtime)
 * - Automatic cleanup when pickers are destroyed
 * - Activation state synchronization across the group
 *
 * @note Currently supports only X-axis proportional synchronization; Y-axis values are displayed
 *       independently per plot based on local data at the synchronized X position.
 * \endif
 *
 * \if CHINESE
 * @brief 用于在多个绘图控件间同步QwtPlotSeriesDataPicker实例的组管理器
 *
 * 此类实现多个绘图picker的协同交互，当前仅支持X轴比例位置同步功能。当用户在子图布局中的
 * 某个绘图上移动鼠标时，活动的picker会显示当前Y值，而同组内其他所有picker会自动在各自
 * 绘图上定位到等效的X轴比例位置，从而创建统一的跨图数据检视体验。
 *
 * 核心特性：
 * - 基于X轴比例的picker位置自动同步
 * - 动态成员管理（运行时添加/移除picker）
 * - picker销毁时的自动清理机制
 * - 激活状态在组内同步
 *
 * @note 当前仅支持X轴比例同步；Y轴值根据同步X位置在各自绘图上独立显示本地数据。
 * \endif
 */
class QWT_EXPORT QwtPlotSeriesDataPickerGroup : public QObject
{
    Q_OBJECT
    QWT_DECLARE_PRIVATE(QwtPlotSeriesDataPickerGroup)
public:
    QwtPlotSeriesDataPickerGroup(QObject* par = nullptr);
    ~QwtPlotSeriesDataPickerGroup();
    // Add Plot Series Data Picker To Group
    void addPicker(QwtPlotSeriesDataPicker* pick);
    QList< QwtPlotSeriesDataPicker* > pickers() const;
public Q_SLOTS:
    void onPickerMove(const QPoint& pos);
    void onPickerActivated(bool on);
    void onPickerDestroy(QObject* obj);
};

#endif  // QWT_PLOT_SERIES_DATA_PICKER_GROUP_H
