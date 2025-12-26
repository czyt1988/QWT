#include "PickLinker.h"
#include <QWidget>
#include <QDebug>
PickLinker::PickLinker(QObject* par) : QObject(par)
{
}

PickLinker::~PickLinker()
{
}

void PickLinker::addPicker(QwtPlotSeriesDataPicker* pick)
{
    m_pickers.append(pick);
    connect(pick, &QwtPlotSeriesDataPicker::moved, this, &PickLinker::onPickerMove);
    connect(pick, &QwtPlotSeriesDataPicker::activated, this, &PickLinker::onPickerActivated);
    connect(pick, &QwtPlotSeriesDataPicker::destroyed, this, &PickLinker::onPickerDestroy);
}

void PickLinker::onPickerMove(const QPoint& pos)
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
    double xPresent = (double)pos.x() / canvas->width();
    // 让其它picker也设置这个位置的移动
    for (QwtPlotSeriesDataPicker* p : qAsConst(m_pickers)) {
        if (p == pick) {
            continue;
        }
        QWidget* c = p->canvas();
        if (!c) {
            continue;
        }
        double x = c->width() * xPresent;
        p->setTrackerPosition(QPoint(x, 0));
        // qDebug() << "set picker" << p << ",setTrackerPosition=" << QPoint(x, 0);
        p->update();
    }
}

void PickLinker::onPickerActivated(bool on)
{
    QwtPlotSeriesDataPicker* pick = qobject_cast< QwtPlotSeriesDataPicker* >(sender());
    if (!pick) {
        return;
    }
    for (QwtPlotSeriesDataPicker* p : qAsConst(m_pickers)) {
        if (p == pick) {
            continue;
        }
        p->setActive(on);
    }
}

void PickLinker::onPickerDestroy(QObject* obj)
{
    QwtPlotSeriesDataPicker* pick = (QwtPlotSeriesDataPicker*)obj;
    m_pickers.removeAll(pick);
}
