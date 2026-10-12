# Stock K-Line Chart (Candlestick Chart) - QwtPlotTradingCurve

`QwtPlotTradingCurve` is a specialized plot item class for drawing stock/financial K-line charts (candlestick charts). It supports displaying trading data such as open, close, high, and low prices, and is commonly used for financial data visualization and technical analysis.

## Key Features

**Features**

- Standard K-line styles: Supports candlestick, American bar, and other styles
- Up/down color differentiation: Automatically displays up/down colors based on open/close prices
- Auto-scaling range: Automatically calculates appropriate display range based on data
- Symbol style customization: Customizable up/down bar styles

## Basic Concepts

### K-Line Composition

Each K-line contains four price data points:

```text
          High
            │
     ┌──────┴──────┐
     │             │
     │    ┌────┐   │  ← Body (open to close)
     │    │    │   │
     │    └────┘   │
     │             │
     └─────────────┘
            │
          Low
```

### K-Line Styles

| Style | Enum Value | Description |
|-------|-----------|-------------|
| Candlestick | `CandleStick` | Displays complete body with upper and lower shadows |
| American bar | `Bar` | Shows lines only, no body drawn |

### Data Structure

Uses `QwtOHLCSample` (declared in `qwt_samples.h`, also reachable via `#include <QwtOHLCSample>`) to represent each K-line's data. Note that the constructor takes the prices in **time, open, high, low, close** order:

```cpp
class QwtOHLCSample {
public:
    QwtOHLCSample(double time = 0.0, double open = 0.0,
                  double high = 0.0, double low = 0.0, double close = 0.0);

    QwtInterval boundingInterval() const;  // Interval spanned by the prices
    bool isValid() const;                  // low <= open/close <= high

    double time;      // Time point (X-axis position)
    double open;      // Open price
    double high;      // High price
    double low;       // Low price
    double close;     // Close price
};
```

## Usage

The stock K-line chart example is located at: `examples/2D/stockchart`. Screenshot below:

![Stock Chart](../../assets/screenshots/stockchart.png)

### 1. Basic K-Line Chart

A trading curve is configured through `setSymbolStyle()` (`Bar` / `CandleStick`, default `CandleStick`): `setSymbolBrush()` assigns the body fill per price `Direction` (`Increasing` / `Decreasing`), and `setSymbolExtent()` sets the body width in scale units. Data is fed to `setSamples()` as `QwtOHLCSample` values (time, open, high, low, close). The example below renders four daily candles on a plot:

```cpp
#include <QwtPlot>
#include <QwtPlotTradingCurve>

QwtPlot* plot = new QwtPlot();
plot->setTitle("Stock K-Line Chart");
plot->setCanvasBackground(Qt::white);

// Create K-line chart
QwtPlotTradingCurve* curve = new QwtPlotTradingCurve("Daily K-Line");

// Set symbol style
curve->setSymbolStyle(QwtPlotTradingCurve::CandleStick);

// Set up/down colors
curve->setSymbolBrush(QwtPlotTradingCurve::Increasing,
    QBrush(Qt::red));   // Up (close > open): red
curve->setSymbolBrush(QwtPlotTradingCurve::Decreasing,
    QBrush(Qt::green)); // Down (close <= open): green

// Set body width
curve->setSymbolExtent(0.8);  // 0.8 scale (x-axis) units wide

// Prepare data: QwtOHLCSample(time, open, high, low, close)
QVector<QwtOHLCSample> samples;
samples << QwtOHLCSample(1, 10.0, 11.0, 9.5, 10.5);   // Up
samples << QwtOHLCSample(2, 10.5, 10.8, 10.0, 10.2);  // Down
samples << QwtOHLCSample(3, 10.2, 11.2, 10.1, 10.8);  // Up
samples << QwtOHLCSample(4, 10.8, 11.0, 10.4, 10.6);  // Down

curve->setSamples(samples);
curve->attach(plot);

// Set axes
plot->setAxisScale(QwtAxis::YLeft, 9, 12);
plot->replot();
```

### 2. K-Line Style Configuration

#### Candlestick Style

In `CandleStick` mode the box between open and close is filled according to the price direction: `setSymbolBrush()` takes a `Direction` (`Increasing` when the close is above the open, `Decreasing` otherwise) plus a brush. The outline pen is set globally via `setSymbolPen()` — there is no per-direction pen overload — and a flat sample (open == close) is painted with the `Decreasing` brush, as there is no separate neutral state. The snippet below assigns distinct fills for up and down days:

