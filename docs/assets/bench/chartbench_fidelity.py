"""Pixel level fidelity of the point reduction algorithms.

The C++ harness exports, for every library, the very same data set rendered
twice: once with every point reduction switch off (the reference) and once per
point reduction strategy. Because both images come from the same library with
identical axes, margins and styling, every pixel that differs is caused by the
reduction algorithm alone - which makes the metrics comparable across libraries
even though the libraries lay their plots out differently.

Metrics written to data/fidelity_metrics.csv:

    changed_frac    fraction of pixels whose largest channel difference > 16
    mae             mean absolute difference over all pixels, 0..255
    span_ratio      worst column: vertical extent of differing pixels / height
    ink_ratio       curve ink of the variant / curve ink of the reference

Run:  python chartbench_fidelity.py
"""

from __future__ import annotations

import os

import numpy as np
import pandas as pd
from PIL import Image

from chartbench_common import (
    DATA_DIR, HERE, IMAGE_DIR, LIBRARY_LABELS, on_primary_backend,
)

CHANGED_THRESHOLD = 16     # grey levels; below that it is antialiasing noise
INK_THRESHOLD = 400        # sum over RGB: 765 is white, a curve pixel is far below

#: Data sets the fidelity suite exports, see BenchSuites::runFidelity().
CASES = ["clean_1k", "spiky_100k", "spiky_1m"]


def _load(path: str) -> np.ndarray:
    return np.asarray(Image.open(path).convert("RGB")).astype(np.int16)


def compare(reference: np.ndarray, variant: np.ndarray) -> dict:
    if reference.shape != variant.shape:
        raise ValueError("image sizes differ")

    difference = np.abs(reference - variant).max(axis=2)
    changed = difference > CHANGED_THRESHOLD

    ink_ref = (reference.sum(axis=2) < INK_THRESHOLD)
    ink_var = (variant.sum(axis=2) < INK_THRESHOLD)

    # Worst vertical extent of the difference inside a single column: a spike
    # that a bucket sampler collapsed into a stroke shows up here.
    span_ratio = 0.0
    rows = np.any(changed, axis=0)
    for column in np.nonzero(rows)[0]:
        index = np.nonzero(changed[:, column])[0]
        if index.size:
            span_ratio = max(span_ratio, (index[-1] - index[0]) / changed.shape[0])

    return {
        "changed_frac": float(changed.mean()),
        "mae": float(np.abs(reference - variant).mean()),
        "span_ratio": float(span_ratio),
        "ink_ratio": float(ink_var.sum() / ink_ref.sum()) if ink_ref.sum() else float("nan"),
    }


def build_table() -> pd.DataFrame:
    if not os.path.isdir(IMAGE_DIR):
        raise SystemExit(f"no exported images in {IMAGE_DIR} - run examples/bench/chartbench/run-bench.sh")

    rows = []
    for name in sorted(os.listdir(IMAGE_DIR)):
        if not name.endswith(".png"):
            continue
        stem = name[:-4]
        # <library>_<backend>_<case>_<strategy>, where <case> itself has an underscore
        for case in CASES:
            marker = f"_{case}_"
            if marker in stem:
                prefix, strategy = stem.split(marker, 1)
                library, backend = prefix.split("_", 1)
                break
        else:
            continue
        if strategy == "full":
            continue

        reference_path = os.path.join(IMAGE_DIR, f"{library}_{backend}_{case}_full.png")
        if not os.path.exists(reference_path):
            continue

        metrics = compare(_load(reference_path), _load(os.path.join(IMAGE_DIR, name)))
        metrics.update({
            "library": library,
            "backend": backend,
            "case": case,
            "strategy": strategy,
        })
        rows.append(metrics)

    table = pd.DataFrame(rows)
    if table.empty:
        raise SystemExit("no comparable image pairs found")
    return table[[
        "library", "backend", "case", "strategy",
        "changed_frac", "mae", "span_ratio", "ink_ratio",
    ]]


def main() -> None:
    table = build_table()
    path = os.path.join(DATA_DIR, "fidelity_metrics.csv")
    table.to_csv(path, index=False)
    print(f"wrote {path} ({len(table)} comparisons, both backends)")

    table = on_primary_backend(table)
    for case in sorted(table["case"].unique()):
        subset = table[table["case"] == case].sort_values("changed_frac")
        print(f"\n{case}: pixel deviation from the library's own full resolution render")
        for _, row in subset.iterrows():
            print(f"  {LIBRARY_LABELS.get(row['library'], row['library']):20s} "
                  f"{row['strategy']:12s} changed={row['changed_frac']*100:6.2f}%  "
                  f"mae={row['mae']:6.2f}  span={row['span_ratio']:.3f}  "
                  f"ink={row['ink_ratio']:.3f}")


if __name__ == "__main__":
    main()
