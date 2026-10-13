/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef TEST_CLIPPER_H
#define TEST_CLIPPER_H

#include <QObject>
#include <QTest>
#include <QCoreApplication>

#include <qwt_interval.h>

/**
 * @brief Tests for QwtClipper::clipCircle
 * @details Directed regression cases for the circle clipper: the angle
 *          convention on vertical tangents (the center exactly on a vertical
 *          rectangle edge) and the corner duplicates of circles passing
 *          through a rectangle corner, both measured against a brute force
 *          angular sampling.
 */
class TestClipper : public QObject
{
    Q_OBJECT

private slots:
    /// Center on the left edge: the arc crossing the vertical tangent keeps its lower half
    void verticalEdgeLeft();

    /// Center on the right edge (mirror case)
    void verticalEdgeRight();

    /// A circle through all four corners: only points, no measurable arc
    void cornerPassingCircle();

    /// Center on a rectangle corner: only the quarter arc inside stays
    void cornerTouchingFromInside();

    /// Inscribed circle: tangents to the edges fall back to the containment test
    void inscribedCircle();

    /// Fully inside -> exactly [0, 2*pi]; fully outside / containing -> empty
    void containmentCases();

    /// Interval measure agrees with brute force sampling for a case matrix
    void measureMatchesBruteForce();
};

#endif  // TEST_CLIPPER_H
