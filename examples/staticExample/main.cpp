/*****************************************************************************
 * Qwt Static Example — Comprehensive showcase of Qwt 2D plotting features
 *
 * Demonstrates:
 *   Plot types: Curve, MultiBarChart, Spectrogram, BoxChart, TradingCurve,
 *               VectorField, Scatter, IntervalCurve, Histogram, ShapeItem
 *   Annotations: Marker, ArrowMarker, TextLabel, ZoneItem
 *   Scales: Linear, Logarithmic, Date
 *   Tools: CanvasZoomer, AxisZoomer, Panner, Magnifier,
 *          SeriesDataPicker, PlotPicker (crosshair)
 *   Extras: SplineCurveFitter, Legend, ColorBar, ContourLevels
 *   Polar: QwtPolarPlot with rose and spiral curves
 *****************************************************************************/

#include "QwtPlot.h"

#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>
#include <cmath>
#include "../common/qwt_example_screenshot.h"

using namespace Qt;

// ---------------------------------------------------------------------------
// Data generation helpers
// ---------------------------------------------------------------------------

static QVector<QPointF> generateSineData(int count, double amp, double freq)
{
    QVector<QPointF> data;
    data.reserve(count);
    for (int i = 0; i < count; ++i) {
        double x = i * 10.0 / count;
        data.append(QPointF(x, amp * std::sin(freq * x)));
    }
    return data;
}

static QVector<QPointF> generateCosineData(int count, double amp, double freq)
{
    QVector<QPointF> data;
    data.reserve(count);
    for (int i = 0; i < count; ++i) {
        double x = i * 10.0 / count;
        data.append(QPointF(x, amp * std::cos(freq * x)));
    }
    return data;
}

static QVector<QPointF> generateScatterData(int count)
{
    QVector<QPointF> data;
    data.reserve(count);
    for (int i = 0; i < count; ++i) {
        double angle = i * 0.037;
        double r = 5.0 * std::sin(3.0 * angle) + (i % 17) * 0.15;
        data.append(QPointF(r * std::cos(angle), r * std::sin(angle)));
    }
    return data;
}

static QVector<QwtOHLCSample> generateOhlcData(int count)
{
    QVector<QwtOHLCSample> samples;
    samples.reserve(count);
    double baseTime = QwtDate::toDouble(
        QDateTime(QDate(2025, 1, 2), QTime(0, 0), Qt::UTC));
    double dayMs = 86400000.0;
    double price = 100.0;
    for (int i = 0; i < count; ++i) {
        double open = price;
        double change = std::sin(i * 0.5) * 3.0 + std::cos(i * 0.3) * 2.0;
        double close = open + change;
        double high = qMax(open, close) + std::abs(std::sin(i * 0.7)) * 2.0;
        double low = qMin(open, close) - std::abs(std::cos(i * 0.9)) * 2.0;
        samples.append(QwtOHLCSample(baseTime + i * dayMs, open, high, low, close));
        price = close;
    }
    return samples;
}

static QVector<double> generateSpectrogramMatrix(int rows, int cols)
{
    QVector<double> values(rows * cols);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            double x = -1.5 + 3.0 * c / cols;
            double y = -1.5 + 3.0 * r / rows;
            values[r * cols + c] = std::sin(x * 3.0) * std::cos(y * 3.0)
                                   + 0.5 * std::sin(x * 7.0 + y * 5.0);
        }
    }
    return values;
}

// ---------------------------------------------------------------------------
// Common style helper
// ---------------------------------------------------------------------------

static void setupGrid(QwtPlot* plot)
{
    plot->setCanvasBackground(Qt::white);
    plot->setFrameStyle(QFrame::NoFrame);

    auto* grid = new QwtPlotGrid();
    grid->setMajorPen(QColor("#c0c0c0"), 0.5, Qt::SolidLine);
    grid->setMinorPen(QColor("#e0e0e0"), 0.5, Qt::DotLine);
    grid->enableXMin(true);
    grid->enableYMin(true);
    grid->attach(plot);
}

// ---------------------------------------------------------------------------
// ① Multi-curve + Marker + Spline + DataPicker
// ---------------------------------------------------------------------------

