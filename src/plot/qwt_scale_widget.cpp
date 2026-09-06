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

#include "qwt_scale_widget.h"
#include "qwt_painter.h"
#include "qwt_color_map.h"
#include "qwt_scale_map.h"
#include "qwt_math.h"
#include "qwt_scale_div.h"
#include "qwt_text.h"
#include "qwt_interval.h"
#include "qwt_scale_engine.h"

#include <qpainter.h>
#include <qevent.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qapplication.h>
#include <qmargins.h>
#include <QDebug>

#ifndef QWTSCALEWIDGET_DEBUG_DRAW
#define QWTSCALEWIDGET_DEBUG_DRAW 0
#endif
class QwtScaleWidget::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtScaleWidget)
public:
    PrivateData(QwtScaleWidget* p) : q_ptr(p)
    {
    }

    ~PrivateData()
    {
    }

    std::unique_ptr< QwtScaleDraw > scaleDraw;

    int borderDist[ 2 ];
    int minBorderDist[ 2 ];
    int scaleLength { 0 };
    int margin { 0 };
    int edgeMargin { 0 };
    int titleOffset { 0 };
    int spacing { 2 };
    QwtText title;

    QwtScaleWidget::LayoutFlags layoutFlags;
    QwtScaleWidget::TitlePosition titlePosition { QwtScaleWidget::TitleCentered };

    // Interaction-related members added for built-in actions
    bool isSelected { false };

    double zoomFactor { 1.2 };  ///< Zoom factor
    QwtScaleWidget::BuiltinActionsFlags builtinActions { QwtScaleWidget::ActionAll };
    QColor selectionColor { Qt::blue };     ///< Selection color
    QColor originTextColor { Qt::black };   ///< Original text color
    QColor originScaleColor { Qt::black };  ///< Original axis color

    struct t_colorBar
    {
        bool isEnabled { false };
        int width { 10 };
        QwtInterval interval;
        std::unique_ptr< QwtColorMap > colorMap;
    } colorBar;
};

/**
 * @brief Create a scale with the position QwtScaleWidget::Left
 * @param parent Parent widget
 */
QwtScaleWidget::QwtScaleWidget(QWidget* parent) : QWidget(parent), QWT_PIMPL_CONSTRUCT
{
    initScale(QwtScaleDraw::LeftScale);
}

/**
 * @brief Constructor
 * @param align Alignment
 * @param parent Parent widget
 */
QwtScaleWidget::QwtScaleWidget(QwtScaleDraw::Alignment align, QWidget* parent) : QWidget(parent), QWT_PIMPL_CONSTRUCT
{
    initScale(align);
}

//! Destructor
QwtScaleWidget::~QwtScaleWidget()
{
}

//! Initialize the scale
void QwtScaleWidget::initScale(QwtScaleDraw::Alignment align)
{
    QWT_D(d);
    if (align == QwtScaleDraw::RightScale)
        d->layoutFlags |= TitleInverted;

    d->borderDist[ 0 ]    = 0;
    d->borderDist[ 1 ]    = 0;
    d->minBorderDist[ 0 ] = 0;
    d->minBorderDist[ 1 ] = 0;

    d->scaleDraw = qwt_make_unique< QwtScaleDraw >();
    d->scaleDraw->setAlignment(align);
    d->scaleDraw->setLength(10);

    d->scaleDraw->setScaleDiv(QwtLinearScaleEngine().divideScale(0.0, 100.0, 10, 5));

    d->colorBar.colorMap = qwt_make_unique< QwtLinearColorMap >();

    const int flags = Qt::AlignHCenter | Qt::TextExpandTabs | Qt::TextWordWrap;
    d->title.setRenderFlags(flags);
    d->title.setFont(font());

    QSizePolicy policy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    if (d->scaleDraw->orientation() == Qt::Vertical)
        policy.transpose();

    setSizePolicy(policy);

    setAttribute(Qt::WA_WState_OwnSizePolicy, false);
}

/**
 * @brief Check if a mouse position falls on the "pure scale area"
 * @param pos Mouse position (relative to this QWidget's coordinates)
 * @return true if on the scale area, false if on margin, edgeMargin, title, colorBar, etc.
 */
bool QwtScaleWidget::isOnScale(const QPoint& pos) const
{
    QRect cr = scaleRect();
    return cr.contains(pos);
}

/**
 * @brief Toggle a layout flag
 * @param flag Layout flag
 * @param on true/false
 * @sa testLayoutFlag(), LayoutFlag
 */
void QwtScaleWidget::setLayoutFlag(LayoutFlag flag, bool on)
{
    if (((m_data->layoutFlags & flag) != 0) != on) {
        if (on)
            m_data->layoutFlags |= flag;
        else
            m_data->layoutFlags &= ~flag;

        update();
    }
}

/**
 * @brief Test a layout flag
 * @param flag Layout flag
 * @return true/false
 * @sa setLayoutFlag(), LayoutFlag
 */
bool QwtScaleWidget::testLayoutFlag(LayoutFlag flag) const
{
    return (m_data->layoutFlags & flag);
}

/**
 * @brief Give title new text contents
 * @param title New title
 * @sa title(), setTitle(const QwtText&)
 */
