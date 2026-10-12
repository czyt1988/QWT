// Streaming data path benchmark for QWT 2D plots:
// ring buffer + QwtPlotDirectPainter (oscilloscope pattern) vs periodic
// full setSamples vs WindowedSeriesData-style rolling window, across
// production rates, window sizes and the FilterPointsLTTB downsampling
// switch. Measures per-frame cost (gen / commit / draw / present phases),
// effective frame rate, dropped frames and per-sample end-to-end latency
// (virtual arrival timestamp -> first paint that includes the sample).
//
// Build (Windows, MSVC 2019 x64, Qt 5.15.16, QWT release build in <repo>/build-release):
//   call vcvars64.bat
//   cl /nologo /O2 /EHsc /std:c++17 /MD ^
//     /I<repo>\src /I<repo>\src\core /I<repo>\src\plot ^
//     /I<qt>\include /I<qt>\include\QtCore /I<qt>\include\QtGui ^
//     /I<qt>\include\QtWidgets /I<qt>\include\QtOpenGL ^
//     /I<qt>\include\QtPrintSupport /I<qt>\include\QtConcurrent ^
//     streaming_bench.cpp /Fe:streaming_bench.exe ^
//     /link /LIBPATH:<repo>\build-release\lib /LIBPATH:<qt>\lib ^
//     qwtplot.lib qwtcore.lib Qt5Widgets.lib Qt5Gui.lib Qt5Core.lib ^
//     Qt5OpenGL.lib Qt5PrintSupport.lib Qt5Concurrent.lib opengl32.lib
//
// Run (needs <qt>\bin and <repo>\build-release\bin on PATH; real desktop session):
//   streaming_bench.exe all                       # full matrix: CSV header + one line per rep
//   streaming_bench.exe <strategy> <rate> <window> [--lttb on|off] [--dur 3.0] [--warm 0.3]
//                                                  # single config, one CSV line per rep
//
//   strategy  ring_dp     ring buffer (custom QwtSeriesData) + DirectPainter
//                         incremental segments, page-static x axis, full
//                         replot at page boundary (mirrors examples/2D/oscilloscope;
//                         canvas BackingStore OFF, ClipPolygons OFF)
//             ring_raw    like ring_dp but the window is re-bound every tick with
//                         setRawSamples(xs+s, ys+s, n) (QwtCPointerData<double>,
//                         SIMD-eligible mapper path) instead of a custom data class
//             ring_dp_bs  like ring_dp but canvas BackingStore ON -> exercises the
//                         double-paint path of QwtPlotDirectPainter
//             ring_replot like ring_dp commit, but sliding-axis full replot every
//                         tick instead of DirectPainter (isolates the DirectPainter gain)
//             setdata     periodic full commit: QVector<QPointF> window copy +
//                         setSamples() + sliding-axis replot every tick (cpuplot-style)
//             window      WindowedSeriesData-style: pre-generated backing arrays,
//                         setWindow(offset, n) + sliding-axis replot every tick
//                         (mirrors examples/bench/renderbench)
//
//   options --lttb on|off    QwtPlotCurve::FilterPointsLTTB paint attribute (default on)
//           --dur 3.0        measured seconds per rep
//           --warm 0.3       warmup seconds per rep (discarded)
//           --reps 3         base repetitions per config; if the spread of the rep
//                            medians exceeds 50% two extra reps are appended
//           --tick 10        producer/consumer timer interval in ms
//           --w 900 --h 500  target canvas size
//           --header         print the CSV header line (single-config mode)
//
// Timing protocol: production is driven by a QTimer in the GUI thread (batch of
// round(R*elapsed) samples per tick, cumulative rounding => zero drift). Each
// tick is timed in four phases with QElapsedTimer: gen (signal synthesis +
// ring append), commit (window rebind / vector copy / setWindow), draw
// (drawSeries or replot call), present (processEvents; the async paint of the
// full-replot strategies lands here). A frame is a "drop" when its total time
// exceeds 2x the rep median. Latency per sample: virtual arrival time
// (g+0.5)/R after rep start -> wall time of the first paint including it.
// Diagnostics go to stderr prefixed with '#'; stdout is pure CSV.

#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QPaintEvent>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QThread>
#include <QTimer>
#include <QVector>

