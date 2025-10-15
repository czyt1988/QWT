# QWT构建说明

!!! tips "提示"
    你不需要编译QWT，只需把`QwtPlot.h`和`QwtPlot.cpp`（这两个文件位于`src-amalgamate`目录下）引入你的工程即可

此文会详细介绍如何构建QWT为**动态库**，如果你不熟悉C++的构建，你只需把`QwtPlot.h`和`QwtPlot.cpp`引入你的工程即可使用

## CMake构建说明

### 编译选项说明

| 编译选项 | 默认值 | 说明 |
|---------|--------|------|
| `QWT_CONFIG_QWTPLOT` | `ON` | 启用 QwtPlot 相关类，这些类是使用 QwtPlot 所需的核心组件 |
| `QWT_CONFIG_QWTPOLAR` | `ON` | 启用 QwtPolar 类，这些类用于使用极坐标图。注意：此选项依赖于 `QWT_CONFIG_QWTPLOT` |
| `QWT_CONFIG_QWTWIDGETS` | `ON` | 启用除 QwtPlot 外的其他控件（如滑块、刻度盘等）相关类 |
| `QWT_CONFIG_QWTSVG` | `ON` | 启用 SVG 图像显示和导出功能，允许在图表上显示 SVG 图像或将图表导出为 SVG 文档 |
| `QWT_CONFIG_QWTOPENGL` | `ON` | 启用 OpenGL 画布支持，允许使用 OpenGL 渲染图表 |
| `QWT_CONFIG_BUILD_EXAMPLE` | `ON` | 构建示例程序 |
| `QWT_CONFIG_BUILD_PLAYGROUND` | `ON` | 构建实验性代码 |
| `QWT_CONFIG_BUILD_STATIC_EXAMPLE` | `OFF` | 构建静态链接示例。注意：构建静态示例会需要相对较长的编译时间 |

### 基本构建步骤

1. 确保已安装 Qt 5.12 或更高版本
2. 安装`CMake`工具，并确认其路径（例如：`C:\Program Files (x86)\cmake3.27.9\bin\cmake.exe`）。
3. 确认你的Qt版本路径和编译器。以Qt5.14.2 MSVC 2017版本为例。
4. 找到Qt安装路径下`Qt5Config.cmake`所在的文件夹（例如：`C:\Qt\Qt5.14.2\5.14.2\msvc2017_64\lib\cmake\Qt5`）。
5. 打开命令行，切换到`qwt`所在目录，执行以下命令：
    
    ```shell
    cmake -B build -S . -G "Visual Studio 15 2017" -A x64 -DQt5_DIR="C:\Qt\Qt5.14.2\5.14.2\msvc2017_64\lib\cmake\Qt5"
    ```
 
    !!! tips "提示"
        如果`cmake.exe`不在环境变量中，上例子需指定完整的程序路径。例如cmake安装路径为`C:\Program Files (x86)\cmake3.27.9\bin\cmake.exe`,那 么上例子应该写为：
        ```shell
        "C:\Program Files (x86)\cmake3.27.9\bin\cmake.exe" -B build -S . -G "Visual Studio 15 2017" -A x64 -DQt5_DIR="C:\Qt\Qt5.14. 2\5.14.2\msvc2017_64\lib\cmake\Qt5"
        ```

6. 执行完成后，可以执行安装命令，把库安装到指定目录：

```shell
cmake --build build --target install --config Debug
cmake --build build --target install --config Release
```

!!! tips "提示"
    如果你对cmake命令行不熟悉，可以使用`Qt Creator`或`Visual Studio`等工具进行构建。
