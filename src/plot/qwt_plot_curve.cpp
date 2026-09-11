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

#include "qwt_plot_curve.h"
#include "qwt_point_data.h"
#include "qwt_math.h"
#include "qwt_clipper.h"
#include "qwt_painter.h"
#include "qwt_scale_map.h"
#include "qwt_plot.h"
#include "qwt_spline_curve_fitter.h"
#include "qwt_symbol.h"
#include "qwt_point_mapper.h"
#include "qwt_text.h"
#include "qwt_graphic.h"

#include <qpainter.h>
#include <qpainterpath.h>

static inline QRectF qwtIntersectedClipRect(const QRectF& rect, QPainter* painter)
{
    QRectF clipRect = rect;
    if (painter->hasClipping())
        clipRect &= painter->clipBoundingRect();

    return clipRect;
}

static void qwtUpdateLegendIconSize(QwtPlotCurve* curve)
{
    if (curve->symbol() && curve->testLegendAttribute(QwtPlotCurve::LegendShowSymbol)) {
        QSize sz = curve->symbol()->boundingRect().size();
        sz += QSize(2, 2);  // margin

        if (curve->testLegendAttribute(QwtPlotCurve::LegendShowLine)) {
            // Avoid, that the line is completely covered by the symbol

            int w = qwtCeil(1.5 * sz.width());
            if (w % 2)
                w++;

            sz.setWidth(qMax(8, w));
        }

        curve->setLegendIconSize(sz);
    }
}

class QwtPlotCurve::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotCurve)
public:
    PrivateData(QwtPlotCurve* p)
        : q_ptr(p)
        , style(QwtPlotCurve::Lines)
        , baseline(0.0)
        , symbol(nullptr)
        , pen(Qt::black)
        , paintAttributes(QwtPlotCurve::ClipPolygons | QwtPlotCurve::FilterPointsLTTB)
    {
        curveFitter = new QwtSplineCurveFitter;
    }

    ~PrivateData()
    {
        delete symbol;
        delete curveFitter;
    }

    QwtPlotCurve::CurveStyle style;
    double baseline;

    QwtSymbol* symbol;
    QwtCurveFitter* curveFitter;

    QPen pen;
    QBrush brush;
    bool m_userSetPen = false;

    QwtPlotCurve::CurveAttributes attributes;
    QwtPlotCurve::PaintAttributes paintAttributes;

    QwtPlotCurve::LegendAttributes legendAttributes;
};

/**
 * @brief Constructor with QwtText title
 * @param[in] title Title of the curve
 */
QwtPlotCurve::QwtPlotCurve(const QwtText& title) : QwtPlotSeriesItem(title), QWT_PIMPL_CONSTRUCT
{
    init();
}

/**
 * @brief Constructor with QString title
 * @param[in] title Title of the curve
 */
QwtPlotCurve::QwtPlotCurve(const QString& title) : QwtPlotSeriesItem(QwtText(title)), QWT_PIMPL_CONSTRUCT
{
    init();
}

/**
 * @brief Destructor
 */
QwtPlotCurve::~QwtPlotCurve()
{
}

/**
 * @brief Initialize internal members
 * @details Sets default attributes and creates internal data structures.
 */
void QwtPlotCurve::init()
{
    setItemAttribute(QwtPlotItem::Legend);
    setItemAttribute(QwtPlotItem::AutoScale);

    setData(new QwtPointSeriesData());

    setZ(20.0);
}

/**
 * @brief Get the runtime type information
 * @return QwtPlotItem::Rtti_PlotCurve
 */
int QwtPlotCurve::rtti() const
{
    return QwtPlotItem::Rtti_PlotCurve;
}

/**
 * @brief Attach the curve to a plot
 * @details If the pen has not been explicitly set by the user via setPen(),
 *          the curve automatically receives a color from the plot's color cycle.
 *          If an uncustomized symbol (default brush/pen) has been assigned, it is
 *          auto-colored to match the curve pen as well.
 * @param plot Plot to attach to (nullptr to detach)
 * @sa QwtPlot::nextColorForItem(), QwtPlot::setColorCycle(), setSymbol()
 */
void QwtPlotCurve::attach(QwtPlot* plot)
{
    QWT_D(d);
    if (plot && !d->m_userSetPen && d->pen.color() == QColor(Qt::black)) {
        const QColor c = plot->nextColorForItem(rtti());
        d->pen         = QPen(c, d->pen.widthF(), d->pen.style());
    }
    // Auto-color an uncustomized symbol to match the curve pen. This covers the
    // case where a default symbol was assigned before attach; symbols assigned
    // after attach are handled in setSymbol().
    if (plot && d->symbol && d->symbol->brush().style() == Qt::NoBrush
        && d->symbol->pen().color() == QColor("#555555")) {
        const QColor c = d->pen.color();
        d->symbol->setBrush(QBrush(c));
        d->symbol->setPen(QPen(c.darker(150), 1));
    }
    QwtPlotItem::attach(plot);
}

/**
 * @brief Set paint attribute
 * @param[in] attribute Paint attribute
 * @param[in] on On/Off
 * @sa testPaintAttribute()
 */
void QwtPlotCurve::setPaintAttribute(PaintAttribute attribute, bool on)
{
    QWT_D(d);
    if (on)
        d->paintAttributes |= attribute;
    else
        d->paintAttributes &= ~attribute;
}

