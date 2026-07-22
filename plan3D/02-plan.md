# 02-plan: 非绘图组件层重构（Drawable / Axis / Label / CoordinateSystem / ColorLegend）

> **前置阅读**：执行本任务前，必须完整阅读 `plan3D/ARCHITECTURE-PRINCIPLES.md`，特别是第二章「命名规范」、第四章第 4.1 节「可保留的组件」、第五章「OpenGL 技术规范」和第九章「不可触碰的约束」。

## 目标（Objective）

移除 5 个非绘图组件的 `namespace Qwt3D`，重命名为 `Qwt3D` 前缀类名。此阶段保留 legacy GL 调用不变（OpenGL 现代化在后续任务执行）。

## 涉及文件

| 文件 | 操作 |
|------|------|
| `qwt3d_drawable.h` / `.cpp` | 修改：去命名空间，重命名 |
| `qwt3d_axis.h` / `.cpp` | 修改：去命名空间，重命名 |
| `qwt3d_label.h` / `.cpp` | 修改：去命名空间，重命名 |
| `qwt3d_coordsys.h` / `.cpp` | 修改：去命名空间，重命名 |
| `qwt3d_colorlegend.h` / `.cpp` | 修改：去命名空间，重命名 |
| `qwt3d_plot.h` / `qwt3d_plot.cpp` / `qwt3d_plot_p.h` | 修改：更新对 5 个类的引用 |
| `qwt3d_io_gl2ps.h` | 修改：提供被调用的 setDeviceLineWidth() 等自由函数的命名空间处理 |

## 前提条件（Prerequisites）

- Task 01 已完成（基础类型层已重构：namespace Qwt3D 已从 types/helper/portability/openglhelper 中移除，Data→Qwt3DData 等类已重命名）
- 构建通过

## 执行步骤（Steps）

### 1. 类重命名映射

| 旧名 | 新名 | 文件 |
|------|------|------|
| `Drawable` | `Qwt3DDrawable` | `qwt3d_drawable.h/.cpp` |
| `Axis` | `Qwt3DAxis` | `qwt3d_axis.h/.cpp` |
| `Label` | `Qwt3DLabel` | `qwt3d_label.h/.cpp` |
| `CoordinateSystem` | `Qwt3DCoordinateSystem` | `qwt3d_coordsys.h/.cpp` |
| `ColorLegend` | `Qwt3DColorLegend` | `qwt3d_colorlegend.h/.cpp` |

### 2. qwt3d_drawable.h / qwt3d_drawable.cpp

- 移除 `namespace Qwt3D { ... }`
- `class Drawable` → `class QWT3D_EXPORT Qwt3DDrawable`
- PIMPL: `QWT_DECLARE_PRIVATE(Drawable)` → `QWT_DECLARE_PRIVATE(Qwt3DDrawable)`
- PrivateData 类名同步更新
- **保留** `saveGLState()` / `restoreGLState()` 和 `attach()` / `detach()` / `draw()` 接口不变
- **保留** legacy GL 调用不变（后续 OpenGL 现代化任务处理）
- 更新所有类型引用：`Triple` → `Triple`（已全局，无需改），`RGBA` → `RGBA`（已全局）

### 3. qwt3d_axis.h / qwt3d_axis.cpp

- 移除 `namespace Qwt3D { ... }`
- `class Axis : public Drawable` → `class QWT3D_EXPORT Qwt3DAxis : public Qwt3DDrawable`
- PIMPL: `QWT_DECLARE_PRIVATE(Axis)` → `QWT_DECLARE_PRIVATE(Qwt3DAxis)`
- PrivateData 类名同步更新（PrivateData 构造函数参数类型从 `Axis*` 改为 `Qwt3DAxis*`）
- **保留** `#include "qwt3d_scale.h"` 和 `#include "qwt3d_autoscaler.h"`（这两个文件在 Task 03 中重命名）
- Scale 和 AutoScaler 引用暂时保持原名（不加 Qwt3D 前缀）。Task 01 已将这些类从 namespace Qwt3D 中移出变为全局类，因此直接使用 `Scale`、`LinearScale`、`ClonePtr<Scale>` 等无限定名引用即可。同理 Step 6 中 colorlegend 对 Scale 的引用也直接使用无限定名。
- Label 引用 → `Qwt3DLabel`（本任务已重命名）
- **保留** legacy GL 调用不变（标注 TODO：Task 08 中现代化）
- **标注**：`saveGLState()` / `restoreGLState()` 临时保留，Task 08 中删除

### 4. qwt3d_label.h / qwt3d_label.cpp

