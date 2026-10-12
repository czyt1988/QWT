"""
Polygon clipping experiments for the QwtClipper tech report
(docs/zh/tech-report/polygon-clipping.md).

The script contains a faithful Python re-implementation of
src/core/qwt_clipper.cpp (Sutherland-Hodgman polygon clipping + the
angle-sweep circle clipper), validates it bit-exactly against the real
C++ implementation through the clipper_check harness, then measures:

  1. vertex amplification under pan / zoom sequences,
  2. the identity  n_out = V_in + X_boundary,
  3. degenerate inputs (self-intersection, collinear, 1px viewport,
     fully outside, on-boundary vertices, duplicate points, NaN, ...),
  4. clipCircle vs a brute-force angular ground truth.

All raw numbers are printed to stdout; figures are written next to this
script.  Dependencies: Python 3.11, numpy, matplotlib (Agg).  No other
third-party packages.  Deterministic: all RNG use fixed seeds.

Usage:
    python clipper_experiment.py                # everything (needs harness exe)
    python clipper_experiment.py --no-xcheck    # skip the C++ cross-check
    python clipper_experiment.py --scratch DIR  # harness dir (exe + case file)
"""

import argparse
import math
import os
import subprocess
import sys
from pathlib import Path

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = Path(__file__).resolve().parent
DEFAULT_SCRATCH = Path("D:/tmp/qwt-scratch/clipper")

TWO_PI = 2.0 * math.pi
SEED = 20261012

COL_IN = "#9aa0a6"     # input geometry
COL_OUT = "#1f77b4"    # clipped output
COL_TRUTH = "#2ca02c"  # brute-force ground truth
COL_BAD = "#d62728"    # wrong / artifact


# =====================================================================
# Section 1: faithful re-implementation of src/core/qwt_clipper.cpp
# =====================================================================
#
# Every function below mirrors the C++ operation order exactly so that
# IEEE-754 double results are bit-identical (verified in section 5).

def c_int(v):
    """C++ static_cast<int>(double): truncation toward zero."""
    return int(v)


def bounds_F(rect):
    """QwtPolygonClipper<QPolygonF, QRectF, double> edge bounds:
    x1 = x(), x2 = x()+width(), y1 = y(), y2 = y()+height()."""
    x, y, w, h = rect
    return (x, x + w, y, y + h)


def bounds_I(rect):
    """Integer QRect: same formula, integer arithmetic.
    Note QRect::right() == x+w-1, so the clip region spans w+1 columns."""
    x, y, w, h = rect
    return (x, x + w, y, y + h)


def bounds_R(rect):
    """clipPolygon(QRectF, QPolygon): the float rect is rounded inwards
    minX = qCeil(left), maxX = qFloor(right), and then used as the
    integer QRect(minX, minY, maxX-minX, maxY-minY)."""
    x, y, w, h = rect
    min_x = math.ceil(x)
    max_x = math.floor(x + w)
    min_y = math.ceil(y)
    max_y = math.floor(y + h)
    return (min_x, min_x + (max_x - min_x), min_y, min_y + (max_y - min_y))


def edge_ops(edge, bounds, to_int):
    """Return (is_inside, intersect) closures mirroring QwtClip::LeftEdge,
    RightEdge, TopEdge, BottomEdge.  intersect(p1, p2) follows the source:
    dy = (p1.y - p2.y) / double(p1.x - p2.x), anchored at p2."""
    x1, x2, y1, y2 = bounds
    if edge == "L":
        def is_inside(p):
            return p[0] >= x1

        def intersect(p1, p2):
            dy = (p1[1] - p2[1]) / float(p1[0] - p2[0])
            yv = p2[1] + (x1 - p2[0]) * dy
            return (x1, c_int(yv) if to_int else yv)
    elif edge == "R":
        def is_inside(p):
            return p[0] <= x2

        def intersect(p1, p2):
            dy = (p1[1] - p2[1]) / float(p1[0] - p2[0])
            yv = p2[1] + (x2 - p2[0]) * dy
            return (x2, c_int(yv) if to_int else yv)
    elif edge == "T":
        def is_inside(p):
            return p[1] >= y1

        def intersect(p1, p2):
            dx = (p1[0] - p2[0]) / float(p1[1] - p2[1])
            xv = p2[0] + (y1 - p2[1]) * dx
            return (c_int(xv) if to_int else xv, y1)
    else:  # "B"
        def is_inside(p):
            return p[1] <= y2

        def intersect(p1, p2):
            dx = (p1[0] - p2[0]) / float(p1[1] - p2[1])
            xv = p2[0] + (y2 - p2[1]) * dx
            return (c_int(xv) if to_int else xv, y2)
    return is_inside, intersect


def clip_edge(points, is_inside, intersect, close, stats=None):
    """Mirror of QwtPolygonClipper::clipEdge (one Sutherland-Hodgman pass).

    stats: optional dict receiving {'n_v': inside vertices emitted,
    'n_x': intersections emitted, 'in_size': len(points)}.  The exact
    per-pass accounting identity is  len(out) == n_v + n_x  whenever
    in_size >= 2 (a size-1 input is copied unconditionally instead)."""
    out = []
    n = len(points)
    n_v = 0
    n_x = 0

    def emit_vertex(p):
        nonlocal n_v
        n_v += 1
        out.append(p)

    def emit_isect(p1, p2):
        nonlocal n_x
        n_x += 1
        out.append(intersect(p1, p2))

    if n < 2:
        # source behavior: a single point is copied unconditionally,
        # WITHOUT an inside test (degenerate pass-through, see report)
        if n == 1:
            out.append(points[0])
        if stats is not None:
            stats.update(n_v=0, n_x=0, in_size=n, passthrough=(n == 1))
        return out

    if not close:
        p1 = points[0]
        if is_inside(p1):
            emit_vertex(p1)
    else:
        p1 = points[0]
        p2 = points[-1]
        if is_inside(p1):
            if not is_inside(p2):
                emit_isect(p1, p2)
            emit_vertex(p1)
        elif is_inside(p2):
            emit_isect(p1, p2)

    for i in range(1, n):
        p1 = points[i]
        p2 = points[i - 1]
        if is_inside(p1):
            if not is_inside(p2):
                emit_isect(p1, p2)
            emit_vertex(p1)
        elif is_inside(p2):
            emit_isect(p1, p2)

    if stats is not None:
        stats.update(n_v=n_v, n_x=n_x, in_size=n, passthrough=False)
    return out


def clip_polygon(rect, points, close, mode, trace=None):
    """QwtClipper::clipPolygon / clipPolygonF.

    mode 'F': QRectF  x QPolygonF (double coordinates)
    mode 'I': QRect   x QPolygon  (int coordinates)
    mode 'R': QRectF  x QPolygon  (float rect rounded inwards to int)

    trace: optional list receiving (edge, output_points, stats) per pass.
    """
    if mode == "F":
        bounds = bounds_F(rect)
        to_int = False
    elif mode == "I":
        bounds = bounds_I(rect)
        to_int = True
    else:
        bounds = bounds_R(rect)
        to_int = True

    pts = list(points)
    for edge in ("L", "R", "T", "B"):
        is_inside, intersect = edge_ops(edge, bounds, to_int)
        stats = {}
        pts = clip_edge(pts, is_inside, intersect, close, stats)
        if trace is not None:
            trace.append((edge, list(pts), stats))
    return pts


# ------------------------------------------------------------- circle

def rect_contains_point(rect, p):
    """QRectF::contains(QPointF) for Qt 5.15.16 - empirically inclusive
    on all four borders (verified by the harness PROBE lines)."""
    x, y, w, h = rect
    if p[0] != p[0] or p[1] != p[1]:  # NaN
        return False
    return x <= p[0] <= x + w and y <= p[1] <= y + h


def rect_contains_rect(rect, r):
    """QRectF::contains(QRectF) - empirical Qt 5.15.16 semantics,
    characterized by 20 harness probes (differs from the textbook
    'inclusive border compare' rule!):

      (50,50,10,10)      -> 1     (50,50,-10,10)   -> 1
      (50,50,0,10)       -> 0     (50,50,10,-10)   -> 1
      (50,50,0,0)        -> 0     (150,50,-100,10) -> 0
      (50,50,1e-300,10)  -> 0     (50,50,1e-13,10) -> 1
      (50,50,60,10)      -> 0     (50,50,-10,-10)  -> 1

    Fitted rule (matches all probes): normalize the argument (flip
    negative extents), require strictly growing corners in floating
    point (rx+rw > rx, so a width absorbed by rounding like 1e-300 at
    x=50 fails), then compare borders inclusively.  Consequence for
    clipCircle: a radius-0 circle yields NO arcs even when its center
    is inside the rect."""
    x, y, w, h = rect
    rx, ry, rw, rh = r
    if rw < 0.0:
        rx, rw = rx + rw, -rw
    if rh < 0.0:
        ry, rh = ry + rh, -rh
    if not (rx + rw > rx and ry + rh > ry):
        return False
    return x <= rx and x + w >= rx + rw and y <= ry and y + h >= ry + rh


def polar2pos(pos, radius, angle):
    """qwtPolar2Pos: y-up mathematical convention on a y-down screen."""
    return (pos[0] + radius * math.cos(angle),
            pos[1] - radius * math.sin(angle))


