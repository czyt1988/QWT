<div align="center">
<img src="./docs/assets/brand/qwt-hero-zh.png" alt="Qwt 7 — 基于 Qt 的高性能 2D/3D 绘图库" width="100%">
<p><a href="./README.MD">English</a> &nbsp;&middot;&nbsp; <b>简体中文</b></p>
<p><a href="https://github.com/czyt1988/QWT/releases/latest"><img src="https://img.shields.io/github/v/release/czyt1988/QWT?style=flat-square&amp;label=release&amp;color=2563eb" alt="最新版本"></a> <a href="https://github.com/czyt1988/QWT/actions/workflows/page.yml"><img src="https://img.shields.io/github/actions/workflow/status/czyt1988/QWT/page.yml?style=flat-square&amp;label=docs" alt="文档构建"></a> <a href="./COPYING"><img src="https://img.shields.io/badge/license-Qwt%201.0%20%2F%20LGPL-3b82f6?style=flat-square" alt="Qwt License 1.0（LGPL）"></a> <a href="https://github.com/czyt1988/QWT/stargazers"><img src="https://img.shields.io/github/stars/czyt1988/QWT?style=flat-square&amp;label=stars&amp;color=2563eb" alt="GitHub 星标"></a></p>
<p><a href="https://doc.qt.io/"><img src="https://img.shields.io/badge/Qt-5.12%2B%20%7C%206-41CD52?style=flat-square&amp;logo=qt&amp;logoColor=white" alt="Qt 5.12+ / Qt 6"></a> <a href="https://en.cppreference.com/w/cpp/11"><img src="https://img.shields.io/badge/C%2B%2B-11%20%2F%2017-00599C?style=flat-square&amp;logo=cplusplus&amp;logoColor=white" alt="C++11 / C++17"></a> <a href="https://czyt1988.github.io/QWT/zh/"><img src="https://img.shields.io/badge/docs-%E4%B8%AD%E6%96%87-2563eb?style=flat-square" alt="中文文档"></a> <a href="https://czyt1988.github.io/QWT/"><img src="https://img.shields.io/badge/docs-English-2563eb?style=flat-square" alt="English documentation"></a></p>
<h3>基于 Qt 的现代化、开源协议友好的 2D/3D 绘图库</h3>
<p><sub>Qwt 7 在 Qwt 6.2.0 最终官方版本的基础上持续维护：CMake、Qt 5/6、现代 C++、集成 3D 绘图，以及更易用的科学可视化工具。</sub></p>
<p><a href="https://czyt1988.github.io/QWT/zh/"><b>中文文档</b></a> &nbsp;&middot;&nbsp; <a href="https://czyt1988.github.io/QWT/zh/use-guide/import-qwt/">引入 Qwt</a> &nbsp;&middot;&nbsp; <a href="#快速开始">快速开始</a> &nbsp;&middot;&nbsp; <a href="#qwt-7-的新特性">新特性</a> &nbsp;&middot;&nbsp; <a href="#绘图展示">绘图展示</a> &nbsp;&middot;&nbsp; <a href="./CHANGES-CN.md">更新日志</a></p>
</div>

## 为什么是 Qwt 7？

Qt 生态里能画图的库不多，主流的为 **QCustomPlot**、**Qwt**、**Qt Charts** 和 **KDChart**，它们的主要差异在协议、渲染架构和成熟度。

