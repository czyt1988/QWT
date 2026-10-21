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

## Qwt 7

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
- [x] Integrated 3D plotting
- [x] 3D items: surface, bar chart, line plot, sliding-window waterfall, function surfaces
- [x] Box chart (`QwtPlotBoxChart`)
- [x] Curve downsampling with 6 algorithms + SIMD acceleration
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

![What you can build with Qwt 7 - a live 3D waterfall with a 2D spectrum on top](./docs/assets/screenshots-3D/waterfall3D.png)

## New Features in Qwt 7

### CMake Support

Qwt 7 uses **CMake** as its primary build system and no longer maintains `.pro` project files, but `qmake` integration remains fully supported (see below).
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

if(QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 COMPONENTS OpenGLWidgets REQUIRED)
    target_link_libraries(YOUR_APP_TARGET PRIVATE Qt6::OpenGLWidgets)
endif()

# gl2ps (3D vector export) needs GLU
find_package(OpenGL REQUIRED)
target_link_libraries(YOUR_APP_TARGET PRIVATE OpenGL::GLU)
```

### Using qmake with Qwt 7

Qwt 7 no longer uses `qmake` as its primary build system, but `qmake` integration is still fully supported. See the `examples/qmakeExample` example for a working demonstration, or refer to the documentation: [qmake Single-File Build](docs/build-guide/qmake-single-file.md)

### Modernized Visual Style

The original Qwt style used an outdated beveled look inconsistent with modern aesthetics.  
I therefore redesigned it.  
Qwt 6.3:

![qwt6.3 effect](./docs/assets/picture/bode-old.png)

Qwt 7:

![qwt7 effect](./docs/assets/screenshots/bode.png)

Key changes: removed the default sunken style, placed axes flush against the plot area, overall appearance now aligns with contemporary design.

### Added Figure Container

Similar to Python's matplotlib, Qwt provides a Figure plotting container that enables convenient layout of multiple plots.

With the newly added `QwtFigure` class, arranging multiple plots becomes effortless, supporting grid layouts (analogous to matplotlib's subplot).

The `QwtFigureWidgetOverlay` class has been configured synchronously, integrating several operations of `QwtFigure`—such as adjusting plot sizes and moving plots.

For details, refer to the tutorial: [Figure Plotting Container](https://czyt1988.github.io/QWT/use-guide/2d/figure-widget/)

### Added Parasite Axes

Qwt 7 now supports **parasite axes**, allowing multiple plots to share the same axes.

For details, refer to the tutorial:[Creating Multiple Coordinate Axes](https://czyt1988.github.io/QWT/use-guide/2d/parasite-axes/)

### Added Axis Interactions

Axis interaction functionality has been added, supporting **drag-and-drop** and **mouse wheel zoom** directly on the axes.

**Panning:**
![qwt-scale-builtin-action-pan](./docs/assets/screenshots/qwt-scale-builtin-action-pan.gif)

**Zooming:**
![qwt-scale-builtin-action-zoom](./docs/assets/screenshots/qwt-scale-builtin-action-zoom.gif)


For details, refer to the tutorial:[Coordinate Axis Interactive Actions](https://czyt1988.github.io/QWT/use-guide/2d/scale-builtin-action/)

### Added Data Picking Functionality

Added the QwtPlotSeriesDataPicker class to support data picking.

![series-data-picker-yvalue](./docs/assets/picture/series-data-picker-yvalue.gif)

![series-data-picker-nearest-value](./docs/assets/picture/series-data-picker-nearest-value.gif)

For details, refer to the tutorial:[Series Data Picker](https://czyt1988.github.io/QWT/use-guide/2d/pick-value/)

### Enhanced Canvas Panning

Refactored `QwtPlotPicker` to enable real-time canvas panning; the original implementation has been renamed `QwtPlotCachePanner`.  
The new panner simultaneously supports dragging on any number of axes.

Preview:

![qwt-realtime-panner](./docs/assets/screenshots/qwt-realtime-panner.gif)

For details, refer to the tutorial: [Panning](https://czyt1988.github.io/QWT/use-guide/2d/panner/)

### New Canvas Zoomer

The legacy `QwtPlotZoomer` requires you to specify two axes for every zoom operation.  
If your plot uses four axes you must attach two zoomers—cumbersome and multi-axis-unfriendly.

The new `QwtPlotCanvasZoomer` needs no axis specification; it zooms the entire canvas as a whole and works seamlessly with any number of axes.

For details, refer to the tutorial: [Zoomer](https://czyt1988.github.io/QWT/use-guide/2d/zoomer/)

### Box Chart (Boxplot) Support

Added `QwtPlotBoxChart` class for creating box-and-whisker plots to visualize statistical distributions:

- Support for both pre-computed statistics and raw data input
- Automatic calculation of statistics (median, quartiles, outliers)
- Multiple box styles: Rectangle, Diamond, Notch
- Vertical and horizontal orientations
- Outlier detection with customizable symbols

See documentation: [Box Chart Guide](https://czyt1988.github.io/QWT/use-guide/2d/boxchart/)

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

For details, refer to the tutorial: [Inside Ticks Guide](https://czyt1988.github.io/QWT/use-guide/2d/ticks-inside/). Example code is located in `examples/2D/ticks_inside`.

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

For details, refer to the tutorial: [QwtPyPlot — Matplotlib-style API](https://czyt1988.github.io/QWT/use-guide/2d/pyplot-api/)

### 3D Plotting

The 3D module (`qwt::plot3d`) mirrors the 2D module with a **Plot + Item** architecture and renders through the modern OpenGL pipeline (VBO/VAO + GLSL 3.3 Core shaders; fixed-function calls are used only for the gl2ps vector-export feedback replay, never for normal drawing):

- `Qwt3DPlot` (a `QOpenGLWidget`) is the rendering window. It owns the GL context, view transform, lighting, coordinate system and mouse interaction, but no plotting data.
- `Qwt3DPlotItem` is the base class of every drawable item. Items own their data and geometry and are mounted with `attach()`. One `Qwt3DPlot` can host any number of items, rendered together in a single GL context.

Built-in items:

| Item | What it draws |
|------|---------------|
| `Qwt3DSurface` | Height-field surface from a regular grid, with colormap coloring and an optional mesh |
| `Qwt3DBar` | 3D bar chart / histogram from a 1D series or a 2D grid |
| `Qwt3DLine` | 3D polylines and trajectories in `Lines`, `Dots` or lit `Tube` style |
| `Qwt3DWaterfall` | Sliding-window waterfall of 1D traces (spectra, chromatograms) driven by `pushTrace()` |

`Qwt3DFunction` and `Qwt3DParametricSurface` generate surface data from a mathematical function or from parametric equations; the result is loaded through `Qwt3DSurface::loadFromData()`.

A minimal surface plot:

```cpp
Qwt3DPlot* plot = new Qwt3DPlot(parent);

