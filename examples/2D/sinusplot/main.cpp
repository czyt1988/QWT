/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include <QwtPlot>
#include <QwtPlotMarker>
#include "qwt_plot_arrowmarker.h"
#include <QwtPlotCurve>
#include <QwtLegend>
#include <QwtSyntheticPointData>
#include <QwtPlotCanvas>
#include <QwtPlotPanner>
#include <QwtPlotMagnifier>
#include <QwtSymbol>
#include <QwtText>
#include <QwtMath>

#include <QApplication>
#include <QPainterPath>
#include <QLayout>
#include "qwt_legend.h"
#include "qwt_plot_legenditem.h"
#include "../../common/qwt_example_screenshot.h"
//-----------------------------------------------------------------
//      A simple example which shows how to use QwtPlot connected
//      to a data class without any storage, calculating each values
//      on the fly.
//-----------------------------------------------------------------

namespace
{
class FunctionData : public QwtSyntheticPointData
{
public:
    FunctionData(double (*fy)(double)) : QwtSyntheticPointData(100), m_y(fy)
    {
    }

    virtual double y(double x) const override
    {
        return m_y(x);
    }

private:
    double (*m_y)(double);
};

class ArrowSymbol : public QwtSymbol
{
public:
    ArrowSymbol()
    {
        QPen pen(Qt::black, 0);
        pen.setJoinStyle(Qt::MiterJoin);

        setPen(pen);
        setBrush(Qt::red);

        QPainterPath path;
        path.moveTo(0, 8);
        path.lineTo(0, 5);
        path.lineTo(-3, 5);
        path.lineTo(0, 0);
        path.lineTo(3, 5);
        path.lineTo(0, 5);

        QTransform transform;
        transform.rotate(-30.0);
        path = transform.map(path);

        setPath(path);
        setPinPoint(QPointF(0, 0));

        setSize(10, 14);
    }
};

class Plot : public QwtPlot
{
public:
    Plot(QWidget* parent = NULL);

protected:
    virtual void resizeEvent(QResizeEvent*) override;

private:
    void populate();
    void updateGradient();
};
}

Plot::Plot(QWidget* parent) : QwtPlot(parent)
{
    setAutoFillBackground(true);
    setPalette(QPalette(QColor(165, 193, 228)));
    updateGradient();

    setTitle("A Simple QwtPlot Demonstration");
    insertLegend(new QwtLegend(), QwtPlot::RightLegend);

    // axes
    setAxisTitle(QwtAxis::XBottom, "x -->");
    setAxisScale(QwtAxis::XBottom, 0.0, 10.0);

    setAxisTitle(QwtAxis::YLeft, "y -->");
    setAxisScale(QwtAxis::YLeft, -1.0, 1.0);

    // canvas
    QwtPlotCanvas* canvas = new QwtPlotCanvas();
    canvas->setFrameStyle(QFrame::NoFrame);
    // canvas->setBorderRadius( 15 );

    QPalette canvasPalette(Qt::white);
    canvasPalette.setColor(QPalette::WindowText, QColor(133, 190, 232));
    canvas->setPalette(canvasPalette);

    setCanvas(canvas);

    // panning with the left mouse button
    (void)new QwtPlotPanner(canvas);

    // zoom in/out with the wheel
    (void)new QwtPlotMagnifier(canvas);

    populate();
}

