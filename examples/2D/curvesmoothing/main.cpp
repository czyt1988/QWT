#include <QApplication>

#include "MainWindow.h"
#include "../../common/qwt_example_screenshot.h"

// The screenshot variants select the interesting windows of the example:
// with 100,000 samples the downsampling is active at the full range, while
// a zoomed window can be below the filter threshold, where all methods fall
// back to painting every sample ( --screenshot <file> --variant <name> )
static void setupScreenshotVariants(MainWindow& window)
{
    QwtExampleScreenshot::addVariant("zoom-hump", [ &window ]() { window.zoomToSampleWindow(30000.0, 30000.0); });
    QwtExampleScreenshot::addVariant("zoom-narrow-peak", [ &window ]() { window.zoomToSampleWindow(15000.0, 1200.0); });
    QwtExampleScreenshot::addVariant("zoom-dip", [ &window ]() { window.zoomToSampleWindow(45000.0, 600.0); });
    QwtExampleScreenshot::addVariant("no-downsampling", [ &window ]() {
        window.setDownsampleMethod(MainWindow::DownsampleNone);
    });
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.resize(1400, 800);
    setupScreenshotVariants(window);
    window.show();

    QwtExampleScreenshot::start( &window );

    return app.exec();
}
