"""
Numerical experiments for the curve downsampling article.

The four reduction strategies are replicated exactly as they are implemented
in src/plot/qwt_point_mapper.cpp ( same bucket arithmetic, same rounding ).
Only the fidelity is measured here - the timings quoted in the article come
from the C++ implementation ( examples/bench/renderbench ).

Metrics ( all defined on the paint device, i.e. in pixel rows ):

  * column envelope: for every pixel column c the union of the vertical
    extents of the polyline segments intersecting [c, c+1).  This is what a
    1 px wide vertical stroke covers, i.e. what the rasterizer has to paint.
  * envelope coverage: length of the intersection of the reduced and the
    reference envelope, divided by the length of the reference envelope,
    averaged over all columns that have a non empty reference envelope.
  * unsigned envelope error: mean and maximum of | max_red - max_ref | and
    | min_red - min_ref | over all columns.
"""

import numpy as np

NUM_COLS = 900
NUM_ROWS = 500


def qround(v):
    """C++ qRound: round half away from zero."""
    return np.floor(v + 0.5) if v >= 0 else np.ceil(v - 0.5)


qround_vec = np.vectorize(qround, otypes=[np.int64])


class LinearMap:
    """QwtScaleMap for a linear scale ( cnv / p1 / ts1 fast path )."""

    def __init__(self, s1, s2, p1, p2):
        self.s1, self.s2, self.p1, self.p2 = s1, s2, p1, p2
        self.cnv = (p2 - p1) / (s2 - s1)
        self.off = p1 - s1 * self.cnv

    def transform(self, v):
        return v * self.cnv + self.off


def make_maps(x, y):
    xmap = LinearMap(x[0], x[-1], 0.0, float(NUM_COLS - 1))
    ymin, ymax = float(np.min(y)), float(np.max(y))
    ymap = LinearMap(ymin, ymax, float(NUM_ROWS - 1), 0.0)
    return xmap, ymap


# ---------------------------------------------------------------- reductions

def reduce_rendered(x, y, xmap, ymap):
    """Reference rendering: the full polyline mapped to the integer pixel
    grid.  This mirrors the QWT7 on-screen pipeline ( RoundPoints ), where
    every point is quantized to its pixel column - the reference the eye
    compares the reduced polyline against."""
    return qround_vec(xmap.transform(x)), qround_vec(ymap.transform(y))


