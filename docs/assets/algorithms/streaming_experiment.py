"""
Analysis for the QWT real-time streaming benchmark (realtime-streaming.md).

    python streaming_experiment.py

Reads streaming_results.csv (written next to this script), aggregates the
per-rep rows to per-config values ("median of medians" protocol), fits the
frame-budget models, prints every number used in the article to stdout and
writes the figures:

    fig_stream_matrix.png     frame time vs production rate per window size
    fig_stream_window.png     frame time vs window size, LTTB on/off
    fig_stream_direct.png     DirectPainter net gain (ring_dp vs ring_replot / _bs)
    fig_stream_latency.png    end-to-end latency distributions
    fig_stream_budget.png     per-tick phase breakdown (gen/commit/draw/flush)
    fig_stream_model.png      fitted frame-budget model vs measurements
    fig_stream_boundary.png   60 FPS sustainability map over (rate, window)

Deterministic: no randomness anywhere; numpy + matplotlib only, Agg backend.
"""

import pathlib
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = pathlib.Path(__file__).parent
CSV = HERE / "streaming_results.csv"

TICK_MS = 10.0          # producer/consumer timer interval used by the bench
BUDGET_MS = 1000.0 / 60.0  # 60 FPS frame budget
CANVAS_PX = 900         # canvas width in px (LTTB budget 4W = 3600 points)
LTTB_OUT = 4 * CANVAS_PX

STRategies = ["ring_dp", "ring_raw", "ring_dp_bs", "ring_replot", "setdata", "window"]
RATES = [1000, 10000, 100000, 1000000]
WINDOWS = [10000, 100000, 1000000]

COL = {
    "ring_dp": "#d62728",
    "ring_raw": "#9467bd",
    "ring_dp_bs": "#e377c2",
    "ring_replot": "#1f77b4",
    "setdata": "#ff7f0e",
    "window": "#2ca02c",
}
MARK = {
    "ring_dp": "o",
    "ring_raw": "s",
    "ring_dp_bs": "D",
    "ring_replot": "^",
    "setdata": "v",
    "window": "P",
}

# ------------------------------------------------------------------ loading


