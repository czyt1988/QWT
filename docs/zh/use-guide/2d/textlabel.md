# 文本标签 - QwtPlotTextLabel

`QwtPlotTextLabel` 用于在绘图上显示文本标签，与 `QwtPlotMarker` 的标签不同，它使用画布几何坐标而非绘图数据坐标定位，适合显示固定的说明文字或标题。

## 主要功能特性

**特性**

- ✅ **画布坐标定位**：使用相对画布位置，不随坐标轴变换
- ✅ **文本样式支持**：支持字体、颜色、边距等样式配置
- ✅ **对齐控制**：可设置文本相对于定位点的对齐方式
- ✅ **背景设置**：可设置文本背景颜色或边框

## 基本概念

### 坐标定位方式

与 `QwtPlotMarker` 的区别：

| 类 | 坐标类型 | 说明 |
|------|----------|------|
| `QwtPlotMarker` | 数据坐标 | 随坐标轴缩放变换 |
| `QwtPlotTextLabel` | 画布坐标 | 固定在画布位置，不变换 |

### 定位示意

下图给出文本标签的定位方式：标签钉在画布的固定相对位置（此处为右上角），与绘图数据坐标无关：

```text
┌────────────────────────────────────┐
│                                    │
│  标签文本            ← 固定在右上角│
│                          (相对位置)│
│                                    │
│            ┌─────────┐             │
│            │ 绘图数据 │             │
│            └─────────┘             │
│                                    │
└────────────────────────────────────┘
```

## 使用方法

### 1. 创建文本标签

文本内容与外观封装在 `QwtText` 对象中（字体、颜色等），通过 `setText()` 交给标签项，再用 `attach()` 挂载到绘图。下面的例子创建一个固定在画布右上角的说明标签：

```cpp
#include <QwtPlot>
#include <QwtPlotTextLabel>
#include <QwtText>

QwtPlot* plot = new QwtPlot();

// 创建文本标签
QwtPlotTextLabel* label = new QwtPlotTextLabel();

// 设置文本内容
QwtText text("图表说明");
text.setFont(QFont("Arial", 12, QFont::Bold));
text.setColor(Qt::black);
label->setText(text);

// 设置位置（相对画布坐标）
label->setPos(0.9, 0.1);  // 右上角（90%宽度, 10%高度）

// 设置对齐方式
label->setAlignment(Qt::AlignRight | Qt::AlignTop);

label->attach(plot);
plot->replot();
```

### 2. 文本样式配置

视觉样式由 `QwtText` 承载：`setFont()` 设置字体、`setColor()` 设置文字颜色、`setBackgroundBrush()` 设置背景、`setBorderPen()` 设置边框。下面的例子配置一个带浅黄背景和灰色边框的标签：

```cpp
QwtText text;

// 设置字体
text.setFont(QFont("Helvetica", 10));

// 设置颜色
text.setColor(Qt::darkBlue);

// 设置背景色
text.setBackgroundBrush(QBrush(QColor(255, 255, 200)));

// 设置边框
text.setBorderPen(QPen(Qt::gray, 1));

// 设置边距
text.setMargins(5);  // 文本与边框的间距

label->setText(text);
```

### 3. 位置和对齐

标签位置用相对画布的归一化坐标表示（取值 0~1），对齐方式则决定文本矩形的哪个角或哪条边锚定到该点上。下面的代码演示居中标签，并列出其他常用对齐组合：

```cpp
// 设置位置（归一化坐标，范围[0,1]）
label->setPos(0.5, 0.5);  // 画布中心

// 设置对齐（相对于定位点）
label->setAlignment(Qt::AlignCenter);

// 其他对齐选项：
// Qt::AlignLeft | Qt::AlignTop    - 左上
// Qt::AlignRight | Qt::AlignBottom - 右下
// Qt::AlignHCenter | Qt::AlignVCenter - 中心
```

### 4. 旋转文本

需要斜向标注时，可以让文本绕定位点旋转任意角度（单位为度）。下面的例子将标签旋转 45 度：

```cpp
// 设置文本旋转角度
label->setRotation(45.0);  // 45度旋转
```

## 核心方法总结

| 方法 | 说明 |
|------|------|
| `setText()` | 设置文本内容 |
| `setPos()` | 设置位置（归一化坐标） |
| `setAlignment()` | 设置对齐方式 |
| `setRotation()` | 设置旋转角度 |
| `setGeometry()` | 设置几何区域 |

> **提示 — 应用场景**
>
> - 图表标题或说明文字
> - 固定位置标注
> - 单位说明
> - 版本信息显示

> **示例 — 相关示例**
>
> - 文本标签演示：参见自定义示例