void QwtScaleWidget::setTitle(const QString& title)
{
    if (m_data->title.text() != title) {
        m_data->title.setText(title);
        layoutScale();
    }
}

/**
 * @brief Give title new text contents
 * @param title New title
 * @sa title()
 * @warning The title flags are interpreted in direction of the label,
 *          AlignTop, AlignBottom can't be set as the title will always be aligned to the scale.
 */
void QwtScaleWidget::setTitle(const QwtText& title)
{
    QwtText t       = title;
    const int flags = title.renderFlags() & ~(Qt::AlignTop | Qt::AlignBottom);
    t.setRenderFlags(flags);

    if (t != m_data->title) {
        m_data->title = t;
        layoutScale();
    }
}

/**
 * @brief Change the alignment
 * @param alignment New alignment
 * @sa alignment()
 */
void QwtScaleWidget::setAlignment(QwtScaleDraw::Alignment alignment)
{
    if (m_data->scaleDraw)
        m_data->scaleDraw->setAlignment(alignment);

    if (!testAttribute(Qt::WA_WState_OwnSizePolicy)) {
        QSizePolicy policy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        if (m_data->scaleDraw->orientation() == Qt::Vertical)
            policy.transpose();

        setSizePolicy(policy);

        setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    }

    layoutScale();
}

/**
 * @brief Get the alignment
 * @return Alignment of the scale
 * @sa setAlignment()
 */
QwtScaleDraw::Alignment QwtScaleWidget::alignment() const
{
    if (!scaleDraw())
        return QwtScaleDraw::LeftScale;

    return scaleDraw()->alignment();
}

/**
 * @brief Specify the margin to the colorBar/base line
 * @param margin Margin
 * @sa margin()
 */
void QwtScaleWidget::setMargin(int margin)
{
    margin = qMax(0, margin);
    if (margin != m_data->margin) {
        m_data->margin = margin;
        layoutScale();
    }
}

/**
 * @brief Specify the distance between color bar, scale and title
 * @param spacing Spacing
 * @sa spacing()
 */
void QwtScaleWidget::setSpacing(int spacing)
{
    spacing = qMax(0, spacing);
    if (spacing != m_data->spacing) {
        m_data->spacing = spacing;
        layoutScale();
    }
}

/**
 * @brief Change the alignment for the labels
 * @param alignment Alignment
 * @sa QwtScaleDraw::setLabelAlignment(), setLabelRotation()
 */
void QwtScaleWidget::setLabelAlignment(Qt::Alignment alignment)
{
    m_data->scaleDraw->setLabelAlignment(alignment);
    layoutScale();
}

/**
 * @brief Change the rotation for the labels
 * @param rotation Rotation angle in degrees
 * @sa QwtScaleDraw::setLabelRotation(), setLabelAlignment()
 */
void QwtScaleWidget::setLabelRotation(double rotation)
{
    m_data->scaleDraw->setLabelRotation(rotation);
    layoutScale();
}

/**
 * @brief Set a scale draw
 * @details scaleDraw has to be created with new and will be deleted in ~QwtScaleWidget()
 *          or the next call of setScaleDraw(). scaleDraw will be initialized with
 *          the attributes of the previous scaleDraw object.
 * @param scaleDraw ScaleDraw object
 * @sa scaleDraw()
 */
void QwtScaleWidget::setScaleDraw(QwtScaleDraw* scaleDraw)
{
    const QwtScaleDraw* sd = m_data->scaleDraw.get();
    if ((scaleDraw == nullptr) || (scaleDraw == sd)) {
        return;
    }
    if (sd) {
        scaleDraw->setAlignment(sd->alignment());
        scaleDraw->setScaleDiv(sd->scaleDiv());

        QwtTransform* transform = nullptr;
        if (sd->scaleMap().transformation())
            transform = sd->scaleMap().transformation()->copy();

        scaleDraw->setTransformation(transform);
    }

    m_data->scaleDraw.reset(scaleDraw);

    layoutScale();
}

/**
 * @brief Get the scale draw (const version)
 * @return scaleDraw of this scale
 * @sa setScaleDraw(), QwtScaleDraw::setScaleDraw()
 */
const QwtScaleDraw* QwtScaleWidget::scaleDraw() const
{
    return m_data->scaleDraw.get();
}

/**
 * @brief Get the scale draw
 * @return scaleDraw of this scale
 * @sa QwtScaleDraw::setScaleDraw()
 */
QwtScaleDraw* QwtScaleWidget::scaleDraw()
{
    return m_data->scaleDraw.get();
}

/**
 * @brief Get the title
 * @return Title text
 * @sa setTitle()
 */
QwtText QwtScaleWidget::title() const
{
    return m_data->title;
}

/**
 * @brief Set the position of the title along the backbone
 * @details Controls where the title is painted along the axis backbone.
 *          The title keeps its natural orientation (vertical for Y axes,
 *          horizontal for X axes). The default is @ref TitleCentered, which
 *          reproduces the legacy behavior.
 * @param position New title position
 * @sa titlePosition(), TitlePosition
 */
