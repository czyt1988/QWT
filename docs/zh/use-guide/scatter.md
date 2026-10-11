# 散点图 - QwtPlotCurve 散点模式

散点图用于展示数据的分布情况，每个数据点用独立的符号标记。在 Qwt 中，散点图通过 `QwtPlotCurve` 配合 `QwtSymbol` 实现。

## 主要功能特性

**特性**

- ✅ **多种符号形状**：支持椭圆、矩形、菱形、三角形、十字等形状
- ✅ **符号样式自定义**：可设置大小、填充色、边框色和边框宽度
- ✅ **高性能散点模式**：使用 `Dots` 样式可高效绘制百万级数据点
- ✅ **符号抖动显示**：可避免重叠点的视觉遮挡

## 基本概念

### 散点图实现方式

Qwt 提供三种方式绘制散点图：

| 方式 | 曲线样式 | 说明 |
|------|----------|------|
| NoCurve + Symbol | `NoCurve` | 仅显示符号，无连线 |
| Dots | `Dots` | 仅绘制像素点，最高性能 |
| Lines + Symbol | `Lines` | 连线与符号同时显示 |

### 性能对比

| 数据量 | NoCurve+Symbol | Dots |
|--------|----------------|------|
| <1000点 | 快 | 最快 |
| 1000-10000点 | 较快 | 快 |
| >10000点 | 较慢 | 快 |
| >100000点 | 很慢 | 最快（推荐） |

## 使用方法

散点图的例子位于:`examples/2D/scatterplot`，例子截图如下：

![Scatter Plot](../../assets/screenshots/scatterplot.png)

### 1. 基本散点图

使用 `NoCurve` 样式和符号绘制散点图：

```cpp
#include <QwtPlot>
#include <QwtPlotCurve>
#include <QwtSymbol>

QwtPlot* plot = new QwtPlot();
plot->setTitle("散点图示例");
plot->setCanvasBackground(Qt::white);

// 创建散点曲线
QwtPlotCurve* scatter = new QwtPlotCurve("数据点");

// 设置为无连线模式
scatter->setStyle(QwtPlotCurve::NoCurve);

// 创建符号
QwtSymbol* symbol = new QwtSymbol(
    QwtSymbol::Ellipse,           // 椭圆形
    QBrush(Qt::blue),             // 蓝色填充
    QPen(Qt::darkBlue, 1),        // 深蓝边框
    QSize(8, 8)                   // 大小
);
scatter->setSymbol(symbol);

// 设置数据
QPolygonF points;
for (int i = 0; i < 100; i++) {
    double x = rand() % 100;
    double y = rand() % 100;
    points << QPointF(x, y);
}
scatter->setSamples(points);

scatter->attach(plot);
plot->replot();
```

### 2. 高性能散点模式

对于大量数据点，使用 `Dots` 样式获得最佳性能：

```cpp
QwtPlotCurve* scatter = new QwtPlotCurve("大规模散点");

// 使用Dots样式 - 仅绘制像素点，性能最高
scatter->setStyle(QwtPlotCurve::Dots);

// 可选：启用图像缓冲优化
scatter->setPaintAttribute(QwtPlotCurve::ImageBuffer, true);

// 设置颜色（Dots样式不需要Symbol）
scatter->setPen(Qt::blue);  // 点的颜色

// 生成100万个点
QVector<double> xData(1000000), yData(1000000);
for (int i = 0; i < 1000000; i++) {
    xData[i] = rand() % 1000;
    yData[i] = rand() % 1000;
}
scatter->setSamples(xData, yData);

scatter->attach(plot);
```

> **提示 — Dots vs Symbol 性能**
>
> - `Dots` 样式直接绘制像素点，适合百万级数据
> - `NoCurve + Symbol` 每个点都是一个完整的符号绘制对象，适合少量数据
> - 对于实时更新的散点图，推荐使用 `Dots`

### 3. 多种符号样式

```cpp
// 圆形符号
QwtSymbol* circle = new QwtSymbol(QwtSymbol::Ellipse, 
    QBrush(Qt::red), QPen(Qt::darkRed, 2), QSize(10, 10));

// 矩形符号
QwtSymbol* rect = new QwtSymbol(QwtSymbol::Rect,
    QBrush(Qt::green), QPen(Qt::darkGreen, 1), QSize(8, 8));

// 菱形符号
QwtSymbol* diamond = new QwtSymbol(QwtSymbol::Diamond,
    QBrush(Qt::blue), QPen(Qt::darkBlue, 1), QSize(12, 12));

// 十字符号（无填充）
QwtSymbol* cross = new QwtSymbol(QwtSymbol::Cross,
    Qt::NoBrush, QPen(Qt::black, 2), QSize(8, 8));

// X形符号
QwtSymbol* xcross = new QwtSymbol(QwtSymbol::XCross,
    Qt::NoBrush, QPen(Qt::magenta, 2), QSize(10, 10));

// 三角形符号
QwtSymbol* triangle = new QwtSymbol(QwtSymbol::Triangle,
    QBrush(Qt::yellow), QPen(Qt::orange, 1), QSize(10, 10));

// 星形符号
QwtSymbol* star = new QwtSymbol(QwtSymbol::Star1,
    QBrush(Qt::cyan), QPen(Qt::darkCyan, 1), QSize(12, 12));

// 六边形符号
QwtSymbol* hexagon = new QwtSymbol(QwtSymbol::Hexagon,
    QBrush(Qt::lightGray), QPen(Qt::gray, 1), QSize(10, 10));
```

