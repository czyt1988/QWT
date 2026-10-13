"""Regenerate the color stops of the matplotlib-sourced colormap presets.

The presets of src/core/qwt_colormap_preset.cpp claim to be "sourced from
matplotlib colormaps".  This script samples the matplotlib built-in tables at
the stop positions used by each preset and prints the C++ initializer lists, so
the tables can be regenerated/verified without hand-picking colors.

Lookup convention: ``matplotlib.colormaps[name](p)`` is matplotlib's own data ->
color mapping for a 256 entry table, i.e. it returns ``lut[int(p * 256)]`` with
the top of the range clamped to ``lut[255]``.  Sampling at the preset positions
therefore gives the color matplotlib shows for a value at that position.

Usage:
    python colormap_stops_gen.py            # print all regenerated presets
    python colormap_stops_gen.py viridis turbo

The output compares against the current source file (which is not modified);
copy the new initializer lists over manually after reviewing the difference.
"""

import sys
from pathlib import Path

import numpy as np
import matplotlib
from matplotlib import colormaps

HERE = Path(__file__).resolve().parent
SOURCE = HERE.parents[2] / "src" / "core" / "qwt_colormap_preset.cpp"

# Stop positions of every preset in the C++ source.  Only the matplotlib
# sourced presets are listed; "rainbow" is a QWT specific closed hue ring
# (OpenCV style) and is deliberately not a matplotlib table.
POSITIONS = {
    "viridis": [i / 8.0 for i in range(9)],
    "plasma": [i / 8.0 for i in range(9)],
    "inferno": [i / 8.0 for i in range(9)],
    "magma": [i / 8.0 for i in range(9)],
    "cividis": [i / 8.0 for i in range(9)],
    "jet": [i / 8.0 for i in range(9)],
    "hot": [0.0, 1.0 / 3.0, 2.0 / 3.0, 1.0],
    "cool": [0.0, 1.0],
    "spring": [0.0, 1.0],
    "summer": [0.0, 1.0],
    "autumn": [0.0, 1.0],
    "winter": [0.0, 1.0],
    "gray": [0.0, 1.0],
    "bone": [0.0, 1.0 / 3.0, 2.0 / 3.0, 1.0],
    "copper": [0.0, 1.0],
    "hsv": [i / 6.0 for i in range(7)],
    "turbo": [i / 8.0 for i in range(9)],
    "coolwarm": [i / 4.0 for i in range(5)],
    "rdylbu": [i / 10.0 for i in range(11)],
    "rdylgn": [i / 10.0 for i in range(11)],
    "spectral": [i / 10.0 for i in range(11)],
}

# The names as spelled in the C++ functions / matplotlib
CPP_FUNC = {
    "viridis": "viridisStops",
    "plasma": "plasmaStops",
    "inferno": "infernoStops",
    "magma": "magmaStops",
    "cividis": "cividisStops",
    "jet": "jetStops",
    "hot": "hotStops",
    "cool": "coolStops",
    "spring": "springStops",
    "summer": "summerStops",
    "autumn": "autumnStops",
    "winter": "winterStops",
    "gray": "grayStops",
    "bone": "boneStops",
    "copper": "copperStops",
    "hsv": "hsvStops",
    "turbo": "turboStops",
    "coolwarm": "coolWarmStops",
    "rdylbu": "rdYlBuStops",
    "rdylgn": "rdYlGnStops",
    "spectral": "spectralStops",
}


def sample(name, positions):
    """Colors of the matplotlib table at the given normalized positions."""
    cmap = colormaps[name]
    return [tuple(int(round(v * 255.0)) for v in np.asarray(cmap(p))[:3]) for p in positions]


def current_stops(name):
    """Parse the stop list of one preset from the C++ source (for comparison)."""
    import re

    text = SOURCE.read_text(encoding="utf-8")
    body = re.search(
        r"static QVector< QPair< double, QColor > > %s\(\)\s*\{(.*?)\}" % CPP_FUNC[name],
        text,
        re.S)
    if not body:
        raise SystemExit(f"cannot find {CPP_FUNC[name]}() in {SOURCE}")
    return [tuple(int(v) for v in m) for m in re.findall(r"QColor\((\d+),\s*(\d+),\s*(\d+)\)", body.group(1))]


def format_stops(positions, colors):
    items = ["{{ {p:.3f}, QColor({r}, {g}, {b}) }}".format(p=p, r=c[0], g=c[1], b=c[2])
             for p, c in zip(positions, colors)]
    lines = []
    for i in range(0, len(items), 3):
        lines.append("             " + ", ".join(items[i:i + 3]) + ",")
    return "\n".join(lines).rstrip(",")


def main():
    names = sys.argv[1:] or sorted(POSITIONS)
    print(f"# matplotlib {matplotlib.__version__}; source: {SOURCE}")
    for name in names:
        positions = POSITIONS[name]
        new = sample(name, positions)
        old = current_stops(name)
        same = old == new
        print(f"\n===== {name} ({CPP_FUNC[name]}) {'UNCHANGED' if same else 'CHANGED'} =====")
        if not same:
            print(f"  current: {old}")
            print(f"  sampled: {new}")
        print(f"static QVector< QPair< double, QColor > > {CPP_FUNC[name]}()")
        print("{")
        print("    return { " + format_stops(positions, new) + " };")
        print("}")


if __name__ == "__main__":
    main()
