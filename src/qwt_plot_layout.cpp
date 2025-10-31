/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_plot_layout.h"
#include "qwt_text.h"
#include "qwt_text_label.h"
#include "qwt_scale_widget.h"
#include "qwt_abstract_legend.h"
#include "qwt_math.h"
#include "qwt_plot_layout_engine.h"
#include <qmargins.h>

namespace
{
class LayoutHintData
{
public:
    LayoutHintData(const QwtPlot* plot);

    int alignedSize(const QwtAxisId) const;

    inline int yAxesWidth() const
    {
        using namespace QwtAxis;
        return axesWidth(YLeft) + axesWidth(YRight);
    }

    inline int yAxesHeight() const
    {
        using namespace QwtAxis;
        return qMax(axesHeight(YLeft), axesHeight(YRight));
    }

    inline int xAxesHeight() const
    {
        using namespace QwtAxis;
        return axesHeight(XTop) + axesHeight(XBottom);
    }

    inline int xAxesWidth() const
    {
        using namespace QwtAxis;
        return qMax(axesWidth(XTop), axesWidth(XBottom));
    }

private:
    struct ScaleData
    {
        ScaleData()
        {
            w = h = minLeft = minRight = tickOffset = 0;
        }

        int w;
        int h;
        int minLeft;
        int minRight;
        int tickOffset;
    };

    const ScaleData& axisData(QwtAxisId axisId) const
    {
        return m_scaleData[ axisId ];
    }

    ScaleData& axisData(QwtAxisId axisId)
    {
        return m_scaleData[ axisId ];
    }

    inline int axesWidth(int axisPos) const
    {
        return m_scaleData[ axisPos ].w;
    }

    inline int axesHeight(int axisPos) const
    {
        return m_scaleData[ axisPos ].h;
    }

    int m_canvasBorder[ QwtAxis::AxisPositions ];
    ScaleData m_scaleData[ QwtAxis::AxisPositions ];
};

LayoutHintData::LayoutHintData(const QwtPlot* plot)
{
    using namespace QwtAxis;

    const QMargins m = plot->canvas()->contentsMargins();

    int contentsMargins[ 4 ];
    contentsMargins[ YLeft ]   = m.left();
    contentsMargins[ XTop ]    = m.top();
    contentsMargins[ YRight ]  = m.right();
    contentsMargins[ XBottom ] = m.bottom();

    for (int axisPos = 0; axisPos < AxisPositions; axisPos++) {
        m_canvasBorder[ axisPos ] = contentsMargins[ axisPos ] + plot->plotLayout()->canvasMargin(axisPos) + 1;
        {
            const QwtAxisId axisId(axisPos);

            if (plot->isAxisVisible(axisId)) {
                const QwtScaleWidget* scl = plot->axisWidget(axisId);

                const QSize hint = scl->minimumSizeHint();

                ScaleData& sd = axisData(axisId);
                sd.w          = hint.width();
                sd.h          = hint.height();
                scl->getBorderDistHint(sd.minLeft, sd.minRight);

                {
                    sd.tickOffset = scl->margin();
                    if (scl->scaleDraw()->hasComponent(QwtAbstractScaleDraw::Ticks))
                        sd.tickOffset += qwtCeil(scl->scaleDraw()->maxTickLength());
                }
            }
        }
    }

    for (int axis = 0; axis < AxisPositions; axis++) {
        const int sz = alignedSize(axis);

        ScaleData& sd = axisData(axis);
        if (isXAxis(axis))
            sd.w = sz;
        else
            sd.h = sz;
    }
}

int LayoutHintData::alignedSize(const QwtAxisId axisId) const
{
    using namespace QwtAxis;

    const ScaleData& sd = axisData(axisId);

    if (sd.w && isXAxis(axisId)) {
        int w = sd.w;

        if (const int leftW = axesWidth(YLeft)) {
            const int shiftLeft = sd.minLeft - m_canvasBorder[ YLeft ];
            if (shiftLeft > 0)
                w -= qMin(shiftLeft, leftW);
        }

        if (const int rightW = axesWidth(YRight)) {
            const int shiftRight = sd.minRight - m_canvasBorder[ YRight ];
            if (shiftRight > 0)
                w -= qMin(shiftRight, rightW);
        }

        return w;
    }

    if (sd.h && isYAxis(axisId)) {
        int h = sd.h;

        if (axesHeight(XBottom)) {
            const int shiftBottom = sd.minLeft - m_canvasBorder[ XBottom ];
            if (shiftBottom > 0)
                h -= qMin(shiftBottom, axisData(XBottom).tickOffset);
        }

        if (axesHeight(XTop)) {
            const int shiftTop = sd.minRight - m_canvasBorder[ XTop ];
            if (shiftTop > 0)
                h -= qMin(shiftTop, axisData(XTop).tickOffset);
        }

        return h;
    }

    return 0;
}
}

