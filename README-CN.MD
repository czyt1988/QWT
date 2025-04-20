在 2D 曲线高效渲染领域，`QWT` 凭借其友好的开源协议，成为 Qt 框架下的优质之选（相比之下，QCustomPlot 采用 GPL 协议，具有一定的传染性）。然而，随着 `Qwt` 项目进入停更状态，原 `QWT` 已无法满足我的一些需求，例如缺乏 `CMake` 支持。鉴于此，我决定对原始 `QWT` 项目进行维护更新，并将其开源，以更好地适配我的开发场景。

目前，我已发布 tag v0.0.4 版本，为原始 `QWT` 项目引入了 `CMake` 支持。若您仅需为 `QWT` 添加 `CMake` 支持，可直接下载该版本。从 tag v0.0.4 后续版本开始，我将基于自身需求对原 `QWT` 源码进行针对性修改，包括功能拓展与接口优化，同时延续原 `QWT` 的版本号序列（从 6.2 版本起延续原有编号体系）。此项目从 7.0 版本起步，需特别提醒的是，7.0 及后续版本在某些接口上与 `QWT` 6.2.0 存在不兼容之处，请使用者留意。

# changelog

## tag:???（还未发布） | version:7.0.0

- 增强 QwtPlotBarChart 的接口以支持pen和brush的设置.

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
