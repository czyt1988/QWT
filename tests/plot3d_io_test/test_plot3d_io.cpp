/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "test_plot3d_io.h"

#include "qwt3d_io.h"

void TestPlot3dIO::rasterFormatsAreCaseInsensitive()
{
    // "png" is the key QImageWriter registers
    QVERIFY(Qwt3DIO::outputHandler(QStringLiteral("png")) != nullptr);
    QVERIFY(Qwt3DIO::outputHandler(QStringLiteral("PNG")) != nullptr);
    QVERIFY(Qwt3DIO::outputHandler(QStringLiteral("PnG")) != nullptr);
}

void TestPlot3dIO::vectorFormatsAreCaseInsensitive()
{
    QVERIFY(Qwt3DIO::outputHandler(QStringLiteral("PDF")) != nullptr);
    QVERIFY(Qwt3DIO::outputHandler(QStringLiteral("pdf")) != nullptr);
    QVERIFY(Qwt3DIO::outputHandler(QStringLiteral("Svg")) != nullptr);
}

void TestPlot3dIO::unknownFormatsAreNull()
{
    QVERIFY(Qwt3DIO::outputHandler(QStringLiteral("nope")) == nullptr);
    QVERIFY(Qwt3DIO::inputHandler(QStringLiteral("nope")) == nullptr);
}

void TestPlot3dIO::customRegistrationsAreCaseInsensitive()
{
    Qwt3DIO::Function noop = [](Qwt3DPlot*, const QString&) { return true; };

    QVERIFY(Qwt3DIO::defineOutputHandler(QStringLiteral("XYZ"), noop));
    QVERIFY(Qwt3DIO::outputHandler(QStringLiteral("xyz")) != nullptr);
    QVERIFY(Qwt3DIO::outputHandler(QStringLiteral("XYZ")) != nullptr);

    // re-registering with a different casing replaces the entry instead of
    // adding a second one
    int before = 0;
    for (const QString& fmt : Qwt3DIO::outputFormatList()) {
        if (fmt.compare(QStringLiteral("XYZ"), Qt::CaseInsensitive) == 0)
            ++before;
    }
    QCOMPARE(before, 1);

    QVERIFY(Qwt3DIO::defineOutputHandler(QStringLiteral("xYz"), noop));

    int after = 0;
    for (const QString& fmt : Qwt3DIO::outputFormatList()) {
        if (fmt.compare(QStringLiteral("XYZ"), Qt::CaseInsensitive) == 0)
            ++after;
    }
    QCOMPARE(after, 1);
}

void TestPlot3dIO::unknownFormatFails()
{
    // no handler: false without dereferencing the (null) plot
    QVERIFY(!Qwt3DIO::save(nullptr, QStringLiteral("out.unkn"), QStringLiteral("unkn")));
    QVERIFY(!Qwt3DIO::load(nullptr, QStringLiteral("in.unkn"), QStringLiteral("unkn")));
}

QTEST_MAIN(TestPlot3dIO)
#include "test_plot3d_io.moc"
