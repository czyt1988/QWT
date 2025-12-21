#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
class QwtFigure;
class QwtPlot;
class QwtFigureWidgetOverlay;
class PickLinker;

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
    //
    void initFigure(QwtFigure* figure);
    // 生成示例数据（线性坐标）
    QVector< QPointF > generateSampleData(int count = 100, double amplitude = 1.0, double frequency = 1.0);
    // 生成适合对数X轴的数据
    QVector< QPointF > generateLogXSampleData(int count = 100, double amplitude = 1.0);
    // 生成适合对数Y轴的数据（确保所有y值为正）
    QVector< QPointF > generateLogYSampleData(int count = 100, double baseValue = 1.0);
    // 生成指数增长数据（适合对数Y轴）
    QVector< QPointF > generateExponentialData(int count = 100, double base = 1.0);
    // 生成带nan值和inf值的示例数据
    QVector< QPointF > generateSampleDataWithNan();

private:
    // 创建工具栏
    void createToolBar();
    // 设置绘图样式
    void setupPlotStyle(QwtPlot* plot, const QString& title, const QColor& color);
    // 创建寄生绘图
    QwtPlot* createParasitePlot();

private:
    Ui::MainWindow* ui;
    QwtFigure* m_figure { nullptr };
    QwtFigureWidgetOverlay* m_figureOverlay { nullptr };
    PickLinker* m_pickerLinker { nullptr };
};

#endif  // MAINWINDOW_H
