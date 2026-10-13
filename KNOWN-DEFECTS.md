# 源码缺陷清单（第二轮 · 技术报告校对期间实证发现，待立项修复）

2026-10-13 对 `docs/zh/tech-report/`（中文）与 `docs/tech-report/`（英文）的 12 篇技术报告做逐篇
"文章 ↔ 最新代码"校对（每篇一个独立校对 Agent，逐条核对文件/行号/符号/常量/行为断言）时，
在核对过程中发现以下源码与实验资产问题。**本轮只做校对与汇总：未修改 `src/`、`tests/`、`examples/`、
`docs/assets/`**（文章本身的不一致已就地修正并提交）。第一轮清单（14 项，全部已修复）见
`KNOWN-DEFECTS-round1-fixed.md`。

严重度定义：**P0** 正确性缺陷；**P1** 覆盖不全的行为陷阱、或使已发布数字/结论不可信的分析缺陷；
**P2** 注释/文档不符、死代码、低危与接口契约问题。

复核标记：**✅** = 本轮我本人读过源码确认；**📋** = 来自校对 Agent 的定位（附文件:行号，未二次复核）。

---

## P1 覆盖不全的行为陷阱与分析缺陷

### 1. ✅ `FilterPointsLTTB`（真 LTTB）原始指针路径的 NaN/Inf 判定只查 y

- [ ] 已修复

- **位置**：`src/plot/qwt_point_mapper.cpp:915-922`（`qwtLttbReduce` 内 `mapSample` 的 `useRaw` 分支）；
  对照已修好的 MinMax 路径 `:682-687`（标量双坐标）、`:725-731`（`!std::isfinite(y)` 预扫描）、`:747-753`（桶极值 x 后置校验）
- **现象**：raw 快路径以 `std::isnan(y)` 判样本有效——只拦 y 的 NaN、不拦 y 的 ±Inf，对 x 完全不做有限性检查。
  x=NaN 的样本若成为可见区间首/末有效样本会直接输出 `(NaN, y)` 顶点；它参与第一趟桶均值会把 `avgX[b]`
  污染成 NaN，从而使该桶（及引用它的下一桶）的面积比较恒假、静默不输出代表点。
- **影响**：v7.5.0 起 `FilterPointsLTTB` 是**默认**绘制属性（真 LTTB），即第一轮 #8 的同类问题
  （x-NaN 伪顶点、`qRound(NaN)=0`）在默认路径上仍然存在；MinMax 桶修好、LTTB 漏修。
- **证据**：nan-rendering 与 curve-downsampling 两篇的校对 Agent **各自独立**定位；`tests/nan_curve_test`
  中两个 `lttb_*` 用例实际配置的是 `MinMaxReduce`，新默认路径无 NaN 回归覆盖。
- **修复建议**：raw 分支改用 `!std::isfinite(y)`，并按 MinMax 的做法对桶代表样本做 x 后置校验（命中时
  标量重算该桶、标量循环双坐标 `!isfinite` 跳过）；补 `LTTBReduce` 的 x-NaN/±Inf 回归测试。
  修复后需重跑并同步 `nan-rendering.md`（§3.5/§5 的遗留提示）与 `curve-downsampling.md`（5.4 节）。

### 2. ✅ gl2ps 矢量导出修复未覆盖 bar / line3d / waterfall

- [ ] 已修复

- **位置**：`src/plot3d/qwt3d_bar.cpp`（draw 末尾）、`src/plot3d/qwt3d_line3d.cpp:821` 附近、
  `src/plot3d/qwt3d_waterfall.cpp:1075` 附近——三者均无 `ctx.vectorExport` / `isVectorExportActive()` 分支
- **现象**：第一轮 #3 只给 `Qwt3DSurface`、坐标系网格/轴线、颜色图例加了即时模式导出路径；
  `Qwt3DBar`/`Qwt3DLine`/`Qwt3DWaterfall` 在 feedback 回放时仍走着色器路径，feedback 缓冲记录的是
  未定义的固定功能顶点色（与曲面修复前同机理）。
- **影响**：以柱/线/瀑布为内容的 PDF/SVG 导出颜色不可用（原机 alpha=0 不可见，其他驱动为杂色）。
- **证据**：`grep -rn "vectorExport" src/plot3d` 仅命中 axis / colorlegend / coordsys / plot / surface 五个文件。
- **修复建议**：为三个 item 各加导出分支（几何数组多已存在：bar 的柱体、line3d 的 Tube/Lines、
  waterfall 的 Ribbon/Wireframe/Tube），复用 `qwt3dEmitVerticesForVectorExport()`；补对应回归检查。

