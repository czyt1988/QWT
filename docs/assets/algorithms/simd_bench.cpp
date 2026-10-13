// simd_bench.cpp - microbenchmark for qwtSimdArgMinMax (Qwt 7.x, src/core/qwt_simd_argminmax.cpp)
//
// This single source file builds into TWO translation units (see build commands
// below). The main TU is compiled with default MSVC flags (x64 SSE2 baseline,
// same as the shipped qwtcore library). The second TU is compiled with
// /arch:AVX2 and exposes wrappers around the AVX2/SSE4.2 implementations, so
// the benchmark can compare "callee returns with dirty upper-YMM state" vs
// "callee compiled as AVX2 (compiler inserts vzeroupper at return)".
//
// Build (MSVC x64, from a vcvars64 shell; needs Qt headers + Qt6Core):
//
//   cl /nologo /O2 /EHsc /std:c++17 /Zc:__cplusplus /permissive- /MD /c ^
//      /I<repo>/src /I<repo>/src/core /I<qt>/include /I<qt>/include/QtCore ^
//      simd_bench.cpp /Fo:simd_bench.obj
//   cl /nologo /O2 /EHsc /std:c++17 /Zc:__cplusplus /permissive- /MD /arch:AVX2 /DQWT_SIMD_BENCH_AVX2_TU /c ^
//      /I<repo>/src /I<repo>/src/core /I<qt>/include /I<qt>/include/QtCore ^
//      simd_bench.cpp /Fo:simd_bench_avx2tu.obj
//   rem the AVX2 kernel of the library lives in its own TU since v7.5.1
//   cl /nologo /O2 /EHsc /std:c++17 /Zc:__cplusplus /permissive- /MD /arch:AVX2 /c ^
//      /I<repo>/src /I<repo>/src/core /I<qt>/include /I<qt>/include/QtCore ^
//      <repo>/src/core/qwt_simd_argminmax_avx2.cpp /Fo:libavx2.obj
//   link /nologo simd_bench.obj simd_bench_avx2tu.obj libavx2.obj ^
//      /LIBPATH:<qt>/lib Qt6Core.lib /OUT:simd_bench.exe
//
// The bench includes qwt_simd_argminmax.cpp (the dispatcher TU) and therefore
// needs qwtArgMinMaxAVX2 from the AVX2 translation unit listed above; the
// main TU deliberately stays at the x86-64 baseline, which is what the
// "shipped form" probes (call_avx2_direct, call_public) measure.
//
// Run (stdout is the results CSV; redirect it to simd_bench_results.csv):
//
//   simd_bench.exe > simd_bench_results.csv
//
// Timing protocol: the benchmark thread is pinned to logical processor 0
// (a P-core). Every measurement point calibrates an internal repetition
// count so one timed run takes ~50 ms, then does 3 warmup runs + 7 measured
// runs and reports the MINIMUM (min-of-7) together with median and max for
// pollution monitoring. If median/min > 1.30 the whole batch is re-measured
// (up to 3 batches, best minimum kept, re-measure count recorded in `note`).
//
// CSV schema (all data rows have exactly these 10 fields):
//   mode,path,p1,p2,reps,min_ns,med_ns,max_ns,gbs,note
// - mode=env       environment rows, p1=value (free text without commas)
// - mode=meta      correctness rows
// - mode=scan      p1=array size n; per-call ns and streaming GB/s
// - mode=bucket    p1=total N (1M), p2=chunk size; per-call ns over chunks
// - mode=bandwidth p1=array size n; memory-bound reference kernels
// - mode=probe     p1=n or chain length; transition-penalty / hybrid-core probes
// - mode=nan       p1=n, p2=NaN position pattern; result string in note

#ifdef QWT_SIMD_BENCH_AVX2_TU

// ===================== AVX2-compiled wrapper TU =====================
// Rename the public entry so both TUs can include the implementation
// without duplicate symbols.
#define qwtSimdArgMinMax qb_avx2tu_unused_qwtSimdArgMinMax
#include "qwt_simd_argminmax.cpp"
#undef qwtSimdArgMinMax

extern "C" QwtArgMinMaxResult qb_wrapped_avx2( const double* data, int count )
{
    return argMinMaxAVX2( data, count );
}

extern "C" QwtArgMinMaxResult qb_wrapped_sse42( const double* data, int count )
{
    return argMinMaxSSE42( data, count );
}

// The main TU is built without /arch:AVX, where MSVC does not declare the
// vzeroupper intrinsic (_mm256_zeroupper in this toolset); export it from
// this AVX2-compiled TU instead. The intrinsic has no data side effect and
// /O2 would elide it, so optimization is disabled for this one function.
#pragma optimize( "", off )
extern "C" void qb_vzeroupper()
{
    _mm256_zeroupper();
}
#pragma optimize( "", on )

// Call a caller-supplied implementation pointer from this /arch:AVX2 context
// (tests whether the caller's ISA context matters for the callee's cost).
extern "C" QwtArgMinMaxResult qb_call_via_ptr( QwtArgMinMaxResult ( *fn )( const double*, int ),
                                               const double* data, int count )
{
    return fn( data, count );
}

// Verbatim copy of the library argMinMaxAVX2 body with 6 rdtsc checkpoints
// (5 segments: setup / loop / spill-store / reduce / tail), compiled with
// /arch:AVX2 in this TU. The rdtsc reads perturb scheduling somewhat; they
// are only used to localize cost, not to report absolute times.
extern "C" __declspec( noinline ) QwtArgMinMaxResult
qb_instr_avx2( const double* data, int count, unsigned long long* seg )
{
    seg[0] = __rdtsc();
    __m256d minVec    = _mm256_set1_pd( DBL_MAX );
    __m256d maxVec    = _mm256_set1_pd( -DBL_MAX );
    __m256d minIdxVec = _mm256_set_pd( 3.0, 2.0, 1.0, 0.0 );
    __m256d maxIdxVec = _mm256_set_pd( 3.0, 2.0, 1.0, 0.0 );
    seg[1] = __rdtsc();
    int i = 0;
    for ( ; i + 4 <= count; i += 4 ) {
        const __m256d vals   = _mm256_loadu_pd( data + i );
        const __m256d curIdx = _mm256_set_pd( static_cast< double >( i + 3 ),
                                              static_cast< double >( i + 2 ),
                                              static_cast< double >( i + 1 ),
                                              static_cast< double >( i ) );
        const __m256d cmpMin = _mm256_cmp_pd( vals, minVec, _CMP_LT_OQ );
        minIdxVec            = _mm256_blendv_pd( minIdxVec, curIdx, cmpMin );
        minVec               = _mm256_min_pd( minVec, vals );
        const __m256d cmpMax = _mm256_cmp_pd( vals, maxVec, _CMP_GT_OQ );
        maxIdxVec            = _mm256_blendv_pd( maxIdxVec, curIdx, cmpMax );
        maxVec               = _mm256_max_pd( maxVec, vals );
    }
    seg[2] = __rdtsc();
    double minArr[4], maxArr[4], minIdxArr[4], maxIdxArr[4];
    _mm256_storeu_pd( minArr, minVec );
    _mm256_storeu_pd( maxArr, maxVec );
    _mm256_storeu_pd( minIdxArr, minIdxVec );
    _mm256_storeu_pd( maxIdxArr, maxIdxVec );
    seg[3] = __rdtsc();
    int fMinIdx    = static_cast< int >( minIdxArr[0] );
    double fMinVal = minArr[0];
    int fMaxIdx    = static_cast< int >( maxIdxArr[0] );
    double fMaxVal = maxArr[0];
    for ( int j = 1; j < 4; ++j ) {
        if ( minArr[j] < fMinVal ) { fMinVal = minArr[j]; fMinIdx = static_cast< int >( minIdxArr[j] ); }
        if ( maxArr[j] > fMaxVal ) { fMaxVal = maxArr[j]; fMaxIdx = static_cast< int >( maxIdxArr[j] ); }
    }
    seg[4] = __rdtsc();
    for ( ; i < count; ++i ) {
        if ( data[i] < fMinVal ) { fMinVal = data[i]; fMinIdx = i; }
        if ( data[i] > fMaxVal ) { fMaxVal = data[i]; fMaxIdx = i; }
    }
    seg[5] = __rdtsc();
    return { fMinIdx, fMaxIdx, fMinVal, fMaxVal };
}