class QwtPlotLayout::PrivateData
{
public:
    QRectF titleRect;
    QRectF footerRect;
    QRectF legendRect;
    QRectF scaleRects[ QwtAxis::AxisPositions ];
    QRectF canvasRect;

    QwtPlotLayoutEngine engine;
};

/*!
   \brief Constructor
 */

QwtPlotLayout::QwtPlotLayout()
{
    m_data = new PrivateData;

    setLegendPosition(QwtPlot::BottomLegend);
    setCanvasMargin(-1);
    setAlignCanvasToScales(false);

    invalidate();
}

//! Destructor
QwtPlotLayout::~QwtPlotLayout()
{
    delete m_data;
}

/*!
   Change a margin of the canvas. The margin is the space
   above/below the scale ticks. A negative margin will
   be set to -1, excluding the borders of the scales.

   设置画布的边距（margin）。这个边距是指 坐标轴刻度线与画布边缘之间的空间

   正数：在刻度线外增加空白区域
   负数：会被设置为-1，表示排除坐标轴边框的影响
   0：紧贴刻度线

   \param margin New margin
   \param axisPos One of QwtAxis::Position. Specifies where the position of the margin.
              -1 means margin at all borders.
   \sa canvasMargin()

   \warning The margin will have no effect when alignCanvasToScale() is true
 */

void QwtPlotLayout::setCanvasMargin(int margin, int axisPos)
{
    if (margin < -1)
        margin = -1;

    QwtPlotLayoutEngine& engine = m_data->engine;

    if (axisPos == -1) {
        for (axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++)
            engine.setCanvasMargin(axisPos, margin);
    } else if (QwtAxis::isValid(axisPos)) {
        engine.setCanvasMargin(axisPos, margin);
    }
}

/*!
    \param axisPos Axis position
    \return Margin around the scale tick borders
    \sa setCanvasMargin()
 */
int QwtPlotLayout::canvasMargin(int axisPos) const
{
    if (!QwtAxis::isValid(axisPos))
        return 0;

    return m_data->engine.canvasMargin(axisPos);
}

/*!
   \brief Set the align-canvas-to-axis-scales flag for all axes

   \param on True/False
   \sa setAlignCanvasToScale(), alignCanvasToScale()
 */
void QwtPlotLayout::setAlignCanvasToScales(bool on)
{
    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++)
        m_data->engine.setAlignCanvas(axisPos, on);
}

/*!
 *  Change the align-canvas-to-axis-scales setting. The canvas may:
 *
 *  - extend beyond the axis scale ends to maximize its size,
 *  - align with the axis scale ends to control its size.
 *
 *  The axisId parameter is somehow confusing as it identifies a border
 *  of the plot and not the axes, that are aligned. F.e when QwtAxis::YLeft
 *  is set, the left end of the the x-axes ( QwtAxis::XTop, QwtAxis::XBottom )
 *  is aligned.
 *
 *  设置画布是否与坐标轴刻度对齐。画布有两种表现方式：
 *
 *  - 延伸：画布可超出轴刻度端点，以最大化绘图区域；
 *  - 对齐：画布严格对齐轴刻度端点，以便精确控制大小。
 *
 *  【注意】参数 axisId 实际指代的是“绘图边框”，而非被对齐的轴本身。例如传入 QwtAxis::YLeft 时，真正被对齐的是两条 X 轴
 *  （QwtAxis::XTop 与 QwtAxis::XBottom）的左端。
 *
 *  @param axisId Axis index
 *  @param on New align-canvas-to-axis-scales setting
 *
 *  @sa setCanvasMargin(), alignCanvasToScale(), setAlignCanvasToScales()
 *  @warning In case of on == true canvasMargin() will have no effect
 */
