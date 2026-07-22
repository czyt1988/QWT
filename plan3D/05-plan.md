# 05-plan: Qwt3DPlot 渲染窗口重写

> **前置阅读**：执行本任务前，必须完整阅读 `plan3D/ARCHITECTURE-PRINCIPLES.md`，特别是第三章第 3.1 节「Qwt3DPlot（渲染窗口）」、第三章第 3.4 节「渲染上下文」、第五章「OpenGL 技术规范」和第九章「不可触碰的约束」。

## 目标（Objective）

将 `Plot3D`（当前同时承担渲染窗口和数据持有者角色）重写为 `Qwt3DPlot`（纯渲染窗口），移除数据相关职责，增加 item 列表管理，使用 `QMatrix4x4` CPU 端矩阵计算替代 legacy GL 矩阵栈。

## 涉及文件

| 文件 | 操作 |
|------|------|
| `qwt3d_plot.h` | **重写**：去命名空间，重命名为 Qwt3DPlot，移除数据职责，增加 item 列表 |
| `qwt3d_plot_p.h` | **重写**：移除数据成员，增加 item 列表和矩阵成员 |
| `qwt3d_plot.cpp` | **重写**：重写 paintGL/initializeGL/resizeGL |
| `qwt3d_mousekeyboard.cpp` | **重写**：更新类名，保持逻辑 |
| `qwt3d_movements.cpp` | **重写**：更新类名，保持逻辑 |
| `qwt3d_lighting.cpp` | **重写**：移除 legacy GL 光照调用，改为 CPU 端参数存储 |
| `qwt3d_io.h` / `qwt3d_io.cpp` | 修改：Plot3D → Qwt3DPlot，去命名空间 |
| `qwt3d_io_gl2ps.h` / `qwt3d_io_gl2ps.cpp` | 修改：Plot3D* → Qwt3DPlot*，去命名空间 |
| `qwt3d_io_reader.h` / `qwt3d_io_reader.cpp` | 修改：Plot3D* → Qwt3DPlot*，去命名空间 |
| `qwt3d_surfaceplot.h` / `qwt3d_surfaceplot.cpp` / `qwt3d_surfaceplot_p.h` | 修改：继承 Plot3D → Qwt3DPlot，注释掉引用已删除方法的代码 |
| `qwt3d_gridplot.cpp` | 修改：注释掉使用已删除方法（plotStyle(), userStyle(), displayLists(), actualData() 等）的代码 |
| `qwt3d_meshplot.cpp` | 修改：同上 |
| `qwt3d_openglhelper.h` | 修改：确认已去命名空间（Task 01），保留 legacy GL 函数不变 |
| `CMakeLists.txt` | 修改：更新文件引用 |

## 前提条件（Prerequisites）

- Task 01–04 已完成
- 构建通过

## 执行步骤（Steps）

### 1. Plot3D → Qwt3DPlot 重命名

- 移除 `namespace Qwt3D { ... }`
- `class Plot3D` → `class QWT3D_EXPORT Qwt3DPlot`
- PIMPL: `QWT_DECLARE_PRIVATE(Plot3D)` → `QWT_DECLARE_PRIVATE(Qwt3DPlot)`
- 所有 `Plot3D::` 方法前缀 → `Qwt3DPlot::`

### 2. 移除数据相关职责

从 `Qwt3DPlot` 中**删除**以下成员和方法：

| 删除项 | 原因 |
|--------|------|
| `Data* m_actualData` | 数据由 item 管理 |
| `virtual calculateHull() = 0` | 由 item 的 `hull()` 替代 |
| `virtual createData() = 0` | 由 item 的 `draw()` 替代 |
| `virtual createEnrichment(Enrichment&)` | enrichment 移到 item |
| `createEnrichments()` | 同上 |
| `OBJECTS enum` | display list 移到 item |
| `std::vector<GLuint> m_displayLists` | 同上 |
| `displayLists()` | 同上 |
| `actualData()` / `setActualData()` | 同上 |
| `setPlotStyle()` / `plotStyle()` / `userStyle()` | 样式移到 item |
| `setShading()` / `shading()` | 同上 |
| `setIsolines()` / `isolines()` | 同上 |
| `setSmoothMesh()` / `smoothDataMesh()` | 同上 |
| `setMeshColor()` / `meshColor()` | 同上 |
| `setMeshLineWidth()` / `meshLineWidth()` | 同上 |
| `setDataColor()` / `dataColor()` | 同上 |
| `setPolygonOffset()` / `polygonOffset()` | 同上 |
| `setCoordinateStyle()` | 移到 CoordinateSystem |
| `addEnrichment()` / `degrade()` | 移到 item |
| `hasData()` | 改为 `hasItems()` |

