# OpenGL Canvas

QWT provides hardware-accelerated rendering through an OpenGL canvas. `QwtPlotOpenGLCanvas` is the canvas class, and using it requires the CMake configuration below.

## CMake Configuration

If you use OpenGL features, add the OpenGL module to your CMake configuration:

```cmake
find_package(Qt${QT_VERSION_MAJOR} ${QWT_MIN_QT_VERSION} COMPONENTS
    OpenGL
    REQUIRED
)
target_link_libraries(${QWT_APP_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::OpenGL
)
if(${QT_VERSION_MAJOR} EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} ${QWT_MIN_QT_VERSION} COMPONENTS
        OpenGLWidgets
        REQUIRED
    )
    target_link_libraries(${QWT_APP_NAME} PRIVATE
        Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    )
endif()
```

## QwtPlotOpenGLCanvas

`QwtPlotOpenGLCanvas` is a canvas widget derived from `QOpenGLWidget`. It does not build a native shader/VBO pipeline for curves: all plot items are still painted by `QPainter` (Qt's OpenGL2 paint engine) into an offscreen FBO, then reach the screen through MSAA resolve, blit and texture compositing. Polyline tessellation happens on the CPU while the GPU only does the filling. Its advantage is concentrated in the item-painting segment of heavy multi-curve, large-canvas scenes, at the cost of a fixed per-frame compositing overhead; for single-curve small datasets or incremental-repaint scenarios the default `QwtPlotCanvas` is the steadier choice. For quantitative evidence see the technical report [Raster vs. OpenGL Rendering Pipelines](../../tech-report/raster-vs-opengl.md).

### Basic Usage

Replace the default canvas with the OpenGL canvas:

```cpp
#include <qwt_plot.h>
#include <qwt_plot_opengl_canvas.h>
#include <qwt_plot_curve.h>

auto* plot = new QwtPlot("OpenGL Plot");

// Create and set OpenGL canvas
auto* canvas = new QwtPlotOpenGLCanvas(plot);
canvas->setFrameStyle(QFrame::Box | QFrame::Plain);
canvas->setLineWidth(1);
plot->setCanvas(canvas);

// Add curves as usual
auto* curve = new QwtPlotCurve("Data");
// ... set data ...
curve->attach(plot);
```

### Custom Surface Format

You can pass a custom `QSurfaceFormat` for advanced OpenGL configuration:

```cpp
QSurfaceFormat format;
format.setSamples(4);  // Enable 4x MSAA anti-aliasing
format.setDepthBufferSize(24);

auto* canvas = new QwtPlotOpenGLCanvas(format, plot);
plot->setCanvas(canvas);
```

### Canvas Properties

`QwtPlotOpenGLCanvas` imitates the `QFrame` API even though it is not derived from `QFrame`:

| Property | Type | Description |
|----------|------|-------------|
| `frameShadow` | `QFrame::Shadow` | Frame shadow style (`Plain`, `Raised`, `Sunken`) |
| `frameShape` | `QFrame::Shape` | Frame shape (`NoFrame`, `Box`, `Panel`, etc.) |
| `lineWidth` | `int` | Width of the frame line in pixels |
| `midLineWidth` | `int` | Width of the middle line for `Panel` shape |
| `borderRadius` | `double` | Border corner radius for rounded borders |

### QwtPlotCanvas Has No OpenGL Buffer Attribute

`QwtPlotCanvas` itself offers no OpenGL paint mode: its `PaintAttribute` enum only contains `BackingStore`, `Opaque`, `HackStyledBackground` and `ImmediatePaint`, so the only entry point to hardware acceleration is switching to `QwtPlotOpenGLCanvas` wholesale. What you can tune on the raster canvas are the caching and refresh attributes:

```cpp
auto* canvas = new QwtPlotCanvas(plot);
canvas->setPaintAttribute(QwtPlotCanvas::BackingStore, false);   // disable the pixmap cache — actually faster for full-frame replots
canvas->setPaintAttribute(QwtPlotCanvas::ImmediatePaint, true);  // replot() goes through the synchronous repaint()
plot->setCanvas(canvas);
```

> **Note**
>
> `BackingStore` serves overlays and `QwtPlotDirectPainter` incremental repaints; in full-frame replot benchmarks, disabling it saves one pixmap allocation and blit. Note that `QOpenGLWidget` switches its entire top-level window onto the OpenGL compositing path, so watch the compositing cost when many regular widgets are mixed in. For quantitative evidence see the [technical report](../../tech-report/raster-vs-opengl.md).

### When to Use OpenGL Canvas

| Scenario | Recommendation |
|----------|---------------|
| Static or small datasets (<10k points) | Default `QwtPlotCanvas` is sufficient |
| Single-curve large datasets (e.g. 100k–5M points) | Default `QwtPlotCanvas` is steadier — on integrated GPUs measurements show GL is actually ~1.1–1.7x slower |
| Heavy multi-curve scenes (≥10 curves and ≥1M total samples) or when antialiasing is needed | `QwtPlotOpenGLCanvas` — wins in the item-painting segment, and AA is nearly free |
| Real-time streaming data (incremental repaints) | Default `QwtPlotCanvas` + `QwtPlotDirectPainter` — the DirectPainter path on the GL canvas shows bimodal, unstable frame times and cannot be relied on |
| Mixed widget UI with complex layouts | Default `QwtPlotCanvas`, avoiding the whole top-level window switching to the OpenGL compositing path |

For selection criteria and measured data see the technical report [Raster vs. OpenGL Rendering Pipelines: End-to-End Frame-Time Breakdown and Selection Criteria](../../tech-report/raster-vs-opengl.md).

> **Example — Related examples**
>
> - `examples/2D/realtime` for real-time plotting with high data throughput
> - `examples/2D/oscilloscope` for oscilloscope-style waveform display

Screenshots of real-time plotting and oscilloscope:

![Real-Time Plot](../../assets/screenshots/realtime.png)

![Oscilloscope](../../assets/screenshots/oscilloscope.png)
