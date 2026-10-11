#ifndef TEST_WATERFALL_H
#define TEST_WATERFALL_H

#include <QtTest/QtTest>

#include "qwt3d_waterfall.h"
#include "qwt3d_color.h"
#include "qwt3d_scale.h"
#include "qwt3d_autoptr.h"

/**
 * @brief Unit tests for Qwt3DWaterfall, Qwt3DTextScale and the P1/P3 fixes
 * @details Only GL-free, pure CPU logic is covered (CI and headless machines
 *          must be able to run the suite). Rendering is verified manually with
 *          the waterfall3D example.
 */
class TestWaterfall : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // Sliding window semantics
    void slidingWindow();
    void hullZGrowsMonotonically();
    void samplesPerTraceContract();
    void binValues();
    void depthOrderKeepsHull();

    // Hull correctness
    void hullRanges();
    void emptyWindow();

    // Capacity changes
    void shrinkCapacity();
    void growCapacity();

    // clear()
    void clearKeepsStyleAndCapacity();

    // Color range push
    void colorRangeByDepth();
    void colorRangeByAmplitude();
    void colorRangeFixed();

    // Qwt3DTextScale / P3 regression
    void textScaleLabels();
    void textScaleSurvivesClonePtr();
    void textScaleNumericFallback();

    // P1 regression: attach() must recalculate the scene
    void attachAfterData();
    void attachHullUnionAndDetachShrink();
    void detachAllResetsHull();

private:
    static QVector<double> flatTrace(double value);
};

#endif // TEST_WATERFALL_H
