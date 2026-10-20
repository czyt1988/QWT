# 3D Module Refactoring Guide

The 3D plotting module (`qwt::plot3d`) was completely refactored: the monolithic widget and its legacy OpenGL renderer became a Plot+Item architecture with Core-Profile rendering.

> **Info — Scope**
>
> The refactor ships in v7.3.5 on the `new3d` branch. For the end-user usage guide see [3D Plot Introduction](../use-guide/3d/3d-plot.md); for the per-module conventions see [`src/plot3d/AGENTS.md`](../../src/plot3d/AGENTS.md).

## 1. Why Refactor

The 3D module originated from the standalone `QwtPlot3D` library (Qt 3/4 era) and was integrated into Qwt 7.1. Over the years it accumulated several structural debts that blocked further development:

- Responsibilities were coupled: the 3D widget mixed window management, view transforms, and plotting data/style in a single class, so adding a new plot type meant editing the central widget.
- The renderer used the fixed-function pipeline (`glBegin/glEnd`, display lists, `gluSphere`/`gluCylinder`, GL matrix stack). That pipeline is deprecated, slow, and incompatible with the OpenGL Core Profile required by modern Qt, ANGLE and OpenGL ES.
- Low-level value objects (color functors, drawables, data mappings) held back-pointers to the top-level widget, creating fragile cycles that caused dangling-pointer crashes and made testing impossible.
- Types lived in `namespace Qwt3D`, while the rest of the library used the `Qwt` class-prefix convention in the global scope.
- The 2D module had long adopted a clean `QwtPlot` (canvas) + `QwtPlotItem` (data) split; 3D had no such separation, so patterns could not be shared.

Every new feature (themes, NaN handling, new plot styles) therefore risked regressions elsewhere. The refactor aligned 3D with the 2D architecture and modern rendering practice.

## 2. Original Architecture

### 2.1 Monolithic `Plot3D` widget

The old `Plot3D` (in `namespace Qwt3D`) was a `QOpenGLWidget` subclass that held the coordinate system, transforms, mouse and keyboard handling, and also the data, color and style:

```cpp
namespace Qwt3D {

class QWT3D_EXPORT Plot3D : public QOpenGLWidget
{
    // ... view transforms, rotation/shift/scale getters ...

    // Data & style were ALSO on the widget:
    void setPlotStyle(Qwt3D::PLOTSTYLE val);
    void setDataColor(Color* col);
    void setMeshColor(Qwt3D::RGBA rgba);
    void setShading(Qwt3D::SHADINGSTYLE val);
    Qwt3D::Enrichment* addEnrichment(Qwt3D::Enrichment const&);
    void showColorLegend(bool);
    void updateData();
    // ...
};

} // namespace Qwt3D
```

`SurfacePlot` subclassed `Plot3D` and added surface-specific loading. Because data and rendering lived together, there was no way to attach multiple independent datasets to one window.

### 2.2 Legacy OpenGL rendering

The old `Drawable` base class managed GL state directly with raw fixed-function arrays:

```cpp
class QWT3D_EXPORT Drawable
{
    // ...
    virtual void saveGLState();
    virtual void restoreGLState();

protected:
    GLdouble modelMatrix[16];
    GLdouble projMatrix[16];
    GLint viewport[4];
};
```

Rendering relied on immediate mode, display lists, GLU quadrics, and the GL matrix stack (`glRotatef`, `glRasterPos3d` + `glDrawPixels` for text).

### 2.3 Reverse pointers (the "deferred smells")

Low-level classes reached up to the top-level widget, violating the dependency direction:

- `Drawable::m_plot`: the Tier-2 drawable base baked in a `Qwt3D::Plot*` back-pointer.
- `GridMapping::m_surface`: the Tier-3 data source held a `SurfacePlot*`:

```cpp
class QWT3D_EXPORT GridMapping : public Mapping
{
protected:
    Qwt3D::SurfacePlot* plotWidget() const;
    void setPlotWidget(Qwt3D::SurfacePlot* pw);
};
```

Color functors likewise stored a `Qwt3D::Plot*`/`SurfacePlot*` and called back into the widget from their mutators (e.g. to trigger `invalidateColors()`), mirroring an anti-pattern 2D had deliberately avoided.

### 2.4 Original file structure

