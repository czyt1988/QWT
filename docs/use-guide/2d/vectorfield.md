# Vector Field Plot - QwtPlotVectorField

`QwtPlotVectorField` is used to draw vector fields, displaying vector data with direction and magnitude information on a 2D plane. Commonly used in fluid mechanics, electromagnetic field analysis, meteorological data, and other fields.

## Key Features

**Features**

- Arrow symbol display: Uses arrows to represent vector direction and magnitude
- Magnitude mapping: Vector magnitude can be mapped to arrow length or color
- Density control: Supports adjusting display density to avoid overcrowding
- Style customization: Customizable arrow shape and style

## Basic Concepts

### Vector Field Representation

Vector field data consists of position and vector components:

```text
      ↑     ↗
      │    /
      │   /
  ←───●───→───●───→
      │   \
      │    \
      ↓     ↙

Each point displays a vector (direction + magnitude)
```

### Data Structure

Uses `QwtVectorFieldSample` to represent a single vector:

```cpp
class QwtVectorFieldSample {
public:
    QwtVectorFieldSample(double x = 0.0, double y = 0.0,
                         double vx = 0.0, double vy = 0.0);

    QPointF pos() const;  // Position as QPointF
    bool isNull() const;  // True when vx == vy == 0

    double x;   // X coordinate of the position
    double y;   // Y coordinate of the position
    double vx;  // X component of the vector
    double vy;  // Y component of the vector
};
```

## Usage

The vector field example is located at: `playground/vectorfield`. Screenshot below:

![Vector Field](../../assets/screenshots/vectorfield.png)

### 1. Basic Vector Field Plot

The field item is a plain `QwtPlotVectorField` fed with `QwtVectorFieldSample` values — each combining a position (x, y) with vector components (vx, vy) — through `setSamples()`. The example below generates a 10 x 10 trigonometric vector grid and attaches it to a plot:

```cpp
#include <QwtPlot>
#include <QwtPlotVectorField>

QwtPlot* plot = new QwtPlot();
plot->setTitle("Vector Field Example");
plot->setCanvasBackground(Qt::white);

// Create vector field
QwtPlotVectorField* vectorField = new QwtPlotVectorField();

// Prepare data
QVector<QwtVectorFieldSample> samples;
for (int x = 0; x < 10; x++) {
    for (int y = 0; y < 10; y++) {
        double vx = std::sin(y * 0.5);  // X component
        double vy = std::cos(x * 0.5);  // Y component
        samples << QwtVectorFieldSample(x, y, vx, vy);
    }
}
vectorField->setSamples(samples);

vectorField->attach(plot);
plot->replot();
```

### 2. Arrow Style Configuration

The arrow shape is provided by a `QwtVectorFieldSymbol` implementation — `QwtVectorFieldArrow` (filled, with adjustable head and tail width) or `QwtVectorFieldThinArrow` (line-based) — assigned via `setSymbol()`, while the stroke comes from the field item's own `setPen()`. `setMagnitudeScaleFactor()` then controls how strongly the vector magnitude affects the arrow length. The snippet below wires these settings together:

```cpp
#include <QwtVectorFieldSymbol>

// QwtVectorFieldSymbol is an abstract base class; use one of the
// built-in implementations (ownership passes to the field item):

// Filled arrow with adjustable head/tail width
vectorField->setSymbol(new QwtVectorFieldArrow(8.0, 2.0));

// Or: line-based thin arrow (this is the default symbol)
vectorField->setSymbol(new QwtVectorFieldThinArrow(6.0));

// Pen and brush are set on the field item itself, not on the symbol:
// the pen strokes the arrow outline, the brush fills it
vectorField->setPen(QPen(Qt::blue, 1));
vectorField->setBrush(QBrush(Qt::blue));

// Set arrow length scale
vectorField->setMagnitudeScaleFactor(0.5);  // Scale factor
```

### 3. Vector Magnitude Mapping

