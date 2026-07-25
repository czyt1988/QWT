/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *
 * Modified by ChenZongYan in 2024 <czy.t@163.com>
 *****************************************************************************/

#include "qwt_plot_boxchart.h"
#include "qwt_plot.h"
#include "qwt_scale_map.h"
#include "qwt_painter.h"
#include "qwt_symbol.h"
#include "qwt_graphic.h"
#include "qwt_math.h"
#include "qwt_text.h"

#include <qpainter.h>
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <qrandom.h>
#else
#include <qdatetime.h>
#endif

class QwtPlotBoxChart::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotBoxChart)

public:
    PrivateData(QwtPlotBoxChart* p)
        : q_ptr(p)
        , boxStyle(Rect)
        , whiskerStyle(StandardWhisker)
        , orientation(Qt::Vertical)
        , boxExtent(0.6)
        , minBoxWidth(2.0)
        , maxBoxWidth(-1.0)
        , medianVisible(true)
        , meanVisible(false)
        , outlierJitter(0.0)
        , paintAttributes(ClipBoxes | ClipOutliers)
    {
        pen           = QPen(Qt::black, 1.0);
        brush         = QBrush(Qt::NoBrush);
        medianPen     = QPen(Qt::black, 2.0);
        outlierSymbol = nullptr;
        meanSymbol    = nullptr;
    }

    BoxStyle boxStyle;
    WhiskerStyle whiskerStyle;
    Qt::Orientation orientation;
    double boxExtent;
    double minBoxWidth;
    double maxBoxWidth;
    QPen pen;
    QPen medianPen;
    QBrush brush;
    const QwtSymbol* outlierSymbol;
    const QwtSymbol* meanSymbol;
    bool medianVisible;
    bool meanVisible;
    double outlierJitter;
    PaintAttributes paintAttributes;
    bool m_userSetPen = false;
};

/**
 * @brief Constructor
 * @param[in] title Title of the chart
 *
 */
QwtPlotBoxChart::QwtPlotBoxChart(const QString& title)
    : QwtPlotSeriesItem(QwtText(title)), QWT_PIMPL_CONSTRUCT, m_outlierData(nullptr)
{
    init();
}

/**
 * @brief Constructor
 * @param[in] title Title of the chart
 *
 */
QwtPlotBoxChart::QwtPlotBoxChart(const QwtText& title)
    : QwtPlotSeriesItem(title), QWT_PIMPL_CONSTRUCT, m_outlierData(nullptr)
{
    init();
}

/**
 * @brief Destructor
 *
 */
QwtPlotBoxChart::~QwtPlotBoxChart()
{
    delete m_outlierData;
}

void QwtPlotBoxChart::init()
{
    setItemAttribute(QwtPlotItem::Legend, true);
    setItemAttribute(QwtPlotItem::AutoScale, true);

    setData(new QwtBoxChartData());
    setZ(20.0);
}

/**
 * @brief Get the runtime type information
 * @return Rtti_PlotBoxChart
 *
 */
int QwtPlotBoxChart::rtti() const
{
    return Rtti_PlotBoxChart;
}

/**
 * @brief Attach the box chart to a plot
 * @details If the pen has not been explicitly set by the user, the box chart
 *          automatically receives a color from the plot's color cycle.
 *          The brush is set to a semi-transparent version of the same color.
 * @param plot Plot to attach to (nullptr to detach)
 */
void QwtPlotBoxChart::attach(QwtPlot* plot)
{
    QWT_D(d);
    if (plot && !d->m_userSetPen && d->pen.color() == QColor(Qt::black)) {
        const QColor c = plot->nextColorForItem(rtti());
        d->pen         = QPen(c, d->pen.widthF(), d->pen.style());
        d->brush       = QBrush(QColor(c.red(), c.green(), c.blue(), 128));
    }
    QwtPlotItem::attach(plot);
}

/**
 * @brief Set a paint attribute
 * @param[in] attr Paint attribute to set
 * @param[in] on True to enable, false to disable
 *
 */
void QwtPlotBoxChart::setPaintAttribute(PaintAttribute attr, bool on)
{
    QWT_D(d);
    if (on)
        d->paintAttributes |= attr;
    else
        d->paintAttributes &= ~attr;
}

/**
 * @brief Test if a paint attribute is enabled
 * @param[in] attr Paint attribute to test
 * @return True if the attribute is enabled
 *
 */
