# Histogram - QwtPlotHistogram

`QwtPlotHistogram` is used to draw histograms, representing a series of intervals associated with values. Each interval corresponds to a numeric value, commonly used in statistics to display data distribution frequency.

## Key Features

- Multiple display styles: Supports outline, column, and line drawing modes
- Interval data structure: Uses interval+value data format, suitable for statistical distributions
- Symbol customization: Configurable display style for each interval column
- Baseline configuration: Supports custom baseline position

## Basic Concepts

### Difference Between Histogram and Bar Chart

| Feature | Bar Chart | Histogram |
|---------|-----------|-----------|
| Data structure | Point values (x, y) | Interval values ([x1, x2], y) |
| Bar width | Fixed or adjustable | Determined by interval |
| Use case | Categorical data comparison | Continuous data distribution |
| X-axis meaning | Category labels | Numeric intervals |

### Histogram Styles

| Style | Enum Value | Description |
|-------|-----------|-------------|
| Outline | `Outline` | Draw overall outline with fill |
| Columns | `Columns` | Draw independent column for each interval (default) |
| Lines | `Lines` | Draw horizontal line for each interval |

### Data Structure

Histograms use `QwtIntervalSample` to represent data:

```cpp
struct QwtIntervalSample {
    double value;      // Value corresponding to the interval (height)
    QwtInterval interval; // Interval range [min, max]
};
```

## Usage

### 1. Basic Histogram

The `QwtPlotHistogram` constructor takes a title string, and `setStyle()` selects the drawing mode (`Outline` / `Columns` / `Lines`, defaulting to `Columns`). Data is fed as a list of `QwtIntervalSample`, each pairing a `QwtInterval` range with a value; `setPen()` and `setBrush()` then control the column border and fill. The example below builds a plot, fills four bins, and attaches the histogram:

```cpp
#include <QwtPlot>
#include <QwtPlotHistogram>
#include <QwtInterval>

QwtPlot* plot = new QwtPlot();
plot->setTitle("Histogram Example");
plot->setCanvasBackground(Qt::white);

// Create histogram
QwtPlotHistogram* histogram = new QwtPlotHistogram("Data Distribution");

// Set style (default is Columns)
histogram->setStyle(QwtPlotHistogram::Columns);

// Prepare interval data
QVector<QwtIntervalSample> samples;
samples << QwtIntervalSample(10, QwtInterval(0, 10));    // [0,10] interval, value=10
samples << QwtIntervalSample(25, QwtInterval(10, 20));   // [10,20] interval, value=25
samples << QwtIntervalSample(15, QwtInterval(20, 30));   // [20,30] interval, value=15
samples << QwtIntervalSample(30, QwtInterval(30, 40));   // [30,40] interval, value=30
histogram->setSamples(samples);

// Set style
histogram->setPen(QPen(Qt::darkBlue, 1));
histogram->setBrush(QBrush(QColor(100, 150, 200)));

histogram->attach(plot);
plot->replot();
```

### 2. Setting Data

`setSamples()` is overloaded: it accepts either a `QVector<QwtIntervalSample>` of interval/value samples, or a `QwtSeriesData<QwtIntervalSample>*` for custom or dynamically generated data sources. The snippet below shows both forms:

```cpp
// Using QwtIntervalSample array
QVector<QwtIntervalSample> samples;
for (int i = 0; i < 10; i++) {
    double min = i * 10;
    double max = (i + 1) * 10;
    double value = rand() % 50 + 10;
    samples << QwtIntervalSample(value, QwtInterval(min, max));
}
histogram->setSamples(samples);

// Using QwtSeriesData
histogram->setSamples(seriesData);
```

### 3. Display Styles

#### Columns Style

This is the most commonly used style, with each interval displayed as an independent column:

```cpp
histogram->setStyle(QwtPlotHistogram::Columns);
histogram->setPen(QPen(Qt::blue, 1));
histogram->setBrush(QBrush(QColor(100, 150, 200, 150)));
```

#### Outline Style

Draws the overall outline, treating all intervals as a single entity:

```cpp
histogram->setStyle(QwtPlotHistogram::Outline);
histogram->setPen(QPen(Qt::darkBlue, 2));
histogram->setBrush(QBrush(Qt::lightGray));

// Note: Outline style requires intervals to be in ascending order and non-overlapping
```