#else  // ===================== main benchmark TU =====================

#include "qwt_simd_argminmax.cpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

extern "C" QwtArgMinMaxResult qb_wrapped_avx2( const double*, int );
extern "C" QwtArgMinMaxResult qb_wrapped_sse42( const double*, int );
extern "C" void qb_vzeroupper();

using Fn = QwtArgMinMaxResult ( * )( const double*, int );

// ---------------------------------------------------------------- helpers

static double elapsedNs( const std::chrono::steady_clock::time_point& t0 )
{
    return std::chrono::duration< double, std::nano >(
        std::chrono::steady_clock::now() - t0 ).count();
}

static long long g_sink = 0;  // prevents dead-code elimination of results

static std::vector< double > makeData( size_t n, uint64_t seed )
{
    std::mt19937_64 rng( seed );
    std::uniform_real_distribution< double > u( -1e6, 1e6 );
    std::vector< double > v( n );
    for ( auto& x : v ) x = u( rng );
    if ( n >= 4 ) {
        // hide the extremes near the middle (no index-proximity artifacts)
        v[n / 2]     = -1e9;
        v[n / 2 + 1] = 1e9;
    }
    return v;
}

// ------------------------------------------------------- timing machinery

struct BenchResult
{
    long long reps   = 0;     // work units per timed run
    double minNs     = 0.0;   // per work unit, minimum over 7 runs
    double medNs     = 0.0;   // per work unit, median over 7 runs
    double maxNs     = 0.0;   // per work unit, maximum over 7 runs
    int remeasured   = 0;     // extra batches taken because med/min > 1.30
};

// unit(rep) executes `rep` work units (e.g. calls); returns nothing.
template< class UNIT >
static BenchResult measure( UNIT unit, double targetMs = 50.0 )
{
    // --- calibrate: find a rep count making one run ~targetMs
    long long reps = 1;
    for ( ;; ) {
        const auto t0 = std::chrono::steady_clock::now();
        unit( reps );
        const double ns = elapsedNs( t0 );
        if ( ns >= 1e6 || reps >= 200000000LL ) {  // >= 1 ms measured
            reps = std::max< long long >( 1,
                static_cast< long long >( reps * ( targetMs * 1e6 ) / std::max( ns, 1.0 ) ) );
            break;
        }
        reps *= 4;
    }

    BenchResult best;
    best.minNs = 1e30;
    for ( int batch = 0; batch < 3; ++batch ) {
        for ( int w = 0; w < 3; ++w ) unit( reps );  // warmup

        double t[7];
        for ( int k = 0; k < 7; ++k ) {
            const auto t0 = std::chrono::steady_clock::now();
            unit( reps );
            t[k] = elapsedNs( t0 ) / static_cast< double >( reps );
        }
        std::vector< double > s( t, t + 7 );
        std::sort( s.begin(), s.end() );
        const double mn = s[0], md = s[3], mx = s[6];
        if ( mn < best.minNs ) { best.minNs = mn; best.medNs = md; best.maxNs = mx; best.reps = reps; }
        best.remeasured = batch;
        if ( md <= 1.30 * mn ) break;  // clean batch
    }
    return best;
}

static void row( const char* mode, const char* path, const std::string& p1, const std::string& p2,
                 long long reps, double minNs, double medNs, double maxNs, double gbs,
                 const std::string& note )
{
    char r[64], a[64], b[64], c[64], g[64];
    if ( reps > 0 ) std::snprintf( r, sizeof r, "%lld", reps ); else r[0] = 0;
    if ( minNs >= 0 && minNs < 1e29 ) std::snprintf( a, sizeof a, "%.4f", minNs ); else a[0] = 0;
    if ( medNs >= 0 ) std::snprintf( b, sizeof b, "%.4f", medNs ); else b[0] = 0;
    if ( maxNs >= 0 ) std::snprintf( c, sizeof c, "%.4f", maxNs ); else c[0] = 0;
    if ( gbs >= 0 ) std::snprintf( g, sizeof g, "%.3f", gbs ); else g[0] = 0;
    std::printf( "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
                 mode, path, p1.c_str(), p2.c_str(), r, a, b, c, g, note.c_str() );
}

static double gbsOf( long long n, double perCallNs )
{
    return ( perCallNs > 0 ) ? static_cast< double >( n ) * 8.0 / perCallNs : -1.0;
}

// ------------------------------------------------------------ env probing

static std::string cpuBrand()
{
    int info[4] = { 0 };
    char buf[49] = { 0 };
    __cpuid( info, 0x80000000 );
    if ( static_cast< unsigned >( info[0] ) >= 0x80000004u ) {
        for ( unsigned leaf = 0x80000002u; leaf <= 0x80000004u; ++leaf ) {
            __cpuid( info, static_cast< int >( leaf ) );
            std::memcpy( buf + ( leaf - 0x80000002u ) * 16, info, 16 );
        }
    }
    std::string s( buf );
    // strip spaces (CSV friendliness)
    std::string out;
    for ( char ch : s ) if ( ch != ' ' ) out += ch;
    return out;
}

static void cacheInfo( std::vector< std::string >& lines )
{
    DWORD len = 0;
    GetLogicalProcessorInformation( nullptr, &len );
    std::vector< BYTE > buf( len );
    if ( !GetLogicalProcessorInformation(
             reinterpret_cast< PSYSTEM_LOGICAL_PROCESSOR_INFORMATION >( buf.data() ), &len ) )
        return;
    const size_t n = len / sizeof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION );
    const auto* p  = reinterpret_cast< PSYSTEM_LOGICAL_PROCESSOR_INFORMATION >( buf.data() );
    for ( size_t i = 0; i < n; ++i ) {
        if ( p[i].Relationship != RelationCache ) continue;
        const CACHE_DESCRIPTOR& c = p[i].Cache;
        char b[160];
        std::snprintf( b, sizeof b, "L%u=%uKB assoc=%u line=%uB mask=0x%llX",
                       c.Level, c.Size / 1024, c.Associativity, c.LineSize,
                       static_cast< unsigned long long >( p[i].ProcessorMask ) );
        lines.emplace_back( b );
    }
}

