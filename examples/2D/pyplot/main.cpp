/*****************************************************************************
 * Qwt Examples - QwtPyPlot Demo
 * Demonstrates the matplotlib-like pyplot API for Qwt
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include <QApplication>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>

#include <QwtFigure>
#include <QwtPyPlot>
#include <QwtPlot>

#include <cmath>
#include <QVector>
#include <QPointF>
#include "../../common/qwt_example_screenshot.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @brief Create a simple single-plot demo
 */
QWidget* createSimplePlotDemo()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    
    auto* plot = new QwtPlot;
    QwtPyPlot plt(plot);
    
    // Generate sine wave data
    QVector<double> x, y;
    for (int i = 0; i <= 100; i++) {
        double t = i * 0.1;
        x.append(t);
        y.append(std::sin(t));
    }
    
    // Plot with format string: red line with circle markers
    plt.plot(x, y, "r-o", "sin(x)");
    plt.setTitle("Simple Sine Wave");
    plt.setXLabel("Time (s)");
    plt.setYLabel("Amplitude");
    plt.grid(true);
    plt.legend();
    
    layout->addWidget(plot);
    return widget;
}

/**
 * @brief Create a multi-subplot demo using QwtFigure
 */
QWidget* createSubplotDemo()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    
    auto* figure = new QwtFigure;
    QwtPyPlot plt(figure);
    
    // First subplot: multiple curves
    plt.subplot(2, 1, 1);
    QVector<double> x;
    for (int i = 0; i <= 50; i++) {
        x.append(i * 0.2);
    }
    
    QVector<double> y1, y2, y3;
    for (double t : x) {
        y1.append(std::sin(t));
        y2.append(std::cos(t));
        y3.append(std::sin(t) * std::cos(t));
    }
    
    plt.plot(x, y1, "b-", "sin(x)");
    plt.plot(x, y2, "g--", "cos(x)");
    plt.plot(x, y3, "r:", "sin(x)*cos(x)");
    plt.setTitle("Trigonometric Functions");
    plt.grid(true);
    plt.legend();
    
    // Second subplot: bar chart
    plt.subplot(2, 1, 2);
    QVector<double> values = {23, 45, 12, 67, 34, 89, 56};
    plt.bar(values, "c", "Sales Data");
    plt.setTitle("Bar Chart Demo");
    plt.setYLabel("Value");
    
    layout->addWidget(figure);
    return widget;
}

/**
 * @brief Create a scatter plot demo
 */
QWidget* createScatterDemo()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    
    auto* plot = new QwtPlot;
    QwtPyPlot plt(plot);
    
    // Generate random-ish scatter data
    QVector<double> x1, y1, x2, y2;
    for (int i = 0; i < 50; i++) {
        x1.append(i * 0.5 + (i % 7) * 0.3);
        y1.append(std::sin(i * 0.3) * 10 + (i % 5));
        
        x2.append(i * 0.4 + (i % 5) * 0.4);
        y2.append(std::cos(i * 0.3) * 8 + (i % 3));
    }
    
    plt.scatter(x1, y1, 30, "r", "Group A");
    plt.scatter(x2, y2, 40, "b", "Group B");
    plt.setTitle("Scatter Plot Demo");
    plt.setXLabel("X Coordinate");
    plt.setYLabel("Y Coordinate");
    plt.legend();
    plt.grid(true, true);  // major + minor grid
    
    layout->addWidget(plot);
    return widget;
}

/**
 * @brief Create a histogram demo
 */
QWidget* createHistogramDemo()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    
    auto* plot = new QwtPlot;
    QwtPyPlot plt(plot);
    
    // Generate normal-distribution-like data
    QVector<double> data;
    for (int i = 0; i < 500; i++) {
        // Simple pseudo-normal using sum of uniform randoms
        double val = 0;
        for (int j = 0; j < 12; j++) {
            val += (i * 7 + j * 13) % 100 / 100.0;
        }
        val = (val - 6.0) * 10 + 50;  // shift to mean=50, std~10
        data.append(val);
    }
    
    plt.hist(data, 20, "m", "Distribution");
    plt.setTitle("Histogram Demo (20 bins)");
    plt.setXLabel("Value");
    plt.setYLabel("Frequency");
    plt.grid(true);
    
    layout->addWidget(plot);
    return widget;
}

/**
 * @brief Create a heatmap (imshow) demo
 */