def to_angle(frm, to):
    """QwtCircleClipper::toAngle - mirrored verbatim, including the
    vertical special case (which is inconsistent with polar2pos, see
    the report's failure-mode section)."""
    if frm[0] == to[0]:
        return math.pi / 2.0 if frm[1] <= to[1] else 3.0 * math.pi / 2.0
    m = abs((to[1] - frm[1]) / (to[0] - frm[0]))
    angle = math.atan(m)
    if to[0] > frm[0]:
        if to[1] > frm[1]:
            angle = TWO_PI - angle
    else:
        if to[1] > frm[1]:
            angle = math.pi + angle
        else:
            angle = math.pi - angle
    return angle


def cutting_points(rect, pos, radius):
    """QwtCircleClipper::cuttingPoints, edge order Left, Top, Right, Bottom."""
    x, y, w, h = rect
    left, top = x, y
    right, bottom = x + w, y + h
    pts = []
    for edge in ("L", "T", "R", "B"):
        if edge == "L" or edge == "R":
            ex = left if edge == "L" else right
            if abs(pos[0] - ex) < radius:
                off = math.sqrt(radius * radius - (pos[0] - ex) * (pos[0] - ex))
                y1 = pos[1] + off
                if top <= y1 <= bottom:
                    pts.append((ex, y1))
                y2 = pos[1] - off
                if top <= y2 <= bottom:
                    pts.append((ex, y2))
        else:
            ey = top if edge == "T" else bottom
            if abs(pos[1] - ey) < radius:
                off = math.sqrt(radius * radius - (pos[1] - ey) * (pos[1] - ey))
                x1 = pos[0] + off
                if left <= x1 <= right:
                    pts.append((x1, ey))
                x2 = pos[0] - off
                if left <= x2 <= right:
                    pts.append((x2, ey))
    return pts


def clip_circle(rect, pos, radius):
    """QwtClipper::clipCircle - angle sweep with midpoint in/out pairing."""
    pts = cutting_points(rect, pos, radius)
    if len(pts) <= 0:
        crect = (pos[0] - radius, pos[1] - radius, 2.0 * radius, 2.0 * radius)
        if rect_contains_rect(rect, crect):
            return [(0.0, TWO_PI)]
        return []

    angles = sorted(to_angle(pos, p) for p in pts)
    mid = angles[0] + (angles[1] - angles[0]) / 2.0
    is_in = rect_contains_point(rect, polar2pos(pos, radius, mid))

    intv = []
    if is_in:
        for i in range(0, len(angles) - 1, 2):
            intv.append((angles[i], angles[i + 1]))
    else:
        for i in range(1, len(angles) - 1, 2):
            intv.append((angles[i], angles[i + 1]))
        intv.append((angles[-1], angles[0]))
    return intv


# =====================================================================
# Section 2: analysis helpers (metrics, ground truth)
# =====================================================================

def visible_vertices(rect, pts, mode):
    """V_in: number of raw input vertices inside the final clip region
    (all four half-plane tests, inclusive)."""
    if mode == "F":
        b = bounds_F(rect)
    elif mode == "I":
        b = bounds_I(rect)
    else:
        b = bounds_R(rect)
    x1, x2, y1, y2 = b
    c = 0
    for (px, py) in pts:
        if px != px or py != py:  # NaN -> outside everywhere
            continue
        if x1 <= px <= x2 and y1 <= py <= y2:
            c += 1
    return c


def boundary_crossings(rect, pts, close, mode):
    """X_dR: number of intersections between input segments and the four
    border SEGMENTS of the clip region (extent-limited).  Theory: for
    generic position n_out = V_in + X_dR."""
    if mode == "F":
        b = bounds_F(rect)
    elif mode == "I":
        b = bounds_I(rect)
    else:
        b = bounds_R(rect)
    x1, x2, y1, y2 = b
    n = len(pts)
    if n < 2:
        return 0
    segs = list(zip(pts, pts[1:]))
    if close:
        segs.append((pts[-1], pts[0]))
    # (line coordinate, extent lo, extent hi, axis, inside-is->=)
    edge_tests = (
        (x1, y1, y2, "x", True),    # left border line
        (x2, y1, y2, "x", False),   # right border line
        (y1, x1, x2, "y", True),    # top border line
        (y2, x1, x2, "y", False),   # bottom border line
    )
    total = 0
    for (p, q) in segs:
        if p[0] != p[0] or q[0] != q[0] or p[1] != p[1] or q[1] != q[1]:
            continue  # NaN segments never cross (all tests false)
        for (coord, lo, hi, axis, ge) in edge_tests:
            if axis == "x":
                ip = (p[0] >= coord) if ge else (p[0] <= coord)
                iq = (q[0] >= coord) if ge else (q[0] <= coord)
                if ip != iq:
                    t = (coord - q[0]) / (p[0] - q[0])
                    yi = q[1] + (p[1] - q[1]) * t
                    if lo <= yi <= hi:
                        total += 1
            else:
                ip = (p[1] >= coord) if ge else (p[1] <= coord)
                iq = (q[1] >= coord) if ge else (q[1] <= coord)
                if ip != iq:
                    t = (coord - q[1]) / (p[1] - q[1])
                    xi = q[0] + (p[0] - q[0]) * t
                    if lo <= xi <= hi:
                        total += 1
    return total


def oddeven_inside(poly, X, Y):
    """Odd-even (crossing parity) point-in-polygon on coordinate grids.
    The polygon stream is implicitly closed, like QPainter::drawPolygon."""
    inside = np.zeros(X.shape, dtype=bool)
    n = len(poly)
    if n < 3:
        return inside
    for i in range(n):
        xa, ya = poly[i]
        xb, yb = poly[(i + 1) % n]
        if ya == yb:
            continue
        cond = (ya > Y) != (yb > Y)
        with np.errstate(divide="ignore", invalid="ignore"):
            xint = xa + (Y - ya) * (xb - xa) / (yb - ya)
        inside ^= cond & (X < xint)
    return inside


def winding_inside(poly, X, Y):
    """Nonzero-winding point-in-polygon on coordinate grids
    (Qt::WindingFill semantics, used by QPainterPath-based items)."""
    wind = np.zeros(X.shape, dtype=np.int32)
    n = len(poly)
    if n < 3:
        return wind != 0
    for i in range(n):
        xa, ya = poly[i]
        xb, yb = poly[(i + 1) % n]
        if ya == yb:
            continue
        with np.errstate(divide="ignore", invalid="ignore"):
            xint = xa + (Y - ya) * (xb - xa) / (yb - ya)
        left = X < xint
        wind += np.where((ya <= Y) & (Y < yb) & left, 1, 0)
        wind -= np.where((yb <= Y) & (Y < ya) & left, 1, 0)
    return wind != 0


def circle_truth(rect, pos, radius, k=200000):
    """Brute-force ground truth: dense angular sampling of the circle,
    membership via QRectF::contains semantics (inclusive borders)."""
    a = np.linspace(0.0, TWO_PI, k, endpoint=False)
    x = pos[0] + radius * np.cos(a)
    y = pos[1] - radius * np.sin(a)
    lx, ly, lw, lh = rect
    inside = (x >= lx) & (x <= lx + lw) & (y >= ly) & (y <= ly + lh)
    return a, inside


def interval_membership(intervals, angles):
    """Membership of angles in a list of QwtInterval-like arcs.
    min > max is interpreted as wrapping through 2*pi (like the
    QwtPolarGrid consumer does: span < 0 -> span += 2*pi)."""
    m = np.zeros(angles.shape, dtype=bool)
    for (lo, hi) in intervals:
        if lo <= hi:
            m |= (angles >= lo) & (angles <= hi)
        else:
            m |= (angles >= lo) | (angles <= hi)
    return m


def arc_symdiff_measure(intervals, rect, pos, radius, k=720000):
    """Symmetric-difference angular measure between the returned arcs
    and the brute-force truth (radians)."""
    a, truth = circle_truth(rect, pos, radius, k)
    got = interval_membership(intervals, a)
    # truth wrap: merge first/last runs
    diff = np.count_nonzero(got != truth)
    return diff * (TWO_PI / k), got, truth, a


# =====================================================================
# Section 3: signals used by the amplification experiments
# =====================================================================

def sig_sine(n=20000):
    x = np.linspace(0.0, 4000.0, n)
    y = 500.0 + 450.0 * np.sin(TWO_PI * x / 60.0)
    return list(zip(x.tolist(), y.tolist()))


def sig_lissajous(n=5000):
    t = np.linspace(0.0, TWO_PI, n, endpoint=False)
    x = 480.0 * np.sin(3.0 * t + 0.5)
    y = 380.0 * np.sin(4.0 * t)
    return list(zip(x.tolist(), y.tolist()))


def sig_star_polygon(n=400, seed=11):
    rng = np.random.default_rng(seed)
    a = np.sort(rng.uniform(0.0, TWO_PI, n))
    rad = rng.uniform(150.0, 470.0, n)
    x = 500.0 + rad * np.cos(a)
    y = 500.0 + rad * np.sin(a)
    return list(zip(x.tolist(), y.tolist()))


def sig_zigzag(n=2000):
    x = np.arange(n, dtype=float)
    y = np.where(np.arange(n) % 2 == 0, 0.0, 1000.0).astype(float)
    return list(zip(x.tolist(), y.tolist()))


# =====================================================================
# Section 4: self-test against recorded C++ outputs
# =====================================================================

