# 股票K线图 - QwtPlotTradingCurve

`QwtPlotTradingCurve` 是专门用于绘制股票/金融K线图（蜡烛图）的绘图项类。它支持显示开盘价、收盘价、最高价、最低价等交易数据，常用于金融数据可视化和技术分析。

## 主要功能特性

**特性**

- ✅ **标准K线样式**：支持蜡烛图、美国线等多种样式
- ✅ **涨跌颜色区分**：自动根据开盘/收盘价显示涨跌颜色
- ✅ **自动缩放范围**：根据数据自动计算合适显示范围
- ✅ **符号样式自定义**：可自定义涨跌柱体的样式

## 基本概念

### K线图组成

每根K线包含四个价格数据：

```text
          最高价
            │
     ┌──────┴──────┐
     │             │
     │    ┌────┐   │  ← 箱体（开盘到收盘）
     │    │    │   │
     │    └────┘   │
     │             │
     └─────────────┘
            │
          最低价
```

### K线样式

| 样式 | 枚举值 | 说明 |
|------|--------|------|
| 蜡烛图 | `CandleStick` | 显示完整的箱体和上下影线 |
| 美国线 | `Bar` | 仅显示线条，不绘制箱体 |

### 数据结构

使用 `QwtOHLCSample`（声明于 `qwt_samples.h`，也可通过 `#include <QwtOHLCSample>` 引入）表示每根K线的数据。注意构造函数的参数顺序为**时间、开盘、最高、最低、收盘**：

```cpp
class QwtOHLCSample {
public:
    QwtOHLCSample(double time = 0.0, double open = 0.0,
                  double high = 0.0, double low = 0.0, double close = 0.0);

    QwtInterval boundingInterval() const;  // 价格覆盖的区间
    bool isValid() const;                  // 校验 low <= open/close <= high

    double time;      // 时间点（X轴位置）
    double open;      // 开盘价
    double high;      // 最高价
    double low;       // 最低价
    double close;     // 收盘价
};
```

## 使用方法

股票K线图的例子位于:`examples/2D/stockchart`，例子截图如下：

![Stock Chart](../../../assets/screenshots/stockchart.png)

### 1. 基本K线图

K线外观由 `setSymbolStyle()` 配置（`Bar` 美国线 / `CandleStick` 蜡烛图，默认 `CandleStick`）：`setSymbolBrush()` 按价格 `Direction`（`Increasing` 上涨 / `Decreasing` 下跌）分别指定箱体填充，`setSymbolExtent()` 设置以坐标单位计的箱体宽度；数据以 `QwtOHLCSample`（时间、开盘、最高、最低、收盘）样本传入 `setSamples()`。下面的例子在绘图上绘制四根日K线：

```cpp
#include <QwtPlot>
#include <QwtPlotTradingCurve>

QwtPlot* plot = new QwtPlot();
plot->setTitle("股票K线图");
plot->setCanvasBackground(Qt::white);

// 创建K线图
QwtPlotTradingCurve* curve = new QwtPlotTradingCurve("日K线");

// 设置符号样式
curve->setSymbolStyle(QwtPlotTradingCurve::CandleStick);

// 设置涨跌颜色
curve->setSymbolBrush(QwtPlotTradingCurve::Increasing,
    QBrush(Qt::red));   // 涨（收盘>开盘）：红色
curve->setSymbolBrush(QwtPlotTradingCurve::Decreasing,
    QBrush(Qt::green)); // 跌（收盘<=开盘）：绿色

// 设置箱体宽度
curve->setSymbolExtent(0.8);  // 宽度为0.8个坐标（X轴）单位

// 准备数据：QwtOHLCSample(时间, 开盘, 最高, 最低, 收盘)
QVector<QwtOHLCSample> samples;
samples << QwtOHLCSample(1, 10.0, 11.0, 9.5, 10.5);   // 涨
samples << QwtOHLCSample(2, 10.5, 10.8, 10.0, 10.2);  // 跌
samples << QwtOHLCSample(3, 10.2, 11.2, 10.1, 10.8);  // 涨
samples << QwtOHLCSample(4, 10.8, 11.0, 10.4, 10.6);  // 跌

curve->setSamples(samples);
curve->attach(plot);

// 设置坐标轴
plot->setAxisScale(QwtAxis::YLeft, 9, 12);
plot->replot();
```

### 2. K线样式配置

#### 蜡烛图样式

`CandleStick` 模式下，开盘价与收盘价之间的箱体按价格方向填充：`setSymbolBrush()` 接受一个 `Direction`（收盘高于开盘为 `Increasing`，反之为 `Decreasing`）和画刷；描边画笔则通过 `setSymbolPen()` 全局设置——没有按方向区分的画笔重载——且平盘样本（开盘=收盘）会按 `Decreasing` 的画刷绘制，不存在独立的平盘状态。下面的代码为涨跌分别指定不同的填充：

