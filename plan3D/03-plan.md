# 03-plan: 颜色 / 装饰 / 刻度组件重构

> **前置阅读**：执行本任务前，必须完整阅读 `plan3D/ARCHITECTURE-PRINCIPLES.md`，特别是第二章「命名规范」、第四章第 4.1 节和第九章「不可触碰的约束」。

## 目标（Objective）

移除颜色、装饰、刻度组件的 `namespace Qwt3D`，重命名为 `Qwt3D` 前缀类名。同时清理 Task 02 中遗留的临时别名。

## 涉及文件

| 文件 | 操作 |
|------|------|
| `qwt3d_color.h` / `.cpp` | 修改：去命名空间，重命名 |
| `qwt3d_colormap_color.h` / `.cpp` | 修改：去命名空间，重命名 |
| `qwt3d_enrichment.h` | 修改：去命名空间，重命名 |
| `qwt3d_enrichment_std.h` / `.cpp` | 修改：去命名空间，重命名 |
| `qwt3d_scale.h` / `.cpp` | 修改：去命名空间，重命名 |
| `qwt3d_autoscaler.h` / `.cpp` | 修改：去命名空间，重命名 |
| `qwt3d_autoptr.h` | 修改：去命名空间，重命名 |

## 前提条件（Prerequisites）

- Task 01、Task 02 已完成
- 构建通过

## 执行步骤（Steps）

### 1. 类重命名映射

| 旧名 | 新名 | 文件 |
|------|------|------|
| `Color` | `Qwt3DColor` | `qwt3d_color.h/.cpp` |
| `StandardColor` | `Qwt3DStandardColor` | `qwt3d_color.h/.cpp` |
| `ColorMapColor` | `Qwt3DColorMapColor` | `qwt3d_colormap_color.h/.cpp` |
| `Enrichment` | `Qwt3DEnrichment` | `qwt3d_enrichment.h` |
| `VertexEnrichment` | `Qwt3DVertexEnrichment` | `qwt3d_enrichment.h` |
| `CrossHair` | `Qwt3DCrossHair` | `qwt3d_enrichment_std.h/.cpp` |
| `Dot` | `Qwt3DDot` | `qwt3d_enrichment_std.h/.cpp` |
| `Cone` | `Qwt3DCone` | `qwt3d_enrichment_std.h/.cpp` |
| `Arrow` | `Qwt3DArrow` | `qwt3d_enrichment_std.h/.cpp` |
| `Scale` | `Qwt3DScale` | `qwt3d_scale.h/.cpp` |
| `LinearScale` | `Qwt3DLinearScale` | `qwt3d_scale.h/.cpp` |
| `LogScale` | `Qwt3DLogScale` | `qwt3d_scale.h/.cpp` |
| `AutoScaler` | `Qwt3DAutoScaler` | `qwt3d_autoscaler.h/.cpp` |
| `LinearAutoScaler` | `Qwt3DLinearAutoScaler` | `qwt3d_autoscaler.h/.cpp` |
| `ClonePtr<T>` | `Qwt3DClonePtr<T>` | `qwt3d_autoptr.h` |

### 2. qwt3d_color.h / qwt3d_color.cpp

- 移除 `namespace Qwt3D { ... }`
- `class Color` → `class QWT3D_EXPORT Qwt3DColor`
- `class StandardColor : public Color` → `class QWT3D_EXPORT Qwt3DStandardColor : public Qwt3DColor`
- PIMPL 类名同步更新（PIMPL 更新仅适用于 `StandardColor`，`Color` 基类不使用 PIMPL）
- `Plot3D*` 引用保持为 `Qwt3D::Plot3D*`（Task 05 会将 Plot3D 重命名为 Qwt3DPlot，届时统一替换所有 Qwt3D::Plot3D 引用为 Qwt3DPlot）
- 头文件中使用 `namespace Qwt3D { class Plot3D; }` 做前向声明
- .cpp 文件中使用 `Qwt3D::Plot3D*` 全限定名
- `Color::operator()` 的签名中 `Triple` 已全局，无需改

### 3. qwt3d_colormap_color.h / qwt3d_colormap_color.cpp

- 移除 `namespace Qwt3D { ... }`
- `class ColorMapColor : public Color` → `class QWT3D_EXPORT Qwt3DColorMapColor : public Qwt3DColor`
- `Plot3D*` 引用保持为 `Qwt3D::Plot3D*`（Task 05 会将 Plot3D 重命名为 Qwt3DPlot，届时统一替换所有 Qwt3D::Plot3D 引用为 Qwt3DPlot）
- 头文件中使用 `namespace Qwt3D { class Plot3D; }` 做前向声明
- .cpp 文件中使用 `Qwt3D::Plot3D*` 全限定名

### 4. qwt3d_enrichment.h