The pre-refactor `src/plot3d/` layout: a monolithic plot widget, unused stubs, plotting logic split across files, and the classes carrying reverse pointers:

```
src/plot3d/
  qwt3d_plot.{h,cpp}            Plot3D widget (monolithic)
  qwt3d_surfaceplot.{h,cpp,_p.h} SurfacePlot subclass
  qwt3d_graphplot.h             stub base (unused)
  qwt3d_multiplot.h             stub (unused)
  qwt3d_volumeplot.h            stub (unused)
  qwt3d_meshplot.cpp            plotting logic (split across files)
  qwt3d_gridplot.cpp            plotting logic (split across files)
  qwt3d_dataviews.cpp           data view glue
  qwt3d_openglhelper.h          legacy GL state helpers
  qwt3d_drawable.{h,cpp}        base with m_plot back-pointer
  qwt3d_gridmapping.{h,cpp}     data mapping with m_surface back-pointer
  ... (axis, label, color, coordsys, theme, io, ...)
```

## 3. Problems Summary

| Problem | Consequence |
|---------|-------------|
| Monolithic widget (data + rendering + window) | Cannot attach multiple datasets; every plot type edits the central class |
| Legacy fixed-function OpenGL | Incompatible with Core Profile; slow; deprecated GLU calls |
| `namespace Qwt3D` | Naming inconsistency with rest of library |
| Reverse pointers (drawable→plot, mapping→surface, color→surface) | Dangling-pointer crashes; cycles; untestable value objects |
| Stub classes (`GraphPlot`, `MultiPlot`, `VolumePlot`) | Dead code masquerading as extensibility points |
| Split plotting logic across `meshplot.cpp`/`gridplot.cpp` | Hard to follow the surface rendering path |
| 3D diverges from 2D `QwtPlot`+`QwtPlotItem` | No shared mental model or patterns |

## 4. Refactoring Approach

The refactor was driven by five principles, applied incrementally across the `new3d` branch commits.

### 4.1 Plot + Item architecture (mirror 2D)

Adopt the same separation as 2D's `QwtPlot` + `QwtPlotItem`: the widget becomes a pure rendering window, and each dataset becomes an item that attaches to it.

```mermaid
classDiagram
    class Qwt3DPlot {
        +attach(item)
        +detach(item)
        +itemList()
        +paintGL()
        -QList~Qwt3DPlotItem~ m_items
    }
    class Qwt3DPlotItem {
        <<abstract>>
        +attach(plot)
        +draw()*
        +hull()*
        +itemChanged()
        +setZ(z)
    }
    class Qwt3DSurface {
        +loadFromData()
        +setPlotStyle()
        +setDataColor()
        +invalidateColors()
        -VBO/VAO/shader
    }
    Qwt3DPlot o-- Qwt3DPlotItem : manages
    Qwt3DPlotItem <|-- Qwt3DSurface
```

`Qwt3DPlot` keeps only GL context, view transforms, lighting, coordinate system, theme, and the item list. All data/style methods (`setPlotStyle`, `setDataColor`, `loadFromData`, `setMeshColor`, `setFloorStyle`, `addEnrichment`, `setShading`, `showNormals`) moved to `Qwt3DSurface`.

### 4.2 Modern OpenGL migration

All legacy GL was replaced with modern, Core-Profile-safe rendering:

- Vertex data lives in a VBO (`QOpenGLBuffer`) and a VAO (`QOpenGLVertexArrayObject`), uploaded once and reused across frames.
- Every primitive type uses a GLSL 3.30 Core shader: 10 shader files under `src/plot3d/shaders/` (`surface`, `polygon`, `line`, `point`, `text`; vert+frag pairs).
- Matrices are computed on the CPU with `QMatrix4x4`; there is no GL matrix stack.
- Text labels use `QOpenGLTexture`, replacing `glRasterPos3d` + `glDrawPixels`.
- Cone and arrow enrichments use CPU-generated geometry instead of `gluCylinder`/`gluDisk`.
- `gl2ps` vector export kept as a Compatibility Profile fallback, compiled in unconditionally (system gl2ps, or the bundled copy under `src/plot3d/3rdparty/gl2ps/`).
- Deleted `qwt3d_openglhelper.h`.

### 4.3 De-namespace + `Qwt3D` prefix

