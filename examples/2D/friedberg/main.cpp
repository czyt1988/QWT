/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include "Plot.h"

#include <QApplication>
#include <QMainWindow>
#include <QComboBox>
#include <QToolBar>
#include <QToolButton>
#include "../../common/qwt_example_screenshot.h"

namespace
{
    class MainWindow : public QMainWindow
    {
      public:
        MainWindow( QWidget* parent = NULL )
            : QMainWindow( parent )
        {
            Plot* plot = new Plot();
            setCentralWidget( plot );

            QComboBox* typeBox = new QComboBox();
            typeBox->addItem( "Bars" );
            typeBox->addItem( "Tube" );
            typeBox->setCurrentIndex( 1 );
            typeBox->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

            QToolButton* btnExport = new QToolButton();
            btnExport->setText( "Export" );
            btnExport->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
            connect( btnExport, SIGNAL(clicked()), plot, SLOT(exportPlot()) );

            QToolBar* toolBar = new QToolBar();
            toolBar->addWidget( typeBox );
            toolBar->addWidget( btnExport );
            addToolBar( toolBar );

            plot->setMode( typeBox->currentIndex() );

            connect( typeBox, SIGNAL(currentIndexChanged(int)),
                plot, SLOT(setMode(int)) );

            // Visual variants for unattended screenshot generation
            QwtExampleScreenshot::addVariant( "bar", [typeBox]() { typeBox->setCurrentIndex( 0 ); } );
            QwtExampleScreenshot::addVariant( "tube", [typeBox]() { typeBox->setCurrentIndex( 1 ); } );
        }
    };
}

int main( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    MainWindow window;
    window.setObjectName( "MainWindow" );
    window.resize( 600, 400 );
    window.show();

    QwtExampleScreenshot::start( &window );

    return app.exec();
}
