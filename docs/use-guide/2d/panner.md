# Panning Tool

> **Note — Panning in Qwt 7**
>
> `QwtPlotPanner` has been refactored for real-time interaction. Panning now moves the live plot content instead of the cached bitmap used by the original `QwtPlotPanner`.

The `Qwt6` version of `QwtPlotPanner` was based on a cache-based panning mechanism, which had two main issues:

- During panning, a cached bitmap was shown instead of the current plot content
- Data changes were not visible during the pan, so interactive exploration was limited

The `Qwt6` `QwtPlotPanner` reduced plot refreshes by caching, but the cached bitmap made panning feel static. `Qwt7` therefore refactored `QwtPlotPanner` so the plot updates while the mouse moves.

## Refactored QwtPlotPanner Improvements

The new `QwtPlotPanner` uses the `QwtPicker` state machine for panning and zooming, and updates the plot while the mouse moves:

The animation below shows panning in progress:

![qwt-realtime-panner](../../assets/screenshots/qwt-realtime-panner.gif)

The new `QwtPlotPanner` supports these axis scales:

- Linear Scale
- Logarithmic Scale
- DateTime Scale
- Multiple axes

> **Warning — Using the Original Cache-Based Picker**
>
> The original cache-based panner is available as `QwtPlotCachePanner`.

| Class Changes                  | Property | Base Class |
|---------------------------|------|-------|
|QwtPanner -> QwtCachePanner| Renamed  |QWidget|
|Original QwtPlotPanner -> QwtPlotCachePanner| Renamed  |QwtCachePanner|
|Original QwtPolarPanner -> QwtPolarCachePanner| Renamed  |QwtCachePanner|
|QwtPlotPanner| Refactored  |QwtPicker|

The `Qwt6` `QwtPlotPanner` was refactored, but its interface is unchanged, so existing code needs no modification.

## Basic Usage

The listing below attaches a `QwtPlotPanner` to the plot canvas, binds panning to the middle mouse button, and connects the `panned` signal to report each drag offset.

```cpp
#include "qwt_plot_panner.h"
#include "qwt_plot.h"
#include "qwt_plot_curve.h"

// Create plot and curve
QwtPlot* plot = new QwtPlot;
QwtPlotCurve* curve = new QwtPlotCurve("Sine Wave");
curve->attach(plot);

// Create real-time panner
QwtPlotPanner* panner = new QwtPlotPanner(plot->canvas());
// Configure mouse button
panner->setMouseButton(Qt::MiddleButton);  // Middle button panning
// panner->setMouseButton(Qt::LeftButton); // Or left button panning

// Enable picker
panner->setEnabled(true);

// Connect signal (optional)
connect(panner, &QwtPlotPanner::panned,
        [](int dx, int dy) {
            qDebug() << "Canvas panned by:" << dx << dy;
        });
```

You can set the panning direction to restrict movement to one direction:

```cpp
// 1. Set panning direction
panner->setOrientations(Qt::Horizontal | Qt::Vertical); // Default: horizontal and vertical
// panner->setOrientations(Qt::Horizontal);            // Horizontal only
// panner->setOrientations(Qt::Vertical);              // Vertical only
```

> **Note — Axis binding**
>
> `QwtPlotPanner` differs from `QwtPlotPicker` in that it does not need to bind to axes, because the movement applies to the entire canvas. All axes move with the canvas.