### 3. ✅ `colormap_experiment.py` 的 farMin 只比较"相距恰好 32 级"的色对

- [ ] 已修复

- **位置**：`docs/assets/algorithms/colormap_experiment.py:353-363`（`min_pairwise_de`）
- **现象**：`d = ciede2000(lab[i], lab[i + gap:])[0]` 取广播结果的第 0 个元素后 `np.argmin(d)` 恒为 0，
  每轮实际只算 `dE(LUT[i], LUT[i+32])`，从不搜索更远配对；函数 docstring 与结果文件表头写的是
  "at least gap apart"（≥32 级），语义不符。所有输出的混淆位置都恰好相差 0.125 级距可为旁证。
- **影响**：colormap 篇表 3/§4.5 的 farMin 数字（"可区分性下界"）被低估为"固定 32 级"口径：
  按"≥gap"全对重算，`spectral` deutan 0.368→0.000、`rdylgn` protan 1.332→0.154、`rainbow`/`hsv`
  原始 6.529→0.577 等，部分结论（如"发散族在 CVD 下可区分"）会加强。
- **证据**：校对 Agent 定位 + 自写 CIEDE2000 全对重算（报告里 numeric 核对已做）。
- **修复建议**：改为对每个 i 在 `i+gap..n-1` 全段取最小并按 (i, j) 记录；重跑脚本更新
  `colormap_results.txt`、文章表 3/§4.5（本轮文章已按**实际行为**改写为"相距 32 级"，修复脚本后需再改回"≥32 级"口径）。

### 4. ✅ `spline_experiment.py` 自然三次样条的 Thomas 前代副对角错位

- [ ] 已修复

- **位置**：`docs/assets/algorithms/spline_experiment.py:118,120`（`moments_open_natural`）
- **现象**：约化系统第 j 行副对角元应为 `a[j]=h_j`（第 107 行注释亦如此自述），前代循环却用
  `a[j-1]=h_{j-1}`；得到的系数矩阵不对称（副对角 `h_{j-1}`、主对角 `2(h_j+h_{j+1})`），
  仅当相邻间距相等时二者重合（均匀参数化、闭合圆等稠密求解不受影响）。
- **影响**：spline-family 篇里非均匀参数化的数字不可信——发夹链的数字可被该错误求解逐位复现
  （|S′(P1)|=1.544/0.400/0.209；改成 `a[j]` 后为 1.192/0.400/0.114），S 弯 Cubic[chordal] 例的
  收敛阶 3.96 会被修正为 4.39。
- **证据**：校对 Agent 定位 + 独立对照（对称矩阵 np.linalg.solve / 与库实现对照）。
- **修复建议**：`a[j-1] → a[j]`（`dp` 同步）后重跑，更新 `spline_experiment.py` 的数字与
  `spline-family.md` §5.2/§5.4 表格（本轮已在文中标注这些行的"存疑"状态）。

---

## P2 注释/文档不符、死代码与低危项

### 5. ✅ `clipCircle` 的 radius-0 契约注释未更新

- [ ] 已修复

- **位置**：`src/core/qwt_clipper.cpp:450-459`（`QwtClipper::clipCircle` 的 Doxygen）vs `:265-273`（实现）
- **现象**：注释仍写 "When the circle is completely inside the clip rectangle, an interval [0.0, 2π] is
  returned"，而半径 0 时无切割点、`QRectF::contains()` 拒绝零宽包围盒 → 实际返回空集。
- **影响**：API 契约与注释不符；第一轮 #2 的修复说明里"注释已澄清"不成立（本轮报告已按现状更正，
  并列出本项）。`tests/clipper_test` 已把半径 0 的实测行为固化为期望空集。
- **修复建议**：把 radius ≤ 0 的语义写进 Doxygen（或对 radius ≤ 0 显示定义返回空集），保持测试一致。

### 6. ✅ `QwtClipper` 的 `#if 0` 早退块引用未声明标识符

- [ ] 已修复

