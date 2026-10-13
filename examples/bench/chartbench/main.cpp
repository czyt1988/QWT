/*****************************************************************************
 * chartbench - rendering performance comparison of Qt plotting libraries
 *
 * Compares Qwt, QCustomPlot, Qt Charts and QIm (ImPlot) by driving all four
 * through the same measurement protocol. Every run produces two CSV tables
 * (per frame samples and per configuration facts) that the analysis scripts in
 * docs/assets/bench turn into the figures of the tech report.
 *
 * See README.md for the protocol and run.sh for the full matrix.
 *****************************************************************************/

#include "BenchCommon.h"
#include "BenchSuites.h"
#include "PlotAdapter.h"

#include <QApplication>
#include <QSurfaceFormat>

#include <cstdio>

using namespace chartbench;

int main(int argc, char** argv)
{
    // ImPlot's GLSL shaders need a 3.3 core profile; the Qwt OpenGL canvas and
    // the QCustomPlot FBO paint buffer are happy with the same format.
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 3);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);

    BenchOptions options;
    QString error;
    if (!BenchOptions::parse(argc, argv, options, error)) {
        std::fputs(qPrintable(error), stderr);
        std::fputc('\n', stderr);
        return 2;
    }

    ResultWriter writer;
    if (!writer.open(options.outputDir, options.library, options.backend)) {
        std::fputs("chartbench: cannot open the result files\n", stderr);
        return 3;
    }

    BenchSuites suites(options, writer);
    if (!suites.initialize(error)) {
        std::fprintf(stderr, "chartbench: %s\n", qPrintable(error));
        return 4;
    }

    const bool ok = suites.run(options.suites);
    std::fprintf(stdout, "chartbench: %s written\n", qPrintable(writer.configPath()));
    return ok ? 0 : 1;
}