void QwtPlotLayout::setAlignCanvasToScale(int axisPos, bool on)
{
    if (QwtAxis::isValid(axisPos))
        m_data->engine.setAlignCanvas(axisPos, on);
}

/*!
   Return the align-canvas-to-axis-scales setting. The canvas may:
   - extend beyond the axis scale ends to maximize its size
   - align with the axis scale ends to control its size.

   \param axisPos Axis position
   \return align-canvas-to-axis-scales setting
   \sa setAlignCanvasToScale(), setAlignCanvasToScale(), setCanvasMargin()
 */
bool QwtPlotLayout::alignCanvasToScale(int axisPos) const
{
    if (!QwtAxis::isValid(axisPos))
        return false;

    return m_data->engine.alignCanvas(axisPos);
}

/*!
   Change the spacing of the plot. The spacing is the distance
   between the plot components.

   \param spacing New spacing
   \sa setCanvasMargin(), spacing()
 */
void QwtPlotLayout::setSpacing(int spacing)
{
    m_data->engine.setSpacing(qMax(0, spacing));
}

/*!
   \return Spacing
   \sa margin(), setSpacing()
 */
int QwtPlotLayout::spacing() const
{
    return m_data->engine.spacing();
}

/*!
   \brief Specify the position of the legend
   \param pos The legend's position.
   \param ratio Ratio between legend and the bounding rectangle
               of title, footer, canvas and axes. The legend will be shrunk
               if it would need more space than the given ratio.
               The ratio is limited to ]0.0 .. 1.0]. In case of <= 0.0
               it will be reset to the default ratio.
               The default vertical/horizontal ratio is 0.33/0.5.

   \sa QwtPlot::setLegendPosition()
 */

void QwtPlotLayout::setLegendPosition(QwtPlot::LegendPosition pos, double ratio)
{
    if (ratio > 1.0)
        ratio = 1.0;

    QwtPlotLayoutEngine& engine = m_data->engine;

    switch (pos) {
    case QwtPlot::TopLegend:
    case QwtPlot::BottomLegend: {
        if (ratio <= 0.0)
            ratio = 0.33;

        engine.setLegendRatio(ratio);
        engine.setLegendPos(pos);

        break;
    }
    case QwtPlot::LeftLegend:
    case QwtPlot::RightLegend: {
        if (ratio <= 0.0)
            ratio = 0.5;

        engine.setLegendRatio(ratio);
        engine.setLegendPos(pos);

        break;
    }
    default:
        break;
    }
}

/*!
   \brief Specify the position of the legend
   \param pos The legend's position. Valid values are
      \c QwtPlot::LeftLegend, \c QwtPlot::RightLegend,
      \c QwtPlot::TopLegend, \c QwtPlot::BottomLegend.

   \sa QwtPlot::setLegendPosition()
 */
void QwtPlotLayout::setLegendPosition(QwtPlot::LegendPosition pos)
{
    setLegendPosition(pos, 0.0);
}

/*!
   \return Position of the legend
   \sa setLegendPosition(), QwtPlot::setLegendPosition(),
      QwtPlot::legendPosition()
 */
QwtPlot::LegendPosition QwtPlotLayout::legendPosition() const
{
    return m_data->engine.legendPos();
}

/*!
   Specify the relative size of the legend in the plot
   \param ratio Ratio between legend and the bounding rectangle
               of title, footer, canvas and axes. The legend will be shrunk
               if it would need more space than the given ratio.
               The ratio is limited to ]0.0 .. 1.0]. In case of <= 0.0
               it will be reset to the default ratio.
               The default vertical/horizontal ratio is 0.33/0.5.
 */
void QwtPlotLayout::setLegendRatio(double ratio)
{
    setLegendPosition(legendPosition(), ratio);
}

/*!
   \return The relative size of the legend in the plot.
   \sa setLegendPosition()
 */
double QwtPlotLayout::legendRatio() const
{
    return m_data->engine.legendRatio();
}

/*!
   \brief Set the geometry for the title

   This method is intended to be used from derived layouts
   overloading activate()

   \sa titleRect(), activate()
 */
void QwtPlotLayout::setTitleRect(const QRectF& rect)
{
    m_data->titleRect = rect;
}