auto* surface = new Qwt3DSurface();
surface->loadFromData(z, cols, rows, minX, maxX, minY, maxY);
surface->attach(plot);
plot->showColorLegend(true);
```

A live waterfall keeps its sliding window inside the item, so the application does no depth bookkeeping:

```cpp
auto* waterfall = new Qwt3DWaterfall();
waterfall->attach(plot);
waterfall->setTraceCapacity(64);                       // keep the newest 64 traces
waterfall->setColorMode(Qwt3DWaterfall::ByAmplitude);  // color by sample value
waterfall->setColorRange(0.0, 1.0);                    // fixed range: no color flicker

// per timer tick
waterfall->pushTrace(frame);                           // QVector<double>
```

Interaction and export are built in: left-drag rotates, the wheel zooms, `Ctrl`+drag pans, `setMouseSpeed()` tunes the drag sensitivity, and `setRollMouse()` assigns the roll gesture. Raster output goes through `savePixmap()`, while PDF/SVG vector output goes through gl2ps. Because a 3D plot is a widget, it can sit next to 2D plots in a `QwtFigure`; `examples/3D/figureSurface3D` keeps a 3D surface, a 2D cross-section and a time-series curve synchronized.

For details, refer to the guide: [3D Plot Introduction](https://czyt1988.github.io/QWT/use-guide/3d/3d-plot/), and the per-item pages [3D Bar Chart](https://czyt1988.github.io/QWT/use-guide/3d/3d-bar-chart/), [3D Line Plot](https://czyt1988.github.io/QWT/use-guide/3d/3d-line-plot/) and [3D Waterfall](https://czyt1988.github.io/QWT/use-guide/3d/3d-waterfall/).

### 3D Theme System

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

// Apply to a single item only
theme.applyToItem(surface);
```

- 10 built-in themes: `Default`, `Dark`, `Scientific`, `Warm`, `Cool`, `Matplotlib`, `EarthTones`, `Ocean`, `HighContrast`, `Presentation`
- 5 lighting presets: `NoLighting`, `FlatLight`, `Studio`, `Outdoor`, `Soft`
- A `Qwt3DColorMapColor` adapter bridges the core colormaps to 3D surfaces, so all 22 scientific presets work in 3D as well

### Scientific Colormap Presets