static QwtPlot* createCurvePlot()
{
    auto* plot = new QwtPlot();
    plot->setTitle("Curves & Markers");
    setupGrid(plot);

    // Curve 1: solid line with spline fitting
    auto* curve1 = new QwtPlotCurve("Sine (Spline)");
    curve1->setSamples(generateSineData(20, 1.0, 2.0));
    curve1->setPen(Qt::blue, 2);
    curve1->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curve1->setCurveFitter(new QwtSplineCurveFitter());
    curve1->attach(plot);

    // Curve 2: dashed line with ellipse symbols
    auto* curve2 = new QwtPlotCurve("Cosine");
    curve2->setSamples(generateCosineData(30, 0.8, 1.5));
    curve2->setPen(Qt::red, 1.5);
    curve2->setStyle(QwtPlotCurve::Lines);
    QPen dashPen(Qt::red, 1.5, Qt::DashLine);
    curve2->setPen(dashPen);
    curve2->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,
                                    QBrush(QColor(255, 200, 200)),
                                    QPen(Qt::red, 1), QSize(6, 6)));
    curve2->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curve2->attach(plot);

    // Curve 3: sticks style
    auto* curve3 = new QwtPlotCurve("Sticks");
    QVector<QPointF> stickData;
    for (int i = 0; i < 15; ++i) {
        double x = i * 0.7;
        stickData.append(QPointF(x, 0.5 * std::sin(x * 1.2) + 0.3));
    }
    curve3->setSamples(stickData);
    curve3->setStyle(QwtPlotCurve::Sticks);
    curve3->setPen(QColor(0, 150, 0), 1.5);
    curve3->attach(plot);

    // Vertical marker
    auto* vMarker = new QwtPlotMarker();
    vMarker->setLineStyle(QwtPlotMarker::VLine);
    vMarker->setLinePen(QPen(QColor(180, 180, 180), 1, Qt::DashDotLine));
    vMarker->setValue(5.0, 0);
    QwtText vLabel("x = 5.0");
    vLabel.setColor(QColor(120, 120, 120));
    vLabel.setFont(QFont("Arial", 8));
    vMarker->setLabel(vLabel);
    vMarker->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
    vMarker->attach(plot);

    // Horizontal marker
    auto* hMarker = new QwtPlotMarker();
    hMarker->setLineStyle(QwtPlotMarker::HLine);
    hMarker->setLinePen(QPen(QColor(200, 100, 100), 1, Qt::DashLine));
    hMarker->setValue(0, 0.5);
    QwtText hLabel("y = 0.5");
    hLabel.setColor(QColor(200, 100, 100));
    hLabel.setFont(QFont("Arial", 8));
    hMarker->setLabel(hLabel);
    hMarker->setLabelAlignment(Qt::AlignRight | Qt::AlignBottom);
    hMarker->attach(plot);

    plot->setAxisTitle(QwtAxis::XBottom, "X");
    plot->setAxisTitle(QwtAxis::YLeft, "Y");
    plot->insertLegend(new QwtLegend(), QwtPlot::BottomLegend);

    // Data picker: tracks Y values across all curves
    auto* picker = new QwtPlotSeriesDataPicker(plot->canvas());
    picker->setPickMode(QwtPlotSeriesDataPicker::PickYValue);
    picker->setInterpolationMode(QwtPlotSeriesDataPicker::LinearInterpolation);
    picker->setEnableDrawFeaturePoint(true);
    picker->setDrawFeaturePointSize(5);
    picker->setTextArea(QwtPlotSeriesDataPicker::TextFollowOnBottom);

    return plot;
}

// ---------------------------------------------------------------------------
// ② Grouped bar chart (MultiBarChart)
// ---------------------------------------------------------------------------

static QwtPlot* createBarChartPlot()
{
    auto* plot = new QwtPlot();
    plot->setTitle("Grouped Bar Chart");
    setupGrid(plot);

    const int numGroups = 6;
    const int numSeries = 3;

    QVector<QVector<double>> values(numGroups);
    for (int i = 0; i < numGroups; ++i) {
        values[i].resize(numSeries);
        values[i][0] = 30 + 50 * std::abs(std::sin(i * 0.8));
        values[i][1] = 20 + 40 * std::abs(std::cos(i * 1.2));
        values[i][2] = 25 + 35 * std::abs(std::sin(i * 0.5 + 1.0));
    }

    auto* barChart = new QwtPlotMultiBarChart("Sales Data");
    barChart->setSamples(values);
    barChart->setStyle(QwtPlotMultiBarChart::Grouped);

    // Per-series bar titles for legend
    QList<QwtText> titles;
    titles << QwtText("Product A") << QwtText("Product B") << QwtText("Product C");
    barChart->setBarTitles(titles);

    // Per-series symbols
    QColor colors[] = { QColor(70, 130, 180), QColor(255, 165, 0), QColor(60, 179, 113) };
    for (int s = 0; s < numSeries; ++s) {
        auto* sym = new QwtColumnSymbol(QwtColumnSymbol::Box);
        sym->setLineWidth(1);
        sym->setFrameStyle(QwtColumnSymbol::Plain);
        sym->setBrush(colors[s]);
        sym->setPen(QPen(colors[s].darker(130), 1));
        barChart->setSymbol(s, sym);
    }

    barChart->setSpacing(15);
    barChart->setMargin(3);
    barChart->attach(plot);

    plot->setAxisScale(QwtAxis::XBottom, -0.5, numGroups - 0.5);
    plot->setAxisTitle(QwtAxis::XBottom, "Quarter");
    plot->setAxisTitle(QwtAxis::YLeft, "Revenue");
    plot->insertLegend(new QwtLegend(), QwtPlot::BottomLegend);

    return plot;
}

// ---------------------------------------------------------------------------
// ③ Spectrogram + Contour + ColorBar
// ---------------------------------------------------------------------------

