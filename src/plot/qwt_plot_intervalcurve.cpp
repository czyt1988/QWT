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

#include "qwt_plot_intervalcurve.h"
#include "qwt_plot.h"
#include "qwt_interval_symbol.h"
#include "qwt_scale_map.h"
#include "qwt_clipper.h"
#include "qwt_painter.h"
#include "qwt_graphic.h"
#include "qwt_text.h"

#include <qpainter.h>
#include <cstring>

static inline bool qwtIsHSampleInside(const QwtIntervalSample& sample, double xMin, double xMax, double yMin, double yMax)
{
    const double y  = sample.value;
    const double x1 = sample.interval.minValue();
    const double x2 = sample.interval.maxValue();

    const bool isOffScreen = (y < yMin) || (y > yMax) || (x1 < xMin && x2 < xMin) || (x1 > xMax && x2 > xMax);

    return !isOffScreen;
}

static inline bool qwtIsVSampleInside(const QwtIntervalSample& sample, double xMin, double xMax, double yMin, double yMax)
{
    const double x  = sample.value;
    const double y1 = sample.interval.minValue();
    const double y2 = sample.interval.maxValue();

    const bool isOffScreen = (x < xMin) || (x > xMax) || (y1 < yMin && y2 < yMin) || (y1 > yMax && y2 > yMax);

    return !isOffScreen;
}

class QwtPlotIntervalCurve::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotIntervalCurve)
public:
    PrivateData(QwtPlotIntervalCurve* p)
        : q_ptr(p), style(QwtPlotIntervalCurve::Tube), symbol(nullptr), pen(Qt::black), brush(Qt::NoBrush)
    {
        paintAttributes = QwtPlotIntervalCurve::ClipPolygons;
        paintAttributes |= QwtPlotIntervalCurve::ClipSymbol;

        pen.setCapStyle(Qt::FlatCap);
    }

    ~PrivateData()
    {
        delete symbol;
    }

    QwtPlotIntervalCurve::CurveStyle style;
    const QwtIntervalSymbol* symbol;

    QPen pen;
    QBrush brush;
    bool m_userSetPen = false;

    QwtPlotIntervalCurve::PaintAttributes paintAttributes;
};

/**
 * @brief Constructor
 * @param[in] title Title of the curve
 *
 */
QwtPlotIntervalCurve::QwtPlotIntervalCurve(const QwtText& title) : QwtPlotSeriesItem(title), QWT_PIMPL_CONSTRUCT
{
    init();
}

/**
 * @brief Constructor
 * @param[in] title Title of the curve
 *
 */
QwtPlotIntervalCurve::QwtPlotIntervalCurve(const QString& title)
    : QwtPlotSeriesItem(QwtText(title)), QWT_PIMPL_CONSTRUCT
{
    init();
}

/**
 * @brief Destructor
 *
 */
QwtPlotIntervalCurve::~QwtPlotIntervalCurve()
{
}

//! Initialize internal members
void QwtPlotIntervalCurve::init()
{
    setItemAttribute(QwtPlotItem::Legend, true);
    setItemAttribute(QwtPlotItem::AutoScale, true);
    setData(new QwtIntervalSeriesData());

    setZ(19.0);
}

/**
 * @brief Get the runtime type information
 * @return QwtPlotItem::Rtti_PlotIntervalCurve
 *
 */
int QwtPlotIntervalCurve::rtti() const
{
    return QwtPlotIntervalCurve::Rtti_PlotIntervalCurve;
}

/**
 * @brief Attach the interval curve to a plot
 * @details If the pen has not been explicitly set by the user, the interval curve
 *          automatically receives a color from the plot's color cycle.
 *          The brush is set to a semi-transparent version of the same color.
 * @param plot Plot to attach to (nullptr to detach)
 */
