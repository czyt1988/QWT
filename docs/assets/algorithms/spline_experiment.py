"""
Numerical experiments for the tech report "The QwtSpline family:
parametrization, accuracy convergence and rendering cost".

    python spline_experiment.py

All spline algorithms below are faithful Python re-implementations of the
C++ code in src/plot/qwt_spline*.cpp (function names of the original are
referenced in the comments).  Every raw number quoted in the report is
printed to stdout; all figures are written next to this script:

    fig_spline_overview.png      fitted curves on the 3 reference shapes
    fig_spline_convergence.png   log-log error convergence + fitted orders
    fig_spline_loops.png         chordal overshoot / loops at a corner
    fig_spline_bezier_cost.png   Catmull-Rom vs the spline family: error/cost
    fig_spline_flatten.png       Willcocks flattening: points vs tolerance

Dependencies: numpy + matplotlib only.  Deterministic: no randomness.
Comments and figure labels are English by project convention.
"""

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from pathlib import Path

HERE = Path(__file__).parent
SEED = 20261012  # fixed seed (the script itself uses no randomness)

TWO_PI = 2.0 * np.pi

# ---------------------------------------------------------------- palette
COL_REF = "#9aa0a6"
COL_POLY = "#7f7f7f"
COL_CUBIC = "#d62728"
COL_CARD = "#1f77b4"
COL_CATROM = "#17becf"
COL_PCHIP = "#2ca02c"
COL_AKIMA = "#9467bd"
COL_PLEAS = "#ff7f0e"
COL_BASIS = "#8c564b"


def set_style(ax):
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.tick_params(labelsize=9)


# ======================================================================
# 1. parametrization  (qwt_spline_parametrization.h valueIncrement*)
# ======================================================================

def param_increments(kind, pts, closed=False):
    """Parameter increments dt_i between consecutive control points.

    kind: 'uniform' | 'chordal' | 'centripetal' | 'x' | 'manhattan'
    Returns array of length n (closed: last increment wraps to pts[0])
    or n-1 (open).
    """
    p = np.asarray(pts, dtype=float)
    q = np.roll(p, -1, axis=0) if closed else p[1:]
    r = p if closed else p[:-1]
    dx = q[:, 0] - r[:, 0]
    dy = q[:, 1] - r[:, 1]
    if kind == "uniform":
        return np.ones(len(dx))
    if kind == "chordal":          # valueIncrementChordal
        return np.hypot(dx, dy)
    if kind == "centripetal":      # valueIncrementCentripetal = sqrt(chordal)
        return np.sqrt(np.hypot(dx, dy))
    if kind == "x":                # valueIncrementX
        return dx
    if kind == "manhattan":        # valueIncrementManhattan
        return np.abs(dx) + np.abs(dy)
    raise ValueError(kind)


def knots_from_increments(dt, closed):
    """t[0] = 0; t[i] = t[i-1] + dt[i-1]  (closed: n+1 knots, t[n] = total)."""
    t = np.concatenate(([0.0], np.cumsum(dt)))
    return t


# ======================================================================
# 2. cubic spline, moment method  (qwt_spline_cubic.cpp)
# ======================================================================
# The C++ code solves the same system in terms of b = M/2:
#     h1*b_{i-1} + 2(h1+h2)*b_i + h2*b_{i+1} = 3(s2 - s1)     (Equation3)
# which is the classic moment equation divided by 2:
#     h_{i-1} M_{i-1} + 2(h_{i-1}+h_i) M_i + h_i M_{i+1} = 6(s_i - s_{i-1})
# Natural boundaries (QwtSplineCubic default: Clamped2, value 0): M = 0.

def moments_open_natural(t, v):
    """Second derivatives M at knots, natural BC, Thomas algorithm O(n)."""
    n = len(t)
    h = np.diff(t)
    s = np.diff(v) / h
    M = np.zeros(n)
    if n == 3:
        # single interior equation
        M[1] = 6.0 * (s[1] - s[0]) / (2.0 * (h[0] + h[1]))
        return M
    # interior unknowns M[1..n-2], tridiagonal
    m = n - 2
    a = h[0:m].copy()            # sub diagonal   (a[j] multiplies M[j])
    b = 2.0 * (h[0:m] + h[1:m + 1])  # main diagonal (M[j+1])
    c = h[1:m + 1].copy()        # super diagonal (M[j+2])
    d = 6.0 * (s[1:m + 1] - s[0:m])
    # natural BC folds into: first row b[0] with a dropped, last row with c dropped
    # Thomas forward sweep
    cp = np.empty(m)
    dp = np.empty(m)
    cp[0] = c[0] / b[0]
    dp[0] = d[0] / b[0]
    for j in range(1, m):
        den = b[j] - a[j - 1] * cp[j - 1]
        cp[j] = c[j] / den if j < m - 1 else 0.0
        dp[j] = (d[j] - a[j - 1] * dp[j - 1]) / den
    # back substitution
    x = np.empty(m)
    x[m - 1] = dp[m - 1]
    for j in range(m - 2, -1, -1):
        x[j] = dp[j] - cp[j] * x[j + 1]
    M[1:n - 1] = x
    return M


def moments_cyclic(t, v):
    """Second derivatives for a closed (periodic) cubic spline.

    Mirrors EquationSystem2 in qwt_spline_cubic.cpp: cyclic tridiagonal
    system over n unknowns M[0..n-1] with M[n] == M[0].  t has n+1 knots
    (t[n] is the closing parameter), v[n] == v[0].
    Small n here, so a dense solve is used (still O(n) in the C++ code).
    """
    n = len(t) - 1               # number of distinct knots
    h = np.diff(t)               # n intervals incl. the closing one
    s = np.diff(v) / h
    A = np.zeros((n, n))
    rhs = np.zeros(n)
    for i in range(n):
        im = (i - 1) % n
        ip = (i + 1) % n
        A[i, im] += h[im]
        A[i, i] += 2.0 * (h[im] + h[i])
        A[i, ip] += h[i]
        rhs[i] = 6.0 * (s[i] - s[im])
    M = np.linalg.solve(A, rhs)
    return np.concatenate((M, [M[0]]))   # length n+1, M[n] == M[0]


def slopes_from_moments(t, v, M):
    """First derivatives at knots from the moments (SlopeStore formulas):
    left end of interval i:  m = s - h*(2*M_i + M_{i+1})/6
    right end of interval i: m = s + h*(M_i + 2*M_{i+1})/6
    """
    h = np.diff(t)
    s = np.diff(v) / h
    n = len(t)
    m = np.empty(n)
    m[0] = s[0] - h[0] * (2.0 * M[0] + M[1]) / 6.0
    for i in range(1, n - 1):
        m[i] = s[i - 1] + h[i - 1] * (M[i - 1] + 2.0 * M[i]) / 6.0
    m[n - 1] = s[n - 2] + h[n - 2] * (M[n - 2] + 2.0 * M[n - 1]) / 6.0
    return m


def cubic_segs(points, param, closed):
    """Bezier segments of the cubic spline (moment method).

    Parametric mode follows qwtSplineC1PathParametric(): build x(t), y(t)
    and solve twice.  param='x' is the non-parametric QwtSplineCubic
    default (QwtSplineC1 ctor sets ParameterX).
    """
    p = np.asarray(points, dtype=float)
    n = len(p)
    dt = param_increments(param, p, closed)
    # closed: dt has n increments (incl. the closing one) -> n+1 knots
    t = knots_from_increments(dt, closed)
    vx = p[:, 0] if not closed else np.concatenate((p[:, 0], [p[0, 0]]))
    vy = p[:, 1] if not closed else np.concatenate((p[:, 1], [p[0, 1]]))
    Mx = moments_cyclic(t, vx) if closed else moments_open_natural(t, vx)
    My = moments_cyclic(t, vy) if closed else moments_open_natural(t, vy)
    mx = slopes_from_moments(t, vx, Mx)
    my = slopes_from_moments(t, vy, My)
    nseg = n if closed else n - 1
    segs = np.empty((nseg, 4, 2))
    for i in range(nseg):
        h = t[i + 1] - t[i]
        p1 = p[i]
        p2 = p[(i + 1) % n] if closed else p[i + 1]
        segs[i, 0] = p1
        segs[i, 1] = (p1[0] + mx[i] * h / 3.0, p1[1] + my[i] * h / 3.0)
        segs[i, 2] = (p2[0] - mx[i + 1] * h / 3.0, p2[1] - my[i + 1] * h / 3.0)
        segs[i, 3] = p2
    return segs


# ======================================================================
# 3. local C1 splines  (qwt_spline_local.cpp)
# ======================================================================
# Boundary default of QwtSplineLocal: LinearRunout value 0 -> the slope at
# the first/last knot equals the slope of the first/last interval
# (QwtSplineC1P::slopeBoundary with r = 0 gives m = s).