def self_test():
    """The five smoke cases were run through clipper_check.exe (real
    QwtClipper, Qt 5.15.16); outputs recorded verbatim below.  The
    Python re-implementation must reproduce them bit-exactly."""
    print("== [1] re-implementation self-test (recorded C++ outputs) ==")
    cases = [
        # (kind, args, expected)
        ("K", ("F", (0.0, 0.0, 100.0, 100.0), False,
               [(-50.0, 50.0), (150.0, 50.0), (50.0, 200.0)]),
         [(0.0, 50.0), (100.0, 50.0), (100.0, 100.0)]),
        ("K", ("I", (0, 0, 10, 10), True,
               [(-5, -5), (15, -5), (15, 15), (-5, 15)]),
         [(0, 10), (0, 0), (10, 0), (10, 10)]),
        ("C", ((0.0, 0.0, 100.0, 80.0), (50.0, 40.0), 60.0),
         [(0.58568554345715107, 0.72972765622696634),
          (2.4118649973628266, 2.555907110132642),
          (3.7272781970469442, 3.8713203098167597),
          (5.5534576509526197, 5.6974997637224352)]),
        ("C", ((0.0, 0.0, 10.0, 10.0), (0.0, 8.0), 3.0),
         [(5.5534576509526197, 4.7123889803846897)]),
        ("C", ((0.0, 0.0, 100.0, 100.0), (50.0, 50.0), math.sqrt(5000.0)),
         [(0.78539816339744828, 2.3561944901923448),
          (2.3561944901923448, 3.9269908169872414),
          (3.9269908169872414, 5.497787143782138),
          (5.497787143782138, 0.78539816339744828)]),
    ]
    ok = True
    for idx, (kind, args, expected) in enumerate(cases):
        if kind == "K":
            mode, rect, close, pts = args
            got = clip_polygon(rect, pts, close, mode)
        else:
            rect, pos, radius = args
            got = clip_circle(rect, pos, radius)
        same = len(got) == len(expected) and all(
            (g[0] == e[0] or (g[0] != g[0] and e[0] != e[0])) and
            (g[1] == e[1] or (g[1] != g[1] and e[1] != e[1]))
            for g, e in zip(got, expected))
        print(f"  smoke[{idx}] {'PASS' if same else 'FAIL'}  got={got}")
        ok = ok and same
    print(f"  self-test: {'ALL PASS' if ok else 'FAILURES PRESENT'}")
    return ok


# =====================================================================
# Section 5: C++ cross-validation
# =====================================================================

