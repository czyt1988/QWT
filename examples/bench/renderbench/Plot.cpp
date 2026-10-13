/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include "Plot.h"
#include "WindowedSeriesData.h"

#include <QPen>
#include <QwtPlotCurve>
#include <QwtPlotCanvas>
#include <QwtPlotLayout>
#include <QwtScaleWidget>

Plot::Plot(QWidget* parent)
    : QwtPlot(parent)
{
    setTitle("Render Benchmark");

    QwtPlotCanvas* canvas = new QwtPlotCanvas();
    canvas->setFrameStyle(QFrame::NoFrame);
    canvas->setPalette(Qt::white);
    canvas->setPaintAttribute(QwtPlotCanvas::BackingStore, false);
    setCanvas(canvas);

    for (int i = 0; i < QwtAxis::AxisPositions; i++) {
        QwtScaleWidget* sw = axisWidget(i);
        if (sw)
            sw->setMargin(0);
    }
    plotLayout()->setAlignCanvasToScales(true);

    setAxisTitle(QwtAxis::XBottom, "X");
    setAxisTitle(QwtAxis::YLeft, "Y");

    m_curve = new QwtPlotCurve("Benchmark");
    m_curve->setPen(QPen(Qt::darkBlue, 1.0));
    m_curve->setRenderHint(QwtPlotItem::RenderAntialiased, false);
    m_curve->attach(this);
}

void Plot::setCurveData(WindowedSeriesData* data)
{
    m_curve->setData(data);
}

void Plot::setRenderingMethod(int methodIndex)
{
    m_curve->setPaintAttribute(QwtPlotCurve::ClipPolygons, true);
    m_curve->setPaintAttribute(QwtPlotCurve::FilterPoints, false);
    m_curve->setPaintAttribute(QwtPlotCurve::FilterPointsAggressive, false);
    m_curve->setPaintAttribute(QwtPlotCurve::FilterPointsPixel, false);
    m_curve->setPaintAttribute(QwtPlotCurve::FilterPointsLTTB, false);
    m_curve->setPaintAttribute(QwtPlotCurve::FilterPointsMinMaxLTTB, false);

    switch (methodIndex) {
    case 0:
        m_curve->setPaintAttribute(QwtPlotCurve::ClipPolygons, false);
        break;
    case 1:
        m_curve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
        break;
    case 2:
        m_curve->setPaintAttribute(QwtPlotCurve::FilterPointsAggressive, true);
        break;
    case 3:
        m_curve->setPaintAttribute(QwtPlotCurve::FilterPointsPixel, true);
        break;
    case 4:
        m_curve->setPaintAttribute(QwtPlotCurve::FilterPointsLTTB, true);
        break;
    case 5:
        m_curve->setPaintAttribute(QwtPlotCurve::FilterPointsMinMaxLTTB, true);
        break;
    }
}

#include "moc_Plot.cpp"