void QwtScaleWidget::setTitlePosition(TitlePosition position)
{
    if (position != m_data->titlePosition) {
        m_data->titlePosition = position;
        layoutScale();
    }
}

/**
 * @brief Get the position of the title along the backbone
 * @return Title position
 * @sa setTitlePosition()
 */
QwtScaleWidget::TitlePosition QwtScaleWidget::titlePosition() const
{
    return m_data->titlePosition;
}

/**
 * @brief Set the horizontal alignment of the title text
 * @details The alignment flags control how the title is laid out within its
 *          drawing rectangle (the vertical direction is always pinned to the
 *          scale, as for the legacy behavior). Only the horizontal alignment
 *          bits (@c Qt::AlignLeft, @c Qt::AlignHCenter, @c Qt::AlignRight) are
 *          meaningful; vertical bits are ignored.
 * @param alignment Horizontal alignment flags
 * @sa titleAlignment(), setTitle()
 */
void QwtScaleWidget::setTitleAlignment(Qt::Alignment alignment)
{
    // Keep only the horizontal alignment bits, drop vertical ones.
    const int hFlags = alignment & (Qt::AlignLeft | Qt::AlignHCenter | Qt::AlignRight | Qt::AlignJustify);
    const int flags = hFlags | Qt::TextExpandTabs | Qt::TextWordWrap;
    if (flags != m_data->title.renderFlags()) {
        m_data->title.setRenderFlags(flags);
        layoutScale();
    }
}

/**
 * @brief Get the horizontal alignment of the title text
 * @return Horizontal alignment flags
 * @sa setTitleAlignment()
 */
Qt::Alignment QwtScaleWidget::titleAlignment() const
{
    return static_cast< Qt::Alignment >(m_data->title.renderFlags()
        & (Qt::AlignLeft | Qt::AlignHCenter | Qt::AlignRight | Qt::AlignJustify));
}

/**
 * @brief Get the margin
 * @return Margin value
 * @sa setMargin()
 */
int QwtScaleWidget::margin() const
{
    return m_data->margin;
}

/**
 * @brief Get the distance between scale and title
 * @return Distance between scale and title
 * @sa setSpacing()
 */
int QwtScaleWidget::spacing() const
{
    return m_data->spacing;
}

/**
 * @brief Set the edge margin (offset between axis and plot canvas edge)
 * @details edgeMargin and margin are opposite: margin is the offset from the plot,
 *          edgeMargin is the offset from the plot canvas edge.
 * @param offset Edge margin offset
 */
void QwtScaleWidget::setEdgeMargin(int offset)
{
    offset = qMax(0, offset);
    if (offset != m_data->edgeMargin) {
        m_data->edgeMargin = offset;
        layoutScale();
    }
}

/**
 * @brief Get the edge margin
 * @details The edge margin is the offset between the axis and the plot canvas edge.
 * @return Edge margin value
 */
int QwtScaleWidget::edgeMargin() const
{
    return m_data->edgeMargin;
}

/**
 * @brief Set the font color of the coordinate axis
 * @param c Text color
 */
void QwtScaleWidget::setTextColor(const QColor& c)
{
    // When drawing the title, the color is obtained via QPalette::Text
    // painter->setPen(palette().color(QPalette::Text));
    QPalette p = palette();
    p.setColor(QPalette::Text, c);
    setPalette(p);
}

/**
 * @brief Get the font color of the coordinate axis
 * @return Text color
 */
QColor QwtScaleWidget::textColor() const
{
    return palette().color(QPalette::Text);
}

/**
 * @brief Set the color of the coordinate axis
 * @param c Scale color
 */
void QwtScaleWidget::setScaleColor(const QColor& c)
{
    // QPalette::WindowText
    QPalette p = palette();
    p.setColor(QPalette::WindowText, c);
    setPalette(p);
}

/**
 * @brief Get the color of the coordinate axis
 * @return Scale color
 */
QColor QwtScaleWidget::scaleColor() const
{
    return palette().color(QPalette::WindowText);
}
/*!
   @brief paintEvent
 */
void QwtScaleWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setClipRegion(event->region());

    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

#if QWTSCALEWIDGET_DEBUG_DRAW
    painter.setPen(QPen(Qt::blue, 0.5));
    painter.drawRect(rect().adjusted(1, 1, -1, -1));
#endif
    draw(&painter);
}

/*!
   @brief draw the scale
 */
void QwtScaleWidget::draw(QPainter* painter) const
{

    m_data->scaleDraw->draw(painter, palette());

    if (m_data->colorBar.isEnabled && m_data->colorBar.width > 0 && m_data->colorBar.interval.isValid()) {
        drawColorBar(painter, colorBarRect(contentsRect()));
    }

    QRect r = contentsRect();
    if (m_data->scaleDraw->orientation() == Qt::Horizontal) {
        r.setLeft(r.left() + m_data->borderDist[ 0 ]);
        r.setWidth(r.width() - m_data->borderDist[ 1 ]);
    } else {
        r.setTop(r.top() + m_data->borderDist[ 0 ]);
        r.setHeight(r.height() - m_data->borderDist[ 1 ]);
    }

    if (!m_data->title.isEmpty())
        drawTitle(painter, m_data->scaleDraw->alignment(), r);
}

