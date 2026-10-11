# 极坐标板块（`src/plot/qwt_polar*`，`qwt::plot`）

CMake 选项 `QWT_CONFIG_QWTPOLAR`（默认 ON）。极坐标是 2D 库里**自成一套**的子系统：
宿主类是 `QwtPolarPlot`（`qwt_polar_plot.h`，继承 `QFrame` + `QwtPolarItemDict`），
**不是** `QwtPlot`。极坐标 item 继承 `QwtPolarItem`（`qwt_polar_item.h`），
同样 **不是 QObject**（见 `gotchas.md`）。

## 类地图

| 用途 | 类 / 头文件 |
|------|------------|
| 宿主绘图 | `QwtPolarPlot` `qwt_polar_plot.h`（scale 标识是 `QwtPolar::Scale`：`ScaleAzimuth`/`ScaleRadius`，见 `qwt_polar.h` 的 `namespace QwtPolar`） |
| item 基类 + 容器 | `QwtPolarItem`、`QwtPolarItemDict` |
| 极坐标曲线 | `QwtPolarCurve` `qwt_polar_curve.h`（数据是 `QwtPointPolar`，来自 core） |
| 极坐标网格 | `QwtPolarGrid` `qwt_polar_grid.h` |
| 极坐标标注 | `QwtPolarMarker` `qwt_polar_marker.h` |
| 极坐标频谱图 | `QwtPolarSpectrogram` `qwt_polar_spectrogram.h` |
| 交互 | `QwtPolarPicker`、`QwtPolarMagnifier`、`QwtPolarCachePanner`、`QwtPolarFitter`（Douglas-Peucker） |
| 布局/画布/导出 | `QwtPolarLayout`、`QwtPolarCanvas`、`QwtPolarRenderer` |

## 读码路径

1. **入门**：`examples/2D/polardemo/Plot.cpp` —— 同时演示 `QwtPolarCurve` +
   `QwtPolarMarker` + `QwtPolarGrid` 的标准装配方式（`attach(plot)` 与 2D 同构）。
2. **频谱图/热力**：`examples/2D/polarspectrogram`。
3. **文档**：`docs/use-guide/polar-plot.md`（中文 `docs/zh/use-guide/polar-plot.md`）。
4. **scale 细节**：`QwtPolarPlot::setScale(QwtPolar::ScaleAzimuth, min, max, step)` /
   `setScaleEngine(scaleId, ...)`——刻度引擎复用 core 的 `QwtScaleEngine` 家族。
   `qwt_polar.h` 里有个**遗留的 `namespace QwtPolar`**（只有枚举，没有类）：
   `AngleUnit`（Radians/Degrees/Gradians/Turns）、`Coordinate`、`Axis`、`Scale`。
   这与 3D 模块"彻底移除 namespace"是两回事，别去删它。

## 注意事项

- 极坐标与 2D 笛卡尔**不共用 item**：`QwtPlotCurve` 不能 attach 到 `QwtPolarPlot`，反之亦然。
  写通用代码时不要假设"plot item 接口统一"。
- `QwtPointPolar`（core，`qwt_point_polar.h`）是极坐标数据点：`QwtPointPolar(azimuth, radius)`，
  并**提供**与 `QPointF` 的互转（`toPoint()` / `setPoint(const QPointF&)`），
  工具函数 `qwtPolar2Pos(pole, radius, angle)`；角度单位由宿主轴的
  `QwtPolarPlot::setAzimuthOrigin()` 及 item 侧约定（弧度制存储），不是构造函数参数。
- 极坐标的 zoomer/panner 与 2D 的类不通用（`QwtPolarMagnifier` ≠ `QwtPlotMagnifier`）。
- `QwtPolarFitter` 是极坐标版降采样（继承 `QwtCurveFitter`），2D 侧别混用。
