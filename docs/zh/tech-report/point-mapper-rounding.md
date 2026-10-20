# 曲线管线中的像素吸附：`QwtPointMapper::RoundPoints` 与密集曲线阶梯

> **摘要** 同一条光滑曲线的两种采样，在屏幕上本应画出同一幅图。QWT 并非如此：开启抗锯齿时，一条存储 20 万采样的谱线在 14 个单位的放大窗口里呈现明显的阶梯折线，同一峰形的 1.86 万采样版本却保持平滑。问题来自 Qwt 6.2 的基线代码。`QwtPainter::roundingAlignment()` 默认为 `true`，`QwtPainter::isAligning()` 只判断绘制引擎是否把坐标向下取整，从不检查画笔是否启用抗锯齿，`QwtPlotCurve::drawLines()` 因此对开启抗锯齿的控件画布也打开了 `QwtPointMapper::RoundPoints`。本文形式化吸附操作，并证明它抹掉了光栅器计算覆盖率所需的亚像素几何。QWT 7 分两层修复：7.x 系列加入保留亚像素坐标的降采样算法（像素列、真 LTTB、MinMax 桶、自适应），v7.5.1 之后的策略只在画笔不启用抗锯齿时做像素对齐，并把 `WeedOutIntermediatePoints` 与 `RoundPoints` 解耦。用库默认设置重跑演示程序，密集峰肩的混合像素比例与关闭对齐的构建完全一致（0.5470，吸附时为 0.284），稀疏峰在两种构建下都是 0.568，`FilterPointsAggressive` 仍把 199,751 个顶点压缩到 7,384 个。吸附的开销在 100 万采样点上按降采样路径逐条测量，几何特征与视觉特征在下文量化。

> **摘要：核心结论**
>
> - `RoundPoints` 对每个坐标执行 `round(p * DPR) / DPR`。设备像素比（DPR）为 1 时量化到整数像素；DPR 为 2 时步长为半个逻辑单位，仍然对应一个完整设备像素。
> - 在 100 万采样点、不降采样时，该标志使映射器耗时增加 35%（14.2 ms 增至 19.2 ms），输出点数一个不少，它本身不是性能优化。
> - 在 LTTB、MinMax 或 Adaptive 降采样之后，开销落在测量噪声内（−1% 至 +2%），因为只有被选出的几千个顶点需要取整。
> - 对光滑的密集信号，吸附把映射后折线的 99.9% 变成严格的水平或垂直线段；不吸附时该比例为 0%。可见阶梯是交给 `QPainter` 之前的几何性质，与光栅器本身无关。
> - 在实际渲染图上，密集峰肩的抗锯齿混合像素比例从 0.547（关闭吸附）降到 0.284（开启吸附）。稀疏峰在两种设置下都保持在 0.568，因为它每列采样数不足，从未触发该效应。
> - QWT 7 的策略（v7.5.1 之后实现）：只在画笔不启用抗锯齿时对齐。抗锯齿光栅、PDF 与 SVG 输出保留亚像素坐标。

---

## 1. 引言

### 1.1 现象

演示程序 `examples/2D/downsample_demo` 用同一线形渲染两条合成谱线：一条在全量程内存储约 20 万采样，一条约 1.86 万。在下文使用的放大视图（波长 858 至 872）中，密集数据集贡献约 3,500 个采样，稀疏数据集约 330 个。两条曲线都设置了 `QwtPlotCurve::RenderAntialiased`，都不做点抽稀，画布为 1400 x 700。

密集谱线出现阶梯：每过几个像素行，绘制边缘就横向跳一个像素，斜边看起来像用一像素笔画的楼梯。稀疏谱线（采样密度只有十分之一）反而平滑。反直觉之处在于方向：数据越多，图越难看。

![同一峰形的密集与稀疏采样放大对比：密集曲线呈阶梯，稀疏曲线平滑](../../assets/algorithms/fig_roundpoints_crops.png)

*图 1 最高峰的上升沿，5 倍最近邻放大。从左到右依次为：密集采样开启像素对齐、密集采样关闭对齐、稀疏采样开启对齐、稀疏采样关闭对齐。四张图的抗锯齿都是开启的，曲线后方可见坐标网格线。*

第一反应「抗锯齿没开」是错的。图 1 的四个面板中，每条边缘都能看到混合像素。四个面板的抗锯齿设置相同，差别在于光栅器收到的几何形状。

### 1.2 QWT 6 为什么会画出这种效果

阶梯背后的代码路径来自 Qwt 6.2，本分支的起点。三个决定叠加在一起。

第一个是全局开关的默认值（`src/plot/qwt_painter.cpp`）：

```cpp
bool QwtPainter::m_roundingAlignment = true;
```