def _secants(t, v):
    return np.diff(v) / np.diff(t)


def slopes_cardinal(t, v, closed):
    n = len(v) - 1 if closed else len(v)
    m = np.empty(len(v))
    if closed:
        # periodic phantom point (qwtSplineBoundariesL1, Periodic/Closed):
        # pn = p0 - (p_last - p_prev) ; t[-1] == total parameter
        m0 = (v[1] - v[n - 1]) / (t[1] - t[0] + (t[n] - t[n - 1]))
        m[0] = m0
        m[n] = m0
        for j in range(1, n):
            m[j] = (v[j + 1] - v[j - 1]) / (t[j + 1] - t[j - 1])
        return m[:n + 1]
    m[0] = v[1] - v[0]
    m[0] /= t[1] - t[0]
    m[-1] = (v[-1] - v[-2]) / (t[-1] - t[-2])
    for j in range(1, len(v) - 1):
        m[j] = (v[j + 1] - v[j - 1]) / (t[j + 1] - t[j - 1])   # qwtSlopeCardinal
    return m


def slopes_pchip(t, v):
    """qwtSlopePChip: weighted harmonic mean, 0 at non-monotonic knots."""
    s = _secants(t, v)
    n = len(v)
    m = np.zeros(n)
    m[0] = s[0]
    m[n - 1] = s[n - 2]
    for j in range(1, n - 1):
        dy1 = v[j] - v[j - 1]
        dy2 = v[j + 1] - v[j]
        if dy1 == 0.0 or dy2 == 0.0 or ((dy1 > 0.0) != (dy2 > 0.0)):
            m[j] = 0.0
        else:
            s1 = s[j - 1]
            s2 = s[j]
            s12 = (dy1 + dy2) / (t[j + 1] - t[j - 1])
            m[j] = 3.0 * (s1 * s2) / (s1 + s2 + s12)
    return m


def slopes_akima(t, v):
    """qwtSlopeAkima with the extended secant list of qwtSplineAkima:
    virtual secants 0.5*s_first / 0.5*s_last at the two ends."""
    s = _secants(t, v)
    n = len(v)
    se = np.empty(len(s) + 2)
    se[0] = 0.5 * s[0]
    se[1:-1] = s
    se[-1] = 0.5 * s[-1]

    def akima(s1, s2, s3, s4):
        if s1 == s2 and s3 == s4:
            return 0.5 * (s2 + s3)
        ds12 = abs(s2 - s1)
        ds34 = abs(s4 - s3)
        return (s2 * ds34 + s3 * ds12) / (ds12 + ds34)

    m = np.empty(n)
    m[0] = s[0]
    m[n - 1] = s[n - 2]
    for j in range(1, n - 1):
        m[j] = akima(se[j - 1], se[j], se[j + 1], se[j + 2])
    return m


def local_segs(points, param, closed, kind):
    """Bezier segments of a local C1 spline (Cardinal or PChip or Akima).

    Parametric mode: qwtSplineC1PathParametric() solves x(t) and y(t)
    separately with the scalar slope rule, then builds control lines
    c1 = p1 + m_i*dt/3, c2 = p2 - m_{i+1}*dt/3.
    """
    p = np.asarray(points, dtype=float)
    n = len(p)
    dt = param_increments(param, p, closed)
    t = knots_from_increments(dt, closed)
    if closed:
        tx = np.concatenate((p[:, 0], [p[0, 0]]))
        ty = np.concatenate((p[:, 1], [p[0, 1]]))
    else:
        tx, ty = p[:, 0], p[:, 1]
    if kind == "cardinal":
        mx = slopes_cardinal(t, tx, True) if closed else slopes_cardinal(t, tx, False)
        my = slopes_cardinal(t, ty, True) if closed else slopes_cardinal(t, ty, False)
    elif kind == "pchip":
        assert not closed
        mx = slopes_pchip(t, tx)
        my = slopes_pchip(t, ty)
    elif kind == "akima":
        assert not closed
        mx = slopes_akima(t, tx)
        my = slopes_akima(t, ty)
    else:
        raise ValueError(kind)
    nseg = n if closed else n - 1
    segs = np.empty((nseg, 4, 2))
    for i in range(nseg):
        h = t[i + 1] - t[i]
        p1 = p[i]
        p2 = p[(i + 1) % n] if closed else p[i + 1]
        segs[i, 0] = p1
        segs[i, 1] = (p1[0] + mx[i] * h / 3.0, p1[1] + my[i] * h / 3.0)
        segs[i, 2] = (p2[0] - mx[i + 1] * h / 3.0, p2[1] - my[i + 1] * h / 3.0)
        segs[i, 3] = p2
    return segs


# ======================================================================
# 4. QwtSplinePleasing  (qwt_spline_pleasing.cpp, faithful port)
# ======================================================================

def _chordal(a, b):
    return float(np.hypot(b[0] - a[0], b[1] - a[1]))


def _vec_cardinal(param, p1, p2, p3):
    """qwtVectorCardinal: t2*(p3-p1)/(t1+t2)."""
    t1 = param_increments(param, [p1, p2])[0]
    t2 = param_increments(param, [p2, p3])[0]
    return (np.asarray(p3) - np.asarray(p1)) * (t2 / (t1 + t2))


def _vec(param, p1, p2):
    """qwtVector: (p2-p1)/param(p1,p2)."""
    t1 = param_increments(param, [p1, p2])[0]
    return (np.asarray(p2) - np.asarray(p1)) / t1


def _tension_pleasing(d13, d23, d24, p1, p2, p3, p4):
    """qwtTensionPleasing: the 1/3 rule with extra rules for narrow angles."""
    b1 = (d13 / 3.0) < d23
    b2 = (d24 / 3.0) < d23
    if b1:
        if b2:
            t1 = 1.0 / 3.0 if not np.array_equal(p1, p2) else 2.0 / 3.0
            t2 = 1.0 / 3.0 if not np.array_equal(p3, p4) else 2.0 / 3.0
        else:
            t1 = t2 = d23 / d24
    else:
        if b2:
            t1 = t2 = d23 / d13
        else:
            t1 = d23 / d13
            t2 = d23 / d24
    return t1, t2


def pleasing_segs(points, param, closed):
    """qwtSplinePathPleasing(), open and closed variant."""
    p = [np.asarray(q, dtype=float) for q in points]
    size = len(p)
    segs = []

    if closed:
        d13 = _chordal(p[0], p[2])
        t1, t2 = _tension_pleasing(_chordal(p[size - 1], p[1]),
                                   _chordal(p[0], p[1]), d13,
                                   p[size - 1], p[0], p[1], p[2])
        vec0 = _vec_cardinal(param, p[size - 1], p[0], p[1])
        vec1 = _vec_cardinal(param, p[0], p[1], p[2])
        segs.append((p[0], p[0] + vec0 * t1, p[1] - vec1 * t2, p[1]))
    else:
        d13 = _chordal(p[0], p[2])
        d01 = _chordal(p[0], p[1])
        t1, t2 = _tension_pleasing(d01, d01, d13, p[0], p[0], p[1], p[2])
        vec0 = 0.5 * _vec(param, p[0], p[1])
        vec1 = _vec_cardinal(param, p[0], p[1], p[2])
        segs.append((p[0], p[0] + vec0 * t1, p[1] - vec1 * t2, p[1]))

    for i in range(1, size - 2):
        d23 = _chordal(p[i], p[i + 1])
        d24 = _chordal(p[i], p[i + 2])
        vec2 = _vec_cardinal(param, p[i], p[i + 1], p[i + 2])
        t1, t2 = _tension_pleasing(d13, d23, d24, p[i - 1], p[i], p[i + 1], p[i + 2])
        segs.append((p[i], p[i] + vec1 * t1, p[i + 1] - vec2 * t2, p[i + 1]))
        d13 = d24
        vec1 = vec2

    if closed:
        d24 = _chordal(p[size - 2], p[0])
        t1, t2 = _tension_pleasing(d13, _chordal(p[size - 2], p[size - 1]), d24,
                                   p[size - 3], p[size - 2], p[size - 1], p[0])
        vec2 = _vec_cardinal(param, p[size - 2], p[size - 1], p[0])
        segs.append((p[size - 2], p[size - 2] + vec1 * t1, p[size - 1] - vec2 * t2, p[size - 1]))
        d34 = _chordal(p[size - 1], p[0])
        d35 = _chordal(p[size - 1], p[1])
        t1, t2 = _tension_pleasing(d24, d34, d35, p[size - 2], p[size - 1], p[0], p[1])
        vec3 = _vec_cardinal(param, p[size - 1], p[0], p[1])
        segs.append((p[size - 1], p[size - 1] + vec2 * t1, p[0] - vec3 * t2, p[0]))
    else:
        d24 = _chordal(p[size - 2], p[size - 1])
        t1, t2 = _tension_pleasing(d13, _chordal(p[size - 2], p[size - 1]), d24,
                                   p[size - 3], p[size - 2], p[size - 1], p[size - 1])
        vec2 = 0.5 * _vec(param, p[size - 2], p[size - 1])
        segs.append((p[size - 2], p[size - 2] + vec1 * t1, p[size - 1] - vec2 * t2, p[size - 1]))

    return np.array(segs)


