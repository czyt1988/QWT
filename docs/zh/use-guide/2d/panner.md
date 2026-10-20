# 平移工具

> **说明 — Qwt 7 中的平移**
>
> `QwtPlotPanner` 已重构为实时交互，平移时移动的是绘图内容本身，而不再使用原版 `QwtPlotPanner` 的缓存位图。

`Qwt6` 的 `QwtPlotPanner` 基于缓存的平移机制，主要有两个问题：

- 平移过程中显示的是缓存的位图，而不是当前的绘图内容
- 平移过程中看不到数据的实时变化，交互式查看受到限制

`Qwt6` 的 `QwtPlotPanner` 通过缓存减少了绘图刷新，但缓存位图让平移看起来是静态的。因此 `Qwt7` 重构了 `QwtPlotPanner`，让绘图在鼠标移动时同步更新。

## 重构后的 QwtPlotPanner

新的 `QwtPlotPanner` 使用 `QwtPicker` 状态机实现平移和缩放，鼠标移动时绘图同步更新：

下面的动图展示了平移效果：

![qwt-realtime-panner](../../../assets/screenshots/qwt-realtime-panner.gif)

新的 `QwtPlotPanner` 支持以下坐标轴类型：

- 线性坐标轴（Linear Scale）
- 对数坐标轴（Logarithmic Scale）
- 日期时间坐标轴（DateTime Scale）
- 多坐标轴

> **警告 — 使用原来的缓存式选择器**
>
> 原来的缓存式平移器是 `QwtPlotCachePanner`，仍可使用。

| 类变更情况                  | 属性 | 基类 |
|---------------------------|------|-------|
|QwtPanner -> QwtCachePanner| 更名  |QWidget|
|原QwtPlotPanner -> QwtPlotCachePanner| 更名  |QwtCachePanner|
|原QwtPolarPanner -> QwtPolarCachePanner| 更名  |QwtCachePanner|
|QwtPlotPanner| 重构  |QwtPicker|

`Qwt6` 的 `QwtPlotPanner` 经过了重构，但接口保持不变，原有代码无需修改。

## 基本使用方法

下面的代码在绘图画布上创建 `QwtPlotPanner`，将平移绑定到鼠标中键，并连接 `panned` 信号以输出每次拖动的偏移量。

```cpp
#include "qwt_plot_panner.h"
#include "qwt_plot.h"
#include "qwt_plot_curve.h"

// 创建绘图和曲线
QwtPlot* plot = new QwtPlot;
QwtPlotCurve* curve = new QwtPlotCurve("Sine Wave");
curve->attach(plot);

// 创建实时panner
QwtPlotPanner* panner = new QwtPlotPanner(plot->canvas());
// 配置鼠标按钮
panner->setMouseButton(Qt::MiddleButton);  // 中键平移
// panner->setMouseButton(Qt::LeftButton); // 或者左键平移

// 启用选择器
panner->setEnabled(true);

// 连接信号（可选）
connect(panner, &QwtPlotPanner::panned, 
        [](int dx, int dy) {
            qDebug() << "Canvas panned by:" << dx << dy;
        });
```

可以设置平移方向，把移动限制在一个方向上：

```cpp
// 1. 设置平移方向
panner->setOrientations(Qt::Horizontal | Qt::Vertical); // 默认：水平和垂直
// panner->setOrientations(Qt::Horizontal);            // 仅水平
// panner->setOrientations(Qt::Vertical);              // 仅垂直
```

> **说明 — 坐标轴绑定**
>
> `QwtPlotPanner` 和 `QwtPlotPicker` 不同，它不需要绑定坐标轴，因为移动作用于整个画布。画布移动时，所有坐标轴一起移动。