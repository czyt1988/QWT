请先阅读 `README.md` 以了解全局规范。

# plan13 — 验证示例基础设施（截图工具、公共头文件、CMake 注册）

## 目标

为所有 3D 示例提供统一的截图导出基础设施，使每个示例都能通过命令行参数自动截图并保存到指定路径。这服务于以下场景：

1. **批量文档生成**：一次性运行所有 3D 示例并截图到 `docs/assets/screenshots-3D/` 目录，用于文档插图
2. **AI 验证**：AI agent 可以运行示例截图，通过图像验证渲染效果是否正确
3. **CI/CD 集成**：在持续集成中自动生成截图，对比基线图片检测回归
4. **交互式使用**：不带截图参数时，示例正常运行，支持鼠标交互

## 前置依赖

无。本计划完全独立，可在任何时刻执行。

## 涉及文件

### 新建文件

| 文件 | 说明 |
|------|------|
| `examples/3D/common/screenshot_helper.h` | Header-only 截图工具（命令行解析、延迟截图、自动退出） |

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `examples/CMakeLists.txt` | 注册所有新增 3D 示例子目录（plan03-12 对应的 9 个示例） |

---

## 技术方案

### 1. 截图工具设计（screenshot_helper.h）

采用 header-only 设计，所有 3D 示例通过 `#include "screenshot_helper.h"` 引入。无需编译额外的 .cpp 文件，也无需链接额外的库。

#### 1.1 命令行接口

| 参数 | 格式 | 说明 |
|------|------|------|
| `-p <path>` | `-p "output/screenshot.png"` | 截图输出路径（PNG 格式）。指定后程序在渲染完成后截图并自动退出 |
| `-w <width>` | `-w 1024` | 窗口宽度（默认 800） |
| `-h <height>` | `-h 768` | 窗口高度（默认 600） |
| `-d <ms>` | `-d 1000` | 截图前延迟毫秒数（默认 500ms，复杂场景可增大） |

使用示例：

```powershell
# 基本截图
scatterplot.exe -p "docs/assets/screenshots-3D/scatterplot-point.png"

# 指定窗口尺寸和延迟
scatterplot.exe -p "docs/assets/screenshots-3D/scatterplot-sphere.png" -w 1024 -h 768 -d 1000

# 交互式运行（不带 -p）
scatterplot.exe
```

#### 1.2 实现

```cpp
// examples/3D/common/screenshot_helper.h
#ifndef SCREENSHOT_HELPER_H
#define SCREENSHOT_HELPER_H

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QOpenGLWidget>
#include <QString>
#include <QTimer>
#include <cstring>

namespace Qwt3DExamples {

/// @brief Screenshot capture configuration parsed from command line
struct ScreenshotConfig {
    QString outputPath;  ///< Output file path (empty = no screenshot)
    int width = 800;     ///< Window width
    int height = 600;    ///< Window height
    int delayMs = 500;   ///< Delay before capture (ms)
};

/// @brief Parse command line arguments for screenshot configuration
/// @param argc Argument count
/// @param argv Argument array
/// @return Parsed configuration; outputPath is empty if no -p flag
inline ScreenshotConfig parseScreenshotArgs(int argc, char** argv)
{
    ScreenshotConfig config;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            config.outputPath = QString::fromLocal8Bit(argv[++i]);
        } else if (std::strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            config.width = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            config.height = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            config.delayMs = std::atoi(argv[++i]);
        }
    }
    return config;
}

/// @brief Capture a screenshot from a QOpenGLWidget and save it
/// @param widget The OpenGL widget to capture
/// @param path Output file path (PNG recommended)
/// @return true if the screenshot was saved successfully
inline bool captureScreenshot(QOpenGLWidget* widget, const QString& path)
{
    if (!widget || path.isEmpty())
        return false;

    // Force a repaint to ensure the latest frame is rendered
    widget->repaint();

    QImage image = widget->grabFramebuffer();
    if (image.isNull())
        return false;

    // Create directory if it doesn't exist
    QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath());

    return image.save(path);
}

/// @brief Set up automatic screenshot capture for a 3D example
/// @details If a -p argument is found in command-line args, this function
///          sets up a delayed capture after the widget's first frame is rendered.
///          The application will quit after saving the screenshot.
///          If no -p argument is found, the application runs normally (interactive).
/// @param widget The 3D plot widget (QOpenGLWidget subclass)
/// @param argc Command-line argument count
/// @param argv Command-line argument array
/// @param app The QApplication instance (needed for quit after capture)
inline void setupScreenshotCapture(QOpenGLWidget* widget, int argc, char** argv,
                                    QApplication& app)
{
    ScreenshotConfig config = parseScreenshotArgs(argc, argv);
    if (config.outputPath.isEmpty())
        return;  // No screenshot requested, run normally

    // Resize widget to configured dimensions
    if (config.width > 0 && config.height > 0)
        widget->resize(config.width, config.height);

    // Use a delayed capture to ensure GL initialization is complete
    // The delay allows the first frame to be rendered before capture
    QTimer::singleShot(config.delayMs, [widget, config, &app]() {
        if (captureScreenshot(widget, config.outputPath)) {
            app.quit();
        } else {
            qWarning("Screenshot capture failed: %s",
                     qPrintable(config.outputPath));
            app.exit(1);
        }
    });
}

}  // namespace Qwt3DExamples

#endif  // SCREENSHOT_HELPER_H
```

