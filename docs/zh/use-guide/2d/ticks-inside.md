# 刻度朝内显示使用指南

`QwtPlot::TickDirection` 控制刻度线的绘制方向，可以把刻度线画在绘图区域内部，而不是默认的轴线外侧。需要紧凑布局或特殊视觉效果时可以用它。

## 主要功能特性

刻度可以朝外（默认）或朝内显示。每个坐标轴（XBottom、XTop、YLeft、YRight）都能独立设置。朝内刻度与朝外刻度共用长度、颜色和线宽，方向可以随时切换，不必重新设置刻度样式。

## 基本概念

### 刻度方向说明

传统绘图中，坐标轴刻度线从轴线向外延伸（朝外显示）。刻度朝内功能则将刻度线从画布边缘向内延伸，实现不同的视觉效果。

```text
刻度朝外（默认）:
    ────┼────┼────┼────┼───→ 主干（轴线）
       │    │    │    │    刻度线（向外延伸）
      0    2    4    6    8  标签

刻度朝内:
    ────┼────┼────┼────┼───→ 主干（轴线）
       │    │    │    │    刻度线（向内延伸）
      0    2    4    6    8  标签（仍在外部）
    ┌──────────────────────┐
    │  ← 刻度线伸入画布    │  画布区域
    └──────────────────────┘
```

### TickDirection 枚举

| 枚举值 | 说明 |
|--------|------|
| `QwtPlot::TickOutside` | 刻度朝外（默认行为） |
| `QwtPlot::TickInside` | 刻度朝内，从画布边缘向内延伸 |

## 使用方法

刻度朝内显示的例子位于:`examples/2D/ticks_inside`，例子截图如下：

![ticks_inside](../../../assets/screenshots/ticks_inside.png)

### 1. 基本使用

设置单个坐标轴的刻度方向：

```cpp
#include <QwtPlot>

QwtPlot* plot = new QwtPlot();

// 设置 YLeft 轴刻度朝内
plot->setAxisTickDirection(QwtAxis::YLeft, QwtPlot::TickInside);

// 设置 XBottom 轴刻度朝内
plot->setAxisTickDirection(QwtAxis::XBottom, QwtPlot::TickInside);

// 刷新显示
plot->replot();

// 效果：YLeft 和 XBottom 轴的刻度线从画布边缘向内显示，
//       主干和标签保持在画布外，与外部刻度样式一致
```

### 2. 查询当前设置

获取坐标轴当前的刻度方向：

```cpp
// 查询 YLeft 轴的刻度方向
QwtPlot::TickDirection dir = plot->axisTickDirection(QwtAxis::YLeft);

if (dir == QwtPlot::TickInside) {
    // 当前刻度朝内
} else {
    // 当前刻度朝外（默认）
}
```

### 3. 恢复默认设置

将刻度方向恢复为默认的朝外显示：

```cpp
// 恢复 YLeft 轴刻度朝外
plot->setAxisTickDirection(QwtAxis::YLeft, QwtPlot::TickOutside);

plot->replot();

// 效果：YLeft 轴刻度恢复为传统的朝外显示
```

### 4. 批量设置所有轴

同时设置多个坐标轴的刻度方向：

```cpp
// 设置所有轴刻度朝内
plot->setAxisTickDirection(QwtAxis::YLeft, QwtPlot::TickInside);
plot->setAxisTickDirection(QwtAxis::YRight, QwtPlot::TickInside);
plot->setAxisTickDirection(QwtAxis::XBottom, QwtPlot::TickInside);
plot->setAxisTickDirection(QwtAxis::XTop, QwtPlot::TickInside);

plot->replot();
```

## 核心方法

| 方法 | 参数 | 说明 |
|------|------|------|
| `setAxisTickDirection(axisId, direction)` | QwtAxisId, TickDirection | 设置指定轴的刻度方向 |
| `axisTickDirection(axisId)` | QwtAxisId | 获取指定轴的刻度方向 |

## 注意事项

> **信息 — 样式继承**
>
> 朝内刻度线自动继承朝外刻度的样式设置，包括：
> - 刻度长度（通过 `QwtScaleDraw::setTickLength()` 设置）
> - 线宽（通过 `QwtScaleDraw::setPenWidthF()` 设置）
> - 颜色（通过 `QwtScaleWidget::setScaleColor()` 设置）

> **警告 — 刷新时机**
>
> 调用 `setAxisTickDirection()` 后会自动触发 `autoRefresh()`，在 `autoReplot` 开启时会自动刷新。如需立即刷新，可调用 `replot()`。

> **提示 — 配合寄生轴使用**
>
> 刻度朝内功能可与寄生轴（Parasite Plot）配合使用，每个寄生绘图可独立设置刻度方向。

## 与 QwtPlotScaleItem 的区别

| 特性 | TickDirection | QwtPlotScaleItem |
|------|---------------|------------------|
| 目的 | 控制主轴刻度方向 | 在画布内添加额外的坐标轴 |
| 刻度样式 | 继承主轴样式 | 可独立设置样式 |
| 显示位置 | 替换/改变主轴刻度 | 作为独立图层叠加 |
| 适用场景 | 需要紧凑布局或特殊效果 | 需要在画布内显示参考刻度 |

> **示例 — 示例代码**
>
> 完整示例位于：`examples/2D/ticks_inside/`
>
> 该示例展示了：
> - 四个坐标轴独立控制
> - 实时切换刻度方向
> - 批量设置功能

## 参考资料

- [坐标轴控件](scale-widget.md) - QwtScaleWidget 详细说明
- [多坐标轴的创建](parasite-axes.md) - 寄生轴功能
- API文档：`QwtPlot::setAxisTickDirection()`