void QwtPlotIntervalCurve::attach(QwtPlot* plot)
{
    QWT_D(d);
    if (plot && !d->m_userSetPen && d->pen.color() == QColor(Qt::black)) {
        const QColor c = plot->nextColorForItem(rtti());
        d->pen         = QPen(c, d->pen.widthF(), d->pen.style());
        d->pen.setCapStyle(Qt::FlatCap);
        d->brush = QBrush(QColor(c.red(), c.green(), c.blue(), 128));
    }
    QwtPlotItem::attach(plot);
}

/**
 * @brief Specify an attribute how to draw the curve
 * @param[in] attribute Paint attribute
 * @param[in] on On/Off
 * @sa testPaintAttribute()
 *
 */
void QwtPlotIntervalCurve::setPaintAttribute(PaintAttribute attribute, bool on)
{
    QWT_D(d);
    if (on)
        d->paintAttributes |= attribute;
    else
        d->paintAttributes &= ~attribute;
}

/**
 * @brief Test if a paint attribute is enabled
 * @return True when attribute is enabled
 * @sa PaintAttribute, setPaintAttribute()
 *
 */
bool QwtPlotIntervalCurve::testPaintAttribute(PaintAttribute attribute) const
{
    QWT_DC(d);
    return (d->paintAttributes & attribute);
}

/**
 * @brief Initialize data with an array of samples
 * @param[in] samples Vector of samples
 *
 */
void QwtPlotIntervalCurve::setSamples(const QVector< QwtIntervalSample >& samples)
{
    setData(new QwtIntervalSeriesData(samples));
}

/**
 * @brief Assign a series of samples
 * @details setSamples() is just a wrapper for setData() without any additional
 *          value - beside that it is easier to find for the developer.
 * @param[in] data Data
 * @warning The item takes ownership of the data object, deleting
 *          it when its not used anymore.
 *
 */
void QwtPlotIntervalCurve::setSamples(QwtSeriesData< QwtIntervalSample >* data)
{
    setData(data);
}

/**
 * @brief Set the curve's drawing style
 * @param[in] style Curve style
 * @sa CurveStyle, style()
 *
 */
