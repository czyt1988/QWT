# 06-plan: SurfacePlot → Qwt3DSurface item 重构

> **前置阅读**：执行本任务前，必须完整阅读 `plan3D/ARCHITECTURE-PRINCIPLES.md`，特别是第三章第 3.3 节「具体绘图 item」、第五章「OpenGL 技术规范」（VBO/VAO 用法）和第九章「不可触碰的约束」。

## 目标（Objective）

将 `SurfacePlot`（当前是 `Qwt3DPlot` 子类 widget）重构为 `Qwt3DSurface`（`Qwt3DPlotItem` 子类），使用 VBO/VAO 替代 display list + immediate mode 进行曲面几何渲染。

## 涉及文件

| 文件 | 操作 |
|------|------|
| `qwt3d_surface.h` | **新建**：Qwt3DSurface 头文件 |
| `qwt3d_surface.cpp` | **新建**：Qwt3DSurface 实现（合并原 surfaceplot.cpp + gridplot.cpp + meshplot.cpp 逻辑） |
| `qwt3d_surface_p.h` | **新建**：Qwt3DSurface 私有数据 |
| `qwt3d_surfaceplot.h` / `.cpp` | **删除**：被 Qwt3DSurface 替代 |
| `qwt3d_surfaceplot_p.h` | **删除** |
| `qwt3d_gridplot.cpp` | **删除**：逻辑合并到 Qwt3DSurface |
| `qwt3d_meshplot.cpp` | **删除**：逻辑合并到 Qwt3DSurface |
| `qwt3d_function.h` / `qwt3d_function.cpp` | **修改**：SurfacePlot& → Qwt3DSurface& |
| `qwt3d_parametricsurface.h` / `qwt3d_parametricsurface.cpp` | **修改**：同上 |
| `qwt3d_gridmapping.h` / `qwt3d_gridmapping.cpp` | **修改**：SurfacePlot* → Qwt3DSurface* |
| `CMakeLists.txt` | 修改：更新文件列表 |

## 前提条件（Prerequisites）

- Task 01–05 已完成
- `Qwt3DPlot` 已重写为纯渲染窗口，具备 item 列表管理
- `Qwt3DPlotItem` 基类已创建
- 构建通过（SurfacePlot 当前是空壳）

## 执行步骤（Steps）

### 0. 创建 Shader 文件

在 `src/plot3d/shaders/` 目录下创建以下 shader 文件：

**surface.vert** — 曲面顶点着色器：
```glsl
#version 330
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;
uniform mat4 uModelView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;
out vec3 vNormal;
out vec4 vColor;
out vec3 vFragPos;
void main() {
    vec4 viewPos = uModelView * vec4(aPosition, 1.0);
    vFragPos = viewPos.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    vColor = aColor;
    gl_Position = uProjection * viewPos;
}
```

**surface.frag** — 曲面片元着色器：
```glsl
#version 330
in vec3 vNormal;
in vec4 vColor;
in vec3 vFragPos;
out vec4 fragColor;
uniform bool uUseLighting;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform float uShininess;
uniform bool uUseOverrideColor;
uniform vec4 uOverrideColor;
void main() {
    if (uUseOverrideColor) {
        fragColor = uOverrideColor;
        return;
    }
    if (!uUseLighting) {
        fragColor = vColor;
        return;
    }
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 ambient = 0.3 * vColor.rgb;
    vec3 diffuse = diff * uLightColor * vColor.rgb;
    fragColor = vec4(ambient + diffuse, vColor.a);
}
```

同时创建 `src/plot3d/shaders/shaders.qrc` Qt 资源文件，在 CMakeLists.txt 中添加 AUTORCC 和 .qrc 文件。

### 1. 创建 qwt3d_surface.h

