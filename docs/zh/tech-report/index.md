# 技术报告（Tech Report）

本目录收录 QWT 的**技术报告**：面向希望深入理解库内部原理的开发者的长篇技术文章，按照学术论文的标准撰写——有摘要、有推导、有可复现的实验数据、有参考文献。它回答的问题是"**为什么这样设计、性能到底有多好**"，而不是"如何使用"。

如果你只想快速画出想要的图，请阅读[使用指南](../use-guide/first-plot.md)——那里是面向用户的分步教程。

## 技术报告 vs 使用指南教程

| | 使用指南（use-guide） | 技术报告（tech-report） |
|---|---|---|
| 读者 | 想快速完成绘图任务的用户 | 想深入理解原理与性能的开发者、算法工程师 |
| 回答的问题 | "这个功能怎么用？" | "它为什么这样设计？数学依据是什么？实测表现如何？" |
| 行文风格 | 通俗易懂、循序渐进、可直接运行的完整示例 | 论文式：摘要 → 引言 → 推导 → 实验 → 讨论 → 参考文献 |
| 插图 | 示例程序运行截图、效果对比图 | matplotlib 数据图（实验结果）+ mermaid 结构图（流程/架构） |
| 数学公式 | 尽量避免，出现时只给结论 | 按需完整推导，TeX 语法（`$...$` / `$$...$$`） |
| 篇幅 | 一篇解决一个任务 | 不设下限，以论证完整为准 |

两者互相引用：教程在"深入了解"处链接技术报告，技术报告在"代码实践"处链接教程与示例程序。

## 写作标准

技术报告按发论文的标准要求，除遵守 [文档编写指南](../doc-writing-guide.md) 的通用规则（平台无关 Markdown、mermaid 优先、英文注释规范等）外，还必须满足：

1. **论文结构**：开头给出摘要（一段话概括问题与方法）与"核心结论"速览（要点列表）；正文按"引言 → 问题形式化 → 方法/推导 → 实验 → 讨论 → 小结 → 参考文献"组织；性质与定理给出证明或证明思路。
2. **实验可复现**：每个实验数字都能被复现。matplotlib 绘图脚本与原始数值统一放在 `docs/assets/algorithms/`（Python + matplotlib + numpy），文中注明脚本文件名。
3. **基准注明环境**：性能数据必须注明 QWT/Qt 版本、编译配置、画布尺寸与硬件类别，并声明"绝对数值随硬件变化、相对关系稳定"。
4. **图表分工**：数据图（曲线、散点、热力图、对比条形图）用 matplotlib 生成 PNG；流程图、架构图、时序图用 mermaid 内嵌。图片统一放 `docs/assets/algorithms/`，文件名以 `fig_` 开头。
5. **诚实的失效分析**：每篇报告必须包含方法/实现的失效模式清单，不回避负面数据。
6. **引用具体**：引用源码给出文件路径（如 `src/plot/qwt_point_mapper.cpp`），引用示例给出目录（如 `examples/bench/renderbench`）。

## 现有报告

### 渲染管线与性能

沿"数据量 → 视觉质量 → 后端选择 → 流式更新"的主线，覆盖 QwtPlotCurve 从数据到像素的整条路径与 3D 侧的 GPU 管线。降采样篇回答"点太多画不动怎么办"，平滑篇回答"画出来像噪声怎么办"，SIMD 篇展开降采样核心内循环的向量化实测，NaN 篇处理缺失数据的语义与代价，Raster/GL 篇给出画布后端的选型判据，流式篇把前面的因子组合成实时应用的帧预算模型。

| 报告 | 主题 | 关键源码 | 配套示例 |
|------|------|----------|----------|
| [LTTB 降采样算法原理与 QWT7 的 MinMax 桶实现](curve-downsampling.md) | 可视化带宽瓶颈、经典 LTTB 推导、MinMax 桶变体的三条性质、SIMD argmin/argmax、像素级保真度实验 | `src/plot/qwt_point_mapper.cpp`、`src/core/qwt_simd_argminmax.cpp` | `examples/bench/renderbench`、`examples/2D/nanperf` |
| [曲线渲染平滑：高斯卷积、Savitzky-Golay 回归与特征保护](curve-smoothing.md) | 偏差-方差分解、两种滤波核的完整推导（矩条件、噪声增益恒等式）、MAD 特征保护、端到端像素实验 | `src/plot/qwt_curve_smoothing.cpp` | `examples/2D/curvesmoothing` |
| [SIMD argmin/argmax 向量化基准](simd-argminmax.md) | AVX2 / SSE4.2 / 标量三路运行时派发实测：加速比-数组长度、缓存层级带宽与 roofline、桶大小敏感性与"SIMD 是否划算"的定量判据、三路 NaN 语义差异矩阵 | `src/core/qwt_simd_argminmax.*`、`src/plot/qwt_point_mapper.cpp` | `examples/bench/renderbench` |
| [Raster 与 OpenGL 渲染管线](raster-vs-opengl.md) | 两条画布管线的帧时间分解（变换/光栅化/合成/blit）、GL canvas 的 QPainter-on-FBO 真实结构、点数×曲线数×画布尺寸矩阵与选型交叉点判据 | `src/plot/qwt_plot_canvas.*`、`src/plot/qwt_plot_opengl_canvas.*` | `examples/2D/refreshtest`、`examples/2D/realtime` |
| [NaN 数据的渲染语义与性能代价](nan-rendering.md) | NaN"跳过而非切段"的桥接语义矩阵（5 种绘制属性 × NaN 分布模式）、视觉伪差像素级扫描、NaN 比例-帧时间曲线族与 boundingRect 分解 | `src/plot/qwt_point_mapper.cpp`、`src/plot/qwt_plot_curve.cpp` | `examples/2D/nanperf` |
| [实时流式数据渲染策略](realtime-streaming.md) | 环形缓冲 / WindowedSeriesData / 全量 setData 三种数据通路的成本剖析、DirectPainter 增量重绘的真实边界、帧预算与端到端延迟模型（144 个配置实测） | `src/plot/qwt_plot_directpainter.*`、`src/core/qwt_series_data.h` | `examples/2D/oscilloscope`、`examples/2D/realtime`、`examples/2D/cpuplot` |
| [3D 表面渲染的 VBO/VAO 管线](3d-surface-pipeline.md) | Qwt3DSurface 数据上传策略、GLSL 3.3 Core 着色器逐阶段成本、帧率-网格规模标度拟合、gl2ps 矢量导出的取舍 | `src/plot3d/` | `examples/3D/qwtplot3d`、`examples/3D/waterfall3D` |

