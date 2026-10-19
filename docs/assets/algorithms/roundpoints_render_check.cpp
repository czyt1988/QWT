// Render check for the RoundPoints report (docs/tech-report/point-mapper-rounding.md).
//
// Renders the same smooth curve four times with different vertical slopes
// (pixel rows per pixel column) and with the global pixel-alignment switch
// both on and off. The vertical slope decides how visible the snapping is:
// on a 45 degree flank a snapped vertex costs at most half a pixel, on a
// shallow flank (0.2 rows per column) the polyline can only move in whole
// pixel steps, which stretches every step into a 5 column tread.
//
// The output PNGs are analyzed by roundpoints_render_metrics.py:
//
//   <outdir>/slope<v>__round.png    QwtPainter::setRoundingAlignment(true)
//   <outdir>/slope<v>__noround.png  QwtPainter::setRoundingAlignment(false)
//
// Build (MSVC 2019 x64, Qt 6.7.3, qwt built in <repo>/build):
//
//   cl /nologo /O2 /EHsc /std:c++17 /Zc:__cplusplus /MD /permissive- ^
//      /I<repo>/src /I<repo>/src/core /I<repo>/src/plot ^
//      /I<qt>/include /I<qt>/include/QtCore /I<qt>/include/QtGui ^
//      /I<qt>/include/QtWidgets /I<qt>/include/QtConcurrent ^
//      /I<qt>/include/QtPrintSupport ^
//      roundpoints_render_check.cpp /Fe:roundpoints_render_check.exe ^
//      /link /LIBPATH:<repo>/build/lib/Release /LIBPATH:<qt>/lib ^
//      qwtplot.lib qwtcore.lib Qt6Widgets.lib Qt6Gui.lib Qt6Core.lib ^
//      Qt6PrintSupport.lib Qt6Concurrent.lib opengl32.lib
//
// Usage: roundpoints_render_check.exe <outdir>   (always offscreen)

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QFrame>
#include <QPolygonF>
#include <QVector>

#include <qwt_axis.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_painter.h>

#include <cmath>
#include <cstdio>

namespace
{

const int kCanvasWidth  = 900;
const int kCanvasHeight = 600;

// One curve, `samplesPerColumn` samples per pixel column, rising with the
// given slope (pixel rows per pixel column) in the central part of the
// canvas. The curve is smooth in the mathematical sense: its second
// derivative is tiny, so any visible step comes from vertex quantization
// and not from the sampling.
QVector< QPointF > makeCurve(double slope, double samplesPerColumn)
{
    const int columns = kCanvasWidth;
    const int count   = int(columns * samplesPerColumn);

    QVector< QPointF > points;
    points.reserve(count);

    // data coordinates: x in [0, columns], y in [0, columns]
    for (int i = 0; i < count; i++) {
        const double x = double(i) / samplesPerColumn;
        const double y = 0.15 * columns + slope * x
            + 0.00002 * x * x;  // very gentle curvature
        points.append(QPointF(x, y));
    }
    return points;
}

void renderOne(const QString& path, double slope, double samplesPerColumn, bool align)
{
    QwtPainter::setRoundingAlignment(align);

    QwtPlot plot;
    plot.setCanvasBackground(Qt::white);
    plot.resize(kCanvasWidth, kCanvasHeight);

    // fixed axes, 1 data unit = 1 pixel in both directions, so the requested
    // slope is exactly the on-screen slope
    plot.setAxisScale(QwtAxis::XBottom, 0.0, kCanvasWidth);
    plot.setAxisScale(QwtAxis::YLeft, kCanvasHeight, 0.0);

    QwtPlotCurve* curve = new QwtPlotCurve();
    curve->setPen(QPen(QColor(32, 48, 192), 2.0));
    curve->setRenderHint(QwtPlotCurve::RenderAntialiased, true);
    curve->setPaintAttribute(QwtPlotCurve::ClipPolygons, true);
    curve->setPaintAttribute(QwtPlotCurve::FilterPointsLTTB, false);  // no reduction: isolate rounding
    curve->setSamples(makeCurve(slope, samplesPerColumn));
    curve->attach(&plot);

    // hide everything but the canvas content: the analysis scans curve pixels
    plot.setAxisVisible(QwtAxis::XBottom, false);
    plot.setAxisVisible(QwtAxis::YLeft, false);
    plot.setAxisVisible(QwtAxis::XTop, false);
    plot.setAxisVisible(QwtAxis::YRight, false);
    if (QwtPlotCanvas* canvas = dynamic_cast< QwtPlotCanvas* >(plot.canvas()))
        canvas->setFrameStyle(QFrame::NoFrame);
    plot.setContentsMargins(0, 0, 0, 0);

    QImage image(kCanvasWidth, kCanvasHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    QPainter painter(&image);
    plot.render(&painter);
    painter.end();

    image.save(path);
    std::printf("%s slope=%.2f samples/col=%.0f align=%d\n",
                qPrintable(path), slope, samplesPerColumn, align ? 1 : 0);
}

}  // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QString outDir = argc > 1 ? QString::fromLocal8Bit(argv[ 1 ]) : QStringLiteral("out_render");

    // slopes in pixel rows per pixel column: a shallow flank shows long
    // treads when snapped, a steep flank hides them
    static const double slopes[] = { 0.20, 0.35, 1.00, 3.00 };
    const int slopeCount         = int(sizeof(slopes) / sizeof(slopes[0]));

    const double samplesPerColumn = 200.0;

    for (int i = 0; i < slopeCount; i++) {
        const double slope = slopes[ i ];
        const QString tag  = QStringLiteral("slope%1").arg(slope, 0, 'f', 2).replace('.', '_');

        renderOne(outDir + QLatin1Char('/') + tag + QStringLiteral("__round.png"), slope, samplesPerColumn, true);
        renderOne(outDir + QLatin1Char('/') + tag + QStringLiteral("__noround.png"), slope, samplesPerColumn, false);
    }

    QwtPainter::setRoundingAlignment(true);
    return 0;
}
