/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
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

    // 内置动作新增的 交互相关成员
    bool isSelected { false };

    double zoomFactor { 1.2 };  ///< 缩放系数
    QwtScaleWidget::BuiltinActionsFlags builtinActions { QwtScaleWidget::ActionAll };
    QColor selectionColor { Qt::blue };

    struct t_colorBar
    {
        bool isEnabled { false };
        int width { 10 };
        QwtInterval interval;
        std::unique_ptr< QwtColorMap > colorMap;
    } colorBar;
};

/*!
   \brief Create a scale with the position QwtScaleWidget::Left
   \param parent Parent widget
 */
QwtScaleWidget::QwtScaleWidget(QWidget* parent) : QWidget(parent), QWT_PIMPL_CONSTRUCT
{
    initScale(QwtScaleDraw::LeftScale);
}

/*!
   \brief Constructor
   \param align Alignment.
   \param parent Parent widget
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

void QwtScaleWidget::doZoom(double factor, const QPoint& centerPos)
{
    const QwtScaleDiv& currentDiv = m_data->scaleDraw->scaleDiv();
    const double centerValue      = mapPosToScaleValue(centerPos);
    const double currentWidth     = currentDiv.upperBound() - currentDiv.lowerBound();
    const double newWidth         = currentWidth / factor;

    // 防止过度缩放
    if (newWidth < 1e-8 || newWidth > 1e8) {
        return;
    }

    const double newLower = centerValue - (centerValue - currentDiv.lowerBound()) / factor;
    const double newUpper = centerValue + (currentDiv.upperBound() - centerValue) / factor;

    Q_EMIT requestScaleRangeUpdate(newLower, newUpper);
}

/**
 * @brief 按照像素移动坐标轴
 * @param deltaPixels 移动的像素
 * @note 此函数会发射@ref requestScaleRangeUpdate 信号请求绘图的改变
 */
void QwtScaleWidget::panScale(int deltaPixels)
{
    const QwtScaleDraw* sd = m_data->scaleDraw.get();
    if (!sd) {
        return;
    }
    // 对于垂直轴，需要考虑坐标方向
    // 数学坐标系：向上移动应该是正值，但屏幕坐标系向下移动是正值
    // 所以需要取反
    // 水平轴向右移动，实际是刻度在减，也是负值，因此这里都是负值
    const double valueDist        = mapLengthToScaleValue(-deltaPixels);
    const QwtScaleDiv& currentDiv = sd->scaleDiv();
    Q_EMIT requestScaleRangeUpdate(currentDiv.lowerBound() + valueDist, currentDiv.upperBound() + valueDist);
}

/**
 * @brief 坐标轴放大
 * @param centerPos
 */
void QwtScaleWidget::zoomIn(const QPoint& centerPos)
{
    doZoom(m_data->zoomFactor, centerPos);
}

/**
 * @brief  坐标轴缩小
 * @param centerPos
 */
void QwtScaleWidget::zoomOut(const QPoint& centerPos)
{
    doZoom(1.0 / m_data->zoomFactor, centerPos);
}

/**
 * @brief 把当前scalewidget窗口上的屏幕点映射到坐标轴上的值
 * @param pos 窗口上的点
 * @return 坐标轴上的值
 */
double QwtScaleWidget::mapPosToScaleValue(const QPoint& pos) const
{
    const QwtScaleDraw* sd = m_data->scaleDraw.get();
    if (!sd) {
        return 0.0;
    }
    const QwtScaleMap& scaleMap = sd->scaleMap();

    QRectF scaleRect = contentsRect();
    if (sd->orientation() == Qt::Vertical) {
        scaleRect.setTop(scaleRect.top() + m_data->borderDist[ 0 ]);
        scaleRect.setHeight(scaleRect.height() - m_data->borderDist[ 0 ] - m_data->borderDist[ 1 ]);
        return scaleMap.invTransform(pos.y() - scaleRect.top());
    } else {
        scaleRect.setLeft(scaleRect.left() + m_data->borderDist[ 0 ]);
        scaleRect.setWidth(scaleRect.width() - m_data->borderDist[ 0 ] - m_data->borderDist[ 1 ]);
        return scaleMap.invTransform(pos.x() - scaleRect.left());
    }
}

