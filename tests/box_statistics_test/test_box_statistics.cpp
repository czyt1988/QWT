/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "test_box_statistics.h"

#include <qwt_math.h>

#include <qnumeric.h>
#include <qwt_samples.h>

#include <algorithm>
#include <cmath>

namespace
{

typedef QwtBoxStatisticsCalculator Calculator;

/**
 * @brief 64 samples from the boxplot report's Percentile regression case
 * @details The same data as the `pctl5` case in
 *          docs/assets/algorithms/boxstats_cases.txt, where the exchanged
 *          whisker levels made outlierCount == 64/64 before the fix.
 */
const double kRegressionData[ 64 ] = {
    1.2462453135326994, 0.01735913799291827, -1.44830085207875, 0.1891556765177059,
    -0.3142981357066606, 0.9803864101114063, -0.5978888804564941, -0.7546439666284438,
    0.30354273613915245, -1.194199782351566, -0.45759955396202173, 1.8560384196638962,
    -0.11294941780339794, 1.224440853848337, 0.6052468003360679, 0.4606886823469001,
    1.2180598470233979, -1.6015873220274324, -0.721982328811185, 2.385938189676337,
    0.6211267313847142, 1.3202346137640737, -0.29299371628815823, 1.6726809229771151,
    1.1752003991473798, -1.8490812591356378, 0.7091864504094479, -1.5887030200605463,
    -1.7829612916914876, 0.47538950743445385, 0.08968196435689599, -0.35404520917270915,
    0.07655989999309695, -0.7931950855675098, 0.6622744948278944, -1.2344030267077903,
    0.35126216338678484, 0.8480630660694783, 0.0929936542342034, -1.3456449703977553,
    -1.149372884612683, -0.7779165770670621, -1.0210223487085606, 0.1317215003871752,
    0.22859270248275693, 1.8609439275329975, -0.5063420836286241, 2.64099595930225,
    0.05666050625133433, 0.7879457164443378, -0.6687455990040275, -1.42300233695019,
    0.9699550870488862, -1.3465162774518402, 0.5014618464230554, 0.8680794277007995,
    0.8320910387608739, -0.5507479486584484, -0.6304153160359038, 0.6049599625713471,
    3.308808499393951, 1.3505997613638738, 0.1851209281443434, -1.6222080826944862,
};

QVector< double > regressionData()
{
    QVector< double > data;
    data.reserve(64);
    for (int i = 0; i < 64; ++i)
        data.append(kRegressionData[ i ]);
    return data;
}

/// Linear ramp 1..100: R-7 quantiles are exact tenths, outliers countable by hand
QVector< double > linearData()
{
    QVector< double > data;
    data.reserve(100);
    for (int i = 1; i <= 100; ++i)
        data.append(i);
    return data;
}

void fuzzyCompare(double got, double expected, double tolerance = 1e-12)
{
    QVERIFY2(std::fabs(got - expected) <= tolerance,
             qPrintable(QString("got %1 expected %2").arg(got, 0, 'g', 17).arg(expected, 0, 'g', 17)));
}

/// R-7 (linear interpolation) quantile of unsorted data - the oracle for the calculator
double quantileOf(const QVector< double >& data, double p)
{
    QVector< double > sorted = data;
    std::sort(sorted.begin(), sorted.end());

    const double index = p * (sorted.size() - 1);
    const int lower    = static_cast< int >(std::floor(index));
    const int upper    = static_cast< int >(std::ceil(index));

    if (lower == upper)
        return sorted[ lower ];

    return sorted[ lower ] + (index - lower) * (sorted[ upper ] - sorted[ lower ]);
}

/// Mean of the finite samples (the calculator skips NaN/Inf)
double meanOf(const QVector< double >& data)
{
    double sum = 0.0;
    int count  = 0;
    for (double v : data) {
        if (!qwt_is_nan_or_inf(v)) {
            sum += v;
            ++count;
        }
    }
    return sum / count;
}

/// Sample standard deviation, ddof = 1 (the calculator's convention)
double sdOf(const QVector< double >& data)
{
    const double m = meanOf(data);
    double sumSq   = 0.0;
    int count      = 0;
    for (double v : data) {
        if (!qwt_is_nan_or_inf(v)) {
            sumSq += (v - m) * (v - m);
            ++count;
        }
    }
    return std::sqrt(sumSq / (count - 1));
}

} // namespace

