"""
Analysis and figures for the qwtSimdArgMinMax tech report
(docs/zh/tech-report/simd-argminmax.md).

    python simd_experiment.py

Reads the measured benchmark CSV produced by simd_bench.cpp
(simd_bench_results.csv, same directory) and writes five figures next to
this script:

    fig_simd_speedup.png      speedup vs scalar over the size sweep
    fig_simd_bandwidth.png    GB/s vs n: kernels + memory ceilings
    fig_simd_bucket.png       bucket-mode per-call cost vs chunk size
    fig_simd_probes.png       fixed-overhead localization probes
    fig_simd_roofline.png     measured roofline with kernel operating points

All analytical models (per-call fixed-cost fits, break-even chunk sizes,
roofline knees, effective issue rates) are computed from the CSV numbers
and printed to stdout. Deterministic: no randomness, no network.
Requires numpy + matplotlib only (Agg backend).
"""

import pathlib
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = pathlib.Path(__file__).parent
CSV = HERE / "simd_bench_results.csv"

COL_SCALAR = "#7f7f7f"
COL_SSE = "#1f77b4"
COL_AVX = "#d62728"
COL_PUB = "#9467bd"
PATH_COLORS = {"scalar": COL_SCALAR, "sse42": COL_SSE, "avx2": COL_AVX, "public": COL_PUB}
PATH_LABELS = {"scalar": "scalar", "sse42": "SSE4.2 path", "avx2": "AVX2 path",
               "public": "qwtSimdArgMinMax (dispatched)"}

# Cache geometry of the benchmark machine (i7-14700, thread pinned to a
# P-core): L1d 48 KB, P-core L2 2 MB, shared L3 33 MB -> doubles per level.
L1_N = 48 * 1024 // 8
L2_N = 2048 * 1024 // 8
L3_N = 33792 * 1024 // 8


# ------------------------------------------------------------------ loading

def load():
    rows = []
    env = {}
    for line in CSV.read_text(encoding="utf-8").splitlines():
        if line.startswith("#"):
            continue
        f = line.split(",")
        if len(f) != 10 or f[0] == "mode":
            continue
        rows.append(f)
        if f[0] == "env":
            env[f[1]] = f[2]
    return rows, env


def sel(rows, mode, path=None, p1=None, p2=None):
    out = []
    for r in rows:
        if r[0] != mode:
            continue
        if path is not None and r[1] != path:
            continue
        if p1 is not None and r[2] != str(p1):
            continue
        if p2 is not None and r[3] != str(p2):
            continue
        out.append(r)
    return out


def fnum(r, col):
    v = r[col]
    return float(v) if v else np.nan


def scan_table(rows):
    """n x path arrays of per-call min_ns from the scan section."""
    sizes = sorted({int(r[2]) for r in sel(rows, "scan")})
    paths = ["scalar", "sse42", "avx2", "public"]
    t = {p: np.full(len(sizes), np.nan) for p in paths}
    med = {p: np.full(len(sizes), np.nan) for p in paths}
    for i, n in enumerate(sizes):
        for p in paths:
            rr = sel(rows, "scan", p, n)
            if rr:
                t[p][i] = fnum(rr[0], 5)
                med[p][i] = fnum(rr[0], 6)
    return np.array(sizes), t, med


def fit_slope(n, t, nlo, nhi):
    """least-squares slope+intercept of t(ns) vs n over nlo <= n <= nhi"""
    m = (n >= nlo) & (n <= nhi)
    A = np.vstack([np.ones(m.sum()), n[m]]).T
    coef, *_ = np.linalg.lstsq(A, t[m], rcond=None)
    return coef[0], coef[1]


def style(ax):
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.tick_params(labelsize=9)


