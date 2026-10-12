# 一键更新文档截图

本项目文档（`docs/assets/screenshots/`、`docs/assets/screenshots-3D/`）中的例子截图
可以通过脚本一键重新生成，无需手动截图。

## 快速开始

```powershell
# 1) 先构建例子（任意方式，例如）
.\build.ps1

# 2) 一键重新生成全部截图（写入 docs/assets/screenshots{,-3D}）
powershell -NoProfile -ExecutionPolicy Bypass -File examples\update-screenshots.ps1

# 或者通过 CMake target（Windows）：
cmake --build build --target update-screenshots
```

常用参数：

```powershell
# 只更新某个/某些例子（通配符匹配例子目标名）
... update-screenshots.ps1 -Only spectrogram
... update-screenshots.ps1 -Only "*3D*"

# 指定构建目录 / 先自动构建缺失的目标
... update-screenshots.ps1 -BuildDir D:\qwt\build -Build
```

注意事项：

- 截图时窗口会短暂弹出，运行期间不要移动鼠标或切换窗口。
- 脚本强制 `QT_SCREEN_SCALE_FACTORS=1`，保证输出 PNG 尺寸与 `--size` 一致，
  与桌面缩放无关。
- 失败的条目不会破坏旧截图（先写临时文件，成功后才替换）。
- GIF 动图、playground 例子的截图不在本脚本范围内，原因见
  `screenshots-manifest.json` 的 `skipped` 段。

## 工作原理

### 1. 统一的截图协议（`common/qwt_example_screenshot.h`）

所有例子共享一个 header-only 助手，实现统一命令行协议：

```
example --screenshot <file.png> [--screenshot-delay <ms>] [--size <WxH>] [--variant <name>]
```

| 参数 | 作用 |
|------|------|
| `--screenshot <file>` | 延时后 grab 主窗口、保存 PNG 并退出（成功 0 / 失败 2） |
| `--screenshot-delay <ms>` | 截图前等待 UI 稳定的时间，默认 1500ms（动画/3D 例子加大） |
| `--size <WxH>` | 截图前调整窗口大小，保证输出尺寸可复现 |
| `--variant <name>` | 选择例子的视觉变体（某个 tab、某种绘图样式等） |

不带 `--screenshot` 时所有函数都是 no-op，例子照常运行。

### 2. 例子集成（每个 main.cpp 一行）

```cpp
// 2D/3D/bench 例子（相对路径按例子层级调整，staticExample 用 ../common/）
#include "../../common/qwt_example_screenshot.h"

int main( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    MainWindow window;
    window.show();

    QwtExampleScreenshot::start( &window );   // 无参数时无副作用
    return app.exec();
}
```

多变体的例子在 main.cpp 中注册变体回调（在 `start()` 之前）：

```cpp
QwtExampleScreenshot::addVariant( "grouped", [typeBox]() { typeBox->setCurrentIndex( 0 ); } );
QwtExampleScreenshot::addVariant( "stacked", [typeBox]() { typeBox->setCurrentIndex( 1 ); } );
```

已注册变体的例子：

| 例子 | 变体 |
|------|------|
| `controls` | `sliders` / `wheel-thermo` / `knob` / `dial`（4 个 tab） |
| `dials` | `compass` / `cockpit` |
| `BarChart` | `grouped` / `stacked` |
| `friedberg` | `bar` / `tube` |
| `rasterview` | `nearest` / `bilinear` / `bicubic`（重采样模式） |
| `spectrogram` | `contour`（勾选等高线） |
| `waterfall3D` | 不用 variant，直接用已有的 `--style N` / `--capacity N` 参数 |

特例：`colormap` 例子保留自己的 headless 批量导出模式
（`colormap --screenshot <目录>`，一次写出 22 张预设图 + overview.png），
脚本通过 manifest 的 `raw` 条目调用它。

### 3. 清单与驱动脚本

- `screenshots-manifest.json`：每张截图一条记录 —— 例子目标名、输出路径
  （相对 `docs/assets`）、窗口尺寸、延时、变体、额外参数；`skipped` 段记录
  无法自动生成的截图及原因。
- `update-screenshots.ps1`：读清单 → 定位 `build/bin` 下的可执行文件（自动兼容
  Debug 的 `d` 后缀与 VS 多配置目录）→ 从 `CMakeCache.txt` 探测 Qt bin 目录加入
  PATH → 逐个运行、超时强杀 → 汇总 OK/FAIL/MISSING，有失败时退出码非 0。

## 如何新增一张截图

1. 若例子还未集成：在其 `main.cpp` 中 include 助手并调用
   `QwtExampleScreenshot::start( &window )`；需要变体则 `addVariant(...)`。
2. 在 `screenshots-manifest.json` 的对应例子下加一条 shot
   （`output`、`size`、可选 `delay`/`variant`/`args`）。
3. 运行 `update-screenshots.ps1 -Only <例子名>` 验证，然后提交新截图。
