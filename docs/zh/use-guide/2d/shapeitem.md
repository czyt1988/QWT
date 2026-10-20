# 形状项 - QwtPlotShapeItem

`QwtPlotShapeItem` 用于在绘图上显示任意形状，支持矩形、圆形、多边形、路径等几何图形。形状使用坐标定位，会随着坐标轴变换自动调整。

## 主要功能特性

形状可以是 QPainterPath 能描述的各种几何图形，包括矩形、圆形和多边形。形状位置使用绘图坐标，会随坐标轴缩放。填充、边框和颜色均可设置，多边形裁剪（`ClipPolygons`）与抽点简化（`setRenderTolerance()`）能降低渲染开销。

## 基本概念

### 形状定位

QwtPlotShapeItem 的形状使用绘图坐标定位，而非像素坐标：

```text
    Y轴
     │  ┌─────────┐
     │  │ 矩形形状 │  ← 使用(x,y)坐标定义位置
     │  └─────────┘
     └───────────────→ X轴
```

### 绘制属性与图例模式

`QwtPlotShapeItem` 没有填充模式（fill mode）设置；渲染与图例行为分别由绘制属性和图例模式控制：

| 设置 | 枚举值 | 说明 |
|------|--------|------|
| 绘制属性 | `ClipPolygons` | 绘制前先裁剪多边形；深度缩放时可显著提速，但曲线/椭圆路径需先转换为多边形，可能反而变慢 |
| 图例模式 | `LegendShape` | 在图例中显示形状的缩小版本 |
| 图例模式 | `LegendColor` | 在图例中显示一个填色矩形 |

## 使用方法

### 1. 创建矩形形状

`QwtPlotShapeItem` 构造时传入图例标题，几何形状以 `QPainterPath` 形式交给 `setShape()`，纯矩形也可直接使用 `setRect()` 便捷接口。路径坐标按绘图坐标系解释，`setPen()` 与 `setBrush()` 分别控制边框与填充。下面的例子在绘图上添加一个半透明矩形区域：

```cpp
#include <QwtPlot>
#include <QwtPlotShapeItem>

QwtPlot* plot = new QwtPlot();

// 创建形状项
QwtPlotShapeItem* shapeItem = new QwtPlotShapeItem("矩形区域");

// 创建矩形路径
QPainterPath path;
path.addRect(QRectF(10, 10, 50, 30));  // x, y, width, height

shapeItem->setShape(path);

// 设置样式
shapeItem->setPen(QPen(Qt::blue, 2));
shapeItem->setBrush(QBrush(QColor(100, 150, 200, 100)));

shapeItem->attach(plot);
plot->replot();
```

### 2. 创建圆形形状

圆形与椭圆通过 `QPainterPath::addEllipse()` 构造，指定中心点和横纵半径，再用 `setShape()` 赋给形状项。下面的例子在绘图坐标 (50, 50) 处绘制一个半径 20 的半透明红色圆：

```cpp
QwtPlotShapeItem* circle = new QwtPlotShapeItem("圆形");

QPainterPath path;
path.addEllipse(QPointF(50, 50), 20, 20);  // 中心点, rx, ry

circle->setShape(path);
circle->setPen(QPen(Qt::red, 1));
circle->setBrush(QBrush(QColor(255, 100, 100, 150)));

circle->attach(plot);
```

### 3. 创建多边形形状

任意多边形由 `moveTo()` / `lineTo()` 序列加 `closeSubpath()` 闭合描述；也可直接用接收 `QPolygonF` 的 `setPolygon()` 便捷接口。下面的例子构造一个闭合三角形，并配置绿色边框与灰色填充：

```cpp
QwtPlotShapeItem* polygon = new QwtPlotShapeItem("多边形");

QPainterPath path;
path.moveTo(0, 0);
path.lineTo(30, 0);
path.lineTo(15, 25);
path.closeSubpath();

polygon->setShape(path);
polygon->setPen(QPen(Qt::green, 2));
polygon->setBrush(QBrush(Qt::lightGray));

polygon->attach(plot);
```

### 4. 样式配置

外观可通过 `setRenderHint()`（如继承自 `QwtPlotItem` 的 `RenderAntialiased` 抗锯齿）、`setPen()`（`Qt::DashLine` 虚线边框）和 `setBrush()` 调整；画刷同样支持 `Qt::FDiagPattern` 等 Qt 图案实现斜线填充。性能相关选项为 `ClipPolygons` 绘制属性（`setPaintAttribute()`）与 `setRenderTolerance()` 抽点简化；没有填充模式设置。下面的代码组合了这些选项：

```cpp
// 绘制前裁剪多边形（性能选项）
shapeItem->setPaintAttribute(QwtPlotShapeItem::ClipPolygons, true);

// 用抽点算法（Douglas-Peucker）简化路径；
// 容差 <= 0.0 时禁用抽点
shapeItem->setRenderTolerance(1.0);

// 设置渲染提示（继承自 QwtPlotItem）
shapeItem->setRenderHint(QwtPlotItem::RenderAntialiased);

// 设置边框样式
shapeItem->setPen(QPen(Qt::black, 1, Qt::DashLine));

// 设置填充样式
shapeItem->setBrush(QBrush(Qt::yellow, Qt::FDiagPattern));  // 斜线填充
```

## 核心方法总结

| 方法 | 说明 |
|------|------|
| `setShape()` | 以 `QPainterPath` 设置形状 |
| `setRect()` | 以矩形设置形状（便捷接口） |
| `setPolygon()` | 以多边形设置形状（便捷接口） |
| `setPen()` | 设置边框画笔 |
| `setBrush()` | 设置填充画刷 |
| `setPaintAttribute()` | 开关 `ClipPolygons` 裁剪 |
| `setRenderTolerance()` | 设置路径简化的抽点容差 |
| `setLegendMode()` | 设置图例显示方式（`LegendShape` / `LegendColor`） |
| `setRenderHint()` | 设置渲染提示（继承自 `QwtPlotItem`） |

> **提示 — 应用场景**
>
> - 区域标注
> - 背景区域
> - 裁剪区域标记
> - 自定义几何图形

> **示例 — 相关示例**
>
> - 形状演示：`playground/shapes`

形状演示的例子截图如下：

![Shapes](../../../assets/screenshots/shapes.png)