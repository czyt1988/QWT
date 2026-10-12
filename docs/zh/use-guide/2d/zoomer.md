# 缩放工具

在 Qwt6 中，`QwtPlotZoomer` 需要指定两个坐标轴来进行缩放操作。如果您的绘图需要四个坐标轴都生效，您需要为绘图绑定两个缩放器：

```cpp
// Qwt6 传统用法 - 需要两个缩放器来覆盖四个坐标轴
QwtPlotZoomer* zoomer1 = new QwtPlotZoomer(QwtAxis::XBottom, QwtAxis::YLeft, canvas);
QwtPlotZoomer* zoomer2 = new QwtPlotZoomer(QwtAxis::XTop, QwtAxis::YRight, canvas);
```

为解决这个问题，新版本进行了以下重构：

- **`QwtPlotAxisZoomer`** - 原 `QwtPlotZoomer` 的重命名，代表绑定两个特定坐标轴的缩放器
- **`QwtPlotCanvasZoomer`** - 新增的缩放器，无需指定坐标轴，会对整个画布进行整体缩放

## 缩放器功能对比

下面是这两个缩放器的功能对比：

| 特性 | `QwtPlotAxisZoomer` | `QwtPlotCanvasZoomer` |
|------|-------------------|---------------------|
| 坐标轴绑定 | 需要指定 X 和 Y 轴 | 自动处理所有四个坐标轴 |
| 使用场景 | 特定坐标轴缩放 | 整体画布缩放 |
| 寄生绘图支持 | 需手动绑定 | 自动支持所有寄生绘图 |

## 使用示例

`QwtPlotAxisZoomer`和`QwtPlotCanvasZoomer`的使用方法一致，区别在于`QwtPlotCanvasZoomer`无需指定坐标轴

### QwtPlotAxisZoomer 使用示例

```cpp
#include "qwt_plot_axis_zoomer.h"

// 创建针对特定坐标轴的缩放器
QwtPlotAxisZoomer* axisZoomer = new QwtPlotAxisZoomer(
    QwtAxis::XBottom,  // X 轴
    QwtAxis::YLeft,    // Y 轴
    plot->canvas(),            // 画布
    true               // 自动重绘
);

// 或者使用默认坐标轴
QwtPlotAxisZoomer* defaultZoomer = new QwtPlotAxisZoomer(plot->canvas());
```

### QwtPlotCanvasZoomer 使用示例

```cpp
#include "qwt_plot_canvas_zoomer.h"

// 创建整体画布缩放器，自动处理所有坐标轴
QwtPlotCanvasZoomer* canvasZoomer = new QwtPlotCanvasZoomer(plot->canvas());
```

## 按键设置说明

### 默认按键绑定

#### 鼠标操作

| 操作 | 默认按钮 | 功能描述 |
|------|---------|----------|
| **缩放选择** | 左键拖拽 | 选择矩形区域进行缩放 |
| **回到基准** | 右键点击 | 缩放到基准视图（完全缩小） |
| **后退一级** | 中键点击 | 后退一级缩放 |
| **前进一级** | 中键+Shift点击 | 前进一级缩放 |

#### 键盘操作

| 操作 | 默认按键 | 功能描述 |
|------|---------|----------|
| **前进一级** | `+` | 前进一级缩放 |
| **后退一级** | `-` | 后退一级缩放 |
| **回到基准** | `Escape` | 回到基准视图 |


### 自定义按键配置

你可以通过下面预定义的按钮配置来配置快捷键

支持如下鼠标快捷键设置：
- MouseSelect2: 重置回基础
- MouseSelect3: 缩放栈回退
- MouseSelect6: 缩放栈前进

支持如下键盘快捷键设置：
- KeyUndo: 缩放栈回退
- KeyRedo: 缩放栈前进
- KeyHome: 重置回基础

```cpp
// 配置自定义按键
zoomer->setKeyPattern(QwtEventPattern::KeyRedo, Qt::Key_Plus,Qt::ShiftModifier);    // 缩放栈前进(放大)
zoomer->setKeyPattern(QwtEventPattern::KeyUndo, Qt::Key_Minus,Qt::ShiftModifier);   // 缩放栈回退(缩小)
zoomer->setKeyPattern(QwtEventPattern::KeyHome, Qt::Key_Escape);  // 重置

// 配置鼠标模式,中间变为重置回基础，右键为回退
zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::MiddleButton);
zoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);
```