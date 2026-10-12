// End-to-end frame time benchmark for QWT 2D plot pipelines:
// raster canvas (QwtPlotCanvas) vs OpenGL canvas (QwtPlotOpenGLCanvas)
// vs offscreen render paths (QwtPlotRenderer / QwtPlot::drawItems /
// QwtPointMapper microbenchmark).
//
// Build (Windows, MSVC 2019 x64, Qt 5.15.16, QWT release build in <repo>/build-release):
//   call vcvars64.bat
//   cl /nologo /O2 /EHsc /std:c++17 /MD ^
//     /I<repo>\src /I<repo>\src\core /I<repo>\src\plot ^
//     /I<qt>\include /I<qt>\include\QtCore /I<qt>\include\QtGui ^
//     /I<qt>\include\QtWidgets /I<qt>\include\QtOpenGL ^
//     /I<qt>\include\QtPrintSupport /I<qt>\include\QtConcurrent ^
//     raster_opengl_bench.cpp /Fe:raster_opengl_bench.exe ^
//     /link /LIBPATH:<repo>\build-release\lib /LIBPATH:<qt>\lib ^
//     qwtplot.lib qwtcore.lib Qt5Widgets.lib Qt5Gui.lib Qt5Core.lib ^
//     Qt5OpenGL.lib Qt5PrintSupport.lib Qt5Concurrent.lib opengl32.lib
//
// Run (needs <qt>\bin and <repo>\build-release\bin on PATH; a real desktop
// session for the raster/gl modes):
//   raster_opengl_bench.exe <mode> <points> <curves> <frames> [options]
//
//   mode    raster   QwtPlotCanvas, replot + processEvents per frame
//           gl       QwtPlotOpenGLCanvas (QOpenGLWidget), same loop
//           renderto QwtPlotRenderer::render into a QImage per frame
//                    (layout + scales + items, no window system)
//           items    QwtPlot::drawItems into a QImage per frame
//                    (pure item painting, raster paint engine)
//           mapper   QwtPointMapper::toPolygonF per curve per frame
//                    (transform + downsampling stage only)
//
//   options --w 900 --h 500        target canvas (or image) size
//           --redraw full|direct|window   (raster/gl only)
//                    full   = QwtPlot::replot() every frame
//                    direct = QwtPlotDirectPainter::drawSeries() on curve 0
//                             every frame (partial repaint)
//                    window = sliding x window over the data (span = n/4),
//                             setRawSamples + setAxisScale + replot per frame
//           --filter lttb|none|pixel|aggressive|points   curve paint attributes
//           --cache on|off         canvas BackingStore paint attribute
//           --immediate on|off     canvas ImmediatePaint attribute (raster)
//           --copybs on|off        QwtPlotDirectPainter::CopyBackingStore
//           --msaa 0|1|4           GL surface format samples (library default: 4)
//           --finish on|off        glFinish() at the end of paintGL (gl mode)
//           --nan <pct>            inject NaN samples (percent)
//           --runs 7 --warmup 3    min-of-N protocol parameters
//
// Timing protocol: 3 warmup frames, then <runs> iterations of <frames>
// timed frames each. Per-frame times are pooled (min/median/mean/max) and
// per-iteration averages are reduced to min/median/max. If
// (iter_med - iter_min) / iter_min > 0.30 the whole protocol is repeated
// (up to 2 retries) to handle external load spikes.
//
// Output: exactly one CSV data line on stdout (columns listed in the runner
// script / results file), diagnostics on stderr (prefixed with '#').

#include <QApplication>
#include <QElapsedTimer>
#include <QImage>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLPaintDevice>
#include <QPaintEngine>
#include <QPainter>
#include <QPixmap>
#include <QSurfaceFormat>
#include <QVector>

#include <qwt_axis.h>
#include <qwt_clipper.h>
#include <qwt_painter.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_directpainter.h>
#include <qwt_plot_opengl_canvas.h>
#include <qwt_plot_renderer.h>
#include <qwt_plot_seriesitem.h>
#include <qwt_point_data.h>
#include <qwt_point_mapper.h>
#include <qwt_scale_map.h>
#include <qwt_series_data.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>

