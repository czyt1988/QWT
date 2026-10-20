# 区间曲线 - QwtPlotIntervalCurve

`QwtPlotIntervalCurve` 用于绘制带有上下限的区间曲线，常用于显示误差范围、置信区间、波动范围等数据。每个数据点包含一个位置值和一个区间范围。

## 主要功能特性

- 区间表示：在曲线周围显示上下边界
- 多种样式：支持曲线、填充区域、符号等显示方式
- 误差棒显示：可绘制垂直或水平的误差棒线条
- 样式自定义：区间边界和填充可独立配置

## 基本概念

### 区间曲线组成

下图给出 `Tube` 区间曲线的组成结构：每个区间的上下限连接成两条边界曲线，两者之间的区域被填充：

```text
     上限
       ───────
      /       \
     / 填充区域 \    ← 由 brush() 填充的区域
    /          \
   ────────────
     下限
     
边界曲线由 pen() 描边，两者之间的区域由 brush() 填充
```

### 数据结构

使用 `QwtIntervalSample` 表示区间数据，即一个轴上的位置值搭配另一个轴上的区间：

```cpp
class QwtIntervalSample {
public:
    QwtIntervalSample();
    QwtIntervalSample(double value, const QwtInterval& interval);
    QwtIntervalSample(double value, double min, double max);

    double value;           // 位置值（曲线位置）
    QwtInterval interval;   // 区间范围 [min, max]
};
```

### 曲线样式

| 样式 | 枚举值 | 说明 |
|------|--------|------|
| 无曲线 | `NoCurve` | 不绘制曲线；符号（如误差棒）仍会显示 |
| 管状 | `Tube` | 用 `pen()` 把上下限绘制成两条边界曲线，并用 `brush()` 填充其间区域 |
| 用户曲线 | `UserCurve` | 保留给重写了 `drawSeries()` 的派生类 |

## 使用方法

### 1. 基本区间曲线

`QwtPlotIntervalCurve` 接收 `QwtIntervalSample` 数据，每个样本由位置值和 `QwtInterval` 区间组成；其 `CurveStyle`（`NoCurve` / `Tube`，默认 `Tube`）决定区间的渲染方式。`Tube` 样式下 `setPen()` 绘制边界曲线，`setBrush()` 填充两者之间的区域。下面的例子创建绘图并把四个样本绘制成半透明带状区域：

```cpp
#include <QwtPlot>
#include <QwtPlotIntervalCurve>
#include <QwtInterval>

QwtPlot* plot = new QwtPlot();
plot->setTitle("区间曲线示例");
plot->setCanvasBackground(Qt::white);

// 创建区间曲线
QwtPlotIntervalCurve* curve = new QwtPlotIntervalCurve("误差范围");

// 设置样式
curve->setStyle(QwtPlotIntervalCurve::Tube);

// 设置上下边界曲线的画笔
curve->setPen(QPen(Qt::blue, 2));

// 设置区间填充样式
curve->setBrush(QBrush(QColor(100, 150, 200, 100)));

// 准备数据
QVector<QwtIntervalSample> samples;
samples << QwtIntervalSample(5.0, QwtInterval(4.0, 6.0));
samples << QwtIntervalSample(8.0, QwtInterval(6.5, 9.5));
samples << QwtIntervalSample(10.0, QwtInterval(8.0, 12.0));
samples << QwtIntervalSample(12.0, QwtInterval(10.0, 14.0));

curve->setSamples(samples);
curve->attach(plot);
plot->replot();
```

### 2. 误差棒样式

`CurveStyle` 中没有专门的误差棒取值；误差棒通过把曲线样式设为 `NoCurve`、再用 `setSymbol()` 挂载 `QwtIntervalSymbol` 实现：符号的 `Style` 选择端帽形状（`Bar` 绘制带端帽的线段），符号自身的 `setPen()` 决定线条样式，`setWidth()` 决定端帽宽度。`setSymbol()` 会接管指针的所有权。下面的代码只绘制误差棒：

```cpp
// 只绘制逐区间的符号，不绘制管状区域
curve->setStyle(QwtPlotIntervalCurve::NoCurve);

// 误差棒符号：带端帽的线段
QwtIntervalSymbol* symbol = new QwtIntervalSymbol(QwtIntervalSymbol::Bar);
symbol->setPen(QPen(Qt::black, 1));
symbol->setWidth(5);  // 误差棒端帽宽度（像素）

// 符号所有权转移给曲线
curve->setSymbol(symbol);
```

### 3. Tube样式配置

`Tube` 样式下，上下边界用项目的 `setPen()` 描边，边界之间的区域由 `setBrush()` 填充（默认 `Qt::NoBrush` 即不填充）。下面的代码为管状区域配置红色描边与半透明粉色填充：

```cpp
curve->setStyle(QwtPlotIntervalCurve::Tube);

// 边界曲线样式（上、下边界共用这支画笔）
curve->setPen(QPen(Qt::red, 2));

// 边界之间区域的填充样式
curve->setBrush(QBrush(QColor(255, 200, 200, 150)));
```

### 4. 符号配置

`QwtIntervalSymbol` 构造时接受 `Style`（`Bar` 为带端帽的线段，`Box` 为矩形），可用 `setPen()`、`setBrush()` 和 `setWidth()`（线宽/端帽宽度，单位像素）配置，再通过曲线的 `setSymbol()` 挂载（所有权转移给曲线）。下面的例子创建一个宽 8 像素的蓝色 `Bar` 符号：

```cpp
#include <QwtIntervalSymbol>

// 创建区间符号
QwtIntervalSymbol* symbol = new QwtIntervalSymbol(QwtIntervalSymbol::Bar);

// 设置样式
symbol->setPen(QPen(Qt::blue, 1));
symbol->setWidth(8);  // 符号宽度（像素）

curve->setSymbol(symbol);  // 所有权转移给曲线

// 符号类型：
// - NoSymbol: 不绘制符号
// - Bar: 带端帽的线段（标准误差棒）
// - Box: 用 pen()/brush() 绘制的矩形
// - UserSymbol: 保留给 QwtIntervalSymbol 的派生类
```

## 核心方法总结

| 方法 | 说明 |
|------|------|
| `setSamples()` | 设置区间数据（`QVector<QwtIntervalSample>` 或 `QwtSeriesData<QwtIntervalSample>*`） |
| `setStyle()` | 设置曲线样式（`NoCurve` / `Tube`） |
| `setPen()` | 设置边界曲线画笔（`Tube` 样式） |
| `setBrush()` | 设置管状区域填充画刷 |
| `setSymbol()` | 设置逐样本绘制的区间符号（所有权转移） |
| `setPaintAttribute()` | 开关裁剪优化（`ClipPolygons` / `ClipSymbol`） |

> **提示 — 应用场景**
>
> - 实验数据误差显示
> - 置信区间可视化
> - 波动范围展示
> - 预测区间显示

> **示例 — 相关示例**
>
> - 区间曲线演示：参见自定义示例