void TestBoxStatistics::percentileRegressionDataSet()
{
    const QwtBoxSample sample =
        Calculator::calculateFromRaw(1.0, regressionData(), Calculator::Percentile, 5.0);

    // R-7 quantiles of the 64 sorted values (verified against numpy.quantile)
    fuzzyCompare(sample.whiskerLower, -1.5996546767323994);
    fuzzyCompare(sample.q1, -0.73014773826549972);
    fuzzyCompare(sample.median, 0.11235757731068929);
    fuzzyCompare(sample.q3, 0.83608404558802496);
    fuzzyCompare(sample.whiskerUpper, 1.8602081013526324);

    // The regression asserted wl > wu (1.860208 / -1.599655) and thus 64 outliers
    QVERIFY(sample.whiskerLower < sample.whiskerUpper);
    QVERIFY(sample.isValid());
}

void TestBoxStatistics::percentileOrderingForAllCoefficients()
{
    const QVector< double > data = regressionData();

    // c <= 25 keeps the whiskers outside the box (quantile levels are ordered)
    const double coefficients[] = { 1.0, 5.0, 10.0, 25.0 };
    for (double c : coefficients) {
        const QwtBoxSample sample =
            Calculator::calculateFromRaw(0.0, data, Calculator::Percentile, c);

        QVERIFY2(sample.isValid(),
                 qPrintable(QString("coefficient %1: wl=%2 q1=%3 med=%4 q3=%5 wu=%6")
                                .arg(c)
                                .arg(sample.whiskerLower)
                                .arg(sample.q1)
                                .arg(sample.median)
                                .arg(sample.q3)
                                .arg(sample.whiskerUpper)));
    }

    // A large tail percentage puts the whiskers into the box; that is the
    // documented meaning of the coefficient, not an invalid sample
    const QwtBoxSample wide =
        Calculator::calculateFromRaw(0.0, data, Calculator::Percentile, 49.0);
    fuzzyCompare(wide.whiskerLower, quantileOf(data, 0.49));
    fuzzyCompare(wide.whiskerUpper, quantileOf(data, 0.51));
    QVERIFY(wide.whiskerLower > wide.q1);
    QVERIFY(wide.whiskerUpper < wide.q3);
}

void TestBoxStatistics::percentileOutlierCountEqualsTailSplit()
{
    // A 5% tail on each side of 64 samples means 4 + 4 outliers
    const QwtBoxSample sample =
        Calculator::calculateFromRaw(0.0, regressionData(), Calculator::Percentile, 5.0);

    QCOMPARE(sample.outlierCount, 8);

    const QVector< double > outliers = Calculator::extractOutliers(sample, [] {
        QVector< double > sorted = regressionData();
        std::sort(sorted.begin(), sorted.end());
        return sorted;
    }());
    QCOMPARE(outliers.size(), 8);
}

void TestBoxStatistics::percentileLinearData()
{
    QVector< double > sorted = linearData();
    const QwtBoxSample sample =
        Calculator::calculate(0.0, sorted, Calculator::Percentile, 5.0);

    // quantile(p) = 1 + 99 * p for the ramp 1..100
    fuzzyCompare(sample.whiskerLower, 5.95);
    fuzzyCompare(sample.whiskerUpper, 95.05);
    QCOMPARE(sample.outlierCount, 10);  // 1..5 below, 96..100 above
}