第二个是 `QwtPainter::isAligning()` 的判断条件。它对 PDF、SVG、未知绘制引擎以及带旋转或缩放变换的画笔返回 `false`，对按整数取整的光栅引擎放行，从不检查 `QPainter::Antialiasing`：

```cpp
const QPaintEngine::Type type = painter->paintEngine()->type();
// PDF、SVG、QPaintEngine::User 与带变换的画笔返回 false
return true;
```

第三个是 `QwtPlotCurve::drawLines()` 的接线。两个开关同时成立时，曲线打开 `RoundPoints`；设置了 `FilterPointsAggressive` 时，再打开 `WeedOutIntermediatePoints`：

```cpp
const bool doAlign = !doFit && QwtPainter::roundingAlignment(painter);
...
if (doAlign) {
    mapper.setFlag(QwtPointMapper::RoundPoints, true);
    mapper.setFlag(QwtPointMapper::WeedOutIntermediatePoints,
                   testPaintAttribute(FilterPointsAggressive));
}
```

对 Qwt 6 面对的渲染环境，这个默认值在当时是合理的。在没有抗锯齿的整数光栅引擎上，吸附后的坐标让一像素细线、网格线和 marker 保持锐利。`WeedOutPoints` 的定义也依赖吸附：落进同一设备像素的采样合并成一个顶点，密集曲线因此画得起。Qwt 6 只有两个降采样属性，`FilterPoints` 和 `FilterPointsAggressive`，二者都定义在吸附后的坐标上。库中没有保留亚像素坐标的算法，吸附步骤不会破坏任何上游结果。抗锯齿在当时被当作改变覆盖率的渲染提示，不影响坐标语义。

到了抗锯齿画布上，同一个决定产生了相反的效果。光栅器收到的折线顶点全部落在像素格点上，在平缓斜边上，它最多只能画出水平段加垂直跳变（§2.2）。库也没有按画笔区分的余地：关掉全局开关能让曲线平滑，代价是非抗锯齿输出失去细线锐度，激进去重路径同时变得不可达。

这套逻辑一直保留到 QWT 7 的 v7.5.1。第 2 到 6 节测量它的几何与时间代价，第 7 节说明 QWT 7 如何改动。

### 1.3 范围与方法

分析围绕五个问题展开：

1. `RoundPoints` 到底做了什么，在哪些地方被打开？
2. 为什么它伤害密集的抗锯齿曲线，却不影响稀疏曲线？
3. 在映射器中，它给各条降采样路径带来多少开销？
4. QWT 6 为什么会出现这种效果，QWT 7 又改了什么？
5. 应用和库本身应该采用哪种标志组合？

映射器与渲染测量基于 v7.5.0 库与 Qt 6.7.3，MSVC 2019 Release 构建（`/O2 /MD /std:c++17`），Intel Core i7-8700 @ 3.20 GHz，Windows 10 x64。绝对耗时随硬件变化，相对顺序与比值稳定。所有图与表由 `docs/assets/algorithms/roundpoints_experiment.py` 从两份原始数据重新生成：`roundpoints_results.csv`（映射器耗时与折线几何，由 `roundpoints_bench.cpp` 产生）和 `roundpoints_render_metrics.csv`（图像指标，由 `roundpoints_render_metrics.py` 产生）。渲染批次来自 `examples/2D/downsample_demo --batch`，分别用修复前的库默认设置与 `--noround` 各生成一次。修复行为的对齐策略晚于 v7.5.1，第 4 到 6 节的数字描述修复前的库，§7.4 用修复后的构建重跑同一批渲染。

### 1.4 与降采样报告的关系

配套报告[《面向屏幕的曲线降采样》](curve-downsampling.md)处理的是「点比像素列多」的问题，推导把 N 个采样压到约 4W 个顶点的分桶算法。同一条管线的另一端是：顶点在降采样之后、从数据坐标转换为绘制坐标时发生了什么。降采样算法只能保住映射器随后没有破坏的东西，两者会相互影响，但它们是两个独立阶段、两组独立开关，混为一谈就会造成混乱。

## 2. 吸附操作

### 2.1 定义

`QwtPointMapper` 先通过坐标轴映射把数据坐标转为绘制坐标，然后可选地施加一个取整函子。该函子定义在 `src/plot/qwt_point_mapper.cpp`：

```cpp
struct QwtRoundF
{
    qreal ratio = 1.0;

    inline double operator()(double value) const
    {
        return qwtRoundValueF(value * ratio) / ratio;
    }
};
```

把 `ratio` 设为目标绘制设备的设备像素比，则每个坐标的操作是

$$
x_{\text{snapped}} = \frac{\operatorname{round}(x_{\text{paint}} \cdot \rho)}{\rho},
\qquad \rho = \text{DPR}
$$

