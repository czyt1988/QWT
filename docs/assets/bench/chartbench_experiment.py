"""Figures and analysis values of the "Qt plotting libraries" tech report.

Every figure of docs/tech-report/plot-library-benchmark.md is produced here from
the CSV tables that examples/bench/chartbench/run-bench.sh writes into data/.
Run the benchmark first, then:

    python chartbench_fidelity.py      # pixel metrics (writes data/fidelity_metrics.csv)
    python chartbench_experiment.py    # all data figures + the numbers quoted in the text

The script prints every value that the report quotes, so the text and the
figures cannot drift apart.
"""

from __future__ import annotations

import os

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

from chartbench_common import (
    DATA_DIR, HERE, LIBRARY_COLORS, LIBRARY_LABELS, LIBRARY_MARKERS, LIBRARY_ORDER,
    PRIMARY_BACKEND, load_configs, load_frames, on_primary_backend, parse_canvas,
    parse_curves, parse_points, parse_window, save, summarise, use_report_style,
)

#: Compact names for tick labels, where the full label does not fit.
SHORT_LABELS = ["Qwt", "QCustomPlot", "QtCharts", "QIm"]

STRATEGY_LABELS = {
    "none": "none",
    "adaptive": "adaptive sampling",
    "filterpoints": "FilterPoints",
    "aggressive": "Aggressive",
    "pixel": "Pixel",
    "lttb": "LTTB",
    "minmaxlttb": "MinMaxLTTB",
    "auto": "Auto",
}


def primary_only(frames: pd.DataFrame) -> pd.DataFrame:
    keep = frames.apply(
        lambda row: PRIMARY_BACKEND.get(row["library"]) == row["backend"], axis=1)
    return frames[keep]


def label(library: str) -> str:
    return LIBRARY_LABELS.get(library, library)


def ordered_libraries(frames: pd.DataFrame):
    present = set(frames["library"].unique())
    return [lib for lib in LIBRARY_ORDER if lib in present]


def fit_exponent(x: np.ndarray, y: np.ndarray) -> tuple[float, float]:
    """Least squares fit of log(y) = a + p*log(x); returns (p, r2)."""
    mask = (x > 0) & (y > 0)
    log_x, log_y = np.log10(x[mask]), np.log10(y[mask])
    p, a = np.polyfit(log_x, log_y, 1)
    predicted = a + p * log_x
    residual = log_y - predicted
    ss_res = float((residual ** 2).sum())
    ss_tot = float(((log_y - log_y.mean()) ** 2).sum())
    return float(p), 1.0 - ss_res / ss_tot if ss_tot else float("nan")


def per_million(table: dict, low: int, high: int) -> float:
    """Marginal cost in ms per million samples between two measured sizes.

    Preferred over a fitted intercept: the small-N points of some libraries lie
    below any straight line through the large-N points (Qt Charts and QIm would
    get a negative "fixed cost"), so the marginal cost is reported for the two
    ranges where the sweep is linear instead.
    """
    return (table[high] - table[low]) / ((high - low) / 1e6)


# ---------------------------------------------------------------------------
# 1. point count scaling
# ---------------------------------------------------------------------------

def figure_points(frames: pd.DataFrame) -> None:
    data = primary_only(frames[frames["suite"] == "points"])
    stats = summarise(data, ["library", "backend", "variant"])
    stats["n"] = stats["variant"].map(parse_points)
    stats = stats.sort_values("n")

    fig, axes = plt.subplots(1, 2, figsize=(9.6, 3.7))

    for library in ordered_libraries(stats):
        subset = stats[stats["library"] == library]
        axes[0].plot(subset["n"], subset["median"], marker=LIBRARY_MARKERS[library],
                     color=LIBRARY_COLORS[library], label=label(library))
        axes[1].plot(subset["n"], subset["n"] / (subset["median"] / 1000.0),
                     marker=LIBRARY_MARKERS[library], color=LIBRARY_COLORS[library],
                     label=label(library))

    axes[0].set(xscale="log", yscale="log",
                xlabel="samples per curve", ylabel="frame time [ms]",
                title="(a) single curve, 1000$\\times$700 canvas")
    axes[1].set(xscale="log", yscale="log",
                xlabel="samples per curve", ylabel="throughput [points/s]",
                title="(b) effective drawing throughput")
    for axis in axes:
        axis.legend(loc="upper left")

    print("\n[points] median frame time [ms], then the cost model of the sweep")
    for library in ordered_libraries(stats):
        subset = stats[stats["library"] == library].sort_values("n")
        large = subset[subset["n"] >= 10000]
        exponent, exponent_r2 = fit_exponent(large["n"].to_numpy(), large["median"].to_numpy())
        table = {int(row.n): row.median for row in subset.itertuples()}
        small = table[1000]
        mid = per_million(table, 100000, 1000000)
        high = per_million(table, 1000000, 5000000)
        print(f"  {label(library):22s} " + "  ".join(
            f"{int(row.n)}:{row.median:8.3f}" for row in subset.itertuples()))
        print(f"    {'':22s} T(1k)={small:5.3f} ms   per Msample: "
              f"{mid:7.2f} ms (100k-1M)  {high:7.2f} ms (1M-5M)   "
              f"loglog exponent={exponent:.3f} (r2={exponent_r2:.4f})")

    fig.tight_layout()
    save(fig, "fig_scaling_points.png")
    plt.close(fig)


