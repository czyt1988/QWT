"""Figures for the RoundPoints report.

Reads the raw measurements produced next to this script:

  roundpoints_results.csv          roundpoints_bench.cpp
  roundpoints_render_metrics.csv   roundpoints_render_metrics.py
  out_render/                      roundpoints_render_check.cpp

and writes every PNG referenced by

  docs/tech-report/point-mapper-rounding.md
  docs/use-guide/2d/downsampling-pixel-align.md

and their Chinese counterparts. Run from this directory:

    python roundpoints_experiment.py

Environment variables (optional) point at the demo batches:

  ROUNDPOINTS_ROUND_DIR    default out_round
  ROUNDPOINTS_NOROUND_DIR  default out_noround

Figures:
  fig_roundpoints_mechanism.png  sub-pixel vertex vs snapped vertex
  fig_roundpoints_dpr.png        what one device pixel is at DPR 1 / 2
  fig_roundpoints_staircase.png  synthetic render: the same line, snapped and not
  fig_roundpoints_crops.png      real spectral peak: dense vs sparse, on vs off
  fig_roundpoints_axis.png       how much of the polyline becomes axis-aligned
  fig_roundpoints_perf.png       mapper time, RoundPoints off vs on
  fig_roundpoints_output.png     output vertex count, off vs on
  fig_roundpoints_metrics.png    antialiasing blend ratio of the real renders
"""

import csv
import os

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from PIL import Image, ImageDraw, ImageFont

BASE = os.path.dirname(os.path.abspath(__file__))
OUT = BASE

plt.rcParams.update(
    {
        "font.family": "DejaVu Sans",
        "font.size": 9,
        "axes.titlesize": 10,
        "axes.labelsize": 9,
        "axes.grid": True,
        "grid.alpha": 0.3,
        "grid.linewidth": 0.6,
        "figure.dpi": 160,
        "savefig.dpi": 160,
        "savefig.bbox": "tight",
    }
)

C_ON = "#c0392b"
C_OFF = "#1f77b4"

ALGO_ORDER = ["none", "weedout", "aggressive", "pixel", "lttb", "minmax", "adaptive"]
ALGO_LABELS = {
    "none": "no reduction",
    "weedout": "WeedOut",
    "aggressive": "Aggressive",
    "pixel": "Pixel column",
    "lttb": "LTTB",
    "minmax": "MinMax",
    "adaptive": "Adaptive",
}


def read_csv(name):
    with open(os.path.join(BASE, name), newline="") as fh:
        return [row for row in csv.reader(fh) if row]


def read_bench():
    rows = []
    for row in read_csv("roundpoints_results.csv"):
        if row[0] == "bench":
            _, shape, algo, rnd, dpr, points, out, mn, med, calls = row
            rows.append(dict(shape=shape, algo=algo, round=int(rnd), dpr=float(dpr),
                             points=int(points), out=int(out),
                             min_ms=float(mn), med_ms=float(med)))
    return rows


def read_geom():
    rows = []
    for row in read_csv("roundpoints_results.csv"):
        if row[0] == "geom":
            _, shape, algo, rnd, dpr, out_n, real, horiz, vert, hp, vp, ap = row
            rows.append(dict(shape=shape, algo=algo, round=int(rnd),
                             out=int(out_n), real=int(real),
                             axis_pct=float(ap)))
    return rows


def read_metrics():
    rows = []
    for row in read_csv("roundpoints_render_metrics.csv"):
        if row[0] == "dataset":
            continue
        dataset, view, algo, rnd, aa, pure, apex, ink = row
        rows.append(dict(dataset=dataset, view=view, algo=algo, round=int(rnd),
                         aa=float(aa), pure=float(pure), apex=int(apex), ink=int(ink)))
    return rows


# --------------------------------------------------------------------------
# Figure 1: mechanism - what snapping a vertex does to a line
# --------------------------------------------------------------------------


