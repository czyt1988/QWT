/*****************************************************************************
 * Qwt Examples
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "SliderTab.h"
#include "WheelTab.h"
#include "KnobTab.h"
#include "DialTab.h"

#include <QApplication>
#include <QTabWidget>
#include "../../common/qwt_example_screenshot.h"

namespace
{
    class TabWidget : public QTabWidget
    {
      public:
        TabWidget()
        {
            SliderTab* sliderTab = new SliderTab();
            sliderTab->setAutoFillBackground( true );
            sliderTab->setPalette( QColor( "DimGray" ) );

            WheelTab* wheelTab = new WheelTab();
            wheelTab->setAutoFillBackground( true );
            wheelTab->setPalette( QColor( "Silver" ) );

            KnobTab* knobTab = new KnobTab();
            knobTab->setAutoFillBackground( true );
            knobTab->setPalette( Qt::darkGray );

            DialTab* dialTab = new DialTab();
            dialTab->setAutoFillBackground( true );
            dialTab->setPalette( Qt::darkGray );

            addTab( sliderTab, "Slider" );
            addTab( wheelTab, "Wheel/Thermo" );
            addTab( knobTab, "Knob" );
            addTab( dialTab, "Dial" );
        }
    };
}

int main( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    TabWidget tabWidget;

    // Visual variants for unattended screenshot generation
    QwtExampleScreenshot::addVariant( "sliders", [&tabWidget]() { tabWidget.setCurrentIndex( 0 ); } );
    QwtExampleScreenshot::addVariant( "wheel-thermo", [&tabWidget]() { tabWidget.setCurrentIndex( 1 ); } );
    QwtExampleScreenshot::addVariant( "knob", [&tabWidget]() { tabWidget.setCurrentIndex( 2 ); } );
    QwtExampleScreenshot::addVariant( "dial", [&tabWidget]() { tabWidget.setCurrentIndex( 3 ); } );

    tabWidget.resize( 800, 600 );
    tabWidget.show();

    QwtExampleScreenshot::start( &tabWidget );

    return app.exec();
}