- **位置**：`src/core/qwt_clipper.cpp:156-159`（`clipPolygon(Polygon& points1, ...)` 内）
- **现象**：块内 `return polygon;` 的 `polygon` 不在该函数作用域（旧签名残留），一旦把 `#if 0` 改成
  `#if 1` 立即编译失败。
- **影响**：死代码；任何"启用包围盒早退"的尝试会先撞编译错误（报告 §3.4 讨论了该早退的禁用取舍）。
- **修复建议**：删除该块，或按当前签名改为 `return points1;` 并补包围盒早退的性能验证。

### 7. 📋 `clipCircle` 负半径输入未定义（契约问题）

- [ ] 已修复

- **位置**：`src/core/qwt_clipper.cpp:266-273`（回退）与 `:337,:349`（严格 `< radius`）
- **现象**：`radius < 0` 时 `qAbs(...) < radius` 恒假 → 无切割点；随后 `QRectF(0,0,2r,2r).moveCenter(pos)`
  为负尺寸包围盒，Qt 的 `contains(QRectF)` 归一化负宽高后可能判"完全在内" → 返回整圆 `[0, 2π]`。
- **影响**：负半径无输入契约，行为（整圆）与直觉（空集/异常）不符。
- **修复建议**：入口对 `radius <= 0` 显式定义（建议返回空集）并加用例。

### 8. ✅ `QwtScaleDiv` 的构造函数绕过 `setTicks()` 的排序+去重

- [ ] 已修复

- **位置**：`src/core/qwt_scale_div.cpp:53-58`（`QwtScaleDiv(const QwtInterval&, QList<double> ticks[])`）
  与 `:70-75`（bounds + ticks 版）直接 `m_ticks[i] = ticks[i]`；`setTicks()` 在 `:365-378` 才归一化
- **现象**：v7.5.1 的"刻度列表排序且无重复"保证只对走 `setTicks()` 的路径成立；线性/对数引擎经构造函数
  组装 division，从不归一化（现有引擎正常路径不产出列表内重复，故未观测故障）。
- **影响**：接口语义不一致；以构造函数直接传入含重复的列表时 `ticks()` 原样返回。
- **修复建议**：构造函数改调 `setTicks()`（或共享同一个归一化 helper）。

### 9. 📋 日期引擎类注释与 `setMaxWeeks()` 文档同实现不符

- [ ] 已修复

- **位置**：`src/plot/qwt_date_scale_engine.cpp:624`（"classifies intervals > 4 weeks as >= Qt::Month"）
  与 `:718-731`（`setMaxWeeks` 文档）vs `:769-775`（实现）
- **现象**：Month 分类实际需 `weeks > maxWeeks` **且** `days > 4*7*maxSteps`；默认预算下 >224 天已蕴含
  `weeks ≥ 33`，因此 `maxWeeks < 33` 时该参数几乎无效（scale-engine 篇 §8.1 的示例注释曾因此写错，
  本轮已改）。
- **修复建议**：改正注释/文档，或让 `maxWeeks` 真正参与 Month 门槛。

### 10. 📋 对数引擎线性回退把"十进"边距当数据单位

- [ ] 已修复

- **位置**：`src/core/qwt_scale_engine.cpp:743`（`autoScale` 的比值 < base 回退），配
  `qwt_scale_engine.h:190` 的 `@warning`
- **现象**：回退时把 `lowerMargin()/upperMargin()`（十进单位）直接 `setMargins` 给临时
  `QwtLinearScaleEngine`（数据单位）——"1 个十进"的边距在回退路径上只外扩 1 个数据单位。
- **影响**：对数窄区间（比值 < base 静默变线性）的边距语义与文档不一致；上游 Qwt 同源。
- **修复建议**：回退时把边距换算成数据单位（或明确文档化该行为）。

### 11. 📋 箱线图 `boundingRect` 不含 q1/q3/mean

- [ ] 已修复

- **位置**：`src/core/qwt_series_data.cpp:172-175`（`qwtBoundingRect(QwtBoxSample)` →
  `QRectF(position, whiskerLower, 0, wu−wl)`），经 `src/plot/qwt_plot_seriesitem.cpp:114-117` 供自动缩放
- **现象**：Percentile `c > 25`（文档化语义，`tests/box_statistics_test` 明确断言 c=49 时 `wl > q1`、
  `wu < q3`）或 SD/SE 误差棒语义下，q1/q3/均值标记位于 boundingRect 之外。
