# 01-plan: 基础类型层重构

> **前置阅读**：执行本任务前，必须完整阅读 `plan3D/ARCHITECTURE-PRINCIPLES.md`，特别是第二章「命名规范」、第四章「保留与重构清单」、第五章「OpenGL 技术规范」和第九章「不可触碰的约束」。

## 目标（Objective）

移除 3D 模块基础类型文件的 `namespace Qwt3D`，重命名类和类型为 `Qwt3D` 前缀，删除 legacy OpenGL 辅助函数，建立新架构的基础类型层。

## 涉及文件

| 文件 | 操作 |
|------|------|
| `qwt3d_global.h` | 修改：移除 MSVC 警告抑制宏中不必要的部分（保留导出宏） |
| `qwt3d_types.h` / `.cpp` | 修改：去命名空间，重命名类 |
| `qwt3d_helper.h` | 修改：去命名空间 |
| `qwt3d_portability.h` | 修改：去命名空间，重命名类 |
| `qwt3d_openglhelper.h` | 修改：去命名空间（legacy GL 函数保留，推迟至 Plan 08） |
| `qwt3d_autoptr.h` | 修改：去命名空间 |
| `CMakeLists.txt` | 修改：更新文件列表（文件名不重命名，仅更新类名引用） |

## 前提条件（Prerequisites）

- 当前在 `new3d` 分支，HEAD 为 `51f5e2a`
- 构建环境可用（CMake + Qt + MSVC）

## 执行步骤（Steps）

### 1. qwt3d_global.h

- 保持 `QWT3D_EXPORT` 宏定义不变
- 保持 `#include <qglobal.h>` 和 `#include "qwt_global.h"`
- 无命名空间，无需修改

### 2. qwt3d_types.h / qwt3d_types.cpp

**去命名空间：** 移除 `namespace Qwt3D { ... }` 包裹。

**类重命名：**

| 旧名 | 新名 |
|------|------|
| `Data` | `Qwt3DData` |
| `GridData` | `Qwt3DGridData` |
| `CellData` | `Qwt3DCellData` |

**保留原名的基础类型（仅去命名空间，不重命名）：**
- `Tuple`、`Triple`、`ParallelEpiped`、`FreeVector`、`RGBA`（简单数据结构，全局可见）
- `TripleField`、`CellField`、`ColorVector`、`FreeVectorField`、`Cell`、`Vertex`、`DataRow`、`DataMatrix`（类型别名）

**保留原名的枚举（仅去命名空间）：**
- `PLOTSTYLE`、`SHADINGSTYLE`、`COORDSTYLE`、`SCALETYPE`、`FLOORSTYLE`、`DATATYPE`
- `AXIS`、`SIDE`、`ANCHOR`

**保留原名的自由函数：**
- `GL2Qt()`、`Qt2GL()`、`tesselationSize()`、`normalizedcross()`、`dotProduct()`、`convexhull2d()`

**PIMPL 调整：**
- `Qwt3DData` 的 `QWT_DECLARE_PRIVATE(Data)` → `QWT_DECLARE_PRIVATE(Qwt3DData)`
- `Qwt3DGridData` 同理
- `Qwt3DCellData` 无 PIMPL，仅需重命名

**include 调整：** 移除 `#include <windows.h>`。

关于 `#include "qwt3d_openglhelper.h"` 的处理：**保留** `#include "qwt3d_openglhelper.h"`（仅去命名空间）。GL 类型替换（GLdouble→double 等）和 include 移除推迟到 Plan 08（OpenGL 现代化），届时所有 legacy GL 调用都会被移除，传递性 GL 类型依赖自然消失。

**PI 常量命名冲突：** `namespace Qwt3D` 中定义的 `const double PI` 去命名空间后变为全局 `PI`，可能与系统头文件冲突。重命名为 `Qwt3D_PI`。

**Doxygen 保护：** 保留 `#ifndef QWT3D_NOT_FOR_DOXYGEN` 保护块不变。

