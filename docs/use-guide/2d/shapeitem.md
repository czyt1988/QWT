# Shape Item - QwtPlotShapeItem

`QwtPlotShapeItem` is used to display arbitrary shapes on a plot, supporting rectangles, circles, polygons, paths, and other geometric figures. Shapes are positioned using plot coordinates and automatically adjust with axis transformations.

## Key Features

Shapes can be any geometry described by QPainterPath, including rectangles, circles and polygons. Positions use plot coordinates, so shapes scale with the axes. Fill, border and color are configurable, and polygon clipping (`ClipPolygons`) with point weeding (`setRenderTolerance()`) reduces rendering cost.

## Basic Concepts

### Shape Positioning

QwtPlotShapeItem shapes are positioned using plot coordinates, not pixel coordinates:

```text
    Y-axis
     │  ┌─────────┐
     │  │ Rectangle│  ← Position defined using (x,y) coordinates
     │  │  shape   │
     │  └─────────┘
     └───────────────→ X-axis
```

### Paint Attributes and Legend Modes

`QwtPlotShapeItem` has no fill-mode setting; rendering and legend behavior are controlled by a paint attribute and a legend mode instead:

| Setting | Enum Value | Description |
|------|-----------|-------------|
| Paint attribute | `ClipPolygons` | Clip polygons before painting them; speeds up deep zoom, but converting curves/ellipses to polygons may hurt performance for such paths |
| Legend mode | `LegendShape` | Display a scaled down version of the shape on the legend |
| Legend mode | `LegendColor` | Display a filled rectangle on the legend |

## Usage

### 1. Creating a Rectangle Shape

The shape item is constructed with a legend title, and its geometry is handed to `setShape()` as a `QPainterPath`; for a plain rectangle the `setRect()` convenience overload also exists. Path coordinates are interpreted in the plot coordinate system, while `setPen()` / `setBrush()` style the outline and fill. The example below adds a semi-transparent rectangular region to a plot:

```cpp
#include <QwtPlot>
#include <QwtPlotShapeItem>

QwtPlot* plot = new QwtPlot();

// Create shape item
QwtPlotShapeItem* shapeItem = new QwtPlotShapeItem("Rectangle Area");

// Create rectangle path
QPainterPath path;
path.addRect(QRectF(10, 10, 50, 30));  // x, y, width, height

shapeItem->setShape(path);

// Set style
shapeItem->setPen(QPen(Qt::blue, 2));
shapeItem->setBrush(QBrush(QColor(100, 150, 200, 100)));

shapeItem->attach(plot);
plot->replot();
```

### 2. Creating a Circle Shape

Circles and ellipses are built with `QPainterPath::addEllipse()`, which takes a center point plus horizontal and vertical radii, and are then assigned via `setShape()`. The example below draws a semi-transparent red circle of radius 20 at plot coordinates (50, 50):

```cpp
QwtPlotShapeItem* circle = new QwtPlotShapeItem("Circle");

QPainterPath path;
path.addEllipse(QPointF(50, 50), 20, 20);  // Center point, rx, ry

circle->setShape(path);
circle->setPen(QPen(Qt::red, 1));
circle->setBrush(QBrush(QColor(255, 100, 100, 150)));

circle->attach(plot);
```

### 3. Creating a Polygon Shape

An arbitrary polygon is described by a sequence of `moveTo()` / `lineTo()` calls closed with `closeSubpath()`; alternatively `setPolygon()` accepts a `QPolygonF` directly. The example below builds a closed triangle and styles it with a green border and gray fill:

```cpp
QwtPlotShapeItem* polygon = new QwtPlotShapeItem("Polygon");

QPainterPath path;
path.moveTo(0, 0);
path.lineTo(30, 0);
path.lineTo(15, 25);
path.closeSubpath();

polygon->setShape(path);
polygon->setPen(QPen(Qt::green, 2));
polygon->setBrush(QBrush(Qt::lightGray));

polygon->attach(plot);
```

### 4. Style Configuration

Appearance is tuned through `setRenderHint()` (e.g. `RenderAntialiased`, inherited from `QwtPlotItem`), `setPen()` (dashed outlines via `Qt::DashLine`), and `setBrush()`; the brush also accepts Qt pattern fills such as `Qt::FDiagPattern` for hatching. Performance options are the `ClipPolygons` paint attribute (`setPaintAttribute()`) and point weeding via `setRenderTolerance()`; there is no fill-mode setting. The snippet below combines these options:

```cpp
// Clip polygons before painting (performance option)
shapeItem->setPaintAttribute(QwtPlotShapeItem::ClipPolygons, true);

// Simplify the path with point weeding (Douglas-Peucker);
// a tolerance <= 0.0 disables weeding
shapeItem->setRenderTolerance(1.0);

// Set render hint (inherited from QwtPlotItem)
shapeItem->setRenderHint(QwtPlotItem::RenderAntialiased);

// Set border style
shapeItem->setPen(QPen(Qt::black, 1, Qt::DashLine));

// Set fill style
shapeItem->setBrush(QBrush(Qt::yellow, Qt::FDiagPattern));  // Diagonal line fill
```

## Core Methods Summary

| Method | Description |
|--------|-------------|
| `setShape()` | Set the shape as a `QPainterPath` |
| `setRect()` | Set the shape as a rectangle (convenience) |
| `setPolygon()` | Set the shape as a polygon (convenience) |
| `setPen()` | Set border pen |
| `setBrush()` | Set fill brush |
| `setPaintAttribute()` | Toggle `ClipPolygons` clipping |
| `setRenderTolerance()` | Set the weeding tolerance for path simplification |
| `setLegendMode()` | Set legend display (`LegendShape` / `LegendColor`) |
| `setRenderHint()` | Set render hint (inherited from `QwtPlotItem`) |

> **Tip — Application Scenarios**
>
> - Region annotation
> - Background areas
> - Clip region marking
> - Custom geometric shapes

> **Example — Related Examples**
>
> - Shape demo: `playground/shapes`

Screenshot of the shape demo:

![Shapes](../../assets/screenshots/shapes.png)
