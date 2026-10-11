# Shape Item - QwtPlotShapeItem

`QwtPlotShapeItem` is used to display arbitrary shapes on a plot, supporting rectangles, circles, polygons, paths, and other geometric figures. Shapes are positioned using plot coordinates and automatically adjust with axis transformations.

## Key Features

**Features**

- Arbitrary shape support: Supports various geometric shapes defined by QPainterPath
- Coordinate binding: Shape positions use plot coordinates, scaling with transformations
- Style customization: Configurable fill, border, color, and other styles
- Fill optimization: Supports multiple fill modes for improved rendering performance

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

### Fill Modes

| Mode | Enum Value | Description |
|------|-----------|-------------|
| Base fill | `Base` | Simple fill, best performance |
| Graphic fill | `Graphic` | Stored using QwtGraphic, supports caching |

## Usage

### 1. Creating a Rectangle Shape

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

```cpp
// Set fill mode
shapeItem->setFillMode(QwtPlotShapeItem::Graphic);

// Set render hint
shapeItem->setRenderHint(QwtPlotShapeItem::RenderAntialiased);

// Set border style
shapeItem->setPen(QPen(Qt::black, 1, Qt::DashLine));

// Set fill style
shapeItem->setBrush(QBrush(Qt::yellow, Qt::FDiagPattern));  // Diagonal line fill
```

## Core Methods Summary

| Method | Description |
|--------|-------------|
| `setShape()` | Set shape path |
| `setPen()` | Set border pen |
| `setBrush()` | Set fill brush |
| `setFillMode()` | Set fill mode |
| `setRenderHint()` | Set render hint |

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

![Shapes](../assets/screenshots/shapes.png)
