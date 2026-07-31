# Qwt 3D 绘图模块 (`qwt::plot3d`) AI Agent 指引

本文件是 `src/plot3d/` 的局部指引，**补充**根目录 `AGENTS.md`。两者冲突时，针对 3D 模块的具体约定以本文件为准；全局规范（构建、PIMPL 宏、命名、clang-format、注释、现代 C++、信号槽）见根 `AGENTS.md`。

## 模块定位

- 输出共享库 `qwt::plot3d`（`qwtplot3d.dll` / `libqwtplot3d.so`），导出宏 `QWT3D_EXPORT`。
- 依赖 `qwt::core`（颜色 / 数学 / 几何 / 变换 / 时间等），**不依赖** `qwt::plot`（2D）。
- 基于 `QOpenGLWidget` 的 3D 绘图：`Qwt3DPlot`（渲染窗口）+ `Qwt3DPlotItem`（数据/绘制 item）架构，VBO/VAO + GLSL 3.3 Core 着色器。

## 架构：Plot + Item

```
Qwt3DPlot  (QOpenGLWidget)           管理 GL 上下文 / 视图变换 / 光照 / 坐标系统 / item 列表 / 主题
  └─ QList<Qwt3DPlotItem*>           通过 attach()/detach() 挂载，paintGL() 按z序遍历 item->draw()
        ├─ Qwt3DSurface              曲面 item：持有数据 + VBO/VAO/EBO/shader + 颜色 functor
        │     ├─ Qwt3DColor*         颜色 functor（纯值对象，z 范围由 surface 推入）
        │     └─ list<Qwt3DEnrichment*>  扩展（当前为 stub，渲染待实现）
        ├─ Qwt3DLine                 线图 item：Tube（沿折线扫掠圆柱，带光照）/ Lines / Dots
        └─ Qwt3DBar                  柱状图 item：逐柱 cuboid（6 面扁平法向），Filled / FilledMesh / Wireframe
```

> 所有 item 均继承 `Qwt3DPlotItem`，实现 `draw()` + `hull()` +（可选）`populateLegendColors()`，自带 VBO/VAO 与颜色 functor。`Qwt3DLine`（Tube/Dots 用共享 `lineShader`/`pointShader`，Tube 用 surface shader）与 `Qwt3DBar`（复用 surface shader）均遵循与 `Qwt3DSurface` 相同的惰性 VBO / `pushColorRange()` / `invalidateColors()` 模式。

**关键原则**：`Qwt3DPlot` 是纯渲染窗口，**不持绘图数据**；数据与绘制逻辑都在 item 里。`Qwt3DPlotItem` 是非 widget 的纯数据 + 绘制对象。

## 分层与依赖方向规则（最重要）

类按概念分层。**依赖只能向下（同层或更低层）；禁止低层类持有高层类的指针 / 回调 / 信号 / 反向引用。** 这是本模块最常被违反的规则。

| Tier | 文件 | 说明 |
|------|------|------|
| 0 纯值/工具 | `qwt3d_types` `qwt3d_global` `qwt3d_portability` `qwt3d_helper` `qwt3d_autoptr` `qwt3d_autoscaler` `qwt3d_scale` `qwt3d_mapping` | 无任何 3D 类指针 |
| 1 静默值对象 | `qwt3d_color` `qwt3d_colormap_color` `qwt3d_enrichment`(+`_std`) `qwt3d_theme` | 不通知任何人；mutator 静默（对应 2D `QwtColorMap`/`QwtSymbol`） |
| 2 drawable | `qwt3d_drawable`(base) `qwt3d_label` `qwt3d_axis` `qwt3d_colorlegend` `qwt3d_coordsys` | 通过 `Qwt3DRenderContext` 参数接收渲染资源，无反向指针 |
| 3 数据源映射 | `qwt3d_gridmapping` `qwt3d_function` `qwt3d_parametricsurface` | `create()` 返回数据，无汇指针 |
| 4 item | `qwt3d_plotitem`(base) `qwt3d_surface`(+`_p`) `qwt3d_bar`(+`_p`) `qwt3d_line3d`(+`_p`) | 持 `Qwt3DPlot*`（item→widget，**合法**） |
| 5 widget | `qwt3d_plot`(+`_p`) `qwt3d_lighting` `qwt3d_mousekeyboard` `qwt3d_movements` | 高持低，方向正确 |
| 横切 I/O | `qwt3d_io` `qwt3d_io_reader` `qwt3d_io_gl2ps` | 仅以 `Qwt3DPlot*` 为 functor 参数，**不存储** |

**唯一合法的向上指针**：`Qwt3DPlotItem::plot()`（item→widget，Tier 4→5）。因为 item 渲染需要 widget 暴露的 GL 上下文 / shaders / 矩阵。**除此之外，任何低→高的反向指针都是 bug。**

### DO / DON'T（典型反例）

