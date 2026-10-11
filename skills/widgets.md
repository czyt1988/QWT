# 控件板块（`src/plot/` 里的 widget 家族，`qwt::plot`）

CMake 选项 `QWT_CONFIG_QWTWIDGETS`（默认 ON）控制。全部继承自 QWidget /
QwtAbstractSlider / QwtAbstractScale，**是正常 QObject，有信号槽**（与 `QwtPlotItem` 相反，
见 `gotchas.md`）。

## 控件速查

| 控件 | 头文件 | 基类 | 示例 |
|------|--------|------|------|
| 仪表盘 | `qwt_dial.h` | `QwtAbstractSlider` | `examples/2D/dials`、`controls` |
| 旋钮 | `qwt_knob.h` | `QwtAbstractSlider` | `controls`、`oscilloscope`（`Knob.cpp`） |
| 滑块 | `qwt_slider.h` | `QwtAbstractSlider` | `controls`、`radio` |
| 滚轮 | `qwt_wheel.h` | `QWidget` | `controls`、`radio` |
| 温度计 | `qwt_thermo.h` | `QwtAbstractScale` | `sysinfo`、`controls` |
| 计数器/步进器 | `qwt_counter.h` | `QWidget` | —（头文件 usage 示例） |
| 指南针 | `qwt_compass.h`（+`qwt_compass_rose.h` 玫瑰线） | `QwtDial` | — |
| 模拟时钟 | `qwt_analog_clock.h` | `QwtDial` | — |
| 箭头按钮 | `qwt_arrow_button.h` | `QWidget` | — |
| 纯文字标签控件 | `qwt_text_label.h` | `QWidget` | — |

## 关键知识（改样式前必读）

- **刻度盘/旋钮的"盘面"和"指针"是分开的**：指针是 `QwtDialNeedle`（`qwt_dial_needle.h`），
  经 `QwtDial::setNeedle()` 安装；刻度环是 `QwtRoundScaleDraw`（`qwt_round_scale_draw.h`），
  经 `setScaleDraw()` 安装。想要"半圆仪表""异形指针"就换这两个对象，
  不要去改 `QwtDial::drawScaleContents` 之类的虚函数。
- **弧形/角度 API 在 `QwtDial` 上**：`setScaleArc(minArc, maxArc)`（角度制，控制盘面张角）、
  `setOrigin`、`setMode`（RotateNeedle 等枚举决定指针旋转还是盘面旋转）；
  步进/范围/朝向由 `QwtAbstractSlider`（`setWrapping`/`setTotalSteps`/…）统一定义，
  子类只管 `drawHandle`/`drawScale`。
- **配色方案**：`QwtDial::setFrameShadow`、调色板 + needle 颜色 API 组合出风格；
  示例 `examples/2D/dials` 演示了多种盘面组合，是样式定制的最佳范本。
- 文档：`docs/use-guide/widgets-controls.md`（中文 `docs/zh/use-guide/widgets-controls.md`）。
- 控件常与 `QwtSamplingThread`（`qwt_sampling_thread.h`）配合做实时采集，
  参见 `examples/2D/oscilloscope`、`cpuplot`。

## 常见任务路由

- "把旋钮改成只有 270 度" → `QwtDial::setScaleArc()`（`src/plot/qwt_dial.h`）+ `examples/2D/dials`。
- "温度计要竖着放/改填充渐变色" → `src/plot/qwt_thermo.h`（`setOrientation`、
  `setFillBrush`、`setAlarmLevel` 等）。
- "给控件换皮肤" → `src/plot/qwt_stylesheet_recorder.h`（`QwtStyleSheetRecorder`）+
  `examples/2D/stylesheets`（支持 stylesheet 换肤的录制/回放）。
