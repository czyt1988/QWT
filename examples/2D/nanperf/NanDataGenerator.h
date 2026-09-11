#ifndef NAN_DATA_GENERATOR_H
#define NAN_DATA_GENERATOR_H

#include <QVector>
#include <QString>

/// Distribution of NaN values within a generated curve.
enum class NanCase
{
    Leading,          ///< NaN at the beginning, then signal
    LeadingTrailing,  ///< NaN at both ends, signal in the middle
    Middle,           ///< Signal at both ends, NaN in the middle
    Trailing,         ///< Signal first, NaN at the end
    XyNan,            ///< NaN in both X and Y at the middle (breaks monotonic X)
    XyInterleavedNan, ///< Alternating X-only and Y-only NaN in the middle
    AllNan,           ///< Every Y value is NaN (X stays as the finite monotonic index)
    Baseline          ///< No NaN (reference)
};

/// Generates X/Y sample arrays for the NaN rendering/performance example.
/// For most cases X is the monotonic finite index 0..N-1 and NaN is placed
/// only in Y. XyNan injects NaN into both X and Y simultaneously.
/// XyInterleavedNan alternates X-only and Y-only NaN so no point has both
/// coordinates NaN. AllNan sets every Y value to NaN (X stays finite),
/// exercising the fully-degenerate all-NaN curve path.
class NanDataGenerator
{
public:
    /// Fill @a x and @a y for @a cs with @a numPoints points and @a nanFraction NaN.
    static void generate(NanCase cs, int numPoints, double nanFraction, QVector< double >& x, QVector< double >& y);

    /// The synthetic signal value at index @a i.
    static double signalValue(int i);

    /// Exact number of NaN points produced for @a cs.
    static int nanCount(NanCase cs, int numPoints, double nanFraction);

    /// Human-readable label for @a cs.
    static QString caseLabel(NanCase cs);
};

#endif