- **影响**：开启轴自动缩放时箱体/均值标记可能被裁到轴范围外（须线区间大于箱体时）。
- **修复建议**：并入 q1/q3（以及有限 mean）的最小/最大外扩，或文档注明该口径。

### 12. 📋 Percentile 系数未做范围校验（越界系数 → `quantile()` 越界索引）

- [ ] 已修复

- **位置**：`src/core/qwt_box_statistics.cpp:24-40`（`quantile`）、`:131-135`（Percentile 分支）
- **现象**：`p = c/100` 直接进入 `index = p(n−1)`，无 clamping：`c > 100` → `sorted[6]`（n=4 时）越界、
  `c < 0` → 负索引；`QVector::operator[]` 在 release 无边界检查。`c ∈ (50,100)` 时
  `wl = q(c/100) > wu = q(1−c/100)`，`isValid()` 恒 false 且全部点计为离群（与第一轮 #1 同症状，仅限越界系数）。
- **修复建议**：Percentile 分支对系数做 `(0, 50]` 校验（钳制或返回无效样本），并在文档中写明有效范围。

### 13. 📋 `StandardError` 的分母与 `sd`/`mean` 的有限样本计数不一致

- [ ] 已修复

- **位置**：`src/core/qwt_box_statistics.cpp:151`（`se = sd/sqrt(sortedData.size())`）vs `:52-96`（跳过 NaN/Inf 并按有限值计数）
- **现象**：直接以含 NaN/Inf 的脏数据调用 `calculate()`（不经 `calculateFromRaw`）时，`sqrt(n)` 用的是
  原始长度、而 sd 用的是有限样本数（ddof=1 的 count−1）。
- **影响**：违约调用下标准误偏小；正常入口（先过滤）不可见。
- **修复建议**：统一用有限样本计数，或让 `calculate()` 明确要求已过滤输入（断言/文档）。

### 14. 📋 `hasMean()`/统计器填 mean 的文档对空输入不成立

- [ ] 已修复

- **位置**：`src/core/qwt_samples.h:392-399`（文档 "always fills the mean"）vs
  `src/core/qwt_box_statistics.cpp:113-114`（空输入直接返回 `QwtBoxSample(position)`，mean 保持 NaN）
- **影响**：空输入（含全 NaN 过滤后为空）时 `hasMean()==false`、均值标记不绘，与文档/文章表述不符；
  `tests/box_statistics_test` 的 `emptyInputIsZeroed` 未覆盖 mean。
- **修复建议**：改文档（"为空时 mean 为 NaN"）或让空样本显式标记；补测试断言。

### 15. 📋 `qwtReflectIndex` 对长度 1 的区间死循环（当前不可达）

- [ ] 已修复

- **位置**：`src/plot/qwt_curve_smoothing.cpp:37-43`
- **现象**：`hi = lo + 1` 时低端镜像到 `lo+1`、高端再镜像回 `lo-1`，while 条件恒真；唯一调用方
  `qwtConvolveSegment` 由 `:89` 的 `i - lo >= window`（window ≥ 3）守卫，故当前不可达。
- **修复建议**：加 `if (hi - lo <= 1) return lo;` 缩短路径（防御性）。

### 16. 📋 平滑的基线（截断箱式均值）与低通（镜像延拓）段端处理不一致（存疑）

- [ ] 已修复

- **位置**：`src/plot/qwt_curve_smoothing.cpp:252-256`（基线截断 + 重归一化）vs `:35-45`（低通镜像）
- **现象**：段端点处基线是中位数偏置的截断均值（线性斜坡端点误差 ≈ m/2·slope），低通却是偶延拓；
  陡峭边缘端点附近的 `|d_i|` 被放大，可能越过 `τσ̂` 而把本应平滑的噪声恢复。
- **修复建议**：端点统一用镜像/反射统计，或对首末 window/2 点用单侧核。

### 17. 📋 3D 光照相关注释仍写 "stored on CPU for future shader uniform upload"

- [ ] 已修复

- **位置**：`src/plot3d/qwt3d_lighting.cpp:67-68, 100-101, 112-113, 251-255, 261-270`；
  `src/plot3d/qwt3d_theme.cpp:504-505`
- **现象**：v7.5.1 已把灯光/材质分量与 shininess 上传为 uniform（`qwt3d_surface.cpp:1212-1253`），
  这些注释与函数头摘要未同步。
