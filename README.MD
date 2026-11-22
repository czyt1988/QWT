<div align="center">
<img src="./docs/assets/brand/qwt-hero-en.png" alt="Qwt 7 — a high-performance 2D and 3D plotting library for Qt" width="100%">
<p><b>English</b> &nbsp;&middot;&nbsp; <a href="./README-CN.MD">简体中文</a></p>
<p><a href="https://github.com/czyt1988/QWT/releases/latest"><img src="https://img.shields.io/github/v/release/czyt1988/QWT?style=flat-square&amp;label=release&amp;color=2563eb" alt="Latest release"></a> <a href="https://github.com/czyt1988/QWT/actions/workflows/page.yml"><img src="https://img.shields.io/github/actions/workflow/status/czyt1988/QWT/page.yml?style=flat-square&amp;label=docs" alt="Documentation build"></a> <a href="./COPYING"><img src="https://img.shields.io/badge/license-Qwt%201.0%20%2F%20LGPL-3b82f6?style=flat-square" alt="Qwt License 1.0 (LGPL)"></a> <a href="https://github.com/czyt1988/QWT/stargazers"><img src="https://img.shields.io/github/stars/czyt1988/QWT?style=flat-square&amp;label=stars&amp;color=2563eb" alt="GitHub stars"></a></p>
<p><a href="https://doc.qt.io/"><img src="https://img.shields.io/badge/Qt-5.12%2B%20%7C%206-41CD52?style=flat-square&amp;logo=qt&amp;logoColor=white" alt="Qt 5.12+ / Qt 6"></a> <a href="https://en.cppreference.com/w/cpp/11"><img src="https://img.shields.io/badge/C%2B%2B-11%20%2F%2017-00599C?style=flat-square&amp;logo=cplusplus&amp;logoColor=white" alt="C++11 / C++17"></a> <a href="https://czyt1988.github.io/QWT/"><img src="https://img.shields.io/badge/docs-English-2563eb?style=flat-square" alt="English documentation"></a> <a href="https://czyt1988.github.io/QWT/zh/"><img src="https://img.shields.io/badge/docs-%E4%B8%AD%E6%96%87-2563eb?style=flat-square" alt="中文文档"></a></p>
<h3>A modern, LGPL-friendly 2D/3D plotting library for Qt</h3>
<p><sub>Qwt 7 continues the final official Qwt 6.2 release with CMake, Qt 5/6, modern C++, integrated 3D plotting and helpers that make scientific visualization easier.</sub></p>
<p><a href="https://czyt1988.github.io/QWT/"><b>Documentation</b></a> &nbsp;&middot;&nbsp; <a href="https://czyt1988.github.io/QWT/use-guide/import-qwt/">Getting started</a> &nbsp;&middot;&nbsp; <a href="#quick-start">Quick start</a> &nbsp;&middot;&nbsp; <a href="#whats-new-in-qwt-7">Features</a> &nbsp;&middot;&nbsp; <a href="#gallery">Gallery</a> &nbsp;&middot;&nbsp; <a href="./CHANGES.md">Changelog</a></p>
</div>

## Why Qwt 7?

There are only a handful of plotting libraries in the Qt ecosystem: **QCustomPlot**, **Qwt**, **Qt Charts** and **KDChart**. They differ mainly in license, rendering model and maturity.

