# Qwt 项目 AI Agent 指引

Qwt 7.x（v7.3.0）是基于原版 Qwt 6.2.0 的维护分支。
基于 Qt 的高性能 2D/3D 绘图库，适用于科学计算和工程数据可视化。许可证：LGPL（Qwt License v1.0）。

快速链接：[GitHub](https://github.com/czyt1988/QWT) | [Gitee](https://gitee.com/czyt1988/QWT) | [文档](https://czyt1988.github.io/QWT/zh/)

## 构建命令

```powershell
# 推荐：使用 build.ps1 一键构建（自动探测 Qt、VS、CMake）
.\build.ps1                          # 全量构建 (Release)
.\build.ps1 build                    # 增量编译
.\build.ps1 rebuild                  # 清除 + 重配 + 编译
.\build.ps1 configure -Examples OFF -Playground OFF  # 仅主库

# 手动构建：Visual Studio 生成器（自动处理 MSVC 环境变量）
cmake -S . -B build -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH="D:/Qt/6.7.3/msvc2019_64"
cmake --build build --config Debug

# Ninja（需先初始化 MSVC 环境变量：INCLUDE, LIB, PATH）
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="..."
```

- CMake 3.5+, Qt 5.12+ / Qt 6.x, MSVC 2019+
- 先检查 `build/CMakeCache.txt` 中 Qt 路径是否正确，清理构建只需删 `CMakeCache.txt` 和 `CMakeFiles/`
- CI 使用 GitHub Actions（Linux + Windows, Qt6.8），构建时跳过 examples/playground
- `build.ps1` 参数：`-Examples`、`-Playground`、`-Tests`、`-OpenGL`、`-Plot3D`（均接受 `ON`/`OFF`）

## 架构关键点

### 三库结构（非单库）

项目生成三个 shared library：

| 目标 | CMake target | 输出名 | DLL 导出宏 |
|------|-------------|--------|-----------|
| 公共基础 | `qwt::core` | `qwtcore.dll` / `libqwtcore.so` | `QWTCORE_EXPORT` |
| 2D 绘图 | `qwt::plot` | `qwtplot.dll` / `libqwtplot.so` | `QWT_EXPORT` |
| 3D 绘图 | `qwt::plot3d` | `qwtplot3d.dll` / `libqwtplot3d.so` | `QWT3D_EXPORT` |

依赖关系：`plot` 和 `plot3d` 都依赖 `core`，但彼此互不依赖。

```
    ┌──────────────────────────┐
    │        qwt::core         │  ← 基础工具库（颜色、数学、数据类型、几何、变换、时间等）
    └──────────────────────────┘
           ↗              ↖
  ┌──────────────┐  ┌───────────────┐
  │   qwt::plot   │  │  qwt::plot3d  │
  │     (2D)      │  │     (3D)      │
  └──────────────┘  └───────────────┘
```

- **core**（`src/core/`）：完整基础工具库，21 个模块，详见下方 "Core Module Structure"
- **plot**（`src/plot/`）：2D 绘图全套功能，通过 `target_link_libraries(plot PUBLIC qwt::core)` 链接 core
- **plot3d**（`src/plot3d/`）：3D 绘图模块，通过 `QWT_CONFIG_QWTPLOT_3D` 控制，同样链接 core，采用 Plot+Item 架构（`Qwt3DPlot` + `Qwt3DPlotItem`），包含 `Qwt3DTheme` 主题系统和 `Qwt3DColorMapColor` 适配器

各模块 Qt 依赖：
- core：`Core Gui`(public)
- 2D：`Core Gui Widgets`(public) + `Concurrent PrintSupport`(private)，可选 `OpenGL OpenGLWidgets Svg`
- 3D：`Qt OpenGL OpenGLWidgets`(public)，内置 `gl2ps` 回退（GLU 仅用于 gl2ps 矢量导出）

### Core Module Structure

`src/core/` 包含 28 个模块，按功能分类如下：

| 类别 | 文件 | 说明 |
|------|------|------|
| **全局** | `qwtcore_global.h` | 模块导出宏 `QWTCORE_EXPORT` |
| **颜色工具** | `qwt_colormap.h/.cpp` | `QwtColorMap` 及子类（线性、HSV、饱和度等） |
| | `qwt_colormap_preset.h/.cpp` | `QwtColorMapPreset`：22 种科学 colormap 预设 |
| | `qwt_color_cycle.h/.cpp` | `QwtColorCycle`：颜色循环系统 |
| **数学工具** | `qwt_math.h/.cpp` | 数学常量、`qwtMinF`/`qwtMaxF` 等工具函数 |
| | `qwt_simd_argminmax.h/.cpp` | SIMD 加速的 argmin/argmax（SSE2/AVX2/NEON） |
| **数据类型** | `qwt_interval.h/.cpp` | `QwtInterval`：区间类型 |
| | `qwt_point_3d.h/.cpp` | `QwtPoint3D`：三维点 |
| | `qwt_point_polar.h/.cpp` | `QwtPointPolar`：极坐标点 |
| | `qwt_samples.h` | `QwtSamples`、`QwtBoxSample` 等样本数据结构 |
| | `qwt_box_statistics.h/.cpp` | `QwtBoxStatistics`：箱线图统计量 |
| **几何算法** | `qwt_bezier.h/.cpp` | `QwtBezier`：贝塞尔曲线（de Casteljau） |
| | `qwt_clipper.h/.cpp` | `QwtClipper`：多边形裁剪算法 |
| **坐标变换** | `qwt_transform.h/.cpp` | `QwtTransform` 及子类（线性、对数等） |
| | `qwt_scale_map.h/.cpp` | `QwtScaleMap`：坐标映射 |
| | `qwt_scale_div.h/.cpp` | `QwtScaleDiv`：刻度划分 |
| | `qwt_scale_engine.h/.cpp` | `QwtScaleEngine` 及子类：刻度引擎（线性、对数、日期） |
| **时间处理** | `qwt_date.h/.cpp` | `QwtDate`：日期时间工具 |
| | `qwt_system_clock.h/.cpp` | `QwtSystemClock`：高精度计时器 |
| **通用算法** | `qwt_algorithm.hpp` | `qwtSelectNextIterator` 等通用算法 |
| | `qwt_qt5qt6_compat.hpp` | Qt5/Qt6 兼容层（`qwt::compat::` 命名空间） |
| **数据容器** | `qwt_grid_data.hpp` | `QwtGridData`：网格数据容器 |
| **数据系列** | `qwt_series_data.h/.cpp` | `QwtSeriesData<T>`：数据系列基类模板 |
| | `qwt_point_data.h/.cpp` | `QwtPointArrayData` 等点数据实现 |
| | `qwt_series_store.h` | `QwtSeriesStore<T>`：数据存储模板 |
| **栅格数据** | `qwt_raster_data.h/.cpp` | `QwtRasterData`：栅格数据基类 |
| | `qwt_matrix_raster_data.h/.cpp` | `QwtMatrixRasterData`：矩阵栅格数据 |
| | `qwt_grid_raster_data.h/.cpp` | `QwtGridRasterData`：网格栅格数据（v7 新增） |

`src/core/CMakeLists.txt` 完整文件列表：

```cmake
set(QWTCORE_HEADER_FILES
    qwtcore_global.h
    qwt_colormap.h           qwt_color_cycle.h        qwt_colormap_preset.h
    qwt_math.h               qwt_interval.h           qwt_point_3d.h
    qwt_point_polar.h        qwt_system_clock.h       qwt_algorithm.hpp
    qwt_qt5qt6_compat.hpp    qwt_samples.h            qwt_bezier.h
    qwt_clipper.h            qwt_date.h               qwt_transform.h
    qwt_scale_map.h          qwt_scale_div.h          qwt_simd_argminmax.h
    qwt_box_statistics.h     qwt_grid_data.hpp
    qwt_scale_engine.h       qwt_series_data.h        qwt_point_data.h
    qwt_series_store.h       qwt_raster_data.h        qwt_matrix_raster_data.h
    qwt_grid_raster_data.h
)

set(QWTCORE_SOURCE_FILES
    qwt_colormap.cpp         qwt_color_cycle.cpp      qwt_colormap_preset.cpp
    qwt_math.cpp             qwt_interval.cpp         qwt_point_3d.cpp
    qwt_point_polar.cpp      qwt_system_clock.cpp     qwt_bezier.cpp
    qwt_clipper.cpp          qwt_date.cpp             qwt_transform.cpp
    qwt_scale_map.cpp        qwt_scale_div.cpp        qwt_simd_argminmax.cpp
    qwt_box_statistics.cpp
    qwt_scale_engine.cpp     qwt_series_data.cpp      qwt_point_data.cpp
    qwt_raster_data.cpp      qwt_matrix_raster_data.cpp
    qwt_grid_raster_data.cpp
)
```

### 条件编译模块

| CMake 选项 | 默认 | 控制内容 |
|-----------|------|---------|
| `QWT_CONFIG_QWTPLOT` | ON | 核心绘图：QwtPlot、曲线、网格、缩放等 |
| `QWT_CONFIG_QWTPOLAR` | ON | 极坐标绘图子模块 |
| `QWT_CONFIG_QWTWIDGETS` | ON | 控件：滑块、旋钮、刻度盘、温度计等 |
| `QWT_CONFIG_QWTSVG` | ON | SVG 导出/渲染 |
| `QWT_CONFIG_QWTOPENGL` | ON | OpenGL 画布 |
| `QWT_CONFIG_QWTPLOT_3D` | ON | 3D 绘图模块 |
| `QWT_CONFIG_BUILD_EXAMPLE` | ON | 构建示例程序 |
| `QWT_CONFIG_BUILD_PLAYGROUND` | ON | 构建实验性代码 |
| `QWT_CONFIG_BUILD_STATIC_EXAMPLE` | ON | 构建静态链接示例 |
| `QWT_CONFIG_BUILD_TESTS` | OFF | 测试构建 |

### PIMPL 模式（自定义宏）

使用 `qwt_global.h` 定义的宏，而非 Qt 的 `Q_DECLARE_PRIVATE`：

```cpp
// 头文件：声明 PIMPL
class QWT_EXPORT QwtFoo : public QWidget {
    QWT_DECLARE_PRIVATE(QwtFoo)  // → class PrivateData; unique_ptr<PrivateData> m_data;
};

// 源文件：实现 PrivateData
class QwtFoo::PrivateData {
    QWT_DECLARE_PUBLIC(QwtFoo)  // → QwtFoo* q_ptr;
};

// 构造函数初始化
QwtFoo::QwtFoo() : QWT_PIMPL_CONSTRUCT {}  // → m_data(qwt_make_unique<PrivateData>(this))

// 访问私有时使用
QWT_D(d);     // PrivateData* d = d_func()
QWT_DC(d);    // const PrivateData* d = d_func()
```

**关键原则**：`m_data` 直接存储 PrivateData（不使用堆指针重定向）。非 const 方法通过 `QWT_D()` 访问，const 方法通过 `QWT_DC()` 访问。

### Qt5/Qt6 兼容层

`qwt_qt5qt6_compat.hpp` 提供 `qwt::compat::` 命名空间下的兼容函数。涉及鼠标/滚轮/字体度量差异时，**必须使用**：
- `qwt::compat::eventPos(event)` → `event->pos()`(Qt5) / `event->position().toPoint()`(Qt6)
- `qwt::compat::wheelEventDelta(event)` → `event->delta()`(Qt5) / `event->angleDelta().y()`(Qt6)
- `qwt::compat::horizontalAdvance(fm, str)` → `fm.width(str)`(Qt5.12-) / `fm.horizontalAdvance(str)`(Qt5.12+)

## 编码规范（必须遵守）

### 格式化

`.clang-format` 基于 WebKit 风格：
- 缩进 4 空格，列宽 120
- 花括号：类/函数/枚举/命名空间后换行，控制语句不换行
- 指针左对齐（`int* p`），模板 `template<` 后无空格
- `SortIncludes: false` —— include 顺序手动控制

### 命名

- 类名：`Qwt` 开头大驼峰，如 `QwtPlotCurve`
- 方法：小驼峰，getter 不加 `get` 前缀，setter 加 `set`
- 成员变量：`m_` 前缀，如 `m_data`
- 局部变量：小驼峰
- 常量：`MAX_WIDTH`

### Include 顺序（手动排序，不可乱）

```cpp
#include "own_header.h"   // 本文件对应头文件优先
// 空行
#include <qnamespace.h>   // Qt 系统头文件 <q...>
#include <QWidget>        // Qt 类头文件 <Q...>
// 空行
#include <algorithm>      // STL
// 空行
#include "qwt_plot.h"     // 本项目其他头文件
```

### 注释规范（Doxygen 纯英文）

所有源码注释**一律使用英文**，禁止中文。Doxygen 关键字统一用 `@` 前缀（`@brief`、`@param`、`@return`、`@details`）。

| 注释类型 | 位置 | 要求 |
|---------|------|------|
| 类注释 | `.h` | 英文 Doxygen，含使用示例 |
| public 函数详细 | `.cpp` | 英文 Doxygen（`@brief`/`@param`/`@return`/`@details`） |
| public 函数简要 | `.h` | 单行英文 `// Comment`（无 Doxygen 块） |
| 信号注释 | `.h` | 英文 Doxygen（信号无 cpp 定义） |
| private/protected 函数 | `.cpp` | 可选，建议英文 |
| 行内注释 | 任意 | 英文 |

参数方向：`@param[out]`、`@param[in,out]` 仅在非 const 引用/指针时必须标注；value 和 const ref 省略。

### 现代 C++ 要求

- 使用 `override`/`final`（不用旧宏 `QWT_OVERRIDE`/`QWT_FINAL`）
- 使用 `nullptr`（不用 `NULL`）
- 使用 `static_cast<>` / `using`（不用 C 风格转换 / `typedef`）
- Qt 容器迭代使用 `qwt_as_const(container)` 防止 COW 深拷贝
- 智能指针使用 `qwt_make_unique<T>(args...)`（C++11 兼容）

### 信号槽

- 使用 `Q_SIGNALS:` / `public Q_SLOTS:` 宏（不用 `signals:` / `public slots:`）
- 优先新式 `connect(sender, &Sender::signal, receiver, &Receiver::slot)` 语法

## 重要命名变更（v7.0.7+）

旧名称已在代码库中移除，**不要使用旧名**：

| 已废弃的旧名 | 当前使用的新名 |
|-------------|--------------|
| `QwtPlotZoomer` | `QwtPlotAxisZoomer`（仅绑定 2 个轴） |
| `QwtPanner` | `QwtCachePanner`（带像素缓存） |
| `QwtPlotPanner` (旧) | `QwtPlotCachePanner` |
| - | `QwtPlotCanvasZoomer`（新增：整体画布缩放，不限轴数） |
| - | `QwtPlotPanner`（新增：实时拖动，基于 QwtPicker） |
| `QwtColorMap::rgb(const QwtInterval&, double)` | `QwtColorMap::rgb(double vMin, double vMax, double value)`（v7.3.1+，已移至 core 模块） |

### 3D 模块命名变更（v7.3.3+）

3D 模块已完全重构，去除了 `namespace Qwt3D`，所有类使用 `Qwt3D` 前缀：

| 已废弃的旧名 | 当前使用的新名 | 说明 |
|-------------|--------------|------|
| `namespace Qwt3D { ... }` | （无命名空间） | 所有类在全局作用域 |
| `Qwt3D::Plot3D` | `Qwt3DPlot` | 纯渲染窗口，不再持有数据 |
| `Qwt3D::SurfacePlot` | `Qwt3DSurface` | 从 widget 变为 item |
| `Qwt3D::Function` | `Qwt3DFunction` | target 改为 `Qwt3DSurface*` |
| `Qwt3D::ParametricSurface` | `Qwt3DParametricSurface` | 同上 |
| `Qwt3D::CoordinateSystem` | `Qwt3DCoordinateSystem` | |
| `Qwt3D::Axis` | `Qwt3DAxis` | |
| `Qwt3D::ColorLegend` | `Qwt3DColorLegend` | |
| `Qwt3D::Color` | `Qwt3DColor` | |
| `Qwt3D::StandardColor` | `Qwt3DStandardColor` | |
| `Qwt3D::ColorMapColor` | `Qwt3DColorMapColor` | |
| `Qwt3D::Qwt3DTheme` | `Qwt3DTheme` | |
| `Qwt3D::GraphPlot` | （已删除） | stub 移除 |
| `Qwt3D::Mapping` / `GridMapping` | `Qwt3DMapping` / `Qwt3DGridMapping` | |
| `Qwt3D::Scale` / `AutoScaler` | `Qwt3DScale` / `Qwt3DAutoScaler` | |
| `Qwt3D::Enrichment` | `Qwt3DEnrichment` | |
| `Qwt3D::Drawable` / `Label` | `Qwt3DDrawable` / `Qwt3DLabel` | |
| `using namespace Qwt3D;` | （删除） | 无命名空间 |
| `qwt3d_surfaceplot.h` | `qwt3d_surface.h` | 头文件改名 |
| `plot->updateData()` | `plot->update()` / `item->itemChanged()` | API 变更 |
| `plot->setPlotStyle()` | `surface->setPlotStyle()` | 方法从 Plot 移到 Surface item |
| `plot->setDataColor()` | `surface->setDataColor()` | 同上 |
| `plot->loadFromData()` | `surface->loadFromData()` | 同上 |

## 关键架构概念

### 坐标轴系统

使用 `QwtAxisId`（包含 Position + 序号），枚举 `QwtAxis::Position`：
- `QwtAxis::XBottom`, `QwtAxis::XTop`, `QwtAxis::YLeft`, `QwtAxis::YRight`

兼容旧版 `Axis` 枚举：`QwtPlot::yLeft` 等仍可用（通过 `QWT_AXIS_COMPAT` 宏）。

### 寄生绘图 (Parasite Plot)

通过 `QwtPlot::createParasitePlot(QwtAxis::Position)` 创建寄生绘图，共享宿主画布区域，支持任意多轴。寄生绘图和宿主通过 `plotList()` / `hostPlot()` / `parasitePlots()` 管理。

### QwtFigure

类似 matplotlib Figure 的多绘图布局容器，支持网格排列。通过 `QwtFigureWidgetOverlay` 提供交互操作（拖动、缩放子绘图）。

### 3D 模块架构（Plot + Item）

v7.3.3+ 起，3D 模块从旧的 widget-per-plot 模式重构为与 2D 对称的 Plot + Item 架构：

- **`Qwt3DPlot`**（`QOpenGLWidget` 子类）：纯渲染窗口，管理 GL 上下文、视图变换、光照、坐标系统、item 列表。**不持有绘图数据**。
- **`Qwt3DPlotItem`**：所有 3D 绘图 item 的抽象基类（非 widget），通过 `attach()` / `detach()` 挂载到 `Qwt3DPlot`。
- **`Qwt3DSurface`**：曲面 item，使用 VBO/VAO + GLSL 3.3 Core 着色器渲染。

```cpp
Qwt3DPlot* plot = new Qwt3DPlot(parent);
Qwt3DSurface* surface = new Qwt3DSurface();
surface->loadFromData(data, cols, rows, minX, maxX, minY, maxY);
surface->attach(plot);
```

**关键原则**：不再有 `namespace Qwt3D`，所有类用 `Qwt3D` 前缀。禁止 legacy OpenGL API（`glBegin/glEnd`、display list、`glRotatef` 等），全部使用 VBO/VAO + GLSL shader。

### 3D 主题系统

`Qwt3DTheme` 封装 3D 绘图的全部视觉属性（背景色、网格色/线宽、数据 colormap、坐标轴颜色、标题样式、光照预设、着色模式、绘图样式、材质参数）。10 种内置预设：`Default`、`Dark`、`Scientific`、`Warm`、`Cool`、`Matplotlib`、`EarthTones`、`Ocean`、`HighContrast`、`Presentation`。

```cpp
// 使用预设
plot->applyTheme(Qwt3DTheme::Dark);
plot->applyTheme("Scientific");

// 手动定制
Qwt3DTheme theme(Qwt3DTheme::Scientific);
theme.setDataColorPreset("plasma");  // 使用 core 模块的 colormap 预设
theme.setShininess(20.0);
theme.apply(plot);  // plot 是 Qwt3DPlot* 指针
```

`Qwt3DColorMapColor` 适配器桥接 core 模块的 `QwtColorMap` 到 3D 的 `Qwt3DColor` 接口，使得 22 种科学 colormap 预设（viridis, plasma, jet, hot 等）可直接用于 3D 表面图。

光照预设（`Qwt3DTheme::LightingPreset`）：`NoLighting`、`FlatLight`、`Studio`、`Outdoor`、`Soft`。

## 不可触碰的文件

| 文件 | 原因 |
|------|------|
| `src-amalgamate/QwtPlot.h` | 自动合成，不要手动编辑 |
| `src-amalgamate/QwtPlot.cpp` | 自动合成，不要手动编辑 |
| `tools/` | 合成脚本，非核心库代码 |

## 测试

- 位于 `tests/`，使用 Qt Test 框架
- 构建时需 `-DQWT_CONFIG_BUILD_TESTS=ON`
- 当前活跃测试：`picker_click_test`, `picker_group_click_test`
- `splineprof/` 和 `splinetest/` 为性能/功能测试，暂未接入 CMake

## 文档

- MkDocs Material 主题 + i18n 插件（中文/英文）
- Doxygen API 文档：`docs/doxygen-doc-file/Doxyfile_en.cfg` / `Doxyfile_cn.cfg`
- CI 自动部署 GitHub Pages（仅 master 分支 push 触发）
- 构建文档依赖：`pip install -r requirements-docs.txt`
- 在线文档：https://czyt1988.github.io/QWT/zh/
