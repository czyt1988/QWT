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