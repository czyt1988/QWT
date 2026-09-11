/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *
 * Modified by ChenZongYan in 2024 <czy.t@163.com>
 *   Summary of major modifications (see ChangeLog.md for full history):
 *   1. CMake build system & C++11 throughout.
 *   2. Core panner/ zoomer refactored:
 *        - QwtPanner → QwtCachePanner (pixmap-cache version)
 *        - New real-time QwtPlotPanner derived from QwtPicker.
 *   3. Zoomer supports multi-axis.
 *   4. Parasite-plot framework:
 *        - QwtFigure, QwtPlotParasiteLayout, QwtPlotTransparentCanvas,
 *        - QwtPlotScaleEventDispatcher, built-in pan/zoom on axis.
 *   5. New picker: QwtPlotSeriesDataPicker (works with date axis).
 *   6. Raster & color-map extensions:
 *        - QwtGridRasterData (2-D table + interpolation)
 *        - QwtLinearColorMap::stopColors(), stopPos() API rename.
 *   7. Bar-chart: expose pen/brush control.
 *   8. Amalgamated build: single QwtPlot.h / QwtPlot.cpp pair in src-amalgamate.
 *****************************************************************************/

#ifndef QWT_TEXT_SCALE_DRAW_H
#define QWT_TEXT_SCALE_DRAW_H

#include "qwt_global.h"
#include "qwt_scale_draw.h"

class QString;
template< class Key, class T >
class QMap;

/**
 * @brief A scale draw that maps values to arbitrary text labels
 * @details QwtTextScaleDraw displays user-defined text labels at specific
 *          numeric tick positions. It keeps a map from double values to
 *          strings and returns the matching string for each major tick
 *          value, so a linear axis can show category text (e.g. "Mon",
 *          "Tue", "Wed") instead of numbers.
 *
 *          The numeric tick positions are controlled by the scale engine
 *          and the QwtScaleDiv like a plain QwtScaleDraw. Only the label
 *          text is replaced through the value-to-string map. Values that
 *          do not match any key produce an empty label.
 *
 *          Usually the scale draw is combined with an explicit QwtScaleDiv
 *          whose major ticks match the map keys.
 * @code
 *          QMap< double, QString > labels;
 *          labels.insert(0.0, "Mon");
 *          labels.insert(1.0, "Tue");
 *          labels.insert(2.0, "Wed");
 *          plot->setAxisScaleDraw(QwtPlot::xBottom, new QwtTextScaleDraw(labels));
 * @endcode
 * @sa QwtScaleDraw, QwtPlot::setAxisScaleDraw()
 */
class QWT_EXPORT QwtTextScaleDraw : public QwtScaleDraw
{
public:
    // Constructs an empty text scale draw (no labels are displayed)
    explicit QwtTextScaleDraw();

    // Constructs a text scale draw with a custom value-to-label map
    explicit QwtTextScaleDraw(const QMap< double, QString >& map);

    // Destructor
    ~QwtTextScaleDraw() override;

    // Sets the map that maps numeric values to text labels
    void setLabelMap(const QMap< double, QString >& map);

    // Returns the map that maps numeric values to text labels
    QMap< double, QString > labelMap() const;

    // Returns the label for a given value by looking up the label map
    virtual QwtText label(double value) const override;

private:
    QWT_DECLARE_PRIVATE(QwtTextScaleDraw)
};

#endif