#### 1.3 截图时序保证

QOpenGLWidget 的 `grabFramebuffer()` 读取当前帧缓冲区内容。为确保截图时已有一帧完整渲染：

1. **`show()` 触发首次渲染**：`QWidget::show()` 触发 `initializeGL()` + `paintGL()`
2. **`QTimer::singleShot(delayMs, ...)`** 延迟截图：延迟期间 Qt 事件循环处理 `paintGL` 事件
3. **`widget->repaint()`** 强制同步重绘：在 `grabFramebuffer()` 前确保最新数据已渲染
4. **`grabFramebuffer()`** 读取帧缓冲：内部调用 `glReadPixels`

默认 500ms 延迟覆盖大多数场景。复杂场景（如体积渲染）可通过 `-d 1000` 增大延迟。

### 2. 示例 main.cpp 标准模板

所有 3D 绘图类型的示例 main.cpp 应遵循此模板：

```cpp
#include <QApplication>
#include <qwt3d_xxxplot.h>
#include "screenshot_helper.h"  // 来自 examples/3D/common/

using namespace Qwt3D;

class Plot : public XxxPlot
{
public:
    Plot()
    {
        // 加载数据、设置样式、坐标系等
        // ...
        updateData();
        update();
    }
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    Plot plot;
    plot.resize(800, 600);
    plot.show();

    // 如果有 -p 参数，截图后自动退出；否则正常交互运行
    Qwt3DExamples::setupScreenshotCapture(&plot, argc, argv, app);

    return app.exec();
}
```

### 3. 示例 CMakeLists.txt 标准模板

每个 3D 示例的 CMakeLists.txt 需添加 common 目录到 include 路径：

```cmake
cmake_minimum_required(VERSION 3.5)
SET(VERSION_SHORT 0.1)
SET(QWT_APP_NAME scatterplot)
project(${QWT_APP_NAME} VERSION ${VERSION_SHORT})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} 5.12 COMPONENTS Core Gui Widgets REQUIRED)

# 关键：添加 common 目录到 include 路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)

file(GLOB APP_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB APP_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(${QWT_APP_NAME}
    ${APP_HEADER_FILES}
    ${APP_SOURCE_FILES}
)

if(NOT TARGET qwt::plot3d)
    find_package(qwt REQUIRED)
endif()

target_link_libraries(${QWT_APP_NAME} PUBLIC qwt::plot3d)

set_target_properties(${QWT_APP_NAME} PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
    WIN32_EXECUTABLE TRUE
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
    EXPORT_NAME ${QWT_APP_NAME}
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS ${QWT_APP_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib
)
```

### 4. examples/CMakeLists.txt 注册更新

在 `examples/CMakeLists.txt` 中，`if(QWT_CONFIG_QWTPLOT_3D)` 块内注册所有新增 3D 示例：

```cmake
if(QWT_CONFIG_QWTPLOT_3D)
    # 现有示例
    add_subdirectory(3D/autoswitch)
    add_subdirectory(3D/axes)
    add_subdirectory(3D/enrichments)
    add_subdirectory(3D/figureSurface3D)
    add_subdirectory(3D/mesh2)
    add_subdirectory(3D/simpleplot3D)
    # 新增示例（plan03-12）
    add_subdirectory(3D/scatterplot)
    add_subdirectory(3D/bar3d)
    add_subdirectory(3D/lineplot3d)
    add_subdirectory(3D/vectorfield)
    add_subdirectory(3D/streamline)
    add_subdirectory(3D/contourslice)
    add_subdirectory(3D/isosurface)
    add_subdirectory(3D/volumeplot)
    add_subdirectory(3D/scatteredsurface)
    add_subdirectory(3D/multiplot)
endif()
```

注意：`examples/3D/common/` 目录仅包含 header 文件，不需要 `add_subdirectory`。

### 5. 截图脚本

可选提供一个批量截图的 PowerShell 脚本 `examples/3D/common/capture-all.ps1`：

