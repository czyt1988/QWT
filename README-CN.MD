在 2D 曲线高效渲染领域，`QWT` 凭借其友好的开源协议，成为 Qt 框架下的优质之选（相比之下，QCustomPlot 采用 GPL 协议，具有一定的传染性）。然而，随着 `Qwt` 项目进入停更状态，原 `QWT` 已无法满足我的一些需求，例如缺乏 `CMake` 支持。鉴于此，我决定对原始 `QWT` 项目进行维护更新，并将其开源，以更好地适配我的开发场景。

目前，我已发布 tag v0.0.4 版本，为原始 `QWT` 项目引入了 `CMake` 支持。若您仅需为 `QWT` 添加 `CMake` 支持，可直接下载该版本。从 tag v0.0.4 后续版本开始，我将基于自身需求对原 `QWT` 源码进行针对性修改，包括功能拓展与接口优化，同时延续原 `QWT` 的版本号序列（从 6.2 版本起延续原有编号体系）。此项目从 7.0 版本起步，需特别提醒的是，7.0 及后续版本在某些接口上与 `QWT` 6.2.0 存在不兼容之处，请使用者留意。

# 为何选用`QWT`而不是`QCustomPlot`

## Qwt有着友好的开源协议

`QCustomPlot`采用`GPL 协议`，如果你的项目使用了`QCustomPlot`,你的项目也应`GPL 协议`

而`Qwt`的许可证是**LGPL（GNU Lesser General Public License）v2.1**，同时附加了3个条款：

### Qwt的附加条款1——**静态链接例外**

```plaintext
Static linking of applications and widgets to the Qwt library does not constitute a derivative work
and does not require the author to provide source code for the application or widget.
```

- ✅ **允许行为**：
  - 商业软件**静态链接** Qwt 库
  - 无需公开**自有代码**的源代码
  - 无需提供 Qwt 的源代码（前提是使用官方未修改版本）

- ⚠️ **限制条件**：
  ```plaintext
  If you link to a modified version of Qwt, 
  then the changes to Qwt must be provided under the terms of the LGPL
  ```
  - 若修改了 Qwt 源代码，**必须公开修改部分的源代码**

### Qwt的附加条款2——**继承派生例外**

```plaintext
Widgets that are subclassed from Qwt widgets do not constitute a derivative work.
```

- ✅ **允许行为**：
  - 从 Qwt 组件派生子类（如继承 `QwtPlot`）
  - 派生类**不被视为衍生作品**
  - 无需遵循 LGPL 的源代码公开要求

### Qwt的附加条款3——**标识要求**

```plaintext
Programs must identify their use of Qwt:
"[program/widget] is based in part on the work of the Qwt project (http://qwt.sf.net)"
```

- 📌 **必须遵守**：
  - 在软件文档或"关于"页面添加声明：`本软件使用了Qwt项目 (http://qwt.sf.net) 的部分成果`

从上面可看出，Qwt的协议是非常友好的

## `Qwt`有着更抽象的封装，和更高的自由度

`Qwt`的架构更符合软件工程原则，但`QCustomPlot`的简约设计在快速开发中则是更胜一筹的

`QCustomPlot`单头文件和cpp文件实现（qcustomplot.h + qcustomplot.cpp），零依赖，高度集成，降低学习曲线，这样导致内部耦合度较高，扩展性受限

`Qwt`设计更加模块化（Plot, Scale, Curve 等独立组件），遵循经典的 MVC 模式，职责分离清晰，但是组件众多导致初始化复杂

`QCustomPlot`的优势是交互功能开箱即用，例如鼠标缩放，坐标轴缩放，而`Qwt`需要较多的代码来实现，但`Qwt`有着更精细的控制能力

从性能看，`Qwt`理论上是比`QCustomPlot`更有优势的，但`QCustomPlot`做了大数据点的抽稀处理，`Qwt`这方面没有处理，导致超大规模渲染上比`QCustomPlot`弱一点，这正是我想对其进行改进的原因

---
# 目标

- [x] CMake支持 
- [ ] 支持Qt6
- [ ] C++11优化
- [ ] 合并为单一文件，简化引入
- [ ] 优化超大规模数据的渲染
- [ ] 提供一些集成的交互方法，让使用更简单
- [ ] 美化控件

# changelog

## tag:???（还未发布） | version:7.0.0

- 增强`QwtPlotBarChart`的接口以支持pen和brush的设置.
- 增加`QwtGridRasterData`类，相比`QwtMatrixRasterData`，它支持一个二维数据表，以及x,y轴进行网格插值
- `QwtLinearColorMap`增加stopColors函数，修改`QwtLinearColorMap`的`colorStop`函数为`stopPos`

