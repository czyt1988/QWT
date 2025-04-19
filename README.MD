Currently, QWT remains the optimal choice for efficient 2D curve rendering in Qt. But since the Qwt project is no longer maintained, and I have some development requirements that the original QWT can't meet, like CMake support, I'm maintaining and updating the original QWT project and have open - sourced it.

Tag v0.0.4 adds CMake support to the original QWT project. To add CMake support to QWT, just download version v0.0.4. This project starts from version 7.0. I'll modify QWT's source code based on my needs. Note that QWT versions after 7.0 aren't compatible with QWT 6.3 in some interfaces.

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