namespace
{
struct Options
{
    QString mode = "raster";
    int points = 100000;
    int curves = 1;
    int frames = 30;
    int runs = 7;
    int warmup = 3;
    int w = 900;
    int h = 500;
    QString redraw = "full";    // full | direct | window
    QString filter = "lttb";    // lttb | none | pixel | aggressive | points
    bool cache = true;
    bool immediate = false;
    bool copybs = false;
    int msaa = 4;
    bool finish = false;
    bool aa = false; // QwtPlotItem::RenderAntialiased (library default: off)
    double nanPct = 0.0;
};

Options parseArgs( int argc, char* argv[] )
{
    Options o;
    if ( argc > 1 )
        o.mode = argv[1];
    if ( argc > 2 )
        o.points = std::atoi( argv[2] );
    if ( argc > 3 )
        o.curves = std::atoi( argv[3] );
    if ( argc > 4 )
        o.frames = std::atoi( argv[4] );

    for ( int i = 5; i < argc - 1; ++i )
    {
        const QString a = argv[i];
        const QString v = argv[i + 1];
        if ( a == "--w" ) o.w = v.toInt();
        else if ( a == "--h" ) o.h = v.toInt();
        else if ( a == "--redraw" ) { o.redraw = v; ++i; }
        else if ( a == "--filter" ) { o.filter = v; ++i; }
        else if ( a == "--cache" ) { o.cache = ( v == "on" ); ++i; }
        else if ( a == "--immediate" ) { o.immediate = ( v == "on" ); ++i; }
        else if ( a == "--copybs" ) { o.copybs = ( v == "on" ); ++i; }
        else if ( a == "--msaa" ) { o.msaa = v.toInt(); ++i; }
        else if ( a == "--finish" ) { o.finish = ( v == "on" ); ++i; }
        else if ( a == "--aa" ) { o.aa = ( v == "on" ); ++i; }
        else if ( a == "--nan" ) { o.nanPct = v.toDouble(); ++i; }
        else if ( a == "--runs" ) { o.runs = v.toInt(); ++i; }
        else if ( a == "--warmup" ) { o.warmup = v.toInt(); ++i; }
    }
    return o;
}

const char* engineName( const QPaintEngine* e )
{
    if ( !e )
        return "null";
    switch ( e->type() )
    {
    case QPaintEngine::Raster: return "Raster";
    case QPaintEngine::OpenGL: return "OpenGL";
    case QPaintEngine::OpenGL2: return "OpenGL2";
    case QPaintEngine::Pdf: return "Pdf";
    case QPaintEngine::SVG: return "SVG";
    default: return "Other";
    }
}

void applyFilter( QwtPlotCurve* curve, const QString& filter )
{
    curve->setPaintAttribute( QwtPlotCurve::FilterPointsLTTB, filter == "lttb" );
    curve->setPaintAttribute( QwtPlotCurve::FilterPointsPixel, filter == "pixel" );
    curve->setPaintAttribute( QwtPlotCurve::FilterPointsAggressive, filter == "aggressive" );
    curve->setPaintAttribute( QwtPlotCurve::FilterPoints,
        filter == "points" || filter == "aggressive" );
}

QwtPointMapper::TransformationFlags mapperFlags( const QString& filter )
{
    QwtPointMapper::TransformationFlags f = QwtPointMapper::RoundPoints;
    if ( filter == "lttb" )
        f |= QwtPointMapper::MinMaxReduce;
    else if ( filter == "pixel" )
        f |= QwtPointMapper::PixelColumnReduce;
    else if ( filter == "aggressive" )
        f |= QwtPointMapper::WeedOutIntermediatePoints | QwtPointMapper::WeedOutPoints;
    else if ( filter == "points" )
        f |= QwtPointMapper::WeedOutPoints;
    return f;
}

// QwtPlotOpenGLCanvas with a configurable surface format, optional per-frame
// glFinish and one-shot paint diagnostics (paint engine types, GL strings).
class BenchGLCanvas : public QwtPlotOpenGLCanvas
{
public:
    BenchGLCanvas( const QSurfaceFormat& fmt, QwtPlot* plot, bool finish )
        : QwtPlotOpenGLCanvas( fmt, plot )
        , m_finish( finish )
    {
    }

    // QOpenGLWidget::paintEngine() is protected - expose it for diagnostics
    const char* widgetEngineName() const
    {
        return engineName( paintEngine() );
    }

