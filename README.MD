In the realm of 2D curve rendering with high efficiency, QWT emerges as a preferable open-source choice within the Qt framework, particularly in contrast to alternatives such as QCustomPlot, which is governed by the GPL license and carries an infectiousness in terms of its licensing implications. However, with the Qwt project no longer being maintained, the original QWT has become unable to meet some of my development needs, like the absence of CMake support. As a result, I have taken it upon myself to maintain and update the original QWT project, and I have also open-sourced it to better fit my development scenario.

So far, I have released the tag v0.0.4 version, which introduces CMake support to the original QWT project. If your only goal is to add CMake support to QWT, you can simply download this version. Starting from the versions after tag v0.0.4, I will make targeted modifications to the original QWT source code based on my own needs. This will include expanding functionality and optimizing interfaces, and I will continue the original QWT version numbering sequence(starting from version 6.2). This project begins at version 7.0. It should be noted that version 7.0 and its subsequent versions have some incompatible interfaces with QWT 6.2.0. USERS ARE ADVISED TO BE AWARE OF THIS.

# changelog

## tag:v0.0.4 | version:6.2.0

- add cmake support

## tag:??? | version:7.0.0

- Enhance the interface of QwtPlotBarChart to support pen and brush settings.

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

    Qwt Widget Library 
    Copyright (C) 1997   Josef Wilgen
    Copyright (C) 2002   Uwe Rathmann

    Qwt is published under the Qwt License, Version 1.0.
    You should have received a copy of this licence in the file
    COPYING.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

# Gallery


## Basic Charts

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

## Real-Time Visualization
![CPU Monitor](docs/screenshots/cpuplot.png)  
`examples/cpuplot`  

![Real-Time Plot](docs/screenshots/realtime.png)  
`examples/realtime`  

![Oscilloscope](docs/screenshots/oscilloscope.png)  
`examples/oscilloscope`  

## Advanced Charts
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

## Dynamic Demos
![Animated Demo](docs/screenshots/animated.png)  
`examples/animated`  

![Curve Tracker](docs/screenshots/curvetracker.png)  
`playground/curvetracker`  

![Refresh Test](docs/screenshots/refreshtest.png)  
`examples/refreshtest`  

## Styling & Symbols
![Legends Demo](docs/screenshots/legends.png)  
`examples/legends`  

![Symbols Demo](docs/screenshots/symbols.png)  
`playground/symbols`  

![Shape Rendering](docs/screenshots/shapes.png)  
`playground/shapes`  

## Control Widgets
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

## Dials

![Compass Dial](docs/screenshots/dials-compass.png)  
`examples/dials`  

![Cockpit Dials](docs/screenshots/dials-cockpit.png)  
`examples/dials`  
