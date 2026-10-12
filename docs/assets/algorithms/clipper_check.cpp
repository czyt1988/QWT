// Cross-validation harness for QwtClipper (Qwt 7.x).
//
// Reads a case file written by clipper_experiment.py, runs the real
// QwtClipper implementation from qwtcore.dll and prints every result
// with full round-trip precision so the Python re-implementation can be
// compared bit-exactly. No timing is performed here by design.
//
// Case file grammar (one case per line, all numbers decimal):
//   P                                     -> emit Qt semantics probes
//   K <F|I|R> <close> <rx> <ry> <rw> <rh> <n> <x0> <y0> ... <xn-1> <yn-1>
//       F : QPolygonF  vs QRectF  (QwtClipper::clipPolygonF)
//       I : QPolygon   vs QRect   (QwtClipper::clipPolygon)
//       R : QPolygon   vs QRectF  (QwtClipper::clipPolygon float-rect overload)
//   C <rx> <ry> <rw> <rh> <cx> <cy> <r>   -> QwtClipper::clipCircle
//
// Output grammar (stdout):
//   PROBE <name> <0|1>
//   R <caseIndex> <m> <v0> <v1> ...        (%.17g for F, %d for I/R)
//   C <caseIndex> <k> <min0> <max0> ...    (%.17g)
//
// Build (MSVC 2019 x64, Qt 5.15.16, prebuilt qwtcore in build-release):
//   see build_clipper_check.cmd; equivalent to
//   cl /nologo /O2 /EHsc /std:c++17 /MD /DQWTCORE_DLL
//      /I<repo>/src /I<repo>/src/core /I<qt>/include /I<qt>/include/QtCore
//      /I<qt>/include/QtGui
//      clipper_check.cpp /Fe:clipper_check.exe
//      /link /LIBPATH:<repo>/build-release/lib /LIBPATH:<qt>/lib
//            qwtcore.lib Qt5Core.lib Qt5Gui.lib
//   run with PATH containing <qt>/bin and <repo>/build-release/bin

#include <QtCore/QRect>
#include <QtCore/QRectF>
#include <QtCore/QPointF>
#include <QtCore/QVector>
#include <QtGui/QPolygon>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "qwt_clipper.h"
#include "qwt_interval.h"

static void emitDouble( double v )
{
    if ( v != v )
        std::printf( " nan" );   // normalize NaN spelling for the parser
    else
        std::printf( " %.17g", v );
}

// MSVC's istream >> double does not reliably parse "nan"; use strtod,
// which is required to (C99/C++11).
static double nextDouble( std::istringstream& ss )
{
    std::string tok;
    ss >> tok;
    return std::strtod( tok.c_str(), nullptr );
}