### 4. 符号抖动显示

当多个数据点位置相近时，可以使用抖动避免重叠：

```cpp
// 设置符号抖动范围
// 参数为相对坐标范围的抖动宽度
scatter->setSymbolJitter(0.05);  // 5%的抖动范围

// 注意：setSymbolJitter是QwtPlotCurve的方法
```

> **信息 — 抖动原理**
>
> 抖动会在绘制时为每个点的X坐标添加随机偏移，使重叠点分散显示，便于观察数据密度分布。

### 5. 多组散点数据

```cpp
// 创建多组不同样式的散点
QwtPlotCurve* group1 = new QwtPlotCurve("组A");
group1->setStyle(QwtPlotCurve::NoCurve);
group1->setSymbol(new QwtSymbol(QwtSymbol::Ellipse, 
    QBrush(Qt::red), QPen(Qt::darkRed), QSize(8, 8)));
group1->setSamples(pointsA);
group1->attach(plot);

QwtPlotCurve* group2 = new QwtPlotCurve("组B");
group2->setStyle(QwtPlotCurve::NoCurve);
group2->setSymbol(new QwtSymbol(QwtSymbol::Rect,
    QBrush(Qt::blue), QPen(Qt::darkBlue), QSize(8, 8)));
group2->setSamples(pointsB);
group2->attach(plot);

QwtPlotCurve* group3 = new QwtPlotCurve("组C");
group3->setStyle(QwtPlotCurve::NoCurve);
group3->setSymbol(new QwtSymbol(QwtSymbol::Diamond,
    QBrush(Qt::green), QPen(Qt::darkGreen), QSize(8, 8)));
group3->setSamples(pointsC);
group3->attach(plot);
```

### 6. 连线与符号组合

```cpp
// 同时显示连线和数据点符号
QwtPlotCurve* curve = new QwtPlotCurve("曲线带符号");

// 设置线条样式
curve->setStyle(QwtPlotCurve::Lines);
curve->setPen(QPen(Qt::blue, 2));

// 设置符号
curve->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,
    QBrush(Qt::white), QPen(Qt::blue), QSize(6, 6)));

curve->setSamples(points);
curve->attach(plot);
```

## 符号形状列表

| 形状 | 枚举值 | 说明 |
|------|--------|------|
| `NoSymbol` | 无符号 | 不显示符号 |
| `Ellipse` | 椭圆 | 圆形或椭圆（根据Size） |
| `Rect` | 矩形 | 正方形或矩形 |
| `Diamond` | 菱形 | 菱形/钻石形 |
| `Triangle` | 三角形 | 向上三角形 |
| `DTriangle` | 倒三角 | 向下三角形 |
| `UTriangle` | 上三角 | 等同于Triangle |
| `LTriangle` | 左三角 | 向左三角形 |
| `RTriangle` | 右三角 | 向右三角形 |
| `Cross` | 十字 | +形十字 |
| `XCross` | X十字 | X形十字 |
| `Star1` | 星形 | 六角星 |
| `Star2` | 星形2 | 五角星 |
| `Hexagon` | 六边形 | 正六边形 |
| `Path` | 路径 | 自定义 QPainterPath |

## 自定义符号路径

```cpp
#include <QPainterPath>

// 创建自定义形状
QPainterPath path;
path.moveTo(0, -10);    // 顶部
path.lineTo(8, 5);      // 右肩
path.lineTo(5, 5);      // 右腰
path.lineTo(0, 0);      // 中心
path.lineTo(-5, 5);     // 左腰
path.lineTo(-8, 5);     // 左肩
path.closeSubpath();

// 创建自定义符号
QwtSymbol* customSymbol = new QwtSymbol();
customSymbol->setStyle(QwtSymbol::Path);
customSymbol->setPath(path);
customSymbol->setBrush(QBrush(Qt::red));
customSymbol->setPen(QPen(Qt::darkRed, 1));
customSymbol->setSize(20, 20);

scatter->setSymbol(customSymbol);
```

## 符号核心方法

| 方法 | 说明 |
|------|------|
| `setStyle()` | 设置符号形状 |
| `setSize()` | 设置符号尺寸 |
| `setBrush()` | 设置填充画笔 |
| `setPen()` | 设置边框画笔 |
| `setPath()` | 设置自定义路径 |
| `setPinPoint()` | 设置符号锚点位置 |
| `drawSymbol()` | 绘制符号 |

> **示例 — 相关示例**
>
> - 散点图：`examples/2D/scatterplot`
> - 符号演示：`playground/symbols`
> - 曲线样式：`examples/2D/curvedemo`