#include <QApplication>
#include <QSurfaceFormat>
#include <QTimer>

#include "MainWindow.h"

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

    // Unattended screenshot mode for documentation builds:
    //   waterfall3D --screenshot file.png [--screenshot-delay ms]
    //   waterfall3D --selftest [dir]
    const QStringList args = app.arguments();
    const int selftestIdx = args.indexOf("--selftest");
    if (selftestIdx >= 0) {
        const QString dir = (selftestIdx + 1 < args.size())
            ? args.at(selftestIdx + 1) : QString();
        QTimer::singleShot(0, &w, [&w, dir]() { w.runSelfTest(dir); });
    }
    // Unattended fps benchmark: after the window has filled and the fps
    // meter settled, print one line "FPS: <value>" and exit.
    const int fpsIdx = args.indexOf("--fps");
    if (fpsIdx >= 0) {
        QTimer::singleShot(12000, &w, [&w]() {
            qInfo("FPS: %s", w.statusLine().toLatin1().constData());
            QApplication::quit();
        });
    }
    const int shotIdx = args.indexOf("--screenshot");
    if (shotIdx >= 0) {
        const QString file = (shotIdx + 1 < args.size())
            ? args.at(shotIdx + 1) : QStringLiteral("waterfall3D.png");
        const int delayIdx = args.indexOf("--screenshot-delay");
        const int delay = (delayIdx >= 0 && delayIdx + 1 < args.size())
            ? args.at(delayIdx + 1).toInt() : 6000;
        QTimer::singleShot(delay, &w, [&w, file]() {
            w.grab().save(file);
            QApplication::quit();
        });
    }

    return app.exec();
}
