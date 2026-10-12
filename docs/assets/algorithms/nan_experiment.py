"""
NaN rendering experiments for docs/zh/tech-report/nan-rendering.md.

Part A - semantic re-implementation and cross validation:
  Every NaN-relevant mapping path of src/plot/qwt_point_mapper.cpp is
  replicated line-by-line (same arithmetic, same rounding functors, same
  branch order), plus the Steps polygon builder of qwt_plot_curve.cpp:

    * qwtToPoints            ( mode "none",       RoundPoints            )
    * qwtToPolylineFiltered  ( mode "points",     WeedOutPoints          )
    * qwtMapPointsQuad       ( mode "aggressive", WeedOutIntermediate    )
    * qwtPixelColumnReduce   ( mode "pixel",      PixelColumnReduce      )
    * qwtMinMaxBucketReduce  ( mode "lttb",       MinMaxReduce, raw ptr  )
    * qwtMinMaxBucketReduce  ( mode "lttbgen",    generic sample() path  )

  The directed pattern matrix and the randomized campaign mirror the C++
  harness nan_mapper_check.cpp bit-for-bit (integer/rational signals only,
  no libm transcendentals; shared xorshift64* RNG; Qt-5.15 qRound double->
  int semantics incl. qRound(NaN)==0 and qRound(+inf)==INT_MIN as measured
  on MSVC x64).  Polygon hashes are compared against the checked-in
  nan_mapper_hashes.csv produced by the real library.

Part B - measurement plots:
  Reads nan_results.csv (C++ measurements: perf sweep + offscreen visual
  scan) and renders the article figures.

    python nan_experiment.py [--assets DIR]

numpy + matplotlib only, Agg backend, English figure labels, fixed seeds.
All numeric results are printed to stdout.
"""

import argparse
import math
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent

MODES = ["none", "points", "aggressive", "pixel", "lttb", "lttbgen"]

INT_MIN = -2147483648
INT_MAX = 2147483647
DBL_MAX = 1.7976931348623157e308


# --------------------------------------------------------------- rounding
# MSVC x64 cvttsd2si: NaN / overflow -> "integer indefinite" = INT_MIN.

def c_int_cast(v):
    if math.isnan(v):
        return INT_MIN
    if v >= 2147483648.0 or v < -2147483648.0:
        return INT_MIN
    return int(v)  # truncation toward zero


def i32wrap(v):
    return ((v + (1 << 31)) % (1 << 32)) - (1 << 31)


def qround(v):
    """Qt 5.15 qRound(double), verbatim:
    d >= 0 ? int(d+0.5) : int(d - double(int(d-1)) + 0.5) + int(d-1)"""
    if v >= 0.0:
        return c_int_cast(v + 0.5)
    inner = c_int_cast(v - 1.0)
    return i32wrap(c_int_cast(v - float(inner) + 0.5) + inner)


def qround_f(v):
    """qwtRoundValueF: floor(v+0.5) / ceil(v-0.5); NaN and inf pass through
    (C++ floor/ceil of NaN/inf return the argument)."""
    if math.isnan(v) or math.isinf(v):
        return v
    return math.floor(v + 0.5) if v >= 0.0 else math.ceil(v - 0.5)


def is_nan_or_inf(x, y):
    return not (math.isfinite(x) and math.isfinite(y))


# --------------------------------------------------------------- scale map

class LinearMap:
    """QwtScaleMap without transformation: transform(s) = p1 + (s - ts1)*cnv
    with ts1 = s1 and cnv = (p2 - p1) / (s2 - s1).  The mapper's linear fast
    path instead evaluates s*cnv + (p1 - ts1*cnv) - a different rounding
    sequence, so both forms are provided."""

    def __init__(self, s1, s2, p1, p2):
        self.s1, self.s2, self.p1, self.p2 = s1, s2, p1, p2
        self.cnv = (p2 - p1) / (s2 - s1)
        self.ts1 = s1
        self.off = p1 - s1 * self.cnv

    def transform(self, v):
        return self.p1 + (v - self.ts1) * self.cnv

    def fast(self, v):
        return v * self.cnv + self.off

    def is_linear(self):
        return True


# ------------------------------------------------- visible range / probing

