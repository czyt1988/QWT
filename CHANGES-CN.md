## tag:v7.5.1 (2026-10-19)

### 新功能

- **自适应列降采样（`QwtPlotCurve::FilterPointsAdaptive`）**
    - 新增 `QwtPointMapper::AdaptiveColumnReduce` 模式：面向过采样平滑曲线（例如每个峰有大量采样点的光谱）的单遍 O(n) 降采样，所有像素量均按设备像素计算
    - 密度门限：每个设备像素列不超过 2 个可见样本时按 1:1 映射，放大后总能恢复完整精度
    - 超过门限时按设备像素列聚类；簇内保留首样本、每个与上一个保留样本的映射 y 相差至少 1 设备像素的样本以及末样本——保持原始顺序与真实亚像素坐标，斜坡仍是斜坡，不会出现逐列 min/max 包络那种 1 像素阶梯
    - 若某列的保留点数将超过 `4 + 该列 y 跨度（设备像素）`（仅高频噪声会触发），退化为有序的 first/min/max/last 四元组。与原曲线的偏差垂直方向不超过 1 设备像素、水平方向不超过 1 列
    - 优先级高于所有其他降采样标志；默认绘制属性仍为 `ClipPolygons | FilterPointsLTTB`
- **新增示例 `examples/2D/downsample_demo`**
    - 渲染密集与稀疏两条合成光谱曲线，运行时切换降采样算法：none、`FilterPoints`、`FilterPointsAggressive`、`FilterPointsPixel`、`FilterPointsLTTB`、`FilterPointsMinMaxLTTB`、`FilterPointsAdaptive`
    - `--batch [dir]` 将每种数据集/算法/视图组合离屏渲染为 PNG 并报告映射点数，`--dots` 标出每个保留顶点，`--noround`/`--nosplit` 切换 `QwtPainter::roundingAlignment()`/`polylineSplitting()`，`--dpr` 按指定设备像素比渲染——像素吸附测量的对照工具

### Bug 修复

- 修复 `QwtPointMapper` 的像素量按逻辑像素而非设备像素计算的问题：LTTB/MinMax 降采样的桶数、像素列传的列数以及 `RoundPoints` 的量化步长都忽略了 `devicePixelRatio()`，在 HiDPI 绘制设备上降采样丢失设备分辨率、取整又把折线吸附到 `devicePixelRatio()` 个设备像素的台阶（陡峭曲线段表现为阶梯）。新增 `setDevicePixelRatio()`/`devicePixelRatio()`；`QwtPlotCurve::drawLines()`、`drawDots()`、`drawSymbols()` 传入绘制设备的 `QwtPainter::devicePixelRatio()`，映射坐标仍保持绘制坐标。比值为 1.0 时行为不变（实测逐字节相同的渲染结果）

### 构建

- 修复 MSVC 下 `examples/staticExample` 因“number of sections exceeded object file format limit”构建失败的问题：该目标增加 `/bigobj`
- 修复 Gitee release 同步工作流：release 事件以 detached HEAD 检出 tag，本地不存在 `master`/`dev`，分支推送报 `src refspec master does not match any`，Gitee release 与合成文件附件都没有创建。现在先物化分支引用，支持带 tag 的手动运行（`workflow_dispatch`），并复用已存在的同 tag release 而不是在创建时失败

### 文档

- **新增技术报告《曲线管线中的像素吸附》（`docs/tech-report/point-mapper-rounding.md`，中英）**：形式化 `QwtPointMapper::RoundPoints`，实测吸附会把平滑密集折线的 99.9 % 变成严格的水平/垂直线段（关闭时为 0 %），并量化渲染图像中的抗锯齿混合比例（密集峰肩 0.547 → 0.284，稀疏峰保持 0.568），同时给出 100 万采样点下各降采样路径的开销
- **新增使用指南《曲线像素对齐》（`docs/use-guide/2d/downsampling-pixel-align.md`，中英）**：症状、背后的全局开关、屏幕/导出/像素精确渲染各自的一行设置，以及用 `downsample_demo` 复现对比的操作；渲染性能指南已加入链接

## tag:v7.5.0 (2026-10-14)

### 新功能

- **曲线真 LTTB 降采样（FilterPointsLTTB 重做）**
    - `FilterPointsLTTB` 现在实现真正的 Largest-Triangle-Three-Buckets 算法（新增 `QwtPointMapper::LTTBReduce` 标志、`qwtLttbReduce()`）：可见数据被分成 4 × 画布宽度个等量桶，每桶保留与「上一个已选点和下一桶均值」构成最大三角形面积的样本；首尾样本永远保留
    - 窄高斯峰保持形状、不再塌缩成竖线；激活阈值（可见采样 > 4 × 画布宽度）与输出预算（约每像素列 4 个点）与原 MinMax 桶算法一致
    - MinMax 桶算法移至新增的 `FilterPointsMinMaxLTTB` 绘制属性 —— 它就是 v7.3.2 - v7.4.2 中 `FilterPointsLTTB` 的原有行为
    - 分发优先级：`PixelColumnReduce` > `LTTBReduce` > `MinMaxReduce` > `WeedOutIntermediatePoints` > `WeedOutPoints`；默认值仍为 `ClipPolygons | FilterPointsLTTB`（现在是保形的真 LTTB）
    - 回归测试：`tests/downsampling_test`；`examples/bench/renderbench` 新增 `FilterPointsMinMaxLTTB` 基准模式
- **3D 瀑布图 item（Qwt3DWaterfall）**
    - 新增 `Qwt3DPlotItem`：把一维轨迹序列（频谱、色谱……）沿深度轴堆叠绘制，内置滑动窗口——`pushTrace()` 追加最新轨迹并丢弃最旧轨迹，用户无需自己管理窗口或深度编号
    - 坐标轴遵循模块约定（数值轴朝上，与 `Qwt3DSurface` 一致）：x = bin 值、y = 轨迹在窗口中的位置（深度）、z = 幅度；`Qwt3DTextScale` 放在深度轴上
    - 轨迹的深度坐标即槽位索引，hull 的 z 范围单调增长到 `[0, traceCapacity()-1]` 后保持稳定 —— 实时瀑布不会出现逐帧深度轴自动缩放抖动
    - 四种几何样式：`Ribbon`（填充帘幕，默认）、`RibbonMesh`（帘幕 + 描边，描边固定在填充之后绘制）、`Wireframe`、`Tube`（带光照，与 `Qwt3DLine::Tube` 同一构造方式）
    - 三种上色模式：`ByDepth`（默认）、`ByAmplitude`（按采样值上色）、`Solid`；`setColorRange()` 固定色域让实时幅度颜色稳定；基线、不透明度与描边可调；一个 item 只向图例贡献一条渐变
    - 新增示例 `examples/3D/waterfall3D`：上图 2D 实时频谱 + 峰值保持，下图 3D 滑动窗口瀑布，两图喂同一帧；工具栏可切换窗口大小/样式/配色/上色模式/不透明度；自动化参数 `--screenshot`、`--selftest`、`--fps`（64 条 × 256 bin、双视图同时刷新实测 51.8 fps）
    - 新增使用指南《3D瀑布图》（中英双语，含截图）；RTTI 值 `Rtti_Plot3DWaterfall = 1004`；`Qwt3DTheme::applyToItem()` 新增分支；classincludes 转发头
- **Qwt3DTextScale：深度轴用户自定义刻度文字**
    - 主刻度标签来自字符串列表的线性 scale，瀑布深度轴可以显示 "-63"、"-48"、……、"0"（距当前帧数）而不是原始数字；越界下标回退为数字标签
- **Qwt3DColorMapColor：可选取色坐标轴**
    - 新增 `Qwt3DColorSource` 枚举（`ColorFromX`/`ColorFromY`/`ColorFromZ`）与 `setSourceAxis()`/`sourceAxis()`；活动范围作用于所选轴。默认 `ColorFromZ` 保持既有 surface/bar/line 渲染逐像素不变；`ColorFromY` 支撑幅度上色。`Qwt3DWaterfall::setColorMode()` 会自动切换 functor 的取色轴
- **Qwt3DPlot：可配置的鼠标视图交互**
    - `setMouseSpeed()`/`mouseSpeed()` 设置拖动速度（每屏旋转圈数、拖动缩放的指数增益、拖动平移的世界单位）；默认值为 (0.5, 5, 2)，即每屏半圈，与 matplotlib 的 3D 旋转灵敏度一致
    - `setRollMouse()`/`rollMouse()` 为 Y 轴旋转（翻滚，普通左键拖动唯一到不了的角度）增加一个手势；默认绑在鼠标中键上，Shift + 左键的原有绑定继续可用
- **曲线渲染的缺口语义（`QwtPlotCurve::BreakAtNan`）**
    - 新增曲线属性：把 NaN/Inf 样本渲染为缺口而非桥接——`Lines` 与 `Steps` 风格下，每个连续有限样本段渲染为独立折线，缺口保持空白（matplotlib 行为）。默认关闭；不开启时缺口照旧以桥接弦跨越
    - 每段独立平滑、拟合、裁剪与填充——平滑核与填充区域都不跨缺口，每段填充独立闭合到基线；降采样预算按段分配。`Dots`/`Sticks` 不受影响（逐样本风格本来就缺口空白）
    - 有限段扫描在每次数据变更时只做一次并缓存（经 `dataChanged()` 失效，与序列 boundingRect 缓存同生命周期），交互式重绘不重复支付 O(n) 扫描
    - 《NaN 数据处理》使用指南（中英）已补充说明；渲染语义与桥接弦的定量测量见 `docs/zh/tech-report/nan-rendering.md`

