# Tech Reports

This section collects QWT **tech reports**: long-form technical articles for developers who want to understand how the library works under the hood, written to academic-paper standards — with an abstract, derivations, reproducible experimental data, and references. They answer "**why is it designed this way, and how well does it actually perform**", not "how do I use it".

If you just want to get a plot on screen, read the [User Guide](../use-guide/first-plot.md) — step-by-step tutorials for users.

## Tech Reports vs User Guide Tutorials

| | User Guide (use-guide) | Tech Report (tech-report) |
|---|---|---|
| Reader | Users who want to get a plotting task done quickly | Developers and algorithm engineers who want depth: principles and performance |
| Question answered | "How do I use this feature?" | "Why is it designed this way? What is the math? What do measurements show?" |
| Style | Plain, progressive, complete runnable examples | Paper-style: abstract → introduction → derivation → experiments → discussion → references |
| Figures | Screenshots of the bundled examples, before/after comparisons | matplotlib data figures (experiments) + mermaid structure diagrams (flow/architecture) |
| Math | Avoided; conclusions only when unavoidable | Derived in full where needed, TeX syntax (`$...$` / `$$...$$`) |
| Length | One task per article | No lower bound; completeness of the argument wins |

The two cross-reference each other: tutorials link to tech reports under "Deep dive", and tech reports link back to tutorials and example programs under "Code practice".

## Writing Standards

Tech reports are held to publication standards. On top of the general rules in the [Doc Writing Guide](../doc-writing-guide.md) (platform-generic Markdown, mermaid-first diagrams, etc.), every report must satisfy:

1. **Paper structure**: start with an abstract (one paragraph on problem and method) plus a "core conclusions" quick list; organize the body as introduction → problem formalization → method/derivation → experiments → discussion → summary → references; state properties and theorems with proofs or proof sketches.
2. **Reproducible experiments**: every measured number must be reproducible. matplotlib plotting scripts and raw values live in `docs/assets/algorithms/` (Python + matplotlib + numpy); the article names the script that produced each figure.
3. **Benchmarks state their environment**: performance data must list the QWT/Qt version, build configuration, canvas size, and hardware class, and must note that absolute numbers vary with hardware while relative orderings are stable.
4. **Division of labor between figure types**: data figures (curves, scatter, heatmaps, comparison bars) are matplotlib-generated PNGs; flow charts, architecture and sequence diagrams are inline mermaid. Images live in `docs/assets/algorithms/` with file names starting with `fig_`.
5. **Honest failure analysis**: every report lists the failure modes of the method/implementation; negative data is not hidden.
6. **Concrete citations**: cite source files by path (e.g. `src/plot/qwt_point_mapper.cpp`) and examples by directory (e.g. `examples/bench/renderbench`).

## Available Reports

### Rendering pipeline and performance

Along the thread "data volume → visual quality → backend choice → streaming updates", these reports cover the full path from samples to pixels for `QwtPlotCurve` plus the GPU pipeline on the 3D side. The downsampling report answers "too many points to draw", the smoothing report answers "the drawing looks like noise", the SIMD report expands the vectorized inner loop of downsampling with measurements, the NaN report covers missing-data semantics and cost, the Raster/GL report derives a backend selection criterion, and the streaming report combines all of these factors into a frame-budget model for real-time applications.

| Report | Topic | Key sources | Companion examples |
|--------|-------|-------------|--------------------|
| [Downsampling Curves for the Screen: LTTB Theory and the QWT7 MinMax Bucket](curve-downsampling.md) | The visualization bandwidth bottleneck, classical LTTB derivation, three provable properties of the MinMax bucket variant, SIMD argmin/argmax, pixel-level fidelity experiments | `src/plot/qwt_point_mapper.cpp`, `src/core/qwt_simd_argminmax.cpp` | `examples/bench/renderbench`, `examples/2D/nanperf` |
| [Curve Rendering: Gaussian, Savitzky-Golay and Feature Preservation](curve-smoothing.md) | Bias-variance decomposition, full derivation of both filter kernels (moment conditions, noise-gain identity), MAD-based feature preservation, end-to-end pixel experiments | `src/plot/qwt_curve_smoothing.cpp` | `examples/2D/curvesmoothing` |
| [SIMD argmin/argmax Vectorization Benchmark](simd-argminmax.md) | Measured three-way runtime dispatch (AVX2 / SSE4.2 / scalar): speedup vs array length, cache-level bandwidth and roofline, bucket-size sensitivity with a quantitative "is SIMD worth it" criterion, NaN semantics divergence matrix | `src/core/qwt_simd_argminmax.*`, `src/plot/qwt_point_mapper.cpp` | `examples/bench/renderbench` |
| [Raster vs OpenGL Rendering Pipelines](raster-vs-opengl.md) | Frame-time breakdown of both canvas pipelines (transform / rasterization / compositing / blit), the QPainter-on-FBO reality of the GL canvas, point-count × curve-count × canvas-size matrix and crossover selection criterion | `src/plot/qwt_plot_canvas.*`, `src/plot/qwt_plot_opengl_canvas.*` | `examples/2D/refreshtest`, `examples/2D/realtime` |
| [NaN Rendering Semantics and Their Performance Cost](nan-rendering.md) | The skip-not-split bridging semantics matrix (5 paint attributes × NaN patterns), pixel-level visual-artifact scans, frame-time curve families vs NaN ratio with boundingRect breakdown | `src/plot/qwt_point_mapper.cpp`, `src/plot/qwt_plot_curve.cpp` | `examples/2D/nanperf` |
| [Real-Time Streaming Rendering Strategies](realtime-streaming.md) | Cost analysis of the three data paths (ring buffer / WindowedSeriesData / full setData), the real boundaries of DirectPainter incremental repaints, frame-budget and end-to-end latency models (144 measured configurations) | `src/plot/qwt_plot_directpainter.*`, `src/core/qwt_series_data.h` | `examples/2D/oscilloscope`, `examples/2D/realtime`, `examples/2D/cpuplot` |
| [The 3D Surface VBO/VAO Pipeline](3d-surface-pipeline.md) | Qwt3DSurface data upload strategy, per-stage cost of the GLSL 3.3 Core shaders, frame-rate vs mesh-size scaling fit, gl2ps vector export trade-offs | `src/plot3d/` | `examples/3D/qwtplot3d`, `examples/3D/waterfall3D` |