static QwtPlot* createSpectrogramPlot()
{
    auto* plot = new QwtPlot();
    plot->setTitle("Spectrogram");
    plot->setCanvasBackground(Qt::white);
    plot->setFrameStyle(QFrame::NoFrame);

    const int rows = 100, cols = 100;
    auto* rasterData = new QwtMatrixRasterData();
    rasterData->setValueMatrix(generateSpectrogramMatrix(rows, cols), cols);
    rasterData->setInterval(Qt::XAxis, QwtInterval(-1.5, 1.5));
    rasterData->setInterval(Qt::YAxis, QwtInterval(-1.5, 1.5));
    rasterData->setInterval(Qt::ZAxis, QwtInterval(-1.5, 1.5));
    rasterData->setResampleMode(QwtMatrixRasterData::BilinearInterpolation);

    auto* colorMap = new QwtLinearColorMap(Qt::darkBlue, Qt::darkRed);
    colorMap->addColorStop(0.2, Qt::blue);
    colorMap->addColorStop(0.4, Qt::cyan);
    colorMap->addColorStop(0.6, Qt::green);
    colorMap->addColorStop(0.8, Qt::yellow);

    auto* spectrogram = new QwtPlotSpectrogram();
    spectrogram->setData(rasterData);
    spectrogram->setColorMap(colorMap);
    spectrogram->setDisplayMode(QwtPlotSpectrogram::ImageMode, true);
    spectrogram->setDisplayMode(QwtPlotSpectrogram::ContourMode, true);
    spectrogram->setDefaultContourPen(QPen(Qt::black, 0.3));

    QList<double> levels;
    for (double l = -1.2; l <= 1.2; l += 0.3)
        levels.append(l);
    spectrogram->setContourLevels(levels);
    spectrogram->attach(plot);

    // ColorBar on YRight
    plot->setAxisScale(QwtAxis::YRight, -1.5, 1.5);
    plot->setAxisVisible(QwtAxis::YRight, true);
    QwtScaleWidget* rightAxis = plot->axisWidget(QwtAxis::YRight);
    rightAxis->setColorBarEnabled(true);
    auto* barColorMap = new QwtLinearColorMap(Qt::darkBlue, Qt::darkRed);
    barColorMap->addColorStop(0.2, Qt::blue);
    barColorMap->addColorStop(0.4, Qt::cyan);
    barColorMap->addColorStop(0.6, Qt::green);
    barColorMap->addColorStop(0.8, Qt::yellow);
    rightAxis->setColorMap(QwtInterval(-1.5, 1.5), barColorMap);
    rightAxis->setColorBarWidth(15);

    plot->setAxisScale(QwtAxis::XBottom, -1.5, 1.5);
    plot->setAxisScale(QwtAxis::YLeft, -1.5, 1.5);
    plot->setAxisTitle(QwtAxis::XBottom, "X");
    plot->setAxisTitle(QwtAxis::YLeft, "Y");

    // Panner (right button, horizontal only)
    auto* panner = new QwtPlotPanner(plot->canvas());
    panner->setMouseButton(Qt::RightButton);
    panner->setOrientations(Qt::Horizontal);

    return plot;
}

// ---------------------------------------------------------------------------
// ④ Box chart (native QwtPlotBoxChart)
// ---------------------------------------------------------------------------

static QwtPlot* createBoxChartPlot()
{
    auto* plot = new QwtPlot();
    plot->setTitle("Box Chart");
    setupGrid(plot);

    QVector<QwtBoxSample> samples;
    samples << QwtBoxSample(0, 5, 15, 25, 35, 50)
            << QwtBoxSample(1, 10, 20, 30, 40, 55)
            << QwtBoxSample(2, 8, 18, 28, 38, 48)
            << QwtBoxSample(3, 12, 22, 32, 42, 60)
            << QwtBoxSample(4, 3, 13, 23, 33, 45);

    auto* boxChart = new QwtPlotBoxChart("Distribution");
    boxChart->setSamples(samples);
    boxChart->setBoxStyle(QwtPlotBoxChart::Rect);
    boxChart->setWhiskerStyle(QwtPlotBoxChart::StandardWhisker);
    boxChart->setBrush(QBrush(QColor(70, 130, 180, 150)));
    boxChart->setPen(QPen(QColor(40, 80, 120), 1.5));
    boxChart->setMedianPen(QPen(Qt::red, 2));
    boxChart->setMedianVisible(true);
    boxChart->setBoxExtent(0.5);

    // Outliers
    QVector<QwtBoxOutlierSample> outliers;
    outliers << QwtBoxOutlierSample(0, { 2.0, 55.0 })
             << QwtBoxOutlierSample(2, { 1.0, 54.0 })
             << QwtBoxOutlierSample(4, { 50.0, 53.0 });
    boxChart->setOutliers(outliers);
    boxChart->setOutlierSymbol(
        new QwtSymbol(QwtSymbol::Diamond, QBrush(Qt::red),
                      QPen(Qt::darkRed, 1), QSize(6, 6)));
    boxChart->setOutlierJitter(0.1);

    boxChart->attach(plot);

    plot->setAxisScale(QwtAxis::XBottom, -0.5, 4.5, 1.0);
    plot->setAxisTitle(QwtAxis::XBottom, "Group");
    plot->setAxisTitle(QwtAxis::YLeft, "Value");
    plot->insertLegend(new QwtLegend(), QwtPlot::BottomLegend);

    return plot;
}

// ---------------------------------------------------------------------------
// ⑤ Trading curve (K-line) + Date axis + Zone highlight
// ---------------------------------------------------------------------------

