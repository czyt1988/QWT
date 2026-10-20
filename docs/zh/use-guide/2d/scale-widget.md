# 坐标轴控件 - QwtScaleWidget

`QwtScaleWidget` 是坐标轴的显示控件，负责绘制刻度线、刻度标签和轴标题。它是 QwtPlot 坐标轴系统的基础组件，也支持独立使用。

## 主要功能特性

- 绘制主刻度、次刻度和刻度标签
- 显示坐标轴标题文字
- 将标题以水平文字画在 scale widget 正下方或正上方（`TitleOutside`）
- 鼠标拖动平移和滚轮缩放（Qwt7 新增）
- 显示颜色条（用于光谱图）
- 自定义刻度长度、标签字体等样式

## 基本概念

### 坐标轴位置

Qwt 使用 `QwtAxis::Position` 枚举标识坐标轴位置：

| 位置 | 枚举值 | 说明 |
|------|--------|------|
| 底部X轴 | `QwtAxis::XBottom` | 底部水平轴 |
| 顶部X轴 | `QwtAxis::XTop` | 顶部水平轴 |
| 左侧Y轴 | `QwtAxis::YLeft` | 左侧垂直轴 |
| 右侧Y轴 | `QwtAxis::YRight` | 右侧垂直轴 |

### 坐标轴组成结构

坐标轴由带刻度线的骨干线、旁边的刻度标签以及轴标题组成；刻度分为长的主刻度和短的次刻度两种。下面的示意图给出一根水平轴的组成结构：

```text
          标题
         时间(s)
            ↓
    ────────┼────┼────┼────┼────→ 刻度线
           0    2    4    6    8   刻度标签
           
刻度分为：主刻度（长）、次刻度（短）
```

## 使用方法

### 1. 通过QwtPlot设置坐标轴

大多数坐标轴设置直接通过 `QwtPlot` 完成：`setAxisTitle()` 设置轴标题、`setAxisScale()` 固定取值范围、`setAxisVisible()` 控制显隐，均以 `QwtAxis::Position` 指定目标轴。下面的例子配置底部 X 轴与左侧 Y 轴：

```cpp
#include <QwtPlot>

QwtPlot* plot = new QwtPlot();

// 设置坐标轴标题
plot->setAxisTitle(QwtAxis::XBottom, "时间 (s)");
plot->setAxisTitle(QwtAxis::YLeft, "电压 (V)");

// 设置坐标轴范围
plot->setAxisScale(QwtAxis::XBottom, 0, 100);  // X轴0-100
plot->setAxisScale(QwtAxis::YLeft, -10, 10);   // Y轴-10到10

// 设置坐标轴可见性
plot->setAxisVisible(QwtAxis::XTop, false);    // 隐藏顶部X轴
plot->setAxisVisible(QwtAxis::YRight, true);   // 显示右侧Y轴

plot->replot();
```

### 2. 获取坐标轴控件

需要更精细的控制时，可用 `QwtPlot::axisWidget()` 取得坐标轴底层的控件；它是 `QwtScaleWidget`，用 `setFont()` 设置刻度标签字体、`setTitle()` 传入带样式的 `QwtText`，用 `setScaleColor()`/`setTextColor()` 分别设置骨干线/刻度与刻度标签的颜色。下面的例子对底部轴控件做样式定制：

```cpp
// 获取特定位置的坐标轴控件
QwtScaleWidget* scaleWidget = plot->axisWidget(QwtAxis::XBottom);

// 设置刻度标签字体
scaleWidget->setFont(QFont("Arial", 8));

// 设置轴标题样式
QwtText title("时间 (s)");
title.setFont(QFont("Arial", 10, QFont::Bold));
title.setColor(Qt::darkBlue);
scaleWidget->setTitle(title);

// 设置刻度与标签颜色
scaleWidget->setScaleColor(Qt::black);   // 骨干线与刻度线
scaleWidget->setTextColor(Qt::darkGray); // 刻度标签
```

### 3. 内置交互功能（Qwt7新增）

Qwt 7.0 新增了坐标轴内置交互功能：点击激活坐标轴后，拖动即可平移、滚轮即可缩放对应轴的刻度范围。功能通过 `setBuildinActions()` 以标志位方式开启（`ActionNone`、`ActionClickPan`、`ActionWheelZoom`、`ActionAll`）：

