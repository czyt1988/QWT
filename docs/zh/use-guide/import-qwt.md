# QWT库的引入

## 直接引入

直接引入只需把`QwtPlot.h`和`QwtPlot.cpp`添加到工程里即可

直接引入的cmake示例如下：

```cmake hl_lines="8 9"
# QwtPlot依赖Core Gui Widgets Svg Concurrent OpenGL PrintSupport这几个模块，需要引入工程
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.8 COMPONENTS Core Gui Widgets Svg Concurrent OpenGL PrintSupport REQUIRED)


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
```

## 基于cmake引入SARibbon库

基于cmake引入`QWT`库按照如下步骤执行：

### 1. 指定QWT的安装目录

把安装目录下的`lib/cmake/qwt`位置设置给`qwt_DIR`变量(CMake在执行`find_package(xx)`时，会先查看是否有`xx_DIR`变量，如果有，会先查询`xx_DIR`下是否有对应的`xxConfig.cmake`文件)

!!! tips "提示"
    如果你使用默认安装目录，那么`qwt_DIR`变量可以不设置，cmake能找到

例如:

```cmake
set(QWT_DIR "C:\src\Qt\SARibbon\bin_qt5.14.2_MSVC_x64\lib\cmake\SARibbonBar")
```

### 2. 使用find_package加载库

`QWT` 提供了标准的 `CMake` 配置文件，可以通过 `find_package` 命令引入

qwt依赖的Qt模块为`Core`、`Gui`、`Widgets`、`Svg`、`Concurrent`、`OpenGL`、`PrintSupport`，因此需要添加这些模块的链接

```cmake hl_lines="4 19"
# QwtPlot依赖Core Gui Widgets Svg Concurrent OpenGL PrintSupport这几个模块，需要引入工程
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.8 COMPONENTS Core Gui Widgets Svg Concurrent OpenGL PrintSupport REQUIRED)
find_package(qwt REQUIRED)

# 链接必要的Qt库
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

# 链接 QWT 库到您的目标
target_link_libraries(your_target PRIVATE qwt::qwt)
```


## 公开的预定义宏

`QWT`在编译过程中有些预定义宏，如果你是用`CMake`管理项目，那么通过find_package加载库时，会自动预定定义好，无需手动添加

但如果你使用类似`vistual studio`的界面来手动引入库，你除了添加lib文件外，还需要指定预定于宏。如果是编译为dll库，你需要预定义宏`QWT_DLL`，此宏将指定`QWT_EXPORT`为`Q_DECL_IMPORT`