// Summarize the cache hierarchy: deduplicate (level,size) pairs, count how
// many distinct caches of each kind exist and how many threads share one.
static std::string cacheSummary()
{
    DWORD len = 0;
    GetLogicalProcessorInformation( nullptr, &len );
    std::vector< BYTE > buf( len );
    if ( !GetLogicalProcessorInformation(
             reinterpret_cast< PSYSTEM_LOGICAL_PROCESSOR_INFORMATION >( buf.data() ), &len ) )
        return "unavailable";
    const size_t n = len / sizeof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION );
    const auto* p  = reinterpret_cast< PSYSTEM_LOGICAL_PROCESSOR_INFORMATION >( buf.data() );
    struct Ent { unsigned level, kb, assoc, threads, count; };
    std::vector< Ent > ents;
    for ( size_t i = 0; i < n; ++i ) {
        if ( p[i].Relationship != RelationCache ) continue;
        const CACHE_DESCRIPTOR& c = p[i].Cache;
        unsigned th = 0;
        for ( ULONG_PTR m = p[i].ProcessorMask; m; m &= m - 1 ) ++th;
        bool found = false;
        for ( auto& e : ents ) {
            if ( e.level == c.Level && e.kb == c.Size / 1024 && e.threads == th ) {
                ++e.count; found = true; break;
            }
        }
        if ( !found ) ents.push_back( { c.Level, c.Size / 1024, c.Associativity, th, 1 } );
    }
    std::string out;
    for ( const auto& e : ents ) {
        char b[128];
        std::snprintf( b, sizeof b, "L%u %uKB x%u (%u-way; %u thr/cache)%s",
                       e.level, e.kb, e.count, e.assoc, e.threads, &e == &ents.back() ? "" : "; " );
        out += b;
    }
    return out;
}

// Effective core clock estimate: xorshift64 chain (shl/shr/xor, each 1-cycle
// latency on all x86 cores -> 6 cycles per iteration, no strength reduction
// possible). GHz = 6 * N / elapsed_ns.
static double clockProbeGhz()
{
    const long long N = 100000000LL;
    unsigned long long x = 0x2545F4914F6CDD1DULL;
    const auto t0 = std::chrono::steady_clock::now();
    for ( long long i = 0; i < N; ++i ) {
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
    }
    const double ns = elapsedNs( t0 );
    if ( x == 12345 ) std::printf( "#unreachable\n" );
    return 6.0 * static_cast< double >( N ) / ns;
}

// Invariant TSC ticks per nanosecond (TSC runs at the nominal base clock).
static double tscRatePerNs()
{
    const unsigned long long t0 = __rdtsc();
    const auto c0 = std::chrono::steady_clock::now();
    volatile unsigned long long x = 0;
    while ( elapsedNs( c0 ) < 2e7 ) x += 1;  // ~20 ms
    const unsigned long long t1 = __rdtsc();
    const auto c1 = std::chrono::steady_clock::now();
    const double ns = std::chrono::duration< double, std::nano >( c1 - c0 ).count();
    return static_cast< double >( t1 - t0 ) / ns;
}

// ------------------------------------------------------------ kernel probes
// Verbatim copies of the library AVX2 kernel compiled in THIS TU (default
// flags, i.e. exactly how qwtcore.dll ships), instrumented with 6 rdtsc
// checkpoints. Purpose: localize the ~325 ns/call fixed overhead seen on
// P-cores for the default-compiled argMinMaxAVX2.

extern "C" QwtArgMinMaxResult qb_instr_avx2( const double*, int, unsigned long long* );
extern "C" QwtArgMinMaxResult qb_call_via_ptr( QwtArgMinMaxResult ( * )( const double*, int ),
                                               const double*, int );

extern "C" __declspec( noinline ) QwtArgMinMaxResult
qb_instr_default( const double* data, int count, unsigned long long* seg )
{
    seg[0] = __rdtsc();
    __m256d minVec    = _mm256_set1_pd( DBL_MAX );
    __m256d maxVec    = _mm256_set1_pd( -DBL_MAX );
    __m256d minIdxVec = _mm256_set_pd( 3.0, 2.0, 1.0, 0.0 );
    __m256d maxIdxVec = _mm256_set_pd( 3.0, 2.0, 1.0, 0.0 );
    seg[1] = __rdtsc();
    int i = 0;
    for ( ; i + 4 <= count; i += 4 ) {
        const __m256d vals   = _mm256_loadu_pd( data + i );
        const __m256d curIdx = _mm256_set_pd( static_cast< double >( i + 3 ),
                                              static_cast< double >( i + 2 ),
                                              static_cast< double >( i + 1 ),
                                              static_cast< double >( i ) );
        const __m256d cmpMin = _mm256_cmp_pd( vals, minVec, _CMP_LT_OQ );
        minIdxVec            = _mm256_blendv_pd( minIdxVec, curIdx, cmpMin );
        minVec               = _mm256_min_pd( minVec, vals );
        const __m256d cmpMax = _mm256_cmp_pd( vals, maxVec, _CMP_GT_OQ );
        maxIdxVec            = _mm256_blendv_pd( maxIdxVec, curIdx, cmpMax );
        maxVec               = _mm256_max_pd( maxVec, vals );
    }
    seg[2] = __rdtsc();
    double minArr[4], maxArr[4], minIdxArr[4], maxIdxArr[4];
    _mm256_storeu_pd( minArr, minVec );
    _mm256_storeu_pd( maxArr, maxVec );
    _mm256_storeu_pd( minIdxArr, minIdxVec );
    _mm256_storeu_pd( maxIdxArr, maxIdxVec );
    seg[3] = __rdtsc();
    int fMinIdx    = static_cast< int >( minIdxArr[0] );
    double fMinVal = minArr[0];
    int fMaxIdx    = static_cast< int >( maxIdxArr[0] );
    double fMaxVal = maxArr[0];
    for ( int j = 1; j < 4; ++j ) {
        if ( minArr[j] < fMinVal ) { fMinVal = minArr[j]; fMinIdx = static_cast< int >( minIdxArr[j] ); }
        if ( maxArr[j] > fMaxVal ) { fMaxVal = maxArr[j]; fMaxIdx = static_cast< int >( maxIdxArr[j] ); }
    }
    seg[4] = __rdtsc();
    for ( ; i < count; ++i ) {
        if ( data[i] < fMinVal ) { fMinVal = data[i]; fMinIdx = i; }
        if ( data[i] > fMaxVal ) { fMaxVal = data[i]; fMaxIdx = i; }
    }
    seg[5] = __rdtsc();
    return { fMinIdx, fMaxIdx, fMinVal, fMaxVal };
}

