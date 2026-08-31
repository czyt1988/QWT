## tag:v7.3.6 (2026-07-31)

### New Features

- **Qwt3DBar and Qwt3DLine — New 3D Plot Items**
    - `Qwt3DBar`: 3D bar chart item, supports 1D series + 2D grid (`Qwt3DFunctionData`) data entry, per-bar flat-normal cuboids (6 faces), Filled/FilledMesh/Wireframe styles reusing the surface shader with two-pass edge overlay
    - `Qwt3DLine`: 3D line/curve item, `setSamples` mirroring `QwtPlotCurve`, `LineStyle` supports Lines/Tube/Dots; Tube swept with parallel-transport framing (robust on straight segments) for lighting; Lines/Dots reuse shared line/point shaders
    - Theme integration: `Qwt3DTheme::applyToItem()` extended to dispatch Qwt3DLine/Qwt3DBar (previously only Qwt3DSurface)
    - New examples: `examples/3D/bar3D` (gaussian peak grid) and `examples/3D/line3D` (helix tube)

- **3D Coordinate Box Aspect Ratio Mode**
    - Added `AUTOFILL`/`DATARATIO` aspect ratio modes for the 3D coordinate box

- **qwtplot3d Comprehensive Demo Example**
    - New example project at `examples/3D/qwtplot3d` with MainWindow and dockable settings for surface styles, axes, legends, view/lighting, and enrichments
    - Settings dock UI syncs with applied theme (`syncFromTheme` method)
    - Legend position combo dynamically updates options based on orientation (vertical: Left/Right, horizontal: Top/Bottom)

### Bug Fixes

