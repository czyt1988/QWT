# Qwt 3D 模块架构原则宪法

> **地位：本文件是 3D 模块的最高设计规范。所有 3D 模块的代码开发、重构、扩展必须遵循本文件确立的原则。**
>
> 创建日期：2026-07-22
> 分支：new3d
> 基线节点：51f5e2a（fix(plot3d): fix 3 bugs in lighting and mouse/wheel event handling）

---

## 一、核心理念

### 1.1 Plot + Item 模式

3D 模块严格遵循与 2D 模块 (`QwtPlot` + `QwtPlotItem`) 对称的架构：

- **`Qwt3DPlot`** 是渲染窗口（`QOpenGLWidget` 子类），负责 GL 上下文管理、视图变换、光照、坐标系统、鼠标/键盘交互。它本身**不持有任何绘图数据**。
- **`Qwt3DPlotItem`** 是所有 3D 绘图元素的抽象基类（非 widget），负责自身数据存储、几何生成、颜色/样式管理。通过 `attach(Qwt3DPlot*)` / `detach()` 挂载到渲染窗口。
- 一个 `Qwt3DPlot` 窗口可以挂载**任意数量的 `Qwt3DPlotItem`**，在同一个 GL 上下文中统一渲染。

### 1.2 为什么放弃 widget-per-plot 模式

上游 libqwtplot3d 的原始设计是每种 3D 绘图类型自身就是一个 `QOpenGLWidget` 子类。这种设计有根本性缺陷：

| 问题 | 说明 |
|------|------|
| 无法组合 | 无法在同一个 3D 空间中同时渲染一个曲面和一条曲线——它们在不同 widget 中，各自有独立 GL 上下文 |
| 资源浪费 | 每个 widget 独立拥有完整的坐标系统、光照、鼠标交互、GL 上下文 |
| API 不一致 | 与 2D 的 `QwtPlot` + `QwtPlotItem` 模式完全不同，用户学习成本高 |
| 扩展困难 | 新增绘图类型必须继承 widget，无法作为 item 叠加到已有 plot 上 |

**结论：widget-per-plot 模式不可接受，必须重构为 Plot + Item 模式。**

### 1.3 与 2D 模块的对称关系

| 2D 模块 | 3D 模块 | 对应关系 |
|---------|---------|---------|
| `QwtPlot` | `Qwt3DPlot` | 渲染窗口 |
| `QwtPlotItem` | `Qwt3DPlotItem` | item 抽象基类 |
| `QwtPlotDict` | `Qwt3DPlotDict` | item 列表管理（或 Plot 内部管理） |
| `QwtPlotCurve` | `Qwt3DSurface` | 具体绘图 item |
| `QwtPlotGrid` | `Qwt3DCoordinateSystem` | 坐标系统 |
| `QwtPlotItem::attach(QwtPlot*)` | `Qwt3DPlotItem::attach(Qwt3DPlot*)` | 挂载模式 |
| `QwtPlotItem::draw()` | `Qwt3DPlotItem::draw()` | 绘制接口 |
| `QwtPlotItem::itemChanged()` | `Qwt3DPlotItem::itemChanged()` | 变更通知 |
| `QwtPlotItem::z()` / `setZ()` | `Qwt3DPlotItem::z()` / `setZ()` | z-order |

---

## 二、命名规范

### 2.1 无命名空间

3D 模块**不使用 `namespace`**，与 2D 模块保持一致。

- **禁止** `namespace Qwt3D { ... }`
- **禁止** `using namespace Qwt3D;`
- 所有类、函数、类型直接定义在全局作用域

### 2.2 类命名

所有类使用 **`Qwt3D` 前缀 + 大驼峰（PascalCase）**：