# ======================================================================
# 5. QwtSplineBasis  (qwt_spline_basis.cpp, faithful port)
# ======================================================================

def basis_segs(points, param, closed):
    """Uniform path (qwtSplineBasisPathUniform) for param='uniform',
    parametrized path (qwtSplineBasisPath) otherwise.
    Only ConditionalBoundaries (open) and ClosedPolygon (closed, uniform).
    """
    pd = [np.asarray(q, dtype=float) for q in points]
    n = len(pd)
    assert n >= 4
    segs = []

    if param == "uniform":
        cp1 = (2.0 * pd[0] + pd[1]) / 3.0
        if closed:
            cpN = (pd[n - 1] + 2.0 * pd[0]) / 3.0
            start = 0.5 * (cpN + cp1)
        else:
            start = pd[0]   # path.moveTo(pd[0])
        for i in range(1, n - 1):
            cp2 = (pd[i - 1] + 2.0 * pd[i]) / 3.0
            cp3 = (2.0 * pd[i] + pd[i + 1]) / 3.0
            end = 0.5 * (cp2 + cp3)
            p0seg = start if not segs else segs[-1][3]
            segs.append((p0seg, cp1, cp2, end))
            cp1 = cp3
        if not closed:
            cp2 = (pd[n - 2] + 2.0 * pd[n - 1]) / 3.0
            segs.append((segs[-1][3], cp1, cp2, pd[n - 1]))
        else:
            cp2 = (pd[n - 2] + 2.0 * pd[n - 1]) / 3.0
            cp3 = (2.0 * pd[n - 1] + pd[0]) / 3.0
            segs.append((segs[-1][3], cp1, cp2, 0.5 * (cp2 + cp3)))
            cp4 = (pd[n - 1] + 2.0 * pd[0]) / 3.0
            cp5 = (2.0 * pd[0] + pd[1]) / 3.0
            segs.append((segs[-1][3], cp3, cp4, 0.5 * (cp4 + cp5)))
            assert np.allclose(segs[-1][3], start)
        return np.array(segs)

    # parametrized variant, open (ConditionalBoundaries) only
    assert not closed
    inc = param_increments(param, np.array(pd), False)
    t1, t2 = inc[0], inc[1]
    t0 = t1                                    # ConditionalBoundaries
    t012 = t0 + t1 + t2
    cp1 = ((t1 + t2) * pd[0] + t0 * pd[1]) / t012
    for i in range(1, n - 2):
        t3 = inc[i + 1]
        t123 = t1 + t2 + t3
        cp2 = (t2 * pd[i - 1] + (t0 + t1) * pd[i]) / t012
        cp3 = ((t2 + t3) * pd[i] + t1 * pd[i + 1]) / t123
        p2 = (t2 * cp2 + t1 * cp3) / (t1 + t2)
        if i == 1:
            segs.append((pd[0], cp1, cp2, p2))
        else:
            segs.append((segs[-1][3], cp1, cp2, p2))
        cp1 = cp3
        t0, t1, t2, t012 = t1, t2, t3, t123
    # final block (t3 = t2 for ConditionalBoundaries)
    t3 = t2
    t123 = t1 + t2 + t3
    cp2 = (t2 * pd[n - 3] + (t0 + t1) * pd[n - 2]) / t012
    cp3 = ((t2 + t3) * pd[n - 2] + t1 * pd[n - 1]) / t123
    p2 = (t2 * cp2 + t1 * cp3) / (t1 + t2)
    segs.append((segs[-1][3], cp1, cp2, p2))
    cp1 = cp3
    t0, t1, t2, t012 = t1, t2, t3, t123
    cp2 = (t2 * pd[n - 2] + (t0 + t1) * pd[n - 1]) / t012
    segs.append((segs[-1][3], cp1, cp2, pd[n - 1]))
    return np.array(segs)


# ======================================================================
# 6. plain Catmull-Rom -> cubic Bezier  (the "simple Bezier fit" baseline)
# ======================================================================

def catmull_rom_segs(points, closed):
    """c1 = P_i + (P_{i+1}-P_{i-1})/6, c2 = P_{i+1} - (P_{i+2}-P_i)/6.
    Open: reflected phantom points P_{-1} = 2P_0 - P_1 (this reproduces
    the LinearRunout(0) boundary of QwtSplineLocal exactly)."""
    p = np.asarray(points, dtype=float)
    n = len(p)
    nseg = n if closed else n - 1

    def P(i):
        if closed:
            return p[i % n]
        if i < 0:
            return 2.0 * p[0] - p[1]
        if i >= n:
            return 2.0 * p[n - 1] - p[n - 2]
        return p[i]

    segs = np.empty((nseg, 4, 2))
    for i in range(nseg):
        a, b, c, d = P(i - 1), P(i), P(i + 1), P(i + 2)
        segs[i, 0] = b
        segs[i, 1] = b + (c - a) / 6.0
        segs[i, 2] = c - (d - b) / 6.0
        segs[i, 3] = c
    return segs


def polyline_segs(points, closed):
    """The raw control polygon: zero extra control points."""
    p = np.asarray(points, dtype=float)
    n = len(p)
    nseg = n if closed else n - 1
    segs = np.empty((nseg, 4, 2))
    for i in range(nseg):
        p2 = p[(i + 1) % n] if closed else p[i + 1]
        segs[i] = (p[i], p[i], p2, p2)
    return segs


# ======================================================================
# 7. Bezier evaluation + Willcocks flattening (src/core/qwt_bezier.cpp)
# ======================================================================

def dense_points(segs, per_seg=24, drop_shared=True):
    """Sample the Bezier chain densely (Bernstein form)."""
    u = np.linspace(0.0, 1.0, per_seg, endpoint=not drop_shared)
    out = []
    for p1, c1, c2, p2 in segs:
        s = 1.0 - u
        pts = (s ** 3)[:, None] * p1 + (3 * s ** 2 * u)[:, None] * c1 \
            + (3 * s * u ** 2)[:, None] * c2 + (u ** 3)[:, None] * p2
        out.append(pts)
    out.append(segs[-1][3][None, :])
    return np.vstack(out)


def _bz_flatness(p1, c1, c2, p2):
    """BezierData::flatness() -- Willcocks measure."""
    ux = 3.0 * c1[0] - 2.0 * p1[0] - p2[0]
    uy = 3.0 * c1[1] - 2.0 * p1[1] - p2[1]
    vx = 3.0 * c2[0] - 2.0 * p2[0] - p1[0]
    vy = 3.0 * c2[1] - 2.0 * p2[1] - p1[1]
    return max(ux * ux, vx * vx) + max(uy * uy, vy * vy)


def _bz_subdivide(p1, c1, c2, p2):
    """de Casteljau split at t = 1/2."""
    m1 = 0.5 * (p1 + c1)
    m2 = 0.5 * (c1 + c2)
    m3 = 0.5 * (c2 + p2)
    q1 = 0.5 * (m1 + m2)
    q2 = 0.5 * (m2 + m3)
    r = 0.5 * (q1 + q2)
    return (p1, m1, q1, r), (r, q2, m3, p2)


def willcocks_leaves(p1, c1, c2, p2, flatness_thr, max_iter=100000):
    """Count flat leaves of QwtBezier::appendToPolygon (iterative stack)."""
    if flatness_thr <= 0.0:
        return 0
    stack = [(np.asarray(p1), np.asarray(c1), np.asarray(c2), np.asarray(p2))]
    leaves = 0
    it = 0
    while stack:
        it += 1
        if it > max_iter:
            raise RuntimeError("flattening does not terminate")
        bz = stack.pop()
        if _bz_flatness(*bz) < flatness_thr:
            leaves += 1
        else:
            l, r = _bz_subdivide(*bz)
            stack.append(r)
            stack.append(l)
    return leaves


def flatten_polygon_count(segs, tol):
    """Total polygon size produced by QwtSpline::polygon(points, tolerance):
    1 shared start point + sum of leaves over all cubic segments."""
    thr = 16.0 * tol * tol          # BezierData::minFlatness
    total = 1
    for p1, c1, c2, p2 in segs:
        total += willcocks_leaves(p1, c1, c2, p2, thr)
    return total


# ======================================================================
# 8. reference curves and exact distances
# ======================================================================

def sample_circle(N):
    th = TWO_PI * np.arange(N) / N
    return np.stack((np.cos(th), np.sin(th)), axis=1)