/**
 * @brief 把当前scalewidget窗口上的屏幕长度映射到坐标轴上的长度值
 * @param length 屏幕长度
 * @return 坐标轴的长度值
 */
double QwtScaleWidget::mapLengthToScaleValue(double length) const
{
    const QwtScaleDraw* sd = m_data->scaleDraw.get();
    if (!sd) {
        return 0.0;
    }
    const QwtScaleMap& map = sd->scaleMap();
    const double valueDist = map.invTransform(length) - map.invTransform(0);
    return valueDist;
}

/**
 * @brief 判断鼠标位置是否落在“纯刻度区域”
 * @param pos 鼠标位置（相对于本 QWidget 的坐标）
 * @return true  落在刻度上
 *         false 落在 margin、edgeMargin、标题、colorBar 等空白处
 */
bool QwtScaleWidget::isOnScale(const QPoint& pos) const
{
    QRect cr = scaleRect();
    return cr.contains(pos);
}

/*!
   Toggle an layout flag

   \param flag Layout flag
   \param on true/false

   \sa testLayoutFlag(), LayoutFlag
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

/*!
   Test a layout flag

   \param flag Layout flag
   \return true/false
   \sa setLayoutFlag(), LayoutFlag
 */
bool QwtScaleWidget::testLayoutFlag(LayoutFlag flag) const
{
    return (m_data->layoutFlags & flag);
}

/*!
   Give title new text contents

   \param title New title
   \sa title(), setTitle(const QwtText &);
 */
void QwtScaleWidget::setTitle(const QString& title)
{
    if (m_data->title.text() != title) {
        m_data->title.setText(title);
        layoutScale();
    }
}

/*!
   Give title new text contents

   \param title New title
   \sa title()
   \warning The title flags are interpreted in
               direction of the label, AlignTop, AlignBottom can't be set
               as the title will always be aligned to the scale.
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

/*!
   Change the alignment

   \param alignment New alignment
   \sa alignment()
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

/*!
    \return position
    \sa setPosition()
 */
QwtScaleDraw::Alignment QwtScaleWidget::alignment() const
{
    if (!scaleDraw())
        return QwtScaleDraw::LeftScale;

    return scaleDraw()->alignment();
}

/*!
   \brief Specify the margin to the colorBar/base line.
   \param margin Margin
   \sa margin()
 */
void QwtScaleWidget::setMargin(int margin)
{
    margin = qMax(0, margin);
    if (margin != m_data->margin) {
        m_data->margin = margin;
        layoutScale();
    }
}

/*!
   \brief Specify the distance between color bar, scale and title
   \param spacing Spacing
   \sa spacing()
 */
void QwtScaleWidget::setSpacing(int spacing)
{
    spacing = qMax(0, spacing);
    if (spacing != m_data->spacing) {
        m_data->spacing = spacing;
        layoutScale();
    }
}

/*!
   \brief Change the alignment for the labels.

   \sa QwtScaleDraw::setLabelAlignment(), setLabelRotation()
 */
void QwtScaleWidget::setLabelAlignment(Qt::Alignment alignment)
{
    m_data->scaleDraw->setLabelAlignment(alignment);
    layoutScale();
}

/*!
   \brief Change the rotation for the labels.
   See QwtScaleDraw::setLabelRotation().

   \param rotation Rotation
   \sa QwtScaleDraw::setLabelRotation(), setLabelFlags()
 */
void QwtScaleWidget::setLabelRotation(double rotation)
{
    m_data->scaleDraw->setLabelRotation(rotation);
    layoutScale();
}

/*!
   Set a scale draw

   scaleDraw has to be created with new and will be deleted in
   ~QwtScaleWidget() or the next call of setScaleDraw().
   scaleDraw will be initialized with the attributes of
   the previous scaleDraw object.

   \param scaleDraw ScaleDraw object
   \sa scaleDraw()
 */
void QwtScaleWidget::setScaleDraw(QwtScaleDraw* scaleDraw)
{
    const QwtScaleDraw* sd = m_data->scaleDraw.get();
    if ((scaleDraw == NULL) || (scaleDraw == sd)) {
        return;
    }
    if (sd) {
        scaleDraw->setAlignment(sd->alignment());
        scaleDraw->setScaleDiv(sd->scaleDiv());

        QwtTransform* transform = NULL;
        if (sd->scaleMap().transformation())
            transform = sd->scaleMap().transformation()->copy();

        scaleDraw->setTransformation(transform);
    }

    m_data->scaleDraw.reset(scaleDraw);

    layoutScale();
}

