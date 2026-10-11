# 2D 绘图板块（`src/plot/` 的绘图部分，`qwt::plot`）

依赖 `qwt::core`，导出宏 `QWT_EXPORT`。控件、极坐标、Figure/PyPlot 虽然也在 `src/plot/` 下，
但已拆到 `widgets.md` / `polar.md` / `figure-pyplot.md`，本文件只管"画图"本身。

## 第一步：读懂 QwtPlot 主类

`src/plot/qwt_plot.h` / `qwt_plot.cpp` —— 一切 2D 绘图的宿主。先搞懂三件事：

1. **坐标轴用 `QwtAxisId`（Position + 序号），不是 int**：`QwtAxis::XBottom/YLeft/XTop/YRight`；
   旧枚举 `QwtPlot::yLeft` 等仍通过 `QWT_AXIS_COMPAT` 兼容。API 形如
   `setAxisScale(QwtAxis::XBottom, ...)` / `enableAxis(QwtAxis::YRight)`。
   参考 `examples/2D/parasitePlot`（多轴）与 `docs/use-guide/parasite-axes.md`。
2. **画布是 `QwtPlotCanvas`（QWidget）**：`plot->canvas()`；
   透明画布 `QwtPlotTransparentCanvas`、OpenGL 画布 `QwtPlotGLCanvas` /
   `QwtPlotOpenGLCanvas`（示例 `examples/2D/animation`、`refreshtest`，文档 `opengl.md`）。
3. **所有"画的东西"都是 `QwtPlotItem`**，通过 `item->attach(plot)` 挂载。
   ⚠ `QwtPlotItem` **不是 QObject**，没有信号槽——详见 `gotchas.md`。

从零开始的最短路径：`examples/2D/simpleplot`（最小可运行）+ `docs/use-guide/first-plot.md`。

## PlotItem 家族：要画什么，读哪个文件、抄哪个示例

| 要画的东西 | 类 / 头文件 | 最佳示例 | 文档（`docs/use-guide/`） |
|-----------|-----------|---------|--------------------------|
| 折线/阶梯/点线曲线（最常用） | `QwtPlotCurve` `qwt_plot_curve.h` | `simpleplot`、`curvedemo` | `curve.md` |
| 实时曲线、增量刷新 | `QwtPlotCurve` + `QwtPlotDirectPainter` | `realtime`、`oscilloscope`（`QwtSamplingThread`）、`cpuplot`、`tvplot` | `curve.md` |
| 曲线平滑 | `QwtPlotCurve::setSmoothAlgorithm()`（`qwt_plot_curve.h`；实现在 `qwt_curve_smoothing.h` 的自由函数里，非公开 API） | `curvesmoothing` | `curve-smoothing.md` |
| 曲线降采样 | `QwtCurveFitter` 族，见下文 | `nanperf` | `curve-downsampling.md` |
| 大数据量/NAN 处理 | `QwtPlotCurve` + fitter | `friedberg`、`nanperf` | `nan-handling.md` |
| 柱状图（分组） | `QwtPlotMultiBarChart` `qwt_plot_multi_barchart.h` | `barchart`、`distrowatch` | `barchart.md` |
| 单柱状图 | `QwtPlotBarChart` `qwt_plot_barchart.h` | `distrowatch` | `barchart.md` |
| 直方图 | `QwtPlotHistogram` `qwt_plot_histogram.h` | `tvplot` | `histogram.md` |
| 箱线图 | `QwtPlotBoxChart` `qwt_plot_boxchart.h`（统计量在 core 的 `QwtBoxStatistics`） | `boxchart` | `boxchart.md` |
| 频谱图/热力图 | `QwtPlotSpectrogram` `qwt_plot_spectrogram.h`（数据用 `QwtRasterData`） | `spectrogram`、`rasterview` | `spectrogram.md` |
| 交易/K线曲线 | `QwtPlotTradingCurve` `qwt_plot_tradingcurve.h` | `stockchart` | `tradingcurve.md` |
| 区间曲线（误差棒） | `QwtPlotIntervalCurve` `qwt_plot_intervalcurve.h` | `friedberg` | `intervalcurve.md` |
| 散点图 | `QwtPlotCurve` + `QwtSymbol::Ellipse`/`XCross` | `scatterplot` | `scatter.md` |
| 矢量场/风向 | `QwtPlotVectorField` `qwt_plot_vectorfield.h` | `parasitePlot`、`playground/vectorfield` | `vectorfield.md` |
| 图形标注（矩形/椭圆/路径） | `QwtPlotShapeItem` `qwt_plot_shapeitem.h` | `itemeditor`（含交互编辑） | `shapeitem.md` |
| 图片/图标 | `QwtPlotGraphicItem`、`QwtPlotSvgItem` | —（看头文件 usage + `qwt_graphic.h`） | — |
| 文字标注 | `QwtPlotTextLabel` `qwt_plot_textlabel.h` | — | `textlabel.md` |
| 高亮区间带 | `QwtPlotZoneItem` `qwt_plot_zoneitem.h` | `stockchart` | — |
| 图表内图例 | `QwtPlotLegendItem` `qwt_plot_legenditem.h` | `legends`、`sinusplot` | `legend.md` |
| 画布角标/十字线 | `QwtPlotMarker` `qwt_plot_marker.h`（`LineStyle`：HLine/VLine/Cross） | `cpuplot`（饼图 marker） | `marker.md` |

