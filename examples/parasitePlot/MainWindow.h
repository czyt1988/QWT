#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qwt_plot.h"

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
    QwtPlotSeriesDataPicker* m_dataPicker;
};

#endif  // MAINWINDOW_H
