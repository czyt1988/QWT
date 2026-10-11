# Figure 多子图与 PyPlot 风格 API（`src/plot/`）

两个相关但独立的能力，都围绕"把多个 QwtPlot 组织成一个图"：

## QwtFigure —— matplotlib 式的 Figure 容器

`src/plot/qwt_figure.h`（`QwtFigure`，继承 `QFrame`）+ `qwt_figure_layout.h`（`QwtFigureLayout`）。
用法是"先建普通 QwtPlot，再按归一化矩形（0~1）摆进 figure"：

```cpp
QwtFigure* fig = new QwtFigure;
QwtPlot*  p1  = new QwtPlot;
fig->addAxes(p1, 0.1, 0.55, 0.8, 0.35);   // left, top, width, height（归一化）
fig->addGridAxes(p2, rows, cols, row, col);  // 网格排布（含 rowSpan/colSpan/间距参数）
fig->setSizeInches(6.4f, 4.8f);            // matplotlib 风格设尺寸
```

- 寄生轴也归 Figure 管：`createParasiteAxes(hostPlot, QwtAxis::Position)` /
  `getParasiteAxes()`（比直接用 `QwtPlot::createParasitePlot()` 多一层布局管理）。
- 当前子图切换：`setCurrentAxes()` / `sca()` / `gca()` / `currentAxes()`。
- **读码顺序**：`examples/2D/figure/MainWindow.cpp`（含对数轴 + `QwtPlotSeriesDataPicker`
  联动拾取，是本仓库信息密度最高的 Figure 示例）；文档 `docs/use-guide/figure-widget.md`。

## QwtFigureWidgetOverlay —— Figure 上的交互层

`src/plot/qwt_figure_widget_overlay.h`（`QWidgetOverlay` 子类）。
拖动、框选缩放单个子绘图等手势由 overlay 实现，**不要自己给子 plot 装 zoomer**——
先看 Figure 示例里 overlay 与 picker 如何分工（`examples/2D/figure/PickLinker.cpp`）。

## QwtPyPlot —— matplotlib 风格的一层封装

`src/plot/qwt_pyplot.h`（`QwtPyPlot`，QObject）。不新建渲染逻辑，只是把
`QwtPlot` + `QwtPlotCurve` + `QwtColorMap` 包装成 `plot/scatter/bar/boxplot/errorbar/imshow/
quiver/subplot/twinx/colorbar/legend/setXLim/setXScale("log")/namedColor()/createColorMap()`
这类命令式 API，内部维护"当前 axes"指针（`gca()`/`sca()`），和 matplotlib pyplot 一致：

```cpp
QwtPyPlot* plt = new QwtPyPlot(figure);   // 或 new QwtPyPlot(plot, this);
plt->subplot(2, 1, 1);
plt->plot({0, 1, 2, 3}, {1, 4, 2, 5}, "r-o", "Temperature");
plt->subplot(2, 1, 2);
plt->scatter(x, y, 50, "r");
plt->legend("best");
```

- 示例：`examples/2D/pyplot/main.cpp`（单图最小 demo）。
- 文档：`docs/use-guide/pyplot-api.md`（中文 `docs/zh/use-guide/pyplot-api.md`）。
- 注意 `QwtPyPlot` 只是薄封装：需要它不支持的细粒度控制时，直接退回操作底层
  `QwtPlot`/`QwtPlotCurve`（见 `plot-2d.md`），不要改封装层去硬塞。