void TestBoxStatistics::otherMethodsStayOrdered()
{
    const QVector< double > data = regressionData();

    // Tukey/Tukey-clamped methods whose whiskers cannot enter the box
    const Calculator::WhiskerMethod methods[] = {
        Calculator::Tukey,
        Calculator::MinMax,
        Calculator::StandardDeviation
    };

    for (Calculator::WhiskerMethod method : methods) {
        const QwtBoxSample sample = Calculator::calculateFromRaw(0.0, data, method, 1.5);
        QVERIFY2(sample.whiskerLower <= sample.q1
                     && sample.q1 <= sample.median
                     && sample.median <= sample.q3
                     && sample.q3 <= sample.whiskerUpper,
                 qPrintable(QString("method %1: wl=%2 q1=%3 med=%4 q3=%5 wu=%6")
                                .arg(int(method))
                                .arg(sample.whiskerLower)
                                .arg(sample.q1)
                                .arg(sample.median)
                                .arg(sample.q3)
                                .arg(sample.whiskerUpper)));
    }

    // StandardDeviation/StandardError are error bars (mean +/- c * spread):
    // verify the formula, and accept that a small spread leaves the whiskers
    // inside the box (the documentated "error bar, not fence" semantics)
    const double mean = meanOf(data);
    const double sd   = sdOf(data);
    const int n       = data.size();

    const QwtBoxSample sdSample =
        Calculator::calculateFromRaw(0.0, data, Calculator::StandardDeviation, 1.5);
    fuzzyCompare(sdSample.whiskerLower, mean - 1.5 * sd);
    fuzzyCompare(sdSample.whiskerUpper, mean + 1.5 * sd);

    const QwtBoxSample seSample =
        Calculator::calculateFromRaw(0.0, data, Calculator::StandardError, 1.5);
    fuzzyCompare(seSample.whiskerLower, mean - 1.5 * sd / std::sqrt(double(n)));
    fuzzyCompare(seSample.whiskerUpper, mean + 1.5 * sd / std::sqrt(double(n)));
    QVERIFY(!seSample.isValid());  // 1.5 * SE sits well inside the box for n = 64
}

void TestBoxStatistics::minMaxHasNoOutliers()
{
    const QwtBoxSample sample =
        Calculator::calculateFromRaw(0.0, regressionData(), Calculator::MinMax);

    QCOMPARE(sample.outlierCount, 0);
    fuzzyCompare(sample.whiskerLower, -1.8490812591356378);
    fuzzyCompare(sample.whiskerUpper, 3.308808499393951);
}

void TestBoxStatistics::rawInputFiltersNanAndInf()
{
    QVector< double > data = regressionData();
    data.append(qQNaN());
    data.append(qInf());
    data.append(-qInf());

    const QwtBoxSample withDirt = Calculator::calculateFromRaw(0.0, data, Calculator::MinMax);
    const QwtBoxSample clean    = Calculator::calculateFromRaw(0.0, regressionData(), Calculator::MinMax);

    fuzzyCompare(withDirt.whiskerLower, clean.whiskerLower);
    fuzzyCompare(withDirt.whiskerUpper, clean.whiskerUpper);
    QCOMPARE(withDirt.outlierCount, 0);
}

void TestBoxStatistics::emptyInputIsZeroed()
{
    const QwtBoxSample sample = Calculator::calculateFromRaw(0.0, QVector< double >());

    fuzzyCompare(sample.whiskerLower, 0.0);
    fuzzyCompare(sample.whiskerUpper, 0.0);
    QCOMPARE(sample.outlierCount, 0);
}

void TestBoxStatistics::calculatorFillsMean()
{
    const QVector< double > data = regressionData();
    const double expected = meanOf(data);

    const Calculator::WhiskerMethod methods[] = {
        Calculator::Tukey,
        Calculator::Percentile,
        Calculator::MinMax,
        Calculator::StandardDeviation,
        Calculator::StandardError
    };

    for (Calculator::WhiskerMethod method : methods) {
        const QwtBoxSample sample = Calculator::calculateFromRaw(0.0, data, method, 1.5);
        QVERIFY(sample.hasMean());
        fuzzyCompare(sample.mean, expected);
    }
}

void TestBoxStatistics::manualSampleHasNoMean()
{
    QwtBoxSample sample(1.0, -1.0, -0.5, 0.0, 0.5, 1.0);

    QVERIFY(!sample.hasMean());
    QVERIFY(qIsNaN(sample.mean));

    sample.mean = 0.25;
    QVERIFY(sample.hasMean());
    fuzzyCompare(sample.mean, 0.25);
}

QTEST_APPLESS_MAIN(TestBoxStatistics)
#include "test_box_statistics.moc"