其中 $\operatorname{round}$ 是 `qwtRoundValueF()` 实现的「四舍五入、远离零」取整。输出类型仍是 `QPolygonF`：坐标保持浮点，只是被约束到格点 $\frac{1}{\rho}\mathbb{Z}$ 上。

**命题 1（设备像素格点）** 对任意 $\rho > 0$，$x_{\text{snapped}} \cdot \rho \in \mathbb{Z}$。无论 DPR 是多少，吸附后的坐标都正好落在设备像素边界上。

*证明*：按构造 $x_{\text{snapped}} = k/\rho$，其中 $k = \operatorname{round}(x_{\text{paint}}\rho) \in \mathbb{Z}$，故 $x_{\text{snapped}}\rho = k$。$\square$

$\rho = 1$ 时格点就是整数网格，是最熟悉的情形。$\rho = 2$ 时格点在逻辑单位下是半整数网格，在设备像素下仍是整数网格，因此顶点可以落在两个逻辑单位之间，绝不会落在两个设备像素之间。图 2 展示了这两种情况。

![DPR 1 与 DPR 2 下的设备像素格点](../../assets/algorithms/fig_roundpoints_dpr.png)

*图 2 RoundPoints 吸附到设备像素。DPR 为 1 时逻辑步长等于一个设备像素；DPR 为 2 时逻辑步长减半，吸附后的顶点可以位于半整数逻辑坐标，仍落在设备像素边界上。*

### 2.2 对折线的影响

考虑一条采样密度远高于像素网格的光滑曲线。映射后的顶点序列，相邻差值远小于一个像素。独立地吸附每个顶点有两个后果：

1. **顶点合并。** 连续多个采样落入同一格点单元，变成同一个点。折线因此丢失顶点，这正是 `WeedOutPoints` 依赖的机制。
2. **方向量化。** 幸存的相邻顶点之差是 $1/\rho$ 的整数倍。在缓坡上，真实曲线每列只上升不到一个像素，吸附后的折线只能在水平段（$y$ 不变）与垂直跳变（$x$ 不变）之间交替，结果就是阶梯。

图 3 单独展示了第二个后果：一条每列上升 0.2 像素行的合成直线。

![同一条直线的吸附与不吸附对比](../../assets/algorithms/fig_roundpoints_staircase.png)

*图 3 每列上升 0.2 像素行的直线，8 倍放大渲染。上：开启 RoundPoints，整数网格迫使每五列出现一个像素的台阶。下：关闭 RoundPoints，直线保持亚像素斜率，抗锯齿光栅器把每条边分摊到两行上。*

阶梯是光栅器对一条确实由水平段和垂直段组成的折线所做的正确光栅化。丢失的信息是每个顶点的亚像素偏移，下游任何阶段都无法恢复：抗锯齿的覆盖率计算只依赖它收到的几何。

**命题 2（不可恢复性）** 设折线 $L$ 的顶点为 $v_i \in \mathbb{R}^2$，$L'$ 为把所有顶点吸附到格点 $\frac{1}{\rho}\mathbb{Z}^2$ 后的折线。任何只依赖折线几何的光栅器都只能产生 $\mathcal{R}(L')$；仅当 $L$ 与 $L'$ 在被覆盖的像素列上一致时才有 $\mathcal{R}(L') = \mathcal{R}(L)$。

*证明*：光栅器收到 $L'$ 后无法访问 $L$，它是输入的纯函数。当差异低于覆盖率分辨率时，不同输入当然可能光栅化出相同结果；但缓坡上每个顶点最多 $1/(2\rho)$ 的差异会沿折线累积，超过该分辨率。$\square$

命题 2 的实用形式：一旦 `RoundPoints` 打开，再开启抗锯齿也无法恢复平滑的斜边。QWT 6 发布的正是这个组合，QWT 7 一直沿用到 v7.5.1。

### 2.3 吸附在哪些地方被打开

`QwtPlotCurve::drawLines()`（`src/plot/qwt_plot_curve.cpp`）读取两个独立开关：

```cpp
const bool doFit   = (d->attributes & Fitted) && d->curveFitter;
const bool doAlign = !doFit && QwtPainter::roundingAlignment(painter);
```

`QwtPainter::roundingAlignment(painter)` 是全局用户开关（`QwtPainter::setRoundingAlignment()`，默认 `true`）与 `QwtPainter::isAligning(painter)` 的合取。后者对 PDF、SVG、未知绘制引擎以及带旋转或缩放变换的画笔返回 `false`，不检查 `QPainter::Antialiasing`。因此启用抗锯齿的普通控件画布会走 §1.2 所示的对齐分支。

