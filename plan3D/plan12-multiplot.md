请先阅读 `README.md` 以了解全局规范。

# plan12 — MultiPlot 多绘图布局

## 目标

实现 `MultiPlot` 类（当前为 stub，仅有空构造函数和纯虚 `createData()`）。MultiPlot 允许在同一个 QOpenGLWidget 画布中渲染多个 3D 子绘图，支持视口分区、共享/独立坐标系、独立旋转或联动旋转。类似 2D 模块的 `QwtFigure` 网格布局，但面向 3D 绘图。

## 前置依赖

- 其他 3D 绘图类型需先可用（SurfacePlot 已有，plan03-11 的新类型）
- **plan00**（渲染基础设施）：使用现代 OpenGL 渲染基础设施

## 涉及文件

### 新建文件

| 文件 | 说明 |
|------|------|
| `src/plot3d/qwt3d_multiplot.cpp` | MultiPlot 类实现（当前仅有 .h stub） |
| `src/plot3d/qwt3d_multiplot_p.h` | PIMPL 私有数据 |
| `examples/3D/multiplot/multiplot.cpp` | 示例程序 |
| `examples/3D/multiplot/CMakeLists.txt` | 示例构建配置 |

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/plot3d/qwt3d_multiplot.h` | 扩展 public/protected API 声明（当前为空 stub） |
| `src/plot3d/CMakeLists.txt` | 添加新源文件 |
| `examples/CMakeLists.txt` | 注册新示例 |

---

## 技术方案

### 1. 设计定位

MultiPlot 与 Plot3D 的关系：

- MultiPlot 继承 Plot3D（因为它本身是一个 QOpenGLWidget）
- 但 MultiPlot 不直接渲染数据，而是管理多个子 Plot3D 实例
- MultiPlot 的 `createData()` 遍历所有子绘图，在各自的视口区域调用子绘图的渲染

与 2D 模块 `QwtFigure` 的对比：

| 特性 | QwtFigure (2D) | MultiPlot (3D) |
|------|----------------|----------------|
| 子绘图类型 | QwtPlot (2D) | Plot3D 及其子类 |
| 布局方式 | 网格(QGridLayout) | 视口分区(glViewport) |
| 共享画布 | 各子绘图独立 QWidget | 共享 QOpenGLWidget |
| 联动交互 | 通过信号槽 | 可选联动旋转/缩放 |

### 2. 核心架构挑战与解决方案

多个 3D 绘图共享同一个 QOpenGLWidget 面临以下挑战：

| 挑战 | 解决方案 |
|------|---------|
| **子绘图是 QOpenGLWidget 但不作为 widget 显示** | 子绘图通过 `addSubPlot()` 接管后，其 `initializeGL()`/`paintGL()` 不由 Qt 调用。MultiPlot 在 `initializeGL()` 中手动调用子绘图的 `initializeSubPlot()` 方法完成 GL 资源初始化。子绘图的 QWidget 部分（窗口创建、事件循环）被忽略，仅使用其数据管理和渲染逻辑。 |
| **视口管理** | 每次渲染子绘图前，通过 `glViewport()` 设置该子绘图的视口区域，渲染后恢复。 |
| **变换隔离** | 每个子绘图有独立的旋转/平移/缩放。渲染前 `glPushMatrix()`/`glPushAttrib()`，渲染后恢复。Core Profile 下手动保存/恢复 GL 状态（详见 §6）。 |
| **坐标系独立** | 每个子绘图维护独立的坐标系（坐标轴范围、刻度、样式）。 |
| **鼠标交互路由** | 鼠标事件根据像素位置确定目标子绘图，转换坐标后转发。 |

**架构决策**：不提取独立的 `Plot3DRenderer` 类（虽然更干净但改动量太大），而是在 `Plot3D` 基类上增加 `renderInViewport()` 和 `initializeSubPlot()` 两个公共方法。这是对 Plot3D 的侵入性修改，但改动范围可控（仅添加方法，不修改现有行为）。

### 3. 类设计

```cpp
namespace Qwt3D {

/// @brief Viewport layout mode
enum class MultiLayout {
    Grid,       ///< Regular grid (rows x columns)
    Horizontal, ///< Horizontal strip
    Vertical,   ///< Vertical strip
    Custom      ///< Custom viewport positions (user-specified)
};

/// @brief Link mode for sub-plot interactions
enum class LinkMode {
    None,       ///< Each sub-plot independent
    Linked,     ///< Rotation/scale synchronized across all sub-plots
    LinkedRotation  ///< Only rotation linked, scale independent
};

/// @brief Sub-plot entry in MultiPlot
struct SubPlot {
    Plot3D* plot;           ///< The sub-plot widget (NOT a child QWidget, but a rendering object)
    QRectF viewport;        ///< Relative viewport [0,1] x [0,1]
    QString title;          ///< Optional sub-plot title
};

/// @brief Multi-plot widget for combining multiple 3D plots in one canvas
class QWT3D_EXPORT MultiPlot : public Plot3D
{
    Q_OBJECT

public:
    explicit MultiPlot(QWidget* parent = nullptr);
    ~MultiPlot() override;