#include <qwt_axis.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_directpainter.h>
#include <qwt_point_data.h>
#include <qwt_scale_div.h>
#include <qwt_scale_map.h>
#include <qwt_series_data.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ------------------------------------------------------------------ options

struct Options
{
    std::string strategy = "ring_dp";
    long long rate = 100000;
    long long window = 100000;
    bool lttb = true;
    double dur = 3.0;
    double warm = 0.3;
    int reps = 3;
    int tick = 10;
    int w = 900;
    int h = 500;
    bool header = false;
    bool all = false;

    static const std::vector< std::string >& strategies()
    {
        static const std::vector< std::string > s = { "ring_dp", "ring_raw", "ring_dp_bs",
            "ring_replot", "setdata", "window" };
        return s;
    }
    static const std::vector< long long >& rates()
    {
        static const std::vector< long long > v = { 1000, 10000, 100000, 1000000 };
        return v;
    }
    static const std::vector< long long >& windows()
    {
        static const std::vector< long long > v = { 10000, 100000, 1000000 };
        return v;
    }
};

static void usage()
{
    std::fprintf( stderr,
        "usage: streaming_bench.exe all | <strategy> <rate> <window> [options]\n"
        "  strategy: ring_dp|ring_raw|ring_dp_bs|ring_replot|setdata|window\n"
        "  options:  --lttb on|off --dur s --warm s --reps n --tick ms --w px --h px --header\n" );
}

static Options parseArgs( int argc, char* argv[] )
{
    Options o;
    int pos = 0;
    for ( int i = 1; i < argc; ++i )
    {
        const std::string a = argv[i];
        if ( a == "all" )
            o.all = true;
        else if ( a == "--lttb" && i + 1 < argc )
            o.lttb = std::string( argv[++i] ) == "on";
        else if ( a == "--dur" && i + 1 < argc )
            o.dur = std::atof( argv[++i] );
        else if ( a == "--warm" && i + 1 < argc )
            o.warm = std::atof( argv[++i] );
        else if ( a == "--reps" && i + 1 < argc )
            o.reps = std::atoi( argv[++i] );
        else if ( a == "--tick" && i + 1 < argc )
            o.tick = std::atoi( argv[++i] );
        else if ( a == "--w" && i + 1 < argc )
            o.w = std::atoi( argv[++i] );
        else if ( a == "--h" && i + 1 < argc )
            o.h = std::atoi( argv[++i] );
        else if ( a == "--header" )
            o.header = true;
        else if ( !a.empty() && a[0] != '-' && pos == 0 )
        {
            o.strategy = a;
            ++pos;
        }
        else if ( pos == 1 )
        {
            o.rate = std::atoll( a.c_str() );
            ++pos;
        }
        else if ( pos == 2 )
        {
            o.window = std::atoll( a.c_str() );
            ++pos;
        }
        else
        {
            usage();
            std::exit( 2 );
        }
    }
    if ( !o.all && pos < 3 )
    {
        usage();
        std::exit( 2 );
    }
    return o;
}

// ------------------------------------------------------------------ signal

// Deterministic splitmix64-based noise so every strategy sees identical data.
static inline quint64 splitmix64( quint64 z )
{
    z += 0x9E3779B97F4A7C15ull;
    z = ( z ^ ( z >> 30 ) ) * 0xBF58476D1CE4E5B9ull;
    z = ( z ^ ( z >> 27 ) ) * 0x94D049BB133111EBull;
    return z ^ ( z >> 31 );
}

// Virtual arrival time of sample g [s]; also used as its x coordinate.
static inline double sampleX( long long g, double rate )
{
    return ( double( g ) + 0.5 ) / rate;
}

static inline double sampleY( long long g, double rate )
{
    const double x = sampleX( g, rate );
    const quint64 h = splitmix64( quint64( g ) );
    const double noise = double( h >> 11 ) / 9007199254740992.0 * 2.0 - 1.0;
    return 0.8 * std::sin( 2.0 * M_PI * 7.0 * x )
           + 0.3 * std::sin( 2.0 * M_PI * 23.0 * x )
           + 0.05 * noise;
}

// ------------------------------------------------------------------ ring store