`qwt::core` ships `QwtColorMapPreset` — 22 well-known scientific colormaps (the same names used by matplotlib / MATLAB), shared by 2D spectrograms and 3D surfaces:

`viridis`, `plasma`, `inferno`, `magma`, `cividis`, `jet`, `hot`, `cool`, `spring`, `summer`, `autumn`, `winter`, `gray`, `bone`, `copper`, `rainbow`, `hsv`, `turbo`, `coolwarm`, `rdylbu`, `rdylgn`, `spectral`

```cpp
auto cmap = QwtColorMapPreset::create("viridis");
// cmap->rgb(vMin, vMax, value) returns the color for a given data value
```

The perceptually uniform maps (`viridis`, `plasma`, `inferno`, `magma`, `cividis`) are colorblind-friendly and print-safe — ideal for scientific publication.

For the effect of all 22 presets and the interactive example, refer to the tutorial: [Scientific Colormap Presets](https://czyt1988.github.io/QWT/use-guide/general/colormap/)

### SIMD-Accelerated Curve Downsampling

Rendering millions of points smoothly is now first-class. `QwtPlotCurve` offers six downsampling filters, and the MinMax-bucket path is accelerated with SIMD (SSE2 / AVX2 / NEON, auto-detected at runtime):

- `FilterPointsAdaptive` — adaptive column reduction, single-pass O(n), preserving slope shapes inside pixel columns (highest priority, recommended)
- `FilterPointsLTTB` — the true Largest-Triangle-Three-Buckets algorithm, shape-preserving and fast (default)
- `FilterPointsMinMaxLTTB` — the MinMax-bucket algorithm, the default behavior from v7.3.2 to v7.4.2
- `FilterPointsPixel` — per-pixel-column min/max envelope, best for pure-noise signals (legacy, kept for compatibility)
- `FilterPointsAggressive` — fast weeding for huge monotonic datasets (legacy, kept for compatibility)
- `FilterPoints` — the original weeding algorithm

The result: million-point curves render at interactive frame rates. Benchmark it yourself with the `renderbench` example.

For details, refer to the tutorial: [Rendering Performance & Curve Smoothing](https://czyt1988.github.io/QWT/use-guide/2d/rendering-performance/)

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
- New `QwtPlotCurve::BreakAtNan` attribute: renders NaN/Inf samples as gaps instead of bridging them, matching matplotlib behavior
- `QwtPlotSeriesDataPicker` now picks from every series item type — trading curves, interval curves, histograms, vector fields, bar / box charts, and spectro curves
- Modern C++ throughout the codebase: `override` / `final`, `nullptr`, move semantics, `constexpr`, `noexcept`, range-based for, and a unified PIMPL macro pattern (180+ files)
- `renderbench` example for measuring curve-rendering performance and generating reports

## Changelog

See [CHANGES.MD](./CHANGES.md) for detailed logs.

## Gallery

### Basic Charts

||||
|:---:|:---:|:---:|
|![Figure Widget](./docs/assets/picture/figure-scale-aligment.png)|![Simple Plot](./docs/assets/screenshots/simpleplot.png)|![Bar Chart - Grouped](./docs/assets/screenshots/BarChart-grouped.png)|
|`examples/2D/figure`|`examples/2D/simpleplot`|`examples/2D/simpleplot`|

||||
|:---:|:---:|:---:|
|![Bar Chart - Stacked](./docs/assets/screenshots/BarChart-stacked.png) |![Scatter Plot](./docs/assets/screenshots/scatterplot.png)  |![Curve Demo](./docs/assets/screenshots/curvedemo.png)  |
|`examples/2D/barchart`  |`examples/2D/scatterplot`  |`examples/2D/curvedemo` |

### Real-Time Visualization

||||
|:---:|:---:|:---:|
|![CPU Monitor](./docs/assets/screenshots/cpuplot.png)|![Real-Time Plot](./docs/assets/screenshots/realtime.png)|![Oscilloscope](./docs/assets/screenshots/oscilloscope.png)  |
|`examples/2D/cpuplot`|`examples/2D/realtime`|`examples/2D/oscilloscope` |

### Advanced Charts

||||
|:---:|:---:|:---:|
|![Polar Demo](./docs/assets/screenshots/polardemo.png)|![Spectrogram](./docs/assets/screenshots/spectrogram.png)|![Contour Spectrogram](./docs/assets/screenshots/spectrogram-contour.png) |
|`examples/2D/polardemo`|`examples/2D/spectrogram`|`examples/2D/spectrogram`|

||||
|:---:|:---:|:---:|
|![Vector Field](./docs/assets/screenshots/vectorfield.png)|![Stock Chart](./docs/assets/screenshots/stockchart.png)|![Bode Plot](./docs/assets/screenshots/bode.png)| 
|`playground/vectorfield`|`examples/2D/stockchart`|`examples/2D/bode`|

||||
|:---:|:---:|:---:|
|![Tube Display](./docs/assets/screenshots/friedberg-tube.png)|![plot matrix](./docs/assets/screenshots/plotmatrix.png)|![Scale Engine](./docs/assets/screenshots/scaleengine.png)|
|`examples/2D/friedberg`|`playground/plotmatrix`|`playground/scaleengine`|

||||
|:---:|:---:|:---:|
|![Rescaler](./docs/assets/screenshots/rescaler.png)|![Graphics Scale](./docs/assets/screenshots/graphicscale.png)|![Spline Editor](./docs/assets/screenshots/splineeditor.png)|
|`playground/rescaler`|`playground/graphicscale`|`examples/2D/splineeditor`|

||||
|:---:|:---:|:---:|
|![Ticks Inside](./docs/assets/screenshots/ticks_inside.png)|![Box Chart](./docs/assets/screenshots/BoxChart.png)|![parasite plot](./docs/assets/screenshots/parasite-plot.png)|
|`examples/2D/ticks_inside`|`examples/2D/boxchart`|`examples/2D/parasitePlot`|

||||
|:---:|:---:|:---:|
|![System Info](./docs/assets/screenshots/sysinfo.png)|![Distro Watch](./docs/assets/screenshots/distrowatch.png)|![Raster View 1](./docs/assets/screenshots/rasterview-1.png)|
|`examples/2D/sysinfo`|`examples/2D/distrowatch`|`examples/2D/rasterview`|
  
||||
|:---:|:---:|:---:|
|![Raster View 2](./docs/assets/screenshots/rasterview-2.png)|![SVG Map](./docs/assets/screenshots/svgmap.png)|![Item Editor](./docs/assets/screenshots/itemeditor.png)|
|`examples/2D/rasterview`|`playground/svgmap`|`examples/2D/itemeditor`|

### 3D Visualization

||||
|:---:|:---:|:---:|
|![Comprehensive 3D Demo](./docs/assets/screenshots-3D/simpleplot3D.png)|![3D Bar Chart](./docs/assets/screenshots-3D/bar3D.png)|![3D Line Plot](./docs/assets/screenshots-3D/line3D.png)|
|`examples/3D/qwtplot3d`|`examples/3D/bar3D`|`examples/3D/line3D`|

||||
|:---:|:---:|:---:|
|![3D Waterfall](./docs/assets/screenshots-3D/waterfall3D.png)|![3D Waterfall - Wireframe](./docs/assets/screenshots-3D/waterfall3D-2.png)|![3D Surface in a Figure](./docs/assets/screenshots-3D/figureSurface3D.png)|
|`examples/3D/waterfall3D`|`examples/3D/waterfall3D --style 2`|`examples/3D/figureSurface3D`|

### Animated Demos

||||
|:---:|:---:|:---:|
|![Animated Demo](./docs/assets/screenshots/animated.png)|![Curve Tracker](./docs/assets/screenshots/curvetracker.png)|![Refresh Test](./docs/assets/screenshots/refreshtest.png)|
|`examples/2D/animation`|`playground/curvetracker`|`examples/2D/refreshtest`|

### Styles & Symbols

||||
|:---:|:---:|:---:|
|![Legends Demo](./docs/assets/screenshots/legends.png)|![Symbols Demo](./docs/assets/screenshots/symbols.png)|![Shape Rendering](./docs/assets/screenshots/shapes.png)|
|`examples/2D/legends`|`playground/symbols`|`playground/shapes`|

### Control Widgets


||||
|:---:|:---:|:---:|
|![Dial Controls](./docs/assets/screenshots/controls-dial.png)|![Knob Controls](./docs/assets/screenshots/controls-knob.png)|![Slider Controls](./docs/assets/screenshots/controls-sliders.png)|
|`examples/2D/controls`|`examples/2D/controls`|`examples/2D/controls`|
 
||||
|:---:|:---:|:---:|
|![Wheel & Thermo](./docs/assets/screenshots/controls-wheel-thermo.png)|![Radio Buttons](./docs/assets/screenshots/radio.png)|![Time Scale Demo](./docs/assets/screenshots/timescale.png)|
|`examples/2D/controls`|`examples/2D/radio`|`playground/timescale`|

### Instrument Panels

||||
|:---:|:---:|:---:|
|![Compass Dial](./docs/assets/screenshots/dials-compass.png)|![Cockpit Dials](./docs/assets/screenshots/dials-cockpit.png)||
|`examples/2D/dials`|`examples/2D/dials`||

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