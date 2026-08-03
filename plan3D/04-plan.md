# 04-plan: Qwt3DPlotItem 基类创建

> **前置阅读**：执行本任务前，必须完整阅读 `plan3D/ARCHITECTURE-PRINCIPLES.md`，特别是第三章第 3.2 节「Qwt3DPlotItem（item 基类）」、第五章「OpenGL 技术规范」和第九章「不可触碰的约束」。

## 目标（Objective）

创建 `Qwt3DPlotItem` 抽象基类，定义 3D 绘图 item 的核心接口（attach/detach/draw/hull/z/title/itemChanged），为后续 Plot+Item 架构提供 item 基类。同时参考 2D 模块的 `QwtPlotItem` 和 `QwtPlotDict` 模式。

## 涉及文件

| 文件 | 操作 |
|------|------|
| `qwt3d_plotitem.h` | **新建**：item 基类头文件 |
| `qwt3d_plotitem.cpp` | **新建**：item 基类实现 |
| `CMakeLists.txt` | 修改：添加新文件 |

## 前提条件（Prerequisites）

- Task 01–03 已完成（所有非 plot 类已去命名空间并重命名）
- 构建通过

## 执行步骤（Steps）

### 1. 参考 2D 模块的 QwtPlotItem

阅读 `src/plot/qwt_plot_item.h` 和 `src/plot/qwt_plot_dict.h`，理解 2D 的 item 管理模式：
- `QwtPlotItem::attach(QwtPlot*)` / `detach()` / `plot()` / `z()` / `setZ()` / `itemChanged()`
- `QwtPlotDict` 持有 `QList<QwtPlotItem*>`，按 z-order 排序

### 2. 创建 qwt3d_plotitem.h

```cpp
#ifndef QWT3D_PLOTITEM_H
#define QWT3D_PLOTITEM_H

#include "qwt3d_global.h"

#include <QString>

#include "qwt3d_types.h"

class Qwt3DPlot;

class QWT3D_EXPORT Qwt3DPlotItem
{
    QWT_DECLARE_PRIVATE(Qwt3DPlotItem)

public:
    Qwt3DPlotItem();
    virtual ~Qwt3DPlotItem();

    void attach(Qwt3DPlot* plot); // Attach item to a plot
    void detach(); // Detach item from its plot

    Qwt3DPlot* plot() const; // Get the plot this item is attached to

    virtual void draw() = 0; // Draw the item using the current GL context
    virtual ParallelEpiped hull() const = 0; // Get the bounding hull of the item

    void setTitle(const QString& title); // Set the item title
    QString title() const; // Get the item title

    void setZ(double z); // Set the z-order value
    double z() const; // Get the z-order value

    void setVisible(bool on); // Set item visibility
    bool isVisible() const; // Get item visibility

    virtual void itemChanged(); // Notify the plot that the item has changed

private:
    Qwt3DPlotItem(const Qwt3DPlotItem&) = delete;
    Qwt3DPlotItem& operator=(const Qwt3DPlotItem&) = delete;
};

#endif
```

### 3. PrivateData 说明

不创建 `_p.h` 文件。PrivateData 在 `.cpp` 中内联定义（跟随 2D `QwtPlotItem` 模式）。

### 4. 创建 qwt3d_plotitem.cpp