int main( int argc, char* argv[] )
{
    if ( argc < 2 ) {
        std::fprintf( stderr, "usage: clipper_check <casefile>\n" );
        return 2;
    }

    std::ifstream in( argv[ 1 ] );
    if ( !in ) {
        std::fprintf( stderr, "cannot open %s\n", argv[ 1 ] );
        return 2;
    }

    std::string line;
    long caseIndex = 0;

    while ( std::getline( in, line ) ) {
        if ( line.empty() )
            continue;

        std::istringstream ss( line );
        char kind;
        ss >> kind;

        if ( kind == 'P' ) {
            // Semantics probes for QRectF::contains, used to align the
            // Python re-implementation with the real Qt behavior.
            const QRectF r( 0.0, 0.0, 10.0, 10.0 );
            std::printf( "PROBE point_left %d\n", r.contains( QPointF( 0.0, 5.0 ) ) );
            std::printf( "PROBE point_top %d\n", r.contains( QPointF( 5.0, 0.0 ) ) );
            std::printf( "PROBE point_right %d\n", r.contains( QPointF( 10.0, 5.0 ) ) );
            std::printf( "PROBE point_bottom %d\n", r.contains( QPointF( 5.0, 10.0 ) ) );
            std::printf( "PROBE point_inside %d\n", r.contains( QPointF( 5.0, 5.0 ) ) );
            std::printf( "PROBE point_outside %d\n", r.contains( QPointF( 10.1, 5.0 ) ) );
            std::printf( "PROBE rect_equal %d\n", r.contains( QRectF( 0.0, 0.0, 10.0, 10.0 ) ) );
            std::printf( "PROBE rect_edge_touch %d\n",
                r.contains( QRectF( 10.0, 0.0, 5.0, 5.0 ) ) == false ? 1 : 0 );
            std::printf( "PROBE rect_bigger %d\n", r.contains( QRectF( -1.0, -1.0, 12.0, 12.0 ) ) );
            // QRect (int) semantics used by the clipper bounds
            const QRect ri( 0, 0, 10, 10 );
            std::printf( "PROBE qrect_right %d\n", ri.right() );
            std::printf( "PROBE qrect_bottom %d\n", ri.bottom() );
            // degenerate contains() used by clipCircle radius-0 fallback
            std::printf( "PROBE contains_degenerate_rect %d\n",
                QRectF( 0.0, 0.0, 100.0, 100.0 ).contains( QRectF( 50.0, 50.0, 0.0, 0.0 ) ) );
            {
                const QRectF m( 0.0, 0.0, 100.0, 100.0 );
                std::printf( "PROBE contains_r_10 %d\n", m.contains( QRectF( 50.0, 50.0, 10.0, 10.0 ) ) );
                std::printf( "PROBE contains_r_w0 %d\n", m.contains( QRectF( 50.0, 50.0, 0.0, 10.0 ) ) );
                std::printf( "PROBE contains_r_h0 %d\n", m.contains( QRectF( 50.0, 50.0, 10.0, 0.0 ) ) );
                std::printf( "PROBE contains_r_00 %d\n", m.contains( QRectF( 50.0, 50.0, 0.0, 0.0 ) ) );
                std::printf( "PROBE contains_r_origin00 %d\n", m.contains( QRectF( 0.0, 0.0, 0.0, 0.0 ) ) );
                std::printf( "PROBE contains_r_edge00 %d\n", m.contains( QRectF( 100.0, 100.0, 0.0, 0.0 ) ) );
                std::printf( "PROBE contains_r_neg %d\n", m.contains( QRectF( 50.0, 50.0, -10.0, -10.0 ) ) );
                std::printf( "PROBE empty_self %d\n", QRectF( 50.0, 50.0, 0.0, 0.0 ).contains( QRectF( 50.0, 50.0, 0.0, 0.0 ) ) );
                std::printf( "PROBE qrectf_right %g\n", QRectF( 50.0, 50.0, 0.0, 0.0 ).right() );
                std::printf( "PROBE qrectf_isempty %d\n", QRectF( 50.0, 50.0, 0.0, 0.0 ).isEmpty() );
                // discriminating probes for the exact contains(QRectF) rule
                std::printf( "PROBE contains_r_negw %d\n", m.contains( QRectF( 50.0, 50.0, -10.0, 10.0 ) ) );
                std::printf( "PROBE contains_r_negh %d\n", m.contains( QRectF( 50.0, 50.0, 10.0, -10.0 ) ) );
                std::printf( "PROBE contains_r_far_negw %d\n", m.contains( QRectF( 150.0, 50.0, -100.0, 10.0 ) ) );
                std::printf( "PROBE contains_r_w0_negh %d\n", m.contains( QRectF( 50.0, 50.0, 0.0, -10.0 ) ) );
                std::printf( "PROBE contains_r_tiny %d\n", m.contains( QRectF( 50.0, 50.0, 1e-300, 10.0 ) ) );
                std::printf( "PROBE contains_r_w1e13 %d\n", m.contains( QRectF( 50.0, 50.0, 1e-13, 10.0 ) ) );
                std::printf( "PROBE contains_r_w1e11 %d\n", m.contains( QRectF( 50.0, 50.0, 1e-11, 10.0 ) ) );
                std::printf( "PROBE contains_r_overright %d\n", m.contains( QRectF( 50.0, 50.0, 60.0, 10.0 ) ) );
            }
            QRectF cRect( 0, 0, 0.0, 0.0 );
            cRect.moveCenter( QPointF( 50.0, 50.0 ) );
            std::printf( "PROBE movecenter_x %.17g\n", cRect.x() );
            std::printf( "PROBE movecenter_y %.17g\n", cRect.y() );
            // NaN parsing of the case file format
            {
                std::istringstream t( "nan" );
                double v = 12345.0;
                t >> v;
                std::printf( "PROBE nan_parse_isnan %d\n", ( v != v ) ? 1 : 0 );
            }
            continue;
        }

        if ( kind == 'K' ) {
            char mode;
            int close;
            ss >> mode >> close;
            const double rx = nextDouble( ss );
            const double ry = nextDouble( ss );
            const double rw = nextDouble( ss );
            const double rh = nextDouble( ss );
            int n;
            ss >> n;

            std::vector< double > x( n ), y( n );
            for ( int i = 0; i < n; i++ ) {
                x[ i ] = nextDouble( ss );
                y[ i ] = nextDouble( ss );
            }

            if ( mode == 'F' ) {
                const QRectF rect( rx, ry, rw, rh );
                QPolygonF poly;
                poly.reserve( n );
                for ( int i = 0; i < n; i++ )
                    poly += QPointF( x[ i ], y[ i ] );

                QwtClipper::clipPolygonF( rect, poly, close != 0 );

                std::printf( "R %ld %d", caseIndex, poly.size() );
                for ( int i = 0; i < poly.size(); i++ ) {
                    emitDouble( poly[ i ].x() );
                    emitDouble( poly[ i ].y() );
                }
                std::printf( "\n" );
            } else if ( mode == 'I' ) {
                const QRect rect = QRect( int( rx ), int( ry ), int( rw ), int( rh ) );
                QPolygon poly;
                poly.reserve( n );
                for ( int i = 0; i < n; i++ )
                    poly += QPoint( int( x[ i ] ), int( y[ i ] ) );

                QwtClipper::clipPolygon( rect, poly, close != 0 );

                std::printf( "R %ld %d", caseIndex, poly.size() );
                for ( int i = 0; i < poly.size(); i++ )
                    std::printf( " %d %d", poly[ i ].x(), poly[ i ].y() );
                std::printf( "\n" );
            } else { // 'R' : float rect, integer polygon
                const QRectF rect( rx, ry, rw, rh );
                QPolygon poly;
                poly.reserve( n );
                for ( int i = 0; i < n; i++ )
                    poly += QPoint( int( x[ i ] ), int( y[ i ] ) );

                QwtClipper::clipPolygon( rect, poly, close != 0 );

                std::printf( "R %ld %d", caseIndex, poly.size() );
                for ( int i = 0; i < poly.size(); i++ )
                    std::printf( " %d %d", poly[ i ].x(), poly[ i ].y() );
                std::printf( "\n" );
            }

            caseIndex++;
            continue;
        }

        if ( kind == 'C' ) {
            const double rx = nextDouble( ss );
            const double ry = nextDouble( ss );
            const double rw = nextDouble( ss );
            const double rh = nextDouble( ss );
            const double cx = nextDouble( ss );
            const double cy = nextDouble( ss );
            const double rad = nextDouble( ss );

            const QVector< QwtInterval > intv =
                QwtClipper::clipCircle( QRectF( rx, ry, rw, rh ), QPointF( cx, cy ), rad );

            std::printf( "C %ld %d", caseIndex, intv.size() );
            for ( int i = 0; i < intv.size(); i++ ) {
                emitDouble( intv[ i ].minValue() );
                emitDouble( intv[ i ].maxValue() );
            }
            std::printf( "\n" );

            caseIndex++;
            continue;
        }

        std::fprintf( stderr, "bad line: %s\n", line.c_str() );
        return 3;
    }

    std::printf( "DONE %ld\n", caseIndex );
    return 0;
}
