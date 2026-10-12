// Micro-isolation of the ring_raw "gen" anomaly: does interleaving the SIMD
// argmin/argmax mapper path with the ring append loop slow the append down?
// Console only, no GUI.
//
// Build (Windows, MSVC 2019 x64, Qt 5.15.16, QWT release build in <repo>/build-release):
//   call vcvars64.bat
//   cl /nologo /O2 /EHsc /std:c++17 /MD ^
//     /I<repo>\src /I<repo>\src\core /I<qt>\include /I<qt>\include\QtCore ^
//     streaming_micro_simd.cpp /Fe:streaming_micro_simd.exe ^
//     /link /LIBPATH:<repo>\build-release\lib /LIBPATH:<qt>\lib ^
//     qwtcore.lib Qt5Core.lib
//
// Run: streaming_micro_simd.exe   (needs <qt>\bin and <repo>\build-release\bin on PATH)
// Raw stdout of the run cited in realtime-streaming.md: streaming_micro_results.txt
#include <QElapsedTimer>
#include <QVector>
#include <qwt_simd_argminmax.h>
#include <immintrin.h>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static inline quint64 splitmix64(quint64 z)
{
    z += 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

int main()
{
    const long long W = 1000000;
    const long long batch = 10000;
    QVector<double> xs(2 * W), ys(2 * W);
    QElapsedTimer clk;

    auto append = [&](long long g0) {
        for (long long g = g0; g < g0 + batch; ++g) {
            const long long p = g % W;
            const double x = (double(g) + 0.5) / 1e6;
            const quint64 h = splitmix64(quint64(g));
            const double noise = double(h >> 11) / 9007199254740992.0 * 2.0 - 1.0;
            const double y = 0.8 * std::sin(2 * M_PI * 7 * x) + 0.3 * std::sin(2 * M_PI * 23 * x) + 0.05 * noise;
            xs[size_t(p)] = x; ys[size_t(p)] = y;
            xs[size_t(p + W)] = x; ys[size_t(p + W)] = y;
        }
    };

    // emulate the SIMD MinMax reduce over the last 10k samples: 1800 buckets
    auto simdReduce = [&](long long gEnd) {
        const double* y = ys.constData() + (gEnd - batch) % (2 * W);
        const int numBuckets = 1800;
        const double bs = double(batch) / numBuckets;
        volatile double sink = 0;
        for (int b = 0; b < numBuckets; ++b) {
            const int start = int(b * bs);
            const int count = int((b + 1) * bs) - start;
            const QwtArgMinMaxResult r = qwtSimdArgMinMax(y + start, count);
            sink += r.minVal + r.maxVal;
        }
    };

    // same as simdReduce but cleans the AVX upper state after each call
    auto simdReduceZero = [&](long long gEnd) {
        const double* y = ys.constData() + (gEnd - batch) % (2 * W);
        const int numBuckets = 1800;
        const double bs = double(batch) / numBuckets;
        volatile double sink = 0;
        for (int b = 0; b < numBuckets; ++b) {
            const int start = int(b * bs);
            const int count = int((b + 1) * bs) - start;
            const QwtArgMinMaxResult r = qwtSimdArgMinMax(y + start, count);
            _mm256_zeroall();
            sink += r.minVal + r.maxVal;
        }
    };

    // emulate the generic reduce: scalar min/max per bucket via plain loads
    auto genericReduce = [&](long long gEnd) {
        const double* y = ys.constData() + (gEnd - batch) % (2 * W);
        const int numBuckets = 1800;
        const double bs = double(batch) / numBuckets;
        volatile double sink = 0;
        for (int b = 0; b < numBuckets; ++b) {
            const int start = int(b * bs);
            const int end = int((b + 1) * bs);
            double mn = y[start], mx = y[start];
            for (int i = start + 1; i < end; ++i) {
                const double v = y[i];
                if (v < mn) mn = v;
                if (v > mx) mx = v;
            }
            sink += mn + mx;
        }
    };

    auto run = [&](const char* name, int mode) {
        std::vector<double> appends;
        long long g = 0;
        clk.start();
        for (int it = 0; it < 120; ++it) {
            const qint64 a0 = clk.nsecsElapsed();
            append(g);
            const qint64 a1 = clk.nsecsElapsed();
            if (mode == 1) simdReduce(g + batch);
            else if (mode == 2) genericReduce(g + batch);
            else if (mode == 3) simdReduceZero(g + batch);
            g += batch;
            appends.push_back(double(a1 - a0) / 1e6);
        }
        std::sort(appends.begin(), appends.end());
        std::printf("%-34s append median = %.3f ms  (p90 %.3f, min %.3f)\n",
            name, appends[appends.size() / 2], appends[size_t(appends.size() * 0.9)], appends[0]);
    };

    run("append only", 0);
    run("append + SIMD reduce", 1);
    run("append + generic reduce", 2);
    run("append + SIMD reduce + zeroall", 3);
    run("append + SIMD reduce (2nd)", 1);
    return 0;
}
