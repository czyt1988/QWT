/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include "CompassGrid.h"
#include "CockpitGrid.h"

#include <QApplication>
#include <QTabWidget>
#include "../../common/qwt_example_screenshot.h"

int main( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    QTabWidget tabWidget;
    tabWidget.addTab( new CompassGrid, "Compass" );
    tabWidget.addTab( new CockpitGrid, "Cockpit" );

    // Visual variants for unattended screenshot generation
    QwtExampleScreenshot::addVariant( "compass", [&tabWidget]() { tabWidget.setCurrentIndex( 0 ); } );
    QwtExampleScreenshot::addVariant( "cockpit", [&tabWidget]() { tabWidget.setCurrentIndex( 1 ); } );

    tabWidget.show();

    QwtExampleScreenshot::start( &tabWidget );

    return app.exec();
}