激进去重标志也在这个分支里设置。这一点关系到 §7.3：修复前的库中，只有 `RoundPoints` 打开时 `FilterPointsAggressive` 才起作用，因为映射器同样把 quad 路径守在 `RoundPoints` 分支内（`toPolygonF()`，`src/plot/qwt_point_mapper.cpp`）。

同一个全局开关还被 `drawDots()`、`drawSteps()`、`drawSticks()`、`drawSymbols()` 以及若干其他绘图项读取（`qwt_plot_intervalcurve.cpp`、`qwt_plot_boxchart.cpp`、`qwt_plot_marker.cpp`、`qwt_plot_shapeitem.cpp`、`qwt_plot_grid.cpp`），关闭它是应用级决定，不是单条曲线的决定。

## 3. 为什么密度起决定作用

阶梯的幅度受每个顶点半个设备像素的限制，可见度取决于每个像素列内有多少顶点，以及该处曲线的斜率。

### 3.1 每列采样数

设可见采样数为 $n$，画布宽度为 $W$ 个设备像素，平均占用率为 $\lambda = n / W$。由此得到三个区间：

- $\lambda \ll 1$：多数列没有采样。折线是从一个采样到下一个采样的一条长弦，列内没有顶点可吸附，画出的线是端点亚像素精确的直线插值。没有阶梯。
- $\lambda \approx 1$：每列一个顶点。吸附会在单元格内移动它，折线仍然每列下降一个顶点，线段保持斜向，阶梯轻微。
- $\lambda \gg 1$：每列多个顶点。吸附后的折线只能在列内垂直移动、在列间水平移动，该列变成一根竖直笔画加一段一像素的水平踏面。可见阶梯。

演示程序的密集谱线在放大窗口内 $\lambda \approx 3500/1300 \approx 2.7$ 个采样每列；稀疏谱线 $\lambda \approx 330/1300 \approx 0.25$。渲染结果的差异，就是第三个区间的起点与第一个区间之间的差异。第 4 节的映射器基准刻意采用极端密度（$\lambda \approx 833$），以便测出变换成本；几何机制在 $\lambda = 2.7$ 时完全相同，第 6 节的渲染混合比例实验可以印证。

### 3.2 斜率

阶梯需要一个方向去量化。在接近垂直的斜边上，吸附后的线段本来就大多是垂直的，踏面只有一像素宽；在接近水平的斜边上，踏面会横跨许多列，变得很显眼。这解释了为什么在峰的肩部（曲线平缓）最容易观察到该效应，在峰顶很难看出。

图 3 的合成渲染刻意采用 0.2 行/列的缓坡，得到五列宽的踏面。图 1 的真实谱线在上沿的局部斜率在 1 到 5 行/列之间，踏面只有一到两列宽，在正常放大倍率下仍然可见。

### 3.3 抗锯齿光栅器如何处理这两种情况

Qt 的光栅引擎根据像素中心到多边形边缘的距离计算每像素覆盖率。亚像素顶点偏移会以不到一个像素的量改变该距离，从而改变相邻两行的混合权重。吸附后的顶点把边缘正好放在像素边界上，一行的覆盖率为 1、另一行为 0，没有混合像素。混合比例因此是一个直接、单调的指标，反映还剩多少亚像素几何，这正是实验所测的内容。

## 4. 映射器中的开销

### 4.1 基准协议

`roundpoints_bench.cpp` 直接在 100 万采样的信号上驱动 `QwtPointMapper::toPolygonF()`，画布 1200 x 800，x 映射覆盖全部数据范围。七种标志配置覆盖所有降采样路径，每种分别在 DPR 1 与 DPR 2 下开关 `RoundPoints`。每个配置先校准到单次计时运行至少 20 ms，然后 3 次预热加 9 次计时，报告中位数。只对映射器计时（不含 `QPainter::drawPolyline()`），数据因此只反映坐标变换与降采样的工作量。

信号为两个长周期正弦，密度足以触发所有降采样路径（$\lambda \approx 833$ 采样每列），在画布尺度上保持光滑。

### 4.2 结果

![每次调用的映射器耗时，RoundPoints 关闭与开启对比](../../assets/algorithms/fig_roundpoints_perf.png)

*图 4 100 万采样、1200 x 800 画布、DPR 1 下 `QwtPointMapper::toPolygonF()` 的单次调用耗时，开关 `RoundPoints` 对比。*