// True ring buffer with the doubled-array (mirror write) trick: every sample g
// is written to slot g%W and slot g%W+W, so any window of <= W consecutive
// samples is one contiguous block [s, s+n) of the 2W-sized arrays. Guarded by
// a QReadWriteLock like the SignalData singleton of examples/2D/oscilloscope
// (uncontended here: the bench produces inside the GUI thread).
class RingStore
{
public:
    void allocate( long long capacity )
    {
        m_cap = capacity;
        m_xs.resize( size_t( 2 * capacity ) );
        m_ys.resize( size_t( 2 * capacity ) );
        m_count = 0;
    }
    void reset() { m_count = 0; }

    // Batch append of global samples [fromG, toG) under the write lock.
    void appendBatch( long long fromG, long long toG, double rate )
    {
        QWriteLocker locker( &m_lock );
        for ( long long g = fromG; g < toG; ++g )
        {
            const long long p = g % m_cap;
            const double x = sampleX( g, rate );
            const double y = sampleY( g, rate );
            m_xs[size_t( p )] = x;
            m_ys[size_t( p )] = y;
            m_xs[size_t( p + m_cap )] = x;
            m_ys[size_t( p + m_cap )] = y;
        }
        m_count = toG;
    }

    long long count() const { return m_count; }
    long long capacity() const { return m_cap; }

    // Contiguous window of the last n samples: start slot.
    long long windowStart( long long n ) const { return ( m_count - n ) % m_cap; }
    long long windowSize() const { return std::min< long long >( m_count, m_cap ); }

    const double* xs() const { return m_xs.constData(); }
    const double* ys() const { return m_ys.constData(); }

    void lockForRead() const { m_lock.lockForRead(); }
    void unlockRead() const { m_lock.unlock(); }

private:
    mutable QReadWriteLock m_lock;
    QVector< double > m_xs, m_ys;
    long long m_cap = 0;
    long long m_count = 0;
};

// Custom series data over the ring window (generic mapper path, mirrors the
// CurveData class of examples/2D/oscilloscope reading through sample(i)).
class RingSeriesData : public QwtSeriesData< QPointF >
{
public:
    void setStore( const RingStore* store ) { m_store = store; }
    void setWindow( long long start, long long size )
    {
        m_start = start;
        m_size = size;
        cachedBoundingRect = QRectF( 0.0, 0.0, -1.0, -1.0 );
    }
    virtual size_t size() const override { return size_t( m_size ); }
    virtual QPointF sample( size_t i ) const override
    {
        const long long k = m_start + long long( i );
        return QPointF( m_store->xs()[size_t( k )], m_store->ys()[size_t( k )] );
    }
    virtual QRectF boundingRect() const override
    {
        if ( cachedBoundingRect.width() < 0.0 )
            cachedBoundingRect = qwtBoundingRect( *this );
        return cachedBoundingRect;
    }

private:
    const RingStore* m_store = nullptr;
    long long m_start = 0;
    long long m_size = 0;
};

// WindowedSeriesData of examples/bench/renderbench: pre-generated backing
// arrays, the displayed window is moved with setWindow() (no per-tick copy).
class WindowedStreamData : public QwtSeriesData< QPointF >
{
public:
    void setBacking( const QVector< double >* xs, const QVector< double >* ys )
    {
        m_xs = xs;
        m_ys = ys;
    }
    void setWindow( long long offset, long long size )
    {
        m_offset = offset;
        m_size = size;
        cachedBoundingRect = QRectF( 0.0, 0.0, -1.0, -1.0 );
    }
    virtual size_t size() const override { return size_t( m_size ); }
    virtual QPointF sample( size_t i ) const override
    {
        const long long k = m_offset + long long( i );
        return QPointF( ( *m_xs )[size_t( k )], ( *m_ys )[size_t( k )] );
    }
    virtual QRectF boundingRect() const override
    {
        if ( cachedBoundingRect.width() < 0.0 )
        {
            if ( m_size <= 0 )
            {
                cachedBoundingRect = QRectF( 0.0, 0.0, 0.0, 0.0 );
            }
            else
            {
                double minY = ( *m_ys )[size_t( m_offset )];
                double maxY = minY;
                for ( long long i = 1; i < m_size; ++i )
                {
                    const double y = ( *m_ys )[size_t( m_offset + i )];
                    if ( y < minY )
                        minY = y;
                    else if ( y > maxY )
                        maxY = y;
                }
                const double x0 = ( *m_xs )[size_t( m_offset )];
                const double x1 = ( *m_xs )[size_t( m_offset + m_size - 1 )];
                cachedBoundingRect = QRectF( x0, minY, x1 - x0, maxY - minY );
            }
        }
        return cachedBoundingRect;
    }

private:
    const QVector< double >* m_xs = nullptr;
    const QVector< double >* m_ys = nullptr;
    long long m_offset = 0;
    long long m_size = 0;
};