def figure_per_point_cost(frames: pd.DataFrame) -> None:
    data = primary_only(frames[frames["suite"] == "points"])
    stats = summarise(data, ["library", "backend", "variant"])
    stats["n"] = stats["variant"].map(parse_points)
    stats["per_1k"] = stats["median"] / stats["n"] * 1000.0

    fig, axis = plt.subplots(figsize=(5.6, 3.7))
    for library in ordered_libraries(stats):
        subset = stats[stats["library"] == library].sort_values("n")
        axis.plot(subset["n"], subset["per_1k"], marker=LIBRARY_MARKERS[library],
                  color=LIBRARY_COLORS[library], label=label(library))
    axis.set(xscale="log", yscale="log", xlabel="samples per curve",
             ylabel="cost per 1000 samples [ms]",
             title="Marginal cost of a sample falls with $N$: fixed frame cost dominates below 10k")
    axis.legend(loc="lower left")
    fig.tight_layout()
    save(fig, "fig_scaling_per_point.png")
    plt.close(fig)


# ---------------------------------------------------------------------------
# 2. series count
# ---------------------------------------------------------------------------

def figure_curves(frames: pd.DataFrame) -> None:
    data = primary_only(frames[frames["suite"] == "curves"])
    stats = summarise(data, ["library", "backend", "variant"])
    stats["curves"] = stats["variant"].map(parse_curves)
    stats = stats.sort_values("curves")

    fig, axes = plt.subplots(1, 2, figsize=(9.6, 3.7))
    for library in ordered_libraries(stats):
        subset = stats[stats["library"] == library]
        axes[0].plot(subset["curves"], subset["median"], marker=LIBRARY_MARKERS[library],
                     color=LIBRARY_COLORS[library], label=label(library))
        single = subset[subset["curves"] == 1]["median"]
        if not single.empty:
            marginal = (subset["median"] - single.iloc[0]) / (subset["curves"] - 1).replace(0, np.nan)
            axes[1].plot(subset["curves"], marginal * 1000.0, marker=LIBRARY_MARKERS[library],
                         color=LIBRARY_COLORS[library], label=label(library))

    axes[0].set(xscale="log", yscale="log", xlabel="series", ylabel="frame time [ms]",
                title="(a) 2000 samples per series, 1000$\\times$700")
    axes[1].set(xscale="log", yscale="log", xlabel="series",
                ylabel="marginal cost per series [$\\mu$s]",
                title="(b) marginal series cost")
    for axis in axes:
        axis.legend(loc="upper left")

    print("\n[curves] median frame time [ms]")
    for library in ordered_libraries(stats):
        subset = stats[stats["library"] == library]
        print(f"  {label(library):22s} " + "  ".join(
            f"{int(row.curves)}:{row.median:7.3f}" for row in subset.itertuples()))
        few = float(subset[subset["curves"] == 10]["median"].iloc[0])
        many = float(subset[subset["curves"] == 100]["median"].iloc[0])
        single = float(subset[subset["curves"] == 1]["median"].iloc[0])
        print(f"    {'':22s} marginal per series 10->100: {(many - few) / 90 * 1000:6.1f} us   "
              f"single series: {single:.3f} ms   (20k samples in 100 series: {many:.3f} ms)")

    fig.tight_layout()
    save(fig, "fig_curve_count.png")
    plt.close(fig)


# ---------------------------------------------------------------------------
# 3. canvas size
# ---------------------------------------------------------------------------

def figure_canvas(frames: pd.DataFrame) -> None:
    data = primary_only(frames[frames["suite"] == "canvas"])
    stats = summarise(data, ["library", "backend", "variant"])
    stats["pixels"] = stats["variant"].map(parse_canvas)
    stats["kind"] = np.where(stats["variant"].str.startswith("single"), "single", "many")

    fig, axes = plt.subplots(1, 2, figsize=(9.6, 3.7))
    for axis, kind, title in zip(axes, ["single", "many"],
                                 ["(a) 1 curve, 100k samples", "(b) 20 curves, 5k samples each"]):
        subset_kind = stats[stats["kind"] == kind]
        for library in ordered_libraries(subset_kind):
            subset = subset_kind[subset_kind["library"] == library].sort_values("pixels")
            axis.plot(subset["pixels"] / 1e6, subset["median"], marker=LIBRARY_MARKERS[library],
                      color=LIBRARY_COLORS[library], label=label(library))
        axis.set(xlabel="canvas area [MPixel]", ylabel="frame time [ms]", title=title)
        axis.legend(loc="upper left")

    print("\n[canvas] median frame time [ms]")
    for kind in ["single", "many"]:
        print(f"  {kind}:")
        subset_kind = stats[stats["kind"] == kind]
        for library in ordered_libraries(subset_kind):
            subset = subset_kind[subset_kind["library"] == library].sort_values("pixels")
            print(f"    {label(library):22s} " + "  ".join(
                f"{row.pixels/1e6:.2f}MP:{row.median:7.3f}" for row in subset.itertuples()))

    fig.tight_layout()
    save(fig, "fig_canvas_size.png")
    plt.close(fig)


# ---------------------------------------------------------------------------
# 4. data update cost
# ---------------------------------------------------------------------------

