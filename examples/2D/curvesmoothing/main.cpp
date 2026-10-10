#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.resize(1400, 800);
    window.show();

    return app.exec();
}
