#include <QApplication>

#include "MainWindow.h"
#include "../../common/qwt_example_screenshot.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.resize(1400, 800);
    window.show();

    QwtExampleScreenshot::start( &window );

    return app.exec();
}