def reduce_uniform(x, y, xmap, ymap, budget):
    n = len(x)
    step = max(1, n // budget)
    idx = np.arange(0, n, step)
    if idx[-1] != n - 1:
        idx = np.append(idx, n - 1)
    return xmap.transform(x[idx]), ymap.transform(y[idx])


def reduce_lttb(x, y, xmap, ymap, budget):
    """Classic Largest-Triangle-Three-Buckets ( Steinarsson 2013 ).

    Three buckets at a time: the previous kept point, the current bucket and
    the centroid of the following bucket.
    """
    n = len(x)
    out_idx = [0]
    every = (n - 2) / (budget - 2)

    a = 0
    for i in range(budget - 2):
        # centroid of the next bucket
        avg_range_start = int(np.floor((i + 1) * every) + 1)
        avg_range_end = int(np.floor((i + 2) * every) + 1)
        avg_range_end = min(avg_range_end, n)
        avg_x = np.mean(x[avg_range_start:avg_range_end])
        avg_y = np.mean(y[avg_range_start:avg_range_end])

        # current bucket
        rng_start = int(np.floor(i * every) + 1)
        rng_end = int(np.floor((i + 1) * every) + 1)
        rng_end = min(rng_end, n)

        candidates = np.arange(rng_start, rng_end)
        areas = np.abs((x[a] - avg_x) * (y[candidates] - y[a])
                       - (x[a] - x[candidates]) * (avg_y - y[a]))
        a = int(candidates[int(np.argmax(areas))])
        out_idx.append(a)

    out_idx.append(n - 1)
    out_idx = np.array(out_idx, dtype=np.int64)[:budget]
    return xmap.transform(x[out_idx]), ymap.transform(y[out_idx])


def reduce_minmax_bucket(x, y, xmap, ymap, bucket_factor=1.0):
    """qwtMinMaxBucketReduce: numBuckets = max(2, 2 * xPixels) equal count
    buckets, the min-Y and max-Y point of every bucket ( index order )."""
    x_pixels = abs(int(qround(xmap.p2 - xmap.p1))) + 1
    num_buckets = max(2, int(2 * x_pixels * bucket_factor))
    n = len(x)

    bucket_size = n / num_buckets
    sx = qround_vec(xmap.transform(x))
    sy = qround_vec(ymap.transform(y))

    xs, ys = [], []
    for b in range(num_buckets):
        start = int(b * bucket_size)
        end = min(int((b + 1) * bucket_size) - 1, n - 1)
        if end < start:
            continue
        seg = y[start:end + 1]
        i_min = start + int(np.argmin(seg))
        i_max = start + int(np.argmax(seg))
        if i_min <= i_max:
            xs.append(sx[i_min]); ys.append(sy[i_min])
            if sy[i_max] != sy[i_min] or sx[i_max] != sx[i_min]:
                xs.append(sx[i_max]); ys.append(sy[i_max])
        else:
            xs.append(sx[i_max]); ys.append(sy[i_max])
            if sy[i_max] != sy[i_min] or sx[i_max] != sx[i_min]:
                xs.append(sx[i_min]); ys.append(sy[i_min])

    return np.array(xs, dtype=float), np.array(ys, dtype=float)


def reduce_pixel_column(x, y, xmap, ymap):
    """qwtPixelColumnReduce: first/min/max/last Y per pixel column, min and
    max placed at the fixed sub pixel positions +0.25 / +0.75."""
    x_pixels = abs(int(qround(xmap.p2 - xmap.p1))) + 1
    x_min = int(qround(xmap.p1))
    sx = qround_vec(xmap.transform(x))
    sy = qround_vec(ymap.transform(y))

    col = sx - x_min
    keep = (col >= 0) & (col < x_pixels)

    first = np.full(x_pixels, 0.0)
    last = np.full(x_pixels, 0.0)
    lo = np.full(x_pixels, 0.0)
    hi = np.full(x_pixels, 0.0)
    cnt = np.zeros(x_pixels, dtype=int)

    for c, v in zip(col[keep], sy[keep]):
        if cnt[c] == 0:
            first[c] = last[c] = lo[c] = hi[c] = v
        else:
            lo[c] = min(lo[c], v)
            hi[c] = max(hi[c], v)
            last[c] = v
        cnt[c] += 1

    xs, ys = [], []
    for c in range(x_pixels):
        if cnt[c] == 0:
            continue
        xc = float(x_min + c)
        if cnt[c] == 1:
            xs.append(xc); ys.append(first[c])
            continue
        gap_before = c > 0 and cnt[c - 1] <= 1
        gap_after = c < x_pixels - 1 and cnt[c + 1] <= 1
        if gap_before:
            xs.append(xc + 0.2); ys.append(first[c])
        xs.append(xc + 0.25); ys.append(lo[c])
        xs.append(xc + 0.75); ys.append(hi[c])
        if gap_after:
            xs.append(xc + 0.8); ys.append(last[c])

    return np.array(xs, dtype=float), np.array(ys, dtype=float)


# ------------------------------------------------------------------- metrics

def raster_envelope(xs, ys, num_cols=NUM_COLS, num_rows=NUM_ROWS, ss=8):
    """Per column [min, max] of the painted rows, computed by rasterizing
    the polyline on an 8 x 8 supersampled grid ( the reference model of an
    antialiased line renderer ).  Every segment marks the sub pixels it
    crosses; a column's envelope is the min/max of the marked sub rows.

    The supersampling removes all boundary attribution artifacts: a pixel
    column covers the half open interval [c, c+1), and a segment that ends
    exactly on the boundary contributes to the column on the other side.
    """
    w, h = num_cols * ss, num_rows * ss
    seen = np.zeros((w, h), dtype=bool)

    for i in range(len(xs) - 1):
        x0, y0, x1, y1 = xs[i], ys[i], xs[i + 1], ys[i + 1]
        if x0 == x1:
            a = int(np.clip(np.floor(x0 * ss), 0, w - 1))
            r0 = int(np.clip(np.floor(min(y0, y1) * ss), 0, h - 1))
            r1 = int(np.clip(np.ceil(max(y0, y1) * ss), 0, h - 1))
            seen[a, r0:min(r1 + 1, h)] = True
            continue

        slope = (y1 - y0) / (x1 - x0)
        s0 = max(0, int(np.floor(min(x0, x1) * ss)))
        s1 = min(w - 1, int(np.ceil(max(x0, x1) * ss)))
        for s in range(s0, s1):
            xa = max(x0, s / ss)
            xb = min(x1, (s + 1) / ss)
            if xa >= xb:
                continue
            ya, yb = y0 + (xa - x0) * slope, y0 + (xb - x0) * slope
            r0 = int(np.clip(np.floor(min(ya, yb) * ss), 0, h - 1))
            r1 = int(np.clip(np.ceil(max(ya, yb) * ss), 0, h - 1))
            seen[s, r0:min(r1 + 1, h)] = True

    lo = np.full(num_cols, np.nan)
    hi = np.full(num_cols, np.nan)
    for c in range(num_cols):
        col = seen[c * ss:(c + 1) * ss, :]
        rows = np.where(col.any(axis=0))[0]
        if rows.size:
            lo[c] = rows[0] / ss
            hi[c] = rows[-1] / ss

    return lo, hi


def compare(ref, red):
    """Envelope coverage and error against the reference envelope."""
    lo_r, hi_r = ref
    lo_d, hi_d = red

    valid = ~np.isnan(lo_r)
    span = hi_r[valid] - lo_r[valid]

    inter_lo = np.maximum(lo_r, np.where(np.isnan(lo_d), np.inf, lo_d))
    inter_hi = np.minimum(hi_r, np.where(np.isnan(hi_d), -np.inf, hi_d))
    inter = np.clip(inter_hi - inter_lo, 0.0, None)

    covered = np.zeros_like(lo_r)
    covered[valid] = inter[valid]

    with np.errstate(invalid="ignore", divide="ignore"):
        ratio = np.where(span > 0.5, covered[valid] / span, 1.0)

    err = np.zeros_like(lo_r)
    err[valid] = np.maximum(np.abs(np.nan_to_num(hi_d) - hi_r[valid]),
                            np.abs(np.nan_to_num(lo_d) - lo_r[valid]))

    return {
        "coverage": float(np.mean(ratio)),
        "coverage_min": float(np.min(ratio)),
        "err_mean": float(np.mean(err[valid])),
        "err_p99": float(np.percentile(err[valid], 99)),
        "err_max": float(np.max(err[valid])),
        "clean": float(np.mean(err[valid] <= 1.0)),
        "columns": int(np.sum(span > 0.5)),
    }


# ------------------------------------------------------------------- signals

def sig_band_limited(n, seed=1):
    """Smooth multi-component signal, all components below the pixel
    Nyquist limit of 0.5 cycles per column ( periods of 6.3 / 3.0 / 2.2
    columns on the 900 column canvas )."""
    rng = np.random.default_rng(seed)
    x = np.arange(n, dtype=float)
    y = (np.sin(x / 700.0) + 0.6 * np.sin(x / 330.0 + 1.0)
         + 0.25 * np.sin(x / 250.0 + 2.0))
    y += 0.02 * rng.standard_normal(n)
    return x, y


def sig_peaks(n, seed=2):
    rng = np.random.default_rng(seed)
    x = np.arange(n, dtype=float)

    def gauss(c, s, h):
        return h * np.exp(-0.5 * ((x - c) / s) ** 2)

    y = 0.5 * np.sin(x / 4000.0)
    y += gauss(0.15 * n, 0.004 * n, 3.0)
    y += gauss(0.32 * n, 120, 4.0)
    y += gauss(0.55 * n, 0.006 * n, 2.5)
    y += gauss(0.78 * n, 90, 5.0)
    y += 0.05 * rng.standard_normal(n)
    return x, y


def sig_noise(n, seed=3):
    rng = np.random.default_rng(seed)
    x = np.arange(n, dtype=float)
    return x, rng.standard_normal(n)


def sig_spike_pairs(n, seed=4):
    """Narrow up/down spike pairs - the case where the centroid criterion of
    the classic LTTB tends to cancel a pair out."""
    rng = np.random.default_rng(seed)
    x = np.arange(n, dtype=float)
    y = 0.3 * np.sin(x / 3000.0) + 0.02 * rng.standard_normal(n)
    for c in range(2000, n - 2000, 4000):
        y[c] += 3.5
        y[c + 2] -= 3.5
    return x, y


def sig_uneven_x(n, seed=5):
    """Narrow peaks on data with strongly uneven x spacing ( the pacing
    varies by +-40 % over a period of ~ 27 pixel columns ).  This is the
    scenario where index based bucketing beats pixel column bucketing."""
    rng = np.random.default_rng(seed)
    t = np.arange(n, dtype=float)
    x = t + 200.0 * np.sin(2.0 * np.pi * t / 3000.0)

    def gauss(c, s, h):
        return h * np.exp(-0.5 * ((t - c) / s) ** 2)

    y = 0.2 * np.sin(t / 1500.0) + 0.05 * rng.standard_normal(n)
    for c, w, h in ((0.22 * n, 60, 3.0), (0.41 * n, 40, 3.5),
                    (0.63 * n, 55, 2.8), (0.85 * n, 35, 4.0)):
        y += gauss(c, w, h)
    return x, y


SIGNALS = {
    "band limited": sig_band_limited,
    "peaks + noise": sig_peaks,
    "spike pairs": sig_spike_pairs,
    "uneven x": sig_uneven_x,
    "white noise": sig_noise,
}


def run(n=100000, budget=None):
    out = {}
    for name, fn in SIGNALS.items():
        x, y = fn(n)
        xmap, ymap = make_maps(x, y)
        budget = budget or 4 * (abs(int(qround(xmap.p2 - xmap.p1))) + 1)

        ref = raster_envelope(*reduce_rendered(x, y, xmap, ymap))
        algos = {
            "rendered (reference)": reduce_rendered(x, y, xmap, ymap),
            "uniform stride": reduce_uniform(x, y, xmap, ymap, budget),
            "LTTB (classic)": reduce_lttb(x, y, xmap, ymap, budget),
            "MinMax bucket": reduce_minmax_bucket(x, y, xmap, ymap),
            "pixel column": reduce_pixel_column(x, y, xmap, ymap),
        }
        rows = {}
        for algo, (xs, ys) in algos.items():
            m = compare(ref, raster_envelope(xs, ys))
            m["points"] = len(xs)
            rows[algo] = m
        out[name] = rows
    return out


if __name__ == "__main__":
    res = run()
    for sig, rows in res.items():
        print(f"\n=== {sig} (N = 100000, canvas {NUM_COLS}x{NUM_ROWS}) ===")
        print(f"{'algorithm':<18} {'points':>7} {'coverage':>9} {'worst col':>10} "
              f"{'err mean':>9} {'err p99':>8} {'err max':>8} {'clean':>7}")
        for algo, m in rows.items():
            print(f"{algo:<18} {m['points']:>7} {m['coverage']:>9.4f} "
                  f"{m['coverage_min']:>10.4f} {m['err_mean']:>9.3f} "
                  f"{m['err_p99']:>8.3f} {m['err_max']:>8.3f} {m['clean']:>7.3f}")
