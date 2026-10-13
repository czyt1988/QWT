/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "test_scale_draw.h"

#include <qwt_scale_div.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_map.h>

#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QVector>

namespace
{

/**
 * @brief Scale draw recording every tick that reaches the painter
 */
class CountingScaleDraw : public QwtScaleDraw
{
public:
    struct Tick
    {
        double value;
        double length;
    };

    mutable QVector< Tick > drawn;

protected:
    void drawTick(QPainter* painter, double value, double len) const override
    {
        drawn.append({ value, len });
        QwtScaleDraw::drawTick(painter, value, len);
    }
};

QVector< CountingScaleDraw::Tick > paintedTicks(const QwtScaleDiv& scaleDiv)
{
    CountingScaleDraw scaleDraw;
    scaleDraw.enableComponent(QwtAbstractScaleDraw::Labels, false);
    scaleDraw.enableComponent(QwtAbstractScaleDraw::Backbone, false);
    scaleDraw.setAlignment(QwtScaleDraw::LeftScale);  // vertical, ticks to the left
    scaleDraw.setTickLength(QwtScaleDiv::MajorTick, 8.0);
    scaleDraw.setTickLength(QwtScaleDiv::MediumTick, 5.0);
    scaleDraw.setTickLength(QwtScaleDiv::MinorTick, 3.0);

    // move()/setLength() derive the paint interval from the origin/length
    scaleDraw.scaleMap().setScaleInterval(scaleDiv.lowerBound(), scaleDiv.upperBound());
    scaleDraw.move(16, 20);
    scaleDraw.setLength(400.0);
    scaleDraw.setScaleDiv(scaleDiv);

    QImage image(24, 440, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    QPalette palette;
    palette.setColor(QPalette::WindowText, Qt::black);
    palette.setColor(QPalette::Text, Qt::black);

    QPainter painter(&image);
    scaleDraw.draw(&painter, palette);
    painter.end();

    return scaleDraw.drawn;
}

bool containsTick(const QVector< CountingScaleDraw::Tick >& ticks, double value, double length)
{
    for (const auto& tick : ticks) {
        if (qFuzzyCompare(tick.value, value) && qFuzzyCompare(tick.length, length))
            return true;
    }

    return false;
}

int countTicks(const QVector< CountingScaleDraw::Tick >& ticks, double value)
{
    int count = 0;
    for (const auto& tick : ticks) {
        if (qFuzzyCompare(tick.value, value))
            ++count;
    }

    return count;
}

} // namespace

void TestScaleDraw::ticksAreSortedAndDeduplicated()
{
    QwtScaleDiv scaleDiv(0.0, 100.0);
    scaleDiv.setTicks(QwtScaleDiv::MinorTick, { 12.0, 5.0, 12.0, 24.5, 5.0 });

    QCOMPARE(scaleDiv.ticks(QwtScaleDiv::MinorTick), (QList< double >{ 5.0, 12.0, 24.5 }));
}

void TestScaleDraw::duplicateValuesArePaintedOnce()
{
    QwtScaleDiv scaleDiv(0.0, 100.0);
    scaleDiv.setTicks(QwtScaleDiv::MajorTick, { 0.0, 100.0 });
    scaleDiv.setTicks(QwtScaleDiv::MinorTick, { 25.0, 25.0, 25.0, 50.0, 50.0 });

    const QVector< CountingScaleDraw::Tick > ticks = paintedTicks(scaleDiv);

    QCOMPARE(countTicks(ticks, 25.0), 1);
    QCOMPARE(countTicks(ticks, 50.0), 1);
    QCOMPARE(countTicks(ticks, 0.0), 1);   // major
    QCOMPARE(countTicks(ticks, 100.0), 1);
    QCOMPARE(ticks.size(), 4);
}

void TestScaleDraw::coincidentTicksKeepTheLongestLength()
{
    // The date engine can collapse a minor tick onto a major tick - the
    // position must keep the major length (8.0), not be painted twice
    QwtScaleDiv scaleDiv(0.0, 100.0);
    scaleDiv.setTicks(QwtScaleDiv::MajorTick, { 50.0 });
    scaleDiv.setTicks(QwtScaleDiv::MediumTick, { 50.0, 75.0 });
    scaleDiv.setTicks(QwtScaleDiv::MinorTick, { 50.0, 75.0, 25.0 });

    const QVector< CountingScaleDraw::Tick > ticks = paintedTicks(scaleDiv);

    QCOMPARE(countTicks(ticks, 50.0), 1);
    QVERIFY(containsTick(ticks, 50.0, 8.0));  // major length

    QCOMPARE(countTicks(ticks, 75.0), 1);
    QVERIFY(containsTick(ticks, 75.0, 5.0));  // medium length

    QCOMPARE(countTicks(ticks, 25.0), 1);
    QVERIFY(containsTick(ticks, 25.0, 3.0));  // minor length
    QCOMPARE(ticks.size(), 3);
}

void TestScaleDraw::ticksOutsideTheIntervalAreSkipped()
{
    QwtScaleDiv scaleDiv(0.0, 100.0);
    scaleDiv.setTicks(QwtScaleDiv::MinorTick, { -10.0, 25.0, 120.0 });

    const QVector< CountingScaleDraw::Tick > ticks = paintedTicks(scaleDiv);

    QCOMPARE(ticks.size(), 1);
    QVERIFY(containsTick(ticks, 25.0, 3.0));
}

QTEST_MAIN(TestScaleDraw)
#include "test_scale_draw.moc"