/**
 * @brief Test paint attribute
 * @param[in] attribute Paint attribute
 * @return True when attribute is enabled
 * @sa setPaintAttribute()
 */
bool QwtPlotCurve::testPaintAttribute(PaintAttribute attribute) const
{
    QWT_DC(d);
    return (d->paintAttributes & attribute);
}

/**
 * @brief Set legend attribute
 * @param[in] attribute Legend attribute
 * @param[in] on On/Off
 * @sa testLegendAttribute(), legendIcon()
 */
void QwtPlotCurve::setLegendAttribute(LegendAttribute attribute, bool on)
{
    QWT_D(d);
    if (on != testLegendAttribute(attribute)) {
        if (on)
            d->legendAttributes |= attribute;
        else
            d->legendAttributes &= ~attribute;

        qwtUpdateLegendIconSize(this);
        legendChanged();
    }
}

/**
 * @brief Test legend attribute
 * @param[in] attribute Legend attribute
 * @return True when attribute is enabled
 * @sa setLegendAttribute()
 */
bool QwtPlotCurve::testLegendAttribute(LegendAttribute attribute) const
{
    QWT_DC(d);
    return (d->legendAttributes & attribute);
}

/**
 * @brief Set legend attributes
 * @param[in] attributes Legend attributes
 * @sa setLegendAttribute(), legendIcon()
 */
void QwtPlotCurve::setLegendAttributes(LegendAttributes attributes)
{
    QWT_D(d);
    if (attributes != d->legendAttributes) {
        d->legendAttributes = attributes;

        qwtUpdateLegendIconSize(this);
        legendChanged();
    }
}

/**
 * @brief Get legend attributes
 * @return Attributes for drawing the legend icon
 * @sa setLegendAttributes(), testLegendAttribute()
 */
QwtPlotCurve::LegendAttributes QwtPlotCurve::legendAttributes() const
{
    QWT_DC(d);
    return d->legendAttributes;
}

/**
 * @brief Set the curve's drawing style
 * @param[in] style Curve style
 * @sa style()
 */
void QwtPlotCurve::setStyle(CurveStyle style)
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
 */
QwtPlotCurve::CurveStyle QwtPlotCurve::style() const
{
    QWT_DC(d);
    return d->style;
}

/**
 * @brief Assign a symbol
 * @details The curve will take the ownership of the symbol, hence the previously
 *          set symbol will be deleted by setting a new one. If symbol is nullptr
 *          no symbol will be drawn.
 *
 *          If the curve is already attached and the new symbol is in its default
 *          (uncustomized) state, it is auto-colored to match the curve pen, so that
 *          a scatter-style curve inherits the color cycle color of its pen.
 * @param[in] symbol Symbol
 * @sa symbol(), attach()
 */