/*!
    \return scaleDraw of this scale
    \sa setScaleDraw(), QwtScaleDraw::setScaleDraw()
 */
const QwtScaleDraw* QwtScaleWidget::scaleDraw() const
{
    return m_data->scaleDraw.get();
}

/*!
    \return scaleDraw of this scale
    \sa QwtScaleDraw::setScaleDraw()
 */
QwtScaleDraw* QwtScaleWidget::scaleDraw()
{
    return m_data->scaleDraw.get();
}

/*!
    \return title
    \sa setTitle()
 */
QwtText QwtScaleWidget::title() const
{
    return m_data->title;
}

/*!
    \return margin
    \sa setMargin()
 */
int QwtScaleWidget::margin() const
{
    return m_data->margin;
}

/*!
    \return distance between scale and title
    \sa setMargin()
 */
int QwtScaleWidget::spacing() const
{
    return m_data->spacing;
}

/**
 * @brief 设置坐标轴和绘图边缘的空白偏移距离
 *
 * edgeMargin和margin刚好相反，margin是和绘图的偏移，edgeMargin是和绘图边缘的偏移
 * @param offset
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
 * @brief 边缘距离
 *
 * @code
 * │<----------------------------- plot yleft edge
 * │      │       │      │tick ┌       ┌-----------------------------------
 * │      │       │      │label│       |
 * │edge  │YLeft  │space │ 6  -│margin │
 * │margin│Title  │      │     │       │
 * │      │       │      │ 5  -│       │
 * │      │       │      │     │       │
 * │      │       │      │ 4  -│       │ plot cavans
 * │      │       │      │     │       │
 * │      │       │      │ 3  -│       │
 * │      │       │      │     │       │
 * │      │       │      │ 2  -│       │
 * │      │       │      │     │       │
 * │      │       │      │ 1  -│       |_________________________________
 * @endcode
 * @return 边缘距离
 */
int QwtScaleWidget::edgeMargin() const
{
    return m_data->edgeMargin;
}

/**
 * @brief Set the font color of the coordinate axis/设置坐标轴的字体颜色
 * @param c
 */
void QwtScaleWidget::setTextColor(const QColor& c)
{
    // 绘制标题时是通过QPalette::Text获取颜色
    // painter->setPen(palette().color(QPalette::Text));
    QPalette p = palette();
    p.setColor(QPalette::Text, c);
    setPalette(p);
}

/**
 * @brief font color of the coordinate axis/坐标轴的字体颜色
 * @return
 */
QColor QwtScaleWidget::textColor() const
{
    return palette().color(QPalette::Text);
}

/**
 * @brief set color of the coordinate axis/设置坐标轴的颜色
 * @param c
 */
void QwtScaleWidget::setScaleColor(const QColor& c)
{
    // QPalette::WindowText
    QPalette p = palette();
    p.setColor(QPalette::WindowText, c);
    setPalette(p);
}

/**
 * @brief color of the coordinate axis/坐标轴的颜色
 * @return
 */
QColor QwtScaleWidget::scaleColor() const
{
    return palette().color(QPalette::WindowText);
}
/*!
   \brief paintEvent
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
   \brief draw the scale
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

/*!
   Calculate the the rectangle for the color bar

   \param rect Bounding rectangle for all components of the scale
   \return Rectangle for the color bar
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
 * @brief 去除了colorBar,margin,edgeMargin,BorderDistHint这些区域的矩形，也就是用来绘制刻度的区域
 * @return
 */
