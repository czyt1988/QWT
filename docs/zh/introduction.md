---
title: QWT 简介
description: QWT 是一个基于 Qt 的高性能绘图库，支持 Qt6，采用 LGPL 协议，适用于科学计算和工程应用中的数据可视化。
keywords: [QWT, Qt绘图库, 科学图表, 数据可视化, Qt6, LGPL协议, C++绘图]
---

# QWT — Qt 绘图库

QWT（Qt Widgets for Technical Applications）是一个基于 Qt 的高性能 2D/3D 绘图库，适用于科学计算和工程应用中的数据可视化。

## 为什么选择 QWT？

Qt 生态里能画图的库不多，主流的有 `QCustomPlot`、`Qwt`、`Qt Charts` 和 `KDChart`。Qt 6.8 之后把原来的 `Qt Charts`（2D）与 `Qt DataVisualization`（3D）合并为统一的 Qt Graphs 模块，底层全部基于 Qt Quick Scene Graph + Qt Quick 3D，放弃了老旧的 Graphics-View/QPainter 管线。不过 Qt Graphs 必须通过 QQuickWidget 或 QQuickWindow 嵌入，必须带 QML runtime，C++ 支持不足，且不支持 Win7 等老系统，对嵌入式也不友好。

| 库 | 协议 | 优势 | 劣势 |
|---|---|---|---|
| **QCustomPlot** | GPL | 简单易用，开箱即用 | GPL 传染性，商业不友好 |
| **Qwt** | LGPL | 性能优越，架构合理 | 原作者停止更新，部署较难 |
| **Qt Charts** | GPL | Qt 官方 | 效率低，GPL 协议 |
| **KDChart** | MIT (3.0+) | 商业友好 | 渲染效果一般 |

**本项目**基于 Qwt 6.2.0 进行维护和改进，添加了现代化功能和修复，使其成为协议友好、性能优越、方便使用的 Qt 绘图库。

## Qwt 7.0 新特性

- [x] **CMake 支持** — `find_package(qwt)` 一键引入
- [x] **支持 Qt6** — 兼容 Qt 5.12+ 和 Qt 6.x
- [x] **单一文件引入** — 类似 QCustomPlot，只需 `QwtPlot.h` + `QwtPlot.cpp`
- [x] **美化控件风格** — 去除老旧浮雕风格，现代化 UI
- [x] **Figure 绘图容器** — 类似 matplotlib 的多绘图布局
- [x] **多坐标轴** — 寄生轴架构，支持任意多个坐标轴
- [x] **坐标轴交互** — 鼠标拖动、滚轮缩放
- [x] **2D/3D 一体化** — 内置 3D 绘图模块
- [x] **C++11 优化** — 全面使用 `override`、`nullptr`、智能指针、范围 for 循环等现代 C++ 特性
- [x] **超大规模数据渲染优化** — 4 种曲线降采样算法 + SIMD 加速，百万级数据流畅渲染
- [x] **颜色循环系统** — 可自定义的颜色循环，自动为系列数据分配颜色
- [x] **箱线图** — `QwtPlotBoxChart` 支持统计分布可视化
- [x] **Flat 风格控件** — 滑块、旋钮、刻度盘等控件支持扁平化样式

## 快速集成

**CMake（推荐）**


```cmake
find_package(qwt REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE qwt::plot)
# 3D 绘图
target_link_libraries(${PROJECT_NAME} PRIVATE qwt::plot3d)
```

**单文件引入**


```cpp
// 将 src-amalgamate/QwtPlot.h 和 QwtPlot.cpp 加入项目
#include "QwtPlot.h"

auto* plot = new QwtPlot("My Plot");
auto* curve = new QwtPlotCurve("Data");
```

## 项目地址

