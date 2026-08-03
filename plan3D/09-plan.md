# 09-plan: 主题与 IO 系统适配

> **前置阅读**：执行本任务前，必须完整阅读 `plan3D/ARCHITECTURE-PRINCIPLES.md`，特别是第二章「命名规范」、第四章第 4.1 节（Qwt3DTheme）和第九章「不可触碰的约束」。

## 目标（Objective）

将 `Qwt3DTheme` 和 I/O 系统（IO、VectorWriter、PixmapWriter、NativeReader）适配到新的 Plot+Item 架构，去命名空间并更新所有引用。

## 涉及文件

| 文件 | 操作 |
|------|------|
| `qwt3d_theme.h` / `.cpp` | 修改：去命名空间，适配 Qwt3DPlot + Qwt3DPlotItem |
| `qwt3d_io.h` / `.cpp` | 修改：去命名空间，更新引用 |
| `qwt3d_io_gl2ps.h` / `.cpp` | 修改：去命名空间，更新引用 |
| `qwt3d_io_reader.h` / `.cpp` | 修改：去命名空间，更新引用 |
| `CMakeLists.txt` | 修改：更新文件引用 |

## 前提条件（Prerequisites）

- Task 01–08 已完成
- `Qwt3DPlot` 和 `Qwt3DSurface` 已完成重构
- 构建通过

## 执行步骤（Steps）

### 1. Qwt3DTheme 适配

**qwt3d_theme.h：**
- 移除 `namespace Qwt3D { ... }`
- `Qwt3DTheme` 类名已符合命名规范，无需重命名
- `apply(Plot3D* plot)` → `apply(Qwt3DPlot* plot)`
- 移除对已删除方法的调用（`setPlotStyle`, `setMeshColor`, `setDataColor`, `setShading`, `setSmoothMesh`, `setIsolines` 等）
- 新增：`applyToItem(Qwt3DPlotItem* item)` 方法，将颜色/样式属性应用到 item

**qwt3d_theme.cpp：**
- `apply(Qwt3DPlot* plot)` 中：
  - 保留：`setBackgroundColor`, 坐标系统颜色, 标题, 光照
  - 移除：对 `setPlotStyle/setMeshColor/setDataColor` 等的调用
  - 新增：遍历 `plot->itemList()`，对每个 `Qwt3DSurface` 类型的 item 调用 `applyToItem()`
  - 在 `apply(Qwt3DPlot* plot)` 方法末尾，将 `plot->updateData()` 调用替换为 `plot->update()`（Plan 05 已删除 `updateData()` 方法）
- `applyToItem(Qwt3DPlotItem* item)` 中：
  - 如果是 `Qwt3DSurface`：设置 meshColor, dataColorPreset, plotStyle, shading, smoothMesh 等
  - 使用 `dynamic_cast<Qwt3DSurface*>(item)` 判断类型

**applyToItem() 完整实现：**
```cpp
void Qwt3DTheme::applyToItem(Qwt3DPlotItem* item) const
{
    auto* surface = dynamic_cast<Qwt3DSurface*>(item);
    if (!surface)
        return;
    surface->setMeshColor(m_meshColor);
    surface->setMeshLineWidth(m_meshLineWidth);
    surface->setSmoothMesh(m_smoothMesh);
    surface->setDataColorPreset(m_dataColorPreset);
    surface->setPlotStyle(m_plotStyle);
    surface->setShading(m_shading);
}
```
注意：需验证 Qwt3DSurface（由 Plan 06 创建）确实有这些 setter 方法。如果缺少 `setDataColorPreset`，需在 Plan 06 中补充。

**跨计划依赖提示**：`surface->setShading(m_shading)` 和 `surface->setDataColorPreset(m_dataColorPreset)` 调用要求 `Qwt3DSurface` 具有这些方法。执行前需验证 Plan 06 的 Qwt3DSurface API 列表是否已包含：
- `setShading(SHADINGSTYLE)` / `shading()` — 如果缺少，需在 Plan 06 中补充
- `setDataColorPreset(QString)` — 如果缺少，可改为 `surface->setDataColor(new Qwt3DColorMapColor(surface, m_dataColorPreset))`

建议在 Plan 06 的 Qwt3DSurface 头文件中增加以下方法：
```cpp
void setShading(SHADINGSTYLE style);
SHADINGSTYLE shading() const;
```
并在 PrivateData 中增加 `SHADINGSTYLE m_shading` 成员。

