/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include "BoxChart.h"

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
        MainWindow( QWidget* = NULL );
    };
}

MainWindow::MainWindow( QWidget* parent )
    : QMainWindow( parent )
{
    BoxChart* chart = new BoxChart();
    setCentralWidget( chart );

    QToolBar* toolBar = new QToolBar();

    QLabel* styleLabel = new QLabel( "Box Style:", toolBar );
    toolBar->addWidget( styleLabel );

    QComboBox* styleBox = new QComboBox( toolBar );
    styleBox->addItem( "Rect" );
    styleBox->addItem( "Diamond" );
    styleBox->addItem( "Notch" );
    styleBox->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    toolBar->addWidget( styleBox );

    toolBar->addSeparator();

    QLabel* orientLabel = new QLabel( "Orientation:", toolBar );
    toolBar->addWidget( orientLabel );

    QComboBox* orientationBox = new QComboBox( toolBar );
    orientationBox->addItem( "Vertical" );
    orientationBox->addItem( "Horizontal" );
    orientationBox->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    toolBar->addWidget( orientationBox );

    toolBar->addSeparator();

    QToolButton* btnExport = new QToolButton( toolBar );
    btnExport->setText( "Export" );
    btnExport->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    connect( btnExport, SIGNAL(clicked()), chart, SLOT(exportChart()) );
    toolBar->addWidget( btnExport );

    addToolBar( toolBar );

    chart->setBoxStyle( styleBox->currentIndex() );
    connect( styleBox, SIGNAL(currentIndexChanged(int)),
        chart, SLOT(setBoxStyle(int)) );

    chart->setOrientation( orientationBox->currentIndex() );
    connect( orientationBox, SIGNAL(currentIndexChanged(int)),
        chart, SLOT(setOrientation(int)) );
}

int main( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    MainWindow window;

    window.resize( 700, 500 );
    window.show();

    QwtExampleScreenshot::start( &window );

    return app.exec();
}