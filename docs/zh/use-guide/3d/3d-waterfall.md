# 3D 瀑布图 - Qwt3DWaterfall

`Qwt3DWaterfall` 是一个将一维轨迹序列（频谱、色谱……）沿深度轴堆叠绘制的 3D 绘图 item。它自带完整的实时数据机制：每个采样周期用 `pushTrace()` 推入一条轨迹，item 会自己维护滑动窗口、深度编号、帘幕几何与颜色映射。

3D 模块总览见 [3D绘图简介](3d-plot.md)。`Qwt3DWaterfall` 是一个 `Qwt3DPlotItem`，与 `Qwt3DSurface`、`Qwt3DLine` 遵循相同的 attach/draw/hull 契约。

坐标轴遵循模块约定（数值轴为 z，与 `Qwt3DSurface` 一致）：x = bin 值、y = 轨迹在窗口中的位置（深度）、z = 幅度（朝上）。请据此标注轴（`X1` "Bin"、`Y1` "Frames ago"、`Z1` "Amplitude"）；`Qwt3DTextScale` 应放在深度轴上。

![waterfall3D 示例](../../../assets/screenshots-3D/waterfall3D.png)

同一实时场景的 `Wireframe`（仅描边）样式，是密集窗口下开销最低的渲染方式，也适合在不加填充时观察轨迹堆叠：

![waterfall3D Wireframe 样式](../../../assets/screenshots-3D/waterfall3D-2.png)

## 核心特性

- 内置滑动窗口：`pushTrace()` 追加最新轨迹并丢弃最旧轨迹；用户无需自己管理窗口或深度编号
- 深度轴稳定：轨迹的深度坐标就是槽位索引，hull 的 y 范围单调增长到 `[0, traceCapacity()-1]` 后保持稳定，不会出现逐帧的深度轴自动缩放抖动
- 四种几何样式：`Ribbon`（填充帘幕）、`RibbonMesh`（帘幕 + 描边）、`Wireframe`（仅描边）、`Tube`（带光照的实体管）
- 三种上色模式：按深度位置、按幅度（采样值）、纯色
- 深度轴文字刻度：用 `Qwt3DTextScale` 把深度轴标成 "距当前帧数" / 时间戳
- 一个 item = 一条图例：瀑布图向颜色图例贡献一条渐变，而不是 N 条曲线的 N 个条目
- 主题集成：由 `Qwt3DTheme::applyToItem()` 识别

## 基本概念

### 几何样式

| 样式 | 说明 |
|------|------|
| `Qwt3DWaterfall::Ribbon` | 从基线填充到轨迹的帘幕（默认）。经典瀑布外观。 |
| `Qwt3DWaterfall::RibbonMesh` | 帘幕 + 顶部折线描边。描边固定在填充之后绘制，与 item 的 z 值无关。 |
| `Qwt3DWaterfall::Wireframe` | 仅折线描边，无填充。开销最低。 |
| `Qwt3DWaterfall::Tube` | 每条轨迹扫掠成带光照的实体管，截面在视图空间为正圆（与 `Qwt3DLine::Tube` 同一构造方式）。密集轨迹会降采样到 ≤129 个环站；开销最高的样式。 |

### 上色模式

| 模式 | 说明 |
|------|------|
| `Qwt3DWaterfall::ByDepth` | colormap 按轨迹在窗口中的位置取色（默认）。每条帘幕为单色。 |
| `Qwt3DWaterfall::ByAmplitude` | colormap 按采样值取色，即常见的"频谱强度"外观。会自动把 `Qwt3DColorMapColor` functor 切到 z 坐标（`setSourceAxis(ColorFromZ)`）。 |
| `Qwt3DWaterfall::Solid` | `setColor()` 指定的纯色。 |

### 滑动窗口语义

窗口是固定数量的 `traceCapacity()` 个槽位，按从旧到新排列：

- `pushTrace()` 把新轨迹写入最新槽位；窗口满时先丢弃最旧槽位（槽位搬移是廉价的移动语义，不做深拷贝）。
- 第一条 `pushTrace()` 确定 `samplesPerTrace()`。之后的帧会被装进这个 bin 宽度（超长截断、过短零填充），窗口宽度（以及 hull 的 x 范围）不会抖动。
- 轨迹的深度 y 就是槽位索引，因此 hull 的 y 范围只会从 `[0,0]` 单调增长到 `[0, traceCapacity()-1]` 后保持稳定。这正是实时瀑布"零自定义代码即可用"的关键，轴自动缩放不会逐帧闪烁。
- `setDepthOrder(NewestBehind)` 反转方向（最新轨迹在 `z == 0`），不改变 hull 范围。
- `setTraceCapacity()` 缩小时保留最新的轨迹；扩大时保留现有数据并让窗口重新填满。
- `clear()` 清空窗口，但保留容量、样式与颜色。

