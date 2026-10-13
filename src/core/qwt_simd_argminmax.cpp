#include "qwt_simd_argminmax.h"

#include <cmath>
#include <cstddef>

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
#include <x86intrin.h>
#endif

namespace
{

enum SimdLevel
{
    Scalar,
    SSE42,
    AVX2
};

SimdLevel detectSimdLevel()
{
#if defined(_MSC_VER)
    int cpuinfo[ 4 ];
    __cpuid(cpuinfo, 0);
    if (cpuinfo[ 0 ] >= 7) {
        __cpuidex(cpuinfo, 7, 0);
        if (cpuinfo[ 1 ] & (1 << 5))
            return AVX2;
    }
    __cpuid(cpuinfo, 1);
    if (cpuinfo[ 2 ] & (1 << 20))
        return SSE42;
    return Scalar;
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__x86_64__) || defined(__i386__)
    __builtin_cpu_init();
    if (__builtin_cpu_supports("avx2"))
        return AVX2;
    if (__builtin_cpu_supports("sse4.2"))
        return SSE42;
#endif
    return Scalar;
#else
    return Scalar;
#endif
}

static const SimdLevel kSimdLevel = detectSimdLevel();

}  // namespace

// ===== Scalar fallback =====
static QwtArgMinMaxResult argMinMaxScalar(const double* data, int count)
{
    int minIdx    = 0;
    int maxIdx    = 0;
    double minVal = data[ 0 ];
    double maxVal = data[ 0 ];

    for (int i = 1; i < count; ++i) {
        const double v = data[ i ];
        if (v < minVal) {
            minVal = v;
            minIdx = i;
        }
        if (v > maxVal) {
            maxVal = v;
            maxIdx = i;
        }
    }

    return { minIdx, maxIdx, minVal, maxVal };
}

// ===== SSE4.2 implementation =====
#if defined(__SSE4_2__) || (defined(_MSC_VER) && _MSC_VER >= 1700)

static QwtArgMinMaxResult argMinMaxSSE42(const double* data, int count)
{
    __m128d minVec    = _mm_set1_pd(DBL_MAX);
    __m128d maxVec    = _mm_set1_pd(-DBL_MAX);
    __m128d minIdxVec = _mm_set_pd(1.0, 0.0);
    __m128d maxIdxVec = _mm_set_pd(1.0, 0.0);

    int i = 0;
    for (; i + 2 <= count; i += 2) {
        const __m128d vals   = _mm_loadu_pd(data + i);
        const __m128d curIdx = _mm_set_pd(static_cast< double >(i + 1), static_cast< double >(i));

        const __m128d cmpMin = _mm_cmp_pd(vals, minVec, _CMP_LT_OQ);
        minIdxVec            = _mm_blendv_pd(minIdxVec, curIdx, cmpMin);
        minVec               = _mm_min_pd(minVec, vals);

        const __m128d cmpMax = _mm_cmp_pd(vals, maxVec, _CMP_GT_OQ);
        maxIdxVec            = _mm_blendv_pd(maxIdxVec, curIdx, cmpMax);
        maxVec               = _mm_max_pd(maxVec, vals);
    }

    double minArr[ 2 ], maxArr[ 2 ], minIdxArr[ 2 ], maxIdxArr[ 2 ];
    _mm_storeu_pd(minArr, minVec);
    _mm_storeu_pd(maxArr, maxVec);
    _mm_storeu_pd(minIdxArr, minIdxVec);
    _mm_storeu_pd(maxIdxArr, maxIdxVec);

    int fMinIdx    = static_cast< int >(minIdxArr[ 0 ]);
    double fMinVal = minArr[ 0 ];
    int fMaxIdx    = static_cast< int >(maxIdxArr[ 0 ]);
    double fMaxVal = maxArr[ 0 ];

    if (minArr[ 1 ] < fMinVal) {
        fMinVal = minArr[ 1 ];
        fMinIdx = static_cast< int >(minIdxArr[ 1 ]);
    }
    if (maxArr[ 1 ] > fMaxVal) {
        fMaxVal = maxArr[ 1 ];
        fMaxIdx = static_cast< int >(maxIdxArr[ 1 ]);
    }

    for (; i < count; ++i) {
        if (data[ i ] < fMinVal) {
            fMinVal = data[ i ];
            fMinIdx = i;
        }
        if (data[ i ] > fMaxVal) {
            fMaxVal = data[ i ];
            fMaxIdx = i;
        }
    }

    return { fMinIdx, fMaxIdx, fMinVal, fMaxVal };
}

#else
#define argMinMaxSSE42 argMinMaxScalar
#endif

// ===== AVX2 implementation =====
// The kernel lives in qwt_simd_argminmax_avx2.cpp, compiled with AVX2 code
// generation (see the file and the CMake setup for the measurements behind
// that split). It is only called after the runtime check below.
#if defined(__AVX2__) || (defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86)))

QwtArgMinMaxResult qwtArgMinMaxAVX2(const double* data, int count);

#define argMinMaxAVX2 qwtArgMinMaxAVX2

#else
#define argMinMaxAVX2 argMinMaxSSE42
#endif

// ===== Public API: function-pointer dispatch =====
QwtArgMinMaxResult qwtSimdArgMinMax(const double* data, int count)
{
    using Fn            = QwtArgMinMaxResult (*)(const double*, int);
    static const Fn kFn = []() -> Fn {
        switch (kSimdLevel) {
        case AVX2:
            return argMinMaxAVX2;
        case SSE42:
            return argMinMaxSSE42;
        default:
            return argMinMaxScalar;
        }
    }();

    QwtArgMinMaxResult result = kFn(data, count);

    if (std::isnan(result.minVal) || std::isnan(result.maxVal))
        return { 0, 0, DBL_MAX, -DBL_MAX };

    return result;
}
