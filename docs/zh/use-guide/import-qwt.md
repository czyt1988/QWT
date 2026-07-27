# QWT库的引入

## 直接引入

直接引入只需把`QwtPlot.h`和`QwtPlot.cpp`添加到工程里即可

直接引入的cmake示例如下：

```cmake hl_lines="8 9"
# QwtPlot依赖Core Gui Widgets Svg Concurrent OpenGL PrintSupport这几个模块，需要引入工程
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.12 COMPONENTS Core Gui Widgets Svg Concurrent OpenGL PrintSupport REQUIRED)


add_executable(YOU_APP_TARGET
    main.cpp
    QwtPlot.h
    QwtPlot.cpp
    # 你的项目其他文件
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

!!! tip "提示"
    qwt7.1之后合并了`qwtplot3d`库，因此需要引入opengl相关的依赖。GLU仅用于gl2ps矢量导出回退。

## 基于cmake引入QWT库

> **这是推荐的引入`QWT`库方式**

首先不建议直接把整个qwt库作为子项目加入到你的项目中，而是先编译并安装到本地，编译安装方法参考:[QWT构建说明](../build-guide/build-instructions.md)

基于cmake引入`QWT`库按照如下步骤执行：

### 1. 指定QWT的安装目录

把安装目录下的`lib/cmake/qwt`位置设置给`qwt_DIR`变量(CMake在执行`find_package(xx)`时，会先查看是否有`xx_DIR`变量，如果有，会先查询`xx_DIR`下是否有对应的`xxConfig.cmake`文件)

!!! tips "提示"
    如果你使用默认安装目录，那么`qwt_DIR`变量可以不设置，cmake能自动找到

例如:

```cmake
set(qwt_DIR "C:\src\my-install-path\cmake\qwt")
```

### 2. 使用find_package加载库

`QWT` 提供了标准的 `CMake` 配置文件，可以通过 `find_package` 命令引入

```cmake hl_lines="4 7"
find_package(qwt REQUIRED)

# 链接 QWT的2D绘图库到您的目标
target_link_libraries(YOU_APP_TARGET PRIVATE qwt::plot) # 会自动把qt的依赖添加到目标中

# 链接 QWT的3D绘图库到您的目标
target_link_libraries(YOU_APP_TARGET PRIVATE qwt::plot3d) # 会自动把qt的依赖以及opengl的依赖添加到目标中
```

`qwt`项目提供了三个模块：

- `core`模块，公共基础库（28 个模块），包含：
    - **颜色工具**：`QwtColorMap`（及子类）、`QwtColorCycle`、`QwtColorMapPreset`（22 种科学 colormap 预设）
    - **数学工具**：`qwt_math.h`、`qwtSimdArgMinMax`（SIMD 加速的 argmin/argmax）
    - **数据类型**：`QwtInterval`、`QwtPoint3D`、`QwtPointPolar`、`QwtSamples`、`QwtBoxStatistics`
    - **几何算法**：`QwtBezier`、`QwtClipper`
    - **坐标变换**：`QwtTransform`、`QwtScaleMap`、`QwtScaleDiv`、`QwtScaleEngine`
    - **时间处理**：`QwtDate`、`QwtSystemClock`
    - **通用算法与兼容层**：`qwt_algorithm.hpp`、`qwt_qt5qt6_compat.hpp`（Qt5/Qt6 兼容层）
    - **数据容器/系列/栅格**：`QwtGridData`、`QwtSeriesData`、`QwtPointData`、`QwtSeriesStore`、`QwtRasterData`、`QwtMatrixRasterData`、`QwtGridRasterData`
- `plot`模块，主要为2D绘图库，是原qwt的功能集成
- `plot3d`模块，主要为3D绘图库（v7.3.5+ 采用 Plot+Item 架构），是qwtplot3d的功能集成

> **依赖说明**
>
>    `plot`和`plot3d`都依赖`core`模块，但彼此独立。qwt依赖的Qt模块为`Core`、`Gui`、`Widgets`（public），`Concurrent`、`PrintSupport`（private），可选`Svg`、`OpenGL`/`OpenGLWidgets`（由`QWT_CONFIG_QWTSVG`/`QWT_CONFIG_QWTOPENGL`控制），这些模块在引入`qwt`库时，会自动添加依赖
>
>   此外`qwt7.1`之后合并了qwtplot3d库，如果3D选项打开，会自动引入`Qt OpenGL`和`Qt OpenGLWidgets`依赖。GLU仅用于gl2ps矢量导出回退。

## 公开的预定义宏

`QWT`在编译过程中有些预定义宏，如果你是用`CMake`管理项目，那么通过`find_package`加载库时，会自动预定定义好，无需手动添加

但如果你使用类似`vistual studio`的界面来手动引入库，你除了添加lib文件外，还需要指定预定于宏。引入qwtcore库你需要预定义宏`QWTCORE_DLL`，引入qwtplot库你需要预定义宏`QWT_DLL`，引入qwtplot3d库你需要预定义宏`QWT3D_DLL`

!!! tips "提示"
    作为现代c++项目，强烈建议使用cmake构建你的项目，这些预定义项都在cmake的配置中预制好