def figure_update(frames: pd.DataFrame) -> None:
    data = primary_only(frames[frames["suite"] == "update"])
    stats = summarise(data, ["library", "backend", "variant"])
    stats["window"] = stats["variant"].map(parse_window)
    stats["kind"] = stats["variant"].str.split("_").str[0]

    windows = sorted(stats["window"].dropna().unique())
    libraries = ordered_libraries(stats)
    width = 0.8 / len(libraries)

    fig, axes = plt.subplots(1, len(windows), figsize=(9.6, 3.6), sharey=True)
    for axis, window in zip(np.atleast_1d(axes), windows):
        for index, library in enumerate(libraries):
            subset = stats[(stats["library"] == library) & (stats["window"] == window)]
            offset = (index - (len(libraries) - 1) / 2) * width

            def value(kind, column="median"):
                row = subset[subset["kind"] == kind]
                return float(row[column].iloc[0]) if not row.empty else 0.0

            axis.bar(index, value("render-only"), width=width, color=LIBRARY_COLORS[library],
                     label=label(library) if window == windows[0] else None)
            axis.bar(index, value("stream"), width=width, bottom=value("render-only"),
                     color=LIBRARY_COLORS[library], alpha=0.45, hatch="//")
            # p95 of the whole frame, drawn as a short black bar on top of the stack
            p95 = value("stream", "p95") + value("stream-handover", "p95")
            axis.plot([index - width / 2, index + width / 2], [p95, p95],
                      color="black", linewidth=1.2, zorder=5)

        axis.set_xticks(range(len(libraries)))
        axis.set_xticklabels(SHORT_LABELS, rotation=25, ha="right")
        axis.set_title(f"window = {int(window):,} samples".replace(",", " "))
        axis.set_yscale("log")
    np.atleast_1d(axes)[0].set_ylabel("frame time [ms]")

    handles = [plt.Rectangle((0, 0), 1, 1, color=LIBRARY_COLORS[lib]) for lib in libraries]
    handles += [
        plt.Rectangle((0, 0), 1, 1, color="0.4"),
        plt.Rectangle((0, 0), 1, 1, color="0.4", alpha=0.45, hatch="//"),
    ]
    fig.legend(handles, [label(lib) for lib in libraries] + ["repaint only", "+ data handover"],
               loc="lower center", ncol=6, bbox_to_anchor=(0.5, -0.06))
    fig.suptitle("Sliding window update: repaint cost and the extra cost of handing over new samples",
                 fontsize=10)

    print("\n[update] median ms (render-only / update-only / stream=update+render)")
    print("         handover is measured from the frames table (update_ms), the")
    print("         update-only configuration performs no repaint at all")
    raw = primary_only(frames[frames["suite"] == "update"])
    raw = raw[raw["frame_idx"] > 0]
    for library in libraries:
        for window in windows:
            subset = stats[(stats["library"] == library) & (stats["window"] == window)]
            values = {row.kind: row.median for row in subset.itertuples()}
            handover = raw[(raw["library"] == library) &
                           (raw["variant"] == f"update-only_w{int(window)}")]["update_ms"].median()
            stream_handover = raw[(raw["library"] == library) &
                                  (raw["variant"] == f"stream_w{int(window)}")]["update_ms"].median()
            stream_total = values.get("stream", float("nan")) + stream_handover
            budget_share = 100.0 * stream_total / 16.7
            print(f"  {label(library):22s} w={int(window):7d}  "
                  f"render={values.get('render-only', float('nan')):8.3f}  "
                  f"handover={handover:8.3f}  "
                  f"stream_render={values.get('stream', float('nan')):8.3f}  "
                  f"stream_handover={stream_handover:7.3f}  "
                  f"stream={stream_total:8.3f} "
                  f"({budget_share:6.1f}% of a 60 Hz frame)")

    fig.tight_layout(rect=(0, 0.03, 1, 0.96))
    save(fig, "fig_update_decomposition.png")
    plt.close(fig)


def figure_streaming(frames: pd.DataFrame) -> None:
    data = primary_only(frames[frames["suite"] == "update"])
    stats = summarise(data, ["library", "backend", "variant"])
    stats["window"] = stats["variant"].map(parse_window)
    stream = stats[stats["variant"].str.startswith("stream")]

    fig, axis = plt.subplots(figsize=(5.6, 3.7))
    for library in ordered_libraries(stream):
        subset = stream[stream["library"] == library].sort_values("window")
        axis.plot(subset["window"], subset["median"], marker=LIBRARY_MARKERS[library],
                  color=LIBRARY_COLORS[library], label=label(library))
        axis.fill_between(subset["window"], subset["median"], subset["p95"],
                          color=LIBRARY_COLORS[library], alpha=0.15)
    axis.axhline(16.7, color="0.3", linestyle="--", linewidth=1)
    axis.text(1100, 18, "60 Hz budget", color="0.3", fontsize=8)
    axis.set(xscale="log", yscale="log", xlabel="visible window [samples]",
             ylabel="frame time [ms]", title="Streaming update: median (line) and p95 (shade)")
    axis.legend(loc="upper left")
    fig.tight_layout()
    save(fig, "fig_streaming.png")
    plt.close(fig)


# ---------------------------------------------------------------------------
# 5. point reduction
# ---------------------------------------------------------------------------

