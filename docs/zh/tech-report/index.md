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

| 报告 | 主题 | 关键源码 | 配套示例 |
|------|------|----------|----------|
| [LTTB 降采样算法原理与 QWT7 的 MinMax 桶实现](curve-downsampling.md) | 可视化带宽瓶颈、经典 LTTB 推导、MinMax 桶变体的三条性质、SIMD argmin/argmax、像素级保真度实验 | `src/plot/qwt_point_mapper.cpp`、`src/core/qwt_simd_argminmax.cpp` | `examples/bench/renderbench`、`examples/2D/nanperf` |
| [曲线渲染平滑：高斯卷积、Savitzky-Golay 回归与特征保护](curve-smoothing.md) | 偏差-方差分解、两种滤波核的完整推导（矩条件、噪声增益恒等式）、MAD 特征保护、端到端像素实验 | `src/plot/qwt_curve_smoothing.cpp` | `examples/2D/curvesmoothing` |

两篇报告互为配套：降采样篇回答"点太多画不动怎么办"，平滑篇回答"画出来像噪声怎么办"，共同覆盖 `QwtPlotCurve` 渲染管线中数据量与视觉质量这对核心矛盾。对应的教程级快速上手见[渲染性能与曲线平滑](../use-guide/2d/rendering-performance.md)。

## 选题路线图（规划中）

以下是候选选题，每项都已确认仓库内有可支撑实验的源码、示例或基准程序。欢迎讨论优先级与增删：

1. **SIMD argmin/argmax 向量化基准** —— `qwtSimdArgMinMax` 的 AVX2 / SSE4.2 / 标量三路运行时派发实测：加速比 vs 数组长度、桶大小敏感性、roofline 分析；ARM 侧 NEON 路径对照。数据源：`src/core/qwt_simd_argminmax.*`、`examples/bench/renderbench`。预期插图：加速比条形图、吞吐量-长度曲线、roofline 散点。
2. **Raster vs OpenGL 渲染管线基准** —— `QwtPlotCanvas` 与 `QwtPlotOpenGLCanvas` 在不同点数、曲线数、重绘频率下的端到端帧率与帧时间分解，给出"何时值得上 OpenGL"的定量判据。数据源：`examples/2D/realtime`、`examples/2D/oscilloscope`、`examples/2D/refreshtest`、`examples/bench/renderbench`。预期插图：点数-FPS 双对数曲线、帧时间堆叠分解图。
3. **NaN 数据的渲染语义与性能代价** —— NaN 如何把折线切段、五种 PointFilter 属性在不同 NaN 比例下的帧率与视觉正确性扫描。数据源：`examples/2D/nanperf`（NaN 比例可调）、`src/plot/qwt_plot_curve.cpp` 切段路径。预期插图：NaN 比例-帧时间曲线族、切段行为示意图。
4. **QwtSpline 样条家族：精度与性能** —— 参数化方式（均匀/弦长/向心）与插值/逼近样条的误差收敛阶、`Fitted` 属性的渲染代价、与简单贝塞尔拟合的对比。数据源：`src/plot/qwt_spline*`、`examples/2D/splineeditor`、`splineprof/`、`splinetest/`。预期插图：误差收敛双对数图、控制多边形偏差可视化、运行时对比。
5. **多边形裁剪算法（QwtClipper）** —— 平移/缩放时裁剪路径的正确性与输出顶点放大率：裁剪算法在退化情形（自交、共线、极窄视口）下的行为与实测顶点数。数据源：`src/core/qwt_clipper.*`、`examples/2D/animation`。预期插图：裁剪前后对比图、顶点放大率曲线。
6. **科学 colormap 的色彩科学评估** —— 22 种预设的感知均匀性定量评测（CIEDE2000 ΔE、亮度单调性、色盲模拟下的可区分性），给出场景化选择指南。数据源：`src/core/qwt_colormap_preset.*`、`examples/2D/colormap`（支持 `--screenshot` 批量导出）。预期插图：亮度剖面图、ΔE 热力图、色盲模拟前后对比。
7. **刻度引擎的"漂亮数字"算法** —— 线性/对数/日期三种 `QwtScaleEngine` 的刻度生成策略、步长选择的数论性质、日期刻度的层级回退。数据源：`src/core/qwt_scale_engine.*`、`examples/2D/scaleengine`、时间刻度示例。预期插图：刻度分布示意图、步长选择决策树。
8. **实时流式数据渲染策略** —— 环形缓冲 / `WindowedSeriesData` + `QwtPlotDirectPainter` + 降采样的组合模式与帧预算模型：示波器类应用如何稳定 60 FPS。数据源：`examples/2D/oscilloscope`、`examples/2D/realtime`、`examples/2D/cpuplot`。预期插图：数据流时序图、吞吐量-延迟曲线。
9. **3D 表面渲染的 VBO/VAO 管线** —— `Qwt3DSurface` 的数据上传策略（全量/增量）、GLSL 3.3 Core 着色器管线的帧率-网格规模关系、与 gl2ps 矢量导出的取舍。数据源：`src/plot3d/`、`examples/3D/qwtplot3d`、`examples/3D/waterfall3D`。预期插图：渲染管线 mermaid 图、帧率-顶点数曲线。
10. **箱线图统计量的分位数算法** —— `QwtBoxStatistics` 采用的分位数定义与常见 R 型分位数（R-1 … R-9）的差异、排序/流式计算代价、离群点判定的敏感性。数据源：`src/core/qwt_box_statistics.*`、`examples/2D/boxchart`。预期插图：分位数方法差异曲线、箱线结构图。

## 图表复现

`docs/assets/algorithms/` 下的 Python 脚本可直接运行复现全部插图与数值：

```bash
pip install matplotlib numpy
cd docs/assets/algorithms
python figures.py                  # 重新生成全部 fig_*.png
python downsampling_experiment.py  # 降采样保真度实验（表 1/表 2 数据）
python smoothing_experiment.py     # 平滑核性质实验（核系数、噪声增益、峰保持率）
```

新增报告的实验脚本也应放入此目录，命名与报告主题对应（如 `spline_experiment.py`），并在报告"实验"一节注明脚本名。
