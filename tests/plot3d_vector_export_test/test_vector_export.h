/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef TEST_VECTOR_EXPORT_H
#define TEST_VECTOR_EXPORT_H

#include <QObject>
#include <QTest>
#include <QApplication>

/**
 * @brief Regression test for the gl2ps vector export of surface data
 * @details The gl2ps writer captures the scene from the GL feedback buffer,
 *          where the per vertex color is the fixed-function current color -
 *          the shader based render path never writes it. Before v7.5.1 the
 *          exported PDF/SVG therefore contained fills with alpha 0 (invisible
 *          surfaces); the check here exports a surface painted by a constant
 *          data color functor and requires the on-page polygon fills to carry
 *          that color.
 *
 *          The test needs a working OpenGL context (Qwt3DPlot is a
 *          QOpenGLWidget); it is skipped when the platform cannot provide one.
 */
class TestVectorExport : public QObject
{
    Q_OBJECT

private slots:
    /// Vector exports paint the surface with its data colors
    void surfaceColorsReachTheVectorExport();

    /// Not a single polygon is fully transparent
    void fillsAreNotTransparent();
};

#endif  // TEST_VECTOR_EXPORT_H
