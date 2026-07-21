# Qwt 3D 模块完整度补全计划 — 总纲

> **本文件是所有子任务计划的入口和全局规范索引。**
> 任何执行者在开始单个 `plan*.md` 任务前，必须先完整阅读本文件。

---

## 1. 项目背景

Qwt 7.x（v7.3.0）是基于 Qt 的高性能 2D/3D 绘图库。项目由三个 shared library 组成：

| 目标 | CMake target | 输出名 | 导出宏 | 源码目录 |
|------|-------------|--------|--------|---------|
| 公共基础 | `qwt::core` | `qwtcore.dll` | `QWTCORE_EXPORT` | `src/core/` |
| 2D 绘图 | `qwt::plot` | `qwtplot.dll` | `QWT_EXPORT` | `src/plot/` |
| 3D 绘图 | `qwt::plot3d` | `qwtplot3d.dll` | `QWT3D_EXPORT` | `src/plot3d/` |

依赖关系：`plot` 和 `plot3d` 都依赖 `core`，彼此互不依赖。

### 3D 模块现状（审阅结论）

**已实现且可用：**

| 组件 | 实现文件 | 说明 |
|------|---------|------|
| SurfacePlot | `qwt3d_surfaceplot.h/.cpp` + `qwt3d_gridplot.cpp` + `qwt3d_meshplot.cpp` | 表面图，支持 GridData + CellData，5种绘制样式 |
| Function | `qwt3d_function.h/.cpp` | 函数曲面 z=f(x,y) |
| ParametricSurface | `qwt3d_parametricsurface.h/.cpp` | 参数曲面 (x,y,z)=f(u,v)，支持周期性 |
| Plot3D 基类 | `qwt3d_plot.h/.cpp` + `qwt3d_plot_p.h` | 抽象基类，坐标系/变换/交互/光照/IO |
| CoordinateSystem | `qwt3d_coordsys.h/.cpp` | 12轴坐标系，BOX/FRAME样式 |
| Color | `qwt3d_color.h/.cpp` + `qwt3d_colormap_color.h/.cpp` | 颜色functor，桥接core模块QwtColorMap |
| Enrichment | `qwt3d_enrichment.h` + `qwt3d_enrichment_std.h/.cpp` | 4种VertexEnrichment: CrossHair/Dot/Cone/Arrow |
| Theme | `qwt3d_theme.h/.cpp` | 10种主题预设 + 5种光照预设 |
| IO | `qwt3d_io.h/.cpp` + `qwt3d_io_reader.h/.cpp` + `qwt3d_io_gl2ps.h/.cpp` | 位图+矢量导出(EPS/PS/PDF/SVG/PGF) |
| Label | `qwt3d_label.h/.cpp` | 3D文本标签，9种锚点 |
| Lighting | `qwt3d_lighting.cpp` | 8盏灯，固定管线光照 |

**仅有头文件声明但未实现（占位/桩）：**

| 类名 | 头文件 | 状态 |
|------|--------|------|
| GraphPlot | `qwt3d_graphplot.h` | 构造函数声明但无.cpp定义，实例化会导致链接错误 |
| VolumePlot | `qwt3d_volumeplot.h` | 注释标注"TODO: not yet fully implemented" |
| MultiPlot | `qwt3d_multiplot.h` | 空构造函数体，纯虚createData() |

**完全缺失的绘图类型：** 3D散点图、3D柱状图、3D线图/参数曲线、3D向量场、3D流线、3D等高线/切片、3D等值面、体积渲染、散乱点插值曲面。

### 渲染架构现状

- **完全使用 OpenGL 固定管线**：immediate mode（glBegin/glEnd）+ display lists
- 无 VBO/VAO/着色器等现代 GPU 编程技术
- 不支持 OpenGL Core Profile（核心上下文下固定管线 API 被移除）
- 无 LOD（细节层次）、无视锥剔除、无多线程

### 已发现的 Bug

