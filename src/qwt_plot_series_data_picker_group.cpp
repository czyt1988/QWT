#include "qwt_plot_series_data_picker_group.h"
#include "qwt_plot_series_data_picker.h"
#include <QWidget>

class QwtPlotSeriesDataPickerGroup::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotSeriesDataPickerGroup)
public:
    PrivateData(QwtPlotSeriesDataPickerGroup* p);
    QList< QwtPlotSeriesDataPicker* > pickers;
};

QwtPlotSeriesDataPickerGroup::PrivateData::PrivateData(QwtPlotSeriesDataPickerGroup* p) : q_ptr(p)
{
}

//----------------------------------------------------
// QwtPlotSeriesDataPickerGroup
//----------------------------------------------------
QwtPlotSeriesDataPickerGroup::QwtPlotSeriesDataPickerGroup(QObject* par) : QObject(par)
{
}

/**
 * \if ENGLISH
 * @brief Adds a picker to the synchronization group
 * @param pick Pointer to QwtPlotSeriesDataPicker instance to be added to the group
 * @note The picker must belong to a QwtPlot with properly configured axes. Adding a null
 *       pointer or a picker already in the group has no effect. The group does not take
 *       ownership of the picker.
 * @warning Pickers from plots with significantly different X-axis scales may show positioning
 *          artifacts due to proportional mapping limitations.
 * \endif
 *
 * \if CHINESE
 * @brief 将picker添加到同步组中
 * @param pick 指向要加入组的QwtPlotSeriesDataPicker实例的指针
 * @note picker必须属于具有正确配置坐标轴的QwtPlot。添加空指针或已在组中的picker将无效。
 *       该组不接管picker的所有权。
 * @warning 来自X轴比例差异显著的绘图的picker，可能因比例映射限制而出现定位异常。
 * \endif
 */
void QwtPlotSeriesDataPickerGroup::addPicker(QwtPlotSeriesDataPicker* pick)
{
    m_data->pickers.append(pick);
    connect(pick, &QwtPlotSeriesDataPicker::moved, this, &QwtPlotSeriesDataPickerGroup::onPickerMove);
    connect(pick, &QwtPlotSeriesDataPicker::activated, this, &QwtPlotSeriesDataPickerGroup::onPickerActivated);
    connect(pick, &QwtPlotSeriesDataPicker::destroyed, this, &QwtPlotSeriesDataPickerGroup::onPickerDestroy);
}

/**
 * \if ENGLISH
 * @brief Returns the list of all pickers currently in the group
 * @return QList containing pointers to all managed QwtPlotSeriesDataPicker instances
 * @note The returned list is a copy; modifications to it do not affect the internal group state.
 *       Use addPicker() to modify group membership.
 * \endif
 *
 * \if CHINESE
 * @brief 返回组内当前所有picker的列表
 * @return 包含所有受管理QwtPlotSeriesDataPicker实例指针的QList
 * @note 返回的列表是副本；修改该列表不会影响组的内部状态。
 *       使用addPicker()修改组成员关系。
 * \endif
 */
QList< QwtPlotSeriesDataPicker* > QwtPlotSeriesDataPickerGroup::pickers() const
{
    return m_data->pickers;
}

void QwtPlotSeriesDataPickerGroup::onPickerMove(const QPoint& pos)
{
    QwtPlotSeriesDataPicker* pick = qobject_cast< QwtPlotSeriesDataPicker* >(sender());
    if (!pick) {
        return;
    }
    // 转换为当前这个picker所在widget的百分比
    QWidget* canvas = pick->canvas();
    if (!canvas) {
        return;
    }
    double xPresent = qBound(0.0, (double)pos.x() / canvas->width(), 1.0);
    double yPresent = qBound(0.0, (double)pos.y() / canvas->height(), 1.0);
    // 让其它picker也设置这个位置的移动
    for (QwtPlotSeriesDataPicker* p : qwt_as_const(m_data->pickers)) {
        if (p == pick) {
            continue;
        }
        QWidget* c = p->canvas();
        if (!c) {
            continue;
        }
        double x = c->width() * xPresent;
        double y = c->height() * yPresent;
        p->setTrackerPosition(QPoint(x, y));
        p->update();
    }
}

void QwtPlotSeriesDataPickerGroup::onPickerActivated(bool on)
{
    QwtPlotSeriesDataPicker* pick = qobject_cast< QwtPlotSeriesDataPicker* >(sender());
    if (!pick) {
        return;
    }
    for (QwtPlotSeriesDataPicker* p : qwt_as_const(m_data->pickers)) {
        if (p == pick) {
            continue;
        }
        p->setActive(on);
    }
}

void QwtPlotSeriesDataPickerGroup::onPickerDestroy(QObject* obj)
{
    QwtPlotSeriesDataPicker* pick = (QwtPlotSeriesDataPicker*)obj;
    m_data->pickers.removeAll(pick);
}
