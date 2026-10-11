# 直方图 - QwtPlotHistogram

`QwtPlotHistogram` 用于绘制直方图，表示一系列区间与值的关联关系。每个区间对应一个数值，常用于统计学中展示数据分布频率。

## 主要功能特性

**特性**

- ✅ **多种显示样式**：支持轮廓、柱状、线条三种绘制模式
- ✅ **区间数据结构**：使用区间+值的数据格式，适合统计分布
- ✅ **符号自定义**：可设置每个区间柱体的显示样式
- ✅ **基线配置**：支持自定义基线位置

## 基本概念

### 直方图与柱状图的区别

| 特性 | 柱状图 (BarChart) | 直方图 (Histogram) |
|------|------------------|-------------------|
| 数据结构 | 点值 (x, y) | 区间值 ([x1, x2], y) |
| 柱体宽度 | 固定或可调 | 由区间决定 |
| 适用场景 | 分类数据对比 | 连续数据分布 |
| X轴含义 | 分类标签 | 数值区间 |

### 直方图样式

| 样式 | 枚举值 | 说明 |
|------|--------|------|
| 轮廓 | `Outline` | 绘制整体轮廓并填充 |
| 柱状 | `Columns` | 每个区间绘制独立柱体（默认） |
| 线条 | `Lines` | 每个区间绘制水平线条 |

### 数据结构

直方图使用 `QwtIntervalSample` 表示数据：

```cpp
struct QwtIntervalSample {
    double value;      // 区间对应的值（高度）
    QwtInterval interval; // 区间范围 [min, max]
};
```

## 使用方法

### 1. 基本直方图

```cpp
#include <QwtPlot>
#include <QwtPlotHistogram>
#include <QwtInterval>

QwtPlot* plot = new QwtPlot();
plot->setTitle("直方图示例");
plot->setCanvasBackground(Qt::white);

// 创建直方图
QwtPlotHistogram* histogram = new QwtPlotHistogram("数据分布");

// 设置样式（默认为Columns）
histogram->setStyle(QwtPlotHistogram::Columns);

// 准备区间数据
QVector<QwtIntervalSample> samples;
samples << QwtIntervalSample(10, QwtInterval(0, 10));    // [0,10]区间，值=10
samples << QwtIntervalSample(25, QwtInterval(10, 20));   // [10,20]区间，值=25
samples << QwtIntervalSample(15, QwtInterval(20, 30));   // [20,30]区间，值=15
samples << QwtIntervalSample(30, QwtInterval(30, 40));   // [30,40]区间，值=30
histogram->setSamples(samples);

// 设置样式
histogram->setPen(QPen(Qt::darkBlue, 1));
histogram->setBrush(QBrush(QColor(100, 150, 200)));

histogram->attach(plot);
plot->replot();
```

### 2. 设置数据

```cpp
// 使用QwtIntervalSample数组
QVector<QwtIntervalSample> samples;
for (int i = 0; i < 10; i++) {
    double min = i * 10;
    double max = (i + 1) * 10;
    double value = rand() % 50 + 10;
    samples << QwtIntervalSample(value, QwtInterval(min, max));
}
histogram->setSamples(samples);

// 使用QwtSeriesData
histogram->setSamples(seriesData);
```

### 3. 显示样式

#### 柱状样式（Columns）

这是最常用的样式，每个区间显示为独立柱体：

```cpp
histogram->setStyle(QwtPlotHistogram::Columns);
histogram->setPen(QPen(Qt::blue, 1));
histogram->setBrush(QBrush(QColor(100, 150, 200, 150)));
```

#### 轮廓样式（Outline）

绘制整体轮廓，所有区间作为一个整体：

```cpp
histogram->setStyle(QwtPlotHistogram::Outline);
histogram->setPen(QPen(Qt::darkBlue, 2));
histogram->setBrush(QBrush(Qt::lightGray));

// 注意：轮廓样式要求区间按递增顺序排列且不重叠
```

> **警告 — 轮廓样式要求**
>
> 轮廓样式（Outline）需要满足以下条件：
> 1. 区间必须按递增顺序排列
> 2. 区间之间不能有重叠
> 3. 区间之间最好不要有间隙

#### 线条样式（Lines）

仅绘制每个区间的值线条：

```cpp
histogram->setStyle(QwtPlotHistogram::Lines);
histogram->setPen(QPen(Qt::red, 2));
// Lines样式不使用填充
```

### 4. 柱体符号配置

```cpp
#include <QwtColumnSymbol>

// 创建柱体符号
QwtColumnSymbol* symbol = new QwtColumnSymbol(QwtColumnSymbol::Box);
symbol->setFrameStyle(QwtColumnSymbol::Plain);  // 平面边框

// 设置填充渐变
QLinearGradient gradient(0, 0, 0, 1);
gradient.setColorAt(0, Qt::blue);
gradient.setColorAt(1, Qt::darkBlue);
symbol->setBrush(QBrush(gradient));

// 设置边框
symbol->setPen(QPen(Qt::black, 1));

histogram->setSymbol(symbol);
```

### 5. 基线设置

```cpp
// 设置基线位置
histogram->setBaseline(0.0);  // 默认从0开始

// 使用负值基线可突出显示正值区间
histogram->setBaseline(-5.0);
```

### 6. 统计直方图示例

从原始数据计算并绘制直方图：

```cpp
// 原始数据
QVector<double> rawData;
for (int i = 0; i < 1000; i++) {
    rawData << (rand() % 100);  // 0-99随机数
}

// 计算直方图区间
int binCount = 10;
double minVal = 0, maxVal = 100;
double binWidth = (maxVal - minVal) / binCount;

QVector<QwtIntervalSample> samples(binCount);
for (int i = 0; i < binCount; i++) {
    double binMin = minVal + i * binWidth;
    double binMax = binMin + binWidth;
    
    // 统计落在该区间的数据数量
    int count = 0;
    for (double val : rawData) {
        if (val >= binMin && val < binMax) {
            count++;
        }
    }
    
    samples[i] = QwtIntervalSample(count, QwtInterval(binMin, binMax));
}

histogram->setSamples(samples);
```

## 核心方法总结

| 方法 | 说明 |
|------|------|
| `setSamples()` | 设置区间数据 |
| `setStyle()` | 设置显示样式 |
| `setPen()` | 设置线条画笔 |
| `setBrush()` | 设置填充画笔 |
| `setSymbol()` | 设置柱体符号 |
| `setBaseline()` | 设置基线位置 |
| `data()` | 获取数据对象 |
| `boundingRect()` | 获取边界矩形 |

## QwtInterval 类

`QwtInterval` 表示一个数值区间：

```cpp
#include <QwtInterval>

// 创建区间
QwtInterval interval(0, 100);  // [0, 100]

// 设置边界条件
interval.setBorderFlags(QwtInterval::IncludeMinimum | 
                        QwtInterval::IncludeMaximum);  // [0, 100] 包含两端

// 或排除边界
interval.setBorderFlags(QwtInterval::ExcludeMinimum |
                        QwtInterval::ExcludeMaximum);  // (0, 100) 排除两端

// 区间操作
double width = interval.width();       // 区间宽度
double min = interval.minValue();      // 最小值
double max = interval.maxValue();      // 最大值
bool contains = interval.contains(50); // 检查值是否在区间内
```

> **提示 — 直方图应用场景**
>
> - 统计数据分布频率
> - 图像亮度分布分析
> - 连续变量的离散化展示

> **示例 — 相关示例**
>
> - 直方图演示：结合自定义示例创建