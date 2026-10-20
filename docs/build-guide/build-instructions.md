# QWT Build Instructions

> **Tip**
>
> You don't have to build QWT as a dynamic library. Simply add `QwtPlot.h` and `QwtPlot.cpp` (located in the `src-amalgamate` directory) to your project.

Building QWT as a dynamic library requires a C++ build system. If you are not familiar with one, simply add `QwtPlot.h` and `QwtPlot.cpp` to your project.

## CMake Build Instructions

### Build Options

| Build Option | Default | Description |
|---------|--------|------|
| `QWT_CONFIG_QWTPLOT` | `ON` | Enable QwtPlot-related classes, which are the core components required for using QwtPlot |
| `QWT_CONFIG_QWTPOLAR` | `ON` | Enable QwtPolar classes for polar coordinate plots. Note: this option depends on `QWT_CONFIG_QWTPLOT` |
| `QWT_CONFIG_QWTWIDGETS` | `ON` | Enable widget classes other than QwtPlot (such as sliders, dials, etc.) |
| `QWT_CONFIG_QWTSVG` | `ON` | Enable SVG image display and export functionality, allowing SVG images on charts or exporting charts as SVG documents |
| `QWT_CONFIG_QWTOPENGL` | `ON` | Enable OpenGL canvas support, allowing charts to be rendered using OpenGL |
| `QWT_CONFIG_QWTPLOT_3D` | `ON` | Enable QwtPlot3D, allowing 3D chart support |
| `QWT_CONFIG_BUILD_EXAMPLE` | `ON` | Build example programs |
| `QWT_CONFIG_BUILD_PLAYGROUND` | `ON` | Build experimental code |
| `QWT_CONFIG_BUILD_STATIC_EXAMPLE` | `ON` | Build statically linked examples. Note: building static examples may require a relatively long compilation time |
| `QWT_CONFIG_BUILD_TESTS` | `OFF` | Build test programs |

> **Tip**
>
> Starting from QWT 7.1, the QwtPlot3D code has been integrated to provide unified 2D and 3D chart support. The QwtPlot3D code originates from [https://github.com/SciDAVis/qwtplot3d](https://github.com/SciDAVis/qwtplot3d).

### Basic Build Steps

1. Make sure Qt 5.12 or a later version is installed.
2. Install the `CMake` tool and confirm its path (e.g., `C:\Program Files (x86)\cmake3.27.9\bin\cmake.exe`).
3. Confirm your Qt version path and compiler. Taking Qt 5.14.2 MSVC 2017 as an example.
4. Locate the folder containing `Qt5Config.cmake` under your Qt installation path (e.g., `C:\Qt\Qt5.14.2\5.14.2\msvc2017_64\lib\cmake\Qt5`).
5. Open a command line, navigate to the `qwt` directory, and run the following command:

    ```shell
    cmake -B build -S . -G "Visual Studio 15 2017" -A x64 -DQt5_DIR="C:\Qt\Qt5.14.2\5.14.2\msvc2017_64\lib\cmake\Qt5"
    ```

    > **Tip**
    >
    > If `cmake.exe` is not in your PATH, you need to specify the full program path in the command above. For example, if CMake is installed at `C:\Program Files (x86)\cmake3.27.9\bin\cmake.exe`, the command should be written as:
    > ```shell
    > "C:\Program Files (x86)\cmake3.27.9\bin\cmake.exe" -B build -S . -G "Visual Studio 15 2017" -A x64 -DQt5_DIR="C:\Qt\Qt5.14.2\5.14.2\msvc2017_64\lib\cmake\Qt5"
    > ```

6. After completion, you can run the install command to install the library to a specified directory:

```shell
cmake --build build --target install --config Debug
cmake --build build --target install --config Release
```

> **Tip**
>
> If you are not familiar with the CMake command line, you can use tools such as `Qt Creator` or `Visual Studio` to build the project.

### Quick Build with build.ps1 (Windows)

On Windows, the `build.ps1` script automates Qt, Visual Studio, and CMake detection:

```powershell
.\build.ps1                          # Full build (Release)
.\build.ps1 build                    # Incremental build
.\build.ps1 rebuild                  # Clean + reconfigure + build
.\build.ps1 configure -Examples OFF -Playground OFF  # Library only
```

Available parameters: `-Examples`, `-Playground`, `-Tests`, `-OpenGL`, `-Plot3D` (all accept `ON`/`OFF`).

### Using Qt 6

For Qt 6, use `CMAKE_PREFIX_PATH` and select the appropriate compiler:

```shell
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.7.3/msvc2019_64"
cmake --build build --config Release
```

### Ninja Generator

If you prefer Ninja, initialize the MSVC environment first (`INCLUDE`, `LIB`, `PATH`):

```shell
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.7.3/msvc2019_64"
cmake --build build
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| CMake cannot find Qt | Set `-DCMAKE_PREFIX_PATH` or `-DQt5_DIR` / `-DQt6_DIR` to the correct path |
| Wrong Qt version detected | Delete `build/CMakeCache.txt` and `build/CMakeFiles/`, then reconfigure |
| MSVC environment not initialized | Use `build.ps1` or run from a "Developer Command Prompt for VS" |
| Ninja build fails with cl.exe not found | Run `vcvars64.bat` before invoking CMake |