def figure_downsample(frames: pd.DataFrame) -> None:
    data = primary_only(frames[frames["suite"] == "downsample"])
    stats = summarise(data, ["library", "backend", "variant"])

    libraries = ordered_libraries(stats)
    strategies = [s for s in ["none", "filterpoints", "aggressive", "pixel", "lttb",
                              "minmaxlttb", "adaptive", "auto"]
                  if s in set(stats["variant"])]

    # A dot plot, not bars: on a logarithmic axis a bar's height does not show
    # the ratio it stands for, and the ratios here span two orders of magnitude.
    fig, axis = plt.subplots(figsize=(7.8, 3.9))
    offset = 0.13
    for index, library in enumerate(libraries):
        subset = stats[stats["library"] == library]
        xs, ys = [], []
        for position, strategy in enumerate(strategies):
            row = subset[subset["variant"] == strategy]
            if row.empty:
                continue
            xs.append(position + (index - (len(libraries) - 1) / 2) * offset)
            ys.append(float(row["median"].iloc[0]))
        axis.plot(xs, ys, linestyle="none", marker=LIBRARY_MARKERS[library], markersize=7,
                  color=LIBRARY_COLORS[library], label=label(library))

    axis.axhline(16.7, color="crimson", linestyle="--", linewidth=1)
    axis.text(len(strategies) - 0.6, 17.5, "60 Hz budget", color="crimson", fontsize=8,
              ha="right")
    for position, strategy in enumerate(strategies):
        axis.axvline(position, color="0.9", linewidth=0.8, zorder=0)
    axis.set_yscale("log")
    axis.set_xticks(range(len(strategies)))
    axis.set_xticklabels([STRATEGY_LABELS.get(s, s) for s in strategies], rotation=20, ha="right")
    axis.set_xlim(-0.6, len(strategies) - 0.4)
    axis.set_ylabel("frame time [ms]")
    axis.set_title("1 000 000 samples, 1000×700: the built-in point reduction of each library")
    axis.legend(loc="lower left", ncol=2)

    print("\n[downsample] median frame time [ms] at 1M samples")
    for library in libraries:
        subset = stats[stats["library"] == library]
        print(f"  {label(library):22s} " + "  ".join(
            f"{row.variant}:{row.median:8.3f}" for row in subset.itertuples()))

    fig.tight_layout()
    save(fig, "fig_downsample_time.png")
    plt.close(fig)


def figure_pareto(frames: pd.DataFrame) -> None:
    path = os.path.join(DATA_DIR, "fidelity_metrics.csv")
    if not os.path.exists(path):
        print("  (skip pareto: run chartbench_fidelity.py first)")
        return

    fidelity = on_primary_backend(pd.read_csv(path))
    fidelity = fidelity[fidelity["case"] == "spiky_1m"]

    data = primary_only(frames[frames["suite"] == "downsample"])
    stats = summarise(data, ["library", "backend", "variant"])
    baselines = {row.library: row.median for row in stats[stats["variant"] == "none"].itertuples()}

    # The libraries have very different absolute speeds, so the x axis is the
    # speedup each strategy achieves against its own library's unreduced path.
    fig, axis = plt.subplots(figsize=(6.6, 4.2))
    values = {}
    for library in ordered_libraries(stats):
        subset = stats[(stats["library"] == library) & (stats["variant"] != "none")]
        for row in subset.itertuples():
            metrics = fidelity[(fidelity["library"] == library) &
                               (fidelity["strategy"] == row.variant)]
            if metrics.empty:
                continue
            speedup = baselines[library] / row.median
            deviation = float(metrics["changed_frac"].iloc[0]) * 100.0
            values[(library, row.variant)] = (speedup, deviation)
            axis.scatter(speedup, deviation, color=LIBRARY_COLORS[library],
                         marker=LIBRARY_MARKERS[library], s=48)

    # Label every point, flipping the label below the marker when the previous
    # label would land in the same place (log x, so compare decades).
    placed = []
    for (library, variant), (speedup, deviation) in sorted(values.items(), key=lambda kv: kv[1][0]):
        above = True
        for x, y, was_above in placed:
            if abs(np.log10(speedup) - np.log10(x)) < 0.45 and abs(deviation - y) < 1.0:
                above = not was_above
                break
        placed.append((speedup, deviation, above))
        axis.annotate(f"{STRATEGY_LABELS.get(variant, variant)} ({label(library).split()[0]})",
                      (speedup, deviation), textcoords="offset points",
                      xytext=(7, 5) if above else (7, -12), fontsize=7.5,
                      color=LIBRARY_COLORS[library], ha="left")

    axis.axvline(1.0, color="0.4", linestyle="--", linewidth=1)
    axis.annotate("no reduction", (1.02, 8.0), textcoords="offset points",
                  xytext=(5, 0), fontsize=8, color="0.35", va="center")
    handles = [plt.Line2D([], [], linestyle="none", marker=LIBRARY_MARKERS[lib],
                          color=LIBRARY_COLORS[lib], label=label(lib))
               for lib in ordered_libraries(stats)]
    axis.legend(handles=handles, loc="upper center", bbox_to_anchor=(0.5, -0.17), ncol=4)
    axis.set(xscale="log", xlabel="speedup against the library's own unreduced frame",
             ylabel="pixels changed vs. full resolution [%]",
             title="Cost and fidelity of point reduction, 1M spiky samples\n"
                   "(lower right is better: fast and faithful)")
    fig.tight_layout()
    save(fig, "fig_fidelity_pareto.png")
    plt.close(fig)


