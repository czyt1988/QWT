/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef TEST_SCALE_DRAW_H
#define TEST_SCALE_DRAW_H

#include <QObject>
#include <QTest>
#include <QApplication>

/**
 * @brief Tests for tick list normalization and tick deduplication
 * @details The date engine can collapse rounded ticks (duplicated values
 *          within one list, or minor ticks landing exactly on major ticks).
 *          QwtScaleDiv::setTicks() normalizes the lists and QwtScaleDraw
 *          paints every position with the length of the longest tick type
 *          only - these tests count the drawTick() calls instead of
 *          comparing pixels (the device aligned tick rectangles render
 *          idempotently, so a pixel comparison cannot see the difference).
 */
class TestScaleDraw : public QObject
{
    Q_OBJECT

private slots:
    /// setTicks() sorts and removes duplicated values
    void ticksAreSortedAndDeduplicated();

    /// Duplicated values in one list are painted once
    void duplicateValuesArePaintedOnce();

    /// A tick landing on a longer tick type is skipped (the longer one wins)
    void coincidentTicksKeepTheLongestLength();

    /// Values outside the interval are not painted
    void ticksOutsideTheIntervalAreSkipped();
};

#endif  // TEST_SCALE_DRAW_H