def fig_mechanism():
    fig, axes = plt.subplots(1, 2, figsize=(7.2, 3.0))

    t = np.linspace(0.0, 1.0, 14)
    xs = 1.0 + 6.0 * t
    ys = 1.0 + 5.0 * (1.0 - t) ** 2 * 0.8 + 2.2 * t

    for ax, snapped in zip(axes, (False, True)):
        ax.set_xlim(0.0, 8.0)
        ax.set_ylim(0.0, 7.0)
        ax.set_xticks(np.arange(0, 9, 1))
        ax.set_yticks(np.arange(0, 8, 1))
        ax.grid(True, which="both", alpha=0.35)
        ax.set_aspect("equal")

        vx = np.round(xs) if snapped else xs
        vy = np.round(ys) if snapped else ys

        tt = np.linspace(0.0, 1.0, 300)
        ax.plot(1.0 + 6.0 * tt, 1.0 + 5.0 * (1.0 - tt) ** 2 * 0.8 + 2.2 * tt,
                color="#999999", lw=1.0, ls="--", zorder=1)
        ax.plot(vx, vy, color=C_ON if snapped else C_OFF, lw=1.6, zorder=2,
                marker="o", ms=3.4, mfc="white", mec=C_ON if snapped else C_OFF)

        if snapped:
            k = 6
            ax.annotate("", xy=(vx[k], vy[k]), xytext=(xs[k], ys[k]),
                        arrowprops=dict(arrowstyle="->", color="#7f8c8d", lw=0.9))
            ax.text(xs[k] + 0.12, ys[k] - 0.5, "snap to grid", color="#7f8c8d", fontsize=7.5)

        ax.set_xlabel("device pixel column")
        ax.set_ylabel("device pixel row")
        ax.set_title("vertices keep sub-pixel coordinates" if not snapped
                     else "RoundPoints: vertices snapped to the pixel grid",
                     color=C_OFF if not snapped else C_ON)

    fig.suptitle("One vertex, two rasterizer inputs: the geometry changes before QPainter sees it", y=1.02)
    fig.savefig(os.path.join(OUT, "fig_roundpoints_mechanism.png"))
    plt.close(fig)


# --------------------------------------------------------------------------
# Figure 2: what one "pixel" is at DPR 1 and DPR 2
# --------------------------------------------------------------------------


def fig_dpr():
    fig, ax = plt.subplots(figsize=(7.2, 2.3))
    ax.set_xlim(-0.4, 5.4)
    ax.set_ylim(-0.1, 2.3)
    ax.axis("off")

    for row, dpr in enumerate((1.0, 2.0)):
        y0 = 1.35 - row * 1.05
        n_cells = 9
        for i in range(n_cells):
            ax.add_patch(plt.Rectangle((i * 0.5, y0), 0.5, 0.5,
                                       facecolor="#f4f6f7", edgecolor="#bdc3c7", lw=0.6))

        step = 1.0 / dpr
        for v in np.arange(0.0, n_cells * 0.5 + 1e-9, step):
            ax.plot([v, v], [y0 - 0.06, y0 + 0.56], color="#34495e", lw=0.5, alpha=0.45)
        for v in np.arange(0.0, n_cells * 0.5 + 1e-9, 0.5):
            ax.plot([v, v], [y0 - 0.10, y0 + 0.60], color="#34495e", lw=0.9)
            ax.text(v, y0 - 0.20, f"{v:g}", ha="center", va="top", fontsize=7)

        ax.text(-0.15, y0 + 0.25, f"DPR {dpr:g}", ha="right", va="center", fontsize=8)
        ax.text(n_cells * 0.5 + 0.15, y0 + 0.25,
                "logical step 1.0\n= 1 device pixel" if dpr == 1.0
                else "logical step 0.5\n= 1 device pixel (2x2 device pixels)",
                va="center", fontsize=7.5, color="#34495e")

    ax.set_title("RoundPoints snaps to device pixels: the step is 1/DPR logical units", pad=6)
    fig.savefig(os.path.join(OUT, "fig_roundpoints_dpr.png"))
    plt.close(fig)


