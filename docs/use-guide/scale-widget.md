# Axis Widget - QwtScaleWidget

`QwtScaleWidget` is the display widget for axes, responsible for drawing tick marks, tick labels, and axis titles. It is a fundamental component of the QwtPlot axis system and also supports standalone use.

## Main Features

**Features**

- ✅ **Tick drawing**: Draws major ticks, minor ticks, and tick labels
- ✅ **Axis title**: Supports displaying axis title text
- ✅ **Outside axis title**: Supports painting the title horizontally below/above the scale widget (`TitleOutside`)
- ✅ **Built-in interaction**: Supports mouse drag panning and scroll wheel zooming (new in Qwt7)
- ✅ **Color bar**: Supports displaying color bars (for spectrograms)
- ✅ **Style customization**: Customizable tick length, label font, etc.

## Basic Concepts

### Axis Position

Qwt uses the `QwtAxis::Position` enum to identify axis positions:

| Position | Enum Value | Description |
|------|--------|------|
| Bottom X-axis | `QwtAxis::XBottom` | Bottom horizontal axis |
| Top X-axis | `QwtAxis::XTop` | Top horizontal axis |
| Left Y-axis | `QwtAxis::YLeft` | Left vertical axis |
| Right Y-axis | `QwtAxis::YRight` | Right vertical axis |

### Axis Structure

```text
          Title
         Time(s)
            ↓
    ────────┼────┼────┼────┼────→ Tick marks
           0    2    4    6    8   Tick labels

Ticks are divided into: major ticks (long), minor ticks (short)
```

## Usage

### 1. Setting Axes via QwtPlot

```cpp
#include <QwtPlot>

QwtPlot* plot = new QwtPlot();

// Set axis titles
plot->setAxisTitle(QwtAxis::XBottom, "Time (s)");
plot->setAxisTitle(QwtAxis::YLeft, "Voltage (V)");

// Set axis ranges
plot->setAxisScale(QwtAxis::XBottom, 0, 100);  // X-axis 0-100
plot->setAxisScale(QwtAxis::YLeft, -10, 10);   // Y-axis -10 to 10

// Set axis visibility
plot->setAxisVisible(QwtAxis::XTop, false);    // Hide top X-axis
plot->setAxisVisible(QwtAxis::YRight, true);   // Show right Y-axis

plot->replot();
```

### 2. Getting Axis Widget

```cpp
// Get axis widget at a specific position
QwtScaleWidget* scaleWidget = plot->axisWidget(QwtAxis::XBottom);

// Set tick label font
scaleWidget->setFont(QFont("Arial", 8));

// Set axis title style
QwtText title("Time (s)");
title.setFont(QFont("Arial", 10, QFont::Bold));
title.setColor(Qt::darkBlue);
scaleWidget->setTitle(title);

// Set tick color
scaleWidget->setPalette(QPalette(Qt::black));
```

### 3. Built-in Interaction Features (New in Qwt7)

Qwt 7.0 adds built-in axis interaction features, supporting direct panning and zooming on axes:

```cpp
// Get axis widget
QwtScaleWidget* scaleWidget = plot->axisWidget(QwtAxis::XBottom);

// Enable built-in panning
scaleWidget->setBuiltInAction(QwtScaleWidget::Pan, true);

// Enable built-in zooming
scaleWidget->setBuiltInAction(QwtScaleWidget::Zoom, true);

// Set interaction mouse buttons
scaleWidget->setActionButton(Qt::LeftButton);  // Left button drag panning
scaleWidget->setActionButtons(Qt::MiddleButton, Qt::ControlModifier);  // Ctrl+middle button zooming
```

Panning effect:

![qwt-scale-builtin-action-pan](../assets/screenshots/qwt-scale-builtin-action-pan.gif)

Zooming effect:

![qwt-scale-builtin-action-zoom](../assets/screenshots/qwt-scale-builtin-action-zoom.gif)

For detailed axis interaction documentation, refer to: [Axis Interaction Actions](scale-builtin-action.md)

### 4. Color Bar Display

Used for scenarios requiring color mapping such as spectrograms:

```cpp
// Enable color bar
scaleWidget->setColorBarEnabled(true);

// Set color bar width
scaleWidget->setColorBarWidth(20);

// Set color map
QwtLinearColorMap* colorMap = new QwtLinearColorMap(Qt::blue, Qt::red);
scaleWidget->setColorMap(QwtInterval(0, 100), colorMap);

// Set color bar position
scaleWidget->setColorBarPosition(QwtScaleWidget::Right);  // On the right side of ticks
```

### 5. Tick Drawing Style

```cpp
// Set tick position (relative to axis direction)
scaleWidget->setTickPosition(QwtScaleWidget::Outside);  // Ticks on outside
// Other options: Inside (inside), Both (both sides)

// Set tick length
scaleWidget->setMajorTickLength(10);  // Major tick length (pixels)
scaleWidget->setMinorTickLength(5);   // Minor tick length (pixels)

// Set tick margin
scaleWidget->setTickMargin(2);        // Distance between ticks and axis line
```