```cpp
// 获取坐标轴控件
QwtScaleWidget* scaleWidget = plot->axisWidget(QwtAxis::XBottom);

// 启用内置的点击拖动平移和滚轮缩放
scaleWidget->setBuildinActions(QwtScaleWidget::ActionClickPan | QwtScaleWidget::ActionWheelZoom);

// 或一次性启用全部内置动作
// scaleWidget->setBuildinActions(QwtScaleWidget::ActionAll);

// 每次滚轮缩放的系数（默认 1.2）
scaleWidget->setZoomFactor(1.2);

// 坐标轴被选中时的高亮颜色
scaleWidget->setSelectionColor(Qt::blue);
```

平移效果：

![qwt-scale-builtin-action-pan](../../../assets/screenshots/qwt-scale-builtin-action-pan.gif)

缩放效果：

![qwt-scale-builtin-action-zoom](../../../assets/screenshots/qwt-scale-builtin-action-zoom.gif)

详细的坐标轴交互说明请参考：[坐标轴交互动作](scale-builtin-action.md)

### 4. 颜色条显示

用于光谱图等需要颜色映射的场景。颜色条没有独立的位置设置接口，它画在哪一侧由刻度绘制（scale draw）的对齐方式决定：

```cpp
// 启用颜色条
scaleWidget->setColorBarEnabled(true);

// 设置颜色条宽度
scaleWidget->setColorBarWidth(20);

// 为取值区间设置颜色映射（控件接管 colorMap 所有权）
QwtLinearColorMap* colorMap = new QwtLinearColorMap(Qt::blue, Qt::red);
scaleWidget->setColorMap(QwtInterval(0, 100), colorMap);
```

### 5. 刻度绘制样式

刻度线外观不在 `QwtScaleWidget` 本身设置：刻度朝向（画布外侧/内侧）是 `QwtPlot` 的按轴配置，刻度长度与刻度-标签间距则属于 `scaleWidget->scaleDraw()` 返回的 `QwtScaleDraw` 对象。下面的代码演示这些设置：

```cpp
// 刻度朝向：QwtPlot 的按轴设置
//（另见《向内刻度使用指南》）
plot->setAxisTickDirection(QwtAxis::XBottom, QwtPlot::TickOutside);
// 其他选项：QwtPlot::TickInside（刻度画进画布内侧）

// 刻度长度：在刻度绘制对象上按刻度类型设置
scaleWidget->scaleDraw()->setTickLength(QwtScaleDiv::MajorTick, 10.0);  // 主刻度长度（像素）
scaleWidget->scaleDraw()->setTickLength(QwtScaleDiv::MediumTick, 7.0);  // 中刻度长度
scaleWidget->scaleDraw()->setTickLength(QwtScaleDiv::MinorTick, 5.0);   // 次刻度长度

// 刻度与刻度标签的间距（默认 4 像素）
scaleWidget->scaleDraw()->setSpacing(2.0);
```

### 6. 刻度标签格式化

通过设置 `QwtScaleDraw` 自定义刻度标签格式：

```cpp
#include <QwtScaleDraw>

// 自定义刻度绘制
class MyScaleDraw : public QwtScaleDraw
{
public:
    virtual QwtText label(double value) const override
    {
        // 自定义标签格式
        return QwtText(QString::number(value, 'f', 1));  // 保留1位小数
    }
};

// 应用自定义刻度绘制
scaleWidget->setScaleDraw(new MyScaleDraw());
```

### 7. 设置刻度划分

刻度划分既可以由刻度引擎通过 `QwtPlot::setAxisAutoScale()` 自动计算，也可以手动构建：为 `QwtScaleDiv` 设置区间并填入 `QwtScaleDiv::MajorTick` 位置列表，再用 `QwtPlot::setAxisScaleDiv()` 交给坐标轴。下面的例子演示这两种方式：

```cpp
#include <QwtScaleDiv>
#include <QwtScaleEngine>

// 使用自动刻度引擎
plot->setAxisAutoScale(QwtAxis::XBottom);  // 自动计算刻度

// 手动设置刻度划分
QwtScaleDiv scaleDiv;
scaleDiv.setInterval(0, 100);
QList<double> majorTicks;
majorTicks << 0 << 25 << 50 << 75 << 100;
scaleDiv.setTicks(QwtScaleDiv::MajorTick, majorTicks);

plot->setAxisScaleDiv(QwtAxis::XBottom, scaleDiv);
```

### 8. 坐标轴外置标题（TitleOutside）

