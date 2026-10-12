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

| Report | Topic | Key sources | Companion examples |
|--------|-------|-------------|--------------------|
| [Downsampling Curves for the Screen: LTTB Theory and the QWT7 MinMax Bucket](curve-downsampling.md) | The visualization bandwidth bottleneck, classical LTTB derivation, three provable properties of the MinMax bucket variant, SIMD argmin/argmax, pixel-level fidelity experiments | `src/plot/qwt_point_mapper.cpp`, `src/core/qwt_simd_argminmax.cpp` | `examples/bench/renderbench`, `examples/2D/nanperf` |
| [Curve Rendering: Gaussian, Savitzky-Golay and Feature Preservation](curve-smoothing.md) | Bias-variance decomposition, full derivation of both filter kernels (moment conditions, noise-gain identity), MAD-based feature preservation, end-to-end pixel experiments | `src/plot/qwt_curve_smoothing.cpp` | `examples/2D/curvesmoothing` |

The two reports are companions: the downsampling report answers "too many points to draw", the smoothing report answers "the drawing looks like noise". Together they cover the core tension of the `QwtPlotCurve` rendering pipeline — data volume versus visual quality.

## Topic Roadmap (planned)

Candidate topics below; each one has source code, examples, or benchmark programs in the repository that can carry the experiments. Priorities, additions and removals are open for discussion:

1. **SIMD argmin/argmax vectorization benchmark** — measured speedups of the three-way runtime dispatch (AVX2 / SSE4.2 / scalar) in `qwtSimdArgMinMax`: speedup vs array length, bucket-size sensitivity, roofline analysis; NEON path as the ARM counterpart. Sources: `src/core/qwt_simd_argminmax.*`, `examples/bench/renderbench`. Planned figures: speedup bars, throughput-vs-length curves, roofline scatter.
2. **Raster vs OpenGL rendering pipeline benchmark** — end-to-end frame rates and frame-time breakdowns for `QwtPlotCanvas` vs `QwtPlotOpenGLCanvas` across point counts, curve counts and refresh rates, yielding a quantitative criterion for "when is OpenGL worth it". Sources: `examples/2D/realtime`, `examples/2D/oscilloscope`, `examples/2D/refreshtest`, `examples/bench/renderbench`. Planned figures: log-log points-vs-FPS curves, stacked frame-time breakdowns.
3. **NaN rendering semantics and their performance cost** — how NaN splits a polyline into segments; a frame-rate and visual-correctness sweep of the five PointFilter paint attributes across NaN ratios. Sources: `examples/2D/nanperf` (adjustable NaN ratio), the segment-splitting path in `src/plot/qwt_plot_curve.cpp`. Planned figures: frame-time curve families vs NaN ratio, segmentation behavior diagrams.
4. **The QwtSpline family: accuracy and performance** — error convergence orders for parameterizations (uniform / chordal / centripetal) and interpolating vs approximating splines, rendering cost of the `Fitted` attribute, comparison against naive Bézier fitting. Sources: `src/plot/qwt_spline*`, `examples/2D/splineeditor`, `splineprof/`, `splinetest/`. Planned figures: log-log error convergence, control-polygon deviation visualizations, runtime comparisons.
5. **The polygon clipper (QwtClipper)** — correctness and output vertex amplification of the clipping path during pan/zoom: behavior in degenerate cases (self-intersection, collinearity, razor-thin viewports) and measured vertex counts. Sources: `src/core/qwt_clipper.*`, `examples/2D/animation`. Planned figures: before/after clipping, vertex amplification curves.
6. **Color science of the scientific colormaps** — quantitative perceptual-uniformity evaluation of the 22 presets (CIEDE2000 ΔE, luminance monotonicity, distinguishability under color-vision-deficiency simulation), resulting in a scenario-based selection guide. Sources: `src/core/qwt_colormap_preset.*`, `examples/2D/colormap` (supports `--screenshot` batch export). Planned figures: luminance profiles, ΔE heatmaps, CVD simulation comparisons.
7. **The scale engines' "nice numbers" algorithm** — tick generation strategies of the linear/log/date `QwtScaleEngine` subclasses, number-theoretic properties of step selection, level fallbacks of the date scale. Sources: `src/core/qwt_scale_engine.*`, `examples/2D/scaleengine`, the time-scale example. Planned figures: tick distribution diagrams, step-selection decision tree.
8. **Real-time streaming rendering strategies** — ring buffers / `WindowedSeriesData` + `QwtPlotDirectPainter` + downsampling as a combined pattern with a frame-budget model: how oscilloscope-style applications hold 60 FPS. Sources: `examples/2D/oscilloscope`, `examples/2D/realtime`, `examples/2D/cpuplot`. Planned figures: data-flow sequence diagram, throughput-latency curves.
9. **The 3D surface VBO/VAO pipeline** — data upload strategies (full/incremental) of `Qwt3DSurface`, frame rate vs mesh size for the GLSL 3.3 Core shader pipeline, trade-offs against gl2ps vector export. Sources: `src/plot3d/`, `examples/3D/qwtplot3d`, `examples/3D/waterfall3D`. Planned figures: mermaid pipeline diagram, frame-rate-vs-vertex-count curves.
10. **Box-plot quantile algorithms** — the quantile definition used by `QwtBoxStatistics` vs the common R-type quantiles (R-1 … R-9), sorting/streaming cost, sensitivity of outlier classification. Sources: `src/core/qwt_box_statistics.*`, `examples/2D/boxchart`. Planned figures: quantile-method difference curves, box anatomy diagram.

## Reproducing the Figures

The Python scripts in `docs/assets/algorithms/` reproduce every figure and number:

```bash
pip install matplotlib numpy
cd docs/assets/algorithms
python figures.py                  # regenerate all fig_*.png
python downsampling_experiment.py  # fidelity experiments (Tables 1/2 data)
python smoothing_experiment.py     # kernel property experiments (coefficients, noise gain, peak retention)
```

Experiment scripts for new reports belong in the same directory, named after the report topic (e.g. `spline_experiment.py`), and referenced by name in the report's experiment section.
