/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_column_symbol.h"
#include "qwt_painter.h"
#include "qwt_math.h"

#include <qpainter.h>
#include <qpalette.h>

/**
 * @brief draw a box
 * __
 * |*|
 * |*|
 *
 * @param p
 * @param rect
 * @param pen
 * @param brush
 */
static void qwtDrawBox(QPainter* p, const QRectF& rect, const QPen& pen, const QBrush& brush)
{
    if (rect.isValid()) {
        p->fillRect(rect, brush);
    }
    qreal lw = pen.widthF();
    if (lw > 0.0) {
        // 仅仅有一条线
        if (rect.width() == 0.0) {
            p->setPen(brush.color());
            p->drawLine(rect.topLeft(), rect.bottomLeft());
            return;
        }
        // 没有高度
        if (rect.height() == 0.0) {
            p->setPen(brush.color());
            p->drawLine(rect.topLeft(), rect.topRight());
            return;
        }
        // 保证线宽不要超过矩形的宽度或者高度的一半
        lw        = qwtMinF(pen.widthF(), rect.height() / 2.0 - 1.0);
        qreal lw2 = qwtMinF(pen.widthF(), rect.width() / 2.0 - 1.0);
        lw        = qwtMinF(lw, lw2);
        QPen newPen(pen);
        newPen.setWidthF(lw);
        p->setPen(newPen);
        p->drawRect(rect);
    }
}

static void qwtDrawPanel(QPainter* painter, const QRectF& rect, const QPen& pen, const QBrush& brush)
{
    qreal lw = pen.widthF();
    if (lw > 0.0) {
        if (rect.width() == 0.0) {
            painter->setPen(brush.color());
            painter->drawLine(rect.topLeft(), rect.bottomLeft());
            return;
        }

        if (rect.height() == 0.0) {
            painter->setPen(brush.color());
            painter->drawLine(rect.topLeft(), rect.topRight());
            return;
        }

        lw        = qwtMinF(lw, rect.height() / 2.0 - 1.0);
        qreal lw2 = qwtMinF(lw, rect.width() / 2.0 - 1.0);
        lw        = qwtMinF(lw, lw2);

        const QRectF outerRect = rect.adjusted(0, 0, 1, 1);
        const QRectF innerRect = outerRect.adjusted(lw, lw, -lw, -lw);

        QPolygonF lines[ 2 ];

        lines[ 0 ] += outerRect.bottomLeft();
        lines[ 0 ] += outerRect.topLeft();
        lines[ 0 ] += outerRect.topRight();
        lines[ 0 ] += innerRect.topRight();
        lines[ 0 ] += innerRect.topLeft();
        lines[ 0 ] += innerRect.bottomLeft();

        lines[ 1 ] += outerRect.topRight();
        lines[ 1 ] += outerRect.bottomRight();
        lines[ 1 ] += outerRect.bottomLeft();
        lines[ 1 ] += innerRect.bottomLeft();
        lines[ 1 ] += innerRect.bottomRight();
        lines[ 1 ] += innerRect.topRight();

        painter->setPen(Qt::NoPen);

        painter->setBrush(brush.color().lighter());
        painter->drawPolygon(lines[ 0 ]);
        painter->setBrush(brush.color().darker());
        painter->drawPolygon(lines[ 1 ]);
    }

    painter->fillRect(rect.adjusted(lw, lw, -lw + 1, -lw + 1), brush);
}

class QwtColumnSymbol::PrivateData
{
public:
    PrivateData() : style(QwtColumnSymbol::Box), frameStyle(QwtColumnSymbol::Plain)
    {
    }

    QwtColumnSymbol::Style style;
    QwtColumnSymbol::FrameStyle frameStyle;
    // add by qwt 2,you can set pen and brush to draw bar
    QPen pen;
    QBrush brush;
};

/*!
   Constructor

   \param style Style of the symbol
   \sa setStyle(), style(), Style
 */
QwtColumnSymbol::QwtColumnSymbol(Style style)
{
    m_data        = new PrivateData();
    m_data->style = style;
    m_data->pen   = QPen(Qt::black, 1);
    m_data->brush = QBrush(Qt::gray);
}

//! Destructor
QwtColumnSymbol::~QwtColumnSymbol()
{
    delete m_data;
}

/*!
   Specify the symbol style

   \param style Style
   \sa style(), setPalette()
 */
