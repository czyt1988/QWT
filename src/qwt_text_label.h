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

#ifndef QWT_TEXT_LABEL_H
#define QWT_TEXT_LABEL_H

#include "qwt_global.h"
#include "qwt_text.h"

#include <qframe.h>

class QString;
class QPaintEvent;
class QPainter;

/*!
   \brief A Widget which displays a QwtText
 */

class QWT_EXPORT QwtTextLabel : public QFrame
{
    Q_OBJECT

    Q_PROPERTY( int indent READ indent WRITE setIndent )
    Q_PROPERTY( int margin READ margin WRITE setMargin )
    Q_PROPERTY( QString plainText READ plainText WRITE setPlainText )

  public:
    explicit QwtTextLabel( QWidget* parent = NULL );
    explicit QwtTextLabel( const QwtText&, QWidget* parent = NULL );
    virtual ~QwtTextLabel();

    void setPlainText( const QString& );
    QString plainText() const;

  public Q_SLOTS:
    void setText( const QString&,
        QwtText::TextFormat textFormat = QwtText::AutoText );
    virtual void setText( const QwtText& );

    void clear();

  public:
    const QwtText& text() const;

    int indent() const;
    void setIndent( int );

    int margin() const;
    void setMargin( int );

    virtual QSize sizeHint() const QWT_OVERRIDE;
    virtual QSize minimumSizeHint() const QWT_OVERRIDE;
    virtual int heightForWidth( int ) const QWT_OVERRIDE;

    QRect textRect() const;

    virtual void drawText( QPainter*, const QRectF& );

  protected:
    virtual void paintEvent( QPaintEvent* ) QWT_OVERRIDE;
    virtual void drawContents( QPainter* );

  private:
    void init();
    int defaultIndent() const;

    class PrivateData;
    PrivateData* m_data;
};

#endif
