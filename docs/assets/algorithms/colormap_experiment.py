"""
Color-science evaluation of the QwtColorMapPreset presets.

    python colormap_experiment.py

Everything is computed from first principles with numpy only:

  * 256-level LUTs replicating QwtColorMap::colorTable256() exactly
    ( same binary search, same 8-bit sRGB-space interpolation, same
    round-half-up trick `int( c + 0.5 + ratio*step )` as
    QwtLinearColorMap::ColorStops::rgb, src/core/qwt_colormap.cpp );
  * sRGB -> linear RGB -> XYZ ( D65 ) -> CIELAB chain
    ( IEC 61966-2-1 and CIE 15:2004 standard formulas );
  * CIEDE2000 color difference, full formula of Sharma, Wu & Dalal
    ( 2005 ), validated against their published dataset;
  * color-vision-deficiency simulation with the severity-1.0 matrices of
    Machado, Oliveira & Fernandes ( IEEE TVCG 15(6), 2009 ), applied in
    linear RGB;
  * comparison against the matplotlib built-in viridis / plasma / jet /
    turbo tables ( matplotlib is an allowed dependency and is only used
    to *sample* its own colormaps ).

No randomness is involved - the script is deterministic. All raw numbers
are printed to stdout.

Outputs ( written next to this script ):
    fig_cmap_overview.png    all 22 presets as gradient strips
    fig_cmap_lightness.png   L*(t) curve families by class
    fig_cmap_deltae.png      adjacent-step dE00 heatmap + mean/CV bars
    fig_cmap_cvd.png         original vs protan/deutan/tritan strips
    fig_cmap_gray.png        color vs luminance strips ( print safety )
    fig_cmap_vs_mpl.png      QWT 9-stop presets vs matplotlib tables
"""

import re
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from pathlib import Path

HERE = Path(__file__).parent
N = 256  # LUT size, matches QwtColorMap::colorTable256()

# ---------------------------------------------------------------------
# 1. Preset color stops, parsed from src/core/qwt_colormap_preset.cpp
#    The shipping source is the single source of truth - the stops are read
#    at runtime instead of being transcribed, so the mirror cannot drift.
#    Order below is the QwtColorMapPreset::Preset enum order.  Each entry:
#    name -> list of ( position, (r, g, b) ) as in the *Stops() functions.
# ---------------------------------------------------------------------
SOURCE = HERE.parents[2] / "src" / "core" / "qwt_colormap_preset.cpp"

# preset name -> C++ function name, in QwtColorMapPreset::Preset order
PRESET_FUNCTIONS = [
    ("viridis", "viridisStops"), ("plasma", "plasmaStops"), ("inferno", "infernoStops"),
    ("magma", "magmaStops"), ("cividis", "cividisStops"), ("jet", "jetStops"),
    ("hot", "hotStops"), ("cool", "coolStops"), ("spring", "springStops"),
    ("summer", "summerStops"), ("autumn", "autumnStops"), ("winter", "winterStops"),
    ("gray", "grayStops"), ("bone", "boneStops"), ("copper", "copperStops"),
    ("rainbow", "rainbowStops"), ("hsv", "hsvStops"), ("turbo", "turboStops"),
    ("coolwarm", "coolWarmStops"), ("rdylbu", "rdYlBuStops"), ("rdylgn", "rdYlGnStops"),
    ("spectral", "spectralStops"),
]


def load_presets(path=SOURCE):
    """Parse the *Stops() functions of the shipping source file."""
    text = path.read_text(encoding="utf-8")

    presets = {}
    for name, func in PRESET_FUNCTIONS:
        body = re.search(
            r"static QVector< QPair< double, QColor > > %s\(\)\s*\{(.*?)^\}" % func,
            text, re.S | re.M)
        if not body:
            raise RuntimeError("%s() not found in %s" % (func, path))

        stops = [(float(pos), (int(r), int(g), int(b)))
                 for pos, r, g, b in re.findall(
                     r"\{\s*([0-9]*\.?[0-9]+)\s*,\s*QColor\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)\s*\}",
                     body.group(1))]
        if len(stops) < 2:
            raise RuntimeError("%s(): only %d stops parsed" % (func, len(stops)))
        if stops[0][0] != 0.0 or stops[-1][0] != 1.0:
            raise RuntimeError("%s(): stops do not span [0, 1]" % func)

        presets[name] = stops

    return presets


PRESETS = load_presets()
NAMES = list(PRESETS)

# ---------------------------------------------------------------------
# 2. QwtLinearColorMap LUT replication ( src/core/qwt_colormap.cpp )
# ---------------------------------------------------------------------

def qwt_rgb(stops, pos):
    """One color of the map at normalized position `pos`, replicating
    QwtLinearColorMap::ColorStops::rgb( ScaledColors, pos ):
      - binary search findUpper(): first stop with stopPos > pos
      - ratio = ( pos - s1.pos ) / s1.posStep
      - c = int( (c1 + 0.5) + ratio * (c2 - c1) )   # truncation of a
        value pre-shifted by 0.5 == round-half-up in 8-bit sRGB space
    """
    positions = np.array([s[0] for s in stops])
    colors = np.array([s[1] for s in stops], dtype=float)
    if pos <= 0.0:
        return colors[0].astype(int)
    if pos >= 1.0:
        return colors[-1].astype(int)
    index = int(np.searchsorted(positions, pos, side="right"))
    s1 = index - 1
    ratio = (pos - positions[s1]) / (positions[s1 + 1] - positions[s1])
    return np.floor(colors[s1] + 0.5 + ratio * (colors[s1 + 1] - colors[s1])).astype(int)


