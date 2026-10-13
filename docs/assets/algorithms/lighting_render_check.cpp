// Renders a lit surface with the parameters the shader used to hardcode, so a
// build with and without the lighting uniform wiring can be compared pixel by
// pixel (regression check for "wiring the lighting parameters into the shader
// must not change the default rendering").
//
// Protocol (v7.5.1, i7-1260P / Qt 6.7.3 / Iris Xe):
//   1. build the plot3d sources with the lighting changes reverted, run
//   2. build with the changes, run, compare the PNGs
//   result: 0 differing pixels for the lit and the unlit render
// The LED check also renders an unlit frame (`<out>_unlit.png`).
//
// Build: see the same recipe as surface_export_check.cpp (link qwtplot3d.lib,
// qwtcore.lib, Qt6{Widgets,Gui,Core,OpenGL,OpenGLWidgets}).
#include "qwt3d_plot.h"
#include "qwt3d_surface.h"

#include <QApplication>
#include <QSurfaceFormat>
#include <QThread>

#include <cmath>
#include <vector>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    const QString out = (argc > 1) ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("lighting.png");

    Qwt3DPlot plot;
    plot.resize(320, 240);

    const int cols = 20, rows = 20;
    std::vector< std::vector< double > > grid(rows, std::vector< double >(cols));
    std::vector< double* > rowsPtr(rows);
    for (int j = 0; j < rows; ++j) {
        rowsPtr[j] = grid[j].data();
        for (int i = 0; i < cols; ++i)
            grid[j][i] = std::sin(2.0 * i / cols) * std::cos(2.0 * j / rows);
    }

    auto* surface = new Qwt3DSurface();
    surface->loadFromData(rowsPtr.data(), cols, rows, 0.0, 1.0, 0.0, 1.0);
    surface->attach(&plot);

    // the parameters that used to be hardcoded in qwt3d_surface.cpp /
    // surface.frag: light (0,0,10), white, shininess 32, ambient 0.3,
    // specular 0.5 - set explicitly so the pre-fix build (which ignores them)
    // and the fixed build (which uses them) start from the same state
    plot.setLightRotation(0.0, 0.0, 0.0, 0);
    plot.setLightComponent(GL_AMBIENT, 0.3, 0.3, 0.3);
    plot.setLightComponent(GL_DIFFUSE, 1.0, 1.0, 1.0);
    plot.setLightComponent(GL_SPECULAR, 1.0, 1.0, 1.0);
    plot.setMaterialComponent(GL_AMBIENT, 1.0, 1.0, 1.0);
    plot.setMaterialComponent(GL_DIFFUSE, 1.0, 1.0, 1.0);
    plot.setMaterialComponent(GL_SPECULAR, 0.5, 0.5, 0.5);
    plot.setShininess(32.0);

    plot.enableLighting();
    plot.illuminate(0);
    plot.show();

    for (int i = 0; i < 60; ++i) {
        app.processEvents();
        QThread::msleep(10);
    }

    const bool ok = plot.savePixmap(out, QStringLiteral("png"));

    QString unlit = out;
    unlit.replace(QStringLiteral(".png"), QStringLiteral("_unlit.png"));
    plot.disableLighting();
    for (int i = 0; i < 20; ++i) {
        app.processEvents();
        QThread::msleep(10);
    }
    plot.savePixmap(unlit, QStringLiteral("png"));
    std::printf("render %s -> %s\n", ok ? "ok" : "failed", qPrintable(out));
    return ok ? 0 : 1;
}