/*!
   \return Geometry for the title
   \sa activate(), invalidate()
 */
QRectF QwtPlotLayout::titleRect() const
{
    return m_data->titleRect;
}

/*!
   \brief Set the geometry for the footer

   This method is intended to be used from derived layouts
   overloading activate()

   \sa footerRect(), activate()
 */
void QwtPlotLayout::setFooterRect(const QRectF& rect)
{
    m_data->footerRect = rect;
}

/*!
   \return Geometry for the footer
   \sa activate(), invalidate()
 */
QRectF QwtPlotLayout::footerRect() const
{
    return m_data->footerRect;
}

/*!
   \brief Set the geometry for the legend

   This method is intended to be used from derived layouts
   overloading activate()

   \param rect Rectangle for the legend

   \sa legendRect(), activate()
 */
void QwtPlotLayout::setLegendRect(const QRectF& rect)
{
    m_data->legendRect = rect;
}

/*!
   \return Geometry for the legend
   \sa activate(), invalidate()
 */
QRectF QwtPlotLayout::legendRect() const
{
    return m_data->legendRect;
}

/*!
   \brief Set the geometry for an axis

   This method is intended to be used from derived layouts
   overloading activate()

   \param axisId Axis
   \param rect Rectangle for the scale

   \sa scaleRect(), activate()
 */
void QwtPlotLayout::setScaleRect(QwtAxisId axisId, const QRectF& rect)
{
    if (QwtAxis::isValid(axisId))
        m_data->scaleRects[ axisId ] = rect;
}

/*!
   \param axisId Axis
   \return Geometry for the scale
   \sa activate(), invalidate()
 */
QRectF QwtPlotLayout::scaleRect(QwtAxisId axisId) const
{
    if (QwtAxis::isValid(axisId))
        return m_data->scaleRects[ axisId ];

    return QRectF();
}

/*!
   \brief Set the geometry for the canvas

   This method is intended to be used from derived layouts
   overloading activate()

   \sa canvasRect(), activate()
 */
void QwtPlotLayout::setCanvasRect(const QRectF& rect)
{
    m_data->canvasRect = rect;
}

QwtPlotLayoutEngine* QwtPlotLayout::layoutEngine()
{
    return &(m_data->engine);
}

/*!
   \return Geometry for the canvas
   \sa activate(), invalidate()
 */
QRectF QwtPlotLayout::canvasRect() const
{
    return m_data->canvasRect;
}

/*!
   Invalidate the geometry of all components.
   \sa activate()
 */
void QwtPlotLayout::invalidate()
{
    m_data->titleRect = m_data->footerRect = m_data->legendRect = m_data->canvasRect = QRectF();

    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++)
        m_data->scaleRects[ axisPos ] = QRect();
}

/*!
   \return Minimum size hint
   \param plot Plot widget

   \sa QwtPlot::minimumSizeHint()
 */