QWidget* createHeatmapDemo()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    
    auto* plot = new QwtPlot;
    QwtPyPlot plt(plot);
    
    // Generate 2D Gaussian data
    QVector<QVector<double>> data;
    int rows = 50, cols = 50;
    for (int i = 0; i < rows; i++) {
        QVector<double> row;
        for (int j = 0; j < cols; j++) {
            double x = (j - cols / 2.0) / 10.0;
            double y = (i - rows / 2.0) / 10.0;
            double val = std::exp(-(x * x + y * y));
            row.append(val);
        }
        data.append(row);
    }
    
    plt.imshow(data, "viridis");
    plt.setTitle("Heatmap Demo (2D Gaussian)");
    
    layout->addWidget(plot);
    return widget;
}

/**
 * @brief Create a fill-between demo
 */
QWidget* createFillBetweenDemo()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    
    auto* plot = new QwtPlot;
    QwtPyPlot plt(plot);
    
    QVector<double> x, y_upper, y_lower, y_mid;
    for (int i = 0; i <= 100; i++) {
        double t = i * 0.1;
        x.append(t);
        y_mid.append(std::sin(t));
        y_upper.append(std::sin(t) + 0.3);
        y_lower.append(std::sin(t) - 0.3);
    }
    
    // Fill the confidence band
    plt.fillBetween(x, y_lower, y_upper, "blue", 0.3);
    
    // Plot the center line
    plt.plot(x, y_mid, "b-", "Mean ± Std");
    
    plt.setTitle("Fill Between Demo (Confidence Band)");
    plt.setXLabel("Time");
    plt.setYLabel("Value");
    plt.legend();
    plt.grid(true);
    
    layout->addWidget(plot);
    return widget;
}

/**
 * @brief Create a twin-axis demo
 */
QWidget* createTwinAxisDemo()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    
    auto* figure = new QwtFigure;
    QwtPyPlot plt(figure);
    
    // Create main plot
    auto* ax1 = plt.subplot(1, 1, 1);
    
    QVector<double> x;
    for (int i = 0; i <= 100; i++) {
        x.append(i * 0.1);
    }
    
    // Temperature on left Y-axis
    QVector<double> temp;
    for (double t : x) {
        temp.append(20 + 5 * std::sin(t * 0.5));
    }
    plt.plot(x, temp, "r-", "Temperature (°C)");
    plt.setYLabel("Temperature (°C)");
    plt.setYLim(10, 30);
    
    // Create twin axis for humidity on right Y-axis
    auto* ax2 = plt.twinx();
    plt.sca(ax2);
    
    QVector<double> humidity;
    for (double t : x) {
        humidity.append(50 + 20 * std::cos(t * 0.3));
    }
    plt.plot(x, humidity, "b--", "Humidity (%)");
    plt.setYLabel("Humidity (%)");
    plt.setYLim(20, 80);
    
    plt.sca(ax1);
    plt.setTitle("Twin Axis Demo (Temperature & Humidity)");
    plt.grid(true);
    plt.legend();
    
    layout->addWidget(figure);
    return widget;
}

/**
 * @brief Create annotation demo
 */
QWidget* createAnnotationDemo()
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    
    auto* plot = new QwtPlot;
    QwtPyPlot plt(plot);
    
    QVector<double> x, y;
    for (int i = 0; i <= 100; i++) {
        double t = i * 0.1;
        x.append(t);
        y.append(std::sin(t) * std::exp(-t * 0.1));
    }
    
    plt.plot(x, y, "g-", "Damped Sine");
    
    // Add reference lines
    plt.axhline(0, "k--");
    plt.axvline(5, "r:");
    
    // Add highlighted zone
    plt.axvspan(3, 7, "yellow", 0.2);
    
    plt.setTitle("Annotation Demo");
    plt.setXLabel("Time (s)");
    plt.setYLabel("Amplitude");
    plt.legend();
    plt.grid(true);
    
    layout->addWidget(plot);
    return widget;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    
    QTabWidget tabWidget;
    tabWidget.setWindowTitle("QwtPyPlot Demo - Matplotlib-like API for Qwt");
    tabWidget.resize(900, 600);
    
    // Add all demo tabs
    tabWidget.addTab(createSimplePlotDemo(), "Simple Plot");
    tabWidget.addTab(createSubplotDemo(), "Subplots");
    tabWidget.addTab(createScatterDemo(), "Scatter");
    tabWidget.addTab(createHistogramDemo(), "Histogram");
    tabWidget.addTab(createHeatmapDemo(), "Heatmap");
    tabWidget.addTab(createFillBetweenDemo(), "Fill Between");
    tabWidget.addTab(createTwinAxisDemo(), "Twin Axis");
    tabWidget.addTab(createAnnotationDemo(), "Annotations");
    
    tabWidget.show();

    QwtExampleScreenshot::start( &tabWidget );
    
    return app.exec();
}