- 移除 `namespace Qwt3D { ... }`
- `class Label : public Drawable` → `class QWT3D_EXPORT Qwt3DLabel : public Qwt3DDrawable`
- PIMPL: `QWT_DECLARE_PRIVATE(Label)` → `QWT_DECLARE_PRIVATE(Qwt3DLabel)`
- PrivateData 类名同步更新
- `Label` 的拷贝构造函数中显式调用 `Drawable()` 基类构造函数，需更新为 `Qwt3DDrawable()`

### 5. qwt3d_coordsys.h / qwt3d_coordsys.cpp

- 移除 `namespace Qwt3D { ... }`
- `class CoordinateSystem : public Drawable` → `class QWT3D_EXPORT Qwt3DCoordinateSystem : public Qwt3DDrawable`
- PIMPL: `QWT_DECLARE_PRIVATE(CoordinateSystem)` → `QWT_DECLARE_PRIVATE(Qwt3DCoordinateSystem)`
- `Axis` 引用 → `Qwt3DAxis`
- `ColorLegend` 引用 → `Qwt3DColorLegend`
- 公开成员 `std::vector<Axis> axes` → `std::vector<Qwt3DAxis> axes`

### 6. qwt3d_colorlegend.h / qwt3d_colorlegend.cpp

- 移除 `namespace Qwt3D { ... }`
- `class ColorLegend : public Drawable` → `class QWT3D_EXPORT Qwt3DColorLegend : public Qwt3DDrawable`
- PIMPL: `QWT_DECLARE_PRIVATE(ColorLegend)` → `QWT_DECLARE_PRIVATE(Qwt3DColorLegend)`
- `Axis` 引用 → `Qwt3DAxis`
- `Scale` 引用保持原名，直接使用无限定名引用（同步骤 3 策略）

### 7. 全局引用更新

在 `src/plot3d/` 下以下文件中更新引用（将旧类名替换为新类名，移除 using namespace Qwt3D; 和 namespace Qwt3D { ... } 块）：

需要更新引用的文件（不限于此）：
- `qwt3d_plot.h` / `qwt3d_plot.cpp` / `qwt3d_plot_p.h`：CoordinateSystem*/ColorLegend/Label/Axis 引用
- `qwt3d_surfaceplot.h` / `qwt3d_surfaceplot_p.h`：继承 Plot3D，引用 Axis 等
- `qwt3d_enrichment.h` / `qwt3d_enrichment_std.h`：引用 Plot3D
- `qwt3d_color.h`：引用 ColorLegend（注释中）
- `qwt3d_function.h` / `qwt3d_gridmapping.h` / `qwt3d_mapping.h`：在 namespace 内
- `qwt3d_io.h` / `qwt3d_io_reader.h` / `qwt3d_io_gl2ps.h`：提供被调用的自由函数
- `qwt3d_theme.h` / `qwt3d_theme.cpp`：在 namespace 内
- `qwt3d_autoscaler.h` / `qwt3d_autoptr.h`：在 namespace 内
- `qwt3d_scale.h` / `qwt3d_scale.cpp`：在 namespace 内。此外需要最小修改：仅将 `friend class Axis;` 更新为 `friend class Qwt3DAxis;`（不重命名 Scale 类本身，那属于 Task 03）。涉及 4 处修改：qwt3d_scale.h 中 3 处（Scale、LinearScale、LogScale 类内各 1 处），qwt3d_scale.cpp 中 1 处（Scale::PrivateData 内）。
- `qwt3d_graphplot.h` / `qwt3d_multiplot.h` / `qwt3d_volumeplot.h`：stub 文件
- `qwt3d_colormap_color.h`：在 namespace 内
- `qwt3d_openglhelper.h`：在 namespace 内（Task 01 已去命名空间，但需确认引用一致）

注意：
- `examples/3D/` 目录不在本步骤范围内（使用 -Examples OFF 跳过），将在 Plan 10 中统一更新
- `src-amalgamate/` 目录为自动生成文件，不可手动编辑，不在本步骤范围内
- `using namespace std;` 保留不变

CMakeLists.txt 无需修改（文件名不变，只改文件内的类名）。

## 构建通过

```powershell
.\build.ps1 rebuild -Examples OFF -Playground OFF
```

构建必须零错误零警告通过。

## git 提交

```powershell
git add -A
git commit -m "refactor(plot3d): rename Drawable/Axis/Label/CoordinateSystem/ColorLegend to Qwt3D prefix

- Drawable -> Qwt3DDrawable
- Axis -> Qwt3DAxis
- Label -> Qwt3DLabel
- CoordinateSystem -> Qwt3DCoordinateSystem
- ColorLegend -> Qwt3DColorLegend
- Remove namespace Qwt3D from these files
- Update all references across plot3d module"
```