Removed `namespace Qwt3D` entirely. All types now use a `Qwt3D` class prefix in the global scope (e.g. `Qwt3DPlot`, `Qwt3DSurface`, `Qwt3DFunction`), matching the rest of the library.

### 4.4 Eliminate reverse pointers

Two "deferred smells" were resolved by inverting the dependency direction:

Smell #1: `Qwt3DDrawable::m_plot`. A value struct `Qwt3DRenderContext` bundles the matrices, viewport, and shared shader programs current at a given `paintGL()` call site. It is passed by const reference down the `draw()` chain, so drawables no longer need a widget pointer:

```cpp
struct QWT3D_EXPORT Qwt3DRenderContext
{
    QMatrix4x4 modelView;
    QMatrix4x4 projection;
    QSize viewport;
    QOpenGLShaderProgram* lineShader = nullptr;
    QOpenGLShaderProgram* polygonShader = nullptr;
    QOpenGLShaderProgram* textShader = nullptr;

    QPointF worldToScreen(const Triple& world) const;
    Triple screenToWorld(const QPointF& screen) const;
    Triple relativePosition(Triple rel) const;
};

class Qwt3DDrawable
{
public:
    virtual void draw(const Qwt3DRenderContext& ctx);  // no back-pointer
};
```

Smell #2: `Qwt3DGridMapping::m_surface`. `create()` now returns data structures (`Qwt3DFunctionData` / `Qwt3DParametricData`) that the caller feeds to `Qwt3DSurface::loadFromData()`, instead of the mapping holding a surface pointer. `m_surface`, `surface()`, `setSurface()`, `assign()`, and the `Qwt3DSurface*` constructor were removed.

### 4.5 Color functors as pure value objects

Color classes (`Qwt3DColor`, `Qwt3DStandardColor`, `Qwt3DColorMapColor`) were made pure value objects: no `Qwt3DSurface*`/`Qwt3DPlot*` pointer, no callbacks, no signals. Mutators are silent. The z-range is pushed in as data by `Qwt3DSurface::pushColorRange()` rather than pulled by the functor. After mutating an attached functor, the caller must invoke `surface->invalidateColors()`, which mirrors 2D's "mutate the `QwtSymbol`, then call `itemChanged()`" contract.

### 4.6 Tiered dependency layering

Classes were organized into tiers; dependencies may only point downward (or same tier). The only legal upward pointer is `Qwt3DPlotItem::plot()` (item→widget), because rendering needs the widget's GL context.

| Tier | Files | Notes |
|------|-------|-------|
| 0 — pure values/tools | `qwt3d_types`, `qwt3d_global`, `qwt3d_portability`, `qwt3d_helper`, `qwt3d_autoptr`, `qwt3d_autoscaler`, `qwt3d_scale`, `qwt3d_mapping` | no 3D class pointers |
| 1 — silent value objects | `qwt3d_color`, `qwt3d_colormap_color`, `qwt3d_enrichment`(+`_std`), `qwt3d_theme` | never notify anyone |
| 2 — drawables | `qwt3d_drawable`(base), `qwt3d_label`, `qwt3d_axis`, `qwt3d_colorlegend`, `qwt3d_coordsys` | receive `Qwt3DRenderContext`, no back-pointer |
| 3 — data-source mappings | `qwt3d_gridmapping`, `qwt3d_function`, `qwt3d_parametricsurface` | `create()` returns data, no sink pointer |
| 4 — items | `qwt3d_plotitem`(base), `qwt3d_surface`(+`_p`), `qwt3d_bar`(+`_p`), `qwt3d_line3d`(+`_p`) | holds `Qwt3DPlot*` (legal) |
| 5 — widget | `qwt3d_plot`(+`_p`), `qwt3d_lighting`, `qwt3d_mousekeyboard`, `qwt3d_movements` | high holds low |
| Cross-cutting I/O | `qwt3d_io`, `qwt3d_io_reader`, `qwt3d_io_gl2ps` | takes `Qwt3DPlot*` as functor param only, never stores it |

## 5. New Architecture

### 5.1 Class responsibilities

The flowchart below maps the new responsibilities: `Qwt3DPlot` as the pure rendering window, `Qwt3DPlotItem` as the abstract base, and `Qwt3DSurface` owning data, VBO/VAO resources, color functor and enrichments:

