# Zoom Tool

In Qwt6, `QwtPlotZoomer` required specifying two axes for zoom operations. If your plot needed all four axes to work, you had to bind two zoomers to the plot:

```cpp
// Qwt6 traditional usage - requires two zoomers to cover four axes
QwtPlotZoomer* zoomer1 = new QwtPlotZoomer(QwtAxis::XBottom, QwtAxis::YLeft, canvas);
QwtPlotZoomer* zoomer2 = new QwtPlotZoomer(QwtAxis::XTop, QwtAxis::YRight, canvas);
```

To solve this problem, the new version introduces the following refactoring:

- **`QwtPlotAxisZoomer`** - A rename of the original `QwtPlotZoomer`, representing a zoomer bound to two specific axes
- **`QwtPlotCanvasZoomer`** - A new zoomer that does not require specifying axes and performs overall zoom on the entire canvas

## Zoomer Feature Comparison

Below is a feature comparison of these two zoomers:

| Feature | `QwtPlotAxisZoomer` | `QwtPlotCanvasZoomer` |
|------|-------------------|---------------------|
| Axis Binding | Requires specifying X and Y axes | Automatically handles all four axes |
| Use Case | Specific axis zoom | Overall canvas zoom |
| Parasite Plot Support | Requires manual binding | Automatically supports all parasite plots |

## Usage Examples

`QwtPlotAxisZoomer` and `QwtPlotCanvasZoomer` have the same usage pattern, with the difference being that `QwtPlotCanvasZoomer` does not require specifying axes.

### QwtPlotAxisZoomer Usage Example

`QwtPlotAxisZoomer` has two constructors: one takes the X and Y axes to bind (as `QwtAxisId`) plus the canvas, and the other takes only the canvas, defaulting to `QwtAxis::XBottom` and `QwtAxis::YLeft`; the trailing `doReplot` flag (default `true`) controls whether the plot repaints automatically after zooming. The example below shows both forms:

```cpp
#include "qwt_plot_axis_zoomer.h"

// Create a zoomer for specific axes
QwtPlotAxisZoomer* axisZoomer = new QwtPlotAxisZoomer(
    QwtAxis::XBottom,  // X axis
    QwtAxis::YLeft,    // Y axis
    plot->canvas(),            // Canvas
    true               // Auto repaint
);

// Or use default axes
QwtPlotAxisZoomer* defaultZoomer = new QwtPlotAxisZoomer(plot->canvas());
```

### QwtPlotCanvasZoomer Usage Example

`QwtPlotCanvasZoomer` needs nothing but the canvas: it zooms the plot as a whole and automatically covers all axes, including parasite plots, without any axis binding. The example below creates one in a single line:

```cpp
#include "qwt_plot_canvas_zoomer.h"

// Create overall canvas zoomer, automatically handles all axes
QwtPlotCanvasZoomer* canvasZoomer = new QwtPlotCanvasZoomer(plot->canvas());
```

## Key Settings Description

### Default Key Bindings

#### Mouse Operations

| Operation | Default Button | Description |
|------|---------|----------|
| **Zoom Selection** | Left button drag | Select rectangular area for zooming |
| **Back to Base** | Right button click | Zoom to base view (fully zoomed out) |
| **Step Back** | Middle button click | Step back one zoom level |
| **Step Forward** | Middle+Shift click | Step forward one zoom level |

#### Keyboard Operations

| Operation | Default Key | Description |
|------|---------|----------|
| **Step Forward** | `+` | Step forward one zoom level |
| **Step Back** | `-` | Step back one zoom level |
| **Back to Base** | `Escape` | Return to base view |


### Custom Key Configuration

You can configure shortcuts using the following predefined button configurations.

The following mouse shortcuts are supported:
- MouseSelect2: Reset to base
- MouseSelect3: Zoom stack back
- MouseSelect6: Zoom stack forward

The following keyboard shortcuts are supported:
- KeyUndo: Zoom stack back
- KeyRedo: Zoom stack forward
- KeyHome: Reset to base

```cpp
// Configure custom keys
zoomer->setKeyPattern(QwtEventPattern::KeyRedo, Qt::Key_Plus,Qt::ShiftModifier);    // Zoom stack forward (zoom in)
zoomer->setKeyPattern(QwtEventPattern::KeyUndo, Qt::Key_Minus,Qt::ShiftModifier);   // Zoom stack back (zoom out)
zoomer->setKeyPattern(QwtEventPattern::KeyHome, Qt::Key_Escape);  // Reset

// Configure mouse patterns, middle button becomes reset to base, right button becomes step back
zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::MiddleButton);
zoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);
```