**自定义 item**：继承 `QwtPlotItem`，实现纯虚 `draw(painter, xMap, yMap, canvasRect)`；
`rtti()`、`boundingRect()` 也应重写（要显示图例条目再加 `legendData()`）。
照抄最简单的现有实现：`src/plot/qwt_plot_zoneitem.cpp` 或 `qwt_plot_textlabel.cpp`。

## 交互工具：缩放、平移、放大、拾取

| 需求 | 类 / 头文件 | 示例 |
|------|-----------|------|
| 框选缩放（按轴） | `QwtPlotAxisZoomer` `qwt_plot_axis_zoomer.h` | `bode`、`spectrogram`、`stockchart` |
| 整画布缩放（不限轴数） | `QwtPlotCanvasZoomer` `qwt_plot_canvas_zoomer.h` | `parasitePlot` |
| 拖拽平移（像素缓存，稳） | `QwtCachePanner` / `QwtPlotCachePanner` | `radio` |
| 实时拖动平移 | `QwtPlotPanner`（基于 `QwtPicker`） | `sinusplot`、`scatterplot` |
| 滚轮放大 | `QwtPlotMagnifier` `qwt_plot_magnifier.h` | `scatterplot`、`rasterview` |
| 鼠标拾取值/最近点 | `QwtPlotPicker` / `QwtPlotSeriesDataPicker`（含 `...Group` 版多曲线联动） | `scatterplot`、`stockchart`、`figure` |
| 自定义拾取状态机 | `QwtPickerMachine` `qwt_picker_machine.h`；事件键位 `QwtEventPattern` | 文档 `picker-state-machine.md`、`pick-value.md` |
| 轴上滚轮滚动 | `QwtPlotAxisWheelInteraction` | 文档 `scale-builtin-action.md` |
| 数据范围变化自动重算轴 | `QwtPlotRescaler` `qwt_plot_rescaler.h` | `playground/rescaler` |
| 轴事件统一分发 | `QwtPlotScaleEventDispatcher` | — |

拾取器是"挂在 canvas 上的 QObject 事件过滤器"，所以才需要
`new QwtPlotAxisZoomer(QwtAxis::XBottom, QwtAxis::YLeft, plot->canvas())` 这种构造方式。

## 刻度、网格、样式、导出

- **刻度**：`QwtScaleDraw`（`src/plot/qwt_scale_draw.h`）+ `QwtScaleWidget`（轴本体）；
  日期轴 `QwtDateScaleDraw/QwtDateScaleEngine`（示例 `playground/timescale`）；
  刻度内嵌 `ticks_inside` 示例；文档 `scale-widget.md` / `scale-engine.md`。
- **网格**：`QwtPlotGrid` `qwt_plot_grid.h`，文档 `grid.md`。
- **文字/富文本**：`QwtText` `qwt_text.h`（跨引擎渲染），`QwtTextLabel`（独立控件版）。
- **曲线拟合与样条**：`QwtCurveFitter` `qwt_curve_fitter.h`，实现类
  `QwtSplineCurveFitter` / `QwtWeedingCurveFitter`（降采样）；`QwtSpline` 家族
  （`qwt_spline.h` + `qwt_spline_cubic/local/pleasing/polynomial/parametrization.h`），
  示例 `splineeditor`。
- **导出/打印（PDF、SVG、图片、矢量）**：`QwtPlotRenderer` `qwt_plot_renderer.h`，
  文档 `renderer.md`。
- **样式表换肤**：`QwtPlotStyling` + `QwtStyleSheetRecorder`（`qwt_stylesheet_recorder.h`），
  示例 `stylesheets`。
- **坐标映射**：`QwtPlotTransform` `qwt_plot_transform.h`（如幂律坐标）。
- **布局**：`QwtPlotLayout` / `QwtPlotLayoutEngine`，文档 `plot-layout.md`；标题放画布外见
  `outsideTitle` 示例。

## 建议的阅读顺序（接到"用 Qwt 画个 XX 图"的任务时）

1. `examples/2D/simpleplot/main.cpp` —— 建立最小骨架（plot → item → attach → show）。
2. 在上表找到离任务最近的示例，读它的 `Plot.h/Plot.cpp`（不是 `main.cpp`）。
3. 对照 `docs/use-guide/<对应文档>.md`（中文版在 `docs/zh/use-guide/`）确认 API 全貌。
4. 仍不确定某个 setter 语义时，直接读 `src/plot/qwt_plot_*.h` 的头文件注释——
   本项目头文件注释即 API 文档（英文 Doxygen）。