```powershell
# capture-all.ps1 — Batch capture screenshots for all 3D examples
# Usage: .\capture-all.ps1 -OutputDir "docs/assets/screenshots-3D"
# Usage: .\capture-all.ps1 -OutputDir "docs/assets/screenshots-3D" -Width 1024 -Height 768

param(
    [string]$OutputDir = "docs/assets/screenshots-3D",
    [int]$Width = 800,
    [int]$Height = 600,
    [int]$Delay = 500
)

$examples = @(
    @{ exe = "scatterplot";    shots = @("point", "sphere") },
    @{ exe = "bar3d";          shots = @("grouped", "stacked") },
    @{ exe = "lineplot3d";     shots = @("simple", "tube") },
    @{ exe = "vectorfield";    shots = @("arrows", "lines") },
    @{ exe = "streamline";     shots = @("simple", "tube") },
    @{ exe = "contourslice";   shots = @("lines", "filled") },
    @{ exe = "isosurface";     shots = @("flat", "smooth") },
    @{ exe = "volumeplot";     shots = @("mip", "composite") },
    @{ exe = "scatteredsurface"; shots = @("delaunay", "tps") },
    @{ exe = "multiplot";      shots = @("grid", "horizontal") }
)

$binDir = "$PSScriptRoot\..\..\build\bin"

foreach ($ex in $examples) {
    $exePath = Join-Path $binDir $ex.exe
    foreach ($shot in $ex.shots) {
        $outputPath = Join-Path $OutputDir "$($ex.exe)-$shot.png"
        Write-Host "Capturing: $exePath -p `"$outputPath`" -w $Width -h $Height -d $Delay"
        & $exePath -p $outputPath -w $Width -h $Height -d $Delay
    }
}

Write-Host "All screenshots saved to: $OutputDir"
```

> 注意：此脚本为可选辅助工具。示例程序本身的 `-p` 参数是核心功能，脚本仅用于批量调用。各示例需要通过命令行参数或程序内代码控制截图时的渲染模式（如球体/点状、分组/堆叠等），脚本仅负责调用和路径管理。

---

## 验证方式

### 编译验证

```powershell
.\build.ps1 build
```

### 功能验证

1. **截图基本验证**：
   ```powershell
   .\build\bin\simpleplot3D.exe -p "test-output/simpleplot3D.png"
   ```
   验证：PNG 文件生成在指定路径，图片内容为 3D 表面图

2. **目录自动创建验证**：
   ```powershell
   .\build\bin\simpleplot3D.exe -p "test-output/nested/deep/simpleplot3D.png"
   ```
   验证：`test-output/nested/deep/` 目录自动创建

3. **自定义尺寸验证**：
   ```powershell
   .\build\bin\simpleplot3D.exe -p "test-output/simpleplot3D-1024.png" -w 1024 -h 768
   ```
   验证：截图分辨率为 1024×768

4. **延迟调整验证**：
   ```powershell
   .\build\bin\simpleplot3D.exe -p "test-output/simpleplot3D-delayed.png" -d 2000
   ```
   验证：延迟 2 秒后截图（适用于复杂渲染场景）

5. **交互模式验证**：
   ```powershell
   .\build\bin\simpleplot3D.exe
   ```
   验证：窗口正常显示，鼠标交互正常（无 `-p` 参数）

6. **退出码验证**：
   ```powershell
   .\build\bin\simpleplot3D.exe -p "test-output/simpleplot3D.png"
   echo $LASTEXITCODE
   ```
   验证：成功时退出码为 0，失败时为 1

### 批量验证

运行 `capture-all.ps1` 脚本（如果提供），验证所有 3D 示例均能生成截图。

---

## 风险与注意事项

1. **GL 初始化时序**：`QOpenGLWidget::grabFramebuffer()` 要求 GL 上下文已初始化且至少渲染了一帧。通过 `QTimer::singleShot` 延迟截图确保这一点。如果 500ms 不足以完成 GL 初始化（极端情况），可通过 `-d` 参数增大延迟。

2. **离屏渲染**：`grabFramebuffer()` 在某些平台/驱动上可能需要窗口可见。使用 `WIN32_EXECUTABLE TRUE` 的 Windows 应用在无桌面环境（如 CI）中可能无法截图。CI 环境可考虑使用虚拟显示（Xvfb）或 `offscreen` 平台插件。

3. **截图质量**：`grabFramebuffer()` 返回的 QImage 分辨率等于窗口大小（乘以设备像素比）。在高 DPI 屏幕上，截图可能比逻辑窗口尺寸大。如需固定分辨率截图，在 `captureScreenshot()` 中可添加 `image.scaled()` 后处理（但会降低质量）。

4. **已有示例兼容**：现有的 6 个 3D 示例（`simpleplot3D` 等）不强制改造为支持 `-p` 参数。本计划仅要求新增示例支持截图功能。但建议后续逐步为已有示例添加支持。

5. **header-only 设计**：`screenshot_helper.h` 使用 `inline` 函数避免多重定义问题。所有函数放在 `Qwt3DExamples` 命名空间中，避免与库代码冲突。

6. **不使用 PIMPL**：`screenshot_helper.h` 中的 `ScreenshotConfig` 是纯数据结构体，`parseScreenshotArgs` / `captureScreenshot` / `setupScreenshotCapture` 都是自由函数（非类成员），因此不需要 PIMPL（符合 README §2.2 豁免规则）。

7. **PowerShell 脚本编码**：`capture-all.ps1` 必须使用 ASCII 或 UTF-8 with BOM 编码。cmd.exe 对 UTF-8 的处理不可靠，PowerShell 脚本中避免使用非 ASCII 字符。