def dist_circle(P):
    return np.abs(np.hypot(P[:, 0], P[:, 1]) - 1.0)


def sample_sine(N):
    x = np.linspace(0.0, TWO_PI, N)
    return np.stack((x, np.sin(x)), axis=1)


def dist_sine(P):
    x, y = P[:, 0], P[:, 1]
    inside = (x >= 0.0) & (x <= TWO_PI)
    dn = np.abs(y - np.sin(x)) / np.sqrt(1.0 + np.cos(x) ** 2)
    d0 = np.hypot(x, y)
    d1 = np.hypot(x - TWO_PI, y)
    return np.where(inside, dn, np.minimum(d0, d1))


KINK_STRAIGHT = 1.5
KINK_THMAX = 1.2


def sample_kink(N):
    """Straight line (kappa = 0) joined G1 to a unit arc (kappa = 1):
    curvature jumps at the seam.  Sampled equidistant in arc length."""
    L = KINK_STRAIGHT + KINK_THMAX
    s = np.linspace(0.0, L, N)
    x = np.where(s <= KINK_STRAIGHT, s - KINK_STRAIGHT, np.sin(s - KINK_STRAIGHT))
    y = np.where(s <= KINK_STRAIGHT, 0.0, 1.0 - np.cos(s - KINK_STRAIGHT))
    return np.stack((x, y), axis=1)


def _dist_point_seg(P, A, B):
    A = np.asarray(A, dtype=float)
    B = np.asarray(B, dtype=float)
    ab = B - A
    t = ((P - A) @ ab) / (ab @ ab)
    t = np.clip(t, 0.0, 1.0)
    proj = A + t[:, None] * ab
    return np.hypot(*(P - proj).T)


def dist_kink(P):
    c = np.array([0.0, 1.0])
    d_seg = _dist_point_seg(P, (-KINK_STRAIGHT, 0.0), (0.0, 0.0))
    v = P - c
    rr = np.hypot(v[:, 0], v[:, 1])
    thv = np.arctan2(v[:, 0], -v[:, 1])
    on_arc = (thv >= 0.0) & (thv <= KINK_THMAX)
    E = np.array([np.sin(KINK_THMAX), 1.0 - np.cos(KINK_THMAX)])
    d_arc = np.where(on_arc, np.abs(rr - 1.0), np.hypot(P[:, 0] - E[0], P[:, 1] - E[1]))
    return np.minimum(d_seg, d_arc)


CURVES = {
    "circle": dict(sample=sample_circle, dist=dist_circle, closed=True,
                   title="unit circle (closed)"),
    "sine": dict(sample=sample_sine, dist=dist_sine, closed=False,
                 title="sinuous S-wave y = sin(x)"),
    "kink": dict(sample=sample_kink, dist=dist_kink, closed=False,
                 title="curvature jump: line -> arc"),
}


# ======================================================================
# 9. method registry
# ======================================================================

def build_segs(method, points, closed):
    name, param = method
    if name == "cubic":
        return cubic_segs(points, param, closed)
    if name in ("cardinal", "pchip", "akima"):
        return local_segs(points, param, closed, name)
    if name == "pleasing":
        return pleasing_segs(points, param, closed)
    if name == "basis":
        return basis_segs(points, param, closed)
    if name == "catmullrom":
        return catmull_rom_segs(points, closed)
    if name == "polyline":
        return polyline_segs(points, closed)
    raise ValueError(name)


def label(method):
    name, param = method
    pretty = {"cubic": "Cubic (natural)", "cardinal": "Cardinal",
              "pchip": "PChip", "akima": "Akima", "pleasing": "Pleasing",
              "basis": "Basis (approx.)", "catmullrom": "Catmull-Rom Bezier",
              "polyline": "control polygon"}[name]
    return f"{pretty} [{param}]" if param else pretty


def max_deviation(method, curve_key, N, per_seg=24):
    cv = CURVES[curve_key]
    pts = cv["sample"](N)
    segs = build_segs(method, pts, cv["closed"])
    dense = dense_points(segs, per_seg)
    err = float(cv["dist"](dense).max())
    # interpolation check at the knots
    knot_err = float(cv["dist"](pts).max())
    return err, knot_err, segs


def mean_chord(points, closed):
    dt = param_increments("chordal", points, closed)
    return float(dt.mean())


# ======================================================================
# 10. self-intersection (loop) detection
# ======================================================================

def count_loops(poly, chunk=512):
    """Proper segment self-intersections of a dense polyline (vectorized).

    Segments i and j (j > i+1) intersect properly when the standard
    cross-product parameters t and u both fall inside [0, 1].
    """
    a = poly[:-1]
    d = np.diff(poly, axis=0)
    n = len(a)
    total = 0
    for i0 in range(0, n, chunk):
        i1 = min(n, i0 + chunk)
        A = a[i0:i1][:, None, :]          # (ci, 1, 2)
        R = d[i0:i1][:, None, :]          # (ci, 1, 2)
        B = a[None, :, :]                 # (1, n, 2)
        S = d[None, :, :]                 # (1, n, 2)
        diff = B - A                      # (ci, n, 2)
        crs = R[..., 0] * S[..., 1] - R[..., 1] * S[..., 0]
        with np.errstate(divide="ignore", invalid="ignore"):
            t = (diff[..., 0] * S[..., 1] - diff[..., 1] * S[..., 0]) / crs
            u = (diff[..., 0] * R[..., 1] - diff[..., 1] * R[..., 0]) / crs
        ii = np.arange(i0, i1)[:, None]
        jj = np.arange(n)[None, :]
        valid = (np.abs(crs) > 1e-14) & (jj > ii + 1) \
            & (t >= 0.0) & (t <= 1.0) & (u >= 0.0) & (u <= 1.0)
        total += int(valid.sum())
    return total


def dist_to_polygon(P, poly):
    """Max distance of sample points P to the control polygon 'poly'."""
    a = poly[:-1]
    d = np.diff(poly, axis=0)
    dd = np.maximum((d * d).sum(axis=1), 1e-30)
    tt = ((P[:, None, :] - a[None, :, :]) * d[None, :, :]).sum(axis=2) / dd[None, :]
    tt = np.clip(tt, 0.0, 1.0)
    proj = a[None, :, :] + tt[..., None] * d[None, :, :]
    diff = P[:, None, :] - proj
    dmin = np.sqrt((diff * diff).sum(axis=2)).min(axis=1)
    return float(dmin.max())


# ======================================================================
# 11. experiments
# ======================================================================

NSEQ = [8, 12, 16, 24, 32, 48, 64, 96, 128]


def run_convergence(cases_by_curve):
    results = {}
    for curve, cases in cases_by_curve.items():
        cv = CURVES[curve]
        for method in cases:
            rows = []
            knot_max = 0.0
            for N in NSEQ:
                err, knot_err, _ = max_deviation(method, curve, N)
                h = mean_chord(cv["sample"](N), cv["closed"])
                rows.append((N, h, err))
                knot_max = max(knot_max, knot_err)
            results[(curve, method)] = rows
            print(f"  {curve:7s} {label(method):28s} knot-interp err = {knot_max:.3e}")
    return results


def fit_order(rows, err_floor=1e-12):
    r = [(h, e) for (N, h, e) in rows if e > err_floor]
    lh = np.log([x[0] for x in r])
    le = np.log([x[1] for x in r])
    if len(r) < 3:
        return float("nan")
    return float(np.polyfit(lh, le, 1)[0])


def experiment_natural_boundary():
    print("\n" + "=" * 78)
    print("[2] NATURAL BOUNDARY CHECK: f = exp(x) and x^2 + sin(3x) on [0, 1]")
    print("    (f'' != 0 at both ends -> the natural condition M = 0 is wrong there)")
    print("=" * 78)
    cases = (("exp(x)", np.exp, np.exp),
             ("x^2 + sin(3x)", lambda x: x * x + np.sin(3.0 * x),
              lambda x: 2.0 - 9.0 * np.sin(3.0 * x)))
    for fname, f, f2 in cases:
        hs, errs = [], []
        e1 = {}
        for N in (32, 64, 128, 256, 512, 1024):
            x = np.linspace(0.0, 1.0, N)
            v = f(x)
            M = moments_open_natural(x, v)
            xq = np.linspace(0.0, 1.0, 4001)
            i = np.clip(np.searchsorted(x, xq) - 1, 0, N - 2)
            h = x[i + 1] - x[i]
            u = xq - x[i]
            c3 = (M[i + 1] - M[i]) / (6.0 * h)
            c2 = M[i] / 2.0
            c1 = (v[i + 1] - v[i]) / h - (c3 * h + c2) * h
            Sq = v[i] + c1 * u + c2 * u * u + c3 * u ** 3
            errs.append(float(np.abs(Sq - f(xq)).max()))
            hs.append(1.0 / (N - 1))
            e1[N] = float(abs(M[1] - f2(x[1])))
        p = float(np.polyfit(np.log(hs), np.log(errs), 1)[0])
        print(f"  {fname:14s} function-error order p = {p:.2f}   "
              f"(err(N=32) = {errs[0]:.3e}, err(N=1024) = {errs[-1]:.3e})")
        print(f"  {'':14s} |S''-f''| at knot 1: N=64 -> {e1[64]:.4f}, "
              f"N=512 -> {e1[512]:.4f}  (O(1) boundary layer, does not vanish)")
        print(f"  {'':14s} theoretical layer amplitude (2-sqrt(3))*|f''(a)| = "
              f"{(2.0 - np.sqrt(3.0)) * abs(f2(0.0)):.4f}")