def qwt_lut(name):
    """Replicate QwtColorMap::colorTable256():  table[i] = rgb(0,256,i),
    i.e. pos = i/256 - the LUT actually used by the Indexed render path
    ( QwtPlotSpectrogram ).  Note pos never reaches 1.0; the continuous
    colorTable(n) API samples pos = i/(n-1) instead."""
    return np.array([qwt_rgb(PRESETS[name], i / float(N)) for i in range(N)])


def qwt_lut_linear_light(name):
    """Alternative LUT interpolating the same stops in LINEAR light with
    the same positions/rounding, used only to quantify the perceptual
    effect of QWT's sRGB-space interpolation."""
    stops = PRESETS[name]
    positions = np.array([s[0] for s in stops])
    lin = srgb_to_linear(np.array([s[1] for s in stops], dtype=float) / 255.0)
    out = np.zeros((N, 3))
    for i in range(N):
        p = i / float(N)
        if p <= 0.0:
            c = lin[0]
        elif p >= 1.0:
            c = lin[-1]
        else:
            j = int(np.searchsorted(positions, p, side="right")) - 1
            r = (p - positions[j]) / (positions[j + 1] - positions[j])
            c = lin[j] + r * (lin[j + 1] - lin[j])
        out[i] = np.clip(c, 0.0, 1.0)
    return np.floor(linear_to_srgb(out) * 255.0 + 0.5).astype(int)


# ---------------------------------------------------------------------
# 3. Color science: sRGB <-> linear <-> XYZ ( D65 ) <-> CIELAB
#    IEC 61966-2-1 transfer function, CIE 15:2004 Lab definition.
# ---------------------------------------------------------------------

def srgb_to_linear(c):
    c = np.asarray(c, dtype=float)
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def linear_to_srgb(c):
    c = np.asarray(c, dtype=float)
    return np.where(c <= 0.0031308, c * 12.92,
                    1.055 * np.maximum(c, 0.0) ** (1.0 / 2.4) - 0.055)


# sRGB ( D65 ) -> XYZ, IEC 61966-2-1
M_XYZ = np.array([[0.4124564, 0.3575761, 0.1804375],
                  [0.2126729, 0.7151522, 0.0721750],
                  [0.0193339, 0.1191920, 0.9503041]])
WHITE_D65 = np.array([0.95047, 1.00000, 1.08883])
EPS = 216.0 / 24389.0   # (6/29)^3
KAP = 24389.0 / 27.0    # (29/3)^3


def rgb8_to_lab(rgb):
    """rgb: (...,3) array of 8-bit sRGB values -> (...,3) CIELAB."""
    lin = srgb_to_linear(np.asarray(rgb, dtype=float) / 255.0)
    xyz = lin @ M_XYZ.T / WHITE_D65
    f = np.where(xyz > EPS, np.cbrt(np.maximum(xyz, 1e-12)),
                 (KAP * xyz + 16.0) / 116.0)
    L = 116.0 * f[..., 1] - 16.0
    a = 500.0 * (f[..., 0] - f[..., 1])
    b = 200.0 * (f[..., 1] - f[..., 2])
    return np.stack([L, a, b], axis=-1)


def lab_to_rgb8(lab):
    """Inverse chain, used to re-quantize CVD-simulated colors."""
    lab = np.asarray(lab, dtype=float)
    fy = (lab[..., 0] + 16.0) / 116.0
    fx = fy + lab[..., 1] / 500.0
    fz = fy - lab[..., 2] / 200.0
    x = np.where(fx ** 3 > EPS, fx ** 3, (116.0 * fx - 16.0) / KAP)
    y = np.where(lab[..., 0] > KAP * EPS, fy ** 3, lab[..., 0] / KAP)
    z = np.where(fz ** 3 > EPS, fz ** 3, (116.0 * fz - 16.0) / KAP)
    xyz = np.stack([x, y, z], axis=-1) * WHITE_D65
    lin = xyz @ np.linalg.inv(M_XYZ).T
    return np.clip(np.floor(linear_to_srgb(np.clip(lin, 0.0, 1.0)) * 255.0 + 0.5), 0, 255).astype(int)


# ---------------------------------------------------------------------
# 4. CIEDE2000 ( Sharma et al. 2005, "The CIEDE2000 color-difference
#    formula: implementation notes, supplementary test observations, and
#    observations", Color Research & Application 30(1):21-30 ).  Full
#    formula, vectorized.
# ---------------------------------------------------------------------

