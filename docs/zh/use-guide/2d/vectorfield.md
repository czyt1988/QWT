# 向量场图 - QwtPlotVectorField

`QwtPlotVectorField` 用于绘制向量场（Vector Field），在二维平面上显示带有方向和大小信息的向量数据。常用于流体力学、电磁场分析、气象数据等领域。

## 主要功能特性

- 用箭头表示向量的方向与大小
- 向量大小可映射到箭头长度或颜色
- 可调整显示密度，避免数据点过密
- 可自定义箭头形状与样式

## 基本概念

### 向量场表示

向量场数据包含位置和向量两个部分：

```text
      ↑     ↗
      │    /
      │   /
  ←───●───→───●───→
      │   \
      │    \
      ↓     ↙

每个点显示一个向量（方向+大小）
```

### 数据结构

使用 `QwtVectorFieldSample` 表示单个向量：

```cpp
class QwtVectorFieldSample {
public:
    QwtVectorFieldSample(double x = 0.0, double y = 0.0,
                         double vx = 0.0, double vy = 0.0);

    QPointF pos() const;  // 以QPointF返回位置
    bool isNull() const;  // vx == vy == 0 时为true

    double x;   // 位置的X坐标
    double y;   // 位置的Y坐标
    double vx;  // 向量的X分量
    double vy;  // 向量的Y分量
};
```

## 使用方法

向量场的例子位于:`playground/vectorfield`，例子截图如下：

![Vector Field](../../../assets/screenshots/vectorfield.png)

### 1. 基本向量场图

向量场项直接构造 `QwtPlotVectorField`，数据以 `QwtVectorFieldSample` 序列传入 `setSamples()`，每个样本由位置 (x, y) 和向量分量 (vx, vy) 组成。下面的例子生成 10 x 10 的三角函数向量网格并挂载到绘图上：

```cpp
#include <QwtPlot>
#include <QwtPlotVectorField>

QwtPlot* plot = new QwtPlot();
plot->setTitle("向量场示例");
plot->setCanvasBackground(Qt::white);

// 创建向量场
QwtPlotVectorField* vectorField = new QwtPlotVectorField();

// 准备数据
QVector<QwtVectorFieldSample> samples;
for (int x = 0; x < 10; x++) {
    for (int y = 0; y < 10; y++) {
        double vx = std::sin(y * 0.5);  // X分量
        double vy = std::cos(x * 0.5);  // Y分量
        samples << QwtVectorFieldSample(x, y, vx, vy);
    }
}
vectorField->setSamples(samples);

vectorField->attach(plot);
plot->replot();
```

### 2. 箭头样式配置

箭头形状由 `QwtVectorFieldSymbol` 的实现类提供，可选 `QwtVectorFieldArrow`（实心箭头，可指定头宽与尾宽）或 `QwtVectorFieldThinArrow`（细线箭头），通过 `setSymbol()` 挂载，描边则由向量场项自身的 `setPen()` 控制；`setMagnitudeScaleFactor()` 决定向量大小对箭头长度的影响程度。下面的代码演示这些配置：

```cpp
#include <QwtVectorFieldSymbol>

// QwtVectorFieldSymbol 是抽象基类，需使用其内置实现类
// （所有权转移给向量场项）：

// 实心箭头，可指定头宽与尾宽
vectorField->setSymbol(new QwtVectorFieldArrow(8.0, 2.0));

// 或：细线箭头（这是默认符号）
vectorField->setSymbol(new QwtVectorFieldThinArrow(6.0));

// 画笔与画刷设置在向量场项上，而不是符号上：
// 画笔描边，画刷填充箭头
vectorField->setPen(QPen(Qt::blue, 1));
vectorField->setBrush(QBrush(Qt::blue));

// 设置箭头长度比例
vectorField->setMagnitudeScaleFactor(0.5);  // 缩放因子
```

### 3. 向量大小映射