| 路径 | 关闭 (ms) | 开启 (ms) | 变化 | 顶点数 关 → 开 |
|---|---:|---:|---:|---:|
| 不降采样 | 14.21 | 19.22 | **+35%** | 1,000,000 → 1,000,000 |
| `WeedOutPoints` | 16.65 | 25.71 | **+54%** | 1,000,000 → 3,898 |
| `WeedOutIntermediatePoints` | 16.29 | 8.42 | **−48%** | 1,000,000 → 1,986 |
| `PixelColumnReduce` | 9.61 | 9.51 | −1% | 2,402 → 2,402 |
| `LTTBReduce` | 9.01 | 8.74 | −3% | 4,804 → 3,166 |
| `MinMaxReduce` | 3.50 | 3.68 | +5% | 4,804 → 4,465 |
| `AdaptiveColumnReduce` | 13.37 | 13.41 | +0.3% | 4,181 → 3,749 |

表中为 9 次计时运行的中位数；按最小值计算的比值略大（不降采样 +41%，`WeedOutPoints` +65%），因为标志的额外工作在最快的几次运行中更容易显现。

第一行是该标志的纯成本：每个坐标多一次乘法、取整和除法，100 万个顶点，输出规模一点没减。对密集未降采样曲线，这是映射器上 35% 到 41% 的额外开销；促成这份报告的最初测量使用了不同的计时协议，当时测得 +51%。

`WeedOutPoints` 一行是标志在做它设计上的工作：100 万个顶点坍缩为 3,898 个，吸附后共享同一设备像素的采样就是重复点。映射器本身更慢（它仍要触碰每个输入），下游绘制成本按顶点数下降两个数量级。这是该标志设计时的取舍。

`WeedOutIntermediatePoints` 一行必须谨慎解读：修复前的库中，关闭 `RoundPoints` 时 aggressive 路径根本不会进入（`toPolygonF()` 只在 `RoundPoints` 分支内检查它），两列比较的是两个不同算法，不是同一算法开关取整。§7.3 移除了这层耦合。

四条现代降采样路径（像素列、LTTB、MinMax、Adaptive）变化在 −3% 到 +5% 之间，处于或仅略高于运行间噪声。它们只选出几千个顶点并对这些顶点取整，对 100 万输入采样的扫描占主导。

![返回的顶点数，关闭与开启对比](../../assets/algorithms/fig_roundpoints_output.png)

*图 5 映射器返回的顶点数。注意纵轴是对数轴：不取整时，未降采样与去重路径每个采样返回一个顶点。*

### 4.3 DPR 维度

DPR 为 2 时，格点在逻辑单位下更细，在设备像素下相同，吸附后幸存的顶点更多（`WeedOutPoints` 为 7,783 而非 3,898），时间代价不变。没有出现新效应，标志只是作用在更细的网格上。DPR 感知本身就是一次修复（提交 `9ce35af`，「make QwtPointMapper pixel units honor the device pixel ratio」），在此之前像素列类算法使用逻辑单位，在高分屏上行为不同。

### 4.4 随采样数的伸缩

| 采样数 | 关闭 (ms) | 开启 (ms) | 比值 |
|---:|---:|---:|---:|
| 100,000 | 1.31 | 1.90 | 1.45 |
| 250,000 | 3.31 | 4.74 | 1.43 |
| 500,000 | 6.56 | 9.93 | 1.51 |
| 1,000,000 | 13.62 | 19.59 | 1.44 |
| 2,000,000 | 27.46 | 39.52 | 1.44 |

比值基本持平，说明是每顶点常数成本，不是缓存或分配效应。两条曲线都随 $n$ 线性增长，符合单遍扫描的预期。

## 5. 几何特征

不渲染也能检查映射器输出：统计返回折线中有多少线段严格水平（`dy == 0.0`）或严格垂直（`dx == 0.0`）。不吸附时，映射到绘制坐标的光滑曲线没有严格轴对齐的线段，浮点值几乎不可能恰好相等。吸附后（DPR 1 下）坐标是整数，线段按构造就是轴对齐的。

![轴对齐线段比例](../../assets/algorithms/fig_roundpoints_axis.png)

*图 6 严格水平或垂直的映射线段比例，100 万采样，光滑信号。*

| 路径 | 关闭 | 开启 |
|---|---:|---:|
| 不降采样 | 0.0% | **99.9%** |
| `WeedOutPoints` | 0.0% | **99.9%** |
| `WeedOutIntermediatePoints` | 0.0% | **99.9%** |
| `PixelColumnReduce` | 39.6% | 39.6% |
| `LTTBReduce` | 0.0% | **79.8%** |
| `MinMaxReduce` | 0.0% | **57.2%** |
| `AdaptiveColumnReduce` | 0.0% | **96.1%** |

这里有两个结果。第一，未降采样与去重路径几乎完全变成轴对齐，折线在几何意义上已不再是曲线，而是水平与垂直步进的序列。第二，`PixelColumnReduce` 不受该标志影响：它按列输出 min/max 包络，输出本身就包含水平连接，这解释了为什么像素列算法即使关闭取整也会出现自己的阶梯。

