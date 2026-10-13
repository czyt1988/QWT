# 源码缺陷清单（技术报告写作期间实证发现，待立项修复）

> **修复状态（2026-10-13）**：14 项全部处理完毕，实施分支 `fix/known-defects`；每项的代码/测试/文档改动见对应的提交，实验脚本已按修复后的库重跑（第 5 项的新数据在 `simd_bench_results_v75{0,1}_*.csv` / `streaming_micro_results_v75{0,1}_*.txt`，第 9 项在 `colormap_results.txt`，第 1 项在 `boxstats_results.txt`，第 2、7 项在 `clipper_results.txt`，第 3 项在 `surface_export_check.txt`），`docs/zh/tech-report/` 与 `docs/tech-report/` 的对应章节已同步。

2026-10-12 完成 `docs/zh/tech-report/` 十篇技术报告期间，通过 C++ 交叉验证、位级比对与实测基准发现以下源码问题。**全部只记载、未修改 `src/`**；本文件供后续修复任务立项使用，修完一项勾一项。每项的完整证据（实验数据、复现脚本、推导）在对应技术报告的失效模式清单章节，实验资产在 `docs/assets/algorithms/`。

严重度定义：**P0** 正确性缺陷（输出错误结果）；**P1** 性能缺陷或行为陷阱（特定场景显著劣化/产生伪影）；**P2** 注释不符、死代码与分析级风险。

---

## P0 正确性缺陷

### 1. 箱线图 Percentile 须线分位数水平互换

- [x] 已修复
- **位置**：`src/core/qwt_box_statistics.cpp:175-179`（`QwtBoxStatisticsCalculator` 的 Percentile 分支）
- **现象**：`whiskerLower = quantile(sorted, 1 - c/100)`、`whiskerUpper = quantile(sorted, c/100)`，上下分位数水平写反。c=5 时须线下端取第 95 百分位、上端取第 5 百分位，`wl > wu`。
- **影响**：系数 c < 50 时 `QwtBoxSample::isValid()` 恒为 false，严格不等式判据把**全部样本判为离群点**（实测 64/64）；箱线图 Percentile 模式完全不可用。c ≥ 50 时不触发（c=95 反而"碰巧正确"），常数数据不触发。
- **证据**：`docs/zh/tech-report/boxplot-quantiles.md` §3.3/§5；`docs/assets/algorithms/boxstats_check.cpp`（实测 pctl5：wl=1.860208 > wu=−1.599655，outlierCount=64/64）。
- **修复建议**：交换两行的分位数水平（lower 用 c/100、upper 用 1−c/100）；补一个 Percentile 模式的单元测试（当前 `tests/` 无覆盖）。
- **连带**：中文教程 `docs/zh/use-guide/2d/boxchart.md` 已加"已知问题"警示（英文版同步），修复后应撤下警示。

### 2. QwtClipper::clipCircle 竖直特例角度交换

- [x] 已修复
- **位置**：`src/core/qwt_clipper.cpp:296-297`（`toAngle` 的 dx==0 竖直分支）
- **现象**：竖直特例返回的角度与 `qwtPolar2Pos`（y = py − r·sin a）及自身非竖直分支的约定相反。
- **影响**：圆心在裁剪矩形竖直边线上时，返回弧区间错误。实测左边界案例：返回 5.442120 rad，真值 2.300518 rad，对称差 3.141601 rad（≈π）；右边界同样偏 π。极坐标网格（`qwt_polar_grid.cpp`）消费此函数。
- **证据**：`docs/zh/tech-report/polygon-clipping.md` §4.3 表与 §5；`docs/assets/algorithms/clipper_check.cpp` + `clipper_experiment.py`（300 组随机圆与暴力采样对照全过，竖直/角点定向用例失败）。
- **修复建议**：竖直分支角度改为与非竖直分支同一约定（对照 `qwtPolar2Pos` 推导）；把定向用例固化进 `tests/`。

### 3. gl2ps 矢量导出：3D 曲面填充全部 alpha=0，输出不可见

