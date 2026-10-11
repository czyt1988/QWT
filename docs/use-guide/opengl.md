# OpenGL Canvas

QWT provides hardware-accelerated rendering through an OpenGL canvas. This page covers CMake configuration and usage of `QwtPlotOpenGLCanvas`.

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

`QwtPlotOpenGLCanvas` is a canvas widget derived from `QOpenGLWidget`. It renders plot content directly on the GPU, providing significantly better performance than the default `QwtPlotCanvas` for large datasets and real-time updates.

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

### Alternative: QwtPlotCanvas with OpenGL Buffer

If you prefer the standard `QwtPlotCanvas` but still want hardware acceleration, use the `OpenGLBuffer` paint mode:

```cpp
auto* canvas = new QwtPlotCanvas(plot);
canvas->setPaintAttribute(QwtPlotCanvas::OpenGLBuffer, true);
plot->setCanvas(canvas);
```

> **Info**
>
> `QwtPlotOpenGLCanvas` renders directly to a `QOpenGLWidget` and generally offers better performance than the `OpenGLBuffer` approach. However, `OpenGLBuffer` integrates more seamlessly into desktop applications with mixed widget layouts.

### When to Use OpenGL Canvas

| Scenario | Recommendation |
|----------|---------------|
| Static or small datasets (<10k points) | Default `QwtPlotCanvas` is sufficient |
| Large datasets (>100k points) | `QwtPlotOpenGLCanvas` for smooth interaction |
| Real-time streaming data | `QwtPlotOpenGLCanvas` for consistent frame rates |
| Mixed widget UI with complex layouts | `QwtPlotCanvas` with `OpenGLBuffer` for better integration |

> **Example — Related examples**
>
> - `examples/realtime` — real-time plotting with high data throughput
> - `examples/oscilloscope` — oscilloscope-style waveform display

Screenshots of real-time plotting and oscilloscope:

![Real-Time Plot](../assets/screenshots/realtime.png)

![Oscilloscope](../assets/screenshots/oscilloscope.png)
