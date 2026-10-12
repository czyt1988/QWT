/*****************************************************************************
 * Qwt Polar Examples - Copyright (C) 2008   Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include "PlotWindow.h"
#include "Plot.h"

#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QToolButton>
#include <QComboBox>
#include <QLabel>

#include <QwtColorMapPreset>
#include "../../common/qwt_example_screenshot.h"

namespace
{
    class ToolButton : public QToolButton
    {
      public:
        ToolButton( const QString& text, QWidget* parent = NULL )
            : QToolButton( parent )
        {
            setText( text );
            setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
        };
    };

    class MainWindow : public QMainWindow
    {
      public:
        MainWindow( QWidget* = NULL );
    };
}

MainWindow::MainWindow( QWidget* parent )
    : QMainWindow( parent )
{
    PlotWindow* plotWindow = new PlotWindow();
    setCentralWidget( plotWindow );

    ToolButton* btnGrid = new ToolButton( "Grid" );
    btnGrid->setCheckable( true );
    btnGrid->setChecked( true );

    ToolButton* btnExport = new ToolButton( "Export" );
    ToolButton* btnRotate = new ToolButton( "Rotate" );
    ToolButton* btnMirror = new ToolButton( "Mirror" );

    QComboBox* themeBox = new QComboBox();
    for ( const QString& name : QwtColorMapPreset::availablePresets() )
        themeBox->addItem( name, name );
    themeBox->setCurrentIndex( 0 );

    QToolBar* toolBar = new QToolBar();

    toolBar->addWidget( btnExport );
    toolBar->addWidget( btnGrid );
    toolBar->addWidget( btnRotate );
    toolBar->addWidget( btnMirror );
    toolBar->addWidget( new QLabel( "  Theme " ) );
    toolBar->addWidget( themeBox );

    addToolBar( toolBar );

    Plot* plot = plotWindow->plot();

    connect( btnExport, SIGNAL(clicked()), plot, SLOT(exportDocument()) );
    connect( btnGrid, SIGNAL(toggled(bool)), plot, SLOT(showGrid(bool)) );
    connect( btnRotate, SIGNAL(clicked()), plot, SLOT(rotate()) );
    connect( btnMirror, SIGNAL(clicked()), plot, SLOT(mirror()) );

    QObject::connect( themeBox, QOverload< int >::of( &QComboBox::currentIndexChanged ),
        plotWindow, [themeBox, plotWindow]( int ) {
            plotWindow->setColorMapPreset( themeBox->currentData().toString() );
        } );
}

int main( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    MainWindow window;
    window.resize( 700, 600 );
    window.show();

    QwtExampleScreenshot::start( &window );

    return app.exec();
}
