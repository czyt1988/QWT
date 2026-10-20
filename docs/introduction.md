---
title: QWT Overview
description: QWT is a high-performance Qt plotting library supporting Qt6, licensed under LGPL, designed for scientific computing and engineering data visualization.
keywords: [QWT, Qt plotting library, scientific charts, data visualization, Qt6, LGPL, C++ plotting]
---

# QWT — Qt Plotting Library

QWT (Qt Widgets for Technical Applications) is a 2D/3D plotting library built on Qt, intended for data visualization in scientific computing and engineering.

## Qwt in the Qt plotting ecosystem

There are only a handful of plotting libraries in the Qt ecosystem. The mainstream options are `QCustomPlot`, `Qwt`, `Qt Charts`, and `KDChart`. After Qt 6.8, the former `Qt Charts` (2D) and `Qt DataVisualization` (3D) were merged into a unified Qt Graphs module built entirely on Qt Quick Scene Graph + Qt Quick 3D, abandoning the legacy Graphics-View / QPainter pipeline. However, Qt Graphs must be embedded via `QQuickWidget` or `QQuickWindow`, requires the QML runtime, has limited C++ support, and drops support for older systems like Windows 7.

| Library | License | Strengths | Weaknesses |
|---------|---------|-----------|------------|
| **QCustomPlot** | GPL | Simple API, widely used | GPL requires opening the application source |
| **Qwt** | LGPL | Fast rendering, clean architecture | Original author stopped maintaining; deployment was difficult |
| **Qt Charts** | GPL | Official Qt module | Low rendering performance; GPL |
| **KDChart** | MIT (3.0+) | Commercial-friendly license | Limited rendering quality |

This project is based on Qwt 6.2.0. It keeps the original architecture and adds CMake support, Qt 6 compatibility, single-file inclusion, 3D plotting, large-data rendering and other fixes.

## Qwt 7.0 New Features

- CMake support: `find_package(qwt)` in one line
- Qt 6 support, compatible with Qt 5.12+ and Qt 6.x
- Single-file inclusion: add `QwtPlot.h` and `QwtPlot.cpp` to the project
- Flat visual style, replacing the legacy embossed look
- Figure container for matplotlib-style multi-plot layouts
- Parasite axes, with no fixed limit on the number of axes
- Mouse drag and wheel zoom on axes
- Built-in 3D plotting module
- C++11 throughout: `override`, `nullptr`, smart pointers, range-based for loops
- 4 curve downsampling algorithms plus SIMD acceleration
- Color cycle that assigns colors to series automatically
- `QwtPlotBoxChart` for statistical distributions
- Flat styling for sliders, knobs, dials and other controls

## Quick Integration

With CMake (recommended):


```cmake
find_package(qwt REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE qwt::plot)
# 3D plotting
target_link_libraries(${PROJECT_NAME} PRIVATE qwt::plot3d)
```

With the single-file amalgamation:


```cpp
// Add src-amalgamate/QwtPlot.h and QwtPlot.cpp to your project
#include "QwtPlot.h"

auto* plot = new QwtPlot("My Plot");
auto* curve = new QwtPlotCurve("Data");
```

## Project Links

