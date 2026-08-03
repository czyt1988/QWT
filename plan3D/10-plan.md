# 10-plan: 示例程序适配与最终清理

> **前置阅读**：执行本任务前，必须完整阅读 `plan3D/ARCHITECTURE-PRINCIPLES.md`，特别是第七章「使用示例（目标 API）」和第九章「不可触碰的约束」。

## 目标（Objective）

更新 `examples/3D/` 下的示例程序使用新 API，确保全量构建（含 Examples）通过。执行最终清理：确认无遗留的 `namespace Qwt3D`、无 legacy GL 调用、无旧类名引用。

## 涉及文件

| 文件 | 操作 |
|------|------|
| `examples/3D/simpleplot3D/` | 修改：使用 Qwt3DPlot + Qwt3DSurface + Qwt3DFunction |
| `examples/3D/mesh2/` | 修改：使用新 API |
| `examples/3D/axes/` | 修改：使用新 API |
| `examples/3D/enrichments/` | 修改：使用新 API |
| `examples/3D/autoswitch/` | 修改：使用新 API |
| `examples/3D/figureSurface3D/` | 修改：使用新 API |
| `src/plot3d/` 全部文件 | 验证：无遗留问题 |

## 前提条件（Prerequisites）

- Task 01–09 全部完成
- 构建通过（`-Examples OFF`）

## 执行步骤（Steps）

### 1. 更新 simpleplot3D 示例

**原代码模式（旧 API）：**
```cpp
#include "qwt3d_surfaceplot.h"
#include "qwt3d_function.h"

SurfacePlot* plot = new SurfacePlot();
plot->setTitle("...");
Plot3D* plot3d = plot;  // 隐式向上转型
// ... function create
```

**新代码模式：**
```cpp
#include "qwt3d_plot.h"
#include "qwt3d_surface.h"
#include "qwt3d_function.h"

Qwt3DPlot* plot = new Qwt3DPlot(parent);
auto* surface = new Qwt3DSurface();
surface->attach(plot);

MyFunction func;
func.setMesh(50, 50);
func.setDomain(-3.14, 3.14, -3.14, 3.14);
func.assign(*surface);
func.create();
```

### 2. 更新 mesh2 示例

**API 映射表：**
| 旧 API | 新 API | 说明 |
|--------|--------|------|
| `SurfacePlot* dataWidget` | `Qwt3DPlot* dataWidget` + `Qwt3DSurface* surface` | 拆分 widget 和 item |
| `dataWidget->loadFromData(...)` | `surface->loadFromData(...)` | 数据加载移到 item |
| `dataWidget->setPlotStyle(...)` | `surface->setPlotStyle(...)` | 样式移到 item |
| `dataWidget->coordinates()` | `dataWidget->coordinates()` | 仍在 Qwt3DPlot 上 |
| `IO::save(dataWidget, name, type)` | `Qwt3DIO::save(dataWidget, name, type)` | dataWidget 是 Qwt3DPlot* |
| `NativeReader` | `Qwt3DNativeReader` | 重命名 |
| `VectorWriter` | `Qwt3DVectorWriter` | 重命名 |
| `lightingdlg_->assign(dataWidget)` | `lightingdlg_->assign(dataWidget)` | dataWidget 从 SurfacePlot* 改为 Qwt3DPlot* |

**注意**：`lightingdlg` 中 `setLightComponent` 等是 `Qwt3DPlot` 的方法（光照由 plot 管理），调用对象保持为 `dataWidget`（Qwt3DPlot*）。`setPlotStyle`、`setMeshColor` 等是 item 的方法，调用对象改为 `surface`。

**lightingdlg 子系统迁移：**

`lightingdlg.h` 和 `lightingdlg.cpp` 包含 4 个需要迁移的类/结构体：

| 类/结构体 | 旧继承 | 新继承 | 迁移要点 |
|-----------|--------|--------|---------|
| `Pointer` | `Qwt3D::VertexEnrichment` | `Qwt3DVertexEnrichment` | `drawBegin()` 中的 legacy GL 调用（glGetIntegerv/glMatrixMode/glPushMatrix/glColor3d/glBegin(GL_LINES)/glVertex3d/glEnd）替换为 VBO + shader |
| `SColor` | `Qwt3D::Color` | `Qwt3DColor` | 重命名继承类 |
| `Sphere` | `ParametricSurface` | `Qwt3DParametricSurface` | `assign` 目标从 `SurfacePlot&` 改为 `Qwt3DSurface&` |
| `Plot` | `SPlot`（= `typedef Qwt3D::SurfacePlot`） | 直接使用 `Qwt3DPlot` + `Qwt3DSurface` | 组合模式重构，不能继承 item（item 不是 widget） |