bool QwtPlotBoxChart::testPaintAttribute(PaintAttribute attr) const
{
    QWT_DC(d);
    return (d->paintAttributes & attr);
}

/**
 * @brief Set the box style
 * @param[in] style Box style to set
 *
 */
void QwtPlotBoxChart::setBoxStyle(BoxStyle style)
{
    QWT_D(d);
    if (d->boxStyle != style) {
        d->boxStyle = style;
        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the box style
 * @return Current box style
 *
 */
QwtPlotBoxChart::BoxStyle QwtPlotBoxChart::boxStyle() const
{
    QWT_DC(d);
    return d->boxStyle;
}

/**
 * @brief Set the whisker style
 * @param[in] style Whisker style to set
 *
 */
void QwtPlotBoxChart::setWhiskerStyle(WhiskerStyle style)
{
    QWT_D(d);
    if (d->whiskerStyle != style) {
        d->whiskerStyle = style;
        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the whisker style
 * @return Current whisker style
 *
 */
QwtPlotBoxChart::WhiskerStyle QwtPlotBoxChart::whiskerStyle() const
{
    QWT_DC(d);
    return d->whiskerStyle;
}

/**
 * @brief Set the orientation
 * @details Vertical orientation means x-position, horizontal means y-position.
 * @param[in] orient Orientation to set
 *
 */
void QwtPlotBoxChart::setOrientation(Qt::Orientation orient)
{
    QWT_D(d);
    if (d->orientation != orient) {
        d->orientation = orient;
        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the orientation
 * @return Current orientation
 *
 */
Qt::Orientation QwtPlotBoxChart::orientation() const
{
    QWT_DC(d);
    return d->orientation;
}

/**
 * @brief Set the box extent (width in scale coordinates)
 * @param[in] extent Box extent to set
 *
 */
void QwtPlotBoxChart::setBoxExtent(double extent)
{
    QWT_D(d);
    extent = qwtMaxF(0.0, extent);
    if (d->boxExtent != extent) {
        d->boxExtent = extent;
        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the box extent
 * @return Current box extent
 *
 */
double QwtPlotBoxChart::boxExtent() const
{
    QWT_DC(d);
    return d->boxExtent;
}

/**
 * @brief Set the minimum box width in pixels
 * @param[in] pixels Minimum width to set
 *
 */
void QwtPlotBoxChart::setMinBoxWidth(double pixels)
{
    QWT_D(d);
    pixels = qwtMaxF(0.0, pixels);
    if (d->minBoxWidth != pixels) {
        d->minBoxWidth = pixels;
        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the minimum box width
 * @return Minimum box width in pixels
 *
 */
double QwtPlotBoxChart::minBoxWidth() const
{
    QWT_DC(d);
    return d->minBoxWidth;
}

/**
 * @brief Set the maximum box width in pixels
 * @param[in] pixels Maximum width to set (negative = unlimited)
 *
 */
void QwtPlotBoxChart::setMaxBoxWidth(double pixels)
{
    QWT_D(d);
    if (d->maxBoxWidth != pixels) {
        d->maxBoxWidth = pixels;
        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the maximum box width
 * @return Maximum box width in pixels (negative = unlimited)
 *
 */
double QwtPlotBoxChart::maxBoxWidth() const
{
    QWT_DC(d);
    return d->maxBoxWidth;
}

/**
 * @brief Set the pen for box outline and whiskers
 * @param[in] color Pen color
 * @param[in] width Pen width
 * @param[in] style Pen style
 *
 */
void QwtPlotBoxChart::setPen(const QColor& color, qreal width, Qt::PenStyle style)
{
    setPen(QPen(color, width, style));
}

/**
 * @brief Set the pen for box outline and whiskers
 * @param[in] pen Pen to set
 *
 */
void QwtPlotBoxChart::setPen(const QPen& pen)
{
    QWT_D(d);
    d->m_userSetPen = true;
    if (d->pen != pen) {
        d->pen = pen;
        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the pen for box outline and whiskers
 * @return Current pen
 *
 */
const QPen& QwtPlotBoxChart::pen() const
{
    QWT_DC(d);
    return d->pen;
}

/**
 * @brief Set the brush for box body fill
 * @param[in] brush Brush to set
 *
 */
void QwtPlotBoxChart::setBrush(const QBrush& brush)
{
    QWT_D(d);
    if (d->brush != brush) {
        d->brush = brush;
        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the brush for box body fill
 * @return Current brush
 *
 */
const QBrush& QwtPlotBoxChart::brush() const
{
    QWT_DC(d);
    return d->brush;
}

/**
 * @brief Set the pen for median line
 * @param[in] pen Pen to set
 *
 */
void QwtPlotBoxChart::setMedianPen(const QPen& pen)
{
    QWT_D(d);
    if (d->medianPen != pen) {
        d->medianPen = pen;
        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the pen for median line
 * @return Current median pen
 *
 */
QPen QwtPlotBoxChart::medianPen() const
{
    QWT_DC(d);
    return d->medianPen;
}

/**
 * @brief Set the symbol for outliers
 * @param[in] symbol Symbol to set
 *
 */
void QwtPlotBoxChart::setOutlierSymbol(const QwtSymbol* symbol)
{
    QWT_D(d);
    if (d->outlierSymbol != symbol) {
        delete d->outlierSymbol;
        d->outlierSymbol = symbol;
        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the symbol for outliers
 * @return Current outlier symbol
 *
 */
const QwtSymbol* QwtPlotBoxChart::outlierSymbol() const
{
    QWT_DC(d);
    return d->outlierSymbol;
}

/**
 * @brief Set the symbol for mean marker
 * @param[in] symbol Symbol to set
 *
 */
void QwtPlotBoxChart::setMeanSymbol(const QwtSymbol* symbol)
{
    QWT_D(d);
    if (d->meanSymbol != symbol) {
        delete d->meanSymbol;
        d->meanSymbol = symbol;
        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the symbol for mean marker
 * @return Current mean symbol
 *
 */
const QwtSymbol* QwtPlotBoxChart::meanSymbol() const
{
    QWT_DC(d);
    return d->meanSymbol;
}

/**
 * @brief Set whether the median line is visible
 * @param[in] visible True to show, false to hide
 *
 */
void QwtPlotBoxChart::setMedianVisible(bool visible)
{
    QWT_D(d);
    if (d->medianVisible != visible) {
        d->medianVisible = visible;
        itemChanged();
    }
}

/**
 * @brief Check if the median line is visible
 * @return True if visible
 *
 */
bool QwtPlotBoxChart::isMedianVisible() const
{
    QWT_DC(d);
    return d->medianVisible;
}

/**
 * @brief Set whether the mean marker is visible
 * @param[in] visible True to show, false to hide
 *
 */
void QwtPlotBoxChart::setMeanVisible(bool visible)
{
    QWT_D(d);
    if (d->meanVisible != visible) {
        d->meanVisible = visible;
        itemChanged();
    }
}

/**
 * @brief Check if the mean marker is visible
 * @return True if visible
 *
 */
bool QwtPlotBoxChart::isMeanVisible() const
{
    QWT_DC(d);
    return d->meanVisible;
}

/**
 * @brief Set the outlier jitter width
 * @param[in] jitterWidth Jitter width for overlapping outliers
 *
 */
void QwtPlotBoxChart::setOutlierJitter(double jitterWidth)
{
    QWT_D(d);
    d->outlierJitter = qwtMaxF(0.0, jitterWidth);
}

/**
 * @brief Get the outlier jitter width
 * @return Jitter width for overlapping outliers
 *
 */
double QwtPlotBoxChart::outlierJitter() const
{
    QWT_DC(d);
    return d->outlierJitter;
}

/**
 * @brief Set box samples from a vector
 * @param[in] samples Vector of box samples
 *
 */
void QwtPlotBoxChart::setSamples(const QVector< QwtBoxSample >& samples)
{
    setData(new QwtBoxChartData(samples));
}

/**
 * @brief Set box samples from series data
 * @param[in] data Series data to set
 *
 */
void QwtPlotBoxChart::setSamples(QwtSeriesData< QwtBoxSample >* data)
{
    setData(data);
}

/**
 * @brief Set outlier samples from a vector
 * @param[in] samples Vector of outlier samples
 *
 */
void QwtPlotBoxChart::setOutliers(const QVector< QwtBoxOutlierSample >& samples)
{
    delete m_outlierData;
    m_outlierData = new QwtBoxOutlierChartData(samples);
    itemChanged();
}

/**
 * @brief Set outlier samples from series data
 * @param[in] data Series data to set
 *
 */
void QwtPlotBoxChart::setOutliers(QwtSeriesData< QwtBoxOutlierSample >* data)
{
    delete m_outlierData;
    m_outlierData = data;
    itemChanged();
}

/**
 * @brief Get the outlier data
 * @return Current outlier data
 *
 */
const QwtSeriesData< QwtBoxOutlierSample >* QwtPlotBoxChart::outlierData() const
{
    return m_outlierData;
}

/**
 * @brief Get the bounding rectangle
 * @return Bounding rectangle of all samples
 *
 */
QRectF QwtPlotBoxChart::boundingRect() const
{
    QWT_DC(d);
    QRectF rect = QwtPlotSeriesItem::boundingRect();

    if (rect.isValid()) {
        const double padding = d->boxExtent * 0.5;
        if (d->orientation == Qt::Vertical) {
            rect.setLeft(rect.left() - padding);
            rect.setRight(rect.right() + padding);
        } else {
            rect.setTop(rect.top() - padding);
            rect.setBottom(rect.bottom() + padding);
        }
    }

    if (m_outlierData && m_outlierData->size() > 0) {
        const QRectF outlierRect = m_outlierData->boundingRect();
        if (outlierRect.isValid()) {
            rect = rect.united(outlierRect);
        }
    }

    return rect;
}

double QwtPlotBoxChart::scaledBoxWidth(const QwtScaleMap& posMap, const QwtScaleMap& valueMap, const QRectF& canvasRect) const
{
    QWT_DC(d);
    Q_UNUSED(valueMap);
    Q_UNUSED(canvasRect);

    if (d->maxBoxWidth > 0.0 && d->minBoxWidth >= d->maxBoxWidth)
        return d->minBoxWidth;

    const double pos = posMap.transform(posMap.s1() + d->boxExtent);
    double width     = qAbs(pos - posMap.p1());

    width = qwtMaxF(width, d->minBoxWidth);
    if (d->maxBoxWidth > 0.0)
        width = qwtMinF(width, d->maxBoxWidth);

    return width;
}

/**
 * @brief Draw the series
 * @param[in] painter Painter
 * @param[in] xMap X-axis scale map
 * @param[in] yMap Y-axis scale map
 * @param[in] canvasRect Canvas rectangle
 * @param[in] from Starting index
 * @param[in] to Ending index
 *
 */
void QwtPlotBoxChart::drawSeries(QPainter* painter,
                                 const QwtScaleMap& xMap,
                                 const QwtScaleMap& yMap,
                                 const QRectF& canvasRect,
                                 int from,
                                 int to) const
{
    QWT_DC(d);

    if (to < 0)
        to = dataSize() - 1;

    if (from < 0)
        from = 0;

    if (from > to || dataSize() == 0)
        return;

    painter->save();

    const Qt::Orientation orient = d->orientation;
    const QwtScaleMap* posMap    = (orient == Qt::Vertical) ? &xMap : &yMap;
    const QwtScaleMap* valueMap  = (orient == Qt::Vertical) ? &yMap : &xMap;

    const double boxWidth = scaledBoxWidth(*posMap, *valueMap, canvasRect);
    const bool doAlign    = QwtPainter::roundingAlignment(painter);

    painter->setPen(d->pen);

    for (int i = from; i <= to; i++) {
        const QwtBoxSample& sample = this->sample(i);

        if (isSampleNanOrInf(sample))
            continue;

        double posPixel = posMap->transform(sample.position);
        if (doAlign)
            posPixel = qRound(posPixel);

        if (d->boxStyle != NoBox) {
            painter->setBrush(d->brush);
            drawBox(painter, sample, orient, boxWidth, posPixel, *valueMap);
        }

        if (d->whiskerStyle != NoWhiskers) {
            drawWhiskers(painter, sample, orient, boxWidth, posPixel, *valueMap);
        }

        if (d->medianVisible) {
            drawMedian(painter, sample, orient, boxWidth, posPixel, *valueMap);
        }

        if (d->meanVisible && d->meanSymbol) {
            double medianPixel = valueMap->transform(sample.median);
            if (doAlign)
                medianPixel = qRound(medianPixel);

            d->meanSymbol->drawSymbol(
                painter, (orient == Qt::Vertical) ? QPointF(posPixel, medianPixel) : QPointF(medianPixel, posPixel));
        }
    }

    if (m_outlierData && m_outlierData->size() > 0) {
        drawOutliers(painter, *posMap, *valueMap, canvasRect, from, to);
    }

    painter->restore();
}

void QwtPlotBoxChart::drawBox(QPainter* painter,
                              const QwtBoxSample& sample,
                              Qt::Orientation orient,
                              double boxWidth,
                              double posPixel,
                              const QwtScaleMap& valueMap) const
{
    QWT_DC(d);
    const bool doAlign = QwtPainter::roundingAlignment(painter);

    const double q1Pixel     = valueMap.transform(sample.q1);
    const double q3Pixel     = valueMap.transform(sample.q3);
    const double medianPixel = valueMap.transform(sample.median);

    if (doAlign) {
        const double aligned = qRound(posPixel);
        posPixel             = aligned;
    }

    const double halfWidth = boxWidth * 0.5;

    switch (d->boxStyle) {
    case Rect: {
        if (orient == Qt::Vertical) {
            QRectF rect(posPixel - halfWidth, q3Pixel, boxWidth, q1Pixel - q3Pixel);
            QwtPainter::drawRect(painter, rect);
        } else {
            QRectF rect(q1Pixel, posPixel - halfWidth, q3Pixel - q1Pixel, boxWidth);
            QwtPainter::drawRect(painter, rect);
        }
        break;
    }

    case Diamond: {
        QPolygonF poly(4);
        if (orient == Qt::Vertical) {
            poly[ 0 ] = QPointF(posPixel, q3Pixel);
            poly[ 1 ] = QPointF(posPixel + halfWidth, medianPixel);
            poly[ 2 ] = QPointF(posPixel, q1Pixel);
            poly[ 3 ] = QPointF(posPixel - halfWidth, medianPixel);
        } else {
            poly[ 0 ] = QPointF(q3Pixel, posPixel);
            poly[ 1 ] = QPointF(medianPixel, posPixel + halfWidth);
            poly[ 2 ] = QPointF(q1Pixel, posPixel);
            poly[ 3 ] = QPointF(medianPixel, posPixel - halfWidth);
        }
        QwtPainter::drawPolygon(painter, poly);
        break;
    }

    case Notch: {
        const double notchWidth  = halfWidth * 0.25;
        const double notchOffset = (q3Pixel - q1Pixel) * 0.1;

        QPolygonF poly(10);
        if (orient == Qt::Vertical) {
            poly[ 0 ] = QPointF(posPixel - halfWidth, q3Pixel);
            poly[ 1 ] = QPointF(posPixel - halfWidth, medianPixel - notchOffset);
            poly[ 2 ] = QPointF(posPixel - notchWidth, medianPixel);
            poly[ 3 ] = QPointF(posPixel - halfWidth, medianPixel + notchOffset);
            poly[ 4 ] = QPointF(posPixel - halfWidth, q1Pixel);
            poly[ 5 ] = QPointF(posPixel + halfWidth, q1Pixel);
            poly[ 6 ] = QPointF(posPixel + halfWidth, medianPixel + notchOffset);
            poly[ 7 ] = QPointF(posPixel + notchWidth, medianPixel);
            poly[ 8 ] = QPointF(posPixel + halfWidth, medianPixel - notchOffset);
            poly[ 9 ] = QPointF(posPixel + halfWidth, q3Pixel);
        } else {
            poly[ 0 ] = QPointF(q1Pixel, posPixel - halfWidth);
            poly[ 1 ] = QPointF(medianPixel - notchOffset, posPixel - halfWidth);
            poly[ 2 ] = QPointF(medianPixel, posPixel - notchWidth);
            poly[ 3 ] = QPointF(medianPixel + notchOffset, posPixel - halfWidth);
            poly[ 4 ] = QPointF(q3Pixel, posPixel - halfWidth);
            poly[ 5 ] = QPointF(q3Pixel, posPixel + halfWidth);
            poly[ 6 ] = QPointF(medianPixel + notchOffset, posPixel + halfWidth);
            poly[ 7 ] = QPointF(medianPixel, posPixel + notchWidth);
            poly[ 8 ] = QPointF(medianPixel - notchOffset, posPixel + halfWidth);
            poly[ 9 ] = QPointF(q1Pixel, posPixel + halfWidth);
        }
        QwtPainter::drawPolygon(painter, poly);
        break;
    }

    default:
        break;
    }
}

void QwtPlotBoxChart::drawWhiskers(QPainter* painter,
                                   const QwtBoxSample& sample,
                                   Qt::Orientation orient,
                                   double boxWidth,
                                   double posPixel,
                                   const QwtScaleMap& valueMap) const
{
    QWT_DC(d);
    const bool doAlign = QwtPainter::roundingAlignment(painter);

    const double whiskerLowerPixel = valueMap.transform(sample.whiskerLower);
    const double whiskerUpperPixel = valueMap.transform(sample.whiskerUpper);
    const double q1Pixel           = valueMap.transform(sample.q1);
    const double q3Pixel           = valueMap.transform(sample.q3);

    if (doAlign) { }

    const double capWidth = boxWidth * 0.3;

    QPen whiskerPen = d->pen;
    whiskerPen.setCapStyle(Qt::FlatCap);
    painter->setPen(whiskerPen);

    switch (d->whiskerStyle) {
    case StandardWhisker: {
        if (orient == Qt::Vertical) {
            QwtPainter::drawLine(painter, posPixel, whiskerLowerPixel, posPixel, q1Pixel);
            QwtPainter::drawLine(
                painter, posPixel - capWidth * 0.5, whiskerLowerPixel, posPixel + capWidth * 0.5, whiskerLowerPixel);

            QwtPainter::drawLine(painter, posPixel, q3Pixel, posPixel, whiskerUpperPixel);
            QwtPainter::drawLine(
                painter, posPixel - capWidth * 0.5, whiskerUpperPixel, posPixel + capWidth * 0.5, whiskerUpperPixel);
        } else {
            QwtPainter::drawLine(painter, whiskerLowerPixel, posPixel, q1Pixel, posPixel);
            QwtPainter::drawLine(
                painter, whiskerLowerPixel, posPixel - capWidth * 0.5, whiskerLowerPixel, posPixel + capWidth * 0.5);

            QwtPainter::drawLine(painter, q3Pixel, posPixel, whiskerUpperPixel, posPixel);
            QwtPainter::drawLine(
                painter, whiskerUpperPixel, posPixel - capWidth * 0.5, whiskerUpperPixel, posPixel + capWidth * 0.5);
        }
        break;
    }

    case MinMaxLine: {
        if (orient == Qt::Vertical) {
            QwtPainter::drawLine(painter, posPixel, whiskerLowerPixel, posPixel, whiskerUpperPixel);
        } else {
            QwtPainter::drawLine(painter, whiskerLowerPixel, posPixel, whiskerUpperPixel, posPixel);
        }
        break;
    }

    default:
        break;
    }
}

void QwtPlotBoxChart::drawMedian(QPainter* painter,
                                 const QwtBoxSample& sample,
                                 Qt::Orientation orient,
                                 double boxWidth,
                                 double posPixel,
                                 const QwtScaleMap& valueMap) const
{
    QWT_DC(d);
    const double medianPixel = valueMap.transform(sample.median);
    const bool doAlign       = QwtPainter::roundingAlignment(painter);

    if (doAlign) { }

    const double halfWidth = boxWidth * 0.5;

    QPen medPen = d->medianPen;
    medPen.setCapStyle(Qt::FlatCap);
    painter->setPen(medPen);

    double lineHalfWidth = halfWidth;
    if (d->boxStyle == Diamond)
        lineHalfWidth *= 0.5;
    else if (d->boxStyle == Notch)
        lineHalfWidth *= 0.8;

    if (orient == Qt::Vertical) {
        QwtPainter::drawLine(painter, posPixel - lineHalfWidth, medianPixel, posPixel + lineHalfWidth, medianPixel);
    } else {
        QwtPainter::drawLine(painter, medianPixel, posPixel - lineHalfWidth, medianPixel, posPixel + lineHalfWidth);
    }
}

void QwtPlotBoxChart::drawOutliers(QPainter* painter,
                                   const QwtScaleMap& posMap,
                                   const QwtScaleMap& valueMap,
                                   const QRectF& canvasRect,
                                   int from,
                                   int to) const
{
    QWT_DC(d);

    if (!m_outlierData || m_outlierData->size() == 0)
        return;

    const QwtSymbol* symbol = d->outlierSymbol;
    if (!symbol) {
        static QwtSymbol defaultSymbol(QwtSymbol::XCross, QBrush(), QPen(Qt::black), QSize(8, 8));
        symbol = &defaultSymbol;
    }

    const Qt::Orientation orient = d->orientation;
    const bool doAlign           = QwtPainter::roundingAlignment(painter);
    const double jitter          = d->outlierJitter;

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    QRandomGenerator* rng = nullptr;
    if (jitter > 0)
        rng = QRandomGenerator::global();
#else
    if (jitter > 0)
        qsrand(static_cast<uint>(QTime::currentTime().msecsSinceStartOfDay()));
#endif

    for (size_t i = 0; i < m_outlierData->size(); ++i) {
        const QwtBoxOutlierSample& outlierSample = m_outlierData->sample(i);

        double basePosPixel = posMap.transform(outlierSample.boxPosition);
        if (doAlign)
            basePosPixel = qRound(basePosPixel);

        for (int j = 0; j < outlierSample.count(); ++j) {
            double valuePixel = valueMap.transform(outlierSample.values[ j ]);
            if (doAlign)
                valuePixel = qRound(valuePixel);

            double posPixel = basePosPixel;
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            if (jitter > 0 && rng) {
                const double offset = rng->bounded(jitter) - jitter * 0.5;
                posPixel += offset;
            }
#else
            if (jitter > 0)
            {
                const double offset = (static_cast<double>(qrand()) / RAND_MAX) * jitter - jitter * 0.5;
                posPixel += offset;
            }
#endif

            QPointF point = (orient == Qt::Vertical) ? QPointF(posPixel, valuePixel) : QPointF(valuePixel, posPixel);

            if (d->paintAttributes & ClipOutliers) {
                if (!canvasRect.contains(point))
                    continue;
            }

            symbol->drawSymbol(painter, point);
        }
    }
}

void QwtPlotBoxChart::drawOutlierSymbol(QPainter* painter, double posPixel, double valuePixel, const QwtSymbol& symbol) const
{
    QWT_DC(d);
    const Qt::Orientation orient = d->orientation;
    QPointF point = (orient == Qt::Vertical) ? QPointF(posPixel, valuePixel) : QPointF(valuePixel, posPixel);
    symbol.drawSymbol(painter, point);
}

/**
 * @brief Get the legend icon
 * @param[in] index Legend entry index
 * @param[in] size Icon size
 * @return Legend icon graphic
 *
 */
QwtGraphic QwtPlotBoxChart::legendIcon(int index, const QSizeF& size) const
{
    QWT_DC(d);
    Q_UNUSED(index);

    QwtGraphic graphic;
    graphic.setDefaultSize(size);

    QPainter painter(&graphic);

    const QRectF rect(0, 0, size.width(), size.height());

    const double centerX   = rect.center().x();
    const double centerY   = rect.center().y();
    const double boxHeight = rect.height() * 0.4;
    const double boxWidth  = rect.width() * 0.3;

    painter.setPen(d->pen);
    painter.setBrush(d->brush);

    if (d->boxStyle != NoBox) {
        QRectF boxRect(centerX - boxWidth * 0.5, centerY - boxHeight * 0.5, boxWidth, boxHeight);
        painter.drawRect(boxRect);
    }

    if (d->whiskerStyle != NoWhiskers) {
        const double whiskerLen = rect.height() * 0.2;
        painter.drawLine(centerX, centerY - boxHeight * 0.5, centerX, centerY - boxHeight * 0.5 - whiskerLen);
        painter.drawLine(centerX - boxWidth * 0.15,
                         centerY - boxHeight * 0.5 - whiskerLen,
                         centerX + boxWidth * 0.15,
                         centerY - boxHeight * 0.5 - whiskerLen);
        painter.drawLine(centerX, centerY + boxHeight * 0.5, centerX, centerY + boxHeight * 0.5 + whiskerLen);
        painter.drawLine(centerX - boxWidth * 0.15,
                         centerY + boxHeight * 0.5 + whiskerLen,
                         centerX + boxWidth * 0.15,
                         centerY + boxHeight * 0.5 + whiskerLen);
    }

    if (d->medianVisible) {
        painter.setPen(d->medianPen);
        painter.drawLine(centerX - boxWidth * 0.5, centerY, centerX + boxWidth * 0.5, centerY);
    }

    painter.end();

    return graphic;
}