# 使用 OpenGL

QWT 通过 OpenGL 画布提供硬件加速渲染。本页介绍 CMake 配置以及 `QwtPlotOpenGLCanvas` 的使用方法。

## CMake 配置

如果你使用 OpenGL，需要在 CMake 中添加 OpenGL 模块的引入：

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

`QwtPlotOpenGLCanvas` 是继承自 `QOpenGLWidget` 的画布控件。它并没有为曲线建立原生的 shader/VBO 管线——所有 plot item 仍由 `QPainter`（Qt 的 OpenGL2 paint engine）绘制到离屏 FBO，再经 MSAA resolve、blit 与纹理合成上屏，折线细分发生在 CPU 侧、GPU 负责填充。它的收益集中在重载多曲线、大画布场景的 item 绘制段，同时每帧要付出固定的合成开销；单曲线小数据量或增量重绘场景下，默认的 `QwtPlotCanvas` 反而更稳。定量依据见技术报告[《Raster 与 OpenGL 渲染管线》](../../tech-report/raster-vs-opengl.md)。

### 基本用法

将默认画布替换为 OpenGL 画布：

```cpp
#include <qwt_plot.h>
#include <qwt_plot_opengl_canvas.h>
#include <qwt_plot_curve.h>

auto* plot = new QwtPlot("OpenGL Plot");

// 创建并设置 OpenGL 画布
auto* canvas = new QwtPlotOpenGLCanvas(plot);
canvas->setFrameStyle(QFrame::Box | QFrame::Plain);
canvas->setLineWidth(1);
plot->setCanvas(canvas);

// 照常添加曲线
auto* curve = new QwtPlotCurve("Data");
// ... 设置数据 ...
curve->attach(plot);
```

### 自定义表面格式

可以传入自定义的 `QSurfaceFormat` 进行高级 OpenGL 配置：

```cpp
QSurfaceFormat format;
format.setSamples(4);  // 启用 4 倍 MSAA 抗锯齿
format.setDepthBufferSize(24);

auto* canvas = new QwtPlotOpenGLCanvas(format, plot);
plot->setCanvas(canvas);
```

### 画布属性

`QwtPlotOpenGLCanvas` 虽然不是继承自 `QFrame`，但模仿了 `QFrame` 的 API：

| 属性 | 类型 | 说明 |
|------|------|------|
| `frameShadow` | `QFrame::Shadow` | 边框阴影样式（`Plain`、`Raised`、`Sunken`） |
| `frameShape` | `QFrame::Shape` | 边框形状（`NoFrame`、`Box`、`Panel` 等） |
| `lineWidth` | `int` | 边框线宽（像素） |
| `midLineWidth` | `int` | `Panel` 形状时中间线的宽度 |
| `borderRadius` | `double` | 圆角边框的圆角半径 |

### QwtPlotCanvas 没有 OpenGL 缓冲属性

`QwtPlotCanvas` 本身不提供 OpenGL 绘制模式（它的 `PaintAttribute` 只有 `BackingStore`、`Opaque`、`HackStyledBackground`、`ImmediatePaint` 四个枚举），硬件加速的唯一入口就是整体换用 `QwtPlotOpenGLCanvas`。raster 画布上可以调节的是缓存与刷新属性：

```cpp
auto* canvas = new QwtPlotCanvas(plot);
canvas->setPaintAttribute(QwtPlotCanvas::BackingStore, false);   // 关掉 pixmap 缓存，整帧 replot 场景下反而更快
canvas->setPaintAttribute(QwtPlotCanvas::ImmediatePaint, true);  // replot() 走同步 repaint()
plot->setCanvas(canvas);
```

> **说明**
>
> `BackingStore` 为覆盖层与 `QwtPlotDirectPainter` 增量重绘场景服务；整帧 replot 的基准测试中关掉它反而省一次 pixmap 分配与 blit。另外 `QOpenGLWidget` 会让所在顶层窗口整体切换为 OpenGL 合成路径，与普通控件大量混排时需留意合成代价的变化。定量依据见[技术报告](../../tech-report/raster-vs-opengl.md)。

### 何时使用 OpenGL 画布

| 场景 | 推荐方案 |
|------|---------|
| 静态或小数据集（<1万点） | 默认 `QwtPlotCanvas` 即可 |
| 单曲线大数据集（如 10万–500万点） | 默认 `QwtPlotCanvas` 更稳——核显实测 GL 反而慢约 1.1–1.7 倍 |
| 重载多曲线（曲线数 ≥10 且总采样数 ≥1M）或需要抗锯齿 | `QwtPlotOpenGLCanvas`，item 绘制段占优、AA 近乎免费 |
| 实时流式数据（增量重绘） | 默认 `QwtPlotCanvas` + `QwtPlotDirectPainter`——GL 画布上的 DirectPainter 路径帧时间双峰不稳定，不可依赖 |
| 复杂布局的混合控件 UI | 默认 `QwtPlotCanvas`，避免顶层窗口整体切换为 OpenGL 合成路径 |

选型判据与实测数据见技术报告[《Raster 与 OpenGL 渲染管线：端到端帧时间分解与选型判据》](../../tech-report/raster-vs-opengl.md)。

> **示例 — 相关示例**
>
> - `examples/2D/realtime` — 高吞吐量的实时绘图
> - `examples/2D/oscilloscope` — 示波器风格的波形显示

实时绘图与示波器的例子截图如下：

![Real-Time Plot](../../../assets/screenshots/realtime.png)

![Oscilloscope](../../../assets/screenshots/oscilloscope.png)
