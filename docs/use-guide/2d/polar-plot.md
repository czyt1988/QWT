# Polar Plot - QwtPolarPlot

`QwtPolarPlot` is a widget for drawing polar coordinate graphics. Unlike the traditional Cartesian coordinate system `QwtPlot`, it uses angle and radius as its two axes, making it suitable for displaying circular data such as radar charts, directional distributions, and astronomical data.

## Main Features

**Features**

- ✅ **Polar coordinate system**: Uses angle (0-360°) and radius as axes
- ✅ **Curve drawing**: Supports drawing curves in polar coordinate system
- ✅ **Grid system**: Provides concentric circle and angular ray grids
- ✅ **Interaction support**: Supports zooming and panning in polar coordinate system

## Basic Concepts

### Polar Coordinate Structure

The sketch below shows the polar canvas: every data point is placed by an azimuth angle and a radius, concentric circles mark the radius scale, and rays from the pole mark the angle scale:

```text
          90°
           │
    135°   │   45°
      \    │    /
       \   │   /
        \  │  /
  180°───●─+─●───0°
        /  │  \
       /   │   \
      /    │    \
    225°   │   315°
           │
          270°

● = Data points
Concentric circles = Radius scales
Rays = Angle scales
```

### Polar Item Types

| Class Name | Description |
|------|------|
| `QwtPolarPlot` | Main polar plot widget |
| `QwtPolarCurve` | Polar curve |
| `QwtPolarGrid` | Polar grid |
| `QwtPolarMarker` | Polar marker |

## Usage

The polar plot example is located at: `examples/2D/polardemo`. Screenshot:

![Polar Demo](../../assets/screenshots/polardemo.png)

### 1. Creating a Polar Plot

A polar plot combines three ingredients: the `QwtPolarPlot` widget itself (azimuth and radius scales set via `setScale()`), a `QwtPolarGrid` item for the background, and one or more `QwtPolarCurve` items. Curves consume `QwtPointPolar` samples wrapped in a `QwtSeriesData<QwtPointPolar>` object passed to `setData()` — for a plain `QVector` of samples a minimal `QwtArraySeriesData` subclass is enough. The example below assembles them into a complete chart:

```cpp
#include <QwtPolarPlot>
#include <QwtPolarGrid>
#include <QwtPolarCurve>
#include <qwt_series_data.h>

#include <cmath>

// QwtPolarCurve consumes QwtSeriesData<QwtPointPolar>; a minimal
// array-based subclass only needs to provide the bounding rectangle
class PolarSeriesData : public QwtArraySeriesData<QwtPointPolar>
{
public:
    PolarSeriesData(const QVector<QwtPointPolar>& samples)
        : QwtArraySeriesData<QwtPointPolar>(samples)
    {
    }

    virtual QRectF boundingRect() const override
    {
        if (cachedBoundingRect.width() < 0.0)
            cachedBoundingRect = qwtBoundingRect(*this);

        return cachedBoundingRect;
    }
};

// Create polar plot
QwtPolarPlot* polarPlot = new QwtPolarPlot();
polarPlot->setTitle("Polar Plot Example");

// Set angle range (default 0-360 degrees)
polarPlot->setScale(QwtPolar::Azimuth, 0.0, 360.0);

// Set radius range
polarPlot->setScale(QwtPolar::Radius, 0.0, 100.0);

// Create polar grid
QwtPolarGrid* grid = new QwtPolarGrid();
grid->setPen(QPen(Qt::gray, 0.5));
grid->attach(polarPlot);

// Create polar curve
QwtPolarCurve* curve = new QwtPolarCurve("Curve Data");

// Prepare data (angle, radius)
QVector<QwtPointPolar> samples;
for (int angle = 0; angle < 360; angle += 10) {
    const double radius = 50 + 30 * std::sin(angle * 3.0 * M_PI / 180.0);
    samples << QwtPointPolar(angle, radius);
}
curve->setData(new PolarSeriesData(samples));
curve->setPen(QPen(Qt::blue, 2));

curve->attach(polarPlot);
polarPlot->replot();
```

### 2. Polar Grid Configuration

`QwtPolarGrid` draws both families of grid lines — `showGrid()` toggles the azimuth rays and radius circles by scale id (`QwtPolar::Azimuth` / `QwtPolar::Radius`) — while `showAxis()` switches the tick labels on or off for each of the `QwtPolar::AxisAzimuth`, `AxisLeft`, `AxisRight`, `AxisTop`, and `AxisBottom` axes:

```cpp
QwtPolarGrid* grid = new QwtPolarGrid();

// Show angle grid lines (rays)
grid->showGrid(QwtPolar::Azimuth, true);

// Show radius grid lines (concentric circles)
grid->showGrid(QwtPolar::Radius, true);

// Set grid style
grid->setPen(QPen(Qt::lightGray, 0.5));

// Show tick labels
grid->showAxis(QwtPolar::AxisAzimuth, true);  // Angle labels
grid->showAxis(QwtPolar::AxisLeft, true);     // Left radius labels
grid->showAxis(QwtPolar::AxisRight, true);    // Right radius labels
grid->showAxis(QwtPolar::AxisTop, true);      // Top radius labels
grid->showAxis(QwtPolar::AxisBottom, true);   // Bottom radius labels
```

### 3. Polar Curve Style