```cpp
#ifndef QWT3D_SURFACE_H
#define QWT3D_SURFACE_H

#include "qwt3d_plotitem.h"
#include "qwt3d_types.h"
#include "qwt3d_color.h"
#include "qwt3d_enrichment.h"

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

class Qwt3DSurface;

class QWT3D_EXPORT Qwt3DSurface : public Qwt3DPlotItem
{
    Q_OBJECT
    QWT_DECLARE_PRIVATE(Qwt3DSurface)

public:
    Qwt3DSurface();
    ~Qwt3DSurface() override;

    // Data loading (from original SurfacePlot)
    void loadFromData(Triple** data, unsigned int columns, unsigned int rows,
                      bool uperiodic = false, bool vperiodic = false);
    void loadFromData(double** data, unsigned int columns, unsigned int rows,
                      double minx, double maxx, double miny, double maxy);
    void loadFromData(TripleField const& nodes, CellField const& poly);

    // Resolution
    int resolution() const;

    // Floor style
    void setFloorStyle(FLOORSTYLE style);
    FLOORSTYLE floorStyle() const;

    // Normals
    void showNormals(bool);
    bool normals() const;
    void setNormalLength(double);
    double normalLength() const;
    void setNormalQuality(int);
    int normalQuality() const;

    // Style (moved from Plot3D)
    void setPlotStyle(PLOTSTYLE style);
    PLOTSTYLE plotStyle() const;
    void setDataColor(Qwt3DColor* color);
    const Qwt3DColor* dataColor() const;
    void setMeshColor(RGBA color);
    RGBA meshColor() const;
    void setMeshLineWidth(double width);
    double meshLineWidth() const;
    void setIsolines(int n);
    int isolines() const;
    void setSmoothMesh(bool smooth);
    bool smoothMesh() const;
    void setPolygonOffset(double offset);
    double polygonOffset() const;

    // Enrichments (moved from Plot3D)
    Qwt3DEnrichment* addEnrichment(Qwt3DEnrichment const&);
    bool degrade(Qwt3DEnrichment*);

    // Qwt3DPlotItem interface
    void draw() override;
    ParallelEpiped hull() const override;

Q_SIGNALS:
    void resolutionChanged(int);

public Q_SLOTS:
    void setResolution(int res);

protected:
    // Data access for subclasses
    Qwt3DGridData* gridData() const;
    Qwt3DCellData* cellData() const;
    bool isGridData() const;

private:
    void buildVBO();
};

#endif
```

### 2. 创建 qwt3d_surface_p.h

包含从原 `SurfacePlot::PrivateData` 迁移的成员：
- `bool m_dataNormals`, `double m_normalLength`, `int m_normalQuality`
- `int m_resolution`, `FLOORSTYLE m_floorStyle`
- `Qwt3DGridData* m_actualDataG`, `Qwt3DCellData* m_actualDataC`
- 新增：`PLOTSTYLE m_plotStyle`, `Qwt3DColor* m_dataColor`, `RGBA m_meshColor`
- 新增：`double m_meshLineWidth`, `int m_isolines`, `bool m_smoothDataMesh`
- 新增：`double m_polygonOffset`, `Qwt3DEnrichment* m_userPlotStyle`
- 新增：`std::list<Qwt3DEnrichment*> m_enrichmentList`
- 新增（GL 资源，从类直接成员移入）：`bool m_vboDirty`, `QOpenGLBuffer m_vertexBuffer`, `QOpenGLBuffer m_indexBuffer`（EBO）, `QOpenGLVertexArrayObject m_vao`

### 3. 创建 qwt3d_surface.cpp

**Vertex 结构体定义**（在 qwt3d_surface.cpp 或 _p.h 中）：
```cpp
struct SurfaceVertex {
    QVector3D position;
    QVector3D normal;
    QVector4D color;
};
```
顶点属性布局：
- location 0 = position (3 floats, offset 0)
- location 1 = normal (3 floats, offset 12)
- location 2 = color (4 floats, offset 24)
- stride = sizeof(SurfaceVertex) = 40 bytes

**数据加载**：从原 `qwt3d_gridplot.cpp` 和 `qwt3d_meshplot.cpp` 迁移 `loadFromData`、`readIn`、`calcNormals`、`sewPeriodic` 逻辑，保持算法不变。

