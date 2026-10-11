"""
Generates all figures for the two algorithm deep dive articles.

    python figures.py

Outputs ( written next to this script ):
    fig_lttb_triangle.png        three bucket geometry of the classic LTTB
    fig_lttb_overview.png        four reduction strategies on a narrow peak
    fig_lttb_spikes.png          spike pairs: LTTB drops them, MinMax keeps
    fig_lttb_fidelity.png        envelope fidelity of all strategies/signals
    fig_lttb_budget.png          MinMax fidelity vs bucket budget
    fig_smooth_kernels.png       Gaussian / Savitzky-Golay / box kernels
    fig_smooth_response.png      frequency responses |H(w)|
    fig_smooth_noisegain.png     residual noise vs window size
    fig_smooth_peakretention.png peak height after filtering vs peak width
    fig_smooth_preserve.png      the feature preservation pipeline
    fig_smooth_spectrum.png      the 100k spectrum before/after smoothing
"""

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Polygon as MplPolygon

import downsampling_experiment as D
import smoothing_experiment as S

HERE = __import__("pathlib").Path(__file__).parent

COL_REF = "#9aa0a6"
COL_LTTB = "#1f77b4"
COL_MM = "#d62728"
COL_PIX = "#2ca02c"
COL_UNI = "#ff7f0e"
ALGO_COLORS = {"uniform stride": COL_UNI, "LTTB (classic)": COL_LTTB,
               "MinMax bucket": COL_MM, "pixel column": COL_PIX}
SIG_ORDER = ["band limited", "peaks + noise", "spike pairs", "uneven x", "white noise"]
ALGO_ORDER = ["uniform stride", "LTTB (classic)", "MinMax bucket", "pixel column"]


# ------------------------------------------------------------------ helpers

def make_maps_for(x, y):
    return D.make_maps(x, y)


def spectrum():
    rng = np.random.default_rng(0xC0FFEE)
    n = 100000
    x = np.arange(n, dtype=float)

    def gauss(c, s, h):
        return h * np.exp(-0.5 * ((x - c) / s) ** 2)

    y = 1.0 + 0.3 * np.sin(x / 8000.0)
    y += 3.0 * gauss(30000.0, 5000.0, 1.0) + 2.0 * gauss(70000.0, 3000.0, 1.0)
    y += 5.0 * gauss(15000.0, 300.0, 1.0) + 3.5 * gauss(55000.0, 250.0, 1.0) + 4.5 * gauss(85000.0, 400.0, 1.0)
    y += rng.normal(0.0, 0.15, n)
    u = rng.uniform(0, 1, n)
    y += np.where(u < 0.01, 0.4 + 0.6 * u, 0.0)
    u = rng.uniform(0, 1, n)
    y -= np.where(u < 0.008, 0.8 + 0.8 * u, 0.0)
    y[44999:45002] -= 3.0
    return x, y


