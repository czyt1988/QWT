/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include "Plot.h"

#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QToolButton>
#include <QComboBox>
#include <QLabel>
#include "../../common/qwt_example_screenshot.h"

namespace
{
    class MainWindow : public QMainWindow
    {
      public:
        MainWindow( QWidget* parent = NULL )
            : QMainWindow( parent )
        {
            Plot* plot = new Plot( this );
            setCentralWidget( plot );

            QToolBar* toolBar = new QToolBar( this );

            QComboBox* rasterBox = new QComboBox( toolBar );
            rasterBox->addItem( "Wikipedia" );

            toolBar->addWidget( new QLabel( "Data ", toolBar ) );
            toolBar->addWidget( rasterBox );
            toolBar->addSeparator();

            QComboBox* modeBox = new QComboBox( toolBar );
            modeBox->addItem( "Nearest Neighbour" );
            modeBox->addItem( "Bilinear Interpolation" );
            modeBox->addItem( "Bicubic Interpolation" );

            toolBar->addWidget( new QLabel( "Resampling ", toolBar ) );
            toolBar->addWidget( modeBox );

            toolBar->addSeparator();

            QToolButton* btnExport = new QToolButton( toolBar );
            btnExport->setText( "Export" );
            btnExport->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
            toolBar->addWidget( btnExport );

            addToolBar( toolBar );

            connect( modeBox, SIGNAL(activated(int)), plot, SLOT(setResampleMode(int)) );
            connect( btnExport, SIGNAL(clicked()), plot, SLOT(exportPlot()) );

            // Visual variants for unattended screenshot generation. The combo
            // is connected to activated() only, so apply the mode directly.
            QwtExampleScreenshot::addVariant( "nearest", [modeBox, plot]() {
                modeBox->setCurrentIndex( 0 ); plot->setResampleMode( 0 );
            } );
            QwtExampleScreenshot::addVariant( "bilinear", [modeBox, plot]() {
                modeBox->setCurrentIndex( 1 ); plot->setResampleMode( 1 );
            } );
            QwtExampleScreenshot::addVariant( "bicubic", [modeBox, plot]() {
                modeBox->setCurrentIndex( 2 ); plot->setResampleMode( 2 );
            } );
        }
    };
}

int main( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    MainWindow window;
    window.resize( 600, 400 );
    window.show();

    QwtExampleScreenshot::start( &window );

    return app.exec();
}
