#include "MainWindow.h"

#include <QApplication>

#include <cstdio>

#include <QByteArray>
#include <QString>

namespace
{
    const QString kScreenshotOption = QStringLiteral("--screenshot");

    // Usage: colormap [--screenshot <directory>]
    //
    // In screenshot mode one PNG is written per preset ( plus overview.png
    // and colormap.png ) and the application exits without showing a window.
    QString screenshotDirectory(int argc, char* argv[])
    {
        for (int i = 1; i < argc; i++)
        {
            const QString argument = QString::fromLocal8Bit(argv[ i ]);

            if (argument.startsWith(kScreenshotOption + QLatin1Char('=')))
                return argument.mid(kScreenshotOption.count() + 1);

            if (argument == kScreenshotOption && i + 1 < argc)
                return QString::fromLocal8Bit(argv[ i + 1 ]);
        }

        return QString();
    }

    void printUsage()
    {
        printf("Usage: colormap [--screenshot <directory>]\n");
        printf("  Without arguments all 22 colormap presets are shown in a window.\n");
        printf("  With --screenshot <directory> one PNG per preset is written to <directory>.\n");
    }
}

int main(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
    {
        const QString argument = QString::fromLocal8Bit(argv[ i ]);
        if (argument == QLatin1String("--help") || argument == QLatin1String("-h"))
        {
            printUsage();
            return 0;
        }
    }

    const QString directory = screenshotDirectory(argc, argv);

    if (!directory.isEmpty())
    {
        // No window is necessary for writing the images, but
        // QT_QPA_PLATFORM=offscreen needs to be set before the QApplication
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    }

    QApplication app(argc, argv);

    MainWindow window;
    window.resize(1500, 900);
    window.show();

    if (!directory.isEmpty())
    {
        // Screenshots are written without running the event loop, but
        // the layout of the window has to be activated first
        QApplication::processEvents();

        if (!window.exportScreenshots(directory))
            return 1;

        return 0;
    }

    return app.exec();
}
