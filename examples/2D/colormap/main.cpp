#include "MainWindow.h"

#include <QApplication>

#include <cstdio>
#include <cstdlib>

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
                return argument.mid(kScreenshotOption.size() + 1);

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

    /**
        @brief Prepare the platform plugin for the screenshot mode
        @details The exported images are rendered by the paint engine of the
                 platform plugin, what also decides about the fonts of the
                 text: the offscreen plugin has no font database and falls
                 back to a box for every character. Therefore the platform
                 of the window system is used whenever there is one, and the
                 offscreen plugin only when there is none ( headless CI ),
                 where it is the only way to write the images at all.

                 An explicitly set QT_QPA_PLATFORM always wins.
     */
    void preparePlatformForScreenshotMode()
    {
        if (!qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
            return;

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
        // A window system is always available on the desktop platforms
        return;
#else
        if (std::getenv("DISPLAY") == nullptr && std::getenv("WAYLAND_DISPLAY") == nullptr)
            qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
#endif
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

    // The platform plugin has to be selected before the QApplication
    if (!directory.isEmpty())
        preparePlatformForScreenshotMode();

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