def experiment_convergence():
    print("\n" + "=" * 78)
    print("[3] CONVERGENCE: max geometric deviation vs sampling density")
    print("=" * 78)
    cases_by_curve = {
        "circle": [("cubic", "uniform"), ("cardinal", "uniform"),
                   ("pleasing", "uniform"), ("basis", "uniform"),
                   ("catmullrom", "")],
        "sine": [("cubic", "x"), ("cubic", "uniform"), ("cubic", "chordal"),
                 ("cubic", "centripetal"),
                 ("cardinal", "chordal"), ("cardinal", "centripetal"),
                 ("pchip", "x"), ("akima", "x"),
                 ("pleasing", "uniform"), ("basis", "uniform")],
        "kink": [("cubic", "x"), ("cubic", "chordal"),
                 ("cardinal", "chordal"), ("cardinal", "centripetal"),
                 ("pchip", "x"), ("pleasing", "uniform"), ("basis", "uniform")],
    }
    res = run_convergence(cases_by_curve)

    print("\n-- raw convergence table (N, mean chord h, max deviation) --")
    for key, rows in res.items():
        curve, method = key
        if method[0] == "catmullrom":
            continue  # printed in the equivalence section instead
        txt = " ".join(f"{e:.3e}" for (N, h, e) in rows)
        print(f"{curve:7s} {label(method):28s} {txt}")
    print("N sequence:", NSEQ)

    print("\n-- fitted convergence orders p (err ~ C*h^p, log-log fit) --")
    orders = {}
    for key, rows in res.items():
        curve, method = key
        p = fit_order(rows)
        orders[key] = p
        print(f"{curve:7s} {label(method):28s} p = {p:6.2f}   "
              f"err(N=8) = {rows[0][2]:.3e}   err(N=128) = {rows[-1][2]:.3e}")
    return res, orders, cases_by_curve


def experiment_equivalence():
    print("\n" + "=" * 78)
    print("[1] SANITY / EQUIVALENCE CHECKS")
    print("=" * 78)

    # (a) Catmull-Rom == Cardinal[uniform] == Pleasing[uniform] on smooth data
    for curve in ("circle", "sine"):
        cv = CURVES[curve]
        pts = cv["sample"](16)
        s1 = catmull_rom_segs(pts, cv["closed"])
        s2 = build_segs(("cardinal", "uniform"), pts, cv["closed"])
        s3 = build_segs(("pleasing", "uniform"), pts, cv["closed"])
        d12 = float(np.abs(s1 - s2).max())
        d13 = float(np.abs(s1 - s3).max())
        print(f"  {curve:7s} max|CatmullRom - Cardinal[uniform]| = {d12:.3e}")
        print(f"  {curve:7s} max|CatmullRom - Pleasing[uniform]|  = {d13:.3e}")

    # (b) equidistant circle samples: uniform == chordal == centripetal
    #     up to an affine reparametrization -> identical geometry
    pts = sample_circle(16)
    sa = build_segs(("cubic", "uniform"), pts, True)
    sb = build_segs(("cubic", "chordal"), pts, True)
    sc = build_segs(("cubic", "centripetal"), pts, True)
    print(f"  circle  max|cubic uniform - cubic chordal|      = {np.abs(sa - sb).max():.3e}")
    print(f"  circle  max|cubic uniform - cubic centripetal|  = {np.abs(sa - sc).max():.3e}")

    # (b2) uniformly-x-sampled sine: ParameterX == ParameterUniform (affine t)
    pts = sample_sine(16)
    sa = build_segs(("cubic", "x"), pts, False)
    sb = build_segs(("cubic", "uniform"), pts, False)
    print(f"  sine    max|cubic x - cubic uniform|            = {np.abs(sa - sb).max():.3e}")

    # (b3) kink: chordal/centripetal/uniform differ near the seam, but the
    #      argmax of the deviation sits in the locally equidistant arc part
    #      where all three parametrizations coincide -> identical max errors
    pts = sample_kink(24)
    d1 = dense_points(build_segs(("cardinal", "chordal"), pts, False), 24)
    d2 = dense_points(build_segs(("cardinal", "centripetal"), pts, False), 24)
    d3 = dense_points(build_segs(("pleasing", "uniform"), pts, False), 24)
    print(f"  kink    max|cardinal chordal - centripetal|     = {np.abs(d1 - d2).max():.3e}")
    print(f"  kink    max|cardinal chordal - pleasing unif|   = {np.abs(d1 - d3).max():.3e}")
    i = int(dist_kink(d1).argmax())
    print(f"  kink    shared argmax of deviation at ({d1[i, 0]:.6f}, {d1[i, 1]:.6f}), "
          f"dist = {dist_kink(d1)[i]:.6e} (on the equidistant arc tail)")

    # (c) interpolation property of the interpolating splines
    for m in (("cubic", "chordal"), ("cardinal", "chordal"),
              ("pchip", "x"), ("akima", "x"), ("pleasing", "uniform")):
        pts = sample_sine(24)
        segs = build_segs(m, pts, False)
        knot_dev = max(np.abs(segs[i, 0] - pts[i]).max() for i in range(len(pts) - 1))
        end_dev = np.abs(segs[-1][3] - pts[-1]).max()
        print(f"  sine    {label(m):28s} knot deviation = {max(knot_dev, end_dev):.3e}")

    # (d) Basis does NOT interpolate (approximating spline):
    #     measure how far the fitted curve passes from the interior knots
    pts = sample_sine(24)
    segs = build_segs(("basis", "uniform"), pts, False)
    dense = dense_points(segs, 64)
    dref = float(dist_sine(dense).max())
    knot_off = float(max(dist_to_polygon(pts[i:i + 1], dense) for i in range(1, len(pts) - 1)))
    print(f"  sine    Basis [uniform]: fitted curve max dev from reference = {dref:.3e}, "
          f"max knot offset (curve misses knots by) = {knot_off:.3e}")


