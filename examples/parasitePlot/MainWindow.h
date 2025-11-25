#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qwt_plot.h"
class QLabel;
class QwtPlotSeriesDataPicker;
class QwtPlotPanner;
class QwtPlotCanvasZoomer;
class QwtPlotMagnifier;
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
    void createToolBar();

private:
    Ui::MainWindow* ui;
    QwtPlot* m_plot { nullptr };
    QwtPlotSeriesDataPicker* m_dataPicker { nullptr };
    QwtPlotPanner* m_panner { nullptr };
    QwtPlotCanvasZoomer* m_zoomer { nullptr };
    QwtPlotMagnifier* m_magnifier { nullptr };
    QLabel* mStatusBarLabel { nullptr };
};

#endif  // MAINWINDOW_H