- [x] 已修复
- **位置**：`src/plot3d/qwt3d_io_gl2ps.cpp`（feedback 导出路径，options 见 :171）+ `src/plot3d/gl2ps/gl2ps.c`（`gl2psGetVertex` 读 feedback 缓冲 p[3..6] 作 RGBA，:2265-2288、:6208）
- **现象**：导出用 `GL_3D_COLOR` feedback 捕获**固定功能管线的逐顶点主色**，而 `Qwt3DSurface` 走 GLSL 3.3 Core 着色器、从不写 `gl_FrontColor`，全模块也无 `glColor*` 调用（仅 `qwt3d_io_gl2ps.cpp:347/388` 为文本颜色保存/恢复）——feedback 捕获到未定义值，实测 alpha=0。
- **影响**：PDF 导出 59 个 ExtGState 全部 `/ca 0`，SVG 同样；**曲面在所有矢量导出中不可见**（只剩坐标轴/网格等 2D 覆盖元素）。字节级验证。
- **证据**：`docs/zh/tech-report/3d-surface-pipeline.md` §5.8/§6；`docs/assets/algorithms/surface3d_bench.cpp`（导出矩阵）、`surface3d_results.csv`。
- **修复建议**：导出前按数据值经 colormap 逐顶点计算颜色并显式喂给 gl2ps（绕开 feedback 取色），或在 feedback 模式下临时以 `glColor4f` 逐图元设置颜色；同时补一个"导出像素非空"的回归测试。

### 4. Qwt3DIO 格式注册表大小写敏感：savePixmap(file, "PNG") 静默失败

- [x] 已修复
- **位置**：`src/plot3d/qwt3d_io.cpp`（`FormatCompare2` 用 `==` 精确比较；光栅格式键注册为小写）；触发处 `examples/3D/qwtplot3d/MainWindow.cpp:382` 传大写 `"PNG"`
- **现象**：`Qwt3DIO::save(file, "PNG")` 查表失败，静默 `return false`，无任何提示；示例的保存功能实际从未成功。
- **影响**：示例级功能失效 + 库接口易踩坑（大小写不一致即静默失败）。
- **证据**：`docs/zh/tech-report/3d-surface-pipeline.md` §6.9。
- **修复建议**：`Qwt3DIO::save/load` 入口对 format 做大小写归一（或改用 `compare(..., Qt::CaseInsensitive)`）；失败时至少 qDebug 警告；修示例传参。

---

## P1 性能缺陷与行为陷阱

### 5. argMinMaxAVX2 返回前缺 VZEROUPPER

- [x] 已修复
- **位置**：`src/core/qwt_simd_argminmax.cpp:142-200`（`argMinMaxAVX2`，返回前无 `_mm256_zeroupper`）
- **现象**：在默认编译标志的 TU 中（库 shipped 形态），单次调用的固定开销约 **325 ns**（≈1695 周期，rdtsc 定位在水平归约尾声段 252.4/246.7 ns；用 `/arch:AVX2` 编译同一 TU 则仅 5.2 ns）；且调用后同核的上半脏状态会拖慢后续重 SSE 的标量代码——流式生产循环（MSVC `std::sin` 密集）实测 append 段 0.181 → 1.913 ms（约 10×），补 `_mm256_zeroall()` 后回到 0.149 ms。
- **影响**：MinMax 桶降采样每帧调用 2W 次（900px 画布 ≈1800 次/帧），325 ns/次 ≈ 0.6 ms/帧的纯派发损耗；单线程"生产+渲染"流式场景下 SIMD 反而使数据生产慢一个量级（`setRawSamples` 路径实测 ring_raw 0.26×）。跨核部署（独立采样线程）不受外部性影响。
- **注意**：短通用 SSE 链探针测不出外部性（chain_after_dirty 14.20 ≈ chain_after_clean 13.87 ns），惩罚大小依赖后续代码的寄存器使用形态；指令级机理未完全闭环——两篇报告已对齐口径。
- **证据**：`docs/zh/tech-report/simd-argminmax.md` §4.5/§5.3；`docs/zh/tech-report/realtime-streaming.md` §5.4；`docs/assets/algorithms/simd_bench.cpp`（rdtsc 分段探针）、`streaming_micro_simd.cpp` + `streaming_micro_results.txt`（单变量隔离实验）。
- **修复建议**（二选一或并用）：① 给 `qwt_simd_argminmax.cpp` 单独加 `/arch:AVX2`（MSVC）/`-mavx2`（GCC/Clang，配 target-attribute 或函数级 pragma）——同时消除 325 ns 内部尾声；② `argMinMaxAVX2` 所有 return 前加 `_mm256_zeroupper()`。SSE4.2 路径仅 128-bit，无需改动。