```cpp
curve->setSymbolStyle(QwtPlotTradingCurve::CandleStick);

// Set up/down fills (one brush per direction)
curve->setSymbolBrush(QwtPlotTradingCurve::Increasing, QBrush(Qt::red));    // Up
curve->setSymbolBrush(QwtPlotTradingCurve::Decreasing, QBrush(Qt::green));  // Down

// The outline pen is shared by both directions
curve->setSymbolPen(QPen(Qt::black, 1));
```

#### American Bar Style

`Bar` style draws only the vertical line spanning low to high, with small horizontal ticks marking the open (left) and close (right) prices — no filled body. Switch to it with `setSymbolStyle(QwtPlotTradingCurve::Bar)`:

```cpp
curve->setSymbolStyle(QwtPlotTradingCurve::Bar);

// American bar shows lines only, no body drawn
// Line extends from low to high, with small horizontal marks indicating open and close prices
```

### 3. Body Width Settings

Body width is governed by three methods: `setSymbolExtent()` defines the width in scale (coordinate) units, while `setMinSymbolWidth()` and `setMaxSymbolWidth()` clamp the rendered width in pixels. The snippet below combines all three:

```cpp
// Set body width (in relative coordinate units)
curve->setSymbolExtent(0.5);  // Body width is 0.5 coordinate units

// Minimum width (in pixels)
curve->setMinSymbolWidth(3);  // Minimum 3 pixels wide

// Maximum width (in pixels)
curve->setMaxSymbolWidth(20); // Maximum 20 pixels wide
```

### 4. Data Settings

`setSamples()` is overloaded to accept either a `QVector<QwtOHLCSample>` — each sample storing time, open, high, low, and close — or a `QwtSeriesData<QwtOHLCSample>*` for custom data sources. Both forms are shown below:

```cpp
// Using a QVector of QwtOHLCSample
QVector<QwtOHLCSample> samples;
for (int i = 0; i < count; i++) {
    samples << QwtOHLCSample(
        time[i], open[i], high[i], low[i], close[i]
    );
}
curve->setSamples(samples);

// Using QwtSeriesData (QwtSeriesData<QwtOHLCSample>*)
curve->setSamples(seriesData);
```

### 5. Combining with Volume Chart

Volume is typically overlaid as a `QwtPlotBarChart` attached to the same plot: its `setSamples()` accepts a `QVector<double>` of per-bar values and `setBrush()` styles the columns. The example below adds a gray volume series beneath the candles:

```cpp
// Create volume bar chart
QwtPlotBarChart* volumeChart = new QwtPlotBarChart("Volume");

QVector<double> volumes;
volumes << 1000 << 1500 << 800 << 2000;
volumeChart->setSamples(volumes);
volumeChart->setBrush(QBrush(Qt::gray));
volumeChart->attach(plot);
```

### 6. Adding Moving Averages

Moving averages are ordinary `QwtPlotCurve` items layered on top of the candles, each with its own pen and sample array. The example below overlays 5-day and 10-day moving averages:

```cpp
#include <QwtPlotCurve>

// Add 5-day moving average
QwtPlotCurve* ma5 = new QwtPlotCurve("MA5");
ma5->setPen(QPen(Qt::blue, 1));
ma5->setSamples(ma5Data);
ma5->attach(plot);

// Add 10-day moving average
QwtPlotCurve* ma10 = new QwtPlotCurve("MA10");
ma10->setPen(QPen(Qt::yellow, 1));
ma10->setSamples(ma10Data);
ma10->attach(plot);
```

## Core Methods Summary

| Method | Description |
|--------|-------------|
| `setSamples()` | Set K-line data (`QVector<QwtOHLCSample>` or `QwtSeriesData<QwtOHLCSample>*`) |
| `setSymbolStyle()` | Set K-line symbol style (`Bar` / `CandleStick`) |
| `setSymbolBrush()` | Set the fill brush for one direction (`Increasing` / `Decreasing`) |
| `setSymbolPen()` | Set the symbol outline pen (one global pen, shared by both directions) |
| `setSymbolExtent()` | Set body width in scale units |
| `setMinSymbolWidth()` | Set minimum width (pixels) |
| `setMaxSymbolWidth()` | Set maximum width (pixels) |

## Up/Down State Determination

| State | Enum Value | Condition |
|-------|-----------|-----------|
| Up | `Increasing` | close > open |
| Down | `Decreasing` | close <= open (a flat close == open uses the `Decreasing` brush) |

> **Tip — Color Conventions**
>
> - Chinese market: Red for up, green for down
> - International market: Green for up, red for down
> - Colors can be adjusted as needed

> **Example — Related Examples**
>
> - Stock chart demo: `examples/2D/stockchart`