QRect QwtScaleWidget::scaleRect() const
{
    if (!m_data->scaleDraw)  // 无刻度对象
        return QRect();

    /* 1. 内容区，去掉外围的 contentsMargins() */
    QRect cr = contentsRect();

    /* 2. 再去掉用户设定的 borderDist（刻度两端留空） */
    int bd0, bd1;
    getBorderDistHint(bd0, bd1);  // 最小必需距离
    bd0 = qMax(bd0, m_data->borderDist[ 0 ]);
    bd1 = qMax(bd1, m_data->borderDist[ 1 ]);

    if (m_data->scaleDraw->orientation() == Qt::Vertical) {
        cr.adjust(0, bd0, 0, -bd1);  // 上下两端
    } else {
        cr.adjust(bd0, 0, -bd1, 0);  // 左右两端
    }

    /* 3. 再去掉 colorBar 占用的区域（如果启用） */
    if (m_data->colorBar.isEnabled && m_data->colorBar.width > 0 && m_data->colorBar.interval.isValid()) {
        const int cw = m_data->colorBar.width + m_data->spacing;
        switch (m_data->scaleDraw->alignment()) {
        case QwtScaleDraw::LeftScale:
            cr.adjust(0, 0, -cw, 0);  // 右边减掉
            break;
        case QwtScaleDraw::RightScale:
            cr.adjust(cw, 0, 0, 0);  // 左边减掉
            break;
        case QwtScaleDraw::TopScale:
            cr.adjust(0, cw, 0, 0);  // 下边减掉
            break;
        case QwtScaleDraw::BottomScale:
            cr.adjust(0, 0, 0, -cw);  // 上边减掉
            break;
        }
    }

    /* 4. 去掉margin和edgeMargin */
    switch (m_data->scaleDraw->alignment()) {
    case QwtScaleDraw::LeftScale:
        cr.adjust(m_data->edgeMargin, 0, -(m_data->margin), 0);  // 右边减掉margin,左边减掉edgeMargin
        break;
    case QwtScaleDraw::RightScale:
        cr.adjust(m_data->margin, 0, -(m_data->edgeMargin), 0);  // 左边减掉margin,右边减掉edgeMargin
        break;
    case QwtScaleDraw::TopScale:
        cr.adjust(0, m_data->edgeMargin, 0, -(m_data->margin));  // 下边减掉margin,上边减掉edgeMargin
        break;
    case QwtScaleDraw::BottomScale:
        cr.adjust(0, m_data->margin, 0, -(m_data->edgeMargin));  // 上边减掉margin,下边减掉edgeMargin
        break;
    }

    return cr;
}

/*!
   Change Event handler
   \param event Change event

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
   \param event Resize event
 */
void QwtScaleWidget::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    layoutScale(false);
}

/*!
   Recalculate the scale's geometry and layout based on
   the current geometry and fonts.

   \param update_geometry Notify the layout system and call update
                         to redraw the scale
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
    // 刻度偏移和edgeMargin无关
    if (m_data->scaleDraw->orientation() == Qt::Vertical) {
        y      = r.top() + bd0;
        length = r.height() - (bd0 + bd1);

        if (m_data->scaleDraw->alignment() == QwtScaleDraw::LeftScale) {  // 左对齐的刻度，坐标轴一般是右坐标轴
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
    // titleoffset和edgeMargin无关
    m_data->titleOffset = m_data->margin + m_data->spacing + colorBarWidth + extent;

    if (update_geometry) {
        updateGeometry();

#if 1
        /*
            for some reason updateGeometry does not send a LayoutRequest event
            when the parent is not visible and has no layout
         */

        if (QWidget* w = parentWidget()) {
            if (!w->isVisible() && w->layout() == NULL) {
                if (w->testAttribute(Qt::WA_WState_Polished))
                    QApplication::postEvent(w, new QEvent(QEvent::LayoutRequest));
            }
        }
#endif

        update();
    }
}

/**
 * @brief 获取此轴窗口对应的axisID
 * @note 注意，此函基于QwtScaleDraw的对其方式来转换，如果QwtScaleDraw的对其方式没设置，那么会返回QwtAxis::AxisPositions
 * @return
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
 * @brief 是否是x坐标轴
 * @return
 */
bool QwtScaleWidget::isXAxis() const
{
    return QwtAxis::isXAxis(axisID());
}

/**
 * @brief 是否是y坐标轴
 * @return
 */
bool QwtScaleWidget::isYAxis() const
{
    return QwtAxis::isYAxis(axisID());
}

/**
 * @brief 设置内置的动作
 * @param actions 内置动作
 * @sa BuildinActions
 */
void QwtScaleWidget::setBuildinActions(BuiltinActionsFlags acts)
{
    if (m_data->builtinActions != acts) {
        m_data->builtinActions = acts;
    }
}

/**
 * @brief 内置的动作
 * @return
 */
QwtScaleWidget::BuiltinActionsFlags QwtScaleWidget::buildinActions() const
{
    return m_data->builtinActions;
}