### Bug 修复

- 修复降采样破坏窄峰视觉形状且无视抗锯齿的问题：默认的 `FilterPointsLTTB`（MinMax 桶，v7.3.2 - v7.4.2）每桶只保留 y 最小/最大的样本，采样点数超过桶容量的峰塌缩成平顶竖线、峰肩全部丢失；且所有降采样路径无视 `RoundPoints` 标志、把映射坐标强制取整到整数像素 —— 开不开抗锯齿画面毫无差别。现在两个降采样算法都遵循 `RoundPoints`（画布不按整数对齐时保留亚像素精度，含低于阈值的回退路径），默认降采样改为保峰形的真 LTTB。回归测试：`tests/downsampling_test`
- 修复单文件合成构建（`src-amalgamate`，`examples/staticExample` 使用）自 3D 瀑布图合入后无法链接的问题：amalgamate 模板从未列入 `qwt3d_waterfall.h/.cpp`，导致 `Qwt3DTheme::applyToItem()` 引用的 `Qwt3DWaterfall` setter 在合成文件内没有定义。已更新模板并重新生成 `src-amalgamate`；同时把 `qwt3d_waterfall.cpp` 中重复的文件级 `clampf()` 辅助函数改名为 `clampFloat()`，使单一翻译单元可以通过编译。另移除 `tests/flat_style_test` 中仅 Qt5 可用的 `QwtPlotGLCanvas` 包含——它在 Qt6 下因不存在 `<qgl.h>` 而编译失败
- 修复坐标轴主线在画布四角不相交、以及 HiDPI 缩放下轴线/刻度线宽度不一致的问题：`QwtScaleDraw` 现在把主线与刻度绘制为设备像素对齐的填充矩形，且笔宽按设备像素解释——四舍五入为整数设备像素、不再乘以设备像素比——`setPenWidthF(1.0)` 在任意缩放比例下都是 1 设备像素的细线（0.0 同样映射为 1），刻度长度仍按逻辑像素计算；刻度恰好止于主线内缘而不再与主线交叉；位于刻度范围端点的刻度吸附到垂直轴的主线上，角上不再残留端点刻度与垂直轴线之间 1 像素的台阶。`QwtScaleDraw::setCornerMiter()` 标志——由 `QwtScaleWidget` 按垂直轴的可见性设置——把每条主线向共享的画布角延伸，延伸终点是垂直轴主线的真实位置（`setCornerTargets()`，由实际轴几何推导，含堆叠在宿主带外侧的寄生轴），不再是固定的猜测值；`QwtPlot::doLayout()` 在轴 widget 内预留角点像素，延伸永不被裁剪——即使最外侧主刻度远离刻度端点、边界距离提示塌缩为 0 也不例外——`QwtPlotRenderer` 也按自身布局设置同样的状态，导出结果确定且与屏幕一致。回归测试：`tests/axis_corner_test`（在设备像素比 1 和 2 下运行）
- 修复 `Qwt3DPlot::attach()` 不重算场景的问题：若数据在 `attach()` 之前装载、且中间没有调用任何 setter（因此未触发 `itemChanged()`），plot 的 hull 停留在 (0,0,0)-(0,0,0)，画面全白且无任何警告。z 排序 + hull 并集 + 坐标系更新被抽取为私有 `recalculateScene()`，`attach()` 与 `detach()` 现在也会调用，detach 后 hull 同步收缩。已加回归测试（`tests/waterfall_test`）
- 修复派生 `Qwt3DScale` 在 `Qwt3DAxis::setScale()` 内被切片的问题：`Qwt3DClonePtr` 赋值经 `clone()` 克隆，未 override `clone()` 的派生 scale 的自定义 `ticLabel()` 静默失效（实测逐像素零差异）。`Qwt3DTextScale` 已 override；`Qwt3DLinearScale` 新增 protected `copyLinearScaleState()` 供派生类复制 autoscaler 状态
- 修复 `Qwt3DWaterfall::pushTrace()` 对短轨迹的越界读取：入帧按已建立的 bin 宽度装填（超长截断、过短零填充）
- 修复 3D 轴标题在实时绘图上不显示的问题：`Qwt3DCoordinateSystem::destroy()`（`init()` 每次重建坐标系时调用）会清空所有轴的标签字符串，且标题绘制默认关闭、必须另行调用 `setLabel(true)`。现在 `setLabelString()` 设置非空标题即启用绘制，`init()`/`destroy()` 不再清除标题；新增 `Qwt3DAxis::label()` 暴露绘制开关
- 修复 `Qwt3DCoordinateSystem::init()` 每次重建时把所有轴的 scale 重置为 `LINEARSCALE`：对 hull 逐帧变化的 item（实时瀑布），用户设置的 `Qwt3DTextScale` 会被静默丢弃、深度轴回退为数字标签。现在 `init()` 只重建几何，并新增 `Qwt3DAxis::scale()` 暴露当前 scale
- 修复 `Qwt3DWaterfall::Tube` 融合成一整块并拖垮帧率的问题：扫掠半径原先在数据空间各向同性，在各向异性视图缩放下相邻轨迹互相重叠、截面被拉成板状，且每帧重建每条轨迹的全部环站。现在半径按各轴 hull 范围取比例（视图空间正圆、上限为半个槽位间距），密集轨迹降采样到 ≤129 个环站（128 条 × 256 bin 实测 17 → 62 fps）
- 修复 3D 视图旋转过快且被取整成整度的问题：鼠标/键盘处理函数保存的是 `int(round(angle)) % 360`，慢速拖动的小数角度被永久丢弃（2000 px 宽的控件上 1 px 只有 0.09°，画面完全不动），角度还会跑出规范区间；拖满整个控件的增益高达 3 整圈（实测 200 px → 220°）。现在角度以 double 保存并折入 [0, 360)，默认增益改为每屏半圈（同一拖动实测 36°）。回归测试：`tests/plot3d_rotation_test`
- 修复 MinMax 桶降采样（`FilterPointsMinMaxLTTB`，即 v7.5.0 重做前的 `FilterPointsLTTB`）对含非有限坐标的数据产出伪顶点的问题：原始指针快速路径的预扫描只用 `isnan` 检查 y 值，x 为 NaN 的样本可能胜出桶极值并经 `qRound(NaN) == 0` 映射为伪顶点（钉在画布左缘的伪尖峰）；±Inf 的 y 值也能穿过预扫描进入 SIMD 路径，产出 `INT_MIN` 像素坐标。现在预扫描以 `!isfinite` 拦截非有限 y 值；SIMD 路径对每桶胜出样本的 x 做后置校验，命中桶改用标量循环重算；标量循环跳过 x/y 非有限的样本——原始指针路径与通用 `sample()` 路径语义对齐。仅 y 数据的索引系列（无原始 x 指针）不受影响。预扫描刻意保持只查 y：实测同时扫描 x 会使 1M 干净点的帧时间回归 +11%（额外内存流量），最终方案实测回归 +1.7%（噪声带内）。回归测试：`tests/nan_curve_test`
- 修复 `qwtToPolylineFiltered`（`FilterPoints`/`WeedOutPoints` 映射路径）对全 NaN 输入返回含 NaN 坐标的单顶点多边形的问题：首非 NaN 扫描停在最后一个样本并无条件将其写入。现在全 NaN 区间返回空多边形（与 Quad 路径的早退一致），调用方"空多边形 = 无数据"的约定恢复。回归测试：`tests/nan_curve_test`