**属性归属区分：**

Plot 级属性（在 `apply(Qwt3DPlot*)` 中设置）：
- backgroundColor / backgroundRGBAColor
- 坐标系统颜色（axisColor, gridColor 等）
- 标题
- 光照参数（light components, shininess, lighting enabled）
- 材质参数（material components）

Item 级属性（在 `applyToItem(Qwt3DPlotItem*)` 中设置）：
- meshColor, meshLineWidth
- dataColorPreset / dataColor
- plotStyle
- shading
- smoothMesh

注意：`shading` 和 `shininess` 的归属需要区分——`shininess` 是光照参数（plot 级），`shading` 是渲染模式（item 级）。

**Qwt3DColorMapColor 构造函数变更：**
- 当前：`ColorMapColor(Plot3D* plot, ...)`
- 新：`Qwt3DColorMapColor(Qwt3DPlotItem* item, ...)` 或 `Qwt3DColorMapColor(Qwt3DSurface* surface, ...)`
- 需确认 z 范围从何处获取（原来通过 plot->actualData()，现在需要从 item 的 hull() 获取）

### 2. IO 系统适配

**qwt3d_io.h / qwt3d_io.cpp：**
- 移除 `namespace Qwt3D { ... }`
- `class IO` → `class QWT3D_EXPORT Qwt3DIO`
- `class PixmapWriter` → `class QWT3D_EXPORT Qwt3DPixmapWriter`
- IO 的 `Function` typedef：`bool (*)(Plot3D*, ...)` → `bool (*)(Qwt3DPlot*, ...)`
- `Functor::operator()(Plot3D*, QString)` → `operator()(Qwt3DPlot*, QString)`
- `IO::save()` / `IO::load()` 中 `Plot3D*` → `Qwt3DPlot*`
- `Plot3D::savePixmap()` / `Plot3D::saveVector()` / `Plot3D::save()` 移到 `Qwt3DPlot` 中（或保留为 Qwt3DPlot 的方法）

**IO 内部嵌套类引用更新：**
- `IO::Function` → `Qwt3DIO::Function`（typedef）
- `IO::Functor` → `Qwt3DIO::Functor`
- `IO::Entry` → `Qwt3DIO::Entry`
- `IO::Wrapper` → `Qwt3DIO::Wrapper`
- `IO::FormatCompare` → `Qwt3DIO::FormatCompare`
- `IO::FormatCompare2` → `Qwt3DIO::FormatCompare2`

**qwt3d_io_gl2ps.h / qwt3d_io_gl2ps.cpp：**
- 移除 `namespace Qwt3D { ... }`
- `class VectorWriter` → `class QWT3D_EXPORT Qwt3DVectorWriter`
- `operator()(Plot3D*, QString)` → `operator()(Qwt3DPlot*, QString)`
- **gl2ps 依赖断裂处理**：
  - `drawDeviceText()` 中调用的 `World2ViewPort()`/`ViewPort2World()` 已在 Plan 08 中删除
  - 替代方案：使用 `Qwt3DPlot` 提供的 `worldToScreen()`/`screenToWorld()` 方法
  - 或在 gl2ps 条件编译块内内联实现 viewport 转换（使用 `QMatrix4x4::map()`）
- `plot->updateData()` 调用替换为 `plot->update()`
- `Label::useDeviceFonts()` 调用：确认 Plan 08 后此方法是否仍存在；如已移除，在条件编译块内移除相关调用

**setDevice* 自由函数处理：**
`qwt3d_io_gl2ps.cpp` 底部定义了 5 个全局函数，被 7 个文件共 15 处调用：
- `setDeviceLineWidth()` — Plan 08 现代化后，调用方应直接使用 shader uniform
- `setDevicePointSize()` — 同上
- `drawDevicePixels()` — 已替换为纹理四边形方案
- `drawDeviceText()` — 已替换为 Qwt3DLabel 纹理渲染
- `setDevicePolygonOffset()` — Core Profile 中 `glPolygonOffset` 可用，保留

处理方式：
- 验证 Plan 05/06/07/08 后这些函数是否还有调用者
- 如果已无调用者，删除这些函数的定义和声明
- 如果仍有调用者（gl2ps 条件编译块内），保留在条件编译块内

