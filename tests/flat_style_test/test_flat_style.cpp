#include "test_flat_style.h"

#include "qwt_slider.h"
#include "qwt_thermo.h"
#include "qwt_wheel.h"
#include "qwt_knob.h"
#include "qwt_dial_needle.h"
#include "qwt_compass_rose.h"
#include "qwt_plot_opengl_canvas.h"

#include <QTest>
#include <QFrame>

void TestFlatStyle::testSliderDefaultFlatStyle()
{
    QwtSlider slider;
    QCOMPARE( slider.flatStyle(), true );
}

void TestFlatStyle::testSliderSetFlatStyle()
{
    QwtSlider slider;
    slider.setFlatStyle( false );
    QCOMPARE( slider.flatStyle(), false );
    slider.setFlatStyle( true );
    QCOMPARE( slider.flatStyle(), true );
}

void TestFlatStyle::testThermoDefaultFlatStyle()
{
    QwtThermo thermo;
    QCOMPARE( thermo.flatStyle(), true );
}

void TestFlatStyle::testThermoSetFlatStyle()
{
    QwtThermo thermo;
    thermo.setFlatStyle( false );
    QCOMPARE( thermo.flatStyle(), false );
    thermo.setFlatStyle( true );
    QCOMPARE( thermo.flatStyle(), true );
}

void TestFlatStyle::testWheelDefaultFlatStyle()
{
    QwtWheel wheel;
    QCOMPARE( wheel.flatStyle(), true );
}

void TestFlatStyle::testWheelSetFlatStyle()
{
    QwtWheel wheel;
    wheel.setFlatStyle( false );
    QCOMPARE( wheel.flatStyle(), false );
    wheel.setFlatStyle( true );
    QCOMPARE( wheel.flatStyle(), true );
}

void TestFlatStyle::testKnobDefaultStyle()
{
    QwtKnob knob;
    QCOMPARE( knob.knobStyle(), QwtKnob::Flat );
}

void TestFlatStyle::testDialNeedleDefaultFlatStyle()
{
    QwtDialSimpleNeedle needle( QwtDialSimpleNeedle::Arrow );
    QCOMPARE( needle.flatStyle(), true );
}

void TestFlatStyle::testDialNeedleSetFlatStyle()
{
    QwtDialSimpleNeedle needle( QwtDialSimpleNeedle::Arrow );
    needle.setFlatStyle( false );
    QCOMPARE( needle.flatStyle(), false );
    needle.setFlatStyle( true );
    QCOMPARE( needle.flatStyle(), true );
}

void TestFlatStyle::testCompassRoseDefaultFlatStyle()
{
    QwtSimpleCompassRose rose;
    QCOMPARE( rose.flatStyle(), true );
}

void TestFlatStyle::testCompassRoseSetFlatStyle()
{
    QwtSimpleCompassRose rose;
    rose.setFlatStyle( false );
    QCOMPARE( rose.flatStyle(), false );
    rose.setFlatStyle( true );
    QCOMPARE( rose.flatStyle(), true );
}

void TestFlatStyle::testGLCanvasDefaultFrameStyle()
{
    // Verify QwtPlotOpenGLCanvas defaults
    {
        QwtPlotOpenGLCanvas canvas;
        QCOMPARE( canvas.frameShape(), QFrame::Box );
        QCOMPARE( canvas.frameShadow(), QFrame::Plain );
        QCOMPARE( canvas.lineWidth(), 1 );
    }
}

QTEST_MAIN( TestFlatStyle )
