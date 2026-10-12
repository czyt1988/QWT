#ifndef QWT_EXAMPLE_SCREENSHOT_H
#define QWT_EXAMPLE_SCREENSHOT_H

/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

/*!
  @brief Unattended screenshot support shared by the Qwt examples

  Header-only helper implementing a common command line protocol so that
  every example can be captured by a script ( examples/update-screenshots.ps1 )
  without user interaction:

  @code
  example --screenshot <file.png> [--screenshot-delay <ms>] [--size <WxH>] [--variant <name>]
  @endcode

  - `--screenshot <file>`: grab the main window after a delay, save it as
    PNG and exit ( code 0 on success, 2 on failure ). The parent directory
    of <file> is created if necessary.
  - `--screenshot-delay <ms>`: wait time for the UI to settle before the
    grab. Defaults to 1500 ms, pass longer values for animated or
    OpenGL based examples.
  - `--size <WxH>`: resize the window before capturing, so that the
    resulting image has a reproducible size.
  - `--variant <name>`: select a visual variant of the example ( e.g. a
    tab, a plot style ). Examples register variants with addVariant().

  Without `--screenshot` all functions are no-ops and the example runs
  as usual.

  Usage in an example's main.cpp ( adjust the relative path to the
  examples/common directory, e.g. ../../common for 2D/3D examples ):

  @code
  #include "../../common/qwt_example_screenshot.h"

  int main( int argc, char* argv[] )
  {
      QApplication app( argc, argv );

      MainWindow window;
      QwtExampleScreenshot::addVariant( "stacked", [&window]() { window.setStacked(); } );
      window.show();

      QwtExampleScreenshot::start( &window );
      return app.exec();
  }
  @endcode
 */

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>

#include <functional>

class QwtExampleScreenshot
{
  public:
    //! Callback that switches the example into a named visual state
    typedef std::function<void ()> VariantSetup;

    /*!
      @brief Register a visual variant
      @param name Name as passed on the command line ( --variant <name> )
      @param setup Callback selecting the variant, called before the capture

      Registering a variant twice replaces the previous callback. Unknown
      variant names requested on the command line are reported with
      qWarning() and ignored.
     */
    static void addVariant( const QString& name, const VariantSetup& setup )
    {
        registry().insert( name, setup );
    }

    //! @return True, when --screenshot was passed on the command line
    static bool isActive()
    {
        return qApp && qApp->arguments().contains( QLatin1String( "--screenshot" ) );
    }

    /*!
      @brief Schedule the capture when running in screenshot mode
      @param window Window to grab, must stay alive until the capture happened

      Applies --size and --variant immediately, then grabs the window after
      --screenshot-delay milliseconds, saves the PNG given by --screenshot
      and quits the application. Does nothing when not in screenshot mode.
     */
    static void start( QWidget* window )
    {
        if ( window == nullptr || !isActive() )
            return;

        const QStringList args = qApp->arguments();

        // Apply the variant before resizing: a variant may change the minimum
        // size of the window ( e.g. hiding a dock ), and resize() would
        // otherwise be clamped by the still visible variant widgets. The
        // layout update triggered by the variant is asynchronous, so process
        // the pending events before resizing.
        const QString variant = value( args, QLatin1String( "--variant" ) );
        if ( !variant.isEmpty() )
        {
            const QMap<QString, VariantSetup>& variants = registry();
            if ( variants.contains( variant ) )
                variants.value( variant )();
            else
                qWarning( "Unknown screenshot variant: %s", qPrintable( variant ) );

            QApplication::processEvents();
        }

        const QString sizeString = value( args, QLatin1String( "--size" ) );
        if ( !sizeString.isEmpty() )
        {
            const QStringList wh = sizeString.split( QLatin1Char( 'x' ) );
            if ( wh.size() == 2 )
                window->resize( wh.at( 0 ).toInt(), wh.at( 1 ).toInt() );
        }

        const QString file = value( args, QLatin1String( "--screenshot" ) );

        int delay = 1500;
        const QString delayString = value( args, QLatin1String( "--screenshot-delay" ) );
        if ( !delayString.isEmpty() )
            delay = delayString.toInt();

        QTimer::singleShot( delay, window, [window, file]()
        {
            bool ok = false;

            const QPixmap pixmap = window->grab();
            if ( !pixmap.isNull() )
            {
                QDir dir = QFileInfo( file ).absoluteDir();
                if ( !dir.exists() )
                    dir.mkpath( QLatin1String( "." ) );

                ok = pixmap.save( file );
            }

            if ( !ok )
                qWarning( "Could not write screenshot: %s", qPrintable( file ) );

            qApp->exit( ok ? 0 : 2 );
        } );
    }

  private:
    static QMap<QString, VariantSetup>& registry()
    {
        static QMap<QString, VariantSetup> variants;
        return variants;
    }

    static QString value( const QStringList& args, const QString& option )
    {
        const int index = args.indexOf( option );
        if ( index >= 0 && index + 1 < args.size() )
            return args.at( index + 1 );

        return QString();
    }
};

#endif
