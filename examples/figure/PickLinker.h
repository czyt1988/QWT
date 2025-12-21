#ifndef PICKLINKER_H
#define PICKLINKER_H
#include "qwt_plot_series_data_picker.h"
#include <QPointer>
class PickLinker : public QObject
{
    Q_OBJECT
public:
    PickLinker(QObject* par);
    ~PickLinker();
    // 添加picker
    void addPicker(QwtPlotSeriesDataPicker* pick);
    //
public Q_SLOTS:
    void onPickerMove(const QPoint& pos);
    void onPickerActivated(bool on);
    void onPickerDestroy(QObject* obj);

private:
    QList< QwtPlotSeriesDataPicker* > m_pickers;
};

#endif  // PCIKLINKER_H