- 修复 `QwtBoxStatisticsCalculator` 的 Percentile 须线水平互换：`whiskerLower` 取第 (100−c) 百分位、`whiskerUpper` 取第 c 百分位，系数 <50 时 wl > wu、样本无效且全部数据被计为离群点（技术报告回归用例 64/64）。现在水平跟随系数（c 为尾部百分比，如 5 表示第 5/95 百分位），`QwtBoxSample` 新增 `mean` 字段并由统计器填入，`QwtPlotBoxChart` 的均值标记画在该字段上（原先重复绘制中位数），同时移除无任何消费方的实例接口（`setWhiskerMethod()`/`setWhiskerCoefficient()`）。新增 `tests/box_statistics_test`
- 修复 `QwtClipper::clipCircle` 在竖直切线上把上方点映到 `3*pi/2`（与 `qwtPolar2Pos` 及自身非竖直分支相反），圆心恰在矩形竖直边线上时多返回 π；扫描同时按小角度容差合并重合切割点（圆过矩形角点时相邻两边各报一次、坐标差几个 ulp），并对每个角度区段独立判定，消除“整圆 vs 零长弧”的 1 ulp 依赖与单位切割点下 `angles[1]` 的越界读。新增 `tests/clipper_test`（定向用例 + 暴力对照）
- 修复 gl2ps 矢量导出的曲面颜色未定义问题：feedback 缓冲记录的是固定功能顶点色，而 GLSL 渲染路径从不写入（原始测量机上填充全部 alpha=0、曲面不可见，其他机器上是与数据无关的颜色）。`Qwt3DSurface`、坐标系网格/轴线与颜色图例在反馈回放时改用即时模式显式提交逐顶点颜色，`Qwt3DPlot::isVectorExportActive()` 由 GL 渲染模式判定，PDF/SVG 导出因此携带数据颜色。新增 `tests/plot3d_vector_export_test` 与 `surface_export_check.cpp` 探针
- 修复 Qwt3DIO 格式注册表大小写敏感：光栅格式键是 `QImageWriter` 的小写名，`savePixmap(file, "PNG")` 静默失败（官方 3D 示例即踩中）。查表、注册与 `saveVector`/`savePixmap` 的格式判定改为大小写不敏感，未注册格式给出 qWarning 警告，示例按文件后缀推导格式。新增 `tests/plot3d_io_test`
- 修复 3D 光照与材质参数被丢弃：`setLightComponent()`/`setMaterialComponent()`/`setShininess()` 现在真正存值，`Qwt3DSurface` 把它们上传为着色器 uniform（光源位置由 `setLightRotation` 推导、ambient/diffuse/specular 逐分量、shininess）；默认值与原先的硬编码逐像素一致，`illuminate()`/`blowout()` 切换唯一那盏灯，5 种主题光照预设因此产生不同效果。新增 getter 与 `tests/plot3d_lighting_test`
- 修复 matplotlib 来源的 colormap 预设色标为手工近似的问题：`inferno`/`magma`/`turbo` 的色标被压缩在前 ~70% 参考曲线上再铺满全区间，`viridis`/`cividis` 最大偏离 17 ΔE00。色标现按 matplotlib 官方 256 级表在预设位置采样（生成脚本 `docs/assets/algorithms/colormap_stops_gen.py`）：色标偏差 0.000、整带偏差 0.445–1.380 ΔE00，`inferno`/`magma` 分级升到 A
- 修复 `QwtScaleDiv::setTicks()` 存入重复刻度值（日期引擎 `qRound(i*minStep)` 塌缩）与 `QwtAbstractScaleDraw::draw()` 重复绘制重合刻度：刻度列表排序去重，与更长刻度类型同值的刻度被跳过，每个位置只画一次且保留最长长度。新增 `tests/scale_draw_test`
- 修复两处与实现不符的注释：`QwtColorMapPreset::presetFromName()` 如实说明未知名回退 Viridis（实现从未返回 −1），`Qwt3DSurface::setResolution()` 如实说明索引步进加全量重建/重上传
- 性能：AVX2 argmin/argmax 内核移入独立编译单元并由 CMake 以 `/arch:AVX2`（MSVC）/ `-mavx2`（GCC/Clang）编译（`qwt_simd_argminmax_avx2.cpp`）。按 x86-64 基线编译时其 256-bit 尾声混入 legacy SSE 且返回时上半 YMM 状态为脏——实测单次调用固定开销 446 ns（n=16），且同线程紧随其后的标量代码慢 16 倍；拆分后单次调用降到 13.5 ns、流式惩罚消失、MinMax 桶判据从约 481 个元素塌缩到 4–8 个。派发方仍在 CPUID 判定后才调用；amalgamate 构建照旧内联该文件
### 文档

- **新增基准示例 `examples/bench/chartbench` 与四库绘图性能对比报告**：该示例用同一套协议驱动 Qwt、仓库内的 QCustomPlot 2.1.0 与 Qt Charts，并以 git submodule 方式引入 QIm；八个测量维度（点数、曲线数、画布尺寸、数据交接 vs 重绘、降采样、帧时间分布、进程内存、启动开销）加图像导出，每个库与后端一对 CSV，另有驱动脚本跑完整个矩阵。协议本身即是结论的一部分——`QOpenGLWidget` 会把普通 `repaint()` 推迟到 Qt 的合成阶段，因此报告记录了改走 `makeCurrent()`/`paintGL()`/`glFinish()` 的做法（朴素协议会把 10 万点曲线测成 0.004 ms），QCustomPlot 副本带有上游 2.1.1 同样采用的 Qt 6 `Q_MOC_RUN`/`Q_NAMESPACE` 最小修复。17 张图与 14 张表由 `docs/assets/bench/chartbench_experiment.py` 从原始 CSV 重新生成；Qt Charts 的 OpenGL 序列路径作为实测阴性结果报告（帧缓冲中无曲线像素）

## tag:v7.4.2 (2026-10-10)

### 新功能

- **曲线渲染平滑**
    - `QwtPlotCurve` 新增 `SmoothAlgorithm` 枚举（`NoSmoothing`/`GaussianSmoothing`/`SavitzkyGolaySmoothing`），配套 `setSmoothAlgorithm()`、`setSmoothWindow()`、`setSmoothPolynomialOrder()` 与 `setSmoothPreserveThreshold()`
    - 两种算法都是作用于降采样并换算到绘制坐标之后的折线的低通滤波器——只修改 y 坐标，所有特征的水平位置保持精确
    - 默认开启的非线性特征保护步骤让显著的峰、谷保持原始位置与高度：像素级噪声带收敛为干净的线，数据不失真
    - 仅对 `QwtPlotCurve::Lines` 生效（含填充曲线），可与 `Fitted` 属性组合使用
    - 新增示例 `examples/2D/curvesmoothing`：100,000 点带随机抖动频谱，左图为默认渲染、右图为平滑后效果，可运行时切换算法/窗口/阶数/阈值并做 replot 基准测试
    - 新增使用指南《曲线渲染平滑》（中英双语，含截图）

### Bug 修复

- 修复绘图首次绘制时坐标轴轴线错位或不显示的问题 — 用户反馈的两个症状：x 轴最左边一小段没有画出来（x/y 轴线在左下角不相交，直到刷新坐标轴参数后才恢复），以及整条轴线在下次更新绘图前不可见。根因：布局用上一轮存储的 borderDist 摆放 scale widget，而轴线绘制用的是实时计算值；收敛所依赖的补偿性重布局由异步 `LayoutRequest` 驱动，在首次显示时序中经常缺失或迟到。本次修复让"摆放"与"绘制"在一轮布局内使用同一个 borderDist，并保证首帧绘制前一定完成一次布局：
    - `QwtPlotLayoutEngine` 的 `LayoutData::ScaleData::init()` 改用 `qMax(getBorderDistHint(), borderDist)` —— 与 `QwtScaleWidget::layoutScale()` 绘制所用的值完全同源 —— 不再读取过期的存储值，从源头消除滞后一轮的问题
    - `QwtPlot::doLayout()` 在 scale 矩形未变化时也刷新轴的 borderDist（刻度标签内容、格式或字体的变化会改变 hint 但不影响几何）
    - `QwtPlot::event()` 在 `PolishRequest` 的 replot 之后执行一次 `updateLayout()`，保证首帧绘制总是使用以最终字体和刻度划分计算的布局（仅在首次显示多一次布局，稳态无额外开销）
    - `QwtScaleWidget::changeEvent()` 处理 `FontChange`/`ApplicationFontChange`，并在 `LocaleChange` 时重排：字体变化不再导致 extent/borderDist 过期且不重绘
    - `QwtScaleWidget::setMinBorderDist()` 与其它几何 setter 一致地重新计算布局（影响 `QwtPlot::alignAxisBorderDist()` 与 `QwtFigure::alignAxes()`）
- 修复 `QwtScaleDraw::setLength()` 无条件把长度钳制到正的 10 的问题：它会把合法的负长度（`QwtPolarGrid` 左/上轴在用）静默变成 +10 并破坏极坐标网格渲染；已恢复 upstream 的钳制语义（小幅值钳到 ±10，更大的负值保留）
- 修复 `QwtSplineCurveFitter::fitCurve()` 丢弃拟合结果的问题：`subPaths.first()` 的返回值被丢弃、恒返回空多边形，导致 fill + Fitted 组合的曲线完全画不出来；已恢复上游行为（单一子路径时返回该子路径多边形）

## tag:v7.4.1 (2026-09-11)

### 新功能

- **外置坐标轴标题（QwtScaleWidget::TitleOutside）**
    - `QwtScaleWidget` 新增 `TitlePlacement` 枚举（`TitleInside`/`TitleOutside`）与 `setTitlePlacement()`/`titlePlacement()`；`QwtPlot` 提供便捷方法 `setAxisTitlePlacement()`/`axisTitlePlacement()`
    - `TitleOutside` 模式下标题不再画在 scale widget 内部、也不再计入轴维度；布局在 scale widget 相邻处预留字幕条（YLeft/YRight/XBottom 为正下方、XTop 为正上方），标题以水平文字画在字幕条内，并随布局动态跟随 scale widget
    - `QwtPlotLayout` 新增 `scaleCaptionRect()` 公开字幕条几何；多轴（parasite）场景由宿主布局聚合所有层的字幕高度需求，各层字幕在底带内按列以相邻字幕列中心为界分区，互不重叠
    - `QwtPlotRenderer` 新增 `renderScaleCaption()`，PNG/SVG/PDF 导出包含外置标题
    - 新增示例 `examples/2D/outsideTitle`：单 plot 与宿主+双 parasite 多轴演示，工具栏可运行时切换标题位置并导出验证；`--export <dir>` 参数可无头输出自检图与导出文件

