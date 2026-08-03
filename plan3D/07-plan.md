# 07-plan: 数据生成器适配（Function / ParametricSurface）

> **前置阅读**：执行本任务前，必须完整阅读 `plan3D/ARCHITECTURE-PRINCIPLES.md`，特别是第二章「命名规范」、第四章第 4.1 节和第九章「不可触碰的约束」。

## 目标（Objective）

将 `Function` 和 `ParametricSurface` 数据生成器的 target 从 `SurfacePlot*`（widget）改为 `Qwt3DSurface*`（item），去命名空间并重命名。

## 涉及文件

| 文件 | 操作 |
|------|------|
| `qwt3d_mapping.h` | 修改：去命名空间，重命名 |
| `qwt3d_gridmapping.h` / `.cpp` | 修改：去命名空间，重命名，target 改为 item |
| `qwt3d_function.h` / `.cpp` | 修改：去命名空间，重命名，target 改为 item |
| `qwt3d_parametricsurface.h` / `.cpp` | 修改：去命名空间，重命名，target 改为 item |
| `CMakeLists.txt` | 无需修改（文件名不变，只改文件内的类名） |

## 前提条件（Prerequisites）

- Task 01–06 已完成
- `Qwt3DSurface` item 已创建，`loadFromData()` 接口可用
- 构建通过

## 执行步骤（Steps）

### 1. 类重命名映射

| 旧名 | 新名 | 文件 |
|------|------|------|
| `Mapping` | `Qwt3DMapping` | `qwt3d_mapping.h` |
| `GridMapping` | `Qwt3DGridMapping` | `qwt3d_gridmapping.h/.cpp` |
| `Function` | `Qwt3DFunction` | `qwt3d_function.h/.cpp` |
| `ParametricSurface` | `Qwt3DParametricSurface` | `qwt3d_parametricsurface.h/.cpp` |

### 2. qwt3d_mapping.h

- 移除 `namespace Qwt3D { ... }`
- `class Mapping` → `class QWT3D_EXPORT Qwt3DMapping`

### 3. qwt3d_gridmapping.h / qwt3d_gridmapping.cpp

- 移除 `namespace Qwt3D { ... }`
- `class GridMapping : public Mapping` → `class QWT3D_EXPORT Qwt3DGridMapping : public Qwt3DMapping`
- PIMPL 类名同步更新
- **关键变更**：`SurfacePlot* plotWidget()` → `Qwt3DSurface* surface()`（方法名也重命名）
- **关键变更**：`setPlotWidget(SurfacePlot*)` → `setSurface(Qwt3DSurface*)`
- 前向声明：`class Qwt3DSurface;` 替代 `class SurfacePlot;`
- **include 更新**：`qwt3d_gridmapping.cpp` 中 `#include "qwt3d_surfaceplot.h"` 更新为 `#include "qwt3d_surface.h"`（Task 06 已重命名该文件）。注意：gridmapping.cpp 仅存储/返回指针，如果可以用前向声明替代则更好，但 create() 方法中需要调用 Qwt3DSurface 的方法，因此保留 include。

### 4. qwt3d_function.h / qwt3d_function.cpp

- 移除 `namespace Qwt3D { ... }`
- `class Function : public GridMapping` → `class QWT3D_EXPORT Qwt3DFunction : public Qwt3DGridMapping`
- **关键变更**：构造函数参数
  - `Function(SurfacePlot&)` → `Qwt3DFunction(Qwt3DSurface&)`
  - `Function(SurfacePlot*)` → `Qwt3DFunction(Qwt3DSurface*)`
- **关键变更**：`assign()` 方法
  - `assign(SurfacePlot&)` → `assign(Qwt3DSurface&)`
  - `assign(SurfacePlot*)` → `assign(Qwt3DSurface*)`
- **关键变更**：`create()` 内部调用
  - 原：`static_cast<SurfacePlot*>(plotWidget())->loadFromData(data, um, vm, minU(), maxU(), minV(), maxV())`
  - 新：`surface()->loadFromData(data, um, vm, minU(), maxU(), minV(), maxV())`（移除 static_cast，surface() 已返回 Qwt3DSurface*）
- `create(SurfacePlot&)` → `create(Qwt3DSurface&)`

**include 顺序修正：**
`qwt3d_function.cpp` 当前 `#include "qwt3d_surfaceplot.h"` 在 `#include "qwt3d_function.h"` 之前，违反 AGENTS.md "本文件对应头文件优先"原则。修正为：
```cpp
#include "qwt3d_function.h"  // 本文件对应头文件优先
// 空行
#include "qwt3d_surface.h"   // 替代 qwt3d_surfaceplot.h
```

### 5. qwt3d_parametricsurface.h / qwt3d_parametricsurface.cpp