// ------------------------------------------------------------------ canvas

class CountingCanvas : public QwtPlotCanvas
{
public:
    explicit CountingCanvas( QwtPlot* plot = nullptr ) : QwtPlotCanvas( plot ) {}
    int paintCount = 0;
    void resetCount() { paintCount = 0; }

protected:
    void paintEvent( QPaintEvent* event ) override
    {
        ++paintCount;
        QwtPlotCanvas::paintEvent( event );
    }
};

// ------------------------------------------------------------------ stats

static double percentile( std::vector< double >& v, double p )
{
    if ( v.empty() )
        return 0.0;
    std::sort( v.begin(), v.end() );
    const double idx = p * double( v.size() - 1 );
    const size_t lo = size_t( std::floor( idx ) );
    const size_t hi = size_t( std::ceil( idx ) );
    if ( lo == hi )
        return v[lo];
    return v[lo] + ( v[hi] - v[lo] ) * ( idx - double( lo ) );
}

static double medianCopy( std::vector< double > v ) { return percentile( v, 0.5 ); }

struct RepResult
{
    double durMs = 0;
    long long frames = 0;
    double fpsEff = 0;
    double frameMed = 0, frameMean = 0, frameP95 = 0, frameP99 = 0, frameMax = 0;
    long long drops = 0;
    double genMed = 0, commitMed = 0, drawMed = 0, presentMed = 0, tickMed = 0;
    int paintEvents = 0;
    long long boundaryReplots = 0;
    long long samples = 0;
    long long latCount = 0;
    double latMed = 0, latP95 = 0, latP99 = 0, latMax = 0;
    int canvasW = 0, canvasH = 0;
    int skippedTicks = 0;
};

// ------------------------------------------------------------------ bench rig

class BenchRig
{
public:
    explicit BenchRig( const Options& o ) : m_o( o )
    {
        m_rate = double( o.rate );
        const bool isRing = o.strategy.rfind( "ring", 0 ) == 0;
        const bool stackSeries = ( o.strategy == "ring_dp" || o.strategy == "ring_dp_bs"
            || o.strategy == "ring_replot" || o.strategy == "window" );
        m_stackSeries = stackSeries;

        m_plot = new QwtPlot();
        m_plot->setAutoReplot( false );

        m_canvas = new CountingCanvas( m_plot );
        m_canvas->setBorderRadius( 0 );
        // ring_dp / ring_raw / ring_replot: BackingStore OFF like the oscilloscope
        // canvas; ring_dp_bs: ON (double-paint probe); setdata / window: library
        // default ON.
        const bool backingStore = ( o.strategy == "ring_dp_bs" || o.strategy == "setdata"
            || o.strategy == "window" );
        m_canvas->setPaintAttribute( QwtPlotCanvas::BackingStore, backingStore );
        m_plot->setCanvas( m_canvas );

        m_plot->setAxisScale( QwtAxis::YLeft, -1.5, 1.5 );
        m_plot->setAxisScale( QwtAxis::XBottom, 0.0, double( o.window ) / m_rate );

        m_curve = new QwtPlotCurve( "stream" );
        m_curve->setStyle( QwtPlotCurve::Lines );
        m_curve->setPen( QColor( "#4caf50" ), 1.0 );
        m_curve->setRenderHint( QwtPlotItem::RenderAntialiased, false );
        m_curve->setPaintAttribute( QwtPlotCurve::FilterPointsLTTB, o.lttb );
        if ( isRing ) // mirrors examples/2D/oscilloscope Plot.cpp:147
            m_curve->setPaintAttribute( QwtPlotCurve::ClipPolygons, false );

        if ( isRing )
        {
            m_ring.allocate( o.window );
            if ( o.strategy != "ring_raw" )
            {
                m_ringSeries.setStore( &m_ring );
                m_curve->setData( &m_ringSeries ); // ownership reclaimed in ~BenchRig
            }
            else
            {
                m_curve->setSamples( QVector< QPointF >() ); // data re-bound per tick
            }
        }
        else
        {
            m_total = totalSamples();
            m_xb.resize( m_total );
            m_yb.resize( m_total );
            for ( long long g = 0; g < m_total; ++g )
            {
                m_xb[size_t( g )] = sampleX( g, m_rate );
                m_yb[size_t( g )] = sampleY( g, m_rate );
            }
            if ( o.strategy == "window" )
            {
                m_winData.setBacking( &m_xb, &m_yb );
                m_curve->setData( &m_winData ); // ownership reclaimed in ~BenchRig
            }
            else // setdata
            {
                m_curve->setSamples( QVector< QPointF >() ); // empty start
            }
        }

        m_curve->attach( m_plot );

        if ( o.strategy == "ring_dp" || o.strategy == "ring_raw" || o.strategy == "ring_dp_bs" )
            m_direct = new QwtPlotDirectPainter( m_plot );

        m_plot->show();
        fitCanvasSize();
        m_plot->replot();
        QApplication::processEvents();
    }

