/******************************************************************************
 * QwtPolar Widget Library
 * Copyright (C) 2008   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_polar_layout.h"
#include "qwt_polar_plot.h"
#include "qwt_polar_canvas.h"
#include "qwt_text.h"
#include "qwt_text_label.h"
#include "qwt_legend.h"

class QwtPolarLayout::LayoutData
{
public:
    void init(const QwtPolarPlot*, const QRectF& rect);

    struct t_legendData
    {
        int frameWidth;
        int hScrollExtent;
        int vScrollExtent;
        QSizeF hint;
    } legend;

    struct t_titleData
    {
        QwtText text;
        int frameWidth;
    } title;

    struct t_canvasData
    {
        int frameWidth;
    } canvas;
};

void QwtPolarLayout::LayoutData::init(const QwtPolarPlot* plot, const QRectF& rect)
{
    // legend

    if (plot->plotLayout()->legendPosition() != QwtPolarPlot::ExternalLegend && plot->legend()) {
        legend.frameWidth    = plot->legend()->frameWidth();
        legend.hScrollExtent = plot->legend()->scrollExtent(Qt::Horizontal);
        legend.vScrollExtent = plot->legend()->scrollExtent(Qt::Vertical);

        const QSizeF hint = plot->legend()->sizeHint();

        double w = qMin(hint.width(), rect.width());
        double h = plot->legend()->heightForWidth(w);
        if (h == 0.0)
            h = hint.height();

        if (h > rect.height())
            w += legend.hScrollExtent;

        legend.hint = QSizeF(w, h);
    }

    // title

    title.frameWidth = 0;
    title.text       = QwtText();

    if (plot->titleLabel()) {
        const QwtTextLabel* label = plot->titleLabel();
        title.text                = label->text();
        if (!(title.text.testPaintAttribute(QwtText::PaintUsingTextFont)))
            title.text.setFont(label->font());

        title.frameWidth = plot->titleLabel()->frameWidth();
    }

    // canvas

    canvas.frameWidth = plot->canvas()->frameWidth();
}

class QwtPolarLayout::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPolarLayout)
public:
    PrivateData(QwtPolarLayout* p) : q_ptr(p), margin(0), spacing(0)
    {
    }

    QRectF titleRect;
    QRectF legendRect;
    QRectF canvasRect;

    QwtPolarLayout::LayoutData layoutData;

    QwtPolarPlot::LegendPosition legendPos;
    double legendRatio;

    unsigned int margin;
    unsigned int spacing;
};

/**
 * @brief Constructor
 */
QwtPolarLayout::QwtPolarLayout() : QWT_PIMPL_CONSTRUCT
{
    setLegendPosition(QwtPolarPlot::BottomLegend);
    invalidate();
}

/**
 * @brief Destructor
 */
QwtPolarLayout::~QwtPolarLayout()
{
}

/**
 * @brief Specify the position of the legend
 * @param pos The legend's position
 * @param ratio Ratio between legend and the bounding rect of title, canvas and axes.
 *              The legend will be shrunk if it would need more space than the given ratio.
 *              The ratio is limited to ]0.0 .. 1.0]. In case of <= 0.0 it will be reset to the default ratio.
 *              The default vertical/horizontal ratio is 0.33/0.5.
 * @sa QwtPolarPlot::setLegendPosition()
 */
void QwtPolarLayout::setLegendPosition(QwtPolarPlot::LegendPosition pos, double ratio)
{
    QWT_D(d);

    if (ratio > 1.0)
        ratio = 1.0;

    switch (pos) {
    case QwtPolarPlot::TopLegend:
    case QwtPolarPlot::BottomLegend: {
        if (ratio <= 0.0)
            ratio = 0.33;
        d->legendRatio = ratio;
        d->legendPos   = pos;
        break;
    }
    case QwtPolarPlot::LeftLegend:
    case QwtPolarPlot::RightLegend: {
        if (ratio <= 0.0)
            ratio = 0.5;
        d->legendRatio = ratio;
        d->legendPos   = pos;
        break;
    }
    case QwtPolarPlot::ExternalLegend: {
        d->legendRatio = ratio;  // meaningless
        d->legendPos   = pos;
        break;
    }
    default:
        break;
    }
}