- 移除 `typedef Qwt3D::SurfacePlot SPlot`（lightingdlg.h）
- `lightingdlg.cpp` 中的 6 处旧式 `SIGNAL()/SLOT()` 连接迁移为新式 `connect(sender, &Sender::signal, receiver, &Receiver::slot)` 语法
- `GL_EMISSION`/`GL_DIFFUSE`/`GL_SPECULAR` 等枚举传给 `setMaterialComponent`/`setLightComponent` 的调用保持不变（方法签名不变）

**其他 mesh2 文件迁移：**

- `mesh2mainwindow.h`：`Qwt3D::SurfacePlot *dataWidget` → `Qwt3DPlot* dataWidget` + `Qwt3DSurface* surface`；`Qwt3D::StandardColor *col_` → `Qwt3DStandardColor* col_`
- `functions.h`：9 个 Function 子类的构造函数参数 `SurfacePlot&` → `Qwt3DSurface&`
- `femreader.h`：`Qwt3D::Triple` → `Triple`、`Qwt3D::Cell` → `Cell`、`Qwt3D::TripleField` → `TripleField`、`Qwt3D::CellField` → `CellField`（去命名空间前缀）
- `designerworkaround.cpp`：1 处 `SIGNAL(triggered())/SLOT(close())` → 新式 connect 语法

### 3. 更新 axes 示例

- `SurfacePlot` → `Qwt3DPlot` + `Qwt3DSurface`
- 坐标轴自定义 API 保持不变（`coordinates()` → `Qwt3DCoordinateSystem`）
- **自定义 Scale 子类迁移**：
  - `Letter`、`Imaginary`、`TimeItems` 继承 `LinearScale` → 改为继承 `Qwt3DLinearScale`
  - `setScale(new Letter)` → `setScale(new Letter)`（调用方式不变，类名已更新）
  - `clone()` 返回类型更新为 `Qwt3DScale*`

### 4. 更新 enrichments 示例

- `SurfacePlot` → `Qwt3DPlot` + `Qwt3DSurface`
- `plot->addEnrichment()` → `surface->addEnrichment()`
- `CrossHair` → `Qwt3DCrossHair`, `Dot` → `Qwt3DDot`, `Cone` → `Qwt3DCone`, `Arrow` → `Qwt3DArrow`
- **自定义 `Bar` 类迁移**：`Bar` 继承 `Qwt3D::VertexEnrichment`，`draw()` 中有 `glBegin(GL_POLYGON)` 等 legacy GL 调用
  - 改为继承 `Qwt3DVertexEnrichment`
  - `draw()` 中的 legacy GL 调用替换为 VBO + polygon shader
  - `clone()` 返回类型更新
- **`Label3D` 类**（`enrichments.h`）：`draw()` 中有大量 legacy GL 调用（`glColor3d`、`glBegin(GL_QUADS)`、`glBegin(GL_LINE_LOOP)`、`glBegin(GL_LINES)`、`glVertex3d`、`glEnd`），需替换为 VBO + polygon/line shader。注意：`Label3D` 不是 Enrichment 子类，是独立的 Drawable 子类，但同样需要 legacy GL 迁移。
- `enrichmentmainwindow.cpp` 中的 `glBlendFunc` 和 `glDisable(GL_LINE_SMOOTH)` 调用需移除（Core Profile 下行为不同）

### 5. 更新 autoswitch 示例

- **多 plot 模式**：当前在 QSplitter 中创建两个 SurfacePlot 实例
- 改为创建两个 `Qwt3DPlot` 实例，各自 attach 一个 `Qwt3DSurface`
- `Saddle` 和 `Hat` 函数的 `assign` 目标从 `SurfacePlot&` 改为 `Qwt3DSurface&`
- `plot1->makeCurrent()` 调用：`Qwt3DPlot` 仍有此方法（继承自 QOpenGLWidget），保持不变

### 6. 更新 figureSurface3D 示例

**架构重构**：`DynamicSurfacePlot` 当前继承 `SurfacePlot`（widget），被 `QwtFigure::addWidget()` 嵌入。新架构下 `Qwt3DSurface` 是 item（非 widget），不能直接嵌入。

需要重构为组合模式：
- `DynamicSurfacePlot` 类拆分为 `Qwt3DPlot* m_plot` + `Qwt3DSurface* m_surface`
- `QwtFigure::addWidget()` 接收的是 `Qwt3DPlot*`（widget），而非 `Qwt3DSurface*`（item）
- `crossSectionY0()` 和 `zAtOrigin()` 需要访问 surface item 的数据
- `advanceTime()` 中 `m_ripple.create(*this)` 改为 `m_ripple.create(*m_surface)`
- `resetView()` 中 `setRotation()` 等是 `Qwt3DPlot` 的方法，调用对象需更新
- 信号 `rotationChanged`/`zoomChanged` 来自 `Qwt3DPlot` 而非 surface item

