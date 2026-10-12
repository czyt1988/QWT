"""
Analysis for the raster-vs-opengl tech report.

    python raster_opengl_experiment.py

Reads the benchmark CSV produced by raster_opengl_bench.cpp
( raster_opengl_results.csv next to this script ), prints every analysis
number to stdout and writes the report figures:

    fig_rgl_fps.png        frame time vs points, per backend / curves / size
    fig_rgl_breakdown.png  stacked frame-time decomposition (in-situ stages)
    fig_rgl_stages.png     drawCanvas (item stage) vs total samples, log-log
    fig_rgl_crossover.png  fitted linear models and the raster/gl crossover
    fig_rgl_redraw.png     full replot vs DirectPainter vs sliding window
    fig_rgl_knobs.png      cache / immediate / MSAA / finish / AA / filter

Rows with frames==30 form the main matrix (one era per block); rows with
frames==31 are the supplementary paired blocks, where all variants of one
comparison were executed back-to-back so that machine load is matched.

Only numpy + matplotlib are used; the Agg backend keeps it headless.
"""

import pathlib
import sys

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = pathlib.Path(__file__).parent
CSV = HERE / "raster_opengl_results.csv"

COLS = [
    "mode", "redraw", "points", "curves", "frames", "runs",
    "canvas_w", "canvas_h", "filter", "cache", "msaa", "immediate",
    "copybs", "finish", "aa", "nan_pct", "retries",
    "iter_min_ms", "iter_med_ms", "iter_max_ms",
    "frame_min_ms", "frame_med_ms", "frame_mean_ms", "frame_max_ms",
    "drawcanvas_ms", "drawcanvas_calls", "paints",
]
INT_COLS = {"points", "curves", "frames", "runs", "canvas_w", "canvas_h",
            "cache", "msaa", "immediate", "copybs", "finish", "aa", "retries",
            "drawcanvas_calls", "paints"}
FLOAT_COLS = {"nan_pct", "iter_min_ms", "iter_med_ms", "iter_max_ms",
              "frame_min_ms", "frame_med_ms", "frame_mean_ms", "frame_max_ms",
              "drawcanvas_ms"}

COL_RASTER = "#1f77b4"
COL_GL = "#d62728"
COL_RENDERTO = "#2ca02c"
COL_ITEMS = "#ff7f0e"
COL_MAPPER = "#7f7f7f"
BACKEND_COLORS = {"raster": COL_RASTER, "gl": COL_GL, "renderto": COL_RENDERTO,
                  "items": COL_ITEMS, "mapper": COL_MAPPER}
BACKEND_LABELS = {"raster": "raster canvas (QwtPlotCanvas)",
                  "gl": "OpenGL canvas (QwtPlotOpenGLCanvas)",
                  "renderto": "QwtPlotRenderer -> QImage",
                  "items": "drawItems -> QImage",
                  "mapper": "QwtPointMapper only"}

MAIN = dict(frames=30)  # main-matrix selector
SUPP = dict(frames=31)  # supplementary paired-block selector

PTS = (1000, 10000, 100000, 1000000, 5000000)
CURVES = (1, 10, 50)
SIZES = ((900, 500), (1920, 1080))


# --------------------------------------------------------------------- data

