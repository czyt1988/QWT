# 标记点 - QwtPlotMarker

`QwtPlotMarker` 是用于在绘图上标记特定位置的绘图项。它可以显示水平线、垂直线、十字线、符号或文本标签，常用于标注关键数据点、阈值线或参考位置。

## 主要功能特性

**特性**

- ✅ **多种线条样式**：水平线、垂直线、十字线或无线条
- ✅ **符号标记**：在指定位置显示自定义符号
- ✅ **文本标签**：可添加文本注释并控制位置和方向
- ✅ **精确定位**：使用坐标值定位，自动跟随坐标轴变换

## 基本概念

### 线条样式类型

QwtPlotMarker 支持四种线条样式：

| 样式 | 枚举值 | 说明 |
|------|--------|------|
| 无线 | `NoLine` | 仅显示符号和标签 |
| 水平线 | `HLine` | 穿过标记点的水平线 |
| 垂直线 | `VLine` | 穋过标记点的垂直线 |
| 十字线 | `Cross` | 水平线和垂直线的组合 |

### 标记点组成结构

```text
         标签文本
            ↓
    ────────●────────  ← 水平线 + 符号
            │
            │          ← 垂直线
            │
    (x, y) 坐标点
```

## 使用方法

### 1. 创建基本标记点

```cpp
#include <QwtPlot>
#include <QwtPlotMarker>

QwtPlot* plot = new QwtPlot();

// 创建标记点
QwtPlotMarker* marker = new QwtPlotMarker();

// 设置标记位置（坐标值）
marker->setXValue(5.0);
marker->setYValue(10.0);
// 或使用 setValue(double x, double y)
marker->setValue(5.0, 10.0);

// 附加到绘图
marker->attach(plot);

plot->replot();
```

### 2. 水平线标记

水平线常用于标注阈值、基准值或Y轴参考线：

```cpp
QwtPlotMarker* hLine = new QwtPlotMarker();

// 设置线条样式为水平线
hLine->setLineStyle(QwtPlotMarker::HLine);

// 设置Y坐标（水平线的位置）
hLine->setYValue(50.0);  // Y=50处的水平线

// 设置线条样式
hLine->setLinePen(QPen(Qt::red, 2.0, Qt::DashLine));

// 可选：添加标签
hLine->setLabel(QwtText("阈值线"));
hLine->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);

hLine->attach(plot);
```

### 3. 垂直线标记

垂直线用于标注时间点、事件位置或X轴参考线：

```cpp
QwtPlotMarker* vLine = new QwtPlotMarker();

// 设置线条样式为垂直线
vLine->setLineStyle(QwtPlotMarker::VLine);

// 设置X坐标
vLine->setXValue(100.0);  // X=100处的垂直线

// 设置线条样式
vLine->setLinePen(QPen(Qt::blue, 1.5, Qt::DotLine));

vLine->attach(plot);
```

### 4. 十字线标记

十字线在特定坐标点显示水平和垂直交叉线：

```cpp
QwtPlotMarker* cross = new QwtPlotMarker();

// 设置十字线样式
cross->setLineStyle(QwtPlotMarker::Cross);

// 设置中心位置
cross->setValue(50.0, 50.0);

// 设置线条样式
cross->setLinePen(QPen(Qt::green, 1.0, Qt::SolidLine));

cross->attach(plot);
```

### 5. 符号标记

在特定位置显示符号（不带线条）：

```cpp
#include <QwtSymbol>

QwtPlotMarker* symbolMarker = new QwtPlotMarker();

// 不显示线条
symbolMarker->setLineStyle(QwtPlotMarker::NoLine);

// 设置位置
symbolMarker->setValue(3.0, 8.0);

// 创建符号
QwtSymbol* symbol = new QwtSymbol(
    QwtSymbol::Diamond,
    QBrush(Qt::red),
    QPen(Qt::darkRed, 2),
    QSize(12, 12)
);
symbolMarker->setSymbol(symbol);

symbolMarker->attach(plot);
```

### 6. 文本标签配置

为标记点添加文本注释：