// Same kernel with an INCREMENTAL index vector (one vaddpd per iteration
// instead of four int->double conversions + unpack + insert): the shape a
// stronger compiler or a hand-tuned loop would produce.
extern "C" __declspec( noinline ) QwtArgMinMaxResult
qb_kernel_incr( const double* data, int count )
{
    __m256d minVec    = _mm256_set1_pd( DBL_MAX );
    __m256d maxVec    = _mm256_set1_pd( -DBL_MAX );
    __m256d minIdxVec = _mm256_set_pd( 3.0, 2.0, 1.0, 0.0 );
    __m256d maxIdxVec = minIdxVec;
    const __m256d step = _mm256_set1_pd( 4.0 );
    __m256d curIdx = minIdxVec;
    int i = 0;
    for ( ; i + 4 <= count; i += 4 ) {
        const __m256d vals   = _mm256_loadu_pd( data + i );
        const __m256d cmpMin = _mm256_cmp_pd( vals, minVec, _CMP_LT_OQ );
        minIdxVec            = _mm256_blendv_pd( minIdxVec, curIdx, cmpMin );
        minVec               = _mm256_min_pd( minVec, vals );
        const __m256d cmpMax = _mm256_cmp_pd( vals, maxVec, _CMP_GT_OQ );
        maxIdxVec            = _mm256_blendv_pd( maxIdxVec, curIdx, cmpMax );
        maxVec               = _mm256_max_pd( maxVec, vals );
        curIdx               = _mm256_add_pd( curIdx, step );
    }
    double minArr[4], maxArr[4], minIdxArr[4], maxIdxArr[4];
    _mm256_storeu_pd( minArr, minVec );
    _mm256_storeu_pd( maxArr, maxVec );
    _mm256_storeu_pd( minIdxArr, minIdxVec );
    _mm256_storeu_pd( maxIdxArr, maxIdxVec );
    int fMinIdx    = static_cast< int >( minIdxArr[0] );
    double fMinVal = minArr[0];
    int fMaxIdx    = static_cast< int >( maxIdxArr[0] );
    double fMaxVal = maxArr[0];
    for ( int j = 1; j < 4; ++j ) {
        if ( minArr[j] < fMinVal ) { fMinVal = minArr[j]; fMinIdx = static_cast< int >( minIdxArr[j] ); }
        if ( maxArr[j] > fMaxVal ) { fMaxVal = maxArr[j]; fMaxIdx = static_cast< int >( maxIdxArr[j] ); }
    }
    for ( ; i < count; ++i ) {
        if ( data[i] < fMinVal ) { fMinVal = data[i]; fMinIdx = i; }
        if ( data[i] > fMaxVal ) { fMaxVal = data[i]; fMaxIdx = i; }
    }
    return { fMinIdx, fMaxIdx, fMinVal, fMaxVal };
}

// Vector loop only (no horizontal reduction, no tail): the four accumulator
// vectors are dumped to a volatile sink. Isolates the loop from the epilogue.
extern "C" __declspec( noinline ) void
qb_kernel_loop_only( const double* data, int count )
{
    __m256d minVec    = _mm256_set1_pd( DBL_MAX );
    __m256d maxVec    = _mm256_set1_pd( -DBL_MAX );
    __m256d minIdxVec = _mm256_set_pd( 3.0, 2.0, 1.0, 0.0 );
    __m256d maxIdxVec = minIdxVec;
    int i = 0;
    for ( ; i + 4 <= count; i += 4 ) {
        const __m256d vals   = _mm256_loadu_pd( data + i );
        const __m256d curIdx = _mm256_set_pd( static_cast< double >( i + 3 ),
                                              static_cast< double >( i + 2 ),
                                              static_cast< double >( i + 1 ),
                                              static_cast< double >( i ) );
        const __m256d cmpMin = _mm256_cmp_pd( vals, minVec, _CMP_LT_OQ );
        minIdxVec            = _mm256_blendv_pd( minIdxVec, curIdx, cmpMin );
        minVec               = _mm256_min_pd( minVec, vals );
        const __m256d cmpMax = _mm256_cmp_pd( vals, maxVec, _CMP_GT_OQ );
        maxIdxVec            = _mm256_blendv_pd( maxIdxVec, curIdx, cmpMax );
        maxVec               = _mm256_max_pd( maxVec, vals );
    }
    static volatile double sinkBuf[16];
    _mm256_storeu_pd( const_cast< double* >( sinkBuf ), minVec );
    _mm256_storeu_pd( const_cast< double* >( sinkBuf ) + 4, maxVec );
    _mm256_storeu_pd( const_cast< double* >( sinkBuf ) + 8, minIdxVec );
    _mm256_storeu_pd( const_cast< double* >( sinkBuf ) + 12, maxIdxVec );
}

// Same as the library AVX2 kernel, but with an explicit vzeroupper inserted
// between the vector loop and the reduction epilogue. If the fixed overhead
// is an AVX-SSE transition effect inside the epilogue, this removes it.
extern "C" __declspec( noinline ) QwtArgMinMaxResult
qb_kernel_vz_before_reduce( const double* data, int count )
{
    __m256d minVec    = _mm256_set1_pd( DBL_MAX );
    __m256d maxVec    = _mm256_set1_pd( -DBL_MAX );
    __m256d minIdxVec = _mm256_set_pd( 3.0, 2.0, 1.0, 0.0 );
    __m256d maxIdxVec = minIdxVec;
    int i = 0;
    for ( ; i + 4 <= count; i += 4 ) {
        const __m256d vals   = _mm256_loadu_pd( data + i );
        const __m256d curIdx = _mm256_set_pd( static_cast< double >( i + 3 ),
                                              static_cast< double >( i + 2 ),
                                              static_cast< double >( i + 1 ),
                                              static_cast< double >( i ) );
        const __m256d cmpMin = _mm256_cmp_pd( vals, minVec, _CMP_LT_OQ );
        minIdxVec            = _mm256_blendv_pd( minIdxVec, curIdx, cmpMin );
        minVec               = _mm256_min_pd( minVec, vals );
        const __m256d cmpMax = _mm256_cmp_pd( vals, maxVec, _CMP_GT_OQ );
        maxIdxVec            = _mm256_blendv_pd( maxIdxVec, curIdx, cmpMax );
        maxVec               = _mm256_max_pd( maxVec, vals );
    }
    qb_vzeroupper();  // <-- the only difference
    double minArr[4], maxArr[4], minIdxArr[4], maxIdxArr[4];
    _mm256_storeu_pd( minArr, minVec );
    _mm256_storeu_pd( maxArr, maxVec );
    _mm256_storeu_pd( minIdxArr, minIdxVec );
    _mm256_storeu_pd( maxIdxArr, maxIdxVec );
    int fMinIdx    = static_cast< int >( minIdxArr[0] );
    double fMinVal = minArr[0];
    int fMaxIdx    = static_cast< int >( maxIdxArr[0] );
    double fMaxVal = maxArr[0];
    for ( int j = 1; j < 4; ++j ) {
        if ( minArr[j] < fMinVal ) { fMinVal = minArr[j]; fMinIdx = static_cast< int >( minIdxArr[j] ); }
        if ( maxArr[j] > fMaxVal ) { fMaxVal = maxArr[j]; fMaxIdx = static_cast< int >( maxIdxArr[j] ); }
    }
    for ( ; i < count; ++i ) {
        if ( data[i] < fMinVal ) { fMinVal = data[i]; fMinIdx = i; }
        if ( data[i] > fMaxVal ) { fMaxVal = data[i]; fMaxIdx = i; }
    }
    return { fMinIdx, fMaxIdx, fMinVal, fMaxVal };
}