void QwtPlotCurve::setSymbol(QwtSymbol* symbol)
{
    QWT_D(d);
    if (symbol != d->symbol) {
        delete d->symbol;
        // Auto-color an uncustomized symbol to match the curve pen when the
        // curve is already attached. A symbol assigned before attach is colored
        // later in attach().
        if (symbol && plot() && symbol->brush().style() == Qt::NoBrush
            && symbol->pen().color() == QColor("#555555")) {
            const QColor c = d->pen.color();
            symbol->setBrush(QBrush(c));
            symbol->setPen(QPen(c.darker(150), 1));
        }
        d->symbol = symbol;

        qwtUpdateLegendIconSize(this);

        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the current symbol
 * @return Current symbol or nullptr when no symbol has been assigned
 * @sa setSymbol()
 */
const QwtSymbol* QwtPlotCurve::symbol() const
{
    QWT_DC(d);
    return d->symbol;
}

/**
 * @brief Build and assign a pen
 * @details In Qt5 the default pen width is 1.0 (0.0 in Qt4) which makes it
 *          non cosmetic (see QPen::isCosmetic()). This method has been introduced
 *          to hide this incompatibility.
 * @param[in] color Pen color
 * @param[in] width Pen width
 * @param[in] style Pen style
 * @sa pen(), brush()
 */
void QwtPlotCurve::setPen(const QColor& color, qreal width, Qt::PenStyle style)
{
    setPen(QPen(color, width, style));
}

/**
 * @brief Assign a pen
 * @param[in] pen New pen
 * @sa pen(), brush()
 */
void QwtPlotCurve::setPen(const QPen& pen)
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
 */
const QPen& QwtPlotCurve::pen() const
{
    QWT_DC(d);
    return d->pen;
}

/**
 * @brief Assign a brush
 * @details In case of brush.style() != QBrush::NoBrush and style() != QwtPlotCurve::Sticks,
 *          the area between the curve and the baseline will be filled.
 *          In case !brush.color().isValid() the area will be filled by pen.color().
 *          The fill algorithm simply connects the first and the last curve point to the baseline.
 *          So the curve data has to be sorted (ascending or descending).
 * @param[in] brush New brush
 * @sa brush(), setBaseline(), baseline()
 */
void QwtPlotCurve::setBrush(const QBrush& brush)
{
    QWT_D(d);
    if (brush != d->brush) {
        d->brush = brush;

        legendChanged();
        itemChanged();
    }
}

/**
 * @brief Get the brush used to fill the area between lines and the baseline
 * @return Brush used to fill the area between lines and the baseline
 * @sa setBrush(), setBaseline(), baseline()
 */
const QBrush& QwtPlotCurve::brush() const
{
    QWT_DC(d);
    return d->brush;
}

/**
 * @brief Draw an interval of the curve
 * @param[in] painter Painter
 * @param[in] xMap Maps x-values into pixel coordinates
 * @param[in] yMap Maps y-values into pixel coordinates
 * @param[in] canvasRect Contents rectangle of the canvas
 * @param[in] from Index of the first point to be painted
 * @param[in] to Index of the last point to be painted. If to < 0 the curve will be painted to its last point.
 * @sa drawCurve(), drawSymbols()
 */
void QwtPlotCurve::drawSeries(QPainter* painter,
                              const QwtScaleMap& xMap,
                              const QwtScaleMap& yMap,
                              const QRectF& canvasRect,
                              int from,
                              int to) const
{
    const size_t numSamples = dataSize();

    if (!painter || numSamples <= 0)
        return;

    if (to < 0)
        to = numSamples - 1;

    if (qwtVerifyRange(numSamples, from, to) > 0) {
        QWT_DC(d);
        painter->save();
        painter->setPen(d->pen);

        /*
           Qt 4.0.0 is slow when drawing lines, but it's even
           slower when the painter has a brush. So we don't
           set the brush before we really need it.
         */

        drawCurve(painter, d->style, xMap, yMap, canvasRect, from, to);
        painter->restore();

        if (d->symbol && (d->symbol->style() != QwtSymbol::NoSymbol)) {
            painter->save();
            drawSymbols(painter, *d->symbol, xMap, yMap, canvasRect, from, to);
            painter->restore();
        }
    }
}

/**
 * @brief Draw the line part (without symbols) of a curve interval
 * @param[in] painter Painter
 * @param[in] style Curve style, see QwtPlotCurve::CurveStyle
 * @param[in] xMap X map
 * @param[in] yMap Y map
 * @param[in] canvasRect Contents rectangle of the canvas
 * @param[in] from Index of the first point to be painted
 * @param[in] to Index of the last point to be painted
 * @sa draw(), drawDots(), drawLines(), drawSteps(), drawSticks()
 */
void QwtPlotCurve::drawCurve(QPainter* painter,
                             int style,
                             const QwtScaleMap& xMap,
                             const QwtScaleMap& yMap,
                             const QRectF& canvasRect,
                             int from,
                             int to) const
{
    switch (style) {
    case Lines:
        if (testCurveAttribute(Fitted)) {
            // we always need the complete
            // curve for fitting
            from = 0;
            to   = dataSize() - 1;
        }
        drawLines(painter, xMap, yMap, canvasRect, from, to);
        break;
    case Sticks:
        drawSticks(painter, xMap, yMap, canvasRect, from, to);
        break;
    case Steps:
        drawSteps(painter, xMap, yMap, canvasRect, from, to);
        break;
    case Dots:
        drawDots(painter, xMap, yMap, canvasRect, from, to);
        break;
    case NoCurve:
    default:
        break;
    }
}

/**
 * @brief Draw lines
 * @details If the CurveAttribute Fitted is enabled a QwtCurveFitter tries
 *          to interpolate/smooth the curve, before it is painted.
 * @param[in] painter Painter
 * @param[in] xMap X map
 * @param[in] yMap Y map
 * @param[in] canvasRect Contents rectangle of the canvas
 * @param[in] from Index of the first point to be painted
 * @param[in] to Index of the last point to be painted
 * @sa setCurveAttribute(), setCurveFitter(), draw(), drawLines(), drawDots(), drawSteps(), drawSticks()
 */
void QwtPlotCurve::drawLines(QPainter* painter,
                             const QwtScaleMap& xMap,
                             const QwtScaleMap& yMap,
                             const QRectF& canvasRect,
                             int from,
                             int to) const
{
    if (from > to)
        return;

    QWT_DC(d);

    const bool doFit   = (d->attributes & Fitted) && d->curveFitter;
    const bool doAlign = !doFit && QwtPainter::roundingAlignment(painter);
    const bool doFill  = (d->brush.style() != Qt::NoBrush) && (d->brush.color().alpha() > 0);

    QRectF clipRect;
    if (d->paintAttributes & ClipPolygons) {
        clipRect = qwtIntersectedClipRect(canvasRect, painter);

        const qreal pw = QwtPainter::effectivePenWidth(painter->pen());
        clipRect       = clipRect.adjusted(-pw, -pw, pw, pw);
    }

    QwtPointMapper mapper;

    if (doAlign) {
        mapper.setFlag(QwtPointMapper::RoundPoints, true);
        mapper.setFlag(QwtPointMapper::WeedOutIntermediatePoints, testPaintAttribute(FilterPointsAggressive));
    }

    mapper.setFlag(QwtPointMapper::PixelColumnReduce, testPaintAttribute(FilterPointsPixel));
    mapper.setFlag(QwtPointMapper::MinMaxReduce, testPaintAttribute(FilterPointsLTTB));

    mapper.setFlag(QwtPointMapper::WeedOutPoints,
                   testPaintAttribute(FilterPoints) || testPaintAttribute(FilterPointsAggressive));

    mapper.setBoundingRect(canvasRect);

    QPolygonF polyline = mapper.toPolygonF(xMap, yMap, data(), from, to);

    if (doFill) {
        if (doFit) {
            // it might be better to extend and draw the curvePath, but for
            // the moment we keep an implementation, where we translate the
            // path back to a polyline.

            polyline = d->curveFitter->fitCurve(polyline);
        }

        if (painter->pen().style() != Qt::NoPen) {
            // here we are wasting memory for the filled copy,
            // do polygon clipping twice etc .. TODO

            QPolygonF filled = polyline;
            fillCurve(painter, xMap, yMap, canvasRect, filled);
            filled.clear();

            if (d->paintAttributes & ClipPolygons)
                QwtClipper::clipPolygonF(clipRect, polyline, false);

            QwtPainter::drawPolyline(painter, polyline);
        } else {
            fillCurve(painter, xMap, yMap, canvasRect, polyline);
        }
    } else {
        if (testPaintAttribute(ClipPolygons)) {
            QwtClipper::clipPolygonF(clipRect, polyline, false);
        }

        if (doFit) {
            if (d->curveFitter->mode() == QwtCurveFitter::Path) {
                const QPainterPath curvePath = d->curveFitter->fitCurvePath(polyline);

                painter->drawPath(curvePath);
            } else {
                polyline = d->curveFitter->fitCurve(polyline);
                QwtPainter::drawPolyline(painter, polyline);
            }
        } else {
            QwtPainter::drawPolyline(painter, polyline);
        }
    }
}

/**
 * @brief Draw sticks
 * @param[in] painter Painter
 * @param[in] xMap X map
 * @param[in] yMap Y map
 * @param[in] canvasRect Contents rectangle of the canvas
 * @param[in] from Index of the first point to be painted
 * @param[in] to Index of the last point to be painted
 * @sa draw(), drawCurve(), drawDots(), drawLines(), drawSteps()
 */
void QwtPlotCurve::drawSticks(QPainter* painter,
                              const QwtScaleMap& xMap,
                              const QwtScaleMap& yMap,
                              const QRectF& canvasRect,
                              int from,
                              int to) const
{
    Q_UNUSED(canvasRect)
    QWT_DC(d);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);

    const bool doAlign = QwtPainter::roundingAlignment(painter);

    double x0 = xMap.transform(d->baseline);
    double y0 = yMap.transform(d->baseline);
    if (doAlign) {
        x0 = qRound(x0);
        y0 = qRound(y0);
    }

    const Qt::Orientation o = orientation();

    const QwtSeriesData< QPointF >* series = data();

    for (int i = from; i <= to; i++) {
        const QPointF sample = series->sample(i);

        if (isSampleNanOrInf(sample))
            continue;

        double xi            = xMap.transform(sample.x());
        double yi            = yMap.transform(sample.y());
        if (doAlign) {
            xi = qRound(xi);
            yi = qRound(yi);
        }

        if (o == Qt::Horizontal)
            QwtPainter::drawLine(painter, x0, yi, xi, yi);
        else
            QwtPainter::drawLine(painter, xi, y0, xi, yi);
    }

    painter->restore();
}

/**
 * @brief Draw dots
 * @param[in] painter Painter
 * @param[in] xMap X map
 * @param[in] yMap Y map
 * @param[in] canvasRect Contents rectangle of the canvas
 * @param[in] from Index of the first point to be painted
 * @param[in] to Index of the last point to be painted
 * @sa draw(), drawCurve(), drawSticks(), drawLines(), drawSteps()
 */
void QwtPlotCurve::drawDots(QPainter* painter,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QRectF& canvasRect,
                            int from,
                            int to) const
{
    const QColor color = painter->pen().color();

    if (painter->pen().style() == Qt::NoPen || color.alpha() == 0) {
        return;
    }

    QWT_DC(d);

    const bool doFill  = (d->brush.style() != Qt::NoBrush) && (d->brush.color().alpha() > 0);
    const bool doAlign = QwtPainter::roundingAlignment(painter);

    QwtPointMapper mapper;
    mapper.setBoundingRect(canvasRect);
    mapper.setFlag(QwtPointMapper::RoundPoints, doAlign);

    if (d->paintAttributes & FilterPoints) {
        if ((color.alpha() == 255) && !(painter->renderHints() & QPainter::Antialiasing)) {
            mapper.setFlag(QwtPointMapper::WeedOutPoints, true);
        }
    }

    if (doFill) {
        mapper.setFlag(QwtPointMapper::WeedOutPoints, false);

        QPolygonF points = mapper.toPolygonF(xMap, yMap, data(), from, to);

        QwtPainter::drawPoints(painter, points);
        fillCurve(painter, xMap, yMap, canvasRect, points);
    } else if (d->paintAttributes & ImageBuffer) {
        const QImage image = mapper.toImage(
            xMap, yMap, data(), from, to, d->pen, painter->testRenderHint(QPainter::Antialiasing), renderThreadCount());

        painter->drawImage(canvasRect.toAlignedRect(), image);
    } else if (d->paintAttributes & MinimizeMemory) {
        const QwtSeriesData< QPointF >* series = data();

        for (int i = from; i <= to; i++) {
            const QPointF sample = series->sample(i);

            if (isSampleNanOrInf(sample))
                continue;

            double xi = xMap.transform(sample.x());
            double yi = yMap.transform(sample.y());

            if (doAlign) {
                xi = qRound(xi);
                yi = qRound(yi);
            }

            QwtPainter::drawPoint(painter, QPointF(xi, yi));
        }
    } else {
        if (doAlign) {
            const QPolygon points = mapper.toPoints(xMap, yMap, data(), from, to);

            QwtPainter::drawPoints(painter, points);
        } else {
            const QPolygonF points = mapper.toPointsF(xMap, yMap, data(), from, to);

            QwtPainter::drawPoints(painter, points);
        }
    }
}

/**
 * @brief Draw step function
 * @details The direction of the steps depends on Inverted attribute.
 * @param[in] painter Painter
 * @param[in] xMap X map
 * @param[in] yMap Y map
 * @param[in] canvasRect Contents rectangle of the canvas
 * @param[in] from Index of the first point to be painted
 * @param[in] to Index of the last point to be painted
 * @sa CurveAttribute, setCurveAttribute(), draw(), drawCurve(), drawDots(), drawLines(), drawSticks()
 */
void QwtPlotCurve::drawSteps(QPainter* painter,
                             const QwtScaleMap& xMap,
                             const QwtScaleMap& yMap,
                             const QRectF& canvasRect,
                             int from,
                             int to) const
{
    QWT_DC(d);

    const bool doAlign = QwtPainter::roundingAlignment(painter);

    QPolygonF polygon(2 * (to - from) + 1);
    QPointF* points = polygon.data();

    bool inverted = orientation() == Qt::Vertical;
    if (d->attributes & Inverted)
        inverted = !inverted;

    const QwtSeriesData< QPointF >* series = data();

    int i, ip;
    for (i = from, ip = 0; i <= to; i++) {
        const QPointF sample = series->sample(i);

        if (isSampleNanOrInf(sample))
            continue;

        double xi            = xMap.transform(sample.x());
        double yi            = yMap.transform(sample.y());
        if (doAlign) {
            xi = qRound(xi);
            yi = qRound(yi);
        }

        if (ip > 0) {
            const QPointF& p0 = points[ ip - 2 ];
            QPointF& p        = points[ ip - 1 ];

            if (inverted) {
                p.rx() = p0.x();
                p.ry() = yi;
            } else {
                p.rx() = xi;
                p.ry() = p0.y();
            }
        }

        points[ ip ].rx() = xi;
        points[ ip ].ry() = yi;
        ip += 2;
    }

    if (ip == 0)
        return;

    polygon.resize(ip - 1);

    if (d->paintAttributes & ClipPolygons) {
        QRectF clipRect = qwtIntersectedClipRect(canvasRect, painter);

        const qreal pw = QwtPainter::effectivePenWidth(painter->pen());
        clipRect       = clipRect.adjusted(-pw, -pw, pw, pw);

        const QPolygonF clipped = QwtClipper::clippedPolygonF(clipRect, polygon, false);

        QwtPainter::drawPolyline(painter, clipped);
    } else {
        QwtPainter::drawPolyline(painter, polygon);
    }

    if (d->brush.style() != Qt::NoBrush)
        fillCurve(painter, xMap, yMap, canvasRect, polygon);
}

/**
 * @brief Set curve attribute
 * @param[in] attribute Curve attribute
 * @param[in] on On/Off
 * @sa testCurveAttribute(), setCurveFitter()
 */
void QwtPlotCurve::setCurveAttribute(CurveAttribute attribute, bool on)
{
    QWT_D(d);
    if (bool(d->attributes & attribute) == on)
        return;

    if (on)
        d->attributes |= attribute;
    else
        d->attributes &= ~attribute;

    itemChanged();
}

/**
 * @brief Test curve attribute
 * @param[in] attribute Curve attribute
 * @return True if attribute is enabled
 * @sa setCurveAttribute()
 */
bool QwtPlotCurve::testCurveAttribute(CurveAttribute attribute) const
{
    QWT_DC(d);
    return d->attributes & attribute;
}

/**
 * @brief Assign a curve fitter
 * @details The curve fitter "smooths" the curve points, when the Fitted
 *          CurveAttribute is set. setCurveFitter(nullptr) also disables curve fitting.
 *          The curve fitter operates on the translated points (= widget coordinates)
 *          to be functional for logarithmic scales. Obviously this is less performant
 *          for fitting algorithms, that reduce the number of points.
 *          For situations, where curve fitting is used to improve the performance
 *          of painting huge series of points it might be better to execute the fitter
 *          on the curve points once and to cache the result in the QwtSeriesData object.
 * @param[in] curveFitter Curve fitter
 * @sa Fitted
 */
void QwtPlotCurve::setCurveFitter(QwtCurveFitter* curveFitter)
{
    QWT_D(d);
    delete d->curveFitter;
    d->curveFitter = curveFitter;

    itemChanged();
}

/**
 * @brief Get the curve fitter
 * @return Curve fitter, or nullptr if curve fitting is disabled
 * @sa setCurveFitter(), Fitted
 */
QwtCurveFitter* QwtPlotCurve::curveFitter() const
{
    QWT_DC(d);
    return d->curveFitter;
}

/**
 * @brief Fill the area between the curve and the baseline with the curve brush
 * @param[in] painter Painter
 * @param[in] xMap X map
 * @param[in] yMap Y map
 * @param[in] canvasRect Contents rectangle of the canvas
 * @param[in,out] polygon Polygon - will be modified
 * @sa setBrush(), setBaseline(), setStyle()
 */
void QwtPlotCurve::fillCurve(QPainter* painter,
                             const QwtScaleMap& xMap,
                             const QwtScaleMap& yMap,
                             const QRectF& canvasRect,
                             QPolygonF& polygon) const
{
    QWT_DC(d);

    if (d->brush.style() == Qt::NoBrush)
        return;

    closePolyline(painter, xMap, yMap, polygon);
    if (polygon.count() <= 2)  // a line can't be filled
        return;

    QBrush brush = d->brush;
    if (!brush.color().isValid())
        brush.setColor(d->pen.color());

    if (d->paintAttributes & ClipPolygons) {
        const QRectF clipRect = qwtIntersectedClipRect(canvasRect, painter);
        QwtClipper::clipPolygonF(clipRect, polygon, true);
    }

    painter->save();

    painter->setPen(Qt::NoPen);
    painter->setBrush(brush);

    QwtPainter::drawPolygon(painter, polygon);

    painter->restore();
}

/**
 * @brief Complete a polygon to be a closed polygon including the area between the original polygon and the baseline
 * @param[in] painter Painter
 * @param[in] xMap X map
 * @param[in] yMap Y map
 * @param[in,out] polygon Polygon to be completed
 */
void QwtPlotCurve::closePolyline(QPainter* painter, const QwtScaleMap& xMap, const QwtScaleMap& yMap, QPolygonF& polygon) const
{
    QWT_DC(d);

    if (polygon.size() < 2)
        return;

    const bool doAlign = QwtPainter::roundingAlignment(painter);

    double baseline = d->baseline;

    if (orientation() == Qt::Vertical) {
        if (yMap.transformation())
            baseline = yMap.transformation()->bounded(baseline);

        double refY = yMap.transform(baseline);
        if (doAlign)
            refY = qRound(refY);

        polygon += QPointF(polygon.last().x(), refY);
        polygon += QPointF(polygon.first().x(), refY);
    } else {
        if (xMap.transformation())
            baseline = xMap.transformation()->bounded(baseline);

        double refX = xMap.transform(baseline);
        if (doAlign)
            refX = qRound(refX);

        polygon += QPointF(refX, polygon.last().y());
        polygon += QPointF(refX, polygon.first().y());
    }
}

/**
 * @brief Draw symbols
 * @param[in] painter Painter
 * @param[in] symbol Curve symbol
 * @param[in] xMap X map
 * @param[in] yMap Y map
 * @param[in] canvasRect Contents rectangle of the canvas
 * @param[in] from Index of the first point to be painted
 * @param[in] to Index of the last point to be painted
 * @sa setSymbol(), drawSeries(), drawCurve()
 */
void QwtPlotCurve::drawSymbols(QPainter* painter,
                               const QwtSymbol& symbol,
                               const QwtScaleMap& xMap,
                               const QwtScaleMap& yMap,
                               const QRectF& canvasRect,
                               int from,
                               int to) const
{
    QwtPointMapper mapper;
    mapper.setFlag(QwtPointMapper::RoundPoints, QwtPainter::roundingAlignment(painter));
    mapper.setFlag(QwtPointMapper::WeedOutPoints, testPaintAttribute(QwtPlotCurve::FilterPoints));

    const QRectF clipRect = qwtIntersectedClipRect(canvasRect, painter);
    mapper.setBoundingRect(clipRect);

    const int chunkSize = 500;

    for (int i = from; i <= to; i += chunkSize) {
        const int n = qMin(chunkSize, to - i + 1);

        const QPolygonF points = mapper.toPointsF(xMap, yMap, data(), i, i + n - 1);

        if (points.size() > 0)
            symbol.drawSymbols(painter, points);
    }
}

/**
 * @brief Set the value of the baseline
 * @details The baseline is needed for filling the curve with a brush or the Sticks drawing style.
 *          The interpretation of the baseline depends on the orientation().
 *          With Qt::Vertical, the baseline is interpreted as a horizontal line at y = baseline(),
 *          with Qt::Horizontal, it is interpreted as a vertical line at x = baseline().
 *          The default value is 0.0.
 * @param[in] value Value of the baseline
 * @sa baseline(), setBrush(), setStyle(), QwtPlotAbstractSeriesItem::orientation()
 */
void QwtPlotCurve::setBaseline(double value)
{
    QWT_D(d);
    if (d->baseline != value) {
        d->baseline = value;
        itemChanged();
    }
}

/**
 * @brief Get the baseline value
 * @return Value of the baseline
 * @sa setBaseline()
 */
double QwtPlotCurve::baseline() const
{
    QWT_DC(d);
    return d->baseline;
}

/**
 * @brief Find the closest curve point for a specific position
 * @param[in] pos Position where to look for the closest curve point
 * @param[out] dist If dist != nullptr, returns the distance between the position and the closest curve point
 * @return Index of the closest curve point, or -1 if none can be found (e.g. when the curve has no points)
 * @note closestPoint() implements a dumb algorithm that iterates over all points
 */
int QwtPlotCurve::closestPoint(const QPointF& pos, double* dist) const
{
    const size_t numSamples = dataSize();

    if (plot() == nullptr || numSamples <= 0)
        return -1;

    const QwtSeriesData< QPointF >* series = data();

    const QwtScaleMap xMap = plot()->canvasMap(xAxis());
    const QwtScaleMap yMap = plot()->canvasMap(yAxis());

    int index   = -1;
    double dmin = 1.0e10;

    for (uint i = 0; i < numSamples; i++) {
        const QPointF sample = series->sample(i);

        if (isSampleNanOrInf(sample))
            continue;

        const double cx = xMap.transform(sample.x()) - pos.x();
        const double cy = yMap.transform(sample.y()) - pos.y();

        const double f = qwtSqr(cx) + qwtSqr(cy);
        if (f < dmin) {
            index = i;
            dmin  = f;
        }
    }
    if (dist)
        *dist = std::sqrt(dmin);

    return index;
}

/**
 * @brief Get the icon representing the curve on the legend
 * @param[in] index Index of the legend entry (ignored as there is only one)
 * @param[in] size Icon size
 * @return Icon representing the curve on the legend
 * @sa QwtPlotItem::setLegendIconSize(), QwtPlotItem::legendData()
 */
QwtGraphic QwtPlotCurve::legendIcon(int index, const QSizeF& size) const
{
    Q_UNUSED(index);
    QWT_DC(d);

    if (size.isEmpty())
        return QwtGraphic();

    QwtGraphic graphic;
    graphic.setDefaultSize(size);
    graphic.setRenderHint(QwtGraphic::RenderPensUnscaled, true);

    QPainter painter(&graphic);
    painter.setRenderHint(QPainter::Antialiasing, testRenderHint(QwtPlotItem::RenderAntialiased));

    if (d->legendAttributes == 0 || d->legendAttributes & QwtPlotCurve::LegendShowBrush) {
        QBrush brush = d->brush;

        if (brush.style() == Qt::NoBrush && d->legendAttributes == 0) {
            if (style() != QwtPlotCurve::NoCurve) {
                brush = QBrush(pen().color());
            } else if (d->symbol && (d->symbol->style() != QwtSymbol::NoSymbol)) {
                brush = QBrush(d->symbol->pen().color());
            }
        }

        if (brush.style() != Qt::NoBrush) {
            QRectF r(0, 0, size.width(), size.height());
            painter.fillRect(r, brush);
        }
    }

    if (d->legendAttributes & QwtPlotCurve::LegendShowLine) {
        if (pen() != Qt::NoPen) {
            QPen pn = pen();
            pn.setCapStyle(Qt::FlatCap);

            painter.setPen(pn);

            const double y = 0.5 * size.height();
            QwtPainter::drawLine(&painter, 0.0, y, size.width(), y);
        }
    }

    if (d->legendAttributes & QwtPlotCurve::LegendShowSymbol) {
        if (d->symbol) {
            QRectF r(0, 0, size.width(), size.height());
            d->symbol->drawSymbol(&painter, r);
        }
    }

    return graphic;
}

/**
 * @brief Assign a series of points
 * @details setSamples() is just a wrapper for setData() without any additional value -
 *          beside that it is easier to find for the developer.
 * @param[in] data Data
 * @warning The item takes ownership of the data object, deleting it when it's not used anymore.
 */
void QwtPlotCurve::setSamples(QwtSeriesData< QPointF >* data)
{
    setData(data);
}

/**
 * @brief Initialize data with an array of points
 * @param[in] samples Vector of points
 * @note QVector is implicitly shared
 * @note QPolygonF is derived from QVector<QPointF>
 */
void QwtPlotCurve::setSamples(const QVector< QPointF >& samples)
{
    setData(new QwtPointSeriesData(samples));
}

/**
 * @brief Initialize data with an array of points (rvalue)
 * @param[in] samples Vector of points
 * @note QVector is implicitly shared
 * @note QPolygonF is derived from QVector<QPointF>
 */
void QwtPlotCurve::setSamples(QVector< QPointF >&& samples)
{
    setData(new QwtPointSeriesData(samples));
}

/**
 * @brief Initialize data by pointing to memory blocks not managed by QwtPlotCurve (double)
 * @details setRawSamples is provided for efficiency. It is important to keep the pointers
 *          during the lifetime of the underlying QwtCPointerData class.
 * @param[in] xData Pointer to x data
 * @param[in] yData Pointer to y data
 * @param[in] size Size of x and y
 * @sa QwtCPointerData
 */
void QwtPlotCurve::setRawSamples(const double* xData, const double* yData, int size)
{
    setData(new QwtCPointerData< double >(xData, yData, size));
}

/**
 * @brief Initialize data by pointing to memory blocks not managed by QwtPlotCurve (float)
 * @details setRawSamples is provided for efficiency. It is important to keep the pointers
 *          during the lifetime of the underlying QwtCPointerData class.
 * @param[in] xData Pointer to x data
 * @param[in] yData Pointer to y data
 * @param[in] size Size of x and y
 * @sa QwtCPointerData
 */
void QwtPlotCurve::setRawSamples(const float* xData, const float* yData, int size)
{
    setData(new QwtCPointerData< float >(xData, yData, size));
}

/**
 * @brief Initialize data by pointing to a memory block not managed by QwtPlotCurve (double, y-axis only)
 * @details The memory contains the y coordinates, while the index is interpreted as x coordinate.
 *          setRawSamples() is provided for efficiency. It is important to keep the pointers
 *          during the lifetime of the underlying QwtCPointerValueData class.
 * @param[in] yData Pointer to y data
 * @param[in] size Size of y data
 * @sa QwtCPointerData
 */
void QwtPlotCurve::setRawSamples(const double* yData, int size)
{
    setData(new QwtCPointerValueData< double >(yData, size));
}

/**
 * @brief Initialize data by pointing to a memory block not managed by QwtPlotCurve (float, y-axis only)
 * @details The memory contains the y coordinates, while the index is interpreted as x coordinate.
 *          setRawSamples() is provided for efficiency. It is important to keep the pointers
 *          during the lifetime of the underlying QwtCPointerValueData class.
 * @param[in] yData Pointer to y data
 * @param[in] size Size of y data
 * @sa QwtCPointerData
 */
void QwtPlotCurve::setRawSamples(const float* yData, int size)
{
    setData(new QwtCPointerValueData< float >(yData, size));
}

/**
 * @brief Set data by copying x- and y-values from specified memory blocks (double)
 * @details Contrary to setRawSamples(), this function makes a 'deep copy' of the data.
 * @param[in] xData Pointer to x values
 * @param[in] yData Pointer to y values
 * @param[in] size Size of xData and yData
 * @sa QwtPointArrayData
 */
void QwtPlotCurve::setSamples(const double* xData, const double* yData, int size)
{
    setData(new QwtPointArrayData< double >(xData, yData, size));
}

/**
 * @brief Set data by copying x- and y-values from specified memory blocks (float)
 * @details Contrary to setRawSamples(), this function makes a 'deep copy' of the data.
 * @param[in] xData Pointer to x values
 * @param[in] yData Pointer to y values
 * @param[in] size Size of xData and yData
 * @sa QwtPointArrayData
 */
void QwtPlotCurve::setSamples(const float* xData, const float* yData, int size)
{
    setData(new QwtPointArrayData< float >(xData, yData, size));
}

/**
 * @brief Initialize data with x- and y-arrays (explicitly shared, double)
 * @param[in] xData X data
 * @param[in] yData Y data
 * @sa QwtPointArrayData
 */
void QwtPlotCurve::setSamples(const QVector< double >& xData, const QVector< double >& yData)
{
    setData(new QwtPointArrayData< double >(xData, yData));
}

/**
 * @brief Initialize data with x- and y-arrays (rvalue, double)
 * @param[in] xData X data
 * @param[in] yData Y data
 * @sa QwtPointArrayData
 */
void QwtPlotCurve::setSamples(QVector< double >&& xData, QVector< double >&& yData)
{
    setData(new QwtPointArrayData< double >(xData, yData));
}

/**
 * @brief Initialize data with x- and y-arrays (explicitly shared, float)
 * @param[in] xData X data
 * @param[in] yData Y data
 * @sa QwtPointArrayData
 */
void QwtPlotCurve::setSamples(const QVector< float >& xData, const QVector< float >& yData)
{
    setData(new QwtPointArrayData< float >(xData, yData));
}

/**
 * @brief Initialize data with x- and y-arrays (rvalue, float)
 * @param[in] xData X data
 * @param[in] yData Y data
 * @sa QwtPointArrayData
 */
void QwtPlotCurve::setSamples(QVector< float >&& xData, QVector< float >&& yData)
{
    setData(new QwtPointArrayData< float >(xData, yData));
}

/**
 * @brief Set data by copying y-values from a specified memory block (double)
 * @details The memory contains the y coordinates, while the index is interpreted as x coordinate.
 * @param[in] yData Y data
 * @param[in] size Size of yData
 * @sa QwtValuePointData
 */
void QwtPlotCurve::setSamples(const double* yData, int size)
{
    setData(new QwtValuePointData< double >(yData, size));
}

/**
 * @brief Set data by copying y-values from a specified memory block (float)
 * @details The vector contains the y coordinates, while the index is interpreted as x coordinate.
 * @param[in] yData Y data
 * @param[in] size Size of yData
 * @sa QwtValuePointData
 */
void QwtPlotCurve::setSamples(const float* yData, int size)
{
    setData(new QwtValuePointData< float >(yData, size));
}

/**
 * @brief Initialize data with an array of y values (explicitly shared, double)
 * @details The vector contains the y coordinates, while the index is the x coordinate.
 * @param[in] yData Y data
 * @sa QwtValuePointData
 */
void QwtPlotCurve::setSamples(const QVector< double >& yData)
{
    setData(new QwtValuePointData< double >(yData));
}

/**
 * @brief Initialize data with an array of y values (explicitly shared, float)
 * @details The vector contains the y coordinates, while the index is the x coordinate.
 * @param[in] yData Y data
 * @sa QwtValuePointData
 */
void QwtPlotCurve::setSamples(const QVector< float >& yData)
{
    setData(new QwtValuePointData< float >(yData));
}
