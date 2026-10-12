#include <QApplication>
#include <QDockWidget>
#include <QSurfaceFormat>

#include "MainWindow.h"
#include "../../common/qwt_example_screenshot.h"

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

    // Screenshot variant without the settings dock: the plain 3D view,
    // matching the landscape layout of the historical simpleplot3D image
    QwtExampleScreenshot::addVariant( "plot", [&w]() {
        const QList<QDockWidget*> docks = w.findChildren<QDockWidget*>();
        for ( QDockWidget* dock : docks )
            dock->hide();
    } );

    QwtExampleScreenshot::start( &w );

    return app.exec();
}