    ~BenchRig()
    {
        if ( m_curve && m_stackSeries )
            m_curve->swapData( nullptr ); // take back the rig-owned series data
        delete m_plot; // deletes attached items (incl. heap-owned series data)
    }

    long long totalSamples() const
    {
        // enough for warmup + measured phase even under heavy overload
        return long long( std::ceil( m_rate * ( m_o.warm + m_o.dur + 2.0 ) ) ) + 16;
    }

    void fitCanvasSize()
    {
        QSize size = m_plot->size();
        for ( int i = 0; i < 4; ++i )
        {
            const QRect cr = m_canvas->contentsRect();
            const int dw = m_o.w - cr.width();
            const int dh = m_o.h - cr.height();
            if ( dw == 0 && dh == 0 )
                break;
            size += QSize( dw, dh );
            m_plot->resize( size );
            QApplication::processEvents();
        }
    }

    // One rep: warmup phase + measured phase. Returns collected stats.
    RepResult runRep()
    {
        runPhase( m_o.warm, false );
        resetStream();
        m_canvas->resetCount();
        RepResult r = runPhase( m_o.dur, true );
        r.paintEvents = m_canvas->paintCount;
        r.canvasW = m_canvas->contentsRect().width();
        r.canvasH = m_canvas->contentsRect().height();
        r.skippedTicks = m_skipped;
        return r;
    }

private:
    void resetStream()
    {
        m_G = 0;
        m_paintedG = 0;
        m_page = 0;
        m_boundaries = 0;
        m_ring.reset();
        m_latencies.clear();
        m_skipped = 0;
        m_inTick = false;
        m_lastTickNs = -1;
        // restore the page-0 axis and repaint once
        m_plot->setAxisScale( QwtAxis::XBottom, 0.0, double( m_o.window ) / m_rate );
        if ( m_direct )
            m_direct->reset();
        m_plot->replot();
        QApplication::processEvents();
    }

    RepResult runPhase( double seconds, bool measure )
    {
        RepResult r;
        std::vector< double > frames, gens, commits, draws, presents, gaps;
        const double durMs = seconds * 1000.0;

        QEventLoop loop;
        QTimer timer;
        timer.setTimerType( Qt::PreciseTimer );
        QObject::connect( &timer, &QTimer::timeout, [&]() {
            if ( m_inTick )
            {
                if ( measure )
                    ++m_skipped;
                return;
            }
            m_inTick = true;
            tick( measure, frames, gens, commits, draws, presents, gaps );
            m_inTick = false;
            if ( m_runClock.elapsed() >= durMs )
                loop.quit();
        } );
        m_runClock.restart();
        timer.start( m_o.tick );
        loop.exec();
        timer.stop();

        if ( measure )
        {
            r.durMs = double( m_runClock.elapsed() );
            r.frames = long long( frames.size() );
            r.fpsEff = r.frames > 0 ? 1000.0 * double( r.frames ) / r.durMs : 0.0;
            r.frameMed = percentile( frames, 0.5 );
            r.frameMean = frames.empty() ? 0.0
                : std::accumulate( frames.begin(), frames.end(), 0.0 ) / double( frames.size() );
            r.frameP95 = percentile( frames, 0.95 );
            r.frameP99 = percentile( frames, 0.99 );
            r.frameMax = frames.empty() ? 0.0 : *std::max_element( frames.begin(), frames.end() );
            for ( double f : frames )
                if ( f > 2.0 * r.frameMed )
                    ++r.drops;
            r.genMed = medianCopy( gens );
            r.commitMed = medianCopy( commits );
            r.drawMed = medianCopy( draws );
            r.presentMed = medianCopy( presents );
            r.tickMed = medianCopy( gaps );
            r.boundaryReplots = m_boundaries;
            r.samples = m_G;
            r.latCount = long long( m_latencies.size() );
            r.latMed = percentile( m_latencies, 0.5 );
            r.latP95 = percentile( m_latencies, 0.95 );
            r.latP99 = percentile( m_latencies, 0.99 );
            r.latMax = m_latencies.empty() ? 0.0
                : *std::max_element( m_latencies.begin(), m_latencies.end() );
        }
        return r;
    }

