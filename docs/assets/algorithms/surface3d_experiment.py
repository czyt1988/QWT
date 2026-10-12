# -*- coding: utf-8 -*-
"""
Analysis for the Qwt3DSurface VBO/VAO pipeline benchmark.

    python surface3d_experiment.py

Reads surface3d_results.csv (raw output of surface3d_bench.cpp; environment
and protocol are recorded in the CSV header comments) and produces every
number quoted in docs/zh/tech-report/3d-surface-pipeline.md plus figures:

    fig_3d_fps_scaling.png      frame time / FPS vs vertex count per style,
                                submit view and GPU-completion (glFinish) view
    fig_3d_frame_breakdown.png  NOPLOT floor vs surface-attributable frame
                                time (glFinish view), pass split, and the
                                submit-vs-finish gap
    fig_3d_upload_stages.png    loadFromData / first-draw build+upload /
                                steady draw / recolor / resolution rebuild
                                vs vertex count, with effective bandwidths
    fig_3d_knobs.png            lighting / MSAA / coord system / resolution
                                deltas (glFinish view)
    fig_3d_export.png           gl2ps export time & file size vs triangle
                                count, PNG raster export for comparison

Only numpy + matplotlib; Agg backend; all figure labels in English; every
derived number is printed to stdout.
"""

import pathlib

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = pathlib.Path(__file__).parent
CSV = HERE / "surface3d_results.csv"

COLUMNS = ("mode,cols,style,light,coord,finish,res,fmt,sort,rounds,fpp,retries,paints,"
           "load_ms,first_draw_ms,steady_draw_ms,recolor_draw_ms,res2_draw_ms,propset_ms,"
           "frame_min_ms,frame_med_ms,frame_p99_ms,rmed1_ms,rmed2_ms,rmed3_ms,"
           "export_ms,export_bytes,prim_count").split(",")

STYLE_ORDER = ["filled", "filledmesh", "wireframe", "hiddenline", "points", "noplot"]
STYLE_COLORS = {"filled": "#1f77b4", "filledmesh": "#d62728", "wireframe": "#2ca02c",
                "hiddenline": "#9467bd", "points": "#ff7f0e", "noplot": "#7f7f7f"}
SIZES = [32, 64, 128, 256, 512, 1024]


# ------------------------------------------------------------------- loading

