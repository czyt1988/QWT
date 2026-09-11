#include <QtTest/QtTest>
#include <cmath>
#include "NanDataGenerator.h"

class TestNanDataGenerator : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testLeadingNan();
    void testLeadingTrailingNan();
    void testMiddleNan();
    void testTrailingNan();
    void testXyNan();
    void testXyInterleavedNan();
    void testAllNan();
    void testBaselineNoNan();
    void testXAlwaysFinite();
};

void TestNanDataGenerator::testLeadingNan()
{
    QVector< double > x, y;
    NanDataGenerator::generate(NanCase::Leading, 1000, 0.5, x, y);
    QCOMPARE(x.size(), 1000);
    QCOMPARE(y.size(), 1000);
    QCOMPARE(NanDataGenerator::nanCount(NanCase::Leading, 1000, 0.5), 500);
    QVERIFY(std::isnan(y[ 0 ]));
    QVERIFY(std::isnan(y[ 499 ]));
    QVERIFY(!std::isnan(y[ 500 ]));
    QVERIFY(!std::isnan(y[ 999 ]));
}

void TestNanDataGenerator::testLeadingTrailingNan()
{
    QVector< double > x, y;
    NanDataGenerator::generate(NanCase::LeadingTrailing, 1000, 0.5, x, y);
    const int expected = 2 * (500 / 2);  // == 500
    QCOMPARE(NanDataGenerator::nanCount(NanCase::LeadingTrailing, 1000, 0.5), expected);
    QVERIFY(std::isnan(y[ 0 ]));
    QVERIFY(!std::isnan(y[ 250 ]));
    QVERIFY(!std::isnan(y[ 749 ]));
    QVERIFY(std::isnan(y[ 999 ]));
}

void TestNanDataGenerator::testMiddleNan()
{
    QVector< double > x, y;
    NanDataGenerator::generate(NanCase::Middle, 1000, 0.5, x, y);
    QCOMPARE(NanDataGenerator::nanCount(NanCase::Middle, 1000, 0.5), 500);
    // midStart = (1000-500)/2 = 250 -> NaN in [250,750)
    QVERIFY(!std::isnan(y[ 249 ]));
    QVERIFY(std::isnan(y[ 250 ]));
    QVERIFY(std::isnan(y[ 749 ]));
    QVERIFY(!std::isnan(y[ 750 ]));
}

void TestNanDataGenerator::testTrailingNan()
{
    QVector< double > x, y;
    NanDataGenerator::generate(NanCase::Trailing, 1000, 0.5, x, y);
    QCOMPARE(NanDataGenerator::nanCount(NanCase::Trailing, 1000, 0.5), 500);
    QVERIFY(!std::isnan(y[ 499 ]));
    QVERIFY(std::isnan(y[ 500 ]));
    QVERIFY(std::isnan(y[ 999 ]));
}

void TestNanDataGenerator::testBaselineNoNan()
{
    QVector< double > x, y;
    NanDataGenerator::generate(NanCase::Baseline, 1000, 0.5, x, y);
    QCOMPARE(NanDataGenerator::nanCount(NanCase::Baseline, 1000, 0.5), 0);
    for (int i = 0; i < y.size(); ++i)
        QVERIFY2(!std::isnan(y[ i ]), qPrintable(QString("y[%1] is NaN").arg(i)));
}

void TestNanDataGenerator::testXyNan()
{
    QVector< double > x, y;
    NanDataGenerator::generate(NanCase::XyNan, 1000, 0.5, x, y);
    QCOMPARE(x.size(), 1000);
    QCOMPARE(y.size(), 1000);
    QCOMPARE(NanDataGenerator::nanCount(NanCase::XyNan, 1000, 0.5), 500);
    // midStart = (1000-500)/2 = 250 -> NaN in [250,750) for both X and Y
    QVERIFY(!std::isnan(x[ 249 ]) && !std::isnan(y[ 249 ]));
    QVERIFY(std::isnan(x[ 250 ]) && std::isnan(y[ 250 ]));
    QVERIFY(std::isnan(x[ 749 ]) && std::isnan(y[ 749 ]));
    QVERIFY(!std::isnan(x[ 750 ]) && !std::isnan(y[ 750 ]));
}

void TestNanDataGenerator::testXyInterleavedNan()
{
    QVector< double > x, y;
    NanDataGenerator::generate(NanCase::XyInterleavedNan, 1000, 0.5, x, y);
    QCOMPARE(x.size(), 1000);
    QCOMPARE(y.size(), 1000);
    QCOMPARE(NanDataGenerator::nanCount(NanCase::XyInterleavedNan, 1000, 0.5), 500);
    // midStart = (1000-500)/2 = 250 -> NaN region [250,750)
    // Even offset: X-only NaN, odd offset: Y-only NaN — never both
    QVERIFY(std::isnan(x[ 250 ]) && !std::isnan(y[ 250 ]));  // offset 0 even
    QVERIFY(!std::isnan(x[ 251 ]) && std::isnan(y[ 251 ]));  // offset 1 odd
    QVERIFY(std::isnan(x[ 252 ]) && !std::isnan(y[ 252 ]));  // offset 2 even
    QVERIFY(!std::isnan(x[ 253 ]) && std::isnan(y[ 253 ]));  // offset 3 odd
    // No point in NaN region has both X and Y NaN
    for (int i = 250; i < 750; ++i)
        QVERIFY(!(std::isnan(x[ i ]) && std::isnan(y[ i ])));
    // Outside NaN region: everything finite
    QVERIFY(!std::isnan(x[ 249 ]) && !std::isnan(y[ 249 ]));
    QVERIFY(!std::isnan(x[ 750 ]) && !std::isnan(y[ 750 ]));
}

void TestNanDataGenerator::testAllNan()
{
    QVector< double > x, y;
    NanDataGenerator::generate(NanCase::AllNan, 1000, 0.5, x, y);
    QCOMPARE(x.size(), 1000);
    QCOMPARE(y.size(), 1000);
    QCOMPARE(NanDataGenerator::nanCount(NanCase::AllNan, 1000, 0.5), 1000);
    // Every Y is NaN; nanFraction is irrelevant for this case.
    for (int i = 0; i < y.size(); ++i)
        QVERIFY2(std::isnan(y[ i ]), qPrintable(QString("y[%1] is not NaN").arg(i)));
    // X stays as the finite monotonic index.
    for (int i = 0; i < x.size(); ++i)
        QCOMPARE(x[ i ], double(i));
}

void TestNanDataGenerator::testXAlwaysFinite()
{
    QVector< double > x, y;
    NanDataGenerator::generate(NanCase::Leading, 1000, 0.5, x, y);
    for (int i = 0; i < x.size(); ++i)
        QCOMPARE(x[ i ], double(i));
}

QTEST_MAIN(TestNanDataGenerator)

#include "tst_NanDataGenerator.moc"