/**
 * @brief Calculate the rectangle for the color bar
 * @param rect Bounding rectangle for all components of the scale
 * @return Rectangle for the color bar
 */
QRectF QwtScaleWidget::colorBarRect(const QRectF& rect) const
{
    QRectF cr = rect;

    if (m_data->scaleDraw->orientation() == Qt::Horizontal) {
        cr.setLeft(cr.left() + m_data->borderDist[ 0 ]);
        cr.setWidth(cr.width() - m_data->borderDist[ 1 ] + 1);
    } else {
        cr.setTop(cr.top() + m_data->borderDist[ 0 ]);
        cr.setHeight(cr.height() - m_data->borderDist[ 1 ] + 1);
    }

    switch (m_data->scaleDraw->alignment()) {
    case QwtScaleDraw::LeftScale: {
        cr.setLeft(cr.right() - m_data->margin - m_data->colorBar.width);
        cr.setWidth(m_data->colorBar.width);
        break;
    }

    case QwtScaleDraw::RightScale: {
        cr.setLeft(cr.left() + m_data->margin);
        cr.setWidth(m_data->colorBar.width);
        break;
    }

    case QwtScaleDraw::BottomScale: {
        cr.setTop(cr.top() + m_data->margin);
        cr.setHeight(m_data->colorBar.width);
        break;
    }

    case QwtScaleDraw::TopScale: {
        cr.setTop(cr.bottom() - m_data->margin - m_data->colorBar.width);
        cr.setHeight(m_data->colorBar.width);
        break;
    }
    }

    return cr;
}

/**
 * @brief Get the rectangle for drawing the scale (excluding colorBar, margin, edgeMargin, borderDist)
 * @return Rectangle used for drawing the scale
 */
QRect QwtScaleWidget::scaleRect() const
{
    if (!m_data->scaleDraw)  // No scale draw object
        return QRect();

    /* 1. Content area, remove the outer contentsMargins() */
    QRect cr = contentsRect();

    /* 2. Remove user-specified borderDist (empty space at scale endpoints) */
    int bd0, bd1;
    getBorderDistHint(bd0, bd1);  // Minimum required distance
    bd0 = qMax(bd0, m_data->borderDist[ 0 ]);
    bd1 = qMax(bd1, m_data->borderDist[ 1 ]);

    if (m_data->scaleDraw->orientation() == Qt::Vertical) {
        cr.adjust(0, bd0, 0, -bd1);  // Top and bottom ends
    } else {
        cr.adjust(bd0, 0, -bd1, 0);  // Left and right ends
    }

    /* 3. Remove the area occupied by colorBar (if enabled) */
    if (m_data->colorBar.isEnabled && m_data->colorBar.width > 0 && m_data->colorBar.interval.isValid()) {
        const int cw = m_data->colorBar.width + m_data->spacing;
        switch (m_data->scaleDraw->alignment()) {
        case QwtScaleDraw::LeftScale:
            cr.adjust(0, 0, -cw, 0);  // Subtract from right
            break;
        case QwtScaleDraw::RightScale:
            cr.adjust(cw, 0, 0, 0);  // Subtract from left
            break;
        case QwtScaleDraw::TopScale:
            cr.adjust(0, cw, 0, 0);  // Subtract from bottom
            break;
        case QwtScaleDraw::BottomScale:
            cr.adjust(0, 0, 0, -cw);  // Subtract from top
            break;
        }
    }

    /* 4. Remove margin and edgeMargin */
    switch (m_data->scaleDraw->alignment()) {
    case QwtScaleDraw::LeftScale:
        cr.adjust(m_data->edgeMargin, 0, -(m_data->margin), 0);  // Subtract margin from right, edgeMargin from left
        break;
    case QwtScaleDraw::RightScale:
        cr.adjust(m_data->margin, 0, -(m_data->edgeMargin), 0);  // Subtract margin from left, edgeMargin from right
        break;
    case QwtScaleDraw::TopScale:
        cr.adjust(0, m_data->edgeMargin, 0, -(m_data->margin));  // Subtract margin from bottom, edgeMargin from top
        break;
    case QwtScaleDraw::BottomScale:
        cr.adjust(0, m_data->margin, 0, -(m_data->edgeMargin));  // Subtract margin from top, edgeMargin from bottom
        break;
    }

    return cr;
}

/*!
   Change Event handler
   @param event Change event

   Invalidates internal caches if necessary
 */
void QwtScaleWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LocaleChange) {
        m_data->scaleDraw->invalidateCache();
    }

    QWidget::changeEvent(event);
}

/*!
   Event handler for resize events
   @param event Resize event
 */
void QwtScaleWidget::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    layoutScale(false);
}

/**
 * @brief Recalculate the scale's geometry and layout based on the current geometry and fonts
 * @param update_geometry Notify the layout system and call update to redraw the scale
 */