Qt 6.8 之后，原来的 Qt Charts（2D）与 Qt DataVisualization（3D）被合并为统一的 **Qt Graphs** 模块（注意不是 Qt Graphics）。它底层完全基于 Qt Quick Scene Graph（QSG）+ Qt Quick 3D，彻底放弃了老旧的 Graphics-View / QPainter 管线，并且必须通过 `QQuickWidget` 或 `QQuickWindow` 嵌入，这意味着 QML runtime 成为硬性依赖，纯 C++ 使用体验较差（[Qt 论坛上的吐槽](https://forum.qt.io/topic/159224/qt-graphs-building-2d-plot-using-c-only)）。它也不再支持 Windows 7 等老系统，对嵌入式设备不友好。因此未来几年，`QCustomPlot`、`Qwt`、`Qt Charts` 和 `KDChart` 仍会是实际可选方案。

| 库 | 协议 | 渲染 | 优势 | 短板 |
|:--|:--|:--|:--|:--|
| **QCustomPlot** | GPL | QPainter | 单文件 `qcustomplot.h/.cpp` 即可引入，使用最广，交互开箱即用 | GPL 有传染性，商业项目难以使用 |
| **Qwt** | LGPL | QPainter | 架构符合软件工程原则，大数据量渲染性能优秀，细节控制能力强 | 官方停止维护，默认样式老旧，集成门槛较高 |
| **Qt Charts / Qt Graphs** | GPLv3 | QSG + Qt Quick 3D | Qt 官方模块 | 科学计算场景性能低，强制 QML runtime，没有 LGPL 选项 |
| **KDChart 3.0** | MIT | QPainter | 商业友好，支持甘特图 | 渲染风格老旧 |

对商业项目而言，现实的选择只剩 **Qwt** 和 **KDChart 3.0**。我个人更偏好 Qwt：它的架构更符合软件工程原则，大规模渲染性能更优。QCustomPlot 的优势是交互功能开箱即用（如鼠标缩放、坐标轴缩放），而 Qwt 实现同样的功能需要更多代码，但提供了更精细的控制能力。因此在我的项目需要绘图时，我选择 Qwt，并根据需要添加功能、改进和优化它——这个项目就是这么来的。

在 Qwt 官方停止维护后，我接手了最终官方版本（**Qwt 6.2.0**）并在此基础上继续维护。**Qwt 7** 保留了 Qwt 原本扎实的工程设计，同时补齐了现代 Qt 项目需要的东西：CMake、Qt 6、现代 C++、集成的 3D 模块、布局与交互辅助类、科学 colormap，以及 matplotlib 风格接口，协议依然是商业友好的 LGPL 系。

- **GitHub** — <https://github.com/czyt1988/QWT>
- **Gitee（镜像）** — <https://gitee.com/czyt1988/QWT>
- **文档** — <https://czyt1988.github.io/QWT/zh/> · [English](https://czyt1988.github.io/QWT/)
- **更新日志** — [CHANGES-CN.md](./CHANGES-CN.md) · [CHANGES.md](./CHANGES.md)

![Qwt 7 的实际效果：上方为实时 2D 频谱，下方为 3D 瀑布图](./docs/assets/screenshots-3D/waterfall3D.png)

## 文档

完整文档基于 [MkDocs Material](https://squidfunk.github.io/mkdocs-material/) 构建，每次推送到 `master` 分支都会自动部署到 **GitHub Pages**：

- **中文文档** — <https://czyt1988.github.io/QWT/zh/>
- **English documentation** — <https://czyt1988.github.io/QWT/>

| 主题 | 中文 | English |
|:-----|:-----|:--------|
| Qwt 7 新特性 | [Qwt 7 新特性](https://czyt1988.github.io/QWT/zh/use-guide/qwt7-new-features/) | [Qwt 7 new features](https://czyt1988.github.io/QWT/use-guide/qwt7-new-features/) |
| 引入与集成 Qwt | [引入 Qwt](https://czyt1988.github.io/QWT/zh/use-guide/import-qwt/) | [Import Qwt](https://czyt1988.github.io/QWT/use-guide/import-qwt/) |
| 第一个绘图 | [第一个绘图](https://czyt1988.github.io/QWT/zh/use-guide/first-plot/) | [First plot](https://czyt1988.github.io/QWT/use-guide/first-plot/) |
| 从源码构建 | [构建说明](https://czyt1988.github.io/QWT/zh/build-guide/build-instructions/) | [Build instructions](https://czyt1988.github.io/QWT/build-guide/build-instructions/) |
| 2D 图表项 | [曲线与图表项](https://czyt1988.github.io/QWT/zh/use-guide/2d/curve/) | [Curve & chart items](https://czyt1988.github.io/QWT/use-guide/2d/curve/) |
| Figure 布局 | [Figure 绘图容器](https://czyt1988.github.io/QWT/zh/use-guide/2d/figure-widget/) | [Figure widget](https://czyt1988.github.io/QWT/use-guide/2d/figure-widget/) |
| 多坐标轴 | [多坐标轴](https://czyt1988.github.io/QWT/zh/use-guide/2d/parasite-axes/) | [Parasite axes](https://czyt1988.github.io/QWT/use-guide/2d/parasite-axes/) |
| 平移、缩放与数据拾取 | [平移](https://czyt1988.github.io/QWT/zh/use-guide/2d/panner/) · [缩放](https://czyt1988.github.io/QWT/zh/use-guide/2d/zoomer/) · [数据拾取](https://czyt1988.github.io/QWT/zh/use-guide/2d/pick-value/) | [Panner](https://czyt1988.github.io/QWT/use-guide/2d/panner/) · [Zoomer](https://czyt1988.github.io/QWT/use-guide/2d/zoomer/) · [Data picker](https://czyt1988.github.io/QWT/use-guide/2d/pick-value/) |
| 性能与降采样 | [渲染性能](https://czyt1988.github.io/QWT/zh/use-guide/2d/rendering-performance/) | [Rendering performance](https://czyt1988.github.io/QWT/use-guide/2d/rendering-performance/) |
| Matplotlib 风格接口 | [PyPlot 接口](https://czyt1988.github.io/QWT/zh/use-guide/2d/pyplot-api/) | [PyPlot API](https://czyt1988.github.io/QWT/use-guide/2d/pyplot-api/) |
| 3D 绘图 | [3D 绘图简介](https://czyt1988.github.io/QWT/zh/use-guide/3d/3d-plot/) | [3D plot guide](https://czyt1988.github.io/QWT/use-guide/3d/3d-plot/) |
| Colormap 预设 | [Colormap 预设](https://czyt1988.github.io/QWT/zh/use-guide/general/colormap/) | [Colormap presets](https://czyt1988.github.io/QWT/use-guide/general/colormap/) |
| 仪表盘与滑块 | [仪表盘与滑块](https://czyt1988.github.io/QWT/zh/use-guide/general/widgets-controls/) | [Widgets & controls](https://czyt1988.github.io/QWT/use-guide/general/widgets-controls/) |
| 技术报告（内部原理） | [技术报告](https://czyt1988.github.io/QWT/zh/tech-report/) | [Tech reports](https://czyt1988.github.io/QWT/tech-report/) |
| 开发指引 | [开发指引](https://czyt1988.github.io/QWT/zh/dev-guide/) | [Developer guide](https://czyt1988.github.io/QWT/dev-guide/) |
| API 参考 | [Doxygen（中文）](https://czyt1988.github.io/QWT/doxygen_cn/index.html) | [Doxygen (English)](https://czyt1988.github.io/QWT/doxygen_en/index.html) |
| 更新日志 | [CHANGES-CN.md](./CHANGES-CN.md) | [CHANGES.md](./CHANGES.md) |

文档源码位于 [`docs/`](./docs) 目录：英文在根目录，中文在 [`docs/zh/`](./docs/zh)；API 参考由 Doxygen 从源码注释生成。

<details>
<summary>本地构建文档</summary>

```bash
pip install -r requirements-docs.txt
mkdocs serve
# 中文:    http://127.0.0.1:8000/QWT/zh/
# English: http://127.0.0.1:8000/QWT/
```

</details>

## 快速开始

### 环境要求

| 项目 | 最低要求 |
|:-----|:---------|
| 构建系统 | CMake 3.10+（仍完整支持 `qmake` 集成） |
| Qt | Qt 5.12+ 或 Qt 6.x |
| C++ | C++11；使用 Qt 6 时为 C++17 |
| 编译器 | MSVC 2019+、GCC 或 Clang |
| 平台 | Windows 与 Linux（CI 覆盖）；macOS 使用同一套 Qt 工具链即可构建 |

### CMake（推荐）

Qwt 7 会安装 CMake package，引入时不再需要手写 include 路径和预定义宏：

```cmake
find_package(qwt REQUIRED)

# 引入 2D 绘图（自动链接 qwt::core）
target_link_libraries(${YOUR_APP_TARGET} PRIVATE qwt::plot)

# 引入 3D 绘图（自动链接 qwt::core）
target_link_libraries(${YOUR_APP_TARGET} PRIVATE qwt::plot3d)

# 仅引入基础工具库：数学函数、数据类型、几何算法、坐标变换等
target_link_libraries(${YOUR_APP_TARGET} PRIVATE qwt::core)
```

三个库的依赖关系刻意保持简单：`qwt::plot` 与 `qwt::plot3d` 都依赖 `qwt::core`，彼此之间互不依赖。

### 单一头文件和源文件

参考 `QCustomPlot` 的做法，整个库（2D 和 3D）也被合并为 `QwtPlot.h` 与 `QwtPlot.cpp`。把这两个文件加入工程即可使用——参见 [`src-amalgamate/`](./src-amalgamate) 和 [`staticExample`](./examples/staticExample)：

```cmake
# QwtPlot 依赖 Core Gui Widgets Svg Concurrent OpenGL PrintSupport
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

# gl2ps（3D 矢量导出）需要 GLU
find_package(OpenGL REQUIRED)
target_link_libraries(YOUR_APP_TARGET PRIVATE OpenGL::GLU)

# MSVC：合并后的单一翻译单元会超过默认 COFF 段数量上限（fatal error C1128）
if(MSVC)
    target_compile_options(YOUR_APP_TARGET PRIVATE /bigobj)
endif()
```

MSVC 下合并后的 `QwtPlot.cpp` 需要 `/bigobj`：单一翻译单元超过默认的 65536 个段（error C1128）。完整目标配置见 [`examples/staticExample`](./examples/staticExample)。

### 第一个绘图

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

    // 1. 创建绘图窗口
    QwtPlot plot;
    plot.setWindowTitle("My First Plot");
    plot.setAxisTitle(QwtAxis::XBottom, "Time [s]");
    plot.setAxisTitle(QwtAxis::YLeft, "Amplitude");
    plot.setAxisScale(QwtAxis::YLeft, -1.5, 1.5);

    // 2. 添加网格
    auto* grid = new QwtPlotGrid();
    grid->attach(&plot);

    // 3. 为曲线准备数据
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

    // 4. 显示
    plot.resize(720, 480);
    plot.show();
    return app.exec();
}
```

逐步讲解见：[第一个绘图](https://czyt1988.github.io/QWT/zh/use-guide/first-plot/)。

### qmake

Qwt 7 不再以 `qmake` 作为主构建系统，但仍然完整支持 `qmake` 集成。可参考 [`qmakeExample`](./examples/qmakeExample) 示例，或阅读 [qmake 单文件构建](./docs/zh/build-guide/qmake-single-file.md)。

### 从源码构建

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.0/gcc_64
cmake --build build --config Release
cmake --install build --prefix /path/to/install
```

Windows 下还提供了自动探测 Qt、Visual Studio 和 CMake 的辅助脚本：

```powershell
.\build.ps1                 # 全量 Release 构建
.\build.ps1 build           # 增量编译
.\build.ps1 rebuild         # 清除 + 重配 + 编译
.\build.ps1 configure -Examples OFF -Playground OFF   # 仅构建主库
```

全部构建选项见：[构建说明](https://czyt1988.github.io/QWT/zh/build-guide/build-instructions/)。

## 核心特性

| 方面 | 你可以获得 |
|:-----|:-----------|
| **2D 绘图** | 20+ 种绘图项：曲线、散点、柱状、箱线、直方图、光谱图、K 线、向量场、区间曲线、极坐标、形状、文本标签、符号等 |
| **3D 绘图** | 曲面、柱状图、线图、瀑布图、函数/参数曲面，基于 VBO/VAO + GLSL 现代管线，Plot + Item 架构与 2D 对称 |
| **性能** | SIMD 加速的曲线降采样（SSE2 / AVX2 / NEON）、OpenGL 画布，以及百万级数据的基准测试 |
| **布局** | `QwtFigure` 提供 matplotlib 风格的 `subplot()` 网格、可拖拽的保留式网格轨道、绘图区自动对齐，寄生轴支持任意多坐标轴 |
| **交互** | 实时画布平移、与坐标轴数量无关的画布缩放、坐标轴拖动/滚轮缩放、覆盖所有系列类型的数据拾取 |
| **外观** | 现代化的扁平风格、10 种内置 3D 主题 + 5 种光照预设、22 种科学 colormap 预设、扁平风格控件 |
| **架构** | 三个清晰的库（`qwt::core`、`qwt::plot`、`qwt::plot3d`）、统一 PIMPL 模式、C++11/17、不使用 legacy OpenGL |
| **集成** | CMake package target、单头文件合成版本、完整 `qmake` 支持、兼容 Qt 5.12+ 与 Qt 6 |

## Qwt 7 的新特性

### CMake 支持

Qwt 7 以 **CMake** 作为主构建系统，不再维护 `.pro` 工程文件，同时仍完整支持 `qmake` 集成。安装后会提供 `qwt::core`、`qwt::plot`、`qwt::plot3d` 三个 CMake target，两行即可引入，具体见[快速开始](#cmake推荐)。

### 单一头文件和源文件

参考 `QCustomPlot` 的做法，整个库被合并为 `QwtPlot.h` 和 `QwtPlot.cpp`，把这两个文件引入项目即可使用，2D 与 3D 绘图均已包含。

具体参阅：[QWT 库的引入](https://czyt1988.github.io/QWT/zh/use-guide/import-qwt/)，示例见 [`staticExample`](./examples/staticExample)。

![staticExample](./docs/assets/screenshots/staticExample.png)

### 通过 qmake 使用 Qwt 7

`qmake` 集成仍然完整支持——可参考 [`qmakeExample`](./examples/qmakeExample) 示例与 [qmake 单文件构建](./docs/zh/build-guide/qmake-single-file.md)，或查看快速开始中的 [qmake 一节](#qmake)。

### 美化了视觉风格

原有 Qwt 样式使用很老旧的浮雕风，和现代审美不符。为此我做了针对性优化：去除默认的凹陷风格，坐标轴紧贴绘图区，整体视觉更符合现代设计。

原版 Qwt 样式：

![qwt6.3 效果](./docs/assets/picture/bode-old.png)

Qwt 7：

![qwt7 效果](./docs/assets/screenshots/bode.png)

### 增加了 Figure 绘图容器

类似 Python 的 matplotlib，Qwt 提供了 `QwtFigure` 绘图容器，可以很方便地进行多个绘图的布局，支持 matplotlib 风格的 `subplot()` 网格。

同时提供了 `QwtFigureWidgetOverlay`，封装了移动子图、调整子图尺寸等交互操作。

7.6 新增**保留式网格布局（retained grid layout）**：`QwtFigureGrid` 将行/列轨道比例作为数据保存，`QwtFigureGridSplitter` 让分隔线可拖拽（`gridSizesChangeFinished()` 信号便于实现撤销），`QwtFigure::setGridAxisAlignment()` 还会自动对齐同一列/行的绘图区，即使各子图的刻度标签、图例或寄生轴宽度不同，坐标轴与数据区也能对齐。

![QwtFigure 保留式网格布局与可拖拽分隔线](./docs/assets/screenshots/figure-grid.png)

具体可参见教程：[Figure 绘图容器](https://czyt1988.github.io/QWT/zh/use-guide/2d/figure-widget/)，以及 [`figureGrid`](./examples/2D/figureGrid) 示例。

### 支持多坐标轴

增加了类似 matplotlib 的寄生轴功能，可以在同一块绘图区中显示任意多个独立坐标轴。

具体可参见教程：[多坐标轴的创建](https://czyt1988.github.io/QWT/zh/use-guide/2d/parasite-axes/)

### 增加了坐标轴交互

坐标轴交互功能已内置，支持直接在坐标轴上使用鼠标**拖动**和**滚轮缩放**。

**拖动：**

![qwt-scale-builtin-action-pan](./docs/assets/screenshots/qwt-scale-builtin-action-pan.gif)

**缩放：**

![qwt-scale-builtin-action-zoom](./docs/assets/screenshots/qwt-scale-builtin-action-zoom.gif)

具体可参见教程：[坐标轴交互动作](https://czyt1988.github.io/QWT/zh/use-guide/2d/scale-builtin-action/)

### 增加了数据拾取功能

新增 `QwtPlotSeriesDataPicker` 类，支持数据拾取，覆盖 K 线、区间曲线、直方图、矢量场、柱状/箱线图、光谱曲线等所有系列类型。

![series-data-picker-yvalue](./docs/assets/picture/series-data-picker-yvalue.gif)

![series-data-picker-nearest-value](./docs/assets/picture/series-data-picker-nearest-value.gif)

具体可参见教程：[绘图数据拾取](https://czyt1988.github.io/QWT/zh/use-guide/2d/pick-value/)

### 增强了画布平移

重构 `QwtPlotPanner` 实现实时画布平移：拖动时移动的是实时绘图内容，而不是缓存位图。原来的缓存式实现以 `QwtPlotCachePanner` 保留，新的平移器同时支持任意多坐标轴。

![qwt-realtime-panner](./docs/assets/screenshots/qwt-realtime-panner.gif)

具体可参见教程：[平移工具](https://czyt1988.github.io/QWT/zh/use-guide/2d/panner/)

### 新增了画布缩放器

原 `QwtPlotZoomer` 需要指定两个坐标轴来进行缩放操作；如果绘图需要四个坐标轴都生效，就得绑定两个缩放器，使用麻烦且对多坐标轴不友好。

新增的 `QwtPlotCanvasZoomer` 无需指定坐标轴，会对整个画布进行整体缩放，并且能支持任意多坐标轴。

具体可参见教程：[缩放工具](https://czyt1988.github.io/QWT/zh/use-guide/2d/zoomer/)

### 新增箱线图（Box Chart）支持

新增 `QwtPlotBoxChart` 类，支持绘制箱线图（Box-and-Whisker Plot），用于展示数据的统计分布特征：

- 支持预计算数据和原始数据两种输入方式
- 自动计算统计量（中位数、四分位数、异常值）
- 多种箱体样式：矩形、菱形、缺口形
- 支持垂直和水平两种显示方向
- 异常值自动检测和自定义符号

具体可参见教程：[箱线图使用指南](https://czyt1988.github.io/QWT/zh/use-guide/2d/boxchart/)

### 坐标刻度可朝内显示

新增 `QwtPlot::setAxisTickDirection()` 接口，支持将坐标轴刻度从画布边缘向画布内部绘制（ticks inside），而轴线与刻度标签仍保留在外部，适用于紧凑布局或特殊视觉风格：

- 每个坐标轴可独立设置为 `TickInside`（朝内）或 `TickOutside`（朝外，默认）
- 主刻度、中刻度、次刻度均同步朝内绘制，并继承朝外刻度的长度、颜色、线宽等样式
- 兼容对数刻度（log）与倒置刻度（inverted），各类刻度引擎下朝内方向均能正确渲染
- 可与寄生轴配合使用，每个寄生绘图独立设置刻度方向

```cpp
// 将 YLeft 与 XBottom 轴刻度设为朝内
plot->setAxisTickDirection(QwtAxis::YLeft, QwtPlot::TickInside);
plot->setAxisTickDirection(QwtAxis::XBottom, QwtPlot::TickInside);
plot->replot();
```

![ticks_inside](./docs/assets/screenshots/ticks_inside.png)

具体可参见教程：[刻度朝内显示使用指南](https://czyt1988.github.io/QWT/zh/use-guide/2d/ticks-inside/)，示例代码位于 [`examples/2D/ticks_inside`](./examples/2D/ticks_inside)。

### Matplotlib 风格绘图接口（`QwtPyPlot`）

7.3.1 新增的 `QwtPyPlot` 是一套受 Python `matplotlib.pyplot` 启发的高层有状态接口。它封装了 `QwtFigure`（或单个 `QwtPlot`），底层委托给现有的 Qwt 设施，既拥有 matplotlib 般的简洁写法，又不损失 Qwt 的强大能力与精细控制。

熟悉的单行调用开箱即用 —— `plot()`、`scatter()`、`bar()`、`hist()`、`imshow()`、`contour()`、`quiver()`、`fillBetween()`、`errorbar()`、`candlestick()`，配合 `subplot()`、`twinx()`/`twiny()`、`savefig()`，以及 matplotlib 风格的格式串（如 `"r-o"` 表示红色实线 + 圆形标记）：

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

具体可参见教程：[QwtPyPlot — Matplotlib 风格接口](https://czyt1988.github.io/QWT/zh/use-guide/2d/pyplot-api/)

### 3D 绘图

3D 模块（`qwt::plot3d`）采用与 2D 模块对称的 **Plot + Item** 架构，渲染完全基于现代 OpenGL（VBO/VAO + GLSL 3.3 Core 着色器；固定管线接口仅用于 gl2ps 矢量导出的回放，正常绘制不会使用）：

- `Qwt3DPlot`（`QOpenGLWidget` 子类）是渲染窗口，负责 GL 上下文、视图变换、光照、坐标系统与鼠标交互，本身不持有数据。
- `Qwt3DPlotItem` 是所有可绘制 item 的基类，item 自持数据与几何，通过 `attach()` 挂载到 `Qwt3DPlot`。一个 `Qwt3DPlot` 可以同时挂载任意多个 item，在同一个 GL 上下文中一起渲染。

内置 item：

| Item | 用途 |
|------|------|
| `Qwt3DSurface` | 规则网格高度场曲面，支持 colormap 着色与叠加网格线 |
| `Qwt3DBar` | 3D 柱状图 / 直方图，支持一维序列与二维网格两种数据形态 |
| `Qwt3DLine` | 3D 折线 / 轨迹，支持 `Lines`、`Dots` 与带光照的 `Tube` 样式 |
| `Qwt3DWaterfall` | 滑动窗口瀑布图，用 `pushTrace()` 推入一维谱线，item 自行维护窗口 |

此外，`Qwt3DFunction` 与 `Qwt3DParametricSurface` 用于由函数或参数方程生成曲面数据，生成结果通过 `Qwt3DSurface::loadFromData()` 载入。

最小曲面示例：

```cpp
Qwt3DPlot* plot = new Qwt3DPlot(parent);

auto* surface = new Qwt3DSurface();
surface->loadFromData(z, cols, rows, minX, maxX, minY, maxY);
surface->attach(plot);
plot->showColorLegend(true);
```

实时瀑布图无需在应用层维护窗口与深度，item 内部完成：

```cpp
auto* waterfall = new Qwt3DWaterfall();
waterfall->attach(plot);
waterfall->setTraceCapacity(64);                       // 保留最近 64 条谱线
waterfall->setColorMode(Qwt3DWaterfall::ByAmplitude);  // 按幅度着色
waterfall->setColorRange(0.0, 1.0);                    // 固定色标范围，避免闪烁

// 每个定时器周期推入一帧
waterfall->pushTrace(frame);                           // QVector<double>
```

交互与导出均为内置能力：左键拖动旋转、滚轮缩放、`Ctrl` + 左键拖动平移，`setMouseSpeed()` 调整拖动灵敏度，`setRollMouse()` 可指定滚转（roll）手势；位图通过 `savePixmap()` 导出，PDF/SVG 矢量图通过 gl2ps 导出。3D 绘图本身也是 widget，可以直接放进 `QwtFigure` 与 2D 绘图组合布局，[`examples/3D/figureSurface3D`](./examples/3D/figureSurface3D) 演示了 3D 曲面与 2D 截面曲线、时间序列曲线的联动。

具体可参见教程：[3D 绘图简介](https://czyt1988.github.io/QWT/zh/use-guide/3d/3d-plot/)，以及各 item 的专页：[3D 柱状图](https://czyt1988.github.io/QWT/zh/use-guide/3d/3d-bar-chart/)、[3D 线图](https://czyt1988.github.io/QWT/zh/use-guide/3d/3d-line-plot/)、[3D 瀑布图](https://czyt1988.github.io/QWT/zh/use-guide/3d/3d-waterfall/)。

### 3D 主题系统

7.3.1 新增的 `Qwt3DTheme` 主题系统将全部视觉属性 —— 背景、网格、colormap、坐标轴、标题、光照、着色模式、材质 —— 打包为一个描述符，一行调用即可套用整套风格：

```cpp
// 创建渲染窗口 + 曲面 item
Qwt3DPlot* plot = new Qwt3DPlot();
Qwt3DSurface* surface = new Qwt3DSurface();
surface->attach(plot);

// 一行套用预设
plot->applyTheme(Qwt3DTheme::Dark);

// 或在预设基础上定制
Qwt3DTheme theme(Qwt3DTheme::Scientific);
theme.setDataColorPreset("plasma");
theme.setLightingPreset(Qwt3DTheme::Studio);
theme.setShininess(20.0);
theme.apply(plot);

// 也可以只应用到单个 item
theme.applyToItem(surface);
```

- 10 种内置主题：`Default`、`Dark`、`Scientific`、`Warm`、`Cool`、`Matplotlib`、`EarthTones`、`Ocean`、`HighContrast`、`Presentation`
- 5 种光照预设：`NoLighting`、`FlatLight`、`Studio`、`Outdoor`、`Soft`
- `Qwt3DColorMapColor` 适配器将 core 模块的 colormap 桥接到 3D 表面，因此 22 种科学预设同样可用于 3D

### 科学 Colormap 预设

`qwt::core` 内置 `QwtColorMapPreset` —— 22 种知名科学 colormap（与 matplotlib / MATLAB 同名），2D 光谱图与 3D 表面图共用：

`viridis`、`plasma`、`inferno`、`magma`、`cividis`、`jet`、`hot`、`cool`、`spring`、`summer`、`autumn`、`winter`、`gray`、`bone`、`copper`、`rainbow`、`hsv`、`turbo`、`coolwarm`、`rdylbu`、`rdylgn`、`spectral`

```cpp
auto cmap = QwtColorMapPreset::create("viridis");
// cmap->rgb(vMin, vMax, value) 返回给定数据值对应的颜色
```

其中感知均匀的 colormap（`viridis`、`plasma`、`inferno`、`magma`、`cividis`）对色盲友好且打印安全，非常适合科研论文。

具体可参见教程：[科学 Colormap 预设](https://czyt1988.github.io/QWT/zh/use-guide/general/colormap/)

### SIMD 加速的曲线降采样

百万级数据点的流畅渲染现已成为一等公民。`QwtPlotCurve` 提供六种降采样算法，其中 MinMax 桶路径采用 SIMD 加速（SSE2 / AVX2 / NEON，运行时自动检测）：

- `FilterPointsAdaptive` —— 自适应列归约，单遍 O(n)，在像素列内保留坡面形状（优先级最高，推荐）
- `FilterPointsLTTB` —— 真正的 Largest-Triangle-Three-Buckets，保形且性能优秀（默认值）
- `FilterPointsMinMaxLTTB` —— MinMax 桶算法，v7.3.2 至 v7.4.2 的默认行为
- `FilterPointsPixel` —— 按像素列取 min/max 包络，适合纯噪声信号（旧算法，保留兼容）
- `FilterPointsAggressive` —— 针对单调大数据集的快速剔除（旧算法，保留兼容）
- `FilterPoints` —— 原始剔除算法

效果：百万点曲线可交互帧率渲染。可用 `renderbench` 示例自行基准测试。

具体可参见教程：[渲染性能与曲线平滑](https://czyt1988.github.io/QWT/zh/use-guide/2d/rendering-performance/)

### 独立的 Core 基础工具库

`qwt::core`（`qwtcore.dll` / `libqwtcore.so`）是一套完整的基础工具库，包含 28 个模块，可独立使用，不引入任何绘图代码：

- 数学与 SIMD：`qwt_math`、`qwt_simd_argminmax`
- 数据类型：`QwtInterval`、`QwtPoint3D`、`QwtPointPolar`、`QwtSamples`、`QwtBoxStatistics`
- 几何算法：`QwtBezier`、`QwtClipper`
- 坐标变换：`QwtTransform`、`QwtScaleMap`、`QwtScaleDiv`、`QwtScaleEngine`
- 日期/时间：`QwtDate`、`QwtSystemClock`
- Colormap 与颜色循环：`QwtColorMap`、`QwtColorMapPreset`、`QwtColorCycle`
- 数据系列与栅格：`QwtSeriesData`、`QwtPointData`、`QwtSeriesStore`、`QwtRasterData`、`QwtMatrixRasterData`、`QwtGridRasterData`

按需链接即可：

```cmake
# 仅引入基础工具库，不引入任何绘图代码
target_link_libraries(${YOUR_APP_TARGET} PUBLIC qwt::core)
```

### 其它修改

- 修复了 `NAN` 值和 `INF` 值对绘图的影响
- 新增 `QwtPlotCurve::BreakAtNan` 属性：将 NaN/Inf 样本渲染为断点而不是跨越连线，行为与 matplotlib 一致
- `QwtPlotSeriesDataPicker` 现已支持所有系列图元类型的拾取 —— K 线、区间曲线、直方图、矢量场、柱状/箱线图、光谱曲线等
- 全代码库现代化 C++：`override`/`final`、`nullptr`、移动语义、`constexpr`、`noexcept`、范围 for，以及统一的 PIMPL 宏模式（180+ 文件）
- `renderbench` 示例用于测量曲线渲染性能并生成报告

## 绘图展示

### 基本图表

||||
|:---:|:---:|:---:|
|![Figure Widget](./docs/assets/picture/figure-scale-aligment.png)|![Simple Plot](./docs/assets/screenshots/simpleplot.png)|![Bar Chart - Grouped](./docs/assets/screenshots/BarChart-grouped.png)|
|`examples/2D/figure`|`examples/2D/simpleplot`|`examples/2D/barchart`|

||||
|:---:|:---:|:---:|
|![Bar Chart - Stacked](./docs/assets/screenshots/BarChart-stacked.png)|![Scatter Plot](./docs/assets/screenshots/scatterplot.png)|![Curve Demo](./docs/assets/screenshots/curvedemo.png)|
|`examples/2D/barchart`|`examples/2D/scatterplot`|`examples/2D/curvedemo`|

### 实时可视化

||||
|:---:|:---:|:---:|
|![CPU Monitor](./docs/assets/screenshots/cpuplot.png)|![Real-Time Plot](./docs/assets/screenshots/realtime.png)|![Oscilloscope](./docs/assets/screenshots/oscilloscope.png)|
|`examples/2D/cpuplot`|`examples/2D/realtime`|`examples/2D/oscilloscope`|

### 高级图表

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

### 3D 可视化

||||
|:---:|:---:|:---:|
|![Comprehensive 3D Demo](./docs/assets/screenshots-3D/simpleplot3D.png)|![3D Bar Chart](./docs/assets/screenshots-3D/bar3D.png)|![3D Line Plot](./docs/assets/screenshots-3D/line3D.png)|
|`examples/3D/qwtplot3d`|`examples/3D/bar3D`|`examples/3D/line3D`|

||||
|:---:|:---:|:---:|
|![3D Waterfall](./docs/assets/screenshots-3D/waterfall3D.png)|![3D Waterfall - Wireframe](./docs/assets/screenshots-3D/waterfall3D-2.png)|![3D Surface in a Figure](./docs/assets/screenshots-3D/figureSurface3D.png)|
|`examples/3D/waterfall3D`|`examples/3D/waterfall3D --style 2`|`examples/3D/figureSurface3D`|

### 动态演示

||||
|:---:|:---:|:---:|
|![Animated Demo](./docs/assets/screenshots/animated.png)|![Curve Tracker](./docs/assets/screenshots/curvetracker.png)|![Refresh Test](./docs/assets/screenshots/refreshtest.png)|
|`examples/2D/animation`|`playground/curvetracker`|`examples/2D/refreshtest`|

### 样式与符号

||||
|:---:|:---:|:---:|
|![Legends Demo](./docs/assets/screenshots/legends.png)|![Symbols Demo](./docs/assets/screenshots/symbols.png)|![Shape Rendering](./docs/assets/screenshots/shapes.png)|
|`examples/2D/legends`|`playground/symbols`|`playground/shapes`|

### 控件

||||
|:---:|:---:|:---:|
|![Dial Controls](./docs/assets/screenshots/controls-dial.png)|![Knob Controls](./docs/assets/screenshots/controls-knob.png)|![Slider Controls](./docs/assets/screenshots/controls-sliders.png)|
|`examples/2D/controls`|`examples/2D/controls`|`examples/2D/controls`|

||||
|:---:|:---:|:---:|
|![Wheel & Thermo](./docs/assets/screenshots/controls-wheel-thermo.png)|![Radio Buttons](./docs/assets/screenshots/radio.png)|![Time Scale Demo](./docs/assets/screenshots/timescale.png)|
|`examples/2D/controls`|`examples/2D/radio`|`playground/timescale`|

### 仪表盘

||||
|:---:|:---:|:---:|
|![Compass Dial](./docs/assets/screenshots/dials-compass.png)|![Cockpit Dials](./docs/assets/screenshots/dials-cockpit.png)||
|`examples/2D/dials`|`examples/2D/dials`||

## 路线图与进度

- [x] CMake 支持
- [x] 支持 Qt 6
- [x] C++11 现代化
- [x] 合并为单一头文件/源文件，简化引入
- [x] 提供集成的交互辅助，简化使用
- [x] 现代化的视觉风格
- [x] `QwtFigure` 布局管理类
- [x] 保留式、可拖拽的网格布局与绘图区自动对齐（7.6）
- [x] 寄生轴支持，实现任意多坐标轴
- [x] 坐标轴交互（拖动与滚轮缩放）
- [x] `QwtPlotSeriesDataPicker` 数据拾取
- [x] 支持多坐标轴的实时画布平移（`QwtPlotPanner`）
- [x] 不绑定坐标轴的画布缩放器（`QwtPlotCanvasZoomer`）
- [x] 集成 3D 绘图
- [x] 3D 绘图项：曲面、柱状图、线图、滑动窗口瀑布图、函数曲面
- [x] 箱线图（`QwtPlotBoxChart`）
- [x] 6 种曲线降采样算法 + SIMD 加速
- [x] 颜色循环系统（`QwtColorCycle`）
- [x] 箭头标记（`QwtPlotArrowMarker`）
- [x] 刻度朝内支持（`TickInside` / `TickOutside`）
- [x] Flat 风格控件（滑块、旋钮、刻度盘等）
- [x] 模块化架构：core（基础工具库）+ plot（2D 绘图）+ plot3d（3D 绘图）
- [x] Matplotlib 风格绘图接口（`QwtPyPlot`）
- [x] 3D 主题系统（10 种预设 + 5 种光照预设）
- [x] 22 种科学 colormap 预设（viridis、plasma、jet、turbo 等）
- [x] 独立的 `qwt::core` 基础工具库（28 个模块，无绘图依赖）

## 参与贡献

- **Bug 反馈与功能建议** —— 请在 [GitHub Issues](https://github.com/czyt1988/QWT/issues) 提交，并尽量附上 Qt 版本、编译器信息以及最小复现代码。
- **Pull Request** —— 请遵循[编码规范](https://czyt1988.github.io/QWT/zh/dev-guide/coding-standards/)（`.clang-format`、英文 Doxygen 注释、现代 C++），并保持提交原子性、使用 Conventional Commits 提交信息。
- **测试** —— 位于 [`tests/`](./tests)，通过 `-DQWT_CONFIG_BUILD_TESTS=ON` 启用。
- **文档** —— 指南位于 [`docs/`](./docs)（英文）与 [`docs/zh/`](./docs/zh)（中文）；API 参考页由源码注释生成。

Gitee 仓库为面向中国大陆用户的镜像，开发在 GitHub 上进行。

## 更新日志

详细日志请参阅 [CHANGES-CN.md](./CHANGES-CN.md)，英文版本见 [CHANGES.md](./CHANGES.md)。版本号遵循 [语义化版本（SemVer）](https://semver.org/lang/zh-CN/)。

## 许可证

Qwt 7 采用 **Qwt License, Version 1.0** —— 即 GNU Lesser General Public License（LGPL）附加若干例外条款，使子类化与静态链接对商业项目更友好。完整协议见 [COPYING](./COPYING)，源码文件保留如下版权声明：

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
