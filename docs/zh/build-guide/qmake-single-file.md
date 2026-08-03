# qmake 单文件构建

除了[将 QWT 构建为动态库](build-instructions.md)之外，Qwt 还提供了一种
**合并单文件**形式，可以完全跳过构建步骤。`src-amalgamate/` 目录下有两个自动生成的
文件：

- `QwtPlot.h` —— 所有 Qwt 头文件（core + 2D 绘图 + 3D 绘图）合并为一个文件。
- `QwtPlot.cpp` —— 所有 Qwt 源文件合并为一个文件。

只需一行 `#include "QwtPlot.h"` 即可获得完整的 Qwt API。你将这两个文件直接编译进
自己的程序，无需链接 `qwtcore` / `qwtplot` / `qwtplot3d` 任何一个库。

本页说明如何从 **qmake** 工程中引入这两个文件：需要引入哪些 Qt 模块、要设置（以及
要避免）哪些宏，并附上一个可直接使用的 `.pro` 文件。可运行的示例位于
`examples/qmakeExample/`。

!!! tip
    单文件方式非常适合快速集成、原型验证和单文件部署。如果你需要使用共享库，请按
    [CMake 构建说明](build-instructions.md)和[引入 qwt](../use-guide/import-qwt.md) 指南操作。

## 前置条件

- Qt 5.12+ 或 Qt 6.x，且 `qmake` 在 `PATH` 中。
- 本仓库的 `src-amalgamate/` 目录。在下文示例中，该目录位于 `.pro` 文件往上两级
  （`../../src-amalgamate`）；在你自己的工程中，只需把该路径指向你存放这两个文件的位置。

## 需要引入的依赖

合并文件内联了**整个** Qwt 工程——core、2D 绘图、3D 绘图、SVG、OpenGL 和极坐标支持
全部折叠进 `QwtPlot.cpp`。与[库构建方式](build-instructions.md)不同（那里可以用
`QWT_CONFIG_*` 选项关闭 SVG、OpenGL 或 3D），单文件没有这种开关：所有功能始终存在，
因此下面列出的每一项依赖都是必须的。

| 依赖 | 是否必需 | 说明 |
|------|----------|------|
| `core`、`gui`、`widgets` | 是 | 控件与绘图的基础 Qt 模块。 |
| `svg` | 是 | 内联的 `QwtPlotSvgItem` 始终需要它。 |
| `concurrent` | 是 | `QwtSamplingThread` 等辅助类使用。 |
| `printsupport` | 是 | `QwtPlotRenderer` 用于打印/导出。 |
| `opengl` | 是 | 内联的 3D 模块和 `QwtPlotOpenGLCanvas` 需要。 |
| `openglwidgets` | 仅 Qt 6 | Qt 6 将 OpenGL 控件拆分为独立模块；Qt 5 使用已废弃的 `QGLWidget`，不需要它。 |
| 系统 OpenGL（`GL` + `GLU`） | 是 | 3D 模块使用现代 OpenGL（GLSL 着色器 + VBO/VAO），需要系统 GL 库。GLU 仅用于 gl2ps 矢量导出回退。GLU 始终位于独立的系统库中，而在某些 Qt 构建中（例如基于 ANGLE/OpenGL ES 的 MSVC 包）`Qt5OpenGL.lib` 也不会传递性地引入桌面 OpenGL 库，因此两者都必须显式链接：Windows 用 `-lopengl32 -lglu32`，Linux 用 `-lGL -lGLU`。macOS 上 OpenGL framework 同时提供 GL 和 GLU，已由 `QT += opengl` 引入。 |

## 可设置的编译宏

Qwt 的 `*_global.h` 头文件**仅在定义了对应 `*_DLL` 宏时**，才会把各模块的 `*_EXPORT`
宏变为 `__declspec(dllexport)` 或 `__declspec(dllimport)`。当未定义任何 `*_DLL` 宏时，
`*_EXPORT` 解析为空——这正是把合并源码直接编译进可执行文件时所需的行为（既不在构建
DLL，也不在消费 DLL）。

| 宏 | 默认值 | 作用 |
|----|--------|------|
| `QWT_DLL` / `QWT_MAKEDLL` | 未定义 | 2D 绘图模块的 DLL 装饰。**不要定义**——否则会给编译进你二进制文件的符号加上 `dllexport`/`dllimport`。 |
| `QWTCORE_DLL` / `QWTCORE_MAKEDLL` | 未定义 | core 模块的 DLL 装饰。**不要定义。** |
| `QWT3D_DLL` / `QWT3D_MAKEDLL` | 未定义 | 3D 模块的 DLL 装饰。**不要定义。** |
| `QWT3D_NODLL` | 未定义 | 安全开关，强制取消定义 3D 模块的 DLL 宏。仅当你的环境已预定义 `QWT3D_DLL` 时才需要取消注释。 |
| `QWT_DEBUG_DRAW` | `0` | 详细的绘图诊断输出。可选；设为 `1` 启用。 |
| `QWT_DEBUG_PRINT` | `0` | 详细的打印诊断输出。可选；设为 `1` 启用。 |