**PrivateData 需删除的成员：**
- `Data* m_actualData`
- `Color* m_dataColor`
- `Enrichment* m_userPlotStyle`
- `std::list<Enrichment*> m_enrichmentList`
- `std::vector<GLuint> m_displayLists`
- `RGBA m_meshColor`
- `double m_meshLineWidth`
- `PLOTSTYLE m_plotStyle`
- `SHADINGSTYLE m_shading`
- `FLOORSTYLE m_floorStyle`
- `double m_polygonOffset`
- `int m_isolines`
- `bool m_smoothDataMesh`
- 构造函数中 `d->m_dataColor = new StandardColor(this, 100)` 需移除
- 析构函数中 `SaveGlDeleteLists`、`delete d->m_dataColor`、`delete d->m_userPlotStyle`、enrichment list 遍历删除等需移除

**updateData() 处理：**
- `updateData()` 当前调用 `calculateHull()`（纯虚，正在删除）、`createData()`（纯虚，正在删除）、`createEnrichments()`、`SaveGlDeleteLists()`、`glGenLists()`、`glNewList()`/`glEndList()`
- **删除 `updateData()` 方法**及其所有调用。`Qwt3DTheme::apply()` 末尾的 `plot->updateData()` 调用改为 `plot->update()`（仅触发重绘）
- gl2ps 的 `operator()` 中的 `plot->updateData()` 调用同样改为 `plot->update()`

**createCoordinateSystem() 无参版本处理：**
- `createCoordinateSystem()`（无参版本）当前调用 `calculateHull()`（纯虚，正在删除）
- 删除无参版本，保留 `createCoordinateSystem(Triple, Triple)` 版本
- 坐标系统范围计算改为：在 `paintGL()` 中遍历所有 item 的 `hull()` 取并集（但此阶段无 item，暂时硬编码默认 hull 或跳过）

**保留**以下成员和方法：

| 保留项 | 原因 |
|--------|------|
| `QOpenGLWidget` 继承 | 渲染窗口 |
| `initializeGL()` / `paintGL()` / `resizeGL()` | GL 上下文管理 |
| `CoordinateSystem* coordinates()` | 坐标系统 |
| `ColorLegend* legend()` | 图例 |
| `ParallelEpiped hull() const` | 所有 item hull 的并集 |
| 视图变换：旋转/缩放/平移/正交 | 渲染窗口职责 |
| 光照系统 | 渲染窗口职责 |
| 鼠标/键盘交互 | 渲染窗口职责 |
| 标题 | 渲染窗口职责 |
| 信号：rotationChanged 等 | 保留 |
| `renderPixmap()` | 保留 |
| `setBackgroundColor()` / `backgroundRGBAColor()` | 保留 |

**showColorLegend() 处理：**
- 当前 `showColorLegend(bool show)` 中有 `d->m_dataColor->createVector(d->m_legend.colors)` 调用
- 移除 `m_dataColor` 后，暂时移除 `createVector` 调用，legend 颜色后续由 item 提供
- 标注 TODO：后续 item 体系完善后，legend 颜色由 Qwt3DSurface item 的 dataColor 提供

### 3. 增加 item 列表管理

**前向声明和 include：**
- `qwt3d_plot.h` 中添加 `class Qwt3DPlotItem;` 前向声明
- `qwt3d_plot_p.h` 中添加 `#include <QMatrix4x4>`
- 移除 `qwt3d_plot.h` 中的 `#include "qwt3d_enrichment_std.h"`（enrichment 相关成员已删除）

```cpp
public:
    void attach(Qwt3DPlotItem* item);
    void detach(Qwt3DPlotItem* item);
    const QList<Qwt3DPlotItem*>& itemList() const;

private:
    QList<Qwt3DPlotItem*> m_items;
```

- `attach()`：如果不在列表中，插入并按 z 排序
- `detach()`：从列表移除（不 delete）
- `itemList()`：返回按 z 排序的列表
- `hull()`：遍历所有 item 的 `hull()` 取并集
- `hasItems()`：`!m_items.isEmpty()`

### 4. 重写 paintGL（CPU 端矩阵 + item 遍历）

```cpp
void Qwt3DPlot::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // CPU 端计算变换矩阵
    QMatrix4x4 modelView;
    // ... translate, scale, rotate

    QMatrix4x4 projection;
    // ... ortho or perspective

    QMatrix4x4 normalMatrix = modelView.inverted().transposed();

    // 遍历 item（暂用 legacy GL，后续任务现代化）
    // 注意：此阶段 item 还未创建，paintGL 中 item 遍历暂时为空
    // 但需保留接口结构

    // 绘制坐标系统（保留 legacy GL 暂时）
    // 绘制图例
    // 绘制标题
}
```

**重要**：此阶段 `paintGL()` 中的 item 遍历暂时为空循环（没有 item 子类存在），但矩阵计算和坐标系统绘制必须工作。