1. `blowout()` 无法关灯（`qwt3d_lighting.cpp:85`，`unlit = false` 应为 `true`）
2. `setLightComponent(intensity, light)` 双重 lightEnum 转换（`qwt3d_lighting.cpp:139`，对 light 1-7 错误操作 GL_LIGHT0）
3. 鼠标/滚轮事件未使用 Qt5/Qt6 兼容层（`qwt3d_mousekeyboard.cpp`，应使用 `qwt::compat::eventPos()` / `qwt::compat::wheelEventDelta()`）

---

## 2. 通用架构与规范 (Spec)

### 2.1 三库结构

所有新代码必须放在 `src/plot3d/` 目录下，使用 `QWT3D_EXPORT` 导出宏。CMake 配置在 `src/plot3d/CMakeLists.txt` 中。新源文件需添加到 `QWTPLOT3D_SOURCE_PLOT_3D` 列表，新头文件需添加到 `QWTPLOT3D_HEADER_PLOT_3D` 列表。

### 2.2 PIMPL 模式（强制要求）

**所有新建的类必须使用 PIMPL 模式**，使用 `qwt_global.h` 定义的自定义宏（非 Qt 的 `Q_DECLARE_PRIVATE`）。参考 `src/plot/` 下 2D 模块代码（如 `qwt_plot_curve.h/.cpp`、`qwt_plot_p.h`）的实现模式。

#### 完整规范

```cpp
// === 头文件 (qwt3d_foo.h) ===
namespace Qwt3D {

class QWT3D_EXPORT Foo : public Plot3D
{
    Q_OBJECT

public:
    explicit Foo(QWidget* parent = nullptr);
    ~Foo() override;

    // ... public API ...

protected:
    QWT_DECLARE_PRIVATE(Foo)  // PIMPL 声明
    // ... protected API ...
};

}  // namespace Qwt3D

// === 私有头文件 (qwt3d_foo_p.h) ===
// 注意：私有头文件不加入安装列表（QWTPLOT3D_HEADER_PLOT_3D），
//       仅加入 CMake 源文件列表以参与编译
#include "qwt3d_foo.h"

class Foo::PrivateData
{
    QWT_DECLARE_PUBLIC(Foo)

public:
    PrivateData(Foo* q);

    // 所有成员变量、VBO/着色器对象、缓存等放在此处
    int m_someValue = 0;
    std::unique_ptr<GLBuffer> m_vertexBuffer;
    // ...
};

// === 源文件 (qwt3d_foo.cpp) ===
#include "qwt3d_foo_p.h"

Foo::PrivateData::PrivateData(Foo* q)
{
    QWT_INIT_PUBLIC(q)  // 设置 q_ptr
}

Foo::Foo(QWidget* parent)
    : Plot3D(parent)
    , QWT_PIMPL_CONSTRUCT  // 自动 m_data = qwt_make_unique<PrivateData>(this)
{
}

void Foo::someMethod()
{
    QWT_D(d);          // 非 const：PrivateData* d = d_func()
    d->m_someValue = 42;
}

int Foo::someValue() const
{
    QWT_DC(d);         // const：const PrivateData* d = d_func()
    return d->m_someValue;
}
```

#### 关键规则

| 规则 | 说明 |
|------|------|
| **所有新类必须 PIMPL** | 公开类的所有非静态成员变量必须放在 `PrivateData` 中 |
| **私有头文件命名** | `qwt3d_xxx_p.h`，与 `qwt3d_xxx.h/.cpp` 同目录 |
| **私有头不入安装列表** | 不加入 `QWTPLOT3D_HEADER_PLOT_3D`，仅加入 CMake 编译 |
| **非 const 方法用 `QWT_D(d)`** | `PrivateData* d = d_func()` |
| **const 方法用 `QWT_DC(d)`** | `const PrivateData* d = d_func()` |
| **构造函数用 `QWT_PIMPL_CONSTRUCT`** | 自动 `m_data = qwt_make_unique<PrivateData>(this)` |
| **PrivateData 构造函数设置 q_ptr** | 通过 `QWT_DECLARE_PUBLIC` 宏中的 `q_ptr` 成员 |