    /// @brief Add a sub-plot
    /// @param plot Sub-plot object (MultiPlot takes ownership)
    /// @param title Optional sub-plot title
    /// @return Index of the added sub-plot
    int addSubPlot(Plot3D* plot, const QString& title = QString());

    /// @brief Add a sub-plot with explicit viewport position
    /// @param viewport Relative viewport [0,1] x [0,1]
    int addSubPlot(Plot3D* plot, QRectF const& viewport,
                   const QString& title = QString());

    /// @brief Remove a sub-plot by index
    void removeSubPlot(int index);

    /// @brief Get sub-plot by index
    Plot3D* subPlot(int index) const;

    /// @brief Get number of sub-plots
    int subPlotCount() const;

    /// @brief Set grid layout
    /// @param rows Number of rows
    /// @param cols Number of columns
    void setGridLayout(unsigned rows, unsigned cols);

    /// @brief Set layout mode
    void setLayout(MultiLayout layout);
    MultiLayout layout() const;

    /// @brief Set link mode for interaction synchronization
    void setLinkMode(LinkMode mode);
    LinkMode linkMode() const;

    /// @brief Set gap between sub-plots (in pixels)
    void setGap(int pixels);
    int gap() const;

protected:
    QWT_DECLARE_PRIVATE(MultiPlot)

    void calculateHull() override;
    void createData() override;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

private:
    /// @brief Compute viewport rectangles for all sub-plots
    void updateLayout();

    /// @brief Find the sub-plot at the given pixel position
    /// @return Sub-plot index or -1 if none
    int subPlotAt(QPoint const& pos) const;

    /// @brief Propagate interaction to linked sub-plots
    void propagateInteraction(int sourceIndex, double xRot, double yRot,
                              double zRot, double xScale, double yScale,
                              double zScale);
};

}  // namespace Qwt3D
```

### 4. 子绘图管理

MultiPlot 的子绘图不是独立的 QWidget（它们不直接显示在屏幕上），而是作为"渲染对象"被 MultiPlot 管理：

```cpp
// Sub-plot ownership model:
// - User creates a Plot3D subclass (e.g., ScatterPlot) without a parent widget
// - User calls multiPlot->addSubPlot(scatterPlot)
// - MultiPlot takes ownership (deletes on destruction)
// - MultiPlot calls sub-plot's rendering methods within its own paintGL()