- **坐标轴标题位置与对齐控制**
    - `QwtScaleWidget` 新增 `TitlePosition` 枚举（`TitleCentered`/`TitleAtStart`/`TitleAtEnd`）控制标题沿轴骨干的绘制位置，新增 `setTitleAlignment()`/`titleAlignment()` 控制标题框内的水平对齐；`QwtPlot` 通过 `setAxisTitlePosition()`/`setAxisTitleAlignment()` 转发
    - `TitleCentered`（默认）完全复现旧版布局；`TitleAtStart` 为垂直轴的底端/水平轴的左端，`TitleAtEnd` 为顶端/右端；标题保持自然方向，仅沿骨干的位置变化
    - `parasitePlot` 示例可运行时演示标题位置与对齐

- **QwtPlotLayout 固定画布尺寸模式**
    - `setFixedCanvasSize(axisPos, on)` 将画布尺寸与刻度标签增长解耦：锁定后画布尺寸保持稳定，溢出的标签被 scale widget 绘制区域裁剪（YLeft/YRight 固定画布宽度，XBottom/XTop 固定高度）
    - 自动模式在启用后的首次布局时捕获画布边缘偏移；手动模式 `setFixedCanvasSize(QSize)` 固定精确尺寸（负分量表示该方向自动捕获）；`resetFixedCanvasSize()` 清除捕获的偏移与手动覆盖
    - `alignScales()` 在对齐画布模式会缩小已固定维度时不再扩展画布
    - 主要面向宿主与 parasite 共享画布区域、不希望因标签增长而相互漂移的寄生绘图场景

- **QwtPlotAxisWheelInteraction — 轴级滚轮缩放与平移**
    - 新增交互类，直接在 `QwtScaleWidget`（轴刻度区域）上安装事件过滤器：普通滚轮以光标为中心通过 `QwtPlot::zoomAxis()` 缩放该轴，Ctrl+滚轮通过 `QwtPlot::panAxis()` 平移该轴
    - 无需先点击选中坐标轴（不同于 `QwtPlotScaleEventDispatcher`）；匹配缩放/平移的滚轮事件被消费，其余透传给 dispatcher
    - 完全可配置：缩放/平移修饰键、缩放因子（默认每步 1.2）、平移因子（默认每步 30 像素）、启用/禁用开关
    - 为继承设计，提供三个虚函数覆写点：`handleWheelEvent()`、`wheelZoom()`、`wheelPan()`
    - `parasitePlot` 示例新增 "Axis Wheel" 工具栏动作演示该类

- **QwtTextScaleDraw — 自定义文本刻度标签**
    - 新增 `QwtTextScaleDraw` 类，通过 `QMap` 将数值刻度值映射为自定义字符串标签，为坐标轴显示分类文本或特定文字提供更灵活的方式

- **曲线符号自动配色**
    - `QwtPlotCurve` 未定制的符号（默认无画刷 + `#555555` 画笔）在曲线画笔于 `attach()` 时已自动取色后自动同步为同色（符号画刷取画笔颜色，符号画笔取 `darker(150)`），散点符号跟随颜色循环；用户自定义的符号保持不变

- **3D item：RTTI、序列化与数据访问**
    - 新增 `Rtti3DValues` 枚举（`Rtti_Plot3DItem`、`Rtti_Plot3DSurface`、`Rtti_Plot3DBar`、`Rtti_Plot3DLine`），所有 3D 绘图 item 覆写 `rtti()`，与 2D `QwtPlotItem` 模式对齐；`Qwt3DPlot::attach()`/`detach()` 改为 virtual
    - 新增 `qwt3d_serialize.h/.cpp`：`Triple`、`RGBA`、`ParallelEpiped`、`Qwt3DFunctionData`、`Qwt3DParametricData`、`Qwt3DTheme` 的 `QDataStream` 运算符
    - 3D API 补充公有 getter（颜色类、颜色图例、坐标系、轴、标签、drawable、plot）；`Qwt3DSurface::gridData()`/`cellData()`/`isGridData()` 从 protected 移到 public
    - `Qwt3DBar` 新增 `samples()`、`isGridData()` 及网格元数据 / z 矩阵 getter（`gridZValues()` 等）

- **3D 线条点形状**
    - `Qwt3DLine` 新增 `PointShape` 枚举（Dot、Cube、Tetrahedron、Octahedron、Sphere）及 `setPointShape()`/`pointShape()`，Dots 样式下渲染 3D 标记
    - `line3D` 示例新增标记形状选择器

- **qwtplot 的 PySide6 绑定**
    - 绑定 CMake 重构为多模块编排结构（`bindings/core/`、`bindings/plot/`），Shiboken6 工具垫片参数化
    - 新增约 95 个类的绑定：picker 与 picker machine、zoomer、panner、放大器、rescaler、overlay、`QwtFigure`、寄生绘图框架、工具静态类、样条和 `QwtPointMapper`
    - 全面的测试覆盖：52 个测试（11 core + 41 plot），涵盖离屏渲染、信号与交互（zoomer、panner、magnifier、picker machine）

### Bug 修复

- 修复寄生绘图坐标轴的四个问题：
    - 共享轴拖动后 parasite 曲线被压缩或画到画布外 — parasite 的 `canvasMap()` 现在使用宿主的绘制区间，同时保留 parasite 自身的刻度区间
    - 共享轴上滚轮缩放范围错误或不刷新 — 滚轮位置现在映射到画布坐标，`replotAll()` 刷新宿主与所有 parasite
    - 选中拖动期间 parasite 轴标签被截断 — 新增 `alignAxisBorderDist()`/`alignAllAxisBorderDist()` 计算宿主与所有 parasite 的最大边距并在每次完整重绘后重新对齐
    - `zoomAxis()` 对反转轴无效（如 `setAxisScale(yRight, 500, 0)`）— 检测并处理反转范围，同时修复像素空间始终反转的非线性 Y 轴
- 修复 parasite 布局可能复制到过期宿主矩形的问题：宿主 `doLayout()` 在自身布局完成后刷新各 parasite 的布局，保证 parasite 复制的矩形（及其派生的字幕条）基于本轮宿主的最终布局
- parasite plot 收到 `LayoutRequest` 时同步刷新宿主布局，使 parasite 轴标题/尺寸变化及时进入宿主的带区预留聚合
- `QwtPlot::setAxisScaleDraw()` 不再在 `TickInside` 模式下静默重新启用外侧刻度（此前替换 scale draw 会导致刻度被双重绘制）
- `QwtPlotIntervalCurve` 默认画笔由 `#555555` 改为黑色，使 attach 时的自动配色对默认构造实例也生效
- `Qwt3DBar::setSamples()` 补上缺失的间距与高度属性赋值，保证柱体尺寸正确

### 构建

- 新增 xmake 构建配置（`xmake.lua`）作为 CMake 之外的替代方案，含 core 库所需的 `QWTCORE_MAKEDLL`/`QWTCORE_DLL` 宏定义
- Qt 5 兼容：核心头文件中 `constexpr` 替换为 `Q_DECL_CONSTEXPR`；`QwtPlotBoxChart` 离群点抖动在 Qt < 5.10 回退到 `qrand()`（画布与刻度事件分发头文件增加版本保护）
- 单文件合并源码（`src-amalgamate/QwtPlot.{h,cpp}`）完整再生成并修复合并模板：移除对 3D Plot+Item 重构中已删除文件的引用，纳入新的 `QwtTextScaleDraw`、`QwtPlotAxisWheelInteraction`、`Qwt3DPlotItem`/`Qwt3DSurface`/`Qwt3DBar`/`Qwt3DLine`、`Qwt3DRenderContext` 与序列化源码 — 单文件构建在当前 7.x API 下恢复可编译
- plot3d：gl2ps 矢量导出改为无条件编译，移除 `QWT3D_ENABLE_GL2PS` 宏守卫及 `qwt::plot3d` 对外传播的 PUBLIC 定义（该宏本就是事实必需 —— `qwt3d_plot.h`/`qwt3d_io.cpp` 的 `saveVector()` 无条件引用 `Qwt3DVectorWriter`，不定义宏根本编不过）；合成单文件构建（`staticExample`）不再需要额外定义

### 重构

- plot3d：移除 `Qwt3DSurface::setResolution()` 前无意义的 `public Q_SLOTS:` 段落；`Qwt3DSurface` 不是 `QObject`，现在就是一个普通 public 方法

### 文档

- 新增固定画布尺寸模式使用文档
- 新增外置坐标轴标题（TitleOutside）使用指南

## tag:v7.3.6 (2026-07-31)

### 新功能

- **Qwt3DBar 和 Qwt3DLine — 新增 3D 绘图项**
    - `Qwt3DBar`：3D 柱状图 item，支持 1D 序列 + 2D 网格（`Qwt3DFunctionData`）数据输入，逐柱平面法向量立方体（6 面），支持 Filled/FilledMesh/Wireframe 三种样式，复用 surface 着色器并两遍叠加边线
    - `Qwt3DLine`：3D 线/曲线 item，`setSamples` 接口与 `QwtPlotCurve` 对齐，`LineStyle` 支持 Lines/Tube/Dots；Tube 使用平行传输标架（parallel-transport framing）生成扫掠圆柱体并支持光照（直线段也稳健），Lines/Dots 复用共享 line/point 着色器
    - 主题集成：`Qwt3DTheme::applyToItem()` 扩展支持 Qwt3DLine/Qwt3DBar 分派（此前仅支持 Qwt3DSurface）
    - 新增示例：`examples/3D/bar3D`（高斯峰网格）和 `examples/3D/line3D`（螺旋管）

