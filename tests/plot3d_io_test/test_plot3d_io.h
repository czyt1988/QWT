/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2026   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef TEST_PLOT3D_IO_H
#define TEST_PLOT3D_IO_H

#include <QObject>
#include <QTest>
#include <QApplication>

/**
 * @brief Tests for the Qwt3DIO format registry
 * @details The registry has to look up format strings case insensitively:
 *          `savePixmap(file, "PNG")` silently failed before v7.5.1 because the
 *          raster keys are the lowercase names of QImageWriter.
 *          No GL context is required for these tests.
 */
class TestPlot3dIO : public QObject
{
    Q_OBJECT

private slots:
    /// Raster format keys resolve for any casing
    void rasterFormatsAreCaseInsensitive();

    /// The vector format keys resolve for any casing too
    void vectorFormatsAreCaseInsensitive();

    /// Unknown formats return nullptr instead of a dangling entry
    void unknownFormatsAreNull();

    /// Custom registrations replace entries case insensitively
    void customRegistrationsAreCaseInsensitive();

    /// load/save report failure for unregistered formats
    void unknownFormatFails();
};

#endif  // TEST_PLOT3D_IO_H