void Plot::populate()
{
    // Insert new curves
    QwtPlotCurve* cSin = new QwtPlotCurve("y = sin(x)");
    cSin->setRenderHint(QwtPlotItem::RenderAntialiased);
    cSin->setLegendAttribute(QwtPlotCurve::LegendShowLine, true);
    cSin->setPen(Qt::red);
    cSin->attach(this);

    QwtPlotCurve* cCos = new QwtPlotCurve("y = cos(x)");
    cCos->setRenderHint(QwtPlotItem::RenderAntialiased);
    cCos->setLegendAttribute(QwtPlotCurve::LegendShowLine, true);
    cCos->setPen(Qt::blue);
    cCos->attach(this);

    // Create sin and cos data
    cSin->setData(new FunctionData(std::sin));
    cCos->setData(new FunctionData(std::cos));

    // Insert markers

    //  ...a horizontal line at y = 0...
    QwtPlotMarker* mY = new QwtPlotMarker();
    mY->setLabel(QString::fromLatin1("y = 0"));
    mY->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
    mY->setLineStyle(QwtPlotMarker::HLine);
    mY->setYValue(0.0);
    mY->attach(this);

    //  ...a vertical line at x = 2 * pi
    QwtPlotMarker* mX = new QwtPlotMarker();
    mX->setLabel(QString::fromLatin1("x = 2 pi"));
    mX->setLabelAlignment(Qt::AlignLeft | Qt::AlignBottom);
    mX->setLabelOrientation(Qt::Vertical);
    mX->setLineStyle(QwtPlotMarker::VLine);
    mX->setLinePen(Qt::black, 0, Qt::DashDotLine);
    mX->setXValue(2.0 * M_PI);
    mX->attach(this);

    const double x = 7.7;

    // an arrow at a specific position
    QwtPlotMarker* mPos = new QwtPlotMarker("Marker");
    mPos->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    mPos->setItemAttribute(QwtPlotItem::Legend, true);
    mPos->setSymbol(new ArrowSymbol());
    mPos->setValue(QPointF(x, std::sin(x)));
    mPos->setLabel(QString("x = %1").arg(x));
    mPos->setLabelAlignment(Qt::AlignRight | Qt::AlignBottom);
    mPos->attach(this);

    // Insert QwtPlotArrowMarker examples
    // Example 1: Simple arrow with explicit start and end points
    QwtPlotArrowMarker* arrow1 = new QwtPlotArrowMarker("Arrow 1");
    arrow1->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    arrow1->setItemAttribute(QwtPlotItem::Legend, true);
    arrow1->setPoints(QPointF(1.0, 0.5), QPointF(3.0, 0.8));
    arrow1->setLinePen(Qt::darkGreen, 2.0);
    arrow1->setHeadStyle(QwtPlotArrowMarker::ArrowHead);
    arrow1->setHeadSize(12.0);
    arrow1->setHeadBrush(Qt::green);
    arrow1->setTailStyle(QwtPlotArrowMarker::Circle);
    arrow1->setTailSize(8.0);
    arrow1->setTailBrush(Qt::yellow);
    arrow1->attach(this);

    // Example 2: Arrow using length and angle
    QwtPlotArrowMarker* arrow2 = new QwtPlotArrowMarker("Arrow 2");
    arrow2->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    arrow2->setItemAttribute(QwtPlotItem::Legend, true);
    arrow2->setStartPoint(QPointF(5.0, -0.5));
    arrow2->setPositionMode(QwtPlotArrowMarker::StartLengthAngle);
    arrow2->setLength(80.0);  // pixels
    arrow2->setAngle(135.0);  // degrees
    arrow2->setLinePen(QColor(255, 128, 0), 3.0, Qt::DashLine);
    arrow2->setHeadStyle(QwtPlotArrowMarker::Diamond);
    arrow2->setHeadSize(QSizeF(10.0, 15.0));
    arrow2->setHeadBrush(QColor(255, 200, 0));
    arrow2->setHeadPen(QPen(Qt::darkRed, 1.5));
    arrow2->attach(this);

    // Example 3: Arrow with custom endpoint styles
    QwtPlotArrowMarker* arrow3 = new QwtPlotArrowMarker("Arrow 3");
    arrow3->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    arrow3->setItemAttribute(QwtPlotItem::Legend, true);
    arrow3->setPoints(QPointF(6.0, 0.0), QPointF(8.0, -0.7));
    arrow3->setLinePen(Qt::magenta, 2.5);

    // Custom head path (star shape)
    QPainterPath starPath;
    starPath.moveTo(0, -5);
    for (int i = 1; i < 5; ++i) {
        double angle = i * 4 * M_PI / 5;
        starPath.lineTo(5 * sin(angle), -5 * cos(angle));
    }
    starPath.closeSubpath();

    arrow3->setHeadStyle(QwtPlotArrowMarker::CustomPath);
    arrow3->setHeadCustomPath(starPath);
    arrow3->setHeadSize(15.0);
    arrow3->setHeadBrush(QColor(255, 105, 180));  // Hot pink
    arrow3->setHeadPen(QPen(Qt::darkMagenta, 1.0));

    arrow3->setTailStyle(QwtPlotArrowMarker::Square);
    arrow3->setTailSize(10.0);
    arrow3->setTailBrush(Qt::cyan);
    arrow3->setTailPen(QPen(Qt::darkBlue, 1.0));
    arrow3->attach(this);

    // Example 4: Vertical arrow with triangle endpoints
    QwtPlotArrowMarker* arrow4 = new QwtPlotArrowMarker("Arrow 4");
    arrow4->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    arrow4->setItemAttribute(QwtPlotItem::Legend, true);
    arrow4->setPoints(QPointF(9.0, 0.8), QPointF(9.0, 0.2));
    arrow4->setLinePen(Qt::darkBlue, 2.0);
    arrow4->setHeadStyle(QwtPlotArrowMarker::Triangle);
    arrow4->setHeadSize(12.0);
    arrow4->setHeadBrush(Qt::blue);
    arrow4->setTailStyle(QwtPlotArrowMarker::Triangle);
    arrow4->setTailSize(12.0);
    arrow4->setTailBrush(Qt::red);
    arrow4->attach(this);

    QwtPlotLegendItem* legItem = new QwtPlotLegendItem();
    legItem->attach(this);

    // show legend
    QwtLegend* leg = qobject_cast< QwtLegend* >(legend());
    if (leg) {
        leg->setVisible(true);
        leg->setDefaultItemMode(QwtLegendData::Checkable);
    }
}

void Plot::updateGradient()
{
    QPalette pal = palette();

    const QColor buttonColor = pal.color(QPalette::Button);

    QLinearGradient gradient(rect().topLeft(), rect().bottomLeft());
    gradient.setColorAt(0.0, Qt::white);
    gradient.setColorAt(0.7, buttonColor);
    gradient.setColorAt(1.0, buttonColor);

    pal.setBrush(QPalette::Window, gradient);
    setPalette(pal);
}

void Plot::resizeEvent(QResizeEvent* event)
{
    QwtPlot::resizeEvent(event);

    // Qt 4.7.1: QGradient::StretchToDeviceMode is buggy on X11
    updateGradient();
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    Plot* plot = new Plot();

    // We put a dummy widget around to have
    // so that Qt paints a widget background
    // when resizing

    QWidget window;

    QHBoxLayout* layout = new QHBoxLayout(&window);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(plot);

    window.resize(600, 400);
    window.show();

    QwtExampleScreenshot::start( &window );

    return app.exec();
}