- 移除 `namespace Qwt3D { ... }`
- `class ParametricSurface : public GridMapping` → `class QWT3D_EXPORT Qwt3DParametricSurface : public Qwt3DGridMapping`
- PIMPL 类名同步更新
- 构造函数和 `assign()` 同 Function 的变更模式（`assign(SurfacePlot&)` → `assign(Qwt3DSurface&)`）
- **关键变更**：`create()` 内部调用
  - 原：`static_cast<SurfacePlot*>(plotWidget())->loadFromData(data, um, vm, uperiodic, vperiodic)`
  - 新：`surface()->loadFromData(data, um, vm, uperiodic, vperiodic)`（移除 static_cast，surface() 已返回 Qwt3DSurface*）
- **include 更新**：`qwt3d_parametricsurface.cpp` 中 `#include "qwt3d_surfaceplot.h"` 更新为 `#include "qwt3d_surface.h"`。此文件的 create() 方法调用 `loadFromData()`，必须包含完整定义。

**修复裁剪逻辑 bug：**
`ParametricSurface::create()` 中的裁剪逻辑使用 `else if` 链，导致如果 x 超出范围则 y 和 z 不会被检查。修复为独立 `if` 语句：
```cpp
// Before (buggy):
if (data[i][j].x > range().maxVertex.x)
    data[i][j].x = range().maxVertex.x;
else if (data[i][j].y > range().maxVertex.y)
    data[i][j].y = range().maxVertex.y;
else if (...)

// After (fixed):
if (data[i][j].x > range().maxVertex.x) data[i][j].x = range().maxVertex.x;
if (data[i][j].y > range().maxVertex.y) data[i][j].y = range().maxVertex.y;
if (data[i][j].z > range().maxVertex.z) data[i][j].z = range().maxVertex.z;
if (data[i][j].x < range().minVertex.x) data[i][j].x = range().minVertex.x;
if (data[i][j].y < range().minVertex.y) data[i][j].y = range().minVertex.y;
if (data[i][j].z < range().minVertex.z) data[i][j].z = range().minVertex.z;
```

### 6. 更新全局引用

在 `src/plot3d/` 下以下文件中更新引用：

**精确替换规则（按上下文匹配，非全局搜索替换）：**
- `: public Function` → `: public Qwt3DFunction`（类继承）
- `Function::` → `Qwt3DFunction::`（类作用域）
- `Function(` → `Qwt3DFunction(`（构造函数调用，注意排除注释中的普通单词 "function"）
- `Function&` / `Function*` → `Qwt3DFunction&` / `Qwt3DFunction*`（类型引用）
- 同理处理 ParametricSurface、GridMapping、Mapping

**注意排除：**
- `IO::Function`（qwt3d_io.h 中的类型别名）不要替换
- 注释中的普通英文单词 "function" 不要替换

**涉及文件：**
- `qwt3d_mapping.h`：Mapping → Qwt3DMapping
- `qwt3d_gridmapping.h/.cpp`：GridMapping → Qwt3DGridMapping, SurfacePlot → Qwt3DSurface
- `qwt3d_function.h/.cpp`：Function → Qwt3DFunction, SurfacePlot → Qwt3DSurface
- `qwt3d_parametricsurface.h/.cpp`：ParametricSurface → Qwt3DParametricSurface, SurfacePlot → Qwt3DSurface

- `Qwt3D::ParallelEpiped` → `ParallelEpiped`（gridmapping.h 声明 + gridmapping.cpp 实现，共约 6 处）
- `Qwt3D::Triple` → `Triple`（parametricsurface.h 的 operator() 返回类型等，约 1 处）
- `Qwt3D::SurfacePlot` → `Qwt3DSurface`（约 10 处，会被 SurfacePlot→Qwt3DSurface 替换间接覆盖，但需确认）
- 其他 `Qwt3D::` 前缀引用 → 去掉 `Qwt3D::` 前缀（Task 01 已将这些类型全局化）

注意：执行前先 grep 确认 4 个目标文件中所有 `Qwt3D::` 引用，确保全部剥离。

移除所有 `using namespace Qwt3D;` 和 `namespace Qwt3D { ... }` 块。

**注意**：`examples/3D/` 目录不在本步骤范围内，将在 Plan 10 中统一更新。

CMakeLists.txt 无需修改（文件名不变，只改文件内的类名）。

## 验证步骤

构建通过后，编写一个简单的 Qwt3DFunction 子类，attach 到 Qwt3DSurface，调用 create()，确认数据加载正常。

## 构建通过

```powershell
.\build.ps1 rebuild -Examples OFF -Playground OFF
```

构建必须通过。

## git 提交

```powershell
git add -A
git commit -m "refactor(plot3d): adapt Function/ParametricSurface to target Qwt3DSurface item

- Mapping -> Qwt3DMapping, GridMapping -> Qwt3DGridMapping
- Function -> Qwt3DFunction, ParametricSurface -> Qwt3DParametricSurface
- Change target from SurfacePlot* (widget) to Qwt3DSurface* (item)
- Remove namespace Qwt3D from mapping files"
```