> **Warning — Outline Style Requirements**
>
> Outline style requires the following conditions:
> 1. Intervals must be in ascending order
> 2. Intervals must not overlap
> 3. Preferably no gaps between intervals

#### Lines Style

Draws only the value line for each interval:

```cpp
histogram->setStyle(QwtPlotHistogram::Lines);
histogram->setPen(QPen(Qt::red, 2));
// Lines style does not use fill
```

### 4. Column Symbol Configuration

In `Columns` style, `setSymbol()` replaces the plain pen/brush rectangle with a `QwtColumnSymbol`. The symbol's constructor takes a `Style` (here `Box`), `setFrameStyle()` picks the frame look (`NoFrame` / `Plain` / `Raised`), and the symbol's own `setPen()` / `setBrush()` control its border and fill. The example below creates a box symbol with a vertical gradient fill and assigns it to the histogram:

```cpp
#include <QwtColumnSymbol>

// Create column symbol
QwtColumnSymbol* symbol = new QwtColumnSymbol(QwtColumnSymbol::Box);
symbol->setFrameStyle(QwtColumnSymbol::Plain);  // Flat border

// Set fill gradient
QLinearGradient gradient(0, 0, 0, 1);
gradient.setColorAt(0, Qt::blue);
gradient.setColorAt(1, Qt::darkBlue);
symbol->setBrush(QBrush(gradient));

// Set border
symbol->setPen(QPen(Qt::black, 1));

histogram->setSymbol(symbol);
```

### 5. Baseline Settings

`setBaseline()` defines the reference value from which each column is drawn; the default is 0.0. The snippet below resets it explicitly and shows how a negative baseline can be used to emphasize positive intervals:

```cpp
// Set baseline position
histogram->setBaseline(0.0);  // Default starts from 0

// Using a negative baseline can highlight positive value intervals
histogram->setBaseline(-5.0);
```

### 6. Statistical Histogram Example

Compute and draw a histogram from raw data:

```cpp
// Raw data
QVector<double> rawData;
for (int i = 0; i < 1000; i++) {
    rawData << (rand() % 100);  // Random numbers 0-99
}

// Compute histogram bins
int binCount = 10;
double minVal = 0, maxVal = 100;
double binWidth = (maxVal - minVal) / binCount;

QVector<QwtIntervalSample> samples(binCount);
for (int i = 0; i < binCount; i++) {
    double binMin = minVal + i * binWidth;
    double binMax = binMin + binWidth;

    // Count data points falling in this interval
    int count = 0;
    for (double val : rawData) {
        if (val >= binMin && val < binMax) {
            count++;
        }
    }

    samples[i] = QwtIntervalSample(count, QwtInterval(binMin, binMax));
}

histogram->setSamples(samples);
```

## Core Methods Summary

| Method | Description |
|--------|-------------|
| `setSamples()` | Set interval data |
| `setStyle()` | Set display style |
| `setPen()` | Set line pen |
| `setBrush()` | Set fill brush |
| `setSymbol()` | Set column symbol |
| `setBaseline()` | Set baseline position |
| `data()` | Get data object |
| `boundingRect()` | Get bounding rectangle |

## QwtInterval Class

`QwtInterval` represents a numeric interval:

```cpp
#include <QwtInterval>

// Create interval
QwtInterval interval(0, 100);  // [0, 100]

// Set border flags
interval.setBorderFlags(QwtInterval::IncludeMinimum |
                        QwtInterval::IncludeMaximum);  // [0, 100] inclusive

// Or exclude borders
interval.setBorderFlags(QwtInterval::ExcludeMinimum |
                        QwtInterval::ExcludeMaximum);  // (0, 100) exclusive

// Interval operations
double width = interval.width();       // Interval width
double min = interval.minValue();      // Minimum value
double max = interval.maxValue();      // Maximum value
bool contains = interval.contains(50); // Check if value is within interval
```

> **Tip — Histogram Application Scenarios**
>
> - Statistical data distribution frequency
> - Image brightness distribution analysis
> - Discretized display of continuous variables

> **Example — Related Examples**
>
> - Histogram demo: Created in conjunction with custom examples