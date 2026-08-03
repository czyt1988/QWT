# QWT API Documentation (English) {#mainpage}

## Overview

**QWT** (Qt Widgets for Technical Applications) is a high-performance 2D/3D plotting library based on Qt,
designed for data visualization in scientific computing and engineering applications.

- **License**: LGPL (commercial-friendly)
- **Qt Support**: Qt 5.12+ and Qt 6
- **C++ Standard**: C++11 (C++17 for Qt6)
- **Build System**: CMake

## Core 2D Plotting Module

| Class | Description |
|-------|-------------|
| `QwtPlot` | 2D plot widget, manages layout, axes, legend, and canvas |
| `QwtPlotCurve` | Data curve item, supports various styles (lines, steps, dots, etc.) |
| `QwtPlotGrid` | Grid item, provides major/minor grid lines |
| `QwtLegend` | Legend container |
| `QwtPlotMarker` | Marker item |
| `QwtPlotSpectrogram` | Spectrogram item |
| `QwtPlotBarChart` | Bar chart item |
| `QwtPlotVectorField` | Vector field item |

## Layout & Containers

| Class | Description |
|-------|-------------|
| `QwtFigure` | Multi-plot layout container (similar to matplotlib Figure) |
| `QwtFigureWidgetOverlay` | Figure interaction overlay, supports drag and resize |
| `QwtPlotLayout` | Single plot layout manager |
| `QwtParasitePlotLayout` | Parasite plot layout manager |

## Interaction Controls

| Class | Description |
|-------|-------------|
| `QwtPlotPanner` | Real-time canvas panning (supports multiple axes) |
| `QwtPlotCachePanner` | Cached panning (original QwtPanner) |
| `QwtPlotCanvasZoomer` | Whole-canvas zoom (supports multiple axes) |
| `QwtPlotAxisZoomer` | Axis zoom (original QwtPlotZoomer) |
| `QwtPlotMagnifier` | Magnifier |
| `QwtPlotSeriesDataPicker` | Data picker |
| `QwtPicker` | Base picker |
| `QwtScaleWidget` | Scale widget (supports built-in pan/zoom actions) |

## 3D Plotting Module

All 3D classes use the `Qwt3D` prefix in the global scope (no namespace).

| Class | Description |
|-------|-------------|
| `Qwt3DPlot` | 3D rendering window (QOpenGLWidget), manages GL context, view, lighting, coordinate system, and item list |
| `Qwt3DPlotItem` | Abstract base class for all 3D plot items |
| `Qwt3DSurface` | 3D surface plot item (handles both grid and cell data) |
| `Qwt3DFunction` | 3D function plot data generator |
| `Qwt3DParametricSurface` | Parametric surface data generator |
| `Qwt3DCoordinateSystem` | 3D coordinate system with 12 axes |
| `Qwt3DColorLegend` | 3D color legend |
| `Qwt3DTheme` | 3D theme system (v7.3.1+) |

## Key Architecture Concepts

### Parasite Plot

Parasite plots allow creating multiple independent axis systems within the same plot area:

- Created via `QwtPlot::createParasitePlot()`
- Shares the plot area with the host plot
- Supports arbitrary number of X/Y axes
- Automatic lifecycle management

### Axis System

Qwt uses `QwtAxis::Position` enum to identify axis positions:

- `QwtAxis::XBottom`, `QwtAxis::XTop`
- `QwtAxis::YLeft`, `QwtAxis::YRight`

## Project Integration

### CMake Package (Recommended)

```cmake
find_package(qwt REQUIRED)
# 2D plotting
target_link_libraries(${PROJECT_NAME} PRIVATE qwt::plot)
# 3D plotting
target_link_libraries(${PROJECT_NAME} PRIVATE qwt::plot3d)
```

### Single File Integration

Add `QwtPlot.h` and `QwtPlot.cpp` from `src-amalgamate/` to your project.

Required Qt modules: `Core`, `Gui`, `Widgets` (public); `Concurrent`, `PrintSupport` (private). Optional: `Svg` and `OpenGL`/`OpenGLWidgets` (gated by `QWT_CONFIG_QWTSVG` / `QWT_CONFIG_QWTOPENGL`).
Qt6 additional: `OpenGLWidgets`
3D features require: `OpenGL` + `OpenGLWidgets` (Qt6). GLU is only needed for gl2ps vector export.

## Copyright

```
Qwt Widget Library
Copyright (C) 1997   Josef Wilgen
Copyright (C) 2002   Uwe Rathmann

Qwt is published under the Qwt License, Version 1.0.
You should have received a copy of this licence in the file COPYING.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```