def experiment_loops():
    print("\n" + "=" * 78)
    print("[4] CONTROL POLYGON DEVIATION: chordal overshoot / loops")
    print("=" * 78)

    # family 1 (overshoot): one sharp corner, short arms r after a long arm 1.
    # All phi are affine-equivalent, so only the ratio r matters; we keep a
    # representative subset and report the overshoot ratio chordal/centripetal.
    cfgs = []
    for phi_deg in (60, 90, 120, 150, 170):
        for r in (0.05, 0.1, 0.2, 0.4):
            phi = np.deg2rad(phi_deg)
            A = np.array([0.0, 0.0])
            B = np.array([1.0, 0.0])
            C = B + r * np.array([np.cos(np.pi - phi), np.sin(np.pi - phi)])
            D = C + r * np.array([np.cos(np.pi - 2 * phi), np.sin(np.pi - 2 * phi)])
            cfgs.append((f"corner phi={phi_deg} r={r}", np.array([A, B, C, D])))

    # family 2 (overshoot): sparse uneven samples of a circle
    for ths in ([0, 20, 40, 180, 300], [0, 10, 30, 150, 270], [0, 30, 60, 200, 320]):
        th = np.deg2rad(np.array(ths, dtype=float))
        cfgs.append((f"circle5 {ths}", np.stack((np.cos(th), np.sin(th)), axis=1)))

    rows = []
    for name, pts in cfgs:
        for spl in (("cubic", "chordal"), ("cubic", "centripetal"), ("cubic", "uniform")):
            segs = build_segs(spl, pts, False)
            dense = dense_points(segs, 64)
            over = dist_to_polygon(dense, pts)
            loops = count_loops(dense)
            rows.append((name, label(spl), over, loops))

    print("  config                        method            overshoot   loops")
    for name, lab, over, loops in rows:
        flag = "  <-- LOOP" if loops else ""
        print(f"  {name:28s} {lab:24s} {over:9.4f} {loops:5d}{flag}")

    # family 3 (loops): deterministic grid of 4-point hairpin configs
    # P0=(0,0), P1=(1,0), P2=P1+a*u(th), P3=P2+b*u(th+ph).  Search for the
    # config where cubic[chordal] loops while cubic[centripetal] stays clean.
    print("\n  -- hairpin grid search: chordal loops, centripetal clean --")
    n_uni_loop = sum(1 for r in rows if "uniform" in r[1] and r[3] > 0)
    print(f"  grid size: {6 * 6 * 5 * 7} hairpin configs; "
          f"corner family {5 * 4} configs, uniform loops in {n_uni_loop} of them")
    cands = []
    for a in (0.05, 0.1, 0.2, 0.5, 1.0, 2.0):
        for b in (0.05, 0.1, 0.2, 0.5, 1.0, 2.0):
            for th_deg in (60, 90, 120, 150, 170):
                for ph_deg in (-90, -60, -30, 0, 30, 60, 90):
                    th = np.deg2rad(th_deg)
                    ph = np.deg2rad(ph_deg)
                    P0 = np.array([0.0, 0.0])
                    P1 = np.array([1.0, 0.0])
                    P2 = P1 + a * np.array([np.cos(th), np.sin(th)])
                    P3 = P2 + b * np.array([np.cos(th + ph), np.sin(th + ph)])
                    P = np.array([P0, P1, P2, P3])
                    segs_c = build_segs(("cubic", "chordal"), P, False)
                    dc = dense_points(segs_c, 48)
                    loops_c = count_loops(dc)
                    if loops_c == 0:
                        continue
                    over_c = dist_to_polygon(dc, P)
                    segs_n = build_segs(("cubic", "centripetal"), P, False)
                    dn = dense_points(segs_n, 48)
                    loops_n = count_loops(dn)
                    over_n = dist_to_polygon(dn, P)
                    if loops_n == 0:
                        cands.append((over_c, a, b, th_deg, ph_deg,
                                      loops_c, over_c, over_n, P))
    # selection rule (deterministic): among the configs where the chordal
    # spline loops and the centripetal one stays clean, take the largest
    # overshoot ratio chordal/centripetal
    cands.sort(key=lambda c: -(c[6] / max(c[7], 1e-12)))
    print(f"  configs with chordal loop AND clean centripetal: {len(cands)}")
    for c in cands[:5]:
        print(f"    a={c[1]:.2f} b={c[2]:.2f} th={c[3]}deg ph={c[4]}deg: "
              f"chordal over={c[6]:.4f}, centripetal over={c[7]:.4f}, "
              f"ratio={c[6] / max(c[7], 1e-12):.1f}")

    best = cands[0]
    P = best[8]
    name = f"hairpin a={best[1]}, b={best[2]}, th={best[3]}deg, ph={best[4]}deg"
    print(f"\n  selected config: {name}")
    print("  control points:")
    for q in P:
        print(f"    ({q[0]:.6f}, {q[1]:.6f})")
    tang = {}
    for par in ("uniform", "chordal", "centripetal"):
        segs = build_segs(("cubic", par), P, False)
        dense = dense_points(segs, 96)
        # tangent magnitude at P1 = end of the long edge: |m| = 3*arm/dt
        dt0 = param_increments(par, P, False)[0]
        arm2 = np.hypot(*(segs[0, 3] - segs[0, 2]))
        edge0 = np.hypot(*(segs[0, 3] - segs[0, 0]))
        edge1 = np.hypot(*(segs[1, 3] - segs[1, 0]))
        tang[par] = 3.0 * arm2 / dt0
        print(f"    cubic[{par:11s}]: overshoot = {dist_to_polygon(dense, P):.4f}, "
              f"loops = {count_loops(dense)}, |S'(P1)| = {tang[par]:.3f}, "
              f"arm/edge(long) = {arm2 / edge0:.2f} "
              f"(next edge length {edge1:.3f}, opposite direction)")
    print(f"    tangent ratio chordal/centripetal = {tang['chordal'] / tang['centripetal']:.2f}, "
          f"chordal/uniform = {tang['chordal'] / tang['uniform']:.2f}")
    return (0.0, name, P, None, None)


def experiment_cost():
    print("\n" + "=" * 78)
    print("[5] CATMULL-ROM vs THE SPLINE FAMILY: error and control point cost")
    print("=" * 78)

    methods = [("polyline", ""), ("catmullrom", ""), ("cardinal", "centripetal"),
               ("cubic", "chordal"), ("pchip", "x"), ("pleasing", "uniform"),
               ("basis", "uniform")]
    for curve in ("circle", "sine"):
        cv = CURVES[curve]
        print(f"\n  -- {cv['title']} --")
        print("  method                        N    segments  ctrl pts  max deviation")
        for N in (16, 32, 64):
            pts = cv["sample"](N)
            for m in methods:
                if m[0] == "pchip" and curve == "circle":
                    continue   # closed PChip not implemented (open curves only)
                if m[0] == "basis" and N < 5:
                    continue
                segs = build_segs(m, pts, cv["closed"])
                dense = dense_points(segs, 24)
                err = float(cv["dist"](dense).max())
                nseg = len(segs)
                nctrl = 0 if m[0] == "polyline" else 2 * nseg
                print(f"  {label(m):28s} {N:4d} {nseg:9d} {nctrl:9d}  {err:.6e}")

    print("\n  -- derived accuracy ratios at equal control point budget --")

    def err_of(curve, m, N):
        cv = CURVES[curve]
        pts = cv["sample"](N)
        return float(cv["dist"](dense_points(build_segs(m, pts, cv["closed"]), 24)).max())

    pairs = [
        ("circle  cubic[chordal] vs Catmull-Rom, N=32", ("cubic", "chordal"), ("catmullrom", ""), "circle", 32),
        ("circle  cubic[chordal] vs Catmull-Rom, N=64", ("cubic", "chordal"), ("catmullrom", ""), "circle", 64),
        ("sine    cubic[chordal] vs Cardinal[centripetal], N=64", ("cubic", "chordal"), ("cardinal", "centripetal"), "sine", 64),
        ("sine    cubic[chordal] vs Catmull-Rom, N=64", ("cubic", "chordal"), ("catmullrom", ""), "sine", 64),
        ("sine    PChip[x] over Catmull-Rom, N=64", ("catmullrom", ""), ("pchip", "x"), "sine", 64),
        ("sine    cubic[chordal] over cubic[x], N=128", ("cubic", "x"), ("cubic", "chordal"), "sine", 128),
        ("circle  Basis[uniform] over control polygon, N=64", ("polyline", ""), ("basis", "uniform"), "circle", 64),
    ]
    for pname, ma, mb, curve, N in pairs:
        ea, eb = err_of(curve, ma, N), err_of(curve, mb, N)
        print(f"    {pname:50s} ratio = {eb / ea:6.2f}")

    # screen-scale flattening cost (radius 200 px circle, tolerance 0.5 px,
    # like QwtSpline::polygon(points, 0.5) used by examples/2D/splineeditor).
    # The raw polyline is NOT flattened by QwtSpline (QwtPlotCurve draws it
    # directly), so its cost is simply N points.
    print("\n  -- flattened polygon size, circle R = 200 px, tol = 0.5 px --")
    print("  method                         N   polygon points   amplification")
    R = 200.0
    for N in (16, 32, 64):
        pts = sample_circle(N) * R
        print(f"  {'control polygon':28s} {N:4d} {N + 1:14d} {1.0:13.2f}  (drawn as is)")
        for m in methods:
            if m[0] in ("pchip", "polyline"):
                continue
            segs = build_segs(m, pts, True)
            npts = flatten_polygon_count(segs, 0.5)
            print(f"  {label(m):28s} {N:4d} {npts:14d} {npts / N:13.2f}")


def experiment_flatten():
    print("\n" + "=" * 78)
    print("[6] RENDERING COST: Willcocks flattening vs tolerance (analytic check)")
    print("=" * 78)
    R = 200.0
    N = 16
    pts = sample_circle(N) * R
    segs = build_segs(("cardinal", "uniform"), pts, True)
    L = TWO_PI * R
    kappa = 1.0 / R
    print(f"  circle R = {R:.0f} px, N = {N}, Cardinal[uniform] "
          f"(= QwtSplineCurveFitter default)")
    print("  tol      polygon pts   analytic L*sqrt(kappa/(8*tol))   ratio")
    rows = []
    tols = [0.01 * 2.0 ** (k / 3.0) for k in range(26)]   # 0.01 .. 3.36
    for tol in tols:
        npts = flatten_polygon_count(segs, tol)
        ana = L * np.sqrt(kappa / (8.0 * tol))
        rows.append((tol, npts, ana))
        print(f"  {tol:6.3f} {npts:12d} {ana:20.1f} {npts / ana:19.3f}")
    free = [(t, n) for (t, n, a_) in rows if n > 3 * (N + 1)]
    ltol = np.log([t for t, n in free])
    ln = np.log([n for t, n in free])
    slope = float(np.polyfit(ltol, ln, 1)[0])
    print(f"  fit range: tol <= {free[-1][0]:.3f} ({len(free)} points above the "
          f"segment floor {N + 1})")
    print(f"  fitted slope d(log n)/d(log tol) = {slope:.3f}  (theory: -0.5)")

    # family overlay: all splines flatten to (almost) the same polygon size
    print("\n  -- flattened size by method, circle R = 200 px, N = 32 --")
    print("  tol      Cardinal[unif]  Cubic[chordal]  Pleasing[unif]  Basis[unif]")
    pts32 = sample_circle(32) * R
    mm = (("cardinal", "uniform"), ("cubic", "chordal"),
          ("pleasing", "uniform"), ("basis", "uniform"))
    for tol in (0.05, 0.2, 0.5):
        counts = [flatten_polygon_count(build_segs(m, pts32, True), tol) for m in mm]
        print(f"  {tol:5.2f} {counts[0]:15d} {counts[1]:15d} {counts[2]:15d} {counts[3]:12d}")
    return rows, slope


