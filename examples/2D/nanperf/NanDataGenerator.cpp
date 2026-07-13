// MSVC does not expose M_PI from <cmath> unless _USE_MATH_DEFINES is defined
// before any header pulls in math.h.
#define _USE_MATH_DEFINES

#include "NanDataGenerator.h"

#include <QtGlobal>

#include <cmath>

double NanDataGenerator::signalValue(int i)
{
    const double t1 = 2.0 * M_PI * i / 2000.0;
    const double t2 = 2.0 * M_PI * i / 200.0;
    return std::sin(t1) + 0.3 * std::sin(t2);
}

int NanDataGenerator::nanCount(NanCase cs, int numPoints, double nanFraction)
{
    const int nanN = static_cast< int >(nanFraction * numPoints);
    switch (cs) {
    case NanCase::Leading:
    case NanCase::Middle:
    case NanCase::Trailing:
    case NanCase::XyNan:
    case NanCase::XyInterleavedNan:
        return nanN;
    case NanCase::LeadingTrailing:
        return 2 * (nanN / 2);
    case NanCase::AllNan:
        return numPoints;
    case NanCase::Baseline:
        return 0;
    }
    return 0;
}

void NanDataGenerator::generate(NanCase cs, int numPoints, double nanFraction, QVector< double >& x, QVector< double >& y)
{
    x.resize(numPoints);
    y.resize(numPoints);
    const int nanN     = static_cast< int >(nanFraction * numPoints);
    const int half     = nanN / 2;
    const int midStart = (numPoints - nanN) / 2;
    const double nan   = qQNaN();

    for (int i = 0; i < numPoints; ++i) {
        x[ i ] = static_cast< double >(i);

        bool isNan = false;
        bool xNan  = false;
        bool yNan  = false;
        switch (cs) {
        case NanCase::Leading:
            isNan = (i < nanN);
            yNan  = isNan;
            break;
        case NanCase::LeadingTrailing:
            isNan = (i < half) || (i >= numPoints - half);
            yNan  = isNan;
            break;
        case NanCase::Middle:
        case NanCase::XyNan:
            isNan = (i >= midStart) && (i < midStart + nanN);
            yNan  = isNan;
            if (cs == NanCase::XyNan && isNan)
                xNan = true;
            break;
        case NanCase::XyInterleavedNan: {
            const bool inRange = (i >= midStart) && (i < midStart + nanN);
            if (inRange) {
                if ((i - midStart) % 2 == 0)
                    xNan = true;  // even: X-only NaN
                else
                    yNan = true;  // odd: Y-only NaN
            }
            isNan = inRange;
            break;
        }
        case NanCase::Trailing:
            isNan = (i >= numPoints - nanN);
            yNan  = isNan;
            break;
        case NanCase::AllNan:
            yNan = true;  // every point NaN regardless of nanFraction
            break;
        case NanCase::Baseline:
            isNan = false;
            break;
        }
        y[ i ] = yNan ? nan : signalValue(i);
        if (xNan)
            x[ i ] = nan;
    }
}

QString NanDataGenerator::caseLabel(NanCase cs)
{
    switch (cs) {
    case NanCase::Leading:
        return QStringLiteral("Leading NaN");
    case NanCase::LeadingTrailing:
        return QStringLiteral("Leading & Trailing NaN");
    case NanCase::Middle:
        return QStringLiteral("Middle NaN");
    case NanCase::Trailing:
        return QStringLiteral("Trailing NaN");
    case NanCase::XyNan:
        return QStringLiteral("X+Y Middle NaN");
    case NanCase::XyInterleavedNan:
        return QStringLiteral("X/Y Interleaved NaN");
    case NanCase::AllNan:
        return QStringLiteral("All NaN");
    case NanCase::Baseline:
        return QStringLiteral("No NaN Baseline");
    }
    return QString();
}