- 移除 `namespace Qwt3D { ... }`
- `class Enrichment` → `class QWT3D_EXPORT Qwt3DEnrichment`
- `class VertexEnrichment : public Enrichment` → `class QWT3D_EXPORT Qwt3DVertexEnrichment : public Qwt3DEnrichment`
- `Plot3D` 引用保持为 `Qwt3D::Plot3D`（Task 05 会将 Plot3D 重命名为 Qwt3DPlot，届时统一替换所有 Qwt3D::Plot3D 引用为 Qwt3DPlot）
- 头文件中使用 `namespace Qwt3D { class Plot3D; }` 做前向声明
- .cpp 文件中使用 `Qwt3D::Plot3D*` 全限定名
- `assign(Plot3D const&)` → `assign(Qwt3D::Plot3D const&)`

### 5. qwt3d_enrichment_std.h / qwt3d_enrichment_std.cpp

- 移除 `namespace Qwt3D { ... }`
- 4 个类全部重命名（CrossHair/Dot/Cone/Arrow 加 Qwt3D 前缀）
- PIMPL 类名同步更新
- `VertexEnrichment` 引用 → `Qwt3DVertexEnrichment`
- `Enrichment::clone()` 返回类型 `Enrichment*` → `Qwt3DEnrichment*`，4 个 enrichment_std 类的 `clone()` 返回类型也需更新
- `qwt3d_enrichment.h` 中 `return Qwt3D::Enrichment::VERTEXENRICHMENT;` → `return Qwt3DEnrichment::VERTEXENRICHMENT;`。在 `qwt3d_surfaceplot.cpp` 中 `Enrichment::VERTEXENRICHMENT` → `Qwt3DEnrichment::VERTEXENRICHMENT`

### 6. qwt3d_scale.h / qwt3d_scale.cpp

- 移除 `namespace Qwt3D { ... }`
- `class Scale` → `class QWT3D_EXPORT Qwt3DScale`
- `class LinearScale : public Scale` → `class QWT3D_EXPORT Qwt3DLinearScale : public Qwt3DScale`
- `class LogScale : public Scale` → `class QWT3D_EXPORT Qwt3DLogScale : public Qwt3DScale`
- PIMPL 类名同步更新
- `ClonePtr<Scale>` → `Qwt3DClonePtr<Qwt3DScale>`
- `friend class Axis` → `friend class Qwt3DAxis`
- `friend class ClonePtr<Scale>` → `friend class Qwt3DClonePtr<Qwt3DScale>`
- `Scale::clone()` 返回类型 `Scale*` → `Qwt3DScale*`，`LinearScale::clone()` → `Qwt3DScale*`，`LogScale::clone()` → `Qwt3DScale*`

### 7. qwt3d_autoscaler.h / qwt3d_autoscaler.cpp

- 移除 `namespace Qwt3D { ... }`
- `class AutoScaler` → `class QWT3D_EXPORT Qwt3DAutoScaler`
- `class LinearAutoScaler : public AutoScaler` → `class QWT3D_EXPORT Qwt3DLinearAutoScaler : public Qwt3DAutoScaler`
- PIMPL 类名同步更新（PIMPL 更新仅适用于 `LinearAutoScaler`，`AutoScaler` 基类不使用 PIMPL）
- `ClonePtr<AutoScaler>` → `Qwt3DClonePtr<Qwt3DAutoScaler>`
- `friend class LinearScale` → `friend class Qwt3DLinearScale`
- `AutoScaler::clone()` 返回 `AutoScaler*` → `Qwt3DAutoScaler*`，`LinearAutoScaler::clone()` → `Qwt3DAutoScaler*`

### 8. qwt3d_autoptr.h

- 移除 `namespace Qwt3D { ... }`
- `template <typename T> class ClonePtr` → `template <typename T> class QWT3DClonePtr`

### 9. 清理 Task 02 的过渡引用

在 Task 02 中，`qwt3d_axis.h/.cpp` 和 `qwt3d_colorlegend.h/.cpp` 保留了 `#include "qwt3d_scale.h"` 和 `#include "qwt3d_autoscaler.h"`，并使用 `Qwt3D::Scale`、`Qwt3D::ClonePtr<Qwt3D::Scale>` 等全限定名引用。

本任务完成后（Scale 已重命名为 Qwt3DScale，ClonePtr 已重命名为 Qwt3DClonePtr），在这些文件中：
- 将 `Qwt3D::Scale` → `Qwt3DScale`
- 将 `Qwt3D::LinearScale` → `Qwt3DLinearScale`
- 将 `Qwt3D::LogScale` → `Qwt3DLogScale`
- 将 `Qwt3D::AutoScaler` → `Qwt3DAutoScaler`
- 将 `Qwt3D::LinearAutoScaler` → `Qwt3DLinearAutoScaler`
- 将 `Qwt3D::ClonePtr` → `Qwt3DClonePtr`
- 保留 `#include "qwt3d_scale.h"` 和 `#include "qwt3d_autoscaler.h"` 不变