- **3D 坐标盒纵横比模式**
    - 为 3D 坐标盒新增 `AUTOFILL`/`DATARATIO` 纵横比模式

- **qwtplot3d 综合演示示例**
    - 新增示例项目位于 `examples/3D/qwtplot3d`，包含 MainWindow 和可停靠设置面板，支持曲面样式、坐标轴、图例、视图/光照和装饰的交互配置
    - 设置面板 UI 与已应用主题同步（`syncFromTheme` 方法）
    - 图例位置组合框根据方向动态更新选项（垂直显示 Left/Right，水平显示 Top/Bottom）

### Bug 修复

- 修复缩放处理后滚轮事件未被消费的问题 — `QwtPlotScaleEventDispatcher::handleWheelEvent` 在缩放选中的坐标轴后现在会 accept 滚轮事件；`QwtMagnifier::widgetWheelEvent` 返回值改为 bool（签名变更：`void` → `bool`），使事件过滤器能够 accept 并消费事件；此前滚轮事件会继续向父窗口传播（例如 QScrollArea 视口在坐标轴/画布缩放的同时发生滚动）
- 修复悬空指针：`reapplyAll()` 中重建 `Qwt3DColorMapColor`，因 `applyTheme()` 会替换并销毁旧 functor
- 稳定暴露轴标签锚点定位 — 用从坐标盒中心到轴中点的外向屏幕向量替代角度启发式，在视口纵横比变化时仍保持一致
- 修复各向异性 tic 长度模型 — tic 长度原为单一世界空间值，每次数据变化时被 `init()` 覆盖；现为逐轴自动模式（scale × 该轴自身数据范围），手动 `setTicLength()` 为持久覆盖，新增 `setTicLengthScale()`/`setAutoTicLength()` API（默认 scale 0.015）

### 重构

- **3D 颜色类变为纯值对象**
    - 移除 `Qwt3DColor`/`Qwt3DStandardColor`/`Qwt3DColorMapColor` 的 `Qwt3DSurface*`/`Qwt3DPlot*` 反向指针与 `notifyColorChanged()`，mutator 变为静默；z 范围由 surface 经 `setActiveRange()` 推入；颜色头文件不再 include/前向声明 `qwt3d_surface.h`/`qwt3d_plot.h`；就地修改已挂载 functor 后需调用 `surface->invalidateColors()`
- **消除反向指针违规（两项遗留 smell 解决）**
    - 引入 `Qwt3DRenderContext` 值结构体（封装着色器、矩阵、视口和坐标转换方法），`Qwt3DDrawable::draw()` 改为接收 `const Qwt3DRenderContext&`；移除 `Qwt3DDrawable` 的 `m_plot`/`plot()`/`setPlot()`
    - `Qwt3DFunction::create()` 和 `Qwt3DParametricSurface::create()` 改为返回数据（`Qwt3DFunctionData`/`Qwt3DParametricData`），不再通过反向指针推入；移除 `Qwt3DGridMapping` 的 `m_surface`；为 `Qwt3DSurface` 新增基于 vector 的 `loadFromData()` 重载
    - 移除 `Qwt3DEnrichment` 中 `plot` 死代码回指

### 文档

- 新增 3D 模块重构开发者指南（双语）：阐述 3D 模块重构为 Plot+Item 架构的原因与方法、分层设计、迁移表
- 新增 Qwt3DBar 和 Qwt3DLine 专属使用指南（双语）：包含关键特性、数据形状/样式表、类图、方法表、技巧和示例链接
- 精简 3D 绘图简介概述（双语），替换内联 bar/line 使用子节为指向专属页面的链接

## tag:v7.3.5 (2026-07-24)

### 破坏性变更

- **3D模块架构重构 — Plot + Item 模式**
    - 将 3D 模块从旧的 widget-per-plot 模式完全重构为 **Plot + Item** 架构，与 2D 模块（`QwtPlot` + `QwtPlotItem`）对称：
        - `Qwt3DPlot`（QOpenGLWidget）：纯渲染窗口，管理 GL 上下文、视图变换、光照、坐标系统和 item 列表。不再持有绘图数据。
        - `Qwt3DPlotItem`（新增）：所有 3D 绘图 item 的抽象基类（attach/detach/draw/hull/z/title），与 `QwtPlotItem` 对称。
        - `Qwt3DSurface`：曲面 item，使用 VBO/VAO + GLSL 3.3 Core 着色器渲染。合并了旧 `gridplot.cpp` 和 `meshplot.cpp` 的逻辑。
    - 移除 `namespace Qwt3D` — 所有类使用 `Qwt3D` 前缀定义在全局作用域（如 `Qwt3DPlot`、`Qwt3DSurface`、`Qwt3DFunction`）。
    - 重命名所有 3D 类：`Plot3D` → `Qwt3DPlot`，`SurfacePlot` → `Qwt3DSurface`，`Function` → `Qwt3DFunction`，`CoordinateSystem` → `Qwt3DCoordinateSystem`，`Axis` → `Qwt3DAxis`，`ColorLegend` → `Qwt3DColorLegend`，`Color` → `Qwt3DColor`，`StandardColor` → `Qwt3DStandardColor`，`ColorMapColor` → `Qwt3DColorMapColor`，`Qwt3DTheme`（已有前缀，仅去命名空间），`Mapping` → `Qwt3DMapping`，`GridMapping` → `Qwt3DGridMapping`，`ParametricSurface` → `Qwt3DParametricSurface`，`Scale` → `Qwt3DScale`，`AutoScaler` → `Qwt3DAutoScaler`，`Enrichment` → `Qwt3DEnrichment`，`Drawable` → `Qwt3DDrawable`，`Label` → `Qwt3DLabel`。
    - 删除 stub 类：`GraphPlot`、`MultiPlot`、`VolumePlot` — 如需要后续作为 `Qwt3DPlotItem` 子类重新实现。
    - 绘图方法从 `Qwt3DPlot` 移至 `Qwt3DSurface`：`setPlotStyle()`、`setDataColor()`、`loadFromData()`、`setResolution()`、`setMeshColor()`、`addEnrichment()`、`setFloorStyle()`、`setShading()`、`showNormals()`。
    - 移除 `updateData()` — 改用 `plot->update()` 或 `item->itemChanged()`。
    - 移除 `setCoordinateStyle()` — 直接使用 `Qwt3DCoordinateSystem` API。
    - `Qwt3DFunction` 和 `Qwt3DParametricSurface` 的 target 从 `SurfacePlot*`（widget）改为 `Qwt3DSurface*`（item）。

- **3D模块：现代 OpenGL 迁移**
    - 将所有旧版 OpenGL 调用（立即模式、显示列表、固定管线、GLU 二次曲面）替换为现代 OpenGL：
        - VBO（`QOpenGLBuffer`）+ VAO（`QOpenGLVertexArrayObject`）管理顶点数据。
        - GLSL 3.30 Core 着色器用于所有渲染（surface、line、point、polygon、text）。
        - CPU 端 `QMatrix4x4` 矩阵计算（不使用 GL 矩阵栈）。
        - `QOpenGLTexture` 用于文本标签（替代 `glRasterPos3d` + `glDrawPixels`）。
        - CPU 生成的三角网格几何体用于 Cone/Arrow 装饰（替代 `gluCylinder`/`gluDisk`）。
    - 删除 `qwt3d_openglhelper.h`（旧版 GL 状态管理工具）。
    - gl2ps 矢量导出封装在 `QWT3D_ENABLE_GL2PS` 条件编译宏下（Compatibility Profile 回退）。
    - 新增 10 个 GLSL 着色器文件在 `src/plot3d/shaders/`（surface、polygon、line、point、text — vert+frag 配对）。

- **3D主题系统：Plot级/Item级拆分**
    - `Qwt3DTheme::apply()` 现在将 plot 级属性（背景、坐标颜色、标题、光照）应用到 `Qwt3DPlot`，将 item 级属性（meshColor、dataColorPreset、plotStyle、shading、smoothMesh）通过 `dynamic_cast` 应用到已挂载的 `Qwt3DSurface` item。

- **3D IO系统适配**
    - `Qwt3DNativeReader` 现在搜索 `plot->itemList()` 中的 `Qwt3DSurface` item（如不存在则创建），替代 `dynamic_cast<SurfacePlot*>(plot)`。
    - 重命名 IO 类：`IO` → `Qwt3DIO`，`PixmapWriter` → `Qwt3DPixmapWriter`，`VectorWriter` → `Qwt3DVectorWriter`，`NativeReader` → `Qwt3DNativeReader`。

- **3D示例更新**
    - `examples/3D/` 下全部 6 个示例更新为新的 Plot + Item API。
    - `SIGNAL()/SLOT()` 连接迁移为新式 `connect()` 语法。
    - 示例装饰中的旧版 GL 调用替换为 VBO + 着色器渲染。

## tag:v7.3.4 (2026-07-12)

### 新功能