- **GitHub**: [https://github.com/czyt1988/QWT](https://github.com/czyt1988/QWT)
- **Gitee**: [https://gitee.com/czyt1988/QWT](https://gitee.com/czyt1988/QWT)
- **在线文档**: [https://czyt1988.github.io/QWT/zh/](https://czyt1988.github.io/QWT/zh/)

## 效果展示

### 基本图表

<div class="grid cards" markdown>

- ![Figure Widget](../assets/screenshots/qwt_figure.png)
  `examples/2D/figure`

- ![Simple Plot](../assets/screenshots/simpleplot.png)
  `examples/2D/simpleplot`

- ![Bar Chart](../assets/screenshots/BarChart-grouped.png)
  `examples/2D/barchart`

- ![Scatter Plot](../assets/screenshots/scatterplot.png)
  `examples/2D/scatterplot`

- ![Curve Demo](../assets/screenshots/curvedemo.png)
  `examples/2D/curvedemo`

- ![Box Chart](../assets/screenshots/BoxChart.png)
  `examples/2D/boxchart`

</div>

### 实时可视化

<div class="grid cards" markdown>

- ![CPU Monitor](../assets/screenshots/cpuplot.png)
  `examples/2D/cpuplot`

- ![Real-Time Plot](../assets/screenshots/realtime.png)
  `examples/2D/realtime`

- ![Oscilloscope](../assets/screenshots/oscilloscope.png)
  `examples/2D/oscilloscope`

- ![Radio](../assets/screenshots/radio.png)
  `examples/2D/radio`

- ![Sysinfo](../assets/screenshots/sysinfo.png)
  `examples/2D/sysinfo`

- ![Animated](../assets/screenshots/animated.png)
  `examples/2D/animation`

</div>

### 高级图表

<div class="grid cards" markdown>

- ![Spectrogram](../assets/screenshots/spectrogram.png)
  `examples/2D/spectrogram`

- ![Vector Field](../assets/screenshots/vectorfield.png)
  `playground/vectorfield`

- ![Stock Chart](../assets/screenshots/stockchart.png)
  `examples/2D/stockchart`

- ![Polar Demo](../assets/screenshots/polardemo.png)
  `examples/2D/polardemo`

- ![Parasite Plot](../assets/screenshots/parasite-plot.png)
  `examples/2D/parasitePlot`

- ![Bode Plot](../assets/screenshots/bode.png)
  `examples/2D/bode`

- ![Shapes](../assets/screenshots/shapes.png)
  `playground/shapes`

- ![Scale Engine](../assets/screenshots/scaleengine.png)
  `playground/scaleengine`

- ![PyPlot](../assets/screenshots/pyplot.png)
  `examples/2D/pyplot`

</div>

### 其他示例

<div class="grid cards" markdown>

- ![Distrowatch](../assets/screenshots/distrowatch.png)
  `examples/2D/distrowatch`

- ![Friedberg Bar](../assets/screenshots/friedberg-bar.png)
  `examples/2D/friedberg`

- ![Friedberg Tube](../assets/screenshots/friedberg-tube.png)
  `examples/2D/friedberg`

- ![Item Editor](../assets/screenshots/itemeditor.png)
  `examples/2D/itemeditor`

- ![Rasterview 1](../assets/screenshots/rasterview-1.png)
  `examples/2D/rasterview`

- ![Rasterview 2](../assets/screenshots/rasterview-2.png)
  `examples/2D/rasterview`

- ![Refresh Test](../assets/screenshots/refreshtest.png)
  `examples/2D/refreshtest`

- ![Spline Editor](../assets/screenshots/splineeditor.png)
  `examples/2D/splineeditor`

- ![TV Plot](../assets/screenshots/tvplot.png)
  `examples/2D/tvplot`

- ![Curve Tracker](../assets/screenshots/curvetracker.png)
  `playground/curvetracker`

- ![Graphic Scale](../assets/screenshots/graphicscale.png)
  `playground/graphicscale`

- ![Plot Matrix](../assets/screenshots/plotmatrix.png)
  `playground/plotmatrix`

- ![Rescaler](../assets/screenshots/rescaler.png)
  `playground/rescaler`

- ![SVG Map](../assets/screenshots/svgmap.png)
  `playground/svgmap`

- ![Time Scale](../assets/screenshots/timescale.png)
  `playground/timescale`

</div>

### 交互演示

<div class="grid cards" markdown>

- ![Axis Pan](../assets/screenshots/qwt-scale-builtin-action-pan.gif)
  坐标轴拖动

- ![Axis Zoom](../assets/screenshots/qwt-scale-builtin-action-zoom.gif)
  坐标轴缩放

- ![Figure Overlay](../assets/screenshots/figure-widget-overlay.gif)
  Figure 交互蒙版

- ![Series Data Picker](../assets/screenshots/series-data-picker.png)
  数据拾取器

</div>

## 版权信息

Qwt 基于 Qwt License 1.0 发布，完整版权声明如下：

```
Qwt Widget Library
Copyright (C) 1997   Josef Wilgen
Copyright (C) 2002   Uwe Rathmann

Qwt is published under the Qwt License, Version 1.0.
```