**索引缓冲区生成（GridData）：**
- 遍历 (cols-1) × (rows-1) 个四边形
- 每个四边形生成 2 个三角形（6 个索引）：
  - 三角形 1: [i*rows+j, (i+1)*rows+j, i*rows+(j+1)]
  - 三角形 2: [(i+1)*rows+j, (i+1)*rows+(j+1), i*rows+(j+1)]
- 需要第二个 QOpenGLBuffer (Index Buffer / EBO) 存储索引
- resolution > 1 时，在索引生成时跳过顶点（索引缓冲区实现，不重建 VBO）

**索引缓冲区生成（CellData）：**
- 对每个多边形 cell 使用 fan 分解：以第一个顶点为中心，生成 (n-2) 个三角形

**线段索引缓冲区生成（GridData）：**
- 网格线由水平边和垂直边组成
- 水平边：每列 (rows-1) 条边，共 cols 列 → cols*(rows-1) 条边
- 垂直边：每行 (cols-1) 条边，共 rows 行 → rows*(cols-1) 条边
- 每条边 2 个索引（起点和终点）
- 水平边索引: [i*rows+j, i*rows+(j+1)] for i in 0..cols-1, j in 0..rows-2
- 垂直边索引: [i*rows+j, (i+1)*rows+j] for i in 0..cols-2, j in 0..rows-1
- 存储到第二个 QOpenGLBuffer (LineEBO) 或同一 EBO 的不同偏移区域

**resolution 处理：**
- resolution > 1 时，在索引生成时按步长跳过（三角形索引和线段索引均跳过 resolution 步长的顶点）
- VBO 保持全量顶点数据不变，仅索引缓冲区变化

**GL 上下文生命周期：**
- VBO/VAO 创建时机：在 `draw()` 中检测 dirty 标志后延迟创建/更新（此时 GL 上下文已由 paintGL() 确保 current）
- 数据变化时（setData/loadFromData）标记 VBO 为 dirty
- 连接 plot 的 `aboutToBeDestroyed` 信号（或类似机制）以清理 GL 资源
- 不在构造函数中创建 VBO（此时 GL 上下文可能不可用）

**draw() 完整流程：**
1. 检测 m_vboDirty → 如脏则 buildVBO()
2. 绑定 shader program（从 plot 获取或自身持有）
3. 设置 uniform：uModelView, uProjection, uNormalMatrix（从 plot 获取）
4. 绑定 VAO
5. 根据 plotStyle() 选择渲染策略：
   - FILLED: 仅绘制填充三角形 — glDrawElements(GL_TRIANGLES, triIndexCount, ...)
   - FILLEDMESH: 先绘制填充三角形，再绘制网格线（两次 draw call）
     - Pass 1: glDrawElements(GL_TRIANGLES, triIndexCount, ...) — 填充面
     - Pass 2: 绑定线段 EBO, glDrawElements(GL_LINES, lineIndexCount, ...) — 网格线
   - WIREFRAME: 仅绘制网格线 — glDrawElements(GL_LINES, lineIndexCount, ...)
   - HIDDENLINE: 先用背景色绘制填充三角形（隐藏面消除），再绘制网格线
     - Pass 1: 使用背景色 uniform 覆盖顶点颜色, glDrawElements(GL_TRIANGLES, triIndexCount, ...)
     - Pass 2: 绑定线段 EBO, glDrawElements(GL_LINES, lineIndexCount, ...) — 网格线
   - POINTS: glDrawArrays(GL_POINTS, 0, vertexCount)
6. 解绑 VAO、shader

**buildVBO() 方法**（替代原 `createDataG()` / `createDataC()`）：
- 遍历 GridData 或 CellData 的顶点
- 构建顶点数组：position(xyz) + normal(xyz) + color(rgba)
- 使用 `Qwt3DColor` functor 计算每顶点颜色
- 上传到 `m_vertexBuffer`（`allocate()`）
- 配置 `m_vao` 顶点属性
- 生成索引缓冲区并上传到 `m_indexBuffer`（EBO）

