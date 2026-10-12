// Cross-check harness for the scale engine tech report.
//
// Reads cases from a text file, one case per line:
//     x1 x2 maxMajorSteps maxMinorSteps stepHint
// and prints the results of the real QWT engines so that the Python
// re-implementation in scaleengine_experiment.py can be compared against
// them ( python scaleengine_experiment.py --cpp-check harness.exe ).
//
// Modes: linear | log | date
//
// Output protocol ( one line per record ):
//     A <i> <autoX1> <autoX2> <autoStep>                     autoScale()
//     D <i> <nMaj> <nMin> <nMed> <firstMaj> <lastMaj>        divideScale( input )
//     P <i> <nMaj> <nMin> <nMed> <firstMaj> <lastMaj>        divideScale( autoScale result )
//
// Build ( MSVC 2019 x64, Qt 5.15.16, prebuilt QWT release libraries ):
//     cl /nologo /O2 /EHsc /std:c++17 /MD /DQWT_DLL /DQWTCORE_DLL
//        /I"%REPO%\src" /I"%REPO%\src\core" /I"%QT%\include"
//        /I"%QT%\include\QtCore" /I"%QT%\include\QtGui"
//        scaleengine_check.cpp /Fe:harness.exe
//        /link /LIBPATH:"%REPO%\build-release\lib" /LIBPATH:"%QT%\lib"
//              qwtcore.lib qwtplot.lib Qt5Core.lib Qt5Gui.lib
//
// No timing is performed - this harness only checks correctness.

#include <qwt_scale_engine.h>
#include <qwt_scale_div.h>
#include <qwt_date_scale_engine.h>

#include <QLocale>
#include <QList>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>

static void printDiv(const char* tag, int index, const QwtScaleDiv& sd)
{
    const QList<double> maj = sd.ticks(QwtScaleDiv::MajorTick);
    const QList<double> min = sd.ticks(QwtScaleDiv::MinorTick);
    const QList<double> med = sd.ticks(QwtScaleDiv::MediumTick);

    const double first = maj.isEmpty() ? std::nan("") : maj.first();
    const double last  = maj.isEmpty() ? std::nan("") : maj.last();

    std::printf("%s %d %d %d %d %.17g %.17g\n",
                tag, index, int(maj.size()), int(min.size()), int(med.size()), first, last);
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        std::fprintf(stderr, "usage: harness <linear|log|date> <casefile>\n");
        return 2;
    }

    const std::string mode = argv[1];

    // Deterministic week alignment.  QwtDate follows QLocale().firstDayOfWeek()
    // at RUNTIME: zh-CN yields Sunday ( CLDR weekStart = sun ), en-GB yields
    // Monday.  The python port defaults to Monday, so force en-GB here.
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedKingdom));

    std::unique_ptr<QwtScaleEngine> engine;
    if (mode == "linear")
        engine.reset(new QwtLinearScaleEngine());
    else if (mode == "log")
        engine.reset(new QwtLogScaleEngine());
    else if (mode == "date")
        engine.reset(new QwtDateScaleEngine(Qt::UTC));
    else {
        std::fprintf(stderr, "unknown mode %s\n", mode.c_str());
        return 2;
    }

    std::ifstream in(argv[2]);
    if (!in) {
        std::fprintf(stderr, "cannot open %s\n", argv[2]);
        return 2;
    }

    int index = 0;
    double x1, x2, hint;
    int maxMajor, maxMinor;
    while (in >> x1 >> x2 >> maxMajor >> maxMinor >> hint) {
        // A: autoScale
        {
            double a1 = x1, a2 = x2, step = 0.0;
            engine->autoScale(maxMajor, a1, a2, step);
            std::printf("A %d %.17g %.17g %.17g\n", index, a1, a2, step);

            // P: the QwtPlot pipeline ( divideScale with the autoScale result )
            const QwtScaleDiv sp = engine->divideScale(a1, a2, maxMajor, maxMinor, step);
            printDiv("P", index, sp);
        }

        // D: divideScale on the raw input
        {
            const QwtScaleDiv sd = engine->divideScale(x1, x2, maxMajor, maxMinor, hint);
            printDiv("D", index, sd);
        }

        index++;
    }

    std::fflush(stdout);
    return 0;
}