```cpp
#include "qwt3d_plotitem.h"

#include <QString>

class Qwt3DPlotItem::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DPlotItem)
public:
    explicit PrivateData(Qwt3DPlotItem* p);
    Qwt3DPlot* m_plot = nullptr;
    double m_z = 0.0;
    QString m_title;
    bool m_visible = true;
};

Qwt3DPlotItem::PrivateData::PrivateData(Qwt3DPlotItem* p)
    : q_ptr(p)
{
}

Qwt3DPlotItem::Qwt3DPlotItem()
    : QWT_PIMPL_CONSTRUCT
{
}

Qwt3DPlotItem::~Qwt3DPlotItem()
{
    detach();
}

void Qwt3DPlotItem::attach(Qwt3DPlot* plot)
{
    QWT_D(d);
    if (plot == d->m_plot)
        return;
    if (d->m_plot)
        detach();
    d->m_plot = plot;
    // TODO: d->m_plot->attachItem(this); // Enabled in Task 05
}

void Qwt3DPlotItem::detach()
{
    QWT_D(d);
    if (!d->m_plot)
        return;
    // TODO: d->m_plot->detachItem(this); // Enabled in Task 05
    d->m_plot = nullptr;
}

Qwt3DPlot* Qwt3DPlotItem::plot() const
{
    QWT_DC(d);
    return d->m_plot;
}

void Qwt3DPlotItem::setTitle(const QString& title)
{
    QWT_D(d);
    if (d->m_title != title) {
        d->m_title = title;
        itemChanged();
    }
}

QString Qwt3DPlotItem::title() const
{
    QWT_DC(d);
    return d->m_title;
}

void Qwt3DPlotItem::setZ(double z)
{
    QWT_D(d);
    if (d->m_z != z) {
        d->m_z = z;
        itemChanged();
    }
}

double Qwt3DPlotItem::z() const
{
    QWT_DC(d);
    return d->m_z;
}

void Qwt3DPlotItem::setVisible(bool on)
{
    QWT_D(d);
    if (d->m_visible != on) {
        d->m_visible = on;
        itemChanged();
    }
}

bool Qwt3DPlotItem::isVisible() const
{
    QWT_DC(d);
    return d->m_visible;
}

void Qwt3DPlotItem::itemChanged()
{
    QWT_D(d);
    // TODO: if (d->m_plot) d->m_plot->update(); // Enabled in Task 05
}
```

**说明**：Task 04 执行时，Qwt3DPlot 尚未重写（当前类名为 Plot3D），因此 `attach()` / `detach()` 中只做指针存储（`m_plot = plot`），不调用 plot 的方法。`itemChanged()` 中的 `m_plot->update()` 调用同样用 TODO 注释跳过。所有 plot 方法调用在 Task 05 完成 Qwt3DPlot 重写后启用。

### 5. 更新 CMakeLists.txt

在 `CMakeLists.txt` 的 `QWTPLOT3D_HEADER_PLOT_3D` 列表中添加 `qwt3d_plotitem.h`，在 `QWTPLOT3D_SOURCE_PLOT_3D` 列表中添加 `qwt3d_plotitem.cpp`。

### 6. 设计说明

**item 不继承 QObject**：与 `QwtPlotItem` 一致，item 不是 QObject，不需要信号槽。item 通过 `itemChanged()` 通知 plot 重绘。

**item 不继承 QWidget/QOpenGLWidget**：item 是纯数据+绘制逻辑对象，不拥有 GL 上下文。在 `draw()` 中使用当前 GL 上下文（由 `Qwt3DPlot::paintGL()` 设置）。

**z-order**：item 按 `z()` 值排序，`Qwt3DPlot` 在 `paintGL()` 中按 z 从小到大遍历 item 调用 `draw()`。

**自定义 PIMPL**：子类如需自定义 PIMPL，通过在子类中重新声明 `QWT_DECLARE_PRIVATE(SubClass)` 并在子类 .cpp 中定义 `SubClass::PrivateData` 来实现，不需要 protected 构造函数。

## 构建通过

```powershell
.\build.ps1 rebuild -Examples OFF -Playground OFF
```

构建必须零错误零警告通过。此任务只新增文件，不修改现有逻辑，风险低。

## git 提交

```powershell
git add -A
git commit -m "feat(plot3d): add Qwt3DPlotItem base class

- New abstract base class for 3D plot items (attach/detach/draw/hull/z/title)
- Mirrors 2D QwtPlotItem pattern
- Item is not a widget; draw() is called within Qwt3DPlot::paintGL() context
- z-order controls draw sequence"
```