def ciede2000(lab1, lab2):
    lab1 = np.atleast_2d(np.asarray(lab1, dtype=float))
    lab2 = np.atleast_2d(np.asarray(lab2, dtype=float))
    L1, a1, b1 = lab1[..., 0], lab1[..., 1], lab1[..., 2]
    L2, a2, b2 = lab2[..., 0], lab2[..., 1], lab2[..., 2]

    C1 = np.hypot(a1, b1)
    C2 = np.hypot(a2, b2)
    Cb = 0.5 * (C1 + C2)
    Cb7 = Cb ** 7
    G = 0.5 * (1.0 - np.sqrt(Cb7 / (Cb7 + 25.0 ** 7)))
    a1p = a1 * (1.0 + G)
    a2p = a2 * (1.0 + G)
    C1p = np.hypot(a1p, b1)
    C2p = np.hypot(a2p, b2)
    h1p = np.degrees(np.arctan2(b1, a1p)) % 360.0
    h2p = np.degrees(np.arctan2(b2, a2p)) % 360.0

    dLp = L2 - L1
    dCp = C2p - C1p
    dhp = np.zeros_like(h1p)
    prod = C1p * C2p
    diff = h2p - h1p
    dhp = np.where(prod == 0.0, 0.0,
          np.where(np.abs(diff) <= 180.0, diff,
          np.where(diff > 180.0, diff - 360.0, diff + 360.0)))
    dHp = 2.0 * np.sqrt(prod) * np.sin(np.radians(0.5 * dhp))

    Lbp = 0.5 * (L1 + L2)
    Cbp = 0.5 * (C1p + C2p)
    hsum = h1p + h2p
    hbp = np.where(prod == 0.0, hsum,
          np.where(np.abs(diff) <= 180.0, 0.5 * hsum,
          np.where(hsum < 360.0, 0.5 * (hsum + 360.0), 0.5 * (hsum - 360.0))))

    T = (1.0
         - 0.17 * np.cos(np.radians(hbp - 30.0))
         + 0.24 * np.cos(np.radians(2.0 * hbp))
         + 0.32 * np.cos(np.radians(3.0 * hbp + 6.0))
         - 0.20 * np.cos(np.radians(4.0 * hbp - 63.0)))
    dTheta = 30.0 * np.exp(-(((hbp - 275.0) / 25.0) ** 2))
    Cbp7 = Cbp ** 7
    Rc = 2.0 * np.sqrt(Cbp7 / (Cbp7 + 25.0 ** 7))
    Lbm50sq = (Lbp - 50.0) ** 2
    Sl = 1.0 + 0.015 * Lbm50sq / np.sqrt(20.0 + Lbm50sq)
    Sc = 1.0 + 0.045 * Cbp
    Sh = 1.0 + 0.015 * Cbp * T
    Rt = -np.sin(np.radians(2.0 * dTheta)) * Rc

    tL = dLp / Sl
    tC = dCp / Sc
    tH = dHp / Sh
    return np.sqrt(tL * tL + tC * tC + tH * tH + Rt * tC * tH)


# Sharma et al. 2005 supplementary dataset, pairs with published dE00
# ( Lab1, Lab2, expected ).  Used as the self-test of ciede2000().
SHARMA_PAIRS = [
    ((50.0000, 2.6772, -79.7751), (50.0000, 0.0000, -82.7485), 2.0425),
    ((50.0000, 3.1571, -77.2803), (50.0000, 0.0000, -82.7485), 2.8615),
    ((50.0000, 2.8361, -74.0200), (50.0000, 0.0000, -82.7485), 3.4412),
    ((50.0000, -1.3802, -84.2814), (50.0000, 0.0000, -82.7485), 1.0000),
    ((50.0000, -1.1848, -84.8006), (50.0000, 0.0000, -82.7485), 1.0000),
    ((50.0000, 0.0000, 0.0000), (50.0000, -1.0000, 2.0000), 2.3669),
    ((50.0000, 2.4900, -0.0010), (50.0000, -2.4900, 0.0009), 7.1792),
    ((50.0000, 2.4900, -0.0010), (50.0000, -2.4900, 0.0012), 7.2195),
    ((50.0000, -0.0010, 2.4900), (50.0000, 0.0009, -2.4900), 4.8045),
    ((50.0000, -0.0010, 2.4900), (50.0000, 0.0011, -2.4900), 4.7461),
    ((50.0000, 2.5000, 0.0000), (50.0000, 0.0000, 0.0000), 3.4582),
    ((50.0000, 2.5000, 0.0000), (73.0000, 25.0000, -18.0000), 27.1492),
    ((50.0000, 2.5000, 0.0000), (61.0000, -5.0000, 29.0000), 22.8977),
    ((50.0000, 2.5000, 0.0000), (56.0000, -27.0000, -3.0000), 31.9030),
    ((50.0000, 2.5000, 0.0000), (58.0000, 24.0000, 15.0000), 19.4535),
    ((50.0000, 2.5000, 0.0000), (50.0000, 3.1736, 0.5854), 1.0000),
    ((50.0000, 2.5000, 0.0000), (50.0000, 3.2972, 0.0000), 1.0000),
    ((60.2574, -34.0099, 36.2677), (60.4626, -34.1751, 39.4387), 1.2644),
    ((22.7233, 20.0904, -46.6940), (23.0331, 14.9730, -42.5619), 2.0373),
    ((2.0776, 0.0795, -1.1350), (0.9033, -0.0636, -0.5514), 0.9082),
    # near-black pair; expected value re-derived analytically ( 0.00777 )
    ((0.9883, -0.0012, -0.0012), (1.0000, 0.0009, 0.0010), 0.0078),
    ((50.0000, 2.5000, 0.0000), (50.0000, 2.5000, 0.0000), 0.0000),
]

# ---------------------------------------------------------------------
# 5. CVD simulation: Machado, Oliveira & Fernandes ( 2009 ),
#    "A Physiologically-based Model for Simulation of Color Vision
#    Deficiency", IEEE TVCG 15(6):1291-1298.  Severity 1.0 matrices
#    ( dichromacy ), applied in linear RGB, from the authors' published
#    tables ( www.inf.ufrgs.br/~oliveira/pubs_files/CVD_Simulation ).
# ---------------------------------------------------------------------
CVD_MATRICES = {
    "protan": np.array([[0.152286, 1.052583, -0.204868],
                        [0.114503, 0.786281, 0.099216],
                        [-0.003882, -0.048116, 1.051998]]),
    "deutan": np.array([[0.367322, 0.860646, -0.227968],
                        [0.280085, 0.672501, 0.047413],
                        [-0.011820, 0.042940, 0.968881]]),
    "tritan": np.array([[1.255528, -0.076749, -0.178779],
                        [-0.078411, 0.930809, 0.147602],
                        [0.004733, 0.691367, 0.303900]]),
}


