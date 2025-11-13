/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_utils.h"
#include <qapplication.h>
#include <QtMath>
#include <QPen>
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_histogram.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_canvas.h"
#include "qwt_scale_widget.h"
#include "qwt_scale_draw.h"
#include "qwt_date_scale_engine.h"
#include "qwt_date_scale_draw.h"
#include "qwt_plot_multi_barchart.h"
#include "qwt_column_symbol.h"
#include "qwt_plot_grid.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_rasteritem.h"
#include "qwt_plot_spectrocurve.h"
#include "qwt_plot_tradingcurve.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_scale_map.h"
#include "qwt_plot_legenditem.h"
#include "qwt_plot_zoneitem.h"
#include "qwt_plot_vectorfield.h"
#include "qwt_math.h"
#define QWT_GLOBAL_STRUT

#if QT_VERSION >= 0x050000
#if QT_VERSION >= 0x060000 || !QT_DEPRECATED_SINCE(5, 15)
#undef QWT_GLOBAL_STRUT
#endif
#endif

namespace Qwt
{

/**
 * @brief 获取item的颜色
 *
 * @note 此函数使用dynamic_cast
 * @param item
 * @param defaultColor 默认颜色，无法获取时返回
 * @return
 */
QColor plotItemColor(QwtPlotItem* item, const QColor& defaultColor)
{
    if (const QwtPlotCurve* p = dynamic_cast< const QwtPlotCurve* >(item)) {
        return p->pen().color();
    } else if (const QwtPlotIntervalCurve* p = dynamic_cast< const QwtPlotIntervalCurve* >(item)) {
        return p->pen().color();
    } else if (const QwtPlotHistogram* p = dynamic_cast< const QwtPlotHistogram* >(item)) {
        return p->brush().color();
    } else if (const QwtPlotBarChart* p = dynamic_cast< const QwtPlotBarChart* >(item)) {
        return p->brush().color();
    } else if (const QwtPlotGrid* grid = dynamic_cast< const QwtPlotGrid* >(item)) {
        return grid->majorPen().color();
    } else if (const QwtPlotMarker* marker = dynamic_cast< const QwtPlotMarker* >(item)) {
        return marker->linePen().color();
    }
    return defaultColor;
}

}  // end namespace qwt

QSize qwtExpandedToGlobalStrut(const QSize& size)
{
#ifdef QWT_GLOBAL_STRUT
    return size.expandedTo(QApplication::globalStrut());
#else
    return size;
#endif
}
