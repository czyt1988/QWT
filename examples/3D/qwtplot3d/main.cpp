#include <QApplication>
#include <QSurfaceFormat>

#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Set high-DPI attributes for sharp rendering
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4); // 4x MSAA for smooth edges
    QSurfaceFormat::setDefaultFormat(format);

    MainWindow w;
    w.show();

    return app.exec();
}