**新代码骨架：**
```cpp
class DynamicSurfacePlot : public QWidget {
    Q_OBJECT
public:
    DynamicSurfacePlot(QWidget* parent = nullptr) : QWidget(parent) {
        m_plot = new Qwt3DPlot(this);
        m_surface = new Qwt3DSurface();
        m_surface->attach(m_plot);
        // ... layout, signal connections ...
    }
    void resetView() { m_plot->setRotation(...); }
    // ... cross-section data access via m_surface ...
private:
    Qwt3DPlot* m_plot;
    Qwt3DSurface* m_surface;
};
```

### 通用：namespace 和枚举引用清理（适用于所有示例）

移除每个示例文件中的 `using namespace Qwt3D;` 和 `typedef Qwt3D::SurfacePlot SurfacePlot` 等。
更新 `Qwt3D::` 前缀的枚举引用：
- `Qwt3D::BOX` → `BOX`（全局枚举）
- `Qwt3D::FILLED` → `FILLED`
- `Qwt3D::RGBA` → `RGBA`
- `Qwt3D::X1` → `X1`（或对应的新枚举名）

**通用：旧式 SIGNAL()/SLOT() 连接迁移**

所有示例中的旧式 `SIGNAL()/SLOT()` 连接需迁移为新式 `connect(sender, &Sender::signal, receiver, &Receiver::slot)` 语法。涉及文件（不限于此）：
- `mesh2/mesh2mainwindow.cpp` — 约 40+ 处
- `mesh2/lightingdlg.cpp` — 7 处（注意：上一处描述为 6 处，实际为 7 处，已修正）
- `axes/axesmainwindow.cpp` — 约 5 处
- `autoswitch/autoswitch.cpp` — 约 1 处
- `mesh2/designerworkaround.cpp` — 1 处

### 6.5. 逐示例编译验证

每个示例更新后，先单独编译该示例，再进行全量构建：
```powershell
.\build.ps1 build -Examples ON -Playground OFF
```
如果某个示例编译失败，定位到具体文件修复后再继续。

### 7. 最终清理验证

在 `src/plot3d/` 下执行以下检查：

**检查 1：无遗留命名空间**
```
grep -r "namespace Qwt3D" src/plot3d/
```
预期结果：零匹配

**检查 2：无遗留旧类名**
```
grep -rn "\bPlot3D\b" src/plot3d/
grep -rn "\bSurfacePlot\b" src/plot3d/
grep -rn "\bDrawable\b" src/plot3d/
grep -rn "\bAxis\b" src/plot3d/
grep -rn "\bColor\b" src/plot3d/
```
预期结果：零匹配（注释中的除外）

**检查 3：无遗留 legacy GL 调用**
```
grep -rn "glBegin\|glEnd\|glNewList\|glCallList\|glRotatef\|glTranslatef\|glScalef\|glPushMatrix\|glPopMatrix\|glOrtho\|glFrustum\|glLightfv\|glMaterialfv\|glColorMaterial\|glShadeModel\|glLoadIdentity\|glLoadMatrixf\|glMultMatrixf\|glGetDoublev\|glGetIntegerv\|glIsEnabled\|glIsList\|gluProject\|gluUnProject\|gluErrorString\|GL_LINE_SMOOTH\|GL_POINT_SMOOTH\|GL_LIGHTING\|glBlendFunc" src/plot3d/
```
预期结果：零匹配（gl2ps 条件编译块内除外）

**检查 4：无遗留 `using namespace Qwt3D`**
```
grep -r "using namespace Qwt3D" src/plot3d/
```
预期结果：零匹配

**检查 5：无遗留 `#include "qwt3d_openglhelper.h"`**
```
grep -r "qwt3d_openglhelper" src/plot3d/
```
预期结果：零匹配

**检查 6：qwt3d_openglhelper.h 已删除**
```
test ! -f src/plot3d/qwt3d_openglhelper.h && echo "PASS: file deleted" || echo "FAIL: file still exists"
```
预期结果：PASS

### 8. 更新 CMakeLists.txt

- 确认 examples 的 CMakeLists.txt 正确引用新的源文件
- 确认 `QWT_CONFIG_BUILD_EXAMPLE` 条件编译正常

## 构建通过

```powershell
.\build.ps1 rebuild
```

**全量构建**（含 Examples）必须通过。此命令等同于：
```powershell
.\build.ps1 rebuild -Examples ON -Playground ON
```

已验证 playground 目录无 plot3d 引用，无需更新。

## git 提交

```powershell
git add -A
git commit -m "refactor(plot3d): update examples to new Plot+Item API and final cleanup

- Update all 3D examples (simpleplot3D, mesh2, axes, enrichments, etc.)
  to use Qwt3DPlot + Qwt3DSurface + Qwt3DFunction new API
- Verify no remaining namespace Qwt3D
- Verify no remaining legacy class names
- Verify no remaining legacy OpenGL calls (except gl2ps fallback)
- Full build with examples passes"
```