void QwtPlotIntervalCurve::setStyle(CurveStyle style)
{
    QWT_D(d);
    if (style != d->style) {
        d->style = style;

        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the curve's drawing style
 * @return Style of the curve
 * @sa setStyle()
 *
 */
QwtPlotIntervalCurve::CurveStyle QwtPlotIntervalCurve::style() const
{
    QWT_DC(d);
    return d->style;
}

/**
 * @brief Assign a symbol
 * @param[in] symbol Symbol
 * @sa symbol()
 *
 */
void QwtPlotIntervalCurve::setSymbol(const QwtIntervalSymbol* symbol)
{
    QWT_D(d);
    if (symbol != d->symbol) {
        delete d->symbol;
        d->symbol = symbol;

        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the current symbol
 * @return Current symbol or nullptr when no symbol has been assigned
 * @sa setSymbol()
 *
 */
const QwtIntervalSymbol* QwtPlotIntervalCurve::symbol() const
{
    QWT_DC(d);
    return d->symbol;
}

/**
 * @brief Build and assign a pen
 * @details In Qt5 the default pen width is 1.0 (0.0 in Qt4) what makes it
 *          non cosmetic (see QPen::isCosmetic()). This method has been introduced
 *          to hide this incompatibility.
 * @param[in] color Pen color
 * @param[in] width Pen width
 * @param[in] style Pen style
 * @sa pen(), brush()
 *
 */
void QwtPlotIntervalCurve::setPen(const QColor& color, qreal width, Qt::PenStyle style)
{
    setPen(QPen(color, width, style));
}

/**
 * @brief Assign a pen
 * @param[in] pen New pen
 * @sa pen(), brush()
 *
 */
void QwtPlotIntervalCurve::setPen(const QPen& pen)
{
    QWT_D(d);
    d->m_userSetPen = true;
    if (pen != d->pen) {
        d->pen = pen;

        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the pen used to draw the lines
 * @return Pen used to draw the lines
 * @sa setPen(), brush()
 *
 */
const QPen& QwtPlotIntervalCurve::pen() const
{
    QWT_DC(d);
    return d->pen;
}

/**
 * @brief Assign a brush
 * @details The brush is used to fill the area in Tube style().
 * @param[in] brush Brush
 * @sa brush(), pen(), setStyle(), CurveStyle
 *
 */
void QwtPlotIntervalCurve::setBrush(const QBrush& brush)
{
    QWT_D(d);
    if (brush != d->brush) {
        d->brush = brush;

        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the brush used to fill the area in Tube style()
 * @return Brush used to fill the area in Tube style()
 * @sa setBrush(), setStyle(), CurveStyle
 *
 */
const QBrush& QwtPlotIntervalCurve::brush() const
{
    QWT_DC(d);
    return d->brush;
}

/**
 * @brief Get the bounding rectangle of all samples
 * @return Bounding rectangle of all samples. For an empty series the rectangle is invalid.
 *
 */
QRectF QwtPlotIntervalCurve::boundingRect() const
{
    QRectF rect = QwtPlotSeriesItem::boundingRect();
    if (orientation() == Qt::Vertical)
        rect.setRect(rect.y(), rect.x(), rect.height(), rect.width());

    return rect;
}

/**
 * @brief Draw a subset of the samples
 * @param[in] painter Painter
 * @param[in] xMap Maps x-values into pixel coordinates
 * @param[in] yMap Maps y-values into pixel coordinates
 * @param[in] canvasRect Contents rectangle of the canvas
 * @param[in] from Index of the first sample to be painted
 * @param[in] to Index of the last sample to be painted. If to < 0, the series will be painted to its last sample.
 * @sa drawTube(), drawSymbols()
 *
 */
void QwtPlotIntervalCurve::drawSeries(QPainter* painter,
                                      const QwtScaleMap& xMap,
                                      const QwtScaleMap& yMap,
                                      const QRectF& canvasRect,
                                      int from,
                                      int to) const
{
    if (to < 0)
        to = dataSize() - 1;

    if (from < 0)
        from = 0;

    if (from > to)
        return;

    QWT_DC(d);
    switch (d->style) {
    case Tube:
        drawTube(painter, xMap, yMap, canvasRect, from, to);
        break;

    case NoCurve:
    default:
        break;
    }

    if (d->symbol && (d->symbol->style() != QwtIntervalSymbol::NoSymbol)) {
        drawSymbols(painter, *d->symbol, xMap, yMap, canvasRect, from, to);
    }
}

/*!
   Draw a tube

   Builds 2 curves from the upper and lower limits of the intervals
   and draws them with the pen(). The area between the curves is
   filled with the brush().

   @param painter Painter
   @param xMap Maps x-values into pixel coordinates.
   @param yMap Maps y-values into pixel coordinates.
   @param canvasRect Contents rectangle of the canvas
   @param from Index of the first sample to be painted
   @param to Index of the last sample to be painted. If to < 0 the
         series will be painted to its last sample.

   @sa drawSeries(), drawSymbols()
 */
void QwtPlotIntervalCurve::drawTube(QPainter* painter,
                                    const QwtScaleMap& xMap,
                                    const QwtScaleMap& yMap,
                                    const QRectF& canvasRect,
                                    int from,
                                    int to) const
{
    QWT_DC(d);
    const bool doAlign = QwtPainter::roundingAlignment(painter);

    painter->save();

    const size_t size = to - from + 1;
    QPolygonF polygon(2 * size);
    QPointF* points = polygon.data();

    // Pre-scan for first valid sample to seed leading-NaN fallback
    QPointF seedMin, seedMax;
    bool hasValid = false;
    for (uint k = 0; k < size; k++) {
        const QwtIntervalSample s = sample(from + k);
        if (!isSampleNanOrInf(s)) {
            if (orientation() == Qt::Vertical) {
                double x  = xMap.transform(s.value);
                double y1 = yMap.transform(s.interval.minValue());
                double y2 = yMap.transform(s.interval.maxValue());
                if (doAlign) {
                    x  = qRound(x);
                    y1 = qRound(y1);
                    y2 = qRound(y2);
                }
                seedMin = QPointF(x, y1);
                seedMax = QPointF(x, y2);
            } else {
                double y  = yMap.transform(s.value);
                double x1 = xMap.transform(s.interval.minValue());
                double x2 = xMap.transform(s.interval.maxValue());
                if (doAlign) {
                    y  = qRound(y);
                    x1 = qRound(x1);
                    x2 = qRound(x2);
                }
                seedMin = QPointF(x1, y);
                seedMax = QPointF(x2, y);
            }
            hasValid = true;
            break;
        }
    }
    if (!hasValid) {
        painter->restore();
        return;
    }

    for (uint i = 0; i < size; i++) {
        QPointF& minValue = points[ i ];
        QPointF& maxValue = points[ 2 * size - 1 - i ];

        const QwtIntervalSample intervalSample = sample(from + i);

        if (isSampleNanOrInf(intervalSample)) {
            if (i > 0) {
                minValue = points[ i - 1 ];
                maxValue = points[ 2 * size - i ];
            } else {
                minValue = seedMin;
                maxValue = seedMax;
            }
            continue;
        }

        if (orientation() == Qt::Vertical) {
            double x  = xMap.transform(intervalSample.value);
            double y1 = yMap.transform(intervalSample.interval.minValue());
            double y2 = yMap.transform(intervalSample.interval.maxValue());
            if (doAlign) {
                x  = qRound(x);
                y1 = qRound(y1);
                y2 = qRound(y2);
            }

            minValue.rx() = x;
            minValue.ry() = y1;
            maxValue.rx() = x;
            maxValue.ry() = y2;
        } else {
            double y  = yMap.transform(intervalSample.value);
            double x1 = xMap.transform(intervalSample.interval.minValue());
            double x2 = xMap.transform(intervalSample.interval.maxValue());
            if (doAlign) {
                y  = qRound(y);
                x1 = qRound(x1);
                x2 = qRound(x2);
            }

            minValue.rx() = x1;
            minValue.ry() = y;
            maxValue.rx() = x2;
            maxValue.ry() = y;
        }
    }

    if (d->brush.style() != Qt::NoBrush) {
        painter->setPen(QPen(Qt::NoPen));
        painter->setBrush(d->brush);

        if (d->paintAttributes & ClipPolygons) {
            const qreal m     = 1.0;
            const QPolygonF p = QwtClipper::clippedPolygonF(canvasRect.adjusted(-m, -m, m, m), polygon, true);

            QwtPainter::drawPolygon(painter, p);
        } else {
            QwtPainter::drawPolygon(painter, polygon);
        }
    }

    if (d->pen.style() != Qt::NoPen) {
        painter->setPen(d->pen);
        painter->setBrush(Qt::NoBrush);

        if (d->paintAttributes & ClipPolygons) {
            qreal pw              = QwtPainter::effectivePenWidth(painter->pen());
            const QRectF clipRect = canvasRect.adjusted(-pw, -pw, pw, pw);

            QPolygonF p(size);

            std::memcpy(p.data(), points, size * sizeof(QPointF));
            QwtPainter::drawPolyline(painter, QwtClipper::clippedPolygonF(clipRect, p));

            std::memcpy(p.data(), points + size, size * sizeof(QPointF));
            QwtPainter::drawPolyline(painter, QwtClipper::clippedPolygonF(clipRect, p));
        } else {
            QwtPainter::drawPolyline(painter, points, size);
            QwtPainter::drawPolyline(painter, points + size, size);
        }
    }

    painter->restore();
}

/*!
   Draw symbols for a subset of the samples

   @param painter Painter
   @param symbol Interval symbol
   @param xMap x map
   @param yMap y map
   @param canvasRect Contents rectangle of the canvas
   @param from Index of the first sample to be painted
   @param to Index of the last sample to be painted

   @sa setSymbol(), drawSeries(), drawTube()
 */
void QwtPlotIntervalCurve::drawSymbols(QPainter* painter,
                                       const QwtIntervalSymbol& symbol,
                                       const QwtScaleMap& xMap,
                                       const QwtScaleMap& yMap,
                                       const QRectF& canvasRect,
                                       int from,
                                       int to) const
{
    QWT_DC(d);
    painter->save();

    QPen pen = symbol.pen();
    pen.setCapStyle(Qt::FlatCap);

    painter->setPen(pen);
    painter->setBrush(symbol.brush());

    const QRectF tr = QwtScaleMap::invTransform(xMap, yMap, canvasRect);

    const double xMin = tr.left();
    const double xMax = tr.right();
    const double yMin = tr.top();
    const double yMax = tr.bottom();

    const bool doClip = d->paintAttributes & ClipSymbol;

    for (int i = from; i <= to; i++) {
        const QwtIntervalSample s = sample(i);

        if (isSampleNanOrInf(s))
            continue;

        if (orientation() == Qt::Vertical) {
            if (!doClip || qwtIsVSampleInside(s, xMin, xMax, yMin, yMax)) {
                const double x  = xMap.transform(s.value);
                const double y1 = yMap.transform(s.interval.minValue());
                const double y2 = yMap.transform(s.interval.maxValue());

                symbol.draw(painter, orientation(), QPointF(x, y1), QPointF(x, y2));
            }
        } else {
            if (!doClip || qwtIsHSampleInside(s, xMin, xMax, yMin, yMax)) {
                const double y  = yMap.transform(s.value);
                const double x1 = xMap.transform(s.interval.minValue());
                const double x2 = xMap.transform(s.interval.maxValue());

                symbol.draw(painter, orientation(), QPointF(x1, y), QPointF(x2, y));
            }
        }
    }

    painter->restore();
}

/**
 * @brief Get the icon for the legend
 * @details In case of Tube style() the icon is a plain rectangle filled with the brush().
 *          If a symbol is assigned it is scaled to size.
 * @param[in] index Index of the legend entry (ignored as there is only one)
 * @param[in] size Icon size
 * @return Icon for the legend
 * @sa QwtPlotItem::setLegendIconSize(), QwtPlotItem::legendData()
 *
 */
QwtGraphic QwtPlotIntervalCurve::legendIcon(int index, const QSizeF& size) const
{
    QWT_DC(d);
    Q_UNUSED(index);

    if (size.isEmpty())
        return QwtGraphic();

    QwtGraphic icon;
    icon.setDefaultSize(size);
    icon.setRenderHint(QwtGraphic::RenderPensUnscaled, true);

    QPainter painter(&icon);
    painter.setRenderHint(QPainter::Antialiasing, testRenderHint(QwtPlotItem::RenderAntialiased));

    if (d->style == Tube) {
        QRectF r(0, 0, size.width(), size.height());
        painter.fillRect(r, d->brush);
    }

    if (d->symbol && (d->symbol->style() != QwtIntervalSymbol::NoSymbol)) {
        QPen pen = d->symbol->pen();
        pen.setWidthF(pen.widthF());
        pen.setCapStyle(Qt::FlatCap);

        painter.setPen(pen);
        painter.setBrush(d->symbol->brush());

        if (orientation() == Qt::Vertical) {
            const double x = 0.5 * size.width();

            d->symbol->draw(&painter, orientation(), QPointF(x, 0), QPointF(x, size.height() - 1.0));
        } else {
            const double y = 0.5 * size.height();

            d->symbol->draw(&painter, orientation(), QPointF(0.0, y), QPointF(size.width() - 1.0, y));
        }
    }

    return icon;
}