- **修复建议**：改注释为"存值并上传 uniform"，指向上传点。

### 18. 📋 `qwt3dEmitVerticesForVectorExport()` 不恢复 GL 状态（低危）

- [ ] 已修复

- **位置**：`src/plot3d/qwt3d_io_gl2ps.cpp:333-338`
- **现象**：公共导出辅助函数进入时 `glUseProgram(0)` 并关闭 attrib 数组 0-2，返回不恢复；库内调用点
  都在 feedback 回放且各自重 bind，未观测故障；库外调用者可能受影响。
- **修复建议**：保存/恢复 program 与 attrib 使能状态，或在文档写明前置条件。

### 19. 📋 TEX 文本导出的第二遍不渲染（存疑）

- [ ] 已修复

- **位置**：`src/plot3d/qwt3d_io_gl2ps.cpp:264-284`
- **现象**：第一遍用同步的 `plot->grabFramebuffer()`（`:248`）；TEX 第二遍在 `gl2psBeginPage(GL2PS_TEX…)`
  与 `gl2psEndPage()` 之间只调 `plot->update()`（异步投递），中间无渲染 → `.tex` 页疑为空。
- **修复建议**：第二遍同样改用 `grabFramebuffer()`（或以 `repaint()` + 事件泵同步）。

### 20. ✅ `QwtPlotOpenGLCanvas` 的 FBO 用应用级 DPR（混合 DPI 下错误）

- [ ] 已修复

- **位置**：`src/plot/qwt_plot_opengl_canvas.cpp:198`（`QwtPainter::devicePixelRatio(nullptr)` → 回退到
  应用级 DPR，`qwt_painter.cpp:1631-1633`）与 `:233-236`（`fboSize`/`scale`）
- **现象**：`QOpenGLWidget` 内部 FBO 按**部件** DPR 建立，而 Qwt 的 FBO 用应用级 DPR；画布位于
  DPR 与系统 DPR 不同的屏幕时内容被错误缩放/裁切（对照 `qwt_plot_canvas.cpp:195` 正确传部件）。
- **影响**：混合 DPI 多屏拖动窗口时 GL 画布渲染错误（单 DPI 环境不可见）。
- **修复建议**：改用 `devicePixelRatioF()`（部件级）计算 FBO 与缩放。

### 21. ✅ `autoReplot` 的文档与实现相反

- [ ] 已修复

- **位置**：`src/plot/qwt_plot.cpp:135,198`（实现默认 `true`）vs `:403-412`（文档 "set to false by
  default"）与 `:683-684`（同源旧注释）
- **影响**：照文档"默认关闭"编写的代码会多触发整帧 replot（流式场景的性能陷阱，realtime-streaming
  篇 §6 第 8 条据此论述）；两篇报告（raster-vs-opengl、realtime-streaming）的校对 Agent 各自命中。
- **修复建议**：改文档为"默认开启"。

### 22. 📋 `QwtPlotCanvas` 的 `PaintAttribute` 默认值注释不完整

- [ ] 已修复

- **位置**：`src/plot/qwt_plot_canvas.h:52-56` vs `src/plot/qwt_plot_canvas.cpp:59-61`
- **现象**：注释称默认仅 `BackingStore` + `Opaque`，构造函数实际还开启 `HackStyledBackground`。
- **修复建议**：补上。

### 23. 📋 陈旧的 `QwtPlotCanvas::OpenGLBuffer` 文档引用

- [ ] 已修复

- **位置**：`src/plot/qwt_plot_opengl_canvas.h:48,51`、`src/plot/qwt_plot_glcanvas.h:58`
- **现象**：`@sa` 指向本 fork 已不存在的符号（`grep -rn OpenGLBuffer src/` 无定义）。
- **修复建议**：删除或改指向现行 API。

### 24. 📋 DirectPainter 文档推荐 Qt5+ 已废弃的 `Qt::WA_PaintOutsidePaintEvent`

- [ ] 已修复

- **位置**：`src/plot/qwt_plot_directpainter.cpp:176-178`（建议）vs `:211-215`（只在
  `#if QT_VERSION < 0x050000` 下检查该属性）
- **现象**：Qt5+ 上该建议是死路径（Qt6 已移除该属性），照做无效果。
- **修复建议**：删除该建议或标注版本。