```cpp
curve->setSymbolStyle(QwtPlotTradingCurve::CandleStick);

// 设置涨跌填充（按方向各一个画刷）
curve->setSymbolBrush(QwtPlotTradingCurve::Increasing, QBrush(Qt::red));    // 涨
curve->setSymbolBrush(QwtPlotTradingCurve::Decreasing, QBrush(Qt::green));  // 跌

// 描边画笔为两个方向共用
curve->setSymbolPen(QPen(Qt::black, 1));
```

#### 美国线样式

`Bar` 样式只绘制从最低价到最高价的竖线，并以左右小横线标记开盘价与收盘价，不绘制箱体。通过 `setSymbolStyle(QwtPlotTradingCurve::Bar)` 切换：

```cpp
curve->setSymbolStyle(QwtPlotTradingCurve::Bar);

// 美国线仅显示线条，不绘制箱体
// 线条从最低价到最高价，左右小横线标记开盘和收盘价
```

### 3. 箱体宽度设置

箱体宽度由三个方法共同控制：`setSymbolExtent()` 以坐标单位设定宽度，`setMinSymbolWidth()` 与 `setMaxSymbolWidth()` 分别限制渲染像素宽度的下限与上限。下面的代码组合使用了三者：

```cpp
// 设置箱体宽度（相对坐标单位）
curve->setSymbolExtent(0.5);  // 箱体宽度为0.5个坐标单位

// 最小宽度（像素）
curve->setMinSymbolWidth(3);  // 最小3像素宽

// 最大宽度（像素）
curve->setMaxSymbolWidth(20); // 最大20像素宽
```

### 4. 数据设置

`setSamples()` 提供两种重载：一种接收 `QVector<QwtOHLCSample>`——每个样本包含时间、开盘、最高、最低、收盘五个值；另一种接收 `QwtSeriesData<QwtOHLCSample>*`，便于接入自定义数据源。下面的代码演示了这两种写法：

```cpp
// 使用QwtOHLCSample数组
QVector<QwtOHLCSample> samples;
for (int i = 0; i < count; i++) {
    samples << QwtOHLCSample(
        time[i], open[i], high[i], low[i], close[i]
    );
}
curve->setSamples(samples);

// 使用QwtSeriesData（QwtSeriesData<QwtOHLCSample>*）
curve->setSamples(seriesData);
```

### 5. 结合成交量图

成交量通常以 `QwtPlotBarChart` 叠加到同一绘图上：其 `setSamples()` 接收 `QVector<double>` 逐柱数值，`setBrush()` 设置柱体填充。下面的例子添加一组灰色成交量柱：

```cpp
// 创建成交量柱状图
QwtPlotBarChart* volumeChart = new QwtPlotBarChart("成交量");

QVector<double> volumes;
volumes << 1000 << 1500 << 800 << 2000;
volumeChart->setSamples(volumes);
volumeChart->setBrush(QBrush(Qt::gray));
volumeChart->attach(plot);
```

### 6. 添加均线

均线就是叠加在K线之上的普通 `QwtPlotCurve`，各自配置画笔与样本数据后挂载即可。下面的例子叠加 5 日与 10 日两条均线：

```cpp
#include <QwtPlotCurve>

// 添加5日均线
QwtPlotCurve* ma5 = new QwtPlotCurve("MA5");
ma5->setPen(QPen(Qt::blue, 1));
ma5->setSamples(ma5Data);
ma5->attach(plot);

// 添加10日均线
QwtPlotCurve* ma10 = new QwtPlotCurve("MA10");
ma10->setPen(QPen(Qt::yellow, 1));
ma10->setSamples(ma10Data);
ma10->attach(plot);
```

## 核心方法总结

| 方法 | 说明 |
|------|------|
| `setSamples()` | 设置K线数据（`QVector<QwtOHLCSample>` 或 `QwtSeriesData<QwtOHLCSample>*`） |
| `setSymbolStyle()` | 设置K线符号样式（`Bar` / `CandleStick`） |
| `setSymbolBrush()` | 按方向（`Increasing` / `Decreasing`）设置箱体填充画刷 |
| `setSymbolPen()` | 设置符号描边画笔（全局一支，两个方向共用） |
| `setSymbolExtent()` | 设置箱体宽度（坐标单位） |
| `setMinSymbolWidth()` | 设置最小宽度（像素） |
| `setMaxSymbolWidth()` | 设置最大宽度（像素） |

## 涨跌状态判断

| 状态 | 枚举值 | 判断条件 |
|------|--------|----------|
| 上涨 | `Increasing` | close > open |
| 下跌 | `Decreasing` | close <= open（平盘 close == open 也使用 `Decreasing` 画刷） |

> **提示 — 颜色约定**
>
> - 中国市场：红涨绿跌
> - 国际市场：绿涨红跌
> - 可根据需要调整颜色配置

> **示例 — 相关示例**
>
> - 股票图演示：`examples/2D/stockchart`