```
Qwt3DPlot          ← 渲染窗口
Qwt3DPlotItem      ← item 基类
Qwt3DSurface       ← 曲面 item
Qwt3DLine          ← 3D 线条 item
Qwt3DScatter       ← 散点 item
Qwt3DBar            ← 3D 柱状图 item
Qwt3DVectorField    ← 矢量场 item
Qwt3DStreamline     ← 流线 item
Qwt3DIsosurface     ← 等值面 item
Qwt3DVolume         ← 体绘制 item
Qwt3DCoordinateSystem ← 坐标系统
Qwt3DColorLegend    ← 颜色图例
Qwt3DAxis           ← 坐标轴
Qwt3DLabel          ← 标签
Qwt3DDrawable       ← 可绘制基类（场景图）
Qwt3DEnrichment     ← 顶点装饰基类
Qwt3DData           ← 数据基类
Qwt3DGridData       ← 网格数据
Qwt3DCellData       ← 单元数据
Qwt3DColor          ← 颜色 functor 基类
Qwt3DStandardColor  ← 标准颜色实现
Qwt3DColorMapColor  ← colormap 适配器
Qwt3DMapping        ← 数据映射基类
Qwt3DGridMapping    ← 网格映射
Qwt3DFunction       ← z=f(x,y) 函数
Qwt3DParametricSurface ← 参数曲面
Qwt3DScale          ← 刻度
Qwt3DAutoScaler     ← 自动缩放
```

**命名规则：**
- 渲染窗口：`Qwt3DPlot`（对应 `QwtPlot`）
- item 基类：`Qwt3DPlotItem`（对应 `QwtPlotItem`）
- 具体 item：`Qwt3D` + 绘图类型名（对应 `QwtPlotCurve` / `QwtPlotGrid` 等模式，不加 `Item` 后缀）
- 非绘图组件：`Qwt3D` + 组件名（如 `Qwt3DAxis`、`Qwt3DColor`）

### 2.3 文件命名

所有文件使用 **`qwt3d_` 前缀 + 小写下划线（snake_case）**：

```
qwt3d_plot.h / qwt3d_plot.cpp              ← Qwt3DPlot
qwt3d_plot_p.h                               ← Qwt3DPlot 私有数据
qwt3d_plotitem.h / qwt3d_plotitem.cpp       ← Qwt3DPlotItem
qwt3d_plotitem_p.h                           ← Qwt3DPlotItem 私有数据
qwt3d_surface.h / qwt3d_surface.cpp          ← Qwt3DSurface
qwt3d_bar.h / qwt3d_bar.cpp                  ← Qwt3DBar            (+ qwt3d_bar_p.h) [已实现]
qwt3d_line3d.h / qwt3d_line3d.cpp            ← Qwt3DLine           (+ qwt3d_line3d_p.h) [已实现]
qwt3d_scatter.h / qwt3d_scatter.cpp          ← Qwt3DScatter
qwt3d_coordsys.h / qwt3d_coordsys.cpp        ← Qwt3DCoordinateSystem
qwt3d_axis.h / qwt3d_axis.cpp                ← Qwt3DAxis
qwt3d_colorlegend.h / qwt3d_colorlegend.cpp  ← Qwt3DColorLegend
qwt3d_drawable.h / qwt3d_drawable.cpp        ← Qwt3DDrawable
qwt3d_enrichment.h                           ← Qwt3DEnrichment
qwt3d_color.h / qwt3d_color.cpp              ← Qwt3DColor
qwt3d_types.h / qwt3d_types.cpp               ← 基础类型 (Triple, RGBA, etc.)
qwt3d_global.h                               ← 导出宏
```

### 2.4 导出宏

使用 `QWT3D_EXPORT` 宏（已在 `qwt3d_global.h` 中定义），与 2D 的 `QWT_EXPORT` 对称：

```cpp
class QWT3D_EXPORT Qwt3DPlot : public QOpenGLWidget { ... };
class QWT3D_EXPORT Qwt3DPlotItem { ... };
```

---

## 三、架构设计

### 3.1 Qwt3DPlot（渲染窗口）

`Qwt3DPlot` 是唯一的渲染窗口，继承 `QOpenGLWidget`。

**职责：**
- GL 上下文管理（`initializeGL` / `paintGL` / `resizeGL`）
- 视图变换（旋转/缩放/平移/正交投影）
- 光照系统（最多 8 盏灯）
- 坐标系统（`Qwt3DCoordinateSystem`）
- 颜色图例（`Qwt3DColorLegend`）
- 标题
- 鼠标/键盘交互
- item 列表管理（`attach` / `detach` / `itemList`）
- 截图/矢量导出

**不负责：**
- 绘图数据存储（由各 item 自行管理）
- 几何生成（由各 item 的 `draw()` 负责）
- 颜色/样式（由各 item 自行管理）

**渲染管线（paintGL）：**

```
1. clear color + depth buffer
2. apply lighting
3. apply view transform (rotation → scale → translation → projection)
4. for each item in m_items (sorted by z-order):
       item->draw()        ← item 在已设置好的 GL 变换矩阵下绘制自身几何
5. draw coordinate system
6. draw legend
7. draw title
```