// IMPORTANT: Sub-plots must NOT be added to any layout or shown as independent widgets
// They render into the MultiPlot's OpenGL context via glViewport partitioning
```

### 5. 渲染流程

```cpp
void MultiPlot::paintGL()
{
    QWT_D(d);

    // Clear the entire canvas
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Save the full viewport
    GLint fullViewport[4];
    glGetIntegerv(GL_VIEWPORT, fullViewport);

    int canvasW = width();
    int canvasH = height();

    for (int i = 0; i < static_cast<int>(d->m_subPlots.size()); ++i) {
        auto const& sp = d->m_subPlots[i];

        // Compute pixel viewport for this sub-plot
        QRect pixelVP;
        pixelVP.setX(static_cast<int>(sp.viewport.x() * canvasW) + d->m_gap);
        pixelVP.setY(static_cast<int>((1.0 - sp.viewport.bottom()) * canvasH) + d->m_gap);
        pixelVP.setWidth(static_cast<int>(sp.viewport.width() * canvasW) - 2 * d->m_gap);
        pixelVP.setHeight(static_cast<int>(sp.viewport.height() * canvasH) - 2 * d->m_gap);

        // Set viewport for this sub-plot
        glViewport(pixelVP.x(), pixelVP.y(), pixelVP.width(), pixelVP.height());

        // Clear depth buffer for this sub-plot
        glClear(GL_DEPTH_BUFFER_BIT);

        // Save GL state
        glPushMatrix();

        // Render the sub-plot
        // Note: Sub-plot's createData() should have been called already
        // We call its display list or VBO rendering here
        sp.plot->renderInViewport(pixelVP);

        // Restore GL state
        glPopMatrix();

        // Draw sub-plot title (optional, using Label system)
        if (!sp.title.isEmpty()) {
            // Draw title at the top of the sub-plot viewport
        }

        // Draw border around sub-plot (optional)
        if (d->m_drawBorders) {
            // Draw a rectangle around the sub-plot
        }
    }

    // Restore full viewport
    glViewport(fullViewport[0], fullViewport[1], fullViewport[2], fullViewport[3]);
}
```

### 6. Plot3D 基类扩展 — 子绘图渲染接口

**这是对 Plot3D 基类的侵入性修改**，需要仔细评估对现有 SurfacePlot 等子类的影响。

在 Plot3D 中添加以下两个公共方法：

```cpp
// 新增到 qwt3d_plot.h 的 public 区域:

/// @brief Initialize GL resources for use as a sub-plot (called by MultiPlot)
/// @details Performs the same initialization as initializeGL() but without
///          creating a standalone GL context. Must be called within MultiPlot's
///          GL context (i.e., from MultiPlot::initializeGL()).
void initializeSubPlot();

