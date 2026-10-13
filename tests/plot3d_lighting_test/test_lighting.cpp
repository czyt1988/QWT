/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "test_lighting.h"

#include <qwt3d_color.h>
#include <qwt3d_plot.h>
#include <qwt3d_surface.h>
#include <qwt3d_theme.h>

#include <QImage>
#include <QTemporaryDir>
#include <QThread>

#include <cmath>
#include <vector>

namespace
{

float fuzzy(double v)
{
    return std::fabs(float(v)) < 1e-6f ? 0.0f : float(v);
}

Qwt3DSurface* makeSurface(Qwt3DPlot& plot)
{
    const int cols = 16, rows = 16;

    static std::vector< std::vector< double > > grid;
    static std::vector< double* > rowsPtr;

    grid.assign(rows, std::vector< double >(cols));
    rowsPtr.resize(rows);
    for (int j = 0; j < rows; ++j) {
        rowsPtr[ j ] = grid[ j ].data();
        for (int i = 0; i < cols; ++i)
            grid[ j ][ i ] = std::sin(double(i)) * std::cos(double(j));
    }

    auto* surface = new Qwt3DSurface();
    surface->loadFromData(rowsPtr.data(), cols, rows, 0.0, 1.0, 0.0, 1.0);
    surface->attach(&plot);

    return surface;
}

/// Renders the plot into a temporary png and loads it back
QImage renderPng(Qwt3DPlot& plot, const QString& path)
{
    if (!plot.savePixmap(path, QStringLiteral("png")))
        return QImage();

    return QImage(path);
}

int differingPixels(const QImage& a, const QImage& b)
{
    if (a.size() != b.size() || a.isNull())
        return -1;

    int diff = 0;
    for (int y = 0; y < a.height(); ++y) {
        for (int x = 0; x < a.width(); ++x) {
            if (a.pixel(x, y) != b.pixel(x, y))
                ++diff;
        }
    }

    return diff;
}

} // namespace

void TestLighting::valuesAreStored()
{
    Qwt3DPlot plot;

    // defaults: the values the shader used to hardcode
    QCOMPARE(fuzzy(plot.shininess()), 32.0f);
    QCOMPARE(fuzzy(plot.lightComponent(GL_AMBIENT).r), 0.3f);
    QCOMPARE(fuzzy(plot.lightComponent(GL_DIFFUSE).r), 1.0f);
    QCOMPARE(fuzzy(plot.lightComponent(GL_SPECULAR).r), 1.0f);
    QCOMPARE(fuzzy(plot.materialComponent(GL_SPECULAR).r), 0.5f);
    QVERIFY(!plot.isLightIlluminated(0));  // lights start unlit

    plot.setShininess(5.0);
    QCOMPARE(fuzzy(plot.shininess()), 5.0f);

    plot.setLightComponent(GL_AMBIENT, 0.2, 0.3, 0.4, 1.0);
    QCOMPARE(fuzzy(plot.lightComponent(GL_AMBIENT).r), 0.2f);
    QCOMPARE(fuzzy(plot.lightComponent(GL_AMBIENT).g), 0.3f);
    QCOMPARE(fuzzy(plot.lightComponent(GL_AMBIENT).b), 0.4f);

    plot.setLightComponent(GL_DIFFUSE, 0.75);  // intensity overload
    QCOMPARE(fuzzy(plot.lightComponent(GL_DIFFUSE).g), 0.75f);

    plot.setMaterialComponent(GL_SPECULAR, 0.25, 0.25, 0.25, 1.0);
    QCOMPARE(fuzzy(plot.materialComponent(GL_SPECULAR).b), 0.25f);

    plot.illuminate(0);
    QVERIFY(plot.isLightIlluminated(0));
    plot.blowout(0);
    QVERIFY(!plot.isLightIlluminated(0));

    // unknown properties keep the neutral default
    QCOMPARE(fuzzy(plot.lightComponent(GL_TEXTURE).r), 1.0f);
    QCOMPARE(fuzzy(plot.materialComponent(GL_EMISSION).r), 1.0f);
}

void TestLighting::themePresetsConfigureTheLight()
{
    Qwt3DPlot plot;

    {
        Qwt3DTheme theme(Qwt3DTheme::Scientific);
        theme.setLightingPreset(Qwt3DTheme::Studio);
        theme.apply(&plot);

        QVERIFY(plot.lightingEnabled());
        QCOMPARE(fuzzy(plot.lightComponent(GL_AMBIENT).r), 0.2f);
        QCOMPARE(fuzzy(plot.lightComponent(GL_DIFFUSE).r), 0.8f);
        QCOMPARE(fuzzy(plot.xLightRotation()), 45.0f);
    }

    {
        Qwt3DTheme theme(Qwt3DTheme::Scientific);
        theme.setLightingPreset(Qwt3DTheme::NoLighting);
        theme.apply(&plot);

        QVERIFY(!plot.lightingEnabled());
    }
}

void TestLighting::parametersAffectTheRender()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    Qwt3DPlot plot;
    plot.resize(240, 180);
    makeSurface(plot);

    plot.enableLighting();
    plot.illuminate(0);
    plot.show();

    for (int i = 0; i < 40; ++i) {
        QApplication::processEvents();
        QThread::msleep(10);
    }

    if (!plot.initializedGL())
        QSKIP("no OpenGL context available on this platform");

    // control: identical settings render identical pixels
    plot.setShininess(32.0);
    const QImage reference = renderPng(plot, dir.path() + QStringLiteral("/lighting_a.png"));
    const QImage again     = renderPng(plot, dir.path() + QStringLiteral("/lighting_b.png"));
    QVERIFY(!reference.isNull());
    QCOMPARE(differingPixels(reference, again), 0);

    // shininess changes the specular highlight
    plot.setShininess(4.0);
    const QImage lowShininess = renderPng(plot, dir.path() + QStringLiteral("/lighting_c.png"));
    QVERIFY(differingPixels(reference, lowShininess) > 0);

    // the light rotation moves the highlight
    plot.setShininess(32.0);
    plot.setLightRotation(60.0, 0.0, 0.0, 0);
    const QImage rotated = renderPng(plot, dir.path() + QStringLiteral("/lighting_d.png"));
    QVERIFY(differingPixels(reference, rotated) > 0);

    // the light component scales the diffuse term
    plot.setLightRotation(0.0, 0.0, 0.0, 0);
    plot.setLightComponent(GL_DIFFUSE, 0.2, 0.2, 0.2);
    const QImage dimmed = renderPng(plot, dir.path() + QStringLiteral("/lighting_e.png"));
    QVERIFY(differingPixels(reference, dimmed) > 0);
}

QTEST_MAIN(TestLighting)
#include "test_lighting.moc"