**item 列表管理：**

```cpp
class QWT3D_EXPORT Qwt3DPlot : public QOpenGLWidget {
public:
    void attach(Qwt3DPlotItem* item);
    void detach(Qwt3DPlotItem* item);
    const QList<Qwt3DPlotItem*>& itemList() const;

private:
    QList<Qwt3DPlotItem*> m_items;
};
```

item 的 `attach(Qwt3DPlot*)` 内部调用 `plot->attach(this)`；`detach()` 内部调用 `plot->detach(this)`。与 2D 的 `QwtPlotItem::attach(QwtPlot*)` 完全对称。

### 3.2 Qwt3DPlotItem（item 基类）

`Qwt3DPlotItem` 是所有 3D 绘图元素的抽象基类，**不是 widget**。

**核心接口：**

```cpp
class QWT3D_EXPORT Qwt3DPlotItem {
public:
    virtual ~Qwt3DPlotItem();

    void attach(Qwt3DPlot* plot);
    void detach();

    virtual void draw() = 0;                    // 在 paintGL 中被调用，item 绘制自身几何
    virtual Qwt3D::ParallelEpiped hull() const = 0;  // item 的包围盒，用于自动坐标系统计算

    void setTitle(const QString& title);
    QString title() const;

    void setZ(double z);                        // z-order，控制绘制顺序
    double z() const;

    virtual void itemChanged();                 // 数据/属性变更后通知 plot 重绘

protected:
    Qwt3DPlot* plot() const;                    // 获取所属 plot（用于访问 GL 上下文、坐标系统等）

private:
    Qwt3DPlot* m_plot = nullptr;
    double m_z = 0.0;
    QString m_title;
};
```

**设计原则：**
- `draw()` 是纯虚函数，item 在此方法中执行所有 GL 绘制调用
- item 可以使用 display list 缓存几何（在 `draw()` 中 `glCallList`），也可以每帧直接绘制——由 item 自行决定
- `hull()` 返回 item 在 3D 空间中的包围盒，`Qwt3DPlot` 遍历所有 item 的 hull 取并集来计算坐标系统范围
- `itemChanged()` 标记 plot 需要 `update()`（重绘），类似 2D 的 `QwtPlotItem::itemChanged()`

### 3.3 具体绘图 item

每个具体绘图类型继承 `Qwt3DPlotItem`：

```
Qwt3DPlotItem (抽象基类)
├── Qwt3DSurface          ← 曲面（网格/单元）             [已实现]
├── Qwt3DLine              ← 3D 线条/曲线 (Tube/Lines/Dots)  [已实现]
├── Qwt3DScatter           ← 3D 散点
├── Qwt3DBar               ← 3D 柱状图 (1D 序列 / 2D 网格)    [已实现]
├── Qwt3DVectorField       ← 3D 矢量场
├── Qwt3DStreamline        ← 流线
├── Qwt3DIsosurface        ← 等值面
├── Qwt3DVolume            ← 体绘制
└── Qwt3DContourSlice      ← 等值线切片
```

> `Qwt3DLine`（文件 `qwt3d_line3d.h/.cpp`+`_p.h`）与 `Qwt3DBar`（`qwt3d_bar.h/.cpp`+`_p.h`）已于 v7.3.4 实现：均继承 `Qwt3DPlotItem`，实现 `draw()`/`hull()`/`populateLegendColors()`，复用 surface shader 与 `Qwt3DColor` functor 体系，并已接入 `Qwt3DTheme::applyToItem()`。`Qwt3DLine` 提供 `setSamples(...)`（镜像 2D `QwtPlotCurve`）、`LineStyle{Lines,Tube,Dots}`、Tube 用 parallel-transport 扫掠几何；`Qwt3DBar` 提供 1D 序列与 2D 网格（`Qwt3DFunctionData`）入口、`BarStyle{Filled,FilledMesh,Wireframe}`、逐柱 6 面扁平法向 cuboid。

每个 item 自行管理：
- 自身的数据（`setData` / `loadFromData`）
- 自身的颜色/样式
- 自身的几何缓存（VBO/VAO）
- 自身的 enrichment（顶点装饰）

### 3.4 渲染上下文