static QwtPlot* createTradingPlot()
{
    auto* plot = new QwtPlot();
    plot->setTitle("Trading Chart (K-Line)");
    setupGrid(plot);

    auto ohlcData = generateOhlcData(30);

    auto* tradingCurve = new QwtPlotTradingCurve("OHLC");
    tradingCurve->setSamples(ohlcData);
    tradingCurve->setSymbolStyle(QwtPlotTradingCurve::CandleStick);
    tradingCurve->setSymbolPen(QPen(Qt::black, 1));
    tradingCurve->setSymbolBrush(QwtPlotTradingCurve::Increasing, QBrush(Qt::white));
    tradingCurve->setSymbolBrush(QwtPlotTradingCurve::Decreasing, QBrush(QColor(220, 50, 50)));
    tradingCurve->setSymbolExtent(86400000.0 * 0.8);
    tradingCurve->setMinSymbolWidth(3);
    tradingCurve->setMaxSymbolWidth(15);
    tradingCurve->attach(plot);

    // Date axis
    plot->setAxisScaleEngine(QwtAxis::XBottom, new QwtDateScaleEngine(Qt::UTC));
    auto* dateScale = new QwtDateScaleDraw(Qt::UTC);
    dateScale->setDateFormat(QwtDate::Day, "MM-dd");
    plot->setAxisScaleDraw(QwtAxis::XBottom, dateScale);
    plot->setAxisLabelRotation(QwtAxis::XBottom, -40.0);
    plot->setAxisLabelAlignment(QwtAxis::XBottom, Qt::AlignLeft | Qt::AlignBottom);

    plot->setAxisTitle(QwtAxis::YLeft, "Price");
    plot->insertLegend(new QwtLegend(), QwtPlot::BottomLegend);

    // Zone highlight
    double zoneStart = QwtDate::toDouble(
        QDateTime(QDate(2025, 1, 10), QTime(0, 0), Qt::UTC));
    double zoneEnd = QwtDate::toDouble(
        QDateTime(QDate(2025, 1, 15), QTime(0, 0), Qt::UTC));

    auto* zone = new QwtPlotZoneItem();
    zone->setOrientation(Qt::Vertical);
    zone->setInterval(zoneStart, zoneEnd);
    QPen zonePen(QColor(255, 200, 0, 100));
    zonePen.setWidth(1);
    zone->setPen(zonePen);
    zone->setBrush(QBrush(QColor(255, 200, 0, 30)));
    zone->attach(plot);

    // Event marker
    double eventTime = QwtDate::toDouble(
        QDateTime(QDate(2025, 1, 20), QTime(0, 0), Qt::UTC));
    auto* eventMarker = new QwtPlotMarker();
    eventMarker->setLineStyle(QwtPlotMarker::VLine);
    eventMarker->setLinePen(QPen(QColor(200, 0, 200), 1, Qt::DashLine));
    eventMarker->setValue(eventTime, 0);
    QwtText eventLabel("Event");
    eventLabel.setColor(QColor(200, 0, 200));
    eventLabel.setFont(QFont("Arial", 8));
    eventMarker->setLabel(eventLabel);
    eventMarker->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
    eventMarker->attach(plot);

    // Panner
    auto* panner = new QwtPlotPanner(plot->canvas());
    panner->setMouseButton(Qt::RightButton);

    return plot;
}

// ---------------------------------------------------------------------------
// ⑥ Vector field
// ---------------------------------------------------------------------------

static QwtPlot* createVectorFieldPlot()
{
    auto* plot = new QwtPlot();
    plot->setTitle("Vector Field");
    setupGrid(plot);

    QVector<QwtVectorFieldSample> samples;
    const int gridSize = 7;
    for (int i = 0; i < gridSize; ++i) {
        for (int j = 0; j < gridSize; ++j) {
            double x = -3.0 + 6.0 * i / (gridSize - 1);
            double y = -3.0 + 6.0 * j / (gridSize - 1);
            double vx = -y * 0.3;
            double vy = x * 0.3;
            samples.append(QwtVectorFieldSample(x, y, vx, vy));
        }
    }

    auto* vectorField = new QwtPlotVectorField("Flow");
    vectorField->setSamples(samples);
    vectorField->setSymbol(new QwtVectorFieldArrow(5.0, 1.5));
    vectorField->setPen(QPen(QColor(0, 100, 180), 1.5));
    vectorField->setBrush(QBrush(QColor(0, 100, 180)));
    vectorField->setMagnitudeMode(QwtPlotVectorField::MagnitudeAsLength, true);
    vectorField->setMagnitudeMode(QwtPlotVectorField::MagnitudeAsColor, true);
    vectorField->setIndicatorOrigin(QwtPlotVectorField::OriginTail);

    auto* magColorMap = new QwtLinearColorMap(Qt::blue, Qt::red);
    vectorField->setColorMap(magColorMap);
    vectorField->setMagnitudeRange(QwtInterval(0.0, 2.0));

    vectorField->attach(plot);

    plot->setAxisScale(QwtAxis::XBottom, -4, 4);
    plot->setAxisScale(QwtAxis::YLeft, -4, 4);
    plot->setAxisTitle(QwtAxis::XBottom, "X");
    plot->setAxisTitle(QwtAxis::YLeft, "Y");
    plot->insertLegend(new QwtLegend(), QwtPlot::BottomLegend);

    return plot;
}

// ---------------------------------------------------------------------------
// ⑦ Scatter plot (big data) + Magnifier
// ---------------------------------------------------------------------------

