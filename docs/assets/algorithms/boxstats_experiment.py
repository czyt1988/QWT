"""
Numerical experiments for the boxplot quantile article.

    python boxstats_experiment.py [cpp_out.txt]

Replicates the QwtBoxStatisticsCalculator pipeline (src/core/qwt_box_statistics.cpp)
exactly in Python, implements all nine R quantile types (Hyatt & Sackrow 2003),
and runs:

  A  estimator identity check: which R type does QWT implement?
  B  spread of Q1/median/Q3 across the nine types vs sample size
  C  outlier-count sensitivity + minimum-n feasibility for the 1.5*IQR rule
  D  P-square streaming quantile error vs exact (sorted) quantiles
  E  figures (fig_box_anatomy / fig_box_spread / fig_box_outliers / fig_box_p2)

If a path to the C++ harness output (cpp_out.txt from boxstats_check.cpp) is
given as argv[1], every case is recomputed here and compared against the real
QwtBoxStatisticsCalculator output.

The case file for the harness (boxstats_cases.txt) is always written next to
this script.  Only numpy + matplotlib are used; the seed is fixed everywhere.
No wall-clock timing is performed.
"""

import math
import sys
from pathlib import Path

import numpy as np

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle as MplRectangle

HERE = Path(__file__).parent

SEED_A = 20251012   # identity check / small tables
SEED_B = 424242     # estimator spread Monte Carlo
SEED_C = 777        # outlier sensitivity fixtures
SEED_D = 9999       # P-square streams
SEED_E = 31337      # anatomy figure + C++ harness cases

P25, P50, P75 = 0.25, 0.5, 0.75

# WhiskerMethod enum values, mirroring qwt_box_statistics.h
T_TUKEY, T_PERCENTILE, T_MINMAX, T_SD, T_SE = 0, 1, 2, 3, 4


# ------------------------------------------------------- QWT source replicas

def qwt_quantile(s, p):
    """Exact port of QwtBoxStatisticsCalculator::quantile().

    index = p*(n-1); linear interpolation between floor/ceil ranks.
    This is the R type 7 formula in 0-based indexing."""
    n = len(s)
    if n == 0:
        return 0.0
    index = p * (n - 1)
    lower = int(math.floor(index))
    upper = int(math.ceil(index))
    if lower == upper:
        return float(s[lower])
    frac = index - lower
    return float(s[lower] + frac * (s[upper] - s[lower]))


def qwt_mean(v):
    """Port of QwtBoxStatisticsCalculator::mean() (nan/inf skipped)."""
    acc = 0.0
    cnt = 0
    for x in v:
        if not (math.isnan(x) or math.isinf(x)):
            acc += x
            cnt += 1
    return acc / cnt if cnt > 0 else float("nan")


def qwt_std(v):
    """Port of QwtBoxStatisticsCalculator::standardDeviation() (ddof = 1)."""
    if len(v) < 2:
        return 0.0
    acc = 0.0
    cnt = 0
    for x in v:
        if not (math.isnan(x) or math.isinf(x)):
            acc += x
            cnt += 1
    if cnt < 2:
        return 0.0 if cnt == 0 else 0.0
    m = acc / cnt
    if cnt == 0:
        return float("nan")
    sq = 0.0
    for x in v:
        if math.isnan(x) or math.isinf(x):
            continue
        d = x - m
        sq += d * d
    return math.sqrt(sq / (cnt - 1))


def qwt_calculate(s, method=T_TUKEY, coeff=1.5):
    """Port of QwtBoxStatisticsCalculator::calculate() on *sorted, filtered*
    data.  The Percentile branch reproduces the source verbatim (since
    v7.5.1 the lower whisker is the c-th percentile, the upper one the
    (100 - c)-th; before, the two levels were exchanged)."""
    n = len(s)
    if n == 0:
        return dict(wl=0.0, q1=0.0, med=0.0, q3=0.0, wu=0.0, count=0, outliers=[])
    minv, maxv = float(s[0]), float(s[-1])
    med = qwt_quantile(s, P50)
    q1 = qwt_quantile(s, P25)
    q3 = qwt_quantile(s, P75)

    if method == T_TUKEY:
        iqr = q3 - q1
        wl = max(minv, q1 - coeff * iqr)
        wu = min(maxv, q3 + coeff * iqr)
    elif method == T_PERCENTILE:
        # source: whiskerLower = quantile(c/100), whiskerUpper = quantile(1 - c/100)
        wl = qwt_quantile(s, coeff / 100.0)
        wu = qwt_quantile(s, 1.0 - coeff / 100.0)
    elif method == T_MINMAX:
        wl, wu = minv, maxv
    elif method == T_SD:
        m = qwt_mean(s)
        sd = qwt_std(s)
        wl = max(minv, m - coeff * sd)
        wu = min(maxv, m + coeff * sd)
    elif method == T_SE:
        m = qwt_mean(s)
        sd = qwt_std(s)
        se = sd / math.sqrt(float(n))
        wl = max(minv, m - coeff * se)
        wu = min(maxv, m + coeff * se)
    else:
        wl, wu = minv, maxv

    outliers = [float(x) for x in s
                if not (math.isnan(x) or math.isinf(x)) and (x < wl or x > wu)]
    return dict(wl=wl, q1=q1, med=med, q3=q3, wu=wu,
                count=len(outliers), outliers=outliers)


