#include "qwt_plot_series_data_picker.h"
#include "qwt_picker_machine.h"
class QwtPlotSeriesDataPicker::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotSeriesDataPicker)
public:
    PrivateData(QwtPlotSeriesDataPicker* p);

public:
    QwtPlotSeriesDataPicker::PickSeriesMode pickMode { QwtPlotSeriesDataPicker::PickYValue };
};

QwtPlotSeriesDataPicker::PrivateData::PrivateData(QwtPlotSeriesDataPicker* p) : q_ptr(p)
{
}

//===============================================================
// QwtPlotSeriesDataPicker
//===============================================================

QwtPlotSeriesDataPicker::QwtPlotSeriesDataPicker(QWidget* canvas) : QwtPlotPicker(canvas), QWT_PIMPL_CONSTRUCT
{
    // 设置追踪模式，始终显示追踪信息
    setTrackerMode(QwtPlotPicker::ActiveOnly);
    // 设置橡皮筋为垂直线
    setRubberBand(QwtPlotPicker::VLineRubberBand);
    // 设置状态机，用于点选择
    setStateMachine(new QwtPickerTrackerMachine);
}

QwtPlotSeriesDataPicker::~QwtPlotSeriesDataPicker()
{
}

void QwtPlotSeriesDataPicker::setPickSeriesMode(PickSeriesMode mode)

{
    QWT_D(d);
    if (mode == d->pickMode) {
        return;
    }
    switch (mode) {
    case PickYValue:
        setRubberBand(QwtPlotPicker::VLineRubberBand);
        break;
    case PickClosePoint:
        setRubberBand(QwtPlotPicker::NoRubberBand);
        break;
    default:
        break;
    }
}

QwtPlotSeriesDataPicker::PickSeriesMode QwtPlotSeriesDataPicker::pickSeriesMode() const
{
    return m_data->pickMode;
}
