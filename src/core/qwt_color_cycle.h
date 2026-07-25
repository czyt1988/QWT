/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_COLOR_CYCLE_H
#define QWT_COLOR_CYCLE_H

#include "qwtcore_global.h"
#include <qcolor.h>
#include <qvector.h>

#if QT_VERSION < 0x060000
template< typename T >
class QVector;
#endif

/**
 * @brief Provides cyclic color selection from predefined palettes.
 * @details QwtColorCycle manages an ordered list of colors and returns
 *          them in a cyclic manner using the color() method. This is used
 *          by QwtPlot to automatically assign distinct colors to plot items
 *          (curves, bar charts, etc.) when they are attached without a
 *          user-specified pen or brush.
 *
 *          Several built-in palettes are available, inspired by matplotlib,
 *          Tableau, ColorBrewer, and the Okabe-Ito colorblind-safe palette.
 *
 * @par Example
 * @code
 *   QwtPlot plot;
 *   plot.setColorCycle(QwtColorCycle(QwtColorCycle::OkabeIto));
 *
 *   auto* curve1 = new QwtPlotCurve("Series 1"); // gets color[0]
 *   curve1->attach(&plot);
 *   auto* curve2 = new QwtPlotCurve("Series 2"); // gets color[1]
 *   curve2->attach(&plot);
 * @endcode
 *
 * @sa QwtPlot::setColorCycle(), QwtPlot::colorCycle()
 */
class QWTCORE_EXPORT QwtColorCycle
{
public:
    /**
     * @brief Built-in color palettes
     */
    enum Palette
    {
        //! matplotlib classic 10 colors
        Default10,

        //! Tableau Tab10 — high-contrast categorical colors
        Tab10,

        //! Tableau Tab20 — 20 colors with light/dark pairs
        Tab20,

        //! ColorBrewer Set1 — 9 colors for qualitative data
        Set1,

        //! ColorBrewer Set2 — 8 colors for qualitative data
        Set2,

        //! ColorBrewer Set3 — 12 colors for qualitative data
        Set3,

        //! Okabe-Ito — 8 colorblind-safe colors
        OkabeIto
    };

    QwtColorCycle();
    explicit QwtColorCycle(Palette palette);
    explicit QwtColorCycle(const QVector< QColor >& colors);

    //! Get a color by index (wraps around)
    QColor color(int index) const;

    //! Get the number of colors in the cycle
    int count() const;

    //! Set the palette from a built-in preset
    void setPalette(Palette);

    //! Set custom colors
    void setColors(const QVector< QColor >&);

    //! Get the current color list
    QVector< QColor > colors() const;

    //! Get the colors for a built-in palette
    static QVector< QColor > paletteColors(Palette);

private:
    QVector< QColor > m_colors;
};

#endif