QSize QwtPlotLayout::minimumSizeHint(const QwtPlot* plot) const
{
    LayoutHintData hintData(plot);

    const int xAxesWidth  = hintData.xAxesWidth();
    const int yAxesHeight = hintData.yAxesHeight();

    const QWidget* canvas = plot->canvas();

    const QMargins m          = canvas->contentsMargins();
    const QSize minCanvasSize = canvas->minimumSize();

    int w  = hintData.yAxesWidth();
    int cw = xAxesWidth + m.left() + 1 + m.right() + 1;
    w += qMax(cw, minCanvasSize.width());

    int h  = hintData.xAxesHeight();
    int ch = yAxesHeight + m.top() + 1 + m.bottom() + 1;
    h += qMax(ch, minCanvasSize.height());

    const QwtTextLabel* labels[ 2 ];
    labels[ 0 ] = plot->titleLabel();
    labels[ 1 ] = plot->footerLabel();

    for (int i = 0; i < 2; i++) {
        const QwtTextLabel* label = labels[ i ];
        if (label && !label->text().isEmpty()) {
            // we center on the plot canvas.
            const bool centerOnCanvas = !(plot->isAxisVisible(QwtAxis::YLeft) && plot->isAxisVisible(QwtAxis::YRight));

            int labelW = w;
            if (centerOnCanvas) {
                labelW -= hintData.yAxesWidth();
            }

            int labelH = label->heightForWidth(labelW);
            if (labelH > labelW)  // Compensate for a long title
            {
                w = labelW = labelH;
                if (centerOnCanvas)
                    w += hintData.yAxesWidth();

                labelH = label->heightForWidth(labelW);
            }
            h += labelH + spacing();
        }
    }

    // Compute the legend contribution

    const QwtAbstractLegend* legend = plot->legend();
    if (legend && !legend->isEmpty()) {
        const QwtPlotLayoutEngine& engine = m_data->engine;

        if (engine.legendPos() == QwtPlot::LeftLegend || engine.legendPos() == QwtPlot::RightLegend) {
            int legendW = legend->sizeHint().width();
            int legendH = legend->heightForWidth(legendW);

            if (legend->frameWidth() > 0)
                w += spacing();

            if (legendH > h)
                legendW += legend->scrollExtent(Qt::Horizontal);

            if (engine.legendRatio() < 1.0)
                legendW = qMin(legendW, int(w / (1.0 - engine.legendRatio())));

            w += legendW + spacing();
        } else {
            int legendW = qMin(legend->sizeHint().width(), w);
            int legendH = legend->heightForWidth(legendW);

            if (legend->frameWidth() > 0)
                h += spacing();

            if (engine.legendRatio() < 1.0)
                legendH = qMin(legendH, int(h / (1.0 - engine.legendRatio())));

            h += legendH + spacing();
        }
    }

    return QSize(w, h);
}

/**
 * @brief Recalculate the geometry of all components./根据给定的外框矩形，重新计算并记录 QwtPlot
 * 内所有子部件（标题、页脚、图例、4 条轴、画布）的几何位置。
 * @param plot Plot to be layout/待布局的 plot 对象
 * @param plotRect Rectangle where to place the components/外部可用矩形（逻辑坐标，单位是像素）
 * @param options Layout options/布局选项，例如是否忽略图例、是否忽略某条轴等
 *
 * 结果全部写入 m_data->xxxRect，外部可通过 titleRect()/footerRect()/legendRect()/canvasRect()/scaleRect() 直接读取。
 *
 * @sa invalidate(), titleRect(), footerRect(), legendRect(), scaleRect(), canvasRect()
 */
void QwtPlotLayout::activate(const QwtPlot* plot, const QRectF& plotRect, Options options)
{
    invalidate();
    doActivate(plot, plotRect, options);
}

/**
 * @brief QwtPlotLayout::activate的具体实现
 * @param plotRect
 * @param options
 */