    // produce + commit + draw for one timer tick
    void tick( bool measure, std::vector< double >& frames, std::vector< double >& gens,
        std::vector< double >& commits, std::vector< double >& draws,
        std::vector< double >& presents, std::vector< double >& gaps )
    {
        const qint64 t0 = m_clock.nsecsElapsed();
        if ( m_lastTickNs >= 0 )
            gaps.push_back( double( t0 - m_lastTickNs ) / 1e6 );
        m_lastTickNs = t0;

        // ---- produce: cumulative rounding => zero long-term drift
        const double elapsedS = double( m_runClock.nsecsElapsed() ) / 1e9;
        const long long targetG = long long( std::llround( m_rate * elapsedS ) );
        const long long g0 = m_G;
        if ( targetG <= g0 )
            return; // early tick: nothing new to consume
        const bool isRing = m_o.strategy.rfind( "ring", 0 ) == 0;

        if ( isRing )
            m_ring.appendBatch( g0, targetG, m_rate );
        m_G = targetG;
        const qint64 t1 = m_clock.nsecsElapsed();

        // ---- commit (window rebind / vector copy) then draw call, then flush.
        // Timeline: t1 (gen done) -> t2 (commit done) -> tDraw (draw call
        // returned) -> tPaint (processEvents flushed the pixels). For the
        // DirectPainter strategies drawSeries() paints synchronously, so the
        // flush is ~0; for the full-replot strategies replot() only posts the
        // update and the flush carries the paint.
        qint64 t2 = t1;
        qint64 tDraw = t1;
        qint64 tPaint = t1;
        const long long W = m_o.window;

        if ( isRing )
        {
            const long long n = m_ring.windowSize();
            const long long s = m_ring.windowStart( n );
            m_ring.lockForRead();

            if ( m_o.strategy == "ring_replot" )
            {
                m_ringSeries.setWindow( s, n );
                m_plot->setAxisScale( QwtAxis::XBottom,
                    double( m_G - n ) / m_rate, double( m_G ) / m_rate );
                t2 = m_clock.nsecsElapsed();
                m_plot->replot();
                tDraw = m_clock.nsecsElapsed();
            }
            else
            {
                if ( m_o.strategy == "ring_raw" )
                    m_curve->setRawSamples( m_ring.xs() + s, m_ring.ys() + s, int( n ) );
                else
                    m_ringSeries.setWindow( s, n );

                const long long page = ( m_G - 1 ) / W;
                if ( page > m_page )
                {
                    // page boundary: re-anchor the x axis to the new page and
                    // repaint everything (oscilloscope incrementInterval pattern)
                    m_page = page;
                    const double pageLen = double( W ) / m_rate;
                    m_plot->setAxisScale( QwtAxis::XBottom,
                        page * pageLen, ( page + 1 ) * pageLen );
                    if ( m_direct )
                        m_direct->reset();
                    t2 = m_clock.nsecsElapsed();
                    m_plot->replot();
                    tDraw = m_clock.nsecsElapsed();
                    ++m_boundaries;
                    m_paintedG = m_G;
                }
                else if ( m_G > m_paintedG && m_direct )
                {
                    // incremental: paint only the new segment (1 point overlap)
                    const long long winBase = m_G - n;
                    long long iFrom = m_paintedG - winBase;
                    iFrom = ( iFrom < 1 ) ? 0 : iFrom - 1;
                    const int from = int( iFrom );
                    const int to = int( n - 1 );

                    // clip region from the bounding rect of the new segment
                    // (mirrors oscilloscope Plot::updateCurve)
                    const QwtScaleMap xMap = m_plot->canvasMap( m_curve->xAxis() );
                    const QwtScaleMap yMap = m_plot->canvasMap( m_curve->yAxis() );
                    const QRectF br = qwtBoundingRect( *m_curve->data(), size_t( from ), size_t( to ) );
                    const QRect clipRect = QwtScaleMap::transform( xMap, yMap, br ).toRect();
                    m_direct->setClipRegion( clipRect );
                    t2 = m_clock.nsecsElapsed();

                    m_direct->drawSeries( m_curve, from, to );
                    tDraw = m_clock.nsecsElapsed();
                    m_paintedG = m_G;
                }
                else
                {
                    t2 = tDraw = m_clock.nsecsElapsed();
                }
            }
            m_ring.unlockRead();
        }
        else if ( m_o.strategy == "setdata" )
        {
            const long long n = std::min< long long >( m_G, W );
            const long long base = m_G - n;
            QVector< QPointF > vec;
            vec.resize( int( n ) );
            for ( long long i = 0; i < n; ++i )
                vec[int( i )] = QPointF( m_xb[size_t( base + i )], m_yb[size_t( base + i )] );
            m_curve->setSamples( vec ); // COW: shares the vector, no second copy
            m_plot->setAxisScale( QwtAxis::XBottom, double( base ) / m_rate, double( m_G ) / m_rate );
            t2 = m_clock.nsecsElapsed();
            m_plot->replot();
            tDraw = m_clock.nsecsElapsed();
        }
        else // window
        {
            const long long n = std::min< long long >( m_G, W );
            const long long base = m_G - n;
            m_winData.setWindow( base, n );
            m_plot->setAxisScale( QwtAxis::XBottom, double( base ) / m_rate, double( m_G ) / m_rate );
            t2 = m_clock.nsecsElapsed();
            m_plot->replot();
            tDraw = m_clock.nsecsElapsed();
        }

        // uniform flush: deliver posted paint events so tPaint is pixel-complete
        QApplication::processEvents();
        tPaint = m_clock.nsecsElapsed();

        // ---- end-to-end latency of the samples first painted in this frame
        if ( measure )
        {
            const double paintEndMs = double( m_runClock.nsecsElapsed() ) / 1e6;
            for ( long long g = g0; g < m_G; ++g )
            {
                const double arrivalMs = sampleX( g, m_rate ) * 1000.0; // virtual arrival
                m_latencies.push_back( paintEndMs - arrivalMs );
            }
        }

        frames.push_back( double( tPaint - t0 ) / 1e6 );
        gens.push_back( double( t1 - t0 ) / 1e6 );
        commits.push_back( double( t2 - t1 ) / 1e6 );
        draws.push_back( double( tDraw - t2 ) / 1e6 );
        presents.push_back( double( tPaint - tDraw ) / 1e6 );
    }