`Qwt3DPlot` 在调用 `item->draw()` 前，已经设置好：
- modelview 矩阵（通过 uniform 传入 shader）
- projection 矩阵（通过 uniform 传入 shader）
- 光照参数（通过 uniform 传入 shader）
- 视口

item 在 `draw()` 中只需关注自身几何绘制（绑定 VBO/VAO、调用 `glDrawArrays`/`glDrawElements`），不需要管理视图变换。这与 2D 中 `QwtPlotItem::draw()` 在已设置好 `QwtScaleMap` 的 QPainter 上绘制是对称的。

---

## 四、保留与重构清单

### 4.1 可保留的组件（仅需去命名空间 + 重命名）

| 当前类名 | 新类名 | 说明 |
|---------|--------|------|
| `Plot3D` | `Qwt3DPlot` | 需重写：去掉数据职责，增加 item 列表 |
| `SurfacePlot` | `Qwt3DSurface` | 需重写：从 widget → item |
| `CoordinateSystem` | `Qwt3DCoordinateSystem` | 去命名空间 + 重命名 |
| `ColorLegend` | `Qwt3DColorLegend` | 去命名空间 + 重命名 |
| `Axis` | `Qwt3DAxis` | 去命名空间 + 重命名 |
| `Label` | `Qwt3DLabel` | 去命名空间 + 重命名 |
| `Drawable` | `Qwt3DDrawable` | 去命名空间 + 重命名 |
| `Enrichment` | `Qwt3DEnrichment` | 去命名空间 + 重命名 |
| `VertexEnrichment` | `Qwt3DVertexEnrichment` | 去命名空间 + 重命名 |
| `CrossHair` / `Dot` / `Cone` / `Arrow` | `Qwt3DCrossHair` 等 | 去命名空间 + 重命名 |
| `Data` | `Qwt3DData` | 去命名空间 + 重命名 |
| `GridData` | `Qwt3DGridData` | 去命名空间 + 重命名 |
| `CellData` | `Qwt3DCellData` | 去命名空间 + 重命名 |
| `Color` | `Qwt3DColor` | 去命名空间 + 重命名 |
| `StandardColor` | `Qwt3DStandardColor` | 去命名空间 + 重命名 |
| `ColorMapColor` | `Qwt3DColorMapColor` | 去命名空间 + 重命名 |
| `Mapping` | `Qwt3DMapping` | 去命名空间 + 重命名 |
| `GridMapping` | `Qwt3DGridMapping` | 去命名空间 + 重命名 |
| `Function` | `Qwt3DFunction` | 去命名空间 + 重命名，target 改为 item |
| `ParametricSurface` | `Qwt3DParametricSurface` | 去命名空间 + 重命名，target 改为 item |
| `Scale` | `Qwt3DScale` | 去命名空间 + 重命名 |
| `AutoScaler` | `Qwt3DAutoScaler` | 去命名空间 + 重命名 |
| `Qwt3DTheme` | `Qwt3DTheme` | 已符合命名，仅需去命名空间 |

### 4.2 需要新建的组件

| 新类名 | 说明 |
|--------|------|
| `Qwt3DPlotItem` | item 抽象基类（attach/detach/draw/hull/z/title） |
| `Qwt3DPlotItem_p` | item 私有数据 |

### 4.3 需要删除的 stub

| 文件 | 说明 |
|------|------|
| `qwt3d_graphplot.h` | 空壳 stub，后续如需要中间抽象类在 item 体系中重建 |
| `qwt3d_multiplot.h` | 空壳 stub，多 plot 组合在 item 体系下无意义 |
| `qwt3d_volumeplot.h` | 空壳 stub，后续作为 `Qwt3DVolume` item 实现 |

### 4.4 基础类型（qwt3d_types.h）

`Triple`、`RGBA`、`Tuple`、`ParallelEpiped`、`TripleField`、`CellField` 等类型保留，去命名空间后可直接使用。如需要可添加 `Qwt3D` 前缀别名保持兼容。

---

## 五、OpenGL 技术规范

### 5.1 版本要求

| 项目 | 要求 |
|------|------|
| OpenGL 版本 | **3.3+ Core Profile**（最低） |
| GLSL 版本 | **#version 330**（最低） |
| Profile | 优先 Core Profile；如平台不支持则回退 Compatibility Profile |
| Qt 依赖 | `QOpenGLFunctions_3_3_Core`（或 `_Core` 后缀的最高可用版本） |