大小到长度的映射由 `setMagnitudeScaleFactor()` 控制：它对每个向量的大小施加线性缩放因子（在 `MagnitudeAsLength` 模式生效时起作用，该模式默认开启），还可用 `setMinArrowLength()` / `setMaxArrowLength()` 限定箭头长度范围。下面的代码演示线性映射、长度限制与固定长度三种配置：

```cpp
// 将向量大小映射到箭头长度（MagnitudeAsLength 默认已开启）
vectorField->setMagnitudeMode(QwtPlotVectorField::MagnitudeAsLength, true);
vectorField->setMagnitudeScaleFactor(1.0);  // 长度 = 大小 * 缩放因子

// 将计算出的长度限制在5~20像素之间
vectorField->setMinArrowLength(5);
vectorField->setMaxArrowLength(20);

// 固定箭头长度（忽略大小）：将最小值与最大值设为相同值，
// 所有非零向量都渲染为10像素长
vectorField->setMinArrowLength(10);
vectorField->setMaxArrowLength(10);
```

### 4. 显示密度控制

显示密度通过栅格化过滤降低：`setRasterSize()` 定义栅格单元大小，配合 `setPaintAttribute()` 启用 `FilterVectors` 属性后，同一单元内的样本会被平均合并。下面的代码演示这一抽稀配置：

```cpp
// 将20 x 20像素栅格单元内的所有向量平均合并为一个箭头
vectorField->setRasterSize(QSizeF(20, 20));

// 启用过滤（不开启FilterVectors时每个样本都会绘制）
vectorField->setPaintAttribute(QwtPlotVectorField::FilterVectors, true);
```

### 5. 颜色映射

按大小着色需要一个 `QwtColorMap`（如两色之间渐变的 `QwtLinearColorMap`），用 `setColorMap()` 赋给向量场，并通过 `setMagnitudeMode()` 启用 `MagnitudeAsColor` 标志；`setMagnitudeRange()` 指定色带覆盖的数值区间。下面的例子配置蓝到红的颜色映射：

```cpp
#include <QwtColorMap>

// 根据向量大小设置颜色
QwtLinearColorMap* colorMap = new QwtLinearColorMap(Qt::blue, Qt::red);
vectorField->setColorMap(colorMap);

// 启用大小到颜色的映射
vectorField->setMagnitudeMode(QwtPlotVectorField::MagnitudeAsColor, true);

// 色带覆盖的数值区间；不设置（无效区间）时
// 由样本数据自动确定范围
vectorField->setMagnitudeRange(QwtInterval(0.0, 2.0));
```

## 核心方法总结

| 方法 | 说明 |
|------|------|
| `setSamples()` | 设置向量数据（`QVector<QwtVectorFieldSample>` 或 `QwtVectorFieldData*`） |
| `setSymbol()` | 设置箭头符号（`QwtVectorFieldArrow` / `QwtVectorFieldThinArrow`） |
| `setPen()` / `setBrush()` | 设置箭头描边画笔 / 填充画刷 |
| `setMagnitudeScaleFactor()` | 设置大小到长度的缩放因子 |
| `setMinArrowLength()` / `setMaxArrowLength()` | 限制箭头长度（像素） |
| `setRasterSize()` + `setPaintAttribute(FilterVectors)` | 按栅格单元平均合并向量，降低显示密度 |
| `setColorMap()` | 设置 `MagnitudeAsColor` 使用的颜色映射 |
| `setMagnitudeMode()` | 启用 `MagnitudeAsColor` / `MagnitudeAsLength` 映射 |
| `setMagnitudeRange()` | 设置颜色映射覆盖的大小区间 |
| `setIndicatorOrigin()` | 设置箭头锚点（`OriginHead` / `OriginTail` / `OriginCenter`） |

> **提示 — 应用场景**
>
> - 流体力学：流速场可视化
> - 电磁学：电场/磁场分布
> - 气象学：风向场显示
> - 数学分析：向量函数可视化

> **示例 — 相关示例**
>
> - 向量场演示：`playground/vectorfield`