void QwtScaleWidget::layoutScale(bool update_geometry)
{
    int bd0, bd1;
    getBorderDistHint(bd0, bd1);
    if (m_data->borderDist[ 0 ] > bd0)
        bd0 = m_data->borderDist[ 0 ];
    if (m_data->borderDist[ 1 ] > bd1)
        bd1 = m_data->borderDist[ 1 ];

    int colorBarWidth = 0;
    if (m_data->colorBar.isEnabled && m_data->colorBar.interval.isValid())
        colorBarWidth = m_data->colorBar.width + m_data->spacing;

    const QRectF r = contentsRect();
    double x, y, length;
    // Scale offset is independent of edgeMargin
    if (m_data->scaleDraw->orientation() == Qt::Vertical) {
        y      = r.top() + bd0;
        length = r.height() - (bd0 + bd1);

        if (m_data->scaleDraw->alignment() == QwtScaleDraw::LeftScale) {  // Left-aligned scale, typically the right axis
            x = r.right() - 1.0 - m_data->margin - colorBarWidth;
        } else {
            x = r.left() + m_data->margin + colorBarWidth;
        }
    } else {
        x      = r.left() + bd0;
        length = r.width() - (bd0 + bd1);

        if (m_data->scaleDraw->alignment() == QwtScaleDraw::BottomScale) {
            y = r.top() + m_data->margin + colorBarWidth;
        } else {
            y = r.bottom() - 1.0 - m_data->margin - colorBarWidth;
        }
    }

    m_data->scaleDraw->move(x, y);
    m_data->scaleDraw->setLength(length);

    const int extent = qwtCeil(m_data->scaleDraw->extent(font()));
    // titleOffset is independent of edgeMargin
    m_data->titleOffset = m_data->margin + m_data->spacing + colorBarWidth + extent;

    if (update_geometry) {
        updateGeometry();

#if 1
        /*
            for some reason updateGeometry does not send a LayoutRequest event
            when the parent is not visible and has no layout
         */

        if (QWidget* w = parentWidget()) {
            if (!w->isVisible() && w->layout() == nullptr) {
                if (w->testAttribute(Qt::WA_WState_Polished))
                    QApplication::postEvent(w, new QEvent(QEvent::LayoutRequest));
            }
        }
#endif

        update();
    }
}

/**
 * @brief Get the axis ID for this scale widget
 * @note This function converts based on QwtScaleDraw alignment. If alignment is not set, returns QwtAxis::AxisPositions.
 * @return Axis ID
 */
QwtAxisId QwtScaleWidget::axisID() const
{
    const QwtScaleDraw* sd = scaleDraw();
    switch (sd->alignment()) {
    case QwtScaleDraw::BottomScale:
        return QwtAxis::XBottom;
    case QwtScaleDraw::TopScale:
        return QwtAxis::XTop;
    case QwtScaleDraw::LeftScale:
        return QwtAxis::YLeft;
    case QwtScaleDraw::RightScale:
        return QwtAxis::YRight;
    default:
        break;
    }
    return QwtAxis::AxisPositions;
}

/**
 * @brief Check if this is an X axis
 * @return true if this is an X axis
 */
bool QwtScaleWidget::isXAxis() const
{
    return QwtAxis::isXAxis(axisID());
}

/**
 * @brief Check if this is a Y axis
 * @return true if this is a Y axis
 */
bool QwtScaleWidget::isYAxis() const
{
    return QwtAxis::isYAxis(axisID());
}

/**
 * @brief Set the built-in actions
 * @param actions Built-in action flags
 * @sa BuiltinActions
 */
void QwtScaleWidget::setBuildinActions(BuiltinActionsFlags acts)
{
    if (m_data->builtinActions != acts) {
        m_data->builtinActions = acts;
    }
}

/**
 * @brief Get the built-in actions flags
 * @return Built-in action flags
 */
QwtScaleWidget::BuiltinActionsFlags QwtScaleWidget::buildinActions() const
{
    return m_data->builtinActions;
}

/**
 * @brief Test if a built-in action is active
 * @param ba Built-in action to test
 * @return true if the action is active
 */
bool QwtScaleWidget::testBuildinActions(QwtScaleWidget::BuiltinActions ba) const
{
    return m_data->builtinActions.testFlag(ba);
}

/**
 * @brief Set the selected state
 * @details This function triggers the selectionChanged signal.
 *          Setting the same state repeatedly will not trigger the signal again.
 * @param selected true to select, false to deselect
 */
void QwtScaleWidget::setSelected(bool selected)
{
    QWT_D(d);
    if (d->isSelected != selected) {
        if (!(d->isSelected)) {
            d->originTextColor  = textColor();
            d->originScaleColor = scaleColor();
            setTextColor(d->selectionColor);
            setScaleColor(d->selectionColor);
        } else {
            setTextColor(d->originTextColor);
            setScaleColor(d->originScaleColor);
        }
        d->scaleDraw->setSelected(selected);
        d->isSelected = selected;
        update();
        Q_EMIT selectionChanged(selected);
    }
}

/**
 * @brief Check if the current axis is selected
 * @return true if selected, false otherwise
 */
bool QwtScaleWidget::isSelected() const
{
    return m_data->isSelected;
}

/**
 * @brief Set the selection color
 * @param color Selection color
 */