def figure_fidelity_metrics() -> None:
    path = os.path.join(DATA_DIR, "fidelity_metrics.csv")
    if not os.path.exists(path):
        print("  (skip fidelity metrics: run chartbench_fidelity.py first)")
        return

    fidelity = on_primary_backend(pd.read_csv(path))
    fidelity = fidelity[fidelity["case"] == "spiky_1m"].copy()
    fidelity["label"] = (fidelity["library"].map(label) + "\n"
                         + fidelity["strategy"].map(lambda s: STRATEGY_LABELS.get(s, s)))

    order = fidelity.sort_values("changed_frac")["label"].tolist()
    positions = np.arange(len(order))
    colors = [LIBRARY_COLORS[lib] for lib in
              fidelity.sort_values("changed_frac")["library"].tolist()]

    fig, axes = plt.subplots(1, 3, figsize=(10.2, 3.9))
    for axis, column, title in zip(
            axes,
            ["changed_frac", "span_ratio", "ink_ratio"],
            ["(a) pixels changed vs. full render [%]",
             "(b) worst column error span [fraction of height]",
             "(c) retained curve ink [ratio]"]):
        values = fidelity.sort_values("changed_frac")[column].to_numpy()
        if column == "changed_frac":
            values = values * 100.0
        axis.barh(positions, values, color=colors)
        axis.set_yticks(positions)
        axis.set_yticklabels(order, fontsize=7)
        axis.set_title(title)
        if column == "ink_ratio":
            axis.axvline(1.0, color="0.3", linestyle="--", linewidth=1)

    print("\n[fidelity] 1M spiky samples, deviation from each library's own full render")
    for row in fidelity.sort_values(["library", "changed_frac"]).itertuples():
        print(f"  {label(row.library):22s} {row.strategy:12s} "
              f"changed={row.changed_frac*100:6.2f}%  span={row.span_ratio:.3f}  "
              f"ink={row.ink_ratio:.3f}  mae={row.mae:.2f}")

    fig.tight_layout()
    save(fig, "fig_fidelity_metrics.png")
    plt.close(fig)


# ---------------------------------------------------------------------------
# 6. frame time stability
# ---------------------------------------------------------------------------

def figure_jitter(frames: pd.DataFrame) -> None:
    data = primary_only(frames[frames["suite"] == "jitter"])

    fig, axes = plt.subplots(1, 2, figsize=(9.6, 3.7))
    for library in ordered_libraries(data):
        for axis, variant in zip(axes, ["static", "stream"]):
            subset = data[(data["library"] == library) &
                          (data["variant"].str.startswith(variant)) &
                          (data["frame_idx"] > 0)]["render_ms"].to_numpy()
            if subset.size == 0:
                continue
            ordered = np.sort(subset)
            axis.step(ordered, np.arange(1, ordered.size + 1) / ordered.size,
                      where="post", color=LIBRARY_COLORS[library],
                      label=label(library), linestyle="-" if variant == "static" else "--")
    for axis, title in zip(axes, ["(a) static data", "(b) sliding window update"]):
        axis.set(xscale="log", xlabel="frame time [ms]", ylabel="cumulative probability",
                 title=title)
        axis.legend(loc="lower right")
    fig.suptitle("Frame time distribution, 100k samples, 300 frames", fontsize=10)
    fig.tight_layout(rect=(0, 0, 1, 0.94))
    save(fig, "fig_jitter_ecdf.png")
    plt.close(fig)

    stats = summarise(data, ["library", "backend", "variant"])
    libraries = ordered_libraries(stats)
    variants = sorted(stats["variant"].unique())
    width = 0.8 / len(variants)

    fig, axis = plt.subplots(figsize=(7.2, 3.7))
    for index, variant in enumerate(variants):
        subset = stats[stats["variant"] == variant].set_index("library")
        values = [float(subset.loc[lib, "p99"]) if lib in subset.index else np.nan
                  for lib in libraries]
        offset = (index - (len(variants) - 1) / 2) * width
        axis.bar(np.arange(len(libraries)) + offset, values, width=width,
                 label=variant, color="0.75" if index == 0 else "0.45")
    axis.set_yscale("log")
    axis.set_xticks(range(len(libraries)))
    axis.set_xticklabels([label(lib) for lib in libraries], rotation=15, ha="right")
    axis.set_ylabel("p99 frame time [ms]")
    axis.axhline(16.7, color="crimson", linestyle="--", linewidth=1)
    axis.text(0.02, 17.5, "60 Hz budget", color="crimson", fontsize=8)
    axis.set_title("Tail latency: p99 of 300 frames")
    axis.legend(loc="upper left")

    print("\n[jitter] p50/p95/p99 of 300 frames [ms]")
    for library in libraries:
        for variant in variants:
            subset = stats[(stats["library"] == library) & (stats["variant"] == variant)]
            if subset.empty:
                continue
            row = subset.iloc[0]
            print(f"  {label(library):22s} {variant:16s} p50={row['median']:7.3f} "
                  f"p95={row['p95']:7.3f} p99={row['p99']:7.3f} max={row['maximum']:8.3f} "
                  f"std={row['std']:6.3f}")

    fig.tight_layout()
    save(fig, "fig_jitter_tail.png")
    plt.close(fig)


# ---------------------------------------------------------------------------
# 7. memory and setup
# ---------------------------------------------------------------------------

