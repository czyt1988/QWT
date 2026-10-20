# Interval Curve - QwtPlotIntervalCurve

`QwtPlotIntervalCurve` is used to draw interval curves with upper and lower bounds, commonly used to display error ranges, confidence intervals, fluctuation ranges, and similar data. Each data point contains a position value and an interval range.

## Key Features

- Interval representation: Displays upper and lower bounds around the curve
- Multiple styles: Supports curve, filled area, symbol, and other display modes
- Error bar display: Can draw vertical or horizontal error bar lines
- Style customization: Interval boundaries and fill can be configured independently

## Basic Concepts

### Interval Curve Composition

The diagram below shows a `Tube` interval curve: the upper and lower interval bounds form two boundary curves, and the area between them is filled:

```text
     Upper bound
       ───────
      /       \
     / Filled  \    ← Area filled with brush()
    /   area    \
   ────────────
     Lower bound

Boundary curves are stroked with pen(), the area between them filled with brush()
```

### Data Structure

Uses `QwtIntervalSample` to represent interval data, where a position value on one axis is paired with an interval on the other:

```cpp
class QwtIntervalSample {
public:
    QwtIntervalSample();
    QwtIntervalSample(double value, const QwtInterval& interval);
    QwtIntervalSample(double value, double min, double max);

    double value;           // Position value (curve position)
    QwtInterval interval;   // Interval range [min, max]
};
```

### Curve Styles

| Style | Enum Value | Description |
|-------|-----------|-------------|
| No curve | `NoCurve` | Don't draw a curve; symbols (e.g. error bars) are still displayed |
| Tube | `Tube` | Draw the upper/lower bounds as two curves with `pen()` and fill the area between them with `brush()` |
| User curve | `UserCurve` | Reserved for derived classes that overload `drawSeries()` |

## Usage

### 1. Basic Interval Curve

`QwtPlotIntervalCurve` consumes `QwtIntervalSample` data, where each sample pairs a median value with a `QwtInterval` range, and its `CurveStyle` (`NoCurve` / `Tube`, default `Tube`) decides how the intervals are rendered. In `Tube` style `setPen()` strokes the boundary curves while `setBrush()` fills the area between them. The example below creates a plot and draws four samples as a translucent band:

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

// Set the pen for the upper/lower boundary curves
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

`CurveStyle` has no dedicated error-bar value; error bars are produced by switching the curve style to `NoCurve` and attaching a `QwtIntervalSymbol` via `setSymbol()`: the symbol's `Style` picks the cap shape (`Bar` draws a line with end caps), the symbol's own `setPen()` determines the stroke and `setWidth()` the cap size. `setSymbol()` takes ownership of the pointer. The snippet below draws error bars only:

```cpp
// Draw the per-interval symbols only, no tube
curve->setStyle(QwtPlotIntervalCurve::NoCurve);

// Error bar symbol: line with end caps
QwtIntervalSymbol* symbol = new QwtIntervalSymbol(QwtIntervalSymbol::Bar);
symbol->setPen(QPen(Qt::black, 1));
symbol->setWidth(5);  // Error bar cap width (pixels)

// Ownership of the symbol is transferred to the curve
curve->setSymbol(symbol);
```

### 3. Tube Style Configuration

In `Tube` style the upper and lower bounds are stroked with the item's `setPen()` and the area between them is filled through `setBrush()` (default `Qt::NoBrush`, i.e. unfilled). The snippet below configures a red stroke with a translucent pink fill for the tube:

```cpp
curve->setStyle(QwtPlotIntervalCurve::Tube);

// Boundary curve style (both the upper and the lower bound use this pen)
curve->setPen(QPen(Qt::red, 2));

// Fill style for the area between the boundaries
curve->setBrush(QBrush(QColor(255, 200, 200, 150)));
```

### 4. Symbol Configuration

`QwtIntervalSymbol` is constructed with a `Style`, either `Bar` for a line with end caps or `Box` for a plain rectangle, and is tuned via `setPen()`, `setBrush()`, and `setWidth()` (stroke/cap width in pixels) before being assigned to the curve with `setSymbol()`, which takes ownership of the pointer. The example below creates a blue `Bar` symbol 8 pixels wide:

```cpp
#include <QwtIntervalSymbol>

// Create interval symbol
QwtIntervalSymbol* symbol = new QwtIntervalSymbol(QwtIntervalSymbol::Bar);

// Set style
symbol->setPen(QPen(Qt::blue, 1));
symbol->setWidth(8);  // Symbol width (pixels)

curve->setSymbol(symbol);  // Ownership is transferred to the curve

// Symbol styles:
// - NoSymbol: No symbol is drawn
// - Bar: Line with end caps (standard error bar)
// - Box: Plain rectangle using pen() and brush()
// - UserSymbol: Reserved for derived classes of QwtIntervalSymbol
```

## Core Methods Summary

| Method | Description |
|--------|-------------|
| `setSamples()` | Set interval data (`QVector<QwtIntervalSample>` or `QwtSeriesData<QwtIntervalSample>*`) |
| `setStyle()` | Set curve style (`NoCurve` / `Tube`) |
| `setPen()` | Set the pen stroking the boundary curves (`Tube` style) |
| `setBrush()` | Set the brush filling the tube area |
| `setSymbol()` | Set the interval symbol drawn per sample (ownership transferred) |
| `setPaintAttribute()` | Toggle clipping hints (`ClipPolygons` / `ClipSymbol`) |

> **Tip — Application Scenarios**
>
> - Experimental data error display
> - Confidence interval visualization
> - Fluctuation range display
> - Prediction interval display

> **Example — Related Examples**
>
> - Interval curve demo: See custom examples