/**
 * @brief 检测内置动作是否激活
 * @param ba
 * @return
 */
bool QwtScaleWidget::testBuildinActions(QwtScaleWidget::BuiltinActions ba) const
{
    return m_data->builtinActions.testFlag(ba);
}

/**
 * @brief 设置当前轴被选中
 *
 * 此函数会触发信号@ref selectionChanged
 *
 * 如果重复设置同一个状态不会重复触发信号
 *
 * @param selected
 *
 */
void QwtScaleWidget::setSelected(bool selected)
{
    if (m_data->isSelected != selected) {
        m_data->isSelected = selected;
        update();
        Q_EMIT selectionChanged(selected);
    }
}

/**
 * @brief 当前轴是否被选中
 * @param selected
 */
bool QwtScaleWidget::isSelected() const
{
    return m_data->isSelected;
}

/**
 * @brief 设置选中的颜色
 * @param color
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
 * @brief 选中的颜色
 * @return
 */
QColor QwtScaleWidget::selectionColor() const
{
    return m_data->selectionColor;
}

/**
 * @brief 设置缩放因子(默认1.2)
 * @param factor
 */
void QwtScaleWidget::setZoomFactor(double factor)
{
    m_data->zoomFactor = qMax(0.1, qMin(10.0, factor));
}

/**
 * @brief 缩放因子
 * @return
 */
double QwtScaleWidget::zoomFactor() const
{
    return m_data->zoomFactor;
}

/*!
   Draw the color bar of the scale widget

   \param painter Painter
   \param rect Bounding rectangle for the color bar

   \sa setColorBarEnabled()
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

/*!
   Rotate and paint a title according to its position into a given rectangle.

   \param painter Painter
   \param align Alignment
   \param rect Bounding rectangle
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

    QwtText title = m_data->title;
    title.setRenderFlags(flags);
    title.draw(painter, QRectF(0.0, 0.0, r.width(), r.height()));

    painter->restore();
}

/*!
   \brief Notify a change of the scale

   This virtual function can be overloaded by derived
   classes. The default implementation updates the geometry
   and repaints the widget.
 */

void QwtScaleWidget::scaleChange()
{
    layoutScale();
}

/*!
   \return a size hint
 */
QSize QwtScaleWidget::sizeHint() const
{
    return minimumSizeHint();
}

/*!
   \return a minimum size hint
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

/*!
   \brief Find the height of the title for a given width.
   \param width Width
   \return height Height
 */

int QwtScaleWidget::titleHeightForWidth(int width) const
{
    return qwtCeil(m_data->title.heightForWidth(width, font()));
}

/*!
   \brief Find the minimum dimension for a given length.
         dim is the height, length the width seen in
         direction of the title.
   \param length width for horizontal, height for vertical scales
   \param scaleFont Font of the scale
   \return height for horizontal, width for vertical scales
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

/*!
   Specify distances of the scale's endpoints from the
   widget's borders. The actual borders will never be less
   than minimum border distance.
   \param dist1 Left or top Distance
   \param dist2 Right or bottom distance
   \sa borderDist()
 */
void QwtScaleWidget::setBorderDist(int dist1, int dist2)
{
    if (dist1 != m_data->borderDist[ 0 ] || dist2 != m_data->borderDist[ 1 ]) {
        m_data->borderDist[ 0 ] = dist1;
        m_data->borderDist[ 1 ] = dist2;
        layoutScale();
    }
}

/*!
    \return start border distance
    \sa setBorderDist()
 */
int QwtScaleWidget::startBorderDist() const
{
    return m_data->borderDist[ 0 ];
}

/*!
    \return end border distance
    \sa setBorderDist()
 */
int QwtScaleWidget::endBorderDist() const
{
    return m_data->borderDist[ 1 ];
}

