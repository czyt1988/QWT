# chartbench - Qt plotting library rendering benchmark

Compares the drawing performance of four Qt plotting libraries with one and the
same measurement protocol:

| Library | Version | Code | Backends measured |
|---------|---------|------|-------------------|
| Qwt | 7.5 (this repository) | `qwt::plot` | `QwtPlotCanvas` raster, `QwtPlotOpenGLCanvas` |
| QCustomPlot | 2.1.0 | `qcustomplot/qcustomplot.{h,cpp}` in this directory | `QCPPaintBufferPixmap`, `QCPPaintBufferGlFbo` |
| Qt Charts | Qt 6.7.3 | `Qt6::Charts` | raster only (see *Known limitations*) |
| QIm | 0.0.3 (git submodule) | `qim/` | OpenGL only (ImGui/ImPlot are GL based) |

The numbers this program produces feed the tech report
[The Rendering Performance of Qt Plotting Libraries](../../../docs/tech-report/plot-library-benchmark.md);
the analysis scripts live in `docs/assets/bench/`.

## Why this benchmark exists

"Which plotting library is fastest" is usually answered with a single
point-count sweep, which hides almost everything that matters: where the time
goes when data changes, what point reduction buys, whether the GPU helps, how
much memory the data costs and how stable the frame time is. chartbench
therefore measures eight dimensions with the same harness:

| Suite | Question |
|-------|----------|
| `points` | How does frame time scale with the number of samples? |
| `curves` | What does one additional series cost? |
| `canvas` | Is the renderer geometry bound or fill-rate bound? |
| `update` | How expensive is handing new data to the library, separately from drawing it? |
| `downsample` | What do the built-in point reduction algorithms cost and what do they preserve? |
| `jitter` | What does the frame time *distribution* look like (p50/p95/p99)? |
| `memory` | How many bytes does one sample cost inside each library? |
| `fidelity` | Pixel deviation of each reduction strategy against the library's own full render |

## Measurement protocol

Every library is driven through the same sequence and the same operations:

1. **create** - construct the plot widget with a fixed canvas size (1000x700 by
   default) and fixed axis ranges.
2. **hand over the data** - one call that replaces the whole series.
3. **frame()** - produce exactly one complete frame and return when the pixels
   exist. Per library this is:
   * Qwt: `QwtPlot::replot()` with `QwtPlotCanvas::ImmediatePaint`, which makes
     the canvas repaint synchronously.
   * QCustomPlot: `replot(rpImmediateRefresh)`, which draws every layer into the
     paint buffer and repaints the widget synchronously.
   * Qt Charts: `QGraphicsView::render()` into a reused image - the same scene
     render `paintEvent` runs.
   * QIm: `makeCurrent()` + `paintGL()` + `glFinish()` on the
     `QOpenGLWidget`. QOpenGLWidget defers a plain `repaint()` to Qt's
     composition step, which would make every frame look instantaneous; driving
     the GL entry points is what Qt does while composing and is the only way to
     time a real QIm frame.
   * Qwt OpenGL canvas: the same `makeCurrent()` + `paintGL()` + `glFinish()`.
4. **warm up** 5 frames (discarded), then measure up to 60 frames, or until the
   per-configuration time budget (4 s) is used up. The first measured frame is
   excluded from the statistics by the analysis scripts because it still
   contains one-off allocations. Every process additionally draws one discarded
   1000-sample configuration (30 frames) before the first measured one, because
   the first configuration of a process is otherwise systematically slower
   (driver initialisation, page faults) - measured at 1.30 ms against 0.81 ms
   for the identical plot later in the same process.
5. **repeat** for every configuration. Each library runs in its **own process**
   so the reported process memory can be attributed to that library alone.

Every frame row in the CSV carries the data handover time and the frame time
separately, so the analysis can tell "drawing is slow" apart from "handing over
data is slow".

Absolute numbers depend on the machine; the ordering and the shape of the curves
are what the report argues with.

## Building and running

The example is part of the normal Qwt example build:

```powershell
cmake -S . -B build -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.7.3/msvc2019_64"
cmake --build build --config Release --target chartbench chartbench_qcpgl
```

`chartbench_qcpgl` is the same program compiled with `QCUSTOMPLOT_USE_OPENGL`,
because QCustomPlot's paint buffer backend is a compile time switch.

The QIm adapter needs the submodule:

```bash
git submodule update --init examples/bench/chartbench/qim
```

Without it the example still configures and builds, it just has no QIm adapter.

Run a single configuration:

```bash
build/bin/Release/chartbench --library=qwt --backend=raster --suite=points --out=out --verbose
```

Run the whole matrix (six processes, roughly 20-30 minutes):

```bash
examples/bench/chartbench/run-bench.sh build docs/assets/bench/data
```

Available suites: `points`, `curves`, `canvas`, `update`, `downsample`,
`jitter`, `memory`, `fidelity`, `selftest` (a two configuration smoke test) and
`all`.

## Results

`run-bench.sh` writes two tables per library and backend:

* `frames_<library>_<backend>.csv` - one row per measured frame
* `configs_<library>_<backend>.csv` - one row per configuration: setup time
  (create / data handover / first frame), frame counts, process memory

The `fidelity` suite additionally writes the rendered frames into `images/`.
`docs/assets/bench/chartbench_fidelity.py` turns those images into the pixel
level fidelity metrics and `docs/assets/bench/chartbench_experiment.py` draws
all figures of the report.

## Known limitations

**QCustomPlot needs a Qt 6 patch.** QCustomPlot 2.1.0 declares its `QCP`
namespace as a class under `Q_MOC_RUN` to get enum reflection. Qt 6's moc emits
`QtPrivate::TypeAndForceComplete<QCP, ...>` for that construct, which does not
compile because `QCP` is a namespace in the real compilation. The copy in this
directory carries the minimal fix (namespace + `Q_NAMESPACE`), which is what
upstream QCustomPlot 2.1.1 does as well; the patch is marked with a comment in
`qcustomplot.h` and changes no rendering code.

**QIm was patched, not worked around.** QIm 0.0.3 used `QPointF` in
`src/core/plot/QImPlotValueTrackerNode.h` without including `<QPointF>`, which
Qt 6.7 no longer pulls in transitively. The fix went upstream (gitee.com/czyt1988/qim,
`fix(qt6)` commit) and the submodule pin carries it, so the adapter no longer
force-includes a compatibility header.

**Qt Charts OpenGL series are not measured.** With `QXYSeries::setUseOpenGL(true)`
Qt Charts renders the series into a framebuffer that Qt composites during the
backing store flush. In a synchronous frame protocol this never happens: both
`QGraphicsView::render()` and `QOpenGLWidget::grabFramebuffer()` return an image
whose curve coverage is 0.0009 (axes only) against 0.0586 for the raster path,
so the GL series never reach the framebuffer. Rather than reporting a frame time
for something that draws nothing, the adapter refuses the OpenGL backend and the
report documents the negative result.

**GL backends are synchronised with `glFinish()`.** For the OpenGL backends a
frame is only complete once the GPU is done, so `frame()` ends with a
`glFinish()`. The reported `gpu_sync_ms` column contains the time that wait
took; it is small (well below a millisecond for line plots) but it is part of
the measured frame time.
