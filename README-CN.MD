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
- [x] 合并为单一文件，简化引入
- [x] 美化控件，坐标轴和画布默认紧贴不再有间隔
- [x] 提供Figure实现绘图的布局
- [x] 增加寄生轴的支持，实现n个坐标轴
- [ ] C++11优化(进行中)
- [ ] 优化超大规模数据的渲染
- [ ] 提供一些集成的交互方法，让使用更简单
    - [x] 提供一个通用的数据拾取功能
    - [] 调整QwtPanner的实现，使其拖动时能实时渲染，并使其能支持多坐标轴
    - [] 调整zoomer的实现，使其支持多坐标轴

总之，我将继续维护`Qwt`，让其成为一个协议友好，性能优越，方便使用的Qt绘图库

## Qwt7.0的新特性

### Cmake支持

Qwt7.0已经支持`CMake`，并且未来将考虑抛弃`QMake`

安装Qwt后，你的项目只需如下即可引入Qwt，免去以往繁琐的配置和预定义宏：

```cmake
target_link_libraries(${YOU_APP_TARGET} PUBLIC qwt::qwt)
```

### 单一头文件和源文件

参考`QCustomPlot`，我把原`Qwt`整个工程合并为`QwtPlot.h`和`QwtPlot.cpp`，只要把这两个文件引入项目即可使用：


### 美化了风格

原有的Qwt样式使用的是很老旧的浮雕风，和现代审美不符，为此我针对性的优化，主要去除了默认的凹陷风格，坐标轴紧贴绘图不进行分离，总体视觉更符合现代风格

具体可见屏幕快照

### 增加了Figure绘图容器

类似Python的matplotlib，Qwt提供了Figure绘图容器，可以很方便的进行多个绘图布局

通过新增的`QwtFigure`类，可以很方便的进行多个绘图布局,支持网格布局（类似matplotlib的subplot）

同步配置了`QwtFigureWidgetOverlay`类，集成了`QwtFigure`的一些操作，例如改变绘图的尺寸，移动绘图

![figure-widget-overlay](./docs/assets/screenshots/figure-widget-overlay.gif)

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

![Figure Widget](./docs/assets/screenshots/qwt_figure.png)


![Simple Plot](./docs/assets/screenshots/simpleplot.png)  
`examples/simpleplot`  

![Bar Chart - Grouped](./docs/assets/screenshots/BarChart-grouped.png)  
`examples/barchart`  

![Bar Chart - Stacked](./docs/assets/screenshots/BarChart-stacked.png)  
`examples/barchart`  

![Scatter Plot](./docs/assets/screenshots/scatterplot.png)  
`examples/scatterplot`  

![Curve Demo](./docs/assets/screenshots/curvedemo.png)  
`examples/curvedemo`  

### 实时可视化

![CPU Monitor](./docs/assets/screenshots/cpuplot.png)  
`examples/cpuplot`  

![Real-Time Plot](./docs/assets/screenshots/realtime.png)  
`examples/realtime`  

![Oscilloscope](./docs/assets/screenshots/oscilloscope.png)  
`examples/oscilloscope`  

### 高级图表

![Polar Demo](./docs/assets/screenshots/polardemo.png)  
`examples/polardemo`  

![Spectrogram](./docs/assets/screenshots/spectrogram.png)  
`examples/spectrogram`  

![Contour Spectrogram](./docs/assets/screenshots/spectrogram-contour.png)  
`examples/spectrogram`  

![Vector Field](./docs/assets/screenshots/vectorfield.png)  
`playground/vectorfield`  

![Stock Chart](./docs/assets/screenshots/stockchart.png)  
`examples/stockchart`  

![Bode Plot](./docs/assets/screenshots/bode.png)  
`examples/bode`  

![Tube Display](./docs/assets/screenshots/friedberg-tube.png)  
`examples/friedberg`  

![plot matrix](./docs/assets/screenshots/plotmatrix.png)  
`playground/plotmatrix`  

![Scale Engine](./docs/assets/screenshots/scaleengine.png)  
`playground/scaleengine`  

![Rescaler](./docs/assets/screenshots/rescaler.png)  
`playground/rescaler`  

![Graphics Scale](./docs/assets/screenshots/graphicscale.png)  
`playground/graphicscale`  

![Spline Editor](./docs/assets/screenshots/splineeditor.png)  
`examples/splineeditor`  

![System Info](./docs/assets/screenshots/sysinfo.png)  
`examples/sysinfo`  

![Distro Watch](./docs/assets/screenshots/distrowatch.png)  
`examples/distrowatch`  

![Raster View 1](./docs/assets/screenshots/rasterview-1.png)  
`examples/rasterview`  

![Raster View 2](./docs/assets/screenshots/rasterview-2.png)  
`examples/rasterview`  

![SVG Map](./docs/assets/screenshots/svgmap.png)  
`playground/svgmap`  

![parasite plot](./docs/assets/screenshots/parasite-plot.png)  
`examples/parasitePlot`  

### 动态演示

![Animated Demo](./docs/assets/screenshots/animated.png)  
`examples/animated`  

![Curve Tracker](./docs/assets/screenshots/curvetracker.png)  
`playground/curvetracker`  

![Refresh Test](./docs/assets/screenshots/refreshtest.png)  
`examples/refreshtest`  

### 样式与符号

![Legends Demo](./docs/assets/screenshots/legends.png)  
`examples/legends`  

![Symbols Demo](./docs/assets/screenshots/symbols.png)  
`playground/symbols`  

![Shape Rendering](./docs/assets/screenshots/shapes.png)  
`playground/shapes`  

### 控件窗口

![Dial Controls](./docs/assets/screenshots/controls-dial.png)  
`examples/controls`  

![Knob Controls](./docs/assets/screenshots/controls-knob.png)  
`examples/controls`  

![Slider Controls](./docs/assets/screenshots/controls-sliders.png)  
`examples/controls`  

![Wheel & Thermo](./docs/assets/screenshots/controls-wheel-thermo.png)  
`examples/controls`  

![Radio Buttons](./docs/assets/screenshots/radio.png)  
`examples/radio`  

![Time Scale Demo](./docs/assets/screenshots/timescale.png)  
`playground/timescale`  

### 仪表盘

![Compass Dial](./docs/assets/screenshots/dials-compass.png)  
`examples/dials`  

![Cockpit Dials](./docs/assets/screenshots/dials-cockpit.png)  
`examples/dials`  