目标平台为 Windows (MSVC) + Qt 5.12+/Qt 6.x，OpenGL 3.3 Core 在主流显卡驱动上均可用。

### 5.2 禁止使用的 Legacy API

以下 API 属于固定管线（Fixed-Function Pipeline）时代，**一律禁止使用**：

| 禁止 API 类别 | 禁止的具体调用 | 现代替代方案 |
|--------------|---------------|-------------|
| **Immediate Mode** | `glBegin` / `glEnd` / `glVertex3f` / `glColor4f` / `glNormal3f` / `glTexCoord2f` | VBO + `glDrawArrays` / `glDrawElements` |
| **Display List** | `glNewList` / `glEndList` / `glCallList` / `glDeleteLists` / `glGenLists` | VBO（GPU 显存常驻，无需编译/回放） |
| **固定管线变换** | `glRotatef` / `glTranslatef` / `glScalef` / `glPushMatrix` / `glPopMatrix` / `glLoadIdentity` / `glLoadMatrixf` / `glMultMatrixf` | `QMatrix4x4` CPU 计算后通过 `uniform mat4` 传入 shader |
| **固定管线投影** | `glOrtho` / `glFrustum` / `gluPerspective` | `QMatrix4x4::ortho()` / `QMatrix4x4::perspective()` 通过 uniform 传入 shader |
| **固定管线光照** | `glLightfv` / `glMaterialfv` / `glEnable(GL_LIGHTING)` / `glLightModeli` | 自定义 GLSL 光照计算（Phong/Blinn-Phong） |
| **固定管线着色** | `glColorMaterial` / `glShadeModel(GL_SMOOTH)` | 顶点着色器输出 `out vec3 vColor`，片元着色器 `in vec3 vColor` |
| **矩阵查询** | `glGetDoublev(GL_MODELVIEW_MATRIX/...)` / `glGetIntegerv(GL_VIEWPORT/...)` | 由 `Qwt3DPlot` 在 CPU 端维护矩阵状态，item 通过 `plot()->viewMatrix()` 等接口获取 |
| **GLU 工具** | `gluProject` / `gluUnProject` / `gluErrorString` / `gluPerspective` | `QMatrix4x4::map()` / `QMatrix4x4::inverted().map()` / `glGetError()` + 自定义错误字符串 |
| **Legacy 状态查询** | `glIsEnabled` / `glIsList` | 由 `Qwt3DPlot` 在 CPU 端维护状态，不查询 GPU 状态（GPU 查询导致管线 stall） |
| **Legacy 线/点平滑** | `glEnable(GL_LINE_SMOOTH)` / `glEnable(GL_POINT_SMOOTH)` | MSAA（`QSurfaceFormat::setSamples(n)`）或 shader 内自定义抗锯齿 |
| **saveGLState / restoreGLState** | 手动保存/恢复大量 fixed-function 状态 | Core Profile 无需管理这些状态；item 各自管理自身 shader uniform |

### 5.3 必须使用的现代 API

#### VBO / VAO（顶点数据管理）

```cpp
// VBO — 顶点数据驻留 GPU 显存
QOpenGLBuffer vbo;
vbo.create();
vbo.bind();
vbo.allocate(vertices.data(), vertices.size() * sizeof(Vertex));
vbo.release();

// VAO — 顶点属性配置
QOpenGLVertexArrayObject vao;
vao.create();
vao.bind();
shaderProgram->enableAttributeArray("position");
shaderProgram->setAttributeBuffer("position", GL_FLOAT, 0, 3, sizeof(Vertex));
vao.release();

// 绘制时
vao.bind();
glDrawArrays(GL_TRIANGLES, 0, vertexCount);
vao.release();
```

**规则：**
- 所有顶点数据必须通过 VBO 上传到 GPU，禁止逐顶点 CPU 提交
- VAO 用于封装顶点属性配置，避免每次绘制重复配置
- VBO/VAO 由各 item 自行创建和管理，item 析构时释放
- 数据未变化时不需要重新上传 VBO（替代了 display list 的缓存功能）

#### GLSL Shader（可编程管线）

```cpp
QOpenGLShaderProgram program;
program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/surface.vert");
program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/surface.frag");
program.link();
program.bind();

// 传入变换矩阵（由 Qwt3DPlot 计算）
program.setUniformValue("uModelView", viewMatrix);
program.setUniformValue("uProjection", projMatrix);
program.setUniformValue("uNormalMatrix", normalMatrix);
```