### 6. 箱线图"均值标记"画在中位数位置

- [x] 已修复
- **位置**：`src/plot/qwt_plot_boxchart.cpp:708-715`（`meanVisible && meanSymbol` 分支用 `valueMap->transform(sample.median)`）
- **现象**：均值标记功能实际绘制的是中位数；`QwtBoxSample`（`src/core/qwt_samples.h:362-443`）根本没有 mean 字段，`QwtBoxStatisticsCalculator` 也不计算均值。
- **影响**：API 语义欺骗——用户开启 mean 标记得到的是 median 的重复绘制。
- **证据**：`docs/zh/tech-report/boxplot-quantiles.md` §5。
- **修复建议**：要么给 `QwtBoxSample` 增加 mean 字段并在统计器中计算，要么移除 mean 标记 API；短期内至少改文档注明现状。属接口变更，需按 SemVer 评估。

### 7. clipCircle 切割点含端点保留：角点重复/零长度弧

- [x] 已修复
- **位置**：`src/core/qwt_clipper.cpp:324-341`（`cuttingPoints` 的端点保留逻辑）+ :277（配对循环）
- **现象**：圆与矩形角点重合等情形下切割点重复，产生零长度弧（实测角点重复案例对称差 6.283185 = 2π）；另有奇数切割点时 `angles[1]` 越界读的**分析级风险**（release QVector 无边界检查；1014 组用例未观测到触发，列 P2 跟踪亦可）。
- **影响**：极坐标网格在圆心贴角时绘制错误弧段。
- **证据**：`docs/zh/tech-report/polygon-clipping.md` §4.3/§5；`clipper_experiment.py` 定向用例。
- **修复建议**：切割点去重（容差内合并）；配对循环前断言/防御偶数个切割点。

### 8. MinMax 桶 NaN 预扫描只查 y：x-NaN 伪顶点与 Inf 穿透

- [x] 已修复（v7.5.0：预扫描谓词改为 `!isfinite(y)` 拦截 Inf；x 的非有限值由 SIMD 路径的每桶后置校验拦截、失效桶用标量循环重跑，标量桶循环对 x、y 双坐标 `!isfinite` 跳过——与通用路径语义对齐。预扫描保持只读 y 数组：实测双坐标预扫描在 1M 干净点上使 lttb 帧时间回归 +11%，超出阈值，故采用桶级后置校验方案）
- **位置**：`src/plot/qwt_point_mapper.cpp`（`qwtMinMaxBucketReduce` 的 NaN 预扫描，约 :597-604，只扫 y 数组）
- **现象**：x 含 NaN 而 y 正常时走 SIMD 快速路径，NaN 的 x 经 `qRound(NaN)=0` 变成屏幕坐标 0——输出多边形含伪顶点（实测 (0,21)）；±Inf 同样穿透（`qRound(+Inf)=INT_MIN`）。
- **影响**：x 脏数据（少见但存在：传感器时间戳坏值）时曲线画到画布左缘/溢出坐标。y-only 预扫描是性能与正确性的折中，但未在文档注明。
- **证据**：`docs/zh/tech-report/nan-rendering.md` §2.3/§3（探针 xnan_lttb_fakeLeft=1）；`docs/assets/algorithms/nan_mapper_check.cpp`。
- **修复建议**：低成本方案是文档注明"x 必须无 NaN/Inf"；彻底方案是预扫描同时查 x（同一次顺序读，代价近乎不变）或对变换结果做 isfinite 过滤。

### 9. colormap 预设色标数据缺陷（inferno/turbo 位置压缩，viridis/cividis 偏离参考）