- Fixed wheel events not being consumed after zoom handling — `QwtPlotScaleEventDispatcher::handleWheelEvent` now accepts the wheel event after zooming a selected axis, and `QwtMagnifier::widgetWheelEvent` returns bool (signature change: `void` → `bool`) so its event filter can accept and consume the event; previously the wheel event kept propagating to parent widgets (e.g. a `QScrollArea` viewport scrolled while the axis/canvas zoom was applied)
- Fixed dangling pointer: recreate `Qwt3DColorMapColor` in `reapplyAll()` because `applyTheme()` replaces and destroys the old functor
- Stabilized exposed axis label anchor positioning — replaced angle-based heuristic with outward screen vector from coordinate box center to axis midpoint, remaining consistent across viewport aspect ratio changes
- Fixed anisotropic tic-length model — tic length was a single world-space value clobbered by `init()` on every data change; now per-axis auto mode (scale × that axis's own data range) with persistent manual override via `setTicLength()`, new `setTicLengthScale()`/`setAutoTicLength()` API (default scale 0.015)

### Refactoring

- **3D Color Classes Become Pure Value Objects**
    - Removed `Qwt3DSurface*`/`Qwt3DPlot*` reverse pointers and `notifyColorChanged()` from `Qwt3DColor`/`Qwt3DStandardColor`/`Qwt3DColorMapColor`; mutators are now silent; z range pushed via `setActiveRange()`; color headers no longer include/forward-declare `qwt3d_surface.h`/`qwt3d_plot.h`; in-place mutation of an attached functor now requires `surface->invalidateColors()`
- **Eliminated Reverse-Pointer Violations (Deferred Smells Resolved)**
    - Introduced `Qwt3DRenderContext` value struct (bundling shaders, matrices, viewport, coordinate-conversion methods); `Qwt3DDrawable::draw()` now takes `const Qwt3DRenderContext&`; removed `m_plot`/`plot()`/`setPlot()` from `Qwt3DDrawable`
    - `Qwt3DFunction::create()` and `Qwt3DParametricSurface::create()` now return data (`Qwt3DFunctionData`/`Qwt3DParametricData`) instead of pushing via back-pointer; removed `m_surface` from `Qwt3DGridMapping`; added vector-based `loadFromData()` overloads to `Qwt3DSurface`
    - Removed dead-code `plot` back-pointer from `Qwt3DEnrichment`

### Documentation

- Added 3D module refactor dev-guide (en/zh): why/how the 3D module was refactored to a Plot+Item architecture, tiered layering, migration table
- Added dedicated Qwt3DBar and Qwt3DLine usage pages (en/zh) with key features, data shapes/styles tables, class diagrams, method tables, tips, and example links
- Refined 3D Plot Introduction overview (en/zh), replacing inline bar/line usage subsections with pointers to the dedicated pages

## tag:v7.3.5 (2026-07-24)

### Breaking Changes

- **3D Module Architecture Refactor — Plot + Item Pattern**
    - Completely refactored the 3D module from the legacy widget-per-plot model to a **Plot + Item** architecture, symmetric with the 2D module (`QwtPlot` + `QwtPlotItem`):
        - `Qwt3DPlot` (QOpenGLWidget): pure rendering window, manages GL context, view transforms, lighting, coordinate system, and item list. No longer holds plotting data.
        - `Qwt3DPlotItem` (new): abstract base class for all 3D drawable items (attach/detach/draw/hull/z/title), mirroring `QwtPlotItem`.
        - `Qwt3DSurface`: concrete surface item using VBO/VAO + GLSL 3.3 Core shaders. Merged logic from the old `gridplot.cpp` and `meshplot.cpp`.
    - Removed `namespace Qwt3D` — all classes now use the `Qwt3D` prefix in the global scope (e.g., `Qwt3DPlot`, `Qwt3DSurface`, `Qwt3DFunction`).
    - Renamed all 3D classes: `Plot3D` → `Qwt3DPlot`, `SurfacePlot` → `Qwt3DSurface`, `Function` → `Qwt3DFunction`, `CoordinateSystem` → `Qwt3DCoordinateSystem`, `Axis` → `Qwt3DAxis`, `ColorLegend` → `Qwt3DColorLegend`, `Color` → `Qwt3DColor`, `StandardColor` → `Qwt3DStandardColor`, `ColorMapColor` → `Qwt3DColorMapColor`, `Qwt3DTheme` (already prefixed, namespace removed), `Mapping` → `Qwt3DMapping`, `GridMapping` → `Qwt3DGridMapping`, `ParametricSurface` → `Qwt3DParametricSurface`, `Scale` → `Qwt3DScale`, `AutoScaler` → `Qwt3DAutoScaler`, `Enrichment` → `Qwt3DEnrichment`, `Drawable` → `Qwt3DDrawable`, `Label` → `Qwt3DLabel`.
    - Deleted stub classes: `GraphPlot`, `MultiPlot`, `VolumePlot` — to be reimplemented as `Qwt3DPlotItem` subclasses if needed.
    - Moved plotting methods from `Qwt3DPlot` to `Qwt3DSurface`: `setPlotStyle()`, `setDataColor()`, `loadFromData()`, `setResolution()`, `setMeshColor()`, `addEnrichment()`, `setFloorStyle()`, `setShading()`, `showNormals()`.
    - Removed `updateData()` — use `plot->update()` or `item->itemChanged()` instead.
    - Removed `setCoordinateStyle()` — use `Qwt3DCoordinateSystem` API directly.
    - `Qwt3DFunction` and `Qwt3DParametricSurface` now target `Qwt3DSurface*` (item) instead of `SurfacePlot*` (widget).

- **3D Module: Modern OpenGL Migration**
    - Replaced ALL legacy OpenGL calls (immediate mode, display lists, fixed-function pipeline, GLU quadrics) with modern OpenGL:
        - VBO (`QOpenGLBuffer`) + VAO (`QOpenGLVertexArrayObject`) for vertex data management.
        - GLSL 3.30 Core shaders for all rendering (surface, line, point, polygon, text).
        - CPU-side `QMatrix4x4` matrix computation (no GL matrix stack).
        - `QOpenGLTexture` for text labels (replacing `glRasterPos3d` + `glDrawPixels`).
        - CPU-generated triangle mesh geometry for Cone/Arrow enrichments (replacing `gluCylinder`/`gluDisk`).
    - Deleted `qwt3d_openglhelper.h` (legacy GL state management utilities).
    - gl2ps vector export wrapped in `QWT3D_ENABLE_GL2PS` conditional compilation (Compatibility Profile fallback).
    - 10 new GLSL shader files in `src/plot3d/shaders/` (surface, polygon, line, point, text — vert+frag pairs).

- **3D Theme System: Plot-level / Item-level Split**
    - `Qwt3DTheme::apply()` now applies plot-level properties (background, coordinate colors, title, lighting) to `Qwt3DPlot` and item-level properties (meshColor, dataColorPreset, plotStyle, shading, smoothMesh) to attached `Qwt3DSurface` items via `dynamic_cast`.

- **3D IO System Adaptation**
    - `Qwt3DNativeReader` now searches `plot->itemList()` for a `Qwt3DSurface` item (creating one if none exists) instead of `dynamic_cast<SurfacePlot*>(plot)`.
    - Renamed IO classes: `IO` → `Qwt3DIO`, `PixmapWriter` → `Qwt3DPixmapWriter`, `VectorWriter` → `Qwt3DVectorWriter`, `NativeReader` → `Qwt3DNativeReader`.

- **3D Examples Updated**
    - All 6 examples in `examples/3D/` updated to the new Plot + Item API.
    - `SIGNAL()/SLOT()` connections migrated to new-style `connect()`.
    - Legacy GL calls in example enrichments replaced with VBO + shader rendering.

## tag:v7.3.4 (2026-07-12)

### New Features

- **Unified NaN/Inf Handling Across All 2D Modules**
    - Unified 5 scattered NaN detection methods (`qwt_is_nan_or_inf`, `!std::isfinite`, `qIsNaN`, `std::isnan`, local bit-level `qwtIsNaN`) into a single `qwt_is_nan_or_inf()` across `src/plot/` and `src/core/`
    - Promoted `isSampleNanOrInf()` from anonymous namespace to public exported API in `qwt_series_data.h`, available for all plot items
    - Added NaN/Inf skipping to 10+ plot items that previously lacked handling:
        - `QwtPlotBarChart`, `QwtPlotHistogram`, `QwtPlotIntervalCurve`, `QwtPlotMultiBarChart`
        - `QwtPlotTradingCurve`, `QwtPlotVectorField`, `QwtPlotBoxChart`, `QwtPlotSpectroCurve`
        - `QwtPolarCurve`, `QwtPlotSpectrogram` (replaced local bit-level `qwtIsNaN`), `QwtPolarSpectrogram`
    - Added NaN/Inf skipping to 4 missing code paths in `QwtPlotCurve`
    - `QwtColorMap::rgb()` and `colorIndex()` now return transparent for NaN/Inf input values
    - Pre-filters NaN/Inf in `QwtBoxStatistics` before sort and statistical calculations
    - Added bilingual NaN handling user guide (`docs/use-guide/nan-handling.md`)

- **nanperf — NaN Performance Benchmark Example**
    - New example at `examples/2D/nanperf/` for benchmarking NaN handling performance
    - `NanDataGenerator` with configurable NaN distribution patterns and unit tests
    - `BenchmarkRunner` with timing harness and analysis across filter modes and data sizes
    - `MainWindow` with 2×2 grid layout, control bar, sweep orchestration, and Markdown report export
    - `FilterModes` and `PlotPanel` components for side-by-side comparison

- **PySide6 Bindings (POC)**
    - Initial PySide6/Shiboken6 binding for `qwt::core` classes
    - Expanded typesystem to cover all core module classes
    - Full functional test suite (`bindings/tests/test_core.py`)
    - CMake integration with `FindPySide6Shiboken.cmake` and `Shiboken6Tools`

- **QwtDate Modernization**
    - Migrated `QwtDate` internals from deprecated `Qt::TimeSpec` to `QTimeZone`
    - Added `qwt::compat::` timezone helper functions in `qwt_qt5qt6_compat.hpp` for Qt 5.12+/Qt6 dual compatibility
    - Updated `qwt_date_scale_draw.cpp` and `qwt_date_scale_engine.cpp` to use new API

### Bug Fixes

- Fixed `QwtVectorFieldSample` NaN check — was checking `vx` twice, missing `vy`
- Fixed `QwtInterval::contains(NaN)` — was incorrectly returning `true`, now returns `false`
- Fixed `qwtBoundingRect` out-of-bounds access and NaN propagation bugs
- Fixed `QwtLinearColorMap::colorIndex()` — comment claimed NaN handling but implementation lacked it
- Fixed `QwtGridData::findValueRange` NaN propagation and `dyMin` calculation bug
- Fixed `QwtPolarSpectrogram` Indexed code path missing NaN guard

### Build

- Ensured UTF-8 narrow string literals on MinGW (CMake `target_compile_definitions` with `UNICODE`/`_UNICODE`)
- Added `qmakeExample` — demonstrates single-file amalgamated build with qmake

### Documentation

- Added qmake single-file build guide (Chinese, `docs/zh/build-guide/qmake-single-file.md`)
- Added qmake usage instructions to bilingual READMEs
- Fixed incorrect default `PaintAttribute` documentation in curve guides and header comment
- Updated downsampling algorithm recommendations in bilingual curve documentation

## tag:v7.3.2 (2026-06-20)

### New Features

- **QwtColorMapPreset Applied to Spectrograms**
    - `QwtPlotSpectrogram` and `QwtPolarSpectrogram` now use Viridis as the default colormap, replacing the legacy blue-to-yellow linear map
    - Spectrogram and polar spectrogram examples updated with preset switching UI controls
    - Classic colormap types remain available for backward compatibility

- **QwtPolarPlot Background Unification**
    - Unified `QwtPolarPlot` background behavior with `QwtPlot`

### Changes

- **3D Default Colormap Migrated to Viridis**
    - `StandardColor::reset()` replaced blue-to-red gradient with viridis preset
    - `Qwt3DTheme` default preset changed from "jet" to "viridis" for consistency with modern visualization standards
    - Refreshed 3D screenshot assets to reflect the updated default colormap

### Documentation

- **Documentation Restructure** — flattened directory hierarchy, added 40+ new user guides
    - Covers curves, charts, 3D plots, interaction tools, and widgets
    - New developer guides: coding standards, comment conventions, PIMPL pattern usage
    - Updated mkdocs.yml navigation for the new flat structure with i18n folder organization
    - Unified screenshot and example path links

## tag:v7.3.1 (2026-06-17)

### New Features

- **qwt::core Shared Library**
    - Extracted `qwt::core` shared library (`qwtcore.dll`) containing color mapping and palette utilities
    - `QwtColorMap` refactored: removed `QwtInterval` dependency, new signature `rgb(vMin, vMax, value)`
    - `QwtColorCycle` migrated from plot module to core
    - New `QwtColorMapPreset` class with 22 scientific colormap presets (viridis, plasma, inferno, magma, cividis, jet, hot, cool, spring, summer, autumn, winter, gray, bone, copper, rainbow, hsv, turbo, coolwarm, rdylbu, rdylgn, spectral)
    - Backward-compatible `QwtColorMapCompat` namespace in `src/plot/qwt_colormap_compat.h` providing `QwtInterval`-based overloads

- **3D Theme System**
    - New `Qwt3DTheme` class with 10 preset themes: Default, Dark, Scientific, Warm, Cool, Matplotlib, EarthTones, Ocean, HighContrast, Presentation
    - New `ColorMapColor` adapter bridging 2D colormaps to 3D color functors
    - 5 lighting presets: NoLighting, FlatLight, Studio, Outdoor, Soft
    - `Plot3D::setTheme()`, `Plot3D::applyTheme()` for one-call theme application
    - `StandardColor::setPreset()` for quick colormap preset switching

### Architecture Changes

- Three-module architecture: `qwtcore.dll` → `qwtplot.dll` + `qwtplot3d.dll`
- `plot` and `plot3d` both link against `core` but remain independent of each other
- Runtime deployment requires `qwtcore.dll` + `qwtplot.dll` (+ `qwtplot3d.dll` if 3D enabled)
- Backward-compatible forwarding headers in `src/plot/` for existing `#include "qwt_color_map.h"` paths
- **Core Module Expansion**: Migrated 24 utility modules from `plot` to `core`, expanding core from 4 modules (color + global) to a complete foundational library of 28 modules:
    - Math utilities: `qwt_math.h/.cpp`, `qwt_simd_argminmax.h/.cpp`
    - Data types: `QwtInterval`, `QwtPoint3D`, `QwtPointPolar`, `QwtSamples`, `QwtBoxStatistics`
    - Geometry: `QwtBezier`, `QwtClipper`
    - Coordinate transforms: `QwtTransform`, `QwtScaleMap`, `QwtScaleDiv`, `QwtScaleEngine`
    - Date/Time: `QwtDate`, `QwtSystemClock`
    - Algorithms & compatibility: `qwt_algorithm.hpp`, `qwt_qt5qt6_compat.hpp`
    - Data containers: `QwtGridData`
    - Data series: `QwtSeriesData`, `QwtPointData`, `QwtSeriesStore`
    - Raster data: `QwtRasterData`, `QwtMatrixRasterData`, `QwtGridRasterData`

### Build System

- New `src/core/CMakeLists.txt` for the core library target
- Updated `src/plot/CMakeLists.txt` to link against `qwt::core`
- Updated `src/plot3d/CMakeLists.txt` to link against `qwt::core` and include theme sources
- Updated amalgamate tool templates to include core module files
- Updated `classincludes/` headers to point to core module paths
- Updated `tools/make-classinclude.py` for new module structure

### Examples

- **figureSurface3D** — added theme switching UI and fixed color legend refresh on theme change

### Bug Fixes

- Fixed 3D animation auto-start crash: deferred animation timer to `showEvent()` to prevent GL context access before widget is visible
- Fixed `figureSurface3D` color legend not refreshing when theme is switched

## tag:v7.3.0 (2026-06-12)

### New Features

- **QwtColorCycle — Color Cycle System**
    - Added `QwtColorCycle` class for managing color palettes with cyclic wrap-around, 7 built-in presets plus custom palettes
    - Integrated with `QwtPlot` — plot items automatically acquire colors from the color cycle on `attach()`
    - Supported items: `QwtPlotCurve`, `QwtPlotMultiBarChart`, `QwtPlotBoxChart`, `QwtPlotHistogram`, `QwtPlotIntervalCurve`, `QwtPlotBarChart`
    - Added color cycle unit tests

- **Curve Downsampling Filters**
    - Added `FilterPointsPixel` and `FilterPointsLTTB` paint attributes to `QwtPlotCurve`
        - `FilterPointsPixel`: pixel-column based downsampling for maximum rendering speed on very large datasets
        - `FilterPointsLTTB`: simplified LTTB (MinMax bucket) algorithm that preserves visual shape while downsampling
    - Changed default paint attribute from `FilterPoints` to `FilterPointsAggressive`
    - `qwtMapPointsQuad` improved NaN/Inf skipping logic and polygon memory pre-allocation

- **SIMD-Accelerated MinMax Bucket Downsampling**
    - Added `qwt_simd_argminmax` module providing SIMD-accelerated argmin/argmax computation
    - Supports SSE2/AVX2/NEON instruction sets with automatic runtime detection
    - Significantly boosts MinMax bucket downsampling performance

- **Picker Enhancements**
    - `QwtPlotSeriesDataPickerGroup` now provides click position (screen and data coordinates) on click events
    - `QwtPlotSeriesDataPicker` extended to handle all `QwtPlotSeriesItem` subtypes:
        - Trading curves (candlestick), interval curves, histograms, vector fields
        - Bar charts, box charts, spectro curves
    - `FeaturePoint` gained a `sampleData` field (QVariant) carrying full sample data (OHLC, interval, vector, etc.)
    - Sample types registered via `Q_DECLARE_METATYPE` for cross-module passing

- **Flat Style Controls**
    - `QwtSimpleCompassRose` — new `flatStyle` property replaces classic two-tone 3D effect with solid colors
    - `QwtWheel` — new `flatStyle` property replaces 3D gradients and borders with flat colors and simple lines
    - `QwtSlider` and `QwtThermo` — new `flatStyle` property for modern flat UI themes
    - `QwtDialNeedle` — multiple flat-style needle designs added
    - `QwtKnob` — defaults to `Flat` mode

### Performance

- `qwtProbeOrientation` and `qwtFindVisibleRange` optimized sampling strategy — probes up to 50 points with a minimum step of 1
- `qwtPixelColumnReduce` — pre-sized polyline vector, direct pointer assignment instead of dynamic appends
- Bins initialization simplified via `QVector` constructor

### Refactoring

- **PIMPL Macro Unification** (180+ files)
    - All classes migrated from manual PIMPL (raw `PrivateData* m_data`) to standard macro pattern
    - Headers use `QWT_DECLARE_PRIVATE(ClassName)`, implementations use `QWT_DECLARE_PUBLIC` + `QWT_PIMPL_CONSTRUCT` + `QWT_D/QWT_DC`
    - `unique_ptr` manages lifetime automatically; all `delete m_data` removed
    - Covers all modules: plot core, axes, text/legend, color maps, canvas, bar charts, interaction, data, splines, widgets, polar, etc.

- **3D Plot Module Overhaul** (53 files)
    - Added `override` specifiers to all virtual functions and destructors
    - `nullptr` replaces `0` for null pointer constants
    - `SurfacePlot` refactored to PIMPL pattern (`QWT_DECLARE_PRIVATE`)
    - `qwt3d_ptr` renamed to `ClonePtr`, unified include guards

- **Modern C++ Adoption**
    - `auto` keyword and range-based for loops throughout the codebase
    - Move semantics and C++11 default member initializers
    - `noexcept` added to trivial getters, setters, and constructors
    - `constexpr` expanded to intervals, axes, scale maps, and math utilities
    - C++ idiom modernization across `src/plot` and `src/plot3d`

### New Examples

- **renderbench** — curve rendering performance benchmark
    - `WindowedSeriesData` for real-time data stream simulation
    - Control panel for adjusting data size, downsampling mode, line width, etc.
    - Generates Markdown-formatted performance reports
    - Located at `examples/bench/renderbench/`
- **staticExample** — rewritten as comprehensive Qwt feature showcase
    - Demonstrates curves, spectrograms, bar charts, box charts, vector fields, and more
    - Modernized color palette and canvas backgrounds across all 2D examples

### Documentation

- Complete curve downsampling algorithm documentation
- Added `QWEN.md` AI Agent guidelines, updated build instructions
- Added English documentation and restructured bilingual documentation site
- Enforced English-only Doxygen comments across all source files
- Large-scale bilingual Doxygen comment completion (40+ batches) covering all core headers

### Bug Fixes

- Fixed Qt6 legend icon appearing blank due to `QwtNullPaintDevice::metric()` returning 0 for device pixel ratio
- Fixed `QwtPlotMarker` arrow endpoints — head and tail were swapped; improved legend icon rendering
- Fixed plot background rendering — now uses dynamic `backgroundRole` from `QPalette::Base` for correct theme-aware coloring

## tag:v7.2.1

- **Inside Tick Display**
    - Added `QwtPlot::TickDirection` enum: `TickOutside` (default) and `TickInside`
    - Added `QwtPlot::setAxisTickDirection()` and `QwtPlot::axisTickDirection()` to control tick direction per axis
    - Inside ticks extend from the canvas edge inward; the backbone and labels remain outside the canvas
    - Inside and outside ticks share the same style settings (length, color, pen width)
    - New example: `examples/2D/ticks_inside/`
    ![ticks_inside](./docs/assets/screenshots/ticks_inside.png)

## tag:v7.2.0

- **Box Chart (Box-and-Whisker Plot)**
    - Added `QwtPlotBoxChart` class for box-and-whisker plots
    - Added `QwtBoxStatisticsCalculator` for automatic statistics computation
    - New data structures: `QwtBoxSample`, `QwtBoxOutlierSample`, `QwtBoxChartData`, `QwtBoxOutlierChartData`
    - Supports both pre-computed and raw data input
    - Box styles: Rect, Diamond, Notch
    - Whisker methods: Tukey (1.5×IQR), percentile, min-max, standard deviation, standard error
    - Vertical and horizontal orientations
    - Automatic outlier detection with custom symbols and jitter display
    - New example: `examples/2D/boxchart/`
    ![Box Chart](./docs/assets/screenshots/BoxChart.png)

## tag:v7.0.8

- `QwtFigure` axis alignment — added `addAxisAlignment` to align sub-plot axes
    ![axis-alignment](./docs/assets/picture/figure-scale-aligment.png)
- `QwtPointMapper` NaN/Inf handling — prevents coordinate mapping corruption from abnormal data values
- `qwt_series_data.cpp` — NaN/Inf checks added to data range computation
- Fixed missing `QwtSlider` class in amalgamated file (Issue #4)

## tag:v7.0.7

- Moved `QwtScaleWidget::panScale` to `QwtPlot` as `panAxis`; fixed panning on logarithmic axes
- Renamed panner classes to reflect caching behavior:
    - `QwtPanner` → `QwtCachePanner`
    - `QwtPlotPanner` → `QwtPlotCachePanner`
    - `QwtPolarPanner` → `QwtPolarCachePanner`

    `QwtPlotPanner` (new, real-time):
    ![series-data-picker-yvalue](./docs/assets/screenshots/qwt-realtime-panner.gif)

- Rewrote `QwtPlotPanner` — now inherits `QwtPicker` for real-time dragging; original cached panner available as `QwtPlotCachePanner`
- Added `QwtCanvasPicker` base class for canvas-specific picker operations
- `QwtPlotPanner` supports linear, logarithmic, and multi-axis real-time panning
- Added `QwtPlotCanvasZoomer` — zooms the entire canvas without axis binding, supports multi-axis zoom
- Renamed `QwtPlotZoomer` → `QwtPlotAxisZoomer`
- `QwtPlotSeriesDataPicker` now inherits `QwtPicker` directly (no longer `QwtPlotPicker`)
- `QwtPlotMagnifier` supports multi-axis scaling
- `QwtScaleMap` — added move semantics
- Added `make-classinclude.py` for exporting the `classincludes` directory

v7.0.7 naming changes:

| Original Name | New Name | Note |
|:--|:--|:--|
| QwtPlotZoomer | QwtPlotAxisZoomer | Renamed — original could only bind 2 axes |
| (new) | QwtPlotCanvasZoomer | Whole-canvas zoomer |
| QwtPanner | QwtCachePanner | Original cannot drag in real-time, hence "Cache" |
| QwtPlotPanner | QwtPlotCachePanner | Same as above |
| QwtPolarPanner | QwtPolarCachePanner | Same as above |
| (new) | QwtPlotPanner | New real-time panner replacing the renamed one |

## tag:v7.0.6

- Added `QwtPlotSeriesDataPicker` for plot data picking
    ![series-data-picker-yvalue](./docs/assets/picture/series-data-picker-yvalue.gif)
    ![series-data-picker-nearest-value](./docs/assets/picture/series-data-picker-nearest-value.gif)
- Improved parasite plot refresh mechanism — full update on construction
- Changed some interface index types from `int` to `size_t`
- `QwtPlotSeriesDataPicker` supports date axis display
- Fixed `QwtFigureWidgetOverlay` event handling conflicting with axis actions

## tag:v7.0.5

- `QwtScaleWidget` built-in axis actions — pan and zoom
    - Added `scaleRect()` to get the tick region rectangle
    - Added `QwtScaleWidget::BuiltinActions` enum
    - Added `requestScaleRangeUpdate` signal for axis range refresh
    - Added `setSelected`/`isSelected` and `selectionChanged` signal
    - Added action properties: `zoomFactor`, `panScale`, etc.
- Added `QwtPlotScaleEventDispatcher` for axis event handling on plots
- Adjusted `QwtPlot` parasite plot interfaces
- Added `setEnableScaleBuildinActions`/`isEnableScaleBuildinActions`/`setupScaleEventDispatcher` to `QwtPlot`
- Added `QwtPlot::plotList()` — returns all plot objects including parasite and host plots

## tag:v7.0.4

- Implemented parasite plot functionality
- Added parasite plot example
- Added `QwtPlot::updateAxisEdgeMargin` for automatic axis positioning with parasite plots

## tag:v7.0.2

- Extracted `QwtPlotLayoutEngine` class
- Added `QwtPlotParasiteLayout` for parasite axis layout
- Added `QwtPlotTransparentCanvas` — fully transparent canvas
- `QwtFigure` — parasite axis support
- `QwtScaleWidget` — added `setEdgeMargin`/`edgeMargin` for tick-to-edge distance
- Adjusted `QwtScaleWidget` layout to support `edgeMargin`
- Added `parasitePlot` example
- `QwtPlot` — added `rescaleAxes`/`syncAxis` for quick axis adjustment
- NaN and Inf exception handling

## tag:v7.0.1

- Added `QwtFigure` — manages multiple `QwtPlot` instances in a grid layout (similar to Matplotlib Figure)

![figure](docs/assets/screenshots/qwt_figure.png)

- Added `figure` example demonstrating `QwtFigure` usage
- Added `QwtFigureWidgetOverlay` for interactive actions on `QwtFigure` — supports dragging and zooming sub-plots

## tag:v7.0.0

- Amalgamated entire project into single `QwtPlot.h` / `QwtPlot.cpp` files in `src-amalgamate/`
- Added `staticExample` example
- Enhanced `QwtPlotBarChart` interface to support pen and brush configuration
- Added `QwtGridRasterData` — supports 2D data tables with grid interpolation on X/Y axes (vs. `QwtMatrixRasterData`)
- `QwtLinearColorMap` — added `stopColors()`, renamed `colorStop()` → `stopPos()`
- Adjusted default initialization for a more modern look

`QwtPlotCanvas` initialization:
```cpp
QwtPlotCanvas::QwtPlotCanvas(QwtPlot* plot) : QFrame(plot), QwtPlotAbstractCanvas(this)
{
    ...

    setLineWidth(0);
    setFrameShadow(QFrame::Plain);
    setFrameShape(QFrame::Box);
}
```

`QwtPlotLayout` initialization:
```cpp
QwtPlotLayout::QwtPlotLayout()
{
...
    setCanvasMargin(-1);
...
}
```

- Removed `QWT_MOC_INCLUDE`
- Adjusted class implementations for single-file amalgamation

------

**Qwt 7.0** is a modified version based on Qwt 6.2.0 source code. It complies with Qwt's open-source license, and the modifications are released as open-source.

Starting from this version, Qwt supports:
- The latest **C++11 standard**
- **CMake build system**

**Modifications in this version:**
1. Enhanced `QwtPlotBarChart` interface to support **pen and brush configuration**
2. Added `QwtGridRasterData` class — supports **2D data tables** with **grid interpolation** on X/Y axes (vs. `QwtMatrixRasterData`)
3. Added `stopColors()` function to `QwtLinearColorMap` and renamed `colorStop()` to `stopPos()`



Qwt 6.2.0

------

0) Requirement for Qt >= 4.8

1) Class Includes added

