#include <QApplication>
#include <QSurfaceFormat>
#include <QTimer>

#include "MainWindow.h"
#include "../../common/qwt_example_screenshot.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Match the other 3D examples: 24 bit depth, 8 bit stencil, 4x MSAA
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    MainWindow w;
    w.show();

    QwtExampleScreenshot::start( &w );

    // Unattended modes for documentation builds:
    //   waterfall3D --screenshot file.png [--screenshot-delay ms] [--size WxH]
    //   waterfall3D --selftest [dir]
    // The screenshot protocol is shared with the other examples, see
    // examples/common/qwt_example_screenshot.h.
    const QStringList args = app.arguments();
    const int selftestIdx = args.indexOf("--selftest");
    if (selftestIdx >= 0) {
        const QString dir = (selftestIdx + 1 < args.size())
            ? args.at(selftestIdx + 1) : QString();
        QTimer::singleShot(0, &w, [&w, dir]() { w.runSelfTest(dir); });
    }
    // Window size / style overrides for benchmarks: --capacity N, --style N
    const int capIdx = args.indexOf("--capacity");
    if (capIdx >= 0 && capIdx + 1 < args.size())
        w.setTraceCapacity(args.at(capIdx + 1).toInt());
    const int styleIdx = args.indexOf("--style");
    if (styleIdx >= 0 && styleIdx + 1 < args.size())
        w.setStyleIndex(args.at(styleIdx + 1).toInt());

    // Unattended fps benchmark: after the window has filled and the fps
    // meter settled, print one line "FPS: <value>" and exit.
    const int fpsIdx = args.indexOf("--fps");
    if (fpsIdx >= 0) {
        QTimer::singleShot(12000, &w, [&w]() {
            qInfo("FPS: %s", w.statusLine().toLatin1().constData());
            QApplication::quit();
        });
    }

    return app.exec();
}