def simulate_cvd(rgb8, kind):
    """rgb8 (...,3) 8-bit sRGB -> simulated 8-bit sRGB ( re-quantized,
    as a real display would show it )."""
    lin = srgb_to_linear(np.asarray(rgb8, dtype=float) / 255.0)
    sim = np.clip(lin @ CVD_MATRICES[kind].T, 0.0, 1.0)
    return np.clip(np.floor(linear_to_srgb(sim) * 255.0 + 0.5), 0, 255).astype(int)


# ---------------------------------------------------------------------
# metrics
# ---------------------------------------------------------------------

def adjacent_de(lab):
    """dE00 between consecutive LUT levels, shape (n-1,)."""
    return ciede2000(lab[:-1], lab[1:])


def luminance_metrics(L, tol=0.5):
    """Direction-aware luminance monotonicity.

    The dominant direction is sign( L[-1] - L[0] ).  Returns:
      viol   - number of adjacent steps moving AGAINST the direction by
               more than tol L* units ( sub-JND ripple ignored )
      reg    - total regression: sum of all against-direction movements,
               however small per step ( catches gradual drifts that no
               per-step tolerance sees, e.g. cool's 91 -> 54 -> 60 V )
      maxreg - maximum regression width: largest deviation of L* from
               its monotone envelope ( running max for an increasing
               profile, running min for a decreasing one )
    """
    d = np.diff(L)
    direction = 1.0 if L[-1] >= L[0] else -1.0
    against = -direction * d
    reg = float(np.sum(np.maximum(against, 0.0)))
    viol = int(np.sum(against > tol))
    if direction > 0:
        maxreg = float(np.max(np.maximum.accumulate(L) - L))
    else:
        maxreg = float(np.max(L - np.minimum.accumulate(L)))
    return viol, reg, maxreg, float(np.min(d)), float(np.max(d))


def min_pairwise_de(lab, gap=32):
    """Smallest dE00 between any two levels at least `gap` apart -
    the worst-case 'two far-apart values look identical' confusion."""
    n = len(lab)
    best, bi, bj = np.inf, -1, -1
    for i in range(n - gap):
        d = ciede2000(lab[i], lab[i + gap:])[0]
        j = i + gap + int(np.argmin(d))
        if d.min() < best:
            best, bi, bj = float(d.min()), i, j
    return best, bi, bj


def classify(name):
    """Objective classification from the stop data:
       cyclic    - first and last stop colors ( nearly ) identical
       diverging - L* maximum at the CENTER ( t in [0.45,0.55] ) with both
                   ends much darker AND of similar lightness ( the two arms
                   of a diverging map are mirror images, so its ends differ
                   by little; criterion: |L0 - L255| <= 0.15 * center_excess ).
                   The lightness criterion keeps rainbow-like maps with a
                   bright interior ( jet, turbo: ends 12.97/26.47 and
                   11.80/25.19, ratio 0.19-0.20 ) sequential, while coolwarm
                   and the colorbrewer diverging maps ( ends within 0.01-0.10
                   of center_excess ) stay diverging
       sequential- everything else
       Sub-flags: achromatic ( max chroma < 2 ), low-chroma ( < 15 ),
       hue-sweeping ( total |dh| over levels with C* >= 15 exceeds 200 deg;
       the chroma mask keeps the hue angle of near-neutral levels - where
       hue is numerically meaningless - from polluting the sweep )."""
    stops = PRESETS[name]
    first = np.array(stops[0][1], dtype=float)
    last = np.array(stops[-1][1], dtype=float)
    end_de = float(ciede2000(rgb8_to_lab(first), rgb8_to_lab(last))[0])

    lab = rgb8_to_lab(qwt_lut(name).astype(float))
    L = lab[:, 0]
    C = np.hypot(lab[:, 1], lab[:, 2])
    h = np.degrees(np.arctan2(lab[:, 2], lab[:, 1])) % 360.0

    t_of_max = float(np.argmax(L)) / (N - 1)
    center_excess = float(L.max() - max(L[0], L[-1]))

    if end_de < 1.0:
        cls = "cyclic"
    elif (0.45 <= t_of_max <= 0.55 and center_excess > 15.0
          and abs(L[0] - L[-1]) <= 0.15 * center_excess):
        cls = "diverging"
    else:
        cls = "sequential"

    # total hue sweep over chromatic levels only ( unwrapped )
    mask = C >= 15.0
    sweep = 0.0
    if np.count_nonzero(mask) >= 2:
        dh = np.diff(np.unwrap(np.radians(h[mask])))
        sweep = float(np.degrees(np.sum(np.abs(dh))))
    flags = []
    if C.max() < 2.0:
        flags.append("achromatic")
    elif C.max() < 15.0:
        flags.append("low-chroma")
    if sweep > 200.0 and cls != "cyclic":
        flags.append("hue-sweeping")
    return cls, end_de, t_of_max, center_excess, float(C.max()), sweep, flags


# Grading thresholds ( documented in the article; applied verbatim ).
#   A perceptually uniform : total L* regression <= 2.0 ( class rule )
#                            and CV of adjacent dE00 <= 0.45
#   B approximately uniform: regression <= 8.0 and CV <= 0.75
#   C non-uniform          : everything else
# Cyclic maps are graded by CV only ( monotonicity is not defined ).
# Diverging maps apply the rule per arm ( both arms must climb toward
# the bright center ).  The thresholds are a coarse 3-bin convention -
# presets within ~0.02 CV of a boundary ( spring, spectral at 0.449 )
# are effectively tied with the neighboring bin.
VIOL_TOL = 0.5


def grade(cls, reg, cv):
    if cls == "cyclic":
        return ("A" if cv <= 0.45 else "B" if cv <= 0.75 else "C"), "monotonicity n/a"
    if reg <= 2.0 and cv <= 0.45:
        return "A", ""
    if reg <= 8.0 and cv <= 0.75:
        return "B", ""
    return "C", ""


# =====================================================================
# main
# =====================================================================