def figure_memory(configs: pd.DataFrame) -> None:
    data = primary_only(configs[configs["suite"] == "memory"]).copy()
    # only the single curve configurations: "c100n10000" is reported separately
    data = data[data["variant"].str.match(r"^n\d+$")]
    data["n"] = data["variant"].map(parse_points)
    data = data.dropna(subset=["n"])
    data["data_mb"] = data["private_mb"] - data["baseline_private_mb"]
    data["bytes_per_point"] = data["data_mb"] * 1024 * 1024 / data["n"]

    fig, axes = plt.subplots(1, 2, figsize=(9.6, 3.7))
    for library in ordered_libraries(data):
        subset = data[data["library"] == library].sort_values("n")
        axes[0].plot(subset["n"], subset["data_mb"], marker=LIBRARY_MARKERS[library],
                     color=LIBRARY_COLORS[library], label=label(library))
        axes[1].plot(subset["n"], subset["bytes_per_point"], marker=LIBRARY_MARKERS[library],
                     color=LIBRARY_COLORS[library], label=label(library))

    axes[0].set(xscale="log", yscale="log", xlabel="samples", ylabel="private bytes added [MB]",
                title="(a) memory the library adds for the data")
    axes[1].set(xscale="log", yscale="log", xlabel="samples", ylabel="bytes per sample",
                title="(b) storage cost of one sample")
    for axis in axes:
        axis.legend(loc="upper left")

    print("\n[memory] private bytes added by the library, one curve")
    for library in ordered_libraries(data):
        subset = data[data["library"] == library].sort_values("n")
        print(f"  {label(library):22s} " + "  ".join(
            f"{int(row.n)}:{row.bytes_per_point:6.1f}B" for row in subset.itertuples()))

    many = primary_only(configs[(configs["suite"] == "memory") &
                                (configs["variant"].str.startswith("c"))])
    if not many.empty:
        print("  100 series x 10000 samples:")
        for row in many.itertuples():
            added = row.private_mb - row.baseline_private_mb
            print(f"    {label(row.library):22s} {added:7.1f} MB "
                  f"({added*1024*1024/1e6:.1f} B/sample)")

    fig.tight_layout()
    save(fig, "fig_memory.png")
    plt.close(fig)


def figure_setup(configs: pd.DataFrame) -> None:
    data = primary_only(configs[configs["suite"] == "points"]).copy()
    data["n"] = data["variant"].map(parse_points)
    data = data.dropna(subset=["n"])

    data["first_frame_ms"] = (data["setup_ms"] - data["create_ms"] - data["handover_ms"]).clip(lower=0)

    libraries = ordered_libraries(data)
    sizes = sorted(data["n"].unique())
    width = 0.8 / len(libraries)

    fig, axes = plt.subplots(1, 2, figsize=(9.6, 3.8))
    axes[0].set_title("(a) total setup time")
    axes[1].set_title("(b) where the setup time goes")
    for library in libraries:
        subset = data[data["library"] == library].sort_values("n")
        axes[0].plot(subset["n"], subset["setup_ms"], marker=LIBRARY_MARKERS[library],
                     color=LIBRARY_COLORS[library], label=label(library))

    # The absolute totals differ by two orders of magnitude, so the decomposition
    # panel shows shares; the totals are annotated above the bars.
    for index, library in enumerate(libraries):
        subset = data[(data["library"] == library) & (data["n"] == max(sizes))]
        if subset.empty:
            continue
        row = subset.iloc[0]
        total = row["setup_ms"]
        bottom = 0.0
        for value, hatch, alpha in [(row["create_ms"], "", 1.0),
                                    (row["handover_ms"], "//", 0.8),
                                    (row["first_frame_ms"], "..", 0.55)]:
            share = 100.0 * value / total
            axes[1].bar(index, share, bottom=bottom, color=LIBRARY_COLORS[library],
                        hatch=hatch, alpha=alpha, width=0.62)
            bottom += share
        axes[1].annotate(f"{total:,.0f} ms".replace(",", " "), (index, 108),
                         ha="center", fontsize=8)

    axes[1].set_xticks(range(len(libraries)))
    axes[1].set_xticklabels(SHORT_LABELS, rotation=15, ha="right")
    axes[1].set_ylim(0, 125)
    axes[1].set_ylabel("share of the setup time [%]")
    handles = [plt.Rectangle((0, 0), 1, 1, color="0.35", fill=True),
               plt.Rectangle((0, 0), 1, 1, color="0.35", alpha=0.8, hatch="//"),
               plt.Rectangle((0, 0), 1, 1, color="0.35", alpha=0.55, hatch="..")]
    axes[1].legend(handles, ["create widget", "hand over data", "first frame"],
                   loc="upper center", bbox_to_anchor=(0.5, -0.16), ncol=3)
    axes[0].set(xscale="log", yscale="log", xlabel="samples per curve", ylabel="setup time [ms]")
    axes[0].legend(loc="upper left")
    fig.suptitle(f"Opening a plot: total (a) and the decomposition of the "
                 f"{int(max(sizes)):,} sample case (b)".replace(",", " "), fontsize=10)

    print("\n[setup] ms: create / hand over data / first frame, and the total")
    for library in libraries:
        subset = data[data["library"] == library].sort_values("n")
        print(f"  {label(library):22s} " + "  ".join(
            f"{int(row.n)}:{row.setup_ms:8.2f}" for row in subset.itertuples()))
        for size in sizes:
            row = subset[subset["n"] == size]
            if row.empty:
                continue
            item = row.iloc[0]
            print(f"      n={int(size):9d} create={item['create_ms']:8.2f} "
                  f"handover={item['handover_ms']:9.2f} first_frame={item['first_frame_ms']:8.2f}")

    fig.tight_layout(rect=(0, 0, 1, 0.93))
    save(fig, "fig_setup_time.png")
    plt.close(fig)