```mermaid
flowchart TD
    subgraph Widget["Qwt3DPlot (Tier 5 — pure window)"]
        GL["GL context / view transforms / lighting / coord system / theme / item list"]
        GL -->|"paintGL: for each item sorted by z"| Loop["item->draw()"]
    end
    subgraph Item["Qwt3DPlotItem (Tier 4 — abstract)"]
        IT["attach/detach/draw/hull/z/title/itemChanged"]
    end
    subgraph Surface["Qwt3DSurface (Tier 4 — concrete)"]
        Data["data storage"]
        VBO["VBO/VAO/EBO/shader"]
        Color["Qwt3DColor functor (Tier 1)"]
        Enr["list of Qwt3DEnrichment (Tier 1)"]
        Data --> VBO
        Color --> VBO
        VBO --> draw
        Enr --> draw
    end
    Widget -->|"ctx = Qwt3DRenderContext"| Surface
    Surface -.->|"inherits from"| Item
    Surface -->|"plot() up-ref (legal)"| Widget
```

- `Qwt3DPlot` is the pure rendering window. It builds a `Qwt3DRenderContext` per draw phase and hands it to drawables, iterates items by z-order, and exposes shared shader accessors (`lineShader()`, `polygonShader()`, `textShader()`).
- `Qwt3DPlotItem` is the abstract base: `attach/detach/draw/hull/z/title/setVisible/itemChanged/populateLegendColors`. It mirrors `QwtPlotItem`.
- `Qwt3DSurface` is the concrete surface item. It owns data, VBO/VAO/EBO/shader, color functor, and enrichments, and holds the methods moved off the old widget (`setPlotStyle`, `setDataColor`, `loadFromData`, `setResolution`, `setMeshColor`, `addEnrichment`, `setFloorStyle`, `setShading`, `showNormals`).
- `Qwt3DBar` / `Qwt3DLine` are additional concrete `Qwt3DPlotItem` subclasses added in v7.3.5, demonstrating the extension point: a bar chart (per-bar cuboids with flat per-face normals; 1D series or 2D grid) and a line/curve item (`Tube`/`Lines`/`Dots`; the Tube style sweeps a circular cross-section with parallel-transport framing). Like `Qwt3DSurface`, they own VBO/VAO + a `Qwt3DColor` functor and implement `draw()`/`hull()`/`populateLegendColors()`; both are wired into `Qwt3DTheme::applyToItem()`.

### 5.2 Theme system split

`Qwt3DTheme::apply()` now splits responsibilities: plot-level properties (background, coordinate colors, title, lighting) apply to `Qwt3DPlot`, while item-level properties (meshColor, dataColorPreset, plotStyle, shading, smoothMesh) apply to attached `Qwt3DSurface` items via `dynamic_cast`.

### 5.3 New file structure

The post-refactor `src/plot3d/` layout, with the new item classes, render context and shader directory:

```
src/plot3d/
  qwt3d_plot.{h,cpp,_p.h}       Qwt3DPlot — pure rendering window (PIMPL)
  qwt3d_plotitem.{h,cpp}        Qwt3DPlotItem — abstract item base (NEW)
  qwt3d_surface.{h,cpp,_p.h}    Qwt3DSurface — surface item (replaces surfaceplot) (NEW)
  qwt3d_bar.{h,cpp,_p.h}        Qwt3DBar — 3D bar chart item (NEW, v7.3.5)
  qwt3d_line3d.{h,cpp,_p.h}    Qwt3DLine — 3D line/curve item (Tube/Lines/Dots) (NEW, v7.3.5)
  qwt3d_render_context.{h,cpp}  Qwt3DRenderContext — value struct (NEW)
  qwt3d_drawable.{h,cpp}        base drawable, draw(ctx) — no back-pointer
  qwt3d_color.{h,cpp}           pure value-object color functors
  qwt3d_colormap_color.{h,cpp}  adapter bridging core QwtColorMap → 3D
  qwt3d_theme.{h,cpp}           theme system (plot/item split)
  qwt3d_coordsys / axis / label / colorlegend  modernized to VBO/VAO + shaders
  qwt3d_function / parametricsurface  create() returns data, targets Qwt3DSurface*
  shaders/                      surface/polygon/line/point/text vert+frag (NEW)
  AGENTS.md                     per-module AI agent guide (NEW)
```

