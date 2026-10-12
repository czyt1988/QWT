# 向量场图 - QwtPlotVectorField

`QwtPlotVectorField` 用于绘制向量场（Vector Field），在二维平面上显示带有方向和大小信息的向量数据。常用于流体力学、电磁场分析、气象数据等领域。

## 主要功能特性

**特性**

- ✅ **箭头符号显示**：使用箭头表示向量方向和大小
- ✅ **大小映射**：向量大小可映射到箭头长度或颜色
- ✅ **密度控制**：支持调整显示密度避免过度拥挤
- ✅ **样式自定义**：可自定义箭头形状和样式

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
struct QwtVectorFieldSample {
    double x;       // X坐标
    double y;       // Y坐标
    double vx;      // X分量
    double vy;      // Y分量
};
```

## 使用方法

向量场的例子位于:`playground/vectorfield`，例子截图如下：

![Vector Field](../../../assets/screenshots/vectorfield.png)

### 1. 基本向量场图

向量场项直接构造 `QwtPlotVectorField`，数据以 `QwtVectorFieldSample` 序列传入 `setSamples()`——每个样本由位置 (x, y) 和向量分量 (vx, vy) 组成。下面的例子生成 10 x 10 的三角函数向量网格并挂载到绘图上：

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

箭头形状由 `QwtVectorFieldSymbol` 的实现类提供——`QwtVectorFieldArrow`（实心箭头，可指定头宽与尾宽）或 `QwtVectorFieldThinArrow`（细线箭头）——通过 `setSymbol()` 挂载，描边则由向量场项自身的 `setPen()` 控制；`setMagnitudeScaleFactor()` 决定向量大小对箭头长度的影响程度。下面的代码演示这些配置：

```cpp
#include <QwtVectorFieldSymbol>

// 创建箭头符号
QwtVectorFieldSymbol* symbol = new QwtVectorFieldSymbol();
symbol->setStyle(QwtVectorFieldSymbol::Arrow);  // 箭头样式

// 设置箭头颜色
symbol->setPen(QPen(Qt::blue, 1));

// 设置箭头长度比例
vectorField->setMagnitudeScaleFactor(0.5);  // 缩放因子

vectorField->setSymbol(symbol);
```

### 3. 向量大小映射

大小到长度的映射由 `setMagnitudeScaleFactor()` 控制——对每个向量的大小施加线性缩放因子，还可用 `setMinArrowLength()` / `setMaxArrowLength()` 限定箭头长度范围。下面的代码通过调整缩放因子改变箭头长度对向量大小的响应：

```cpp
// 将向量大小映射到箭头长度
vectorField->setMagnitudeScaleFactor(1.0);  // 线性映射

// 设置固定箭头长度（忽略大小）
vectorField->setMagnitudeScaleFactor(0.0);
vectorField->setSymbolSize(10);  // 固定10像素
```

### 4. 显示密度控制

显示密度通过栅格化过滤降低：`setRasterSize()` 定义栅格单元大小，配合 `setPaintAttribute()` 启用 `FilterVectors` 属性后，同一单元内的样本会被平均合并。下面的代码演示这一抽稀配置：

```cpp
// 设置采样密度（避免过度拥挤）
vectorField->setSamplesPerInch(20);  // 每英寸20个向量

// 或设置固定跳过间隔
vectorField->setIndirectionCount(2);  // 每隔2个点显示一个
```

### 5. 颜色映射

按大小着色需要一个 `QwtColorMap`（如两色之间渐变的 `QwtLinearColorMap`），用 `setColorMap()` 赋给向量场，并通过 `setMagnitudeMode()` 启用 `MagnitudeAsColor` 标志；`setMagnitudeRange()` 指定色带覆盖的数值区间。下面的例子配置蓝到红的颜色映射：

```cpp
#include <QwtColorMap>

// 根据向量大小设置颜色
QwtLinearColorMap* colorMap = new QwtLinearColorMap(Qt::blue, Qt::red);
vectorField->setColorMap(colorMap);
vectorField->setColorMagnitude(true);  // 启用颜色映射
```

## 核心方法总结

| 方法 | 说明 |
|------|------|
| `setSamples()` | 设置向量数据 |
| `setSymbol()` | 设置箭头符号 |
| `setMagnitudeScaleFactor()` | 设置大小缩放因子 |
| `setSamplesPerInch()` | 设置显示密度 |
| `setColorMap()` | 设置颜色映射 |
| `setColorMagnitude()` | 启用颜色大小映射 |

> **提示 — 应用场景**
>
> - 流体力学：流速场可视化
> - 电磁学：电场/磁场分布
> - 气象学：风向场显示
> - 数学分析：向量函数可视化

> **示例 — 相关示例**
>
> - 向量场演示：`playground/vectorfield`