def poly_smooth(poly, kernel, preserve=3.0):
    """QWT7 render smoothing on the pixel polyline ( mirror reflection at
    the borders, two box passes as baseline, MAD noise scale, blend )."""
    n = len(poly)
    m = len(kernel) // 2

    res = poly.copy()
    for i in range(n):
        acc = 0.0
        for j, c in enumerate(kernel):
            idx = i + j - m
            while idx < 0 or idx >= n:
                if idx < 0:
                    idx = -idx
                else:
                    idx = 2 * (n - 1) - idx
            acc += c * poly[idx, 1]
        res[i, 1] = acc

    if preserve <= 0:
        return res, None, None, None

    wb = max(61, 6 * len(kernel))
    wb = wb + 1 if wb % 2 == 0 else wb
    base = poly.copy()
    for _ in range(2):
        out = base.copy()
        for k in range(n):
            a = max(0, k - wb // 2)
            b = min(n - 1, k + wb // 2)
            out[k, 1] = base[a:b + 1, 1].mean()
        base = out

    dev = np.abs(poly[:, 1] - base[:, 1])
    med = np.median(dev)
    if med <= 0:
        return res, base, dev, np.zeros(n)

    lo = preserve * med
    hi = 1.2 * lo
    w = np.clip((dev - lo) / (hi - lo), 0.0, 1.0)
    res[:, 1] = res[:, 1] + w * (poly[:, 1] - res[:, 1])
    return res, base, dev, w


def set_style(ax):
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.tick_params(labelsize=9)


# ============================================================ LTTB article

def fig_lttb_triangle_geometry():
    """The three bucket geometry behind the classic LTTB area criterion."""
    fig, ax = plt.subplots(figsize=(8.2, 3.6))

    # previous selected point a
    a = np.array([1.0, 2.2])
    # next bucket centroid c (black diamond)
    c = np.array([8.6, 3.4])
    # candidates in the current bucket
    cand = np.array([[3.2, 3.7], [4.0, 1.6], [4.9, 3.3], [5.7, 2.0], [6.5, 3.9]])

    ax.plot([a[0]], [a[1]], "o", ms=9, color=COL_LTTB, zorder=5)
    ax.annotate("previous selected\npoint $a$", (a[0], a[1]), (a[0] - 0.6, 0.35),
                fontsize=10, ha="center")
    ax.plot([c[0]], [c[1]], "D", ms=8, color="#555555", zorder=5)
    ax.annotate("centroid of the\nnext bucket $c$", (c[0], c[1]), (c[0] + 0.6, 4.5),
                fontsize=10, ha="center")

    # the bucket ranges
    ax.axvspan(2.6, 7.0, color="#e8eef7", zorder=0)
    ax.annotate("current bucket $B_i$", (4.8, 4.55), fontsize=10, ha="center", color="#3f5575")

    # candidate points and their triangles
    for p in cand:
        ax.plot([p[0]], [p[1]], "o", ms=6, color="#777777", zorder=4)
        tri = MplPolygon([a, p, c], closed=True, facecolor=COL_LTTB, edgecolor="none", alpha=0.18)
        ax.add_patch(tri)

    # the winning candidate
    p_star = cand[2]
    tri = MplPolygon([a, p_star, c], closed=True, facecolor=COL_LTTB, edgecolor=COL_LTTB, alpha=0.42)
    ax.add_patch(tri)
    ax.annotate("chosen point\n$\\hat{p}_i$ (max area)", (p_star[0], p_star[1]),
                (p_star[0] + 1.15, 4.7), fontsize=10, ha="center",
                arrowprops=dict(arrowstyle="->", color=COL_LTTB))

    ax.plot([a[0], c[0]], [a[1], c[1]], "--", color="#999999", lw=1)
    ax.text(4.6, 2.95, "$A(p) = \\frac{1}{2}\\,|\\,(a_x - c_x)(p_y - a_y) - (a_x - p_x)(c_y - a_y)\\,|$",
            fontsize=11, color="#444444")

    ax.set_xlim(0.4, 9.8)
    ax.set_ylim(0, 5.6)
    ax.set_xticks([])
    ax.set_yticks([])
    set_style(ax)
    fig.tight_layout()
    fig.savefig(HERE / "fig_lttb_triangle.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_lttb_overview():
    """The four reduction strategies around one narrow peak."""
    x, y = D.sig_peaks(100000)
    xmap, ymap = make_maps_for(x, y)
    ref_xs, ref_ys = D.reduce_rendered(x, y, xmap, ymap)

    # the narrow peak at 0.32 n -> column 288; zoom on cols 300..330
    c0, c1 = 282, 316
    from_ = max(0, int((c0 - 2) / 899 * 99999))
    to_ = min(99999, int((c1 + 2) / 899 * 99999))

    budget = 4 * 900
    algos = {
        "uniform stride": D.reduce_uniform(x, y, xmap, ymap, budget),
        "LTTB (classic)": D.reduce_lttb(x, y, xmap, ymap, budget),
        "MinMax bucket": D.reduce_minmax_bucket(x, y, xmap, ymap),
    }

    fig, axes = plt.subplots(2, 2, figsize=(9.6, 6.4), sharex=True)
    titles = ["reference (100,000 points)", "uniform stride",
              "classic LTTB (three buckets)", "QWT7 MinMax bucket (2 buckets / column)"]

    for ax, (title, (xs, ys)) in zip(axes.flat, [("reference", (ref_xs, ref_ys))] + [(t, a) for t, a in algos.items()]):
        sel = (xs >= c0 - 4) & (xs <= c1 + 4)
        ax.plot(xs[sel], ys[sel], color=COL_MM if "MinMax" in title else COL_LTTB, lw=1.1, zorder=3)
        if title != "reference":
            ax.plot(xs[sel], ys[sel], "o", ms=2.4, color=("white" if "MinMax" in title else "#123a63"),
                    mec=COL_MM if "MinMax" in title else COL_LTTB, mew=0.6, zorder=4)
        ax.set_title(title, fontsize=9.5)
        ax.set_xlim(c0, c1)
        ax.set_ylim(-20, 130)
        set_style(ax)

    for ax in axes[:, 0]:
        ax.set_ylabel("pixel row (y)")
    for ax in axes[1]:
        ax.set_xlabel("pixel column (x)")
    for ax in axes.flat:
        ax.tick_params(labelsize=8)

    fig.suptitle("100,000-sample signal with noise, one narrow peak at column 288\n"
                 "(output budget ≈ 3600 points in all reduced plots)", fontsize=10.5)
    fig.tight_layout(rect=[0, 0, 1, 0.94])
    fig.savefig(HERE / "fig_lttb_overview.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_lttb_spikes():
    """Classic LTTB drops a narrow up/down spike pair, MinMax keeps both."""
    x, y = D.sig_spike_pairs(100000)
    xmap, ymap = make_maps_for(x, y)
    ref_xs, ref_ys = D.reduce_rendered(x, y, xmap, ymap)
    lx, ly = D.reduce_lttb(x, y, xmap, ymap, 4 * 900)
    mx, my = D.reduce_minmax_bucket(x, y, xmap, ymap)

    # find the spike pair at index 26000 -> column ~234
    c0, c1 = 228, 241
    fig, axes = plt.subplots(1, 2, figsize=(9.6, 3.4), sharex=True, sharey=True)

    for ax, (title, xs, ys, col) in zip(axes,
            [("classic LTTB", lx, ly, COL_LTTB), ("MinMax bucket", mx, my, COL_MM)]):
        sel_r = (ref_xs >= c0 - 3) & (ref_xs <= c1 + 3)
        ax.plot(ref_xs[sel_r], ref_ys[sel_r], color="#cfd4da", lw=2.0, zorder=1)
        sel = (xs >= c0 - 3) & (xs <= c1 + 3)
        ax.plot(xs[sel], ys[sel], color=col, lw=1.1, zorder=3)
        ax.plot(xs[sel], ys[sel], "o", ms=3.2, color="white", mec=col, mew=0.7, zorder=4)
        ax.set_title(title, fontsize=10)
        set_style(ax)

    axes[0].annotate("spike pair vanished -\nthe centroid criterion\ncancels both", (235, 225),
                     (229.3, 60), fontsize=8.5, ha="center", color="#a33",
                     arrowprops=dict(arrowstyle="->", color="#a33"))
    axes[1].annotate("both extremes kept\n(min and max of the bucket)", (235, 5),
                     (229.2, 320), fontsize=8.5, ha="center", color="#a33",
                     arrowprops=dict(arrowstyle="->", color="#a33"))

    axes[0].set_ylabel("pixel row")
    for ax in axes:
        ax.set_xlabel("pixel column")
        ax.tick_params(labelsize=8)

    fig.suptitle("narrow up/down spike pair (2 samples wide, ±3.5)", fontsize=10.5)
    fig.tight_layout(rect=[0, 0, 1, 0.92])
    fig.savefig(HERE / "fig_lttb_spikes.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_lttb_fidelity():
    """Envelope fidelity (row coverage) of the four strategies per signal."""
    res = D.run()

    fig, axes = plt.subplots(1, 2, figsize=(10.2, 3.6))
    xpos = np.arange(len(SIG_ORDER))
    width = 0.2

    for ax, metric, title in ((axes[0], "coverage", "envelope coverage (fraction of the reference rows)"),
                              (axes[1], "err_mean", "mean envelope error (pixel rows)")):
        for k, algo in enumerate(ALGO_ORDER):
            vals = [res[s][algo][metric] for s in SIG_ORDER]
            ax.bar(xpos + (k - 1.5) * width, vals, width, label=algo,
                   color=ALGO_COLORS[algo], edgecolor="white")
        ax.set_xticks(xpos)
        ax.set_xticklabels(SIG_ORDER, fontsize=8.5, rotation=12)
        ax.set_title(title, fontsize=9.5)
        ax.legend(fontsize=8, ncol=2, frameon=False)
        set_style(ax)

    axes[1].set_yscale("log")
    axes[1].set_ylim(0.1, 400)
    fig.tight_layout()
    fig.savefig(HERE / "fig_lttb_fidelity.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_lttb_budget():
    """MinMax fidelity vs bucket budget on the peaks + noise signal."""
    x, y = D.sig_peaks(100000)
    xmap, ymap = make_maps_for(x, y)
    ref = D.raster_envelope(*D.reduce_rendered(x, y, xmap, ymap))

    factors = np.array([0.25, 0.5, 1.0, 2.0, 4.0, 8.0])
    covs, errs, pts = [], [], []
    for f in factors:
        xs, ys = D.reduce_minmax_bucket(x, y, xmap, ymap, bucket_factor=f)
        m = D.compare(ref, D.raster_envelope(xs, ys))
        covs.append(m["coverage"])
        errs.append(m["err_mean"])
        pts.append(len(xs))

    fig, ax = plt.subplots(figsize=(7.4, 3.2))
    ax2 = ax.twinx()
    ax.plot(factors, covs, "o-", color=COL_MM, lw=1.6, label="envelope coverage")
    ax2.plot(factors, errs, "s--", color="#7f7f7f", lw=1.4, label="mean error (px)")
    ax.axvline(1.0, color="#cccccc", ls=":", lw=1.2)
    ax.annotate("QWT7 default:\n2 buckets / column\n(4 points / column)", (1.0, 0.62),
                (1.55, 0.55), fontsize=9, arrowprops=dict(arrowstyle="->", color="#888888"))
    ax.set_xscale("log")
    ax.set_xticks(factors)
    ax.set_xticklabels([f"{f:g}× 2W" for f in factors], fontsize=9)
    ax.set_xlabel("bucket budget (fraction of the QWT7 default 2·W)")
    ax.set_ylabel("envelope coverage", color=COL_MM)
    ax2.set_ylabel("mean envelope error (px)", color="#7f7f7f")
    ax.set_ylim(0.4, 1.01)
    ax.tick_params(labelsize=9)
    ax2.tick_params(labelsize=9)
    lines = ax.get_lines() + ax2.get_lines()
    ax.legend(lines, [l.get_label() for l in lines], fontsize=9, frameon=False, loc="lower right")
    set_style(ax)
    fig.tight_layout()
    fig.savefig(HERE / "fig_lttb_budget.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


# ========================================================== smoothing article

def fig_smooth_kernels():
    fig, ax = plt.subplots(figsize=(7.6, 3.4))
    w = 15
    j = np.arange(-(w // 2), w // 2 + 1)

    for name, k, c in (("box (= SG, order 1)", S.box_kernel(w), "#999999"),
                       ("Gaussian (σ = w/4)", S.gaussian_kernel(w), COL_LTTB),
                       ("Savitzky-Golay, order 2 / 3", S.savitzky_golay_kernel(w, 2), COL_MM),
                       ("Savitzky-Golay, order 4", S.savitzky_golay_kernel(w, 4), "#9467bd")):
        ax.plot(j, k, "o-", ms=4, lw=1.2, color=c, label=name)

    ax.set_xlabel("offset from the window center (polyline points)")
    ax.set_ylabel("coefficient $c_j$")
    ax.legend(fontsize=8.5, frameon=False, loc="upper right")
    ax.set_title("filter kernels, window w = 15", fontsize=10)
    set_style(ax)
    fig.tight_layout()
    fig.savefig(HERE / "fig_smooth_kernels.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_smooth_response():
    fig, ax = plt.subplots(figsize=(7.6, 3.8))
    w = 15
    for name, k, c in (("box / SG order 1", S.box_kernel(w), "#999999"),
                       ("Gaussian (σ = 3.75)", S.gaussian_kernel(w), COL_LTTB),
                       ("SG order 2 / 3", S.savitzky_golay_kernel(w, 2), COL_MM),
                       ("SG order 4", S.savitzky_golay_kernel(w, 4), "#9467bd")):
        f, H = S.frequency_response(k)
        ax.plot(f, H, lw=1.5, color=c, label=name)

    ax.axhline(10 ** (-3 / 20), color="#bbbbbb", ls=":", lw=1)
    ax.text(0.002, 10 ** (-3 / 20) * 1.15, "-3 dB", fontsize=8.5, color="#666666")
    ax.axvline(0.5, color="#dddddd", ls="--", lw=1)
    ax.text(0.502, 0.02, "pixel Nyquist\n(1 line pair / 2 px)", fontsize=8, color="#888888")
    ax.set_xlabel("frequency (cycles / polyline point)")
    ax.set_ylabel("$|H(f)|$")
    ax.set_xlim(0, 0.5)
    ax.legend(fontsize=8.5, frameon=False, loc="upper right")
    ax.set_title("frequency response, window w = 15", fontsize=10)
    ax.set_yscale("log")
    set_style(ax)
    fig.tight_layout()
    fig.savefig(HERE / "fig_smooth_response.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_smooth_noisegain():
    """sqrt(sum c_j^2) vs window size, theory + Monte Carlo check."""
    fig, ax = plt.subplots(figsize=(7.6, 3.6))
    windows = np.arange(3, 41, 2)

    rng = np.random.default_rng(7)
    noise = rng.standard_normal(40000)

    for name, kern, c in (("box", S.box_kernel, "#999999"),
                          ("Gaussian σ = w/4", S.gaussian_kernel, COL_LTTB),
                          ("SG order 2 / 3", lambda w: S.savitzky_golay_kernel(w, 2), COL_MM)):
        g = np.array([np.sum(kern(w) ** 2) for w in windows])
        ax.plot(windows, np.sqrt(g), lw=1.6, color=c, label=f"{name} (theory)")

        # Monte Carlo: filter white noise, measure the std ratio
        mc_w, mc_g = [], []
        for w in windows:
            k = kern(w)
            m = len(k) // 2
            out = np.zeros_like(noise)
            for i in range(len(noise)):
                acc = 0.0
                for jj, ck in enumerate(k):
                    idx = i + jj - m
                    if 0 <= idx < len(noise):
                        acc += ck * noise[idx]
                out[i] = acc
            mc_w.append(w)
            mc_g.append(np.std(out) / np.std(noise))
        ax.plot(mc_w, mc_g, "o", ms=3.3, color=c, mfc="white")

    ax.set_xlabel("window $w$ (odd polyline points)")
    ax.set_ylabel("residual noise ratio $\\sqrt{\\sum_j c_j^2}$")
    ax.legend(fontsize=8.5, frameon=False, loc="upper right")
    ax.set_title("white noise after the filter: theory vs Monte Carlo", fontsize=10)
    set_style(ax)
    fig.tight_layout()
    fig.savefig(HERE / "fig_smooth_noisegain.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_smooth_peakretention():
    fig, ax = plt.subplots(figsize=(7.6, 3.6))
    widths = np.array([1, 2, 3, 5, 8, 12, 20, 40], dtype=float)
    sigma = 3.75

    theory = widths / np.sqrt(widths ** 2 + sigma ** 2)
    ax.plot(widths, theory, "-", color=COL_LTTB, lw=1.6,
            label="Gaussian kernel, theory $b/\\sqrt{b^2 + \\sigma^2}$")

    for name, kern, c in (("Gaussian (measured)", lambda w: S.gaussian_kernel(15), COL_LTTB),
                          ("SG order 2", lambda w: S.savitzky_golay_kernel(15, 2), COL_MM),
                          ("SG order 4", lambda w: S.savitzky_golay_kernel(15, 4), "#9467bd")):
        kern = kern(15)
        meas = [S.peak_retention(b, kern) for b in widths]
        ax.plot(widths, meas, "o-", ms=4, lw=1.2, color=c, label=name)

    ax.axvline(2.4, color="#dddddd", ls="--", lw=1)
    ax.text(2.5, 0.42, "peak width of a\nsingle polyline point", fontsize=8, color="#888888")
    ax.set_xscale("log")
    ax.set_xticks(widths)
    ax.set_xticklabels([str(int(w)) for w in widths], fontsize=8)
    ax.set_xlabel("peak width $b$ (polyline points, std of the Gaussian bump)")
    ax.set_ylabel("peak height after filtering (fraction)")
    ax.legend(fontsize=8.5, frameon=False, loc="lower right")
    ax.set_title("narrow peak height after the filter, window w = 15", fontsize=10)
    set_style(ax)
    fig.tight_layout()
    fig.savefig(HERE / "fig_smooth_peakretention.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_smooth_preserve():
    """The feature preservation pipeline on a small excerpt."""
    x, y = D.sig_peaks(100000)
    xmap, ymap = make_maps_for(x, y)
    xs, ys = D.reduce_minmax_bucket(x, y, xmap, ymap)
    poly = np.stack([xs, ys], axis=1)

    # excerpt around the narrow peak at column 288
    sel = (poly[:, 0] >= 268) & (poly[:, 0] <= 312)
    excerpt = poly[sel].copy()
    # renumber to 0..n-1 to make the convolution valid on the excerpt
    excerpt[:, 0] = np.arange(len(excerpt))

    gm = S.gaussian_kernel(15)
    res, base, dev, wts = poly_smooth(excerpt, gm, 3.0)
    lo = 3.0 * np.median(dev)

    fig, axes = plt.subplots(2, 2, figsize=(9.8, 6.2))

    ax = axes[0][0]
    ax.plot(excerpt[:, 0], excerpt[:, 1], lw=0.9, color="#888888", label="noisy polyline")
    ax.plot(excerpt[:, 0], base[:, 1], lw=1.6, color=COL_LTTB, label="baseline (2 box passes, $w_b$ = 91)")
    ax.set_title("(a) signal and the wide baseline", fontsize=9.5)
    ax.legend(fontsize=8, frameon=False)
    set_style(ax)

    ax = axes[0][1]
    cmap = plt.cm.get_cmap("Reds")
    ax.plot(excerpt[:, 0], dev, lw=1.0, color="#b38a8a")
    ax.fill_between(excerpt[:, 0], 0, dev, color="#d62728", alpha=0.25)
    ax.axhline(lo, color=COL_MM, ls="--", lw=1.2)
    ax.axhline(1.2 * lo, color=COL_MM, ls=":", lw=1.2)
    ax.text(excerpt[0, 0] + 1, lo * 1.12, "$\\tau \\cdot \\hat{\\sigma}$  (preserve threshold)", fontsize=8.5, color=COL_MM)
    ax.text(excerpt[0, 0] + 1, 1.2 * lo * 1.04, "$1.2\\,\\tau \\hat{\\sigma}$  (full blend)", fontsize=8.5, color=COL_MM)
    ax.set_title("(b) deviation from the baseline $|d_i|$", fontsize=9.5)
    set_style(ax)

    ax = axes[1][0]
    ax.plot(excerpt[:, 0], wts, lw=1.4, color=COL_MM)
    ax.axhline(0, color="#dddddd", lw=0.8)
    ax.set_ylim(-0.08, 1.12)
    ax.set_title("(c) preservation weight $w_i$", fontsize=9.5)
    set_style(ax)

    ax = axes[1][1]
    ax.plot(excerpt[:, 0], excerpt[:, 1], lw=0.9, color="#bbbbbb", label="noisy input")
    ax.plot(excerpt[:, 0], res[:, 1], lw=1.4, color=COL_LTTB, label="pure low pass")
    pure, _, _, _ = poly_smooth(excerpt, gm, 0.0)
    ax.plot(excerpt[:, 0], pure[:, 1], lw=1.2, color="#999999", ls="--", label="low pass + preservation")
    ax.set_title("(d) result: pure filter vs preserved", fontsize=9.5)
    ax.legend(fontsize=8, frameon=False)
    set_style(ax)

    fig.suptitle("feature preservation (Gaussian w = 15, threshold τ = 3.0)", fontsize=10.5)
    fig.tight_layout(rect=[0, 0, 1, 0.95])
    fig.savefig(HERE / "fig_smooth_preserve.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


def fig_smooth_spectrum():
    """The 100k spectrum: MinMax only, plus Gaussian / SG with preservation."""
    x, y = spectrum()
    xmap, ymap = make_maps_for(x, y)
    xs, ys = D.reduce_minmax_bucket(x, y, xmap, ymap)
    poly = np.stack([xs, ys], axis=1)

    gm = S.gaussian_kernel(15)
    sg2 = S.savitzky_golay_kernel(15, 2)
    g_p, _, _, _ = poly_smooth(poly, gm, 3.0)
    s_p, _, _, _ = poly_smooth(poly, sg2, 3.0)

    fig, axes = plt.subplots(3, 1, figsize=(10.6, 6.6), sharex=True)
    for ax, (title, p, c) in zip(axes,
            (("MinMax only (default rendering)", poly, "#888888"),
             ("Gaussian w=15, τ=3 (preserved)", g_p, COL_LTTB),
             ("Savitzky-Golay w=15, order 2, τ=3", s_p, COL_MM))):
        ax.plot(p[:, 0], p[:, 1], lw=0.9, color=c)
        ax.set_ylabel("pixel row")
        ax.set_title(title, fontsize=9.5, loc="left")
        ax.set_yticks([])
        ax.tick_params(labelsize=8)

    axes[2].set_xlabel("pixel column (900 px canvas)")
    fig.suptitle("100,000-sample spectrum (humps, narrow peaks, jitter, spikes, one deep dip)", fontsize=10.5)
    fig.tight_layout(rect=[0, 0, 1, 0.96])
    fig.savefig(HERE / "fig_smooth_spectrum.png", dpi=200, bbox_inches="tight")
    plt.close(fig)


if __name__ == "__main__":
    fig_lttb_triangle_geometry()
    fig_lttb_overview()
    fig_lttb_spikes()
    fig_lttb_fidelity()
    fig_lttb_budget()

    fig_smooth_kernels()
    fig_smooth_response()
    fig_smooth_noisegain()
    fig_smooth_peakretention()
    fig_smooth_preserve()
    fig_smooth_spectrum()

    print("figures written to", HERE)