/// @brief Render the plot within a specific viewport (called by MultiPlot)
/// @param viewport Pixel viewport rectangle in the parent widget's coordinates
/// @param canvasWidth, canvasHeight Parent widget dimensions (for aspect ratio)
void renderInViewport(QRect const& viewport, int canvasWidth, int canvasHeight);
```

#### `initializeSubPlot()` 实现

```cpp
void Plot3D::initializeSubPlot()
{
    // Perform the same GL setup as initializeGL(), but:
    // 1. Don't call makeCurrent() (already current in MultiPlot's context)
    // 2. Don't set up widget-level event handlers
    initializeOpenGLFunctions();
    RenderContext::instance().detect();
    // Initialize display lists, coordinate system, etc.
    m_initialized = true;
}
```

#### `renderInViewport()` 实现

```cpp
void Plot3D::renderInViewport(QRect const& viewport, int canvasWidth, int canvasHeight)
{
    // Save GL state (compat profile only — see Core Profile note below)
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushMatrix();

    // Set viewport (flip Y: Qt top-left → GL bottom-left)
    int glY = canvasHeight - viewport.bottom() - 1;
    glViewport(viewport.x(), glY, viewport.width(), viewport.height());

    // Set scissor to clip rendering to this viewport
    glEnable(GL_SCISSOR_TEST);
    glScissor(viewport.x(), glY, viewport.width(), viewport.height());
    glClear(GL_DEPTH_BUFFER_BIT);

    // Set up projection and modelview (reuse existing Plot3D logic)
    setupProjection(viewport.width(), viewport.height());
    setupModelView();

    // Render data
    if (hasData()) {
        auto const& lists = displayLists();
        if (lists.size() > DataObject && lists[DataObject] != 0)
            glCallList(lists[DataObject]);
    }

    // Render coordinate system and enrichments
    m_coordinates.draw();
    createEnrichments();

    // Restore GL state
    glDisable(GL_SCISSOR_TEST);
    glPopMatrix();
    glPopAttrib();
}
```

**Core Profile 注意事项**：`glPushAttrib`/`glPopAttrib` 和 `glPushMatrix`/`glPopMatrix` 在 Core Profile 下不可用。Core Profile 下的替代方案是手动保存/恢复 GL 状态结构体：

```cpp
struct GLStateSnapshot {
    GLuint currentProgram, currentVAO, currentDrawFBO;
    GLint viewport[4], scissorBox[4];
    GLboolean depthTest, blend, cullFace, scissorTest;
};
```

初始版本仅支持兼容模式（使用 `glPushAttrib`），Core Profile 状态管理作为后续优化。

### 7. 交互路由

鼠标事件需要路由到正确的子绘图：

```cpp
void MultiPlot::mousePressEvent(QMouseEvent* e)
{
    QWT_D(d);
    QPoint pos = qwt::compat::eventPos(e);

    int idx = subPlotAt(pos);
    if (idx >= 0) {
        d->m_activeSubPlot = idx;
        // Convert global position to sub-plot local position
        QRect vp = subPlotViewport(idx);
        QPoint localPos(pos.x() - vp.x(), pos.y() - vp.y());
        // Forward to sub-plot's mouse handling
        d->m_subPlots[idx].plot->handleMousePress(localPos, e->button(), e->modifiers());
    }
}

void MultiPlot::mouseMoveEvent(QMouseEvent* e)
{
    QWT_D(d);
    if (d->m_activeSubPlot >= 0) {
        QPoint pos = qwt::compat::eventPos(e);
        QRect vp = subPlotViewport(d->m_activeSubPlot);
        QPoint localPos(pos.x() - vp.x(), pos.y() - vp.y());

        auto* plot = d->m_subPlots[d->m_activeSubPlot].plot;
        plot->handleMouseMove(localPos);

        // If linked, propagate rotation to other sub-plots
        if (d->m_linkMode != LinkMode::None) {
            propagateInteraction(d->m_activeSubPlot,
                                 plot->xRotation(), plot->yRotation(),
                                 plot->zRotation(),
                                 d->m_linkMode == LinkMode::Linked ? plot->xScale() : 1.0,
                                 d->m_linkMode == LinkMode::Linked ? plot->yScale() : 1.0,
                                 d->m_linkMode == LinkMode::Linked ? plot->zScale() : 1.0);
        }
    }
    update();  // Trigger repaint
}

void MultiPlot::propagateInteraction(int sourceIndex, double xRot, double yRot,
                                      double zRot, double xScale, double yScale,
                                      double zScale)
{
    QWT_D(d);
    for (int i = 0; i < static_cast<int>(d->m_subPlots.size()); ++i) {
        if (i == sourceIndex) continue;
        auto* plot = d->m_subPlots[i].plot;
        plot->setRotation(xRot, yRot, zRot);
        if (d->m_linkMode == LinkMode::Linked) {
            plot->setScale(xScale, yScale, zScale);
        }
    }
}
```

### 8. 布局算法

#### 8.1 网格布局

```cpp
void MultiPlot::setGridLayout(unsigned rows, unsigned cols)
{
    QWT_D(d);
    d->m_rows = rows;
    d->m_cols = cols;
    updateLayout();
}