**hull() 实现说明：**
- 原 `calculateHull()` 调用 `setHull()` 修改 plot 状态
- 新架构下 `hull() const` 返回值，不修改 plot
- 实现中遍历数据顶点计算包围盒并返回

**坐标系统更新流程：**
- `loadFromData()` 后调用 `itemChanged()` 通知 plot
- plot 的 `itemChanged()` 实现中重新计算所有 item 的 hull 并集，更新坐标系统

**地板渲染（createFloorData/Data2FloorG/C/Isolines2FloorG/C）：**
- 暂不迁移地板渲染逻辑。在 `setFloorStyle()` 中添加 TODO 注释
- 后续可作为独立 item（如 Qwt3DFloorProjection）实现

**关键变化**：
- 原 `createData()` 在 display list 内编译 GL 调用 → 现 `draw()` 直接使用 VBO/VAO
- 原 `glBegin(GL_TRIANGLE_STRIP)` 逐行绘制 → 现 `glDrawElements` 一次性提交
- 原 `setColorFromVertexG()` 逐顶点调用 `glColor4f` → 现颜色写入 VBO 顶点数据
- 原法线绘制通过 Arrow enrichment → 保持 enrichment 机制但改用 VBO

### 4. 信号槽处理

**信号槽处理：**
推荐方案：让 `Qwt3DPlotItem` 继承 `QObject`（在 Plan 04 中已有设计说明修改），所有 item 都有信号能力。如果 Plan 04 未修改，则 `Qwt3DSurface` 使用多继承 `QObject, Qwt3DPlotItem`（QObject 必须是第一个基类）。
备选方案：移除 `resolutionChanged` 信号，改用 `itemChanged()` 通知 plot 重绘。

### 5. 删除旧文件

- 删除 `qwt3d_surfaceplot.h`、`qwt3d_surfaceplot_p.h`、`qwt3d_surfaceplot.cpp`
- 删除 `qwt3d_gridplot.cpp`、`qwt3d_meshplot.cpp`

### 6. 同步处理下游依赖

- 修改 `qwt3d_function.h` / `qwt3d_function.cpp`：将所有 `SurfacePlot&` 参数改为 `Qwt3DSurface&`
- 修改 `qwt3d_parametricsurface.h` / `qwt3d_parametricsurface.cpp`：同上
- 修改 `qwt3d_gridmapping.h` / `qwt3d_gridmapping.cpp`：将 `SurfacePlot*` 改为 `Qwt3DSurface*`

### 7. 更新 CMakeLists.txt

- 移除：`qwt3d_surfaceplot.h`、`qwt3d_surfaceplot.cpp`、`qwt3d_gridplot.cpp`、`qwt3d_meshplot.cpp`
- 添加：`qwt3d_surface.h`、`qwt3d_surface.cpp`
- 添加 Qt OpenGL 依赖：`Qt6::OpenGL`（已有）
- 添加 `src/plot3d/shaders/shaders.qrc` 到 target sources
- 启用 AUTORCC（或在 CMake 中添加 qt_add_resources）

### 8. 更新 Qwt3DPlot::paintGL

确保 `paintGL()` 中遍历 `m_items` 调用 `item->draw()`：

```cpp
for (auto* item : m_items) {
    item->draw();
}
```

## 构建通过

```powershell
.\build.ps1 rebuild -Examples OFF -Playground OFF
```

构建必须通过。`Qwt3DSurface` 应能加载数据并渲染基本曲面。

## git 提交

```powershell
git add -A
git commit -m "refactor(plot3d): rewrite SurfacePlot as Qwt3DSurface item with VBO/VAO

- Convert SurfacePlot (widget) to Qwt3DSurface (Qwt3DPlotItem)
- Replace display list + immediate mode with VBO/VAO + glDrawElements
- Move data/color/style/enrichment management from Plot3D to item
- Merge gridplot.cpp + meshplot.cpp logic into qwt3d_surface.cpp
- Delete old SurfacePlot files (surfaceplot.h/.cpp, gridplot.cpp, meshplot.cpp)"
```