def experiment_opcounts():
    print("\n" + "=" * 78)
    print("[7] ANALYTIC OPERATION COUNTS (estimates, NOT wall-clock timings)")
    print("=" * 78)
    print("  method                     per-point cost (flops)      locality  tridiag solves")
    print("  Cubic [x]      (natural)   ~15 N (1 solve 8N + asm 7N)     0        1 scalar")
    print("  Cubic [chordal/centr.]     ~33 N (2 solves + N sqrt)       0        2 scalar")
    print("  Cardinal [param]           ~22 N                           1        0")
    print("  PChip [param]              ~34 N                           1        0")
    print("  Akima [param]              ~38 N                           2        0")
    print("  Pleasing [param]           ~33 N (3 sqrt per segment)      2        0")
    print("  Basis [uniform]            ~24 N (no sqrt, no solve)       2        0")
    for N in (1000, 100000, 1000000):
        print(f"  N = {N:8d}: cubic[chordal] ~ {33 * N / 1e6:7.2f} Mflop, "
              f"cardinal ~ {22 * N / 1e6:7.2f} Mflop, basis ~ {24 * N / 1e6:7.2f} Mflop")
    print("  Fitted chain (analytic): N data -> visible window n_vis (binary search)")
    print("    -> MinMax reduce ~4W pts -> n'-1 cubic segments -> flatten at tol:")
    print("       polygon pts ~ max(n', sum_i L_i*sqrt(kappa_i/(8*tol)))")
    R = 200.0
    for tol in (0.05, 0.5):
        print(f"    circle R=200px: tol={tol} -> analytic flatten total "
              f"~ {TWO_PI * R * np.sqrt((1.0 / R) / (8.0 * tol)):.0f} pts")


# ======================================================================
# 12. figures
# ======================================================================