- [x] 已修复
- **位置**：`src/core/qwt_colormap_preset.cpp`（inferno/turbo/viridis/cividis 的色标表）
- **现象**：inferno 的 9 个色标颜色精确取自 matplotlib 参考曲线 t∈[0, 0.6994] 段、turbo 近似取自同段（最近距离 0.70–2.27 ΔE00），但被摆在 0–1 全区间位置上（色标位置压缩）；viridis/cividis 中间色标与 mpl 同名曲线同位置偏差最大 11.4–17.2 ΔE00（plasma 精确、rainbow 属定义不同）。
- **影响**：inferno/magma 的 ΔE00 变异系数从 mpl 原版的 0.332/0.348 劣化到 0.587/0.582（感知均匀性降级）；与 matplotlib 图并排比对时同名 colormap 颜色不一致。
- **证据**：`docs/zh/tech-report/colormap-color-science.md` 表 5/§4.7（含逐色标溯源探针）；`docs/assets/algorithms/colormap_experiment.py`。
- **修复建议**：按 mpl 官方 RGB 表在全 [0,1] 域重新采样色标；修复后需重跑 `colormap_experiment.py` 更新报告表 2/表 5 数字（报告已注明此依赖）。

---

## P2 注释不符、死代码与低危项

### 10. Qwt3DSurface::setResolution docstring 与实现不符

- [x] 已修复
- **位置**：`src/plot3d/qwt3d_surface.cpp:388-401`
- **现象**：注释称 "without rebuilding the VBO"，实现置 `m_vboDirty = true`（分辨率只影响索引，但顶点数组全量重建）。
- **修复建议**：改注释如实描述；或实现真正的索引-only 更新。

### 11. qwt_colormap_preset.h 注释与实现不符

- [x] 已修复
- **位置**：`src/core/qwt_colormap_preset.h:131`（"returns -1 if not found"）vs `.cpp:366`（回退返回 `Viridis`）
- **修复建议**：改注释为"未知名回退 Viridis"；或按注释实现返回 -1（属行为变更，倾向改注释）。

### 12. QwtBoxStatisticsCalculator 的 vestigial 实例成员

- [x] 已修复
- **位置**：`src/core/qwt_box_statistics.h:51-63, 105-106`（构造函数、`setWhiskerMethod()/setWhiskerCoefficient()`、`m_method/m_coefficient`）
- **现象**：实例配置成员不被任何静态计算路径消费（计算接口全部为静态方法），属死状态。
- **修复建议**：删除实例接口，或让 `calculate*` 真正消费实例配置（后者是行为增强，需设计决策）。

### 13. Qwt3D 光照/材质为存值 stub（功能未完成，非错误）

- [x] 已处理
- **位置**：`src/plot3d/qwt3d_lighting.cpp:71/96/108`（TODO 注释）；着色器光照 uniform 硬编码（`qwt3d_plot` 上传路径 :1104-1112 附近）
- **现象**：光照四 setter 与 illuminate/blowout 只存值不生效；着色器用硬编码参数（光源 (0,0,10)、白、shininess 32）；主题系统 5 种光照预设实际只有开/关两态。
- **说明**：教程 `3d-plot.md`（中英）已按现状如实描述并链接技术报告；此项是功能路线问题，修复=把存值接入 shader uniform。

### 14. QwtAbstractScaleDraw::draw 不去重刻度（行为特性，低危）

- [x] 已评估
- **位置**：`src/plot/qwt_abstract_scale_draw.cpp`（按 tickType 列表逐条绘制）
- **现象**：日期引擎在特定预算下主/中/次刻度值重合（如 10 年场景次刻度 12/12 与主刻度重合），绘制端不去重，重合刻度被叠加绘制两次。
- **说明**：视觉影响轻微（重绘同位置），列此供知悉；如修复需在 draw 或引擎端去重。证据：`docs/zh/tech-report/scale-engine.md` §5。

---

## 修复任务立项提示

- 每项修复后：跑对应报告的实验脚本更新受影响数字（第 5、9 项明确需要），撤下教程中已加的"已知问题"警示（第 1 项：`boxchart.md` 中英两版）。
- 构建配方与基准复现方式见 `docs/assets/algorithms/*.cpp` 头部注释；`build-release/` 已有 Release 库。
- 本清单与 `docs/zh/tech-report/`（中文版）、`docs/tech-report/`（英文版）各报告的失效模式清单章节保持同源，修一项请同步勾掉此处复选框。
