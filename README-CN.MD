# 概述

Qt 生态里能画图的库不多，主流的为`QCustomPlot`、`Qwt`、`Qt Charts`和`KDChart`，Qt6.8之后把原来的 `Qt Charts`（2D） 与 Qt DataVisualization（3D） 合并为统一的Qt Graphs模块（注意不是Qt Graphics），底层全部基于 Qt Quick Scene Graph（QSG）+ Qt Quick 3D，彻底放弃了老旧的 Graphics-View/QPainter 管线，不过Qt Graphs 须通过 QQuickWidget 或 QQuickWindow 嵌入，必须带 QML runtime，C++支持不足，[论坛吐槽不少](https://forum.qt.io/topic/159224/qt-graphs-building-2d-plot-using-c-only),虽然Qt Graphs 是 Qt 官方“大一统”的未来，但这个未来可能3年内不会到来，且不支持win7等老系统，对嵌入式也不友好，因此，`QCustomPlot`、`Qwt`、`Qt Charts`和`KDChart`还会是最近几年绘图控件的选项。

这几个控件`QCustomPlot`最简单、美观，推广度最高，只要引入`qcustomplot.h`头文件，`qcustomplot.cpp`源文件，就可以直接使用（[官方文档](https://www.qcustomplot.com/index.php/documentation)），也支持Qt6，然而`QCustomPlot`最大的问题是其开源协议为`GPL`，有传染性，如果你使用了`QCustomPlot`，意味着你的软件也要成为`GPL`，这对商业非常不友好。

`Qwt`是老牌的绘图控件（[官方文档](https://qwt.sourceforge.io/index.html)），有着不错的性能，但部署难，让很多人望而怯步。它的协议为`LGPL`，商业相对友好。

`Qt Charts`是Qt官方的绘图控件（[官方文档](https://doc.qt.io/qt-5/qtcharts-index.html)），效率不高(可以说很低)，不适合做科学计算，同时，Qt Charts 没有 LGPL 选项，开源版是GPLv3，只要你在项目中使用了Qt Charts，就必须 把整个项目以 GPLv3 协议开源。

`KDChart`是KDAB的绘图控件（[官方文档](https://www.kdab.com/software-technologies/developer-tools/kd-chart/)），`KDChart3.0`起是MIT协议，对商业非常友好，但渲染效果一般，一股excel2003的风格，但`KDChart`有甘特图，这是上面3款都没有的。

因此，如果你的项目需要商业用途，那么你只有`Qwt`和`KDChart3.0`两种选择，但`Qwt`作者停止更新，我个人更喜欢`Qwt`，因为`Qwt`的架构更符合软件工程原则，其大规模渲染性能更优，像`QCustomPlot`的优势是交互功能开箱即用，例如鼠标缩放，坐标轴缩放，而`Qwt`需要较多的代码来实现，但`Qwt`有着更精细的控制能力，因此在我的项目需要绘图的时候，我会选择`Qwt`，并根据我的需求添加一些自己需要的功能，改进、优化它，因此，就有了此项目。

项目文档：[https://czyt1988.github.io/QWT/zh/](https://czyt1988.github.io/QWT/zh/)

## Qwt 7.0

我在`Qwt`最后版本上进行了维护，后续我将我需要的功能添加进去，同时逐步修改、优化一些已有的功能，例如它默认的老旧样式

项目地址为：

[Github：https://github.com/czyt1988/QWT](https://github.com/czyt1988/QWT)

[Gitee：https://gitee.com/czyt1988/QWT](https://gitee.com/czyt1988/QWT)

下面是我的目标以及目前我已经完成的一些工作：

- [x] CMake支持
- [x] 支持Qt6
- [x] 合并为单一文件QwtPlot.h/cpp，简化引入
- [x] 美化控件，坐标轴和画布默认紧贴不再有间隔
- [x] 提供Figure实现绘图的布局
- [x] 增加寄生轴的支持，实现n个坐标轴
- [x] C++11优化
- [x] 提供一些集成的交互方法，让使用更简单
  - [x] 提供一个通用的数据拾取功能
  - [x] 调整QwtPanner的实现，使其拖动时能实时渲染，并使其能支持多坐标轴
  - [x] 调整zoomer的实现，使其支持多坐标轴
- [x] 集成3D绘图
- [x] 箱线图（`QwtPlotBoxChart`）
- [x] 曲线降采样算法（4种算法 + SIMD加速），百万级数据流畅渲染
- [x] 颜色循环系统（`QwtColorCycle`）
- [x] 箭头标记（`QwtPlotArrowMarker`）
- [x] 支持坐标刻度朝内(ticks inside)
- [x] Flat风格控件（滑块、旋钮、刻度盘等）
- [x] 模块化架构：core（基础工具库）+ plot（2D绘图）+ plot3d（3D绘图）
- [x] Matplotlib风格绘图接口（`QwtPyPlot`）
- [x] 3D主题系统（10种预设 + 5种光照预设）
- [x] 22种科学colormap预设（viridis、plasma、jet、turbo 等）
- [x] 独立的 `qwt::core` 基础工具库（28个模块，无绘图依赖）

总之，我将继续维护`Qwt`，让其成为一个协议友好，性能优越，方便使用的Qt绘图库

## Qwt7的新特性

### Cmake支持

Qwt7.0已经支持`CMake`，并且抛弃`QMake`

安装Qwt后，你的项目只需如下即可引入Qwt，免去以往繁琐的配置和预定义宏：

```cmake
find_package(qwt)
# 引入2D绘图（自动链接 qwt::core）
target_link_libraries(${YOUR_APP_TARGET} PUBLIC qwt::plot)
# 引入3D绘图（自动链接 qwt::core）
target_link_libraries(${YOUR_APP_TARGET} PUBLIC qwt::plot3d)
# 仅引入基础工具库（数学函数、数据类型、几何算法、坐标变换等）
target_link_libraries(${YOUR_APP_TARGET} PUBLIC qwt::core)
```

### 单一头文件和源文件

参考`QCustomPlot`，我把原`Qwt`整个工程合并为`QwtPlot.h`和`QwtPlot.cpp`，只要把这两个文件引入项目即可使用2D和3D绘图

具体参阅:[QWT库的引入](https://czyt1988.github.io/QWT/zh/use-guide/import-qwt)

参见例子：`examples\staticExample`

![staticExample](./docs/assets/screenshots/staticExample.png)

### 通过 qmake 使用 Qwt 7.0

Qwt 7 不再使用 `qmake` 作为主构建系统，但仍然完全支持通过 `qmake` 集成。你可以参考 `examples/qmakeExample` 示例了解具体用法，或查阅文档：[qmake 单文件构建](docs\zh\build-guide\qmake-single-file.md)

### 美化了风格

原有的Qwt样式使用的是很老旧的浮雕风，和现代审美不符，为此我针对性的优化，主要去除了默认的凹陷风格，坐标轴紧贴绘图不进行分离，总体视觉更符合现代风格

具体可见屏幕快照

### 增加了Figure绘图容器

类似Python的matplotlib，Qwt提供了Figure绘图容器，可以很方便的进行多个绘图布局

通过新增的`QwtFigure`类，可以很方便的进行多个绘图布局,支持网格布局（类似matplotlib的subplot）

同步配置了`QwtFigureWidgetOverlay`类，集成了`QwtFigure`的一些操作，例如改变绘图的尺寸，移动绘图

![figure-widget-overlay](./docs/assets/screenshots/figure-widget-overlay.gif)

`QwtFigure`提供了子图坐标轴对齐功能，可以让多个绘图的坐标轴对齐

![figure-widget-overlay](./docs/assets/picture/figure-scale-aligment.png)

具体可参见教程：[Figure绘图容器](https://czyt1988.github.io/QWT/zh/use-guide/figure-widget/)

### 支持多坐标轴

增加了类似matplotlib的寄生轴功能，支持任意多个坐标轴显示

具体可参见教程：[多坐标轴的创建](https://czyt1988.github.io/QWT/zh/use-guide/parasite-axes/)

### 增加了坐标轴交互

增加坐标轴交互功能，支持在坐标轴上使用鼠标拖动、滚轮缩放等功能

拖动：
![qwt-scale-builtin-action-pan](./docs/assets/screenshots/qwt-scale-builtin-action-pan.gif)

缩放：
![qwt-scale-builtin-action-zoom](./docs/assets/screenshots/qwt-scale-builtin-action-zoom.gif)

具体可参见教程：[坐标轴交互动作](https://czyt1988.github.io/QWT/zh/use-guide/scale-builtin-action/)

### 增加了数据拾取功能

增加`QwtPlotSeriesDataPicker`类，支持数据的拾取

![series-data-picker-yvalue](./docs/assets/picture/series-data-picker-yvalue.gif)

![series-data-picker-nearest-value](./docs/assets/picture/series-data-picker-nearest-value.gif)

具体可参见教程：[绘图数据拾取](https://czyt1988.github.io/QWT/zh/use-guide/pick-value/)

### 修改了画布拖动

重构`QwtPlotPicker`，实现实时拖动画布，原来的`QwtPlotPicker`改名为`QwtPlotCachePanner`。同步支持多坐标轴的画布拖动

效果如下：

![qwt-realtime-panner](./docs/assets/screenshots/qwt-realtime-panner.gif)

具体可参见教程：[拖动工具](https://czyt1988.github.io/QWT/zh/use-guide/panner/)

### 新增了画布放大器

原 `QwtPlotZoomer`需要指定两个坐标轴来进行缩放操作。如果您的绘图需要四个坐标轴都生效，您需要为绘图绑定两个缩放器，使用麻烦，且对于多坐标轴的支持不友好

新增了`QwtPlotCanvasZoomer`无需指定坐标轴，会对整个画布进行整体缩放，且能支持任意多坐标轴

具体可参见教程：[缩放工具](https://czyt1988.github.io/QWT/zh/use-guide/zoomer/)

### 新增箱线图(Box Chart)支持

新增`QwtPlotBoxChart`类，支持绘制箱线图（Box-and-Whisker Plot），用于展示数据的统计分布特征

- 支持预计算数据和原始数据两种输入方式
- 自动计算统计量（中位数、四分位数、异常值）
- 多种箱体样式：矩形、菱形、缺口形
- 支持垂直和水平两种显示方向
- 异常值自动检测和自定义符号

![Box Chart](./docs/assets/screenshots/BoxChart.png)

具体可参见教程：[箱线图使用指南](https://czyt1988.github.io/QWT/zh/use-guide/boxchart/)

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

具体可参见教程：[刻度朝内显示使用指南](https://czyt1988.github.io/QWT/zh/use-guide/ticks-inside/)，示例代码位于 `examples/2D/ticks_inside`

### Matplotlib风格绘图接口（`QwtPyPlot`）

7.3 新增的 `QwtPyPlot` 是一套受 Python `matplotlib.pyplot` 启发的高层有状态接口。它封装了 `QwtFigure`（或单个 `QwtPlot`），底层委托给现有的 Qwt 设施，既拥有 matplotlib 般的简洁写法，又不损失 Qwt 的强大能力与精细控制。

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

具体可参见教程：[QwtPyPlot — Matplotlib风格接口](https://czyt1988.github.io/QWT/zh/use-guide/pyplot-api/)

### 3D绘图与主题系统

集成的 3D 模块（`qwt::plot3d`）采用 **Plot + Item** 架构（`Qwt3DPlot` + `Qwt3DPlotItem`），与 2D 模块对称。提供表面图、网格图和函数图，基于现代 OpenGL 渲染（VBO/VAO + GLSL着色器），并支持完整的鼠标交互（旋转 / 缩放 / 平移）。

7.3 新增的 `Qwt3DTheme` 主题系统将全部视觉属性 —— 背景、网格、colormap、坐标轴、标题、光照、着色模式、材质 —— 打包为一个描述符，一行调用即可套用整套风格：

```cpp
// 创建渲染窗口 + 曲面item
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
```

- 10 种内置主题：`Default`、`Dark`、`Scientific`、`Warm`、`Cool`、`Matplotlib`、`EarthTones`、`Ocean`、`HighContrast`、`Presentation`
- 5 种光照预设：`NoLighting`、`FlatLight`、`Studio`、`Outdoor`、`Soft`
- `Qwt3DColorMapColor` 适配器将 core 模块的 colormap 桥接到 3D 表面，因此 22 种科学预设同样可用于 3D

具体可参见教程：[3D绘图](https://czyt1988.github.io/QWT/zh/use-guide/3d-plot/)

### 科学Colormap预设

`qwt::core` 内置 `QwtColorMapPreset` —— 22 种知名科学 colormap（与 matplotlib / MATLAB 同名），2D 光谱图与 3D 表面图共用：

`viridis`、`plasma`、`inferno`、`magma`、`cividis`、`jet`、`hot`、`cool`、`spring`、`summer`、`autumn`、`winter`、`gray`、`bone`、`copper`、`rainbow`、`hsv`、`turbo`、`coolwarm`、`rdylbu`、`rdylgn`、`spectral`

```cpp
auto cmap = QwtColorMapPreset::create("viridis");
// cmap->rgb(vMin, vMax, value) 返回给定数据值对应的颜色
```

其中感知均匀的 colormap（`viridis`、`plasma`、`inferno`、`magma`、`cividis`）对色盲友好且打印安全，非常适合科研论文。

### SIMD加速的曲线降采样

百万级数据点的流畅渲染现已成为一等公民。`QwtPlotCurve` 提供四种降采样算法，其中 MinMax-bucket 路径采用 SIMD 加速（SSE2 / AVX2 / NEON，运行时自动检测）：

- `FilterPoints` —— 原始剔除算法
- `FilterPointsAggressive` —— 针对单调大数据集的快速剔除
- `FilterPointsPixel` —— 按像素列归约，适用于像素对齐的粗趋势
- `FilterPointsLTTB` —— 保形且性能最优的 LTTB（MinMax bucket）（新默认值）

效果：百万点曲线可交互帧率渲染。可用 `renderbench` 示例自行基准测试。

具体可参见教程：[曲线降采样](https://czyt1988.github.io/QWT/zh/use-guide/curve-downsampling/)

### 独立的Core基础工具库

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

- 修复了`NAN`值和`INF`值对绘图的影响
- `QwtPlotSeriesDataPicker` 现已支持所有系列图元类型的拾取 —— K线、区间曲线、直方图、矢量场、柱状/箱线图、光谱曲线等
- 全代码库现代化 C++：`override`/`final`、`nullptr`、移动语义、`constexpr`、`noexcept`、范围 for，以及统一的 PIMPL 宏模式（180+ 文件）
- `renderbench` 示例用于测量曲线渲染性能并生成报告

## changelog

详细的日志请参阅[CHANGES.MD](./CHANGES.md)

## Copyright

---------

    Qwt Widget Library 
    Copyright (C) 1997   Josef Wilgen
    Copyright (C) 2002   Uwe Rathmann

    Qwt is published under the Qwt License, Version 1.0.
    You should have received a copy of this licence in the file
    COPYING.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

----------------------

## 绘图展示

项目文档：[https://czyt1988.github.io/QWT/zh/](https://czyt1988.github.io/QWT/zh/)

### 基本图表

|  |  |  |
|:---:|:---:|:---:|
|![Figure Widget](./docs/assets/picture/figure-scale-aligment.png)|![Simple Plot](./docs/assets/screenshots/simpleplot.png)|![Bar Chart - Grouped](./docs/assets/screenshots/BarChart-grouped.png)|
|`examples/figure`|`examples/simpleplot`|`examples/simpleplot`|

|  |  |  |
|:---:|:---:|:---:|
|![Bar Chart - Stacked](./docs/assets/screenshots/BarChart-stacked.png) |![Scatter Plot](./docs/assets/screenshots/scatterplot.png)  |![Curve Demo](./docs/assets/screenshots/curvedemo.png)  |
|`examples/barchart`  |`examples/scatterplot`  |`examples/curvedemo` |

### 实时可视化

|  |  |  |
|:---:|:---:|:---:|
|![CPU Monitor](./docs/assets/screenshots/cpuplot.png)|![Real-Time Plot](./docs/assets/screenshots/realtime.png)|![Oscilloscope](./docs/assets/screenshots/oscilloscope.png)  |
|`examples/cpuplot`|`examples/realtime`|`examples/oscilloscope` |

### 高级图表

|  |  |  |
|:---:|:---:|:---:|
|![Polar Demo](./docs/assets/screenshots/polardemo.png)|![Spectrogram](./docs/assets/screenshots/spectrogram.png)|![Contour Spectrogram](./docs/assets/screenshots/spectrogram-contour.png) |
|`examples/polardemo`|`examples/spectrogram`|`examples/spectrogram`|

|  |  |  |
|:---:|:---:|:---:|
|![Vector Field](./docs/assets/screenshots/vectorfield.png)|![Stock Chart](./docs/assets/screenshots/stockchart.png)|![Bode Plot](./docs/assets/screenshots/bode.png)|
|`playground/vectorfield`|`examples/stockchart`|`examples/bode`|

|  |  |  |
|:---:|:---:|:---:|
|![Tube Display](./docs/assets/screenshots/friedberg-tube.png)|![plot matrix](./docs/assets/screenshots/plotmatrix.png)|![Scale Engine](./docs/assets/screenshots/scaleengine.png)|
|`examples/friedberg`|`playground/plotmatrix`|`playground/scaleengine`|

|  |  |  |
|:---:|:---:|:---:|
|![Rescaler](./docs/assets/screenshots/rescaler.png)|![Graphics Scale](./docs/assets/screenshots/graphicscale.png)|![Spline Editor](./docs/assets/screenshots/splineeditor.png)|
|`playground/rescaler`|`playground/graphicscale`|`examples/splineeditor`|

|  |  |  |
|:---:|:---:|:---:|
|![Ticks Inside](./docs/assets/screenshots/ticks_inside.png)|![Box Chart](./docs/assets/screenshots/BoxChart.png)|![parasite plot](./docs/assets/screenshots/parasite-plot.png)|
|`examples/ticks_inside`|`examples/boxchart`|`examples/parasitePlot`|

|  |  |  |
|:---:|:---:|:---:|
|![System Info](./docs/assets/screenshots/sysinfo.png)|![Distro Watch](./docs/assets/screenshots/distrowatch.png)|![Raster View 1](./docs/assets/screenshots/rasterview-1.png)|
|`examples/sysinfo`|`examples/distrowatch`|`examples/rasterview`|
  
|  |  |  |
|:---:|:---:|:---:|
|![Raster View 2](./docs/assets/screenshots/rasterview-2.png)|![SVG Map](./docs/assets/screenshots/svgmap.png)|![Item Editor](./docs/assets/screenshots/itemeditor.png)|
|`examples/rasterview`|`playground/svgmap`|`examples/itemeditor`|

### 3D可视化

|  |  |  |
|:---:|:---:|:---:|
|![Simple 3D Plot](./docs/assets/screenshots-3D/simpleplot3D.png)|![3D Axes](./docs/assets/screenshots-3D/axes.png)|![3D Enrichments](./docs/assets/screenshots-3D/enrichments.png)|
|`examples/3D/simpleplot3D`|`examples/3D/axes`|`examples/3D/enrichments`|

|  |  |  |
|:---:|:---:|:---:|
|![3D Autoswitch](./docs/assets/screenshots-3D/autoswitch.png)|![figureSurface3D](./docs/assets/screenshots-3D/figureSurface3D.png)|
|`examples/3D/autoswitch`|`examples/3D/figureSurface3D`|

### 动态演示

|  |  |  |
|:---:|:---:|:---:|
|![Animated Demo](./docs/assets/screenshots/animated.png)|![Curve Tracker](./docs/assets/screenshots/curvetracker.png)|![Refresh Test](./docs/assets/screenshots/refreshtest.png)|
|`examples/animated`|`playground/curvetracker`|`examples/refreshtest`|

### 样式与符号

|  |  |  |
|:---:|:---:|:---:|
|![Legends Demo](./docs/assets/screenshots/legends.png)|![Symbols Demo](./docs/assets/screenshots/symbols.png)|![Shape Rendering](./docs/assets/screenshots/shapes.png)|
|`examples/legends`|`playground/symbols`|`playground/shapes`|

### 控件窗口

|  |  |  |
|:---:|:---:|:---:|
|![Dial Controls](./docs/assets/screenshots/controls-dial.png)|![Knob Controls](./docs/assets/screenshots/controls-knob.png)|![Slider Controls](./docs/assets/screenshots/controls-sliders.png)|
|`examples/controls`|`examples/controls`|`examples/controls`|

|  |  |  |
|:---:|:---:|:---:|
|![Wheel & Thermo](./docs/assets/screenshots/controls-wheel-thermo.png)|![Radio Buttons](./docs/assets/screenshots/radio.png)|![Time Scale Demo](./docs/assets/screenshots/timescale.png)|
|`examples/controls`|`examples/radio`|`playground/timescale`|

### 仪表盘

|  |  |  |
|:---:|:---:|:---:|
|![Compass Dial](./docs/assets/screenshots/dials-compass.png)|![Cockpit Dials](./docs/assets/screenshots/dials-cockpit.png)||
|`examples/dials`|`examples/dials`||