### 25. ✅ GCC/Clang 默认标志下 AVX2 内核不可达（v7.5.1 修复只在 MSVC 生效）

- [ ] 已修复

- **位置**：`src/core/qwt_simd_argminmax.cpp:143,149-150`（`#if defined(__AVX2__) || (MSVC && x86)`）、
  `src/core/qwt_simd_argminmax_avx2.cpp:14`、`src/core/CMakeLists.txt:82-86`
- **现象**：派发方 TU 在 GCC/Clang 默认标志下 `__AVX2__` 未定义 → 宏塌缩为 `argMinMaxSSE42`（默认无
  `-msse4.2` 时再塌缩为标量）；CMake 只给内核文件 `-mavx2`，于是运行时 `kSimdLevel == AVX2` 仍指向标量，
  AVX2 TU 成为死代码。要生效需打包方给**整个**构建加 `-mavx2`/`-march`（此时派发方守卫为真、可用）。
- **影响**：Linux/GCC 发行包在默认编译标志下享受不到 v7.5.1 的性能修复（与修复前一致，非回归）；
  `CHANGES.md` 的 "GCC/Clang 用 -mavx2" 表述容易误导。
- **修复建议**：把派发方的可用性判定与自身编译标志解耦（如按 `__x86_64__` 等平台宏 + CMake 探测编译器
  是否支持 `-mavx2`，不支持则退回 SSE 路径），或在 CHANGES/报告中写明"GCC/Clang 需全局 `-mavx2`"。

### 26. 📋 amalgamate / staticExample 构建仍带旧尾声惩罚（文档未提示）

- [ ] 已修复

- **位置**：`tools/amalgamate/QWTAmalgamTemplate.cpp:16-17`、`src-amalgamate/QwtPlot.cpp:70,247`、
  `examples/staticExample/CMakeLists.txt:12-15`
- **现象**：两个 `.cpp` 内联进同一 `QwtPlot.cpp`，该 TU 未加 `/arch:AVX2` 且 MSVC 分支守卫为真 →
  内核在基线 TU 编译，AVX-SSE 过渡/溢出重载与缺 VZEROUPPER 的问题照旧（`noinline` 在单 TU 内无意义）。
- **影响**：用 amalgamate 单文件构建的用户享受不到性能修复。
- **修复建议**：在 amalgamate 说明/CHANGES 中提示；或在模板里给该片段加编译标志/`#pragma` 说明。

### 27. 📋 `qwt_simd_argminmax.h` 的 NaN 契约与实现不符

- [ ] 已修复

- **位置**：`src/core/qwt_simd_argminmax.h:26`（"NaN values are naturally ignored …"）
- **现象**：该表述只对索引轨道成立；值轨道 `minpd/maxpd` 的 src2 规则会用 NaN 覆盖累加器
  （`qwt_simd_argminmax_avx2.cpp:74`、`qwt_simd_argminmax.cpp:94`），实测 12/33 单元出错（报告 2.3 节）。
  库内靠 mapper 预扫描兜底，直接调用方按注释使用会踩坑。
- **修复建议**：改注释为"NaN 输入必须自行预过滤"。

### 28. 📋 MSVC ARM64 无法编译该内核文件（存疑）

- [ ] 已修复

- **位置**：`src/core/qwt_simd_argminmax.cpp:24-35,78`（SSE4.2 守卫只看 `_MSC_VER>=1700`；`detectSimdLevel`
  用 `__cpuid`）
- **现象**：MSVC ARM64 上 `__cpuid`/`_mm_*` 不可用 → 编译失败而非回退标量（未实测，按守卫推断）。
- **修复建议**：把守卫限定到 x86 平台宏；非 x86 直接走标量。

### 29. 📋 `QwtSplineBasis` 头注释称首末点不在曲线上（实现相反）

- [ ] 已修复

- **位置**：`src/plot/qwt_spline_basis.h:39` vs `src/plot/qwt_spline_basis.cpp:96-97,112-114,152-153,209-210`
- **现象**：注释 "The end points of the spline do not match the original points"，实现却
  `moveTo(pd[0])` 并以 `cubicTo(..., pd[n-1])` 结束（文章 §3.5 的描述与实现一致）。
- **修复建议**：改注释。

### 30. 📋 `qwt_spline.cpp` 的 `@sa` 指向不存在的类

