# Overview

There are only a handful of plotting libraries in the Qt ecosystem, the mainstream ones being `QCustomPlot`, `Qwt`, `Qt Charts`, and `KDChart`.  
After Qt 6.8 the former `Qt Charts` (2-D) and Qt DataVisualization (3-D) were merged into a unified Qt Graphs module (note: **not** Qt Graphics). The new back-end is built entirely on Qt Quick Scene Graph (QSG) + Qt Quick 3D, completely abandoning the aging Graphics-View / QPainter pipeline.  
However, Qt Graphs must be embedded through `QQuickWidget` or `QQuickWindow`, which means the QML runtime is mandatory and C++ support is poor—[complaints on the forum](https://forum.qt.io/topic/159224/qt-graphs-building-2d-plot-using-c-only) are numerous.  
Although Qt Graphs is Qt’s official “unified” future, that future probably will not arrive within the next three years, and it drops support for older systems such as Windows 7 and is unfriendly to embedded devices.  
Therefore, `QCustomPlot`, `Qwt`, `Qt Charts`, and `KDChart` will remain the practical choices for the next few years.

- `QCustomPlot` is the simplest and most attractive, and enjoys the widest adoption.  
  Just include `qcustomplot.h` and `qcustomplot.cpp` and you are ready to go ([official docs](https://www.qcustomplot.com/index.php/documentation)).  
  It also supports Qt 6.  
  Its biggest drawback, however, is the **GPL** license, which is highly “viral”: any program that uses `QCustomPlot` must itself be GPL—a deal-breaker for commercial use.

- `Qwt` is a veteran plotting library ([official docs](https://qwt.sourceforge.io/index.html)) with solid performance, yet its deployment difficulty deters many users.  
  It is licensed under **LGPL**, which is relatively commercial-friendly.

- `Qt Charts` is Qt’s own plotting package ([official docs](https://doc.qt.io/qt-5/qtcharts-index.html)) but its performance is poor—arguably very low—and unsuitable for scientific computing.  
  Worse, Qt Charts has **no LGPL option**; the open-source version is **GPL v3**, so using it in a project forces the entire project to be open-sourced under GPL v3.

- `KDChart` is KDAB’s plotting library ([official docs](https://www.kdab.com/software-technologies/developer-tools/kd-chart/)).  
  Starting with **KDChart 3.0** it is **MIT-licensed**, making it extremely commercial-friendly.  
  Its rendering style, however, is mediocre—reminiscent of Excel 2003.  
  A unique feature is **Gantt charts**, unavailable in the other three.

Hence, for commercial projects you are effectively limited to `Qwt` and `KDChart 3.0`.  
Because the `Qwt` author has ceased maintenance, I personally prefer `Qwt`: its architecture conforms better to software-engineering principles and its large-scale rendering performance is superior.  
`QCustomPlot` delivers out-of-the-box interactive features such as mouse zooming and axis scaling, whereas `Qwt` requires more code to achieve the same, yet it offers finer-grained control.  
When my own projects need plotting I therefore choose `Qwt`, enhancing and optimizing it with the features I need—hence this project.

**Documentation**：[https://czyt1988.github.io/QWT/](https://czyt1988.github.io/QWT/)

## Qwt7.0

I have taken over maintenance of the final official `Qwt` release, adding the features I need while gradually refining existing ones, e.g., its outdated default styling.

Project repositories:

- [GitHub: https://github.com/czyt1988/QWT](https://github.com/czyt1988/QWT)  
- [Gitee: https://gitee.com/czyt1988/QWT](https://gitee.com/czyt1988/QWT)

Goals and current progress:

- [x] CMake support  
- [x] Qt 6 support  
- [x] C++11 modernization  
- [x] Merged into single header/source for easier inclusion  
- [x] Provide integrated interaction helpers for simpler usage  
- [x] Modernized visual style  
- [x] Figure class for layout management  
- [x] Add parasite-axis support for unlimited axes
- [x] Axis interaction (drag and scroll-wheel zoom on axes)
- [x] Data picking with `QwtPlotSeriesDataPicker`
- [x] Real-time canvas panner with multi-axis support (`QwtPlotPanner`)
- [x] Canvas zoomer without axis binding (`QwtPlotCanvasZoomer`)
- [x] Box chart (`QwtPlotBoxChart`)
- [x] Curve downsampling with 4 algorithms + SIMD acceleration
- [x] Color cycle system (`QwtColorCycle`)
- [x] Arrow marker (`QwtPlotArrowMarker`)
- [x] Inside ticks support (`TickInside` / `TickOutside`)
- [x] Flat-style controls (sliders, knobs, dials, etc.)
- [x] Modular architecture: core (foundational utilities) + plot (2D plotting) + plot3d (3D plotting)
- [x] Matplotlib-style plotting API (`QwtPyPlot`)
- [x] 3D theme system with 10 presets + 5 lighting presets
- [x] 22 scientific colormap presets (viridis, plasma, jet, turbo, ...)
- [x] Standalone `qwt::core` foundational library (28 modules, no plotting dependency)

In short, I will keep maintaining `Qwt` so it becomes a license-friendly, high-performance, and easy-to-use Qt plotting library.

## New Features in Qwt7.0

### CMake Support

Qwt7.0 now supports **CMake**; `qmake` may be dropped in the future.  
After installing Qwt you can simply link it in your project:

```cmake
find_package(qwt)
# 2D plotting (automatically links qwt::core)
target_link_libraries(${YOUR_APP_TARGET} PUBLIC qwt::plot)
# 3D plotting (automatically links qwt::core)
target_link_libraries(${YOUR_APP_TARGET} PUBLIC qwt::plot3d)
# Foundational utilities only (math, data types, geometry, transforms, etc.)
target_link_libraries(${YOUR_APP_TARGET} PUBLIC qwt::core)
```

### Single Header & Source File

Following the example of `QCustomPlot`, I have merged the entire `Qwt` library into `QwtPlot.h` and `QwtPlot.cpp`.  
Drop these two files into your project and you are ready to go.

Example `CMakeLists.txt`:

```cmake
# QwtPlot requires Core Gui Widgets Svg Concurrent OpenGL PrintSupport
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.12 COMPONENTS Core Gui Widgets Svg Concurrent OpenGL PrintSupport REQUIRED)

add_executable(YOUR_APP_TARGET
    main.cpp
    QwtPlot.h
    QwtPlot.cpp
)

target_link_libraries(YOUR_APP_TARGET
    PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::PrintSupport
)
```

### Using qmake with Qwt 7.0

Qwt 7 no longer uses `qmake` as its primary build system, but `qmake` integration is still fully supported. See the `examples/qmakeExample` example for a working demonstration, or refer to the documentation: [qmake Single-File Build](docs/zh/build-guide/qmake-single-file.md)

### Modernized Visual Style

The original Qwt style used an outdated beveled look inconsistent with modern aesthetics.  
I therefore redesigned it.  
Qwt 6.3:

![qwt6.3 effect](./docs/assets/picture/bode-old.png)

Qwt 7.0:

![qwt7.0 effect](./docs/assets/screenshots/bode.png)

Key changes: removed the default sunken style, placed axes flush against the plot area, overall appearance now aligns with contemporary design.

### Added Figure Container

Similar to Python's matplotlib, Qwt provides a Figure plotting container that enables convenient layout of multiple plots.

With the newly added `QwtFigure` class, arranging multiple plots becomes effortless, supporting grid layouts (analogous to matplotlib's subplot).

The `QwtFigureWidgetOverlay` class has been configured synchronously, integrating several operations of `QwtFigure`—such as adjusting plot sizes and moving plots.

For details, refer to the tutorial: [Figure Plotting Container](https://czyt1988.github.io/QWT/zh/use-guide/figure-widget/)

### Added Parasite Axes

Qwt7 now supports **parasite axes**, allowing multiple plots to share the same axes.

For details, refer to the tutorial:[Creating Multiple Coordinate Axes](https://czyt1988.github.io/QWT/zh/use-guide/parasite-axes/)

### Added Axis Interactions

Axis interaction functionality has been added, supporting **drag-and-drop** and **mouse wheel zoom** directly on the axes.

**Panning:**
![qwt-scale-builtin-action-pan](./docs/assets/screenshots/qwt-scale-builtin-action-pan.gif)

**Zooming:**
![qwt-scale-builtin-action-zoom](./docs/assets/screenshots/qwt-scale-builtin-action-zoom.gif)


For details, refer to the tutorial:[Coordinate Axis Interactive Actions](https://czyt1988.github.io/QWT/zh/use-guide/scale-builtin-action/)

### Added Data Picking Functionality

Added the QwtPlotSeriesDataPicker class to support data picking.

![series-data-picker-yvalue](./docs/assets/picture/series-data-picker-yvalue.gif)

![series-data-picker-nearest-value](./docs/assets/picture/series-data-picker-nearest-value.gif)

For details, refer to the tutorial:[Series Data Picker](https://czyt1988.github.io/QWT/zh/use-guide/pick-value/)

### Enhanced Canvas Panning

Refactored `QwtPlotPicker` to enable real-time canvas panning; the original implementation has been renamed `QwtPlotCachePanner`.  
The new panner simultaneously supports dragging on any number of axes.

Preview:

![qwt-realtime-panner](./docs/assets/screenshots/qwt-realtime-panner.gif)

For details, refer to the tutorial: [Panning](https://czyt1988.github.io/QWT/zh/use-guide/panner/)

### New Canvas Zoomer

The legacy `QwtPlotZoomer` requires you to specify two axes for every zoom operation.  
If your plot uses four axes you must attach two zoomers—cumbersome and multi-axis-unfriendly.

The new `QwtPlotCanvasZoomer` needs no axis specification; it zooms the entire canvas as a whole and works seamlessly with any number of axes.

For details, refer to the tutorial: [Zoomer](https://czyt1988.github.io/QWT/zh/use-guide/zoomer/)

### Box Chart (Boxplot) Support

Added `QwtPlotBoxChart` class for creating box-and-whisker plots to visualize statistical distributions:

- Support for both pre-computed statistics and raw data input
- Automatic calculation of statistics (median, quartiles, outliers)
- Multiple box styles: Rectangle, Diamond, Notch
- Vertical and horizontal orientations
- Outlier detection with customizable symbols

See documentation: [Box Chart Guide](https://czyt1988.github.io/QWT/zh/use-guide/boxchart/)

### Inside Ticks

Added `QwtPlot::setAxisTickDirection()` to draw axis tick marks from the canvas edge toward the interior (ticks inside), while the backbone and tick labels remain outside — useful for compact layouts or distinctive visual styles:

- Each axis can be set independently to `TickInside` or `TickOutside` (default)
- Major, medium, and minor ticks are all drawn inside, inheriting the length, color, and pen width of the outside ticks
- Works with logarithmic and inverted scales — inside ticks render correctly under every scale engine
- Can be combined with parasite axes, where each parasite plot sets its own tick direction

```cpp
// Draw YLeft and XBottom ticks inside
plot->setAxisTickDirection(QwtAxis::YLeft, QwtPlot::TickInside);
plot->setAxisTickDirection(QwtAxis::XBottom, QwtPlot::TickInside);
plot->replot();
```

![ticks_inside](./docs/assets/screenshots/ticks_inside.png)

For details, refer to the tutorial: [Inside Ticks Guide](https://czyt1988.github.io/QWT/zh/use-guide/ticks-inside/). Example code is located in `examples/2D/ticks_inside`.

### Matplotlib-style Plotting API (`QwtPyPlot`)

New in 7.3, `QwtPyPlot` is a high-level, stateful API inspired by Python's `matplotlib.pyplot`. It wraps a `QwtFigure` (or a single `QwtPlot`) and delegates to the existing Qwt infrastructure, so you get matplotlib-style brevity without giving up Qwt's power and fine-grained control.

Familiar one-liners just work — `plot()`, `scatter()`, `bar()`, `hist()`, `imshow()`, `contour()`, `quiver()`, `fillBetween()`, `errorbar()`, `candlestick()` — together with `subplot()`, `twinx()`/`twiny()`, `savefig()`, and matplotlib format strings such as `"r-o"` (red line + circle markers):

```cpp
#include <QwtFigure>
#include <QwtPyPlot>

QwtFigure* fig = new QwtFigure;
QwtPyPlot plt(fig);

plt.subplot(2, 1, 1);
plt.plot(x, y, "r-o", "Temperature");
plt.setTitle("Sensor Data");
plt.grid(true);
plt.legend();

plt.subplot(2, 1, 2);
plt.bar({10, 20, 30, 40}, "b", "Sales");

plt.savefig("output.png", 300);
fig->show();
```

For details, refer to the tutorial: [QwtPyPlot — Matplotlib-style API](https://czyt1988.github.io/QWT/zh/use-guide/pyplot-api/)

### 3D Plotting with Theme System

The integrated 3D module (`qwt::plot3d`) uses a **Plot + Item** architecture (`Qwt3DPlot` + `Qwt3DPlotItem`), symmetric with the 2D module. It provides surface plots, mesh plots, and function plots with modern OpenGL rendering (VBO/VAO + GLSL shaders) and full mouse interaction (rotate / zoom / pan).

New in 7.3, the `Qwt3DTheme` system packages every visual property — background, mesh, colormap, axes, title, lighting, shading, and material — into a single descriptor, so a complete look can be applied in one call:

```cpp
// Create plot + surface item
Qwt3DPlot* plot = new Qwt3DPlot();
Qwt3DSurface* surface = new Qwt3DSurface();
surface->attach(plot);

// One-call preset
plot->applyTheme(Qwt3DTheme::Dark);

// Or start from a preset and customize
Qwt3DTheme theme(Qwt3DTheme::Scientific);
theme.setDataColorPreset("plasma");
theme.setLightingPreset(Qwt3DTheme::Studio);
theme.setShininess(20.0);
theme.apply(plot);
```

- 10 built-in themes: `Default`, `Dark`, `Scientific`, `Warm`, `Cool`, `Matplotlib`, `EarthTones`, `Ocean`, `HighContrast`, `Presentation`
- 5 lighting presets: `NoLighting`, `FlatLight`, `Studio`, `Outdoor`, `Soft`
- A `Qwt3DColorMapColor` adapter bridges the core colormaps to 3D surfaces, so all 22 scientific presets work in 3D as well

For details, refer to the tutorial: [3D Plotting](https://czyt1988.github.io/QWT/zh/use-guide/3d-plot/)

### Scientific Colormap Presets

`qwt::core` ships `QwtColorMapPreset` — 22 well-known scientific colormaps (the same names used by matplotlib / MATLAB), shared by 2D spectrograms and 3D surfaces:

`viridis`, `plasma`, `inferno`, `magma`, `cividis`, `jet`, `hot`, `cool`, `spring`, `summer`, `autumn`, `winter`, `gray`, `bone`, `copper`, `rainbow`, `hsv`, `turbo`, `coolwarm`, `rdylbu`, `rdylgn`, `spectral`

```cpp
auto cmap = QwtColorMapPreset::create("viridis");
// cmap->rgb(vMin, vMax, value) returns the color for a given data value
```

The perceptually uniform maps (`viridis`, `plasma`, `inferno`, `magma`, `cividis`) are colorblind-friendly and print-safe — ideal for scientific publication.

### SIMD-Accelerated Curve Downsampling

Rendering millions of points smoothly is now first-class. `QwtPlotCurve` offers four downsampling filters, and the MinMax-bucket path is accelerated with SIMD (SSE2 / AVX2 / NEON, auto-detected at runtime):

- `FilterPoints` — the original weeding algorithm
- `FilterPointsAggressive` — fast weeding for huge monotonic datasets 
- `FilterPointsPixel` — per-pixel-column reduction for pixel-aligned trend observation
- `FilterPointsLTTB` — shape-preserving and best-performing LTTB (MinMax bucket) (new default)

The result: million-point curves render at interactive frame rates. Benchmark it yourself with the `renderbench` example.

For details, refer to the tutorial: [Curve Downsampling](https://czyt1988.github.io/QWT/zh/use-guide/curve-downsampling/)

### Standalone Core Foundational Library

`qwt::core` (`qwtcore.dll` / `libqwtcore.so`) is a complete foundational toolkit of 28 modules — usable on its own, with no plotting code pulled in:

- Math & SIMD: `qwt_math`, `qwt_simd_argminmax`
- Data types: `QwtInterval`, `QwtPoint3D`, `QwtPointPolar`, `QwtSamples`, `QwtBoxStatistics`
- Geometry: `QwtBezier`, `QwtClipper`
- Coordinate transforms: `QwtTransform`, `QwtScaleMap`, `QwtScaleDiv`, `QwtScaleEngine`
- Date / time: `QwtDate`, `QwtSystemClock`
- Colormaps & color cycle: `QwtColorMap`, `QwtColorMapPreset`, `QwtColorCycle`
- Data series & raster: `QwtSeriesData`, `QwtPointData`, `QwtSeriesStore`, `QwtRasterData`, `QwtMatrixRasterData`, `QwtGridRasterData`

Link only what you need:

```cmake
# Just the foundational utilities — no plotting code pulled in
target_link_libraries(${YOUR_APP_TARGET} PUBLIC qwt::core)
```

### Other Modifications

- Fixed the impact of `NAN` and `INF` values on plotting
- `QwtPlotSeriesDataPicker` now picks from every series item type — trading curves, interval curves, histograms, vector fields, bar / box charts, and spectro curves
- Modern C++ throughout the codebase: `override` / `final`, `nullptr`, move semantics, `constexpr`, `noexcept`, range-based for, and a unified PIMPL macro pattern (180+ files)
- `renderbench` example for measuring curve-rendering performance and generating reports

## Changelog

See [CHANGES.MD](./CHANGES.md) for detailed logs.

## Gallery

### Basic Charts

|  |  |  |
|:---:|:---:|:---:|
|![Figure Widget](./docs/assets/picture/figure-scale-aligment.png)|![Simple Plot](./docs/assets/screenshots/simpleplot.png)|![Bar Chart - Grouped](./docs/assets/screenshots/BarChart-grouped.png)|
|`examples/figure`|`examples/simpleplot`|`examples/simpleplot`|

|  |  |  |
|:---:|:---:|:---:|
|![Bar Chart - Stacked](./docs/assets/screenshots/BarChart-stacked.png) |![Scatter Plot](./docs/assets/screenshots/scatterplot.png)  |![Curve Demo](./docs/assets/screenshots/curvedemo.png)  |
|`examples/barchart`  |`examples/scatterplot`  |`examples/curvedemo` |

### Real-Time Visualization

|  |  |  |
|:---:|:---:|:---:|
|![CPU Monitor](./docs/assets/screenshots/cpuplot.png)|![Real-Time Plot](./docs/assets/screenshots/realtime.png)|![Oscilloscope](./docs/assets/screenshots/oscilloscope.png)  |
|`examples/cpuplot`|`examples/realtime`|`examples/oscilloscope` |

### Advanced Charts

|  |  |  |
|:---:|:---:|:---:|
|![Polar Demo](./docs/assets/screenshots/polardemo.png)|![Spectrogram](./docs/assets/screenshots/spectrogram.png)|![Contour Spectrogram](./docs/assets/screenshots/spectrogram-contour.png) |
|`examples/polardemo`|`examples/spectrogram`|`examples/spectrogram`|

|  |  |  |
|:---:|:---:|:---:|
|![Vector Field](./docs/assets/screenshots/vectorfield.png)|![Stock Chart](./docs/assets/screenshots/stockchart.png)|![Bode Plot](./docs/assets/screenshots/bode.png)| 
|`playground/vectorfield`|`examples/stockchart`|`examples/bode`|

|  |  |  |
|:---:|:---:|:---:|
|![Tube Display](./docs/assets/screenshots/friedberg-tube.png)|![plot matrix](./docs/assets/screenshots/plotmatrix.png)|![Scale Engine](./docs/assets/screenshots/scaleengine.png)|
|`examples/friedberg`|`playground/plotmatrix`|`playground/scaleengine`|

|  |  |  |
|:---:|:---:|:---:|
|![Rescaler](./docs/assets/screenshots/rescaler.png)|![Graphics Scale](./docs/assets/screenshots/graphicscale.png)|![Spline Editor](./docs/assets/screenshots/splineeditor.png)|
|`playground/rescaler`|`playground/graphicscale`|`examples/splineeditor`|

|  |  |  |
|:---:|:---:|:---:|
|![Ticks Inside](./docs/assets/screenshots/ticks_inside.png)|![Box Chart](./docs/assets/screenshots/BoxChart.png)|![parasite plot](./docs/assets/screenshots/parasite-plot.png)|
|`examples/ticks_inside`|`examples/boxchart`|`examples/parasitePlot`|

|  |  |  |
|:---:|:---:|:---:|
|![System Info](./docs/assets/screenshots/sysinfo.png)|![Distro Watch](./docs/assets/screenshots/distrowatch.png)|![Raster View 1](./docs/assets/screenshots/rasterview-1.png)|
|`examples/sysinfo`|`examples/distrowatch`|`examples/rasterview`|
  
|  |  |  |
|:---:|:---:|:---:|
|![Raster View 2](./docs/assets/screenshots/rasterview-2.png)|![SVG Map](./docs/assets/screenshots/svgmap.png)|![Item Editor](./docs/assets/screenshots/itemeditor.png)|
|`examples/rasterview`|`playground/svgmap`|`examples/itemeditor`|

### 3D Visualization

|  |  |  |
|:---:|:---:|:---:|
|![Simple 3D Plot](./docs/assets/screenshots-3D/simpleplot3D.png)|![3D Axes](./docs/assets/screenshots-3D/axes.png)|![3D Enrichments](./docs/assets/screenshots-3D/enrichments.png)|
|`examples/3D/simpleplot3D`|`examples/3D/axes`|`examples/3D/enrichments`|

|  |  |  |
|:---:|:---:|:---:|
|![3D Autoswitch](./docs/assets/screenshots-3D/autoswitch.png)||
|`examples/3D/autoswitch`||

### Animated Demos

|  |  |  |
|:---:|:---:|:---:|
|![Animated Demo](./docs/assets/screenshots/animated.png)|![Curve Tracker](./docs/assets/screenshots/curvetracker.png)|![Refresh Test](./docs/assets/screenshots/refreshtest.png)|
|`examples/animated`|`playground/curvetracker`|`examples/refreshtest`|

### Styles & Symbols

|  |  |  |
|:---:|:---:|:---:|
|![Legends Demo](./docs/assets/screenshots/legends.png)|![Symbols Demo](./docs/assets/screenshots/symbols.png)|![Shape Rendering](./docs/assets/screenshots/shapes.png)|
|`examples/legends`|`playground/symbols`|`playground/shapes`|

### Control Widgets


|  |  |  |
|:---:|:---:|:---:|
|![Dial Controls](./docs/assets/screenshots/controls-dial.png)|![Knob Controls](./docs/assets/screenshots/controls-knob.png)|![Slider Controls](./docs/assets/screenshots/controls-sliders.png)|
|`examples/controls`|`examples/controls`|`examples/controls`|
 
|  |  |  |
|:---:|:---:|:---:|
|![Wheel & Thermo](./docs/assets/screenshots/controls-wheel-thermo.png)|![Radio Buttons](./docs/assets/screenshots/radio.png)|![Time Scale Demo](./docs/assets/screenshots/timescale.png)|
|`examples/controls`|`examples/radio`|`playground/timescale`|

### Instrument Panels

|  |  |  |
|:---:|:---:|:---:|
|![Compass Dial](./docs/assets/screenshots/dials-compass.png)|![Cockpit Dials](./docs/assets/screenshots/dials-cockpit.png)||
|`examples/dials`|`examples/dials`||

## Copyright

    Qwt Widget Library
    Copyright (C) 1997   Josef Wilgen
    Copyright (C) 2002   Uwe Rathmann

    Qwt is published under the Qwt License, Version 1.0.
    You should have received a copy of this licence in the file
    COPYING.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.