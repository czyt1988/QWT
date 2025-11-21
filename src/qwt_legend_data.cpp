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

#include "qwt_legend_data.h"
#include "qwt_text.h"
#include "qwt_graphic.h"

//! Constructor
QwtLegendData::QwtLegendData()
{
}

//! Destructor
QwtLegendData::~QwtLegendData()
{
}

/*!
   Set the legend attributes

   QwtLegendData actually is a QMap<int, QVariant> with some
   convenience interfaces

   \param map Values
   \sa values()
 */
void QwtLegendData::setValues( const QMap< int, QVariant >& map )
{
    m_map = map;
}

/*!
   \return Legend attributes
   \sa setValues()
 */
const QMap< int, QVariant >& QwtLegendData::values() const
{
    return m_map;
}

/*!
   \param role Attribute role
   \return True, when the internal map has an entry for role
 */
bool QwtLegendData::hasRole( int role ) const
{
    return m_map.contains( role );
}

/*!
   Set an attribute value

   \param role Attribute role
   \param data Attribute value

   \sa value()
 */
void QwtLegendData::setValue( int role, const QVariant& data )
{
    m_map[role] = data;
}

/*!
   \param role Attribute role
   \return Attribute value for a specific role
 */
QVariant QwtLegendData::value( int role ) const
{
    if ( !m_map.contains( role ) )
        return QVariant();

    return m_map[role];
}

//! \return True, when the internal map is empty
bool QwtLegendData::isValid() const
{
    return !m_map.isEmpty();
}

//! \return Value of the TitleRole attribute
QwtText QwtLegendData::title() const
{
    QwtText text;

    const QVariant titleValue = value( QwtLegendData::TitleRole );
    if ( titleValue.canConvert< QwtText >() )
    {
        text = qvariant_cast< QwtText >( titleValue );
    }
    else if ( titleValue.canConvert< QString >() )
    {
        text.setText( qvariant_cast< QString >( titleValue ) );
    }

    return text;
}

//! \return Value of the IconRole attribute
QwtGraphic QwtLegendData::icon() const
{
    const QVariant iconValue = value( QwtLegendData::IconRole );

    QwtGraphic graphic;
    if ( iconValue.canConvert< QwtGraphic >() )
    {
        graphic = qvariant_cast< QwtGraphic >( iconValue );
    }

    return graphic;
}

//! \return Value of the ModeRole attribute
QwtLegendData::Mode QwtLegendData::mode() const
{
    const QVariant modeValue = value( QwtLegendData::ModeRole );
    if ( modeValue.canConvert< int >() )
    {
        const int mode = qvariant_cast< int >( modeValue );
        return static_cast< QwtLegendData::Mode >( mode );
    }

    return QwtLegendData::ReadOnly;
}