**gl2ps 条件编译方案：**
- 定义宏 `QWT3D_ENABLE_GL2PS`（在 CMakeLists.txt 中根据 GL2PS 是否可用设置）
- 用 `#ifdef QWT3D_ENABLE_GL2PS` 包裹 gl2ps 相关代码
- 在 `setupHandler()` 中条件注册 gl2ps handler
- 在 `saveVector()` 中条件返回错误或回退到 pixmap
- CMakeLists.txt 中增加：
```cmake
if (QWT_CONFIG_GL2PS)
    add_compile_definitions(QWT3D_ENABLE_GL2PS)
endif()
```

**qwt3d_io_reader.h / qwt3d_io_reader.cpp：**
- 移除 `namespace Qwt3D { ... }`
- `class NativeReader` → `class QWT3D_EXPORT Qwt3DNativeReader`
- `operator()(Plot3D*, QString)` → `operator()(Qwt3DPlot*, QString)`
- **关键变更**：`NativeReader::operator()` 中 `dynamic_cast<SurfacePlot*>(plot)` → `dynamic_cast<Qwt3DSurface*>(plot)`（但 plot 是 Qwt3DPlot，不是 item）
  - 调整逻辑：`Qwt3DPlot` 持有 item 列表，需要找到第一个 `Qwt3DSurface` item
  - `auto* surface = ...; // find first Qwt3DSurface in plot->itemList()`
  - `surface->loadFromData(...)`

**Qwt3DNativeReader::operator() 完整实现：**
```cpp
bool Qwt3DNativeReader::operator()(Qwt3DPlot* plot, const QString& fname)
{
    // ... read data from file (保持原逻辑) ...

    Qwt3DSurface* surface = nullptr;
    for (auto* item : plot->itemList()) {
        surface = dynamic_cast<Qwt3DSurface*>(item);
        if (surface)
            break;
    }
    if (!surface) {
        surface = new Qwt3DSurface();
        surface->attach(plot);
    }
    surface->loadFromData(data, xmesh, ymesh, minx, maxx, miny, maxy);
    return true;
}
```

**PIMPL 宏更新：**
- `QWT_DECLARE_PRIVATE(PixmapWriter)` → `QWT_DECLARE_PRIVATE(Qwt3DPixmapWriter)`
- `QWT_DECLARE_PRIVATE(VectorWriter)` → `QWT_DECLARE_PRIVATE(Qwt3DVectorWriter)`
- `QWT_DECLARE_PRIVATE(NativeReader)` → `QWT_DECLARE_PRIVATE(Qwt3DNativeReader)`

### 3. 更新全局引用

在 `src/plot3d/` 下所有文件中：
- `IO` → `Qwt3DIO`
- `VectorWriter` → `Qwt3DVectorWriter`
- `PixmapWriter` → `Qwt3DPixmapWriter`
- `NativeReader` → `Qwt3DNativeReader`
- `Plot3D` → `Qwt3DPlot`（IO 上下文中）
- `SurfacePlot` → `Qwt3DSurface`（reader 上下文中）
- 移除所有 `using namespace Qwt3D;` 和 `namespace Qwt3D { ... }` 块

### 4. 清理 qwt3d_global.h

确认 `qwt3d_global.h` 中不再有任何 `namespace Qwt3D` 痕迹。该文件本身无命名空间，但确保所有 include 它的文件也不使用命名空间。

### 验证步骤

构建通过后，执行以下手动验证：
1. 导出 PNG（验证 PixmapWriter 正常工作）
2. 读取 .mes 文件（验证 NativeReader 正常工作）
3. 应用一个主题预设（验证 Qwt3DTheme::apply 和 applyToItem 正常工作）
4. 如条件编译启用 gl2ps，导出 EPS（验证矢量导出回退工作）

## 构建通过

```powershell
.\build.ps1 rebuild -Examples OFF -Playground OFF
```

构建必须通过。

## git 提交

```powershell
git add -A
git commit -m "refactor(plot3d): adapt theme and IO system to new Plot+Item architecture

- Qwt3DTheme: apply() targets Qwt3DPlot, add applyToItem() for item-level properties
- IO -> Qwt3DIO, VectorWriter -> Qwt3DVectorWriter
- PixmapWriter -> Qwt3DPixmapWriter, NativeReader -> Qwt3DNativeReader
- NativeReader: find Qwt3DSurface in plot->itemList() instead of casting plot
- gl2ps: mark as Compatibility Profile fallback (Core Profile TODO)
- Remove namespace Qwt3D from all remaining files"
```