`QwtPolarCurve::setStyle()` chooses between `Lines` (points connected by straight segments, optionally smoothed by a curve fitter) and `NoCurve` (symbols only); values above `UserCurve` (100) are reserved for user-defined styles. Pen and `QwtSymbol` are set exactly as on Cartesian curves:

```cpp
#include <QwtSymbol>

QwtPolarCurve* curve = new QwtPolarCurve();

// Set curve style
curve->setStyle(QwtPolarCurve::Lines);  // Line style
// Available styles: NoCurve (draw symbols only), Lines, UserCurve (> 100,
// reserved for user-specific styles); for a scatter plot use NoCurve + a symbol

// Set line style
curve->setPen(QPen(Qt::red, 2));

// Set symbol
QwtSymbol* symbol = new QwtSymbol(QwtSymbol::Ellipse,
    QBrush(Qt::red), QPen(Qt::darkRed), QSize(5, 5));
curve->setSymbol(symbol);
```

### 4. Angle Range Settings

The azimuth axis takes the usual `setScale()` / `setScaleMaxMajor()` calls: limit the range to a sector (e.g. 0–180°), cap the number of major ticks, or swap the limits to reverse the angle direction:

```cpp
// Set angle range (e.g., show only 0-180 degrees)
polarPlot->setScale(QwtPolar::Azimuth, 0.0, 180.0);

// Set angle step
polarPlot->setScaleMaxMajor(QwtPolar::Azimuth, 8);  // 8 major ticks (every 45 degrees)

// Reverse angle direction
polarPlot->setScale(QwtPolar::Azimuth, 360.0, 0.0);  // Decreasing from 360 to 0
```

### 5. Radius Range Settings

The radius axis is adjusted the same way, with `setScaleMaxMinor()` additionally controlling the minor ticks between two majors; for data spanning several orders of magnitude, `setScaleEngine()` installs a logarithmic engine (`QwtLogScaleEngine`, the same core engine as for Cartesian axes) on the radius scale:

```cpp
// Set radius range
polarPlot->setScale(QwtPolar::Radius, 0.0, 200.0);

// Set radius ticks
polarPlot->setScaleMaxMajor(QwtPolar::Radius, 5);   // 5 major ticks
polarPlot->setScaleMaxMinor(QwtPolar::Radius, 4);   // 4 minor ticks per interval

// Logarithmic radius scale (the radius interval must be positive then)
#include <QwtScaleEngine>
polarPlot->setScaleEngine(QwtPolar::Radius, new QwtLogScaleEngine());
polarPlot->setScale(QwtPolar::Radius, 1.0, 200.0);
```

### 6. Polar Markers

`QwtPolarMarker` annotates a single position given as a `QwtPointPolar`: `setSymbol()` draws an icon there, while `setLabel()` plus `setLabelAlignment()` attach a `QwtText` caption with the chosen alignment:

```cpp
#include <QwtPolarMarker>
#include <QwtSymbol>

QwtPolarMarker* marker = new QwtPolarMarker();

// Set marker position
marker->setPosition(QwtPointPolar(90.0, 80.0));  // Angle 90°, radius 80

// Set marker symbol
QwtSymbol* symbol = new QwtSymbol(QwtSymbol::Cross,
    Qt::NoBrush, QPen(Qt::red, 2), QSize(10, 10));
marker->setSymbol(symbol);

// Set label
QwtText label("Key Point");
label.setColor(Qt::red);
marker->setLabel(label);
marker->setLabelAlignment(Qt::AlignCenter);

marker->attach(polarPlot);
```

## Polar Axis Identifiers

| Identifier | Enum Value | Description |
|------|--------|------|
| Angle axis | `QwtPolar::Azimuth` | Angle coordinate (0-360°) |
| Radius axis | `QwtPolar::Radius` | Radius coordinate |

## Data Point Structure

All polar items consume `QwtPointPolar` samples, which simply pair an azimuth (in degrees) with a radius, accessible through `azimuth()` / `radius()`; the constructor taking a `QPointF` converts Cartesian coordinates automatically, and `toPoint()` converts back:

```cpp
// Polar coordinate point
QwtPointPolar point(90.0, 80.0);  // azimuth 90 degrees, radius 80

double a = point.azimuth();  // angle
double r = point.radius();   // radius

// Can convert from Cartesian coordinates
QwtPointPolar polar(QPointF(1.0, 1.0));  // automatically calculates angle and radius
QPointF cartesian = polar.toPoint();     // and back
```

## Core Method Summary

### QwtPolarPlot

| Method | Description |
|------|------|
| `setScale()` | Set axis range |
| `setScaleMaxMajor()` | Set number of major ticks |
| `setScaleMaxMinor()` | Set number of minor ticks |
| `setScaleEngine()` | Set scale engine |
| `replot()` | Refresh plot |

### QwtPolarCurve

| Method | Description |
|------|------|
| `setData()` | Set data (takes ownership of a `QwtSeriesData<QwtPointPolar>` object) |
| `setStyle()` | Set curve style |
| `setPen()` | Set line style |
| `setSymbol()` | Set data point symbol |

> **Tip — Polar Plot Application Scenarios**
>
> - Radar displays
> - Astronomical data (star charts)
> - Directional distribution statistics
> - Antenna radiation patterns

> **Example — Related Examples**
>
> - Polar demo: `examples/2D/polardemo`
> - Polar spectrogram: `examples/2D/polarspectrogram`

Screenshot of the polar spectrogram:

![Polar Spectrogram](../../assets/screenshots/polarspectrogram.png)