#### PIMPL 豁免规则

以下类型**不需要**使用 PIMPL：

| 类型 | 原因 | 示例 |
|------|------|------|
| 纯数据结构体（POD-like） | 无逻辑，仅数据聚合 | `TriangleMesh`、`SlicePlane`、`SubPlot` |
| 仅含静态方法的工具类 | 无实例状态 | `TubeBuilder`（全部 static 方法） |
| 枚举/别名定义 | 非类 | `enum class PrimitiveType` |
| 着色器源码常量 | 编译期字符串常量 | `qwt3d_shaders.h` 中的 `kSurfaceVertexShaderCore` 等 |

**判断准则**：如果一个类有非静态成员变量且需要 ABI 稳定性（导出到 DLL），则必须使用 PIMPL。纯数据聚合体或无状态工具类可豁免。

### 2.3 Qt5/Qt6 兼容层

涉及鼠标/滚轮/字体度量差异时**必须使用** `qwt::compat::` 命名空间下的函数：

| 兼容函数 | 用途 |
|---------|------|
| `qwt::compat::eventPos(event)` | 鼠标位置（替代 `event->pos()` / `event->position().toPoint()`） |
| `qwt::compat::wheelEventDelta(event)` | 滚轮增量（替代 `event->delta()` / `event->angleDelta().y()`） |
| `qwt::compat::horizontalAdvance(fm, str)` | 文本宽度（替代 `fm.width()` / `fm.horizontalAdvance()`） |

### 2.4 命名规范

| 对象 | 规范 | 示例 |
|------|------|------|
| 类名 | `Qwt3D` 前缀大驼峰 | `Qwt3DScatterPlot` |
| 方法 | 小驼峰，getter 不加 `get`，setter 加 `set` | `setDataColor()` |
| 成员变量 | `m_` 前缀 | `m_data` |
| 局部变量 | 小驼峰 | `pointSize` |
| 常量 | 全大写下划线 | `MAX_POINTS` |
| 命名空间 | `Qwt3D` | `namespace Qwt3D { ... }` |

### 2.5 Include 顺序

```cpp
#include "own_header.h"   // 本文件对应头文件优先
                         // 空行
#include <qnamespace.h>   // Qt 系统头文件 <q...>
#include <QWidget>        // Qt 类头文件 <Q...>
                         // 空行
#include <algorithm>      // STL
                         // 空行
#include "qwt3d_plot.h"   // 本项目其他头文件
```

`.clang-format` 配置中 `SortIncludes: false` — include 顺序手动控制，不可自动排序。

### 2.6 注释规范（Doxygen 纯英文）

**所有源码注释一律使用英文，禁止中文。** Doxygen 关键字统一用 `@` 前缀。

| 注释类型 | 位置 | 要求 |
|---------|------|------|
| 类注释 | `.h` | 英文 Doxygen，含使用示例 |
| public 函数详细 | `.cpp` | 英文 Doxygen（`@brief`/`@param`/`@return`/`@details`） |
| public 函数简要 | `.h` | 单行英文 `// Comment` |
| 信号注释 | `.h` | 英文 Doxygen |
| private/protected 函数 | `.cpp` | 可选，建议英文 |

参数方向标注：`@param[out]`、`@param[in,out]` 仅在非 const 引用/指针时必须标注。

### 2.7 现代 C++ 要求

- 使用 `override`/`final`（不用旧宏 `QWT_OVERRIDE`/`QWT_FINAL`）
- 使用 `nullptr`（不用 `NULL`）
- 使用 `static_cast<>` / `using`（不用 C 风格转换 / `typedef`）
- Qt 容器迭代使用 `qwt_as_const(container)` 防止 COW 深拷贝
- 智能指针使用 `qwt_make_unique<T>(args...)`（C++11 兼容）

### 2.8 信号槽

