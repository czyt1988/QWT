#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
class QAction;
class QLabel;
class QwtFigure;
class QwtPlot;
class QwtFigureWidgetOverlay;
class QwtPlotSeriesDataPicker;
class QwtPlotSeriesDataPickerGroup;

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
    // 创建 2x2 保留网格 + 可拖拽分隔条演示
    void initGridDemoFigure(QwtFigure* figure);
    // 重建 figure（经典演示 / 网格演示之间切换）
    void rebuildFigure(bool gridDemo);
    // 设置绘图样式
    void setupPlotStyle(QwtPlot* plot, const QString& title, const QColor& color);
    // 创建寄生绘图
    QwtPlot* createParasitePlot();
private Q_SLOTS:
    void onActionResizeTriggered(bool on);
    void onActionGridDemoTriggered(bool on);
    void onPickerGroupClicked(QwtPlotSeriesDataPicker* picker, const QPoint& pos);

private:
    Ui::MainWindow* ui;
    QwtFigure* m_figure { nullptr };
    QwtFigureWidgetOverlay* m_figureOverlay { nullptr };
    QwtPlotSeriesDataPickerGroup* m_pickerLinker { nullptr };
    QLabel* m_clickInfoLabel { nullptr };
    QAction* m_actionResize { nullptr };
    QAction* m_actionGridDemo { nullptr };
    bool m_gridDemo { false };
};

#endif  // MAINWINDOW_H
