---
name: qwt
description: >-
  在本仓库（QWT 7.4.2，Qwt 6.2.0 维护分支）中开发或使用 QWT 绘图库时的导航技能。当任务涉及
  src/core、src/plot、src/plot3d、examples、playground、tests 下的代码，或提到 QwtPlot、
  QwtPlotCurve、QwtPlotItem、Qwt3DPlot、极坐标、colormap、刻度引擎、曲线平滑/降采样、
  控件（QwtDial/QwtKnob/QwtSlider/QwtWheel/QwtThermo）、QwtFigure、QwtPyPlot 时使用。
  它是路由技能：告诉你项目有哪些功能板块、每个板块去读哪个源码文件、哪个示例、哪篇文档。
  触发词：QWT、Qwt、qwt、QwtPlot、QwtPlotItem、Qwt3D、2D 绘图、3D 绘图、极坐标绘图。
version: 1.0.0
---

# QWT 项目导航（Router）

QWT 7.4.2，基于 Qt 的 2D/3D 绘图库，v7 起拆成**三个独立共享库**：

| 库 | 源码 | 输出 | 说明 |
|----|------|------|------|
| `qwt::core` | `src/core/` | `qwtcore.dll` / `libqwtcore.so` | 纯基础库：颜色、数学、数据容器、几何、坐标变换、刻度系统。**不依赖 Widgets 之外的重物，也不依赖 2D/3D** |
| `qwt::plot` | `src/plot/` | `qwtplot.dll` / `libqwtplot.so` | 2D 绘图全套：QwtPlot + 各种 PlotItem、交互工具、控件、极坐标、Figure/PyPlot。**依赖 core** |
| `qwt::plot3d` | `src/plot3d/` | `qwtplot3d.dll` / `libqwtplot3d.so` | 3D 绘图（Qwt3DPlot + item）。**依赖 core，不依赖 2D** |

宏控制可选模块（见根 `CMakeLists.txt` 与 `src/plot/CMakeLists.txt`）：`QWT_CONFIG_QWTPLOT`、
`QWT_CONFIG_QWTPOLAR`、`QWT_CONFIG_QWTWIDGETS`、`QWT_CONFIG_QWTSVG`、`QWT_CONFIG_QWTOPENGL`。

## 任务分诊表

先判断任务落在哪个板块，再读对应的详细文件；拿不准时先读 `gotchas.md`（高频陷阱）。

| 任务类型 | 读这个文件 |
|---------|-----------|
| 颜色/colormap、数学工具、区间、数据系列(QwtSeriesData)、坐标变换、刻度引擎/刻度划分、样条曲线 | `core.md` |
| QwtPlot 画曲线/柱状/直方图/散点/频谱图/交易曲线/矢量场、画布、缩放/拖拽/放大、拾取、图例、布局 | `plot-2d.md` |
| QwtDial、QwtKnob、QwtCompass、QwtSlider、QwtWheel、QwtThermo、QwtCounter、模拟时钟 | `widgets.md` |
| QwtFigure 多子图布局、QwtPyPlot（matplotlib 风格 API）、数据点拾取联动 | `figure-pyplot.md` |
| 极坐标绘图 QwtPolarPlot | `polar.md` |
| 3D 曲面/线/柱状图、主题、光照、gl2ps 导出 | `plot3d.md` |
| 信号槽用不了、改了样式不刷新、找不到旧类名、编译报缺头文件 | `gotchas.md` |

## 三条通用规则（动手前必读）

1. **根目录 `AGENTS.md` 是项目宪法**：三库结构、PIMPL 宏（`QWT_DECLARE_PRIVATE` / `QWT_D()`）、
   编码规范（4 空格、120 列、clang-format WebKit 风格、include 顺序手排）、注释一律英文 Doxygen。
   本 skill 只做"去哪读"的导航，规范本身去读 `AGENTS.md`。
2. **3D 模块有自己的局部宪法**：`src/plot3d/AGENTS.md`。3D 任务必须先读它（分层依赖规则、
   禁止 legacy OpenGL、通知机制都在里面）。
3. **示例就是最好的 API 文档**：`examples/2D/`（35 个）、`examples/3D/`（4 个）几乎覆盖每个功能点，
   同类任务的正确写法优先从示例抄结构，而不是凭空发明 API。

## 代码在哪里（速查）

```
src/
  core/          21+ 个模块，无 Widgets/Widgets 之外的 Qt 依赖，见 core.md
  plot/          2D 全库（含控件、极坐标、Figure/PyPlot），见 plot-2d.md
  plot3d/        3D 全库，见 plot3d.md
examples/
  2D/           按功能分类的示例（曲线、动画、实时、频谱图、极坐标、PyPlot、样式表…）
  3D/           bar3D、line3D、qwtplot3d、figureSurface3D
  bench/        renderbench（渲染性能基准）
playground/     实验性代码（vectorfield、plotmatrix、curvetracker…），不是稳定 API 范本
tests/          Qt Test 单元测试，构建需 -DQWT_CONFIG_BUILD_TESTS=ON
docs/
  use-guide/    英文使用指南（按功能一篇，如 curve.md / spectrogram.md / zoomer.md）
  zh/use-guide/ 中文使用指南（与英文对应）
  dev-guide/    开发规范（coding-standards / pimpl-pattern / comment-standards）
```

## 构建与自检

```powershell
.\build.ps1 build                 # 增量编译（自动探测 Qt/VS/CMake）
.\build.ps1 configure -Examples OFF -Playground OFF   # 只构建主库，最快验证编译
```

验证"我的改动能用"的最小回路：改完代码 → `.\build.ps1 build` → 运行 `examples/2D/` 下最接近的示例。

## 文档 → 源码 的对应关系

使用指南 `docs/use-guide/*.md`（中文在 `docs/zh/use-guide/`）**按功能一篇**，
每篇含"特性 → 用法 → 核心方法摘要"，多数在文末列出对应示例目录。
按功能名查目录即可：曲线 `curve.md`、平滑 `curve-smoothing.md`、降采样 `curve-downsampling.md`、
数据系列 `series-data.md`、NaN 处理 `nan-handling.md`、缩放 `zoomer.md`、平移 `panner.md`、
拾取 `pick-value.md` / `picker-state-machine.md`、图例 `legend.md` 等。
开发规范（PIMPL、注释、编码标准）在 `docs/dev-guide/`。