- 使用 `Q_SIGNALS:` / `public Q_SLOTS:` 宏（不用 `signals:` / `public slots:`）
- 优先新式 `connect(sender, &Sender::signal, receiver, &Receiver::slot)` 语法

### 2.9 渲染架构策略 — 混合渐进式

本计划采用**混合渐进式**渲染架构：

1. **新绘图类型**使用现代 OpenGL（VBO/VAO/着色器），通过 `plan00` 提供的基础设施实现
2. **保持与旧代码兼容**：新基础设施自动检测 OpenGL Core Profile 可用性，在兼容上下文下回退到固定管线
3. **后续逐步迁移**旧代码（SurfacePlot 等）到可编程管线，但不在本次计划范围内
4. 新绘图的着色器代码以内嵌字符串形式提供，不依赖外部 .glsl 文件

### 2.10 导出宏

3D 模块统一使用 `QWT3D_EXPORT`（定义在 `qwt3d_global.h`）。所有公开类必须标注此宏。

---

## 3. 注意事项

### 3.1 性能考量

- **VBO 更新策略**：静态数据用 `GL_STATIC_DRAW`，动态数据用 `GL_DYNAMIC_DRAW`，流式数据用 `GL_STREAM_DRAW`
- **实例化渲染**：散点图、柱状图、向量场等重复几何体应使用 `glDrawArraysInstanced` 减少绘制调用
- **视锥剔除**：大数据集应实现视锥剔除，跳过不可见区域
- **LOD（细节层次）**：表面图应支持降采样（已有 `setResolution()`），新类型也应考虑
- **着色器编译缓存**：着色器程序应在首次使用时编译并缓存，避免重复编译

### 3.2 错误处理

- OpenGL 错误检查：使用 `qwt3d_openglhelper.h` 中的 `gl_error()` 函数
- 着色器编译/链接失败：输出错误日志到 `qWarning()`，回退到固定管线
- VBO 创建失败：回退到 immediate mode 渲染
- 数据有效性检查：在 `loadFromData()` 入口检查数据指针和尺寸

### 3.3 依赖管理

- **不引入新的第三方库**：所有新功能基于 Qt OpenGL 模块和现有依赖
- **gl2ps**：矢量导出继续使用内置的 gl2ps（`src/plot3d/3rdparty/gl2ps/`）
- **core 模块复用**：颜色映射复用 `QwtColorMap`/`QwtColorMapPreset`，栅格数据复用 `QwtGridRasterData`/`QwtMatrixRasterData`
- **OpenGL 函数**：通过 `QOpenGLFunctions` 或 `QOpenGLExtraFunctions` 获取，不直接链接系统 GL 库

### 3.4 OpenGL Core Profile 兼容性

- 固定管线 API（`glBegin`/`glEnd`/`glLightfv`/`glMaterialfv` 等）在 Core Profile 下不可用
- 现有代码全部使用固定管线，因此当前不支持 Core Profile
- 新基础设施（plan00）应提供 Core Profile 检测和自动回退机制
- 新绘图类型应优先使用可编程管线，确保在 Core Profile 下可用

### 3.5 线程安全

- 所有 OpenGL 调用必须在主线程（QOpenGLWidget 上下文线程）执行
- 数据计算（如 Marching Cubes、RK4 积分、Delaunay 三角化）可在后台线程执行
- 后台线程完成后通过信号通知主线程更新 VBO 和触发重绘

### 3.6 内存管理

- `Color` 对象由 `Plot3D` 管理（通过 `destroy()` 虚方法删除）
- `Enrichment` 对象由 `Plot3D` 的 `EnrichmentList` 管理
- VBO/VAO 使用 RAII 封装，析构时自动释放 GL 资源
- 着色器程序使用 RAII 封装

### 3.7 脚本路径约定

