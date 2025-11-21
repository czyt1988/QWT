#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qwt_plot.h"
#include "qwt_plot_panner.h"

class QwtPlotSeriesDataPicker;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    // 创建绘图
    QwtPlot* createPlot(QWidget* par);
    // 创建工具栏
    void createToolBar(QwtPlot* hostplot);

private:
    Ui::MainWindow* ui;
    QwtPlot* m_plot;
    QwtPlotSeriesDataPicker* m_dataPicker;
    QwtPlotPanner* m_panner;
};

#endif  // MAINWINDOW_H