- ❌ 在颜色 functor（Tier 1）里存 `Qwt3DSurface*`/`Qwt3DPlot*` 并在 mutator 里回调 `invalidateColors()`——这是 2D 刻意回避的反模式。本模块已在 v7.3.x 移除该反向指针。
- ❌ 给颜色 functor / enrichment / theme 加「方便起见」的 owner 回调或 `std::function` 通知。值对象必须静默。
- ❌ 让 drawable（Tier 2）通过 `m_plot` 反向调用 item/widget 的业务方法。
- ✅ 通知一律由上层 item 的 setter 触发（见下）。
- ✅ 低层需要的数据，由上层**作为参数/数据推入**，而非低层去拉高层指针。例：z 范围由 `Qwt3DSurface::pushColorRange()` 推入 functor，functor 不去问 `plot()->hull()`。

## 通知机制（与 2D 对齐）

值对象（颜色 functor、enrichment、theme）**绝不**通知容器：mutator 静默，最多重建内部缓存。通知由 item 拥有，对应 2D `QwtPlotItem`：

- `Qwt3DPlotItem::itemChanged()` → 通知 `Qwt3DPlot` 自动刷新（repaint）。
- `Qwt3DSurface::invalidateColors()` → `m_vboDirty = true; itemChanged();`（下一帧 `buildVBO()` 重算顶点色）。
- 所有 visual-property setter（`setDataColor` / `setMeshColor` / `setPlotStyle` …）存值后调 `itemChanged()`。

### 颜色 functor 契约（`Qwt3DColor` 及子类 `Qwt3DStandardColor` / `Qwt3DColorMapColor`）

1. **纯值对象**：无 `Qwt3DSurface*` / `Qwt3DPlot*` 反向指针、无回调、无 signal。头文件不 include、不前向声明 `qwt3d_surface.h` / `qwt3d_plot.h`。
2. **z 范围作为数据由上层推入**：`Qwt3DSurface::pushColorRange()` 在 `buildVBO()` 顶点循环前、`populateLegendColors()` 调 `createVector` 前，经 `setActiveRange(zMin, zMax)` 推入并集 hull 的 z 范围；`operator()` 读 `activeZMin()` / `activeZMax()`。`setInterval()` 的 manual override 仍优先于 active range。
3. **就地修改已挂载 functor 后，调用方必须调 `surface->invalidateColors()`**：`setAlpha` / `setPreset` / `setColorMap` / `setInterval` / `setColorVector` / `reset` 现在都是静默的。对应 2D「改 `QwtSymbol` 后调 `itemChanged()`」契约，`invalidateColors()` 的 docstring 已声明此契约。
4. 构造函数**不接受** `Qwt3DPlot*` 参数（v7.3.x 已移除）。range 默认 `[0,1]`，attach 后由 surface 推入真实范围。

## 已解决 deferred smell（RESOLVED）

以下两项已评估并解决。**不要"为了一致性"把颜色类的反向指针加回来，也不要在未做完整方案前贸然拆它们：**

1. **`Qwt3DDrawable::m_plot`（Tier 2 base 烤入 `Qwt3DPlot*`）** —— RESOLVED：引入 `Qwt3DRenderContext` 值结构体，`draw(const Qwt3DRenderContext& ctx)` 传 const 引用，移除 base 里的 concrete 指针。`paintGL()` 在每个调用点用当前矩阵构造 ctx 并下传。`axis` / `label` / `legend` / `coordsys` 全部 draw 路径已迁移。
2. **`Qwt3DGridMapping::m_surface`（Tier 3 数据源持汇指针）** —— RESOLVED：`create()` 返回 `Qwt3DFunctionData` / `Qwt3DParametricData` 数据结构体，由调用方喂给 `Qwt3DSurface::loadFromData()`。移除了 `m_surface`、`surface()`、`setSurface()`、`assign()` 及带 `Qwt3DSurface` 参数的构造函数。依赖方向从 Tier 3→4（违规）翻转为 Tier 4→3（合法）。

> ⚠ `Qwt3DFunction` / `Qwt3DParametricSurface` 的 `create()` 现在返回数据，不再需要 `Qwt3DSurface*`。与颜色类无关，勿与 `Qwt3DColor` 混淆。

## 命名与 OpenGL 规范

- 所有类 `Qwt3D` 前缀，**全局作用域，无 `namespace Qwt3D`**（v7.3.3+ 已彻底移除命名空间）。
- **禁止 legacy GL**：`glBegin/glEnd`、display list、`glRotatef` 等固定管线 API 一律不用。全部使用 VBO/VAO + GLSL 3.3 Core shader。
- PIMPL 用 `QWT_DECLARE_PRIVATE` / `QWT_D()` / `QWT_DC()`（详见根 `AGENTS.md`）。3D 私有头：`qwt3d_plot_p.h`、`qwt3d_surface_p.h`、`qwt3d_bar_p.h`、`qwt3d_line3d_p.h`。
- 主题：`Qwt3DTheme`（10 种内置预设）+ `Qwt3DColorMapColor`（适配器，桥接 core 的 22 种科学 colormap 预设到 3D 表面）。
- 光照预设：`Qwt3DTheme::LightingPreset`（`NoLighting` / `FlatLight` / `Studio` / `Outdoor` / `Soft`）。

## 不可触碰

- `src-amalgamate/` 合成文件、`tools/` 合成脚本（见根 `AGENTS.md`）。