- 所有新增源文件放在 `src/plot3d/` 目录下
- 新增私有头文件（PIMPL）放在 `src/plot3d/` 目录下，不加入 `QWTPLOT3D_HEADER_PLOT_3D` 安装列表
- 新增示例放在 `examples/3D/` 目录下，在 `examples/CMakeLists.txt` 中注册
- 不可触碰的文件：`src-amalgamate/` 和 `tools/` 目录下的文件

### 3.8 验证示例与截图基础设施

**强制要求**：每个新增 3D 绘图类型（plan03-12）必须配套编写验证示例，位于 `examples/3D/<plot-name>/` 目录下。所有示例**必须支持命令行截图导出**，以便批量文档生成和 AI 验证。

#### 截图命令行接口

所有 3D 示例必须支持 `-p` 命令行参数，用于将界面截图保存到指定路径：

```powershell
# 运行示例并截图到指定路径
.\example.exe -p "docs/assets/screenshots-3D/scatterplot.png"

# 不带 -p 参数则正常运行（交互式）
.\example.exe
```

#### 实现规范

所有 3D 示例共享一个 header-only 截图工具（详见 `plan13-examples-infrastructure.md`）：

```cpp
// 示例 main.cpp 模板
#include "screenshot_helper.h"  // 来自 examples/3D/common/

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    MyPlot plot;
    plot.resize(800, 600);  // 统一窗口尺寸，确保截图一致性
    plot.show();

    // 如果有 -p 参数，截图后自动退出；否则正常交互运行
    Qwt3DExamples::setupScreenshotCapture(&plot, argc, argv, app);

    return app.exec();
}
```

#### 截图要求

| 要求 | 说明 |
|------|------|
| **格式** | PNG（无损，适合文档） |
| **窗口尺寸** | 统一 800×600，确保所有截图比例一致 |
| **自动建目录** | 输出路径中的目录不存在时自动创建 |
| **渲染等待** | 延迟 500ms 确保 GL 初始化和首帧渲染完成后再截图 |
| **退出行为** | 截图保存成功后自动退出（非零返回码表示失败） |
| **默认运行** | 不带 `-p` 参数时正常运行，交互式操作 |

#### 示例覆盖要求

每个绘图类型的示例必须覆盖以下场景（至少 2 个截图）：

| 场景 | 说明 |
|------|------|
| **基本渲染** | 默认数据、默认样式、默认视角的截图 |
| **功能切换** | 切换渲染模式/颜色映射/样式后的截图（不同模式各一张） |

截图命名约定：`<plot-name>-<mode>.png`，例如 `scatterplot-sphere.png`、`scatterplot-point.png`

---

## 4. 已有可复用的基础设施

执行者在实现新绘图类型时，应优先复用以下已有设施：

### 4.1 数据结构（`qwt3d_types.h`）

| 类型 | 说明 | 适用场景 |
|------|------|---------|
| `Triple` | 三维点(x,y,z)，含向量运算 | 所有3D绘图的基础数据类型 |
| `TripleField` | `vector<Triple>` 点云 | 散点图、线图、流线 |
| `FreeVector` | 基点+终点向量 | 向量场 |
| `FreeVectorField` | `vector<FreeVector>` | 向量场图 |
| `Cell` | `vector<unsigned>` 多边形顶点索引 | 等值面、散乱曲面 |
| `CellField` | `vector<Cell>` 多边形集合 | 网格数据 |
| `ParallelEpiped` | 包围盒(minVertex, maxVertex) | 数据范围 |
| `RGBA` | 颜色(r,g,b,a) | 颜色映射 |
| `Data` | 抽象数据基类 | 可扩展新数据类型 |
| `GridData` | 规则网格数据 | 表面图、等高线 |
| `CellData` | 多边形单元数据 | 等值面、散乱曲面 |

### 4.2 颜色系统（`qwt3d_color.h` + `qwt3d_colormap_color.h`）

- `Color` 抽象 functor：`operator()(double x, double y, double z) → RGBA`
- `StandardColor`：z 值驱动着色，通过 `setPreset()` 使用 core 模块的 22 种 colormap 预设
- `ColorMapColor`：直接桥接 core 模块 `QwtColorMap`（v7 新增）
- `ColorLegend`：颜色图例组件，支持水平/垂直方向