## 用法

实时 3D 瀑布图示例位于 `examples/3D/waterfall3D`（上图 2D 实时频谱 + 下图 3D 滑动窗口瀑布，两图喂同一帧数据）。

### 1. 静态瀑布（零自定义代码）

一次性的静态瀑布不需要任何自定义类：创建 `Qwt3DWaterfall`，在喂数据*之前*先 attach 到 `Qwt3DPlot`，用 `setTraceCapacity()` 定窗口大小，`setPlotStyle()` 选几何风格（`Ribbon` / `RibbonMesh` / `Wireframe` / `Tube`），`setColorMode()` 定上色方式，然后每条迹线一次 `pushTrace()` 把窗口填满，如下例所示：

```cpp
#include <qwt3d_plot.h>
#include <qwt3d_waterfall.h>

Qwt3DPlot* plot = new Qwt3DPlot(parent);

auto* waterfall = new Qwt3DWaterfall();
waterfall->attach(plot);                       // 先 attach（见坑 1）
waterfall->setTraceCapacity(64);
waterfall->setPlotStyle(Qwt3DWaterfall::RibbonMesh);
waterfall->setColorMode(Qwt3DWaterfall::ByAmplitude);
waterfall->setColorRange(0.0, 1.0);            // 固定色域，避免自动范围抖动

for (int i = 0; i < 64; ++i)
    waterfall->pushTrace(frame(i));            // QVector<double>，256 bins

plot->setAspectRatioMode(AUTOFILL);

// 轴标题（x = bin、y = 深度、z = 幅度）；设置标题即启用其绘制，
// 且标题在坐标系重建后仍然保留
plot->coordinates()->axes[X1].setLabelString("Bin");
plot->coordinates()->axes[Y1].setLabelString("Frames ago");
plot->coordinates()->axes[Z1].setLabelString("Amplitude");
```

> **说明 — 强各向异性数据与视图缩放**
>
> 典型频谱瀑布的跨度是 bins `0..M-1`、幅度 `0..1`、深度 `0..capacity-1`，强各向异性。应使用 `AUTOFILL`：它在推导投影之前把每个轴归一化到最大数据范围，之后再用 `plot->setScale()` 塑造归一化立方体（示例用 `(1.0, 0.35, 0.6)`：满 bin 宽、压缩深度、60% 幅度高度）。在 `DATARATIO` 下真实比例会让帘幕成为 255 单位宽场景里 1 单位高的细条，而且逐轴 `setScale()` 救不回来，因为投影半径是按未缩放的 hull 推导的。

### 2. 实时瀑布（30 Hz）

实时模式复用同一个对象：初始化时把容量、颜色模式和固定的 `setColorRange()` 色域配置好，之后每个定时器 tick 只需一次 `pushTrace()`，它会自动滑动窗口并触发 `itemChanged()` → `update()` 刷新：

```cpp
// 初始化：
waterfall->setTraceCapacity(64);
waterfall->setColorMode(Qwt3DWaterfall::ByAmplitude);
waterfall->setColorRange(0.0, 1.0);   // 幅度颜色稳定

// 每个定时器 tick：
const QVector<double>& frame = generator.nextFrame();
waterfall->pushTrace(frame);          // 触发 itemChanged() -> update()
```

实时瀑布务必调用 `setColorRange()`：`ByAmplitude` 的自动范围跟随当前数据最小/最大值，颜色会逐帧漂移。固定色域让"黄色 = 0 dB、紫色 = 底噪"保持稳定。

示例还会塑造归一化视图（`AUTOFILL` 加 `setScale(1.0, 0.35, 0.6)`，压缩深度轴），并在显示前预填满窗口，瀑布从满窗开始而不是从一条曲线慢慢爬升。

### 3. 深度轴文字（"距当前帧数"）

`Qwt3DTextScale` 是一个主刻度标签来自用户字符串列表的线性 scale：

```cpp
#include <qwt3d_scale.h>

const int capacity = waterfall->traceCapacity();
const int majors = 4;

QVector<QString> labels;
for (int k = 0; k <= majors; ++k) {
    const int framesAgo = qRound(capacity - 1 - k * double(capacity - 1) / majors);
    labels << QString::number(-framesAgo);   // "-63", "-48", "-32", "-16", "0"
}

auto& axis = plot->coordinates()->axes[Y1];   // 深度沿 y 轴
axis.setAutoScale(false);
axis.setMajors(majors);
auto* scale = new Qwt3DTextScale();
scale->setLabels(labels);
axis.setScale(scale);
```

> **警告 — 坑：每个轴各要一个 scale 对象**
>
> `Qwt3DAxis::setScale()` 会把 scale 交给一个克隆指针，克隆并消耗传入的对象。在多个轴之间共享同一个 `Qwt3DTextScale*` 会留下悬垂指针并崩溃。每个轴各建一个实例。