- **GitHub**: [https://github.com/czyt1988/QWT](https://github.com/czyt1988/QWT)
- **Gitee**: [https://gitee.com/czyt1988/QWT](https://gitee.com/czyt1988/QWT)
- **Documentation**: [https://czyt1988.github.io/QWT/](https://czyt1988.github.io/QWT/)

## Showcase

### Basic Charts

<div class="grid cards" markdown>

- ![Figure Widget](assets/screenshots/qwt_figure.png)
  `examples/2D/figure`

- ![Simple Plot](assets/screenshots/simpleplot.png)
  `examples/2D/simpleplot`

- ![Bar Chart](assets/screenshots/BarChart-grouped.png)
  `examples/2D/barchart`

- ![Scatter Plot](assets/screenshots/scatterplot.png)
  `examples/2D/scatterplot`

- ![Curve Demo](assets/screenshots/curvedemo.png)
  `examples/2D/curvedemo`

- ![Box Chart](assets/screenshots/BoxChart.png)
  `examples/2D/boxchart`

</div>

### Real-Time Visualization

<div class="grid cards" markdown>

- ![CPU Monitor](assets/screenshots/cpuplot.png)
  `examples/2D/cpuplot`

- ![Real-Time Plot](assets/screenshots/realtime.png)
  `examples/2D/realtime`

- ![Oscilloscope](assets/screenshots/oscilloscope.png)
  `examples/2D/oscilloscope`

- ![Radio](assets/screenshots/radio.png)
  `examples/2D/radio`

- ![Sysinfo](assets/screenshots/sysinfo.png)
  `examples/2D/sysinfo`

- ![Animated](assets/screenshots/animated.png)
  `examples/2D/animation`

</div>

### Advanced Charts

<div class="grid cards" markdown>

- ![Spectrogram](assets/screenshots/spectrogram.png)
  `examples/2D/spectrogram`

- ![Vector Field](assets/screenshots/vectorfield.png)
  `playground/vectorfield`

- ![Stock Chart](assets/screenshots/stockchart.png)
  `examples/2D/stockchart`

- ![Polar Demo](assets/screenshots/polardemo.png)
  `examples/2D/polardemo`

- ![Parasite Plot](assets/screenshots/parasite-plot.png)
  `examples/2D/parasitePlot`

- ![Bode Plot](assets/screenshots/bode.png)
  `examples/2D/bode`

- ![Shapes](assets/screenshots/shapes.png)
  `playground/shapes`

- ![Scale Engine](assets/screenshots/scaleengine.png)
  `playground/scaleengine`

- ![PyPlot](assets/screenshots/pyplot.png)
  `examples/2D/pyplot`

</div>

### More Examples

<div class="grid cards" markdown>

- ![Distrowatch](assets/screenshots/distrowatch.png)
  `examples/2D/distrowatch`

- ![Friedberg Bar](assets/screenshots/friedberg-bar.png)
  `examples/2D/friedberg`

- ![Friedberg Tube](assets/screenshots/friedberg-tube.png)
  `examples/2D/friedberg`

- ![Item Editor](assets/screenshots/itemeditor.png)
  `examples/2D/itemeditor`

- ![Rasterview 1](assets/screenshots/rasterview-1.png)
  `examples/2D/rasterview`

- ![Rasterview 2](assets/screenshots/rasterview-2.png)
  `examples/2D/rasterview`

- ![Refresh Test](assets/screenshots/refreshtest.png)
  `examples/2D/refreshtest`

- ![Spline Editor](assets/screenshots/splineeditor.png)
  `examples/2D/splineeditor`

- ![TV Plot](assets/screenshots/tvplot.png)
  `examples/2D/tvplot`

- ![Curve Tracker](assets/screenshots/curvetracker.png)
  `playground/curvetracker`

- ![Graphic Scale](assets/screenshots/graphicscale.png)
  `playground/graphicscale`

- ![Plot Matrix](assets/screenshots/plotmatrix.png)
  `playground/plotmatrix`

- ![Rescaler](assets/screenshots/rescaler.png)
  `playground/rescaler`

- ![SVG Map](assets/screenshots/svgmap.png)
  `playground/svgmap`

- ![Time Scale](assets/screenshots/timescale.png)
  `playground/timescale`

</div>

### Interactive Demos

<div class="grid cards" markdown>

- ![Axis Pan](assets/screenshots/qwt-scale-builtin-action-pan.gif)
  Axis Dragging

- ![Axis Zoom](assets/screenshots/qwt-scale-builtin-action-zoom.gif)
  Axis Zooming

- ![Figure Overlay](assets/screenshots/figure-widget-overlay.gif)
  Figure Overlay

- ![Series Data Picker](assets/screenshots/series-data-picker.png)
  Data Picker

</div>

## License

Qwt is published under the Qwt License, Version 1.0. The full copyright notice is reproduced below:

```
Qwt Widget Library
Copyright (C) 1997   Josef Wilgen
Copyright (C) 2002   Uwe Rathmann

Qwt is published under the Qwt License, Version 1.0.
```