### 算法与数据语义

深挖五个独立组件的数学基础与实现细节：样条的收敛阶、裁剪的正确性、刻度的数论、统计量的估计器选择、色彩感知的定量评测。

| 报告 | 主题 | 关键源码 | 配套示例 |
|------|------|----------|----------|
| [QwtSpline 样条家族](spline-family.md) | 六种参数化增量公式与仿射等价性、矩法三对角推导与 O(h⁴)/O(h²) 收敛阶实测、弦长参数化起环机理、Fitted 属性的渲染代价 | `src/plot/qwt_spline*` | `examples/2D/splineeditor`、`tests/splineprof` |
| [多边形裁剪算法（QwtClipper）](polygon-clipping.md) | Sutherland–Hodgman 逐函数算法识别与正确性证明、平移/缩放顶点放大率实测、退化输入行为矩阵（2337 组 C++ 位级交叉验证） | `src/core/qwt_clipper.*` | `examples/2D/animation` |
| [刻度引擎的"漂亮数字"算法](scale-engine.md) | {1,2,5} 步长基准集合的十进对偶闭包唯一性与 minimax 差距、线性/对数/日期三引擎剖析、日期刻度八级回退决策树（3013 组 C++ 交叉验证） | `src/core/qwt_scale_engine.*`、`src/plot/qwt_date_scale_engine.*` | `examples/2D/bode`、`examples/2D/stockchart` |
| [箱线图统计量的分位数算法](boxplot-quantiles.md) | QwtBoxStatisticsCalculator 的 R-7 分位数身份判定（34,440 探针零失配）、九种 R 型定义的差异与衰减规律、1.5×IQR 离群判定敏感性与小样本病态 | `src/core/qwt_box_statistics.*` | `examples/2D/boxchart` |
| [科学 Colormap 的色彩科学评估](colormap-color-science.md) | 22 种预设的 CIEDE2000 感知均匀性、亮度单调性、色觉缺陷模拟下的可区分性评测与场景化选择指南 | `src/core/qwt_colormap_preset.*`、`src/core/qwt_colormap.*` | `examples/2D/colormap` |

教程级入口：渲染组对应[渲染性能与曲线平滑](../use-guide/2d/rendering-performance.md)与 [NaN 数据处理](../use-guide/general/nan-handling.md)，算法组对应[刻度计算引擎](../use-guide/2d/scale-engine.md)、[箱形图](../use-guide/2d/boxchart.md)与 [Colormap 预设](../use-guide/general/colormap.md)。

## 图表复现

`docs/assets/algorithms/` 下的 Python 脚本可直接运行复现全部插图与数值：

```bash
pip install matplotlib numpy
cd docs/assets/algorithms
python figures.py                    # 降采样/平滑两篇的全部插图
python downsampling_experiment.py    # 降采样保真度实验（表 1/表 2 数据）
python smoothing_experiment.py       # 平滑核性质实验（核系数、噪声增益、峰保持率）
python spline_experiment.py          # 样条收敛阶、起环扫描与扁平化实验
python clipper_experiment.py         # 裁剪语义、顶点放大率与退化输入实验
python colormap_experiment.py        # colormap 色彩科学评测（CIEDE2000、色觉模拟）
python scaleengine_experiment.py     # 刻度引擎步长统计与日期层级实验
python boxstats_experiment.py        # R 型分位数对比与离群敏感性实验
python nan_experiment.py             # NaN 语义重实现 + 读取 nan_results.csv 绘图
python simd_experiment.py            # 读取 simd_bench_results.csv 绘图（roofline、桶判据）
python raster_opengl_experiment.py   # 读取 raster_opengl_results.csv 绘图（帧时间模型）
python streaming_experiment.py       # 帧预算模型 + 读取 streaming_results.csv 绘图
python surface3d_experiment.py       # 读取 surface3d_results.csv 绘图（标度拟合）
```

性能类报告另附 C++ 基准与本机原始数据：`simd_bench.cpp`、`raster_opengl_bench.cpp`、`nan_bench.cpp`、`streaming_bench.cpp`、`surface3d_bench.cpp`（计时）与 `clipper_check.cpp`、`scaleengine_check.cpp`、`surface_export_check.cpp`、`lighting_render_check.cpp`、`boxstats_check.cpp`、`nan_mapper_check.cpp`（正确性交叉验证，不计时）。编译命令写在各 `.cpp` 文件头部注释（MSVC 2019 + Qt 5.15，依赖仓库构建产物）；`*_results.csv` 的 `#` 头注记录了采集机器的硬件、编译配置与计时协议。在另一台机器上复现的路径：重新编译运行 bench 得到新 CSV，再重跑对应 Python 脚本重新生成插图与分析数值；交叉验证 harness 可直接运行比对语义。新增报告的实验脚本也应放入此目录，命名与报告主题对应，并在报告"实验"一节注明脚本名。
