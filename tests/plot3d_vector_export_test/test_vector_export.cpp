/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "test_vector_export.h"

#include <qwt3d_color.h>
#include <qwt3d_coordsys.h>
#include <qwt3d_plot.h>
#include <qwt3d_surface.h>

#include <QFile>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QThread>

#include <cmath>
#include <vector>

namespace
{

/// Paints every vertex with the same color, so the expected export is known
class FlatColor : public Qwt3DColor
{
public:
    RGBA operator()(double, double, double) const override { return RGBA(1.0, 0.0, 0.0, 1.0); }
};

struct SvgStats
{
    int onPage      = 0;
    int dataColor   = 0;
    int transparent = 0;

    // area weighted share of the data color: labels and other pixmap derived
    // primitives are tiny and numerous, counting them by number would drown
    // the surface
    double totalArea     = 0.0;
    double dataColorArea = 0.0;

    double dataColorAreaShare() const
    {
        return totalArea > 0.0 ? dataColorArea / totalArea : 0.0;
    }
};

SvgStats analyzeSvg(const QString& path)
{
    SvgStats stats;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return stats;

    const QString text = QString::fromUtf8(file.readAll());

    double pageW = 0, pageH = 0;
    const auto viewBox = QRegularExpression(QStringLiteral("viewBox=.([-0-9. ]+).")).match(text);
    if (viewBox.hasMatch()) {
        const QStringList parts = viewBox.captured(1).split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (parts.size() == 4) {
            pageW = parts[ 2 ].toDouble();
            pageH = parts[ 3 ].toDouble();
        }
    }

    const QRegularExpression polyRe(QStringLiteral("<polygon([^>]*)/?>"));
    const QRegularExpression fillRe(QStringLiteral("fill=.#([0-9a-fA-F]{6})."));
    const QRegularExpression opacityRe(QStringLiteral("fill-opacity=.([0-9.eE+-]+)."));
    const QRegularExpression pointsRe(QStringLiteral("points=.([^\x22]*)"));
    const QRegularExpression pointRe(QStringLiteral("(-?[0-9.]+),(-?[0-9.]+)"));

    auto it = polyRe.globalMatch(text);
    while (it.hasNext()) {
        const QString attrs = it.next().captured(1);

        const auto fillMatch    = fillRe.match(attrs);
        const auto pointsMatch  = pointsRe.match(attrs);
        const auto opacityMatch = opacityRe.match(attrs);
        if (!fillMatch.hasMatch() || !pointsMatch.hasMatch())
            continue;

        if (opacityMatch.hasMatch() && opacityMatch.captured(1).toDouble() <= 0.0)
            ++stats.transparent;

        QVector< QPointF > points;
        auto pit = pointRe.globalMatch(pointsMatch.captured(1));
        while (pit.hasNext()) {
            const auto pair = pit.next();
            points.append(QPointF(pair.captured(1).toDouble(), pair.captured(2).toDouble()));
        }
        if (points.size() < 3)
            continue;

        // centroid and area (shoelace)
        QPointF centroid;
        double area2 = 0.0;
        for (int k = 0; k < points.size(); ++k) {
            const QPointF& a = points[ k ];
            const QPointF& b = points[ (k + 1) % points.size() ];
            area2 += a.x() * b.y() - b.x() * a.y();
            centroid += a;
        }
        centroid /= double(points.size());

        if (pageW > 0 && pageH > 0
            && (centroid.x() < 0 || centroid.y() < 0 || centroid.x() > pageW || centroid.y() > pageH)) {
            continue;  // off page primitives are not part of the picture
        }

        const double area = qAbs(area2) * 0.5;
        if (pageW > 0 && pageH > 0 && area > 0.5 * pageW * pageH)
            continue;  // the gl2ps background rectangle is not scene content

        ++stats.onPage;
        stats.totalArea += area;

        const QString hex = fillMatch.captured(1);
        const double d = std::sqrt(qPow(hex.mid(0, 2).toInt(nullptr, 16) - 255, 2.0)
                                   + qPow(hex.mid(2, 2).toInt(nullptr, 16), 2.0)
                                   + qPow(hex.mid(4, 2).toInt(nullptr, 16), 2.0));
        if (d <= 40.0) {
            ++stats.dataColor;
            stats.dataColorArea += area;
        }
    }

    return stats;
}

/**
 * @brief Renders a small surface painted by a constant color and exports it
 * @return The SVG stats; onPage == 0 when no GL context was available
 */
SvgStats exportFlatSurface(const QString& dir, bool& glOk, bool minimalScene)
{
    Qwt3DPlot plot;
    plot.resize(320, 240);

    const int cols = 16, rows = 16;
    std::vector< std::vector< double > > grid(rows, std::vector< double >(cols));
    std::vector< double* > rowsPtr(rows);
    for (int j = 0; j < rows; ++j) {
        rowsPtr[ j ] = grid[ j ].data();
        for (int i = 0; i < cols; ++i)
            grid[ j ][ i ] = std::sin(double(i)) * std::cos(double(j));
    }

    FlatColor flatColor;

    auto* surface = new Qwt3DSurface();
    surface->loadFromData(rowsPtr.data(), cols, rows, 0.0, 1.0, 0.0, 1.0);
    surface->setDataColor(&flatColor);
    if (minimalScene) {
        // only the surface itself: no axes/grid, no mesh lines, no legend, so
        // every on page polygon is a data polygon
        surface->setPlotStyle(FILLED);  // the enum is global (qwt3d_types.h)
        plot.coordinates()->setStyle(NOCOORD);
        plot.showColorLegend(false);
    }
    surface->attach(&plot);

    plot.show();

    for (int i = 0; i < 40; ++i) {
        QApplication::processEvents();
        QThread::msleep(10);
    }

    glOk = plot.initializedGL();

    const QString outDir = qEnvironmentVariable("QWT_EXPORT_TEST_DIR");
    const QString svgPath = (outDir.isEmpty() ? dir : outDir) + QStringLiteral("/surface_export_test.svg");
    if (!plot.save(svgPath, QStringLiteral("SVG")))
        return SvgStats();

    return analyzeSvg(svgPath);
}

} // namespace

void TestVectorExport::surfaceColorsReachTheVectorExport()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    bool glOk = false;
    const SvgStats stats = exportFlatSurface(dir.path(), glOk, true);
    if (!glOk)
        QSKIP("no OpenGL context available on this platform");

    QVERIFY(stats.onPage > 100);  // the surface must be in there at all

    const double share = stats.dataColorAreaShare();
    QVERIFY2(share >= 0.80,
             qPrintable(QStringLiteral("%1 of %2 on page polygons carry the data color; they cover %3 % of the polygon area")
                            .arg(stats.dataColor)
                            .arg(stats.onPage)
                            .arg(100.0 * share, 0, 'f', 1)));
}

void TestVectorExport::fillsAreNotTransparent()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    bool glOk = false;
    // the full scene (box coordinate system, FILLEDMESH style, legend off by
    // default): its furniture must not turn the surface fills transparent
    const SvgStats stats = exportFlatSurface(dir.path(), glOk, false);
    if (!glOk)
        QSKIP("no OpenGL context available on this platform");

    QCOMPARE(stats.transparent, 0);
    QVERIFY(stats.dataColor > 0);
    QVERIFY(stats.dataColorAreaShare() > 0.2);
}

QTEST_MAIN(TestVectorExport)
#include "test_vector_export.moc"