Include files, that match the class names are available now. So
it is possible to write "#include <QwtPlot>" now instead of "include qwt_plot.h"

2) BSD License for examples

Where possible the code of the examples is available under the 3-clause BSD License

3) MathML text renderer removed

The code can be found at https://github.com/uwerat/qwt-mml-dev now and is intended
to become a standalone lib. Anyone who is interested to work on it, please let me know.

4) Spline interpolation

The broken implementation of QwtSpline has been replaced by a bunch of classes
offering all sort of functionalities around splines.

The most popular spline approximation/interpolation algos have been implemented:

	- Basis
	- Cardinal
	- ParabolicBlending
	- Akima
	- The one used in MS Excel
	- Cubic

An implementation of the de Casteljau's algorithm has been added

	- QwtBezier

5) New plot items

	- QwtPlotVectorField
      A new type of plot item for vector fields

    - QwtPlotGraphicItem
      An item displaying a QwtGraphic image ( f.e used by QwtPlotSvgItem )

6) Plot Canvas

	- QwtAbstractPlotCanvas introduced
	- QwtPlotOpenGLCanvas added to support QOpenGLWidget

7) QwtPlotCurve

	- QwtPlotCurve::FilterPointsAggressive mode added - a fast weeding algo
      for huge datasets with increasing x or y values

	- QwtPlotCurve::closestPoint is virtual now
	- QwtPlotCurve: polygon clipping includes the painter clip
	- QwtPlotCurve::setLegendAttributes added
	- QwtValuePointData added for curves, where the x values are the index
	- a couple of new QwtPlotCurve::setSamples alternatives

8) QwtPlotSpectrogram

	- QwtPlotSpectrogram::setColorTableSize added
	- QwtRasterData::setInterval/interval changed into a pure virtual getter
	- QwtMatrixRasterData::BicubicInterpolation added
	- QwtMatrixRasterData::interval: API cleanup
	- QwtHueColorMap, QwtSaturationValueColorMap added

9) QwtPlotRenderer

	- using QPdfWriter where possible

10)
	- LOG_MIN/LOG_MAX removed, use QwtTransform::LogMin/LogMax instead ( values differ ! )
	- qwt_compat.h removed
	- qwtFuzzyGreaterOrEqual/qwtFuzzyLessOrEqual removed
	- qwtGetMin/qwtGetMax removed

11)
    - Not aligning unknown paint engines ( f.e EMF )
    - QwtNullPaintDevice is using a different type than QPaintEngine::User now

12) Many other changes ...
