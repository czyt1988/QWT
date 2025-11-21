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

#ifndef QWT_PLOT_DICT
#define QWT_PLOT_DICT

#include "qwt_global.h"
#include "qwt_plot_item.h"

#include <qlist.h>

typedef QList< QwtPlotItem* > QwtPlotItemList;
typedef QList< QwtPlotItem* >::ConstIterator QwtPlotItemIterator;

/*!
   \brief A dictionary for plot items

   QwtPlotDict organizes plot items in increasing z-order.
   If autoDelete() is enabled, all attached items will be deleted
   in the destructor of the dictionary.
   QwtPlotDict can be used to get access to all QwtPlotItem items - or all
   items of a specific type -  that are currently on the plot.

   \sa QwtPlotItem::attach(), QwtPlotItem::detach(), QwtPlotItem::z()
 */
class QWT_EXPORT QwtPlotDict
{
  public:
    explicit QwtPlotDict();
    virtual ~QwtPlotDict();

    void setAutoDelete( bool );
    bool autoDelete() const;

    const QwtPlotItemList& itemList() const;
    QwtPlotItemList itemList( int rtti ) const;

    void detachItems( int rtti = QwtPlotItem::Rtti_PlotItem,
        bool autoDelete = true );

  protected:
    void insertItem( QwtPlotItem* );
    void removeItem( QwtPlotItem* );

  private:
    class PrivateData;
    PrivateData* m_data;
};

#endif