def load():
    rows = []
    header = None
    with open(CSV, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith("#"):
                continue
            if header is None:
                # tolerate leading junk (e.g. compiler stdout captured before the
                # CSV header): only accept the real header line
                if not line.startswith("strategy,rate,window,lttb,"):
                    continue
                header = line.split(",")
                continue
            vals = line.split(",")
            if len(vals) != len(header):
                continue  # truncated tail line (file still being written)
            rows.append(dict(zip(header, vals)))
    if not rows:
        raise SystemExit("no data rows in " + str(CSV))

    data = {}
    for r in rows:
        key = (r["strategy"], int(r["rate"]), int(r["window"]), int(r["lttb"]))
        data.setdefault(key, []).append(r)

    agg = {}
    for key, reps in data.items():
        def med(col):
            return float(np.median([float(x[col]) for x in reps]))
        agg[key] = dict(
            nreps=len(reps),
            reps_run=int(reps[0]["reps_run"]),
            jitter=int(reps[0]["jitter"]),
            frames=med("frames"),
            fps=med("fps_eff"),
            f_med=med("frame_med_ms"),
            f_mean=med("frame_mean_ms"),
            f_p95=med("frame_p95_ms"),
            f_p99=med("frame_p99_ms"),
            f_max=med("frame_max_ms"),
            drops=int(np.median([int(x["drops"]) for x in reps])),
            gen=med("gen_med_ms"),
            commit=med("commit_med_ms"),
            draw=med("draw_med_ms"),
            present=med("present_med_ms"),
            tick=med("tick_med_ms"),
            paint_ev=med("paint_events"),
            bnd=med("boundary_replots"),
            samples=med("samples"),
            lat_med=med("lat_med_ms"),
            lat_p95=med("lat_p95_ms"),
            lat_p99=med("lat_p99_ms"),
            lat_max=med("lat_max_ms"),
            skipped=med("skipped_ticks"),
        )
    return agg


def get(agg, strat, rate, win, lttb=1, field="f_med"):
    return agg[(strat, rate, win, lttb)][field]

# ------------------------------------------------------------------ printing


def print_table(agg):
    print("=" * 110)
    print("AGGREGATED RESULTS (median of per-rep medians; fps = median of per-reps fps_eff)")
    print("=" * 110)
    hdr = ("strategy", "rate", "window", "lttb", "fps", "f_med", "f_p95", "f_p99",
           "drops", "gen", "commit", "draw", "flush", "lat_med", "lat_p99", "bnd", "pnt_ev", "jit")
    print(("%-12s %9s %9s %4s %7s %8s %8s %8s %6s %7s %7s %8s %8s %8s %8s %5s %6s %4s") % hdr)
    for strat in STRategies:
        for rate in RATES:
            for win in WINDOWS:
                for lttb in (1, 0):
                    key = (strat, rate, win, lttb)
                    if key not in agg:
                        continue
                    a = agg[key]
                    print(("%-12s %9d %9d %4d %7.1f %8.3f %8.3f %8.3f %6d %7.3f %7.3f "
                           "%8.3f %8.3f %8.3f %8.3f %5.0f %6.0f %4d") % (
                        strat, rate, win, lttb, a["fps"], a["f_med"], a["f_p95"],
                        a["f_p99"], a["drops"], a["gen"], a["commit"], a["draw"],
                        a["present"], a["lat_med"], a["lat_p99"], a["bnd"],
                        a["paint_ev"], a["jitter"]))
    print()

# ------------------------------------------------------------------ models


def w_eff(rate, win, warm=0.3, dur=3.0):
    """Median of min(win, rate*t) over the measured phase: the window is only
    partially filled when rate*dur < win, and the median frame cost tracks the
    median visible sample count."""
    t = np.linspace(warm, warm + dur, 2001)
    return float(np.median(np.minimum(float(win), rate * t)))


def fit_full_replot_models(agg):
    """Pooled fit over all (rate, window) cells of one strategy, regressed on
    the effective window W_eff (median visible sample count):

      LTTB off: T = a + b * W_eff            (transform + rasterize all points)
      LTTB on : T = a + c * W_eff + d * min(W_eff, 3600)
                (c = per-sample bucket scan, d = per-point polyline raster)

    Coefficient meaning: a = fixed per-frame cost (axes, layout, canvas flush,
    BackingStore blit); the per-sample terms cover commit copy (setdata),
    transform scan and (without LTTB) rasterization.
    """
    out = {}
    for strat in ["ring_replot", "setdata", "window"]:
        for lttb in (1, 0):
            xs, ys, cells = [], [], []
            for rate in RATES:
                for win in WINDOWS:
                    we = w_eff(rate, win)
                    xs.append(we)
                    ys.append(get(agg, strat, rate, win, lttb))
                    cells.append((rate, win, we))
            x = np.array(xs)
            y = np.array(ys)
            if lttb == 0:
                A = np.stack([np.ones_like(x), x / 1e6], axis=1)
            else:
                A = np.stack([np.ones_like(x), x / 1e6,
                              np.minimum(x, float(LTTB_OUT)) / 1e3], axis=1)
            coef, *_ = np.linalg.lstsq(A, y, rcond=None)
            pred = A @ coef
            ss = 1.0 - float(np.sum((y - pred) ** 2)) / float(np.sum((y - y.mean()) ** 2))
            out[(strat, lttb)] = dict(coef=coef, r2=ss, cells=cells, y=y, pred=pred)
    return out


def fit_direct_model(agg):
    """ring_dp per-tick cost: T = a + b*(R*tick) [+ boundary amortization].

    b lumps signal synthesis, ring append, clip-rect bounding scan and the
    incremental segment transform+rasterization per produced sample. Cells
    whose page time W/R is below 0.3 s are excluded: there the page-boundary
    replot fires (almost) every tick and the steady-state model does not apply.
    """
    out = {}
    for lttb in (1, 0):
        pts = []
        for rate in RATES:
            for win in WINDOWS:
                if win / rate < 0.3:
                    continue  # boundary replot every few ticks: not the regime
                T = get(agg, "ring_dp", rate, win, lttb)
                pts.append((rate * TICK_MS / 1000.0, T))
        x = np.array([p[0] for p in pts]) / 1e3  # thousands of samples per tick
        y = np.array([p[1] for p in pts])
        A = np.stack([np.ones_like(x), x], axis=1)
        coef, *_ = np.linalg.lstsq(A, y, rcond=None)
        pred = A @ coef
        ss = 1.0 - float(np.sum((y - pred) ** 2)) / float(np.sum((y - y.mean()) ** 2))
        out[lttb] = (coef, x, y, ss)
    return out


def boundary_60fps(agg, models_full, model_direct):
    """Solve T(W) = 16.67 ms for the full-replot strategies (max window at
    60 FPS) and T(R*tick) = 16.67 for ring_dp (max production rate)."""
    print("=" * 110)
    print("60 FPS BOUNDARIES (frame budget %.2f ms, steady state = filled window)" % BUDGET_MS)
    print("=" * 110)
    for strat in ["ring_replot", "setdata", "window"]:
        for lttb in (1, 0):
            m = models_full[(strat, lttb)]
            coef = m["coef"]
            if lttb == 0:
                a, b = coef[0], coef[1] / 1e6
                wstar = (BUDGET_MS - a) / b if b > 0 else np.inf
                print("full-replot  %-12s lttb=0  T = %.3f + %.3f ms/M  ->  W* = %.0f samples"
                      % (strat, a, b * 1e6 / 1e6, wstar))
            else:
                a, c, d = coef[0], coef[1] / 1e6, coef[2] / 1e3
                # above LTTB saturation the polyline term is constant:
                wstar = (BUDGET_MS - a - d * LTTB_OUT / 1e3) / c if c > 0 else np.inf
                print("full-replot  %-12s lttb=1  T = %.3f + %.3f ms/M + %.4f ms/k-poly  "
                      "->  W* = %.0f samples" % (strat, a, c * 1e6 / 1e6, d, wstar))
    for lttb in (1, 0):
        coef = model_direct[lttb][0]
        a, b = coef[0], coef[1]  # b per 1000 samples/tick
        nstar = (BUDGET_MS - a) / b * 1e3 if b > 0 else np.inf
        rstar = nstar / (TICK_MS / 1000.0)
        print("direct       ring_dp      lttb=%d  a=%.3f ms  b=%.4f ms/ksamples-per-tick  "
              "R* (tick=10ms) = %.0f samples/s" % (lttb, a, b, rstar))
    print()


def latency_model_check(agg):
    """L_med ~ dt_eff/2 + T_frame with dt_eff = max(tick, T_frame + tick overhead)."""
    print("=" * 110)
    print("LATENCY MODEL CHECK  (L_med predicted = dt_eff/2 + T_frame, dt_eff = max(tick, T_frame))")
    print("=" * 110)
    print(("%-12s %8s %8s %4s %9s %9s %9s %9s %8s" % (
        "strategy", "rate", "window", "lt", "T_frame", "L_med", "L_pred", "err", "L_p99")))
    errs = []
    for strat in STRategies:
        for rate in RATES:
            for win in WINDOWS:
                for lttb in (1, 0):
                    key = (strat, rate, win, lttb)
                    if key not in agg:
                        continue
                    T = agg[key]["f_med"]
                    dt = max(TICK_MS, T)
                    lpred = dt / 2.0 + T
                    lmed = agg[key]["lat_med"]
                    err = lpred - lmed
                    errs.append(abs(err))
                    if (rate, win) in [(100000, 100000), (1000000, 1000000),
                                       (1000000, 10000), (1000, 1000000)]:
                        print(("%-12s %8d %8d %4d %9.3f %9.3f %9.3f %9.3f %8.3f") % (
                            strat, rate, win, lttb, T, lmed, lpred, err,
                            agg[key]["lat_p99"]))
    print("mean |err| over all %d cells: %.3f ms   median |err|: %.3f ms" % (
        len(errs), float(np.mean(errs)), float(np.median(errs))))
    print()

# ------------------------------------------------------------------ figures


def set_style(ax):
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.tick_params(labelsize=8.5)
    ax.grid(True, ls=":", lw=0.5, alpha=0.5)


def fig_matrix(agg):
    fig, axes = plt.subplots(1, 3, figsize=(11.5, 3.5), sharey=True)
    for ax, win in zip(axes, WINDOWS):
        for strat in STRategies:
            for lttb, ls in ((1, "-"), (0, "--")):
                y = [get(agg, strat, r, win, lttb) for r in RATES]
                ax.plot(RATES, y, ls, marker=MARK[strat], ms=4, lw=1.2,
                        color=COL[strat], label=(strat + (" (LTTB)" if lttb else " (no LTTB)"))
                        if win == WINDOWS[0] and ls == "-" else None)
        ax.axhline(BUDGET_MS, color="#555555", ls=":", lw=1.2)
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xticks(RATES)
        ax.set_xticklabels(["1k", "10k", "100k", "1M"])
        ax.set_title("window = %s samples" % f"{win:,}", fontsize=9.5)
        ax.set_xlabel("production rate (samples/s)")
        set_style(ax)
    axes[0].set_ylabel("median frame time (ms)")
    axes[0].text(1100, BUDGET_MS * 1.15, "60 FPS budget (16.7 ms)", fontsize=8, color="#555555")
    axes[2].text(1100, 0.055, "solid: LTTB on\ndashed: LTTB off", fontsize=7.5, color="#555555")
    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, ncol=6, fontsize=8, frameon=False,
               loc="lower center", bbox_to_anchor=(0.5, -0.06))
    fig.tight_layout(rect=[0, 0.05, 1, 1])
    fig.savefig(HERE / "fig_stream_matrix.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_window(agg):
    fig, axes = plt.subplots(1, 4, figsize=(13.0, 3.4), sharey=True)
    for ax, rate in zip(axes, RATES):
        for strat in ["ring_dp", "ring_raw", "ring_dp_bs", "ring_replot", "setdata", "window"]:
            for lttb, ls in ((1, "-"), (0, "--")):
                y = [get(agg, strat, rate, w, lttb) for w in WINDOWS]
                ax.plot(WINDOWS, y, ls, marker=MARK[strat], ms=4, lw=1.2, color=COL[strat])
        ax.axhline(BUDGET_MS, color="#555555", ls=":", lw=1.2)
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xticks(WINDOWS)
        ax.set_xticklabels(["10k", "100k", "1M"])
        ax.set_title("R = %s samples/s" % f"{rate:,}", fontsize=9.5)
        ax.set_xlabel("window size (samples)")
        set_style(ax)
    axes[0].set_ylabel("median frame time (ms)")
    fig.suptitle("solid: FilterPointsLTTB on, dashed: off", fontsize=8.5, y=1.02)
    fig.tight_layout()
    fig.savefig(HERE / "fig_stream_window.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_direct(agg):
    fig, axes = plt.subplots(1, 3, figsize=(11.5, 3.5))
    for ax, win in zip(axes, WINDOWS):
        x = np.arange(len(RATES))
        width = 0.26
        for k, (strat, lab) in enumerate([
                ("ring_dp", "DirectPainter (BS off)"),
                ("ring_dp_bs", "DirectPainter (BS on)"),
                ("ring_replot", "full replot / tick")]):
            y = [get(agg, strat, r, win, 1) for r in RATES]
            ax.bar(x + (k - 1) * width, y, width, label=lab, color=COL[strat],
                   edgecolor="white", lw=0.4)
        ax.set_yscale("log")
        ax.set_xticks(x)
        ax.set_xticklabels(["1k", "10k", "100k", "1M"])
        ax.axhline(BUDGET_MS, color="#555555", ls=":", lw=1.2)
        ax.set_title("window = %s, LTTB on" % f"{win:,}", fontsize=9.5)
        ax.set_xlabel("production rate (samples/s)")
        set_style(ax)
    axes[0].set_ylabel("median frame time (ms)")
    axes[0].legend(fontsize=7.5, frameon=False, loc="upper left")
    fig.tight_layout()
    fig.savefig(HERE / "fig_stream_direct.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_latency(agg):
    fig, axes = plt.subplots(1, 2, figsize=(10.5, 3.6))

    ax = axes[0]
    strat_show = ["ring_dp", "ring_raw", "ring_dp_bs", "ring_replot", "setdata", "window"]
    x = np.arange(len(strat_show))
    rate, win = 1000000, 100000
    for k, (field, lab, alpha) in enumerate([
            ("lat_med", "median", 1.0), ("lat_p95", "p95", 0.75), ("lat_p99", "p99", 0.5)]):
        for si, strat in enumerate(strat_show):
            v = get(agg, strat, rate, win, 1, field)
            ax.bar(si + (k - 1) * 0.25, v, 0.25, color=COL[strat], alpha=alpha,
                   edgecolor="white", lw=0.4,
                   label=lab if si == 0 else None)
    ax.set_yscale("log")
    ax.set_xticks(x)
    ax.set_xticklabels(strat_show, fontsize=8, rotation=18)
    ax.set_ylabel("end-to-end latency (ms)")
    ax.set_title("latency distribution, R = 1M/s, W = 100k, LTTB on", fontsize=9.5)
    ax.legend(fontsize=8, frameon=False, ncol=3)
    set_style(ax)

    ax = axes[1]
    for strat in ["ring_dp", "ring_replot", "setdata", "window"]:
        y = [get(agg, strat, r, 100000, 1, "lat_med") for r in RATES]
        y99 = [get(agg, strat, r, 100000, 1, "lat_p99") for r in RATES]
        ax.plot(RATES, y, "-o", ms=4, lw=1.3, color=COL[strat], label=strat + " median")
        ax.plot(RATES, y99, "--", lw=0.9, color=COL[strat], alpha=0.55)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xticks(RATES)
    ax.set_xticklabels(["1k", "10k", "100k", "1M"])
    ax.set_xlabel("production rate (samples/s)")
    ax.set_ylabel("latency (ms)")
    ax.set_title("latency vs rate, W = 100k (dashed: p99)", fontsize=9.5)
    ax.legend(fontsize=7.5, frameon=False)
    set_style(ax)

    fig.tight_layout()
    fig.savefig(HERE / "fig_stream_latency.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_budget(agg):
    cells = [
        ("ring_dp", 1000000, 1000000, 1),
        ("ring_raw", 1000000, 1000000, 1),
        ("ring_dp_bs", 1000000, 1000000, 1),
        ("ring_replot", 100000, 1000000, 1),
        ("setdata", 100000, 1000000, 1),
        ("window", 100000, 1000000, 1),
        ("setdata", 1000000, 1000000, 0),
        ("window", 1000000, 1000000, 0),
    ]
    fig, ax = plt.subplots(figsize=(10.0, 3.8))
    labels, gens, commits, draws, flushes = [], [], [], [], []
    for strat, rate, win, lttb in cells:
        a = agg[(strat, rate, win, lttb)]
        labels.append("%s\nR=%s W=%s %s" % (
            strat, f"{rate // 1000}k" if rate >= 1000 else rate,
            f"{win // 1000}k" if win >= 1000 else win,
            "LTTB" if lttb else "no-LTTB"))
        gens.append(a["gen"])
        commits.append(a["commit"])
        draws.append(a["draw"])
        flushes.append(a["present"])
    x = np.arange(len(cells))
    bottom = np.zeros(len(cells))
    for vals, name, color in [
            (gens, "gen (synthesis + ring append)", "#8c9bab"),
            (commits, "commit (rebind / copy / axis)", "#f0a05a"),
            (draws, "draw call (drawSeries / replot)", "#5b8bd0"),
            (flushes, "flush (processEvents paint)", "#d65f5f")]:
        ax.bar(x, vals, 0.62, bottom=bottom, label=name, color=color,
               edgecolor="white", lw=0.4)
        bottom = bottom + np.array(vals)
    ax.set_yscale("log")
    ax.set_xticks(x)
    ax.set_xticklabels(labels, fontsize=7.2)
    ax.axhline(BUDGET_MS, color="#333333", ls=":", lw=1.2)
    ax.text(len(cells) - 0.4, BUDGET_MS * 1.12, "60 FPS budget", fontsize=8,
            ha="right", color="#333333")
    ax.set_ylabel("median per-tick time (ms)")
    ax.set_title("per-tick phase breakdown (median frame times)", fontsize=10)
    ax.legend(fontsize=7.8, frameon=False, loc="upper left")
    set_style(ax)
    fig.tight_layout()
    fig.savefig(HERE / "fig_stream_budget.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_model(agg, models_full, model_direct):
    fig, axes = plt.subplots(1, 2, figsize=(11.0, 3.9))

    # left: full-replot strategies, T vs effective window (pooled over rates)
    ax = axes[0]
    Wg = np.logspace(np.log10(1000), np.log10(3e6), 80)
    for strat in ["ring_replot", "setdata", "window"]:
        for lttb, ls in ((1, "-"), (0, "--")):
            m = models_full[(strat, lttb)]
            coef = m["coef"]
            if lttb == 0:
                Tm = coef[0] + coef[1] * Wg / 1e6
            else:
                Tm = coef[0] + coef[1] * Wg / 1e6 + coef[2] * np.minimum(Wg, LTTB_OUT) / 1e3
            ax.plot(Wg, np.maximum(Tm, 1e-3), ls, lw=1.3, color=COL[strat])
            for (rate, win, we), ym, yp in zip(m["cells"], m["y"], m["pred"]):
                pass  # points plotted below from agg to keep marker semantics
            for rate in RATES:
                for win in WINDOWS:
                    we = w_eff(rate, win)
                    ax.plot([we], [get(agg, strat, rate, win, lttb)],
                            MARK[strat], ms=5, color=COL[strat],
                            mfc="white" if lttb == 0 else COL[strat], mew=1.2,
                            label=("%s LTTB=%d" % (strat, lttb)) if (rate, win) == (RATES[0], WINDOWS[0]) else None)
    ax.axhline(BUDGET_MS, color="#555555", ls=":", lw=1.2)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("effective window $W_{eff}$ (median visible samples)")
    ax.set_ylabel("median frame time (ms)")
    ax.set_title("full-replot strategies: pooled fit (lines) vs measured (markers)", fontsize=9.5)
    ax.legend(fontsize=7, frameon=False, ncol=2, loc="upper left")
    set_style(ax)

    # right: direct strategy, T vs samples-per-tick
    ax = axes[1]
    for lttb, ls in ((1, "-"), (0, "--")):
        coef, x, y, r2 = model_direct[lttb]
        xg = np.linspace(0, max(x.max() * 1.15, 12), 50)
        ax.plot(xg, coef[0] + coef[1] * xg, ls, lw=1.3, color=COL["ring_dp"],
                label="fit LTTB=%d: %.3f + %.4f ms/k ($R^2$=%.3f)" % (lttb, coef[0], coef[1], r2))
        ax.plot(x, y, "o", ms=5, color=COL["ring_dp"], mfc="white" if lttb == 0 else COL["ring_dp"],
                mew=1.3)
    ax.axhline(BUDGET_MS, color="#555555", ls=":", lw=1.2)
    ax.axhline(TICK_MS, color="#999999", ls="-.", lw=1.0)
    ax.text(0.15, TICK_MS * 1.1, "tick interval (10 ms): timer-paced ceiling", fontsize=7.5,
            color="#777777")
    ax.set_yscale("log")
    ax.set_xlabel("samples produced per tick (thousands), R x 10 ms")
    ax.set_ylabel("median frame time (ms)")
    ax.set_title("ring_dp incremental cost vs per-tick batch (page time >= 0.3 s cells)", fontsize=9.5)
    ax.legend(fontsize=7.5, frameon=False, loc="upper left")
    set_style(ax)

    fig.tight_layout()
    fig.savefig(HERE / "fig_stream_model.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_boundary(agg, models_full, model_direct):
    """60 FPS sustainability map: measured fps_eff over the (rate, window) grid."""
    strat_show = ["ring_dp", "ring_raw", "ring_dp_bs", "ring_replot", "setdata", "window"]
    fig, axes = plt.subplots(2, 6, figsize=(15.5, 4.6), sharey=True)
    for si, strat in enumerate(strat_show):
        for li, lttb in enumerate((1, 0)):
            ax = axes[li][si]
            M = np.zeros((len(WINDOWS), len(RATES)))
            for wi, win in enumerate(WINDOWS):
                for ri, rate in enumerate(RATES):
                    M[wi, ri] = get(agg, strat, rate, win, lttb, "fps")
            im = ax.imshow(np.minimum(M, 105.0), cmap="RdYlGn", vmin=0, vmax=105,
                           aspect="auto")
            for wi in range(len(WINDOWS)):
                for ri in range(len(RATES)):
                    v = M[wi, ri]
                    ax.text(ri, wi, "%.0f" % v, ha="center", va="center", fontsize=7.2,
                            color="black" if v > 30 else "white")
            ax.set_xticks(range(len(RATES)))
            ax.set_xticklabels(["1k", "10k", "100k", "1M"], fontsize=7.5)
            ax.set_yticks(range(len(WINDOWS)))
            ax.set_yticklabels(["10k", "100k", "1M"], fontsize=7.5)
            ax.set_title("%s%s" % (strat, "  (LTTB on)" if lttb == 1 else "  (LTTB off)"),
                         fontsize=8.5)
            if si == 0:
                ax.set_ylabel("window\n(samples)", fontsize=8)
    axes[1][0].set_xlabel("rate (samples/s)", fontsize=8.5)
    fig.suptitle("effective FPS over the (rate, window) matrix - green >= 60 FPS, red = far below "
                 "(cell text: measured fps_eff, capped at 105)", fontsize=10)
    fig.colorbar(im, ax=axes.ravel().tolist(), shrink=0.8, label="fps_eff", pad=0.01)
    fig.subplots_adjust(left=0.05, right=0.92, top=0.86, bottom=0.12, wspace=0.25, hspace=0.35)
    fig.savefig(HERE / "fig_stream_boundary.png", dpi=200, bbox_inches="tight")
    plt.close(fig)

# ------------------------------------------------------------------ driver


def print_key_numbers(agg):
    print("=" * 110)
    print("KEY NUMBERS FOR THE ARTICLE")
    print("=" * 110)
    for rate in RATES:
        for win in WINDOWS:
            dp = get(agg, "ring_dp", rate, win, 1)
            rp = get(agg, "ring_replot", rate, win, 1)
            sd = get(agg, "setdata", rate, win, 1)
            wd = get(agg, "window", rate, win, 1)
            bs = get(agg, "ring_dp_bs", rate, win, 1)
            raw = get(agg, "ring_raw", rate, win, 1)
            print("R=%-8d W=%-8d LTTB  ring_dp=%8.3f  ring_raw=%8.3f  ring_dp_bs=%8.3f  "
                  "ring_replot=%8.3f  setdata=%8.3f  window=%8.3f  (ms/frame)" % (
                      rate, win, dp, raw, bs, rp, sd, wd))
    print()
    for rate in RATES:
        for win in WINDOWS:
            dp = get(agg, "ring_dp", rate, win, 0)
            rp = get(agg, "ring_replot", rate, win, 0)
            sd = get(agg, "setdata", rate, win, 0)
            wd = get(agg, "window", rate, win, 0)
            print("R=%-8d W=%-8d noLTTB ring_dp=%8.3f  ring_replot=%8.3f  setdata=%8.3f  "
                  "window=%8.3f  (ms/frame)" % (rate, win, dp, rp, sd, wd))
    print()
    print("DirectPainter net gain (ring_replot / ring_dp, LTTB on):")
    for rate in RATES:
        row = []
        for win in WINDOWS:
            g = get(agg, "ring_replot", rate, win, 1) / max(get(agg, "ring_dp", rate, win, 1), 1e-9)
            row.append("W=%-7d %6.1fx" % (win, g))
        print("  R=%-8d %s" % (rate, "  ".join(row)))
    print()
    print("BackingStore double-paint penalty (ring_dp_bs / ring_dp, LTTB on):")
    for rate in RATES:
        row = []
        for win in WINDOWS:
            g = get(agg, "ring_dp_bs", rate, win, 1) / max(get(agg, "ring_dp", rate, win, 1), 1e-9)
            row.append("W=%-7d %6.2fx" % (win, g))
        print("  R=%-8d %s" % (rate, "  ".join(row)))
    print()
    print("SIMD mapper path effect (ring_dp / ring_raw frame time, LTTB on; <1 = SIMD slower):")
    for rate in RATES:
        row = []
        for win in WINDOWS:
            g = get(agg, "ring_dp", rate, win, 1) / max(get(agg, "ring_raw", rate, win, 1), 1e-9)
            row.append("W=%-7d %6.2fx" % (win, g))
        print("  R=%-8d %s" % (rate, "  ".join(row)))
    print()
    print("setdata commit cost (commit_med_ms at R=1M/s, window filled within the run,"
          " = per-tick QVector<QPointF> window copy):")
    for win in WINDOWS:
        print("  R=1000000  W=%-8d %8.3f ms" % (win, get(agg, "setdata", 1000000, win, 1, "commit")))
    print()
    print("60 FPS sustainable cells (median frame time <= %.2f ms, LTTB on):" % BUDGET_MS)
    for strat in STRategies:
        cells = []
        for rate in RATES:
            for win in WINDOWS:
                if get(agg, strat, rate, win, 1) <= BUDGET_MS:
                    cells.append("(R=%s,W=%s)" % (f"{rate // 1000}k" if rate >= 1000 else rate,
                                                  f"{win // 1000}k"))
        print("  %-12s %d/%d cells: %s" % (strat, len(cells), len(RATES) * len(WINDOWS),
                                            " ".join(cells)))
    print()
    print("latency table (LTTB on; med/p95/p99 ms):")
    for strat in STRategies:
        for rate, win in [(100000, 100000), (1000000, 100000), (1000000, 1000000)]:
            a = agg[(strat, rate, win, 1)]
            print("  %-12s R=%-8d W=%-8d  %8.2f %8.2f %8.2f  (T_frame=%8.3f fps=%6.1f)" % (
                strat, rate, win, a["lat_med"], a["lat_p95"], a["lat_p99"],
                a["f_med"], a["fps"]))
    print()
    print("paint_events validation (canvas paintEvent count during 3 s, expect ~frames for "
          "full replot, ~boundaries for direct):")
    for strat in STRategies:
        a = agg[(strat, 100000, 100000, 1)]
        print("  %-12s frames=%3.0f paint_ev=%5.1f boundary=%4.1f skipped=%4.1f" % (
            strat, a["frames"], a["paint_ev"], a["bnd"], a["skipped"]))
    print()


def main():
    agg = load()
    n_cfg = len(agg)
    n_rows = sum(a["nreps"] for a in agg.values())
    print("loaded %d rep rows, %d configs" % (n_rows, n_cfg))
    njit = sum(1 for a in agg.values() if a["jitter"])
    print("configs with jitter retry: %d" % njit)
    print()
    print_table(agg)
    models_full = fit_full_replot_models(agg)
    model_direct = fit_direct_model(agg)

    print("=" * 110)
    print("FITTED FRAME-BUDGET MODELS (regressed on effective window / per-tick batch)")
    print("=" * 110)
    for strat in ["ring_replot", "setdata", "window"]:
        for lttb in (1, 0):
            m = models_full[(strat, lttb)]
            coef = m["coef"]
            if lttb == 0:
                print("full  %-12s lttb=0  T = %.3f + %.3f * W_eff[M]   R2=%.3f  max_resid=%.3f ms" % (
                    strat, coef[0], coef[1], m["r2"],
                    float(np.max(np.abs(m["y"] - m["pred"])))))
            else:
                print("full  %-12s lttb=1  T = %.3f + %.3f * W_eff[M] + %.4f * min(W_eff,3600)[k]  "
                      "R2=%.3f  max_resid=%.3f ms" % (
                          strat, coef[0], coef[1], coef[2], m["r2"],
                          float(np.max(np.abs(m["y"] - m["pred"])))))
            for (rate, win, we), ym, yp in zip(m["cells"], m["y"], m["pred"]):
                print("        R=%-8d W=%-8d W_eff=%9.0f  meas=%8.3f  pred=%8.3f" % (
                    rate, win, we, ym, yp))
    for lttb in (1, 0):
        coef, x, y, r2 = model_direct[lttb]
        pred = coef[0] + coef[1] * x
        print("direct ring_dp      lttb=%d  T = %.3f + %.4f * n_tick[k]  R2=%.3f" % (
            lttb, coef[0], coef[1], r2))
        for xi, yi, pi in zip(x, y, pred):
            print("        n_tick=%8.2fk  meas=%8.3f  pred=%8.3f" % (xi, yi, pi))
    print()
    boundary_60fps(agg, models_full, model_direct)
    latency_model_check(agg)
    print_key_numbers(agg)

    fig_matrix(agg)
    fig_window(agg)
    fig_direct(agg)
    fig_latency(agg)
    fig_budget(agg)
    fig_model(agg, models_full, model_direct)
    fig_boundary(agg, models_full, model_direct)
    print("figures written to", HERE)


if __name__ == "__main__":
    main()