降采样算法处于中间，因为它们保留少量真实采样：LTTB 每桶选一个，MinMax 选两个，Adaptive 选容差筛选后的子集。当这少数顶点被吸附时，连接它们的大多数线段变成轴对齐，顶点本身仍沿曲线分布，结果是更粗的阶梯而非完整的阶梯。

## 6. 视觉实验

### 6.1 设置

`examples/2D/downsample_demo --batch <dir>` 把数据集 x 降采样 x 视图的矩阵渲染为 PNG。用同一份二进制生成两批：一批使用修复前的库默认设置（`roundingAlignment` 开启），一批加 `--noround`。分析裁剪区为 `win14` 视图中最高峰的上升肩部，120 x 360 像素，该处斜边平缓、阶梯最明显。

脚本对每个裁剪区统计曲线像素（蓝色主导的像素），将其分为纯笔色与混合色，报告混合比例；同时记录最上方曲线像素行（峰顶），它不应移动。

### 6.2 结果

![峰肩的混合像素比例](../../assets/algorithms/fig_roundpoints_metrics.png)

*图 7 峰肩的抗锯齿混合像素比例，密集与稀疏采样、对齐开启与关闭对比。每一根柱子都启用了抗锯齿。*

| 数据集 | 降采样 | 关闭对齐 | 开启对齐 | 峰顶行 |
|---|---|---:|---:|---:|
| 密集（20 万） | 不降采样 | 0.547 | **0.284** | 0 |
| 密集（20 万） | `FilterPointsAggressive` | 0.547 | **0.287** | 0 |
| 密集（20 万） | `FilterPointsPixel` | 0.511 | 0.511 | 0 |
| 密集（20 万） | `FilterPointsLTTB` | 0.544 | **0.288** | 0 |
| 密集（20 万） | `FilterPointsAdaptive` | 0.552 | **0.289** | 0 |
| 稀疏（1.86 万） | 不降采样 | 0.568 | 0.564 | 0 |
| 稀疏（1.86 万） | `FilterPointsLTTB` | 0.568 | 0.564 | 0 |
| 稀疏（1.86 万） | `FilterPointsAdaptive` | 0.568 | 0.565 | 0 |

对比结果如下：

1. 开启吸附时密集数据损失一半混合像素。0.28 与 0.55 的比值就是图 1 视觉差异的定量形式。绝对值取决于裁剪区与笔宽，两倍的差距才是信号。
2. 稀疏数据不受影响。两种设置下比例都是 0.56，因为采样间距足够大，吸附对每个顶点的移动小于抗锯齿分辨率，也从不产生多顶点列。
3. `FilterPointsPixel` 不受全局开关影响，与几何分析的预测一致：该算法自行计算像素域包络，忽略 `RoundPoints`。它的 0.51 低于亚像素路径，因为其输出本身包含水平连接。

峰顶行在所有配置下相同，吸附改变斜边形状，不改变峰的位置。

### 6.3 失效模式与局限

- 混合比例指标依赖笔宽、裁剪区与渲染后端。这里只用它做同一配置两次渲染之间的相对比较，不作为绝对质量分。
- `win14` 裁剪区之所以被选中，是因为该处阶梯可见；其他窗口与全范围视图的差异更小。机制在这些视图中依然存在，强度取决于每列采样数与局部斜率。
- 图 3 的合成直线是展示机制的最佳情形（斜率 0.2）。真实谱线斜边更陡，踏面相应更窄。
- 测量只覆盖 DPR 1 与 DPR 2 的光栅渲染。PDF 与 SVG 输出不受影响，因为 `QwtPainter::isAligning()` 已排除这些引擎；该结论来自源码分析。

## 7. QWT 7 的修复

QWT 7 从两端同时处理这个问题。7.x 系列给映射器加入保留亚像素坐标的降采样算法，密集数据可以降采样而不被量化；v7.5.1 之后的对齐策略则阻止最后一步把这些坐标吸附掉。

### 7.1 选型规则

证据支持一条简单规则，以下决策表覆盖真正相关的两个开关。

| 场景 | `RoundPoints` | 理由 |
|---|---|---|
| 抗锯齿光栅（控件画布），任意降采样 | **关闭** | 亚像素几何正是光栅器需要的输入 |
| 非抗锯齿光栅、细线、强调像素精确 | 开启 | 顶点落在真实像素上，边缘保持锐利 |
| 用 `WeedOutPoints` 做像素去重 | 开启 | 该算法的目的就是按设备像素合并采样 |
| `WeedOutIntermediatePoints`（aggressive） | 开启 | quad 路径内部按设备像素取整 |
| `PixelColumnReduce` | 无关 | 算法不读取该标志 |
| PDF / SVG / 旋转或缩放画笔 | 关闭 | 已被 `QwtPainter::isAligning()` 排除 |
| LTTB / MinMax / Adaptive 之后，为性能 | **关闭** | 无可测收益，密集数据上有可见损失 |