void QwtPlotLayout::doActivate(const QwtPlot* plot, const QRectF& plotRect, Options options)
{
    QRectF rect(plotRect);  // undistributed rest of the plot rect

    // We extract all layout relevant parameters from the widgets,
    // and save them to m_data->layoutData.
    // 从各个控件中提取所有与布局相关的参数，
    // 并将其保存到 m_data->layoutData 中。
    QwtPlotLayoutEngine::LayoutData layoutData(plot);

    QSize legendHint;
    // 先处理图例 —— 如果存在、非空、且选项允许
    if (!(options & IgnoreLegend) && plot->legend() && !plot->legend()->isEmpty()) {

        // 让图例根据自身内容计算一个理想大小
        legendHint = layoutData.legendData.legendHint(plot->legend(), rect);

        m_data->legendRect = m_data->engine.layoutLegend(options, layoutData.legendData, rect, legendHint);

        // subtract m_data->legendRect from rect
        // 从剩余矩形里“抠掉”图例区域
        const QRegion region(rect.toRect());
        rect = region.subtracted(m_data->legendRect.toRect()).boundingRect();

        switch (m_data->engine.legendPos()) {
        case QwtPlot::LeftLegend: {
            rect.setLeft(rect.left() + spacing());
            break;
        }
        case QwtPlot::RightLegend: {
            rect.setRight(rect.right() - spacing());
            break;
        }
        case QwtPlot::TopLegend: {
            rect.setTop(rect.top() + spacing());
            break;
        }
        case QwtPlot::BottomLegend: {
            rect.setBottom(rect.bottom() - spacing());
            break;
        }
        }
    }

    /*
     +---+-----------+---+
     |       Title       |
     +---+-----------+---+
     |   |   Axis    |   |
     +---+-----------+---+
     | A |           | A |
     | x |  Canvas   | x |
     | i |           | i |
     | s |           | s |
     +---+-----------+---+
     |   |   Axis    |   |
     +---+-----------+---+
     |      Footer       |
     +---+-----------+---+
     */

    // title, footer and axes include text labels. The height of each
    // label depends on its line breaks, that depend on the width
    // for the label. A line break in a horizontal text will reduce
    // the available width for vertical texts and vice versa.
    // layoutDimensions finds the height/width for title, footer and axes
    // including all line breaks.
    // 标题、页脚和坐标轴都包含文本标签。
    // 每个标签的高度取决于它的换行，而换行又受标签可用宽度的限制。
    // 水平文本中的换行会减少垂直文本的可用宽度，反之亦然。
    // layoutDimensions 用于计算标题、页脚和坐标轴的最终高度/宽度，已包含所有换行带来的影响。
    using namespace QwtAxis;

    const QwtPlotLayoutEngine::Dimensions dimensions = m_data->engine.layoutDimensions(options, layoutData, rect);

    if (dimensions.dimTitle > 0) {
        QRectF& labelRect = m_data->titleRect;

        labelRect.setRect(rect.left(), rect.top(), rect.width(), dimensions.dimTitle);

        rect.setTop(labelRect.bottom() + spacing());

        if (!layoutData.hasSymmetricYAxes()) {
            // if only one of the y axes is missing we align
            // the title centered to the canvas

            labelRect = dimensions.centered(rect, labelRect);
        }
    }

    if (dimensions.dimFooter > 0) {
        QRectF& labelRect = m_data->footerRect;

        labelRect.setRect(rect.left(), rect.bottom() - dimensions.dimFooter, rect.width(), dimensions.dimFooter);

        rect.setBottom(labelRect.top() - spacing());

        if (!layoutData.hasSymmetricYAxes()) {
            // if only one of the y axes is missing we align
            // the footer centered to the canvas

            labelRect = dimensions.centered(rect, labelRect);
        }
    }

    m_data->canvasRect = dimensions.innerRect(rect);

    for (int axisPos = 0; axisPos < AxisPositions; axisPos++) {
        // set the rects for the axes

        const int pos = 0;
        {
            const QwtAxisId axisId(axisPos);

            if (dimensions.dimAxis(axisId)) {
                const int dim = dimensions.dimAxis(axisId);

                const QRectF& canvasRect = m_data->canvasRect;

                QRectF& scaleRect = m_data->scaleRects[ axisId ];
                scaleRect         = canvasRect;

                switch (axisPos) {
                case YLeft: {
                    scaleRect.setX(canvasRect.left() - pos - dim);
                    scaleRect.setWidth(dim);
                    break;
                }
                case YRight: {
                    scaleRect.setX(canvasRect.right() + pos);
                    scaleRect.setWidth(dim);
                    break;
                }
                case XBottom: {
                    scaleRect.setY(canvasRect.bottom() + pos);
                    scaleRect.setHeight(dim);
                    break;
                }
                case XTop: {
                    scaleRect.setY(canvasRect.top() - pos - dim);
                    scaleRect.setHeight(dim);
                    break;
                }
                }
                scaleRect = scaleRect.normalized();
            }
        }
    }

    // +---+-----------+---+
    // |  <-   Axis   ->   |
    // +-^-+-----------+-^-+
    // | | |           | | |
    // |   |           |   |
    // | A |           | A |
    // | x |  Canvas   | x |
    // | i |           | i |
    // | s |           | s |
    // |   |           |   |
    // | | |           | | |
    // +-V-+-----------+-V-+
    // |   <-  Axis   ->   |
    // +---+-----------+---+

    // The ticks of the axes - not the labels above - should
    // be aligned to the canvas. So we try to use the empty
    // corners to extend the axes, so that the label texts
    // left/right of the min/max ticks are moved into them.

    m_data->engine.alignScales(options, layoutData, m_data->canvasRect, m_data->scaleRects);

    if (!m_data->legendRect.isEmpty()) {
        // We prefer to align the legend to the canvas - not to
        // the complete plot - if possible.

        m_data->legendRect = m_data->engine.alignLegend(legendHint, m_data->canvasRect, m_data->legendRect);
    }
}
