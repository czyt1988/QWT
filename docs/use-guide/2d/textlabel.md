# Text Label - QwtPlotTextLabel

`QwtPlotTextLabel` is used to display text labels on a plot. Unlike `QwtPlotMarker` labels, it uses canvas geometry coordinates rather than plot data coordinates for positioning, making it suitable for displaying fixed descriptive text or titles.

## Key Features

The label is aligned to the plot canvas and does not transform with the axes. Font, color, background and border come from the `QwtText` object, whose render flags also place the text inside the canvas rect. `setMargin()` sets the distance between the canvas contents rect and the label rect.

## Basic Concepts

### Coordinate Positioning Methods

Difference from `QwtPlotMarker`:

| Class | Coordinate Type | Description |
|-------|----------------|-------------|
| `QwtPlotMarker` | Data coordinates | Transforms with axis scaling |
| `QwtPlotTextLabel` | Canvas coordinates | Fixed at canvas position, no transformation |

### Positioning Diagram

The diagram below shows how a text label is pinned to a fixed relative position on the canvas (here the upper right corner), independent of the plot's data coordinates:

```text
┌────────────────────────────────────┐
│                                    │
│  Label text          ← Fixed at    │
│                       upper right  │
│                      (relative pos)│
│                                    │
│            ┌─────────┐             │
│            │Plot data│             │
│            └─────────┘             │
│                                    │
└────────────────────────────────────┘
```

## Usage

### 1. Creating a Text Label

The text content and its appearance are wrapped in a `QwtText` object (font, color, alignment flags, etc.) and handed to the item through `setText()`; the item is then mounted onto the plot with `attach()`. There is no explicit position setter - the label is aligned inside the plot canvas according to the render flags of the `QwtText`. The example below creates a description label pinned to the upper right corner of the canvas:

```cpp
#include <QwtPlot>
#include <QwtPlotTextLabel>
#include <QwtText>

QwtPlot* plot = new QwtPlot();

// Create text label
QwtPlotTextLabel* label = new QwtPlotTextLabel();

// Set text content
QwtText text("Chart Description");
text.setFont(QFont("Arial", 12, QFont::Bold));
text.setColor(Qt::black);

// Align the label at the upper right corner of the canvas
text.setRenderFlags(Qt::AlignRight | Qt::AlignTop);
label->setText(text);

// Keep 10 pixels distance between the canvas contents rect
// and the rectangle where the label can be displayed
label->setMargin(10);

label->attach(plot);
plot->replot();
```

### 2. Text Style Configuration

The visual style is carried by `QwtText`: `setFont()` sets the font, `setColor()` the text color, `setBackgroundBrush()` the background and `setBorderPen()` the border. The example below configures a label with a light-yellow background and a gray border:

```cpp
QwtText text;

// Set font
text.setFont(QFont("Helvetica", 10));

// Set color
text.setColor(Qt::darkBlue);

// Set background color
text.setBackgroundBrush(QBrush(QColor(255, 255, 200)));

// Set border
text.setBorderPen(QPen(Qt::gray, 1));

// Round the corners of the background
text.setBorderRadius(4);

label->setText(text);

// The distance between the canvas contents rect and the label
// is a property of the label item (QwtText has no margins):
label->setMargin(5);
```

### 3. Position and Alignment

The label always covers the canvas rect (shrunk by `setMargin()`) and the alignment decides where inside this rect the text is placed; it is configured through the render flags of the `QwtText` (any combination of `Qt::AlignmentFlag` values). The code below demonstrates a centered label and lists other common alignment combinations:

```cpp
// Centered label - the alignment is part of the QwtText
QwtText text("Centered Label");
text.setRenderFlags(Qt::AlignCenter);
label->setText(text);

// Distance between the canvas contents rect and the label rect
label->setMargin(20);

// Other alignment options (QwtText::setRenderFlags):
// Qt::AlignLeft | Qt::AlignTop      - Upper left
// Qt::AlignRight | Qt::AlignBottom  - Lower right
// Qt::AlignHCenter | Qt::AlignTop   - Centered at the top
```

### 4. Custom Placement via textRect()

`QwtPlotTextLabel` does not support rotated text - when a rotated annotation is needed, implement a custom `QwtPlotItem` and use `QPainter::rotate()` in its `draw()` method. The hook for custom placement inside the canvas is the virtual method `textRect()`: it receives the canvas rect with margins subtracted plus the size required by the text and returns the rect where the label is drawn. The example below pins the label to the lower right corner:

```cpp
// QwtPlotTextLabel has no setRotation(): for rotated text, derive from
// QwtPlotItem and rotate the painter inside draw() instead.

class MyTextLabel : public QwtPlotTextLabel
{
public:
    QRectF textRect(const QRectF& rect, const QSizeF& textSize) const override
    {
        // Pin the label to the lower right corner, 10 pixels away
        QRectF r(QPointF(), textSize);
        r.moveBottomRight(rect.bottomRight() - QPointF(10, 10));
        return r;
    }
};

MyTextLabel* label = new MyTextLabel();
label->setText(QwtText("Custom Placement"));
label->attach(plot);
```

## Core Methods Summary

| Method | Description |
|--------|-------------|
| `setText()` | Set the text (a `QwtText` carrying font, color, alignment flags, background) |
| `text()` | Return the current text |
| `setMargin()` | Distance between the canvas contents rect and the label rect |
| `margin()` | Return the margin |
| `textRect()` | Virtual hook computing the rect where the label is drawn |

> **Tip — Application Scenarios**
>
> - Chart title or descriptive text
> - Fixed position annotations
> - Unit descriptions
> - Version information display

> **Example — Related Examples**
>
> - Text label demo: See custom examples