**规则：**
- 所有渲染必须通过 GLSL shader，禁止依赖固定管线
- shader 源码放在 `src/plot3d/shaders/` 目录，以 `.vert` / `.frag` / `.geom` 扩名区分
- shader 通过 Qt 资源系统或文件加载，不在 C++ 中硬编码 GLSL 字符串
- 每个 item 可以使用自己的 shader，也可以共享 plot 级别的通用 shader
- 变换矩阵、光照参数、颜色等通过 uniform 变量传入 shader

#### 矩阵管理（CPU 端）

```cpp
// 在 Qwt3DPlot 中维护，不使用 GL 矩阵栈
QMatrix4x4 m_modelView;   // 旋转 + 缩放 + 平移
QMatrix4x4 m_projection;  // 正交/透视投影
QMatrix4x4 m_normalMatrix; // modelView 的逆转置（用于法线变换）
```

**规则：**
- 所有变换矩阵在 CPU 端用 `QMatrix4x4` 计算
- `Qwt3DPlot::paintGL()` 中计算好 modelView 和 projection，通过 uniform 传入当前 shader
- item 不自行设置 view/projection 矩阵（由 plot 统一管理）
- item 如需自身局部变换（如柱状图的逐柱位置），在自身 modelMatrix 上叠加，通过 uniform 传入

### 5.4 渲染管线（现代 OpenGL 版）

```cpp
void Qwt3DPlot::paintGL() {
    // 1. 清除
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. 计算变换矩阵（CPU 端，不使用 GL 矩阵栈）
    QMatrix4x4 modelView;
    modelView.translate(-center);
    modelView.scale(zoom * scaleX, zoom * scaleY, zoom * scaleZ);
    modelView.rotate(xRot - 90, 1, 0, 0);
    modelView.rotate(yRot, 0, 1, 0);
    modelView.rotate(zRot, 0, 0, 1);
    modelView.translate(shiftX, shiftY, shiftZ);

    QMatrix4x4 projection;
    if (m_ortho)
        projection.ortho(-radius, radius, -radius, radius, 0, 40 * radius);
    else
        projection.perspective(fov, aspect, near, far);
    projection.translate(vpShiftX, vpShiftY, -7 * radius);

    QMatrix4x4 normalMatrix = modelView.inverted().transposed();

    // 3. 设置光照 uniform（当前 shader）
    m_lightUBO.bind();  // 或通过 uniform 逐项传入
    // ... 传入光源位置、颜色、材质参数

    // 4. 遍历 item 列表，逐个渲染
    for (auto* item : m_items) {
        item->draw();  // item 内部：bind shader → set uniforms → bind VAO → glDraw*
    }

    // 5. 绘制坐标系统（同样使用 VBO + shader）
    m_coordinateSystem->draw();

    // 6. 绘制图例、标题
    m_legend->draw();
    drawTitle();
}
```

### 5.5 VBO/VAO 所有权与生命周期

| 管理者 | 职责 |
|--------|------|
| `Qwt3DPlot` | 拥有共享 shader（通用 surface/line/point shader）、坐标系统 VBO、图例 VBO |
| `Qwt3DPlotItem` 子类 | 拥有自身的 VBO/VAO、自身专用 shader（如有）；数据变化时重新 `allocate()` VBO；析构时 `destroy()` |

**VBO 更新策略：**
- 数据首次加载时 `allocate()` 上传全部顶点
- 数据变化时（`setData` / `loadFromData`），标记 VBO 为 dirty
- `draw()` 中检测 dirty 标志，重新上传后再绘制
- 不使用 `glMapBuffer`（除非需要 streaming 场景），优先 `allocate()` + `glBufferSubData`

### 5.6 Qt OpenGL 封装使用规范

使用 Qt 提供的 OpenGL 封装类，不直接调用裸 GL 函数（除非 Qt 无对应封装）：