void MultiPlot::updateLayout()
{
    QWT_D(d);
    if (d->m_layout == MultiLayout::Grid) {
        double cellW = 1.0 / d->m_cols;
        double cellH = 1.0 / d->m_rows;
        for (size_t i = 0; i < d->m_subPlots.size() && i < d->m_rows * d->m_cols; ++i) {
            unsigned row = i / d->m_cols;
            unsigned col = i % d->m_cols;
            d->m_subPlots[i].viewport = QRectF(
                col * cellW, 1.0 - (row + 1) * cellH,  // x, y (bottom-left origin)
                cellW, cellH);                          // width, height
        }
    }
    // ... handle Horizontal, Vertical, Custom layouts
}
```

#### 8.2 水平/垂直布局

```cpp
// Horizontal: n sub-plots side by side, each 1/n width, full height
// Vertical: n sub-plots stacked, each 1/n height, full width
```

### 9. 数据更新

当子绘图的数据变化时，通过 `GraphPlot::dataChanged()` 信号（plan02 中定义）触发 MultiPlot 重绘：

```cpp
int MultiPlot::addSubPlot(Plot3D* plot, const QString& title)
{
    QWT_D(d);
    SubPlot sp;
    sp.plot = plot;
    sp.title = title;

    int index = static_cast<int>(d->m_subPlots.size());
    d->m_subPlots.push_back(sp);

    // Connect to sub-plot's dataChanged signal (if it's a GraphPlot subclass)
    auto* graphPlot = dynamic_cast<GraphPlot*>(plot);
    if (graphPlot) {
        connect(graphPlot, &GraphPlot::dataChanged, this, [this, index]() {
            // Re-create data for this sub-plot and trigger repaint
            d->m_subPlots[index].plot->updateData();
            update();
        });
    }

    // For non-GraphPlot sub-plots (e.g., SurfacePlot, IsosurfacePlot),
    // the user must manually call multiPlot->update() after changing sub-plot data.

    // Initialize sub-plot GL resources if MultiPlot is already initialized
    if (d->m_initialized) {
        plot->initializeSubPlot();
    }

    updateLayout();
    updateData();
    update();

    return index;
}
```

**子绘图初始化时序**：

```cpp
void MultiPlot::initializeGL()
{
    QWT_D(d);

    // Initialize MultiPlot's own GL resources
    initializeOpenGLFunctions();
    RenderContext::instance().detect();

    // Initialize all sub-plots' GL resources in our context
    for (auto& sp : d->m_subPlots) {
        sp.plot->initializeSubPlot();
    }

    d->m_initialized = true;
}
```

### 10. 与 Plot3D::createData() 的关系

MultiPlot 的 `createData()` 遍历所有子绘图，调用它们的数据构建方法：

```cpp
void MultiPlot::createData()
{
    QWT_D(d);
    for (auto& sp : d->m_subPlots) {
        // Trigger sub-plot's data creation
        // This calls calculateHull() and createData() on the sub-plot
        sp.plot->updateData();
    }
}
```

MultiPlot 自身不创建显示列表（它不直接渲染数据），而是在 `paintGL()` 中逐个渲染子绘图。

---

## 验证方式

1. **编译验证**：`.\build.ps1 build`
2. **示例验证**：运行 `examples/3D/multiplot`，展示：
   - 2×2 网格布局，4 个子绘图：
     - SurfacePlot（表面图）
     - ScatterPlot（散点图，依赖 plan03）
     - LinePlot3D（线图，依赖 plan05）
     - VectorFieldPlot（向量场，依赖 plan06）
   - 切换 Grid/Horizontal/Vertical 布局
   - 切换联动旋转开/关
   - 鼠标点击不同子绘图区域，验证交互路由正确
3. **渲染隔离验证**：每个子绘图的视口正确隔离，变换不互相影响
4. **交互验证**：
   - 在子绘图 A 中旋转，验证 A 的视角变化
   - 联动模式下，旋转 A 时 B/C/D 同步旋转
   - 独立模式下，旋转 A 时 B/C/D 不变
5. **性能验证**：4 个子绘图同时渲染，帧率 > 30fps
6. **缩放验证**：窗口缩放时，子绘图视口正确重新计算

---

## 风险与注意事项

1. **子绘图不是独立 Widget**：MultiPlot 的子绘图不创建独立窗口，它们在 MultiPlot 的 QOpenGLWidget 上下文中渲染。这意味着：
   - 子绘图不能独立显示（不能调用 `show()`）
   - 子绘图的 `paintGL()` 不被 Qt 直接调用（由 MultiPlot 的 `paintGL()` 间接调用）
   - 子绘图的 `initializeGL()` 需要在 MultiPlot 的 `initializeGL()` 中被调用

2. **GL 状态泄漏**：子绘图渲染时可能修改 GL 状态（如 `glEnable(GL_LIGHTING)`、`glDepthMask`等），影响后续子绘图渲染。每个子绘图渲染前后必须保存/恢复完整 GL 状态。使用 `glPushAttrib(GL_ALL_ATTRIB_BITS)` / `glPopAttrib()`（兼容模式）或手动保存/恢复（Core Profile）。

3. **显示列表 vs VBO**：现有 SurfacePlot 使用显示列表。显示列表在共享上下文中有效，但 VBO 需要在正确上下文中创建。MultiPlot 的 `initializeGL()` 中应确保所有子绘图的 GL 资源已初始化。

4. **坐标轴标签重叠**：相邻子绘图的坐标轴标签可能重叠。应通过 `gap` 参数提供间距，或在布局算法中考虑标签宽度。

5. **子绘图生命周期**：MultiPlot 获取子绘图所有权（通过 `addSubPlot(Plot3D*)`），在析构时删除所有子绘图。如果用户需要保留子绘图引用，需在 MultiPlot 析构前调用 `removeSubPlot()` 取消所有权。

6. **与 QwtFigure 的关系**：2D 模块的 `QwtFigure` 是独立的多绘图布局容器（使用 QGridLayout 管理多个 QwtPlot widget）。3D 的 MultiPlot 不同——它使用单个 QOpenGLWidget + 视口分区，因为多个 QOpenGLWidget 的 GL 上下文共享在跨平台上不可靠。

7. **鼠标坐标转换**：鼠标事件的坐标是相对于 MultiPlot widget 的（Qt 坐标系：原点在左上角，y 轴向下）。子绘图的视口矩形也在 Qt 坐标系中定义。因此**鼠标坐标不需要 y 轴翻转**来确定目标子绘图——直接用 Qt 坐标匹配即可。仅在 `renderInViewport()` 中设置 `glViewport` 时需要翻转 y 轴（`glY = canvasHeight - viewport.bottom() - 1`）。转发给子绘图的本地坐标计算：`localX = globalX - viewport.x()`，`localY = globalY - viewport.y()`。

8. **子绘图的 Q_OBJECT**：子绘图对象（如 SurfacePlot）继承自 QOpenGLWidget，但 MultiPlot 不会将它们作为子 widget 添加。因此子绘图不会收到 Qt 的 resize/paint 事件。所有 GL 操作通过 MultiPlot 间接触发。

9. **性能优化**：对于静态子绘图，可以预渲染到 FBO（帧缓冲对象），然后在 `paintGL()` 中将 FBO 作为纹理绘制。这避免了每帧重新渲染所有子绘图。但这增加了实现复杂度，初始版本建议直接渲染。

10. **Plan 依赖**：MultiPlot 的示例需要至少 2-3 种可用的 3D 绘图类型。建议在 plan03-07 中的至少 2 个完成后再实现 MultiPlot 示例。如果某些绘图类型未实现，示例可以使用已有 SurfacePlot 的不同配置（不同函数、不同样式）作为子绘图。
