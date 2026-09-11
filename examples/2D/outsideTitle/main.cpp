#include "MainWindow.h"
#include <QApplication>
#include <QDir>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    MainWindow win;

    // Headless verification mode: write grabs and renderer exports into a
    // directory, then quit. Usage: outsideTitle --export <dir>
    const QStringList args    = app.arguments();
    const int exportIdx       = args.indexOf(QStringLiteral("--export"));
    if (exportIdx >= 0) {
        const QString dir = (exportIdx + 1 < args.size()) ? args.at(exportIdx + 1) : QStringLiteral(".");
        QDir().mkpath(dir);
        win.show();
        return win.runSelfTest(dir) ? 0 : 1;
    }

    win.show();
    return app.exec();
}