Magnitude-to-length mapping is governed by `setMagnitudeScaleFactor()`, a linear factor applied to each vector's magnitude (active while the `MagnitudeAsLength` mode is enabled, which is the default), optionally clamped with `setMinArrowLength()` / `setMaxArrowLength()`. The snippet below shows linear mapping, clamping, and a fixed-length configuration:

```cpp
// Map vector magnitude to arrow length (MagnitudeAsLength is on by default)
vectorField->setMagnitudeMode(QwtPlotVectorField::MagnitudeAsLength, true);
vectorField->setMagnitudeScaleFactor(1.0);  // Length = magnitude * factor

// Clamp the computed length into 5...20 pixels
vectorField->setMinArrowLength(5);
vectorField->setMaxArrowLength(20);

// Fixed arrow length (ignore magnitude): set minimum and maximum
// to the same value, so every non-null vector renders 10 pixels long
vectorField->setMinArrowLength(10);
vectorField->setMaxArrowLength(10);
```

### 4. Display Density Control

Density is reduced by averaging neighbouring samples into grid cells: `setRasterSize()` defines the cell size, and the `FilterVectors` paint attribute (enabled via `setPaintAttribute()`) activates the filtering. The snippet below configures this decimation:

```cpp
// Average all vectors inside grid cells of 20 x 20 pixels into one arrow
vectorField->setRasterSize(QSizeF(20, 20));

// Activate the filtering (without FilterVectors, every sample is drawn)
vectorField->setPaintAttribute(QwtPlotVectorField::FilterVectors, true);
```

### 5. Color Mapping

Color-by-magnitude requires a `QwtColorMap` — e.g. a `QwtLinearColorMap` between two colors — assigned with `setColorMap()`, plus the `MagnitudeAsColor` flag enabled through `setMagnitudeMode()`; `setMagnitudeRange()` defines the value interval spanned by the map. The example below sets up a blue-to-red mapping:

```cpp
#include <QwtColorMap>

// Set color based on vector magnitude
QwtLinearColorMap* colorMap = new QwtLinearColorMap(Qt::blue, Qt::red);
vectorField->setColorMap(colorMap);

// Enable magnitude-to-color mapping
vectorField->setMagnitudeMode(QwtPlotVectorField::MagnitudeAsColor, true);

// Value interval spanned by the color map; when left unset (invalid),
// the range is determined from the samples themselves
vectorField->setMagnitudeRange(QwtInterval(0.0, 2.0));
```

## Core Methods Summary

| Method | Description |
|--------|-------------|
| `setSamples()` | Set vector data (`QVector<QwtVectorFieldSample>` or `QwtVectorFieldData*`) |
| `setSymbol()` | Set the arrow symbol (`QwtVectorFieldArrow` / `QwtVectorFieldThinArrow`) |
| `setPen()` / `setBrush()` | Set the arrow outline pen / fill brush |
| `setMagnitudeScaleFactor()` | Set the magnitude-to-length scale factor |
| `setMinArrowLength()` / `setMaxArrowLength()` | Clamp the arrow length (pixels) |
| `setRasterSize()` + `setPaintAttribute(FilterVectors)` | Reduce display density by averaging vectors per grid cell |
| `setColorMap()` | Set the color map used for `MagnitudeAsColor` |
| `setMagnitudeMode()` | Enable `MagnitudeAsColor` / `MagnitudeAsLength` mapping |
| `setMagnitudeRange()` | Set the magnitude interval mapped by the color map |
| `setIndicatorOrigin()` | Set the arrow anchor (`OriginHead` / `OriginTail` / `OriginCenter`) |

> **Tip — Application Scenarios**
>
> - Fluid mechanics: Flow velocity field visualization
> - Electromagnetics: Electric/magnetic field distribution
> - Meteorology: Wind direction field display
> - Mathematical analysis: Vector function visualization

> **Example — Related Examples**
>
> - Vector field demo: `playground/vectorfield`