# ---------------------------------------------------------------------------
# 8. backends
# ---------------------------------------------------------------------------

def figure_backend(frames: pd.DataFrame) -> None:
    data = frames[(frames["suite"] == "points") &
                  (frames["library"].isin(["qwt", "qcustomplot"]))]
    stats = summarise(data, ["library", "backend", "variant"])
    stats["n"] = stats["variant"].map(parse_points)

    libraries = sorted(stats["library"].unique())
    sizes = sorted(stats["n"].dropna().unique())
    width = 0.35

    fig, axes = plt.subplots(1, len(libraries), figsize=(9.6, 3.7), sharey=True)
    for axis, library in zip(np.atleast_1d(axes), libraries):
        for index, backend in enumerate(["raster", "opengl"]):
            subset = stats[(stats["library"] == library) & (stats["backend"] == backend)]
            values = [float(subset[subset["n"] == n]["median"].iloc[0])
                      if not subset[subset["n"] == n].empty else np.nan for n in sizes]
            axis.bar(np.arange(len(sizes)) + (index - 0.5) * width, values, width=width,
                     label=backend, color="#0072B2" if backend == "raster" else "#CC79A7")
        axis.set_xticks(range(len(sizes)))
        axis.set_xticklabels([f"{int(n/1000)}k" for n in sizes], rotation=60, ha="right")
        axis.set_yscale("log")
        axis.set_title(label(library))
        axis.legend(loc="upper left")

    np.atleast_1d(axes)[0].set_ylabel("frame time [ms]")
    fig.suptitle("QPainter raster canvas vs. OpenGL canvas, one curve", fontsize=10)

    print("\n[backend] median frame time [ms] raster vs OpenGL")
    for library in libraries:
        for backend in ["raster", "opengl"]:
            subset = stats[(stats["library"] == library) & (stats["backend"] == backend)]
            if subset.empty:
                continue
            print(f"  {label(library):22s} {backend:7s} " + "  ".join(
                f"{int(row.n/1000)}k:{row.median:8.3f}" for row in
                subset.sort_values("n").itertuples()))

    fig.tight_layout(rect=(0, 0, 1, 0.93))
    save(fig, "fig_backend.png")
    plt.close(fig)


# ---------------------------------------------------------------------------
# 9. summary
# ---------------------------------------------------------------------------

def figure_summary(frames: pd.DataFrame) -> None:
    """Frame time at the three reference sizes, with and without reduction."""
    points = primary_only(frames[frames["suite"] == "points"])
    points_stats = summarise(points, ["library", "backend", "variant"])
    points_stats["n"] = points_stats["variant"].map(parse_points)

    reduction = primary_only(frames[frames["suite"] == "downsample"])
    reduction_stats = summarise(reduction, ["library", "backend", "variant"])

    libraries = ordered_libraries(points_stats)
    sizes = [100000, 1000000, 5000000]
    width = 0.8 / len(libraries)

    fig, axis = plt.subplots(figsize=(7.6, 3.9))
    for index, library in enumerate(libraries):
        subset = points_stats[points_stats["library"] == library]
        values = [float(subset[subset["n"] == n]["median"].iloc[0])
                  if not subset[subset["n"] == n].empty else np.nan for n in sizes]
        offset = (index - (len(libraries) - 1) / 2) * width
        bars = axis.bar(np.arange(len(sizes)) + offset, values, width=width,
                        color=LIBRARY_COLORS[library], label=label(library))
        for position, value in zip(bars, values):
            if not np.isnan(value):
                axis.annotate(f"{value:.1f}", (position.get_x() + position.get_width() / 2, value),
                              textcoords="offset points", xytext=(0, 2), ha="center", fontsize=6.5)

    axis.set_yscale("log")
    axis.set_xticks(range(len(sizes)))
    axis.set_xticklabels([f"{n/1e6:.1f}M".replace(".0M", "M") for n in sizes])
    axis.set_ylabel("frame time [ms]")
    axis.set_title("Single curve, no point reduction: every library is inside the 60 Hz budget at\n"
                   "100 000 samples and every one of them has left it at 1 M")
    axis.axhline(16.7, color="crimson", linestyle="--", linewidth=1)
    axis.text(-0.4, 18, "60 Hz budget", color="crimson", fontsize=8)
    axis.legend(loc="upper left")

    print("\n[summary] median frame time [ms]")
    for library in libraries:
        subset = points_stats[points_stats["library"] == library]
        print(f"  {label(library):22s} " + "  ".join(
            f"{int(row.n)}:{row.median:9.3f}" for row in subset.sort_values("n").itertuples()))
    print("  with the library's own point reduction at 1M samples:")
    for library in libraries:
        subset = reduction_stats[reduction_stats["library"] == library]
        print(f"    {label(library):22s} " + "  ".join(
            f"{row.variant}:{row.median:8.3f}" for row in subset.itertuples()))

    fig.tight_layout()
    save(fig, "fig_summary.png")
    plt.close(fig)