### 3. qwt3d_helper.h

- 移除 `namespace Qwt3D { ... }`
- `isPracticallyZero()` 函数变为全局

### 4. qwt3d_portability.h

- 移除 `namespace Qwt3D { ... }`
- `MouseState` → `Qwt3DMouseState`
- `KeyboardState` → `Qwt3DKeyboardState`
- 宏 `QWT3DLOCAL8BIT` 和 `SingleLine` 保持不变

### 5. qwt3d_openglhelper.h

**仅移除 `namespace Qwt3D` 包裹，保留所有函数不变。**

该文件包含以下 legacy GL 辅助函数，被约 10 个文件、50+ 处调用：

- `class GLStateBewarer`（固定管线状态管理）
- `gl_error()`（使用 gluErrorString）
- `SaveGlDeleteLists()`（display list 管理）
- `getMatrices()`（glGetDoublev 矩阵查询）
- `ViewPort2World()`（gluUnProject）
- `World2ViewPort()`（gluProject）

**不删除该文件。** 这些 legacy GL 函数将在 Plan 08（OpenGL 现代化）中随各调用方一起重构并最终删除。在本步骤中，仅对文件去命名空间，使其内容在全局作用域可见。

### 6. CMakeLists.txt 更新

- 文件列表不变（`qwt3d_openglhelper.h` 保留在 header 列表中，`qwt3d_autoptr.h` 确保已在列表中）
- 其他文件名不变（文件名不重命名，只改文件内的类名）

### 7. 全局引用更新

在 `src/plot3d/` 下所有 `.h` 和 `.cpp` 文件中：
- 保留 `#include "qwt3d_openglhelper.h"`（文件已去命名空间但内容不变）
- 将 `Qwt3D::Data` → `Qwt3DData`，`Qwt3D::GridData` → `Qwt3DGridData`，`Qwt3D::CellData` → `Qwt3DCellData`
- 将 `Qwt3D::Triple` → `Triple`，`Qwt3D::RGBA` → `RGBA` 等所有类型去命名空间前缀
- 将 `Qwt3D::MouseState` → `Qwt3DMouseState`，`Qwt3D::KeyboardState` → `Qwt3DKeyboardState`
- 所有 `Qwt3D::PLOTSTYLE` 等枚举去命名空间前缀
- 移除所有 `using namespace Qwt3D;`
- 移除所有 `namespace Qwt3D { ... }` 块

**涉及文件清单：**

- 所有 `.h` 文件（约 20 个）：移除 `namespace Qwt3D` 块、更新 `Qwt3D::` 前缀引用
- 所有 `.cpp` 文件（约 26 个）：移除 `using namespace Qwt3D`、更新引用
- `examples/3D/` 目录不在本步骤范围内（使用 `-Examples OFF` 跳过），将在 Plan 10 中统一更新
- `src-amalgamate/` 目录为自动生成文件，不可手动编辑，不在本步骤范围内

**注意**：此步骤只做机械性的查找替换，不修改逻辑。

## 构建通过

```powershell
.\build.ps1 rebuild -Examples OFF -Playground OFF
```

构建必须零错误零警告通过。常见编译问题：
- 遗漏的 `Qwt3D::` 前缀引用
- PIMPL 宏的类名不匹配

## git 提交

```powershell
git add -A
git commit -m "refactor(plot3d): remove namespace Qwt3D, rename base types to Qwt3D prefix, de-namespace GL helpers

- Remove namespace Qwt3D from all plot3d files
- Rename Data -> Qwt3DData, GridData -> Qwt3DGridData, CellData -> Qwt3DCellData
- Rename MouseState -> Qwt3DMouseState, KeyboardState -> Qwt3DKeyboardState
- Keep basic types (Triple, RGBA, Tuple, etc.) in global scope
- De-namespace qwt3d_openglhelper.h (legacy GL cleanup deferred to Plan 08)
- Update all references across plot3d module"
```
