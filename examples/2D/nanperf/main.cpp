#include <QApplication>
#include "MainWindow.h"
#include "../../common/qwt_example_screenshot.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    w.show();

    QwtExampleScreenshot::start( &w );
    return app.exec();
}
