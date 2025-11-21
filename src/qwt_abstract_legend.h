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

#ifndef QWT_ABSTRACT_LEGEND_H
#define QWT_ABSTRACT_LEGEND_H
#include <QFrame>
#include <QVariant>
#include <QList>
#include "qwt_global.h"
#include "qwt_legend_data.h"

/*!
   \brief Abstract base class for legend widgets

   Legends, that need to be under control of the QwtPlot layout system
   need to be derived from QwtAbstractLegend.

   \note Other type of legends can be implemented by connecting to
        the QwtPlot::legendDataChanged() signal. But as these legends
        are unknown to the plot layout system the layout code
        ( on screen and for QwtPlotRenderer ) need to be organized
        in application code.

   \sa QwtLegend
 */
class QWT_EXPORT QwtAbstractLegend : public QFrame
{
        Q_OBJECT

public:
        explicit QwtAbstractLegend(QWidget* parent = NULL);
        virtual ~QwtAbstractLegend();

        /*!
           Render the legend into a given rectangle.

           \param painter Painter
           \param rect Bounding rectangle
           \param fillBackground When true, fill rect with the widget background

           \sa renderLegend() is used by QwtPlotRenderer
         */
        virtual void renderLegend(QPainter* painter, const QRectF& rect, bool fillBackground) const = 0;

        //! \return True, when no plot item is inserted
        virtual bool isEmpty() const = 0;

        virtual int scrollExtent(Qt::Orientation) const;

public Q_SLOTS:

        /*!
           \brief Update the entries for a plot item

           \param itemInfo Info about an item
           \param data List of legend entry attributes for the  item
         */
        virtual void updateLegend(const QVariant& itemInfo, const QList< QwtLegendData >& data) = 0;
};

#endif