!!! warning
    `QWT_CONFIG_*`（例如 `QWT_CONFIG_QWTSVG`）是 **CMake 构建选项**，不是 C++ 预处理宏。
    它们对合并单文件无效——单文件始终包含完整功能集。

你**也不需要** `QT_NO_KEYWORDS` 或 `CONFIG += no_keywords`。Qwt 内部使用
`Q_SIGNALS` / `Q_SLOTS` 宏，无论 Qt 关键字是否启用都能正常工作。

## 完整 .pro 示例

下面是 `examples/qmakeExample/` 所用的工程文件。复制后把 `AMALGAMATE_DIR` 改为指向你
自己的 `src-amalgamate/` 即可。

```pro
######################################################################
# Qwt Examples - Copyright (C) 2002 Uwe Rathmann
# This file may be used under the terms of the 3-clause BSD License
######################################################################

TEMPLATE = app
TARGET   = qmakeExample
CONFIG  += warn_on

# --- 合并单文件 ---
AMALGAMATE_DIR = $$PWD/../../src-amalgamate
INCLUDEPATH += $$AMALGAMATE_DIR
DEPENDPATH  += $$AMALGAMATE_DIR
HEADERS += $$AMALGAMATE_DIR/QwtPlot.h     # 自动 moc：所有 Q_OBJECT 都在此
SOURCES += $$AMALGAMATE_DIR/QwtPlot.cpp   # 内部无 Q_OBJECT，无需 moc
SOURCES += main.cpp

# --- Qt 模块：所有 Qwt 功能均已内联，故全部必须 ---
greaterThan(QT_MAJOR_VERSION, 5) {
    QT += core gui widgets svg concurrent opengl printsupport openglwidgets
    CONFIG += c++17
} else {
    QT += core gui widgets svg concurrent opengl printsupport
    CONFIG += c++11
}

# --- 系统 OpenGL GL + GLU ---
win32 {
    LIBS += -lopengl32 -lglu32
}
unix:!macx {
    LIBS += -lGL -lGLU
}
```

!!! tip "为什么 `QwtPlot.h` 要放进 `HEADERS`"
    合并头文件包含全部 `Q_OBJECT` 声明（共 50 处），而 `QwtPlot.cpp` 中一个都没有。
    把 `QwtPlot.h` 放入 `HEADERS`，qmake 的自动 moc 就会生成 `moc_QwtPlot.cpp`，其中
    包含全部元对象代码——这正是 CMake `AUTOMOC` 的 qmake 等价物。`QwtPlot.cpp` 永远
    不需要单独的 moc 步骤。

## 构建步骤

先用 `qmake` 生成 Makefile，再用与你的 Qt 工具链匹配的 make 工具构建：

```shell
cd examples/qmakeExample
qmake qmakeExample.pro
make                # Linux
mingw32-make        # Windows + MinGW
nmake               # Windows + MSVC（在“开发者命令提示符”中运行）
```

或者，在 **Qt Creator** 中打开 `qmakeExample.pro`，直接在 IDE 中构建。

!!! note
    `src-amalgamate/QwtPlot.cpp` 是一个约 2.4 MB 的合并源文件，内联了整个 Qwt 工程。
    首次编译明显比普通示例慢；之后的增量构建很快，因为只会重编译发生变化的文件。

## C++ 标准

Qt 6 要求 C++17；Qt 5 至少需要 C++11。上面的 `.pro` 按条件设置：

```pro
greaterThan(QT_MAJOR_VERSION, 5) {
    CONFIG += c++17
} else {
    CONFIG += c++11
}
```

## 常见问题

| 问题 | 解决办法 |
|------|----------|
| `unresolved external symbol glu*` 或 `gl*`（如 `glGenLists`、`glOrtho`） | 添加系统 OpenGL 库：Windows 用 `-lopengl32 -lglu32`，Linux 用 `-lGL -lGLU`。某些 Qt 构建（ANGLE/OpenGL ES）不会通过 `Qt5OpenGL.lib` 引入桌面 GL 库，需显式链接。macOS 由 OpenGL framework 同时提供。 |
| `Q_OBJECT` / moc 报错，如 `undefined reference to vtable` | 确保把 `QwtPlot.h` 放入 `HEADERS`（而不是仅放 `SOURCES`），这样 qmake 才会对它运行 moc。 |
| `dllexport`/`dllimport` 不匹配或 `QWT_DLL` 重定义 | 删除任何 `DEFINES += QWT_DLL`（或 `QWTCORE_DLL` / `QWT3D_DLL`）行。单文件构建必须让所有 DLL 宏保持未定义。 |
| Qt 6：找不到 `QOpenGLWidget` | 添加 `QT += openglwidgets`（Qt 6 将其拆为独立模块）。 |
| Qt 6 上出现 C++17 特性错误 | 添加 `CONFIG += c++17`。 |
| `QwtPlot.cpp` 编译非常慢 | 首次构建属于正常现象（约 2.4 MB 源码）。之后请使用增量构建；干净构建可考虑使用构建缓存（ccache/clang-cl）。 |