    Options m_o;
    double m_rate = 0;
    QwtPlot* m_plot = nullptr;
    CountingCanvas* m_canvas = nullptr;
    QwtPlotCurve* m_curve = nullptr;
    QwtPlotDirectPainter* m_direct = nullptr;
    RingStore m_ring;
    RingSeriesData m_ringSeries;
    WindowedStreamData m_winData;
    bool m_stackSeries = false;
    QVector< double > m_xb, m_yb; // pre-generated backing arrays
    long long m_total = 0;

    QElapsedTimer m_clock;    // nsec phase clock (free running)
    QElapsedTimer m_runClock; // stream clock, zero at phase start
    long long m_G = 0;        // samples produced in this phase
    long long m_paintedG = 0; // samples painted so far (ring/direct strategies)
    long long m_page = 0;     // current page index (ring/direct strategies)
    long long m_boundaries = 0;
    int m_skipped = 0;
    bool m_inTick = false;
    qint64 m_lastTickNs = -1;
    std::vector< double > m_latencies;
};

// ------------------------------------------------------------------ CSV

static const char* csvHeader =
    "strategy,rate,window,lttb,rep,reps_run,jitter,canvas_w,canvas_h,dur_ms,frames,fps_eff,"
    "frame_med_ms,frame_mean_ms,frame_p95_ms,frame_p99_ms,frame_max_ms,drops,gen_med_ms,"
    "commit_med_ms,draw_med_ms,present_med_ms,tick_med_ms,paint_events,boundary_replots,"
    "samples,lat_count,lat_med_ms,lat_p95_ms,lat_p99_ms,lat_max_ms,skipped_ticks";

