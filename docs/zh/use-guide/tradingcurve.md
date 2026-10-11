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

使用 `QwtTradingSample` 表示每根K线的数据：

```cpp
struct QwtTradingSample {
    double time;      // 时间点（X轴位置）
    double open;      // 开盘价
    double close;     // 收盘价
    double high;      // 最高价
    double low;       // 最低价
};
```

## 使用方法

股票K线图的例子位于:`examples/2D/stockchart`，例子截图如下：

![Stock Chart](../../assets/screenshots/stockchart.png)

### 1. 基本K线图

```cpp
#include <QwtPlot>
#include <QwtPlotTradingCurve>

QwtPlot* plot = new QwtPlot();
plot->setTitle("股票K线图");
plot->setCanvasBackground(Qt::white);

// 创建K线图
QwtPlotTradingCurve* curve = new QwtPlotTradingCurve("日K线");

// 设置样式
curve->setStyle(QwtPlotTradingCurve::CandleStick);

// 设置涨跌颜色
curve->setSymbolBrush(QwtPlotTradingCurve::Positive, 
    QBrush(Qt::red));   // 涨（收盘>开盘）：红色
curve->setSymbolBrush(QwtPlotTradingCurve::Negative,
    QBrush(Qt::green)); // 跌（收盘<开盘）：绿色

// 设置箱体宽度
curve->setSymbolExtent(0.8);  // 相对宽度的80%

// 准备数据
QVector<QwtTradingSample> samples;
samples << QwtTradingSample(1, 10.0, 10.5, 11.0, 9.5);   // 涨
samples << QwtTradingSample(2, 10.5, 10.2, 10.8, 10.0);  // 跌
samples << QwtTradingSample(3, 10.2, 10.8, 11.2, 10.1);  // 涨
samples << QwtTradingSample(4, 10.8, 10.6, 11.0, 10.4);  // 跌

curve->setSamples(samples);
curve->attach(plot);

// 设置坐标轴
plot->setAxisScale(QwtAxis::YLeft, 9, 12);
plot->replot();
```

### 2. K线样式配置

#### 蜡烛图样式

```cpp
curve->setStyle(QwtPlotTradingCurve::CandleStick);

// 设置涨跌样式
curve->setSymbolBrush(QwtPlotTradingCurve::Positive, QBrush(Qt::red));
curve->setSymbolPen(QwtPlotTradingCurve::Positive, QPen(Qt::darkRed, 1));

curve->setSymbolBrush(QwtPlotTradingCurve::Negative, QBrush(Qt::green));
curve->setSymbolPen(QwtPlotTradingCurve::Negative, QPen(Qt::darkGreen, 1));

// 平盘样式（开盘=收盘）
curve->setSymbolBrush(QwtPlotTradingCurve::Neutral, QBrush(Qt::gray));
curve->setSymbolPen(QwtPlotTradingCurve::Neutral, QPen(Qt::black, 1));
```

#### 美国线样式

```cpp
curve->setStyle(QwtPlotTradingCurve::Bar);

// 美国线仅显示线条，不绘制箱体
// 线条从最低价到最高价，左右小横线标记开盘和收盘价
```

### 3. 箱体宽度设置

```cpp
// 设置箱体宽度（相对坐标单位）
curve->setSymbolExtent(0.5);  // 箱体宽度为0.5个坐标单位

// 最小宽度（像素）
curve->setMinSymbolWidth(3);  // 最小3像素宽

// 最大宽度（像素）
curve->setMaxSymbolWidth(20); // 最大20像素宽
```

### 4. 数据设置

```cpp
// 使用QwtTradingSample数组
QVector<QwtTradingSample> samples;
for (int i = 0; i < count; i++) {
    samples << QwtTradingSample(
        time[i], open[i], close[i], high[i], low[i]
    );
}
curve->setSamples(samples);

// 使用QwtSeriesData
curve->setSamples(seriesData);
```

### 5. 结合成交量图

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
| `setSamples()` | 设置K线数据 |
| `setStyle()` | 设置K线样式 |
| `setSymbolBrush()` | 设置涨跌填充色 |
| `setSymbolPen()` | 设置涨跌边框色 |
| `setSymbolExtent()` | 设置箱体宽度 |
| `setMinSymbolWidth()` | 设置最小宽度（像素） |
| `setMaxSymbolWidth()` | 设置最大宽度（像素） |

## 涨跌状态判断

| 状态 | 枚举值 | 判断条件 |
|------|--------|----------|
| 上涨 | `Positive` | close > open |
| 下跌 | `Negative` | close < open |
| 平盘 | `Neutral` | close == open |

> **提示 — 颜色约定**
>
> - 中国市场：红涨绿跌
> - 国际市场：绿涨红跌
> - 可根据需要调整颜色配置

> **示例 — 相关示例**
>
> - 股票图演示：`examples/2D/stockchart`