### 4.3 坐标轴系统（`qwt3d_coordsys.h` + `qwt3d_axis.h` + `qwt3d_scale.h`）

- 12 轴体系（X/Y/Z 各 4 根）
- `BOX`/`FRAME`/`NOCOORD` 三种坐标系样式
- `LinearScale` / `LogScale` 内置刻度引擎
- 可自定义刻度引擎（继承 `Scale` 类）

### 4.4 Enrichment 系统（`qwt3d_enrichment.h` + `qwt3d_enrichment_std.h`）

- `Enrichment` 抽象基类，4 种类型枚举（仅 VERTEXENRICHMENT 已实现）
- `VertexEnrichment`：每顶点调用 `draw(Triple const&)`
- `Arrow`：已有完整的 3D 箭头几何和方向旋转算法，**向量场图可直接复用**
- `CrossHair`/`Dot`/`Cone`：基础标注元素

### 4.5 Plot3D 基类 API（`qwt3d_plot.h`）

新绘图类型继承 `Plot3D` 后可使用以下 protected 接口：

| 方法 | 说明 |
|------|------|
| `displayLists()` | 获取显示列表 vector（DataObject/LegendObject/NormalObject） |
| `actualData()` | 获取当前数据指针 |
| `setActualData(Data*)` | 设置当前数据指针 |
| `setHull(ParallelEpiped)` | 设置数据包围盒 |
| `createEnrichment(Enrichment&)` | 渲染 enrichment（虚函数，可 override） |
| `createEnrichments()` | 遍历 enrichment 列表渲染 |
| `createCoordinateSystem()` | 创建坐标系 |

纯虚函数（必须实现）：
- `calculateHull()` — 从数据计算包围盒
- `createData()` — 创建 OpenGL 渲染数据（显示列表或 VBO）

### 4.6 主题系统（`qwt3d_theme.h`）

- `Qwt3DTheme` 封装全部视觉属性
- `apply(Plot3D* plot)` 一次性应用
- 新绘图类型应确保兼容主题系统（颜色、光照、着色模式等属性应能被主题覆盖）

### 4.7 IO 系统（`qwt3d_io.h`）

- `IO::save()` / `IO::load()` 统一接口
- 可通过 `defineOutputHandler()` / `defineInputHandler()` 注册新格式
- 矢量导出通过 gl2ps 自动捕获 OpenGL 绘制调用

---

## 5. 计划文件依赖关系

```
plan01 (Bug修复) — 独立，可随时执行，不依赖其他计划

plan13 (示例基础设施) — 独立，可随时执行，为所有示例提供截图工具

plan00 (渲染基础设施 + 公共组件)
    ├─→ plan02 (GraphPlot基类) ──→ plan03 (散点图)
    │                          ──→ plan04 (柱状图)
    │                          ──→ plan05 (线图)
    │                          ──→ plan06 (向量场, 需修改 Arrow::calcRotation 可见性)
    │                          ──→ plan07 (流线)
    ├─→ plan08 (等高线/切片)
    ├─→ plan09 (等值面)
    ├─→ plan10 (体积渲染)
    └─→ plan11 (散乱曲面)

plan03-11 中至少 2 种绘图就绪 ──→ plan12 (MultiPlot)
```

plan00 包含三个独立子模块，可按需分步实现：
- **GPU 资源管理**（GLBuffer/GLVertexArray/ShaderProgram/RenderContext）— plan02-12 的前提
- **Field3D 抽象**（ScalarField/VectorField/GridScalarField/GridVectorField）— plan07-10 的前提
- **公共几何工具**（TriangleMesh/TubeBuilder）— plan05/plan07/plan09 的前提

### 建议执行顺序