- [ ] 已修复

- **位置**：`src/plot/qwt_spline.cpp:804`（`QwtSplineBezier::toPolygon()`；实际类为 `QwtBezier`）
- **修复建议**：修正引用。

### 31. ✅ `qwt_spline.h` 自包含

- [ ] 已修复

- **位置**：`src/plot/qwt_spline.h:31`（`#include "qwt_spline.h"`，被自身 include guard 吞掉）
- **影响**：无行为影响，编辑残留。
- **修复建议**：删除。

### 32. 📋 `figures.py` 面板 (d) 的两条曲线图例互换

- [ ] 已修复

- **位置**：`docs/assets/algorithms/figures.py:491-493`（`res = poly_smooth(excerpt, gm, 3.0)` 标为
  "pure low pass"，`pure = poly_smooth(excerpt, gm, 0.0)` 标为 "low pass + preservation"）
- **影响**：curve-smoothing 篇 §5.1 用该图展示特征保护效果，读者会把结论读反。
- **修复建议**：交换两处 label 并重绘该图。

### 33. 📋 `downsampling_experiment.py` 的 band-limited 信号缺 2π 因子

- [ ] 已修复

- **位置**：`docs/assets/algorithms/downsampling_experiment.py:270-277`
- **现象**：docstring 写 "periods of 6.3 / 3.0 / 2.2 columns"，但 `np.sin(x/700)` 一族的周期是
  2π×700 ≈ 4398 采样 ≈ 39.6 列，三分量实际为 39.6/18.7/14.1 列（最高约 0.07 周期/列），
  并未检验"近奈奎斯特带限信号"这一宣称场景。
- **影响**：curve-downsampling 篇表 1 的 band-limited 行口径与描述不符（本轮文章已按实际改写）。
- **修复建议**：改为 `np.sin(2*np.pi*x/700)` 等并重跑，或改文档描述。

### 34. 📋 renderbench 批量对比只覆盖 5/6 种方法且第 6 种显示 "Unknown"

- [ ] 已修复

- **位置**：`examples/bench/renderbench/MainWindow.cpp:20-26`（6 个名字）、`:51,155`（`index < 5` 上界）；
  `examples/bench/renderbench/ControlPanel.cpp:48-54`（6 个选项）
- **现象**：`Run All (Compare)` 止于下标 5，`FilterPointsMinMaxLTTB` 不在批量对比内；单独运行该方法时
  状态栏/报告的方法名显示 "Unknown"。
- **修复建议**：上界改为方法数，补齐 methodName。

### 35. 📋 oscilloscope `SignalData::clearStaleValues` 末样本 off-by-one

- [ ] 已修复

- **位置**：`examples/2D/oscilloscope/SignalData.cpp:119-130`（`while (index < values.size() - 1)`）
- **现象**：页边界重建向量时，位于 `values.size()-1` 的最新样本永远不会被 append（例：[A,B,C,D] 且 A
  过界 → 只保留 A,B,C），每次翻页丢 1 个样本并造成一帧末端缺样。
- **修复建议**：循环上界改为 `values.size()`（或单独 append 末样本）。

---

## 受影响的报告与资产（修复后需同步）

| 修复项 | 需要重跑/同步的报告与资产 |
|---|---|
| 1（LTTB NaN） | `nan-rendering.md`、`curve-downsampling.md`（§3.5/§5.4 的遗留提示与数字）；`tests/nan_curve_test` 需补 LTTB 路径用例 |
| 2（bar/line/waterfall 导出） | `3d-surface-pipeline.md` §5.8/§6 第 5 条（当前只声明 surfave/网格/轴/图例已覆盖） |
| 3（farMin 口径） | `colormap_experiment.py` + `colormap_results.txt` + `colormap-color-science.md` 表 3/§4.5 |
| 4（样条求解） | `spline_experiment.py` + `spline-family.md` §5.2/§5.4 表格 |
| 5（radius-0 注释） | `qwt_clipper.cpp` 注释 + `clipper_test` 期望保持不变 |
| 25/26（AVX2 生效范围） | `CHANGES.md`/`CHANGES-CN.md`、`simd-argminmax.md`（GCC/Clang 与 amalgamate 的边界说明） |

本清单与 `docs/zh/tech-report/`、`docs/tech-report/` 各报告保持同源；修一项请勾掉对应复选框并同步上表。
