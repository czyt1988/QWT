# 区间曲线 - QwtPlotIntervalCurve

`QwtPlotIntervalCurve` 用于绘制带有上下限的区间曲线，常用于显示误差范围、置信区间、波动范围等数据。每个数据点包含一个位置值和一个区间范围。

## 主要功能特性

**特性**

- ✅ **区间表示**：在曲线周围显示上下边界
- ✅ **多种样式**：支持曲线、填充区域、符号等显示方式
- ✅ **误差棒显示**：可绘制垂直或水平的误差棒线条
- ✅ **样式自定义**：区间边界和填充可独立配置

## 基本概念

### 区间曲线组成

```text
     上限
       ───────
      /      \
     /  曲线  \     ← 中值曲线
    /          \
   ────────────
     下限
     
区间曲线展示：中值曲线 + 上/下边界
```

### 数据结构

使用 `QwtIntervalSample` 表示区间数据：

```cpp
struct QwtIntervalSample {
    double value;      // 中值（曲线位置）
    QwtInterval interval; // 区间范围 [min, max]
};
```

### 曲线样式

| 样式 | 枚举值 | 说明 |
|------|--------|------|
| 无曲线 | `NoCurve` | 仅显示区间填充或符号 |
| 曲线 | `Tube` | 绘制区间填充区域 |
| 带误差棒 | `ErrorBars` | 绘制误差棒线条 |

## 使用方法

### 1. 基本区间曲线

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

// 设置中值曲线样式
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

```cpp
curve->setStyle(QwtPlotIntervalCurve::ErrorBars);

// 设置误差棒线条样式
curve->setPen(QPen(Qt::black, 1));

// 设置误差棒符号（可选）
QwtIntervalSymbol* symbol = new QwtIntervalSymbol(QwtIntervalSymbol::Bar);
symbol->setPen(QPen(Qt::black, 1));
curve->setSymbol(symbol);

// 设置误差棒宽度
curve->setWidth(5);  // 误差棒横线宽度（像素）
```

### 3. Tube样式配置

```cpp
curve->setStyle(QwtPlotIntervalCurve::Tube);

// 中值曲线样式
curve->setPen(QPen(Qt::red, 2));

// 填充样式
curve->setBrush(QBrush(QColor(255, 200, 200, 150)));

// 边界线样式
curve->setOutlinePen(QPen(Qt::darkRed, 1));
curve->setOutlineMode(QwtPlotIntervalCurve::OutlineBoth);  // 绘制上下边界
```

### 4. 符号配置

```cpp
#include <QwtIntervalSymbol>

// 创建区间符号
QwtIntervalSymbol* symbol = new QwtIntervalSymbol(QwtIntervalSymbol::Bar);

// 设置样式
symbol->setPen(QPen(Qt::blue, 1));
symbol->setWidth(8);  // 符号宽度（像素）

curve->setSymbol(symbol);

// 符号类型：
// - Bar: 水平横线（标准误差棒）
// - Box: 矩形框
// - Diamond: 菱形
```

## 核心方法总结

| 方法 | 说明 |
|------|------|
| `setSamples()` | 设置区间数据 |
| `setStyle()` | 设置曲线样式 |
| `setPen()` | 设置中值曲线画笔 |
| `setBrush()` | 设置区间填充画笔 |
| `setSymbol()` |设置区间符号 |
| `setOutlinePen()` | 设置边界线画笔 |
| `setOutlineMode()` | 设置边界显示模式 |

> **提示 — 应用场景**
>
> - 实验数据误差显示
> - 置信区间可视化
> - 波动范围展示
> - 预测区间显示

> **示例 — 相关示例**
>
> - 区间曲线演示：参见自定义示例