| 裸 GL 用途 | Qt 封装 | 说明 |
|-----------|---------|------|
| Shader 编译/链接/绑定 | `QOpenGLShaderProgram` | 优先使用 |
| VBO 管理 | `QOpenGLBuffer` | 优先使用 |
| VAO 管理 | `QOpenGLVertexArrayObject` | 优先使用（Qt5.x+） |
| FBO（离屏渲染） | `QOpenGLFramebufferObject` | 用于拾取、截图 |
| 纹理 | `QOpenGLTexture` | 用于 colormap 纹理 |
| GL 函数入口 | `QOpenGLFunctions` | `QOpenGLWidget` 自带，`initializeGL()` 中 `initializeOpenGLFunctions()` |
| 高版本函数 | `QOpenGLFunctions_3_3_Core` | 如需 3.3+ 特性（UBO 等），request 该版本 |

### 5.7 坐标拾取（Picking）

禁止使用 `gluUnProject` / `glReadPixels` 查询 GPU 状态来拾取。替代方案：
- **射线投射法**：鼠标点击 → 构造射线 → CPU 端与 item 包围盒/三角形求交
- `Qwt3DPlot` 提供鼠标坐标 → 世界坐标的转换接口（CPU 端矩阵计算）

### 5.8 矢量导出（gl2ps）

`gl2ps` 库依赖 fixed-function 管线，在 Core Profile 下不工作。矢量导出方案：
- 保留 gl2ps 作为 Compatibility Profile 下的回退选项
- Core Profile 下优先考虑：通过遍历 item 的几何数据（VBO 中的顶点）直接生成矢量输出，不依赖 GL 回调

### 5.9 性能准则

| 准则 | 说明 |
|------|------|
| **最小化 draw call** | 同类型 item 尽量合并为一个 VBO 一次 `glDrawElements`；避免逐元素 `glDrawArrays` |
| **VBO 持久化** | 数据未变化时不重新上传；使用 dirty 标志延迟更新 |
| **Shader 复用** | 通用 shader（surface/line/point）由 plot 持有共享；item 专用 shader 仅在该 item 绑定 |
| **Face Culling** | 对封闭曲面启用 `GL_CULL_FACE`（透明面除外） |
| **Depth Test** | 默认启用 `GL_DEPTH_TEST`；透明 item 需关闭深度写入（`glDepthMask(GL_FALSE)`） |
| **批量渲染** | 同 shader 的 item 尽量在同一 shader bind 区间内连续绘制 |
| **避免运行时 shader 编译** | shader 在 `initializeGL` 中编译并缓存，不在 `paintGL` 中编译 |

---

## 六、开发规范

### 6.1 PIMPL 模式

遵循项目自定义 PIMPL 宏（见 AGENTS.md）：

```cpp
// 头文件
class QWT3D_EXPORT Qwt3DPlotItem {
    QWT_DECLARE_PRIVATE(Qwt3DPlotItem)
};

// 源文件
class Qwt3DPlotItem::PrivateData {
    QWT_DECLARE_PUBLIC(Qwt3DPlotItem)
};

Qwt3DPlotItem::Qwt3DPlotItem() : QWT_PIMPL_CONSTRUCT {}
```

### 6.2 现代 C++

- 使用 `override` / `final`（不用旧宏 `QWT_OVERRIDE` / `QWT_FINAL`）
- 使用 `nullptr`（不用 `NULL`）
- 使用 `static_cast<>` / `using`（不用 C 风格转换 / `typedef`）
- Qt 容器迭代使用 `qwt_as_const(container)`
- 智能指针使用 `qwt_make_unique<T>(args...)`

### 6.3 信号槽

- 使用 `Q_SIGNALS:` / `public Q_SLOTS:` 宏
- 优先新式 `connect(sender, &Sender::signal, receiver, &Receiver::slot)` 语法

### 6.4 注释规范（Doxygen 纯英文）

所有源码注释**一律使用英文**，禁止中文。Doxygen 关键字统一用 `@` 前缀。

| 注释类型 | 位置 | 要求 |
|---------|------|------|
| 类注释 | `.h` | 英文 Doxygen，含使用示例 |
| public 函数详细 | `.cpp` | 英文 Doxygen（`@brief`/`@param`/`@return`/`@details`） |
| public 函数简要 | `.h` | 单行英文 `// Comment` |
| 信号注释 | `.h` | 英文 Doxygen |
| private/protected 函数 | `.cpp` | 可选，建议英文 |

### 6.5 Include 顺序

```cpp
#include "own_header.h"   // 本文件对应头文件优先
// 空行
#include <qnamespace.h>   // Qt 系统头文件
#include <QOpenGLWidget>   // Qt 类头文件
// 空行
#include <algorithm>       // STL
// 空行
#include "qwt3d_plot.h"    // 本项目其他头文件
```