// ------------------------------------------------------------------ probes

// Runtime-fed buffers so the compiler cannot constant-fold the probe bodies.
static double g_chainBuf[64];

// Dirty the upper halves of YMM registers with a real 256-bit op on runtime
// data. `seed` varies per round so nothing can be hoisted out of the
// measurement loop (loop-invariant code motion folded an earlier version
// of this probe into oblivion).
static inline void dirtyUpper( double seed )
{
    const __m256d s = _mm256_set1_pd( seed );
    const __m256d a = _mm256_loadu_pd( g_chainBuf );
    const __m256d b = _mm256_add_pd( a, s );
    _mm256_storeu_pd( g_chainBuf + 8, b );
}

// K dependent LEGACY-encoded SSE adds on runtime data (this TU is built
// without /arch:AVX, so _mm_add_pd emits legacy ADDPD - the instruction
// class subject to the AVX-SSE transition penalty while the upper-YMM state
// is dirty). The seed makes every round's chain unique, defeating LICM and
// constant folding; without it the whole chain is hoisted out of the loop.
static inline void legacySseChain( int K, double seed )
{
    const __m128d c0 = _mm_loadu_pd( g_chainBuf + 16 );
    const __m128d x0 = _mm_set1_pd( seed );
    __m128d x = _mm_xor_pd( x0, _mm_loadu_pd( g_chainBuf + 24 ) );
    for ( int i = 0; i < K; ++i )
        x = _mm_add_pd( x, c0 );
    g_chainBuf[26] = _mm_cvtsd_f64( x ) + seed;
}

// ---------------------------------------------------------------- NaN matrix

static std::string fmtVal( double v )
{
    if ( std::isnan( v ) ) return "NaN";
    if ( v == DBL_MAX ) return "DBL_MAX";
    if ( v == -DBL_MAX ) return "-DBL_MAX";
    char b[32];
    std::snprintf( b, sizeof b, "%g", v );
    return b;
}

static std::vector< double > applyPattern( const std::vector< double >& base, const char* pat )
{
    const int n = static_cast< int >( base.size() );
    std::vector< double > v = base;
    const double NaN = std::nan( "" );
    if ( !std::strcmp( pat, "none" ) ) {
    } else if ( !std::strcmp( pat, "first" ) ) {
        v[0] = NaN;
    } else if ( !std::strcmp( pat, "last" ) ) {
        v[n - 1] = NaN;
    } else if ( !std::strcmp( pat, "mid" ) ) {
        v[n / 2] = NaN;
    } else if ( !std::strcmp( pat, "first+last" ) ) {
        v[0] = NaN; v[n - 1] = NaN;
    } else if ( !std::strcmp( pat, "all" ) ) {
        for ( auto& x : v ) x = NaN;
    } else if ( !std::strcmp( pat, "pair_mid" ) ) {
        v[n / 2] = NaN; v[n / 2 + 1] = NaN;
    } else if ( !std::strcmp( pat, "even_idx" ) ) {
        for ( int i = 0; i < n; i += 2 ) v[i] = NaN;
    } else if ( !std::strcmp( pat, "odd_idx" ) ) {
        for ( int i = 1; i < n; i += 2 ) v[i] = NaN;
    } else if ( pat[0] == 'p' ) {  // p<k>: single NaN at index k
        v[std::atoi( pat + 1 )] = NaN;
    }
    return v;
}

// NaN-ignoring reference: min/max over non-NaN elements, first occurrence wins.
static void nanFreeReference( const std::vector< double >& v,
                              int& riMin, double& rvMin, int& riMax, double& rvMax )
{
    riMin = riMax = -1;
    rvMin = DBL_MAX; rvMax = -DBL_MAX;
    for ( int i = 0; i < static_cast< int >( v.size() ); ++i ) {
        if ( std::isnan( v[i] ) ) continue;
        if ( riMin < 0 || v[i] < rvMin ) { rvMin = v[i]; riMin = i; }
        if ( riMax < 0 || v[i] > rvMax ) { rvMax = v[i]; riMax = i; }
    }
}

// -------------------------------------------------------------- bandwidth

static void sumScalarPass( const double* d, long long n )
{
    // 8 independent accumulators: latency-hidden, load-port bound
    double s0 = 0, s1 = 0, s2 = 0, s3 = 0, s4 = 0, s5 = 0, s6 = 0, s7 = 0;
    long long i = 0;
    for ( ; i + 8 <= n; i += 8 ) {
        s0 += d[i]; s1 += d[i + 1]; s2 += d[i + 2]; s3 += d[i + 3];
        s4 += d[i + 4]; s5 += d[i + 5]; s6 += d[i + 6]; s7 += d[i + 7];
    }
    for ( ; i < n; ++i ) s0 += d[i];
    volatile double keep = s0 + s1 + s2 + s3 + s4 + s5 + s6 + s7;
    ( void )keep;
}

static void sumAvx2Pass( const double* d, long long n )
{
    __m256d a0 = _mm256_setzero_pd(), a1 = _mm256_setzero_pd();
    __m256d a2 = _mm256_setzero_pd(), a3 = _mm256_setzero_pd();
    long long i = 0;
    for ( ; i + 16 <= n; i += 16 ) {
        a0 = _mm256_add_pd( a0, _mm256_loadu_pd( d + i ) );
        a1 = _mm256_add_pd( a1, _mm256_loadu_pd( d + i + 4 ) );
        a2 = _mm256_add_pd( a2, _mm256_loadu_pd( d + i + 8 ) );
        a3 = _mm256_add_pd( a3, _mm256_loadu_pd( d + i + 12 ) );
    }
    for ( ; i + 4 <= n; i += 4 ) a0 = _mm256_add_pd( a0, _mm256_loadu_pd( d + i ) );
    a0 = _mm256_add_pd( _mm256_add_pd( a0, a1 ), _mm256_add_pd( a2, a3 ) );
    double t[4];
    _mm256_storeu_pd( t, a0 );
    double s = t[0] + t[1] + t[2] + t[3];
    for ( ; i < n; ++i ) s += d[i];
    volatile double keep = s;
    ( void )keep;
    qb_vzeroupper();
}

static double* g_copyDst = nullptr;

static void copyPass( const double* d, long long n )
{
    std::memcpy( g_copyDst, d, static_cast< size_t >( n ) * sizeof( double ) );
    volatile double keep = g_copyDst[0];
    ( void )keep;
}

// --------------------------------------------------------------------- main