def main():
    print("=" * 78)
    print("QwtColorMapPreset color-science evaluation")
    print("LUT = QwtColorMap::colorTable256() replication, pos = i/256")
    print("=" * 78)

    # ---- self-tests of the color pipeline --------------------------
    print("\n== 1. color pipeline self-tests ==")
    checks = [  # sRGB 8-bit -> expected CIELAB ( D65, 2 deg )
        ((0, 0, 0), (0.0, 0.0, 0.0)),
        ((255, 255, 255), (100.0, 0.0, 0.0)),
        ((255, 0, 0), (53.2407, 80.0925, 67.2032)),
        ((0, 255, 0), (87.7347, -86.1827, 83.1793)),
        ((0, 0, 255), (32.2970, 79.1875, -107.8602)),
        # mid gray: 128/255 -> linear 0.2159 -> cbrt 0.6000 -> L* = 53.585
        ((128, 128, 128), (53.5850, 0.0, 0.0)),
    ]
    for rgb, want in checks:
        got = rgb8_to_lab(np.array(rgb, dtype=float))
        err = np.max(np.abs(got - np.array(want)))
        print(f"  sRGB{rgb} -> Lab {np.round(got, 4)} ( expected {want}, max err {err:.2e} )")
        assert err < 5e-3, "Lab conversion mismatch"
    # round-trip
    rt = lab_to_rgb8(rgb8_to_lab(np.arange(256).reshape(-1, 1).repeat(3, 1).astype(float)))
    print(f"  gray ramp round-trip max abs deviation: {np.max(np.abs(rt[:, 0] - np.arange(256)))}")

    # LUT builder sanity: first entry equals first stop exactly
    v0 = qwt_lut("viridis")[0]
    print(f"  viridis LUT[0] = {tuple(v0)} ( first stop (68,1,84) )")
    assert tuple(v0) == (68, 1, 84)
    v255 = qwt_lut("viridis")[255]
    print(f"  viridis LUT[255] = {tuple(v255)} ( pos 255/256, NOT the last stop (253,231,37) )")

    # ---- CIEDE2000 validation ---------------------------------------
    print("\n== 2. CIEDE2000 validation ( Sharma et al. 2005 dataset ) ==")
    max_err = 0.0
    for lab1, lab2, want in SHARMA_PAIRS:
        got = float(ciede2000(np.array(lab1), np.array(lab2))[0])
        rev = float(ciede2000(np.array(lab2), np.array(lab1))[0])
        err = abs(got - want)
        sym = abs(got - rev)
        max_err = max(max_err, err)
        print(f"  dE00 = {got:8.4f}  expected {want:8.4f}  |err| {err:.2e}  |asym| {sym:.2e}")
        assert err < 5e-4, "CIEDE2000 mismatch vs Sharma dataset"
        assert sym < 1e-9, "CIEDE2000 not symmetric"
    print(f"  PASS: max |err| = {max_err:.2e}, symmetry exact on all pairs")

    # ---- classification ---------------------------------------------
    print("\n== 3. objective classification ( from stop data + LUT ) ==")
    print(f"  {'preset':10s} {'class':10s} {'end dE00':>9s} {'t(Lmax)':>8s} "
          f"{'centerExc':>9s} {'maxC*':>7s} {'hueSweep':>8s}  flags")
    classes = {}
    for name in NAMES:
        cls, end_de, tmax, cexc, cmax, sweep, flags = classify(name)
        classes[name] = cls
        print(f"  {name:10s} {cls:10s} {end_de:9.3f} {tmax:8.3f} {cexc:9.2f} "
              f"{cmax:7.2f} {sweep:8.1f}  {','.join(flags) if flags else '-'}")

    # ---- per-preset perceptual metrics -------------------------------
    print("\n== 4. luminance & perceptual uniformity ( original vision ) ==")
    print(f"  viol = steps against the dominant L* direction by > {VIOL_TOL} L*;")
    print(f"  reg = total regression against that direction ( any step size );")
    print(f"  maxreg = largest deviation of L* from its monotone envelope;")
    print(f"  path = sum of adjacent dE00 ( total length of the color path ).")
    print(f"  {'preset':10s} {'L0':>6s} {'L255':>6s} {'Lmin':>6s} {'Lmax':>6s} "
          f"{'viol':>4s} {'reg':>7s} {'maxreg':>7s} {'dE mean':>7s} {'dE max':>7s} "
          f"{'dE min':>7s} {'CV':>5s} {'path':>7s} {'grade':>5s}")
    data = {}
    for name in NAMES:
        lut = qwt_lut(name)
        lab = rgb8_to_lab(lut.astype(float))
        L = lab[:, 0]
        de = adjacent_de(lab)
        cv = float(np.std(de) / np.mean(de))
        cls = classes[name]
        if cls == "diverging":
            # monotonicity per arm ( center = level 128 at pos ~0.502 ):
            # both arms must climb monotonically toward the bright center
            h1, r1, m1, _, _ = luminance_metrics(L[:129], VIOL_TOL)
            h2, r2, m2, _, _ = luminance_metrics(L[127:][::-1], VIOL_TOL)
            viol, reg, maxreg = h1 + h2, r1 + r2, max(m1, m2)
        else:
            viol, reg, maxreg, _, _ = luminance_metrics(L, VIOL_TOL)
        g, _ = grade(cls, reg, cv)
        data[name] = dict(lut=lut, lab=lab, L=L, de=de, cv=cv, viol=viol, reg=reg,
                          maxreg=maxreg, grade=g, cls=cls, path=float(de.sum()))
        print(f"  {name:10s} {L[0]:6.2f} {L[-1]:6.2f} {L.min():6.2f} {L.max():6.2f} "
              f"{viol:4d} {reg:7.2f} {maxreg:7.2f} {de.mean():7.3f} {de.max():7.3f} "
              f"{de.min():7.3f} {cv:5.3f} {de.sum():7.2f} {g:>5s}")

    # ---- CVD simulation ------------------------------------------------
    print("\n== 5. CVD simulation ( Machado et al. 2009, severity 1.0 ) ==")
    print("  min adjacent dE00 of the simulated LUT = local distinguishability;")
    print("  min pairwise dE00 ( gap>=32 levels ) = worst far-apart confusion.")
    print(f"  {'preset':10s} {'orig adjMin':>11s} {'orig farMin':>11s} | "
          f"{'prot adj':>8s} {'prot far':>8s} {'deut adj':>8s} {'deut far':>8s} "
          f"{'trit adj':>8s} {'trit far':>8s}")
    cvd_data = {}
    for name in NAMES:
        row = {}
        lab0 = data[name]["lab"]
        far0, _, _ = min_pairwise_de(lab0)
        row["orig"] = (float(data[name]["de"].min()), far0)
        line = f"  {name:10s} {row['orig'][0]:11.3f} {row['orig'][1]:11.3f} |"
        for kind in ("protan", "deutan", "tritan"):
            sim = simulate_cvd(data[name]["lut"], kind)
            simlab = rgb8_to_lab(sim.astype(float))
            adj = float(adjacent_de(simlab).min())
            far, _, _ = min_pairwise_de(simlab)
            row[kind] = (adj, far)
            cvd_data[(name, kind)] = sim
            line += f" {adj:8.3f} {far:8.3f}"
        cvd_data[name] = row
        print(line)

    # worst far-apart confusions under deutan, with positions
    print("\n  worst deutan far-apart confusions ( gap>=32 ):")
    offenders = []
    for name in NAMES:
        sim = cvd_data[(name, "deutan")]
        simlab = rgb8_to_lab(sim.astype(float))
        far, bi, bj = min_pairwise_de(simlab)
        offenders.append((far, name, bi, bj))
    for far, name, bi, bj in sorted(offenders)[:8]:
        print(f"    {name:10s} dE00 = {far:6.3f}  between t = {bi/(N-1):.3f} and t = {bj/(N-1):.3f}")

    # ---- grayscale printing --------------------------------------------
    print("\n== 6. grayscale print safety ( L* IS the gray profile ) ==")
    print("  monotone: sequential -> L*(t) monotone on the whole range;")
    print("            diverging -> both arms monotone toward the center")
    print("            ( the SIGN information survives only if the two ends")
    print("            differ in L*, reported as end dL* ).")
    print(f"  {'preset':10s} {'min |dL*|':>9s} {'viol(0.5)':>9s} {'reg':>8s} "
          f"{'end dL*':>8s} {'monotone':>8s}")
    for name in NAMES:
        cls = data[name]["cls"]
        L = data[name]["L"]
        d = np.abs(np.diff(L))
        end_dl = abs(float(L[0]) - float(L[-1]))
        if cls == "cyclic":
            mono = "n/a"
            viol, reg = data[name]["viol"], data[name]["reg"]
        else:
            viol, reg = data[name]["viol"], data[name]["reg"]
            mono = "yes" if reg <= 2.0 else "NO"
        print(f"  {name:10s} {d.min():9.4f} {viol:9d} {reg:8.2f} {end_dl:8.2f} {mono:>8s}")

    # ---- matplotlib comparison ------------------------------------------
    print("\n== 7. comparison with matplotlib built-in tables ==")
    print("  QWT stops sampled at pos=i/256 ( colorTable256 grid ) vs matplotlib's")
    print("  256-entry tables at the same positions.  'stop dE' compares each QWT")
    print("  color stop against the matplotlib table at the same position - it")
    print("  shows whether the stop data was actually sampled from matplotlib.")
    MPL_NAMES = {"rdylbu": "RdYlBu", "rdylgn": "RdYlGn", "spectral": "Spectral"}
    print(f"  {'preset':10s} {'mean dE00':>9s} {'max dE00':>9s} "
          f"{'stop mean':>9s} {'stop max':>9s}")
    t = np.arange(N) / float(N)
    mpl_cmp = {}
    for name in NAMES:
        mpl_name = MPL_NAMES.get(name, name)
        mpl = plt.get_cmap(mpl_name)(t)[:, :3]
        mpl8 = np.floor(mpl * 255.0 + 0.5).astype(int)
        qwt8 = qwt_lut(name)
        de = ciede2000(rgb8_to_lab(qwt8.astype(float)),
                       rgb8_to_lab(mpl8.astype(float)))
        # per-stop deviation from the matplotlib reference curve, sampled
        # at the EXACT stop positions ( continuous colormap lookup )
        stop_de = []
        for pos, rgb in PRESETS[name]:
            ref = np.floor(np.array(plt.get_cmap(mpl_name)(pos)[:3]) * 255.0 + 0.5)
            stop_de.append(float(ciede2000(rgb8_to_lab(np.array(rgb, dtype=float)),
                                           rgb8_to_lab(ref))[0]))
        mpl_cmp[name] = (float(de.mean()), float(de.max()),
                         float(np.mean(stop_de)), float(np.max(stop_de)))
        print(f"  {name:10s} {de.mean():9.3f} {de.max():9.3f} "
              f"{np.mean(stop_de):9.3f} {np.max(stop_de):9.3f}")

    print("\n  matplotlib's own tables on the same metrics ( reference ):")
    print(f"  {'map':10s} {'dE mean':>8s} {'dE max':>8s} {'CV':>6s}")
    for name in ("viridis", "plasma", "inferno", "magma", "cividis", "jet", "turbo",
                 "coolwarm"):
        mpl8 = np.floor(plt.get_cmap(name)(t)[:, :3] * 255.0 + 0.5).astype(int)
        de = adjacent_de(rgb8_to_lab(mpl8.astype(float)))
        print(f"  {name:10s} {de.mean():8.3f} {de.max():8.3f} "
              f"{np.std(de)/np.mean(de):6.3f}")

    print("\n  stop provenance probe: nearest matplotlib-curve position for each")
    print("  QWT stop ( mpl curve densely sampled at 4096 positions ).  If the")
    print("  stop data was sampled from matplotlib, 'nearest t' equals the stop")
    print("  position and 'dE nearest' is ~0.")
    tt = np.linspace(0.0, 1.0, 4096)
    for name in ("viridis", "plasma", "inferno", "magma", "cividis", "turbo"):
        cmap = plt.get_cmap(name)
        reflab = rgb8_to_lab(np.floor(cmap(tt)[:, :3] * 255.0 + 0.5))
        print(f"  --- {name}")
        for pos, rgb in PRESETS[name]:
            lab = rgb8_to_lab(np.array(rgb, dtype=float))
            de = ciede2000(np.tile(lab, (len(tt), 1)), reflab)
            j = int(np.argmin(de))
            same = np.floor(np.array(cmap(pos)[:3]) * 255.0 + 0.5)
            de_same = float(ciede2000(lab, rgb8_to_lab(same))[0])
            print(f"    pos {pos:.3f} rgb {tuple(int(c) for c in rgb)} -> nearest mpl t "
                  f"{tt[j]:.4f} ( dE {de[j]:.3f} ); dE at same pos {de_same:.3f}")

    # ---- sRGB-space vs linear-light interpolation -------------------------
    print("\n== 8. QWT sRGB-space interpolation vs linear-light interpolation ==")
    print("  dE00 between the QWT LUT and a LUT of the same stops interpolated")
    print("  in linear light ( gamma-correct ), per level:")
    print(f"  {'preset':10s} {'mean':>7s} {'max':>7s}")
    interp = {}
    for name in NAMES:
        alt = qwt_lut_linear_light(name)
        de = ciede2000(rgb8_to_lab(data[name]["lut"].astype(float)),
                       rgb8_to_lab(alt.astype(float)))
        interp[name] = (float(de.mean()), float(de.max()))
        print(f"  {name:10s} {de.mean():7.3f} {de.max():7.3f}")

    # =================================================================
    # figures
    # =================================================================
    print("\n== figures ==")

    def strip(ax, lut):
        ax.imshow(lut.reshape(1, -1, 3), aspect="auto",
                  extent=[0, 1, 0, 1], interpolation="nearest")
        ax.set_xticks([]); ax.set_yticks([])

    # ---- fig_cmap_overview: all 22 presets ----------------------------
    fig, axes = plt.subplots(len(NAMES), 1, figsize=(8.6, 9.4))
    for ax, name in zip(axes, NAMES):
        strip(ax, data[name]["lut"])
        ax.set_ylabel(name, rotation=0, ha="right", va="center", fontsize=8,
                      labelpad=44)
    fig.suptitle("All 22 QwtColorMapPreset presets ( 256-level colorTable256 LUTs )",
                 fontsize=10)
    fig.tight_layout(rect=[0, 0, 1, 0.985])
    fig.savefig(HERE / "fig_cmap_overview.png", dpi=140)
    plt.close(fig)
    print("  fig_cmap_overview.png")

    # ---- fig_cmap_lightness: L*(t) families ---------------------------
    groups = [
        ("Sequential, perceptual class", ["viridis", "plasma", "inferno", "magma",
                                          "cividis", "gray", "bone", "copper"]),
        ("Sequential, classic / rainbow", ["jet", "hot", "cool", "spring", "summer",
                                           "autumn", "winter", "turbo"]),
        ("Diverging", ["coolwarm", "rdylbu", "rdylgn", "spectral"]),
        ("Cyclic", ["rainbow", "hsv"]),
    ]
    fig, axes = plt.subplots(2, 2, figsize=(11.0, 7.6))
    tt = np.arange(N) / (N - 1.0)
    for ax, (title, members) in zip(axes.ravel(), groups):
        for name in members:
            ax.plot(tt, data[name]["L"], lw=1.3, label=name)
        ax.set_title(title, fontsize=10)
        ax.set_xlabel("normalized value t")
        ax.set_ylabel("CIELAB L*")
        ax.grid(alpha=0.3)
        ax.legend(fontsize=7, ncol=2, loc="best")
    fig.suptitle("Lightness profiles L*(t) of all presets", fontsize=11)
    fig.tight_layout(rect=[0, 0, 1, 0.97])
    fig.savefig(HERE / "fig_cmap_lightness.png", dpi=140)
    plt.close(fig)
    print("  fig_cmap_lightness.png")

    # ---- fig_cmap_deltae: heatmap + bars --------------------------------
    de_mat = np.array([data[n]["de"] for n in NAMES])  # (22, 255)
    fig = plt.figure(figsize=(11.5, 7.4))
    gs = fig.add_gridspec(1, 2, width_ratios=[2.35, 1.0], wspace=0.28)
    ax = fig.add_subplot(gs[0, 0])
    im = ax.imshow(de_mat, aspect="auto", cmap="inferno", vmin=0,
                   vmax=min(20.0, de_mat.max()),
                   extent=[0, 1, len(NAMES) - 0.5, -0.5])
    ax.set_yticks(range(len(NAMES)))
    ax.set_yticklabels(NAMES, fontsize=8)
    ax.set_xlabel("normalized value t")
    ax.set_title("Adjacent-step CIEDE2000 $\\Delta E_{00}(t)$  ( clipped at 20 )",
                 fontsize=10)
    fig.colorbar(im, ax=ax, label="$\\Delta E_{00}$ per 1/256 step")
    ax = fig.add_subplot(gs[0, 1])
    means = [data[n]["de"].mean() for n in NAMES]
    cvs = [data[n]["cv"] for n in NAMES]
    cols = {"A": "#2ca02c", "B": "#ff7f0e", "C": "#d62728"}
    ypos = np.arange(len(NAMES))
    ax.barh(ypos, means, color=[cols[data[n]["grade"]] for n in NAMES], height=0.72)
    for i, (m, c) in enumerate(zip(means, cvs)):
        ax.text(m + 0.08, i, f"CV {c:.2f}", va="center", fontsize=6.5)
    ax.set_yticks(ypos); ax.set_yticklabels(NAMES, fontsize=8)
    ax.invert_yaxis()
    ax.set_xlabel("mean adjacent $\\Delta E_{00}$")
    ax.set_title("Uniformity grades\n( green A / orange B / red C )", fontsize=10)
    ax.set_xlim(0, max(means) * 1.45)
    fig.suptitle("Perceptual uniformity of the presets", fontsize=11)
    fig.tight_layout(rect=[0, 0, 1, 0.965])
    fig.savefig(HERE / "fig_cmap_deltae.png", dpi=140)
    plt.close(fig)
    print("  fig_cmap_deltae.png")

    # ---- fig_cmap_cvd: simulation strips ---------------------------------
    sel = ["viridis", "cividis", "jet", "turbo", "rainbow", "rdylgn", "coolwarm"]
    kinds = ["protan", "deutan", "tritan"]
    fig, axes = plt.subplots(len(sel), 4, figsize=(9.2, 6.6))
    for r, name in enumerate(sel):
        strip(axes[r, 0], data[name]["lut"])
        axes[r, 0].set_ylabel(name, rotation=0, ha="right", va="center",
                              fontsize=8, labelpad=40)
        for c, kind in enumerate(kinds):
            strip(axes[r, c + 1], cvd_data[(name, kind)])
    axes[0, 0].set_title("original", fontsize=9)
    for c, kind in enumerate(kinds):
        axes[0, c + 1].set_title(f"{kind} ( severity 1.0 )", fontsize=9)
    fig.suptitle("Color-vision-deficiency simulation ( Machado et al. 2009 )",
                 fontsize=11)
    fig.tight_layout(rect=[0, 0, 1, 0.95])
    fig.savefig(HERE / "fig_cmap_cvd.png", dpi=140)
    plt.close(fig)
    print("  fig_cmap_cvd.png")

    # ---- fig_cmap_gray: color vs grayscale --------------------------------
    sel = ["viridis", "cividis", "gray", "jet", "turbo", "rainbow", "hot", "coolwarm"]
    fig, axes = plt.subplots(len(sel), 2, figsize=(7.6, 6.8))
    for r, name in enumerate(sel):
        lut = data[name]["lut"]
        strip(axes[r, 0], lut)
        Y = rgb8_to_lab(lut.astype(float))[:, 0]  # L* = perceptual gray level
        g8 = lab_to_rgb8(np.stack([Y, np.zeros_like(Y), np.zeros_like(Y)], -1))
        strip(axes[r, 1], g8)
        axes[r, 0].set_ylabel(name, rotation=0, ha="right", va="center",
                              fontsize=8, labelpad=6)
    axes[0, 0].set_title("color", fontsize=9)
    axes[0, 1].set_title("grayscale ( equal L* )", fontsize=9)
    fig.suptitle("What survives grayscale printing", fontsize=11)
    fig.tight_layout(rect=[0, 0, 1, 0.955])
    fig.savefig(HERE / "fig_cmap_gray.png", dpi=140)
    plt.close(fig)
    print("  fig_cmap_gray.png")

    # ---- fig_cmap_vs_mpl --------------------------------------------------
    fig, axes = plt.subplots(2, 2, figsize=(10.0, 6.4))
    for ax, name in zip(axes.ravel(), ("viridis", "plasma", "jet", "turbo")):
        mpl = plt.get_cmap(name)(t)[:, :3]
        mpl8 = np.floor(mpl * 255.0 + 0.5).astype(int)
        qwt8 = qwt_lut(name)
        de = ciede2000(rgb8_to_lab(qwt8.astype(float)), rgb8_to_lab(mpl8.astype(float)))
        ax.plot(t, de, lw=1.2, color="#1f77b4")
        ax.axhline(1.0, color="gray", ls=":", lw=0.9)
        ax.set_title(f"{name}: QWT preset vs matplotlib  ( mean dE00 {de.mean():.2f}, "
                     f"max {de.max():.2f} )", fontsize=9)
        ax.set_xlabel("normalized value t")
        ax.set_ylabel("$\\Delta E_{00}$")
        ax.grid(alpha=0.3)
        # color strips on top
        axins = ax.inset_axes([0, 1.06, 1, 0.16])
        axins.imshow(np.vstack([qwt8.reshape(1, -1, 3), mpl8.reshape(1, -1, 3)]),
                     aspect="auto", interpolation="nearest")
        axins.set_xticks([]); axins.set_yticks([])
    fig.suptitle("9-stop QWT approximations vs the matplotlib reference tables\n"
                 "( top strip: QWT, bottom strip: matplotlib; dotted line: dE00 = 1 JND )",
                 fontsize=10)
    fig.tight_layout(rect=[0, 0, 1, 0.93])
    fig.savefig(HERE / "fig_cmap_vs_mpl.png", dpi=140)
    plt.close(fig)
    print("  fig_cmap_vs_mpl.png")

    print("\nAll figures written to", HERE)


if __name__ == "__main__":
    main()