# --------------------------------------------------------------------------
# Figure 3: synthetic render - the same line, snapped vs not
# --------------------------------------------------------------------------


def fig_staircase():
    """Zoom of out_render/slope0_20__{round,noround}.png.

    A line rising 0.2 pixel rows per column is the worst case for snapping:
    the integer grid forces whole-pixel steps every five columns.
    """
    try:
        font = ImageFont.truetype("arial.ttf", 16)
    except OSError:
        font = ImageFont.load_default()

    src = os.path.join(BASE, "out_render")
    box = (190, 166, 430, 214)  # 240 x 48 crop, 8x zoom

    cells = []
    for tag, label in (("round", "pixel alignment ON (RoundPoints)"),
                       ("noround", "pixel alignment OFF (sub-pixel)")):
        img = Image.open(os.path.join(src, f"slope0_20__{tag}.png")).convert("RGB").crop(box)
        img = img.resize((img.width * 8, img.height * 8), Image.NEAREST)
        draw = ImageDraw.Draw(img)
        draw.rectangle((0, 0, img.width - 1, 26), fill=(255, 255, 255))
        draw.text((4, 4), label, fill=(0, 0, 0), font=font)
        cells.append(img)

    sheet = Image.new("RGB", (cells[0].width, cells[0].height * 2), (255, 255, 255))
    sheet.paste(cells[0], (0, 0))
    sheet.paste(cells[1], (0, cells[0].height))
    sheet.save(os.path.join(OUT, "fig_roundpoints_staircase.png"))
    print("wrote fig_roundpoints_staircase.png", sheet.size)


# --------------------------------------------------------------------------
# Figure 4: real spectral peak - dense vs sparse, on vs off
# --------------------------------------------------------------------------


def fig_crops():
    try:
        font = ImageFont.truetype("arial.ttf", 15)
    except OSError:
        font = ImageFont.load_default()

    round_dir = os.environ.get("ROUNDPOINTS_ROUND_DIR", "out_round")
    noround_dir = os.environ.get("ROUNDPOINTS_NOROUND_DIR", "out_noround")

    # upper flank of the tallest peak, 60 x 160 px, 5x nearest-neighbour zoom:
    # at this magnification the snapped polyline shows its 1 px treads while
    # the sub-pixel polyline stays a smooth diagonal
    box = (1020, 80, 1080, 240)
    rows = [("many", "dense 200k"), ("few", "sparse 18.6k")]
    cells = []

    for ds, label in rows:
        for tag, state in (("round", "alignment ON"), ("noround", "alignment OFF")):
            batch = round_dir if tag == "round" else noround_dir
            img = Image.open(os.path.join(batch, f"{ds}__none__win14.png")).convert("RGB").crop(box)
            img = img.resize((img.width * 5, img.height * 5), Image.NEAREST)
            draw = ImageDraw.Draw(img)
            draw.rectangle((0, 0, img.width - 1, 24), fill=(255, 255, 255))
            draw.text((5, 4), f"{label}, {state}", fill=(0, 0, 0), font=font)
            cells.append(img)

    sheet = Image.new("RGB", (cells[0].width * 4, cells[0].height), (255, 255, 255))
    for i, cell in enumerate(cells):
        sheet.paste(cell, (i * cell.width, 0))
    sheet.save(os.path.join(OUT, "fig_roundpoints_crops.png"))
    print("wrote fig_roundpoints_crops.png", sheet.size)


# --------------------------------------------------------------------------
# Figure 5: axis-aligned segment fraction
# --------------------------------------------------------------------------