def load_rows():
    rows = []
    with open(CSV, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            rows.append(dict(zip(COLUMNS, line.split(","))))
    return rows


def num(rec, key, default=np.nan):
    v = rec.get(key, "")
    if v is None or v == "":
        return default
    try:
        return float(v)
    except ValueError:
        return default


def vertices(c):
    return c * c


def triangles(c):
    return 2 * (c - 1) * (c - 1)


def tri_indices(c):
    return 6 * (c - 1) * (c - 1)


def line_indices(c):
    return 4 * (c - 1) * (c - 1) + 2 * ((c - 1) + (c - 1))


def line_segments(c):
    return line_indices(c) // 2


def vbo_bytes(c):
    return 40 * vertices(c)          # SurfaceVertex = 3+3+4 floats = 40 B


def ebo_bytes(c):
    return 4 * (tri_indices(c) + line_indices(c))


def select(rows, **kw):
    out = []
    for r in rows:
        if all(str(r.get(k, "")) == str(v) for k, v in kw.items()):
            out.append(r)
    return out


def frame_med(rows, cols, style="filledmesh", light=0, coord="box", finish=0,
              res=1, msaa=4):
    """median frame time of a frames run, or NaN when absent"""
    sel = select(rows, mode="frames", cols=str(cols), style=style, light=str(light),
                 coord=coord, finish=str(finish), res=str(res), fmt=f"msaa{msaa}")
    return num(sel[0], "frame_med_ms") if sel else np.nan


def frame_field(rows, field, cols, style="filledmesh", light=0, coord="box",
                finish=0, res=1, msaa=4):
    sel = select(rows, mode="frames", cols=str(cols), style=style, light=str(light),
                 coord=coord, finish=str(finish), res=str(res), fmt=f"msaa{msaa}")
    return num(sel[0], field) if sel else np.nan


def power_fit(x, y):
    """log-log least squares y = A x^b -> (b, A, r2)"""
    x = np.asarray(x, float)
    y = np.asarray(y, float)
    m = np.isfinite(x) & np.isfinite(y) & (x > 0) & (y > 0)
    if m.sum() < 2:
        return np.nan, np.nan, np.nan
    lx, ly = np.log(x[m]), np.log(y[m])
    b, la = np.polyfit(lx, ly, 1)
    pred = la + b * lx
    ss_res = float(np.sum((ly - pred) ** 2))
    ss_tot = float(np.sum((ly - ly.mean()) ** 2))
    r2 = 1.0 - ss_res / ss_tot if ss_tot > 0 else np.nan
    return float(b), float(np.exp(la)), r2


def affine_fit(x, y):
    x = np.asarray(x, float)
    y = np.asarray(y, float)
    m = np.isfinite(x) & np.isfinite(y)
    k, t0 = np.polyfit(x[m], y[m], 1)
    pred = t0 + k * x[m]
    ss_res = float(np.sum((y[m] - pred) ** 2))
    ss_tot = float(np.sum((y[m] - y[m].mean()) ** 2))
    r2 = 1.0 - ss_res / ss_tot if ss_tot > 0 else np.nan
    return float(t0), float(k), r2


# ------------------------------------------------------------------ analysis

def analyze(rows):
    print("=" * 78)
    print("A. analytic mesh quantities (index loops of Qwt3DSurface::buildVBO)")
    print("=" * 78)
    print(f"{'cols':>5} {'vertices':>9} {'triangles':>10} {'triIdx':>9} {'lineIdx':>9} "
          f"{'lineSegs':>9} {'VBO MB':>7} {'EBO MB':>7}")
    for c in SIZES:
        print(f"{c:>5} {vertices(c):>9} {triangles(c):>10} {tri_indices(c):>9} "
              f"{line_indices(c):>9} {line_segments(c):>9} "
              f"{vbo_bytes(c)/1e6:>7.2f} {ebo_bytes(c)/1e6:>7.2f}")

    # ---- B. frame times, submit view (no glFinish) ----
    print()
    print("=" * 78)
    print("B. frame time, SUBMIT view (finish off; median of 3x100 rotating frames, ms)")
    print("   NOTE: at large meshes this under-reports the sustained rate; GPU work")
    print("   hides in the command queue (see p99 column and section C)")
    print("=" * 78)
    print(f"{'cols':>5} " + " ".join(f"{s:>11}" for s in STYLE_ORDER) + f" {'p99(fmesh)':>11}")
    tab_submit = {}
    for c in SIZES:
        cells = []
        for s in STYLE_ORDER:
            v = frame_med(rows, c, style=s, finish=0)
            tab_submit[(c, s)] = v
            cells.append(f"{v:>11.3f}" if np.isfinite(v) else f"{'-':>11}")
        p99 = frame_field(rows, "frame_p99_ms", c, finish=0)
        print(f"{c:>5} " + " ".join(cells) + f" {p99:>11.1f}")
    print("round-median jitter retries across all frames rows: "
          f"{int(sum(num(r, 'retries', 0) for r in select(rows, mode='frames')))} "
          "(protocol repeats the whole run when max/min round median > 1.5)")

    # ---- C. frame times, GPU-completion view ----
    print()
    print("=" * 78)
    print("C. frame time, GPU-COMPLETION view (glFinish at end of paintGL, ms)")
    print("=" * 78)
    print(f"{'cols':>5} " + " ".join(f"{s:>11}" for s in STYLE_ORDER))
    tab_fin = {}
    for c in SIZES:
        cells = []
        for s in STYLE_ORDER:
            v = frame_med(rows, c, style=s, finish=1)
            tab_fin[(c, s)] = v
            cells.append(f"{v:>11.3f}" if np.isfinite(v) else f"{'-':>11}")
        print(f"{c:>5} " + " ".join(cells))

    print()
    print("FPS (1000/median), GPU-completion view:")
    print(f"{'cols':>5} " + " ".join(f"{s:>11}" for s in STYLE_ORDER))
    for c in SIZES:
        cells = []
        for s in STYLE_ORDER:
            v = tab_fin.get((c, s), np.nan)
            cells.append(f"{1000/v:>11.1f}" if np.isfinite(v) and v > 0 else f"{'-':>11}")
        print(f"{c:>5} " + " ".join(cells))

    print()
    print("power-law fits frame_med = A * V^b (V = cols^2):")
    for tab, name in ((tab_submit, "submit view"), (tab_fin, "glFinish view")):
        print(f"  [{name}]")
        for s in STYLE_ORDER:
            xs = [vertices(c) for c in SIZES]
            ys = [tab.get((c, s), np.nan) for c in SIZES]
            if all(np.isfinite(y) for y in ys):
                b, A, r2 = power_fit(xs, ys)
                print(f"    {s:>11}: b = {b:.3f}  A = {A:.4g} ms  R^2 = {r2:.4f}")
    print("affine fits frame_med = t0 + k*V  [glFinish view]:")
    for s in STYLE_ORDER:
        xs = [vertices(c) for c in SIZES]
        ys = [tab_fin.get((c, s), np.nan) for c in SIZES]
        if all(np.isfinite(y) for y in ys):
            t0, k, r2 = affine_fit(xs, ys)
            print(f"    {s:>11}: t0 = {t0:.3f} ms   k = {k*1e6:.3f} ms/Mvertex   R^2 = {r2:.4f}")
    ys = [tab_fin.get((c, "filledmesh"), np.nan) for c in SIZES]
    if all(np.isfinite(y) for y in ys):
        b512 = np.log(ys[-1] / ys[-2]) / np.log(4)
        b128 = np.log(ys[-2] / ys[2]) / np.log(16)
        print(f"    local exponent 512->1024 (filledmesh, finish): {b512:.3f}")
        print(f"    local exponent 128->512  (filledmesh, finish): {b128:.3f}")

    # ---- D. decomposition ----
    print()
    print("=" * 78)
    print("D. decomposition (glFinish view): floor = NOPLOT (clear + coordinate")
    print("   system + labels + compose + sync); surface = filledmesh - floor")
    print("=" * 78)
    print(f"{'cols':>5} {'floor':>8} {'filledmesh':>11} {'surface':>9} "
          f"{'tri pass':>9} {'line pass':>10}")
    for c in SIZES:
        fl = tab_fin.get((c, "noplot"), np.nan)
        fm = tab_fin.get((c, "filledmesh"), np.nan)
        fi = tab_fin.get((c, "filled"), np.nan)
        s1 = fm - fl if np.isfinite(fm) and np.isfinite(fl) else np.nan
        tp = fi - fl if np.isfinite(fi) and np.isfinite(fl) else np.nan
        lp = fm - fi if np.isfinite(fm) and np.isfinite(fi) else np.nan
        f = lambda v: f"{v:>9.3f}" if np.isfinite(v) else f"{'-':>9}"
        print(f"{c:>5} {fl:>8.3f} {fm:>11.3f} {f(s1)} {f(tp)} {f(lp):>10}")

    print()
    print("submit-vs-finish gap (filledmesh, queue masking):")
    for c in SIZES:
        a = tab_submit.get((c, "filledmesh"), np.nan)
        b = tab_fin.get((c, "filledmesh"), np.nan)
        p99 = frame_field(rows, "frame_p99_ms", c, finish=0)
        if np.isfinite(a) and np.isfinite(b):
            print(f"  cols={c:>4}: submit {a:>7.3f} ms | glFinish {b:>7.3f} ms "
                  f"| submit p99 {p99:>7.1f} ms")

    # ---- E. knobs ----
    print()
    print("=" * 78)
    print("E. knobs at 1024^2 (median frame ms; submit | glFinish)")
    print("=" * 78)
    variants = [
        ("baseline (box, MSAA4, light off, res 1)", dict()),
        ("lighting on (Blinn-Phong frag)", dict(light=1)),
        ("coordinate system NOCOORD", dict(coord="nocoord")),
        ("MSAA 0", dict(msaa=0)),
        ("resolution 2 (index decimation)", dict(res=2)),
        ("resolution 4", dict(res=4)),
    ]
    for label, kw in variants:
        s = frame_med(rows, 1024, finish=0, **kw)
        f = frame_med(rows, 1024, finish=1, **kw)
        print(f"  {label:<42} {s:>8.3f} | {f:>8.3f}")
    print("lighting across sizes (glFinish view, filledmesh):")
    for c in SIZES:
        off = frame_med(rows, c, finish=1)
        on = frame_med(rows, c, light=1, finish=1)
        if np.isfinite(on):
            print(f"  cols={c:>4}: off {off:.3f}  on {on:.3f}  delta {on-off:+.3f}")
    print("lighting across sizes (submit view, filled style):")
    for c in SIZES:
        off = frame_med(rows, c, style="filled", light=0, finish=0)
        on = frame_med(rows, c, style="filled", light=1, finish=0)
        print(f"  cols={c:>4}: off {off:.3f}  on {on:.3f}  delta {on-off:+.3f}")
    print("lighting across sizes (submit view, filledmesh style):")
    for c in SIZES:
        off = frame_med(rows, c, style="filledmesh", light=0, finish=0)
        on = frame_med(rows, c, style="filledmesh", light=1, finish=0)
        print(f"  cols={c:>4}: off {off:.3f}  on {on:.3f}  delta {on-off:+.3f}")

    # ---- F. upload ----
    print()
    print("=" * 78)
    print("F. upload path (median of 3 reps; glFinish inside each stage, ms)")
    print("=" * 78)
    print(f"{'cols':>5} {'load CPU':>9} {'first draw':>11} {'steady draw':>12} "
          f"{'recolor':>9} {'res2 rebuild':>13} {'propset':>9}")
    up = {}
    for c in SIZES:
        sel = select(rows, mode="upload", cols=str(c))
        if not sel:
            continue
        r0 = sel[0]
        up[c] = dict(load=num(r0, "load_ms"), first=num(r0, "first_draw_ms"),
                     steady=num(r0, "steady_draw_ms"), recolor=num(r0, "recolor_draw_ms"),
                     res2=num(r0, "res2_draw_ms"), prop=num(r0, "propset_ms"))
        print(f"{c:>5} {up[c]['load']:>9.3f} {up[c]['first']:>11.3f} {up[c]['steady']:>12.3f} "
              f"{up[c]['recolor']:>9.3f} {up[c]['res2']:>13.3f} {up[c]['prop']:>9.4f}")
    print()
    print("derived: buildVBO+upload = first - steady; per-vertex cost; effective BW")
    for c in SIZES:
        if c not in up:
            continue
        V = vertices(c)
        build = max(up[c]["first"] - up[c]["steady"], 1e-9)
        bw_load = 56.0 * V / (up[c]["load"] / 1e3) / 1e9
        bw_build = (40.0 * V + 40.0 * V) / (build / 1e3) / 1e9  # VBO + EBO bytes
        print(f"  cols={c:>4}: load {up[c]['load']*1e6/V:>6.1f} ns/vtx (~{bw_load:>5.1f} GB/s), "
              f"build+upload {build*1e6/V:>6.1f} ns/vtx (~{bw_build:>5.1f} GB/s VBO+EBO), "
              f"steady {up[c]['steady']*1e6/V:>6.2f} ns/vtx, "
              f"recolor {up[c]['recolor']*1e6/V:>6.1f} ns/vtx, "
              f"load+build total {up[c]['load']+build:.3f} ms")
    for key, label in (("load", "loadFromData (CPU)"),
                       ("first", "first draw (build+upload+draw+finish)"),
                       ("recolor", "invalidateColors rebuild"),
                       ("res2", "setResolution(2) rebuild"),
                       ("steady", "steady draw (finish)")):
        xs = [vertices(c) for c in SIZES if c in up]
        ys = [up[c][key] for c in SIZES if c in up]
        if len(xs) >= 3:
            b, A, r2 = power_fit(xs, ys)
            print(f"  power fit {label:<44}: b = {b:.3f}, R^2 = {r2:.4f}")
    print()
    print("full refresh cycle cost (loadFromData + rebuild+upload) per data change:")
    for c in SIZES:
        if c not in up:
            continue
        build = max(up[c]["first"] - up[c]["steady"], 0.0)
        tot = up[c]["load"] + build
        rate = 1000 / tot if tot > 0 else float("inf")
        print(f"  cols={c:>4}: {tot:>8.2f} ms  (<= {rate:.1f} full data refreshes/s)")

    # ---- G. export ----
    print()
    print("=" * 78)
    print("G. export: gl2ps vector (feedback capture) vs PNG raster")
    print("=" * 78)
    print(f"{'fmt':>4} {'sort':>7} {'cols':>5} {'time ms':>10} {'bytes':>11} "
          f"{'svg prims':>10} {'us/tri':>8}")
    for r0 in select(rows, mode="export"):
        c = int(num(r0, "cols"))
        t = num(r0, "export_ms")
        b = num(r0, "export_bytes")
        p = num(r0, "prim_count")
        ntri = triangles(c)
        print(f"{r0['fmt']:>4} {r0['sort']:>7} {c:>5} {t:>10.1f} {b:>11.0f} "
              f"{p:>10.0f} {t*1e3/ntri:>8.2f}")
    pdf = [(int(num(r, "cols")), num(r, "export_ms"), num(r, "export_bytes"))
           for r in select(rows, mode="export", fmt="PDF", sort="simple")]
    if len(pdf) >= 3:
        xs = [triangles(c) for c, _, _ in pdf]
        ts = [t for _, t, _ in pdf]
        bs = [b for _, _, b in pdf]
        bt, _, r2t = power_fit(xs, ts)
        bsz, _, r2s = power_fit(xs, bs)
        print(f"PDF simple-sort: time ~ tri^{bt:.2f} (R^2={r2t:.3f}), "
              f"size ~ tri^{bsz:.2f} (R^2={r2s:.3f})")
    bsp = [(int(num(r, "cols")), num(r, "export_ms"))
           for r in select(rows, mode="export", sort="bsp")]
    for c, t in bsp:
        simple = [tt for cc, tt, _b in pdf if cc == c]
        if simple:
            print(f"BSP vs SIMPLE at cols={c}: {t:.0f} ms vs {simple[0]:.0f} ms "
                  f"({t/simple[0]:.0f}x)")
    svg = [(int(num(r, "cols")), num(r, "export_ms"), num(r, "export_bytes"),
            num(r, "prim_count")) for r in select(rows, mode="export", fmt="SVG")]
    for c, t, b, p in svg:
        print(f"SVG at cols={c}: {p:.0f} captured polygons vs {triangles(c)} source "
              f"triangles + {line_segments(c)} line segments "
              f"(inflation x{p/max(triangles(c),1):.1f})")
    return tab_submit, tab_fin, up


# ------------------------------------------------------------------- figures

def fig_scaling(tab_submit, tab_fin):
    fig, axes = plt.subplots(1, 3, figsize=(15, 4.2))
    for ax, tab, ttl in ((axes[0], tab_submit, "Submit view (no glFinish)"),
                         (axes[1], tab_fin, "GPU-completion view (glFinish in paintGL)")):
        for s in STYLE_ORDER:
            xs = [vertices(c) for c in SIZES]
            ys = [tab.get((c, s), np.nan) for c in SIZES]
            if not any(np.isfinite(y) for y in ys):
                continue
            ax.plot(xs, ys, "o-", color=STYLE_COLORS[s], label=s, ms=4)
        xr = np.array([vertices(32), vertices(1024)], float)
        y0 = tab.get((32, "filledmesh"), np.nan)
        if np.isfinite(y0) and y0 > 0:
            ax.plot(xr, y0 * (xr / xr[0]), "--", color="0.6", lw=1)
            ax.annotate("O(V)", (xr[1] * 0.5, y0 * 14), color="0.4", fontsize=9)
            ax.plot(xr, y0 * (xr / xr[0]) ** 0.5, ":", color="0.6", lw=1)
            ax.annotate("O(V^0.5)", (xr[1] * 0.22, y0 * 3.6), color="0.4", fontsize=9)
        ax.set_xscale("log"); ax.set_yscale("log")
        ax.set_xlabel("vertices V = cols x rows")
        ax.set_ylabel("frame time (ms, median of 3x100)")
        ax.set_title(ttl)
        ax.legend(fontsize=8); ax.grid(alpha=0.3, which="both")
    ax = axes[2]
    for s in STYLE_ORDER:
        ys = [1000.0 / tab_fin[(c, s)]
              if np.isfinite(tab_fin.get((c, s), np.nan)) and tab_fin.get((c, s), 0) > 0
              else np.nan for c in SIZES]
        if any(np.isfinite(y) for y in ys):
            ax.plot(SIZES, ys, "o-", color=STYLE_COLORS[s], label=s, ms=4)
    ax.axhline(60, color="0.4", ls=":", lw=1)
    ax.annotate("60 FPS budget", (33, 63), fontsize=8, color="0.4")
    ax.axhline(30, color="0.4", ls=":", lw=1)
    ax.annotate("30 FPS", (33, 31), fontsize=8, color="0.4")
    ax.set_xscale("log"); ax.set_yscale("log")
    ax.set_xticks(SIZES); ax.set_xticklabels([str(c) for c in SIZES])
    ax.set_xlabel("grid size (cols = rows)")
    ax.set_ylabel("frames per second (GPU-completion)")
    ax.set_title("Interactive rate vs mesh size")
    ax.legend(fontsize=8); ax.grid(alpha=0.3, which="both")
    fig.tight_layout()
    fig.savefig(HERE / "fig_3d_fps_scaling.png", dpi=150)
    plt.close(fig)


def fig_breakdown(rows, tab_fin, tab_submit):
    fig, axes = plt.subplots(1, 3, figsize=(15, 4.2))
    ax = axes[0]
    xs, floor, surf = [], [], []
    for c in SIZES:
        fl = tab_fin.get((c, "noplot"), np.nan)
        fm = tab_fin.get((c, "filledmesh"), np.nan)
        if np.isfinite(fl) and np.isfinite(fm):
            xs.append(c)
            floor.append(fl)
            surf.append(fm - fl)
    ax.plot(xs, floor, "s-", color="#7f7f7f",
            label="NOPLOT floor (clear + coordsys + labels + compose + sync)")
    ax.plot(xs, surf, "o-", color="#d62728",
            label="surface attributable (filledmesh - floor)")
    ax.set_xscale("log"); ax.set_xticks(xs); ax.set_xticklabels([str(c) for c in xs])
    ax.set_xlabel("grid size (cols = rows)"); ax.set_ylabel("frame time (ms)")
    ax.set_title("Decomposition, glFinish view")
    ax.legend(fontsize=7.5); ax.grid(alpha=0.3)
    ax = axes[1]
    xs2, tri, line = [], [], []
    for c in SIZES:
        fl = tab_fin.get((c, "noplot"), np.nan)
        fi = tab_fin.get((c, "filled"), np.nan)
        fm = tab_fin.get((c, "filledmesh"), np.nan)
        if np.isfinite(fl) and np.isfinite(fi) and np.isfinite(fm):
            xs2.append(c); tri.append(fi - fl); line.append(fm - fi)
    if xs2:
        ax.plot(xs2, tri, "o-", color="#1f77b4", label="triangle pass (filled - floor)")
        ax.plot(xs2, line, "^-", color="#2ca02c",
                label="mesh-line pass (filledmesh - filled)")
        ax.set_xscale("log"); ax.set_xticks(xs2); ax.set_xticklabels([str(c) for c in xs2])
        ax.set_xlabel("grid size"); ax.set_ylabel("pass time (ms)")
        ax.set_title("Pass split: the mesh-line pass costs more than fills")
        ax.legend(fontsize=8); ax.grid(alpha=0.3)
    ax = axes[2]
    xs3, sub, fin = [], [], []
    for c in SIZES:
        s = tab_submit.get((c, "filledmesh"), np.nan)
        f = tab_fin.get((c, "filledmesh"), np.nan)
        if np.isfinite(s) and np.isfinite(f):
            xs3.append(c); sub.append(s); fin.append(f)
    ax.plot(xs3, sub, "o-", color="#1f77b4", label="submit view (median)")
    ax.plot(xs3, fin, "s-", color="#d62728", label="glFinish view (median)")
    p99s = [frame_field(rows, "frame_p99_ms", c, finish=0) for c in xs3]
    ax.plot(xs3, p99s, "v--", color="#ff7f0e", label="submit view p99 (queue stalls)")
    ax.set_xscale("log"); ax.set_yscale("log")
    ax.set_xticks(xs3); ax.set_xticklabels([str(c) for c in xs3])
    ax.set_xlabel("grid size"); ax.set_ylabel("frame time (ms)")
    ax.set_title("filledmesh: what the CPU sees vs what the GPU pays")
    ax.legend(fontsize=8); ax.grid(alpha=0.3, which="both")
    fig.tight_layout()
    fig.savefig(HERE / "fig_3d_frame_breakdown.png", dpi=150)
    plt.close(fig)


def fig_upload(up):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11.5, 4.3))
    cs = sorted(up.keys())
    xs = [vertices(c) for c in cs]
    for key, lab, col, mk in (
            ("load", "loadFromData (CPU: copy + normals)", "#1f77b4", "o"),
            ("first", "first draw (buildVBO + upload + draw + finish)", "#d62728", "s"),
            ("recolor", "invalidateColors rebuild", "#9467bd", "^"),
            ("res2", "setResolution(2) rebuild", "#8c564b", "v"),
            ("steady", "steady draw (uniforms + draw + finish)", "#2ca02c", "D")):
        ax1.plot(xs, [up[c][key] for c in cs], mk + "-", color=col, label=lab, ms=4)
    ax1.set_xscale("log"); ax1.set_yscale("log")
    ax1.set_xlabel("vertices V = cols x rows")
    ax1.set_ylabel("stage time (ms)")
    ax1.legend(fontsize=7.5); ax1.grid(alpha=0.3, which="both")
    ax1.set_title("Data change = full CPU rebuild + full re-upload")
    bw_load = [56.0 * vertices(c) / (up[c]["load"] / 1e3) / 1e9 if up[c]["load"] > 0
               else np.nan for c in cs]
    bw_build = [(vbo_bytes(c) + ebo_bytes(c)) /
                (max(up[c]["first"] - up[c]["steady"], 1e-9) / 1e3) / 1e9 for c in cs]
    ax2.plot(xs, bw_load, "o-", color="#1f77b4", label="loadFromData (~56 B/vtx traffic)")
    ax2.plot(xs, bw_build, "s-", color="#d62728", label="buildVBO+upload (VBO+EBO bytes)")
    ax2.set_xscale("log")
    ax2.set_xlabel("vertices V"); ax2.set_ylabel("effective bandwidth (GB/s)")
    ax2.legend(fontsize=8); ax2.grid(alpha=0.3, which="both")
    ax2.set_title("Effective bandwidth (iGPU shared memory, single thread)")
    fig.tight_layout()
    fig.savefig(HERE / "fig_3d_upload_stages.png", dpi=150)
    plt.close(fig)