库层面的实现是：

```cpp
const bool doAlign = !doFit
    && !(painter->renderHints() & QPainter::Antialiasing)
    && QwtPainter::roundingAlignment(painter);
```

非抗锯齿光栅绘制保留原有行为，PDF/SVG 与带变换的画笔继续走亚像素路径，会带来损害的场合不再开启该标志。

### 7.2 QWT 7 新增的降采样算法

管线的降采样一侧在 7.x 系列中逐步扩充：

| 版本 | 属性 | 映射器路径 | 保留内容 |
|---|---|---|---|
| v7.3.1 | `FilterPointsPixel` | `PixelColumnReduce` | 每个设备像素列的 min/max 包络 |
| v7.3.1 | `FilterPointsLTTB` | 桶降采样（后成为 `MinMaxReduce`） | 每桶的 min/max 采样 |
| v7.5.0 | `FilterPointsLTTB` | `LTTBReduce` | 每桶按三角形面积选出的一个采样（真 LTTB） |
| v7.5.0 | `FilterPointsMinMaxLTTB` | `MinMaxReduce` | 原先的桶实现 |
| v7.5.1 | `FilterPointsAdaptive` | `AdaptiveColumnReduce` | 密度门限下的簇内采样点，保留真实 y 坐标 |

这些路径都遵循 `RoundPoints`：标志关闭时返回选中采样的原始亚像素坐标，光栅器可以按真实斜率计算覆盖率。`PixelColumnReduce` 按设计例外，它的输出是包络而非采样。v7.5.1 还把各降采样路径的像素预算从逻辑像素改为设备像素（`setDevicePixelRatio()`，提交 `9ce35af`），任意 DPR 下对应的设备像素数保持不变。

输出规模让密集数据画得起：演示程序的 120 万采样数据集在任一现代降采样下渲染耗时 4 到 8 ms，不降采样为 44 ms。降采样发生在映射器输出几何之前，这些算法都不对幸存的顶点取整。

### 7.3 抗锯齿感知的对齐策略

最后的改动（提交 `7294c68`，v7.5.1 之后）把 §7.1 的规则应用到曲线路径：

- `drawLines()` 与 `drawDots()` 只在画笔不启用抗锯齿时打开 `RoundPoints`。非抗锯齿光栅输出保留原有的吸附行为，`roundingAlignment(painter)` 仍排除 PDF、SVG 与带变换的画笔。
- `legendIcon()` 仅在图标以非抗锯齿方式录制时把水平线吸附到像素格点，非抗锯齿图例中的一像素线保持锐利。
- `WeedOutIntermediatePoints` 不再依赖 `RoundPoints`。`toPolygonF()` 中的 quad 去重路径内部按设备像素取整，现在排在 `RoundPoints` 分支之前，`FilterPointsAggressive` 在抗锯齿画布上仍然可达。整数版 `toPolygon()` 原本就把两个标志独立处理。

没有改动的部分：非抗锯齿光栅绘制仍然吸附，PDF/SVG 与带变换的画笔本来就被排除，曲线管线之外的绘图项（网格、marker、箱线图）仍读取全局开关。后者属于另一个范围，本次修复针对的是曲线自身几何被量化的路径。

### 7.4 实测效果

修复后的构建用同一套批处理管线、库默认设置（不带 `--noround`），得到的是关闭对齐的结果：

| 指标 | 修复前默认 | 修复后默认 | `--noround` |
|---|---:|---:|---:|
| 密集峰肩混合比例（`many`，不降采样） | 0.284 | **0.5470** | 0.5470 |
| 稀疏峰肩混合比例（`few`，不降采样） | 0.564 | 0.5682 | 0.5682 |
| `FilterPointsAggressive` 顶点数，密集 win14 | 7,384 | **7,384** | 7,384 |

修复后默认批次与 `--noround` 批次的曲线像素完全一致，aggressive 的顶点数也没有变化：抗锯齿画笔上的 `RoundPoints` 关闭后，解耦让 quad 路径仍然可达。不同的像素只有网格线，后者仍响应全局开关。图 1 中两块平滑面板不再需要应用层开关或逐曲线配置。

§4 的映射器开销表仍适用于非抗锯齿路径，那里继续开启吸附。抗锯齿路径上吸附遍历消失，顶点数也不再因附带去重而减少，降采样属性成为控制成本的工具。配套降采样报告假定的正是这种分工。