**架构豁免声明**：此阶段坐标系统/图例的 legacy GL 调用（glBegin/glEnd 等）暂时保留。这是对 ARCHITECTURE-PRINCIPLES.md 第九章约束 7（不得使用 legacy OpenGL API）的已知临时豁免，将在 Plan 08（OpenGL 现代化）中彻底清除。

### 5. 重写 initializeGL

- 移除 `glEnable(GL_LIGHTING)` 等 legacy 光照调用
- 移除 `glMaterialfv` / `glLightfv` 调用
- 保留 `glEnable(GL_BLEND)` / `glEnable(GL_DEPTH_TEST)`（这些在 Core Profile 中仍可用）
- 设置 `m_initializedGL = true`

### 6. 重写光照系统（CPU 端参数存储）

`qwt3d_lighting.cpp` 中：
- **保留**光照参数存储（Light 结构体：unlit, rot, shift）
- **保留** `illuminate()` / `blowout()` / `lightingEnabled()`
- **删除** `applyLight()` / `applyLights()` 中的所有 `glLightfv` / `glMaterialfv` / `glPushMatrix` / `glRotatef` 调用
- **保留** `setMaterialComponent()` / `setLightComponent()` 等方法签名（只存储参数，不调用 GL）
- 光照参数后续通过 uniform 传入 shader（在 OpenGL 现代化任务中实现）
- `enableLighting()` 中移除 `glEnable(GL_LIGHTING)` / `glDisable(GL_LIGHTING)` 调用，仅保留 flag 设置和 `update()` 调用
- `setMaterialComponent()` / `setLightComponent()` / `setShininess()` 中移除 `makeCurrent()` 和所有 GL 调用，仅存储参数
- `initializeGL()` 中移除 `glShadeModel(GL_SMOOTH)` 和 `glEnable(GL_COLOR_MATERIAL)` 等 Core Profile 中不存在的调用

### 7. 重写鼠标/键盘/移动

`qwt3d_mousekeyboard.cpp` / `qwt3d_movements.cpp`：
- 类名 `Plot3D::` → `Qwt3DPlot::`
- 保持所有逻辑不变
- 信号签名中 `Plot3D` 相关引用更新

### 8. 更新 Qwt3DTheme

`Qwt3DTheme::apply(Qwt3DPlot*)` 中的 `Plot3D*` → `Qwt3DPlot*`。移除对已删除方法（setPlotStyle, setMeshColor, setDataColor 等）的调用，这些属性后续移到 item 层。暂时只设置背景色、坐标系统、标题、光照。

### 9. 更新全局引用

在 `src/plot3d/` 下所有文件中：
- `Plot3D` → `Qwt3DPlot`
- `Qwt3D::Plot3D` → `Qwt3DPlot`
- 移除 `namespace Qwt3D { ... }`

**注意**：`SurfacePlot` 仍然继承 `Plot3D`，此阶段改为继承 `Qwt3DPlot`。但 SurfacePlot 中引用的已删除方法（`calculateHull`, `createData`, `displayLists` 等）会导致编译错误。**此阶段暂时注释掉 SurfacePlot 中引用已删除方法的代码**，让 SurfacePlot 变成空壳。Task 06 会将 SurfacePlot 重构为 Qwt3DSurface item。

- 所有 .cpp 文件中的 `using namespace Qwt3D;` 和 `using namespace std;` 需要移除
- 移除后，裸用的类型需要补全全局作用域限定或补全 include

### 10. 删除 stub 文件

删除以下文件（空壳，新架构下无意义）：
- `qwt3d_graphplot.h`
- `qwt3d_multiplot.h`
- `qwt3d_volumeplot.h`
- `qwt3d_dataviews.cpp`

从 CMakeLists.txt 中移除 `qwt3d_dataviews.cpp`（它本来就不在编译列表中）。

## 构建通过

```powershell
.\build.ps1 rebuild -Examples OFF -Playground OFF
```

构建必须通过。SurfacePlot 暂时是空壳（编译通过但不绘图），examples 会编译失败，使用 `-Examples OFF` 跳过。

## git 提交

```powershell
git add -A
git commit -m "refactor(plot3d): rewrite Plot3D -> Qwt3DPlot as pure rendering window

- Remove data/enrichment/display-list responsibilities from Qwt3DPlot
- Add Qwt3DPlotItem list management (attach/detach/itemList)
- Use QMatrix4x4 for CPU-side view transform calculation
- Remove legacy GL matrix stack calls (glRotatef/glTranslatef/glPushMatrix)
- Remove legacy GL lighting calls (glMaterialfv/glLightfv)
- Store lighting params on CPU for future shader uniform upload
- SurfacePlot temporarily stubbed (will be refactored to Qwt3DSurface item)
- Delete stub files: graphplot, multiplot, volumeplot, dataviews"
```