    int paintCount = 0; // counts paintGL calls (vsync/coalescing diagnostics)

protected:
    void paintGL() override
    {
        ++paintCount;
        QwtPlotOpenGLCanvas::paintGL();

        if ( !m_logged )
        {
            m_logged = true;

            QOpenGLContext* ctx = QOpenGLContext::currentContext();
            if ( ctx )
            {
                QOpenGLFunctions* f = ctx->functions();
                std::fprintf( stderr, "# gl: vendor=%s\n",
                    reinterpret_cast< const char* >( f->glGetString( GL_VENDOR ) ) );
                std::fprintf( stderr, "# gl: renderer=%s\n",
                    reinterpret_cast< const char* >( f->glGetString( GL_RENDERER ) ) );
                std::fprintf( stderr, "# gl: version=%s\n",
                    reinterpret_cast< const char* >( f->glGetString( GL_VERSION ) ) );
                std::fprintf( stderr, "# gl: context format samples=%d swapInterval=%d\n",
                    ctx->format().samples(), ctx->format().swapInterval() );
            }

            // engine used by QPainter directly on the widget (inside paintGL)
            QPainter pw( this );
            std::fprintf( stderr, "# gl: QPainter(widget) inside paintGL engine=%s active=%d\n",
                engineName( pw.paintEngine() ), int( pw.isActive() ) );
            pw.end();

            // engine used by Qwt for the FBO pass (QOpenGLPaintDevice)
            QOpenGLPaintDevice pd( size() );
            QPainter pp( &pd );
            std::fprintf( stderr, "# gl: QPainter(QOpenGLPaintDevice) engine=%s\n",
                engineName( pp.paintEngine() ) );
            pp.end();

            std::fprintf( stderr, "# gl: widget->paintEngine() inside-paintGL=%s\n",
                engineName( paintEngine() ) );
        }

        if ( m_finish )
        {
            QOpenGLContext* ctx = QOpenGLContext::currentContext();
            if ( ctx )
                ctx->functions()->glFinish();
        }
    }

private:
    bool m_finish;
    bool m_logged = false;
};

// QwtPlot with an instrumented drawCanvas (the item painting stage runs
// through this virtual on both the raster and the GL canvas path).
// On the first call it additionally probes the curve pipeline stages
// (transform -> clip -> polyline rasterization) on the real painter.
class BenchPlot : public QwtPlot
{
public:
    qint64 drawCanvasNs = 0;
    int drawCanvasCount = 0;

    // probe setup (may stay null)
    const QwtSeriesData< QPointF >* probeSeries = nullptr;
    int probeN = 0;
    QString probeFilter;

    void drawCanvas( QPainter* painter ) override
    {
        QElapsedTimer t;
        t.start();
        QwtPlot::drawCanvas( painter );
        drawCanvasNs += t.nsecsElapsed();
        ++drawCanvasCount;

        if ( !m_probed && probeSeries && probeN > 0 && canvas() )
        {
            m_probed = true;

            const QwtScaleMap xMap = canvasMap( QwtAxis::XBottom );
            const QwtScaleMap yMap = canvasMap( QwtAxis::YLeft );
            const QRectF canvasRect = canvas()->contentsRect();

            QwtPointMapper mapper;
            mapper.setFlags( mapperFlags( probeFilter ) );
            mapper.setBoundingRect( canvasRect );

            t.restart();
            QPolygonF poly = mapper.toPolygonF( xMap, yMap, probeSeries, 0, probeN - 1 );
            const double mapMs = t.nsecsElapsed() * 1e-6;

            QRectF clipRect = canvasRect.adjusted( -1.0, -1.0, 1.0, 1.0 );
            t.restart();
            QwtClipper::clipPolygonF( clipRect, poly, false );
            const double clipMs = t.nsecsElapsed() * 1e-6;
            const int clippedSize = poly.size();

            painter->save();
            painter->setPen( QPen( QColor( 30, 90, 200 ), 1 ) );
            painter->setRenderHint( QPainter::Antialiasing, false );
            t.restart();
            QwtPainter::drawPolyline( painter, poly );
            const double drawMs = t.nsecsElapsed() * 1e-6;

            t.restart();
            painter->drawPolyline( poly );
            const double rawMs = t.nsecsElapsed() * 1e-6;
            painter->restore();

            // same polygon on fresh QImage targets, one painter reused across
            // draws: first draw pays the raster engine per-instance warmup
            // (span buffer growth), later draws show the steady-state cost
            double imgFirst = 0.0;
            double imgBest = 1e9;
            double rgbBest = 1e9;
            QImage probeImg( 1000, 600, QImage::Format_ARGB32_Premultiplied );
            probeImg.fill( Qt::white );
            QImage probeRgb( 1000, 600, QImage::Format_RGB32 );
            probeRgb.fill( Qt::white );
            {
                QPainter p( &probeImg );
                p.setPen( QPen( QColor( 30, 90, 200 ), 1 ) );
                p.setRenderHint( QPainter::Antialiasing, false );
                for ( int r = 0; r < 5; ++r )
                {
                    t.restart();
                    p.drawPolyline( poly );
                    const double ms = t.nsecsElapsed() * 1e-6;
                    if ( r == 0 )
                        imgFirst = ms;
                    imgBest = std::min( imgBest, ms );
                }
                p.end();

                QPainter p2( &probeRgb );
                p2.setPen( QPen( QColor( 30, 90, 200 ), 1 ) );
                p2.setRenderHint( QPainter::Antialiasing, false );
                for ( int r = 0; r < 5; ++r )
                {
                    t.restart();
                    p2.drawPolyline( poly );
                    rgbBest = std::min( rgbBest, t.nsecsElapsed() * 1e-6 );
                }
                p2.end();
            }

            std::fprintf( stderr,
                "# probe-canvas: engine=%s map=%.3f clip=%.3f qwtDraw=%.3f rawDraw=%.3f "
                "imgARGB first=%.3f best=%.3f imgRGB32=%.3f ms (size=%d rect=%gx%g)\n",
                engineName( painter->paintEngine() ), mapMs, clipMs, drawMs, rawMs,
                imgFirst, imgBest, rgbBest,
                clippedSize, canvasRect.width(), canvasRect.height() );
        }
    }

private:
    bool m_probed = false;
};

// Filter logging the paint engine a QPainter gets on the raster canvas (once)
// and counting the paint events actually delivered (vsync/coalescing check).
class EngineLogger : public QObject
{
public:
    bool eventFilter( QObject* obj, QEvent* ev ) override
    {
        if ( ev->type() == QEvent::Paint )
        {
            ++paintCount;
            if ( !m_done )
            {
                m_done = true;
                QWidget* w = qobject_cast< QWidget* >( obj );
                QPainter p( w );
                std::fprintf( stderr, "# raster: QPainter(canvas) inside paint event engine=%s active=%d\n",
                    engineName( p.paintEngine() ), int( p.isActive() ) );
            }
        }
        return QObject::eventFilter( obj, ev );
    }