static void printRow( const Options& o, int rep, int repsRun, int jitter, const RepResult& r )
{
    std::printf( "%s,%lld,%lld,%d,%d,%d,%d,%d,%d,%.1f,%lld,%.2f,%.4f,%.4f,%.4f,%.4f,%.4f,%lld,"
                 "%.4f,%.4f,%.4f,%.4f,%.3f,%d,%lld,%lld,%lld,%.3f,%.3f,%.3f,%.3f,%d\n",
        o.strategy.c_str(), o.rate, o.window, int( o.lttb ), rep, repsRun, jitter,
        r.canvasW, r.canvasH, r.durMs, r.frames, r.fpsEff,
        r.frameMed, r.frameMean, r.frameP95, r.frameP99, r.frameMax, r.drops,
        r.genMed, r.commitMed, r.drawMed, r.presentMed, r.tickMed,
        r.paintEvents, r.boundaryReplots, r.samples,
        r.latCount, r.latMed, r.latP95, r.latP99, r.latMax, r.skippedTicks );
    std::fflush( stdout );
}

static void runConfig( const Options& o )
{
    std::vector< RepResult > results;
    std::vector< double > meds;
    int repsRun = 0;
    int jitter = 0;

    for ( int pass = 0; pass < 2; ++pass )
    {
        const int target = ( pass == 0 ) ? o.reps : o.reps + 2;
        while ( repsRun < target )
        {
            BenchRig rig( o );
            const RepResult r = rig.runRep();
            ++repsRun;
            results.push_back( r );
            meds.push_back( r.frameMed );
            std::fprintf( stderr, "# cfg %s rate=%lld win=%lld lttb=%d rep=%d canvas=%dx%d "
                "med=%.3f p95=%.3f fps=%.1f drops=%lld bnd=%lld paint_ev=%d\n",
                o.strategy.c_str(), o.rate, o.window, int( o.lttb ), repsRun,
                r.canvasW, r.canvasH, r.frameMed, r.frameP95, r.fpsEff, r.drops,
                r.boundaryReplots, r.paintEvents );
            QThread::msleep( 250 );
        }
        const double mn = *std::min_element( meds.begin(), meds.end() );
        const double mx = *std::max_element( meds.begin(), meds.end() );
        if ( mn <= 0 || ( mx - mn ) / mn <= 0.5 )
            break; // stable
        jitter = 1;
        std::fprintf( stderr, "# cfg %s rate=%lld win=%lld lttb=%d jitter %.0f%% -> 2 extra reps\n",
            o.strategy.c_str(), o.rate, o.window, int( o.lttb ), 100.0 * ( mx - mn ) / mn );
    }

    for ( size_t i = 0; i < results.size(); ++i )
        printRow( o, int( i ) + 1, repsRun, jitter, results[i] );
}

int main( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    const Options base = parseArgs( argc, argv );

    std::fprintf( stderr, "# streaming bench: qt=%s strategy=%s rate=%lld window=%lld lttb=%d "
        "dur=%.1f warm=%.1f reps=%d tick=%d target=%dx%d\n",
        QT_VERSION_STR, base.strategy.c_str(), base.rate, base.window, int( base.lttb ),
        base.dur, base.warm, base.reps, base.tick, base.w, base.h );

    if ( base.header || base.all )
    {
        std::printf( "%s\n", csvHeader );
        std::fflush( stdout );
    }

    if ( base.all )
    {
        for ( const std::string& strat : Options::strategies() )
            for ( long long rate : Options::rates() )
                for ( long long win : Options::windows() )
                    for ( int lttb = 1; lttb >= 0; --lttb )
                    {
                        Options o = base;
                        o.strategy = strat;
                        o.rate = rate;
                        o.window = win;
                        o.lttb = ( lttb != 0 );
                        std::fprintf( stderr, "# === config %s rate=%lld win=%lld lttb=%d ===\n",
                            strat.c_str(), rate, win, lttb );
                        runConfig( o );
                        QThread::msleep( 1000 ); // cool down between configs
                    }
    }
    else
    {
        runConfig( base );
    }

    std::fprintf( stderr, "# done\n" );
    return 0;
}
