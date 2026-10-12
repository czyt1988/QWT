# Panning Tool

> **Success — New Feature**
>
> `QwtPlotPicker` is a completely refactored real-time interactive plot picker, providing a more intuitive user experience than the original `QwtPlotPicker` which implemented panning through caching.

The `Qwt6` version of `QwtPlotPicker` was based on a cache-based panning mechanism, which had two main issues:

- During panning, a cached bitmap was displayed instead of real-time updated plot content
- Users could not see real-time data changes during panning, preventing smooth interactive data exploration

Although in some aspects, the `Qwt6` `QwtPlotPicker` greatly reduced plot refreshes, it was very unfriendly to the visual experience. Therefore, `Qwt7` refactored `QwtPlotPicker` with the goal of achieving smooth interactive data exploration.

## Refactored QwtPlotPicker Improvements

The new `QwtPlotPicker` is based on the `QwtPicker` state machine mechanism, implementing real-time panning and zooming:

The effect is as follows:

![qwt-realtime-panner](../../assets/screenshots/qwt-realtime-panner.gif)

The new `QwtPlotPicker` provides complete axis support:

- ✅ Linear Scale
- ✅ Logarithmic Scale
- ✅ DateTime Scale
- ✅ Multiple axes

> **Warning — Using the Original Cache-Based Picker**
>
> If you want to use the original cache-based picker, you can use `QwtPlotCachePicker`.

| Class Changes                  | Property | Base Class |
|---------------------------|------|-------|
|QwtPanner -> QwtCachePanner| Renamed  |QWidget|
|Original QwtPlotPanner -> QwtPlotCachePanner| Renamed  |QwtCachePanner|
|Original QwtPolarPanner -> QwtPolarCachePanner| Renamed  |QwtCachePanner|
|QwtPlotPanner| Refactored  |QwtPicker|

The `Qwt6` `QwtPlotPanner` has been completely refactored, but the interface remains unchanged, so existing code basically requires no modifications.

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
connect(picker, &QwtPlotPicker::panned,
        [](int dx, int dy) {
            qDebug() << "Canvas panned by:" << dx << dy;
        });
```

You can set the panning direction to restrict movement to a single direction:

```cpp
// 1. Set panning direction
panner->setOrientations(Qt::Horizontal | Qt::Vertical); // Default: horizontal and vertical
// panner->setOrientations(Qt::Horizontal);            // Horizontal only
// panner->setOrientations(Qt::Vertical);              // Vertical only
```

> **Warning — Note**
>
> `QwtPlotPanner` differs from `QwtPlotPicker` in that it does not need to bind to axes, because the movement applies to the entire canvas. During canvas movement, all axes are moved.