static QwtPlot* createScatterPlot()
{
    auto* plot = new QwtPlot();
    plot->setTitle("Scatter Plot (100K pts)");
    setupGrid(plot);

    auto data = generateScatterData(100000);

    auto* curve = new QwtPlotCurve("Scatter");
    curve->setSamples(data);
    curve->setStyle(QwtPlotCurve::Dots);
    curve->setPen(QPen(QColor(0, 100, 200, 100), 1));
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, false);
    curve->setPaintAttribute(QwtPlotCurve::ImageBuffer, true);
    curve->attach(plot);

    plot->setAxisTitle(QwtAxis::XBottom, "X");
    plot->setAxisTitle(QwtAxis::YLeft, "Y");

    // Magnifier (scroll wheel zoom)
    auto* magnifier = new QwtPlotMagnifier(plot->canvas());
    magnifier->setMouseButton(Qt::NoButton);

    return plot;
}

// ---------------------------------------------------------------------------
// ⑧ Interval curve + Histogram
// ---------------------------------------------------------------------------

static QwtPlot* createIntervalHistPlot()
{
    auto* plot = new QwtPlot();
    plot->setTitle("Interval & Histogram");
    setupGrid(plot);

    // Interval curve (confidence band)
    QVector<QwtIntervalSample> intervalData;
    for (int i = 0; i < 20; ++i) {
        double x = i * 0.5;
        double y = std::sin(x * 0.8);
        intervalData.append(QwtIntervalSample(x, y - 0.3, y + 0.3));
    }

    auto* intervalCurve = new QwtPlotIntervalCurve("Confidence Band");
    intervalCurve->setSamples(intervalData);
    intervalCurve->setStyle(QwtPlotIntervalCurve::Tube);
    intervalCurve->setBrush(QBrush(QColor(70, 130, 180, 80)));
    intervalCurve->setPen(QPen(QColor(70, 130, 180), 1));
    intervalCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    intervalCurve->attach(plot);

    // Mean line
    QVector<QPointF> meanLine;
    for (int i = 0; i < 20; ++i) {
        double x = i * 0.5;
        meanLine.append(QPointF(x, std::sin(x * 0.8)));
    }
    auto* meanCurve = new QwtPlotCurve("Mean");
    meanCurve->setSamples(meanLine);
    meanCurve->setPen(QPen(QColor(40, 80, 120), 2));
    meanCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    meanCurve->attach(plot);

    // Histogram
    QVector<QwtIntervalSample> histData;
    for (int i = 0; i < 10; ++i) {
        double center = i + 0.5;
        double h = 2.0 + 1.5 * std::sin(i * 0.6);
        histData.append(QwtIntervalSample(center, 0, h));
    }

    auto* histogram = new QwtPlotHistogram("Histogram");
    histogram->setSamples(histData);
    histogram->setStyle(QwtPlotHistogram::Columns);
    histogram->setBrush(QBrush(QColor(255, 165, 0, 100)));
    histogram->setPen(QPen(QColor(200, 130, 0), 1));
    histogram->setBaseline(0);
    histogram->attach(plot);

    plot->setAxisTitle(QwtAxis::XBottom, "X");
    plot->setAxisTitle(QwtAxis::YLeft, "Y");
    plot->insertLegend(new QwtLegend(), QwtPlot::BottomLegend);

    return plot;
}

// ---------------------------------------------------------------------------
// ⑨ Shapes + Arrow markers + Text label
// ---------------------------------------------------------------------------

