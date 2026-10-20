# 文本标签 - QwtPlotTextLabel

`QwtPlotTextLabel` 用于在绘图上显示文本标签，与 `QwtPlotMarker` 的标签不同，它使用画布几何坐标而非绘图数据坐标定位，适合显示固定的说明文字或标题。

## 主要功能特性

标签对齐到绘图画布，不随坐标轴变换。字体、颜色、背景与边框由 `QwtText` 对象提供，文本在画布矩形内的落点由它的 render flags 决定。`setMargin()` 设置画布内容区与标签矩形的距离。

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

文本内容与外观封装在 `QwtText` 对象中（字体、颜色、对齐标志等），通过 `setText()` 交给标签项，再用 `attach()` 挂载到绘图。标签没有独立的位置设置接口，它按照 `QwtText` 的 render flags 在画布矩形内对齐。下面的例子创建一个固定在画布右上角的说明标签：

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

// 通过 render flags 让标签对齐到画布右上角
text.setRenderFlags(Qt::AlignRight | Qt::AlignTop);
label->setText(text);

// 画布内容区与标签可显示矩形之间保留 10 像素距离
label->setMargin(10);

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

// 设置背景圆角
text.setBorderRadius(4);

label->setText(text);

// 画布内容区与标签之间的距离属于标签项的属性
//（QwtText 没有 margins 接口）：
label->setMargin(5);
```

### 3. 位置和对齐

标签始终覆盖画布矩形（先扣除 `setMargin()` 边距），对齐方式决定文本在这个矩形内的落点；它通过 `QwtText` 的 render flags 配置（可任意组合 `Qt::AlignmentFlag` 取值）。下面的代码演示居中标签，并列出其他常用对齐组合：

```cpp
// 居中标签——对齐方式是 QwtText 的一部分
QwtText text("居中标签");
text.setRenderFlags(Qt::AlignCenter);
label->setText(text);

// 画布内容区与标签矩形的距离
label->setMargin(20);

// 其他对齐选项（QwtText::setRenderFlags）：
// Qt::AlignLeft | Qt::AlignTop      - 左上
// Qt::AlignRight | Qt::AlignBottom  - 右下
// Qt::AlignHCenter | Qt::AlignTop   - 顶部居中
```

### 4. 通过 textRect() 自定义落点

`QwtPlotTextLabel` 不支持旋转文本；需要旋转标注时，请自定义 `QwtPlotItem` 子类，在其 `draw()` 中用 `QPainter::rotate()` 实现。画布内自定义落点的入口是虚函数 `textRect()`：它接收扣除边距后的画布矩形和文本所需尺寸，返回标签实际绘制的矩形。下面的例子把标签钉在右下角：

```cpp
// QwtPlotTextLabel 没有 setRotation()：需要旋转文本时，
// 请继承 QwtPlotItem 并在 draw() 中旋转 painter。

class MyTextLabel : public QwtPlotTextLabel
{
public:
    QRectF textRect(const QRectF& rect, const QSizeF& textSize) const override
    {
        // 把标签钉在右下角，向内留 10 像素
        QRectF r(QPointF(), textSize);
        r.moveBottomRight(rect.bottomRight() - QPointF(10, 10));
        return r;
    }
};

MyTextLabel* label = new MyTextLabel();
label->setText(QwtText("自定义落点"));
label->attach(plot);
```

## 核心方法总结

| 方法 | 说明 |
|------|------|
| `setText()` | 设置文本（`QwtText` 承载字体、颜色、对齐标志、背景） |
| `text()` | 返回当前文本 |
| `setMargin()` | 画布内容区与标签矩形的距离 |
| `margin()` | 返回边距 |
| `textRect()` | 计算标签绘制矩形的虚函数（自定义落点钩子） |

> **提示 — 应用场景**
>
> - 图表标题或说明文字
> - 固定位置标注
> - 单位说明
> - 版本信息显示

> **示例 — 相关示例**
>
> - 文本标签演示：参见自定义示例