def fig_axis(geom):
    algos = ALGO_ORDER
    x = np.arange(len(algos))
    w = 0.38

    sel = [g for g in geom if g["shape"] == "smooth"]
    off = {g["algo"]: g["axis_pct"] for g in sel if g["round"] == 0}
    on = {g["algo"]: g["axis_pct"] for g in sel if g["round"] == 1}

    fig, ax = plt.subplots(figsize=(7.2, 2.8))
    ax.bar(x - w / 2, [off[a] * 100 for a in algos], w, label="RoundPoints off", color=C_OFF)
    ax.bar(x + w / 2, [on[a] * 100 for a in algos], w, label="RoundPoints on", color=C_ON)

    for i, a in enumerate(algos):
        ax.text(i - w / 2, off[a] * 100 + 1.5, f"{off[a]*100:.1f}", ha="center", fontsize=6.5)
        ax.text(i + w / 2, on[a] * 100 + 1.5, f"{on[a]*100:.1f}", ha="center", fontsize=6.5)

    ax.set_xticks(x)
    ax.set_xticklabels([ALGO_LABELS[a] for a in algos], rotation=15, ha="right")
    ax.set_ylabel("axis-aligned segments (%)")
    ax.set_ylim(0, 112)
    ax.set_title("Segments of the mapped polyline that are exactly horizontal or vertical\n"
                 "(smooth 1M-point signal, 1200x800 canvas, DPR 1)")
    ax.legend(frameon=False, fontsize=8, loc="upper center",
              bbox_to_anchor=(0.5, -0.24), ncol=2)
    fig.savefig(os.path.join(OUT, "fig_roundpoints_axis.png"))
    plt.close(fig)


# --------------------------------------------------------------------------
# Figure 6: mapper time, off vs on
# --------------------------------------------------------------------------


def fig_perf(rows):
    algos = ALGO_ORDER
    smooth = [r for r in rows if r["shape"] == "smooth" and r["dpr"] == 1.0]
    off = {r["algo"]: r["med_ms"] for r in smooth if r["round"] == 0}
    on = {r["algo"]: r["med_ms"] for r in smooth if r["round"] == 1}

    x = np.arange(len(algos))
    w = 0.38

    fig, ax = plt.subplots(figsize=(7.2, 3.0))
    b1 = ax.bar(x - w / 2, [off[a] for a in algos], w, label="RoundPoints off", color=C_OFF)
    b2 = ax.bar(x + w / 2, [on[a] for a in algos], w, label="RoundPoints on", color=C_ON)

    for bars in (b1, b2):
        for rect in bars:
            h = rect.get_height()
            ax.text(rect.get_x() + rect.get_width() / 2, h + 0.3, f"{h:.1f}",
                    ha="center", va="bottom", fontsize=7)

    ax.set_xticks(x)
    ax.set_xticklabels([ALGO_LABELS[a] for a in algos], rotation=15, ha="right")
    ax.set_ylabel("mapper time per call (ms)")
    ax.set_title("QwtPointMapper::toPolygonF, 1,000,000 samples, 1200x800 canvas, DPR 1")
    ax.legend(frameon=False, fontsize=8)
    ax.set_ylim(0, max(max(off.values()), max(on.values())) * 1.2)
    fig.savefig(os.path.join(OUT, "fig_roundpoints_perf.png"))
    plt.close(fig)


# --------------------------------------------------------------------------
# Figure 7: output vertex count, off vs on
# --------------------------------------------------------------------------