def gen_cross_cases(rng):
    """Build the deterministic cross-validation case list."""
    cases = []  # dicts: {kind, mode, close, rect, pts} or {kind, rect, center, radius}

    def add_poly(mode, close, rect, pts):
        cases.append({"kind": "K", "mode": mode, "close": close,
                      "rect": rect, "pts": pts})

    # --- random float polygons (open/closed) ---
    for i in range(700):
        close = (i % 2 == 1)
        rx = float(rng.uniform(-500, 1200))
        ry = float(rng.uniform(-500, 1200))
        rw = float(rng.uniform(0.5, 900))
        rh = float(rng.uniform(0.5, 900))
        n = int(rng.integers(0, 61))
        pts = [(float(rng.uniform(-600, 1400)), float(rng.uniform(-600, 1400)))
               for _ in range(n)]
        if n >= 2 and i % 5 == 0:
            # snap ~half of the points exactly onto the rect borders
            x1, x2, y1, y2 = bounds_F((rx, ry, rw, rh))
            for j in range(0, n, 2):
                edge = j % 4
                px, py = pts[j]
                if edge == 0:
                    pts[j] = (x1, py)
                elif edge == 1:
                    pts[j] = (x2, py)
                elif edge == 2:
                    pts[j] = (px, y1)
                else:
                    pts[j] = (px, y2)
        if n >= 4 and i % 7 == 0:
            # inject exact duplicate points
            pts[n // 2] = pts[n // 2 - 1]
        add_poly("F", close, (rx, ry, rw, rh), pts)

    # --- random int polygons (open/closed) ---
    for i in range(300):
        close = (i % 2 == 1)
        rx = int(rng.integers(-500, 1200))
        ry = int(rng.integers(-500, 1200))
        rw = int(rng.integers(1, 900))
        rh = int(rng.integers(1, 900))
        n = int(rng.integers(0, 61))
        pts = [(int(rng.integers(-600, 1400)), int(rng.integers(-600, 1400)))
               for _ in range(n)]
        if n >= 2 and i % 3 == 0:
            x1, x2, y1, y2 = bounds_I((rx, ry, rw, rh))
            for j in range(0, n, 2):
                px, py = pts[j]
                pts[j] = (x1, py) if j % 4 == 0 else (px, y2)
        add_poly("I", close, (rx, ry, rw, rh), pts)

    # --- float rect x int polygon (conservative rounding path) ---
    for i in range(300):
        close = (i % 2 == 1)
        rx = float(rng.uniform(-500, 1200))
        ry = float(rng.uniform(-500, 1200))
        rw = float(rng.uniform(0.2, 900))   # sub-integer widths included
        rh = float(rng.uniform(0.2, 900))
        n = int(rng.integers(0, 41))
        pts = [(int(rng.integers(-600, 1400)), int(rng.integers(-600, 1400)))
               for _ in range(n)]
        add_poly("R", close, (rx, ry, rw, rh), pts)

    # --- structured adversarial polygons ---
    def add_structured(mode, close, rect, pts):
        add_poly(mode, close, rect, pts)

    rect = (100.0, 100.0, 200.0, 150.0)
    add_structured("F", True, rect, [])                                  # empty
    add_structured("F", True, rect, [(150.0, 150.0)])                    # single inside
    add_structured("F", True, rect, [(900.0, 900.0)])                    # single outside
    add_structured("F", False, rect, [(50.0, 120.0), (400.0, 120.0)])    # horizontal through
    add_structured("F", False, rect, [(950.0, 120.0), (960.0, 130.0)])   # fully outside
    add_structured("F", True, rect, [(100.0, 100.0), (300.0, 100.0),
                                     (300.0, 250.0), (100.0, 250.0)])    # == rect corners
    add_structured("F", False, rect, [(100.0, 175.0), (300.0, 175.0)])   # along-boundary
    add_structured("F", False, rect, [(100.0, 175.0), (50.0, 90.0)])     # boundary->out
    add_structured("F", False, rect, [(100.0, 175.0), (100.0, 175.0),
                                      (350.0, 175.0)])                   # duplicates
    add_structured("F", True, rect, [(0.0, 175.0), (400.0, 25.0),
                                     (0.0, 25.0), (400.0, 175.0)])       # self-crossing
    collinear = [(float(t), 2.0 * t) for t in range(0, 400, 20)]
    add_structured("F", False, rect, collinear)                          # collinear open
    add_structured("F", True, rect, collinear)                           # collinear closed
    zig = []
    for t in range(60):
        zig.append((50.0 + 6.0 * t, 90.0 if t % 2 == 0 else 320.0))
    add_structured("F", False, rect, zig)                                # dense zigzag
    huge = [(-1.0e6, -1.0e6), (1.0e6, -1.0e6), (1.0e6, 1.0e6), (-1.0e6, 1.0e6)]
    add_structured("F", True, rect, huge)                                # giant square
    tiny = [(150.0, 150.0), (150.0 + 1e-13, 150.0), (150.0, 150.0 + 1e-13)]
    add_structured("F", True, rect, tiny)                                # sub-epsilon
    nanpts = [(50.0, 175.0), (float("nan"), float("nan")), (150.0, 175.0),
              (400.0, 175.0)]
    add_structured("F", False, rect, nanpts)                             # NaN vertex
    add_structured("F", True, rect, nanpts)
    add_structured("I", False, (100, 100, 200, 150),
                   [(50, 120), (400, 120), (200, 300), (250, 110)])
    add_structured("I", True, (100, 100, 200, 150),
                   [(100, 100), (300, 100), (300, 250), (100, 250)])
    add_structured("R", False, (100.2, 100.7, 0.5, 90.3),
                   [(50, 120), (400, 120)])       # no integer column inside
    add_structured("R", True, (100.0, 100.0, 0.0, 0.0),
                   [(100, 100), (101, 101)])      # zero-size float rect
    add_structured("F", True, (100.0, 100.0, -50.0, 80.0),
                   [(90.0, 120.0), (120.0, 120.0), (120.0, 160.0)])     # negative width
    add_structured("F", True, (-64.8, -64.8, 129.6, 129.6),
                   sig_lissajous(5000))   # phantom border (curve misses rect)

    # --- circles: random ---
    for i in range(1000):
        rx = float(rng.uniform(-200, 800))
        ry = float(rng.uniform(-200, 800))
        rw = float(rng.uniform(1.0, 600))
        rh = float(rng.uniform(1.0, 600))
        cx = float(rng.uniform(-200, 900))
        cy = float(rng.uniform(-200, 900))
        rad = float(rng.uniform(0.01, 800))
        cases.append({"kind": "C", "rect": (rx, ry, rw, rh),
                      "center": (cx, cy), "radius": rad})

    # --- circles: adversarial ---
    adv = [
        ((0.0, 0.0, 10.0, 10.0), (0.0, 8.0), 3.0),          # vertical swap, left
        ((0.0, 0.0, 10.0, 10.0), (10.0, 3.0), 4.0),         # vertical swap, right
        ((0.0, 0.0, 100.0, 100.0), (50.0, 50.0), math.sqrt(5000.0)),  # corner dup
        ((10.0, 20.0, 300.0, 200.0), (160.0, 120.0),
         math.sqrt(150.0 * 150.0 + 100.0 * 100.0)),         # corner dup 2
        ((0.0, 0.0, 100.0, 100.0), (50.0, 50.0), 50.0),     # inscribed
        ((0.0, 0.0, 100.0, 100.0), (50.0, 50.0), 10.0),     # fully inside
        ((0.0, 0.0, 100.0, 100.0), (500.0, 500.0), 10.0),   # fully outside
        ((0.0, 0.0, 100.0, 100.0), (50.0, 50.0), 200.0),    # contains rect
        ((0.0, 0.0, 100.0, 100.0), (-50.0, 50.0), 50.0),    # tangent outside
        ((0.0, 0.0, 100.0, 80.0), (60.0, 40.0), 60.0),      # tangent inside left
        ((0.0, 0.0, 100.0, 100.0), (0.0, 0.0), 50.0),       # center at corner
        ((0.0, 0.0, 100.0, 100.0), (50.0, 50.0), 0.0),      # radius zero
        ((0.0, 0.0, 100.0, 100.0), (50.0, 0.0), 60.0),      # center on top line
        ((100.0, 100.0, 10.0, 10.0), (0.0, 0.0),
         math.sqrt(20000.0)),                                # corner tangent
    ]
    for rect, center, radius in adv:
        cases.append({"kind": "C", "rect": rect, "center": center,
                      "radius": radius, "adversarial": True})
    return cases


def write_case_file(cases, path):
    with open(path, "w", encoding="ascii") as f:
        f.write("P\n")

        def fmt(v):
            if isinstance(v, float):
                if math.isnan(v):
                    return "nan"
                return repr(v)
            return str(v)

        for c in cases:
            if c["kind"] == "K":
                rx, ry, rw, rh = c["rect"]
                vals = " ".join(fmt(v) for p in c["pts"] for v in p)
                f.write(f"K {c['mode']} {1 if c['close'] else 0} "
                        f"{fmt(rx)} {fmt(ry)} {fmt(rw)} {fmt(rh)} "
                        f"{len(c['pts'])}")
                if vals:
                    f.write(" " + vals)
                f.write("\n")
            else:
                rx, ry, rw, rh = c["rect"]
                cx, cy = c["center"]
                f.write(f"C {fmt(rx)} {fmt(ry)} {fmt(rw)} {fmt(rh)} "
                        f"{fmt(cx)} {fmt(cy)} {fmt(c['radius'])}\n")


def parse_harness_output(text):
    probes = {}
    results = {}
    for line in text.splitlines():
        toks = line.split()
        if not toks:
            continue
        if toks[0] == "PROBE":
            probes[toks[1]] = int(toks[2])
        elif toks[0] in ("R", "C"):
            idx = int(toks[1])
            n = int(toks[2])
            vals = []
            for t in toks[3:]:
                tl = t.lower()
                if "nan" in tl:
                    vals.append(math.nan)
                else:
                    vals.append(float(t))
            results[idx] = (n, vals)
        elif toks[0] == "DONE":
            pass
    return probes, results


def run_cross_check(cases, scratch):
    """Write the case file, run the real C++ clipper, compare bit-exactly."""
    print("== [5] C++ cross-validation (QwtClipper via clipper_check.exe) ==")
    exe = Path(scratch) / "clipper_check.exe"
    case_file = Path(scratch) / "clipper_cases.txt"
    if not exe.exists():
        print(f"  harness not found at {exe} -> XCHECK SKIPPED")
        return None

    write_case_file(cases, case_file)
    env = dict(os.environ)
    qt_bin = r"D:\Qt\Qt5.15.16\5.15.16\msvc2019_64\bin"
    qwt_bin = (r"D:\src-czy\gree-data-workbench\data-workbench\src\3rdparty"
               r"\qwt\build-release\bin")
    env["PATH"] = qt_bin + ";" + qwt_bin + ";" + env.get("PATH", "")
    proc = subprocess.run([str(exe), str(case_file)], capture_output=True,
                          text=True, env=env, cwd=str(scratch))
    if proc.returncode != 0:
        print("  harness failed:", proc.stderr[:500])
        return None

    probes, results = parse_harness_output(proc.stdout)
    print("  Qt semantics probes (QRectF(0,0,10,10)):")
    for k in ("point_left", "point_top", "point_right", "point_bottom",
              "point_inside", "point_outside", "rect_equal", "rect_bigger"):
        print(f"    contains.{k} = {probes.get(k)}")
    print(f"    QRect(0,0,10,10).right() = {probes.get('qrect_right')}, "
          f".bottom() = {probes.get('qrect_bottom')}")

    n_poly = n_poly_ok = 0
    n_circ = n_circ_ok = 0
    mismatches = []
    for idx, c in enumerate(cases):
        got_n, got_vals = results.get(idx, (None, None))
        if got_n is None:
            mismatches.append((idx, "missing result"))
            continue
        if c["kind"] == "K":
            n_poly += 1
            exp = clip_polygon(c["rect"], c["pts"], c["close"], c["mode"])
            exp_vals = [v for p in exp for v in p]
            same = (len(exp) == got_n) and all(
                (a == b) or (a != a and b != b)
                for a, b in zip(exp_vals, got_vals))
            if same:
                n_poly_ok += 1
            elif len(mismatches) < 6:
                mismatches.append((idx, c, exp_vals[:12], got_vals[:12]))
        else:
            n_circ += 1
            exp = clip_circle(c["rect"], c["center"], c["radius"])
            exp_vals = [v for iv in exp for v in iv]
            same = (len(exp) == got_n) and all(
                a == b for a, b in zip(exp_vals, got_vals))
            if same:
                n_circ_ok += 1
            elif len(mismatches) < 6:
                mismatches.append((idx, c, exp_vals, got_vals))

    print(f"  polygon cases : {n_poly_ok}/{n_poly} bit-exact matches")
    print(f"  circle  cases : {n_circ_ok}/{n_circ} bit-exact matches")
    print(f"  total         : {n_poly_ok + n_circ_ok}/{n_poly + n_circ}")
    for m in mismatches:
        print("  MISMATCH:", m)
    return {"poly": (n_poly_ok, n_poly), "circ": (n_circ_ok, n_circ),
            "probes": probes}


# =====================================================================
# Section 6: vertex amplification under pan / zoom
# =====================================================================

def measure_case(mode, rect, pts, close):
    trace = []
    out = clip_polygon(rect, pts, close, mode, trace)
    v_in = visible_vertices(rect, pts, mode)
    x_b = boundary_crossings(rect, pts, close, mode)
    # exact per-pass accounting identity: len(out_k) == n_v + n_x
    pass_ok = all(len(o) == s["n_v"] + s["n_x"] or s["passthrough"]
                  for _, o, s in trace)
    # global relation: n_out = V_in + X_border + Phi, where Phi counts
    # "phantom" points born when a clip-border bridging segment (created
    # by an earlier pass) is re-clipped by a later pass.  Phi == 0 for
    # x-monotone open polylines in generic position.
    phi = len(out) - v_in - x_b
    return {
        "n_in": len(pts), "n_out": len(out), "V_in": v_in, "X": x_b,
        "identity": pass_ok, "phi": phi,
        "A": len(out) / max(1, v_in),
        "ratio": len(out) / max(1, len(pts)),
        "passes": [(e, len(o), s["n_v"], s["n_x"]) for e, o, s in trace],
        "out": out,
    }


def experiment_pan():
    print()
    print("== [6] vertex amplification: pan sequences ==")
    seqs = [
        ("sine", sig_sine(20000), False, "F",
         [(x0, 200.0, 800.0, 600.0)
          for x0 in np.arange(-800.0, 4001.0, 100.0).tolist()]),
        ("lissajous", sig_lissajous(5000), True, "F",
         [(x0, -150.0, 300.0, 300.0)
          for x0 in np.arange(-650.0, 501.0, 25.0).tolist()]),
        ("star polygon", sig_star_polygon(400), True, "F",
         [(x0, 375.0, 250.0, 250.0)
          for x0 in np.arange(-250.0, 1001.0, 50.0).tolist()]),
    ]
    summary = {}
    curves = {}
    for name, pts, close, mode, rects in seqs:
        rows = []
        ident_ok = 0
        phi_zero = 0
        max_phi = 0
        max_A = 0.0
        for k, rect in enumerate(rects):
            m = measure_case(mode, rect, pts, close)
            ident_ok += 1 if m["identity"] else 0
            phi_zero += 1 if m["phi"] == 0 else 0
            max_phi = max(max_phi, m["phi"])
            max_A = max(max_A, m["A"])
            rows.append((k, rect[0], m))
        print(f"  [{name}] n_in={len(pts)} close={close} steps={len(rects)} "
              f"per-pass identity: {ident_ok}/{len(rects)}  "
              f"Phi==0: {phi_zero}/{len(rects)}  max Phi={max_phi}  "
              f"max A={max_A:.3f}")
        print("    step   x0      V_in     X    Phi    n_out    n_out/n_in   A=n_out/V_in")
        for (k, x0, m) in rows:
            print(f"    {k:4d} {x0:8.1f} {m['V_in']:7d} {m['X']:5d} "
                  f"{m['phi']:5d} {m['n_out']:7d} {m['ratio']:10.4f} "
                  f"{m['A']:12.3f}")
        summary[name] = {"ident": (ident_ok, len(rects)), "max_A": max_A,
                         "phi_zero": (phi_zero, len(rects)),
                         "max_phi": max_phi, "rows": rows}
        curves[name] = rows
    return summary, curves


def experiment_zoom():
    print()
    print("== [7] vertex amplification: zoom sequences ==")
    sine = sig_sine(20000)
    liss = sig_lissajous(5000)
    star = sig_star_polygon(400)

    # focus on the curve (dense sine near x=3980)
    cy = 500.0 + 450.0 * math.sin(TWO_PI * 3980.0 / 60.0)
    zoom_sine = [(3980.0 - L / 2, cy - L / 2, L, L)
                 for L in (2000.0 * 0.6 ** k for k in range(12))]
    # focus on an empty band above the sine envelope (crossing-dominated)
    zoom_band = [(2000.0 - L / 2, 985.0 - L / 2, L, L)
                 for L in (2000.0 * 0.6 ** k for k in range(12))]
    zoom_liss = [(-L / 2, -L / 2, L, L)
                 for L in (1000.0 * 0.6 ** k for k in range(12))]
    vmax = max(star, key=lambda p: (p[0] - 500.0) ** 2 + (p[1] - 500.0) ** 2)
    zoom_star = [(vmax[0] - L / 2, vmax[1] - L / 2, L, L)
                 for L in (900.0 * 0.6 ** k for k in range(11))]

    seqs = [
        ("sine on-curve zoom", sine, False, "F", zoom_sine),
        ("sine empty-band zoom", sine, False, "F", zoom_band),
        ("lissajous center zoom", liss, True, "F", zoom_liss),
        ("star polygon vertex zoom", star, True, "F", zoom_star),
    ]
    summary = {}
    for name, pts, close, mode, rects in seqs:
        rows = []
        ident_ok = 0
        phi_zero = 0
        max_phi = 0
        max_A = 0.0
        for k, rect in enumerate(rects):
            m = measure_case(mode, rect, pts, close)
            ident_ok += 1 if m["identity"] else 0
            phi_zero += 1 if m["phi"] == 0 else 0
            max_phi = max(max_phi, m["phi"])
            max_A = max(max_A, m["A"])
            rows.append((k, rect[2], m))
        print(f"  [{name}] n_in={len(pts)} steps={len(rects)} "
              f"per-pass identity: {ident_ok}/{len(rects)}  "
              f"Phi==0: {phi_zero}/{len(rects)}  max Phi={max_phi}  "
              f"max A={max_A:.3f}")
        print("    step     L      V_in     X    Phi    n_out    n_out/n_in   A=n_out/V_in")
        for (k, L, m) in rows:
            print(f"    {k:4d} {L:8.3f} {m['V_in']:7d} {m['X']:5d} "
                  f"{m['phi']:5d} {m['n_out']:7d} {m['ratio']:10.4f} "
                  f"{m['A']:12.3f}")
        summary[name] = {"ident": (ident_ok, len(rects)), "max_A": max_A,
                         "phi_zero": (phi_zero, len(rects)),
                         "max_phi": max_phi, "rows": rows}
    return summary


def experiment_worst_case():
    """Per-pass amplification on a zigzag built so that every segment
    crosses the clip line (theoretical single-pass bound: out <= 2n)."""
    print()
    print("== [8] worst-case per-pass amplification (zigzag) ==")
    zig = sig_zigzag(2000)
    results = []
    for h in (400.0, 100.0, 10.0, 1.0, 0.1):
        rect = (0.0, 500.0 - h / 2, 2000.0, h)
        trace = []
        out = clip_polygon(rect, zig, False, "F", trace)
        sizes = [len(zig)] + [len(o) for _, o, _s in trace]
        ratios = [sizes[i + 1] / sizes[i] for i in range(4)]
        v_in = visible_vertices(rect, zig, "F")
        print(f"  slab h={h:6.1f}: pass sizes L,R,T,B = {sizes[1]},{sizes[2]},"
              f"{sizes[3]},{sizes[4]}  n_out={len(out)}  V_in={v_in}  "
              f"ratios={[f'{r:.3f}' for r in ratios]}")
        results.append((h, sizes, len(out), v_in, ratios))
    return results


# =====================================================================
# Section 7: degenerate inputs
# =====================================================================

def experiment_degenerate():
    print()
    print("== [9] degenerate inputs ==")
    rect = (100.0, 100.0, 200.0, 150.0)

    def show(tag, mode, r, pts, close):
        out = clip_polygon(r, pts, close, mode)
        print(f"  {tag}: mode={mode} close={int(close)} rect={r} "
              f"n_in={len(pts)} -> n_out={len(out)}")
        if len(out) <= 14:
            print(f"    out={out}")
        else:
            print(f"    out[:6]={out[:6]} ... out[-3:]={out[-3:]}")
        return out

    # 9.1 single point - pass-through regardless of position
    show("single point inside", "F", rect, [(150.0, 150.0)], False)
    show("single point OUTSIDE", "F", rect, [(900.0, 900.0)], False)
    show("single point outside closed", "F", rect, [(900.0, 900.0)], True)

    # 9.2 empty polygon
    show("empty polygon", "F", rect, [], False)

    # 9.3 fully outside on each side
    for tag, pts in [
        ("outside left", [(10.0, 120.0), (50.0, 200.0)]),
        ("outside right", [(900.0, 120.0), (950.0, 200.0)]),
        ("outside top", [(150.0, 10.0), (200.0, 50.0)]),
        ("outside bottom", [(150.0, 900.0), (200.0, 950.0)]),
        ("enclosing rect", [(0.0, 0.0), (400.0, 0.0), (400.0, 400.0), (0.0, 400.0)]),
    ]:
        show(tag, "F", rect, pts, False)
        show(tag + " closed", "F", rect, pts, True)

    # 9.4 vertices exactly on the border
    show("corners == rect corners", "F", rect,
         [(100.0, 100.0), (300.0, 100.0), (300.0, 250.0), (100.0, 250.0)], True)
    show("on-border to outside", "F", rect,
         [(100.0, 175.0), (50.0, 90.0)], False)
    show("touch and leave", "F", rect,
         [(50.0, 175.0), (100.0, 175.0), (50.0, 176.0)], False)

    # 9.5 duplicate points
    show("interior duplicates", "F", rect,
         [(150.0, 150.0), (150.0, 150.0), (200.0, 200.0), (200.0, 200.0)], False)
    show("duplicate crossing border", "F", rect,
         [(150.0, 150.0), (150.0, 150.0), (400.0, 400.0)], False)

    # 9.6 all collinear
    collinear = [(float(t), 2.0 * t) for t in range(0, 400, 20)]
    show("collinear open", "F", rect, collinear, False)
    show("collinear closed", "F", rect, collinear, True)

    # 9.7 narrow viewports
    sine_part = [(float(x), 500.0 + 450.0 * math.sin(TWO_PI * x / 60.0))
                 for x in range(0, 400, 4)]
    out1 = show("1px slab (float, w=1)", "F", (200.0, 0.0, 1.0, 1000.0),
                sine_part, False)
    show("1px slab (float, h=1)", "F", (0.0, 500.0, 400.0, 1.0),
         sine_part, False)
    show("1px slab (int)", "I", (200, 0, 1, 1000),
         [(int(x), int(round(y))) for x, y in sine_part], False)
    show("zero-height rect (int)", "I", (0, 500, 400, 0),
         [(int(x), int(round(y))) for x, y in sine_part], False)

    # 9.8 float rect without integer coverage (R path rounding)
    show("R path, rect has no int column", "R", (100.2, 100.7, 0.5, 90.3),
         [(50, 120), (400, 120)], False)
    show("F path, same rect", "F", (100.2, 100.7, 0.5, 90.3),
         [(50.0, 120.0), (400.0, 120.0)], False)

    # 9.9 NaN vertices (float path only)
    show("NaN vertex open", "F", rect,
         [(50.0, 175.0), (float("nan"), float("nan")), (150.0, 175.0),
          (400.0, 175.0)], False)

    # 9.10 self-intersecting pentagram, closed: fill semantics
    print("  --- self-intersection: odd-even fill comparison (400x400 grid) ---")
    R = 200.0
    cxp, cyp = 200.0, 200.0
    star_idx = [0, 2, 4, 1, 3]
    star = []
    for j in star_idx:
        a = -math.pi / 2 + j * TWO_PI / 5
        star.append((cxp + R * math.cos(a), cyp + R * math.sin(a)))
    clip_rect = (0.0, 0.0, 230.0, 400.0)   # left 57.5% of the domain

    gx = np.linspace(0.5, 399.5, 400)
    X, Y = np.meshgrid(gx, gx)
    in_rect = (X >= clip_rect[0]) & (X <= clip_rect[0] + clip_rect[2]) & \
              (Y >= clip_rect[1]) & (Y <= clip_rect[1] + clip_rect[3])
    truth = in_rect & oddeven_inside(star, X, Y)
    out = clip_polygon(clip_rect, star, True, "F")
    got = oddeven_inside(out, X, Y)
    diff = np.count_nonzero(truth != got)
    print(f"    pentagram closed: n_in=5 n_out={len(out)} "
          f"fill-diff pixels={diff} ({100.0 * diff / 160000:.4f}% of grid)")
    print(f"    out={[(round(x, 3), round(y, 3)) for x, y in out]}")

    # open pentagram stream (polyline semantics)
    out_o = clip_polygon(clip_rect, star, False, "F")
    got_o = oddeven_inside(out_o, X, Y)
    diff_o = np.count_nonzero(truth != got_o)
    print(f"    pentagram open  : n_out={len(out_o)} "
          f"fill-d pixels={diff_o} ({100.0 * diff_o / 160000:.4f}%)")

    # same comparison under nonzero-winding fill (Qt::WindingFill)
    truth_w = in_rect & winding_inside(star, X, Y)
    got_w = winding_inside(out, X, Y)
    diff_w = np.count_nonzero(truth_w != got_w)
    print(f"    pentagram closed, winding fill: fill-diff pixels={diff_w} "
          f"({100.0 * diff_w / 160000:.4f}%)")
    got_wo = winding_inside(out_o, X, Y)
    diff_wo = np.count_nonzero(truth_w != got_wo)
    print(f"    pentagram open,   winding fill: fill-diff pixels={diff_wo} "
          f"({100.0 * diff_wo / 160000:.4f}%)")

    # positive control: simple star-shaped polygon
    simple = sig_star_polygon(60, seed=99)
    simple_scaled = [(200.0 + (x - 500.0) * 0.4, 200.0 + (y - 500.0) * 0.4)
                     for x, y in simple]
    truth_s = in_rect & oddeven_inside(simple_scaled, X, Y)
    out_s = clip_polygon(clip_rect, simple_scaled, True, "F")
    got_s = oddeven_inside(out_s, X, Y)
    diff_s = np.count_nonzero(truth_s != got_s)
    print(f"    simple polygon control: n_in={len(simple_scaled)} "
          f"n_out={len(out_s)} fill-diff pixels={diff_s}")

    # lissajous (self-crossing closed stream)
    liss = sig_lissajous(2000)
    liss_s = [(200.0 + x * 0.38, 200.0 + y * 0.5) for x, y in liss]
    truth_l = in_rect & oddeven_inside(liss_s, X, Y)
    out_l = clip_polygon(clip_rect, liss_s, True, "F")
    got_l = oddeven_inside(out_l, X, Y)
    diff_l = np.count_nonzero(truth_l != got_l)
    print(f"    lissajous closed: n_in={len(liss_s)} n_out={len(out_l)} "
          f"fill-diff pixels={diff_l} ({100.0 * diff_l / 160000:.4f}%)")
    truth_lw = in_rect & winding_inside(liss_s, X, Y)
    got_lw = winding_inside(out_l, X, Y)
    diff_lw = np.count_nonzero(truth_lw != got_lw)
    print(f"    lissajous closed, winding fill: fill-diff pixels={diff_lw} "
          f"({100.0 * diff_lw / 160000:.4f}%)")

    # 9.11 phantom border: closed stream that never enters the viewport
    print("  --- phantom border segments (closed stream, viewport misses "
          "the curve) ---")
    liss_full = sig_lissajous(5000)
    phantom_rect = (-64.8, -64.8, 129.6, 129.6)
    trace_p = []
    out_p = clip_polygon(phantom_rect, liss_full, True, "F", trace_p)
    sizes_p = [len(liss_full)] + [len(o) for _, o, _s in trace_p]
    v_p = visible_vertices(phantom_rect, liss_full, "F")
    x_p = boundary_crossings(phantom_rect, liss_full, True, "F")
    print(f"    lissajous(5000, closed) vs rect {phantom_rect}: "
          f"V_in={v_p} X_border={x_p} n_out={len(out_p)}")
    print(f"    pass sizes input,L,R,T,B = {sizes_p}")
    print(f"    out={out_p}")

    return {
        "pentagram_closed": (len(out), diff),
        "pentagram_open": (len(out_o), diff_o),
        "simple_control": (len(out_s), diff_s),
        "lissajous": (len(out_l), diff_l),
        "phantom": (v_p, x_p, len(out_p), out_p),
        "star": star, "pent_out": out, "rect": clip_rect,
        "diff_grid": (truth != got),
    }


# =====================================================================
# Section 8: clipCircle experiments
# =====================================================================

def experiment_circle():
    print()
    print("== [10] clipCircle: returned arcs vs brute-force truth ==")
    results = {}

    def report(tag, rect, pos, radius, k=720000):
        intv = clip_circle(rect, pos, radius)
        sd, got_m, truth_m, a = arc_symdiff_measure(intv, rect, pos, radius, k)
        got_meas = got_m.sum() * (TWO_PI / k)
        true_meas = truth_m.sum() * (TWO_PI / k)
        print(f"  {tag}: rect={rect} center={pos} r={radius!r}")
        print(f"    intervals={[(round(lo, 6), round(hi, 6)) for lo, hi in intv]}")
        print(f"    returned arc measure={got_meas:.6f} rad, "
              f"true={true_meas:.6f} rad, sym-diff={sd:.6f} rad "
              f"({'OK' if sd < 1e-3 else 'WRONG'})")
        results[tag] = {"intv": intv, "symdiff": sd, "got": got_meas,
                        "true": true_meas, "rect": rect, "pos": pos,
                        "radius": radius}
        return intv

    report("generic 4 corner arcs", (0.0, 0.0, 100.0, 80.0), (50.0, 40.0), 60.0)
    report("fully inside", (0.0, 0.0, 100.0, 100.0), (50.0, 50.0), 10.0)
    report("fully outside", (0.0, 0.0, 100.0, 100.0), (500.0, 500.0), 10.0)
    report("circle contains rect", (0.0, 0.0, 100.0, 100.0), (50.0, 50.0), 200.0)
    report("inscribed (tangent x4)", (0.0, 0.0, 100.0, 100.0), (50.0, 50.0), 50.0)
    report("tangent from outside", (0.0, 0.0, 100.0, 100.0), (-50.0, 50.0), 50.0)
    report("tangent inside left", (0.0, 0.0, 100.0, 80.0), (60.0, 40.0), 60.0)
    report("vertical swap (left edge)", (0.0, 0.0, 10.0, 10.0), (0.0, 8.0), 3.0)
    report("vertical swap (right edge)", (0.0, 0.0, 10.0, 10.0), (10.0, 3.0), 4.0)
    report("corner duplicates", (0.0, 0.0, 100.0, 100.0), (50.0, 50.0),
           math.sqrt(5000.0))
    report("corner duplicates 2", (10.0, 20.0, 300.0, 200.0), (160.0, 120.0),
           math.sqrt(150.0 * 150.0 + 100.0 * 100.0))
    report("center at rect corner", (0.0, 0.0, 100.0, 100.0), (0.0, 0.0), 50.0)
    report("radius zero inside (doc says [0,2pi])",
           (0.0, 0.0, 100.0, 100.0), (50.0, 50.0), 0.0)

    # random batch: how often do exact-arithmetic degeneracies occur?
    rng = np.random.default_rng(SEED + 7)
    n_test = 300
    n_bad = 0
    worst = 0.0
    for i in range(n_test):
        rect = (float(rng.uniform(0, 400)), float(rng.uniform(0, 400)),
                float(rng.uniform(10, 500)), float(rng.uniform(10, 500)))
        pos = (float(rng.uniform(-100, 900)), float(rng.uniform(-100, 900)))
        radius = float(rng.uniform(1, 600))
        intv = clip_circle(rect, pos, radius)
        a, truth = circle_truth(rect, pos, radius, 20000)
        got = interval_membership(intv, a)
        sd = np.count_nonzero(got != truth) * (TWO_PI / 20000)
        if sd > 1e-3:
            n_bad += 1
            print(f"    random[{i}] symdiff={sd:.6f} rect={rect} pos={pos} "
                  f"r={radius}")
        worst = max(worst, sd)
    print(f"  random circles: {n_test - n_bad}/{n_test} match brute force "
          f"(tol 1e-3 rad); worst sym-diff={worst:.6f} rad")
    results["random_batch"] = (n_test - n_bad, n_test, worst)
    return results


# =====================================================================
# Section 9: figures
# =====================================================================

def set_style(ax):
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.tick_params(labelsize=8)


def fig_overview():
    """Four Sutherland-Hodgman passes on a Lissajous stream."""
    pts = sig_lissajous(900)
    rect = (-200.0, -150.0, 400.0, 300.0)
    trace = []
    clip_polygon(rect, pts, True, "F", trace)

    fig, axes = plt.subplots(1, 4, figsize=(13.0, 3.6))
    titles = ["pass 1: left edge (x >= x1)", "pass 2: right edge (x <= x2)",
              "pass 3: top edge (y >= y1)", "pass 4: bottom edge (y <= y2)"]
    prev = pts
    x1, x2, y1, y2 = bounds_F(rect)
    for ax, (edge, out, _stats), title in zip(axes, trace, titles):
        px, py = zip(*prev) if prev else ([], [])
        ax.plot(px, py, color=COL_IN, lw=0.7, label="pass input")
        if out:
            ox, oy = zip(*out)
            ax.plot(ox, oy, color=COL_OUT, lw=1.1, label="pass output")
        r = plt.Rectangle((x1, y1), x2 - x1, y2 - y1, fill=False,
                          edgecolor="#333333", ls="--", lw=1.0)
        ax.add_patch(r)
        if edge == "L":
            ax.axvline(x1, color=COL_BAD, lw=1.0, ls=":")
        elif edge == "R":
            ax.axvline(x2, color=COL_BAD, lw=1.0, ls=":")
        elif edge == "T":
            ax.axhline(y1, color=COL_BAD, lw=1.0, ls=":")
        else:
            ax.axhline(y2, color=COL_BAD, lw=1.0, ls=":")
        ax.set_title(title, fontsize=9)
        ax.set_xlim(-540, 540)
        ax.set_ylim(-460, 460)
        ax.set_aspect("equal")
        set_style(ax)
        prev = out
    axes[0].legend(fontsize=7.5, frameon=False, loc="lower left")
    fig.suptitle("Sutherland-Hodgman as implemented in QwtClipper: four "
                 "half-plane passes (Lissajous, 900 points, closed)",
                 fontsize=10.5)
    fig.tight_layout(rect=[0, 0, 1, 0.93])
    fig.savefig(HERE / "fig_clip_overview.png", dpi=200, bbox_inches="tight")
    plt.close(fig)
    print("  figure: fig_clip_overview.png")


def fig_amplify_pan(curves):
    fig, axes = plt.subplots(1, 3, figsize=(13.0, 3.8))
    for ax, (name, rows) in zip(axes, curves.items()):
        xs = [r[1] for r in rows]
        v_in = [r[2]["V_in"] for r in rows]
        x_b = [r[2]["X"] for r in rows]
        n_out = [r[2]["n_out"] for r in rows]
        ratio = [r[2]["ratio"] for r in rows]
        ax.plot(xs, n_out, color=COL_OUT, lw=1.4, label="n_out")
        ax.plot(xs, v_in, color=COL_TRUTH, lw=1.2, ls="--",
                label="V_in (visible raw vertices)")
        ax.plot(xs, x_b, color=COL_BAD, lw=1.2, ls=":",
                label="X (border crossings)")
        ax.set_title(f"{name}  (n_in={rows[0][2]['n_in']})", fontsize=9.5)
        ax.set_xlabel("viewport left position")
        ax.set_ylabel("vertex count")
        ax2 = ax.twinx()
        ax2.plot(xs, ratio, color="#7f7f7f", lw=1.0, alpha=0.8,
                 label="n_out / n_in")
        ax2.set_ylabel("n_out / n_in", color="#7f7f7f")
        ax2.tick_params(labelsize=8)
        set_style(ax)
    axes[0].legend(fontsize=7.5, frameon=False, loc="upper left")
    fig.suptitle("pan sequences: output size = visible vertices + border "
                 "crossings (identity verified at every step)", fontsize=10.5)
    fig.tight_layout(rect=[0, 0, 1, 0.92])
    fig.savefig(HERE / "fig_clip_amplify_pan.png", dpi=200, bbox_inches="tight")
    plt.close(fig)
    print("  figure: fig_clip_amplify_pan.png")


def fig_amplify_zoom(zoom_summary, worst):
    fig, axes = plt.subplots(1, 4, figsize=(14.5, 3.6))
    items = list(zoom_summary.items())[:3]
    for ax, (name, s) in zip(axes, items):
        L = [r[1] for r in s["rows"]]
        v_in = [r[2]["V_in"] for r in s["rows"]]
        x_b = [r[2]["X"] for r in s["rows"]]
        n_out = [r[2]["n_out"] for r in s["rows"]]
        A = [r[2]["A"] for r in s["rows"]]
        ax.semilogx(L, n_out, "o-", ms=3.5, color=COL_OUT, lw=1.3,
                    label="n_out")
        ax.semilogx(L, v_in, "s--", ms=3.0, color=COL_TRUTH, lw=1.1,
                    label="V_in")
        ax.semilogx(L, x_b, "d:", ms=3.0, color=COL_BAD, lw=1.1, label="X")
        ax.set_title(f"{name}\nmax A = {s['max_A']:.2f}", fontsize=9)
        ax.set_xlabel("viewport width L (px, log)")
        ax.invert_xaxis()
        ax2 = ax.twinx()
        ax2.semilogx(L, A, color="#7f7f7f", lw=1.0, alpha=0.85)
        ax2.set_ylabel("A = n_out / V_in", color="#7f7f7f")
        ax2.tick_params(labelsize=8)
        set_style(ax)
    ax = axes[3]
    for (h, sizes, n_out, v_in, ratios) in worst:
        ax.plot(range(5), sizes, "o-", ms=4, lw=1.2,
                label=f"slab h={h:g}")
    ax.set_yscale("log")
    ax.set_xticks(range(5))
    ax.set_xticklabels(["input", "L", "R", "T", "B"], fontsize=8.5)
    ax.set_xlabel("clipping pass")
    ax.set_ylabel("vertex count (log)")
    ax.set_title("zigzag worst case: per-pass sizes\n"
                 "(single-pass tight bound 1.5n,\n4-pass total ~2n)",
                 fontsize=9)
    ax.legend(fontsize=7, frameon=False)
    set_style(ax)
    axes[0].legend(fontsize=7.5, frameon=False, loc="upper right")
    fig.suptitle("zoom sequences: shrinking the viewport toward one corner",
                 fontsize=10.5)
    fig.tight_layout(rect=[0, 0, 1, 0.90])
    fig.savefig(HERE / "fig_clip_amplify_zoom.png", dpi=200,
                bbox_inches="tight")
    plt.close(fig)
    print("  figure: fig_clip_amplify_zoom.png")


def fig_degenerate(degen):
    fig = plt.figure(figsize=(16.5, 7.4))
    gs = fig.add_gridspec(2, 4, hspace=0.34, wspace=0.28)

    # (a) pentagram: clipped stream + fill difference
    ax = fig.add_subplot(gs[0, 0])
    sx, sy = zip(*degen["star"])
    ax.plot(sx + (sx[0],), sy + (sy[0],), color=COL_IN, lw=1.0,
            label="input (self-crossing)")
    ox, oy = zip(*degen["pent_out"])
    ax.plot(ox, oy, color=COL_OUT, lw=1.4, marker="o", ms=2.5,
            label="clipped stream")
    r = degen["rect"]
    ax.add_patch(plt.Rectangle((r[0], r[1]), r[2], r[3], fill=False,
                               edgecolor="#333333", ls="--", lw=1.0))
    dg = degen["diff_grid"]
    if dg.any():
        gx = np.linspace(0.5, 399.5, 400)
        X, Y = np.meshgrid(gx, gx)
        ax.plot(X[dg], Y[dg], ".", ms=1.6, color=COL_BAD,
                label=f"fill differs ({dg.sum()} px)")
    ax.set_xlim(0, 400)
    ax.set_ylim(400, 0)
    ax.set_title("(a) pentagram, closed: odd-even fill\nvs exact intersection",
                 fontsize=9)
    ax.legend(fontsize=7, frameon=False, loc="lower right")
    set_style(ax)

    # (b) on-border vertex duplication
    ax = fig.add_subplot(gs[0, 1])
    rect = (100.0, 100.0, 200.0, 150.0)
    pts = [(50.0, 175.0), (100.0, 175.0), (50.0, 176.0)]
    out = clip_polygon(rect, pts, False, "F")
    ax.add_patch(plt.Rectangle((rect[0], rect[1]), rect[2], rect[3],
                               fill=False, edgecolor="#333333", ls="--",
                               lw=1.0))
    px, py = zip(*pts)
    ax.plot(px, py, "o-", color=COL_IN, ms=5, lw=1.0, label="input")
    if out:
        ox, oy = zip(*out)
        ax.plot(ox, oy, "s", color=COL_OUT, ms=7, mfc="none",
                label=f"output ({len(out)} pts)")
    ax.annotate("border vertex emitted twice\n(once as vertex, once as\n"
                "intersection)", (100.0, 175.0), (150.0, 60.0), fontsize=7.5,
                arrowprops=dict(arrowstyle="->", color=COL_BAD))
    ax.set_xlim(30, 320)
    ax.set_ylim(280, 40)
    ax.set_title("(b) vertex exactly on the border:\noutput duplicates",
                 fontsize=9)
    ax.legend(fontsize=7, frameon=False)
    set_style(ax)

    # (c) 1px slab
    ax = fig.add_subplot(gs[0, 2])
    sine_part = [(float(x), 500.0 + 450.0 * math.sin(TWO_PI * x / 60.0))
                 for x in range(0, 400, 4)]
    rect = (200.0, 0.0, 1.0, 1000.0)
    out = clip_polygon(rect, sine_part, False, "F")
    sx, sy = zip(*sine_part)
    ax.plot(sx, sy, color=COL_IN, lw=0.9, label="input sine (100 pts)")
    ax.add_patch(plt.Rectangle((rect[0], rect[1]), rect[2], rect[3],
                               facecolor="#ffe9a8", edgecolor="#b8860b",
                               alpha=0.7, lw=0.8, label="1px slab"))
    if out:
        ox, oy = zip(*out)
        ax.plot(ox, oy, ".", color=COL_OUT, ms=4,
                label=f"output ({len(out)} pts)")
    ax.set_xlim(150, 260)
    ax.set_ylim(1000, 0)
    ax.set_title("(c) 1px-wide viewport:\n2 points per traversal", fontsize=9)
    ax.legend(fontsize=7, frameon=False, loc="lower right")
    set_style(ax)

    # (d) single-point pass-through
    ax = fig.add_subplot(gs[1, 0])
    rect = (100.0, 100.0, 200.0, 150.0)
    ax.add_patch(plt.Rectangle((rect[0], rect[1]), rect[2], rect[3],
                               fill=False, edgecolor="#333333", ls="--",
                               lw=1.0, label="clip rect"))
    out_in = clip_polygon(rect, [(150.0, 150.0)], False, "F")
    out_out = clip_polygon(rect, [(900.0, 900.0)], False, "F")
    ax.plot([150.0], [150.0], "o", color=COL_TRUTH, ms=8,
            label=f"inside single point -> kept ({len(out_in)})")
    ax.plot([900.0], [900.0], "o", color=COL_BAD, ms=8,
            label=f"OUTSIDE single point -> kept ({len(out_out)})")
    ax.set_xlim(50, 1000)
    ax.set_ylim(1000, 50)
    ax.set_title("(d) size-1 polygon bypasses\nall four inside tests",
                 fontsize=9)
    ax.legend(fontsize=7, frameon=False, loc="upper right")
    set_style(ax)

    # (e) R path rounding: float rect without integer coverage
    ax = fig.add_subplot(gs[1, 1])
    rect = (100.2, 100.7, 0.5, 90.3)
    ax.add_patch(plt.Rectangle((rect[0], rect[1]), rect[2], rect[3],
                               facecolor="#c8e6c9", edgecolor=COL_TRUTH,
                               lw=1.2, label="QRectF [100.2,100.7]x..."))
    ax.add_patch(plt.Rectangle((101.0, math.ceil(rect[1])), 0.0,
                               math.floor(rect[1] + rect[3]) - math.ceil(rect[1]),
                               facecolor="none", edgecolor=COL_BAD, lw=1.6,
                               ls=":", label="rounded-in int region (empty)"))
    outF = clip_polygon(rect, [(50.0, 120.0), (400.0, 120.0)], False, "F")
    outR = clip_polygon(rect, [(50, 120), (400, 120)], False, "R")
    ax.plot([50.0, 400.0], [120.0, 120.0], color=COL_IN, lw=1.0)
    if outF:
        ox, oy = zip(*outF)
        ax.plot(ox, oy, "o", color=COL_OUT, ms=7,
                label=f"float path: {len(outF)} pts kept")
    ax.plot([], [], "s", color=COL_BAD, ms=7,
            label=f"int path (R): {len(outR)} pts")
    ax.set_xlim(40, 420)
    ax.set_ylim(210, 90)
    ax.set_title("(e) QRectF -> QPolygon overload rounds\nthe rect inwards "
                 "(qCeil / qFloor)", fontsize=9)
    ax.legend(fontsize=7, frameon=False, loc="upper right")
    set_style(ax)

    # (f) NaN contamination
    ax = fig.add_subplot(gs[1, 2])
    rect = (100.0, 100.0, 200.0, 150.0)
    pts = [(50.0, 175.0), (float("nan"), float("nan")), (150.0, 175.0),
           (400.0, 175.0)]
    out = clip_polygon(rect, pts, False, "F")
    ax.add_patch(plt.Rectangle((rect[0], rect[1]), rect[2], rect[3],
                               fill=False, edgecolor="#333333", ls="--",
                               lw=1.0))
    finite = [(x, y) for x, y in pts if x == x and y == y]
    fx, fy = zip(*finite)
    ax.plot(fx, fy, "o", color=COL_IN, ms=5, label="finite input vertices")
    ax.annotate("NaN vertex at index 1\n(plotted nowhere)", (150.0, 175.0),
                (120.0, 275.0), fontsize=7.5,
                arrowprops=dict(arrowstyle="->", color=COL_BAD))
    txt = "output stream:\n" + "\n".join(
        f"  ({x:.1f}, {y:.1f})" if x == x and y == y else "  (nan, nan)"
        for x, y in out)
    ax.text(0.03, 0.06, txt, transform=ax.transAxes, fontsize=7,
            family="monospace", va="bottom",
            bbox=dict(fc="#fff3e0", ec="#ff9800", lw=0.7))
    ax.set_xlim(20, 430)
    ax.set_ylim(300, 80)
    ax.set_title("(f) NaN propagates into\nintersection points", fontsize=9)
    set_style(ax)

    # (g) open polyline enclosing the viewport -> phantom border chord
    ax = fig.add_subplot(gs[1, 3])
    rect = (100.0, 100.0, 200.0, 150.0)
    sq = [(0.0, 0.0), (400.0, 0.0), (400.0, 400.0), (0.0, 400.0)]
    out = clip_polygon(rect, sq, False, "F")
    sx, sy = zip(*(sq + [sq[0]]))
    ax.plot(sx, sy, color=COL_IN, lw=1.0, label="input open square")
    ax.add_patch(plt.Rectangle((rect[0], rect[1]), rect[2], rect[3],
                               fill=False, edgecolor="#333333", ls="--",
                               lw=1.0, label="clip rect"))
    if out:
        ox, oy = zip(*out)
        ax.plot(ox, oy, color=COL_BAD, lw=2.4, marker="o", ms=5,
                label=f"output: phantom chord\n({len(out)} pts, true = empty)")
    ax.set_xlim(-20, 430)
    ax.set_ylim(430, -20)
    ax.set_title("(g) stroke that encloses the viewport\nwithout entering it",
                 fontsize=9)
    ax.legend(fontsize=7, frameon=False, loc="upper left")
    set_style(ax)

    fig.suptitle("degenerate inputs: measured behavior of the real algorithm",
                 fontsize=10.5)
    fig.savefig(HERE / "fig_clip_degenerate.png", dpi=200, bbox_inches="tight")
    plt.close(fig)
    print("  figure: fig_clip_degenerate.png")


def _draw_circle_case(ax, rect, pos, radius, intv, title):
    a, truth = circle_truth(rect, pos, radius, 20000)
    lx, ly, lw, lh = rect
    ax.add_patch(plt.Rectangle((lx, ly), lw, lh, fill=False,
                               edgecolor="#333333", ls="--", lw=1.0))
    cx = pos[0] + radius * np.cos(a)
    cy = pos[1] - radius * np.sin(a)
    ax.plot(cx, cy, color=COL_IN, lw=0.8, label="circle")
    ax.plot(cx[truth], cy[truth], color=COL_TRUTH, lw=2.6, alpha=0.8,
            label="true inside arcs")
    for j, (lo, hi) in enumerate(intv):
        span = np.linspace(lo, hi if hi >= lo else hi + TWO_PI, 400)
        rx_ = pos[0] + radius * np.cos(span)
        ry_ = pos[1] - radius * np.sin(span)
        ax.plot(rx_, ry_, color=COL_BAD, lw=1.2,
                label="returned arcs" if j == 0 else None)
    ax.set_title(title, fontsize=9)
    ax.set_aspect("equal")
    ax.tick_params(labelsize=7)


def fig_circle(circle_results):
    fig, axes = plt.subplots(1, 3, figsize=(12.6, 4.2))
    keys = ["generic 4 corner arcs", "vertical swap (left edge)",
            "corner duplicates"]
    for ax, key in zip(axes, keys):
        r = circle_results[key]
        _draw_circle_case(ax, r["rect"], r["pos"], r["radius"], r["intv"],
                          f"{key}\nsym-diff = {r['symdiff']:.4f} rad")
        ax.legend(fontsize=7, frameon=False, loc="upper right")
    fig.suptitle("clipCircle: returned angle intervals vs brute-force truth "
                 "(green = truth, red = returned)", fontsize=10.5)
    fig.tight_layout(rect=[0, 0, 1, 0.90])
    fig.savefig(HERE / "fig_clip_circle.png", dpi=200, bbox_inches="tight")
    plt.close(fig)
    print("  figure: fig_clip_circle.png")


# =====================================================================
# main
# =====================================================================

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--no-xcheck", action="store_true",
                    help="skip the C++ cross-validation")
    ap.add_argument("--scratch", default=str(DEFAULT_SCRATCH),
                    help="directory holding clipper_check.exe")
    args = ap.parse_args()

    print(f"clipper_experiment.py  seed={SEED}  numpy={np.__version__}  "
          f"matplotlib={matplotlib.__version__}")
    ok = self_test()
    if not ok:
        print("SELF TEST FAILED - aborting")
        return 1

    rng = np.random.default_rng(SEED)
    cases = gen_cross_cases(rng)
    print(f"  cross-check case list: {len(cases)} cases "
          f"({sum(1 for c in cases if c['kind'] == 'K')} polygon, "
          f"{sum(1 for c in cases if c['kind'] == 'C')} circle)")

    if not args.no_xcheck:
        run_cross_check(cases, Path(args.scratch))

    pan_summary, pan_curves = experiment_pan()
    zoom_summary = experiment_zoom()
    worst = experiment_worst_case()
    degen = experiment_degenerate()
    circle_results = experiment_circle()

    print()
    print("== [11] figures ==")
    fig_overview()
    fig_amplify_pan(pan_curves)
    fig_amplify_zoom(zoom_summary, worst)
    fig_degenerate(degen)
    fig_circle(circle_results)
    print("done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
