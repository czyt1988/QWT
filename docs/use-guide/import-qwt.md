# Importing the QWT Library

## Direct Import

Direct import simply requires adding `QwtPlot.h` and `QwtPlot.cpp` to your project.

Here is a CMake example for direct import:

```cmake hl_lines="8 9"
# QwtPlot depends on Core Gui Widgets Svg Concurrent OpenGL PrintSupport modules
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.12 COMPONENTS Core Gui Widgets Svg Concurrent OpenGL PrintSupport REQUIRED)


add_executable(YOU_APP_TARGET
    main.cpp
    QwtPlot.h
    QwtPlot.cpp
    # Your other project files
)

target_link_libraries(YOU_APP_TARGET
    PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::PrintSupport
    )

if(${QT_VERSION_MAJOR} EQUAL 6)
   find_package(Qt${QT_VERSION_MAJOR} ${QWT_MIN_QT_VERSION} COMPONENTS
       OpenGLWidgets
       REQUIRED
   )
   target_link_libraries(YOU_APP_TARGET PRIVATE
       Qt${QT_VERSION_MAJOR}::OpenGLWidgets
   )
endif()
find_package(OpenGL COMPONENTS OpenGL REQUIRED)
target_link_libraries(YOU_APP_TARGET PRIVATE OpenGL::GLU)
```

!!! tip "Tip"
    Since Qwt 7.1, the `qwtplot3d` library has been merged, so OpenGL-related dependencies are required. GLU is only needed for gl2ps vector export.

## Importing the QWT Library via CMake

> **This is the recommended way to import the `QWT` library.**

First, it is not recommended to add the entire QWT library as a subproject to your project. Instead, build and install it locally first. For build and installation instructions, refer to: [QWT Build Instructions](../build-guide/build-instructions.md)

To import the `QWT` library via CMake, follow these steps:

### 1. Specify the QWT Installation Directory

Set the `qwt_DIR` variable to the `lib/cmake/qwt` path under the installation directory. (When CMake executes `find_package(xx)`, it first checks whether an `xx_DIR` variable exists. If so, it looks for the corresponding `xxConfig.cmake` file under `xx_DIR`.)

!!! tips "Tip"
    If you use the default installation directory, you can skip setting the `qwt_DIR` variable — CMake will find it automatically.

For example:

```cmake
set(qwt_DIR "C:\src\my-install-path\cmake\qwt")
```

### 2. Use find_package to Load the Library

`QWT` provides standard `CMake` configuration files and can be imported via the `find_package` command.

```cmake hl_lines="4 7"
find_package(qwt REQUIRED)

# Link QWT's 2D plotting library to your target
target_link_libraries(YOU_APP_TARGET PRIVATE qwt::plot) # Automatically adds Qt dependencies to the target

# Link QWT's 3D plotting library to your target
target_link_libraries(YOU_APP_TARGET PRIVATE qwt::plot3d) # Automatically adds Qt and OpenGL dependencies to the target
```

The `qwt` project provides three modules:

- `core` module — shared base library containing 28 modules of foundational utilities:
    - **Color utilities**: `QwtColorMap` (and subclasses), `QwtColorCycle`, `QwtColorMapPreset` (22 scientific colormap presets)
    - **Math utilities**: `qwt_math.h`, `qwtSimdArgMinMax` (SIMD-accelerated argmin/argmax)
    - **Data types**: `QwtInterval`, `QwtPoint3D`, `QwtPointPolar`, `QwtSamples`, `QwtBoxStatistics`
    - **Geometry**: `QwtBezier` (Bézier curves), `QwtClipper` (polygon clipping)
    - **Coordinate transforms**: `QwtTransform`, `QwtScaleMap`, `QwtScaleDiv`, `QwtScaleEngine`
    - **Date/Time**: `QwtDate`, `QwtSystemClock`
    - **Algorithms & compatibility**: `qwt_algorithm.hpp`, `qwt_qt5qt6_compat.hpp` (Qt5/Qt6 compatibility layer)
    - **Data containers/series/raster**: `QwtGridData`, `QwtSeriesData`, `QwtPointData`, `QwtSeriesStore`, `QwtRasterData`, `QwtMatrixRasterData`, `QwtGridRasterData`
- `plot` module — the 2D plotting library, integrating the original Qwt functionality
- `plot3d` module — the 3D plotting library, integrating the qwtplot3d functionality

```
    ┌────────────────────┐
    │     qwt::core      │  ← foundational utilities (color, math, types, geometry, transforms, time)
    └────────────────────┘
          ↗         ↖
 ┌────────────┐  ┌─────────────┐
 │  qwt::plot  │  │ qwt::plot3d │
 │    (2D)     │  │    (3D)     │
 └────────────┘  └─────────────┘
```

> **Dependency Notes**
>
>    Both `plot` and `plot3d` depend on the `core` module, but are independent of each other.
>
>    - `qwt::core` depends only on Qt `Core` + `Gui`
>    - `qwt::plot` depends on Qt `Core` + `Gui` + `Widgets` (public); `Concurrent` + `PrintSupport` (private). Optional: `Svg` and `OpenGL`/`OpenGLWidgets` (gated by `QWT_CONFIG_QWTSVG` / `QWT_CONFIG_QWTOPENGL`). All dependencies are added automatically when importing via CMake.
>    - `qwt::plot3d` depends on Qt `Core` + `Gui` + `Widgets` + `Qt OpenGL` + `Qt OpenGLWidgets` (Qt6). GLU is only needed for gl2ps vector export.
>
>    If the 3D option is enabled, all OpenGL dependencies are automatically included.

## Public Predefined Macros

`QWT` uses several predefined macros during compilation. If you manage your project with `CMake`, these macros are automatically defined when loading the library via `find_package` — no manual configuration is needed.

However, if you use an IDE such as `Visual Studio` to manually import the library, you need to specify the predefined macros in addition to adding the lib files. When importing the qwtcore library, you need the predefined macro `QWTCORE_DLL`. When importing the qwtplot library compiled as a DLL, you need the predefined macro `QWT_DLL`. When importing the qwtplot3d library, you need the predefined macro `QWT3D_DLL`.

!!! tips "Tip"
    As a modern C++ project, it is strongly recommended to use CMake to build your project. All these predefined macros are pre-configured in the CMake configuration.
