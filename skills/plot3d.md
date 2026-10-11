# 3D 绘图板块（`src/plot3d/`，`qwt::plot3d`）

CMake 选项 `QWT_CONFIG_QWTPLOT_3D`（默认 ON）。依赖 `qwt::core`，**不依赖 2D**。

> ⚠ **动手前必须先读 `src/plot3d/AGENTS.md`**。那是本模块的局部宪法：分层依赖规则
> （Tier 0~5，低层禁止反向持有高层指针）、值对象静默契约、禁止 legacy OpenGL
> （`glBegin/glEnd`、display list 一律不用，全部 VBO/VAO + GLSL 3.3 Core）。
> 本文件只回答"要做什么去读哪"。

## 架构：Plot（窗口）+ Item（数据/绘制）

- **`Qwt3DPlot`（`qwt3d_plot.h`，`QOpenGLWidget` 子类）**：管 GL 上下文、视图变换、光照、
  坐标系统、item 列表、主题。**不持绘图数据**。API：`attach(Qwt3DPlotItem*)` /
  `detach()` / `itemList()` / `applyTheme(Qwt3DTheme::Preset 或名字)`。
- **`Qwt3DPlotItem`（`qwt3d_plotitem.h`）**：item 基类（非 widget，不是 QObject），
  纯虚 `draw()` + `hull()`，`attach(plot)` 挂载。
- **三个现成 item**：
  | item | 头文件 | 说明 |
  |------|--------|------|
  | 曲面 | `qwt3d_surface.h` | `loadFromData(double**, cols, rows, minX, maxX, minY, maxY)` 等 5 个重载；VBO/VAO + shader；颜色 functor |
  | 柱状 | `qwt3d_bar.h` | 逐柱 cuboid，Filled / FilledMesh / Wireframe |
  | 折线 | `qwt3d_line3d.h` | Lines / Dots / Tube（沿路径扫掠圆柱） |

## 数据从哪来

- `Qwt3DFunction`（`qwt3d_function.h`）/ `Qwt3DParametricSurface`（`qwt3d_parametricsurface.h`）：
  `create()` **返回数据**（`Qwt3DFunctionData` / `Qwt3DParametricData`），不碰 item。
  把返回值喂给 `surface->loadFromData()` 即可——这是 v7.3.x 后的正确姿势，
  旧 API 让函数对象直接持有 surface 的写法已移除。
- 大量网格数据也可由 core 的 `QwtGridData` / `QwtMatrixRasterData` 产生后转换。

## 主题与配色

- `Qwt3DTheme`（`qwt3d_theme.h`）：10 个预设（`Default/Dark/Scientific/Warm/Cool/
  Matplotlib/EarthTones/Ocean/HighContrast/Presentation`）+ 5 个光照预设
  （`NoLighting/FlatLight/Studio/Outdoor/Soft`）。`plot->applyTheme(Qwt3DTheme::Dark)` 最省事。
- `Qwt3DColorMapColor`（`qwt3d_colormap_color.h`）：适配器，把 core 的
  `QwtColorMap`（viridis/plasma/jet 等 22 种科学预设）接到 3D 表面的颜色 functor 接口。
- ⚠ 颜色 functor（`Qwt3DColor` 家族）是**纯值对象，静默**：就地改了它之后
  必须调 `surface->invalidateColors()` 才会重算顶点色（契约见 `qwt3d_surface.h` 注释）。

## 交互、坐标、导出

- 鼠标/键盘交互：`qwt3d_mousekeyboard.h`、`qwt3d_movements.h`。
- 坐标轴/坐标系统：`qwt3d_axis.h`、`qwt3d_coordsys.h`；色标 `qwt3d_colorlegend.h`；
  文字 `qwt3d_label.h`（`Qwt3DEnrichment` 目前是 stub，勿依赖）。
- 矢量导出（PDF/PS/SVG/TEX）：`qwt3d_io_gl2ps.h`（内置 gl2ps 回退，见 `src/plot3d/3rdparty/`）；
  文件读写 `qwt3d_io.h` / `qwt3d_io_reader.h`。
- 着色器源码在 `src/plot3d/shaders/`，新增绘制风格先看那里。

## 示例（都在 `examples/3D/`）

| 示例 | 看什么 |
|------|--------|
| `qwtplot3d` | 曲面 item 的完整装配：loadFromData + attach + theme + 设置面板 |
| `bar3D` | 柱状 item + 逐柱属性设置 |
| `line3D` | 折线 item（Lines/Dots/Tube 切换） |
| `figureSurface3D` | 动态刷新曲面（数据更新 → 重画） |

文档：`docs/use-guide/3d-plot.md`、`3d-bar-chart.md`、`3d-line-plot.md`
（中文在 `docs/zh/use-guide/`）。

**最小骨架**（与 2D 同构：plot 是宿主，item 才是数据）：

```cpp
Qwt3DPlot* plot = new Qwt3DPlot;
Qwt3DSurface* surface = new Qwt3DSurface();
surface->loadFromData(data, cols, rows, minX, maxX, minY, maxY);
surface->attach(plot);          // plot 不持有数据；数据在 item 里
plot->applyTheme(Qwt3DTheme::Scientific);
```