### 10. 全局引用更新

在 `src/plot3d/` 下以下文件中更新引用（将旧类名替换为新类名）：

- `qwt3d_plot.h`：`Color*` → `Qwt3DColor*`，`Enrichment*` → `Qwt3DEnrichment*`
- `qwt3d_plot_p.h`：`Color* m_dataColor` → `Qwt3DColor*`，`Enrichment* m_userPlotStyle` → `Qwt3DEnrichment*`，`std::list<Enrichment*> m_enrichmentList` → `std::list<Qwt3DEnrichment*>`
- `qwt3d_plot.cpp`：`new StandardColor` → `new Qwt3DStandardColor`，`Color*` → `Qwt3DColor*`
- `qwt3d_surfaceplot.h`：`createEnrichment(Qwt3D::Enrichment&)` → `createEnrichment(Qwt3DEnrichment&)`
- `qwt3d_surfaceplot.cpp`：`Enrichment::VERTEXENRICHMENT` → `Qwt3DEnrichment::VERTEXENRICHMENT`，`static_cast<VertexEnrichment*>` → `static_cast<Qwt3DVertexEnrichment*>`
- `qwt3d_theme.cpp`：`new ColorMapColor` → `new Qwt3DColorMapColor`
- `qwt3d_axis.h`：`setScale(Scale*)` → `setScale(Qwt3DScale*)`，`ClonePtr<Scale>` → `Qwt3DClonePtr<Qwt3DScale>`
- `qwt3d_axis.cpp`：`ClonePtr<Scale>` → `Qwt3DClonePtr<Qwt3DScale>`，`new LinearScale` → `new Qwt3DLinearScale`
- `qwt3d_scale.cpp`：`Scale::PrivateData` 中的 `friend class Axis` → `friend class Qwt3DAxis`，`QWT_DECLARE_PUBLIC(Scale)` → `QWT_DECLARE_PUBLIC(Qwt3DScale)`
- 移除所有 `using namespace Qwt3D;` 和 `namespace Qwt3D { ... }` 块

**namespace 移除范围澄清：**

本步骤（Task 03）仅移除以下文件中的 `namespace Qwt3D { ... }` 块和 `using namespace Qwt3D;`：
- Steps 2-8 直接修改的文件（color/enrichment/scale/autoscaler/autoptr 等 .h/.cpp）
- Step 10 中列出的 .cpp 文件中的 `using namespace Qwt3D;`

以下文件的 `namespace Qwt3D { }` 块**保留不变**，待后续 Task 处理：
- `qwt3d_plot.h` / `qwt3d_plot_p.h` — 保留至 Task 05（Plot3D 重命名时统一移除）
- `qwt3d_surfaceplot.h` — 保留至 Task 06（SurfacePlot 重构时统一移除）
- `qwt3d_io.h` / `qwt3d_io_gl2ps.h` / `qwt3d_io_reader.h` — 保留至 Task 09（IO 系统适配时统一移除）
- `qwt3d_theme.h` / `qwt3d_theme.cpp` — 保留至 Task 09（Theme 适配时统一移除）

Step 10 中对这些文件仅更新类名引用（如 `Color*` → `Qwt3DColor*`），不移除 namespace 块。

注意：
- `examples/3D/` 目录不在本步骤范围内，将在 Plan 10 中统一更新
- `src-amalgamate/` 为自动生成文件，不可手动编辑
- 注意排除 `IO::Function`（qwt3d_io.h 中的类型别名）不被误替换

## 构建通过

```powershell
.\build.ps1 rebuild -Examples OFF -Playground OFF
```

构建必须零错误零警告通过。

## git 提交

```powershell
git add -A
git commit -m "refactor(plot3d): rename Color/Enrichment/Scale/AutoScaler/ClonePtr to Qwt3D prefix

- Color -> Qwt3DColor, StandardColor -> Qwt3DStandardColor
- ColorMapColor -> Qwt3DColorMapColor
- Enrichment -> Qwt3DEnrichment, VertexEnrichment -> Qwt3DVertexEnrichment
- CrossHair/Dot/Cone/Arrow -> Qwt3D-prefixed
- Scale -> Qwt3DScale, LinearScale -> Qwt3DLinearScale, LogScale -> Qwt3DLogScale
- AutoScaler -> Qwt3DAutoScaler, LinearAutoScaler -> Qwt3DLinearAutoScaler
- ClonePtr -> Qwt3DClonePtr
- Remove namespace Qwt3D from these files
- Clean up temporary aliases from Task 02"
```