## 8. 复现

以下结果都可由仓库重新生成：

```bash
# 1. 构建库与演示程序
./build.ps1 build
cmake --build build --config Release --target downsample_demo

# 2. 渲染两批图像
build/bin/Release/downsample_demo --batch out_round
build/bin/Release/downsample_demo --batch out_noround --noround

# 3. 编译并运行映射器基准
cd docs/assets/algorithms
cmd //c compile_roundpoints_bench.bat          # 批处理内含 MSVC 与 Qt 路径
roundpoints_bench.exe all > roundpoints_results.csv

# 4. 编译并运行合成渲染检查
cmd //c compile_roundpoints_render.bat
roundpoints_render_check.exe out_render -platform offscreen

# 5. 图像指标与绘图
python roundpoints_render_metrics.py out_round 1
python roundpoints_render_metrics.py out_noround 0 --append
python roundpoints_experiment.py
```

两个 `.bat` 辅助脚本根据自身位置推导仓库根目录，通过环境变量 `QT_DIR`（Qt 安装目录）与 `QWT_BUILD_DIR`（构建目录）定位依赖，默认值分别是本机的 Qt 6.7.3 路径与 `<repo>/build`；换机器时调整这两项即可。`roundpoints_bench.cpp` 与 `roundpoints_render_check.cpp` 的文件头注释中也给出了等价的 `cl` 命令行。

第 4 到 6 节的渲染测量基于修复前的库。当前构建不会对抗锯齿画笔做吸附，`out_round` 与 `out_noround` 现在包含相同的曲线像素，复现修复前的图像需要 v7.5.1 或更早版本的构建。仓库内提交的 CSV 与由它们生成的图不依赖重新渲染，仍可复现。批处理现在渲染三个数据集（`many`、`few` 与 120 万采样的 `million`），指标脚本分析其中的 `many` 与 `few`。

## 9. 结论

QWT 6 从 Qwt 6.2 继承的吸附策略，是为整数光栅、无抗锯齿输出设计的：默认打开的全局开关、忽略抗锯齿的引擎判断、完全定义在吸附坐标上的降采样工具箱。这三者在抗锯齿画布上叠加，密集折线在光栅器看到它之前就被量化，结果就是图 1 的阶梯。

QWT 7 同时修补了两端。7.x 系列加入只挑选采样、不取整的降采样算法，v7.5.1 之后的策略只在画笔不启用抗锯齿时开启像素吸附。密集峰肩现在与关闭对齐的构建得到相同混合比例（0.5470 对 0.284），稀疏数据不变，激进去重路径在抗锯齿下仍把 199,751 个顶点压到 7,384 个。吸附的每顶点开销（未降采样百万点曲线上约占映射器耗时的 45%）现在只花在能带来好处的地方：非抗锯齿输出上的锐利细线。

## 10. 参考文献

1. QWT 7.5.0 源码：`src/plot/qwt_point_mapper.cpp`（`QwtRoundF`、`qwtAdaptiveColumnReduce`、`qwtLttbReduce`、`qwtMinMaxBucketReduce`、`qwtPixelColumnReduce`、`toPolygonF()` 分派），`src/plot/qwt_plot_curve.cpp`（`drawLines()`），`src/plot/qwt_painter.cpp`（`isAligning()`、`roundingAlignment()`）。
2. QWT 7.5.1 发行说明：`CHANGES.md` 中 `FilterPointsAdaptive`、设备像素修复（提交 `9ce35af`）与 downsample demo 的条目。
3. 抗锯齿感知对齐修复（v7.5.1 之后）：提交 `7294c68`，涉及 `QwtPlotCurve::drawLines()`/`drawDots()`/`legendIcon()` 与 `QwtPointMapper::toPolygonF()`。
4. 配套报告：[《面向屏幕的曲线降采样：LTTB 理论与 QWT7 的 MinMax 桶》](curve-downsampling.md)。
5. 配套报告：[《曲线渲染平滑：高斯卷积、Savitzky-Golay 回归与特征保护》](curve-smoothing.md)。
6. 演示程序：`examples/2D/downsample_demo`（带渲染耗时读数的交互界面，外加 `--batch`、`--noround`、`--nosplit`、`--dots` 模式）。
7. 原始数据与脚本：`docs/assets/algorithms/roundpoints_bench.cpp`、`roundpoints_render_check.cpp`、`roundpoints_render_metrics.py`、`roundpoints_experiment.py`、`roundpoints_results.csv`、`roundpoints_render_metrics.csv`。
8. Steinarsson, S. *Downsampling Time Series for Visual Representation.* 冰岛大学硕士论文，2013。
9. Qt 文档：`QPainter::Antialiasing`、`QPaintEngine::Type`。