static QwtPlot* createShapePlot()
{
    auto* plot = new QwtPlot();
    plot->setTitle("Shapes & Arrows");
    setupGrid(plot);

    // Star shape
    QPainterPath starPath;
    double cx = 3.0, cy = 5.0, outerR = 1.5, innerR = 0.6;
    for (int i = 0; i < 10; ++i) {
        double angle = M_PI / 2 + i * M_PI / 5;
        double r = (i % 2 == 0) ? outerR : innerR;
        double px = cx + r * std::cos(angle);
        double py = cy + r * std::sin(angle);
        if (i == 0)
            starPath.moveTo(px, py);
        else
            starPath.lineTo(px, py);
    }
    starPath.closeSubpath();

    auto* starItem = new QwtPlotShapeItem("Star");
    starItem->setShape(starPath);
    starItem->setBrush(QBrush(QColor(255, 215, 0, 200)));
    starItem->setPen(QPen(QColor(200, 160, 0), 1.5));
    starItem->setItemAttribute(QwtPlotItem::Legend, true);
    starItem->setLegendMode(QwtPlotShapeItem::LegendShape);
    starItem->attach(plot);

    // Hexagon shape
    QPainterPath hexPath;
    double hx = 7.0, hy = 5.0, hr = 1.2;
    for (int i = 0; i < 6; ++i) {
        double angle = i * M_PI / 3;
        double px = hx + hr * std::cos(angle);
        double py = hy + hr * std::sin(angle);
        if (i == 0)
            hexPath.moveTo(px, py);
        else
            hexPath.lineTo(px, py);
    }
    hexPath.closeSubpath();

    auto* hexItem = new QwtPlotShapeItem("Hexagon");
    hexItem->setShape(hexPath);
    hexItem->setBrush(QBrush(QColor(100, 180, 255, 180)));
    hexItem->setPen(QPen(QColor(60, 120, 200), 1.5));
    hexItem->setItemAttribute(QwtPlotItem::Legend, true);
    hexItem->setLegendMode(QwtPlotShapeItem::LegendShape);
    hexItem->attach(plot);

    // Ellipse shape
    QPainterPath ellipsePath;
    ellipsePath.addEllipse(QPointF(5.0, 2.0), 1.8, 0.8);

    auto* ellipseItem = new QwtPlotShapeItem("Ellipse");
    ellipseItem->setShape(ellipsePath);
    ellipseItem->setBrush(QBrush(QColor(180, 255, 180, 150)));
    ellipseItem->setPen(QPen(QColor(80, 160, 80), 1.5));
    ellipseItem->setItemAttribute(QwtPlotItem::Legend, true);
    ellipseItem->setLegendMode(QwtPlotShapeItem::LegendShape);
    ellipseItem->attach(plot);

    // Arrow from star to hexagon (drawn as a shape path)
    auto* arrow1 = new QwtPlotShapeItem("Arrow 1");
    {
        QPainterPath arrowPath;
        // Line body
        double x1 = 4.3, y1 = 5.0, x2 = 5.8, y2 = 5.0;
        double headLen = 0.4, headW = 0.2;
        double dx = x2 - x1, dy = y2 - y1;
        double len = std::sqrt(dx * dx + dy * dy);
        double ux = dx / len, uy = dy / len;
        double px = -uy, py = ux;
        // Shaft
        arrowPath.moveTo(x1, y1);
        arrowPath.lineTo(x2 - ux * headLen, y2 - uy * headLen);
        // Arrow head
        double hx = x2 - ux * headLen, hy = y2 - uy * headLen;
        arrowPath.lineTo(hx + px * headW, hy + py * headW);
        arrowPath.lineTo(x2, y2);
        arrowPath.lineTo(hx - px * headW, hy - py * headW);
        arrowPath.lineTo(hx, hy);
        arrowPath.closeSubpath();
        arrow1->setShape(arrowPath);
    }
    arrow1->setBrush(QBrush(QColor(150, 50, 50)));
    arrow1->setPen(QPen(QColor(150, 50, 50), 1));
    arrow1->attach(plot);

    // Arrow from hexagon down to ellipse
    auto* arrow2 = new QwtPlotShapeItem("Arrow 2");
    {
        QPainterPath arrowPath;
        double x1 = 7.0, y1 = 3.8, x2 = 6.0, y2 = 2.6;
        double headLen = 0.35, headW = 0.18;
        double dx = x2 - x1, dy = y2 - y1;
        double len = std::sqrt(dx * dx + dy * dy);
        double ux = dx / len, uy = dy / len;
        double px = -uy, py = ux;
        arrowPath.moveTo(x1, y1);
        arrowPath.lineTo(x2 - ux * headLen, y2 - uy * headLen);
        double hx = x2 - ux * headLen, hy = y2 - uy * headLen;
        arrowPath.lineTo(hx + px * headW, hy + py * headW);
        arrowPath.lineTo(x2, y2);
        arrowPath.lineTo(hx - px * headW, hy - py * headW);
        arrowPath.lineTo(hx, hy);
        arrowPath.closeSubpath();
        arrow2->setShape(arrowPath);
    }
    arrow2->setBrush(QBrush(QColor(50, 100, 150)));
    arrow2->setPen(QPen(QColor(50, 100, 150), 1));
    arrow2->attach(plot);

    // Text label
    auto* textLabel = new QwtPlotTextLabel();
    QwtText note("Shapes, arrows and text\ncan be placed on the canvas");
    note.setRenderFlags(Qt::AlignLeft | Qt::AlignTop);
    QFont noteFont;
    noteFont.setItalic(true);
    noteFont.setPointSize(9);
    note.setFont(noteFont);
    note.setColor(QColor(100, 100, 100));
    textLabel->setText(note);
    textLabel->setMargin(5);
    textLabel->attach(plot);

    plot->setAxisScale(QwtAxis::XBottom, 0, 10);
    plot->setAxisScale(QwtAxis::YLeft, 0, 8);
    plot->setAxisTitle(QwtAxis::XBottom, "X");
    plot->setAxisTitle(QwtAxis::YLeft, "Y");
    plot->insertLegend(new QwtLegend(), QwtPlot::BottomLegend);

    return plot;
}

// ---------------------------------------------------------------------------
// ⑩ Bode plot (log scale + dual Y axis + AxisZoomer + crosshair picker)
// ---------------------------------------------------------------------------

