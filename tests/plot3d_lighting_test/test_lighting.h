/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef TEST_LIGHTING_H
#define TEST_LIGHTING_H

#include <QObject>
#include <QTest>
#include <QApplication>

/**
 * @brief Tests for the 3D lighting/material parameters
 * @details The classic setLightComponent()/setMaterialComponent()/setShininess()
 *          API used to discard its arguments ("stored on CPU" TODOs without
 *          storage). The values are stored now and uploaded to the surface
 *          shader uniforms - these tests check both the storage (no GL
 *          needed) and that the parameters actually change the rendered
 *          surface pixels (skipped without an OpenGL context).
 */
class TestLighting : public QObject
{
    Q_OBJECT

private slots:
    /// The setters store what the getters return
    void valuesAreStored();

    /// Theme lighting presets configure light rotation and components
    void themePresetsConfigureTheLight();

    /// Shininess and light rotation change the rendered pixels
    void parametersAffectTheRender();
};

#endif  // TEST_LIGHTING_H