def fig_overview():
    fig, axes = plt.subplots(1, 3, figsize=(11.4, 3.9))

    # circle, N = 12
    ax = axes[0]
    pts = sample_circle(12)
    th = np.linspace(0, TWO_PI, 720)
    ax.plot(np.cos(th), np.sin(th), color=COL_REF, lw=2.4, zorder=1)
    ax.plot(np.r_[pts[:, 0], pts[0, 0]], np.r_[pts[:, 1], pts[0, 1]],
            ":o", color="#bbbbbb", ms=3.5, lw=1.0, zorder=2, label="control polygon")
    for m, c in ((("cubic", "uniform"), COL_CUBIC), (("cardinal", "uniform"), COL_CARD),
                 (("pleasing", "uniform"), COL_PLEAS), (("basis", "uniform"), COL_BASIS)):
        d = dense_points(build_segs(m, pts, True), 40)
        ax.plot(d[:, 0], d[:, 1], color=c, lw=1.3, zorder=3, label=label(m))
    ax.set_title("circle, N = 12", fontsize=10)
    ax.set_aspect("equal")
    ax.legend(fontsize=7.2, frameon=False, loc="upper left", bbox_to_anchor=(1.02, 1.0))
    set_style(ax)

    # sine, N = 10
    ax = axes[1]
    pts = sample_sine(10)
    xr = np.linspace(0, TWO_PI, 720)
    ax.plot(xr, np.sin(xr), color=COL_REF, lw=2.4, zorder=1)
    ax.plot(pts[:, 0], pts[:, 1], ":o", color="#bbbbbb", ms=3.5, lw=1.0, zorder=2)
    for m, c in ((("cubic", "x"), COL_CUBIC), (("cardinal", "chordal"), COL_CARD),
                 (("pchip", "x"), COL_PCHIP), (("basis", "uniform"), COL_BASIS)):
        d = dense_points(build_segs(m, pts, False), 40)
        ax.plot(d[:, 0], d[:, 1], color=c, lw=1.3, zorder=3, label=label(m))
    ax.set_title("S-wave y = sin(x), N = 10", fontsize=10)
    ax.legend(fontsize=7.2, frameon=False)
    set_style(ax)

    # kink, N = 24
    ax = axes[2]
    pts = sample_kink(24)
    s = np.linspace(0, KINK_STRAIGHT + KINK_THMAX, 720)
    xr = np.where(s <= KINK_STRAIGHT, s - KINK_STRAIGHT, np.sin(s - KINK_STRAIGHT))
    yr = np.where(s <= KINK_STRAIGHT, 0.0, 1.0 - np.cos(s - KINK_STRAIGHT))
    ax.plot(xr, yr, color=COL_REF, lw=2.4, zorder=1, label="reference (G2 break)")
    ax.plot(pts[:, 0], pts[:, 1], ":o", color="#bbbbbb", ms=3.0, lw=0.9, zorder=2)
    for m, c in ((("cubic", "chordal"), COL_CUBIC), (("cardinal", "centripetal"), COL_CARD),
                 (("pleasing", "uniform"), COL_PLEAS)):
        d = dense_points(build_segs(m, pts, False), 40)
        ax.plot(d[:, 0], d[:, 1], color=c, lw=1.3, zorder=3, label=label(m))
    ax.set_title("curvature jump, N = 24 (arc-length)", fontsize=10)
    ax.legend(fontsize=7.2, frameon=False)
    set_style(ax)

    fig.suptitle("the QwtSpline family on three reference shapes (sparse sampling)", fontsize=11)
    fig.tight_layout(rect=[0, 0, 1, 0.93])
    fig.savefig(HERE / "fig_spline_overview.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_convergence(res, orders):
    fig, axes = plt.subplots(2, 2, figsize=(10.6, 7.4))

    def panel(ax, curve, methods, title):
        for m in methods:
            key = (curve, m)
            if key not in res:
                continue
            rows = res[key]
            h = [r[1] for r in rows]
            e = [r[2] for r in rows]
            col = {"cubic": COL_CUBIC, "cardinal": COL_CARD, "pchip": COL_PCHIP,
                   "akima": COL_AKIMA, "pleasing": COL_PLEAS, "basis": COL_BASIS,
                   "catmullrom": COL_CATROM, "polyline": COL_POLY}[m[0]]
            ls = "-" if m[1] in ("x", "uniform", "") else ("--" if m[1] == "chordal" else ":")
            p = orders[key]
            ax.plot(h, e, ls, marker="o", ms=3.0, lw=1.3, color=col,
                    label=f"{label(m)}, p = {p:.2f}")
        # reference slopes
        h0, e0 = 0.55, 2.0
        ax.plot([0.02, 0.6], [e0 * (np.array([0.02, 0.6]) / h0) ** 4, ][0],
                color="#dddddd", lw=1.0)
        ax.annotate("$O(h^4)$", (0.045, e0 * (0.045 / h0) ** 4), fontsize=8, color="#999999")
        ax.plot([0.02, 0.6], [e0 * (np.array([0.02, 0.6]) / h0) ** 2, ][0],
                color="#dddddd", lw=1.0)
        ax.annotate("$O(h^2)$", (0.32, e0 * (0.32 / h0) ** 2 * 1.2), fontsize=8, color="#999999")
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xlabel("mean chord length $h$")
        ax.set_ylabel("max deviation")
        ax.set_title(title, fontsize=9.5)
        ax.legend(fontsize=6.6, frameon=False, loc="lower left")
        ax.grid(True, which="both", ls=":", lw=0.4, color="#e8e8e8")
        set_style(ax)

    panel(axes[0][0], "circle",
          [("cubic", "uniform"), ("cardinal", "uniform"),
           ("pleasing", "uniform"), ("basis", "uniform")],
          "circle (closed, equidistant): interpolating vs approximating")
    panel(axes[0][1], "sine",
          [("cubic", "x"), ("cubic", "uniform"), ("cubic", "chordal"),
           ("cubic", "centripetal")],
          "S-wave: cubic spline, four parametrizations")
    panel(axes[1][0], "sine",
          [("cardinal", "chordal"), ("cardinal", "centripetal"),
           ("pchip", "x"), ("akima", "x"),
           ("pleasing", "uniform"), ("basis", "uniform")],
          "S-wave: local C1 splines, Pleasing and Basis")
    panel(axes[1][1], "kink",
          [("cubic", "x"), ("cubic", "chordal"), ("cardinal", "chordal"),
           ("cardinal", "centripetal"), ("pchip", "x"),
           ("pleasing", "uniform"), ("basis", "uniform")],
          "curvature jump: order drops at the G2 break")

    fig.suptitle("error convergence of the re-implemented QwtSpline algorithms "
                 "(fitted orders p in the legend)", fontsize=10.5)
    fig.tight_layout(rect=[0, 0, 1, 0.95])
    fig.savefig(HERE / "fig_spline_convergence.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_loops(best):
    _, name, pts, _, _ = best
    # zoom on the corner: bounding box of all three curves + control polygon
    dens = {par: dense_points(build_segs(("cubic", par), pts, False), 96)
            for par in ("uniform", "chordal", "centripetal")}
    allp = np.vstack(list(dens.values()) + [pts])
    x0, x1 = allp[:, 0].min(), allp[:, 0].max()
    y0, y1 = allp[:, 1].min(), allp[:, 1].max()
    mx, my = 0.08 * (x1 - x0), 0.25 * (y1 - y0)
    # the interesting action is at the hairpin end of the polygon
    x0 = pts[1:, 0].min() - mx
    x1 = x1 + mx
    y0 = y0 - my
    y1 = y1 + my

    fig, axes = plt.subplots(1, 3, figsize=(11.4, 2.6))
    for ax, par in zip(axes, ("uniform", "chordal", "centripetal")):
        dense = dens[par]
        over = dist_to_polygon(dense, pts)
        loops = count_loops(dense)
        ax.plot(pts[:, 0], pts[:, 1], ":o", color="#888888", ms=7, lw=1.4,
                label="control polygon")
        ax.plot(dense[:, 0], dense[:, 1], color=COL_CUBIC, lw=1.7,
                label="cubic spline")
        if loops:
            ax.annotate("self-intersection (loop)",
                        xy=(x0 + 0.72 * (x1 - x0), y0 + 0.28 * (y1 - y0)),
                        xytext=(x0 + 0.30 * (x1 - x0), y0 + 0.80 * (y1 - y0)),
                        fontsize=8.5, color="#a33", ha="left",
                        arrowprops=dict(arrowstyle="->", color="#a33"))
        ax.set_xlim(x0, x1)
        ax.set_ylim(y0, y1)
        ax.set_aspect("equal")
        ax.set_title(f"{label(('cubic', par))}\novershoot = {over:.4f}, loops = {loops}",
                     fontsize=9.5)
        ax.legend(fontsize=7.5, frameon=False, loc="upper left")
        set_style(ax)
        ax.tick_params(labelsize=7.5)
    fig.suptitle(f"natural cubic spline on a sparse hairpin turn ({name}):\n"
                 "the parametrization decides over loops and overshoot "
                 "(zoom on the corner)", fontsize=10)
    fig.tight_layout(rect=[0, 0, 1, 0.86])
    fig.savefig(HERE / "fig_spline_loops.png", dpi=200, bbox_inches="tight")
    plt.close(fig)

    print("\n  -- loop figure numbers (96 samples per segment) --")
    for par in ("uniform", "chordal", "centripetal"):
        segs = build_segs(("cubic", par), pts, False)
        dense = dense_points(segs, 96)
        print(f"  cubic[{par:11s}]              overshoot = {dist_to_polygon(dense, pts):.4f}, "
              f"loops = {count_loops(dense)}")


def fig_cost():
    fig, axes = plt.subplots(1, 2, figsize=(10.8, 3.9))
    methods = [("polyline", ""), ("catmullrom", ""), ("cardinal", "centripetal"),
               ("cubic", "chordal"), ("pleasing", "uniform"), ("basis", "uniform")]
    colors = {"polyline": COL_POLY, "catmullrom": COL_CATROM, "cardinal": COL_CARD,
              "cubic": COL_CUBIC, "pleasing": COL_PLEAS, "basis": COL_BASIS}

    # (a) grouped bars: error vs N on the circle
    ax = axes[0]
    Ns = (16, 32, 64)
    width = 0.13
    xpos = np.arange(len(Ns))
    for k, m in enumerate(methods):
        errs = []
        for N in Ns:
            pts = sample_circle(N)
            segs = build_segs(m, pts, True)
            errs.append(float(dist_circle(dense_points(segs, 24)).max()))
        ax.bar(xpos + (k - 2.5) * width, errs, width, color=colors[m[0]],
               label=label(m), edgecolor="white")
    ax.set_yscale("log")
    ax.set_xticks(xpos)
    ax.set_xticklabels([f"N = {N}" for N in Ns])
    ax.set_ylabel("max deviation (log)")
    ax.set_title("circle: error at equal control point budget\n"
                 "(every method: 1 cubic per gap, 2 ctrl pts each)", fontsize=9)
    ax.legend(fontsize=6.8, frameon=False, ncol=2)
    set_style(ax)

    # (b) grouped bars: error vs N on the sine wave (open curves, ParameterX
    #     for the function-valued methods)
    ax = axes[1]
    methods_s = [("polyline", ""), ("catmullrom", ""), ("cardinal", "centripetal"),
                 ("cubic", "chordal"), ("pchip", "x"), ("pleasing", "uniform"),
                 ("basis", "uniform")]
    colors_s = dict(colors)
    colors_s["pchip"] = COL_PCHIP
    width = 0.115
    for k, m in enumerate(methods_s):
        errs = []
        for N in Ns:
            pts = sample_sine(N)
            segs = build_segs(m, pts, False)
            errs.append(float(dist_sine(dense_points(segs, 24)).max()))
        ax.bar(xpos + (k - 3.0) * width, errs, width, color=colors_s[m[0]],
               label=label(m), edgecolor="white")
    ax.set_yscale("log")
    ax.set_xticks(xpos)
    ax.set_xticklabels([f"N = {N}" for N in Ns])
    ax.set_ylabel("max deviation (log)")
    ax.set_title("S-wave: Catmull-Rom vs the interpolating family\n"
                 "(PChip pays shape preservation with accuracy)", fontsize=9)
    ax.legend(fontsize=6.4, frameon=False, ncol=2)
    set_style(ax)

    fig.tight_layout()
    fig.savefig(HERE / "fig_spline_bezier_cost.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_flatten(rows, slope):
    fig, ax = plt.subplots(figsize=(7.6, 4.2))
    tol = [r[0] for r in rows]
    n = [r[1] for r in rows]
    ana = [r[2] for r in rows]
    ax.plot(tol, n, "o-", color=COL_CARD, lw=1.6, ms=4,
            label="Cardinal[uniform], measured (Willcocks)")
    # family overlay on the same circle: cubic chordal
    R = 200.0
    pts = sample_circle(16) * R
    segs_c = build_segs(("cubic", "chordal"), pts, True)
    n_c = [flatten_polygon_count(segs_c, t) for t in tol]
    ax.plot(tol, n_c, "s--", color=COL_CUBIC, lw=1.2, ms=3.5,
            label="Cubic[chordal], measured")
    ax.plot(tol, ana, "--", color="#888888", lw=1.4,
            label="analytic $L\\sqrt{\\kappa/(8\\,tol)}$")
    ax.plot(tol, max(n) * (np.array(tol) / tol[0]) ** -0.5, ":",
            color="#bbbbbb", lw=1.2, label="slope $-1/2$ guide")
    ax.axhline(17, color="#dddddd", lw=1.0)
    ax.annotate("segment floor N + 1 = 17", (0.011, 19), fontsize=8, color="#888888")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("flattening tolerance (px)")
    ax.set_ylabel("polygon points for the full circle")
    ax.set_title(f"QwtBezier flattening cost, circle R = 200 px, N = 16\n"
                 f"fitted slope = {slope:.3f} (theory $-1/2$)", fontsize=10)
    ax.legend(fontsize=8.5, frameon=False, loc="upper right")
    ax.grid(True, which="both", ls=":", lw=0.4, color="#e8e8e8")
    set_style(ax)
    fig.tight_layout()
    fig.savefig(HERE / "fig_spline_flatten.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


# ======================================================================

def main():
    np.random.seed(SEED)   # not used; fixed for policy/reproducibility
    print("spline_experiment.py -- QwtSpline family experiments")
    print(f"N sequence: {NSEQ}")

    experiment_equivalence()
    experiment_natural_boundary()
    res, orders, _ = experiment_convergence()
    best = experiment_loops()
    experiment_cost()
    rows, slope = experiment_flatten()
    experiment_opcounts()

    fig_overview()
    fig_convergence(res, orders)
    fig_loops(best)
    fig_cost()
    fig_flatten(rows, slope)
    print("\nfigures written to", HERE)


if __name__ == "__main__":
    main()