static QwtPlot* createBodePlot()
{
    auto* plot = new QwtPlot();
    plot->setTitle("Bode Plot (Log Scale)");
    plot->setAutoReplot(false);
    setupGrid(plot);

    // Log X axis
    plot->setAxisScaleEngine(QwtAxis::XBottom, new QwtLogScaleEngine());
    plot->setAxisMaxMajor(QwtAxis::XBottom, 6);
    plot->setAxisMaxMinor(QwtAxis::XBottom, 9);
    plot->setAxisScale(QwtAxis::XBottom, 0.1, 1000.0);

    // Dual Y axes
    plot->setAxisScale(QwtAxis::YLeft, -60, 10);
    plot->setAxisTitle(QwtAxis::YLeft, "Magnitude (dB)");
    plot->setAxisVisible(QwtAxis::YRight, true);
    plot->setAxisScale(QwtAxis::YRight, -100, 10);
    plot->setAxisTitle(QwtAxis::YRight, "Phase (deg)");
    plot->setAxisTitle(QwtAxis::XBottom, "Frequency (rad/s)");

    // Generate data: H(s) = 1/(1 + s/10)
    const int n = 200;
    QVector<double> freq(n), mag(n), phase(n);
    double lxMin = std::log(0.1), lxMax = std::log(1000.0);
    double lStep = (lxMax - lxMin) / (n - 1);
    for (int i = 0; i < n; ++i) {
        double w = std::exp(lxMin + i * lStep);
        freq[i] = w;
        mag[i] = -10.0 * std::log10(1.0 + w * w / 100.0);
        phase[i] = -std::atan(w / 10.0) * 180.0 / M_PI;
    }

    // Magnitude curve → YLeft
    auto* magCurve = new QwtPlotCurve("Magnitude");
    magCurve->setSamples(freq.data(), mag.data(), n);
    magCurve->setPen(QPen(QColor(0, 100, 200), 2));
    magCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    magCurve->setYAxis(QwtAxis::YLeft);
    magCurve->setLegendAttribute(QwtPlotCurve::LegendShowLine);
    magCurve->attach(plot);

    // Phase curve → YRight
    auto* phaseCurve = new QwtPlotCurve("Phase");
    phaseCurve->setSamples(freq.data(), phase.data(), n);
    phaseCurve->setPen(QPen(QColor(200, 50, 50), 2));
    phaseCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    phaseCurve->setYAxis(QwtAxis::YRight);
    phaseCurve->setLegendAttribute(QwtPlotCurve::LegendShowLine);
    phaseCurve->attach(plot);

    // Cutoff frequency marker
    auto* cutoffMarker = new QwtPlotMarker();
    cutoffMarker->setLineStyle(QwtPlotMarker::VLine);
    cutoffMarker->setLinePen(QPen(QColor(150, 150, 150), 1, Qt::DashDotLine));
    cutoffMarker->setValue(10.0, 0);
    QwtText cutoffLabel("fc = 10 rad/s");
    cutoffLabel.setColor(QColor(100, 100, 100));
    cutoffLabel.setFont(QFont("Arial", 8));
    cutoffMarker->setLabel(cutoffLabel);
    cutoffMarker->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
    cutoffMarker->attach(plot);

    // -3dB reference line
    auto* dbMarker = new QwtPlotMarker();
    dbMarker->setLineStyle(QwtPlotMarker::HLine);
    dbMarker->setLinePen(QPen(QColor(200, 100, 100), 1, Qt::DashLine));
    dbMarker->setValue(0, -3.0);
    dbMarker->setYAxis(QwtAxis::YLeft);
    QwtText dbLabel("-3 dB");
    dbLabel.setColor(QColor(200, 100, 100));
    dbLabel.setFont(QFont("Arial", 8));
    dbMarker->setLabel(dbLabel);
    dbMarker->setLabelAlignment(Qt::AlignRight | Qt::AlignBottom);
    dbMarker->attach(plot);

    plot->insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    plot->setAutoReplot(true);

    // Axis zoomer for (XBottom, YLeft)
    auto* zoomer = new QwtPlotAxisZoomer(QwtAxis::XBottom, QwtAxis::YLeft,
                                         plot->canvas());
    zoomer->setRubberBand(QwtPicker::RectRubberBand);
    zoomer->setTrackerMode(QwtPicker::ActiveOnly);

    // Crosshair picker for coordinate tracking
    auto* picker = new QwtPlotPicker(
        QwtAxis::XBottom, QwtAxis::YLeft,
        QwtPicker::CrossRubberBand, QwtPicker::ActiveOnly,
        plot->canvas());

    return plot;
}

// ---------------------------------------------------------------------------
// ⑪ Polar plot (QwtPolarPlot — added via addWidget)
// ---------------------------------------------------------------------------

class SpiralData : public QwtSeriesData<QwtPointPolar>
{
public:
    explicit SpiralData(int count) : m_count(count) {}

    size_t size() const override { return m_count; }

    QwtPointPolar sample(size_t i) const override
    {
        double theta = i * 2.0 * M_PI / m_count;
        double r = 5.0 * theta / (2.0 * M_PI);
        return QwtPointPolar(theta, r);
    }

    QRectF boundingRect() const override
    {
        if (m_cachedRect.isNull()) {
            double maxR = 5.0;
            m_cachedRect = QRectF(-maxR, -maxR, 2 * maxR, 2 * maxR);
        }
        return m_cachedRect;
    }

private:
    int m_count;
    mutable QRectF m_cachedRect;
};

class RoseData : public QwtSeriesData<QwtPointPolar>
{
public:
    explicit RoseData(int count, int petals = 4)
        : m_count(count), m_petals(petals)
    {
    }

    size_t size() const override { return m_count; }

    QwtPointPolar sample(size_t i) const override
    {
        double theta = i * 2.0 * M_PI / m_count;
        double r = 8.0 * std::abs(std::sin(m_petals * theta));
        return QwtPointPolar(theta, r);
    }

    QRectF boundingRect() const override
    {
        if (m_cachedRect.isNull()) {
            double maxR = 8.0;
            m_cachedRect = QRectF(-maxR, -maxR, 2 * maxR, 2 * maxR);
        }
        return m_cachedRect;
    }

private:
    int m_count;
    int m_petals;
    mutable QRectF m_cachedRect;
};