Since Qt 6.8, Qt Charts (2D) and Qt DataVisualization (3D) have been merged into the unified **Qt Graphs** module (not to be confused with Qt Graphics). Its back end is built entirely on the Qt Quick Scene Graph (QSG) and Qt Quick 3D, dropping the classic Graphics-View / QPainter pipeline. Qt Graphs has to be embedded through `QQuickWidget` or `QQuickWindow`, which makes the QML runtime mandatory and C++-only usage awkward ([discussion on the Qt forum](https://forum.qt.io/topic/159224/qt-graphs-building-2d-plot-using-c-only)). It also drops support for older systems such as Windows 7 and is unfriendly to embedded devices, so `QCustomPlot`, `Qwt`, `Qt Charts` and `KDChart` remain the practical choices for the coming years.

| Library | License | Rendering | Strengths | Limitations |
|:--------|:--------|:----------|:----------|:------------|
| **QCustomPlot** | GPL | QPainter | single `qcustomplot.h/.cpp` drop-in, widest adoption, interaction out of the box | GPL is viral — a deal-breaker for commercial software |
| **Qwt** | LGPL | QPainter | software-engineering-friendly architecture, excellent large-data performance, fine-grained control | upstream maintenance stopped, dated default style, heavier integration |
| **Qt Charts / Qt Graphs** | GPLv3 | QSG + Qt Quick 3D | official Qt module | low performance for scientific data, QML runtime required, no LGPL option |
| **KDChart 3.0** | MIT | QPainter | commercial friendly, Gantt charts | dated visuals |

For commercial software the realistic options are **Qwt** and **KDChart 3.0**. I prefer Qwt: its architecture follows software-engineering principles and its performance on large data sets is stronger. QCustomPlot is easier to start with — mouse zooming and axis scaling work out of the box — while Qwt needs more code for the same result but gives much finer control. That trade-off is why, when my own projects need plotting, I choose Qwt, add the features I need and improve it along the way. This project is the result.

When upstream maintenance stopped, I took over the final official release (**Qwt 6.2.0**) and continued from there. **Qwt 7** keeps everything that made Qwt a solid engineering choice and adds what a modern Qt project expects: CMake, Qt 6, modern C++, an integrated 3D module, layout and interaction helpers, scientific colormaps and a matplotlib-style API — all under the same LGPL-compatible license.

- **GitHub** — <https://github.com/czyt1988/QWT>
- **Gitee (mirror)** — <https://gitee.com/czyt1988/QWT>
- **Documentation** — <https://czyt1988.github.io/QWT/> · [中文文档](https://czyt1988.github.io/QWT/zh/)
- **Changelog** — [CHANGES.md](./CHANGES.md) · [CHANGES-CN.md](./CHANGES-CN.md)

![Qwt 7 in action: a live 3D waterfall with a 2D spectrum on top](./docs/assets/screenshots-3D/waterfall3D.png)

## Documentation

The full documentation is built with [MkDocs Material](https://squidfunk.github.io/mkdocs-material/) and deployed to **GitHub Pages** on every push to `master`:

- **English documentation** — <https://czyt1988.github.io/QWT/>
- **中文文档** — <https://czyt1988.github.io/QWT/zh/>

| Topic | English | 中文 |
|:------|:--------|:-----|
| What's new in Qwt 7 | [Qwt 7 new features](https://czyt1988.github.io/QWT/use-guide/qwt7-new-features/) | [Qwt 7 新特性](https://czyt1988.github.io/QWT/zh/use-guide/qwt7-new-features/) |
| Installing & importing Qwt | [Import Qwt](https://czyt1988.github.io/QWT/use-guide/import-qwt/) | [引入 Qwt](https://czyt1988.github.io/QWT/zh/use-guide/import-qwt/) |
| Your first plot | [First plot](https://czyt1988.github.io/QWT/use-guide/first-plot/) | [第一个绘图](https://czyt1988.github.io/QWT/zh/use-guide/first-plot/) |
| Building from source | [Build instructions](https://czyt1988.github.io/QWT/build-guide/build-instructions/) | [构建说明](https://czyt1988.github.io/QWT/zh/build-guide/build-instructions/) |
| 2D chart items | [Curve & chart items](https://czyt1988.github.io/QWT/use-guide/2d/curve/) | [曲线与图表项](https://czyt1988.github.io/QWT/zh/use-guide/2d/curve/) |
| Figure layout | [Figure widget](https://czyt1988.github.io/QWT/use-guide/2d/figure-widget/) | [Figure 绘图容器](https://czyt1988.github.io/QWT/zh/use-guide/2d/figure-widget/) |
| Multiple axes | [Parasite axes](https://czyt1988.github.io/QWT/use-guide/2d/parasite-axes/) | [多坐标轴](https://czyt1988.github.io/QWT/zh/use-guide/2d/parasite-axes/) |
| Pan, zoom & data picking | [Panner](https://czyt1988.github.io/QWT/use-guide/2d/panner/) · [Zoomer](https://czyt1988.github.io/QWT/use-guide/2d/zoomer/) · [Data picker](https://czyt1988.github.io/QWT/use-guide/2d/pick-value/) | [平移](https://czyt1988.github.io/QWT/zh/use-guide/2d/panner/) · [缩放](https://czyt1988.github.io/QWT/zh/use-guide/2d/zoomer/) · [数据拾取](https://czyt1988.github.io/QWT/zh/use-guide/2d/pick-value/) |
| Performance & downsampling | [Rendering performance](https://czyt1988.github.io/QWT/use-guide/2d/rendering-performance/) | [渲染性能](https://czyt1988.github.io/QWT/zh/use-guide/2d/rendering-performance/) |
| Matplotlib-style API | [PyPlot API](https://czyt1988.github.io/QWT/use-guide/2d/pyplot-api/) | [PyPlot 接口](https://czyt1988.github.io/QWT/zh/use-guide/2d/pyplot-api/) |
| 3D plotting | [3D plot guide](https://czyt1988.github.io/QWT/use-guide/3d/3d-plot/) | [3D 绘图](https://czyt1988.github.io/QWT/zh/use-guide/3d/3d-plot/) |
| Colormap presets | [Colormap presets](https://czyt1988.github.io/QWT/use-guide/general/colormap/) | [Colormap 预设](https://czyt1988.github.io/QWT/zh/use-guide/general/colormap/) |
| Widgets & controls | [Widgets & controls](https://czyt1988.github.io/QWT/use-guide/general/widgets-controls/) | [仪表盘与滑块](https://czyt1988.github.io/QWT/zh/use-guide/general/widgets-controls/) |
| Tech reports (internals) | [Tech reports](https://czyt1988.github.io/QWT/tech-report/) | [技术报告](https://czyt1988.github.io/QWT/zh/tech-report/) |
| Developer guide | [Developer guide](https://czyt1988.github.io/QWT/dev-guide/) | [开发指引](https://czyt1988.github.io/QWT/zh/dev-guide/) |
| API reference | [Doxygen (English)](https://czyt1988.github.io/QWT/doxygen_en/index.html) | [Doxygen (中文)](https://czyt1988.github.io/QWT/doxygen_cn/index.html) |
| Changelog | [CHANGES.md](./CHANGES.md) | [CHANGES-CN.md](./CHANGES-CN.md) |

The documentation sources live in [`docs/`](./docs) — English at the root, Chinese under [`docs/zh/`](./docs/zh). The API reference is generated from the source comments with Doxygen.

<details>
<summary>Build the documentation locally</summary>

```bash
pip install -r requirements-docs.txt
mkdocs serve
# English: http://127.0.0.1:8000/QWT/
# Chinese: http://127.0.0.1:8000/QWT/zh/
```

</details>

## Quick Start

### Requirements

| Requirement | Minimum |
|:------------|:--------|
| Build system | CMake 3.10+ (`qmake` integration is still supported) |
| Qt | Qt 5.12+ or Qt 6.x |
| C++ | C++11; C++17 when building against Qt 6 |
| Compiler | MSVC 2019+, GCC or Clang |
| Platforms | Windows and Linux (CI-verified); macOS builds with the same Qt toolchain |

### CMake (recommended)

Qwt 7 installs a CMake package, so linking it no longer needs manual include paths or preprocessor defines:

```cmake
find_package(qwt REQUIRED)

# 2D plotting (links qwt::core automatically)
target_link_libraries(${YOUR_APP_TARGET} PRIVATE qwt::plot)

# 3D plotting (links qwt::core automatically)
target_link_libraries(${YOUR_APP_TARGET} PRIVATE qwt::plot3d)

# Foundational utilities only — math, data types, geometry, transforms, ...
target_link_libraries(${YOUR_APP_TARGET} PRIVATE qwt::core)
```

The three libraries have a deliberately simple dependency graph: `qwt::plot` and `qwt::plot3d` both depend on `qwt::core`, and never on each other.

### Single header & source

Following the example of QCustomPlot, the whole library (2D and 3D) is also amalgamated into `QwtPlot.h` and `QwtPlot.cpp`. Drop the two files into your project and you are ready to go — see [`src-amalgamate/`](./src-amalgamate) and the [`staticExample`](./examples/staticExample):

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

# MSVC: the amalgamated translation unit exceeds the default COFF section limit (fatal error C1128)
if(MSVC)
    target_compile_options(YOUR_APP_TARGET PRIVATE /bigobj)
endif()
```

On MSVC the amalgamated `QwtPlot.cpp` needs `/bigobj` — one translation unit exceeds the default limit of 65536 sections (error C1128). See [`examples/staticExample`](./examples/staticExample) for a complete target.

### Your first plot

```cpp
#include <QApplication>
#include <QPolygonF>
#include <QwtPlot>
#include <QwtPlotCurve>
#include <QwtPlotGrid>

#include <cmath>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 1. Create the plot window
    QwtPlot plot;
    plot.setWindowTitle("My First Plot");
    plot.setAxisTitle(QwtAxis::XBottom, "Time [s]");
    plot.setAxisTitle(QwtAxis::YLeft, "Amplitude");
    plot.setAxisScale(QwtAxis::YLeft, -1.5, 1.5);

    // 2. Attach a grid
    auto* grid = new QwtPlotGrid();
    grid->attach(&plot);

    // 3. Feed the curve with samples
    QPolygonF samples;
    for (int i = 0; i < 200; ++i) {
        const double x = i * 0.05;
        samples << QPointF(x, std::sin(x));
    }

    auto* curve = new QwtPlotCurve("Sine");
    curve->setPen(QColor("#2563eb"), 2.0);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curve->setSamples(samples);
    curve->attach(&plot);

    // 4. Show it
    plot.resize(720, 480);
    plot.show();
    return app.exec();
}
```

Read the step-by-step walkthrough in [First plot](https://czyt1988.github.io/QWT/use-guide/first-plot/).

### qmake

Qwt 7 no longer uses `qmake` as its primary build system, but `qmake` integration is fully supported. See the [`qmakeExample`](./examples/qmakeExample) example for a working demonstration, or read [qmake single-file build](./docs/build-guide/qmake-single-file.md).

### Build from source

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.0/gcc_64
cmake --build build --config Release
cmake --install build --prefix /path/to/install
```

The repository also ships a convenience script for Windows that auto-detects Qt, Visual Studio and CMake:

```powershell
.\build.ps1                 # full Release build
.\build.ps1 build           # incremental build
.\build.ps1 rebuild         # clean + reconfigure + build
.\build.ps1 configure -Examples OFF -Playground OFF   # library only
```

See [Build instructions](https://czyt1988.github.io/QWT/build-guide/build-instructions/) for all options.

## Highlights

| Area | What you get |
|:-----|:-------------|
| **2D plotting** | 20+ plot items: curves, scatter, bar, box, histogram, spectrogram, trading curve, vector field, interval curve, polar plots, shapes, labels, symbols and more |
| **3D plotting** | Surfaces, bars, lines, waterfalls and function / parametric surfaces on a modern VBO/VAO + GLSL pipeline, with a Plot + Item architecture that mirrors the 2D API |
| **Performance** | SIMD-accelerated curve downsampling (SSE2 / AVX2 / NEON), OpenGL canvas and a benchmark suite for million-point data sets |
| **Layout** | `QwtFigure` with matplotlib-like `subplot()` grids, retained draggable grid tracks, automatic plot-area alignment, parasite axes for any number of axes |
| **Interaction** | Real-time canvas panner, axis-agnostic canvas zoomer, axis drag/zoom, data picking for every series type |
| **Look & feel** | Modernized flat style, 10 built-in 3D themes with 5 lighting presets, 22 scientific colormap presets, flat-style control widgets |
| **Architecture** | Three clean libraries (`qwt::core`, `qwt::plot`, `qwt::plot3d`), unified PIMPL pattern, C++11/17, no legacy OpenGL |
| **Integration** | CMake package targets, single-header amalgamation, full `qmake` support, Qt 5.12+ and Qt 6 |

## What's New in Qwt 7

### CMake support

Qwt 7 uses **CMake** as its primary build system and no longer maintains `.pro` project files, while `qmake` integration remains fully supported. It installs a proper CMake package with the `qwt::core`, `qwt::plot` and `qwt::plot3d` targets — see [Quick Start](#cmake-recommended) for the two-line usage.

### Single header & source file

Following the example of `QCustomPlot`, the entire library is merged into `QwtPlot.h` and `QwtPlot.cpp`. Drop these two files into your project and you are ready to go — both 2D and 3D plotting are included.

See [Importing Qwt](https://czyt1988.github.io/QWT/use-guide/import-qwt/) and the [`staticExample`](./examples/staticExample).

![staticExample](./docs/assets/screenshots/staticExample.png)

### Using qmake with Qwt 7

`qmake` integration is still fully supported — see [`qmakeExample`](./examples/qmakeExample) and [qmake single-file build](./docs/build-guide/qmake-single-file.md), or the [qmake section](#qmake) of Quick Start.

### Modernized visual style

The original Qwt style used an outdated beveled look that is inconsistent with modern aesthetics, so it was redesigned: the default sunken style is gone and the axes sit flush against the plot area.

Original Qwt style:

![qwt6.3 effect](./docs/assets/picture/bode-old.png)

Qwt 7:

![qwt7 effect](./docs/assets/screenshots/bode.png)

### Added Figure container

Similar to Python's matplotlib, Qwt provides a `QwtFigure` plotting container that makes it easy to lay out multiple plots, including matplotlib-style `subplot()` grids.

`QwtFigureWidgetOverlay` integrates several interactions with the figure, such as resizing and moving plots.

New in 7.6, the **retained grid layout** keeps the row/column track ratios as data: `QwtFigureGrid` owns the tracks and members, `QwtFigureGridSplitter` makes the dividers draggable (with an undo-friendly `gridSizesChangeFinished()` signal), and `QwtFigure::setGridAxisAlignment()` aligns the plot areas of a column/row so axis backbones and data areas line up even when tick labels, legends or parasite axes differ in size.

![QwtFigure retained grid layout with draggable dividers](./docs/assets/screenshots/figure-grid.png)

See the tutorial: [Figure plotting container](https://czyt1988.github.io/QWT/use-guide/2d/figure-widget/), and the [`figureGrid`](./examples/2D/figureGrid) example.

### Added parasite axes

Qwt 7 supports **parasite axes**, allowing multiple plots to share the same plot area with independent axes — as many X/Y axes as needed.

See the tutorial: [Creating multiple coordinate axes](https://czyt1988.github.io/QWT/use-guide/2d/parasite-axes/)

### Added axis interactions

Axis interaction is built in: **mouse dragging** and **mouse-wheel zoom** work directly on the axes.

**Panning:**

![qwt-scale-builtin-action-pan](./docs/assets/screenshots/qwt-scale-builtin-action-pan.gif)

**Zooming:**

![qwt-scale-builtin-action-zoom](./docs/assets/screenshots/qwt-scale-builtin-action-zoom.gif)

See the tutorial: [Coordinate axis interactive actions](https://czyt1988.github.io/QWT/use-guide/2d/scale-builtin-action/)

### Added data picking

`QwtPlotSeriesDataPicker` implements data picking for every series item type — trading curves, interval curves, histograms, vector fields, bar / box charts and spectro curves.

![series-data-picker-yvalue](./docs/assets/picture/series-data-picker-yvalue.gif)

![series-data-picker-nearest-value](./docs/assets/picture/series-data-picker-nearest-value.gif)

See the tutorial: [Series data picker](https://czyt1988.github.io/QWT/use-guide/2d/pick-value/)

### Enhanced canvas panning

`QwtPlotPanner` was refactored for real-time canvas panning: panning now moves the live plot content instead of a cached bitmap. The original cache-based implementation is available as `QwtPlotCachePanner`, and the new panner supports dragging on any number of axes at once.

![qwt-realtime-panner](./docs/assets/screenshots/qwt-realtime-panner.gif)

See the tutorial: [Panning](https://czyt1988.github.io/QWT/use-guide/2d/panner/)

### New canvas zoomer

The legacy `QwtPlotZoomer` requires two axes for every zoom operation. If your plot uses four axes you must attach two zoomers — cumbersome and multi-axis-unfriendly.

The new `QwtPlotCanvasZoomer` needs no axis specification: it zooms the entire canvas as a whole and works with any number of axes.

See the tutorial: [Zoomer](https://czyt1988.github.io/QWT/use-guide/2d/zoomer/)

### Box chart (boxplot) support

`QwtPlotBoxChart` creates box-and-whisker plots to visualize statistical distributions:

- Support for both pre-computed statistics and raw data input
- Automatic calculation of statistics (median, quartiles, outliers)
- Multiple box styles: Rectangle, Diamond, Notch
- Vertical and horizontal orientations
- Outlier detection with customizable symbols

See the guide: [Box chart](https://czyt1988.github.io/QWT/use-guide/2d/boxchart/)

### Inside ticks

`QwtPlot::setAxisTickDirection()` draws axis tick marks from the canvas edge toward the interior (ticks inside), while the backbone and tick labels remain outside — useful for compact layouts or distinctive visual styles:

- Each axis can be set independently to `TickInside` or `TickOutside` (default)
- Major, medium and minor ticks are all drawn inside, inheriting the length, color and pen width of the outside ticks
- Works with logarithmic and inverted scales — inside ticks render correctly under every scale engine
- Can be combined with parasite axes, where each parasite plot sets its own tick direction

```cpp
// Draw YLeft and XBottom ticks inside
plot->setAxisTickDirection(QwtAxis::YLeft, QwtPlot::TickInside);
plot->setAxisTickDirection(QwtAxis::XBottom, QwtPlot::TickInside);
plot->replot();
```

![ticks_inside](./docs/assets/screenshots/ticks_inside.png)

See the guide: [Inside ticks](https://czyt1988.github.io/QWT/use-guide/2d/ticks-inside/). The example code is in [`examples/2D/ticks_inside`](./examples/2D/ticks_inside).

### Matplotlib-style plotting API (`QwtPyPlot`)

New in 7.3.1, `QwtPyPlot` is a high-level, stateful API inspired by Python's `matplotlib.pyplot`. It wraps a `QwtFigure` (or a single `QwtPlot`) and delegates to the existing Qwt infrastructure, so you get matplotlib-style brevity without giving up Qwt's power and fine-grained control.

Familiar one-liners just work — `plot()`, `scatter()`, `bar()`, `hist()`, `imshow()`, `contour()`, `quiver()`, `fillBetween()`, `errorbar()`, `candlestick()` — together with `subplot()`, `twinx()` / `twiny()`, `savefig()`, and matplotlib format strings such as `"r-o"` (red line + circle markers):

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

See the tutorial: [QwtPyPlot — matplotlib-style API](https://czyt1988.github.io/QWT/use-guide/2d/pyplot-api/)

### 3D plotting

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

Interaction and export are built in: left-drag rotates, the wheel zooms, `Ctrl`+drag pans, `setMouseSpeed()` tunes the drag sensitivity, and `setRollMouse()` assigns the roll gesture. Raster output goes through `savePixmap()`, while PDF/SVG vector output goes through gl2ps. Because a 3D plot is a widget, it can sit next to 2D plots in a `QwtFigure`; [`examples/3D/figureSurface3D`](./examples/3D/figureSurface3D) keeps a 3D surface, a 2D cross-section and a time-series curve synchronized.

See the guide: [3D plot introduction](https://czyt1988.github.io/QWT/use-guide/3d/3d-plot/), plus the per-item pages [3D bar chart](https://czyt1988.github.io/QWT/use-guide/3d/3d-bar-chart/), [3D line plot](https://czyt1988.github.io/QWT/use-guide/3d/3d-line-plot/) and [3D waterfall](https://czyt1988.github.io/QWT/use-guide/3d/3d-waterfall/).

### 3D theme system

New in 7.3.1, the `Qwt3DTheme` system packages every visual property — background, mesh, colormap, axes, title, lighting, shading and material — into a single descriptor, so a complete look can be applied in one call:

```cpp
// Create the plot and a surface item
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

### Scientific colormap presets

`qwt::core` ships `QwtColorMapPreset` — 22 well-known scientific colormaps (the same names used by matplotlib / MATLAB), shared by 2D spectrograms and 3D surfaces:

`viridis`, `plasma`, `inferno`, `magma`, `cividis`, `jet`, `hot`, `cool`, `spring`, `summer`, `autumn`, `winter`, `gray`, `bone`, `copper`, `rainbow`, `hsv`, `turbo`, `coolwarm`, `rdylbu`, `rdylgn`, `spectral`

```cpp
auto cmap = QwtColorMapPreset::create("viridis");
// cmap->rgb(vMin, vMax, value) returns the color for a given data value
```

The perceptually uniform maps (`viridis`, `plasma`, `inferno`, `magma`, `cividis`) are colorblind-friendly and print-safe — ideal for scientific publication.

See the tutorial: [Scientific colormap presets](https://czyt1988.github.io/QWT/use-guide/general/colormap/)

### SIMD-accelerated curve downsampling

Rendering millions of points smoothly is now first-class. `QwtPlotCurve` offers six downsampling filters, and the MinMax-bucket path is accelerated with SIMD (SSE2 / AVX2 / NEON, auto-detected at runtime):

- `FilterPointsAdaptive` — adaptive column reduction, single-pass O(n), preserving slope shapes inside pixel columns (highest priority, recommended)
- `FilterPointsLTTB` — the true Largest-Triangle-Three-Buckets algorithm, shape-preserving and fast (default)
- `FilterPointsMinMaxLTTB` — the MinMax-bucket algorithm, the default behavior from v7.3.2 to v7.4.2
- `FilterPointsPixel` — per-pixel-column min/max envelope, best for pure-noise signals (legacy, kept for compatibility)
- `FilterPointsAggressive` — fast weeding for huge monotonic datasets (legacy, kept for compatibility)
- `FilterPoints` — the original weeding algorithm

The result: million-point curves render at interactive frame rates. Benchmark it yourself with the `renderbench` example.

See the tutorial: [Rendering performance & curve smoothing](https://czyt1988.github.io/QWT/use-guide/2d/rendering-performance/)

### Standalone core foundational library

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

### Other modifications

- Fixed the impact of `NAN` and `INF` values on plotting
- New `QwtPlotCurve::BreakAtNan` attribute: renders NaN/Inf samples as gaps instead of bridging them, matching matplotlib behavior
- `QwtPlotSeriesDataPicker` now picks from every series item type — trading curves, interval curves, histograms, vector fields, bar / box charts and spectro curves
- Modern C++ throughout the codebase: `override` / `final`, `nullptr`, move semantics, `constexpr`, `noexcept`, range-based for, and a unified PIMPL macro pattern (180+ files)
- `renderbench` example for measuring curve-rendering performance and generating reports

## Gallery

### Basic charts

||||
|:---:|:---:|:---:|
|![Figure Widget](./docs/assets/picture/figure-scale-aligment.png)|![Simple Plot](./docs/assets/screenshots/simpleplot.png)|![Bar Chart - Grouped](./docs/assets/screenshots/BarChart-grouped.png)|
|`examples/2D/figure`|`examples/2D/simpleplot`|`examples/2D/barchart`|

||||
|:---:|:---:|:---:|
|![Bar Chart - Stacked](./docs/assets/screenshots/BarChart-stacked.png)|![Scatter Plot](./docs/assets/screenshots/scatterplot.png)|![Curve Demo](./docs/assets/screenshots/curvedemo.png)|
|`examples/2D/barchart`|`examples/2D/scatterplot`|`examples/2D/curvedemo`|

### Real-time visualization

||||
|:---:|:---:|:---:|
|![CPU Monitor](./docs/assets/screenshots/cpuplot.png)|![Real-Time Plot](./docs/assets/screenshots/realtime.png)|![Oscilloscope](./docs/assets/screenshots/oscilloscope.png)|
|`examples/2D/cpuplot`|`examples/2D/realtime`|`examples/2D/oscilloscope`|

### Advanced charts

||||
|:---:|:---:|:---:|
|![Polar Demo](./docs/assets/screenshots/polardemo.png)|![Spectrogram](./docs/assets/screenshots/spectrogram.png)|![Contour Spectrogram](./docs/assets/screenshots/spectrogram-contour.png)|
|`examples/2D/polardemo`|`examples/2D/spectrogram`|`examples/2D/spectrogram`|

||||
|:---:|:---:|:---:|
|![Vector Field](./docs/assets/screenshots/vectorfield.png)|![Stock Chart](./docs/assets/screenshots/stockchart.png)|![Bode Plot](./docs/assets/screenshots/bode.png)|
|`playground/vectorfield`|`examples/2D/stockchart`|`examples/2D/bode`|

||||
|:---:|:---:|:---:|
|![Tube Display](./docs/assets/screenshots/friedberg-tube.png)|![Plot Matrix](./docs/assets/screenshots/plotmatrix.png)|![Scale Engine](./docs/assets/screenshots/scaleengine.png)|
|`examples/2D/friedberg`|`playground/plotmatrix`|`playground/scaleengine`|

||||
|:---:|:---:|:---:|
|![Rescaler](./docs/assets/screenshots/rescaler.png)|![Graphics Scale](./docs/assets/screenshots/graphicscale.png)|![Spline Editor](./docs/assets/screenshots/splineeditor.png)|
|`playground/rescaler`|`playground/graphicscale`|`examples/2D/splineeditor`|

||||
|:---:|:---:|:---:|
|![Ticks Inside](./docs/assets/screenshots/ticks_inside.png)|![Box Chart](./docs/assets/screenshots/BoxChart.png)|![Parasite Plot](./docs/assets/screenshots/parasite-plot.png)|
|`examples/2D/ticks_inside`|`examples/2D/boxchart`|`examples/2D/parasitePlot`|

||||
|:---:|:---:|:---:|
|![System Info](./docs/assets/screenshots/sysinfo.png)|![Distro Watch](./docs/assets/screenshots/distrowatch.png)|![Raster View 1](./docs/assets/screenshots/rasterview-1.png)|
|`examples/2D/sysinfo`|`examples/2D/distrowatch`|`examples/2D/rasterview`|

||||
|:---:|:---:|:---:|
|![Raster View 2](./docs/assets/screenshots/rasterview-2.png)|![SVG Map](./docs/assets/screenshots/svgmap.png)|![Item Editor](./docs/assets/screenshots/itemeditor.png)|
|`examples/2D/rasterview`|`playground/svgmap`|`examples/2D/itemeditor`|

||||
|:---:|:---:|:---:|
|![Retained Grid Layout](./docs/assets/screenshots/figure-grid.png)|||
|`examples/2D/figureGrid`|||

### 3D visualization

||||
|:---:|:---:|:---:|
|![Comprehensive 3D Demo](./docs/assets/screenshots-3D/simpleplot3D.png)|![3D Bar Chart](./docs/assets/screenshots-3D/bar3D.png)|![3D Line Plot](./docs/assets/screenshots-3D/line3D.png)|
|`examples/3D/qwtplot3d`|`examples/3D/bar3D`|`examples/3D/line3D`|

||||
|:---:|:---:|:---:|
|![3D Waterfall](./docs/assets/screenshots-3D/waterfall3D.png)|![3D Waterfall - Wireframe](./docs/assets/screenshots-3D/waterfall3D-2.png)|![3D Surface in a Figure](./docs/assets/screenshots-3D/figureSurface3D.png)|
|`examples/3D/waterfall3D`|`examples/3D/waterfall3D --style 2`|`examples/3D/figureSurface3D`|

### Animated demos

||||
|:---:|:---:|:---:|
|![Animated Demo](./docs/assets/screenshots/animated.png)|![Curve Tracker](./docs/assets/screenshots/curvetracker.png)|![Refresh Test](./docs/assets/screenshots/refreshtest.png)|
|`examples/2D/animation`|`playground/curvetracker`|`examples/2D/refreshtest`|

### Styles & symbols

||||
|:---:|:---:|:---:|
|![Legends Demo](./docs/assets/screenshots/legends.png)|![Symbols Demo](./docs/assets/screenshots/symbols.png)|![Shape Rendering](./docs/assets/screenshots/shapes.png)|
|`examples/2D/legends`|`playground/symbols`|`playground/shapes`|

### Control widgets

||||
|:---:|:---:|:---:|
|![Dial Controls](./docs/assets/screenshots/controls-dial.png)|![Knob Controls](./docs/assets/screenshots/controls-knob.png)|![Slider Controls](./docs/assets/screenshots/controls-sliders.png)|
|`examples/2D/controls`|`examples/2D/controls`|`examples/2D/controls`|

||||
|:---:|:---:|:---:|
|![Wheel & Thermo](./docs/assets/screenshots/controls-wheel-thermo.png)|![Radio Buttons](./docs/assets/screenshots/radio.png)|![Time Scale Demo](./docs/assets/screenshots/timescale.png)|
|`examples/2D/controls`|`examples/2D/radio`|`playground/timescale`|

### Instrument panels

||||
|:---:|:---:|:---:|
|![Compass Dial](./docs/assets/screenshots/dials-compass.png)|![Cockpit Dials](./docs/assets/screenshots/dials-cockpit.png)||
|`examples/2D/dials`|`examples/2D/dials`||

## Roadmap & Progress

- [x] CMake support
- [x] Qt 6 support
- [x] C++11 modernization
- [x] Merged into a single header/source for easier inclusion
- [x] Integrated interaction helpers for simpler usage
- [x] Modernized visual style
- [x] `QwtFigure` class for layout management
- [x] Retained, draggable grid layout with plot-area alignment (7.6)
- [x] Parasite-axis support for unlimited axes
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

## Contributing

- **Bug reports & feature requests** — open an issue on [GitHub](https://github.com/czyt1988/QWT/issues). Please include the Qt version, compiler and a minimal reproducer when relevant.
- **Pull requests** — follow the [coding standards](https://czyt1988.github.io/QWT/dev-guide/coding-standards/) (`.clang-format`, English Doxygen comments, modern C++) and keep commits atomic with Conventional Commit messages.
- **Tests** — live in [`tests/`](./tests) and are enabled with `-DQWT_CONFIG_BUILD_TESTS=ON`.
- **Documentation** — the guides live in [`docs/`](./docs) (English) and [`docs/zh/`](./docs/zh) (Chinese); API reference pages are generated from the source comments.

The Gitee repository mirrors the project for users in mainland China; development happens on GitHub.

## Changelog

See [CHANGES.md](./CHANGES.md) for the detailed log, or [CHANGES-CN.md](./CHANGES-CN.md) for the Chinese version. Releases follow [Semantic Versioning](https://semver.org/).

## License

Qwt 7 is distributed under the **Qwt License, Version 1.0** — the GNU Lesser General Public License (LGPL) with exceptions that keep subclassing and static linking commercial-friendly. See [COPYING](./COPYING) for the full license text. The source files carry the following copyright notice:

```
Qwt Widget Library
Copyright (C) 1997   Josef Wilgen
Copyright (C) 2002   Uwe Rathmann

Qwt is published under the Qwt License, Version 1.0.
You should have received a copy of this licence in the file
COPYING.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```