int main()
{
    // --- pin to logical processor 0 (P-core), raise timer friendliness
    SetThreadAffinityMask( GetCurrentThread(), 1 );

    // ---- human-readable comment header (CSV consumers skip '#' lines)
    std::printf( "# qwtSimdArgMinMax microbenchmark - measured results\n" );
    std::printf( "# cpu: %s\n", cpuBrand().c_str() );
    {
        SYSTEM_INFO si;
        GetSystemInfo( &si );
        std::printf( "# logical_processors: %lu (benchmark thread pinned to lp0 = P-core; E-core probes on lp16)\n",
                     si.dwNumberOfProcessors );
    }
    std::printf( "# cache: %s\n", cacheSummary().c_str() );
    std::printf( "# compiler: MSVC %d /O2 /std:c++17 /MD; main TU default x64 flags (SSE2 baseline, same as shipped qwtcore); wrapper TU /arch:AVX2\n", _MSC_VER );
    std::printf( "# os: Windows (win32 x64)\n" );
    std::printf( "# built: %s %s\n", __DATE__, __TIME__ );
    std::printf( "# protocol: min-of-7 timed runs after 3 warmup runs; each run auto-calibrated to ~50 ms;\n" );
    std::printf( "#           batch remeasured (max 3) if median/min > 1.30; note=remeasured marks such points\n" );
    std::printf( "# schema: mode,path,p1,p2,reps,min_ns,med_ns,max_ns,gbs,note (min/med/max are per work unit)\n" );
    std::printf( "mode,path,p1,p2,reps,min_ns,med_ns,max_ns,gbs,note\n" );

    // ---- env
    row( "env", "cpu", cpuBrand(), "", 0, -1, -1, -1, -1, "brand_from_cpuid" );
    {
        SYSTEM_INFO si;
        GetSystemInfo( &si );
        char b[64];
        std::snprintf( b, sizeof b, "%lu", si.dwNumberOfProcessors );
        row( "env", "logical_processors", b, "", 0, -1, -1, -1, -1,
             "thread_pinned_to_lp0" );
    }
    {
        std::vector< std::string > lines;
        cacheInfo( lines );
        std::string joined;
        for ( size_t i = 0; i < lines.size(); ++i ) {
            if ( i ) joined += ";";
            joined += lines[i];
        }
        row( "env", "cache", joined, "", 0, -1, -1, -1, -1, "GetLogicalProcessorInformation" );
    }
    {
        char b[64];
        std::snprintf( b, sizeof b, "%.3f", clockProbeGhz() );
        row( "env", "clock_ghz_probe", b, "", 0, -1, -1, -1, -1, "xorshift64_chain;6x1cyc_latency_assumed" );
    }
    {
        char b[64];
        std::snprintf( b, sizeof b, "%d", _MSC_VER );
        row( "env", "msvc", b, "default_flags_O2", 0, -1, -1, -1, -1,
             "main_TU_without_arch_flag;wrapper_TU_with_arch_AVX2" );
    }
    row( "env", "simd_level", std::to_string( static_cast< int >( kSimdLevel ) ), "",
         0, -1, -1, -1, -1, "0=scalar;1=sse42;2=avx2" );
    row( "env", "date", __DATE__, __TIME__, 0, -1, -1, -1, -1, "build_timestamp" );
    row( "env", "protocol", "min-of-7", "warmup=3", 0, -1, -1, -1, -1,
         "run~50ms;remeasure_if_med_over_min_gt_1.30;max_3_batches" );

    // ---- correctness on clean data (three statics must agree exactly)
    {
        std::mt19937_64 rng( 0xC0FFEE );
        bool ok = true;
        for ( int n = 1; n <= 100000 && ok; n = n < 64 ? n + 1 : n * 7 + 13 ) {
            std::vector< double > v( n );
            std::uniform_real_distribution< double > u( -1e6, 1e6 );
            for ( auto& x : v ) x = u( rng );
            const QwtArgMinMaxResult a = argMinMaxScalar( v.data(), n );
            const QwtArgMinMaxResult b = argMinMaxSSE42( v.data(), n );
            const QwtArgMinMaxResult c = argMinMaxAVX2( v.data(), n );
            ok = a.minIdx == b.minIdx && a.minIdx == c.minIdx
                 && a.maxIdx == b.maxIdx && a.maxIdx == c.maxIdx
                 && a.minVal == b.minVal && a.minVal == c.minVal
                 && a.maxVal == b.maxVal && a.maxVal == c.maxVal;
            if ( !ok ) {
                char b2[96];
                std::snprintf( b2, sizeof b2, "n=%d", n );
                row( "meta", "correctness", "FAIL", b2, 0, -1, -1, -1, -1, "paths_disagree" );
            }
        }
        if ( ok ) row( "meta", "correctness", "PASS", "clean_data_n_1_to_100k", 0, -1, -1, -1, -1, "" );
    }

    // ---- scan: whole-array argmin/argmax, n = 8 .. 16M
    {
        const struct { const char* name; Fn fn; } paths[] = {
            { "scalar", argMinMaxScalar },
            { "sse42", argMinMaxSSE42 },
            { "avx2", argMinMaxAVX2 },
            { "public", qwtSimdArgMinMax },
        };
        for ( long long n = 8; n <= 16LL * 1024 * 1024; n *= 2 ) {
            const std::vector< double > v = makeData( static_cast< size_t >( n ), 0x51ED );
            for ( const auto& p : paths ) {
                const double* dp = v.data();
                const int ni = static_cast< int >( n );
                Fn fn = p.fn;
                BenchResult r = measure( [&]( long long reps ) {
                    for ( long long i = 0; i < reps; ++i ) {
                        const QwtArgMinMaxResult res = fn( dp, ni );
                        g_sink += res.minIdx + res.maxIdx;
                    }
                } );
                std::string note = r.remeasured ? "remeasured" : "";
                row( "scan", p.name, std::to_string( n ), "", r.reps,
                     r.minNs, r.medNs, r.maxNs, gbsOf( n, r.minNs ), note );
            }
        }
    }

    // ---- bucket mode: one 1M array consumed as consecutive chunks
    //      (exactly how qwtMinMaxBucketReduce calls qwtSimdArgMinMax)
    {
        const long long N = 1024 * 1024;
        const std::vector< double > v = makeData( static_cast< size_t >( N ), 0xB0C );
        const struct { const char* name; Fn fn; } paths[] = {
            { "scalar", argMinMaxScalar },
            { "sse42", argMinMaxSSE42 },
            { "avx2", argMinMaxAVX2 },
            { "public", qwtSimdArgMinMax },
        };
        for ( int chunk : { 4, 8, 16, 32, 64, 128, 256, 512, 735, 1024, 4096 } ) {
            const int nChunks = static_cast< int >( N / chunk );
            for ( const auto& p : paths ) {
                const double* dp = v.data();
                Fn fn = p.fn;
                BenchResult r = measure( [&]( long long reps ) {
                    for ( long long pass = 0; pass < reps; ++pass ) {
                        long long acc = 0;
                        for ( int c = 0; c < nChunks; ++c ) {
                            const QwtArgMinMaxResult res = fn( dp + static_cast< long long >( c ) * chunk, chunk );
                            acc += res.minIdx + res.maxIdx;
                        }
                        g_sink += acc;
                    }
                } );
                const double perCall = r.minNs / nChunks;
                const double perCallMed = r.medNs / nChunks;
                const double perCallMax = r.maxNs / nChunks;
                std::string note = r.remeasured ? "remeasured" : "";
                row( "bucket", p.name, std::to_string( N ), std::to_string( chunk ), r.reps,
                     perCall, perCallMed, perCallMax, gbsOf( chunk, perCall ), note );
            }
        }
    }

    // ---- bandwidth references: scalar sum, AVX2 sum, memcpy
    {
        for ( long long n = 8192; n <= 16LL * 1024 * 1024; n *= 2 ) {
            const std::vector< double > v = makeData( static_cast< size_t >( n ), 0xB4D );
            std::vector< double > dst( static_cast< size_t >( n ) );
            g_copyDst = dst.data();
            const double* dp = v.data();

            BenchResult r1 = measure( [&]( long long reps ) {
                for ( long long i = 0; i < reps; ++i ) sumScalarPass( dp, n );
            } );
            row( "bandwidth", "sum_scalar", std::to_string( n ), "", r1.reps,
                 r1.minNs, r1.medNs, r1.maxNs, gbsOf( n, r1.minNs ),
                 r1.remeasured ? "remeasured" : "" );

            BenchResult r2 = measure( [&]( long long reps ) {
                for ( long long i = 0; i < reps; ++i ) sumAvx2Pass( dp, n );
            } );
            row( "bandwidth", "sum_avx2", std::to_string( n ), "", r2.reps,
                 r2.minNs, r2.medNs, r2.maxNs, gbsOf( n, r2.minNs ),
                 r2.remeasured ? "remeasured" : "" );

            BenchResult r3 = measure( [&]( long long reps ) {
                for ( long long i = 0; i < reps; ++i ) copyPass( dp, n );
            } );
            // copy moves 2n*8 bytes (read + write)
            row( "bandwidth", "copy", std::to_string( n ), "", r3.reps,
                 r3.minNs, r3.medNs, r3.maxNs, 2.0 * gbsOf( n, r3.minNs ),
                 r3.remeasured ? "remeasured" : "" );
            g_copyDst = nullptr;
        }
    }

    // ---- transition-penalty probes
    {
        for ( int k = 0; k < 64; ++k ) g_chainBuf[k] = 1e-6 * ( k + 1 );
        const std::vector< double > v16 = makeData( 16, 0xA11 );
        const int CH = 64;  // legacy SSE ops per probe round

        // (a) legacy SSE chain cost depending on preceding state
        struct ChainCase { const char* name; int setup; };
        const ChainCase cases[] = {
            { "chain_after_clean", 0 },        // vzeroupper before chain
            { "chain_after_dirty", 1 },        // 256-bit op, no vzeroupper
            { "chain_after_avx2_call", 2 },    // argMinMaxAVX2(v16) before chain
            { "chain_after_sse_call", 3 },     // argMinMaxSSE42(v16)
            { "chain_after_public_call", 4 },  // qwtSimdArgMinMax(v16)
            { "chain_after_avx2_vz", 5 },      // argMinMaxAVX2 + vzeroupper
            { "chain_pre_vz_avx2_call", 6 },   // vzeroupper + argMinMaxAVX2 (clean entry)
        };
        for ( const auto& cc : cases ) {
            const double* dp = v16.data();
            const int id = cc.setup;
            BenchResult r = measure( [&]( long long reps ) {
                for ( long long i = 0; i < reps; ++i ) {
                    const double seed = static_cast< double >( i ) * 1e-9;
                    switch ( id ) {
                    case 0: qb_vzeroupper(); break;
                    case 1: dirtyUpper( seed ); break;
                    case 2: { QwtArgMinMaxResult x = argMinMaxAVX2( dp, 16 ); g_sink += x.minIdx; } break;
                    case 3: { QwtArgMinMaxResult x = argMinMaxSSE42( dp, 16 ); g_sink += x.minIdx; } break;
                    case 4: { QwtArgMinMaxResult x = qwtSimdArgMinMax( dp, 16 ); g_sink += x.minIdx; } break;
                    case 5: { QwtArgMinMaxResult x = argMinMaxAVX2( dp, 16 ); g_sink += x.minIdx; qb_vzeroupper(); } break;
                    case 6: { qb_vzeroupper(); QwtArgMinMaxResult x = argMinMaxAVX2( dp, 16 ); g_sink += x.minIdx; } break;
                    }
                    legacySseChain( CH, seed );
                }
            } );
            char p1[32];
            std::snprintf( p1, sizeof p1, "%d", CH );
            row( "probe", cc.name, p1, "round=setup+chain", r.reps,
                 r.minNs, r.medNs, r.maxNs, -1, r.remeasured ? "remeasured" : "" );
        }

        // (b) call-cost variants at n=16 and n=4096 (isolate the anomaly)
        struct CallCase { const char* name; int id; };
        const CallCase calls[] = {
            { "call_avx2_direct", 0 },
            { "call_avx2_vz", 1 },        // explicit qb_vzeroupper() after each call
            { "call_avx2_wrapped", 2 },   // /arch:AVX2 TU wrapper (auto vzeroupper)
            { "call_sse_direct", 3 },
            { "call_sse_wrapped", 4 },    // wrapper-overhead control
            { "call_scalar", 5 },
            { "call_public", 6 },
            { "call_vz_only", 7 },       // qb_vzeroupper call-overhead control
        };
        for ( long long n : { 16LL, 4096LL } ) {
            const std::vector< double > v = makeData( static_cast< size_t >( n ), 0xCA11 );
            for ( const auto& cc : calls ) {
                const double* dp = v.data();
                const int ni = static_cast< int >( n );
                const int id = cc.id;
                BenchResult r = measure( [&]( long long reps ) {
                    for ( long long i = 0; i < reps; ++i ) {
                        QwtArgMinMaxResult x;
                        switch ( id ) {
                        case 0: x = argMinMaxAVX2( dp, ni ); break;
                        case 1: x = argMinMaxAVX2( dp, ni ); qb_vzeroupper(); break;
                        case 2: x = qb_wrapped_avx2( dp, ni ); break;
                        case 3: x = argMinMaxSSE42( dp, ni ); break;
                        case 4: x = qb_wrapped_sse42( dp, ni ); break;
                        case 5: x = argMinMaxScalar( dp, ni ); break;
                        case 6: x = qwtSimdArgMinMax( dp, ni ); break;
                        default: qb_vzeroupper(); x = argMinMaxScalar( dp, ni ); g_sink -= x.minIdx + x.maxIdx; break;
                        }
                        g_sink += x.minIdx + x.maxIdx;
                    }
                } );
                row( "probe", cc.name, std::to_string( n ), "", r.reps,
                     r.minNs, r.medNs, r.maxNs, gbsOf( n, r.minNs ),
                     r.remeasured ? "remeasured" : "" );
            }
        }

        // (b2) localization of the ~325 ns/call fixed overhead that the
        //      default-compiled argMinMaxAVX2 shows on P-cores
        const double tscPerNs = tscRatePerNs();
        {
            char b[64];
            std::snprintf( b, sizeof b, "%.4f", tscPerNs );
            row( "env", "tsc_rate_per_ns", b, "", 0, -1, -1, -1, -1, "invariant_tsc" );
        }
        {
            struct XCase { const char* name; int id; };
            const XCase xcalls[] = {
                { "call_avx2_via_avx2tu_ptr", 0 },  // default-compiled callee, AVX2-compiled caller
                { "call_kernel_incr", 1 },          // same loop, incremental index vector
                { "call_loop_only", 2 },            // vector loop without epilogue
                { "call_avx2_direct_2x", 3 },       // two direct calls per iteration
                { "call_vz_before_reduce", 4 },     // library-shape kernel + vzeroupper before epilogue
                { "call_avx2_pre_vz", 5 },          // vzeroupper before each call (clean entry state)
            };
            for ( long long n : { 16LL, 4096LL } ) {
                const std::vector< double > v = makeData( static_cast< size_t >( n ), 0xCA11 );
                for ( const auto& cc : xcalls ) {
                    const double* dp = v.data();
                    const int ni = static_cast< int >( n );
                    const int id = cc.id;
                    BenchResult r = measure( [&]( long long reps ) {
                        for ( long long i = 0; i < reps; ++i ) {
                            switch ( id ) {
                            case 0: { QwtArgMinMaxResult x = qb_call_via_ptr( argMinMaxAVX2, dp, ni ); g_sink += x.minIdx + x.maxIdx; } break;
                            case 1: { QwtArgMinMaxResult x = qb_kernel_incr( dp, ni ); g_sink += x.minIdx + x.maxIdx; } break;
                            case 2: qb_kernel_loop_only( dp, ni ); break;
                            case 3: {
                                QwtArgMinMaxResult x = argMinMaxAVX2( dp, ni );
                                QwtArgMinMaxResult y = argMinMaxAVX2( dp, ni );
                                g_sink += x.minIdx + y.maxIdx;
                            } break;
                            case 4: { QwtArgMinMaxResult x = qb_kernel_vz_before_reduce( dp, ni ); g_sink += x.minIdx + x.maxIdx; } break;
                            default: { qb_vzeroupper(); QwtArgMinMaxResult x = argMinMaxAVX2( dp, ni ); g_sink += x.minIdx + x.maxIdx; } break;
                            }
                        }
                    } );
                    row( "probe", cc.name, std::to_string( n ), "", r.reps,
                         r.minNs, r.medNs, r.maxNs, gbsOf( n, r.minNs ),
                         r.remeasured ? "remeasured" : "" );
                }
            }

            // rdtsc-segmented runs of the verbatim kernel copies
            static const char* segName[] = { "setup", "loop", "spill_store", "reduce", "tail" };
            for ( long long n : { 16LL, 4096LL } ) {
                const std::vector< double > v = makeData( static_cast< size_t >( n ), 0xCA11 );
                for ( int which = 0; which < 2; ++which ) {
                    const char* tag = which ? "instr_avx2tu" : "instr_default";
                    const int ITER = 20000;
                    std::vector< std::vector< double > > segNs( 5 );
                    unsigned long long seg[6];
                    for ( int it = 0; it < ITER; ++it ) {
                        const QwtArgMinMaxResult x = which
                            ? qb_instr_avx2( v.data(), static_cast< int >( n ), seg )
                            : qb_instr_default( v.data(), static_cast< int >( n ), seg );
                        g_sink += x.minIdx;
                        for ( int s = 0; s < 5; ++s )
                            segNs[s].push_back( static_cast< double >( seg[s + 1] - seg[s] ) / tscPerNs );
                    }
                    for ( int s = 0; s < 5; ++s ) {
                        std::sort( segNs[s].begin(), segNs[s].end() );
                        row( "probe", tag, std::to_string( n ), segName[s], ITER,
                             segNs[s][0], segNs[s][ITER / 2], segNs[s][ITER - 1], -1,
                             "rdtsc_segments_perturbed" );
                    }
                }
            }
        }

        // (c) hybrid cores: repeat key variants on an E-core (logical 16+)
        SYSTEM_INFO si;
        GetSystemInfo( &si );
        if ( si.dwNumberOfProcessors > 17 ) {
            SetThreadAffinityMask( GetCurrentThread(), 1ULL << 16 );
            char b[64];
            std::snprintf( b, sizeof b, "%.3f", clockProbeGhz() );
            row( "probe", "ecore_clock_ghz", b, "", 0, -1, -1, -1, -1, "lp16" );
            for ( long long n : { 16LL, 4096LL } ) {
                const std::vector< double > v = makeData( static_cast< size_t >( n ), 0xE0 );
                const double* dp = v.data();
                const int ni = static_cast< int >( n );
                const struct { const char* name; int id; } e[] = {
                    { "ecore_avx2_direct", 0 }, { "ecore_avx2_vz", 1 },
                    { "ecore_sse_direct", 2 }, { "ecore_scalar", 3 } };
                for ( const auto& cc : e ) {
                    const int id = cc.id;
                    BenchResult r = measure( [&]( long long reps ) {
                        for ( long long i = 0; i < reps; ++i ) {
                            QwtArgMinMaxResult x;
                            switch ( id ) {
                            case 0: x = argMinMaxAVX2( dp, ni ); break;
                            case 1: x = argMinMaxAVX2( dp, ni ); qb_vzeroupper(); break;
                            case 2: x = argMinMaxSSE42( dp, ni ); break;
                            default: x = argMinMaxScalar( dp, ni ); break;
                            }
                            g_sink += x.minIdx + x.maxIdx;
                        }
                    } );
                    row( "probe", cc.name, std::to_string( n ), "", r.reps,
                         r.minNs, r.medNs, r.maxNs, gbsOf( n, r.minNs ),
                         r.remeasured ? "remeasured" : "" );
                }
            }
            SetThreadAffinityMask( GetCurrentThread(), 1 );
        }
    }

    // ---- NaN semantics matrix (untimed)
    {
        const char* pats[] = { "none", "first", "last", "mid", "first+last", "all",
                               "pair_mid", "even_idx", "odd_idx", "p1", "p2", "p3" };
        for ( int n : { 8, 16 } ) {
            const std::vector< double > base = makeData( n, 0x4A4 );
            for ( const char* pat : pats ) {
                const std::vector< double > v = applyPattern( base, pat );
                int riMin, riMax;
                double rvMin, rvMax;
                nanFreeReference( v, riMin, rvMin, riMax, rvMax );
                char refBuf[160];
                std::snprintf( refBuf, sizeof refBuf, "ref: min=%d:%s max=%d:%s",
                               riMin, fmtVal( rvMin ).c_str(), riMax, fmtVal( rvMax ).c_str() );
                row( "nan", "reference", std::to_string( n ), pat, 0, -1, -1, -1, -1, refBuf );

                const struct { const char* name; Fn fn; } paths[] = {
                    { "scalar", argMinMaxScalar },
                    { "sse42", argMinMaxSSE42 },
                    { "avx2", argMinMaxAVX2 },
                    { "public", qwtSimdArgMinMax },
                };
                for ( const auto& p : paths ) {
                    const QwtArgMinMaxResult x = p.fn( v.data(), n );
                    char buf[192];
                    std::snprintf( buf, sizeof buf, "min=%d:%s max=%d:%s",
                                   x.minIdx, fmtVal( x.minVal ).c_str(),
                                   x.maxIdx, fmtVal( x.maxVal ).c_str() );
                    row( "nan", p.name, std::to_string( n ), pat, 0, -1, -1, -1, -1, buf );
                }
            }
        }
    }

    std::printf( "# done. sink=%lld\n", g_sink );
    return 0;
}

#endif  // QWT_SIMD_BENCH_AVX2_TU