def load():
    rows = []
    with open(CSV, "r", encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line or line.startswith("#") or line.startswith("mode,"):
                continue
            parts = line.split(",")
            if len(parts) != len(COLS):
                print(f"SKIP malformed row: {line[:80]}", file=sys.stderr)
                continue
            rec = {}
            for name, val in zip(COLS, parts):
                if name in INT_COLS:
                    rec[name] = int(float(val))
                elif name in FLOAT_COLS:
                    rec[name] = float(val)
                else:
                    rec[name] = val
            rows.append(rec)
    return rows


def sel(rows, **kw):
    out = rows
    for key, val in kw.items():
        out = [r for r in out if r[key] == val]
    return out


def one(rows, **kw):
    hits = sel(rows, **kw)
    return hits[0] if hits else None


def set_style(ax):
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.tick_params(labelsize=9)


def fmt_pts(n):
    if n >= 1000000:
        return f"{n // 1000000}M" if n % 1000000 == 0 else f"{n / 1e6:.1f}M"
    if n >= 1000:
        return f"{n // 1000}k" if n % 1000 == 0 else f"{n / 1e3:.1f}k"
    return str(n)


def fit_linear(A, y):
    coef, *_ = np.linalg.lstsq(A, y, rcond=None)
    pred = A @ coef
    ss_res = float(np.sum((y - pred) ** 2))
    ss_tot = float(np.sum((y - np.mean(y)) ** 2))
    r2 = 1.0 - ss_res / ss_tot if ss_tot > 0 else float("nan")
    return coef, r2


def cell(rows, mode, pts, curves, w, h, **extra):
    """Main-matrix full-replot LTTB cell."""
    kw = dict(mode=mode, redraw="full", points=pts, curves=curves, canvas_w=w,
              canvas_h=h, filter="lttb", aa=0, cache=1, immediate=0, **MAIN)
    kw.update(extra)
    return one(rows, **kw)


# ---------------------------------------------------------------- analysis

def main_matrix_table(rows, w, h, stat="iter_min_ms"):
    print(f"\n=== main matrix, canvas {w}x{h}, redraw=full filter=lttb aa=off "
          f"cache=on immediate=off msaa=4(gl), statistic={stat} (ms/frame) ===")
    print(f"{'points':>8} {'curves':>6} | {'raster':>9} {'gl':>9} {'renderto':>9} "
          f"| {'dc_raster':>9} {'dc_gl':>9} | {'mapper':>8} | {'gl/raster':>9}")
    table = {}
    for curves in CURVES:
        for pts in PTS:
            vals = {}
            for mode in ("raster", "gl", "renderto"):
                r = cell(rows, mode, pts, curves, w, h)
                vals[mode] = r[stat] if r else float("nan")
                if r and mode in ("raster", "gl"):
                    vals[f"dc_{mode}"] = r["drawcanvas_ms"]
            m = one(rows, mode="mapper", points=pts, curves=curves,
                    canvas_w=w, canvas_h=h, filter="lttb", **MAIN)
            vals["mapper"] = m[stat] if m else float("nan")
            ratio = vals["gl"] / vals["raster"] if vals.get("raster") else float("nan")
            table[(pts, curves)] = vals
            print(f"{fmt_pts(pts):>8} {curves:>6} | {vals['raster']:>9.3f} "
                  f"{vals['gl']:>9.3f} {vals['renderto']:>9.3f} | "
                  f"{vals.get('dc_raster', float('nan')):>9.3f} "
                  f"{vals.get('dc_gl', float('nan')):>9.3f} | "
                  f"{vals['mapper']:>8.3f} | {ratio:>9.2f}")
    return table


def empty_scene(rows):
    print("\n=== empty scene (curves=0): fixed per-frame overhead, ms ===")
    out = {}
    for (w, h) in SIZES:
        for mode in ("raster", "gl", "renderto", "items"):
            r = one(rows, mode=mode, redraw="full", curves=0, canvas_w=w,
                    canvas_h=h, **MAIN)
            if r:
                out[(mode, w, h)] = r
                extra = ""
                if mode in ("raster", "gl"):
                    extra = f"  drawcanvas={r['drawcanvas_ms']:.4f}"
                print(f"{mode:>9} {w}x{h}: frame_min={r['iter_min_ms']:.4f} "
                      f"frame_med={r['frame_med_ms']:.4f}{extra}")
    for (w, h) in SIZES:
        for msaa in (0, 4):
            r = one(rows, mode="gl", redraw="full", curves=0, canvas_w=w,
                    canvas_h=h, msaa=msaa, **SUPP)
            if r:
                print(f"      gl {w}x{h} msaa={msaa} (paired block): "
                      f"frame_min={r['iter_min_ms']:.4f} frame_med={r['frame_med_ms']:.4f}")
                out[("gl", w, h, f"msaa{msaa}")] = r
    return out


def decomposition(rows, w=900, h=500):
    """In-situ stage decomposition for raster and gl (frame_med based).

    frame_med = transform + itemdraw + fixed + extra
      transform = mapper mode frame_med (QwtPointMapper::toPolygonF)
      itemdraw  = drawCanvas_mean - transform (in-canvas polyline rasterization)
      fixed     = empty-scene frame_med (background fill + blit/MSAA resolve +
                  composite + flush + event pump)
      extra     = frame_med - drawCanvas_mean - fixed (workload-dependent
                  blit/composite increment, GPU spillover, drift; clamped >= 0)
    """
    print(f"\n=== frame time decomposition (in-situ stages), {w}x{h}, frame_med ===")
    print("frame_med = transform + itemdraw + fixed(empty frame) + extra")
    dec = {}
    for curves in CURVES:
        for pts in (1000, 100000, 1000000, 5000000):
            for mode in ("raster", "gl"):
                r = cell(rows, mode, pts, curves, w, h)
                m = one(rows, mode="mapper", points=pts, curves=curves,
                        canvas_w=w, canvas_h=h, filter="lttb", **MAIN)
                er = one(rows, mode=mode, redraw="full", curves=0, canvas_w=w,
                         canvas_h=h, **MAIN)
                if not r or not m or not er:
                    continue
                transform = m["frame_med_ms"]
                dc = r["drawcanvas_ms"]
                itemdraw = max(dc - transform, 0.0)
                fixed = er["frame_med_ms"]
                extra = max(r["frame_med_ms"] - dc - fixed, 0.0)
                dec[(mode, pts, curves)] = dict(
                    transform=transform, itemdraw=itemdraw, fixed=fixed,
                    extra=extra, frame=r["frame_med_ms"], dc=dc)
    for key in sorted(dec, key=lambda k: (k[1] * k[2], k[1], k[0])):
        d = dec[key]
        mode, pts, curves = key
        print(f"{mode:>6} {fmt_pts(pts):>5} x{curves:<3} frame_med={d['frame']:8.3f} = "
              f"transform {d['transform']:7.3f} + itemdraw {d['itemdraw']:7.3f} + "
              f"fixed {d['fixed']:6.3f} + extra {d['extra']:7.3f}")
    return dec


def fit_models(rows, w, h):
    print(f"\n=== linear model fits, canvas {w}x{h} (frame ms, iter_min, main block) ===")
    fits = {}
    for mode in ("raster", "gl"):
        pts_list, k_list, y, ydc = [], [], [], []
        for curves in CURVES:
            for pts in PTS:
                r = cell(rows, mode, pts, curves, w, h)
                if r:
                    pts_list.append(pts)
                    k_list.append(curves)
                    y.append(r["iter_min_ms"])
                    if r["drawcanvas_calls"] > 100:
                        ydc.append(r["drawcanvas_ms"])
        if len(y) < 5:
            continue
        N = np.array(pts_list, dtype=float)
        K = np.array(k_list, dtype=float)
        y = np.array(y)

        A1 = np.column_stack([np.ones_like(N), N, K])
        c1, r21 = fit_linear(A1, y)
        A2 = np.column_stack([np.ones_like(N), N * K, K])
        c2, r22 = fit_linear(A2, y)

        dcfits = None
        if len(ydc) >= 5:
            Ad = np.column_stack([np.ones_like(N), N * K, K])
            cd, r2d = fit_linear(Ad, np.array(ydc))
            dcfits = (cd, r2d)

        fits[(mode, w, h)] = dict(lin=(c1, r21), inter=(c2, r22), dc=dcfits)
        print(f"{mode}: frame = a + b*N + c*K          -> a={c1[0]:.4f} "
              f"b={c1[1] * 1e6:.4f} ms/Mpt c={c1[2]:.4f}  R2={r21:.4f}")
        print(f"{mode}: frame = a + b*(N*K) + c*K       -> a={c2[0]:.4f} "
              f"b={c2[1] * 1e6:.4f} ms/Mpt c={c2[2]:.4f}  R2={r22:.4f}")
        if dcfits:
            cd, r2d = dcfits
            print(f"{mode}: drawCanvas = a + b*(N*K)+c*K -> a={cd[0]:.4f} "
                  f"b={cd[1] * 1e6:.4f} ms/Mpt c={cd[2]:.4f}  R2={r2d:.4f}")

    kr, kg = fits.get(("raster", w, h)), fits.get(("gl", w, h))
    if kr and kg:
        cr, cg = kr["inter"][0], kg["inter"][0]
        db = cr[1] - cg[1]
        da = cg[0] - cr[0]
        print(f"--- crossover {w}x{h} (interaction model, frame level) ---")
        if db > 0 and da > 0:
            tstar = da / db
            print(f"crossover total samples N*K* = {tstar:,.0f} ({tstar / 1e6:.2f}M)")
            for K in CURVES:
                print(f"  K={K:>2}: N* = {tstar / K:,.0f} ({tstar / K / 1e6:.3f}M)")
        else:
            print(f"no positive crossover in fitted range (da={da:.4f}, db={db * 1e6:.4f})")
        if kr["dc"] and kg["dc"]:
            dr, dg = kr["dc"][0], kg["dc"][0]
            dbd = dr[1] - dg[1]
            dad = dg[0] - dr[0]
            if dbd > 0:
                t = dad / dbd if dad > 0 else 0.0
                print(f"item-stage (drawCanvas) crossover: N*K* = {t:,.0f}")
    return fits


def empirical_crossover(rows, w=900, h=500):
    print(f"\n=== empirical gl-vs-raster frame comparison, {w}x{h} (iter_min, main block) ===")
    for curves in CURVES:
        line = []
        for pts in PTS:
            rr = cell(rows, "raster", pts, curves, w, h)
            rg = cell(rows, "gl", pts, curves, w, h)
            if rr and rg:
                who = "GL " if rg["iter_min_ms"] < rr["iter_min_ms"] else "ras"
                line.append(f"{fmt_pts(pts)}:{who}({rg['iter_min_ms'] / rr['iter_min_ms']:.2f}x)")
        print(f"  K={curves:>2}  " + "  ".join(line))


def redraw_subset(rows):
    print("\n=== redraw modes (900x500, lttb, cache=on), ms/frame iter_min ===")
    out = {}
    for curves in (1, 10):
        for pts in (100000, 1000000):
            for mode in ("raster", "gl"):
                vals = {}
                for redraw in ("full", "direct", "window"):
                    r = one(rows, mode=mode, redraw=redraw, points=pts,
                            curves=curves, canvas_w=900, canvas_h=500, **MAIN)
                    vals[redraw] = r["iter_min_ms"] if r else float("nan")
                out[(mode, pts, curves)] = vals
                print(f"{mode:>6} {fmt_pts(pts):>5} x{curves:<3} full={vals['full']:8.3f} "
                      f"direct={vals['direct']:8.3f} window={vals['window']:8.3f}")
    print("note: gl direct is bimodal (iter_min far below iter_med, retries exhausted);")
    print("      raster direct with cache=on paints the item twice (backing store + widget)")
    return out


def knob_table(rows, title, cells):
    print(f"\n=== {title} ===")
    out = []
    for label, kw in cells:
        r = one(rows, canvas_w=900, canvas_h=500, **kw)
        v = r["iter_min_ms"] if r else float("nan")
        out.append((label, v))
        extra = ""
        if r and r["mode"] in ("raster", "gl"):
            extra = f" dc={r['drawcanvas_ms']:.3f}"
        print(f"  {label:<44} {v:9.3f} ms{extra}")
    return out


def supp_analysis(rows):
    """Paired supplementary block (frames=31): load-matched comparisons."""
    print("\n=== supplementary paired block (frames=31, back-to-back runs) ===")

    print("\n--- GL variants per workload: iter_min / frame_med / dc / (frame_med - dc) ---")
    variants = [("msaa=0", dict(msaa=0, finish=0, cache=1)),
                ("msaa=1", dict(msaa=1, finish=0, cache=1)),
                ("msaa=4", dict(msaa=4, finish=0, cache=1)),
                ("msaa=4+glFinish", dict(msaa=4, finish=1, cache=1)),
                ("msaa=4,noFBOreuse", dict(msaa=4, finish=0, cache=0))]
    for pts, curves in ((100000, 1), (1000000, 1), (100000, 10), (1000000, 10)):
        print(f"  gl {fmt_pts(pts)} x{curves} (900x500):")
        for label, kw in variants:
            r = one(rows, mode="gl", redraw="full", points=pts, curves=curves,
                    canvas_w=900, canvas_h=500, filter="lttb", aa=0,
                    immediate=0, **kw, **SUPP)
            if r:
                print(f"    {label:<20} iter_min={r['iter_min_ms']:8.3f} "
                      f"frame_med={r['frame_med_ms']:8.3f} dc={r['drawcanvas_ms']:8.3f} "
                      f"nonitem={r['frame_med_ms'] - r['drawcanvas_ms']:8.3f}")

    print("\n--- empty scene GL fixed cost split (paired) ---")
    for (w, h) in SIZES:
        for msaa in (0, 4):
            r = one(rows, mode="gl", redraw="full", curves=0, canvas_w=w,
                    canvas_h=h, msaa=msaa, **SUPP)
            if r:
                print(f"  gl empty {w}x{h} msaa={msaa}: iter_min={r['iter_min_ms']:.3f} "
                      f"frame_med={r['frame_med_ms']:.3f}")
    r = one(rows, mode="raster", redraw="full", curves=0, canvas_w=1920,
            canvas_h=1080, **SUPP)
    if r:
        print(f"  raster empty 1920x1080 (paired ref): iter_min={r['iter_min_ms']:.3f} "
              f"frame_med={r['frame_med_ms']:.3f}")

    print("\n--- antialiasing on/off (paired, 900x500) ---")
    for mode in ("raster", "gl"):
        for pts, curves in ((100000, 1), (1000000, 1), (100000, 10), (1000000, 10)):
            ro = one(rows, mode=mode, redraw="full", points=pts, curves=curves,
                     canvas_w=900, canvas_h=500, filter="lttb", aa=0, cache=1,
                     immediate=0, **SUPP)
            rn = one(rows, mode=mode, redraw="full", points=pts, curves=curves,
                     canvas_w=900, canvas_h=500, filter="lttb", aa=1, cache=1,
                     immediate=0, **SUPP)
            if ro and rn:
                print(f"  {mode:>6} {fmt_pts(pts):>5} x{curves:<3} AA off={ro['iter_min_ms']:8.3f} "
                      f"on={rn['iter_min_ms']:8.3f}  ratio={rn['iter_min_ms'] / ro['iter_min_ms']:5.2f}  "
                      f"dc off={ro['drawcanvas_ms']:7.3f} on={rn['drawcanvas_ms']:7.3f}")

    print("\n--- ImmediatePaint on/off (paired, 900x500) ---")
    for mode in ("raster", "gl"):
        for pts, curves in ((100000, 1), (1000000, 10)):
            ro = one(rows, mode=mode, redraw="full", points=pts, curves=curves,
                     canvas_w=900, canvas_h=500, filter="lttb", aa=0, cache=1,
                     immediate=0, **SUPP)
            rn = one(rows, mode=mode, redraw="full", points=pts, curves=curves,
                     canvas_w=900, canvas_h=500, filter="lttb", aa=0, cache=1,
                     immediate=1, **SUPP)
            if ro and rn:
                print(f"  {mode:>6} {fmt_pts(pts):>5} x{curves:<3} update()={ro['iter_min_ms']:8.3f} "
                      f"repaint()={rn['iter_min_ms']:8.3f}")

    print("\n--- raster BackingStore on/off (paired supp2, 900x500) ---")
    for pts, curves in ((100000, 1), (1000000, 10)):
        ro = one(rows, mode="raster", redraw="full", points=pts, curves=curves,
                 canvas_w=900, canvas_h=500, filter="lttb", aa=0, cache=1,
                 immediate=0, **SUPP)
        rn = one(rows, mode="raster", redraw="full", points=pts, curves=curves,
                 canvas_w=900, canvas_h=500, filter="lttb", aa=0, cache=0,
                 immediate=0, **SUPP)
        if ro and rn:
            print(f"  raster {fmt_pts(pts):>5} x{curves:<3} cache on={ro['iter_min_ms']:8.3f} "
                  f"off={rn['iter_min_ms']:8.3f}")

    print("\n--- 1920x1080 MSAA pair ---")
    for msaa in (0, 4):
        r = one(rows, mode="gl", redraw="full", points=100000, curves=1,
                canvas_w=1920, canvas_h=1080, filter="lttb", aa=0, cache=1,
                immediate=0, msaa=msaa, **SUPP)
        if r:
            print(f"  gl 100k x1 1920x1080 msaa={msaa}: iter_min={r['iter_min_ms']:.3f} "
                  f"frame_med={r['frame_med_ms']:.3f} dc={r['drawcanvas_ms']:.3f}")

    print("\n--- era3 control block (frames=32, machine-state probe) ---")
    era3 = sel(rows, frames=32)
    for r in era3:
        fin = "+glFinish" if r["finish"] else ""
        print(f"  {r['mode']:>6} {fmt_pts(r['points']):>5} x{r['curves']:<3} msaa={r['msaa']} "
              f"{fin:<10} iter_min={r['iter_min_ms']:8.3f} iter_med={r['iter_med_ms']:8.3f} "
              f"frame_med={r['frame_med_ms']:8.3f} dc={r['drawcanvas_ms']:8.3f} "
              f"retries={r['retries']}")


def size_scaling(rows):
    print("\n=== size scaling (fixed cost and reference cells), iter_min, main block ===")
    for mode in ("raster", "gl", "renderto"):
        line = []
        for (w, h) in SIZES:
            e = one(rows, mode=mode, redraw="full", curves=0, canvas_w=w,
                    canvas_h=h, **MAIN)
            small = cell(rows, mode, 100000, 1, w, h)
            if e and small:
                line.append(f"{w}x{h}: empty={e['iter_min_ms']:.3f} "
                            f"100kx1={small['iter_min_ms']:.3f}")
        print(f"  {mode:>9}  " + " | ".join(line))


# ------------------------------------------------------------------ figures

def fig_fps(rows):
    fig, axes = plt.subplots(2, 3, figsize=(11.4, 6.4), sharex=True)
    for row, (w, h) in enumerate(SIZES):
        for col, curves in enumerate(CURVES):
            ax = axes[row][col]
            for mode in ("raster", "gl", "renderto"):
                xs, ys = [], []
                for pts in PTS:
                    r = cell(rows, mode, pts, curves, w, h)
                    if r:
                        xs.append(pts)
                        ys.append(r["iter_min_ms"])
                if xs:
                    ax.plot(xs, ys, "o-", ms=4, lw=1.4, color=BACKEND_COLORS[mode],
                            label=BACKEND_LABELS[mode])
            ax.set_xscale("log")
            ax.set_yscale("log")
            ax.set_xticks([1000, 100000, 5000000])
            ax.set_xticklabels(["1k", "100k", "5M"])
            ax.grid(True, which="both", ls=":", lw=0.5, alpha=0.5)
            if col == 0:
                ax.set_ylabel(f"frame time, min-of-7 (ms)\n{w}x{h}")
            ax.set_title(f"{curves} curve{'s' if curves > 1 else ''}", fontsize=10)
            set_style(ax)
    ax = axes[0][0]
    for fps, c in ((60, "#bbbbbb"), (30, "#999999")):
        ax.axhline(1000.0 / fps, color=c, ls="--", lw=0.9)
        ax.text(1050, 1000.0 / fps * 1.08, f"{fps} FPS", fontsize=8, color=c)
    axes[1][1].set_xlabel("points per curve")
    axes[0][0].legend(fontsize=7.6, frameon=False, loc="upper left")
    fig.suptitle("end-to-end frame time vs data size (LTTB filter on, AA off, vsync off)",
                 fontsize=11)
    fig.tight_layout(rect=[0, 0, 1, 0.95])
    fig.savefig(HERE / "fig_rgl_fps.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_breakdown(rows, dec):
    combos = [("raster", 100000, 1), ("gl", 100000, 1),
              ("raster", 1000000, 10), ("gl", 1000000, 10),
              ("raster", 5000000, 50), ("gl", 5000000, 50)]
    fig, ax = plt.subplots(figsize=(9.8, 4.4))
    labels, segs = [], {k: [] for k in ("transform", "itemdraw", "fixed", "extra")}
    for mode, pts, curves in combos:
        d = dec.get((mode, pts, curves))
        if d is None:
            continue
        labels.append(f"{mode}\n{fmt_pts(pts)}x{curves}")
        for k in segs:
            segs[k].append(d[k])
    bottom = np.zeros(len(labels))
    colors = {"transform": "#8ecae6", "itemdraw": "#1f77b4",
              "fixed": "#bdbdbd", "extra": "#d62728"}
    names = {"transform": "transform + LTTB reduce (mapper)",
             "itemdraw": "polyline rasterization in canvas (drawCanvas - mapper)",
             "fixed": "backend fixed cost (empty-scene frame)",
             "extra": "workload-dependent blit / composite / GPU spillover"}
    for k in ("transform", "itemdraw", "fixed", "extra"):
        ax.bar(labels, segs[k], 0.62, bottom=bottom, color=colors[k],
               edgecolor="white", label=names[k])
        bottom += np.array(segs[k])
    for i, tot in enumerate(bottom):
        ax.text(i, tot * 1.15, f"{tot:.1f}", ha="center", fontsize=8)
    ax.set_yscale("log")
    ax.set_ylim(0.05, 4000)
    ax.set_ylabel("frame time, median (ms, log scale)")
    ax.tick_params(axis="x", labelsize=8.5)
    ax.legend(fontsize=8, frameon=False, loc="upper left")
    ax.set_title("in-situ decomposition, 900x500 canvas (frame_med = transform + "
                 "itemdraw + fixed + extra)", fontsize=10)
    set_style(ax)
    fig.tight_layout()
    fig.savefig(HERE / "fig_rgl_breakdown.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_stages(rows):
    fig, axes = plt.subplots(1, 2, figsize=(11.0, 4.0))
    ax = axes[0]
    for mode in ("raster", "gl"):
        xs, ys = [], []
        for curves in CURVES:
            for pts in PTS:
                r = cell(rows, mode, pts, curves, 900, 500)
                if r and r["drawcanvas_calls"] > 100:
                    xs.append(pts * curves)
                    ys.append(r["drawcanvas_ms"])
        if xs:
            order = np.argsort(xs)
            ax.plot(np.array(xs)[order], np.array(ys)[order], "o", ms=4.5,
                    color=BACKEND_COLORS[mode],
                    label=f"{mode}: QwtPlot::drawCanvas (in situ)")
    mx, my = [], []
    for curves in CURVES:
        for pts in PTS:
            m = one(rows, mode="mapper", points=pts, curves=curves,
                    canvas_w=900, canvas_h=500, filter="lttb", **MAIN)
            if m:
                mx.append(pts * curves)
                my.append(m["iter_min_ms"])
    if mx:
        order = np.argsort(mx)
        ax.plot(np.array(mx)[order], np.array(my)[order], "s", ms=3.6, mfc="white",
                color=COL_MAPPER, label="mapper only (transform + LTTB reduce)")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("total samples N x K")
    ax.set_ylabel("item painting stage (ms)")
    ax.grid(True, which="both", ls=":", lw=0.5, alpha=0.5)
    ax.legend(fontsize=8, frameon=False, loc="upper left")
    ax.set_title("item stage: raster engine vs QPainter-OpenGL2 engine", fontsize=10)
    set_style(ax)

    ax = axes[1]
    for mode in ("raster", "gl"):
        xs, ys = [], []
        for curves in CURVES:
            for pts in PTS:
                r = cell(rows, mode, pts, curves, 900, 500)
                if r and r["drawcanvas_calls"] > 100:
                    xs.append(pts * curves)
                    ys.append(r["iter_min_ms"] - r["drawcanvas_ms"])
        if xs:
            order = np.argsort(xs)
            ax.plot(np.array(xs)[order], np.array(ys)[order], "o-", ms=4, lw=1.2,
                    color=BACKEND_COLORS[mode], label=f"{mode}: frame - drawCanvas")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("total samples N x K")
    ax.set_ylabel("everything around the item stage (ms)")
    ax.grid(True, which="both", ls=":", lw=0.5, alpha=0.5)
    ax.legend(fontsize=8, frameon=False, loc="center right")
    ax.set_title("non-item part of the frame (blit / resolve / composite / events)",
                 fontsize=10)
    set_style(ax)

    fig.tight_layout()
    fig.savefig(HERE / "fig_rgl_stages.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_crossover(rows, fits):
    fig, axes = plt.subplots(1, 2, figsize=(11.0, 4.2))
    for ax, (w, h) in zip(axes, SIZES):
        for mode in ("raster", "gl"):
            xs, ys = [], []
            for curves in CURVES:
                for pts in PTS:
                    r = cell(rows, mode, pts, curves, w, h)
                    if r:
                        xs.append(pts * curves)
                        ys.append(r["iter_min_ms"])
            if xs:
                ax.plot(xs, ys, "o", ms=4.2, color=BACKEND_COLORS[mode],
                        label=f"{mode} (measured)")
        for mode in ("raster", "gl"):
            key = (mode, w, h)
            if key not in fits:
                continue
            c = fits[key]["inter"][0]
            t = np.logspace(3, np.log10(2.5e8), 60)
            ax.plot(t, np.maximum(c[0] + c[1] * t + c[2], 0.05), "--", lw=1.2,
                    color=BACKEND_COLORS[mode], label=f"{mode} fit")
        kr, kg = fits.get(("raster", w, h)), fits.get(("gl", w, h))
        if kr and kg:
            db = kr["inter"][0][1] - kg["inter"][0][1]
            da = kg["inter"][0][0] - kr["inter"][0][0]
            if db > 0 and da > 0:
                tstar = da / db
                ax.axvline(tstar, color="#555555", ls=":", lw=1.2)
                ax.annotate(f"crossover\nN x K = {tstar / 1e6:.2f}M",
                            (tstar * 1.15, 0.12), fontsize=8.5, ha="left",
                            color="#333333")
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xlabel("total samples N x K")
        ax.grid(True, which="both", ls=":", lw=0.5, alpha=0.5)
        ax.set_title(f"canvas {w}x{h}", fontsize=10)
        set_style(ax)
    axes[0].set_ylabel("frame time, min-of-7 (ms)")
    axes[0].legend(fontsize=7.8, frameon=False, loc="upper left")
    fig.suptitle("frame = a + b*(N x K) + c*K fits and the raster/GL crossover (dotted)",
                 fontsize=10.5)
    fig.tight_layout(rect=[0, 0, 1, 0.93])
    fig.savefig(HERE / "fig_rgl_crossover.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_redraw(rows, red):
    keys = [(m, p, c) for c in (1, 10) for p in (100000, 1000000) for m in ("raster", "gl")]
    keys = [k for k in keys if k in red]
    labels = [f"{m}\n{fmt_pts(p)}x{c}" for m, p, c in keys]
    x = np.arange(len(keys))
    fig, ax = plt.subplots(figsize=(10.4, 4.0))
    width = 0.26
    for i, redraw in enumerate(("full", "direct", "window")):
        vals = [red[k][redraw] for k in keys]
        ax.bar(x + (i - 1) * width, vals, width, label=redraw,
               color=("#4c72b0", "#dd8452", "#55a868")[i], edgecolor="white")
        for xi, v in zip(x + (i - 1) * width, vals):
            if np.isfinite(v):
                ax.text(xi, v * 1.08, f"{v:.2f}", ha="center", fontsize=7)
    ax.set_yscale("log")
    ax.set_xticks(x)
    ax.set_xticklabels(labels, fontsize=8)
    ax.set_ylabel("frame time (ms, log)")
    ax.set_ylim(0.08, 300)
    ax.legend(fontsize=9, frameon=False)
    ax.set_title("redraw strategies, 900x500: full replot vs QwtPlotDirectPainter "
                 "(redraws curve 0 only) vs sliding window (span = N/4)\n"
                 "GL 'direct' shows the bimodal low branch - unreliable on QOpenGLWidget, "
                 "see report section 6", fontsize=9.5)
    set_style(ax)
    fig.tight_layout()
    fig.savefig(HERE / "fig_rgl_redraw.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_knobs(rows):
    fig, axes = plt.subplots(2, 3, figsize=(12.6, 6.8))

    def grouped(ax, groups, title):
        """groups: list of (xlabel, [(legend, value)])"""
        x = np.arange(len(groups))
        legends = []
        for _, series in groups:
            for lg, _ in series:
                if lg not in legends:
                    legends.append(lg)
        width = 0.8 / max(len(legends), 1)
        for li, lg in enumerate(legends):
            vals = []
            for _, series in groups:
                v = next((val for name, val in series if name == lg), np.nan)
                vals.append(v)
            ax.bar(x + li * width, vals, width, label=lg, edgecolor="white",
                   color=plt.cm.tab10(li / max(len(legends), 1)))
            for xi, v in zip(x + li * width, vals):
                if np.isfinite(v) and v > 0:
                    ax.text(xi, v * 1.06, f"{v:.2f}", ha="center", fontsize=6.4)
        ax.set_xticks(x + width * (len(legends) - 1) / 2)
        ax.set_xticklabels([g[0] for g in groups], fontsize=7.4)
        ax.set_yscale("log")
        ax.set_ylabel("ms/frame (min-of-N)", fontsize=8)
        ax.set_title(title, fontsize=9)
        ax.legend(fontsize=7, frameon=False)
        ax.tick_params(labelsize=7.5)
        set_style(ax)

    wcs = ((100000, 1), (1000000, 1), (100000, 10), (1000000, 10))

    # (a) raster BackingStore on/off (paired supp2)
    groups = []
    for pts, curves in ((100000, 1), (1000000, 10)):
        on = one(rows, mode="raster", redraw="full", points=pts, curves=curves,
                 canvas_w=900, canvas_h=500, filter="lttb", aa=0, cache=1,
                 immediate=0, **SUPP)
        off = one(rows, mode="raster", redraw="full", points=pts, curves=curves,
                  canvas_w=900, canvas_h=500, filter="lttb", aa=0, cache=0,
                  immediate=0, **SUPP)
        groups.append((f"{fmt_pts(pts)}x{curves}", [
            ("cache on", on["iter_min_ms"] if on else np.nan),
            ("cache off", off["iter_min_ms"] if off else np.nan)]))
    grouped(axes[0][0], groups, "(a) raster: BackingStore attribute (full replot)")

    # (b) ImmediatePaint (paired supp)
    groups = []
    for mode in ("raster", "gl"):
        for pts, curves in ((100000, 1), (1000000, 10)):
            off = one(rows, mode=mode, redraw="full", points=pts, curves=curves,
                      canvas_w=900, canvas_h=500, filter="lttb", aa=0, cache=1,
                      immediate=0, **SUPP)
            on = one(rows, mode=mode, redraw="full", points=pts, curves=curves,
                     canvas_w=900, canvas_h=500, filter="lttb", aa=0, cache=1,
                     immediate=1, **SUPP)
            groups.append((f"{mode} {fmt_pts(pts)}x{curves}", [
                ("update()", off["iter_min_ms"] if off else np.nan),
                ("repaint()", on["iter_min_ms"] if on else np.nan)]))
    grouped(axes[0][1], groups, "(b) ImmediatePaint: update() vs repaint()")

    # (c) MSAA samples (paired supp, gl)
    groups = []
    for pts, curves in wcs:
        series = []
        for msaa in (0, 1, 4):
            r = one(rows, mode="gl", redraw="full", points=pts, curves=curves,
                    canvas_w=900, canvas_h=500, filter="lttb", aa=0, cache=1,
                    immediate=0, msaa=msaa, finish=0, **SUPP)
            series.append((f"{msaa}x MSAA", r["iter_min_ms"] if r else np.nan))
        groups.append((f"{fmt_pts(pts)}x{curves}", series))
    grouped(axes[0][2], groups, "(c) GL: MSAA samples of the Qwt FBO")

    # (d) glFinish / FBO reuse (paired supp, gl, msaa=4)
    groups = []
    for pts, curves in wcs:
        base = one(rows, mode="gl", redraw="full", points=pts, curves=curves,
                   canvas_w=900, canvas_h=500, filter="lttb", aa=0, cache=1,
                   immediate=0, msaa=4, finish=0, **SUPP)
        fin = one(rows, mode="gl", redraw="full", points=pts, curves=curves,
                  canvas_w=900, canvas_h=500, filter="lttb", aa=0, cache=1,
                  immediate=0, msaa=4, finish=1, **SUPP)
        nobs = one(rows, mode="gl", redraw="full", points=pts, curves=curves,
                   canvas_w=900, canvas_h=500, filter="lttb", aa=0, cache=0,
                   immediate=0, msaa=4, finish=0, **SUPP)
        groups.append((f"{fmt_pts(pts)}x{curves}", [
            ("default", base["iter_min_ms"] if base else np.nan),
            ("+glFinish", fin["iter_min_ms"] if fin else np.nan),
            ("no FBO reuse", nobs["iter_min_ms"] if nobs else np.nan)]))
    grouped(axes[1][0], groups, "(d) GL: glFinish and BackingStore (FBO reuse)")

    # (e) antialiasing (paired supp)
    groups = []
    for mode in ("raster", "gl"):
        for pts, curves in wcs:
            off = one(rows, mode=mode, redraw="full", points=pts, curves=curves,
                      canvas_w=900, canvas_h=500, filter="lttb", aa=0, cache=1,
                      immediate=0, **SUPP)
            on = one(rows, mode=mode, redraw="full", points=pts, curves=curves,
                     canvas_w=900, canvas_h=500, filter="lttb", aa=1, cache=1,
                     immediate=0, **SUPP)
            groups.append((f"{mode} {fmt_pts(pts)}x{curves}", [
                ("AA off", off["iter_min_ms"] if off else np.nan),
                ("AA on", on["iter_min_ms"] if on else np.nan)]))
    grouped(axes[1][1], groups, "(e) RenderAntialiased curve render hint")

    # (f) downsampling filter (main block, adjacent runs)
    groups = []
    for mode in ("raster", "gl"):
        for pts in (100000, 1000000):
            r1 = cell(rows, mode, pts, 1, 900, 500)
            r2 = one(rows, mode=mode, redraw="full", points=pts, curves=1,
                     canvas_w=900, canvas_h=500, filter="none", aa=0, cache=1,
                     immediate=0, **MAIN)
            groups.append((f"{mode} {fmt_pts(pts)}x1", [
                ("LTTB on", r1["iter_min_ms"] if r1 else np.nan),
                ("no filter", r2["iter_min_ms"] if r2 else np.nan)]))
    grouped(axes[1][2], groups, "(f) LTTB downsampling vs full point count")

    fig.suptitle("pipeline knobs, 900x500 canvas (paired supplementary block except panel f)",
                 fontsize=11)
    fig.tight_layout(rect=[0, 0, 1, 0.95])
    fig.savefig(HERE / "fig_rgl_knobs.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def retry_report(rows):
    retried = [r for r in rows if r["retries"] > 0]
    print(f"\n=== protocol: {len(rows)} cells, {len(retried)} needed re-measurement "
          f"(iter spread > 30%) ===")
    for r in retried:
        print(f"  {r['mode']:>9} {r['redraw']:>6} {fmt_pts(r['points']):>5} x{r['curves']:<3} "
              f"{r['canvas_w']}x{r['canvas_h']} frames={r['frames']} retries={r['retries']} "
              f"iter_min={r['iter_min_ms']:.3f} iter_med={r['iter_med_ms']:.3f}")
    spread = []
    for r in rows:
        if r["iter_min_ms"] > 0:
            spread.append((r["iter_med_ms"] - r["iter_min_ms"]) / r["iter_min_ms"])
    if spread:
        spread = np.array(spread)
        print(f"  iter spread (med-min)/min over all cells: median={np.median(spread):.3f} "
              f"p90={np.percentile(spread, 90):.3f} max={spread.max():.3f}")


def main():
    rows = load()
    print(f"loaded {len(rows)} result rows from {CSV.name} "
          f"({len(sel(rows, frames=30))} main + {len(sel(rows, frames=31))} supplementary "
          f"+ {len(sel(rows, frames=32))} control)")

    main_matrix_table(rows, 900, 500)
    main_matrix_table(rows, 1920, 1080)
    empties = empty_scene(rows)
    dec = decomposition(rows, 900, 500)
    fits900 = fit_models(rows, 900, 500)
    fits1080 = fit_models(rows, 1920, 1080)
    empirical_crossover(rows, 900, 500)
    empirical_crossover(rows, 1920, 1080)
    red = redraw_subset(rows)
    size_scaling(rows)

    knob_table(rows, "direct-painter variants (1M points, 900x500, main block)", [
        ("raster direct, cache on", dict(mode="raster", redraw="direct", points=1000000, curves=1, cache=1, copybs=0, frames=30)),
        ("raster direct, cache off", dict(mode="raster", redraw="direct", points=1000000, curves=1, cache=0, copybs=0, frames=30)),
        ("raster direct, CopyBackingStore", dict(mode="raster", redraw="direct", points=1000000, curves=1, cache=1, copybs=1, frames=30)),
        ("gl direct (bimodal - see report)", dict(mode="gl", redraw="direct", points=1000000, curves=1, cache=1, frames=30)),
        ("raster full  (reference)", dict(mode="raster", redraw="full", points=1000000, curves=1, cache=1, immediate=0, aa=0, filter="lttb", frames=30)),
        ("gl full      (reference)", dict(mode="gl", redraw="full", points=1000000, curves=1, cache=1, immediate=0, aa=0, filter="lttb", msaa=4, frames=30)),
    ])
    knob_table(rows, "no-filter stress (1 curve, 900x500, adjacent runs)", [
        ("raster 100k filter=none", dict(mode="raster", redraw="full", points=100000, curves=1, cache=1, immediate=0, aa=0, filter="none", frames=30)),
        ("gl 100k filter=none", dict(mode="gl", redraw="full", points=100000, curves=1, cache=1, immediate=0, aa=0, filter="none", msaa=4, frames=30)),
        ("raster 1M filter=none", dict(mode="raster", redraw="full", points=1000000, curves=1, cache=1, immediate=0, aa=0, filter="none", frames=30)),
        ("gl 1M filter=none", dict(mode="gl", redraw="full", points=1000000, curves=1, cache=1, immediate=0, aa=0, filter="none", msaa=4, frames=30)),
    ])
    supp_analysis(rows)
    retry_report(rows)

    fits_all = {}
    fits_all.update(fits900)
    fits_all.update(fits1080)

    fig_fps(rows)
    fig_breakdown(rows, dec)
    fig_stages(rows)
    fig_crossover(rows, fits_all)
    fig_redraw(rows, red)
    fig_knobs(rows)
    print("\nfigures written to", HERE)


if __name__ == "__main__":
    main()