- **全 2D 模块统一 NaN/Inf 处理**
    - 将 5 种混用的 NaN 检测方式（`qwt_is_nan_or_inf`、`!std::isfinite`、`qIsNaN`、`std::isnan`、本地位级 `qwtIsNaN`）统一为 `qwt_is_nan_or_inf()`，覆盖 `src/plot/` 和 `src/core/`
    - 将 `isSampleNanOrInf()` 从匿名命名空间提升为 `qwt_series_data.h` 中的公共导出 API，供所有绘图项使用
    - 为 10+ 个原先缺失 NaN 处理的绘图项添加 NaN/Inf 跳过：
        - `QwtPlotBarChart`、`QwtPlotHistogram`、`QwtPlotIntervalCurve`、`QwtPlotMultiBarChart`
        - `QwtPlotTradingCurve`、`QwtPlotVectorField`、`QwtPlotBoxChart`、`QwtPlotSpectroCurve`
        - `QwtPolarCurve`、`QwtPlotSpectrogram`（替换本地位级 `qwtIsNaN`）、`QwtPolarSpectrogram`
    - 补全 `QwtPlotCurve` 中 4 处缺失的 NaN 跳过路径
    - `QwtColorMap::rgb()` 和 `colorIndex()` 对 NaN/Inf 输入值返回透明色
    - `QwtBoxStatistics` 在排序和统计计算前预过滤 NaN/Inf
    - 新增双语 NaN 处理用户指南（`docs/use-guide/nan-handling.md`）

- **nanperf — NaN 性能基准测试示例**
    - 新增示例位于 `examples/2D/nanperf/`，用于基准测试 NaN 处理性能
    - `NanDataGenerator` 支持可配置的 NaN 分布模式，附带单元测试
    - `BenchmarkRunner` 提供计时框架，跨过滤模式和数据量进行分析
    - `MainWindow` 采用 2×2 网格布局、控制栏、扫描编排和 Markdown 报告导出
    - `FilterModes` 和 `PlotPanel` 组件用于并排对比

- **PySide6 绑定（POC）**
    - 初始 PySide6/Shiboken6 绑定，覆盖 `qwt::core` 类
    - 类型系统扩展至全部 core 模块类
    - 完整功能测试套件（`bindings/tests/test_core.py`）
    - CMake 集成，包含 `FindPySide6Shiboken.cmake` 和 `Shiboken6Tools`

- **QwtDate 现代化**
    - 将 `QwtDate` 内部实现从已弃用的 `Qt::TimeSpec` 迁移至 `QTimeZone`
    - 在 `qwt_qt5qt6_compat.hpp` 中新增 `qwt::compat::` 时区辅助函数，保持 Qt 5.12+/Qt6 双兼容
    - 更新 `qwt_date_scale_draw.cpp` 和 `qwt_date_scale_engine.cpp` 使用新 API

### Bug 修复

- 修复 `QwtVectorFieldSample` NaN 检查 — 原先检查 `vx` 两次，漏检 `vy`
- 修复 `QwtInterval::contains(NaN)` — 原先错误返回 `true`，现正确返回 `false`
- 修复 `qwtBoundingRect` 越界访问和 NaN 传播 bug
- 修复 `QwtLinearColorMap::colorIndex()` — 注释声称处理 NaN 但实现未处理
- 修复 `QwtGridData::findValueRange` NaN 传播和 `dyMin` 计算 bug
- 修复 `QwtPolarSpectrogram` Indexed 路径缺失 NaN 守卫

### 构建

- 确保 MinGW 下 UTF-8 窄字符串字面量正确（CMake `target_compile_definitions` 添加 `UNICODE`/`_UNICODE`）
- 新增 `qmakeExample` — 演示使用 qmake 进行单文件合并构建

### 文档

- 新增 qmake 单文件构建指南（中文，`docs/zh/build-guide/qmake-single-file.md`）
- 在双语 README 中添加 qmake 使用说明
- 修复曲线文档和头文件注释中错误的默认 `PaintAttribute` 描述
- 更新双语曲线文档中的降采样算法推荐说明

## tag:v7.3.2 (2026-06-20)

### 新功能

- **QwtColorMapPreset 应用于光谱图**
    - `QwtPlotSpectrogram` 和 `QwtPolarSpectrogram` 默认使用 Viridis 色彩映射替代旧的蓝-黄线性映射
    - 光谱图和极坐标光谱图示例新增预设切换 UI 控件
    - 经典色彩映射类型保留向后兼容

- **QwtPolarPlot 背景统一**
    - 统一 `QwtPolarPlot` 背景行为与 `QwtPlot` 一致

### 变更

- **3D 默认色彩映射迁移至 viridis**
    - `StandardColor::reset()` 从蓝-红渐变改为 viridis 预设
    - `Qwt3DTheme` 默认预设从 "jet" 改为 "viridis"，符合现代可视化标准
    - 刷新 3D 截图资源以反映新的默认色彩映射

### 文档

- **文档结构重组** — 扁平化目录层级，新增 40+ 篇用户指南
    - 覆盖曲线、图表、3D 绘图、交互工具和控件等全部主要功能
    - 新增开发者指南：编码规范、注释规范、PIMPL 模式用法
    - 更新 mkdocs.yml 导航，反映新的扁平结构和 i18n 文件夹组织
    - 统一截图和示例路径链接

## tag:v7.3.1 (2026-06-17)

### 新功能

- **qwt::core 共享库**
    - 提取 `qwt::core` 共享库（`qwtcore.dll`），包含颜色映射和调色板公共功能
    - `QwtColorMap` 重构：移除 `QwtInterval` 依赖，新签名 `rgb(vMin, vMax, value)`
    - `QwtColorCycle` 从 plot 模块迁移至 core
    - 新增 `QwtColorMapPreset` 类，提供 22 种科学可视化色彩映射预设（viridis、plasma、inferno、magma、cividis、jet、hot、cool、spring、summer、autumn、winter、gray、bone、copper、rainbow、hsv、turbo、coolwarm、rdylbu、rdylgn、spectral）
    - `src/plot/qwt_colormap_compat.h` 提供向后兼容的 `QwtColorMapCompat` 命名空间，包含基于 `QwtInterval` 的重载函数

- **3D 主题系统**
    - 新增 `Qwt3DTheme` 类，提供 10 种预设主题：Default、Dark、Scientific、Warm、Cool、Matplotlib、EarthTones、Ocean、HighContrast、Presentation
    - 新增 `ColorMapColor` 适配器，桥接 2D 色彩映射到 3D 颜色函数
    - 5 种光照预设：NoLighting、FlatLight、Studio、Outdoor、Soft
    - `Plot3D::setTheme()`、`Plot3D::applyTheme()` 一键应用主题
    - `StandardColor::setPreset()` 快速切换色彩映射预设

### 架构变更

- 三模块架构：`qwtcore.dll` → `qwtplot.dll` + `qwtplot3d.dll`
- `plot` 和 `plot3d` 都依赖 `core`，但彼此独立
- 运行时需要 `qwtcore.dll` + `qwtplot.dll`（+ `qwtplot3d.dll` 如果启用 3D）
- `src/plot/` 中保留向后兼容的转发头文件，支持现有 `#include "qwt_color_map.h"` 路径
- **Core 模块扩展**：将 24 个通用工具模块从 `plot` 迁移至 `core`，core 模块从 4 个模块（颜色 + 全局导出）扩展为包含 28 个模块的完整基础工具库：
    - 数学工具：`qwt_math.h/.cpp`、`qwt_simd_argminmax.h/.cpp`
    - 数据类型：`QwtInterval`、`QwtPoint3D`、`QwtPointPolar`、`QwtSamples`、`QwtBoxStatistics`
    - 几何算法：`QwtBezier`、`QwtClipper`
    - 坐标变换：`QwtTransform`、`QwtScaleMap`、`QwtScaleDiv`、`QwtScaleEngine`
    - 时间处理：`QwtDate`、`QwtSystemClock`
    - 通用算法与兼容层：`qwt_algorithm.hpp`、`qwt_qt5qt6_compat.hpp`
    - 数据容器：`QwtGridData`
    - 数据系列：`QwtSeriesData`、`QwtPointData`、`QwtSeriesStore`
    - 栅格数据：`QwtRasterData`、`QwtMatrixRasterData`、`QwtGridRasterData`

### 构建系统

- 新增 `src/core/CMakeLists.txt` 用于 core 库目标
- 更新 `src/plot/CMakeLists.txt` 链接 `qwt::core`
- 更新 `src/plot3d/CMakeLists.txt` 链接 `qwt::core` 并包含主题源文件
- 更新 amalgamate 工具模板以包含 core 模块文件
- 更新 `classincludes/` 头文件指向 core 模块路径
- 更新 `tools/make-classinclude.py` 适配新模块结构

### 示例

- **figureSurface3D** — 新增主题切换 UI 并修复主题切换时颜色图例的刷新问题

### Bug 修复

- 修复 3D 动画自动启动崩溃问题：将动画定时器延迟到 `showEvent()` 中启动，避免在控件可见前访问 GL 上下文
- 修复 `figureSurface3D` 切换主题时颜色图例不刷新的问题

## tag:v7.3.0 (2026-06-12)

### 新功能

- **QwtColorCycle 颜色循环系统**
    - 新增`QwtColorCycle`类，管理颜色调色板并支持循环使用，内置7种预设调色板及自定义调色板
    - 与`QwtPlot`集成，绘图项在`attach()`时自动从颜色循环中获取颜色
    - 支持的绘图项：`QwtPlotCurve`、`QwtPlotMultiBarChart`、`QwtPlotBoxChart`、`QwtPlotHistogram`、`QwtPlotIntervalCurve`、`QwtPlotBarChart`
    - 新增颜色循环测试用例

