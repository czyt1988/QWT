"""Render metrics for the RoundPoints report (docs/tech-report/point-mapper-rounding.md).

Scans the PNG batch produced by examples/2D/downsample_demo and measures the
pixel-level signature of vertex snapping:

  aa       blend pixels / all curve pixels inside the peak-shoulder crop.
           Antialiased edges of a diagonal line are blend pixels; once the
           vertices are snapped, long stretches of the polyline become exactly
           horizontal or vertical, their edges land on pixel boundaries and
           the blend pixels disappear. A high ratio is the visible signature
           of sub-pixel geometry.
  pure     pure pen pixels / all curve pixels (1 - aa), same crop.
  apex     topmost curve pixel row - the peak height must not change.
  ink      number of curve colored pixels inside the crop.

Usage:
    python roundpoints_render_metrics.py <batch-dir> <round:0|1> [--append]

Writes roundpoints_render_metrics.csv in the script directory. Run it once
for the batch rendered with pixel alignment on and once for `--noround`:

    downsample_demo --batch out_round
    downsample_demo --batch out_noround --noround
    python roundpoints_render_metrics.py out_round 1
    python roundpoints_render_metrics.py out_noround 0 --append
"""

import csv
import os
import sys

import numpy as np
from PIL import Image

BASE = os.path.dirname(os.path.abspath(__file__))

PEN = (32, 48, 192)
PEN_TOL = 12

# Crop box (left, top, right, bottom) around the rising shoulder of the
# tallest peak at DPR 1 - the shallowest part of the flank of the window
# views, where a snapped polyline shows the staircase best.
VIEWS = {
    "shoulder": (1000, 60, 1120, 420),
    "flank": (1090, 450, 1310, 660),
}

ALGOS = ["none", "weedout", "aggressive", "pixel", "lttb", "minmaxlttb", "adaptive"]


def analyze(path, box):
    a = np.asarray(Image.open(path).convert("RGB").crop(box)).astype(int)

    blue = (a[:, :, 2] - a[:, :, 0]) >= 30
    pure = ((np.abs(a[:, :, 0] - PEN[0]) <= PEN_TOL)
            & (np.abs(a[:, :, 1] - PEN[1]) <= PEN_TOL)
            & (np.abs(a[:, :, 2] - PEN[2]) <= PEN_TOL))

    ink = int(blue.sum())
    pure_n = int(pure.sum())
    blend = ink - pure_n

    aa = blend / ink if ink else 0.0
    pure_frac = pure_n / ink if ink else 0.0

    rows = np.nonzero(blue.any(axis=1))[0]
    apex = int(rows.min()) if len(rows) else -1

    return dict(aa=aa, pure=pure_frac, apex=apex, ink=ink)


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else "out_round"
    round_points = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    append = "--append" in sys.argv

    out_path = os.path.join(BASE, "roundpoints_render_metrics.csv")
    mode = "a" if append else "w"

    with open(out_path, mode, newline="") as fh:
        writer = csv.writer(fh)
        if not append:
            writer.writerow(["dataset", "view", "algo", "round", "aa", "pure", "apex", "ink"])

        for dataset in ("many", "few"):
            for view, box in VIEWS.items():
                for algo in ALGOS:
                    path = os.path.join(outdir, f"{dataset}__{algo}__win14.png")
                    if not os.path.exists(path):
                        continue
                    m = analyze(path, box)
                    writer.writerow([dataset, view, algo, round_points,
                                     f"{m['aa']:.4f}", f"{m['pure']:.4f}",
                                     m["apex"], m["ink"]])

    print("wrote", out_path)


if __name__ == "__main__":
    main()