默认情况下轴标题画在 scale widget 内部（垂直轴为旋转文字）。使用 `QwtScaleWidget::TitleOutside` 后，标题改为以水平文字画在 scale widget 外侧的字幕条中：`YLeft`/`YRight`/`XBottom` 在其正下方，`XTop` 在其正上方。布局会自动预留字幕条空间，并在每次重新布局（窗口缩放、轴显隐、刻度变化等）时保持字幕条跟随 scale widget。

```cpp
#include <QwtPlot>
#include <QwtScaleWidget>

QwtPlot* plot = new QwtPlot();
plot->setAxisTitle(QwtAxis::YLeft, "电压 (V)");
plot->setAxisTitle(QwtAxis::YRight, "电流 (mA)");

// 将 Y 轴标题以水平文字画在各自 scale widget 正下方
plot->setAxisTitlePlacement(QwtAxis::YLeft, QwtScaleWidget::TitleOutside);
plot->setAxisTitlePlacement(QwtAxis::YRight, QwtScaleWidget::TitleOutside);

// 等价地通过 scale widget 设置（此处：X 轴标题画在底部刻度下方）
plot->axisWidget(QwtAxis::XBottom)->setTitlePlacement(QwtScaleWidget::TitleOutside);

// 随时切回传统的 widget 内部标题
plot->setAxisTitlePlacement(QwtAxis::YLeft, QwtScaleWidget::TitleInside);
```

效果（来自示例 `examples/2D/outsideTitle`；左：单 plot，右：带 parasite 层的多轴图，每层标题位于各自轴列的正下方）：

![outside-title](../../../assets/screenshots/outside-title.png)

行为说明

- `TitleOutside` 模式下标题不再计入轴维度，改由布局在相邻带区预留字幕条；`QwtPlotLayout::scaleCaptionRect()` 可查询该字幕条的几何。
- 多坐标轴（见[多坐标轴的创建](parasite-axes.md)）：所有层共享底部带区。宿主布局聚合所有层的字幕高度需求，并以相邻轴列中心为界在层间分区，因此字幕始终位于本列正下方且互不重叠。
- 字幕文字的水平对齐遵循 `setTitleAlignment()`；`setTitlePosition()`（沿骨干线的位置）仅对 `TitleInside` 生效。
- 字幕文字宽于其分区时会自动换行，窄列的多轴场景建议使用短标题。
- `QwtPlotRenderer` 的 PNG/SVG/PDF 导出包含外置标题（`renderScaleCaption()`）。

## 核心方法总结

### QwtPlot坐标轴方法

| 方法 | 说明 |
|------|------|
| `setAxisTitle()` | 设置轴标题 |
| `setAxisTitlePlacement()` | 设置标题位置（widget 内部或外置字幕） |
| `setAxisScale()` | 设置轴范围 |
| `setAxisAutoScale()` | 启用自动刻度 |
| `setAxisVisible()` | 设置轴可见性 |
| `setAxisTickDirection()` | 设置刻度朝向（`TickOutside`/`TickInside`） |
| `axisWidget()` | 获取轴控件 |
| `setAxisMaxMajor()` | 设置主刻度数量上限 |
| `setAxisMaxMinor()` | 设置次刻度数量上限 |

### QwtScaleWidget方法

| 方法 | 说明 |
|------|------|
| `setTitle()` | 设置轴标题 |
| `setTitlePlacement()` | 设置标题位置（`TitleInside`/`TitleOutside`） |
| `setFont()` | 设置标签字体 |
| `setScaleColor()` / `setTextColor()` | 设置骨干线/刻度与标签颜色 |
| `setBuildinActions()` | 启用内置交互（`ActionClickPan`/`ActionWheelZoom`/`ActionAll`） |
| `setColorBarEnabled()` | 启用颜色条 |
| `setColorMap()` | 设置颜色映射 |
| `scaleDraw()` | 访问刻度绘制对象（刻度长度用 `setTickLength()`） |
| `setScaleDraw()` | 设置刻度绘制对象 |

> **提示 — 坐标轴配置建议**
>
> - 对于数值轴，使用自动刻度即可满足大多数需求
> - 对于时间轴，使用 `QwtDateScaleEngine`
> - 对于对数轴，使用 `QwtLogScaleEngine`

> **示例 — 相关示例**
>
> - 坐标轴交互：`examples/2D` 中多个示例
> - 外置轴标题：`examples/2D/outsideTitle`
> - 颜色条：`examples/2D/spectrogram`
> - 刻度演示：`playground/scaleengine`