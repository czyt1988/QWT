#include <QApplication>
#include "MainWindow.h"

/**
 * @brief 创建一个寄生轴
 * @param figure
 */
void createGrid32_parasitePlot(QwtFigure* figure);

// 创建示例应用程序
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 创建主窗口
    MainWindow mainWindow;
    mainWindow.setWindowTitle("QwtFigure Layout Example with Log Scales");
    mainWindow.resize(1200, 800);
    mainWindow.show();
    return app.exec();
}
