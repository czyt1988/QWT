# qmakeExample

This example shows how to integrate **Qwt** into a **qmake** project using the
amalgamated single file (`src-amalgamate/QwtPlot.h` + `QwtPlot.cpp`).

It compiles the merged source directly into the executable, so it does **not**
link against the built `qwtcore`/`qwtplot`/`qwtplot3d` libraries. A single
`#include "QwtPlot.h"` exposes the entire Qwt API.

For a full explanation of every dependency and compile macro, see
[Qmake Single-File Build](../../docs/build-guide/qmake-single-file.md).

## Prerequisites

- Qt 5.12+ or Qt 6.x (with the modules `Widgets`, `Svg`, `Concurrent`,
  `OpenGL`, `PrintSupport`, and on Qt 6 also `OpenGLWidgets`).
- `qmake` (shipped with Qt).
- The `src-amalgamate/` directory from this repository (two levels up).

## Build

Run `qmake` to generate the Makefile, then build with the make tool that
matches your Qt toolchain:

```shell
cd examples/qmakeExample
qmake qmakeExample.pro
make            # Linux: make / Windows MSVC: nmake / MinGW: mingw32-make
```

Or simply open `qmakeExample.pro` in **Qt Creator** and build from the IDE.

> **Note**
>
> `src-amalgamate/QwtPlot.cpp` is a ~2.4 MB amalgamated source that inlines
> the whole Qwt project. The first compile takes noticeably longer than a
> normal example; subsequent incremental builds are fast.

## What it demonstrates

- A `QwtPlot` with title, grid and canvas background.
- Two `QwtPlotCurve` items (sine and cosine) with different pens and symbols.
- A `QwtPlotMarker` annotation.
- A `QwtLegend`.
- Interaction: `QwtPlotCanvasZoomer` (drag-to-zoom), `QwtPlotPanner`
  (middle-button pan) and `QwtPlotMagnifier` (wheel zoom).