```cpp
#include <QwtText>

QwtPlotMarker* labelMarker = new QwtPlotMarker();
labelMarker->setValue(10.0, 20.0);
labelMarker->setLineStyle(QwtPlotMarker::Cross);

// 创建文本标签
QwtText label("关键点 (10, 20)");
label.setColor(Qt::black);
label.setFont(QFont("Arial", 10, QFont::Bold));

// 设置标签
labelMarker->setLabel(label);

// 设置标签对齐方式
// 相对于标记点的位置
labelMarker->setLabelAlignment(Qt::AlignLeft | Qt::AlignBottom);

// 设置标签方向
labelMarker->setLabelOrientation(Qt::Horizontal);  // 水平文本
// 或 labelMarker->setLabelOrientation(Qt::Vertical);  // 垂直文本

// 设置标签与标记点的间距
labelMarker->setSpacing(5);  // 5像素间距

labelMarker->attach(plot);
```

### 7. 综合示例 - 标注数据峰值

```cpp
// 查找数据峰值并标注
QwtPlotCurve* curve = new QwtPlotCurve("数据");
curve->setSamples(xData, yData, count);
curve->attach(plot);

// 找到最大值点
double maxVal = yData[0];
int maxIdx = 0;
for (int i = 1; i < count; i++) {
    if (yData[i] > maxVal) {
        maxVal = yData[i];
        maxIdx = i;
    }
}

// 创建峰值标记
QwtPlotMarker* peakMarker = new QwtPlotMarker();
peakMarker->setValue(xData[maxIdx], yData[maxIdx]);

// 设置样式
peakMarker->setLineStyle(QwtPlotMarker::VLine);  // 垂直参考线
peakMarker->setLinePen(QPen(Qt::red, 1, Qt::DashLine));

// 设置符号
QwtSymbol* peakSymbol = new QwtSymbol(
    QwtSymbol::Triangle,
    QBrush(Qt::red),
    QPen(Qt::darkRed, 1),
    QSize(10, 10)
);
peakMarker->setSymbol(peakSymbol);

// 设置标签
QwtText label(QString("峰值: %1").arg(maxVal));
label.setColor(Qt::red);
peakMarker->setLabel(label);
peakMarker->setLabelAlignment(Qt::AlignTop | Qt::AlignHCenter);

peakMarker->attach(plot);
```

## 标签对齐位置

标签相对于标记点的对齐位置由 `Qt::AlignmentFlag` 控制：

| 对齐方式 | 位置说明 |
|----------|----------|
| `Qt::AlignLeft | Qt::AlignTop` | 左上方 |
| `Qt::AlignRight | Qt::AlignTop` | 右上方 |
| `Qt::AlignLeft | Qt::AlignBottom` | 左下方 |
| `Qt::AlignRight | Qt::AlignBottom` | 右下方 |
| `Qt::AlignHCenter | Qt::AlignTop` | 正上方 |
| `Qt::AlignHCenter | Qt::AlignBottom` | 正下方 |
| `Qt::AlignLeft | Qt::AlignVCenter` | 正左侧 |
| `Qt::AlignRight | Qt::AlignVCenter` | 正右侧 |

## 核心方法总结

| 方法 | 说明 |
|------|------|
| `setValue(x, y)` | 设置标记位置坐标 |
| `setXValue(x)` | 仅设置X坐标 |
| `setYValue(y)` | 仅设置Y坐标 |
| `value()` | 获取标记位置坐标 |
| `xValue()` | 获取X坐标 |
| `yValue()` | 获取Y坐标 |
| `setLineStyle()` | 设置线条样式 |
| `lineStyle()` | 获取线条样式 |
| `setLinePen()` | 设置线条画笔 |
| `linePen()` | 获取线条画笔 |
| `setSymbol()` | 设置符号 |
| `symbol()` | 获取符号 |
| `setLabel()` | 设置文本标签 |
| `label()` | 获取文本标签 |
| `setLabelAlignment()` | 设置标签对齐方式 |
| `setLabelOrientation()` | 设置标签方向 |
| `setSpacing()` | 设置标签间距 |

> **提示 — 标记层级控制**
>
> 标记通常应绘制在其他绘图项上方，可以通过设置Z值控制：
> ```cpp
> marker->setZ(100);  // 较高的Z值会在上层绘制
> ```

> **示例 — 相关示例**
>
> - 曲线峰值标注：`examples/2D/bode`
> - 曲线追踪器：`playground/curvetracker`

曲线峰值标注的例子截图如下：

![Bode Plot](../../../assets/screenshots/bode.png)