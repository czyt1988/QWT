# core 基础库（`src/core/`，`qwt::core`）

不依赖 `qwt::plot` 和 `qwt::plot3d`，可单独使用（例如做数据处理/算法，不画图）。
所有类导出宏 `QWTCORE_EXPORT`。前缀头文件统一 `qwt_*.h`。

## 按功能找文件

| 想做什么 | 读哪个文件 |
|---------|-----------|
| 取颜色映射（viridis/plasma/jet 等 22 种预设）、自定义 colormap | `qwt_colormap.h`、`qwt_colormap_preset.h` |
| 曲线/柱子的配色循环、主题色板 | `qwt_color_cycle.h` |
| 数学常量、`qwtMinF/qwtMaxF`、`qwtFuzzyCompare` 等工具 | `qwt_math.h` |
| 区间类型（刻度范围、数据区间裁剪） | `qwt_interval.h` |
| 喂数据给绘图项：`QwtSeriesData<T>` 及实现（`QwtPointArrayData` 等） | `qwt_series_data.h`、`qwt_point_data.h` |
| 通用数据存储模板 `QwtSeriesStore<T>` | `qwt_series_store.h` |
| 栅格/矩阵/网格数据（频谱图、3D 曲面的数据底座） | `qwt_raster_data.h`、`qwt_matrix_raster_data.h`、`qwt_grid_raster_data.h` |
| 通用网格容器 `QwtGridData`（头文件-only，.hpp） | `qwt_grid_data.hpp` |
| 坐标变换：线性/对数/幂/自定义 | `qwt_transform.h` |
| 数值 ↔ 设备坐标映射 | `qwt_scale_map.h` |
| 刻度划分：主/次刻度、刻度列表 | `qwt_scale_div.h` |
| 刻度引擎：自动选"好看"的刻度步长 | `qwt_scale_engine.h` |
| 日期时间解析/换算（`QwtDate`，纯值工具） | `qwt_date.h` |
| 贝塞尔曲线求值/分割 | `qwt_bezier.h` |
| 多边形裁剪 | `qwt_clipper.h` |
| 三次点、极坐标点 | `qwt_point_3d.h`、`qwt_point_polar.h` |
| 样本数据结构（曲线点、箱线图样本） | `qwt_samples.h` |
| 箱线图统计量计算 | `qwt_box_statistics.h` |
| 高精度计时（实时绘图节流） | `qwt_system_clock.h` |
| SIMD 加速的 argmin/argmax | `qwt_simd_argminmax.h` |
| 通用迭代器算法 `qwtSelectNextIterator` 等（.hpp） | `qwt_algorithm.hpp` |
| Qt5/Qt6 兼容：鼠标位置、滚轮 delta、字体宽度 | `qwt_qt5qt6_compat.hpp` |

## 典型任务的读码路径

- **给曲线灌数据** → `src/core/qwt_series_data.h`（接口）+ `src/core/qwt_point_data.h`（现成实现，
  如 `QwtPointArrayData`）；示例 `examples/2D/curvedemo`、文档 `docs/use-guide/series-data.md`。
- **改 colormap / 让频谱图用 plasma** → `src/core/qwt_colormap_preset.h`（`QwtColorMapPreset` 预设名表）；
  用法示例 `examples/2D/spectrogram`（2D）与 `src/plot3d/qwt3d_colormap_color.h`（3D 适配器）。
- **对数轴刻度不好看/想自动刻度** → `src/core/qwt_scale_engine.h`（`QwtScaleEngine` 及
  Linear/Log 子类，core 里没有日期引擎）；对照组示例 `examples/2D/bode`（对数轴）。
  日期时间轴在 2D 侧：`src/plot/qwt_date_scale_engine.h` + `qwt_date_scale_draw.h`，
  示例 `playground/timescale`、文档 `docs/use-guide/scale-engine.md`。
- **Qt5/Qt6 兼容 bug（鼠标坐标偏、滚轮方向反）** → 一律改用
  `qwt::compat::eventPos()` / `qwt::compat::wheelEventDelta()`，见 `qwt_qt5qt6_compat.hpp`。
- **曲线平滑/降采样算法** → 平滑在 2D 侧（`src/plot/qwt_curve_smoothing.h`，
  示例 `examples/2D/curvesmoothing`，文档 `docs/use-guide/curve-smoothing.md`）；
  相关深度文章在 `docs/assets/algorithms/`。

## 注意事项

- core 里几乎**没有 QObject 派生类**：`QwtColorMap`、`QwtInterval`、`QwtScaleDiv` 等都是纯值对象，
  没有信号槽；改完值由调用方（plot item）负责触发刷新。详见 `gotchas.md`。
- 头文件分两种：`.h`（声明/实现分离）和 `.hpp`（模板/头文件-only，如 `qwt_grid_data.hpp`、
  `qwt_algorithm.hpp`）。模板 `QwtSeriesData<T>` 在 `.h` 里但实现也在头部/`src/core` 有显式实例化，
  自定义数据源时照抄 `qwt_point_data.h` 的写法。