def figure_render_comparison() -> None:
    """Side by side rendering of identical data, straight from the harness."""
    from PIL import Image

    cases = [("clean_1k", "1000 samples"), ("spiky_100k", "100 000 samples")]
    libraries = [lib for lib in LIBRARY_ORDER
                 if os.path.exists(os.path.join(DATA_DIR, "images",
                                                f"{lib}_"
                                                f"{PRIMARY_BACKEND[lib]}_clean_1k_full.png"))]
    if not libraries:
        print("  (skip render comparison: no exported images)")
        return

    fig, axes = plt.subplots(len(cases), len(libraries),
                             figsize=(3.6 * len(libraries), 2.7 * len(cases)))
    axes = np.asarray(axes).reshape(len(cases), len(libraries))
    for row, (case, case_label) in enumerate(cases):
        for column, library in enumerate(libraries):
            path = os.path.join(DATA_DIR, "images",
                                f"{library}_{PRIMARY_BACKEND[library]}_{case}_full.png")
            axis = axes[row, column]
            if os.path.exists(path):
                axis.imshow(np.asarray(Image.open(path)))
            axis.set_xticks([])
            axis.set_yticks([])
            axis.grid(False)
            if row == 0:
                axis.set_title(label(library), fontsize=9)
            if column == 0:
                axis.set_ylabel(case_label, fontsize=9)
    fig.suptitle("Identical data, identical canvas size - what each library actually draws",
                 fontsize=10)
    fig.tight_layout(rect=(0, 0, 1, 0.95))
    save(fig, "fig_render_comparison.png")
    plt.close(fig)

    print("\n[renders] ink coverage of the exported full-resolution frames")
    for case, case_label in cases:
        for library in libraries:
            path = os.path.join(DATA_DIR, "images",
                                f"{library}_{PRIMARY_BACKEND[library]}_{case}_full.png")
            if not os.path.exists(path):
                continue
            pixels = np.asarray(Image.open(path).convert("RGB")).astype(int)
            ink = (pixels.sum(axis=2) < 400).mean()
            print(f"  {case_label:18s} {label(library):22s} ink={ink*100:6.3f}%")


def figure_downsample_visual() -> None:
    """One library, full resolution vs reduced, zoomed into a spike region."""
    from PIL import Image

    pairs = [
        ("qwt", "raster", "minmaxlttb", "Qwt MinMaxLTTB"),
        ("qcustomplot", "raster", "adaptive", "QCustomPlot adaptive"),
        ("qim", "opengl", "minmaxlttb", "QIm MinMaxLTTB"),
    ]
    available = [(lib, backend, strategy, name) for lib, backend, strategy, name in pairs
                 if os.path.exists(os.path.join(DATA_DIR, "images",
                                                f"{lib}_{backend}_spiky_1m_full.png"))]
    if not available:
        print("  (skip downsample visual: no exported images)")
        return

    fig, axes = plt.subplots(2, len(available), figsize=(3.1 * len(available), 4.4))
    axes = np.asarray(axes).reshape(2, len(available))
    for column, (library, backend, strategy, name) in enumerate(available):
        reference = np.asarray(Image.open(os.path.join(
            DATA_DIR, "images", f"{library}_{backend}_spiky_1m_full.png")))
        reduced_path = os.path.join(DATA_DIR, "images",
                                    f"{library}_{backend}_spiky_1m_{strategy}.png")
        if not os.path.exists(reduced_path):
            continue
        reduced = np.asarray(Image.open(reduced_path))

        difference = np.abs(reference.astype(np.int16) - reduced.astype(np.int16)).max(axis=2)
        changed = np.argwhere(difference > 16)
        if changed.size:
            top = int(changed[:, 0].min())
            left = int(changed[:, 1].min())
            crop = (max(0, top - 20), min(reference.shape[0], top + 60),
                    max(0, left - 40), min(reference.shape[1], left + 120))
            for row, image in enumerate([reference, reduced]):
                axes[row, column].imshow(image[crop[0]:crop[1], crop[2]:crop[3]])
        else:
            for row, image in enumerate([reference, reduced]):
                axes[row, column].imshow(image[200:340, 300:600])

        axes[0, column].set_title(name, fontsize=9)
        for row in range(2):
            axes[row, column].set_xticks([])
            axes[row, column].set_yticks([])
            axes[row, column].grid(False)
    axes[0, 0].set_ylabel("full resolution", fontsize=9)
    axes[1, 0].set_ylabel("reduced", fontsize=9)
    fig.suptitle("Worst deviation region of a 1M sample spiky signal", fontsize=10)
    fig.tight_layout(rect=(0, 0, 1, 0.94))
    save(fig, "fig_downsample_visual.png")
    plt.close(fig)


def main() -> None:
    use_report_style()
    frames = load_frames()
    configs = load_configs()
    if frames.empty:
        raise SystemExit("no CSV data - run examples/bench/chartbench/run-bench.sh first")

    print(f"loaded {len(frames)} frames and {len(configs)} configurations from {DATA_DIR}")

    figure_points(frames)
    figure_per_point_cost(frames)
    figure_curves(frames)
    figure_canvas(frames)
    figure_update(frames)
    figure_streaming(frames)
    figure_downsample(frames)
    figure_pareto(frames)
    figure_fidelity_metrics()
    figure_jitter(frames)
    figure_memory(configs)
    figure_setup(configs)
    figure_backend(frames)
    figure_summary(frames)
    figure_render_comparison()
    figure_downsample_visual()


if __name__ == "__main__":
    main()
