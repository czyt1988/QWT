# 符号样式 - QwtSymbol

`QwtSymbol` 用于定义数据点的显示符号样式，支持多种预定义形状和自定义路径。符号可以设置大小、填充、边框等属性，用于在曲线和散点图上标记数据点。

## 主要功能特性

预定义形状包括椭圆、矩形、菱形、三角形和十字等，也可以用 QPainterPath 定义任意形状。符号的尺寸、填充色和边框色均可设置，图钉点则决定符号相对数据点的锚点位置。

## 基本概念

### 符号形状列表

| 形状 | 枚举值 | 说明 |
|------|--------|------|
| 无符号 | `NoSymbol` | 不显示符号 |
| 椭圆 | `Ellipse` | 圆形或椭圆 |
| 矩形 | `Rect` | 正方形或矩形 |
| 菱形 | `Diamond` | 菱形 |
| 三角形 | `Triangle` | 向上三角形 |
| 倒三角 | `DTriangle` | 向下三角形 |
| 上三角 | `UTriangle` | 向上三角形 |
| 左三角 | `LTriangle` | 向左三角形 |
| 右三角 | `RTriangle` | 向右三角形 |
| 十字 | `Cross` | +形十字 |
| X十字 | `XCross` | X形斜十字 |
| 水平线 | `HLine` | 水平线段 |
| 垂直线 | `VLine` | 垂直线段 |
| 星形 | `Star1` | X 与 + 组合的星形 |
| 星形2 | `Star2` | 六角星 |
| 六边形 | `Hexagon` | 正六边形 |
| 路径 | `Path` | 自定义QPainterPath |
| 位图 | `Pixmap` | QPixmap 位图 |
| 图形 | `Graphic` | QwtGraphic对象 |
| SVG | `SvgDocument` | SVG文档图形 |

## 使用方法

符号演示的例子位于:`playground/symbols`，例子截图如下：

![Symbols Demo](../../../assets/screenshots/symbols.png)

### 1. 创建基本符号

符号既可以通过便捷构造函数 `QwtSymbol(style, brush, pen, size)` 一次性配置完成，也可以先创建空对象再用 `setStyle()`、`setBrush()`、`setPen()`、`setSize()` 逐步设置。下面的例子演示了这两种写法：

```cpp
#include <QwtSymbol>

// 方式1：便捷构造函数
QwtSymbol* symbol = new QwtSymbol(
    QwtSymbol::Ellipse,     // 形状
    QBrush(Qt::red),        // 填充
    QPen(Qt::darkRed, 1),   // 边框
    QSize(10, 10)           // 大小
);

// 方式2：逐步设置
QwtSymbol* symbol2 = new QwtSymbol();
symbol2->setStyle(QwtSymbol::Diamond);
symbol2->setBrush(QBrush(Qt::blue));
symbol2->setPen(QPen(Qt::black, 2));
symbol2->setSize(8, 8);
```

### 2. 应用符号到曲线

通过 `QwtPlotCurve::setSymbol()` 把符号交给曲线（曲线接管指针所有权）后，符号会在每个样本点位置重复绘制。下面的例子为曲线挂上一个填充符号：

```cpp
#include <QwtPlotCurve>

QwtPlotCurve* curve = new QwtPlotCurve();

// 设置符号（QwtSymbol 没有 Circle 枚举，圆形用正方形尺寸的 Ellipse 实现）
curve->setSymbol(new QwtSymbol(
    QwtSymbol::Ellipse,
    QBrush(Qt::yellow),
    QPen(Qt::blue, 1),
    QSize(6, 6)
));

// 符号会在每个数据点位置显示
```

### 3. 常用符号形状示例

便捷构造函数的第一个参数直接传 `QwtSymbol::Style` 枚举值，再为不同形状搭配填充 `QBrush` 与轮廓 `QPen`；`Cross`、`XCross` 这类线形符号通常搭配 `Qt::NoBrush` 只绘制线条。下面的代码给出几种常用形状的构建方式：

```cpp
// 圆形符号
QwtSymbol* circle = new QwtSymbol(QwtSymbol::Ellipse, 
    QBrush(Qt::white), QPen(Qt::blue, 2), QSize(10, 10));

// 矩形符号
QwtSymbol* rect = new QwtSymbol(QwtSymbol::Rect,
    QBrush(Qt::green), QPen(Qt::darkGreen), QSize(8, 8));

// 菱形符号
QwtSymbol* diamond = new QwtSymbol(QwtSymbol::Diamond,
    QBrush(Qt::cyan), QPen(Qt::darkCyan), QSize(12, 12));

// 十字符号（仅线条，无填充）
QwtSymbol* cross = new QwtSymbol(QwtSymbol::Cross,
    Qt::NoBrush, QPen(Qt::red, 2), QSize(10, 10));

// X十字符号
QwtSymbol* xcross = new QwtSymbol(QwtSymbol::XCross,
    Qt::NoBrush, QPen(Qt::magenta, 2), QSize(12, 12));

// 星形符号
QwtSymbol* star = new QwtSymbol(QwtSymbol::Star1,
    QBrush(Qt::yellow), QPen(Qt::orange), QSize(15, 15));
```