1. **plan01** (Bug修复) — 独立，可立即执行，风险最低
2. **plan13** (示例基础设施) — 独立，可立即执行，为所有示例提供截图工具
3. **plan00** (渲染基础设施 + 公共组件) — 所有现代 OpenGL 绘图和场可视化的前提
4. **plan02** (GraphPlot基类) — plan03-07 的前提
5. **plan03-07** (图基绘图类型) — 可并行执行，每个需配合 plan13 的截图工具
6. **plan08-11** (标量场可视化) — 可并行执行，每个需配合 plan13 的截图工具
7. **plan12** (MultiPlot) — 依赖至少 2 种绘图类型就绪

---

## 6. 样式参考

本目录下所有 `plan*.md` 文件应遵循以下排版规范：

### 6.1 标题层级

```
# 计划标题（H1，仅一个）
## 大节（H2）
### 子节（H3）
```

### 6.2 代码块

- C++ 代码用 ` ```cpp` 标注
- CMake 代码用 ` ```cmake` 标注
- Shell 命令用 ` ```powershell` 标注
- 着色器代码用 ` ```glsl` 标注

### 6.3 表格

使用 GitHub 风格 Markdown 表格，表头和分隔行必须存在：

```markdown
| 列1 | 列2 | 列3 |
|------|------|------|
| 值1 | 值2 | 值3 |
```

### 6.4 文件路径

文件路径使用反引号包裹，相对于项目根目录：`src/plot3d/qwt3d_plot.h`

### 6.5 每个计划文件的结构模板

```markdown
请先阅读 `README.md` 以了解全局规范。

# [计划名称]

## 目标
[本计划要实现什么]

## 前置依赖
[依赖哪些其他 plan，或无依赖]

## 涉及文件
[需要创建/修改的文件清单，含 PIMPL 私有头文件 `_p.h`]

## 技术方案
### 数据模型
### 渲染管线
### API 设计
### PIMPL 设计
[PrivateData 类的成员变量和关键方法]

## 验证方式
### 单元测试
### 示例验证
[示例程序路径和功能描述]
### 截图验证
[使用 -p 参数导出的截图列表，每个模式一张]

## 风险与注意事项
```

### 6.6 语言

所有计划文件使用中文编写。代码注释、API 名称、类名等保持英文。

---

## 7. 计划文件清单

| 文件 | 标题 | 依赖 | 状态 |
|------|------|------|------|
| `README.md` | 总纲 | — | 本文件 |
| `plan00-rendering-infrastructure.md` | 渲染基础设施 + 公共组件（GLBuffer/Shader/Field3D/TriangleMesh/TubeBuilder） | 无 | 待实现 |
| `plan01-bugfixes.md` | 3个Bug修复 | 无 | 待实现 |
| `plan02-graphplot-base.md` | GraphPlot基类实现 | plan00 | 待实现 |
| `plan03-scatter-plot.md` | 3D散点图 | plan02 | 待实现 |
| `plan04-bar-chart.md` | 3D柱状图 | plan02 | 待实现 |
| `plan05-line-curve-plot.md` | 3D线图/参数曲线 | plan02 | 待实现 |
| `plan06-vector-field.md` | 3D向量场 | plan02 | 待实现 |
| `plan07-streamline.md` | 3D流线 | plan00 + plan02 | 待实现 |
| `plan08-contour-slice.md` | 3D等高线/切片 | plan00 (Field3D) | 待实现 |
| `plan09-isosurface.md` | 3D等值面 | plan00 (Field3D + TriangleMesh) | 待实现 |
| `plan10-volume-rendering.md` | 体积渲染(VolumePlot) | plan00 (Field3D) | 待实现 |
| `plan11-scattered-surface.md` | 散乱点插值曲面 | 无(扩展SurfacePlot) | 待实现 |
| `plan12-multiplot.md` | MultiPlot多绘图布局 | 各绘图类型 | 待实现 |
| `plan13-examples-infrastructure.md` | 验证示例基础设施（截图工具、公共头文件、CMake注册） | 无 | 待实现 |