static QwtPolarPlot* createPolarPlot()
{
    auto* polarPlot = new QwtPolarPlot();
    polarPlot->setTitle("Polar Plot");
    polarPlot->setPlotBackground(QColor(245, 245, 250));

    polarPlot->setScale(QwtPolar::Azimuth, 0, 360, 30);
    polarPlot->setScale(QwtPolar::Radius, 0, 10);
    polarPlot->setScaleMaxMinor(QwtPolar::Azimuth, 2);

    // Rose curve
    auto* rose = new QwtPolarCurve("Rose (4 petals)");
    rose->setData(new RoseData(360, 4));
    rose->setStyle(QwtPolarCurve::Lines);
    rose->setPen(QPen(QColor(220, 50, 50), 2));
    rose->setRenderHint(QwtPolarItem::RenderAntialiased, true);
    rose->setLegendAttribute(QwtPolarCurve::LegendShowLine);
    rose->attach(polarPlot);

    // Spiral curve
    auto* spiral = new QwtPolarCurve("Spiral");
    spiral->setData(new SpiralData(500));
    spiral->setStyle(QwtPolarCurve::Lines);
    spiral->setPen(QPen(QColor(50, 100, 200), 1.5));
    spiral->setRenderHint(QwtPolarItem::RenderAntialiased, true);
    spiral->setLegendAttribute(QwtPolarCurve::LegendShowLine);
    spiral->attach(polarPlot);

    // Grid
    auto* grid = new QwtPolarGrid();
    grid->showGrid(QwtPolar::Azimuth, true);
    grid->showGrid(QwtPolar::Radius, true);
    grid->showMinorGrid(QwtPolar::Azimuth, true);
    grid->showMinorGrid(QwtPolar::Radius, true);
    grid->setMajorGridPen(QPen(QColor(180, 180, 180), 0.5));
    grid->setMinorGridPen(QPen(QColor(220, 220, 220), 0.3));
    grid->showAxis(QwtPolar::AxisAzimuth, true);
    grid->showAxis(QwtPolar::AxisLeft, false);
    grid->showAxis(QwtPolar::AxisRight, false);
    grid->showAxis(QwtPolar::AxisTop, false);
    grid->showAxis(QwtPolar::AxisBottom, false);
    grid->attach(polarPlot);

    polarPlot->insertLegend(new QwtLegend(), QwtPolarPlot::BottomLegend);

    // Magnifier
    new QwtPolarMagnifier(polarPlot->canvas());

    return polarPlot;
}

// ---------------------------------------------------------------------------
// Main: assemble QwtFigure with 4×3 grid layout
// ---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Qwt Static Example — Comprehensive Feature Showcase");
    mainWindow.resize(1600, 1200);

    auto* centralWidget = new QWidget(&mainWindow);
    auto* mainLayout = new QVBoxLayout(centralWidget);

    // QwtFigure container
    auto* figure = new QwtFigure(centralWidget);
    figure->setSizeInches(16, 12);
    figure->setFaceColor(QColor(240, 240, 240));

    // Row 0: Curves, BarChart, Spectrogram
    figure->addGridAxes(createCurvePlot(), 4, 3, 0, 0);
    figure->addGridAxes(createBarChartPlot(), 4, 3, 0, 1);
    figure->addGridAxes(createSpectrogramPlot(), 4, 3, 0, 2);

    // Row 1: BoxChart, Trading, VectorField
    figure->addGridAxes(createBoxChartPlot(), 4, 3, 1, 0);
    figure->addGridAxes(createTradingPlot(), 4, 3, 1, 1);
    figure->addGridAxes(createVectorFieldPlot(), 4, 3, 1, 2);

    // Row 2: Scatter, Interval+Histogram, Shapes+Arrows
    figure->addGridAxes(createScatterPlot(), 4, 3, 2, 0);
    figure->addGridAxes(createIntervalHistPlot(), 4, 3, 2, 1);
    figure->addGridAxes(createShapePlot(), 4, 3, 2, 2);

    // Row 3: Bode (span 2 cols) + Polar (via addWidget)
    figure->addGridAxes(createBodePlot(), 4, 3, 3, 0, 1, 2);
    figure->addWidget(createPolarPlot(), 4, 3, 3, 2);

    // Control buttons
    auto* buttonLayout = new QHBoxLayout();

    auto* saveButton = new QPushButton("Save Figure (300 DPI)");
    QObject::connect(saveButton, &QPushButton::clicked, [figure, &mainWindow]() {
        QString path = "qwt_static_example.png";
        if (figure->saveFig(path, 300))
            mainWindow.statusBar()->showMessage("Saved to " + path, 3000);
        else
            mainWindow.statusBar()->showMessage("Save failed!", 3000);
    });

    auto* clearButton = new QPushButton("Clear All");
    QObject::connect(clearButton, &QPushButton::clicked, [figure, &mainWindow]() {
        figure->clear();
        mainWindow.statusBar()->showMessage("All plots cleared", 3000);
    });

    auto* replotButton = new QPushButton("Replot All");
    QObject::connect(replotButton, &QPushButton::clicked, [figure, &mainWindow]() {
        figure->replotAll();
        mainWindow.statusBar()->showMessage("All plots replotted", 3000);
    });

    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(replotButton);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addStretch();

    mainLayout->addWidget(figure);
    mainLayout->addLayout(buttonLayout);

    mainWindow.setCentralWidget(centralWidget);
    mainWindow.statusBar()->showMessage(
        "Ready — Hover over Plot 1 for data capture, drag to zoom on Plot 10, "
        "right-click drag to pan on Plots 3/5, scroll to zoom on Plot 7",
        8000);
    mainWindow.show();

    QwtExampleScreenshot::start( &mainWindow );

    return app.exec();
}