### 4. 自定义形状

使用 QPainterPath 创建自定义形状：

```cpp
QwtSymbol* customSymbol = new QwtSymbol();

// 创建自定义路径（例如箭头形状）
QPainterPath path;
path.moveTo(0, -10);     // 顶部尖端
path.lineTo(6, 5);       // 右肩
path.lineTo(0, 2);       // 中心凹陷
path.lineTo(-6, 5);      // 左肩
path.closeSubpath();

customSymbol->setStyle(QwtSymbol::Path);
customSymbol->setPath(path);
customSymbol->setBrush(QBrush(Qt::red));
customSymbol->setPen(QPen(Qt::darkRed, 1));
customSymbol->setSize(20, 20);
```

### 5. 符号大小设置

符号尺寸由 `setSize()` 设置：它提供接受 `QSize` 与分别接受宽、高两个重载，因此可以绘制椭圆形等非正方形符号。下面的例子演示了两种形式：

```cpp
// 设置固定大小
symbol->setSize(QSize(10, 10));  // 10x10像素

// 设置不同宽高（椭圆形）
symbol->setSize(15, 8);  // 宽15，高8

// 动态大小（根据数据值）
// 需在自定义绘制中实现
```

### 6. 符号图钉点

图钉点（pin point）定义的是符号自身坐标系内（`Path` 为 painter path 坐标、`Pixmap` 为像素坐标、`SvgDocument` 为 viewBox 坐标）与数据点对齐的位置，它仅对 `Path`、`Pixmap`、`Graphic`、`SvgDocument` 这几种符号生效，坐标不是 [0,1] 归一化值。默认图钉点未启用，符号以中心对齐数据点。

```cpp
// 默认未启用图钉点对齐：符号中心对准数据点

// 让符号自身坐标系的原点 (0, 0) 对准数据点——
// 例如把画在 (0, 0) 处的箭头尖端正压在数据值上。
// setPinPoint() 会同时启用对齐：
symbol->setPinPoint(QPointF(0.0, 0.0));

// 符号坐标系内的任意位置都可以，
// 例如原点右下方 5 个单位处：
symbol->setPinPoint(QPointF(5.0, 5.0));

// 关闭图钉点对齐：符号恢复为以中心对准数据点
symbol->setPinPointEnabled(false);
```

### 7. 符号填充样式

填充由 `setBrush()` 控制，`QBrush` 可以是纯色、渐变（如 `QLinearGradient`）或填充图案；传入 `Qt::NoBrush` 则只保留轮廓。下面的代码给出几种常见的填充写法：

```cpp
// 简单填充
symbol->setBrush(QBrush(Qt::red));

// 渐变填充
QLinearGradient gradient(0, 0, 0, 1);
gradient.setColorAt(0, Qt::red);
gradient.setColorAt(1, Qt::darkRed);
symbol->setBrush(QBrush(gradient));

// 无填充（仅显示边框）
symbol->setBrush(Qt::NoBrush);

// 图案填充
symbol->setBrush(QBrush(Qt::blue, Qt::DiagCrossPattern));
```

### 8. 边框样式

边框由 `setPen()` 控制，线型、宽度与颜色都来自传入的 `QPen`；传入 `Qt::NoPen` 可完全去掉边框。下面的例子演示了几种典型的画笔配置：

```cpp
// 实线边框
symbol->setPen(QPen(Qt::black, 1, Qt::SolidLine));

// 虚线边框
symbol->setPen(QPen(Qt::blue, 2, Qt::DashLine));

// 无边框
symbol->setPen(Qt::NoPen);

// 圆角边框（仅对矩形有效）
// 需通过自定义Path实现
```

## 核心方法总结

| 方法 | 说明 |
|------|------|
| `setStyle()` | 设置符号形状 |
| `setSize()` | 设置符号大小 |
| `setBrush()` | 设置填充画笔 |
| `setPen()` | 设置边框画笔 |
| `setPath()` | 设置自定义路径 |
| `setPinPoint()` | 设置图钉点位置 |
| `drawSymbol()` | 绘制符号 |

> **提示 — 符号使用建议**
>
> - 数据点密集时使用小符号或不显示符号
> - 强调关键数据点时使用大符号
> - 不同数据系列使用不同形状便于区分
> - 无填充符号（Cross、XCross）性能更好

> **示例 — 相关示例**
>
> - 符号演示：`playground/symbols`