void QwtScaleWidget::setSelectionColor(const QColor& color)
{
    if (m_data->selectionColor != color) {
        m_data->selectionColor = color;
        if (m_data->isSelected) {
            update();
        }
    }
}

/**
 * @brief Get the selection color
 * @return Selection color
 */
QColor QwtScaleWidget::selectionColor() const
{
    return m_data->selectionColor;
}

/**
 * @brief Set the zoom factor (default 1.2)
 * @param factor Zoom factor (range 0.1 to 10.0)
 */
void QwtScaleWidget::setZoomFactor(double factor)
{
    m_data->zoomFactor = qMax(0.1, qMin(10.0, factor));
}

/**
 * @brief Get the zoom factor
 * @return Zoom factor
 */
double QwtScaleWidget::zoomFactor() const
{
    return m_data->zoomFactor;
}

/**
 * @brief Set the pen width offset for the axis when it is in selected state
 * @details When an axis (e.g., X-axis or Y-axis) is selected by the user, the pen width used
 *          for drawing will be increased by this offset value, achieving a visual highlighting effect.
 * @param offset The additional width value to be added when selected (unit: pixels).
 *               This value should be non-negative. If it is 0, the line width in selected state
 *               will be the same as in normal state.
 * @sa QwtScaleWidget::selectedPenWidthOffset, QwtAbstractScaleDraw::setSelectedPenWidthOffset
 */
void QwtScaleWidget::setSelectedPenWidthOffset(qreal offset)
{
    m_data->scaleDraw->setSelectedPenWidthOffset(offset);
}

/**
 * @brief Get the current pen width offset for the axis when it is in selected state
 * @return The current width offset value
 * @sa QwtScaleWidget::setSelectedPenWidthOffset, QwtAbstractScaleDraw::selectedPenWidthOffset
 */
qreal QwtScaleWidget::selectedPenWidthOffset() const
{
    return m_data->scaleDraw->selectedPenWidthOffset();
}

/**
 * @brief Draw the color bar of the scale widget
 * @param painter Painter
 * @param rect Bounding rectangle for the color bar
 * @sa setColorBarEnabled()
 */
void QwtScaleWidget::drawColorBar(QPainter* painter, const QRectF& rect) const
{
    QWT_DC(d);
    if (!d->colorBar.interval.isValid()) {
        return;
    }

    QwtPainter::drawColorBar(painter,
                             *(d->colorBar.colorMap),
                             d->colorBar.interval.normalized(),
                             d->scaleDraw->scaleMap(),
                             d->scaleDraw->orientation(),
                             rect);
}

/**
 * @brief Rotate and paint a title according to its position into a given rectangle
 * @param painter Painter
 * @param align Alignment
 * @param rect Bounding rectangle
 */
void QwtScaleWidget::drawTitle(QPainter* painter, QwtScaleDraw::Alignment align, const QRectF& rect) const
{
    QRectF r = rect;
    double angle;
    int flags = m_data->title.renderFlags() & ~(Qt::AlignTop | Qt::AlignBottom | Qt::AlignVCenter);

    switch (align) {
    case QwtScaleDraw::LeftScale:
        angle = -90.0;
        flags |= Qt::AlignTop;
        r.setRect(r.left() + m_data->edgeMargin, r.bottom(), r.height(), r.width() - m_data->titleOffset);
        break;

    case QwtScaleDraw::RightScale:
        angle = -90.0;
        flags |= Qt::AlignTop;
        r.setRect(r.left() + m_data->titleOffset, r.bottom(), r.height(), r.width() - m_data->titleOffset - m_data->edgeMargin);
        break;

    case QwtScaleDraw::BottomScale:
        angle = 0.0;
        flags |= Qt::AlignBottom;
        r.setTop(r.top() + m_data->titleOffset);
        r.setBottom(r.bottom() - m_data->edgeMargin);
        break;

    case QwtScaleDraw::TopScale:
    default:
        angle = 0.0;
        flags |= Qt::AlignTop;
        r.setBottom(r.bottom() - m_data->titleOffset);
        r.setTop(r.top() + m_data->edgeMargin);
        break;
    }

    if (m_data->layoutFlags & TitleInverted) {
        if (align == QwtScaleDraw::LeftScale || align == QwtScaleDraw::RightScale) {
            angle = -angle;
            r.setRect(r.x() + r.height(), r.y() - r.width(), r.width(), r.height());
        }
    }

    painter->save();
    painter->setFont(font());
    painter->setPen(palette().color(QPalette::Text));

    painter->translate(r.x(), r.y());
    if (angle != 0.0)
        painter->rotate(angle);

    // After the translate/rotate above the local coordinate system is laid out
    // so that the local x axis runs along the backbone (extent = r.width()) and
    // the local y axis runs across the title thickness (extent = r.height()).
    // Restrict the drawing rectangle along the backbone according to the
    // requested title position. TitleCentered keeps the full backbone extent,
    // reproducing the legacy layout exactly.
    double x0 = 0.0;
    double drawW = r.width();
    if (m_data->titlePosition != TitleCentered) {
        // Natural length of the title along its reading direction.
        double natLen = m_data->title.textSize(font()).width();
        if (natLen > r.width())
            natLen = r.width();  // falls back to the full span, wrapping as before

        drawW = natLen;
        const bool atEnd = (m_data->titlePosition == TitleAtEnd);

        if (align == QwtScaleDraw::LeftScale || align == QwtScaleDraw::RightScale) {
            // Vertical axis. With angle == -90 local x=0 is at the bottom end,
            // with angle == +90 (TitleInverted) local x=0 is at the top end.
            // The contract is positional: AtStart = bottom, AtEnd = top.
            const bool localZeroAtBottom = (angle < 0.0);
            const bool wantBottom = !atEnd;
            const bool offsetToEnd = (localZeroAtBottom != wantBottom);
            x0 = offsetToEnd ? (r.width() - natLen) : 0.0;
        } else {
            // Horizontal axis: local x=0 is at the left end.
            // AtStart = left, AtEnd = right.
            x0 = atEnd ? (r.width() - natLen) : 0.0;
        }
    }

    QwtText title = m_data->title;
    title.setRenderFlags(flags);
    title.draw(painter, QRectF(x0, 0.0, drawW, r.height()));

    painter->restore();
}