def fig_knobs(rows, tab_fin):
    fig, axes = plt.subplots(1, 3, figsize=(14, 4.0))
    for style, ax in zip(("filledmesh", "filled"), (axes[0], axes[1])):
        off = [tab_fin.get((c, style), np.nan) for c in SIZES]
        on = [frame_med(rows, c, style=style, light=1, finish=1) for c in SIZES]
        m = np.isfinite(off)
        ax.plot(np.array(SIZES)[m], np.array(off)[m], "o-", color="#1f77b4",
                label="lighting off")
        mo = np.isfinite(on)
        if mo.any():
            ax.plot(np.array(SIZES)[mo], np.array(on)[mo], "s-", color="#d62728",
                    label="lighting on")
        ax.set_xscale("log"); ax.set_xticks(SIZES)
        ax.set_xticklabels([str(c) for c in SIZES], fontsize=8)
        ax.set_xlabel("grid size"); ax.set_ylabel("frame time (ms, glFinish)")
        ax.set_title(f"{style}: Blinn-Phong fragment lighting delta")
        ax.legend(fontsize=8); ax.grid(alpha=0.3)
    ax = axes[2]
    labels, vals, cols_ = [], [], []
    for lab, kw, col in (("baseline", dict(), "#4c72b0"),
                         ("lighting on", dict(light=1), "#c44e52"),
                         ("NOCOORD", dict(coord="nocoord"), "#55a868"),
                         ("MSAA 0", dict(msaa=0), "#8172b2"),
                         ("resolution 2", dict(res=2), "#ccb974"),
                         ("resolution 4", dict(res=4), "#64b5cd")):
        v = frame_med(rows, 1024, finish=1, **kw)
        if np.isfinite(v):
            labels.append(lab); vals.append(v); cols_.append(col)
    ax.bar(range(len(vals)), vals, color=cols_)
    ax.set_xticks(range(len(labels)))
    ax.set_xticklabels(labels, fontsize=7.5, rotation=20, ha="right")
    ax.set_ylabel("frame time (ms, glFinish)")
    ax.set_title("1024^2 filledmesh: knob deltas (GPU-completion view)")
    ax.grid(alpha=0.3, axis="y")
    fig.tight_layout()
    fig.savefig(HERE / "fig_3d_knobs.png", dpi=150)
    plt.close(fig)


