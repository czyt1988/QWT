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

#include "qwt_text_scale_draw.h"
#include "qwt_text.h"

#include <qmap.h>

class QwtTextScaleDraw::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtTextScaleDraw)
public:
    PrivateData(QwtTextScaleDraw* p) : q_ptr(p)
    {
    }

    QMap< double, QString > labelMap;
};

/**
 * @brief Constructor
 * @details Constructs a text scale draw with an empty label map, so no
 *          labels are displayed until setLabelMap() is called.
 */
QwtTextScaleDraw::QwtTextScaleDraw() : QWT_PIMPL_CONSTRUCT
{
}

/**
 * @brief Constructor
 * @param[in] map Value to label map
 * @details Constructs a text scale draw with a custom value-to-label map.
 *          The values of the major ticks are looked up in this map to
 *          obtain the displayed text.
 * @sa setLabelMap(), labelMap()
 */
QwtTextScaleDraw::QwtTextScaleDraw(const QMap< double, QString >& map) : QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    d->labelMap = map;
}

/**
 * @brief Destructor
 */
QwtTextScaleDraw::~QwtTextScaleDraw()
{
}

/**
 * @brief Set the map that maps values to labels
 * @param[in] map Value to label map
 * @details The values of the major ticks are found by looking into this map.
 *          Values that do not match any key produce an empty label.
 * @warning The map has no effect for values that are not major tick values.
 *          Major ticks are controlled by the scale engine and QwtScaleDiv.
 * @sa labelMap(), QwtScaleDraw::setScale()
 */
void QwtTextScaleDraw::setLabelMap(const QMap< double, QString >& map)
{
    QWT_D(d);
    d->labelMap = map;
}

/**
 * @brief Get the map that maps values to labels
 * @return Map mapping values to labels
 * @sa setLabelMap()
 */
QMap< double, QString > QwtTextScaleDraw::labelMap() const
{
    QWT_DC(d);
    return d->labelMap;
}

/**
 * @brief Map a value to a corresponding label
 * @param[in] value Value that will be mapped
 * @return The label corresponding to value, or a null text if value matches no key
 * @details label() looks up the labelMap() for a corresponding label for value.
 *          Matching uses a small tolerance (1e-6) to be robust against floating
 *          point drift of the tick values. If no key matches, a null text is
 *          returned.
 * @sa labelMap(), setLabelMap()
 */
QwtText QwtTextScaleDraw::label(double value) const
{
    QWT_DC(d);
    for (auto it = d->labelMap.constBegin(); it != d->labelMap.constEnd(); ++it) {
        if (qAbs(it.key() - value) < 1e-6)
            return QwtText(it.value());
    }
    return QwtText();
}
