# Interval Curve - QwtPlotIntervalCurve

`QwtPlotIntervalCurve` is used to draw interval curves with upper and lower bounds, commonly used to display error ranges, confidence intervals, fluctuation ranges, and similar data. Each data point contains a position value and an interval range.

## Key Features

**Features**

- Interval representation: Displays upper and lower bounds around the curve
- Multiple styles: Supports curve, filled area, symbol, and other display modes
- Error bar display: Can draw vertical or horizontal error bar lines
- Style customization: Interval boundaries and fill can be configured independently

## Basic Concepts

### Interval Curve Composition

```text
     Upper bound
       ───────
      /      \
     /  Curve \     ← Median curve
    /          \
   ────────────
     Lower bound

Interval curve shows: median curve + upper/lower bounds
```

### Data Structure

Uses `QwtIntervalSample` to represent interval data:

```cpp
struct QwtIntervalSample {
    double value;      // Median value (curve position)
    QwtInterval interval; // Interval range [min, max]
};
```

### Curve Styles

| Style | Enum Value | Description |
|-------|-----------|-------------|
| No curve | `NoCurve` | Show interval fill or symbols only |
| Curve | `Tube` | Draw interval filled area |
| Error bars | `ErrorBars` | Draw error bar lines |

## Usage

### 1. Basic Interval Curve

```cpp
#include <QwtPlot>
#include <QwtPlotIntervalCurve>
#include <QwtInterval>

QwtPlot* plot = new QwtPlot();
plot->setTitle("Interval Curve Example");
plot->setCanvasBackground(Qt::white);

// Create interval curve
QwtPlotIntervalCurve* curve = new QwtPlotIntervalCurve("Error Range");

// Set style
curve->setStyle(QwtPlotIntervalCurve::Tube);

// Set median curve style
curve->setPen(QPen(Qt::blue, 2));

// Set interval fill style
curve->setBrush(QBrush(QColor(100, 150, 200, 100)));

// Prepare data
QVector<QwtIntervalSample> samples;
samples << QwtIntervalSample(5.0, QwtInterval(4.0, 6.0));
samples << QwtIntervalSample(8.0, QwtInterval(6.5, 9.5));
samples << QwtIntervalSample(10.0, QwtInterval(8.0, 12.0));
samples << QwtIntervalSample(12.0, QwtInterval(10.0, 14.0));

curve->setSamples(samples);
curve->attach(plot);
plot->replot();
```

### 2. Error Bar Style

```cpp
curve->setStyle(QwtPlotIntervalCurve::ErrorBars);

// Set error bar line style
curve->setPen(QPen(Qt::black, 1));

// Set error bar symbol (optional)
QwtIntervalSymbol* symbol = new QwtIntervalSymbol(QwtIntervalSymbol::Bar);
symbol->setPen(QPen(Qt::black, 1));
curve->setSymbol(symbol);

// Set error bar width
curve->setWidth(5);  // Error bar cross-line width (pixels)
```

### 3. Tube Style Configuration

```cpp
curve->setStyle(QwtPlotIntervalCurve::Tube);

// Median curve style
curve->setPen(QPen(Qt::red, 2));

// Fill style
curve->setBrush(QBrush(QColor(255, 200, 200, 150)));

// Boundary line style
curve->setOutlinePen(QPen(Qt::darkRed, 1));
curve->setOutlineMode(QwtPlotIntervalCurve::OutlineBoth);  // Draw both upper and lower bounds
```

### 4. Symbol Configuration

```cpp
#include <QwtIntervalSymbol>

// Create interval symbol
QwtIntervalSymbol* symbol = new QwtIntervalSymbol(QwtIntervalSymbol::Bar);

// Set style
symbol->setPen(QPen(Qt::blue, 1));
symbol->setWidth(8);  // Symbol width (pixels)

curve->setSymbol(symbol);

// Symbol types:
// - Bar: Horizontal line (standard error bar)
// - Box: Rectangle
// - Diamond: Diamond shape
```

## Core Methods Summary

| Method | Description |
|--------|-------------|
| `setSamples()` | Set interval data |
| `setStyle()` | Set curve style |
| `setPen()` | Set median curve pen |
| `setBrush()` | Set interval fill brush |
| `setSymbol()` | Set interval symbol |
| `setOutlinePen()` | Set boundary line pen |
| `setOutlineMode()` | Set boundary display mode |

> **Tip — Application Scenarios**
>
> - Experimental data error display
> - Confidence interval visualization
> - Fluctuation range display
> - Prediction interval display

> **Example — Related Examples**
>
> - Interval curve demo: See custom examples