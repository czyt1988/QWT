"""Shared loading and aggregation helpers for the chartbench data.

The C++ harness in examples/bench/chartbench writes one pair of CSV tables per
(library, backend) run:

    data/frames_<library>_<backend>.csv    one row per measured frame
    data/configs_<library>_<backend>.csv   one row per measured configuration

Both are long format; every figure script of the report loads them through
load_frames() / load_configs() so that all plots share one definition of the
measured quantities.
"""

from __future__ import annotations

import os
import glob
import re

import numpy as np
import pandas as pd

HERE = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(HERE, "data")
IMAGE_DIR = os.path.join(DATA_DIR, "images")

#: Library id -> label used in every figure and table.
LIBRARY_LABELS = {
    "qwt": "Qwt 7.5",
    "qcustomplot": "QCustomPlot 2.1",
    "qtcharts": "Qt Charts 6.7",
    "qim": "QIm 0.3 (ImPlot)",
}

#: Okabe-Ito inspired, distinguishable in print and for colour vision deficiency.
LIBRARY_COLORS = {
    "qwt": "#0072B2",
    "qcustomplot": "#D55E00",
    "qtcharts": "#009E73",
    "qim": "#CC79A7",
}

LIBRARY_MARKERS = {
    "qwt": "o",
    "qcustomplot": "s",
    "qtcharts": "^",
    "qim": "D",
}

#: Draw order, keeps the legend stable across figures.
LIBRARY_ORDER = ["qwt", "qcustomplot", "qtcharts", "qim"]

#: The backend each library is normally used with. The OpenGL variants of Qwt
#: and QCustomPlot are compared separately in the backend figure, everything
#: else reports the primary backend only.
PRIMARY_BACKEND = {
    "qwt": "raster",
    "qcustomplot": "raster",
    "qtcharts": "raster",
    "qim": "opengl",
}


def on_primary_backend(table: pd.DataFrame) -> pd.DataFrame:
    keep = table.apply(
        lambda row: PRIMARY_BACKEND.get(row["library"]) == row["backend"], axis=1)
    return table[keep]


def _read(pattern: str) -> pd.DataFrame:
    files = sorted(glob.glob(os.path.join(DATA_DIR, pattern)))
    frames = []
    for path in files:
        table = pd.read_csv(path)
        table["source"] = os.path.basename(path)
        frames.append(table)
    if not frames:
        return pd.DataFrame()
    return pd.concat(frames, ignore_index=True)


def load_frames() -> pd.DataFrame:
    """Every measured frame of every run."""
    return _read("frames_*.csv")


def load_configs() -> pd.DataFrame:
    """Every measured configuration (setup time, memory, frame count)."""
    return _read("configs_*.csv")


def series_key(row) -> str:
    """Label that distinguishes library and backend in a legend."""
    if row["backend"] == "opengl" and row["library"] != "qim":
        return f"{LIBRARY_LABELS.get(row['library'], row['library'])} (GL)"
    return LIBRARY_LABELS.get(row["library"], row["library"])


def summarise(frames: pd.DataFrame, group) -> pd.DataFrame:
    """Median/p95 frame time per group, dropping warm-up noise.

    The first measured frame of a configuration still contains one-off work
    (paint buffer allocation, GL program setup), so it is excluded from the
    aggregate statistics exactly like the warm-up frames before it.
    """
    trimmed = frames[frames["frame_idx"] > 0]
    grouped = trimmed.groupby(group, dropna=False)["render_ms"]
    result = grouped.agg(
        median="median",
        mean="mean",
        p95=lambda values: np.percentile(values, 95),
        p99=lambda values: np.percentile(values, 99),
        maximum="max",
        minimum="min",
        std="std",
        frames="count",
    ).reset_index()
    return result


# ---------------------------------------------------------------------------
# variant parsing
# ---------------------------------------------------------------------------

_NUMBER = re.compile(r"n(\d+)")
_CURVES = re.compile(r"c(\d+)")
_WINDOW = re.compile(r"_w(\d+)")
_CANVAS = re.compile(r"_(\d+)x(\d+)$")


def parse_points(variant: str) -> float:
    match = _NUMBER.search(variant)
    return float(match.group(1)) if match else np.nan


def parse_curves(variant: str) -> float:
    match = _CURVES.search(variant)
    return float(match.group(1)) if match else np.nan


def parse_window(variant: str) -> float:
    match = _WINDOW.search(variant)
    return float(match.group(1)) if match else np.nan


def parse_canvas(variant: str) -> float:
    match = _CANVAS.search(variant)
    if not match:
        return np.nan
    return float(int(match.group(1)) * int(match.group(2)))


# ---------------------------------------------------------------------------
# plotting defaults
# ---------------------------------------------------------------------------

def use_report_style() -> None:
    import matplotlib

    matplotlib.rcParams.update({
        "figure.dpi": 160,
        "savefig.dpi": 200,
        "savefig.bbox": "tight",
        "font.size": 9,
        "axes.titlesize": 10,
        "axes.labelsize": 9,
        "legend.fontsize": 8,
        "xtick.labelsize": 8,
        "ytick.labelsize": 8,
        "axes.grid": True,
        "grid.alpha": 0.3,
        "grid.linewidth": 0.5,
        "axes.axisbelow": True,
        "axes.spines.top": False,
        "axes.spines.right": False,
        "lines.linewidth": 1.6,
        "lines.markersize": 4.5,
        "legend.frameon": False,
    })


def save(fig, name: str) -> str:
    path = os.path.join(HERE, name)
    fig.savefig(path)
    print(f"  wrote {os.path.basename(path)}")
    return path