/*!
   @brief Notify a change of the scale

   This virtual function can be overloaded by derived
   classes. The default implementation updates the geometry
   and repaints the widget.
 */

void QwtScaleWidget::scaleChange()
{
    layoutScale();
}

/**
 * @brief Get a size hint
 * @return Size hint
 */
QSize QwtScaleWidget::sizeHint() const
{
    return minimumSizeHint();
}

/**
 * @brief Get a minimum size hint
 * @return Minimum size hint
 */
QSize QwtScaleWidget::minimumSizeHint() const
{
    const Qt::Orientation o = m_data->scaleDraw->orientation();

    // Border Distance cannot be less than the scale borderDistHint
    // Note, the borderDistHint is already included in minHeight/minWidth
    int length = 0;
    int mbd1, mbd2;
    getBorderDistHint(mbd1, mbd2);
    length += qMax(0, m_data->borderDist[ 0 ] - mbd1);
    length += qMax(0, m_data->borderDist[ 1 ] - mbd2);
    length += m_data->scaleDraw->minLength(font());

    int dim = dimForLength(length, font());
    if (length < dim) {
        // compensate for long titles
        length = dim;
        dim    = dimForLength(length, font());
    }

    QSize size(length + 2, dim);
    if (o == Qt::Vertical)
        size.transpose();

    const QMargins m = contentsMargins();
    return size + QSize(m.left() + m.right(), m.top() + m.bottom());
}

/**
 * @brief Find the height of the title for a given width
 * @param width Width
 * @return Height
 */
int QwtScaleWidget::titleHeightForWidth(int width) const
{
    return qwtCeil(m_data->title.heightForWidth(width, font()));
}

/**
 * @brief Find the minimum dimension for a given length
 * @details dim is the height, length the width seen in direction of the title.
 * @param length Width for horizontal, height for vertical scales
 * @param scaleFont Font of the scale
 * @return Height for horizontal, width for vertical scales
 */
int QwtScaleWidget::dimForLength(int length, const QFont& scaleFont) const
{
    const int extent = qwtCeil(m_data->scaleDraw->extent(scaleFont));

    int dim = m_data->margin + extent + 1 + m_data->edgeMargin;

    if (!m_data->title.isEmpty())
        dim += titleHeightForWidth(length) + m_data->spacing;

    if (m_data->colorBar.isEnabled && m_data->colorBar.interval.isValid())
        dim += m_data->colorBar.width + m_data->spacing;

    return dim;
}

/**
 * @brief Specify distances of the scale's endpoints from the widget's borders
 * @details The actual borders will never be less than minimum border distance.
 * @param dist1 Left or top distance
 * @param dist2 Right or bottom distance
 * @sa startBorderDist(), endBorderDist()
 */
void QwtScaleWidget::setBorderDist(int dist1, int dist2)
{
    if (dist1 != m_data->borderDist[ 0 ] || dist2 != m_data->borderDist[ 1 ]) {
        m_data->borderDist[ 0 ] = dist1;
        m_data->borderDist[ 1 ] = dist2;
        layoutScale();
    }
}

/**
 * @brief Get the start border distance
 * @return Start border distance
 * @sa setBorderDist()
 */
int QwtScaleWidget::startBorderDist() const
{
    return m_data->borderDist[ 0 ];
}

/**
 * @brief Get the end border distance
 * @return End border distance
 * @sa setBorderDist()
 */
int QwtScaleWidget::endBorderDist() const
{
    return m_data->borderDist[ 1 ];
}

/**
 * @brief Calculate a hint for the border distances
 * @details This member function calculates the distance of the scale's endpoints
 *          from the widget borders which is required for the mark labels to fit
 *          into the widget. The maximum of this distance and the minimum border
 *          distance is returned.
 * @param[out] start Border width at the beginning of the scale
 * @param[out] end Border width at the end of the scale
 * @warning The minimum border distance depends on the font.
 * @sa setMinBorderDist(), getMinBorderDist(), setBorderDist()
 */
