# Text Label - QwtPlotTextLabel

`QwtPlotTextLabel` is used to display text labels on a plot. Unlike `QwtPlotMarker` labels, it uses canvas geometry coordinates rather than plot data coordinates for positioning, making it suitable for displaying fixed descriptive text or titles.

## Key Features

**Features**

- Canvas coordinate positioning: Uses position relative to canvas, does not transform with axes
- Text style support: Supports font, color, margin, and other style configuration
- Alignment control: Configurable text alignment relative to the anchor point
- Background settings: Configurable text background color or border

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

The text content and its appearance are wrapped in a `QwtText` object (font, color, etc.) and handed to the item through `setText()`; the item is then mounted onto the plot with `attach()`. The example below creates a description label pinned to the upper right corner of the canvas:

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
label->setText(text);

// Set position (relative to canvas coordinates)
label->setPos(0.9, 0.1);  // Upper right corner (90% width, 10% height)

// Set alignment
label->setAlignment(Qt::AlignRight | Qt::AlignTop);

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

// Set margins
text.setMargins(5);  // Spacing between text and border

label->setText(text);
```

### 3. Position and Alignment

The label position is given in normalized canvas coordinates ranging from 0 to 1, while the alignment decides which corner or side of the text rectangle is anchored to that point. The code below demonstrates a centered label and lists other common alignment combinations:

```cpp
// Set position (normalized coordinates, range [0,1])
label->setPos(0.5, 0.5);  // Canvas center

// Set alignment (relative to anchor point)
label->setAlignment(Qt::AlignCenter);

// Other alignment options:
// Qt::AlignLeft | Qt::AlignTop    - Upper left
// Qt::AlignRight | Qt::AlignBottom - Lower right
// Qt::AlignHCenter | Qt::AlignVCenter - Center
```

### 4. Rotated Text

For diagonal annotations the text can be rotated around its anchor by an arbitrary angle in degrees. The example below rotates a label by 45 degrees:

```cpp
// Set text rotation angle
label->setRotation(45.0);  // 45 degree rotation
```

## Core Methods Summary

| Method | Description |
|--------|-------------|
| `setText()` | Set text content |
| `setPos()` | Set position (normalized coordinates) |
| `setAlignment()` | Set alignment |
| `setRotation()` | Set rotation angle |
| `setGeometry()` | Set geometry region |

> **Tip — Application Scenarios**
>
> - Chart title or descriptive text
> - Fixed position annotations
> - Unit descriptions
> - Version information display

> **Example — Related Examples**
>
> - Text label demo: See custom examples