### 4. 上色与颜色 functor

三种 `ColorMode` 对应不同的颜色来源：`ByAmplitude` 用 `Qwt3DColorMapColor` functor（通过 `setDataColor()` 安装，可按 `"plasma"` 等预设名选择）按幅度 z 值上色，`ByDepth` 按迹线在窗口中的槽位上色（默认），`Solid` 则使用 `setColor()` 给定的单一 `RGBA` 颜色：

```cpp
// 指定 preset 的幅度上色：
waterfall->setColorMode(Qwt3DWaterfall::ByAmplitude);
waterfall->setDataColor(new Qwt3DColorMapColor("plasma"));

// 深度上色（默认）：
waterfall->setColorMode(Qwt3DWaterfall::ByDepth);

// 纯色：
waterfall->setColorMode(Qwt3DWaterfall::Solid);
waterfall->setColor(RGBA(0.4, 0.6, 0.9, 1.0));
```

`setColorMode(ByAmplitude)` 会通过 `setSourceAxis(ColorFromZ)` 把 `Qwt3DColorMapColor` functor 切到 z 坐标（幅度轴）；`ByDepth` 切到 y（深度轴）。functor 默认 source 是 `ColorFromZ`，因此既有的 surface/bar/line 渲染不受影响。若使用其它类型的 functor，则由其所有者决定如何解释 `(x, y, z)` 参数。

颜色在 VBO 构建时烘焙；就地修改 functor 后需调用 `invalidateColors()`（或任意 setter）触发重建。

### 5. 基线、不透明度与描边

另外三个 setter 用来微调帘幕观感：`setBaseline()` 固定帘幕起绘的数值（默认为幅度最小值），`setOpacity()` 让帘幕半透明混色，`setOutlineColor()` 与 `setOutlineWidth()` 则控制 `RibbonMesh` / `Wireframe` 风格描边的颜色与线宽：

```cpp
waterfall->setBaseline(0.0);        // 帘幕从 y == 0 开始（默认：幅度最小值）
waterfall->setOpacity(0.6);         // 半透明帘幕
waterfall->setOutlineColor(RGBA(0.1, 0.1, 0.1, 1.0));
waterfall->setOutlineWidth(1.0);    // RibbonMesh / Wireframe
```

半透明帘幕按"从最旧槽位到最新槽位"的顺序绘制。当视线几乎平行于帘幕平面时可能出现穿插伪影；建议从正面或上方俯视窗口（示例默认 `setRotation(55, 0, 25)`）。

## 两个实测验证过的坑

1. 先 `attach()` 再喂数据。v7.5.0 之前 `Qwt3DPlot::attach()` 不重算场景。若数据在 `attach()` *之前*装载、且中间没有调用任何 setter（因此没有触发 `itemChanged()`），plot 的 hull 会停留在 `(0,0,0)-(0,0,0)`，画面全白且无任何警告。现在 `attach()`/`detach()` 会重算 hull 并集与坐标系，detach 后 hull 也会收缩。

2. 派生 scale 必须 override `clone()`。`Qwt3DAxis::setScale()` 把 scale 交给克隆指针，其赋值会调用 `clone()`。没有 override `clone()` 的用户派生 scale 会在 `setScale()` 内部被切片成基类，自定义 `ticLabel()` 静默失效（修复前实测逐像素零差异）。`Qwt3DTextScale` 已 override；自写 scale 类必须同样处理。

## 性能建议

| 工况 | 典型帧耗时（MSVC 2019, Release, 1000×700） |
|------|-------------------------------------------|
| 64 条 × 256 bin，RibbonMesh（示例默认） | ~4 ms paintGL |
| 128 条 × 512 bin，Ribbon | 远在 16 ms 帧预算之内 |

- 每次 `pushTrace()` 重建整窗 VBO（所有槽位都在移动）。上传预算为每帧 `capacity × bins × 28 字节`；目标工况（≤ 128 × 512）在 30 fps 下约 14 MB/s。
- 超大窗口（`capacity > 128` 且 `bins > 1024`）的优化通道已在源码中注释：每槽位一次 draw call、行 z 由 item 私有 shader 的 uniform 提供，每帧只重新上传最新一行。需要前先实测。
- 只看形状用 `Wireframe`；只有需要粗的、带光照的笔画才用 `Tube`，它是开销最高的样式（每条轨迹一个扫掠面）。管半径上限为半个槽位，相邻轨迹不会融合；超大窗口建议用 `Ribbon`/`RibbonMesh`。

## 另见

- [3D绘图简介](3d-plot.md)：模块架构、`Qwt3DPlot` 与 item 契约
- [3D线图](3d-line-plot.md)：单曲线的同类 item（`Qwt3DLine`），共享管状几何构造
- `examples/3D/waterfall3D`：实时频谱演示，覆盖全部样式、上色模式与窗口尺寸