void QwtScaleWidget::getBorderDistHint(int& start, int& end) const
{
    m_data->scaleDraw->getBorderDistHint(font(), start, end);

    if (start < m_data->minBorderDist[ 0 ])
        start = m_data->minBorderDist[ 0 ];

    if (end < m_data->minBorderDist[ 1 ])
        end = m_data->minBorderDist[ 1 ];
}

/**
 * @brief Set a minimum value for the distances of the scale's endpoints from the widget borders
 * @details This is useful to avoid that the scales are "jumping" when the tick labels
 *          or their positions change often.
 * @param start Minimum for the start border
 * @param end Minimum for the end border
 * @sa getMinBorderDist(), getBorderDistHint(), startMinBorderDist(), endMinBorderDist()
 */
void QwtScaleWidget::setMinBorderDist(int start, int end)
{
    m_data->minBorderDist[ 0 ] = start;
    m_data->minBorderDist[ 1 ] = end;
}

/**
 * @brief Get the minimum value for the distances of the scale's endpoints from the widget borders (left or top)
 * @return Start minimum border distance
 * @sa getMinBorderDist(), getBorderDistHint()
 */
int QwtScaleWidget::startMinBorderDist() const
{
    return m_data->minBorderDist[ 0 ];
}

/**
 * @brief Get the minimum value for the distances of the scale's endpoints from the widget borders (right or bottom)
 * @return End minimum border distance
 * @sa getMinBorderDist(), getBorderDistHint()
 */
int QwtScaleWidget::endMinBorderDist() const
{
    return m_data->minBorderDist[ 1 ];
}

/**
 * @brief Get the minimum value for the distances of the scale's endpoints from the widget borders
 * @param[out] start Border width at the beginning of the scale
 * @param[out] end Border width at the end of the scale
 * @sa setMinBorderDist(), getBorderDistHint()
 */
void QwtScaleWidget::getMinBorderDist(int& start, int& end) const
{
    start = m_data->minBorderDist[ 0 ];
    end   = m_data->minBorderDist[ 1 ];
}

/**
 * @brief Assign a scale division
 * @details The scale division determines where to set the tick marks.
 * @param scaleDiv Scale Division
 * @sa For more information about scale divisions, see QwtScaleDiv.
 */
void QwtScaleWidget::setScaleDiv(const QwtScaleDiv& scaleDiv)
{
    QWT_D(d);
    if (d->scaleDraw->scaleDiv() != scaleDiv) {
        d->scaleDraw->setScaleDiv(scaleDiv);
        layoutScale();

        Q_EMIT scaleDivChanged();
    }
}

/**
 * @brief Set the transformation
 * @param transformation Transformation
 * @sa QwtAbstractScaleDraw::scaleDraw(), QwtScaleMap
 */
void QwtScaleWidget::setTransformation(QwtTransform* transformation)
{
    m_data->scaleDraw->setTransformation(transformation);
    layoutScale();
}

/**
 * @brief Enable/disable a color bar associated to the scale
 * @param on true to enable, false to disable
 * @sa isColorBarEnabled(), setColorBarWidth()
 */
void QwtScaleWidget::setColorBarEnabled(bool on)
{
    if (on != m_data->colorBar.isEnabled) {
        m_data->colorBar.isEnabled = on;
        layoutScale();
    }
}

/**
 * @brief Check if color bar is enabled
 * @return true when the color bar is enabled
 * @sa setColorBarEnabled(), setColorBarWidth()
 */
bool QwtScaleWidget::isColorBarEnabled() const
{
    return m_data->colorBar.isEnabled;
}

/**
 * @brief Set the width of the color bar
 * @param width Width
 * @sa colorBarWidth(), setColorBarEnabled()
 */
void QwtScaleWidget::setColorBarWidth(int width)
{
    if (width != m_data->colorBar.width) {
        m_data->colorBar.width = width;
        if (isColorBarEnabled())
            layoutScale();
    }
}

/**
 * @brief Get the width of the color bar
 * @return Width of the color bar
 * @sa setColorBarEnabled(), setColorBarWidth()
 */
int QwtScaleWidget::colorBarWidth() const
{
    return m_data->colorBar.width;
}

/**
 * @brief Get the value interval for the color bar
 * @return Value interval for the color bar
 * @sa setColorMap(), colorMap()
 */
QwtInterval QwtScaleWidget::colorBarInterval() const
{
    return m_data->colorBar.interval;
}

/**
 * @brief Set the color map and value interval used for displaying the color bar
 * @param interval Value interval
 * @param colorMap Color map
 * @sa colorMap(), colorBarInterval()
 */
void QwtScaleWidget::setColorMap(const QwtInterval& interval, QwtColorMap* colorMap)
{
    m_data->colorBar.interval = interval;

    if (colorMap != m_data->colorBar.colorMap.get()) {
        m_data->colorBar.colorMap.reset(colorMap);
    }

    if (isColorBarEnabled())
        layoutScale();
}

/**
 * @brief Get the color map
 * @return Color map
 * @sa setColorMap(), colorBarInterval()
 */
const QwtColorMap* QwtScaleWidget::colorMap() const
{
    return m_data->colorBar.colorMap.get();
}
