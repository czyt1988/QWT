/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *
 * Modified by ChenZongYan in 2024 <czy.t@163.com>
 *****************************************************************************/

#include "qwt_simd_argminmax.h"

#if defined(__AVX2__) || (defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86)))

#include <cfloat>

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#if defined(_MSC_VER)
#define QWT_SIMD_AVX2_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define QWT_SIMD_AVX2_NOINLINE __attribute__((noinline))
#else
#define QWT_SIMD_AVX2_NOINLINE
#endif

/**
 * @brief AVX2 implementation of the argmin/argmax scan
 * @details This translation unit is compiled with AVX2 code generation
 *          ( /arch:AVX2 for MSVC, -mavx2 for GCC/Clang, see the CMake setup ),
 *          which is safe because the dispatcher in qwt_simd_argminmax.cpp
 *          calls the function only after its runtime CPUID check detected
 *          AVX2 support.
 *
 *          The separate translation unit is a performance fix, not a
 *          stylistic choice: with the default x86-64 code generation the
 *          function reads its 256-bit accumulators back through the stack
 *          ( 32 byte spill stores, narrow scalar/legacy SSE reloads ) and
 *          mixes legacy encoded SSE into the epilogue, and the compiler
 *          schedules both the spill/reload pattern and the VZEROUPPER it was
 *          explicitly asked for in ways that keep the AVX-SSE transition
 *          penalty alive. Compiled as AVX2 the whole epilogue is VEX encoded
 *          and the compiler inserts a VZEROUPPER at the return, which the
 *          measurements in docs/zh/tech-report/simd-argminmax.md quantify:
 *          ~330 ns of fixed per call overhead and a ~10x slowdown of scalar
 *          code following the call collapse to a few ns.
 *
 *          noinline: with link time code generation the function must not be
 *          inlined into the (default architecture) dispatcher, where its AVX2
 *          instructions could then be reached without the CPUID check.
 */
QWT_SIMD_AVX2_NOINLINE QwtArgMinMaxResult qwtArgMinMaxAVX2(const double* data, int count)
{
    __m256d minVec    = _mm256_set1_pd(DBL_MAX);
    __m256d maxVec    = _mm256_set1_pd(-DBL_MAX);
    __m256d minIdxVec = _mm256_set_pd(3.0, 2.0, 1.0, 0.0);
    __m256d maxIdxVec = _mm256_set_pd(3.0, 2.0, 1.0, 0.0);

    int i = 0;
    for (; i + 4 <= count; i += 4) {
        const __m256d vals   = _mm256_loadu_pd(data + i);
        const __m256d curIdx = _mm256_set_pd(static_cast< double >(i + 3),
                                             static_cast< double >(i + 2),
                                             static_cast< double >(i + 1),
                                             static_cast< double >(i));

        const __m256d cmpMin = _mm256_cmp_pd(vals, minVec, _CMP_LT_OQ);
        minIdxVec            = _mm256_blendv_pd(minIdxVec, curIdx, cmpMin);
        minVec               = _mm256_min_pd(minVec, vals);

        const __m256d cmpMax = _mm256_cmp_pd(vals, maxVec, _CMP_GT_OQ);
        maxIdxVec            = _mm256_blendv_pd(maxIdxVec, curIdx, cmpMax);
        maxVec               = _mm256_max_pd(maxVec, vals);
    }

    double minArr[ 4 ], maxArr[ 4 ], minIdxArr[ 4 ], maxIdxArr[ 4 ];
    _mm256_storeu_pd(minArr, minVec);
    _mm256_storeu_pd(maxArr, maxVec);
    _mm256_storeu_pd(minIdxArr, minIdxVec);
    _mm256_storeu_pd(maxIdxArr, maxIdxVec);

    int fMinIdx    = static_cast< int >(minIdxArr[ 0 ]);
    double fMinVal = minArr[ 0 ];
    int fMaxIdx    = static_cast< int >(maxIdxArr[ 0 ]);
    double fMaxVal = maxArr[ 0 ];

    for (int j = 1; j < 4; ++j) {
        if (minArr[ j ] < fMinVal) {
            fMinVal = minArr[ j ];
            fMinIdx = static_cast< int >(minIdxArr[ j ]);
        }
        if (maxArr[ j ] > fMaxVal) {
            fMaxVal = maxArr[ j ];
            fMaxIdx = static_cast< int >(maxIdxArr[ j ]);
        }
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

#endif  // AVX2 capable x86