### Algorithms and data semantics

Deep dives into the mathematical foundations and implementation details of five components: spline convergence, clipping correctness, the number theory of ticks, the quantile estimator behind box plots, and quantitative color perception.

| Report | Topic | Key sources | Companion examples |
|--------|-------|-------------|--------------------|
| [The QwtSpline Family](spline-family.md) | Six parameterization increment formulas and affine equivalence, moment-method tridiagonal derivation with measured O(h⁴)/O(h²) convergence orders, the chordal looping mechanism, rendering cost of the Fitted attribute | `src/plot/qwt_spline*` | `examples/2D/splineeditor`, `tests/splineprof` |
| [The Polygon Clipper (QwtClipper)](polygon-clipping.md) | Per-function algorithm identification with correctness proofs (Sutherland–Hodgman), measured pan/zoom vertex amplification, degenerate-input behavior matrix (2337 bit-exact C++ cross-validation cases) | `src/core/qwt_clipper.*` | `examples/2D/animation` |
| [The Scale Engines' Nice-Numbers Algorithm](scale-engine.md) | Decimal-dual-closure uniqueness of the {1,2,5} step base and its minimax gap, linear/log/date engine analysis, the eight-level date tick fallback decision tree (3013 C++ cross-validation cases) | `src/core/qwt_scale_engine.*`, `src/plot/qwt_date_scale_engine.*` | `examples/2D/bode`, `examples/2D/stockchart` |
| [Box-Plot Quantile Algorithms](boxplot-quantiles.md) | Identification of QwtBoxStatisticsCalculator as an R-7 quantile (34,440 probes, zero mismatches), divergence and decay across the nine R-type definitions, 1.5×IQR outlier sensitivity and small-sample pathology | `src/core/qwt_box_statistics.*` | `examples/2D/boxchart` |
| [Color Science of the Scientific Colormaps](colormap-color-science.md) | CIEDE2000 perceptual uniformity, lightness monotonicity and color-vision-deficiency distinguishability of the 22 presets, with a scenario-based selection guide | `src/core/qwt_colormap_preset.*`, `src/core/qwt_colormap.*` | `examples/2D/colormap` |

Tutorial-level entry points: the rendering group pairs with [Rendering Performance & Curve Smoothing](../use-guide/2d/rendering-performance.md) and [NaN Handling](../use-guide/general/nan-handling.md); the algorithms group pairs with [Scale Engine](../use-guide/2d/scale-engine.md), [Box Chart](../use-guide/2d/boxchart.md) and [Colormap Presets](../use-guide/general/colormap.md).

## Reproducing the Figures

The Python scripts in `docs/assets/algorithms/` reproduce every figure and number:

```bash
pip install matplotlib numpy
cd docs/assets/algorithms
python figures.py                    # all figures of the downsampling/smoothing reports
python downsampling_experiment.py    # fidelity experiments (Tables 1/2 data)
python smoothing_experiment.py       # kernel property experiments (coefficients, noise gain, peak retention)
python spline_experiment.py          # spline convergence, looping sweep and flattening experiments
python clipper_experiment.py         # clipping semantics, vertex amplification and degenerate inputs
python colormap_experiment.py        # colormap color-science evaluation (CIEDE2000, CVD simulation)
python scaleengine_experiment.py     # scale engine step statistics and date level experiments
python boxstats_experiment.py        # R-type quantile comparison and outlier sensitivity
python nan_experiment.py             # NaN semantics reimplementation + plots from nan_results.csv
python simd_experiment.py            # plots from simd_bench_results.csv (roofline, bucket criterion)
python raster_opengl_experiment.py   # plots from raster_opengl_results.csv (frame-time models)
python streaming_experiment.py       # frame-budget model + plots from streaming_results.csv
python surface3d_experiment.py       # plots from surface3d_results.csv (scaling fits)
```

The performance-oriented reports additionally ship C++ benchmarks and the raw data measured on the reference machine: `simd_bench.cpp`, `raster_opengl_bench.cpp`, `nan_bench.cpp`, `streaming_bench.cpp`, `surface3d_bench.cpp` (timed) and `clipper_check.cpp`, `scaleengine_check.cpp`, `boxstats_check.cpp`, `nan_mapper_check.cpp` (correctness cross-validation, untimed). Compile commands are in the header comment of each `.cpp` (MSVC 2019 + Qt 5.15, depending on the repository build products); the `#` headers of the `*_results.csv` files record the hardware, build configuration and timing protocol of the collecting machine. To reproduce on another machine: rebuild and rerun the benchmark to obtain a fresh CSV, then rerun the corresponding Python script to regenerate figures and analysis values; the cross-validation harnesses run standalone for semantic comparisons. Experiment scripts for new reports belong in the same directory, named after the report topic, and referenced by name in the report's experiment section.