void QwtColumnSymbol::setStyle(Style style)
{
    m_data->style = style;
}

/*!
   \return Current symbol style
   \sa setStyle()
 */
QwtColumnSymbol::Style QwtColumnSymbol::style() const
{
    return m_data->style;
}

/**
 * @brief Specify the outline pen
 * @param pen
 */
void QwtColumnSymbol::setPen(const QPen& pen)
{
    m_data->pen = pen;
}

/**
 * @brief  outline pen
 * @return
 */
QPen QwtColumnSymbol::pen() const
{
    return m_data->pen;
}

/**
 * @brief Specify draw brush
 * @param b
 */
void QwtColumnSymbol::setBrush(const QBrush& b)
{
    m_data->brush = b;
}

/**
 * @brief draw brush
 * @return
 */
QBrush QwtColumnSymbol::brush() const
{
    return m_data->brush;
}

/*!
   Set the frame, that is used for the Box style.

   \param frameStyle Frame style
   \sa frameStyle(), setLineWidth(), setStyle()
 */
void QwtColumnSymbol::setFrameStyle(FrameStyle frameStyle)
{
    m_data->frameStyle = frameStyle;
}

/*!
   \return Current frame style, that is used for the Box style.
   \sa setFrameStyle(), lineWidth(), setStyle()
 */
QwtColumnSymbol::FrameStyle QwtColumnSymbol::frameStyle() const
{
    return m_data->frameStyle;
}

/*!
   Set the line width of the frame, that is used for the Box style.

   \param width Width
   \sa lineWidth(), setFrameStyle()
 */
void QwtColumnSymbol::setLineWidth(int width)
{
    if (width < 0)
        width = 0;

    m_data->pen.setWidth(width);
}

/*!
   \return Line width of the frame, that is used for the Box style.
   \sa setLineWidth(), frameStyle(), setStyle()
 */
int QwtColumnSymbol::lineWidth() const
{
    return m_data->pen.width();
}

/*!
   Draw the symbol depending on its style.

   \param painter Painter
   \param rect Directed rectangle

   \sa drawBox()
 */
void QwtColumnSymbol::draw(QPainter* painter, const QwtColumnRect& rect) const
{
    painter->save();

    switch (m_data->style) {
    case QwtColumnSymbol::Box: {
        drawBox(painter, rect);
        break;
    }
    default:;
    }

    painter->restore();
}

/*!
   Draw the symbol when it is in Box style.

   \param painter Painter
   \param rect Directed rectangle

   \sa draw()
 */
void QwtColumnSymbol::drawBox(QPainter* painter, const QwtColumnRect& rect) const
{
    QRectF r = rect.toRect();
    if (QwtPainter::roundingAlignment(painter)) {
        r.setLeft(qRound(r.left()));
        r.setRight(qRound(r.right()));
        r.setTop(qRound(r.top()));
        r.setBottom(qRound(r.bottom()));
    }

    switch (m_data->frameStyle) {
    case QwtColumnSymbol::Raised: {
        qwtDrawPanel(painter, r, m_data->pen, m_data->brush);
        break;
    }
    case QwtColumnSymbol::Plain: {
        // qwtDrawBox(painter, r, m_data->palette, m_data->lineWidth);
        qwtDrawBox(painter, r, m_data->pen, m_data->brush);
        break;
    }
    default: {
        painter->fillRect(r.adjusted(0, 0, 1, 1), m_data->brush);
    }
    }
}

//! \return A normalized QRect built from the intervals
QRectF QwtColumnRect::toRect() const
{
    QRectF r(hInterval.minValue(),
             vInterval.minValue(),
             hInterval.maxValue() - hInterval.minValue(),
             vInterval.maxValue() - vInterval.minValue());

    r = r.normalized();

    if (hInterval.borderFlags() & QwtInterval::ExcludeMinimum)
        r.adjust(1, 0, 0, 0);

    if (hInterval.borderFlags() & QwtInterval::ExcludeMaximum)
        r.adjust(0, 0, -1, 0);

    if (vInterval.borderFlags() & QwtInterval::ExcludeMinimum)
        r.adjust(0, 1, 0, 0);

    if (vInterval.borderFlags() & QwtInterval::ExcludeMaximum)
        r.adjust(0, 0, 0, -1);

    return r;
}