/**
 * @brief Specify the position of the legend
 * @param pos The legend's position. Valid values are \c QwtPolarPlot::LeftLegend,
 *            \c QwtPolarPlot::RightLegend, \c QwtPolarPlot::TopLegend, \c QwtPolarPlot::BottomLegend.
 * @sa QwtPolarPlot::setLegendPosition()
 */
void QwtPolarLayout::setLegendPosition(QwtPolarPlot::LegendPosition pos)
{
    setLegendPosition(pos, 0.0);
}

/**
 * @brief Get the position of the legend
 * @return Position of the legend
 * @sa setLegendPosition(), QwtPolarPlot::setLegendPosition(), QwtPolarPlot::legendPosition()
 */
QwtPolarPlot::LegendPosition QwtPolarLayout::legendPosition() const
{
    QWT_DC(d);
    return d->legendPos;
}

/**
 * @brief Specify the relative size of the legend in the plot
 * @param ratio Ratio between legend and the bounding rect of title, canvas and axes.
 *              The legend will be shrunk if it would need more space than the given ratio.
 *              The ratio is limited to ]0.0 .. 1.0]. In case of <= 0.0 it will be reset to the default ratio.
 *              The default vertical/horizontal ratio is 0.33/0.5.
 * @sa setLegendPosition()
 */
void QwtPolarLayout::setLegendRatio(double ratio)
{
    setLegendPosition(legendPosition(), ratio);
}

/**
 * @brief Get the relative size of the legend in the plot
 * @return The relative size of the legend in the plot
 * @sa setLegendPosition()
 */
double QwtPolarLayout::legendRatio() const
{
    QWT_DC(d);
    return d->legendRatio;
}

/**
 * @brief Get geometry for the title
 * @return Geometry for the title
 * @sa activate(), invalidate()
 */
const QRectF& QwtPolarLayout::titleRect() const
{
    QWT_DC(d);
    return d->titleRect;
}

/**
 * @brief Get geometry for the legend
 * @return Geometry for the legend
 * @sa activate(), invalidate()
 */
const QRectF& QwtPolarLayout::legendRect() const
{
    QWT_DC(d);
    return d->legendRect;
}

/**
 * @brief Get geometry for the canvas
 * @return Geometry for the canvas
 * @sa activate(), invalidate()
 */
const QRectF& QwtPolarLayout::canvasRect() const
{
    QWT_DC(d);
    return d->canvasRect;
}

/**
 * @brief Invalidate the geometry of all components
 * @sa activate()
 */
void QwtPolarLayout::invalidate()
{
    QWT_D(d);
    d->titleRect = d->legendRect = d->canvasRect = QRect();
}

/**
 * @brief Find the geometry for the legend
 * @param options Options how to layout the legend
 * @param[in,out] rect Rectangle where to place the legend
 * @return Geometry for the legend
 */
QRectF QwtPolarLayout::layoutLegend(Options options, QRectF& rect) const
{
    QWT_DC(d);

    const QSizeF hint(d->layoutData.legend.hint);

    int dim;
    if (d->legendPos == QwtPolarPlot::LeftLegend || d->legendPos == QwtPolarPlot::RightLegend) {
        // We don't allow vertical legends to take more than
        // half of the available space.

        dim = qMin(double(hint.width()), rect.width() * d->legendRatio);

        if (!(options & IgnoreScrollbars)) {
            if (hint.height() > rect.height()) {
                // The legend will need additional
                // space for the vertical scrollbar.

                dim += d->layoutData.legend.hScrollExtent;
            }
        }
    } else {
        dim = qMin(double(hint.height()), rect.height() * d->legendRatio);
        dim = qMax(dim, d->layoutData.legend.vScrollExtent);
    }

    QRectF legendRect = rect;
    switch (d->legendPos) {
    case QwtPolarPlot::LeftLegend: {
        legendRect.setWidth(dim);
        rect.setLeft(legendRect.right());
        break;
    }
    case QwtPolarPlot::RightLegend: {
        legendRect.setX(rect.right() - dim + 1);
        legendRect.setWidth(dim);
        rect.setRight(legendRect.left());
        break;
    }
    case QwtPolarPlot::TopLegend: {
        legendRect.setHeight(dim);
        rect.setTop(legendRect.bottom());
        break;
    }
    case QwtPolarPlot::BottomLegend: {
        legendRect.setY(rect.bottom() - dim + 1);
        legendRect.setHeight(dim);
        rect.setBottom(legendRect.top());
        break;
    }
    case QwtPolarPlot::ExternalLegend:
        break;
    }

    return legendRect;
}