def fig_export(rows):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11.5, 4.3))
    styles = {"PDF": ("#1f77b4", "o", "-"), "SVG": ("#d62728", "s", "-"),
              "EPS": ("#2ca02c", "^", "-"), "png": ("#7f7f7f", "D", "-")}
    for f, (col, mk, ls) in styles.items():
        sel = select(rows, mode="export", fmt=f, sort="simple")
        if not sel:
            continue
        cs = sorted(int(num(r, "cols")) for r in sel)
        ts = [num(select(rows, mode="export", fmt=f, sort="simple", cols=str(c))[0],
                  "export_ms") for c in cs]
        bs = [num(select(rows, mode="export", fmt=f, sort="simple", cols=str(c))[0],
                  "export_bytes") for c in cs]
        xs = [triangles(c) for c in cs]
        ax1.plot(xs, ts, mk + ls, color=col, label=f if f != "png" else "PNG (raster)")
        ax2.plot(xs, [b / 1e6 for b in bs], mk + ls, color=col,
                 label=f if f != "png" else "PNG (raster)")
    bsp = select(rows, mode="export", sort="bsp")
    if bsp:
        cs = sorted(int(num(r, "cols")) for r in bsp)
        ts = [num(select(rows, mode="export", sort="bsp", cols=str(c))[0], "export_ms")
              for c in cs]
        ax1.plot([triangles(c) for c in cs], ts, "v--", color="#9467bd",
                 label="PDF BSP sort")
    for ax, ylab, ttl in ((ax1, "export time (ms)", "Export time vs source triangle count"),
                          (ax2, "file size (MB)", "Export file size")):
        ax.set_xscale("log"); ax.set_yscale("log")
        ax.set_xlabel("source triangles 2(cols-1)^2")
        ax.set_ylabel(ylab); ax.set_title(ttl)
        ax.legend(fontsize=8); ax.grid(alpha=0.3, which="both")
    fig.tight_layout()
    fig.savefig(HERE / "fig_3d_export.png", dpi=150)
    plt.close(fig)


def main():
    rows = load_rows()
    tab_submit, tab_fin, up = analyze(rows)
    fig_scaling(tab_submit, tab_fin)
    fig_breakdown(rows, tab_fin, tab_submit)
    if up:
        fig_upload(up)
    fig_knobs(rows, tab_fin)
    fig_export(rows)
    print()
    print("figures written: fig_3d_fps_scaling.png fig_3d_frame_breakdown.png "
          "fig_3d_upload_stages.png fig_3d_knobs.png fig_3d_export.png")


if __name__ == "__main__":
    main()