def fig_output(rows):
    algos = ALGO_ORDER
    smooth = [r for r in rows if r["shape"] == "smooth" and r["dpr"] == 1.0]
    off = {r["algo"]: r["out"] for r in smooth if r["round"] == 0}
    on = {r["algo"]: r["out"] for r in smooth if r["round"] == 1}

    x = np.arange(len(algos))
    w = 0.38

    fig, ax = plt.subplots(figsize=(7.2, 2.8))
    ax.bar(x - w / 2, [off[a] for a in algos], w, label="RoundPoints off", color=C_OFF)
    ax.bar(x + w / 2, [on[a] for a in algos], w, label="RoundPoints on", color=C_ON)
    ax.set_yscale("log")
    ax.set_xticks(x)
    ax.set_xticklabels([ALGO_LABELS[a] for a in algos], rotation=15, ha="right")
    ax.set_ylabel("vertices returned (log scale)")
    ax.set_title("How many vertices the mapper returns (1M samples)")
    ax.legend(frameon=False, fontsize=8)
    ax.set_ylim(1e3, 4e6)

    for i, a in enumerate(algos):
        ax.text(i - w / 2, off[a] * 1.15, f"{off[a]:,}", ha="center", va="bottom", fontsize=6, rotation=90)
        ax.text(i + w / 2, on[a] * 1.15, f"{on[a]:,}", ha="center", va="bottom", fontsize=6, rotation=90)

    fig.savefig(os.path.join(OUT, "fig_roundpoints_output.png"))
    plt.close(fig)


# --------------------------------------------------------------------------
# Figure 8: antialiasing blend ratio of the real renders
# --------------------------------------------------------------------------


def fig_metrics(metrics):
    algos = ["none", "aggressive", "pixel", "lttb", "adaptive"]
    labels = {"none": "no reduction", "aggressive": "Aggressive", "pixel": "Pixel column",
              "lttb": "LTTB", "adaptive": "Adaptive"}

    sel = [m for m in metrics if m["view"] == "shoulder"]
    x = np.arange(len(algos) * 2)
    w = 0.38

    fig, ax = plt.subplots(figsize=(7.2, 2.8))

    for k, ds in enumerate(("many", "few")):
        off = {m["algo"]: m["aa"] for m in sel if m["dataset"] == ds and m["round"] == 0}
        on = {m["algo"]: m["aa"] for m in sel if m["dataset"] == ds and m["round"] == 1}
        pos = x[k * len(algos):(k + 1) * len(algos)]
        ax.bar(pos - w / 2, [off[a] for a in algos], w, color=C_OFF,
               label="alignment OFF" if k == 0 else None)
        ax.bar(pos + w / 2, [on[a] for a in algos], w, color=C_ON,
               label="alignment ON" if k == 0 else None)

    ax.set_xticks(x)
    ax.set_xticklabels([labels[a] for a in algos] * 2, rotation=20, ha="right", fontsize=7.5)
    ax.axvline(len(algos) - 0.5, color="#bdc3c7", lw=0.8)
    ax.text(len(algos) / 2 - 0.5, 0.62, "dense: 200k points", ha="center", fontsize=8)
    ax.text(len(algos) * 1.5 - 0.5, 0.62, "sparse: 18.6k points", ha="center", fontsize=8)
    ax.set_ylabel("blend pixels / all curve pixels")
    ax.set_ylim(0, 0.72)
    ax.set_title("Antialiasing blend ratio of the drawn peak shoulder\n"
                 "(antialiasing is on in both cases; a snapped polyline loses its blend pixels)")
    ax.legend(frameon=False, fontsize=8, loc="upper right")
    fig.savefig(os.path.join(OUT, "fig_roundpoints_metrics.png"))
    plt.close(fig)


def main():
    fig_mechanism()
    fig_dpr()

    if os.path.isdir(os.path.join(BASE, "out_render")):
        fig_staircase()
    else:
        print("out_render not found, skipping fig_roundpoints_staircase.png")

    batch_round = os.path.join(BASE, os.environ.get("ROUNDPOINTS_ROUND_DIR", "out_round"))
    batch_noround = os.path.join(BASE, os.environ.get("ROUNDPOINTS_NOROUND_DIR", "out_noround"))
    if os.path.isdir(batch_round) and os.path.isdir(batch_noround):
        fig_crops()
    else:
        print("render batches not found, skipping fig_roundpoints_crops.png")

    bench = read_bench()
    fig_perf(bench)
    fig_output(bench)
    fig_axis(read_geom())
    fig_metrics(read_metrics())

    print("done")


if __name__ == "__main__":
    main()