### 6. Tick Label Formatting

Customize tick label format by setting `QwtScaleDraw`:

```cpp
#include <QwtScaleDraw>

// Custom tick drawing
class MyScaleDraw : public QwtScaleDraw
{
public:
    virtual QwtText label(double value) const override
    {
        // Custom label format
        return QwtText(QString::number(value, 'f', 1));  // Keep 1 decimal place
    }
};

// Apply custom tick drawing
scaleWidget->setScaleDraw(new MyScaleDraw());
```

### 7. Setting Tick Division

```cpp
#include <QwtScaleDiv>
#include <QwtScaleEngine>

// Use automatic scale engine
plot->setAxisAutoScale(QwtAxis::XBottom);  // Automatically calculate ticks

// Manually set tick division
QwtScaleDiv scaleDiv;
scaleDiv.setInterval(0, 100);
QList<double> majorTicks;
majorTicks << 0 << 25 << 50 << 75 << 100;
scaleDiv.setTicks(QwtScaleDiv::MajorTick, majorTicks);

plot->setAxisScaleDiv(QwtAxis::XBottom, scaleDiv);
```

### 8. Outside Axis Titles (TitleOutside)

By default the axis title is painted inside the scale widget (rotated for vertical axes). With `QwtScaleWidget::TitleOutside` the title is painted as horizontal text into a caption strip outside the scale widget instead: directly **below** it for `YLeft`/`YRight`/`XBottom` and **above** it for `XTop`. The plot layout reserves the strip automatically and keeps it attached to the scale widget through every relayout (window resize, axis visibility or scale changes, ...).

```cpp
#include <QwtPlot>
#include <QwtScaleWidget>

QwtPlot* plot = new QwtPlot();
plot->setAxisTitle(QwtAxis::YLeft, "Voltage (V)");
plot->setAxisTitle(QwtAxis::YRight, "Current (mA)");

// Paint the Y titles horizontally below their scale widgets
plot->setAxisTitlePlacement(QwtAxis::YLeft, QwtScaleWidget::TitleOutside);
plot->setAxisTitlePlacement(QwtAxis::YRight, QwtScaleWidget::TitleOutside);

// Equivalent via the scale widget (here: X title below the bottom scale)
plot->axisWidget(QwtAxis::XBottom)->setTitlePlacement(QwtScaleWidget::TitleOutside);

// Switch back to the legacy in-widget title at any time
plot->setAxisTitlePlacement(QwtAxis::YLeft, QwtScaleWidget::TitleInside);
```

Effect (from the example `examples/2D/outsideTitle`; left: single plot, right: multi axis plot with parasite layers, every layer title below its own axis column):

![outside-title](../assets/screenshots/outside-title.png)

**Behavior notes**

- In `TitleOutside` mode the title no longer contributes to the axis dimension; the layout reserves a caption strip in the adjacent band instead. `QwtPlotLayout::scaleCaptionRect()` exposes the geometry of that strip.
- Multi axis plots (see [Parasite Axes](parasite-axes.md)): all layers share the bottom band. The host layout aggregates the caption demands of every layer and splits the band between the captions at the mid points of the neighboring axis columns, so captions stay below their own column and never overlap.
- The horizontal alignment of the caption text follows `setTitleAlignment()`; `setTitlePosition()` (position along the backbone) only applies to `TitleInside`.
- Caption text wraps when it is wider than its region - prefer short titles for narrow multi axis columns.
- `QwtPlotRenderer` includes the captions in PNG/SVG/PDF exports (`renderScaleCaption()`).

## Core Method Summary

### QwtPlot Axis Methods

| Method | Description |
|------|------|
| `setAxisTitle()` | Set axis title |
| `setAxisTitlePlacement()` | Set title placement (inside the widget or outside as caption) |
| `setAxisScale()` | Set axis range |
| `setAxisAutoScale()` | Enable auto scaling |
| `setAxisVisible()` | Set axis visibility |
| `axisWidget()` | Get axis widget |
| `setAxisMaxMajor()` | Set maximum number of major ticks |
| `setAxisMaxMinor()` | Set maximum number of minor ticks |

### QwtScaleWidget Methods

| Method | Description |
|------|------|
| `setTitle()` | Set axis title |
| `setTitlePlacement()` | Set title placement (`TitleInside`/`TitleOutside`) |
| `setFont()` | Set label font |
| `setBuiltInAction()` | Enable built-in interaction |
| `setColorBarEnabled()` | Enable color bar |
| `setColorMap()` | Set color map |
| `setTickPosition()` | Set tick position |
| `setScaleDraw()` | Set scale draw object |

!!! tip "Axis Configuration Recommendations"
    - For numeric axes, automatic scaling meets most needs
    - For time axes, use `QwtDateScaleEngine`
    - For logarithmic axes, use `QwtLogScaleEngine`

!!! example "Related Examples"
    - Axis interaction: Multiple examples in `examples/2D`
    - Outside axis titles: `examples/2D/outsideTitle`
    - Color bar: `examples/2D/spectrogram`
    - Tick demo: `playground/scaleengine`