### 6.6 格式化

遵循 `.clang-format`（WebKit 风格基础）：
- 缩进 4 空格，列宽 120
- 花括号：类/函数/枚举/命名空间后换行，控制语句不换行
- 指针左对齐（`int* p`）

### 6.7 Qt5/Qt6 兼容

涉及鼠标/滚轮/字体度量差异时，必须使用 `qwt::compat::` 命名空间下的兼容函数。

---

## 七、使用示例（目标 API）

### 7.1 基本使用：一个曲面

```cpp
auto plot = new Qwt3DPlot(parent);

auto surface = new Qwt3DSurface();
surface->loadFromData(data, columns, rows, minX, maxX, minY, maxY);
surface->attach(plot);
```

### 7.2 组合使用：曲面 + 曲线 + 散点

```cpp
auto plot = new Qwt3DPlot(parent);

auto surface = new Qwt3DSurface();
surface->loadFromData(gridData, cols, rows, 0, 10, 0, 10);
surface->attach(plot);

auto line = new Qwt3DLine();
line->setSamples(points);
line->attach(plot);

auto scatter = new Qwt3DScatter();
scatter->setSamples(scatterPoints);
scatter->setPointStyle(Qwt3DScatter::Sphere);
scatter->attach(plot);
```

### 7.3 函数曲面

```cpp
class MyFunction : public Qwt3DFunction {
    double operator()(double x, double y) override { return sin(x) * cos(y); }
};

auto plot = new Qwt3DPlot(parent);

auto surface = new Qwt3DSurface();
surface->attach(plot);

MyFunction func;
func.setMesh(50, 50);
func.setDomain(-3.14, 3.14, -3.14, 3.14);
func.assign(*surface);  // target 是 item 而非 widget
func.create();
```

---

## 八、重构路径

重构按以下顺序推进，每一步应可独立编译通过：

1. **基础类型层**：去命名空间 + 重命名 `qwt3d_types.h`、`qwt3d_global.h`、`qwt3d_helper.h` 等基础文件
2. **非 widget 组件层**：重命名 `Drawable`、`Axis`、`Label`、`CoordinateSystem`、`ColorLegend`、`Color`、`Enrichment`、`Scale`、`AutoScaler` 等非 widget 类
3. **item 基类**：新建 `Qwt3DPlotItem`，定义 attach/detach/draw/hull 接口
4. **渲染窗口**：重写 `Qwt3DPlot`，去掉数据职责，增加 item 列表管理，保留视图变换/光照/交互
5. **具体 item**：将 `SurfacePlot` 重构为 `Qwt3DSurface`（从 widget → item），后续逐步添加其他 item 类型
6. **数据生成器**：调整 `Function`/`ParametricSurface` 的 target 从 widget → item
7. **主题/IO**：调整 `Qwt3DTheme`、I/O 系统适配新架构
8. **OpenGL 现代化**：将所有渲染从 legacy fixed-function 迁移到 VBO/VAO + GLSL shader，删除 `glBegin/glEnd`、display list、`glRotatef` 等全部 legacy 调用

---

## 九、不可触碰的约束

1. **不得恢复 `namespace Qwt3D`**——所有代码在全局作用域
2. **不得让 `Qwt3DPlot` 持有绘图数据**——数据由 item 管理
3. **不得让 `Qwt3DPlotItem` 继承 `QWidget`/`QOpenGLWidget`**——item 不是 widget
4. **不得在 item 的 `draw()` 中管理视图变换**——视图变换由 plot 负责
5. **不得新增 widget-per-plot 类型的绘图**——所有绘图类型必须是 `Qwt3DPlotItem` 子类
6. **不得使用中文注释**——所有源码注释一律英文
7. **不得使用 legacy OpenGL API**——禁止 `glBegin/glEnd`、`glNewList/glCallList`、`glRotatef/glTranslatef/glScalef`、`glOrtho/glFrustum`、`glLightfv/glMaterialfv` 等全部固定管线调用
8. **不得使用 GL 矩阵栈**——所有变换矩阵在 CPU 端用 `QMatrix4x4` 计算，通过 uniform 传入 shader
9. **不得在 C++ 代码中硬编码 GLSL 字符串**——shader 源码放在 `src/plot3d/shaders/` 目录，通过文件或 Qt 资源加载