- **曲线降采样过滤器**
    - `QwtPlotCurve`新增`FilterPointsPixel`和`FilterPointsLTTB`两种绘制属性
        - `FilterPointsPixel`：基于像素列的降采样，适用于超大数据集的极速渲染
        - `FilterPointsLTTB`：简化版LTTB（MinMax桶）算法，在降采样同时更好地保留视觉形状
    - 默认绘制属性由`FilterPoints`改为`FilterPointsAggressive`
    - `qwtMapPointsQuad`优化NaN/Inf值跳过逻辑和多边形内存预分配

- **SIMD加速的MinMax桶降采样**
    - 新增`qwt_simd_argminmax`模块，提供SIMD加速的argmin/argmax计算
    - 支持SSE2/AVX2/NEON指令集，自动检测并选择最优指令集
    - 显著提升MinMax桶降采样的性能

- **Picker增强**
    - `QwtPlotSeriesDataPickerGroup`增加点击位置获取功能，支持在点击时获取屏幕坐标和数据坐标
    - `QwtPlotSeriesDataPicker`扩展支持所有`QwtPlotSeriesItem`子类型的数据拾取：
        - 交易曲线(K线图)、区间曲线、直方图、向量场
        - 柱状图、箱线图、光谱曲线
    - `FeaturePoint`新增`sampleData`字段（QVariant），可携带完整样本数据（OHLC、区间、向量等）
    - 通过`Q_DECLARE_METATYPE`注册样本类型，支持跨模块传递

- **扁平化风格控件**
    - `QwtSimpleCompassRose`新增`flatStyle`属性，启用后用纯色替代经典双色3D效果
    - `QwtWheel`新增`flatStyle`属性，启用后用扁平颜色和简单线条替代3D渐变
    - `QwtSlider`、`QwtThermo`新增`flatStyle`属性支持现代扁平化UI
    - `QwtDialNeedle`增加多种扁平化指针样式
    - `QwtKnob`默认使用`Flat`模式

### 性能优化

- `qwtProbeOrientation`和`qwtFindVisibleRange`优化采样策略，最多检查50个采样点并确保最小步长为1
- `qwtPixelColumnReduce`优化：预分配polyline向量、使用直接指针赋值替代动态追加
- bins初始化使用`QVector`构造函数简化

### 代码重构

- **PIMPL宏统一迁移**（涉及180+文件）
    - 所有类从手动PIMPL（裸指针`PrivateData* m_data`）迁移至标准宏模式
    - 头文件使用`QWT_DECLARE_PRIVATE(ClassName)`，实现使用`QWT_DECLARE_PUBLIC` + `QWT_PIMPL_CONSTRUCT` + `QWT_D/QWT_DC`
    - 使用`unique_ptr`自动管理生命周期，移除所有`delete m_data`
    - 覆盖绘图核心、坐标轴、文本/图例、颜色映射、画布、柱状图、交互类、数据类、样条、控件、极坐标等全部模块

- **3D绘图模块重构**（涉及53文件）
    - 添加`override`说明符到所有虚函数和析构函数
    - `nullptr`替代`0`作为空指针常量
    - `SurfacePlot`重构为PIMPL模式（`QWT_DECLARE_PRIVATE`）
    - `qwt3d_ptr`重命名为`ClonePtr`，统一include guards

- **现代C++特性**
    - 使用`auto`关键字和基于范围的for循环进行代码现代化
    - 添加移动语义和C++11默认成员初始化器
    - 为简单getter/setter和构造函数添加`noexcept`
    - 扩展`constexpr`到区间、坐标轴、刻度映射和数学工具函数
    - 跨`src/plot`和`src/plot3d`模块进行C++惯用法现代化

### 新增示例

- **renderbench** - 曲线渲染性能基准测试
    - 窗口化数据系列（`WindowedSeriesData`），支持实时数据流模拟
    - 控制面板可调整数据量、降采样模式、线宽等参数
    - 支持生成Markdown格式的性能报告
    - 位于`examples/bench/renderbench/`
- **staticExample** - 重写为Qwt综合特性展示示例
    - 演示曲线、频谱图、柱状图、箱线图、向量场等多种绘图类型
    - 统一更新所有2D示例的颜色调色板和画布背景风格

### 文档

- 新增曲线降采样算法完整文档（中文）
- 新增`QWEN.md` AI Agent指引文件，更新构建说明
- 添加英文文档并重构双语文档站点结构
- 统一执行英文注释规范，所有源码注释改为纯英文Doxygen格式
- 大批量Doxygen双语注释补全（40+批次），覆盖所有核心头文件

### Bug修复

- 修复Qt6下legend图标因`QwtNullPaintDevice::metric()`返回0（设备像素比）导致的空白问题
- 修复`QwtPlotMarker`箭头首尾端点反转问题，改进图例图标渲染
- 修复绘图背景渲染问题，改用`QPalette::Base`动态`backgroundRole`，确保主题切换时背景色正确

## tag:v7.2.1

- 新增刻度朝内显示功能
    - 新增`QwtPlot::TickDirection`枚举，支持`TickOutside`（刻度朝外，默认）和`TickInside`（刻度朝内）
    - 新增`QwtPlot::setAxisTickDirection()`和`QwtPlot::axisTickDirection()`方法，可独立控制每个坐标轴的刻度方向
    - 刻度朝内时，刻度线从画布边缘向内延伸，主干和标签保持在画布外
    - 朝内刻度与朝外刻度共享相同的样式设置（长度、颜色、线宽）
    - 新增示例程序：`examples/2D/ticks_inside/`
    ![ticks_inside](./docs/assets/screenshots/ticks_inside.png)

## tag:v7.2.0

- 新增箱线图(Box Chart)支持
    - 新增`QwtPlotBoxChart`类，用于绘制箱线图（Box-and-Whisker Plot）
    - 新增`QwtBoxStatisticsCalculator`类，用于自动计算统计量
    - 新增数据结构：`QwtBoxSample`、`QwtBoxOutlierSample`、`QwtBoxChartData`、`QwtBoxOutlierChartData`
    - 支持预计算数据和原始数据两种输入方式
    - 支持多种箱体样式：矩形(Rect)、菱形(Diamond)、缺口形(Notch)
    - 支持多种须须计算方法：Tukey(1.5×IQR)、百分位数、最小最大值、标准差、标准误
    - 支持垂直和水平两种显示方向
    - 支持异常值自动检测、自定义符号和抖动显示
    - 新增示例程序：`examples/2D/boxchart/`
	![Box Chart](./docs/assets/screenshots/BoxChart.png)

## tag:v7.0.8

- `QwtFigure`增加`addAxisAlignment`等坐标轴对齐功能，可以指定子绘图的坐标轴进行对齐
	坐标轴对齐的效果如下：
	![axis-alignment](./docs/assets/picture/figure-scale-aligment.png)

- `QwtPointMapper`添加对`NaN`和`Inf`值的异常处理，在数据存在异常值时不会导致坐标映射异常
- `qwt_series_data.cpp`的数据范围判断增加对`NaN`和`Inf`值的异常处理
- 解决合并文件漏了QwtSlider类的问题（Issue #4）

## tag:v7.0.7

- v7.0.5~v7.0.6新增的`QwtScaleWidget::panScale`函数移动到`QwtPlot`中，并改名为`panAxis`，并修复了在对数坐标轴移动异常的问题，解决对数坐标轴坐标移动问题
- 原来`QwtPanner`类改名为`QwtCachePanner`,代表带缓存的`Panner`。Qwt6.0的Panner为了避免频繁刷新使用了一个pixmap进行缓存，因此把这类Panner统称为`CachePanner`
	- 同步`QwtPlotPanner`类改名为`QwtPlotCachePanner`
	- 同步`QwtPolarPanner`类改名为`QwtPolarCachePanner` 
	
	`QwtPlotPanner`效果如下：
	![series-data-picker-yvalue](./docs/assets/screenshots/qwt-realtime-panner.gif)
	
- 重写`QwtPlotPanner`类，继承`QwtPicker`，可实现拖动过程能实时刷新，如果想要原来的带缓存的panner，可使用`CachePanner`相关类
- 新增`QwtCanvasPicker`针对canvas的picker操作都继承此类
- `QwtPlotPanner`支持线性坐标轴、对数坐标轴、多坐标轴的实时平移
- 针对`QwtPlotZoomer`只能绑定2个坐标轴问题，新增`QwtPlotCanvasZoomer`类
	- `QwtPlotCanvasZoomer`无需绑定坐标轴，直接对整个cavas进行缩放
	- `QwtPlotCanvasZoomer`支持多坐标轴的缩放
- 原`QwtPlotZoomer`更名为`QwtPlotAxisZoomer`
- `QwtPlotSeriesDataPicker`将直接继承`QwtPicker`，不再继承`QwtPlotPicker`
- `QwtPlotMagnifier`支持多坐标轴的缩放
- `QwtScaleMap`增加移动语义
- 新增`make-classinclude.py`,可以导出`classincludes`目录，使用方法见`make-classinclude.py`
--