def qwt_from_raw(raw, method=T_TUKEY, coeff=1.5):
    """Port of calculateFromRaw(): filter nan/inf, sort, calculate."""
    filtered = [float(x) for x in raw if not (math.isnan(x) or math.isinf(x))]
    filtered.sort()
    return qwt_calculate(filtered, method, coeff)


# ------------------------------------------------------ R types 1..9 (H&S03)

def r_quantile(s, p, rtype):
    """Hyatt & Sackrow (2003) quantile types 1..9 on sorted data s (0-based)."""
    n = len(s)
    if n == 0:
        return float("nan")
    if p <= 0.0:
        return float(s[0])
    if p >= 1.0:
        return float(s[-1])

    if rtype == 1:
        k = int(math.ceil(n * p))
        k = max(k, 1)
        return float(s[k - 1])

    if rtype == 2:
        h = n * p
        k = int(math.ceil(h))
        if h == int(h):
            k = int(h)
            if k >= n:
                return float(s[-1])
            return float(0.5 * (s[k - 1] + s[k]))
        return float(s[k - 1])

    if rtype == 3:
        h = n * p
        k = int(math.floor(h + 0.5))
        if h + 0.5 == k:            # exact tie -> nearest even rank
            if k % 2 != 0:
                k -= 1
        k = min(max(k, 1), n)
        return float(s[k - 1])

    # types 4..9: h(p) formulas, then linear interpolation of order statistics
    if rtype == 4:
        h = n * p
    elif rtype == 5:
        h = n * p + 0.5
    elif rtype == 6:
        h = (n + 1.0) * p
    elif rtype == 7:
        h = (n - 1.0) * p + 1.0
    elif rtype == 8:
        h = (n + 1.0 / 3.0) * p + 1.0 / 3.0
    elif rtype == 9:
        h = (n + 0.25) * p + 0.375
    else:
        raise ValueError(rtype)

    j = int(math.floor(h))
    g = h - j
    if j < 1:
        return float(s[0])
    if j >= n:
        return float(s[-1])
    return float((1.0 - g) * s[j - 1] + g * s[j])


def quantile_coefs(n, p, rtype):
    """Coefficients a_i (1-based) with q(p) = sum a_i x_i for the given type.
    Used for the exact LP/gap analysis of outlier feasibility."""
    a = np.zeros(n)
    if rtype == 1:
        k = min(max(int(math.ceil(n * p)), 1), n)
        a[k - 1] = 1.0
    elif rtype == 2:
        h = n * p
        if h == int(h):
            k = int(h)
            if k >= n:
                a[n - 1] = 1.0
            else:
                a[k - 1] = a[k] = 0.5
        else:
            k = min(max(int(math.ceil(h)), 1), n)
            a[k - 1] = 1.0
    elif rtype == 3:
        h = n * p
        k = int(math.floor(h + 0.5))
        if h + 0.5 == k and k % 2 != 0:
            k -= 1
        k = min(max(k, 1), n)
        a[k - 1] = 1.0
    else:
        hh = {4: n * p,
              5: n * p + 0.5,
              6: (n + 1.0) * p,
              7: (n - 1.0) * p + 1.0,
              8: (n + 1.0 / 3.0) * p + 1.0 / 3.0,
              9: (n + 0.25) * p + 0.375}[rtype]
        j = int(math.floor(hh))
        g = hh - j
        if j < 1:
            a[0] = 1.0
        elif j >= n:
            a[n - 1] = 1.0
        else:
            a[j - 1] += 1.0 - g
            a[j] += g
    return a


def min_n_feasible(rtype, coeff=1.5, nmax=24):
    """Smallest n for which the largest observation CAN be flagged as an
    outlier under the Tukey fence built from this quantile type.

    Exact argument: fence(x) = (1+c)*q3(x) - c*q1(x) is a fixed linear form
    in the order statistics; x_n > fence is c*x > 0 with sum(c)=0, hence
    translation invariant.  Writing x_i as cumulative sums of nonnegative
    gaps g_j gives c*x = sum_j d_j g_j, feasible iff some d_j > 0."""
    for n in range(2, nmax + 1):
        c3 = quantile_coefs(n, P75, rtype)
        c1 = quantile_coefs(n, P25, rtype)
        f = (1.0 + coeff) * c3 - coeff * c1
        cvec = np.zeros(n)
        cvec[n - 1] = 1.0
        cvec -= f
        # x_i = sum_{j<i} g_j  ->  d_j = sum_{i>j} c_i
        d = np.array([cvec[j + 1:].sum() for j in range(n - 1)])
        if d.max() > 1e-12:
            g = np.zeros(n - 1)
            g[int(d.argmax())] = 1.0
            x = np.concatenate([[0.0], np.cumsum(g)])
            return n, x
    return None, None