Removed: `graphplot.h`, `multiplot.h`, `volumeplot.h`, `meshplot.cpp`, `gridplot.cpp`, `surfaceplot.*`, `dataviews.cpp`, `openglhelper.h`.

## 6. Benefits

1. `Qwt3DPlot`+`Qwt3DPlotItem` mirrors `QwtPlot`+`QwtPlotItem`; patterns such as attach, z-order, `itemChanged` and value-object styles transfer directly between 2D and 3D.
2. Any number of `Qwt3DPlotItem`s can attach to one `Qwt3DPlot`; the old architecture could not do this.
3. VBO/VAO and GLSL shaders are faster, portable to ES/ANGLE, and avoid all deprecated GLU and immediate-mode calls.
4. The tiered layering with a single legal up-pointer (`item→plot`) eliminates dangling-pointer crashes and makes low-level classes unit-testable in isolation.
5. Color functors and themes are silent, copyable and self-contained, matching the 2D `QwtColorMap`/`QwtSymbol` contract.
6. A new 3D plot type is just a new `Qwt3DPlotItem` subclass; the widget never needs editing. (The old stubs `GraphPlot`/`MultiPlot`/`VolumePlot` were deleted as dead code.) `Qwt3DBar` and `Qwt3DLine` in v7.3.5 were each added as a self-contained item with no changes to `Qwt3DPlot`.
7. Dropping `namespace Qwt3D` for the `Qwt3D` class prefix unifies the library's naming convention.
8. `Qwt3DColorMapColor` bridges the core module's 22 scientific colormap presets (viridis, plasma, …) directly onto 3D surfaces, so 2D and 3D share one color system.

## 7. Migration Reference

| Old (v7.3.4) | New (v7.3.5) |
|--------------|--------------|
| `Qwt3D::Plot3D` | `Qwt3DPlot` |
| `Qwt3D::SurfacePlot` | `Qwt3DSurface` (item) |
| `Qwt3D::Function` | `Qwt3DFunction` (targets `Qwt3DSurface*`) |
| `Qwt3D::ParametricSurface` | `Qwt3DParametricSurface` |
| `Qwt3D::CoordinateSystem` | `Qwt3DCoordinateSystem` |
| `Qwt3D::Axis` | `Qwt3DAxis` |
| `Qwt3D::ColorLegend` | `Qwt3DColorLegend` |
| `Qwt3D::Color` | `Qwt3DColor` |
| `Qwt3D::StandardColor` | `Qwt3DStandardColor` |
| `Qwt3D::ColorMapColor` | `Qwt3DColorMapColor` |
| `Qwt3D::Drawable` | `Qwt3DDrawable` |
| `Qwt3D::Label` | `Qwt3DLabel` |
| `Qwt3D::GridMapping` | `Qwt3DGridMapping` |
| `Qwt3D::Scale` / `AutoScaler` / `Enrichment` / `Mapping` | `Qwt3DScale` / `Qwt3DAutoScaler` / `Qwt3DEnrichment` / `Qwt3DMapping` |
| `plot->setPlotStyle(...)` | `surface->setPlotStyle(...)` |
| `plot->setDataColor(...)` | `surface->setDataColor(...)` |
| `plot->loadFromData(...)` | `surface->loadFromData(...); surface->attach(plot)` |
| `plot->updateData()` | `plot->update()` or `item->itemChanged()` |
| `plot->setCoordinateStyle(...)` | use `Qwt3DCoordinateSystem` API directly |
| `SIGNAL()/SLOT()` connects | new-style `connect()` function pointers |

> **Warning — Breaking change**
>
> This is a source-incompatible refactor. Existing 3D user code must migrate call sites from widget-level data/style APIs to the corresponding `Qwt3DSurface` item APIs. See the [changelog](../../CHANGES.md) for the full list.

## 8. Conclusion

The 3D module now uses a Plot+Item architecture with Core-Profile OpenGL and acyclic, tiered dependencies, replacing the monolithic fixed-function widget. It matches the 2D module's design, keeps the rendering path portable, and provides extension points for new 3D plot types, with the color and theme ecosystem shared between 2D and 3D.
