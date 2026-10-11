"""
Numerical experiments for the curve smoothing article.

The kernels replicate src/plot/qwt_curve_smoothing.cpp exactly:
  * qwtGaussianKernel: sigma = max(1, window/4), normalized
  * qwtSavitzkyGolayKernel: moment matrix of the local least squares fit,
    solved with Gaussian elimination ( pivot search ), evaluated at the
    window center

The experiments quantify the properties derived in the article:

  * noise gain  g = sum(c_j^2):  the variance of white noise after the
    filter is g * sigma^2 ( with the identity g = c_0 for Savitzky-Golay ).
  * frequency response |H(w)| and its -3 dB cutoff.
  * peak height retention of a Gaussian bump as a function of its width.
  * the feature preservation step ( baseline, MAD noise scale, threshold ).
"""

import numpy as np


# ------------------------------------------------------------ kernels (C++)

def gaussian_kernel(window):
    """qwtGaussianKernel - odd window, sigma = max(1, window / 4)."""
    m = window // 2
    sigma = max(1.0, 0.25 * window)
    j = np.arange(-m, m + 1)
    k = np.exp(-0.5 * j * j / (sigma * sigma))
    return k / k.sum()


def solve_linear(matrix, rhs, n):
    """qwtSolveLinearSystem - Gaussian elimination with partial pivoting."""
    A = matrix.copy()
    b = rhs.copy()
    for k in range(n):
        pivot = k + int(np.argmax(np.abs(A[k:, k])))
        if abs(A[pivot, k]) < 1.0e-12:
            return None
        if pivot != k:
            A[[k, pivot]] = A[[pivot, k]]
            b[k], b[pivot] = b[pivot], b[k]
        for i in range(k + 1, n):
            f = A[i, k] / A[k, k]
            A[i, k:] -= f * A[k, k:]
            b[i] -= f * b[k]
    x = np.zeros(n)
    for i in range(n - 1, -1, -1):
        x[i] = (b[i] - A[i, i + 1:] @ x[i + 1:]) / A[i, i]
    return x


def savitzky_golay_kernel(window, order):
    """qwtSavitzkyGolayKernel - centre value of the order-th polynomial
    fitted by least squares to the window."""
    m = window // 2
    np_ = order + 1

    moments = np.zeros(2 * order + 1)
    for j in range(-m, m + 1):
        v = 1.0
        for i in range(2 * order + 1):
            moments[i] += v
            v *= j

    matrix = np.zeros((np_, np_))
    for i in range(np_):
        for k in range(np_):
            matrix[i, k] = moments[i + k]

    rhs = np.zeros(np_)
    rhs[0] = 1.0
    sol = solve_linear(matrix, rhs, np_)
    if sol is None:
        return None

    kernel = np.zeros(window)
    for j in range(-m, m + 1):
        v = 1.0
        c = 0.0
        for i in range(np_):
            c += sol[i] * v
            v *= j
        kernel[j + m] = c
    return kernel


def box_kernel(window):
    return np.full(window, 1.0 / window)


def noise_gain(k):
    return float(np.sum(k * k))


def frequency_response(k, nf=4096):
    """|H(f)| on [0, 0.5] cycles / sample for the symmetric FIR kernel k."""
    f = np.linspace(0.0, 0.5, nf)
    j = np.arange(len(k)) - len(k) // 2
    H = np.abs(np.exp(-2j * np.pi * np.outer(f, j)) @ k)
    return f, H


def cutoff3db(k):
    """The frequency ( in cycles / sample ) where |H| drops by 3 dB."""
    f, H = frequency_response(k)
    i = np.argmax(H <= 10 ** (-3.0 / 20.0) * H[0])
    return float(f[i])


def peak_retention(b, kernel):
    """Filter a Gaussian bump of std b ( in samples ) with the kernel and
    return the fraction of the original height that survives.  The borders
    are mirrored like qwtReflectIndex."""
    x = np.arange(-400, 401)
    y = np.exp(-0.5 * (x / b) ** 2)
    n = len(y)
    m = len(kernel) // 2
    out = np.zeros(n)
    for i in range(n):
        acc = 0.0
        for j, c in enumerate(kernel):
            idx = i + j - m
            while idx < 0 or idx >= n:
                if idx < 0:
                    idx = 2 * 0 - idx
                else:
                    idx = 2 * (n - 1) - idx
            acc += c * y[idx]
        out[i] = acc
    return float(out.max())


# ------------------------------------------------------------------ checks

def verify_identities():
    """Identity 1: for Savitzky-Golay, sum(c_j^2) == c_0.  This is
    e_1^T (A^T A)^{-1} A^T A (A^T A)^{-1} e_1 = e_1^T (A^T A)^{-1} e_1.
    Identity 2: orders 2 and 3 share the same kernel ( odd moments
    of a symmetric window vanish )."""
    print("window order  c_0      sum c_j^2  equal p=2/p=3")
    prev = None
    for w in (5, 7, 9, 15, 21, 31):
        k2 = savitzky_golay_kernel(w, 2)
        k3 = savitzky_golay_kernel(w, 3)
        print(f"{w:>6}   {1:>2}   {k2[len(k2)//2]:.6f} {noise_gain(k2):.6f}  {np.allclose(k2, k3)}")


def kernel_table():
    print("\nkernel coefficients")
    for w, p in ((5, 2), (7, 2), (9, 2), (9, 3), (15, 2), (15, 3)):
        k = savitzky_golay_kernel(w, p)
        g = savitzky_golay_kernel(w, p)
        print(f"SG(w={w}, p={p}): "
              + " ".join(f"{v:+.4f}" for v in k))
    for w in (15, 21):
        kg = gaussian_kernel(w)
        print(f"Gauss(w={w}): " + " ".join(f"{v:+.4f}" for v in kg))


if __name__ == "__main__":
    np.set_printoptions(precision=4, suppress=True)
    verify_identities()

    print("\nnoise gain sqrt(g) and -3dB cutoff (cycles / sample)")
    print(f"{'filter':<16} {'sqrt(g)':>8} {'-3dB':>8}")
    for w in (9, 15, 21, 31):
        for name, k in (("Gaussian", gaussian_kernel(w)),
                        ("SG p=2", savitzky_golay_kernel(w, 2)),
                        ("SG p=3", savitzky_golay_kernel(w, 3)),
                        ("box", box_kernel(w))):
            if k is None:
                continue
            print(f"{name + f' w={w}':<16} {np.sqrt(noise_gain(k)):>8.4f} "
                  f"{cutoff3db(k):>8.4f}")