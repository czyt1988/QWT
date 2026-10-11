# QwtPlot 布局管理

`QwtPlotLayout` 是 QwtPlot 的布局引擎，负责组织绘图窗口内部各个组件的位置和大小，包括标题、图例、坐标轴和画布区域。

## 主要功能特性

**特性**

- ✅ **自动布局计算**：根据组件内容自动计算合理的空间分配
- ✅ **灵活的间距控制**：支持自定义画布边距和组件间距
- ✅ **画布对齐选项**：可选择画布与坐标轴刻度对齐或保持独立
- ✅ **图例位置配置**：支持图例在四个边界的放置，并可设置比例
- ✅ **导出优化选项**：导出时可忽略滚动条、框架等元素

## 基本概念

### 绘图窗口组成结构

一个 QwtPlot 窗口由以下组件构成：

```text
┌─────────────────────────────────────────────┐
│                  标题区域                    │
├─────────────────────────────────────────────┤
│  ┌───────────┬───────────────┬───────────┐  │
│  │           │               │           │  │
│  │ 左图例    │    画布区域    │ 右图例    │  │
│  │ (可选)    │               │ (可选)    │  │
│  │           │               │           │  │
│  ├───────────┼───────────────┼───────────┤  │
│  │ 左坐标轴  │   底部坐标轴   │ 右坐标轴  │  │
│  ├───────────┴───────────────┴───────────┤  │
│                  页脚区域                    │
└─────────────────────────────────────────────┘
```

### 组件区域获取

QwtPlotLayout 提供方法获取各组件的布局矩形：

| 方法 | 说明 |
|------|------|
| `titleRect()` | 获取标题区域矩形 |
| `footerRect()` | 获取页脚区域矩形 |
| `legendRect()` | 获取图例区域矩形 |
| `scaleRect(QwtAxisId)` | 获取指定坐标轴区域矩形 |
| `canvasRect()` | 获取画布区域矩形 |

## 使用方法

### 设置画布边距

画布边距是指画布与坐标轴之间的空白区域：

```cpp
QwtPlot* plot = new QwtPlot();

// 获取布局对象（QwtPlot内部已创建）
QwtPlotLayout* layout = plot->plotLayout();

// 设置所有坐标轴的画布边距为10像素
layout->setCanvasMargin(10);

// 仅设置左侧Y轴的画布边距
layout->setCanvasMargin(15, QwtAxis::YLeft);

// 获取特定坐标轴的画布边距
int margin = layout->canvasMargin(QwtAxis::XBottom);
```

> **提示 — 边距作用**
>
> 画布边距主要用于控制坐标轴刻度标签与画布边缘的距离，防止刻度标签被裁剪。

### 设置组件间距

间距控制标题、画布、图例、页脚之间的距离：

```cpp
// 设置组件间距为5像素
layout->setSpacing(5);

// 获取当前间距
int spacing = layout->spacing();
```

### 画布与坐标轴对齐

默认情况下，画布边界与坐标轴刻度线对齐，这确保刻度正好落在画布边缘。你也可以取消对齐让画布保持独立：

```cpp
// 设置画布与所有坐标轴对齐（默认行为）
layout->setAlignCanvasToScales(true);

// 设置画布仅与底部X轴对齐
layout->setAlignCanvasToScale(QwtAxis::XBottom, true);

// 取消画布与左侧Y轴的对齐
layout->setAlignCanvasToScale(QwtAxis::YLeft, false);

// 检查特定坐标轴的对齐状态
bool aligned = layout->alignCanvasToScale(QwtAxis::YRight);
```

> **信息 — 对齐效果说明**
>
> - **对齐模式**：画布边界与坐标轴刻度线精确对齐，刻度标签可能延伸到画布外
> - **不对齐模式**：画布保持固定大小，坐标轴区域独立计算，刻度标签完整显示

### 图例位置配置

QwtPlotLayout 支持将图例放置在四个边界位置：

```cpp
// 设置图例在右侧，占比20%
layout->setLegendPosition(QwtPlot::RightLegend, 0.2);

// 设置图例在底部
layout->setLegendPosition(QwtPlot::BottomLegend);

// 仅设置图例位置（不改变比例）
layout->setLegendPosition(QwtPlot::LeftLegend);

// 设置图例比例（占边界空间的百分比）
layout->setLegendRatio(0.15);  // 图例占15%

// 获取当前图例位置和比例
QwtPlot::LegendPosition pos = layout->legendPosition();
double ratio = layout->legendRatio();
```

图例位置选项：

| 位置 | 枚举值 | 说明 |
|------|--------|------|
| 左侧 | `QwtPlot::LeftLegend` | 图例在YLeft轴左侧 |
| 右侧 | `QwtPlot::RightLegend` | 图例在YRight轴右侧 |
| 底部 | `QwtPlot::BottomLegend` | 图例在页脚下方 |
| 顶部 | `QwtPlot::TopLegend` | 图例在标题上方 |

### 布局激活与刷新

布局在 QwtPlot 尺寸变化或组件内容变化时需要重新计算：

```cpp
// 手动激活布局（通常由QwtPlot自动调用）
layout->activate(plot, QRectF(0, 0, 600, 400));

// 使布局失效，强制下次刷新时重新计算
layout->invalidate();
```

### 导出时的布局选项

导出绘图时可以使用特殊选项忽略某些组件：

```cpp
// 导出时忽略标题和图例
QwtPlotLayout::Options options;
options |= QwtPlotLayout::IgnoreTitle;
options |= QwtPlotLayout::IgnoreLegend;

layout->activate(plot, exportRect, options);
```

| 选项 | 说明 |
|------|------|
| `AlignScales` | 对齐坐标轴（未使用） |
| `IgnoreScrollbars` | 忽略滚动条尺寸 |
| `IgnoreFrames` | 忽略所有框架边框 |
| `IgnoreLegend` | 忽略图例 |
| `IgnoreTitle` | 忽略标题 |
| `IgnoreFooter` | 忽略页脚 |

## 自定义布局示例

以下示例展示了如何自定义绘图布局：

```cpp
#include <QwtPlot>
#include <QwtPlotLayout>

// 创建绘图
QwtPlot* plot = new QwtPlot();
plot->setTitle("自定义布局示例");

// 获取布局对象
QwtPlotLayout* layout = plot->plotLayout();

// 设置组件间距
layout->setSpacing(8);

// 设置画布边距
layout->setCanvasMargin(20, QwtAxis::XBottom);
layout->setCanvasMargin(15, QwtAxis::YLeft);

// 画布不对齐坐标轴（保持固定大小）
layout->setAlignCanvasToScales(false);

// 图例放在右侧，占25%宽度
layout->setLegendPosition(QwtPlot::RightLegend, 0.25);

// 刷新布局
plot->replot();
```

> **警告 — 布局变更时机**
>
> 布局设置变更后，需要调用 `plot->replot()` 或等待下次自动刷新才能看到效果。

## 最小尺寸提示

QwtPlotLayout 可以根据组件内容计算最小尺寸：

```cpp
// 获取绘图的最小尺寸提示
QSize minSize = layout->minimumSizeHint(plot);

// 应用最小尺寸
plot->setMinimumSize(minSize);
```

> **示例 — 相关示例**
>
> - 基础布局：所有 `examples/2D/` 下的示例
> - 多绘图布局：`examples/figure`（使用 QwtFigure）