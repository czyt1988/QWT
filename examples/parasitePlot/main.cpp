/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include <QApplication>
#include "MainWindow.h"

// 创建示例应用程序
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 创建主窗口
    MainWindow w;
    w.setWindowTitle("Qwt Parasite Plot Example");
    w.resize(1200, 800);

    w.show();

    return app.exec();
}