/**
 * @brief Recalculate the geometry of all components
 * @param plot Plot to be layout
 * @param boundingRect Rect where to place the components
 * @param options Options
 * @sa invalidate(), titleRect(), legendRect(), canvasRect()
 */
void QwtPolarLayout::activate(const QwtPolarPlot* plot, const QRectF& boundingRect, Options options)
{
    QWT_D(d);

    invalidate();

    QRectF rect(boundingRect);  // undistributed rest of the plot rect
    int margin = static_cast< int >(d->margin);
    rect.adjust(margin, margin, -margin, -margin);

    // We extract all layout relevant data from the widgets
    // and save them to d->layoutData.

    d->layoutData.init(plot, rect);
    if (!(options & IgnoreLegend) && d->legendPos != QwtPolarPlot::ExternalLegend && plot->legend()
        && !plot->legend()->isEmpty()) {
        d->legendRect = layoutLegend(options, rect);
        if (d->layoutData.legend.frameWidth && !(options & IgnoreFrames)) {
            // In case of a frame we have to insert a spacing.
            // Otherwise the leading of the font separates
            // legend and scale/canvas

            switch (d->legendPos) {
            case QwtPolarPlot::LeftLegend:
                rect.setLeft(rect.left() + d->spacing);
                break;

            case QwtPolarPlot::RightLegend:
                rect.setRight(rect.right() - d->spacing);
                break;

            case QwtPolarPlot::TopLegend:
                rect.setTop(rect.top() + d->spacing);
                break;

            case QwtPolarPlot::BottomLegend:
                rect.setBottom(rect.bottom() - d->spacing);
                break;

            case QwtPolarPlot::ExternalLegend:
                break;  // suppress compiler warning
            }
        }
    }

    if (!(options & IgnoreTitle) && !d->layoutData.title.text.isEmpty()) {
        int h = d->layoutData.title.text.heightForWidth(rect.width());
        if (!(options & IgnoreFrames))
            h += 2 * d->layoutData.title.frameWidth;

        d->titleRect = QRectF(rect.x(), rect.y(), rect.width(), h);

        // subtract title
        rect.setTop(rect.top() + h + d->spacing);
    }

    if (plot->zoomPos().radius() > 0.0 || plot->zoomFactor() < 1.0) {
        // In zoomed state we have no idea about the geometry that
        // is best for the plot. So we use the complete rectangle
        // accepting, that there might a lot of space wasted
        // around the plot.

        d->canvasRect = rect;
    } else {
        // In full state we know, that we want
        // to display something circular.

        const int dim = qMin(rect.width(), rect.height());

        d->canvasRect.setX(rect.center().x() - dim / 2);
        d->canvasRect.setY(rect.y());
        d->canvasRect.setSize(QSize(dim, dim));
    }

    if (!d->legendRect.isEmpty()) {
        if (d->legendPos == QwtPolarPlot::LeftLegend || d->legendPos == QwtPolarPlot::RightLegend) {
            // We prefer to align the legend to the canvas - not to
            // the complete plot - if possible.

            if (d->layoutData.legend.hint.height() < d->canvasRect.height()) {
                d->legendRect.setY(d->canvasRect.y());
                d->legendRect.setHeight(d->canvasRect.height());
            }
        }
    }
}