    int paintCount = 0;

private:
    bool m_done = false;
};

double median( std::vector< double > v )
{
    if ( v.empty() )
        return 0.0;
    std::sort( v.begin(), v.end() );
    const size_t n = v.size();
    return ( n % 2 ) ? v[n / 2] : 0.5 * ( v[n / 2 - 1] + v[n / 2] );
}
}

int main( int argc, char* argv[] )
{
    // No vsync: GL frame times should reflect real cost, not the refresh rate.
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setSwapInterval( 0 );
    QSurfaceFormat::setDefaultFormat( fmt );

    QApplication app( argc, argv );

    const Options o = parseArgs( argc, argv );
    const bool widgetMode = ( o.mode == "raster" || o.mode == "gl" );

    std::fprintf( stderr, "# bench: mode=%s points=%d curves=%d frames=%d runs=%d w=%d h=%d "
        "redraw=%s filter=%s cache=%d immediate=%d copybs=%d msaa=%d finish=%d nan=%.1f\n",
        qPrintable( o.mode ), o.points, o.curves, o.frames, o.runs, o.w, o.h,
        qPrintable( o.redraw ), qPrintable( o.filter ), int( o.cache ), int( o.immediate ),
        int( o.copybs ), o.msaa, int( o.finish ), o.nanPct );
    std::fprintf( stderr, "# qt: %s\n", QT_VERSION_STR );

    // --------------------------------------------------------------- data
    const int n = o.points;
    const int k = o.curves;
    QVector< double > xs;
    QVector< double > ys; // k * n, curve c occupies [c*n, (c+1)*n)
    if ( k > 0 && n > 0 )
    {
        if ( qint64( k ) * n > INT_MAX )
        {
            std::fprintf( stderr, "# ERROR: sample count exceeds INT_MAX\n" );
            return 2;
        }
        try
        {
            xs.resize( n );
            ys.resize( k * n );
        }
        catch ( const std::bad_alloc& )
        {
            std::fprintf( stderr, "# ERROR: allocation failed\n" );
            return 2;
        }

        for ( int i = 0; i < n; ++i )
            xs[i] = i;
        for ( int c = 0; c < k; ++c )
        {
            double* y = ys.data() + qint64( c ) * n;
            for ( int i = 0; i < n; ++i )
            {
                double v = std::sin( 0.01 * i + 0.7 * c ) + 0.5 * std::sin( 0.13 * i + 0.37 * c );
                if ( o.nanPct > 0.0 && ( i % 1000 ) < int( o.nanPct * 10.0 ) )
                    v = std::nan( "" );
                y[i] = v;
            }
        }
    }

    // ---------------------------------------------------------------- plot
    BenchPlot plot;
    plot.setAutoReplot( false );

    QwtPlotDirectPainter* directPainter = nullptr;
    EngineLogger engineLogger;
    BenchGLCanvas* glCanvas = nullptr;

    if ( o.mode == "raster" )
    {
        QwtPlotCanvas* canvas = new QwtPlotCanvas( &plot );
        canvas->setFrameStyle( QFrame::NoFrame );
        canvas->setPalette( Qt::white );
        canvas->setPaintAttribute( QwtPlotCanvas::BackingStore, o.cache );
        canvas->setPaintAttribute( QwtPlotCanvas::ImmediatePaint, o.immediate );
        plot.setCanvas( canvas );
        canvas->installEventFilter( &engineLogger );
    }
    else if ( o.mode == "gl" )
    {
        QSurfaceFormat glFmt = QSurfaceFormat::defaultFormat();
        glFmt.setSwapInterval( 0 );
        glFmt.setSamples( o.msaa );
        glCanvas = new BenchGLCanvas( glFmt, &plot, o.finish );
        glCanvas->setFrameStyle( QFrame::NoFrame );
        glCanvas->setPaintAttribute( QwtPlotAbstractGLCanvas::BackingStore, o.cache );
        plot.setCanvas( glCanvas );
        std::fprintf( stderr, "# gl: widget format request samples=%d swapInterval=%d\n",
            glCanvas->format().samples(), glCanvas->format().swapInterval() );
    }

    QVector< QwtPlotCurve* > curveList;
    QwtCPointerData< double >* probeData = nullptr;
    for ( int c = 0; c < k; ++c )
    {
        QwtPlotCurve* curve = new QwtPlotCurve( QString( "c%1" ).arg( c ) );
        curve->setPen( QColor( 30, 90, 200 ), 1 );
        curve->setRenderHint( QwtPlotItem::RenderAntialiased, o.aa );
        applyFilter( curve, o.filter );
        if ( k > 0 && n > 0 )
            curve->setRawSamples( xs.constData(), ys.constData() + qint64( c ) * n, n );
        curve->attach( &plot );
        curveList.append( curve );

        if ( c == 0 && widgetMode && n > 0 )
        {
            probeData = new QwtCPointerData< double >( xs.constData(), ys.constData(), size_t( n ) );
            plot.probeSeries = probeData;
            plot.probeN = n;
            plot.probeFilter = o.filter;
        }
    }

    // ------------------------------------------------------- canvas sizing
    int canvasW = o.w;
    int canvasH = o.h;
    if ( !widgetMode )
        plot.resize( o.w + 120, o.h + 120 ); // QwtPlotRenderer needs a valid widget size
    else
    {
        plot.resize( o.w + 120, o.h + 120 );
        plot.show();
        app.processEvents();

        // calibrate: make the canvas (not the plot widget) exactly w x h
        const QSize delta( plot.width() - plot.canvas()->width(),
            plot.height() - plot.canvas()->height() );
        plot.resize( o.w + delta.width(), o.h + delta.height() );
        app.processEvents();

        canvasW = plot.canvas()->width();
        canvasH = plot.canvas()->height();
        std::fprintf( stderr, "# canvas: actual=%dx%d (target %dx%d)\n",
            canvasW, canvasH, o.w, o.h );
    }

    plot.replot();
    app.processEvents();

    if ( o.mode == "gl" )
    {
        // what a QPainter gets when opened on the GL canvas outside of a paint
        // event - this is what QwtPlotDirectPainter does on its incremental path
        QPainter p( plot.canvas() );
        std::fprintf( stderr, "# gl: QPainter(canvas) outside paint engine=%s active=%d\n",
            engineName( p.paintEngine() ), int( p.isActive() ) );
        p.end();
    }

    if ( o.redraw == "direct" && widgetMode )
    {
        directPainter = new QwtPlotDirectPainter( &plot );
        directPainter->setAttribute( QwtPlotDirectPainter::CopyBackingStore, o.copybs );
    }

    // ------------------------------------------- offscreen helpers (items)
    QwtScaleMap maps[QwtAxis::AxisPositions];
    if ( o.mode == "items" || o.mode == "mapper" )
    {
        // build maps like QwtPlotRenderer does: data interval from the scale
        // divisions, paint interval = image coordinates
        const QwtScaleDiv xDiv = plot.axisScaleDiv( QwtAxis::XBottom );
        const QwtScaleDiv yDiv = plot.axisScaleDiv( QwtAxis::YLeft );
        maps[QwtAxis::XBottom].setScaleInterval( xDiv.lowerBound(), xDiv.upperBound() );
        maps[QwtAxis::XBottom].setPaintInterval( 0, canvasW - 1 );
        maps[QwtAxis::YLeft].setScaleInterval( yDiv.lowerBound(), yDiv.upperBound() );
        maps[QwtAxis::YLeft].setPaintInterval( canvasH - 1, 0 );
    }

    QImage image;
    if ( o.mode == "renderto" || o.mode == "items" )
    {
        image = QImage( canvasW, canvasH, QImage::Format_ARGB32_Premultiplied );
        image.fill( Qt::white );
    }

    // one-shot diagnostic: what the mapper produces for curve 0 with the
    // maps this mode will use (compares offscreen vs widget geometry)
    if ( k > 0 && n > 0 )
    {
        QwtScaleMap dbgX, dbgY;
        if ( widgetMode )
        {
            dbgX = plot.canvasMap( QwtAxis::XBottom );
            dbgY = plot.canvasMap( QwtAxis::YLeft );
        }
        else
        {
            dbgX = maps[QwtAxis::XBottom];
            dbgY = maps[QwtAxis::YLeft];
        }

        QwtPointMapper mapper;
        mapper.setFlags( mapperFlags( o.filter ) );
        mapper.setBoundingRect( QRectF( 0, 0, canvasW, canvasH ) );
        const QwtCPointerData< double > series( xs.constData(), ys.constData(), size_t( n ) );
        const QPolygonF poly = mapper.toPolygonF( dbgX, dbgY, &series, 0, n - 1 );
        const QRectF bb = poly.isEmpty() ? QRectF() : poly.boundingRect();

        std::fprintf( stderr, "# dbg: xMap s=[%g,%g] p=[%g,%g] yMap s=[%g,%g] p=[%g,%g]\n",
            dbgX.s1(), dbgX.s2(), dbgX.p1(), dbgX.p2(),
            dbgY.s1(), dbgY.s2(), dbgY.p1(), dbgY.p2() );
        std::fprintf( stderr, "# dbg: polygon size=%zu bbox=(%g,%g,%g,%g)\n",
            size_t( poly.size() ), bb.x(), bb.y(), bb.width(), bb.height() );

        // target probe: identical polyline drawn to QImage vs QPixmap,
        // alternating to cancel machine load drift
        if ( o.mode == "items" || o.mode == "raster" )
        {
            QImage probeImg( canvasW, canvasH, QImage::Format_ARGB32_Premultiplied );
            probeImg.fill( Qt::white );
            QPixmap probePix( canvasW, canvasH );
            probePix.fill( Qt::white );
            const QPen probePen( QColor( 30, 90, 200 ), 1 );

            QElapsedTimer pt;
            double imgMs = 0.0;
            double pixMs = 0.0;
            double beginMs = 0.0;
            double drawMs = 0.0;
            double endMs = 0.0;
            const int probeRuns = 10;
            for ( int r = 0; r < probeRuns; ++r )
            {
                double b = 0.0;
                double d = 0.0;
                double e = 0.0;

                pt.restart();
                QPainter p1( &probeImg );
                p1.setPen( probePen );
                b = pt.nsecsElapsed() * 1e-6;

                pt.restart();
                p1.drawPolyline( poly );
                d = pt.nsecsElapsed() * 1e-6;

                pt.restart();
                p1.end();
                e = pt.nsecsElapsed() * 1e-6;

                beginMs += b;
                drawMs += d;
                endMs += e;
                imgMs += b + d + e;

                pt.restart();
                QPainter p2( &probePix );
                p2.setPen( probePen );
                p2.drawPolyline( poly );
                p2.end();
                pixMs += pt.nsecsElapsed() * 1e-6;
            }
            std::fprintf( stderr, "# dbg: probe drawPolyline QImage=%.3f ms (begin=%.3f draw=%.3f end=%.3f) QPixmap=%.3f ms (mean of %d)\n",
                imgMs / probeRuns, beginMs / probeRuns, drawMs / probeRuns, endMs / probeRuns,
                pixMs / probeRuns, probeRuns );

            // draw the same polygon directly onto the canvas backing store
            // pixmap (outside of the paint event) and report its properties:
            // separates "target format" from "paint context" effects
            if ( o.mode == "raster" )
            {
                // mode==raster guarantees the canvas is a QwtPlotCanvas
                QwtPlotCanvas* rc = static_cast< QwtPlotCanvas* >( plot.canvas() );
                const QPixmap* bsp = rc ? rc->backingStore() : nullptr;
                if ( bsp && !bsp->isNull() )
                {
                    QPixmap& bsPix = const_cast< QPixmap& >( *bsp );
                    double bsBest = 1e9;
                    for ( int r = 0; r < 5; ++r )
                    {
                        QPainter p( &bsPix );
                        p.setPen( probePen );
                        p.setRenderHint( QPainter::Antialiasing, false );
                        pt.restart();
                        p.drawPolyline( poly );
                        bsBest = std::min( bsBest, pt.nsecsElapsed() * 1e-6 );
                        p.end();
                    }
                    const QImage bsInfo = bsPix.toImage();
                    std::fprintf( stderr,
                        "# dbg: backingStore draw best=%.3f ms depth=%d alpha=%d format=%d size=%dx%d "
                        "(probePix depth=%d alpha=%d, probeImg format=%d)\n",
                        bsBest, bsPix.depth(), int( bsPix.hasAlphaChannel() ), int( bsInfo.format() ),
                        bsPix.width(), bsPix.height(),
                        probePix.depth(), int( probePix.hasAlphaChannel() ), int( probeImg.format() ) );
                }
            }
        }
    }

    // ------------------------------------------------------- frame action
    int winOffset = 0;
    const int span = std::max( 2, n / 4 );
    const int step = std::max( 1, span / std::max( 1, o.frames ) );

    // stage accumulators (ns): [0] = data/update, [1] = draw, [2] = event pump
    qint64 stage[3] = { 0, 0, 0 };
    QElapsedTimer st;

    const auto frameAction = [&]( QElapsedTimer& t )
    {
        t.restart();

        if ( o.mode == "renderto" )
        {
            st.restart();
            QPainter painter( &image );
            QwtPlotRenderer renderer;
            renderer.render( &plot, &painter, QRectF( 0, 0, canvasW, canvasH ) );
            painter.end();
            stage[1] += st.nsecsElapsed();
        }
        else if ( o.mode == "items" )
        {
            st.restart();
            image.fill( Qt::white );
            stage[0] += st.nsecsElapsed();

            st.restart();
            QPainter painter( &image );
            plot.drawItems( &painter, QRectF( 0, 0, canvasW, canvasH ), maps );
            painter.end();
            stage[1] += st.nsecsElapsed();
        }
        else if ( o.mode == "mapper" )
        {
            st.restart();
            QwtPointMapper mapper;
            mapper.setFlags( mapperFlags( o.filter ) );
            mapper.setBoundingRect( QRectF( 0, 0, canvasW, canvasH ) );
            for ( int c = 0; c < k; ++c )
            {
                const QwtCPointerData< double > series(
                    xs.constData(), ys.constData() + qint64( c ) * n, size_t( n ) );
                const QPolygonF poly = mapper.toPolygonF(
                    maps[QwtAxis::XBottom], maps[QwtAxis::YLeft], &series, 0, n - 1 );
                if ( poly.isEmpty() && n > 0 )
                    std::fprintf( stderr, "# WARN: empty polygon\n" );
            }
            stage[1] += st.nsecsElapsed();
        }
        else if ( o.redraw == "direct" )
        {
            st.restart();
            if ( curveList.isEmpty() )
                plot.replot();
            else
                directPainter->drawSeries( curveList[0], 0, n - 1 );
            stage[1] += st.nsecsElapsed();

            st.restart();
            app.processEvents();
            stage[2] += st.nsecsElapsed();
        }
        else if ( o.redraw == "window" )
        {
            st.restart();
            winOffset = ( winOffset + step ) % std::max( 1, n - span + 1 );
            for ( int c = 0; c < k; ++c )
            {
                curveList[c]->setRawSamples( xs.constData() + winOffset,
                    ys.constData() + qint64( c ) * n + winOffset, span );
            }
            plot.setAxisScale( QwtAxis::XBottom, winOffset, winOffset + span - 1 );
            stage[0] += st.nsecsElapsed();

            st.restart();
            plot.replot();
            stage[1] += st.nsecsElapsed();

            st.restart();
            app.processEvents();
            stage[2] += st.nsecsElapsed();
        }
        else // full replot
        {
            st.restart();
            plot.replot();
            stage[1] += st.nsecsElapsed();

            st.restart();
            app.processEvents();
            stage[2] += st.nsecsElapsed();
        }
    };

    // ----------------------------------------------------------- protocol
    // warmup
    QElapsedTimer t;
    for ( int i = 0; i < o.warmup; ++i )
        frameAction( t );

    std::vector< double > iterAvg;
    std::vector< double > frameMs;
    int retries = 0;

    for ( ;; )
    {
        iterAvg.clear();
        frameMs.clear();
        frameMs.reserve( size_t( o.runs ) * o.frames );

        for ( int r = 0; r < o.runs; ++r )
        {
            double sum = 0.0;
            for ( int f = 0; f < o.frames; ++f )
            {
                frameAction( t );
                const double ms = t.nsecsElapsed() * 1e-6;
                frameMs.push_back( ms );
                sum += ms;
            }
            iterAvg.push_back( sum / o.frames );
        }

        const double itMin = *std::min_element( iterAvg.begin(), iterAvg.end() );
        const double itMed = median( iterAvg );
        if ( itMin > 0.0 && ( itMed - itMin ) / itMin > 0.30 && retries < 2 )
        {
            ++retries;
            std::fprintf( stderr, "# retry %d: iter_min=%.3f iter_med=%.3f spread>30%%\n",
                retries, itMin, itMed );
            continue;
        }
        break;
    }

    // ------------------------------------------------------ diagnostics
    const double totalFrames = double( ( retries + 1 ) * o.runs * o.frames );
    std::fprintf( stderr, "# stages ms/frame: update=%.4f draw=%.4f events=%.4f\n",
        stage[0] * 1e-6 / totalFrames, stage[1] * 1e-6 / totalFrames, stage[2] * 1e-6 / totalFrames );
    if ( o.mode == "raster" )
    {
        std::fprintf( stderr, "# paints: canvas paintEvents=%d (warmup %d + measured %.0f frames)\n",
            engineLogger.paintCount, o.warmup, totalFrames );
    }
    else if ( o.mode == "gl" && glCanvas )
    {
        std::fprintf( stderr, "# paints: paintGL calls=%d (warmup %d + measured %.0f frames)\n",
            glCanvas->paintCount, o.warmup, totalFrames );
    }
    if ( widgetMode )
    {
        std::fprintf( stderr, "# stages: plot->drawCanvas calls=%d ms/call=%.4f\n",
            plot.drawCanvasCount,
            plot.drawCanvasCount ? plot.drawCanvasNs * 1e-6 / plot.drawCanvasCount : 0.0 );
    }

    const double itMin = *std::min_element( iterAvg.begin(), iterAvg.end() );
    const double itMed = median( iterAvg );
    const double itMax = *std::max_element( iterAvg.begin(), iterAvg.end() );
    const double fMin = *std::min_element( frameMs.begin(), frameMs.end() );
    const double fMed = median( frameMs );
    double fMean = 0.0;
    for ( double v : frameMs )
        fMean += v;
    fMean /= double( frameMs.size() );
    const double fMax = *std::max_element( frameMs.begin(), frameMs.end() );

    // in-situ item painting stage (widget modes only)
    const double dcMs = plot.drawCanvasCount
        ? plot.drawCanvasNs * 1e-6 / plot.drawCanvasCount : 0.0;
    const int paints = ( o.mode == "raster" ) ? engineLogger.paintCount
        : ( o.mode == "gl" && glCanvas ? glCanvas->paintCount : 0 );

    // mode,redraw,points,curves,frames,runs,canvas_w,canvas_h,filter,cache,
    // msaa,immediate,copybs,finish,aa,nan_pct,retries,iter_min_ms,iter_med_ms,
    // iter_max_ms,frame_min_ms,frame_med_ms,frame_mean_ms,frame_max_ms,
    // drawcanvas_ms,drawcanvas_calls,paints
    std::printf( "%s,%s,%d,%d,%d,%d,%d,%d,%s,%d,%d,%d,%d,%d,%d,%.1f,%d,"
        "%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%d,%d\n",
        qPrintable( o.mode ), qPrintable( o.redraw ), o.points, o.curves, o.frames, o.runs,
        canvasW, canvasH, qPrintable( o.filter ), int( o.cache ), o.msaa, int( o.immediate ),
        int( o.copybs ), int( o.finish ), int( o.aa ), o.nanPct, retries,
        itMin, itMed, itMax, fMin, fMed, fMean, fMax,
        dcMs, plot.drawCanvasCount, paints );

    return 0;
}