/**
 * @brief Calculate a hint for the border distances./计算边框距离的“建议值”
 *
 * This member function calculates the distance
 * of the scale's endpoints from the widget borders which
 * is required for the mark labels to fit into the widget.
 * The maximum of this distance an the minimum border distance
 * is returned.
 *
 * 本函数根据刻度标记文字的大小，计算刻度两端与控件边框之间所需的距离，以确保文字完整显示。
 * 最终返回该距离与最小边框距离中的较大值。
 *
 * @param start Return parameter for the border width at
 *             the beginning of the scale/刻度起始端与边框的宽度
 * @param end Return parameter for the border width at the
 *           end of the scale/刻度末端与边框的宽度
 *
 * @warning
 * <ul> <li>The minimum border distance depends on the font./最小边框距离与当前字体有关。</ul>
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
 * Set a minimum value for the distances of the scale's endpoints from
 * the widget borders. This is useful to avoid that the scales
 * are "jumping", when the tick labels or their positions change
 * often.
 *
 * 设置刻度两端与控件边框的最小距离。当刻度标签或其位置频繁变化时，可避免刻度出现“跳动”现象。
 *
 * @param start Minimum for the start border/起始端最小边距
 * @param end Minimum for the end border/末端最小边距
 * @sa getMinBorderDist(), getBorderDistHint(), startMinBorderDist(), endMinBorderDist()
 */
void QwtScaleWidget::setMinBorderDist(int start, int end)
{
    m_data->minBorderDist[ 0 ] = start;
    m_data->minBorderDist[ 1 ] = end;
}

/**
 * @brief minimum value for the distances of the scale's endpoints from
 *   the widget borders.(Left or top Distance)
 *
 * @sa getMinBorderDist(), getBorderDistHint()
 */
int QwtScaleWidget::startMinBorderDist() const
{
    return m_data->minBorderDist[ 0 ];
}

/**
 * @brief  minimum value for the distances of the scale's endpoints from
 *   the widget borders.(Right or bottom distance)
 *
 * @return
 */
int QwtScaleWidget::endMinBorderDist() const
{
    return m_data->minBorderDist[ 1 ];
}

/*!
   Get the minimum value for the distances of the scale's endpoints from
   the widget borders.

   \param start Return parameter for the border width at
               the beginning of the scale
   \param end Return parameter for the border width at the
             end of the scale

   \sa setMinBorderDist(), getBorderDistHint()
 */
void QwtScaleWidget::getMinBorderDist(int& start, int& end) const
{
    start = m_data->minBorderDist[ 0 ];
    end   = m_data->minBorderDist[ 1 ];
}

/*!
   \brief Assign a scale division

   The scale division determines where to set the tick marks.

   \param scaleDiv Scale Division
   \sa For more information about scale divisions, see QwtScaleDiv.
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

/*!
   Set the transformation

   \param transformation Transformation
   \sa QwtAbstractScaleDraw::scaleDraw(), QwtScaleMap
 */
void QwtScaleWidget::setTransformation(QwtTransform* transformation)
{
    m_data->scaleDraw->setTransformation(transformation);
    layoutScale();
}

/*!
   En/disable a color bar associated to the scale
   \sa isColorBarEnabled(), setColorBarWidth()
 */
void QwtScaleWidget::setColorBarEnabled(bool on)
{
    if (on != m_data->colorBar.isEnabled) {
        m_data->colorBar.isEnabled = on;
        layoutScale();
    }
}

/*!
   \return true, when the color bar is enabled
   \sa setColorBarEnabled(), setColorBarWidth()
 */
bool QwtScaleWidget::isColorBarEnabled() const
{
    return m_data->colorBar.isEnabled;
}

/*!
   Set the width of the color bar

   \param width Width
   \sa colorBarWidth(), setColorBarEnabled()
 */
void QwtScaleWidget::setColorBarWidth(int width)
{
    if (width != m_data->colorBar.width) {
        m_data->colorBar.width = width;
        if (isColorBarEnabled())
            layoutScale();
    }
}

/*!
   \return Width of the color bar
   \sa setColorBarEnabled(), setColorBarEnabled()
 */
int QwtScaleWidget::colorBarWidth() const
{
    return m_data->colorBar.width;
}

/*!
   \return Value interval for the color bar
   \sa setColorMap(), colorMap()
 */
QwtInterval QwtScaleWidget::colorBarInterval() const
{
    return m_data->colorBar.interval;
}

/*!
   Set the color map and value interval, that are used for displaying
   the color bar.

   \param interval Value interval
   \param colorMap Color map

   \sa colorMap(), colorBarInterval()
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

/*!
   \return Color map
   \sa setColorMap(), colorBarInterval()
 */
const QwtColorMap* QwtScaleWidget::colorMap() const
{
    return m_data->colorBar.colorMap.get();
}