def main():
    rows, env = load()
    print("=" * 78)
    print("qwtSimdArgMinMax benchmark analysis  (data: %s)" % CSV.name)
    print("=" * 78)
    for k in ("cpu", "clock_ghz_probe", "msvc", "simd_level", "tsc_rate_per_ns",
              "logical_processors"):
        if k in env:
            print("env %-20s %s" % (k, env[k]))
    corr = sel(rows, "meta", "correctness")
    print("env correctness        %s" % (corr[0][2] if corr else "?"))
    remeas = [r for r in rows if "remeasured" in r[9]]
    print("protocol               min-of-7, warmup 3, ~50 ms runs; remeasured points: %d"
          % len(remeas))

    sizes, t, med = scan_table(rows)

    # probe table (used by several sections)
    probes = {}
    for r in sel(rows, "probe"):
        probes[(r[1], r[2], r[3])] = (fnum(r, 5), fnum(r, 6), fnum(r, 8))

    # ---------------- 1. size sweep
    print("\n--- 1. size sweep (per-call ns, min-of-7) ---")
    print("%10s %10s %10s %10s %10s | %6s %6s %6s" %
          ("n", "scalar", "sse42", "avx2", "public", "sp_sse", "sp_avx", "sp_pub"))
    for i, n in enumerate(sizes):
        sp = [t["scalar"][i] / t[p][i] for p in ("sse42", "avx2", "public")]
        print("%10d %10.2f %10.2f %10.2f %10.2f | %6.2f %6.2f %6.2f" %
              (n, t["scalar"][i], t["sse42"][i], t["avx2"][i], t["public"][i], *sp))

    worst = max(np.nanmax(med[p] / t[p]) for p in t)
    print("pollution check: worst median/min ratio over all scan points = %.3f" % worst)

    print("\nregional speedup vs scalar (min..max over sizes in region):")
    regions = [("L1 (n<=4K)", 8, 4096), ("L2 (32K..256K)", 32768, 262144),
               ("L3 (512K..4M)", 524288, 4194304), ("DRAM (8M..16M)", 8388608, 16777216)]
    for name, lo, hi in regions:
        m = (sizes >= lo) & (sizes <= hi)
        line = "  %-18s" % name
        for p in ("sse42", "avx2", "public"):
            s = t["scalar"][m] / t[p][m]
            line += " %s %.2f-%.2f " % (p, s.min(), s.max())
        print(line)

    # ---------------- 2. per-call cost model
    print("\n--- 2. per-call cost model  t(n) = F + n*r ---")
    print("    marginal rate r: least-squares over the L2-resident window (32K..1M);")
    print("    fixed cost F = t(16) - 16*r from the small-n point")
    fits = {}
    i16 = list(sizes).index(16)
    for p in ("scalar", "sse42", "avx2", "public"):
        _, r = fit_slope(sizes, t[p], 32768, 1048576)
        F = t[p][i16] - 16 * r
        fits[p] = (F, r)
        print("%-8s F = %8.2f ns   r = %6.4f ns/elem  (%.2f GB/s marginal)" %
              (p, F, r, 8.0 / r))
    Fa, ra = fits["avx2"]
    Fs, rs = fits["scalar"]
    Fq, rq = fits["sse42"]
    print("break-even (whole-array scan): avx2 vs scalar at n = %.0f" % ((Fs - Fa) / (ra - rs)))
    print("break-even (whole-array scan): avx2 vs sse42 at n = %.0f" % ((Fq - Fa) / (ra - rq)))
    print("break-even (whole-array scan): sse42 vs scalar at n = %.0f" % ((Fs - Fq) / (rq - rs)))
    ik16 = probes[("call_kernel_incr", "16", "")][0]
    ik4k = probes[("call_kernel_incr", "4096", "")][0]
    r_id = (ik4k - ik16) / (4096 - 16)
    print("ideal-AVX2 reference (incremental index vector, default flags): "
          "F ~ %.0f ns, r = %.4f ns/elem (%.1f GB/s marginal)" %
          (ik16 - 16 * r_id, r_id, 8.0 / r_id))

    # ---------------- 3. bucket mode
    chunks = sorted({int(r[3]) for r in sel(rows, "bucket")})
    ca = np.array(chunks, dtype=float)
    print("\n--- 3. bucket mode: 1M-element array, per-call ns ---")
    print("%8s %10s %10s %10s %10s | %8s %8s" %
          ("chunk", "scalar", "sse42", "avx2", "public", "sp_sse", "sp_pub"))
    bt = {p: [] for p in ("scalar", "sse42", "avx2", "public")}
    for c in chunks:
        vals = []
        for p in ("scalar", "sse42", "avx2", "public"):
            rr = sel(rows, "bucket", p, 1048576, c)
            v = fnum(rr[0], 5)
            bt[p].append(v)
            vals.append(v)
        print("%8d %10.1f %10.1f %10.1f %10.1f | %8.2f %8.2f" %
              (c, *vals, vals[0] / vals[1], vals[0] / vals[3]))
    bt = {p: np.array(v) for p, v in bt.items()}

    print("\nbucket per-call fits t = F + c*r, large-chunk window [256, 4096]:")
    bfits = {}
    for p in ("scalar", "sse42", "avx2", "public"):
        F, r = fit_slope(ca, bt[p], 256, 4096)
        bfits[p] = (F, r)
        print("%-8s F = %8.2f ns   r = %6.4f ns/elem" % (p, F, r))
    print("small-chunk window [4, 128]:")
    for p in ("scalar", "sse42", "avx2", "public"):
        F, r = fit_slope(ca, bt[p], 4, 128)
        print("%-8s F = %8.2f ns   r = %6.4f ns/elem" % (p, F, r))
    Fp, rp = bfits["public"]
    Fsc, rsc = bfits["scalar"]
    Fss, rss = bfits["sse42"]
    be_model = (Fsc - Fp) / (rp - rsc)
    sp256 = bt["scalar"][chunks.index(256)] / bt["public"][chunks.index(256)]
    sp512 = bt["scalar"][chunks.index(512)] / bt["public"][chunks.index(512)]
    cx = 256 * 2 ** ((0 - np.log(sp256)) / (np.log(sp512) - np.log(sp256)))
    print("break-even chunk (public/dispatched-AVX2 vs scalar): model %.0f,"
          " measured interpolation %.0f elements" % (be_model, cx))
    print("public vs sse42: sse42 faster for EVERY measured chunk: %s" %
          bool(np.all(bt["sse42"] < bt["public"])))
    print("sse42 vs scalar speedup range over chunks: %.2f-%.2f" %
          ((bt["scalar"] / bt["sse42"]).min(), (bt["scalar"] / bt["sse42"]).max()))
    for pts, W in ((1_000_000, 900), (10_000_000, 900), (1_000_000, 1920)):
        s = pts / (2 * W)
        print("realistic chunk s = %d pts / (2*%d px) = %.0f  ->  model per-call:"
              " scalar %.0f ns, public(AVX2) %.0f ns, sse42 %.0f ns" %
              (pts, W, s, Fsc + s * rsc, Fp + s * rp, Fss + s * rss))

    # ---------------- 4. bandwidth references / roofline inputs
    bwn = np.array(sorted({int(r[2]) for r in sel(rows, "bandwidth")}))
    print("\n--- 4. memory bandwidth references (GB/s, min-of-7) ---")
    print("%10s %12s %12s %12s | %10s %10s %10s" %
          ("n", "sum_scalar", "sum_avx2", "copy(r+w)", "scan_sc", "scan_sse", "scan_avx"))
    bw = {}
    for p in ("sum_scalar", "sum_avx2", "copy"):
        bw[p] = np.array([fnum(sel(rows, "bandwidth", p, n)[0], 8) for n in bwn])
    scan_gbs = {p: np.array([fnum(sel(rows, "scan", p, n)[0], 8)
                             if sel(rows, "scan", p, n) else np.nan for n in bwn])
                for p in ("scalar", "sse42", "avx2")}
    for i, n in enumerate(bwn):
        print("%10d %12.1f %12.1f %12.1f | %10.1f %10.1f %10.1f" %
              (n, bw["sum_scalar"][i], bw["sum_avx2"][i], bw["copy"][i],
               scan_gbs["scalar"][i], scan_gbs["sse42"][i], scan_gbs["avx2"][i]))

    def plateau(p, lo, hi):
        m = (sizes >= lo) & (sizes <= hi)
        return np.mean(8.0 * sizes[m] / t[p][m])

    pl_sc = plateau("scalar", 8192, 262144)
    pl_sse = plateau("sse42", 8192, 262144)
    pl_avx = plateau("avx2", 8192, 262144)
    print("\nkernel plateaus (L1/L2-resident, mean GB/s): scalar %.1f, sse42 %.1f, avx2 %.1f"
          % (pl_sc, pl_sse, pl_avx))
    m_dram = bwn >= 8388608
    dram_ceil = float(np.mean(bw["sum_avx2"][m_dram]))
    print("DRAM-resident ceilings (n>=8M): sum_scalar %.1f, sum_avx2 %.1f, copy %.1f GB/s"
          % (np.mean(bw["sum_scalar"][m_dram]), dram_ceil, np.mean(bw["copy"][m_dram])))
    below = bw["sum_avx2"] < max(pl_avx, pl_sse)
    if below.any():
        print("knee: sum_avx2 memory ceiling drops below the kernel plateaus at n >= %d"
              % bwn[np.argmax(below)])
    else:
        print("knee: the sum_avx2 memory ceiling stays above both kernel plateaus over the"
              " whole sweep (DRAM %.1f GB/s vs plateau %.1f) -> the kernels are"
              " compute/issue-bound at EVERY size, never bandwidth-bound"
              % (dram_ceil, max(pl_avx, pl_sse)))
    print("cache-level knees of sum_avx2 itself: 8K(L1/L2 border) %.0f -> L2 %.0f -> L3 %.0f -> DRAM %.0f GB/s"
          % (sa_at_border := float(bw["sum_avx2"][0]),
             np.mean(bw["sum_avx2"][(bwn > L1_N) & (bwn <= L2_N)]),
             np.mean(bw["sum_avx2"][(bwn > L2_N) & (bwn <= L3_N)]),
             np.mean(bw["sum_avx2"][bwn > L3_N])))
    print("kernel GB/s at DRAM size (16M): scalar %.1f, sse42 %.1f, avx2 %.1f"
          " (vs sum_avx2 ceiling %.1f: the 4-wide unroll limits DRAM streaming)"
          % (scan_gbs["scalar"][-1], scan_gbs["sse42"][-1], scan_gbs["avx2"][-1], dram_ceil))

    print("\narithmetic intensity (instruction counts from the MSVC /O2 disassembly):")
    g_incr = probes[("call_kernel_incr", "4096", "")][2]
    for name, ins, byt, g in (("scalar", 6.0, 8, pl_sc), ("sse42", 16.0, 16, pl_sse),
                              ("avx2 default", 24.0, 32, pl_avx),
                              ("avx2 incr-idx", 13.0, 32, g_incr)):
        ai = ins / byt
        print("  %-14s %.2f instr/byte   plateau %5.1f GB/s -> effective issue rate %.1f Ginstr/s"
              % (name, ai, g, g * ai))

    # ---------------- 5. localization probes
    print("\n--- 5. fixed-overhead localization probes (n=16 unless noted) ---")
    interesting = [
        ("call_scalar", "16"), ("call_sse_direct", "16"),
        ("call_avx2_direct", "16"), ("call_avx2_vz", "16"),
        ("call_avx2_pre_vz", "16"), ("call_avx2_via_avx2tu_ptr", "16"),
        ("call_avx2_wrapped", "16"), ("call_sse_wrapped", "16"),
        ("call_vz_only", "16"), ("call_public", "16"),
        ("call_loop_only", "16"), ("call_kernel_incr", "16"),
        ("call_vz_before_reduce", "16"), ("call_avx2_direct_2x", "16"),
        ("call_avx2_direct", "4096"), ("call_avx2_wrapped", "4096"),
        ("call_kernel_incr", "4096"), ("call_loop_only", "4096"),
        ("call_vz_before_reduce", "4096"),
        ("ecore_avx2_direct", "16"), ("ecore_avx2_direct", "4096"),
        ("ecore_sse_direct", "16"), ("ecore_sse_direct", "4096"),
        ("ecore_scalar", "16"), ("ecore_scalar", "4096"),
    ]
    for name, n in interesting:
        if (name, n, "") in probes:
            mn, md, g = probes[(name, n, "")]
            print("  %-28s n=%-5s min %8.2f ns  med %8.2f ns" % (name, n, mn, md))
    d2x = probes[("call_avx2_direct_2x", "16", "")][0]
    d1x = probes[("call_avx2_direct", "16", "")][0]
    print("additivity: two calls %.1f ns vs 2x single %.1f ns -> per-call constant overhead"
          % (d2x, 2 * d1x))

    print("\nlegacy-SSE chain probes (64 dependent legacy ADDPD, ns/round):")
    for name in ("chain_after_clean", "chain_after_dirty", "chain_after_avx2_call",
                 "chain_after_sse_call", "chain_after_avx2_vz", "chain_pre_vz_avx2_call"):
        key = (name, "64", "round=setup+chain")
        if key in probes:
            mn, md, _ = probes[key]
            print("  %-28s min %8.2f  med %8.2f" % (name, mn, md))
    dc = probes[("chain_after_dirty", "64", "round=setup+chain")][0]
    cc = probes[("chain_after_clean", "64", "round=setup+chain")][0]
    print("  dirty-vs-clean chain delta = %.2f ns -> a generic 'legacy SSE after dirty"
          " upper' penalty is NOT reproducible on this P-core" % (dc - cc))

    print("\nrdtsc-segmented verbatim kernel copies (median ns per segment):")
    segs = ["setup", "loop", "spill_store", "reduce", "tail"]
    for tag in ("instr_default", "instr_avx2tu"):
        for n in ("16", "4096"):
            vals = [fnum(sel(rows, "probe", tag, n, s)[0], 6)
                    if sel(rows, "probe", tag, n, s) else np.nan for s in segs]
            print("  %-14s n=%-5s " % (tag, n) +
                  "  ".join("%s=%.1f" % (s, v) for s, v in zip(segs, vals)))

    # ---------------- 6. NaN matrix
    print("\n--- 6. NaN semantics matrix (untimed, n=16) ---")
    nan_rows = sel(rows, "nan")
    pats = []
    for r in nan_rows:
        if r[2] == "16" and r[1] == "reference" and r[3] not in pats:
            pats.append(r[3])

    def cell(pat, path):
        rr = [r for r in nan_rows if r[2] == "16" and r[3] == pat and r[1] == path]
        if not rr:
            return "?"
        return rr[0][9].replace("ref: ", "")

    print("%-11s | %-34s | %-24s | %-24s | %-24s" %
          ("pattern", "reference(NaN-free)", "scalar", "sse42", "avx2 (=public)"))
    for pat in pats:
        av = cell(pat, "avx2")
        pb = cell(pat, "public")
        avp = av if av == pb else "raw " + av + " / pub " + pb
        print("%-11s | %-34s | %-24s | %-24s | %s" %
              (pat, cell(pat, "reference"), cell(pat, "scalar"),
               cell(pat, "sse42"), avp))
    nanpats = [p for p in pats if p != "none"]
    mism = sum(1 for pat in nanpats for p in ("scalar", "sse42", "avx2")
               if cell(pat, p) != cell(pat, "reference"))
    print("cells disagreeing with the NaN-free reference (%d NaN patterns x 3 raw paths): %d/%d"
          % (len(nanpats), mism, 3 * len(nanpats)))

    # ======================================================= figures

    # fig 1: speedup
    fig, ax = plt.subplots(figsize=(7.8, 3.9))
    for p in ("sse42", "avx2", "public"):
        ax.plot(sizes, t["scalar"] / t[p], "o-", ms=3.4, lw=1.4,
                color=PATH_COLORS[p], label=PATH_LABELS[p])
    ax.axhline(1.0, color="#bbbbbb", ls="--", lw=1)
    for xv, lab in ((L1_N, "L1 48KB"), (L2_N, "L2 2MB"), (L3_N, "L3 33MB")):
        ax.axvline(xv, color="#dddddd", ls=":", lw=1)
        ax.text(xv * 1.15, 0.035, lab, fontsize=8, color="#888888", rotation=90, va="bottom")
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.set_xticks([8, 64, 512, 4096, 32768, 262144, 2 ** 21, 2 ** 24])
    ax.set_xticklabels(["8", "64", "512", "4K", "32K", "256K", "2M", "16M"], fontsize=8)
    ax.set_xlabel("array size n (doubles)")
    ax.set_ylabel("speedup vs scalar path")
    ax.set_title("whole-array scan: speedup of the SIMD paths over scalar\n"
                 "(shipped default-flag build; the AVX2 path pays ~330 ns fixed cost per call)",
                 fontsize=9.5)
    ax.legend(fontsize=8.5, frameon=False, loc="upper left")
    ax.grid(True, which="major", alpha=0.15)
    style(ax)
    fig.tight_layout()
    fig.savefig(HERE / "fig_simd_speedup.png", dpi=200, bbox_inches="tight")
    plt.close(fig)

    # fig 2: bandwidth vs n with memory ceilings
    fig, ax = plt.subplots(figsize=(7.8, 4.1))
    for p in ("scalar", "sse42", "avx2"):
        ax.plot(sizes, 8.0 * sizes / t[p], "o-", ms=3.2, lw=1.4,
                color=PATH_COLORS[p], label="argminmax " + PATH_LABELS[p])
    ax.plot(bwn, bw["sum_avx2"], "s--", ms=3.4, lw=1.2, color="#2ca02c",
            label="sum reduce (AVX2, read ceiling)")
    ax.plot(bwn, bw["sum_scalar"], "^--", ms=3.4, lw=1.2, color="#8c564b",
            label="sum reduce (scalar)")
    ax.plot(bwn, bw["copy"], "v--", ms=3.4, lw=1.2, color="#17becf", label="memcpy (read+write)")
    for xv, lab in ((L1_N, "L1"), (L2_N, "L2"), (L3_N, "L3")):
        ax.axvline(xv, color="#dddddd", ls=":", lw=1)
        ax.text(xv * 1.1, 1.3, lab, fontsize=8.5, color="#888888")
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.set_xticks([8, 64, 512, 4096, 32768, 262144, 2 ** 21, 2 ** 24])
    ax.set_xticklabels(["8", "64", "512", "4K", "32K", "256K", "2M", "16M"], fontsize=8)
    ax.set_xlabel("array size n (doubles)")
    ax.set_ylabel("effective bandwidth (GB/s)")
    ax.set_title("argminmax bandwidth vs measured memory ceilings (single P-core)\n"
                 "the kernels stay below every memory ceiling until DRAM sizes: issue/latency-bound",
                 fontsize=9.5)
    ax.legend(fontsize=7.4, frameon=False, ncol=2, loc="upper right")
    ax.grid(True, which="major", alpha=0.15)
    style(ax)
    fig.tight_layout()
    fig.savefig(HERE / "fig_simd_bandwidth.png", dpi=200, bbox_inches="tight")
    plt.close(fig)

    # fig 3: bucket mode
    fig, ax = plt.subplots(figsize=(7.8, 4.1))
    for p in ("scalar", "sse42", "avx2", "public"):
        ax.plot(ca, bt[p], "o-", ms=4, lw=1.4, color=PATH_COLORS[p], label=PATH_LABELS[p])
        F, r = bfits[p]
        xx = np.linspace(256, 4096, 50)
        ax.plot(xx, F + xx * r, "--", lw=1.0, color=PATH_COLORS[p], alpha=0.55)
    ax.axvline(556, color="#333333", ls=":", lw=1.2)
    ax.text(590, 13, "realistic chunk\n1M pts / (2x900 px)\n= 556", fontsize=8, color="#333333")
    ax.axvline(cx, color=COL_PUB, ls="-.", lw=1.0, alpha=0.7)
    ax.text(cx * 0.42, 1500, "public=scalar\nbreak-even\n~%.0f" % cx, fontsize=8, color=COL_PUB)
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.set_xticks([4, 16, 64, 256, 1024, 4096])
    ax.set_xticklabels(["4", "16", "64", "256", "1024", "4096"], fontsize=8.5)
    ax.set_xlabel("chunk (bucket) size, elements per qwtSimdArgMinMax call")
    ax.set_ylabel("per-call cost (ns)")
    ax.set_title("bucket mode: 1M-element array consumed as consecutive chunks\n"
                 "(how qwtMinMaxBucketReduce calls the kernel; dashed = F + c*r fits, window 256-4096)",
                 fontsize=9.5)
    ax.legend(fontsize=8.5, frameon=False, loc="upper left")
    ax.grid(True, which="major", alpha=0.15)
    style(ax)
    fig.tight_layout()
    fig.savefig(HERE / "fig_simd_bucket.png", dpi=200, bbox_inches="tight")
    plt.close(fig)

    # fig 4: localization probes
    order = [
        ("call_scalar", "scalar path"),
        ("call_sse_direct", "SSE4.2 path"),
        ("ecore_avx2_direct", "AVX2 default build, on E-core"),
        ("call_avx2_wrapped", "AVX2, /arch:AVX2-compiled copy"),
        ("call_loop_only", "AVX2 vector loop only (no epilogue)"),
        ("call_vz_before_reduce", "AVX2 default + vzeroupper before reduce"),
        ("call_kernel_incr", "AVX2 default, incremental index vec"),
        ("call_avx2_via_avx2tu_ptr", "AVX2 default build, AVX2 caller"),
        ("call_avx2_pre_vz", "AVX2 default build, vz before call"),
        ("call_avx2_vz", "AVX2 default build, vz after call"),
        ("call_avx2_direct", "AVX2 default build (= shipped)"),
    ]
    labels, vals, colors = [], [], []
    for key, lab in order:
        if (key, "16", "") in probes:
            labels.append(lab)
            vals.append(probes[(key, "16", "")][0])
            colors.append(COL_AVX if "default build" in lab else
                          (COL_SCALAR if lab == "scalar path" else COL_SSE))
    ypos = np.arange(len(vals))
    fig, ax = plt.subplots(figsize=(8.4, 4.3))
    ax.barh(ypos, vals, color=colors, alpha=0.85, height=0.62)
    for y, v in zip(ypos, vals):
        ax.text(v * 1.05 + 1.5, y, "%.1f ns" % v, va="center", fontsize=8.5)
    ax.set_yticks(ypos)
    ax.set_yticklabels(labels, fontsize=8.5)
    ax.set_xscale("log")
    ax.set_xlim(3, 1400)
    ax.set_xlabel("per-call cost, n = 16 (ns, log scale)")
    ax.set_title("where the ~330 ns fixed cost comes from: controlled variants of the same call\n"
                 "(all binaries default-flag compiled except the '/arch:AVX2' and E-core rows)",
                 fontsize=9.5)
    ax.grid(True, axis="x", alpha=0.15)
    style(ax)
    fig.tight_layout()
    fig.savefig(HERE / "fig_simd_probes.png", dpi=200, bbox_inches="tight")
    plt.close(fig)

    # fig 5: roofline
    fig, ax = plt.subplots(figsize=(7.8, 4.5))
    sa = bw["sum_avx2"]
    mem_ceil = [
        ("L2 sum_avx2", float(np.mean(sa[(bwn > L1_N) & (bwn <= L2_N)]))),
        ("L3 sum_avx2", float(np.mean(sa[(bwn > L2_N) & (bwn <= L3_N)]))),
        ("DRAM sum_avx2", dram_ceil)]
    for lab, c in mem_ceil:
        ax.plot([0.1, 3.0], [c, c], ls="--", lw=1.1, color="#2ca02c", alpha=0.55)
        ax.text(0.105, c * 1.07, "%s %.0f GB/s" % (lab, c), fontsize=7.8, color="#2ca02c")
    kern = [
        ("scalar (6 instr/8B)", 6 / 8, pl_sc, COL_SCALAR, "o", -40),
        ("sse42 (16/16B)", 16 / 16, pl_sse, COL_SSE, "s", 14),
        ("avx2 default (24/32B)", 24 / 32, pl_avx, COL_AVX, "^", 16),
        ("avx2 incr-idx (13/32B)", 13 / 32, g_incr, "#ff7f0e", "D", -46),
    ]
    for lab, ai, g, c, mk, dy in kern:
        ax.plot([ai], [g], mk, ms=8, color=c, zorder=5)
        # label offset must be in POINTS: data-space offsets go negative on
        # the log y-axis and blow up the tight bbox (dy was meant as pixels)
        ax.annotate("%s\n%.1f GB/s" % (lab, g), (ai, g),
                    textcoords="offset points", xytext=(34, dy),
                    fontsize=8, color=c, va="center",
                    arrowprops=dict(arrowstyle="-", color=c, lw=0.6))
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlim(0.1, 3.0)
    ax.set_ylim(3, 600)
    ax.set_xlabel("operational intensity (retired instructions per byte loaded)")
    ax.set_ylabel("attainable bandwidth (GB/s)")
    ax.set_title("measured roofline: all kernels sit deep in the compute-bound region\n"
                 "(memory ceilings: sum-reduce/memcpy probes; kernel points: scan plateaus)",
                 fontsize=9.5)
    ax.grid(True, which="major", alpha=0.15)
    style(ax)
    fig.tight_layout()
    fig.savefig(HERE / "fig_simd_roofline.png", dpi=200, bbox_inches="tight")
    plt.close(fig)

    print("\nfigures written to", HERE)


if __name__ == "__main__":
    main()