def brute_force_check(rtype, n, trials, rng, coeff=1.5):
    """Random search for an outlier configuration at fixed (type, n).
    Returns (found, dataset)."""
    for _ in range(trials):
        mode = rng.integers(0, 3)
        if mode == 0:
            x = np.sort(rng.uniform(0.0, 1.0, n))
        elif mode == 1:
            x = np.sort(np.cumsum(rng.exponential(rng.uniform(0.1, 30.0), n)))
        else:
            x = np.sort(np.concatenate([rng.uniform(0, 1, max(1, n // 2)),
                                        rng.uniform(1.0, 1e4, n - max(1, n // 2))]))
        q3 = r_quantile(x, P75, rtype)
        q1 = r_quantile(x, P25, rtype)
        if x[-1] > q3 + coeff * (q3 - q1) or x[0] < q1 - coeff * (q3 - q1):
            return True, x
    return False, None


# ------------------------------------------------------------- distributions

def make_dist(name, size, rng):
    if name == "normal":
        return rng.standard_normal(size)
    if name == "lognormal":
        return rng.lognormal(0.0, 1.0, size)
    if name == "t3":
        return rng.standard_t(3, size)
    if name == "bimodal":
        left = rng.random(size) < 0.5
        x = np.where(left, rng.normal(-2.0, 0.5, size), rng.normal(2.0, 0.5, size))
        return x
    raise ValueError(name)


DIST_NAMES = ["normal", "lognormal", "t3", "bimodal"]
NSIZES = [8, 16, 32, 64, 256]


# ============================================================== experiment A

def exp_a_identity():
    print("=" * 78)
    print("A. Which R type does QwtBoxStatisticsCalculator implement?")
    print("=" * 78)
    rng = np.random.default_rng(SEED_A)
    maxdiff = {t: 0.0 for t in range(1, 10)}
    ndiff = {t: 0 for t in range(1, 10)}
    cases = 0
    for n in list(range(2, 40)) + [64, 128, 256]:
        for _ in range(120):
            mode = rng.integers(0, 3)
            if mode == 0:
                x = rng.standard_normal(n)
            elif mode == 1:
                x = np.round(rng.standard_normal(n), 0)      # heavy ties
            else:
                x = rng.uniform(-5, 5, n)
            s = np.sort(x)
            for p in (P25, P50, P75, 0.05, 0.95, 0.1, 0.9):
                qwt = qwt_quantile(s, p)
                cases += 1
                for t in range(1, 10):
                    d = abs(r_quantile(s, p, t) - qwt)
                    if d > maxdiff[t]:
                        maxdiff[t] = d
                    if d > 1e-12:
                        ndiff[t] += 1
    print(f"probe quantiles compared: {cases}")
    for t in range(1, 10):
        tag = "  <-- EXACT MATCH" if ndiff[t] == 0 else ""
        print(f"  R-{t}: max|diff| = {maxdiff[t]:.6g}, mismatches = {ndiff[t]}{tag}")

    # hand-checkable small tables
    for data in ([1, 2, 3, 4, 5, 6, 7, 8], [1, 2, 3, 4, 5, 6, 7, 8, 9],
                 [1, 1, 2, 3, 5, 8, 13]):
        s = np.array(data, dtype=float)
        print(f"\nsorted data {data} (n={len(s)}):")
        hdr = "  {:6s} {:>10s} {:>10s} {:>10s}".format("type", "Q1", "median", "Q3")
        print(hdr)
        for t in range(1, 10):
            print("  R-{:<4d} {:>10.4f} {:>10.4f} {:>10.4f}".format(
                t, r_quantile(s, P25, t), r_quantile(s, P50, t), r_quantile(s, P75, t)))
        print("  {:6s} {:>10.4f} {:>10.4f} {:>10.4f}".format(
            "QWT", qwt_quantile(s, P25), qwt_quantile(s, P50), qwt_quantile(s, P75)))


# ============================================================== experiment B

def exp_b_spread(trials=1000):
    print()
    print("=" * 78)
    print("B. Q1/median/Q3 spread across R-1..R-9 (normalized by the R-7 IQR)")
    print("=" * 78)
    rng = np.random.default_rng(SEED_B)
    results = {}
    for dist in DIST_NAMES:
        results[dist] = {}
        for n in NSIZES:
            acc = {"q1": [], "med": [], "q3": []}
            for _ in range(trials):
                x = np.sort(make_dist(dist, n, rng))
                iqr = qwt_quantile(x, P75) - qwt_quantile(x, P25)
                if iqr <= 0:
                    continue
                for key, p in (("q1", P25), ("med", P50), ("q3", P75)):
                    vals = [r_quantile(x, p, t) for t in range(1, 10)]
                    acc[key].append((max(vals) - min(vals)) / iqr)
            results[dist][n] = {k: np.asarray(v) for k, v in acc.items()}
        print(f"\ndistribution: {dist}  ({trials} Monte Carlo trials per cell)")
        print("  {:>4s} | {:>26s} | {:>26s} | {:>26s}".format(
            "n", "Q1 spread mean/p95", "median spread mean/p95", "Q3 spread mean/p95"))
        for n in NSIZES:
            row = []
            for key in ("q1", "med", "q3"):
                a = results[dist][n][key]
                row.append(f"{a.mean():.4f}/{np.percentile(a, 95):.4f}")
            print(f"  {n:>4d} | {row[0]:>26s} | {row[1]:>26s} | {row[2]:>26s}")
        # decay slope on log-log
        for key in ("q1", "med", "q3"):
            m = np.array([results[dist][n][key].mean() for n in NSIZES])
            slope = np.polyfit(np.log(NSIZES), np.log(m), 1)[0]
            print(f"  decay slope ({key:>3s}): spread ~ n^({slope:+.3f})")
    return results


# ============================================================== experiment C

def exp_c_outliers():
    print()
    print("=" * 78)
    print("C1. Outlier counts vs quantile type, 1.5*IQR fence, fixed datasets")
    print("=" * 78)
    rng = np.random.default_rng(SEED_C)
    setA = np.concatenate([rng.standard_normal(64), [6.5, 7.0, -6.0]])
    setB = rng.lognormal(0.0, 1.0, 32)
    fixtures = {"A: normal(64) + {6.5, 7.0, -6.0} (n=67)": setA,
                "B: lognormal(0,1), n=32": setB}
    for name, x in fixtures.items():
        s = np.sort(x)
        print(f"\ndataset {name}")
        for t in range(1, 10):
            q1 = r_quantile(s, P25, t)
            q3 = r_quantile(s, P75, t)
            iqr = q3 - q1
            lo, hi = q1 - 1.5 * iqr, q3 + 1.5 * iqr
            cnt = int(np.sum((s < lo) | (s > hi)))
            print(f"  R-{t}: Q1={q1:+.4f} Q3={q3:+.4f} fence=[{lo:+.4f},{hi:+.4f}] outliers={cnt}")
        st = qwt_from_raw(x)
        print(f"  QWT : Q1={st['q1']:+.4f} Q3={st['q3']:+.4f} "
              f"fence=[{st['wl']:+.4f},{st['wu']:+.4f}] outliers={st['count']}")

    print()
    print("=" * 78)
    print("C2. Minimum sample size for which an outlier is possible (Tukey 1.5)")
    print("=" * 78)
    rng = np.random.default_rng(SEED_C + 1)
    for t in range(1, 10):
        n, witness = min_n_feasible(t)
        s = np.sort(witness)
        q1 = r_quantile(s, P25, t)
        q3 = r_quantile(s, P75, t)
        fence = q3 + 1.5 * (q3 - q1)
        ok_bf, _ = brute_force_check(t, n, 400, rng)
        ok_below = True
        if n > 2:
            found, _ = brute_force_check(t, n - 1, 2000, rng)
            ok_below = not found
        print(f"  R-{t}: min n = {n:>2d} | witness {np.array2string(s, precision=4)} "
              f"| upper fence = {fence:.4f} < x_max = {s[-1]:.4f} "
              f"| brute force confirms n: {ok_bf}, n-1 infeasible: {ok_below}")

    print()
    print("QWT (R-7) boundary cases, computed with the exact source semantics:")
    for data in ([0.0, 1.0, 2.0], [0.0, 1.0, 2.0, 100.0], [0.0, 1.0, 2.0, 3.0, 100.0]):
        st = qwt_from_raw(data)
        print(f"  n={len(data)} {data} -> Q1={st['q1']:.4f} Q3={st['q3']:.4f} "
              f"IQR={st['q3'] - st['q1']:.4f} fence=[{st['wl']:.4f},{st['wu']:.4f}] "
              f"outliers={st['count']} {st['outliers']}")
    # analytic proof data for n<=3 (R-7): fence - x_n = (x2 + 0.5*x3 - 1.5*x1)/2 >= 0
    print("  n=3, R-7: fence(x3)-x3 = (x2 + 0.5*x3 - 1.5*x1)/2; with x1<=x2<=x3 this is")
    print("          >= (0.5*x3 - 0.5*x1)/2 >= 0  ->  no upper outlier is possible.")

    print()
    print("=" * 78)
    print("C3. Ties and IQR = 0 (QWT semantics)")
    print("=" * 78)
    for data in ([5.0] * 6 + [12.0], [1.0] * 5 + [5.0, 9.0], [3.0] * 7):
        st = qwt_from_raw(data)
        print(f"  {data} -> Q1={st['q1']:.3f} Q3={st['q3']:.3f} IQR={st['q3'] - st['q1']:.3f} "
              f"whiskers=[{st['wl']:.3f},{st['wu']:.3f}] outliers={st['count']} {st['outliers']}")

    print()
    print("Whisker tip convention: QWT draws the whisker AT the fence, R at the")
    print("most extreme data point inside the fence.  Fixed dataset A (n=67):")
    s = np.sort(setA)
    st = qwt_from_raw(setA)
    r_tip_hi = float(s[s <= st["q3"] + 1.5 * (st["q3"] - st["q1"])].max())
    r_tip_lo = float(s[s >= st["q1"] - 1.5 * (st["q3"] - st["q1"])].min())
    print(f"  QWT whiskerUpper = {st['wu']:.6f} (fence), R convention tip = {r_tip_hi:.6f} "
          f"(gap {st['wu'] - r_tip_hi:.6f})")
    print(f"  QWT whiskerLower = {st['wl']:.6f} (fence), R convention tip = {r_tip_lo:.6f} "
          f"(gap {r_tip_lo - st['wl']:.6f})")
    return setA, setB


# ============================================================== experiment D

class P2Quantile:
    """The P-square algorithm (Jain & Chlamtac 1985) for one probability p."""

    def __init__(self, p):
        self.p = p
        self.q = []
        self.n = None
        self.np = None
        self.dn = [0.0, p / 2.0, p, (1.0 + p) / 2.0, 1.0]

    def add(self, x):
        if len(self.q) < 5:
            self.q.append(x)
            if len(self.q) == 5:
                self.q.sort()
                self.n = [1, 2, 3, 4, 5]
                p = self.p
                self.np = [1.0, 1.0 + 2.0 * p, 1.0 + 4.0 * p, 3.0 + 2.0 * p, 5.0]
            return
        q, n = self.q, self.n
        if x < q[0]:
            q[0] = x
            k = 0
        elif x >= q[4]:
            q[4] = x
            k = 3
        else:
            k = 0
            for i in range(3, -1, -1):
                if q[i] <= x:
                    k = i
                    break
        for i in range(k + 1, 5):
            n[i] += 1
        for i in range(5):
            self.np[i] += self.dn[i]
        for i in (1, 2, 3):
            d = self.np[i] - n[i]
            if (d >= 1.0 and n[i + 1] - n[i] > 1) or (d <= -1.0 and n[i - 1] - n[i] < -1):
                sgn = 1 if d >= 0 else -1
                par = q[i] + sgn / (n[i + 1] - n[i - 1]) * (
                    (n[i] - n[i - 1] + sgn) * (q[i + 1] - q[i]) / (n[i + 1] - n[i])
                    + (n[i + 1] - n[i] - sgn) * (q[i] - q[i - 1]) / (n[i] - n[i - 1]))
                if q[i - 1] < par < q[i + 1]:
                    q[i] = par
                else:
                    j = i + sgn
                    q[i] = q[i] + sgn * (q[j] - q[i]) / (n[j] - n[i])
                n[i] += sgn

    def result(self):
        if len(self.q) >= 5:
            return self.q[2]
        # fewer than 5 observations: P2 undefined, fall back to exact R-7
        s = sorted(self.q)
        if not s:
            return float("nan")
        return qwt_quantile(s, self.p)


def exp_d_p2():
    print()
    print("=" * 78)
    print("D. P-square streaming quantiles vs exact (sorted) R-7 quantiles")
    print("=" * 78)
    lengths = [1000, 10000, 100000]
    out = {}
    for dist in ("normal", "lognormal"):
        print(f"\nstream: {dist}, 5 independent runs per length (mean |err| / sample IQR)")
        rng = np.random.default_rng(SEED_D)
        for L in lengths:
            errs = {p: [] for p in (P25, P50, P75)}
            abserr = {p: [] for p in (P25, P50, P75)}
            for _ in range(5):
                stream = make_dist(dist, L, rng)
                exact = {p: float(np.percentile(stream, p * 100)) for p in (P25, P50, P75)}
                iqr = exact[P75] - exact[P25]
                est = {p: P2Quantile(p) for p in (P25, P50, P75)}
                for x in stream:
                    for p in est:
                        est[p].add(float(x))
                for p in (P25, P50, P75):
                    e = est[p].result() - exact[p]
                    errs[p].append(abs(e) / iqr)
                    abserr[p].append(abs(e))
            out[(dist, L)] = (errs, abserr)
            row = " | ".join(f"p{int(p * 100)}: {np.mean(errs[p]):.5f} (abs {np.mean(abserr[p]):.5f})"
                             for p in (P25, P50, P75))
            print(f"  n={L:>6d}: {row}")
    # short-stream behaviour
    rng = np.random.default_rng(SEED_D + 1)
    tiny = rng.standard_normal(4)
    e = P2Quantile(0.5)
    for x in tiny:
        e.add(float(x))
    print(f"\nstream of 4 values {np.array2string(tiny, precision=4)}: P2 has no markers yet;")
    print(f"  fallback exact median = {e.result():.6f} (numpy median = {np.median(tiny):.6f})")
    return out


# ============================================================== experiment E

def fig_anatomy(setA):
    """Annotated box anatomy: QWT fence whiskers vs R data-point whiskers."""
    st = qwt_from_raw(setA)
    s = np.sort(setA)
    q1, q3, med = st["q1"], st["q3"], st["med"]
    iqr = q3 - q1
    fhi, flo = q3 + 1.5 * iqr, q1 - 1.5 * iqr
    tip_hi = float(s[s <= fhi].max())
    tip_lo = float(s[s >= flo].min())
    outliers = st["outliers"]

    fig, axes = plt.subplots(1, 2, figsize=(10.4, 5.4), sharey=True)
    box_w = 0.34          # boxExtent-style width in position units
    cap_w = 0.3 * box_w   # capWidth = 0.3 * boxWidth, per qwt_plot_boxchart.cpp

    jitter = np.random.default_rng(11).uniform(-0.14, 0.14, len(s))

    for ax, (wl, wu, title) in zip(axes, [
            (st["wl"], st["wu"], "QWT convention: whisker at the fence"),
            (tip_lo, tip_hi, "R / classic Tukey: whisker at a data point")]):
        ax.plot(jitter + 1.0, s, ".", ms=3.5, color="#b8bcc4", zorder=1)
        ax.add_patch(MplRectangle((1.0 - box_w / 2, q1), box_w, q3 - q1,
                                  facecolor="#1f77b4", alpha=0.25, edgecolor="#1f77b4", lw=1.4, zorder=2))
        ax.plot([1.0 - box_w / 2, 1.0 + box_w / 2], [med, med], color="#111111", lw=2.0, zorder=3)
        ax.plot([1.0, 1.0], [q3, wu], color="#1f77b4", lw=1.2, zorder=3)
        ax.plot([1.0, 1.0], [q1, wl], color="#1f77b4", lw=1.2, zorder=3)
        ax.plot([1.0 - cap_w / 2, 1.0 + cap_w / 2], [wu, wu], color="#1f77b4", lw=1.2, zorder=3)
        ax.plot([1.0 - cap_w / 2, 1.0 + cap_w / 2], [wl, wl], color="#1f77b4", lw=1.2, zorder=3)
        for v in outliers:
            ax.plot([1.0], [v], "x", ms=7, color="#d62728", mew=1.6, zorder=4)
        ax.set_xlim(0.4, 1.75)
        ax.set_xticks([1.0])
        ax.set_xticklabels(["position"])
        ax.set_title(title, fontsize=10)
        ax.spines["top"].set_visible(False)
        ax.spines["right"].set_visible(False)

    ax = axes[0]
    kw = dict(fontsize=9, ha="left")
    ax.annotate("whiskerUpper\n= min(max, Q3 + 1.5*IQR)", (1.02, st["wu"]), (1.16, st["wu"] + 0.35), **kw,
                arrowprops=dict(arrowstyle="-", color="#666666", lw=0.8))
    ax.annotate("q3", (1.0 - box_w / 2, q3), (0.44, q3 + 0.1), **kw,
                arrowprops=dict(arrowstyle="-", color="#666666", lw=0.8))
    ax.annotate("median", (1.0 - box_w / 2, med), (0.42, med), **kw,
                arrowprops=dict(arrowstyle="-", color="#666666", lw=0.8))
    ax.annotate("q1", (1.0 - box_w / 2, q1), (0.44, q1 - 0.35), **kw,
                arrowprops=dict(arrowstyle="-", color="#666666", lw=0.8))
    ax.annotate("whiskerLower\n= max(min, Q1 - 1.5*IQR)", (1.02, st["wl"]), (1.16, st["wl"] - 0.9), **kw,
                arrowprops=dict(arrowstyle="-", color="#666666", lw=0.8))
    ax.annotate(f"outliers (QwtBoxOutlierSample.values),\noutlierCount = {st['count']}",
                (1.0, max(outliers)), (1.14, max(outliers) - 1.9), **kw,
                arrowprops=dict(arrowstyle="-", color="#d62728", lw=0.8))
    ax.axhline(fhi, color="#999999", ls=":", lw=0.9)
    ax.text(0.42, fhi + 0.08, "Q3 + 1.5*IQR (fence)", fontsize=8, color="#777777")
    axes[1].axhline(tip_hi, color="#2ca02c", ls="--", lw=0.9)
    axes[1].text(1.30, tip_hi + 0.08, f"largest x <= fence\n({tip_hi:.3f})", fontsize=8, color="#2ca02c")
    axes[0].set_ylabel("value")
    fig.suptitle("QwtBoxSample fields vs rendered elements (dataset A, n=67, R-7 quartiles)", fontsize=11)
    fig.tight_layout(rect=[0, 0, 1, 0.95])
    fig.savefig(HERE / "fig_box_anatomy.png", dpi=200, bbox_inches="tight")
    plt.close(fig)
    print(f"\nfig_box_anatomy.png written (whiskerUpper QWT={st['wu']:.4f} vs R tip={tip_hi:.4f}, "
          f"whiskerLower QWT={st['wl']:.4f} vs R tip={tip_lo:.4f})")


def fig_spread(results):
    fig, axes = plt.subplots(2, 2, figsize=(10.0, 6.4), sharex=True)
    colors = {"q1": "#1f77b4", "med": "#d62728", "q3": "#2ca02c"}
    labels = {"q1": "Q1 spread", "med": "median spread", "q3": "Q3 spread"}
    for ax, dist in zip(axes.flat, DIST_NAMES):
        for key in ("q1", "med", "q3"):
            m = [results[dist][n][key].mean() for n in NSIZES]
            ax.plot(NSIZES, m, "o-", ms=4, lw=1.4, color=colors[key], label=labels[key])
            slope = np.polyfit(np.log(NSIZES), np.log(m), 1)[0]
            ax.text(0.98, 0.95 if key == "q1" else (0.84 if key == "med" else 0.73),
                    f"$\\alpha$={-slope:.2f}" if key == "q1" else f"{-slope:.2f}",
                    transform=ax.transAxes, ha="right", fontsize=8.5, color=colors[key])
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xticks(NSIZES)
        ax.set_xticklabels([str(n) for n in NSIZES], fontsize=8)
        ax.set_title(dist, fontsize=10)
        ax.set_ylabel("max-min spread / IQR")
        ax.grid(alpha=0.25, which="both")
        ax.spines["top"].set_visible(False)
        ax.spines["right"].set_visible(False)
    axes.flat[0].legend(fontsize=8.5, frameon=False)
    fig.suptitle("Spread of Q1/median/Q3 across the nine R quantile types\n"
                 "(normalized by the sample IQR, mean over 1000 trials; annotated: decay exponent)",
                 fontsize=11)
    fig.tight_layout(rect=[0, 0, 1, 0.92])
    fig.savefig(HERE / "fig_box_spread.png", dpi=200, bbox_inches="tight")
    plt.close(fig)
    print("fig_box_spread.png written")


def fig_outliers(setA, setB):
    fig, axes = plt.subplots(1, 3, figsize=(12.4, 3.6))
    types = np.arange(1, 10)

    for ax, x, title in ((axes[0], setA, "dataset A (normal + 3 injected, n=67)"),
                         (axes[1], setB, "dataset B (lognormal, n=32)")):
        s = np.sort(x)
        counts = []
        for t in types:
            q1 = r_quantile(s, P25, t)
            q3 = r_quantile(s, P75, t)
            counts.append(int(np.sum((s < q1 - 1.5 * (q3 - q1)) | (s > q3 + 1.5 * (q3 - q1)))))
        cols = ["#d62728" if t == 7 else "#1f77b4" for t in types]
        ax.bar(types, counts, color=cols)
        for i, c in enumerate(counts):
            ax.text(i + 1, c + 0.04, str(c), ha="center", fontsize=8.5)
        ax.set_xlabel("R quantile type")
        ax.set_ylabel("outlier count (1.5 x IQR fence)")
        ax.set_title(title, fontsize=9.5)
        ax.set_xticks(types)
        ax.spines["top"].set_visible(False)
        ax.spines["right"].set_visible(False)

    mins = [min_n_feasible(t)[0] for t in types]
    cols = ["#d62728" if t == 7 else "#7f7f7f" for t in types]
    axes[2].bar(types, mins, color=cols)
    for i, m in enumerate(mins):
        axes[2].text(i + 1, m + 0.06, str(m), ha="center", fontsize=8.5)
    axes[2].set_xlabel("R quantile type")
    axes[2].set_ylabel("smallest n admitting an outlier")
    axes[2].set_title("minimum n for the 1.5 x IQR rule to fire\n(red = R-7 = QWT)", fontsize=9.5)
    axes[2].set_xticks(types)
    axes[2].set_ylim(0, max(mins) + 1)
    axes[2].spines["top"].set_visible(False)
    axes[2].spines["right"].set_visible(False)

    fig.tight_layout()
    fig.savefig(HERE / "fig_box_outliers.png", dpi=200, bbox_inches="tight")
    plt.close(fig)
    print("fig_box_outliers.png written (min n per type:", mins, ")")


def fig_p2(out):
    lengths = [1000, 10000, 100000]
    fig, ax = plt.subplots(figsize=(7.2, 3.8))
    styles = {(P25, "normal"): ("#1f77b4", "o-"), (P50, "normal"): ("#1f77b4", "s-"),
              (P75, "normal"): ("#1f77b4", "^-"),
              (P25, "lognormal"): ("#d62728", "o--"), (P50, "lognormal"): ("#d62728", "s--"),
              (P75, "lognormal"): ("#d62728", "^--")}
    for dist in ("normal", "lognormal"):
        for p in (P25, P50, P75):
            vals = [np.mean(out[(dist, L)][0][p]) for L in lengths]
            c, ls = styles[(p, dist)]
            ax.plot(lengths, vals, ls, ms=5, lw=1.3, color=c, label=f"{dist}, p={p}")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("stream length n")
    ax.set_ylabel("mean |P2 estimate - exact R-7| / IQR")
    ax.set_title("P-square streaming error vs exact sorted quantiles", fontsize=10.5)
    ax.legend(fontsize=8, frameon=False, ncol=2)
    ax.grid(alpha=0.25, which="both")
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    fig.tight_layout()
    fig.savefig(HERE / "fig_box_p2.png", dpi=200, bbox_inches="tight")
    plt.close(fig)
    print("fig_box_p2.png written")


# ================================================== C++ harness I/O + check

def write_cases(path):
    rng = np.random.default_rng(SEED_E)
    cases = []

    def add(cid, method, coeff, vals):
        cases.append((cid, method, coeff, list(vals)))

    add("norm64_a", T_TUKEY, 1.5, rng.normal(50, 10, 64))
    add("norm64_b", T_TUKEY, 1.5, rng.standard_normal(64))
    add("norm5", T_TUKEY, 1.5, rng.standard_normal(5))
    add("norm8", T_TUKEY, 1.5, rng.standard_normal(8))
    add("norm256", T_TUKEY, 1.5, rng.standard_normal(256))
    add("lognorm64", T_TUKEY, 1.5, rng.lognormal(0, 1, 64))
    add("witness_n3", T_TUKEY, 1.5, [0, 1, 2])
    add("witness_n4", T_TUKEY, 1.5, [0, 1, 2, 100])
    add("witness_n5", T_TUKEY, 1.5, [0, 1, 2, 3, 100])
    add("ties7", T_TUKEY, 1.5, [5, 5, 5, 5, 5, 5, 12])
    add("const7", T_TUKEY, 1.5, [3] * 7)
    add("single", T_TUKEY, 1.5, [42])
    add("pair", T_TUKEY, 1.5, [1, 2])
    add("empty", T_TUKEY, 1.5, [])
    mixed = [1.0, 2.0, float("nan"), 3.0, float("inf"), 4.0, 5.0]
    add("with_nan", T_TUKEY, 1.5, mixed)
    pctl_data = rng.standard_normal(64)
    add("pctl5", T_PERCENTILE, 5.0, pctl_data)
    add("pctl10", T_PERCENTILE, 10.0, rng.standard_normal(64))
    add("sd1", T_SD, 1.0, rng.normal(100, 15, 64))
    add("se1", T_SE, 1.0, rng.normal(100, 15, 64))
    add("minmax", T_MINMAX, 1.5, rng.standard_normal(64))

    def fmt(v):
        if math.isnan(v):
            return "nan"
        if math.isinf(v):
            return "inf" if v > 0 else "-inf"
        return repr(float(v))

    with open(path, "w", encoding="utf-8") as f:
        for cid, method, coeff, vals in cases:
            f.write(f"{cid} {method} {coeff!r} {len(vals)}\n")
            for v in vals:
                f.write(fmt(v) + "\n")
    print(f"harness case file written: {path} ({len(cases)} cases)")
    return cases


def check_cpp(cpp_path, cases):
    print()
    print("=" * 78)
    print(f"F. Cross-validation against the real C++ QwtBoxStatisticsCalculator ({cpp_path})")
    print("=" * 78)
    got = {}
    with open(cpp_path, "r", encoding="utf-8") as f:
        for line in f:
            tok = line.split()
            if tok and tok[0] == "CASE":
                got[tok[1]] = ([float(v) for v in tok[2:8]], [float(v) for v in tok[8:]])
    worst = 0.0
    nfail = 0
    for cid, method, coeff, vals in cases:
        if cid not in got:
            print(f"  {cid}: MISSING in C++ output")
            nfail += 1
            continue
        st = qwt_from_raw(vals, method, coeff)
        py = [st["wl"], st["q1"], st["med"], st["q3"], st["wu"], float(st["count"])]
        cpp, cpp_out = got[cid]
        d = max(abs(a - b) for a, b in zip(py, cpp))
        rel = d / max(1.0, max(abs(v) for v in py))
        py_out = st["outliers"]
        same_out = len(py_out) == len(cpp_out) and all(
            abs(a - b) <= 1e-9 * max(1.0, abs(a)) for a, b in zip(py_out, cpp_out))
        ok = rel <= 1e-12 and same_out
        worst = max(worst, rel)
        if not ok:
            nfail += 1
        print(f"  {cid:<12s} method={method} coeff={coeff:<5g} "
              f"wl={cpp[0]:+.6g} q1={cpp[1]:+.6g} med={cpp[2]:+.6g} q3={cpp[3]:+.6g} "
              f"wu={cpp[4]:+.6g} nOut={int(cpp[5])} | max rel diff={rel:.2e} "
              f"outliers match={same_out} -> {'OK' if ok else 'FAIL'}")
    print(f"\ncross-validation: {len(cases) - nfail}/{len(cases)} cases OK, worst rel diff = {worst:.3e}")
    if "pctl5" in got:
        c = got["pctl5"][0]
        # since v7.5.1 the levels follow the coefficient: the lower whisker is
        # the c-th percentile, the upper one the (100 - c)-th
        print(f"Percentile-method evidence (real C++): whiskerLower={c[0]:.6f} < "
              f"whiskerUpper={c[4]:.6f} (5th/95th percentiles), outlierCount={int(c[5])} of 64 values")
    return nfail == 0


# ====================================================================== main

def main():
    exp_a_identity()
    results = exp_b_spread(trials=1000)
    setA, setB = exp_c_outliers()
    p2out = exp_d_p2()

    fig_anatomy(setA)
    fig_spread(results)
    fig_outliers(setA, setB)
    fig_p2(p2out)

    cases = write_cases(HERE / "boxstats_cases.txt")
    if len(sys.argv) > 1:
        ok = check_cpp(sys.argv[1], cases)
        if not ok:
            sys.exit(1)


if __name__ == "__main__":
    main()