## tag:v0.0.4 | version:6.2.0

- 添加 CMake 支持

# The Qwt Widget Library

this is fork from qwt lib

add the cmake ,can build with qt5 and qt6

Perhaps some improvements will also be submitted to this hub

----------------------

      Qwt is an extension to the libraries of the Qt Project.  

      The Qwt library contains widgets and components which are 
      primarily useful for technical and scientifical purposes.
      It includes a 2-D plotting widget, different kinds of sliders, 
      and much more.

      Qwt is hosted at http://qwt.sf.net

# Copyright

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

# 绘图展示

## 基本图表

![Simple Plot](docs/screenshots/simpleplot.png)  
`examples/simpleplot`  

![Bar Chart - Grouped](docs/screenshots/BarChart-grouped.png)  
`examples/barchart`  

![Bar Chart - Stacked](docs/screenshots/BarChart-stacked.png)  
`examples/barchart`  

![Scatter Plot](docs/screenshots/scatterplot.png)  
`examples/scatterplot`  

![Curve Demo](docs/screenshots/curvedemo.png)  
`examples/curvedemo`  

## 实时可视化

![CPU Monitor](docs/screenshots/cpuplot.png)  
`examples/cpuplot`  

![Real-Time Plot](docs/screenshots/realtime.png)  
`examples/realtime`  

![Oscilloscope](docs/screenshots/oscilloscope.png)  
`examples/oscilloscope`  

## 高级图表

![Polar Demo](docs/screenshots/polardemo.png)  
`examples/polardemo`  

![Spectrogram](docs/screenshots/spectrogram.png)  
`examples/spectrogram`  

![Contour Spectrogram](docs/screenshots/spectrogram-contour.png)  
`examples/spectrogram`  

![Vector Field](docs/screenshots/vectorfield.png)  
`playground/vectorfield`  

![Stock Chart](docs/screenshots/stockchart.png)  
`examples/stockchart`  

![Bode Plot](docs/screenshots/bode.png)  
`examples/bode`  

![Tube Display](docs/screenshots/friedberg-tube.png)  
`examples/friedberg`  

![plot matrix](docs/screenshots/plotmatrix.png)  
`playground/plotmatrix`  

![Scale Engine](docs/screenshots/scaleengine.png)  
`playground/scaleengine`  

![Rescaler](docs/screenshots/rescaler.png)  
`playground/rescaler`  

![Graphics Scale](docs/screenshots/graphicscale.png)  
`playground/graphicscale`  

![Spline Editor](docs/screenshots/splineeditor.png)  
`examples/splineeditor`  

![System Info](docs/screenshots/sysinfo.png)  
`examples/sysinfo`  

![Distro Watch](docs/screenshots/distrowatch.png)  
`examples/distrowatch`  

![Raster View 1](docs/screenshots/rasterview-1.png)  
`examples/rasterview`  

![Raster View 2](docs/screenshots/rasterview-2.png)  
`examples/rasterview`  

![SVG Map](docs/screenshots/svgmap.png)  
`playground/svgmap`  

## 动态演示

![Animated Demo](docs/screenshots/animated.png)  
`examples/animated`  

![Curve Tracker](docs/screenshots/curvetracker.png)  
`playground/curvetracker`  

![Refresh Test](docs/screenshots/refreshtest.png)  
`examples/refreshtest`  

## 样式与符号

![Legends Demo](docs/screenshots/legends.png)  
`examples/legends`  

![Symbols Demo](docs/screenshots/symbols.png)  
`playground/symbols`  

![Shape Rendering](docs/screenshots/shapes.png)  
`playground/shapes`  

## 控件窗口

![Dial Controls](docs/screenshots/controls-dial.png)  
`examples/controls`  

![Knob Controls](docs/screenshots/controls-knob.png)  
`examples/controls`  

![Slider Controls](docs/screenshots/controls-sliders.png)  
`examples/controls`  

![Wheel & Thermo](docs/screenshots/controls-wheel-thermo.png)  
`examples/controls`  

![Radio Buttons](docs/screenshots/radio.png)  
`examples/radio`  

![Time Scale Demo](docs/screenshots/timescale.png)  
`playground/timescale`  

## 仪表盘

![Compass Dial](docs/screenshots/dials-compass.png)  
`examples/dials`  

![Cockpit Dials](docs/screenshots/dials-cockpit.png)  
`examples/dials`  