def find_visible_range(xmap, xs, frm, to):
    """qwtFindVisibleRange, verbatim (returns None when the range is not
    narrowed - the caller keeps [frm, to])."""
    if not xmap.is_linear() or to - frm < 20:
        return None
    x0 = xs[frm]
    xn = xs[to]
    if x0 == xn:
        return None
    increasing = xn > x0
    step = max(1, (to - frm) // 50)
    prev = x0
    i = frm + step
    while i < to:
        xi = xs[i]
        if ((xi > prev) != increasing) and (xi != prev):
            return None
        prev = xi
        i += step
    vis_min = min(xmap.s1, xmap.s2)
    vis_max = max(xmap.s1, xmap.s2)
    lo, hi = frm, to
    if increasing:
        while lo < hi:
            mid = lo + (hi - lo) // 2
            if xs[mid] < vis_min:
                lo = mid + 1
            else:
                hi = mid
        vis_from = max(frm, lo - 1)
        hi = to
        while lo < hi:
            mid = lo + (hi - lo + 1) // 2
            if xs[mid] > vis_max:
                hi = mid - 1
            else:
                lo = mid
        vis_to = min(to, hi + 1)
    else:
        while lo < hi:
            mid = lo + (hi - lo) // 2
            if xs[mid] > vis_max:
                lo = mid + 1
            else:
                hi = mid
        vis_from = max(frm, lo - 1)
        hi = to
        while lo < hi:
            mid = lo + (hi - lo + 1) // 2
            if xs[mid] < vis_min:
                hi = mid - 1
            else:
                lo = mid
        vis_to = min(to, hi + 1)
    if vis_from <= vis_to:
        return vis_from, vis_to
    return None


def probe_orientation(xs, frm, to):
    """qwtProbeOrientation, verbatim."""
    if to - frm < 20:
        return "H"
    x0 = xs[frm]
    xn = xs[to]
    if x0 == xn:
        return "V"
    step = max(1, (to - frm) // 50)
    increasing = xn > x0
    x1 = x0
    i = frm + step
    while i < to:
        x2 = xs[i]
        if x2 != x1:
            if ((x2 > x1) != increasing):
                return "V"
        x1 = x2
        i += step
    return "H"


# ------------------------------------------------------------- quadrupels

class QuadX:
    """QwtPolygonQuadrupelX: merges runs of equal integer x."""

    def __init__(self):
        self.x0 = self.y1 = self.ymin = self.ymax = self.y2 = 0

    def start(self, x, y):
        self.x0 = x
        self.y1 = self.ymin = self.ymax = self.y2 = y

    def append(self, x, y):
        if self.x0 != x:
            return False
        if y < self.ymin:
            self.ymin = y
        elif y > self.ymax:
            self.ymax = y
        self.y2 = y
        return True

    def flush(self, poly):
        poly.append((float(self.x0), float(self.y1)))
        ymin, ymax = self.ymin, self.ymax
        if self.y2 > self.y1:
            ymin, ymax = ymax, ymin
        if ymax != self.y1:
            poly.append((float(self.x0), float(ymax)))
        if ymin != ymax:
            poly.append((float(self.x0), float(ymin)))
        if self.y2 != ymin:
            poly.append((float(self.x0), float(self.y2)))


class QuadY:
    """QwtPolygonQuadrupelY: merges runs of equal integer y."""

    def __init__(self):
        self.y0 = self.x1 = self.xmin = self.xmax = self.x2 = 0

    def start(self, x, y):
        self.y0 = y
        self.x1 = self.xmin = self.xmax = self.x2 = x

    def append(self, x, y):
        if self.y0 != y:
            return False
        if x < self.xmin:
            self.xmin = x
        elif x > self.xmax:
            self.xmax = x
        self.x2 = x
        return True

    def flush(self, poly):
        poly.append((float(self.x1), float(self.y0)))
        xmin, xmax = self.xmin, self.xmax
        if self.x2 > self.x1:
            xmin, xmax = xmax, xmin
        if xmax != self.x1:
            poly.append((float(xmax), float(self.y0)))
        if xmin != xmax:
            poly.append((float(xmin), float(self.y0)))
        if self.x2 != xmin:
            poly.append((float(self.x2), float(self.y0)))


# ------------------------------------------------------------- quad reduce

def _quad_pass_series(quad_cls, xmap, ymap, xs, ys, frm, to):
    """qwtMapPointsQuad first pass (series -> polygon), NaN skipped."""
    poly = []
    real_from = frm
    s0x, s0y = xs[frm], ys[frm]
    while real_from < to and is_nan_or_inf(s0x, s0y):
        real_from += 1
        s0x, s0y = xs[real_from], ys[real_from]
    if real_from >= to and is_nan_or_inf(s0x, s0y):
        return poly
    q = quad_cls()
    q.start(qround(xmap.fast(s0x)), qround(ymap.fast(s0y)))
    for i in range(real_from, to + 1):
        sx, sy = xs[i], ys[i]
        if is_nan_or_inf(sx, sy):
            continue
        x = qround(xmap.fast(sx))
        y = qround(ymap.fast(sy))
        if not q.append(x, y):
            q.flush(poly)
            q.start(x, y)
    q.flush(poly)
    return poly


def _quad_pass_polygon(quad_cls, poly):
    """qwtMapPointsQuad second pass (polygon -> polygon)."""
    if len(poly) < 3:
        return poly
    out = []
    q = quad_cls()
    q0x, q0y = poly[0]
    q.start(int(q0x), int(q0y))
    for (px, py) in poly:
        x, y = int(px), int(py)
        if not q.append(x, y):
            q.flush(out)
            q.start(x, y)
    q.flush(out)
    return out


def map_points_quad(xmap, ymap, xs, ys, frm, to):
    if frm > to:
        return []
    orientation = probe_orientation(xs, frm, to)
    if orientation == "H":
        poly = _quad_pass_series(QuadY, xmap, ymap, xs, ys, frm, to)
        poly = _quad_pass_polygon(QuadX, poly)
    else:
        poly = _quad_pass_series(QuadX, xmap, ymap, xs, ys, frm, to)
        poly = _quad_pass_polygon(QuadY, poly)
    return poly


# --------------------------------------------------------------- to_points

def to_points_none(xmap, ymap, xs, ys, frm, to):
    """qwtToPoints with QwtRoundF and an invalid bounding rect (mode none)."""
    poly = []
    for i in range(frm, to + 1):
        sx, sy = xs[i], ys[i]
        if is_nan_or_inf(sx, sy):
            continue
        poly.append((qround_f(xmap.transform(sx)), qround_f(ymap.transform(sy))))
    return poly


def to_polyline_filtered(xmap, ymap, xs, ys, frm, to):
    """qwtToPolylineFiltered with QwtRoundF (mode points).  No all-NaN early
    return: a lone (nan, nan) vertex is produced, mirroring the C++ code."""
    real_from = frm
    s0x, s0y = xs[frm], ys[frm]
    while real_from < to and is_nan_or_inf(s0x, s0y):
        real_from += 1
        s0x, s0y = xs[real_from], ys[real_from]
    pts = [(qround_f(xmap.transform(s0x)), qround_f(ymap.transform(s0y)))]
    pos = 0
    for i in range(real_from + 1, to + 1):
        sx, sy = xs[i], ys[i]
        if is_nan_or_inf(sx, sy):
            continue
        p = (qround_f(xmap.transform(sx)), qround_f(ymap.transform(sy)))
        if pts[pos] != p:
            pts.append(p)
            pos += 1
    return pts


# ------------------------------------------------------ pixel column reduce

def pixel_column_reduce(xmap, ymap, xs, ys, frm, to):
    """qwtPixelColumnReduce, verbatim (linear fast path)."""
    if frm > to:
        return []
    vis = find_visible_range(xmap, xs, frm, to)
    vis_from, vis_to = vis if vis else (frm, to)
    x_pixels = abs(qround(xmap.p2 - xmap.p1)) + 1
    if x_pixels <= 0:
        return []
    first_y = [0] * x_pixels
    min_y = [0] * x_pixels
    max_y = [0] * x_pixels
    last_y = [0] * x_pixels
    count = [0] * x_pixels
    x_min = qround(xmap.p1)
    for i in range(vis_from, vis_to + 1):
        sx, sy = xs[i], ys[i]
        if is_nan_or_inf(sx, sy):
            continue
        x = qround(xmap.fast(sx))
        y = qround(ymap.fast(sy))
        col = x - x_min
        if col < 0 or col >= x_pixels:
            continue
        if count[col] == 0:
            first_y[col] = min_y[col] = max_y[col] = last_y[col] = y
        else:
            if y < min_y[col]:
                min_y[col] = y
            elif y > max_y[col]:
                max_y[col] = y
            last_y[col] = y
        count[col] += 1
    poly = []
    for col in range(x_pixels):
        if count[col] == 0:
            continue
        x = float(x_min + col)
        if count[col] == 1:
            poly.append((x, float(first_y[col])))
            continue
        gap_before = col > 0 and count[col - 1] <= 1
        gap_after = col < x_pixels - 1 and count[col + 1] <= 1
        if gap_before:
            poly.append((x + 0.2, float(first_y[col])))
        poly.append((x + 0.25, float(min_y[col])))
        poly.append((x + 0.75, float(max_y[col])))
        if gap_after:
            poly.append((x + 0.8, float(last_y[col])))
    return poly


# ----------------------------------------------------- minmax bucket reduce

def _argminmax_first(vals, start, end):
    """First-occurrence argmin/argmax (the value semantics of both the SIMD
    ordered-predicate path and the scalar path; see the article)."""
    min_i = max_i = start
    min_v = max_v = vals[start]
    for i in range(start + 1, end + 1):
        v = vals[i]
        if v < min_v:
            min_v, min_i = v, i
        if v > max_v:
            max_v, max_i = v, i
    return min_i, max_i


def minmax_bucket_reduce(xmap, ymap, xs, ys, frm, to, generic=False):
    """qwtMinMaxBucketReduce, verbatim (linear paths).  generic=False uses
    the raw-pointer branches (NaN pre-scan on Y only; scalar branch checks
    isnan(y) only, so an x-NaN sample can win an extremum and maps to
    qRound(NaN)==0).  generic=True uses the virtual-sample branch with the
    full qwt_is_nan_or_inf check."""
    if frm > to:
        return []
    vis = find_visible_range(xmap, xs, frm, to)
    vis_from, vis_to = vis if vis else (frm, to)
    num_points = vis_to - vis_from + 1
    if num_points <= 0:
        return []
    x_pixels = abs(qround(xmap.p2 - xmap.p1)) + 1
    num_buckets = max(2, 2 * x_pixels)
    if num_points <= num_buckets * 2:
        return map_points_quad(xmap, ymap, xs, ys, vis_from, vis_to)
    bucket_size = float(num_points) / num_buckets
    poly = []

    def emit(sx_min, sy_min, sx_max, sy_max, i_min, i_max):
        if i_min <= i_max:
            poly.append((float(sx_min), float(sy_min)))
            if sy_max != sy_min or sx_max != sx_min:
                poly.append((float(sx_max), float(sy_max)))
        else:
            poly.append((float(sx_max), float(sy_max)))
            if sy_max != sy_min or sx_max != sx_min:
                poly.append((float(sx_min), float(sy_min)))

    if not generic:
        has_nan = False
        for i in range(vis_from, vis_to + 1):
            if math.isnan(ys[i]):
                has_nan = True
                break
        for b in range(num_buckets):
            start = vis_from + int(b * bucket_size)
            end = vis_from + int((b + 1) * bucket_size) - 1
            b_end = min(end, vis_to)
            if b_end - start + 1 <= 0:
                continue
            if not has_nan:
                # SIMD fast path: no NaN in the visible Y range
                min_i, max_i = _argminmax_first(ys, start, b_end)
            else:
                # scalar raw-pointer path: isnan(y) check only
                min_i = max_i = -1
                min_y = max_y = 0.0
                first = True
                for i in range(start, b_end + 1):
                    y = ys[i]
                    if math.isnan(y):
                        continue
                    if first:
                        min_i = max_i = i
                        min_y = max_y = y
                        first = False
                    else:
                        if y < min_y:
                            min_y, min_i = y, i
                        if y > max_y:
                            max_y, max_i = y, i
                if first:
                    continue
            sx_min = qround(xmap.fast(xs[min_i]))
            sx_max = qround(xmap.fast(xs[max_i]))
            sy_min = qround(ymap.fast(ys[min_i]))
            sy_max = qround(ymap.fast(ys[max_i]))
            emit(sx_min, sy_min, sx_max, sy_max, min_i, max_i)
    else:
        for b in range(num_buckets):
            start = vis_from + int(b * bucket_size)
            end = vis_from + int((b + 1) * bucket_size) - 1
            b_end = min(end, vis_to)
            min_x = max_x = 0
            min_y, max_y = 1e300, -1e300
            min_i = max_i = -1
            first = True
            for i in range(start, b_end + 1):
                sx, sy = xs[i], ys[i]
                if is_nan_or_inf(sx, sy):
                    continue
                px = qround(xmap.fast(sx))
                if first:
                    min_x = max_x = px
                    min_y = max_y = sy
                    min_i = max_i = i
                    first = False
                else:
                    if sy < min_y:
                        min_y, min_x, min_i = sy, px, i
                    if sy > max_y:
                        max_y, max_x, max_i = sy, px, i
            if first:
                continue
            emit(min_x, qround(ymap.fast(min_y)), max_x, qround(ymap.fast(max_y)), min_i, max_i)
    return poly


# ------------------------------------------------------------------ Steps

def steps_polygon(xmap, ymap, xs, ys, frm, to, inverted=True, do_align=True):
    """QwtPlotCurve::drawSteps polygon construction (NaN samples skipped,
    corner points as in the C++ loop)."""
    pts = []
    for i in range(frm, to + 1):
        sx, sy = xs[i], ys[i]
        if is_nan_or_inf(sx, sy):
            continue
        xi = xmap.transform(sx)
        yi = ymap.transform(sy)
        if do_align:
            xi = float(qround(xi))
            yi = float(qround(yi))
        if pts:
            p0x, p0y = pts[-1]
            if inverted:
                pts.append((p0x, yi))
            else:
                pts.append((xi, p0y))
        pts.append((xi, yi))
    return pts


# ------------------------------------------------------------ mode dispatch

def run_mode(mode, xmap, ymap, xs, ys, frm, to):
    if mode == "none":
        return to_points_none(xmap, ymap, xs, ys, frm, to)
    if mode == "points":
        return to_polyline_filtered(xmap, ymap, xs, ys, frm, to)
    if mode == "aggressive":
        return map_points_quad(xmap, ymap, xs, ys, frm, to)
    if mode == "pixel":
        return pixel_column_reduce(xmap, ymap, xs, ys, frm, to)
    if mode == "lttb":
        return minmax_bucket_reduce(xmap, ymap, xs, ys, frm, to, generic=False)
    if mode == "lttbgen":
        return minmax_bucket_reduce(xmap, ymap, xs, ys, frm, to, generic=True)
    raise ValueError(mode)


# ------------------------------------------------------------------- hash

HASH_M = 4294967291  # 2^32 - 5


def quant_vertex(v):
    if math.isnan(v):
        return -2
    if v > 1e12:
        return -3
    if v < -1e12:
        return -4
    return int(math.floor(v * 1e6 + 0.5))


def hash_polygon(poly):
    h = len(poly) % HASH_M
    for (vx, vy) in poly:
        h = (h * 1000003 + quant_vertex(vx) % HASH_M) % HASH_M
        h = (h * 1000003 + quant_vertex(vy) % HASH_M) % HASH_M
    return h


# -------------------------------------------------------------------- RNG

MASK64 = (1 << 64) - 1


class Rng64:
    """xorshift64* - identical stream to the C++ harness."""

    def __init__(self, seed):
        self.s = seed if seed else 88172645463325252

    def next(self):
        s = self.s
        s ^= s >> 12
        s = (s ^ (s << 25)) & MASK64
        s ^= s >> 27
        self.s = s
        return (s * 2685821657736338717) & MASK64


# ---------------------------------------------------------------- signals

def exact_signal(i, period):
    """Rational spike-carrying signal (no transcendentals => bit-identical
    in C++ and Python).  Mirrors exactSignal() in nan_mapper_check.cpp."""
    t = float(i % period) - float(period) * 0.5
    v = 2.4 / (1.0 + t * t / (float(period) * float(period) / 400.0))
    u = float(i % 47)
    v += 0.35 * (u * u - 21.0 * u) / 250.0
    if i % 991 == 0:
        v += 2.2
    if i % 733 == 17:
        v -= 1.9
    return v


def random_signal(i):
    t = float(i % 173) - 86.0
    y = 1.7 / (1.0 + t * t / 300.0) + float(i % 13) * 0.07 - 0.42
    if i % 97 == 5:
        y += 1.1
    return y


NAN = float("nan")


def make_pattern(kind, n, period):
    """Mirrors makePattern() in nan_mapper_check.cpp (integer divisions!)."""
    x = [float(i) for i in range(n)]
    y = [exact_signal(i, period) for i in range(n)]
    if kind == "baseline":
        pass
    elif kind == "isolated":
        for i in range(137, n, 997):
            y[i] = NAN
    elif kind == "run":
        for c in range(3):
            start = (n // 4) * (c + 1)
            for i in range(start, min(start + max(8, n // 25), n)):
                y[i] = NAN
    elif kind == "leading":
        for i in range(n // 10):
            y[i] = NAN
    elif kind == "trailing":
        for i in range(n - n // 10, n):
            y[i] = NAN
    elif kind == "leadtrail":
        for i in range(n // 10):
            y[i] = NAN
        for i in range(n - n // 10, n):
            y[i] = NAN
    elif kind == "all":
        for i in range(n):
            y[i] = NAN
    elif kind == "extremum":
        i_min, i_max = 0, 0
        for i in range(1, n):
            if y[i] < y[i_min]:
                i_min = i
            if y[i] > y[i_max]:
                i_max = i
        y[i_min] = NAN
        y[i_max] = NAN
        y[n // 2] = NAN
    elif kind == "periodic":
        for i in range(n):
            if (i % 50) < 5:
                y[i] = NAN
    elif kind == "xynan":
        start = (n - n // 10) // 2
        for i in range(start, start + n // 10):
            x[i] = NAN
            y[i] = NAN
    elif kind == "xyinter":
        start = (n - n // 10) // 2
        for i in range(start, start + n // 10):
            if (i - start) % 2 == 0:
                x[i] = NAN
            else:
                y[i] = NAN
    elif kind == "xonly":
        x[n // 2] = NAN
    elif kind == "inf":
        y[n // 3] = math.inf
        y[2 * n // 3] = -math.inf
    elif kind == "spikenan":
        for idx in (n // 8, 3 * n // 8, 5 * n // 8, 7 * n // 8):
            y[idx] = NAN
    else:
        raise ValueError(kind)
    return x, y


def matrix_postprocess(kind, n, y):
    """runMatrix() adds spikes to extremum/spikenan and re-NaNs the global
    max for extremum, in this exact order."""
    if kind in ("extremum", "spikenan"):
        for idx in (n // 8, 3 * n // 8, 5 * n // 8, 7 * n // 8):
            y[idx] += 2.2  # NaN + 2.2 = NaN for spikenan
    if kind == "extremum":
        i_max, best = -1, -DBL_MAX
        for i in range(n):
            if math.isfinite(y[i]) and y[i] > best:
                best, i_max = y[i], i
        if i_max >= 0:
            y[i_max] = NAN
    return y


def y_scale_from_data(x, y):
    """Mirrors yScaleFromData() in the harness."""
    n = len(x)
    xs1 = 0.0
    xs2 = max(1.0, float(n - 1))
    lo, hi = DBL_MAX, -DBL_MAX
    for i in range(n):
        if not math.isfinite(y[i]) or not math.isfinite(x[i]):
            continue
        lo = min(lo, y[i])
        hi = max(hi, y[i])
    if lo > hi:
        return xs1, xs2, -1.0, 1.0
    if lo == hi:
        return xs1, xs2, lo - 1.0, hi + 1.0
    return xs1, xs2, lo, hi


def make_maps(xs1, xs2, ys1, ys2, W, H):
    xmap = LinearMap(xs1, xs2, 0.0, float(W - 1))
    ymap = LinearMap(ys1, ys2, float(H - 1), 0.0)
    return xmap, ymap


# --------------------------------------------- directed matrix regeneration

PATTERNS = ["baseline", "isolated", "run", "leading", "trailing", "leadtrail",
            "all", "extremum", "periodic", "xynan", "xyinter", "xonly", "inf",
            "spikenan"]

TINY_X = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]
TINY_Y = [1, 2, NAN, 0, NAN, NAN, 3, NAN, 2, 1, NAN, 0]


def directed_cases():
    """(caseId, x, y, W, H, scales) - mirrors runMatrix() exactly."""
    cases = [("tiny", [float(v) for v in TINY_X], [float(v) for v in TINY_Y],
              12, 8, (0.0, 11.0, 0.0, 3.0))]
    n = 8000
    for kind in PATTERNS:
        x, y = make_pattern(kind, n, 2000)
        y = matrix_postprocess(kind, n, y)
        scales = y_scale_from_data(x, y)
        cases.append(("pat_%s" % kind, x, y, 900, 500, scales))
    return cases


def random_cases(count, seed):
    """Mirrors runRandom() in the harness, including RNG short-circuits."""
    rng = Rng64(seed)
    out = []
    for c in range(count):
        n = 3 + int(rng.next() % 500)
        p_y = int(rng.next() % 61)
        p_x = int(rng.next() % 11)
        all_nan = (rng.next() % 25) == 0
        x = [0.0] * n
        y = [0.0] * n
        for i in range(n):
            x[i] = float(i)
            y[i] = random_signal(i)
            if all_nan or int(rng.next() % 100) < p_y:
                y[i] = NAN
            if (not all_nan) and int(rng.next() % 100) < p_x:
                x[i] = NAN
        for (W, H) in ((40, 30), (900, 500)):
            scales = y_scale_from_data(x, y)
            out.append((c, W, H, x, y, scales))
    return out


# -------------------------------------------------------- semantic metrics

def finite_mask(x, y):
    return [math.isfinite(x[i]) and math.isfinite(y[i]) for i in range(len(x))]


def ideal_segments(x, y):
    """Finite sample runs; mirrors the C++ visual scan loop which iterates
    one step past the end so a trailing finite run is flushed."""
    mask = finite_mask(x, y)
    n = len(mask)
    segs = []
    start = None
    for i in range(n + 1):
        ok = (i < n) and mask[i]
        if ok and start is None:
            start = i
        elif not ok and start is not None:
            segs.append((start, i))
            start = None
    return segs


def internal_gaps(x, y):
    """Internal NaN runs: gaps with a finite neighbour on both sides."""
    segs = ideal_segments(x, y)
    gaps = []
    for a, b in zip(segs, segs[1:]):
        gaps.append((a[1], b[0]))  # [gapStart, gapEnd)
    return gaps


def count_garbage(poly):
    n = 0
    for (vx, vy) in poly:
        if not math.isfinite(vx) or not math.isfinite(vy) or abs(vx) > 1e8 or abs(vy) > 1e8:
            n += 1
    return n


def count_bridged_gaps(poly, gaps, xmap):
    """How many internal NaN runs are crossed by at least one polygon
    segment (a pseudo segment bridging the gap)."""
    if len(poly) < 2 or not gaps:
        return 0
    seg_xs = []
    for (x1, _), (x2, _) in zip(poly, poly[1:]):
        if math.isfinite(x1) and math.isfinite(x2):
            seg_xs.append((min(x1, x2), max(x1, x2)))
    bridged = 0
    for (g0, g1) in gaps:
        cx = xmap.fast((g0 + g1 - 1) * 0.5)
        for (a, b) in seg_xs:
            if a < cx < b:
                bridged += 1
                break
    return bridged


# ------------------------------------------------------------ part A: main

def run_crossval(hash_file):
    """Compare regenerated polygon hashes against the C++ library hashes."""
    cpp = {}
    with open(hash_file, "r", encoding="utf-8") as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue
            parts = line.split()
            if parts[0] == "MH":
                cpp[("M", parts[1], parts[2])] = (int(parts[3]), int(parts[4]))
            elif parts[0] == "RH":
                cpp[("R", int(parts[1]), int(parts[2]), int(parts[3]), parts[4])] = (
                    int(parts[5]), int(parts[6]))

    total = match = 0
    mismatches = []
    by_mode = {m: [0, 0] for m in MODES}

    # directed cases
    for (cid, x, y, W, H, scales) in directed_cases():
        xmap, ymap = make_maps(*scales, W, H)
        for mode in MODES:
            poly = run_mode(mode, xmap, ymap, x, y, 0, len(x) - 1)
            h = hash_polygon(poly)
            key = ("M", cid, mode)
            if key not in cpp:
                continue
            n_cpp, h_cpp = cpp[key]
            total += 1
            by_mode[mode][1] += 1
            if h == h_cpp and len(poly) == n_cpp:
                match += 1
                by_mode[mode][0] += 1
            else:
                mismatches.append((key, len(poly), h, n_cpp, h_cpp))

    # random campaign
    for (c, W, H, x, y, scales) in random_cases(250, 20261012):
        xmap, ymap = make_maps(*scales, W, H)
        for mode in MODES:
            poly = run_mode(mode, xmap, ymap, x, y, 0, len(x) - 1)
            h = hash_polygon(poly)
            key = ("R", c, W, H, mode)
            if key not in cpp:
                continue
            n_cpp, h_cpp = cpp[key]
            total += 1
            by_mode[mode][1] += 1
            if h == h_cpp and len(poly) == n_cpp:
                match += 1
                by_mode[mode][0] += 1
            else:
                mismatches.append((key, len(poly), h, n_cpp, h_cpp))

    print("== cross validation vs real library (nan_mapper_hashes.csv) ==")
    for mode in MODES:
        ok, tot = by_mode[mode]
        print("mode %-11s exact match %5d / %5d  (%.2f%%)" % (mode, ok, tot, 100.0 * ok / max(tot, 1)))
    print("TOTAL %d / %d  (%.4f%%)" % (match, total, 100.0 * match / max(total, 1)))
    for mm in mismatches[:10]:
        print("  MISMATCH", mm)
    return match, total


def run_semantic_matrix():
    """The article's semantic matrix: pattern x mode summaries plus the
    Steps/Dots/Sticks semantic rows (analysis, exact for Steps)."""
    print()
    print("== semantic matrix (directed patterns, N=8000, canvas 900x500) ==")
    hdr = ("pattern", "nFinite", "idealSeg", "intGaps", "mode", "nVerts",
           "garbage", "bridgedGaps", "polylines")
    print("%-10s %7s %8s %7s %-11s %7s %7s %11s %9s" % hdr)
    rows = []
    n = 8000
    for kind in PATTERNS:
        x, y = make_pattern(kind, n, 2000)
        y = matrix_postprocess(kind, n, y)
        scales = y_scale_from_data(x, y)
        xmap, ymap = make_maps(*scales, 900, 500)
        n_fin = sum(finite_mask(x, y))
        segs = ideal_segments(x, y)
        gaps = internal_gaps(x, y)
        for mode in MODES:
            poly = run_mode(mode, xmap, ymap, x, y, 0, n - 1)
            g = count_garbage(poly)
            br = count_bridged_gaps(poly, gaps, xmap)
            n_polys = 0 if not poly else 1
            row = (kind, n_fin, len(segs), len(gaps), mode, len(poly), g, br, n_polys)
            rows.append(row)
            print("%-10s %7d %8d %7d %-11s %7d %7d %11d %9d" % row)
        # Steps (polygon mirror, exact) - always a single polyline
        sp = steps_polygon(xmap, ymap, x, y, 0, n - 1)
        br = count_bridged_gaps(sp, gaps, xmap)
        row = (kind, n_fin, len(segs), len(gaps), "steps(style)", len(sp),
               count_garbage(sp), br, 0 if not sp else 1)
        rows.append(row)
        print("%-10s %7d %8d %7d %-11s %7d %7d %11d %9d" % row)
        # Dots / Sticks: per-sample styles - no connectivity, no bridging
        for style in ("dots(style)", "sticks(style)"):
            row = (kind, n_fin, len(segs), len(gaps), style, n_fin, 0, 0, n_fin)
            rows.append(row)
            print("%-10s %7d %8d %7d %-11s %7d %7d %11d %9d" % row)
    return rows


def run_tiny_example():
    print()
    print("== tiny worked example (N=12, canvas 12x8, y in 0..3) ==")
    x = [float(v) for v in TINY_X]
    y = [float(v) for v in TINY_Y]
    xmap, ymap = make_maps(0.0, 11.0, 0.0, 3.0, 12, 8)
    for mode in MODES:
        poly = run_mode(mode, xmap, ymap, x, y, 0, 11)
        vs = " ".join("(%g,%g)" % (vx, vy) for vx, vy in poly)
        print("%-11s %2d verts: %s" % (mode, len(poly), vs))
    sp = steps_polygon(xmap, ymap, x, y, 0, 11)
    vs = " ".join("(%g,%g)" % (vx, vy) for vx, vy in sp)
    print("%-11s %2d verts: %s" % ("steps", len(sp), vs))


# ================================================================ part B
# Measurement plots from nan_results.csv (C++ runs) + the segments
# schematic rendered from the validated Python mirror.

MODE_COLORS = {
    "none": "#9aa0a6",
    "points": "#9467bd",
    "aggressive": "#ff7f0e",
    "pixel": "#2ca02c",
    "lttb": "#d62728",
}
MODE_LABELS = {
    "none": "ClipPolygons only (no filter)",
    "points": "FilterPoints",
    "aggressive": "FilterPointsAggressive",
    "pixel": "FilterPointsPixel",
    "lttb": "FilterPointsLTTB",
}
PLOT_MODES = ["none", "points", "aggressive", "pixel", "lttb"]


def load_results(path):
    perf, vis, probes = [], [], {}
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or line.startswith("record,"):
                continue
            p = line.split(",")
            if p[0] == "perf":
                perf.append(dict(sweep=p[1], case=p[2], mode=p[3], points=int(p[4]),
                                 nan_pct=float(p[5]), nan_points=int(p[6]),
                                 br=float(p[7]), rmin=float(p[8]), rmed=float(p[9]),
                                 fps=float(p[10]), rounds=int(p[11])))
            elif p[0] == "vis":
                vis.append(dict(pattern=p[1], mode=p[2], segs=int(p[3]), diff=int(p[4]),
                                img=int(p[5]), maxdev=int(p[6]),
                                bb=(int(p[7]), int(p[8]), int(p[9]), int(p[10]))))
            elif p[0] == "probe":
                probes[p[1]] = p[2]
    return perf, vis, probes


def _perf_lookup(perf, **kw):
    out = []
    for r in perf:
        if all(r.get(k) == v for k, v in kw.items()):
            out.append(r)
    return out


def print_perf_tables(perf):
    print()
    print("== sweep A: Middle case, frame time (replot min-of-7, ms) ==")
    for points in (100000, 1000000):
        print("-- points = %d --" % points)
        print("%6s %10s %10s %10s %10s %10s" % ("nan%", *PLOT_MODES))
        for pct in (0.0, 1.0, 5.0, 10.0, 25.0, 50.0):
            row = []
            for mode in PLOT_MODES:
                rs = _perf_lookup(perf, sweep="A", mode=mode, points=points, nan_pct=pct)
                row.append(rs[0]["rmin"] if rs else float("nan"))
            print("%6.0f %10.2f %10.2f %10.2f %10.2f %10.2f" % (pct, *row))
    print()
    print("== sweep A: boundingRect (cold, min-of-5, ms; mode-independent O(n) scan) ==")
    for points in (100000, 1000000):
        vals = {}
        for pct in (0.0, 1.0, 5.0, 10.0, 25.0, 50.0):
            rs = _perf_lookup(perf, sweep="A", points=points, nan_pct=pct)
            vals[pct] = (min(r["br"] for r in rs), max(r["br"] for r in rs))
        print("points=%d:" % points, " ".join("%.0f%%:%.1f-%.1f" % (k, v[0], v[1]) for k, v in vals.items()))
    print()
    print("== sweep B: NaN distribution cases, 100k points, 10% NaN, replot min (ms) ==")
    cases = ["baseline", "leading", "leadtrail", "middle", "trailing", "xynan", "xyinter", "allnan"]
    print("%10s %10s %10s %10s %10s %10s" % ("case", *PLOT_MODES))
    for case in cases:
        row = []
        for mode in PLOT_MODES:
            rs = _perf_lookup(perf, sweep="B", case=case, mode=mode)
            row.append(rs[0]["rmin"] if rs else float("nan"))
        print("%10s %10.2f %10.2f %10.2f %10.2f %10.2f" % (case, *row))
    print()
    print("== sweep C: degenerate data ==")
    for r in perf:
        if r["sweep"] == "C":
            print("  %-8s %-11s boundingRect %.4f ms  replot %.4f ms" % (r["case"], r["mode"], r["br"], r["rmin"]))
    print()
    print("== SIMD cliff analysis (nanperf-style ratios, sweep A) ==")
    for points in (100000, 1000000):
        l0 = _perf_lookup(perf, sweep="A", mode="lttb", points=points, nan_pct=0.0)[0]["rmin"]
        l1 = _perf_lookup(perf, sweep="A", mode="lttb", points=points, nan_pct=1.0)[0]["rmin"]
        o0 = np.mean([_perf_lookup(perf, sweep="A", mode=m, points=points, nan_pct=0.0)[0]["rmin"]
                      for m in ("none", "points", "aggressive", "pixel")])
        o1 = np.mean([_perf_lookup(perf, sweep="A", mode=m, points=points, nan_pct=1.0)[0]["rmin"]
                      for m in ("none", "points", "aggressive", "pixel")])
        print("points=%d: lttb 0%%->1%%: %.2f -> %.2f ms (ratio %.3f); other modes mean %.2f -> %.2f (ratio %.3f); delta %+.3f"
              % (points, l0, l1, l1 / l0, o0, o1, o1 / o0, l1 / l0 - o1 / o0))


def print_vis_table(vis, probes):
    print()
    print("== visual scan: single NaN curve vs ideal split rendering (960x560, N=20000) ==")
    pats = []
    for r in vis:
        if r["pattern"] not in pats:
            pats.append(r["pattern"])
    print("%10s %8s %8s %8s %8s %8s" % ("pattern", *PLOT_MODES))
    for pat in pats:
        row = []
        for mode in PLOT_MODES:
            rs = [r for r in vis if r["pattern"] == pat and r["mode"] == mode]
            row.append(100.0 * rs[0]["diff"] / rs[0]["img"] if rs else float("nan"))
        print("%10s %8.3f %8.3f %8.3f %8.3f %8.3f" % (pat, *row))
    print()
    print("== behaviour probes ==")
    for k in sorted(probes):
        print("  %-32s %s" % (k, probes[k]))


# ------------------------------------------------------------------ figures

def _segments_data():
    n = 8000
    x, y = make_pattern("run", n, 2000)
    y = matrix_postprocess("run", n, y)
    scales = y_scale_from_data(x, y)
    xmap, ymap = make_maps(*scales, 900, 500)
    return x, y, xmap, ymap, n


def _bridge_flags(poly, gaps, xmap):
    flags = []
    for i in range(len(poly) - 1):
        (x1, _), (x2, _) = poly[i], poly[i + 1]
        b = False
        if math.isfinite(x1) and math.isfinite(x2):
            a, c = min(x1, x2), max(x1, x2)
            for (g0, g1) in gaps:
                cx = xmap.fast((g0 + g1 - 1) * 0.5)
                if a < cx < c:
                    b = True
                    break
        flags.append(b)
    return flags


def fig_nan_segments(assets):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    x, y, xmap, ymap, n = _segments_data()
    gaps = internal_gaps(x, y)
    segs = ideal_segments(x, y)

    fig, axes = plt.subplots(3, 2, figsize=(10.2, 7.6), sharex=True, sharey=True)
    panels = [("ideal split (reference)", None)] + [(MODE_LABELS[m], m) for m in PLOT_MODES]
    for ax, (title, mode) in zip(axes.ravel(), panels):
        for (g0, g1) in gaps:
            ax.axvspan(xmap.fast(g0 - 0.5), xmap.fast(g1 - 0.5), color="0.88", zorder=0)
        if mode is None:
            for (a, b) in segs:
                px = [xmap.fast(x[i]) for i in range(a, b)]
                py = [ymap.fast(y[i]) for i in range(a, b)]
                ax.plot(px, py, lw=0.8, color="#1f77b4")
            nverts = sum(b - a for a, b in segs)
            ax.set_title("%s\n%d polylines, %d vertices" % (title, len(segs), nverts), fontsize=9, loc="left")
        else:
            poly = run_mode(mode, xmap, ymap, x, y, 0, n - 1)
            flags = _bridge_flags(poly, gaps, xmap)
            for i in range(len(poly) - 1):
                (x1, y1), (x2, y2) = poly[i], poly[i + 1]
                if not (math.isfinite(x1) and math.isfinite(y1) and math.isfinite(x2) and math.isfinite(y2)):
                    continue
                ax.plot([x1, x2], [y1, y2], lw=1.4 if flags[i] else 0.7,
                        color="#d62728" if flags[i] else MODE_COLORS[mode], zorder=3 if flags[i] else 2)
            nbridge = sum(flags)
            ax.set_title("%s\n1 polyline, %d vertices, %d bridging segment(s)" % (title, len(poly), nbridge),
                         fontsize=9, loc="left")
        ax.invert_yaxis()
        ax.set_xlim(-5, 905)
        ax.set_ylim(505, -5)
        ax.tick_params(labelsize=7)
    for ax in axes[-1]:
        ax.set_xlabel("pixel column (900 px canvas)")
    axes[0][0].set_ylabel("pixel row")
    axes[1][0].set_ylabel("pixel row")
    axes[2][0].set_ylabel("pixel row")
    fig.suptitle('"run" pattern (N=8000, 3 gaps of 320 samples): every filter mode bridges the gaps\n'
                 "(grey bands = NaN runs, red = segments crossing a gap; ideal split shows the reference semantics)",
                 fontsize=10)
    fig.tight_layout(rect=[0, 0, 1, 0.93])
    fig.savefig(assets / "fig_nan_segments.png", dpi=200, bbox_inches="tight")
    plt.close(fig)
    print("fig_nan_segments.png written")


def fig_nan_frame_curves(assets, perf):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    fig, axes = plt.subplots(1, 2, figsize=(10.2, 3.8), sharey=True)
    pcts = [0.0, 1.0, 5.0, 10.0, 25.0, 50.0]
    for ax, points in zip(axes, (100000, 1000000)):
        for mode in PLOT_MODES:
            vals = []
            for pct in pcts:
                rs = _perf_lookup(perf, sweep="A", mode=mode, points=points, nan_pct=pct)
                vals.append(rs[0]["rmin"] if rs else float("nan"))
            ax.plot(pcts, vals, "o-", ms=3.5, lw=1.2, color=MODE_COLORS[mode], label=MODE_LABELS[mode])
        ax.set_yscale("log")
        ax.set_xlabel("NaN fraction (%)")
        ax.set_title("%s points, Middle case" % ("100,000" if points == 100000 else "1,000,000"),
                     fontsize=9.5, loc="left")
        ax.grid(True, which="both", lw=0.3, alpha=0.4)
        ax.tick_params(labelsize=8)
    axes[0].set_ylabel("canvas replot frame time (ms, min of 7)")
    axes[0].legend(fontsize=7.5, loc="upper right")
    fig.tight_layout()
    fig.savefig(assets / "fig_nan_frame_curves.png", dpi=200, bbox_inches="tight")
    plt.close(fig)
    print("fig_nan_frame_curves.png written")


def fig_nan_breakdown(assets, perf):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    fig, axes = plt.subplots(1, 3, figsize=(11.4, 3.4))

    # (a) boundingRect cost vs NaN fraction (mode-independent, mean +/- spread)
    ax = axes[0]
    for points, col, mk in ((100000, "#1f77b4", "o"), (1000000, "#d62728", "s")):
        pcts = [0.0, 1.0, 5.0, 10.0, 25.0, 50.0]
        means, mins, maxs = [], [], []
        for pct in pcts:
            rs = _perf_lookup(perf, sweep="A", points=points, nan_pct=pct)
            v = [r["br"] for r in rs]
            means.append(float(np.mean(v)))
            mins.append(min(v))
            maxs.append(max(v))
        ax.plot(pcts, means, mk + "-", ms=3.5, lw=1.2, color=col,
                label="%s points" % ("100k" if points == 100000 else "1M"))
        ax.fill_between(pcts, mins, maxs, color=col, alpha=0.15)
    ax.set_xlabel("NaN fraction (%)")
    ax.set_ylabel("boundingRect() cold scan (ms, min of 5)")
    ax.set_title("(a) O(n) pre-scan, mode independent", fontsize=9, loc="left")
    ax.legend(fontsize=7.5)
    ax.tick_params(labelsize=8)
    ax.grid(True, lw=0.3, alpha=0.4)

    # (b) replot 0% vs 1% NaN at 1M: the masked SIMD cliff
    ax = axes[1]
    idx = np.arange(len(PLOT_MODES))
    v0 = [_perf_lookup(perf, sweep="A", mode=m, points=1000000, nan_pct=0.0)[0]["rmin"] for m in PLOT_MODES]
    v1 = [_perf_lookup(perf, sweep="A", mode=m, points=1000000, nan_pct=1.0)[0]["rmin"] for m in PLOT_MODES]
    ax.bar(idx - 0.18, v0, 0.36, color="#4c72b0", label="0% NaN (SIMD path)")
    ax.bar(idx + 0.18, v1, 0.36, color="#dd8452", label="1% NaN (scalar fallback)")
    ax.set_yscale("log")
    ax.set_xticks(idx)
    ax.set_xticklabels([m[:6] for m in PLOT_MODES], fontsize=7.5)
    ax.set_ylabel("replot frame time (ms)")
    ax.set_title("(b) 1M points: SIMD cliff vs data reduction", fontsize=9, loc="left")
    ax.legend(fontsize=7.5)
    ax.tick_params(labelsize=8)
    ax.grid(True, axis="y", lw=0.3, alpha=0.4)

    # (c) normalized frame time vs NaN fraction at 1M
    ax = axes[2]
    pcts = [0.0, 1.0, 5.0, 10.0, 25.0, 50.0]
    for mode in PLOT_MODES:
        base = _perf_lookup(perf, sweep="A", mode=mode, points=1000000, nan_pct=0.0)[0]["rmin"]
        vals = [_perf_lookup(perf, sweep="A", mode=mode, points=1000000, nan_pct=p)[0]["rmin"] / base for p in pcts]
        ax.plot(pcts, vals, "o-", ms=3.5, lw=1.2, color=MODE_COLORS[mode], label=MODE_LABELS[mode])
    ax.axhline(1.0, color="0.4", lw=0.6, ls="--")
    ax.set_xlabel("NaN fraction (%)")
    ax.set_ylabel("frame time / frame time at 0% NaN")
    ax.set_title("(c) 1M points, normalized", fontsize=9, loc="left")
    ax.legend(fontsize=7)
    ax.tick_params(labelsize=8)
    ax.grid(True, lw=0.3, alpha=0.4)

    fig.tight_layout()
    fig.savefig(assets / "fig_nan_breakdown.png", dpi=200, bbox_inches="tight")
    plt.close(fig)
    print("fig_nan_breakdown.png written")


def fig_nan_matrix_heatmap(assets, vis):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    pats = []
    for r in vis:
        if r["pattern"] not in pats:
            pats.append(r["pattern"])
    data = np.zeros((len(pats), len(PLOT_MODES)))
    for i, pat in enumerate(pats):
        for j, mode in enumerate(PLOT_MODES):
            rs = [r for r in vis if r["pattern"] == pat and r["mode"] == mode]
            data[i, j] = 100.0 * rs[0]["diff"] / rs[0]["img"] if rs else np.nan
    fig, ax = plt.subplots(figsize=(7.8, 5.6))
    im = ax.imshow(data, cmap="YlOrRd", aspect="auto", vmin=0.0, vmax=float(np.nanmax(data)))
    for i in range(len(pats)):
        for j in range(len(PLOT_MODES)):
            ax.text(j, i, "%.2f" % data[i, j], ha="center", va="center", fontsize=7,
                    color="white" if data[i, j] > 0.6 * np.nanmax(data) else "black")
    ax.set_xticks(range(len(PLOT_MODES)))
    ax.set_xticklabels(PLOT_MODES, fontsize=8)
    ax.set_yticks(range(len(pats)))
    ax.set_yticklabels(pats, fontsize=8)
    ax.set_xlabel("paint attribute mode")
    ax.set_ylabel("NaN pattern")
    ax.set_title("Visual artifact vs ideal split rendering: differing pixels (% of 960x560 image)", fontsize=9.5)
    fig.colorbar(im, ax=ax, shrink=0.85, label="differing pixels (%)")
    fig.tight_layout()
    fig.savefig(assets / "fig_nan_matrix_heatmap.png", dpi=200, bbox_inches="tight")
    plt.close(fig)
    print("fig_nan_matrix_heatmap.png written")


def fig_nan_cases(assets, perf):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    cases = ["baseline", "leading", "leadtrail", "middle", "trailing", "xynan", "xyinter", "allnan"]
    fig, ax = plt.subplots(figsize=(10.2, 3.6))
    w = 0.16
    idx = np.arange(len(cases))
    for j, mode in enumerate(PLOT_MODES):
        vals = []
        for case in cases:
            rs = _perf_lookup(perf, sweep="B", case=case, mode=mode)
            vals.append(rs[0]["rmin"] if rs else 0.0)
        ax.bar(idx + (j - 2) * w, vals, w, color=MODE_COLORS[mode], label=MODE_LABELS[mode])
    ax.set_yscale("log")
    ax.set_xticks(idx)
    ax.set_xticklabels(cases, fontsize=8)
    ax.set_ylabel("replot frame time (ms, min of 7)")
    ax.set_title("NaN distribution cases (100k points, 10% NaN): gap position barely matters;\n"
                 "x-NaN cases differ only where they flip code paths (aggressive: orientation probe -> Vertical)",
                 fontsize=9.5, loc="left")
    ax.legend(fontsize=7.5, ncol=2)
    ax.tick_params(labelsize=8)
    ax.grid(True, axis="y", lw=0.3, alpha=0.4)
    fig.tight_layout()
    fig.savefig(assets / "fig_nan_cases.png", dpi=200, bbox_inches="tight")
    plt.close(fig)
    print("fig_nan_cases.png written")


def make_figures(assets):
    csv_path = assets / "nan_results.csv"
    if not csv_path.exists():
        print("!! %s not found - Part B skipped" % csv_path)
        return
    perf, vis, probes = load_results(csv_path)
    print_perf_tables(perf)
    print_vis_table(vis, probes)
    fig_nan_segments(assets)
    fig_nan_frame_curves(assets, perf)
    fig_nan_breakdown(assets, perf)
    fig_nan_matrix_heatmap(assets, vis)
    fig_nan_cases(assets, perf)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--assets", default=str(HERE))
    ap.add_argument("--no-figures", action="store_true")
    args = ap.parse_args()
    assets = Path(args.assets)

    run_tiny_example()
    run_semantic_matrix()
    hash_file = assets / "nan_mapper_hashes.csv"
    if hash_file.exists():
        run_crossval(hash_file)
    else:
        print("!! %s not found - cross validation skipped" % hash_file)

    if not args.no_figures:
        make_figures(assets)
    return 0


if __name__ == "__main__":
    sys.exit(main())