v7.0.7对Qwt的命名进行了调整

|原名称|新名称|备注|
|:--|:--|:--|
|QwtPlotZoomer|QwtPlotAxisZoomer|由于原QwtPlotZoomer只能绑定2个坐标轴，故改名为QwtPlotAxisZoomer|
|新增|QwtPlotCanvasZoomer|针对整个画布缩放的zoomer|
|QwtPanner|QwtCachePanner|原Panner无法实时拖动，故名Cache Panner|
|QwtPlotPanner|QwtPlotCachePanner|原Panner无法实时拖动，故名Cache Panner|
|QwtPolarPanner|QwtPolarCachePanner|原Panner无法实时拖动，故名Cache Panner|
|新增|QwtPlotPanner|原QwtPlotPanner改名为QwtPlotCachePanner后新增QwtPlotPanner|

## tag:v7.0.6

- 新增`QwtPlotSeriesDataPicker`类，提供了绘图数据的拾取
    `QwtPlotSeriesDataPicker`效果如下：
    ![series-data-picker-yvalue](./docs/assets/picture/series-data-picker-yvalue.gif)
    ![series-data-picker-nearest-value](./docs/assets/picture/series-data-picker-nearest-value.gif)
- 完善了寄生绘图的刷新机制，不会在构造时无法完全更新
- 有些接口的索引类型由int改为size_t
- `QwtPlotSeriesDataPicker`支持日期坐标轴的正常显示
- 修正了`QwtFigureWidgetOverlay`的一些事件处理会和坐标轴动作冲突的问题

## tag:v7.0.5

- `QwtScaleWidget`增加坐标轴内置动作功能，实现坐标轴的pan和zoom两种内置动作
	- `QwtScaleWidget`增加`scaleRect`函数，可以获取坐标刻度所在矩形，也就是用来绘制刻度的区域
	- `QwtScaleWidget`增加`QwtScaleWidget::BuiltinActions`枚举，目前包含了两个内置动作
	- 增加`requestScaleRangeUpdate`信号，用于请求绘图刷新坐标轴范围
	- 增加`setSelected`/`isSelected`函数，让坐标轴可选中，增加信号：`selectionChanged`
	- 增加坐标轴动作相关的一些属性设置如：`zoomFactor`、`panScale`等函数
- 增加`QwtPlotScaleEventDispatcher`类，实现绘图上处理坐标轴的相关事件
- `QwtPlot`的寄生绘图相关接口进行了一定调整
- `QwtPLot`增加`setEnableScaleBuildinActions`/`isEnableScaleBuildinActions`/`setupScaleEventDispatcher`等函数用于控制坐标轴的缩放和平移
- `QwtPLot`增加`plotList`函数，能获取所有绘图对象，包括寄生绘图和宿主绘图

## tag:v7.0.4

- 实现寄生绘图功能
- 增加寄生绘图的例子
- `QwtPlot`添加`updateAxisEdgeMargin`函数，让寄生绘图可以自动调整坐标轴位置

## tag:v7.0.2

- 抽取出`QwtPlotLayoutEngine`类
- 增加`QwtPlotParasiteLayout`类，用于实现寄生轴的布局
- 增加`QwtPlotTransparentCanvas`类，用于实现一个完全透明的画布
- `QwtFigure`类实现寄生轴功能
- `QwtScaleWidget`添加了`setEdgeMargin`和`edgeMargin`方法，可以实现刻度和绘图边缘的距离设置
- 调整了`QwtScaleWidget`的布局方案，能支持`edgeMargin`
- 例子增加`parasitePlot`演示如何使用寄生轴
- `QwtPlot`增加`rescaleAxes`,`syncAxis`等方法，用于实现快速调整坐标轴
- 针对nan和inf值的异常处理

## tag:v7.0.1

- 增加`QwtFigure`类，`QwtFigure`用于管理多个QwtPlot，实现类似Matplotlib的Figure功能，支持网格化布局

`QwtFigure`类的效果如下：

![figure](docs/assets/screenshots/qwt_figure.png)

- example增加figure例子，用于演示`QwtFigure`的使用
- 新增`QwtFigureWidgetOverlay`，可以实现在`QwtFigure`上面进行一些动作，目前支持拖动绘图，缩放绘图的功能

## tag:v7.0.0

- 把整个工程合并为QwtPlot.h和QwtPlot.cpp，直接可以引入，文件位于src-amalgamate
- 例子增加staticExample
- 增强`QwtPlotBarChart`的接口以支持pen和brush的设置.
- 增加`QwtGridRasterData`类，相比`QwtMatrixRasterData`，它支持一个二维数据表，以及x,y轴进行网格插值
- `QwtLinearColorMap`增加stopColors函数，修改`QwtLinearColorMap`的`colorStop`函数为`stopPos`
- `Qwt`的初始化参数进行了调整，让默认绘图更符合当前绘图的审美

`QwtPlotCanvas`初始化如下：
```cpp
QwtPlotCanvas::QwtPlotCanvas(QwtPlot* plot) : QFrame(plot), QwtPlotAbstractCanvas(this)
{
    ...

    setLineWidth(0);
    setFrameShadow(QFrame::Plain);
    setFrameShape(QFrame::Box);
}
```

`QwtPlotLayout`初始化调整如下

```cpp
QwtPlotLayout::QwtPlotLayout()
{
...
    setCanvasMargin(-1);
...
}
```

- 去除QWT_MOC_INCLUDE
- 调整了一些类的实现，以便能合并到一个文件中

------

**Qwt 7.0** is my modified version based on Qwt 6.2.0 source code. It complies with Qwt's open-source license, and I'm releasing these modifications as open-source.

Starting from this version, Qwt will support:
- The latest **C++11 standard**
- **CMake build system**

**Modifications in this version:**
1. Enhanced `QwtPlotBarChart` interface to support **pen and brush configuration**  
2. Added `QwtGridRasterData` class – supports **2D data tables** with **grid interpolation** on X/Y axes (vs. `QwtMatrixRasterData`)  
3. Added `stopColors()` function to `QwtLinearColorMap` and renamed `colorStop()` to `stopPos()`  


> **Qwt7.0**是我基于Qwt6.2.0版本对源码进行修改后的版本，遵循Qwt的开源协议，我将修改进行开源
>
>从此版本的Qwt，将有如下改动:
>	
>- 支持最新的**C++11标准**
>- **支持CMake构建**
>
>下面是此版本的修改内容：
>- 增强`QwtPlotBarChart`的接口以支持pen和brush的设置.
>- 增加`QwtGridRasterData`类，相比`QwtMatrixRasterData`，它支持一个二维数据表，以及x,y轴进行网格插值
>- `QwtLinearColorMap`增加stopColors函数，修改`QwtLinearColorMap`的`colorStop`函数为`stopPos`



Qwt 6.2.0

------

0) Requirement for Qt >= 4.8

1) Class Includes added

Include files, that match the class names are available now. So
it is possible to write "#include <QwtPlot>" now instead of "include qwt_plot.h"

2) BSD License for examples

Where possible the code of the examples is available under the 3-clause BSD License 

3) MathML text renderer removed

The code can be found at https://github.com/uwerat/qwt-mml-dev now and is intended
to become a standalone lib. Anyone who is interested to workon it, please let me know.

4) Spline interpolation

The broken implementation of QwtSpline has been replaced by a bunch of classes
offering all sort of functionalities around splines.

The most popular spline approximation/interpolation algos have been implemented:

	- Basis
	- Cardinal
	- ParabolicBlending
	- Akima
	- The one used in MS Excel
	- Cubic

An implementation of the de Casteljau’s algorithm has been added

	- QwtBezier

5) New plot items

	- QwtPlotVectorField
      A new type of plot item for vector fields

    - QwtPlotGraphicItem
      An item displaying a QwtGraphic image ( f.e used by QwtPlotSvgItem )

6) Plot Canvas

	- QwtAbstractPlotCanvas introduced
	- QwtPlotOpenGLCanvas added to support QOpenGLWidget

7) QwtPlotCurve

	- QwtPlotCurve::FilterPointsAggressive mode added - a fast weeding algo
      for huge datasets with increasing x or y values

	- QwtPlotCurve::closestPoint is virtual now
	- QwtPlotCurve: polygon clipping includes the painter clip
	- QwtPlotCurve::setLegendAttributes added
	- QwtValuePointData added for curves, where the x values are the index
	- a couple of new QwtPlotCurve::setSamples alternatives

8) QwtPlotSpectrogram

	- QwtPlotSpectrogram::setColorTableSize added
	- QwtRasterData::setInterval/interval changed into a pure virtual getter
	- QwtMatrixRasterData::BicubicInterpolation added
	- QwtMatrixRasterData::interval: API cleanup
	- QwtHueColorMap, QwtSaturationValueColorMap added

9) QwtPlotRenderer

	- using QPdfWriter where possible

10)
	- LOG_MIN/LOG_MAX removed, use QwtTransform::LogMin/LogMax instead ( values differ ! )
	- qwt_compat.h removed
	- qwtFuzzyGreaterOrEqual/qwtFuzzyLessOrEqual removed
	- qwtGetMin/qwtGetMax removed

11)
    - Not aligning unknown paint engines ( f.e EMF )
    - QwtNullPaintDevice is using a different type than QPaintEngine::User now

12) Many other changes ...
