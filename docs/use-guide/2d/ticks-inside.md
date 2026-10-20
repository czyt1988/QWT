# Inward-Facing Ticks Usage Guide

`QwtPlot::TickDirection` controls where axis tick marks are drawn: inside the plot area instead of outside the axis line. This suits compact layouts or special visual effects.

## Main Features

Ticks can point outward (the default) or inward. Each axis (XBottom, XTop, YLeft, YRight) is configured independently. Inward ticks use the same length, color and line width as outward ticks, and the direction can be switched at any time without resetting the tick style.

## Basic Concepts

### Tick Direction Description

In traditional plots, axis tick marks extend outward from the axis line (outward display). The inward tick feature makes tick marks extend inward from the canvas edge, achieving different visual effects.

```text
Outward ticks (default):
    ────┼────┼────┼────┼───→ Main axis (axis line)
       │    │    │    │    Tick marks (extending outward)
      0    2    4    6    8  Labels

Inward ticks:
    ────┼────┼────┼────┼───→ Main axis (axis line)
       │    │    │    │    Tick marks (extending inward)
      0    2    4    6    8  Labels (still outside)
    ┌──────────────────────┐
    │  ← Ticks extend into canvas    │  Canvas area
    └──────────────────────┘
```

### TickDirection Enum

| Enum Value | Description |
|--------|------|
| `QwtPlot::TickOutside` | Outward ticks (default behavior) |
| `QwtPlot::TickInside` | Inward ticks, extending inward from canvas edge |

## Usage

An example of inward tick display is located at: `examples/2D/ticks_inside`. Screenshot:

![ticks_inside](../../assets/screenshots/ticks_inside.png)

### 1. Basic Usage

Setting the tick direction for a single axis:

```cpp
#include <QwtPlot>

QwtPlot* plot = new QwtPlot();

// Set YLeft axis ticks to face inward
plot->setAxisTickDirection(QwtAxis::YLeft, QwtPlot::TickInside);

// Set XBottom axis ticks to face inward
plot->setAxisTickDirection(QwtAxis::XBottom, QwtPlot::TickInside);

// Refresh display
plot->replot();

// Effect: Tick marks on YLeft and XBottom axes display inward from canvas edge,
//         main axis and labels remain outside canvas, consistent with outward tick style
```

### 2. Querying Current Settings

Getting the current tick direction of an axis:

```cpp
// Query YLeft axis tick direction
QwtPlot::TickDirection dir = plot->axisTickDirection(QwtAxis::YLeft);

if (dir == QwtPlot::TickInside) {
    // Ticks currently face inward
} else {
    // Ticks currently face outward (default)
}
```

### 3. Restoring Default Settings

Restoring tick direction to the default outward display:

```cpp
// Restore YLeft axis ticks to face outward
plot->setAxisTickDirection(QwtAxis::YLeft, QwtPlot::TickOutside);

plot->replot();

// Effect: YLeft axis ticks restored to traditional outward display
```

### 4. Batch Setting All Axes

Setting tick direction for multiple axes simultaneously:

```cpp
// Set all axes to inward ticks
plot->setAxisTickDirection(QwtAxis::YLeft, QwtPlot::TickInside);
plot->setAxisTickDirection(QwtAxis::YRight, QwtPlot::TickInside);
plot->setAxisTickDirection(QwtAxis::XBottom, QwtPlot::TickInside);
plot->setAxisTickDirection(QwtAxis::XTop, QwtPlot::TickInside);

plot->replot();
```

## Core Methods

| Method | Parameters | Description |
|------|------|------|
| `setAxisTickDirection(axisId, direction)` | QwtAxisId, TickDirection | Set tick direction for specified axis |
| `axisTickDirection(axisId)` | QwtAxisId | Get tick direction for specified axis |

## Notes

> **Info — Style Inheritance**
>
> Inward tick marks automatically inherit the style settings of outward ticks, including:
> - Tick length (set via `QwtScaleDraw::setTickLength()`)
> - Line width (set via `QwtScaleDraw::setPenWidthF()`)
> - Color (set via `QwtScaleWidget::setScaleColor()`)

> **Warning — Refresh Timing**
>
> Calling `setAxisTickDirection()` automatically triggers `autoRefresh()`. When `autoReplot` is enabled, it will refresh automatically. To refresh immediately, call `replot()`.

> **Tip — Use with Parasite Axes**
>
> The inward tick feature can be used with parasite plots (Parasite Plot). Each parasite plot can independently set its tick direction.

## Differences from QwtPlotScaleItem

| Feature | TickDirection | QwtPlotScaleItem |
|------|---------------|------------------|
| Purpose | Control main axis tick direction | Add additional axes inside canvas |
| Tick Style | Inherits main axis style | Can set style independently |
| Display Position | Replaces/changes main axis ticks | Overlays as independent layer |
| Use Cases | Compact layout or special effects needed | Reference scales needed inside canvas |

> **Example — Example Code**
>
> Complete example located at: `examples/2D/ticks_inside/`
>
> This example demonstrates:
> - Independent control of four axes
> - Real-time tick direction switching
> - Batch setting functionality

## References

- [Axis Widget](scale-widget.md) - Detailed QwtScaleWidget documentation
- [Creating Multiple Axes](parasite-axes.md) - Parasite axis feature
- API documentation: `QwtPlot::setAxisTickDirection()`
