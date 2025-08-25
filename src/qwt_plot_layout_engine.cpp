#include "qwt_plot_layout_engine.h"
#include "qwt_abstract_legend.h"
#include "qwt_math.h"
#include "qwt_text.h"
#include "qwt_text_label.h"
#include "qwt_scale_widget.h"
#include "qwt_plot.h"
#include "qwt_plot_layout.h"
//----------------------------------------------------
// QwtPlotLayoutEngine::Dimensions
//----------------------------------------------------
QwtPlotLayoutEngine::Dimensions::Dimensions()
{
	dimTitle = dimFooter = 0;
	for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++)
		m_dimAxes[ axisPos ] = 0;
}

/**
 * @brief 获取指定轴的占用尺寸 / Get the stored dimension for a given axis
 * @param axisId  轴 ID（QwtAxis::XTop …）/ axis identifier
 * @return 该轴所占像素宽度或高度 / the reserved pixels for this axis
 */
int QwtPlotLayoutEngine::Dimensions::dimAxis(QwtAxisId axisId) const
{
    return m_dimAxes[ axisId ];
}

/**
 * @brief 设置指定轴的占用尺寸 / Set the stored dimension for a given axis
 * @param axisId  轴 ID / axis identifier
 * @param dim     新的像素尺寸 / new dimension in pixels
 */
void QwtPlotLayoutEngine::Dimensions::setDimAxis(QwtAxisId axisId, int dim)
{
    m_dimAxes[ axisId ] = dim;
}

/**
 * @brief 按轴位置索引读取尺寸 / Read dimension by axis position index
 * @param axisPos  轴位置枚举值 / axis position enum value
 * @return 对应轴的像素尺寸 / pixel size of the axis
 */
int QwtPlotLayoutEngine::Dimensions::dimAxes(int axisPos) const
{
    return m_dimAxes[ axisPos ];
}

/**
 * @brief 左右 Y 轴总宽度之和 / Sum width of left and right Y axes
 * @return 总像素宽度 / combined width in pixels
 */
int QwtPlotLayoutEngine::Dimensions::dimYAxes() const
{
    return dimAxes(QwtAxis::YLeft) + dimAxes(QwtAxis::YRight);
}

/**
 * @brief 上下 X 轴总高度之和 / Sum height of top and bottom X axes
 * @return 总像素高度 / combined height in pixels
 */
int QwtPlotLayoutEngine::Dimensions::dimXAxes() const
{
    return dimAxes(QwtAxis::XTop) + dimAxes(QwtAxis::XBottom);
}

/**
 * @brief Center a label rectangle within available space / 在可用空间内居中标签矩形
 *
 * This method adjusts the label rectangle to be centered horizontally
 * within the available space after accounting for Y axis dimensions.
 *
 * 此方法调整标签矩形，在考虑Y轴尺寸后在可用空间内水平居中。
 *
 * @param rect The available rectangle / 可用矩形区域
 * @param labelRect The label rectangle to center / 要居中的标签矩形
 * @return Centered label rectangle / 居中后的标签矩形
 */
QRectF QwtPlotLayoutEngine::Dimensions::centered(const QRectF& rect, const QRectF& labelRect) const
{
	QRectF r = labelRect;
	r.setX(rect.left() + dimAxes(QwtAxis::YLeft));
	r.setWidth(rect.width() - dimYAxes());

	return r;
}

/**
 * @brief Calculate inner rectangle after accounting for axis dimensions / 计算考虑轴尺寸后的内部矩形
 *
 * This method calculates the available space for the canvas after
 * reserving space for all axes.
 *
 * 此方法计算为所有轴保留空间后，画布的可用空间。
 *
 * @param rect The outer rectangle / 外部矩形区域
 * @return Inner rectangle for canvas / 画布的内部矩形区域
 */
QRectF QwtPlotLayoutEngine::Dimensions::innerRect(const QRectF& rect) const
{
	QRectF r(rect.x() + dimAxes(QwtAxis::YLeft),
             rect.y() + dimAxes(QwtAxis::XTop),
             rect.width() - dimYAxes(),
             rect.height() - dimXAxes());

	if (r.width() < 0) {
		r.setX(rect.center().x());
		r.setWidth(0);
	}
	if (r.height() < 0) {
		r.setY(rect.center().y());
		r.setHeight(0);
	}

	return r;
}

//----------------------------------------------------
// QwtPlotLayoutEngine::LayoutData::LegendData
//----------------------------------------------------
/**
 * @brief Initialize legend data from a QwtAbstractLegend / 从QwtAbstractLegend初始化图例数据
 *
 * Extracts frame width, scroll extents and size hint from the legend widget.
 *
 * 从图例部件提取边框宽度、滚动范围和尺寸提示。
 *
 * @param legend Pointer to the legend widget / 指向图例部件的指针
 */
void QwtPlotLayoutEngine::LayoutData::LegendData::init(const QwtAbstractLegend* legend)
{
	if (legend) {
		frameWidth    = legend->frameWidth();
		hScrollExtent = legend->scrollExtent(Qt::Horizontal);
		vScrollExtent = legend->scrollExtent(Qt::Vertical);

		hint = legend->sizeHint();
	}
}

/**
 * @brief Calculate optimal legend size for given rectangle / 计算给定矩形的优化图例尺寸
 *
 * Determines the best size for the legend within the available space,
 * considering both width and height constraints.
 *
 * 在可用空间内确定图例的最佳尺寸，同时考虑宽度和高度限制。
 *
 * @param legend Pointer to the legend widget / 指向图例部件的指针
 * @param rect Available rectangle for the legend / 图例的可用矩形区域
 * @return Optimal size for the legend / 图例的优化尺寸
 */
QSize QwtPlotLayoutEngine::LayoutData::LegendData::legendHint(const QwtAbstractLegend* legend, const QRectF& rect) const
{
	const int w = qMin(hint.width(), qwtFloor(rect.width()));

	int h = legend->heightForWidth(w);
	if (h <= 0)
		h = hint.height();

	return QSize(w, h);
}

//----------------------------------------------------
// QwtPlotLayoutEngine::LayoutData::LabelData
//----------------------------------------------------

/**
 * @brief Initialize label data from a QwtTextLabel / 从QwtTextLabel初始化标签数据
 *
 * Extracts text content and frame width from the label widget.
 *
 * 从标签部件提取文本内容和边框宽度。
 *
 * @param label Pointer to the text label widget / 指向文本标签部件的指针
 */
void QwtPlotLayoutEngine::LayoutData::LabelData::init(const QwtTextLabel* label)
{
	frameWidth = 0;
	text       = QwtText();

	if (label) {
		text = label->text();
		if (!(text.testPaintAttribute(QwtText::PaintUsingTextFont)))
			text.setFont(label->font());

		frameWidth = label->frameWidth();
	}
}

//----------------------------------------------------
// QwtPlotLayoutEngine::LayoutData::ScaleData
//----------------------------------------------------

/**
 * @brief Initialize scale data from a QwtScaleWidget / 从QwtScaleWidget初始化刻度数据
 *
 * Extracts various geometric properties from the scale widget including
 * border distances, tick offsets, and dimensions without title.
 *
 * 从刻度部件提取各种几何属性，包括边框距离、刻度偏移量和无标题时的尺寸。
 *
 * @param axisWidget Pointer to the scale widget / 指向刻度部件的指针
 */
void QwtPlotLayoutEngine::LayoutData::ScaleData::init(const QwtScaleWidget* axisWidget)
{
	isVisible = true;

	scaleWidget = axisWidget;
	scaleFont   = axisWidget->font();

	start = axisWidget->startBorderDist();
	end   = axisWidget->endBorderDist();

	baseLineOffset = axisWidget->margin();
    edgeMargin     = axisWidget->edgeMargin();
    tickOffset     = axisWidget->margin();
	if (axisWidget->scaleDraw()->hasComponent(QwtAbstractScaleDraw::Ticks))
		tickOffset += axisWidget->scaleDraw()->maxTickLength();

	dimWithoutTitle = axisWidget->dimForLength(QWIDGETSIZE_MAX, scaleFont);
	if (!axisWidget->title().isEmpty())
		dimWithoutTitle -= axisWidget->titleHeightForWidth(QWIDGETSIZE_MAX);
}

/**
 * @brief Reset scale data to default values / 将刻度数据重置为默认值
 *
 * Sets all scale data properties to zero or false, effectively
 * making the axis invisible in layout calculations.
 *
 * 将所有刻度数据属性设置为零或false，在布局计算中使轴不可见。
 */
void QwtPlotLayoutEngine::LayoutData::ScaleData::reset()
{
	isVisible       = false;
	start           = 0;
	end             = 0;
	baseLineOffset  = 0;
	tickOffset      = 0.0;
	dimWithoutTitle = 0;
}

//----------------------------------------------------
// QwtPlotLayoutEngine::LayoutData::CanvasData
//----------------------------------------------------

/**
 * @brief Initialize canvas data from a QWidget / 从QWidget初始化画布数据
 *
 * Extracts content margins from the canvas widget for all four sides.
 *
 * 从画布部件提取四个方向的内容边距。
 *
 * @param canvas Pointer to the canvas widget / 指向画布部件的指针
 */
void QwtPlotLayoutEngine::LayoutData::CanvasData::init(const QWidget* canvas)
{
	const QMargins m = canvas->contentsMargins();

	contentsMargins[ QwtAxis::YLeft ]   = m.left();
	contentsMargins[ QwtAxis::XTop ]    = m.top();
	contentsMargins[ QwtAxis::YRight ]  = m.right();
	contentsMargins[ QwtAxis::XBottom ] = m.bottom();
}

//----------------------------------------------------
// QwtPlotLayoutEngine::LayoutData
//----------------------------------------------------

/**
 * @brief Construct LayoutData from a QwtPlot / 从QwtPlot构造LayoutData
 *
 * Initializes all layout data by extracting information from
 * the plot's components including legend, labels, axes and canvas.
 *
 * 通过从绘图组件（包括图例、标签、轴和画布）提取信息来初始化所有布局数据。
 *
 * @param plot Pointer to the QwtPlot / 指向QwtPlot的指针
 */
QwtPlotLayoutEngine::LayoutData::LayoutData(const QwtPlot* plot)
{
	legendData.init(plot->legend());
	labelData[ Title ].init(plot->titleLabel());
	labelData[ Footer ].init(plot->footerLabel());

	for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++) {
		{
			const QwtAxisId axisId(axisPos);

			ScaleData& scaleData = axisData(axisId);

			if (plot->isAxisVisible(axisId)) {
				const QwtScaleWidget* scaleWidget = plot->axisWidget(axisId);
				scaleData.init(scaleWidget);
			} else {
				scaleData.reset();
			}
		}
	}

	canvasData.init(plot->canvas());
}

/**
 * @brief Check if Y axes are symmetric / 检查Y轴是否对称
 *
 * Determines if both left and right Y axes are visible or both are hidden.
 *
 * 确定左右Y轴是否都可见或都隐藏。
 *
 * @return True if both Y axes have the same visibility state / 如果两个Y轴具有相同的可见性状态，则返回true
 */
bool QwtPlotLayoutEngine::LayoutData::hasSymmetricYAxes() const
{
    return m_scaleData[ QwtAxis::YLeft ].isVisible == m_scaleData[ QwtAxis::YRight ].isVisible;
}

/**
 * @brief Get scale data for a specific axis / 获取特定轴的刻度数据
 * @param axisId Axis identifier / 轴标识符
 * @return Constant reference to the scale data / 刻度数据的常量引用
 */
const QwtPlotLayoutEngine::LayoutData::ScaleData& QwtPlotLayoutEngine::LayoutData::axisData(QwtAxisId axisId) const
{
    return m_scaleData[ axisId ];
}

/**
 * @brief Get scale data for a specific axis / 获取特定轴的刻度数据
 * @param axisId Axis identifier / 轴标识符
 * @return Mutable reference to the scale data / 刻度数据的可变引用
 */
QwtPlotLayoutEngine::LayoutData::ScaleData& QwtPlotLayoutEngine::LayoutData::axisData(QwtAxisId axisId)
{
    return m_scaleData[ axisId ];
}

/**
 * @brief Get tick offset for a specific axis position / 获取特定轴位置的刻度偏移量
 * @param axisPos Axis position (0-3) / 轴位置（0-3）
 * @return Tick offset value / 刻度偏移值
 */
double QwtPlotLayoutEngine::LayoutData::tickOffset(int axisPos) const
{
    return axisData(axisPos).tickOffset;
}

//----------------------------------------------------
// QwtPlotLayoutEngine
//----------------------------------------------------

/**
 * @brief Default constructor for QwtPlotLayoutEngine / QwtPlotLayoutEngine的默认构造函数
 *
 * Initializes the layout engine with default values:
 * - Legend position: Bottom
 * - Legend ratio: 1.0
 * - Spacing: 5 pixels
 *
 * 使用默认值初始化布局引擎：
 * - 图例位置：底部
 * - 图例比例：1.0
 * - 间距：5像素
 */
QwtPlotLayoutEngine::QwtPlotLayoutEngine() : m_legendPos(QwtPlot::BottomLegend), m_legendRatio(1.0), m_spacing(5)
{
}

/**
 * @brief Calculate legend rectangle within available space / 在可用空间内计算图例矩形
 *
 * Determines the optimal position and size for the legend based on
 * the specified legend position and ratio constraints.
 *
 * 根据指定的图例位置和比例约束，确定图例的最佳位置和大小。
 *
 * @param plotLayoutOptions Layout options bitmask / 布局选项位掩码
 * @param legendData Precalculated legend data / 预计算的图例数据
 * @param rect Available rectangle for layout / 布局的可用矩形区域
 * @param legendHint Preferred size hint for the legend / 图例的首选尺寸提示
 * @return Calculated legend rectangle / 计算出的图例矩形
 */
QRectF QwtPlotLayoutEngine::layoutLegend(int plotLayoutOptions,
                                         const LayoutData::LegendData& legendData,
                                         const QRectF& rect,
                                         const QSize& legendHint) const
{
	QwtPlotLayout::Options options = static_cast< QwtPlotLayout::Options >(plotLayoutOptions);
	int dim;
	if (m_legendPos == QwtPlot::LeftLegend || m_legendPos == QwtPlot::RightLegend) {
		// We don't allow vertical legends to take more than
		// half of the available space.

		dim = qMin(legendHint.width(), int(rect.width() * m_legendRatio));

		if (!(options & QwtPlotLayout::IgnoreScrollbars)) {
			if (legendHint.height() > rect.height()) {
				// The legend will need additional
				// space for the vertical scrollbar.

				dim += legendData.hScrollExtent;
			}
		}
	} else {
		dim = qMin(legendHint.height(), int(rect.height() * m_legendRatio));
		dim = qMax(dim, legendData.vScrollExtent);
	}

	QRectF legendRect = rect;
	switch (m_legendPos) {
	case QwtPlot::LeftLegend: {
		legendRect.setWidth(dim);
		break;
	}
	case QwtPlot::RightLegend: {
		legendRect.setX(rect.right() - dim);
		legendRect.setWidth(dim);
		break;
	}
	case QwtPlot::TopLegend: {
		legendRect.setHeight(dim);
		break;
	}
	case QwtPlot::BottomLegend: {
		legendRect.setY(rect.bottom() - dim);
		legendRect.setHeight(dim);
		break;
	}
	}

	return legendRect;
}

/**
 * @brief Align legend rectangle relative to canvas / 相对于画布对齐图例矩形
 *
 * Adjusts the legend rectangle to be properly aligned with the canvas,
 * ensuring it doesn't extend beyond the canvas boundaries when possible.
 *
 * 调整图例矩形以与画布正确对齐，确保在可能的情况下不超出画布边界。
 *
 * @param legendHint Preferred size hint for the legend / 图例的首选尺寸提示
 * @param canvasRect Canvas rectangle / 画布矩形
 * @param legendRect Initial legend rectangle / 初始图例矩形
 * @return Aligned legend rectangle / 对齐后的图例矩形
 */
QRectF QwtPlotLayoutEngine::alignLegend(const QSize& legendHint, const QRectF& canvasRect, const QRectF& legendRect) const
{
	QRectF alignedRect = legendRect;

	if (m_legendPos == QwtPlot::BottomLegend || m_legendPos == QwtPlot::TopLegend) {
		if (legendHint.width() < canvasRect.width()) {
			alignedRect.setX(canvasRect.x());
			alignedRect.setWidth(canvasRect.width());
		}
	} else {
		if (legendHint.height() < canvasRect.height()) {
			alignedRect.setY(canvasRect.y());
			alignedRect.setHeight(canvasRect.height());
		}
	}

	return alignedRect;
}

/**
 * @brief Align scale rectangles with canvas / 将刻度矩形与画布对齐
 *
 * Adjusts the positions of scale rectangles to ensure proper alignment
 * with the canvas, taking into account border distances and tick offsets.
 *
 * 调整刻度矩形的位置以确保与画布正确对齐，考虑边框距离和刻度偏移量。
 *
 * 这个函数的主要职责是调整坐标轴刻度矩形的位置，确保它们与画布正确对齐。
 * 这是一个复杂的布局计算过程，涉及到多个坐标轴和画布之间的空间协调。
 * 具体计算过程可见代码里的注释
 *
 *
 * @param plotLayoutOptions Layout options bitmask / 布局选项位掩码
 * @param layoutData Precalculated layout data / 预计算的布局数据
 * @param canvasRect Canvas rectangle (may be modified) / 画布矩形（可能被修改）
 * @param scaleRect Array of scale rectangles (will be modified) / 刻度矩形数组（将被修改）
 */
void QwtPlotLayoutEngine::alignScales(int plotLayoutOptions,
                                      const LayoutData& layoutData,
                                      QRectF& canvasRect,
                                      QRectF scaleRect[ QwtAxis::AxisPositions ]) const
{
	QwtPlotLayout::Options options = static_cast< QwtPlotLayout::Options >(plotLayoutOptions);
	using namespace QwtAxis;

	//!1. 初始化骨干偏移量 (Backbone Offset)
	//! 首先计算每个轴的骨干偏移量，这个偏移量由以下几部分组成：
	//! - 如果不对齐画布到刻度，则加上画布边距
	//! - 如果不忽略框架，则加上画布内容边距
	int backboneOffset[ AxisPositions ];
	for (int axisPos = 0; axisPos < AxisPositions; axisPos++) {
		backboneOffset[ axisPos ] = 0;

		if (!m_alignCanvas[ axisPos ]) {
			backboneOffset[ axisPos ] += m_canvasMargin[ axisPos ];
		}

		if (!(options & QwtPlotLayout::IgnoreFrames)) {
			backboneOffset[ axisPos ] += layoutData.canvasData.contentsMargins[ axisPos ];
		}
	}

	//! 2. 处理X轴和Y轴的相互影响
	//! 函数遍历所有轴位置，处理X轴和Y轴之间的相互空间影响：
	//!

	for (int axisPos = 0; axisPos < AxisPositions; axisPos++) {
		{
			QRectF& axisRect = scaleRect[ axisPos ];
			if (!axisRect.isValid())
				continue;

			const QwtAxisId axisId(axisPos);

			const int startDist = layoutData.axisData(axisId).start;
			const int endDist   = layoutData.axisData(axisId).end;
			//! 对于X轴（水平轴）：
			//! 检查左侧Y轴和右侧Y轴的影响
			//! 计算左右偏移量，考虑轴起始/结束距离和骨干偏移
			//! 调整X轴矩形位置，必要时调整画布矩形
			//!
			//! 对于Y轴（垂直轴）：
			//! 检查底部X轴和顶部X轴的影响
			//! 计算上下偏移量，考虑轴起始/结束距离和骨干偏移
			//! 调整Y轴矩形位置，必要时调整画布矩形
			if (isXAxis(axisPos)) {
				const QRectF& leftScaleRect = scaleRect[ YLeft ];
				const int leftOffset        = backboneOffset[ YLeft ] - startDist;

				if (leftScaleRect.isValid()) {
					const double dx = leftOffset + leftScaleRect.width();

					//! 当轴需要比可用空间更多的空间时，函数会调整画布矩形
					if (m_alignCanvas[ YLeft ] && dx < 0.0) {
						/*
						   The axis needs more space than the width
						   of the left scale.
						 */
						const double cLeft = canvasRect.left();  // qreal -> double
						canvasRect.setLeft(qwtMaxF(cLeft, axisRect.left() - dx));
					} else {
						const double minLeft = leftScaleRect.left();
						const double left    = axisRect.left() + leftOffset;
						axisRect.setLeft(qwtMaxF(left, minLeft));
					}
				} else {
					if (m_alignCanvas[ YLeft ] && leftOffset < 0) {
						canvasRect.setLeft(qwtMaxF(canvasRect.left(), axisRect.left() - leftOffset));
					} else {
						if (leftOffset > 0)
							axisRect.setLeft(axisRect.left() + leftOffset);
					}
				}

				const QRectF& rightScaleRect = scaleRect[ YRight ];
				const int rightOffset        = backboneOffset[ YRight ] - endDist + 1;

				if (rightScaleRect.isValid()) {
					const double dx = rightOffset + rightScaleRect.width();
					if (m_alignCanvas[ YRight ] && dx < 0) {
						/*
						   The axis needs more space than the width
						   of the right scale.
						 */
						const double cRight = canvasRect.right();  // qreal -> double
						canvasRect.setRight(qwtMinF(cRight, axisRect.right() + dx));
					}

					const double maxRight = rightScaleRect.right();
					const double right    = axisRect.right() - rightOffset;
					axisRect.setRight(qwtMinF(right, maxRight));
				} else {
					if (m_alignCanvas[ YRight ] && rightOffset < 0) {
						canvasRect.setRight(qwtMinF(canvasRect.right(), axisRect.right() + rightOffset));
					} else {
						if (rightOffset > 0)
							axisRect.setRight(axisRect.right() - rightOffset);
					}
				}
			} else  // y axes
			{
				const QRectF& bottomScaleRect = scaleRect[ XBottom ];
				const int bottomOffset        = backboneOffset[ XBottom ] - endDist + 1;

				if (bottomScaleRect.isValid()) {
					const double dy = bottomOffset + bottomScaleRect.height();
					if (m_alignCanvas[ XBottom ] && dy < 0) {
						/*
						   The axis needs more space than the height
						   of the bottom scale.
						 */
						const double cBottom = canvasRect.bottom();  // qreal -> double
						canvasRect.setBottom(qwtMinF(cBottom, axisRect.bottom() + dy));
					} else {
						const double maxBottom = bottomScaleRect.top() + layoutData.tickOffset(XBottom);
						const double bottom    = axisRect.bottom() - bottomOffset;
						axisRect.setBottom(qwtMinF(bottom, maxBottom));
					}
				} else {
					if (m_alignCanvas[ XBottom ] && bottomOffset < 0) {
						canvasRect.setBottom(qwtMinF(canvasRect.bottom(), axisRect.bottom() + bottomOffset));
					} else {
						if (bottomOffset > 0)
							axisRect.setBottom(axisRect.bottom() - bottomOffset);
					}
				}

				const QRectF& topScaleRect = scaleRect[ XTop ];
				const int topOffset        = backboneOffset[ XTop ] - startDist;

				if (topScaleRect.isValid()) {
					const double dy = topOffset + topScaleRect.height();
					if (m_alignCanvas[ XTop ] && dy < 0) {
						/*
						   The axis needs more space than the height
						   of the top scale.
						 */
						const double cTop = canvasRect.top();  // qreal -> double
						canvasRect.setTop(qwtMaxF(cTop, axisRect.top() - dy));
					} else {
						const double minTop = topScaleRect.bottom() - layoutData.tickOffset(XTop);

						const double top = axisRect.top() + topOffset;
						axisRect.setTop(qwtMaxF(top, minTop));
					}
				} else {
					if (m_alignCanvas[ XTop ] && topOffset < 0) {
						canvasRect.setTop(qwtMaxF(canvasRect.top(), axisRect.top() - topOffset));
					} else {
						if (topOffset > 0)
							axisRect.setTop(axisRect.top() + topOffset);
					}
				}
			}
		}
	}

	/*
	   The canvas has been aligned to the scale with largest
	   border distances. Now we have to realign the other scale.
	 */

	for (int axisPos = 0; axisPos < AxisPositions; axisPos++) {
		{
			const QwtAxisId axisId(axisPos);

			QRectF& sRect                         = scaleRect[ axisPos ];
			const LayoutData::ScaleData& axisData = layoutData.axisData(axisId);

			if (!sRect.isValid())
				continue;

			if (isXAxis(axisPos)) {

				//! 在所有轴相互调整后，函数进行最终的对齐调整，确保轴与画布边缘正确对齐
				if (m_alignCanvas[ YLeft ]) {
					double y = canvasRect.left() - axisData.start;
					if (!(options & QwtPlotLayout::IgnoreFrames))
						y += layoutData.canvasData.contentsMargins[ YLeft ];

					sRect.setLeft(y);
				}

				if (m_alignCanvas[ YRight ]) {
					double y = canvasRect.right() - 1 + axisData.end;
					if (!(options & QwtPlotLayout::IgnoreFrames))
						y -= layoutData.canvasData.contentsMargins[ YRight ];

					sRect.setRight(y);
				}

				if (m_alignCanvas[ axisPos ]) {
					if (axisPos == XTop)
						sRect.setBottom(canvasRect.top());
					else
						sRect.setTop(canvasRect.bottom());
				}
			} else {
				if (m_alignCanvas[ XTop ]) {
					double x = canvasRect.top() - axisData.start;
					if (!(options & QwtPlotLayout::IgnoreFrames))
						x += layoutData.canvasData.contentsMargins[ XTop ];

					sRect.setTop(x);
				}

				if (m_alignCanvas[ XBottom ]) {
					double x = canvasRect.bottom() - 1 + axisData.end;
					if (!(options & QwtPlotLayout::IgnoreFrames))
						x -= layoutData.canvasData.contentsMargins[ XBottom ];

					sRect.setBottom(x);
				}

				if (m_alignCanvas[ axisPos ]) {
					if (axisPos == YLeft)
						sRect.setRight(canvasRect.left());
					else
						sRect.setLeft(canvasRect.right());
				}
			}
		}
	}
}

void QwtPlotLayoutEngine::alignScalesToCanvas(int plotLayoutOptions,
                                              const QwtPlotLayoutEngine::LayoutData& layoutData,
                                              const QRectF& canvasRect,
                                              QRectF scaleRect[ QwtAxis::AxisPositions ]) const
{
	QwtPlotLayout::Options options = static_cast< QwtPlotLayout::Options >(plotLayoutOptions);
	using namespace QwtAxis;

	//!1. 初始化骨干偏移量 (Backbone Offset)
	//! 首先计算每个轴的骨干偏移量，这个偏移量由以下几部分组成：
	//! - 如果不对齐画布到刻度，则加上画布边距
	//! - 如果不忽略框架，则加上画布内容边距
	int backboneOffset[ AxisPositions ];
	for (int axisPos = 0; axisPos < AxisPositions; axisPos++) {
		backboneOffset[ axisPos ] = 0;

		if (!m_alignCanvas[ axisPos ]) {
			backboneOffset[ axisPos ] += m_canvasMargin[ axisPos ];
		}

		if (!(options & QwtPlotLayout::IgnoreFrames)) {
			backboneOffset[ axisPos ] += layoutData.canvasData.contentsMargins[ axisPos ];
		}
	}

	//! 2. 处理X轴和Y轴的相互影响
	//! 函数遍历所有轴位置，处理X轴和Y轴之间的相互空间影响：
	//!

	for (int axisPos = 0; axisPos < AxisPositions; axisPos++) {
		{
			QRectF& axisRect = scaleRect[ axisPos ];
			if (!axisRect.isValid())
				continue;

			const QwtAxisId axisId(axisPos);

			const int startDist = layoutData.axisData(axisId).start;
			const int endDist   = layoutData.axisData(axisId).end;
			//! 对于X轴（水平轴）：
			//! 检查左侧Y轴和右侧Y轴的影响
			//! 计算左右偏移量，考虑轴起始/结束距离和骨干偏移
			//! 调整X轴矩形位置，必要时调整画布矩形
			//!
			//! 对于Y轴（垂直轴）：
			//! 检查底部X轴和顶部X轴的影响
			//! 计算上下偏移量，考虑轴起始/结束距离和骨干偏移
			//! 调整Y轴矩形位置，必要时调整画布矩形
			if (isXAxis(axisPos)) {
				const QRectF& leftScaleRect = scaleRect[ YLeft ];
				const int leftOffset        = backboneOffset[ YLeft ] - startDist;

				if (leftScaleRect.isValid()) {
					const double minLeft = leftScaleRect.left();
					const double left    = axisRect.left() + leftOffset;
					axisRect.setLeft(qwtMaxF(left, minLeft));
				} else {
					if (leftOffset > 0) {
						axisRect.setLeft(axisRect.left() + leftOffset);
					}
				}

				const QRectF& rightScaleRect = scaleRect[ YRight ];
				const int rightOffset        = backboneOffset[ YRight ] - endDist + 1;

				if (rightScaleRect.isValid()) {
					const double maxRight = rightScaleRect.right();
					const double right    = axisRect.right() - rightOffset;
					axisRect.setRight(qwtMinF(right, maxRight));
				} else {
					if (rightOffset > 0) {
						axisRect.setRight(axisRect.right() - rightOffset);
					}
				}
			} else {  // y axes
				const QRectF& bottomScaleRect = scaleRect[ XBottom ];
				const int bottomOffset        = backboneOffset[ XBottom ] - endDist + 1;

				if (bottomScaleRect.isValid()) {
					const double maxBottom = bottomScaleRect.top() + layoutData.tickOffset(XBottom);
					const double bottom    = axisRect.bottom() - bottomOffset;
					axisRect.setBottom(qwtMinF(bottom, maxBottom));
				} else {

					if (bottomOffset > 0) {
						axisRect.setBottom(axisRect.bottom() - bottomOffset);
					}
				}

				const QRectF& topScaleRect = scaleRect[ XTop ];
				const int topOffset        = backboneOffset[ XTop ] - startDist;

				if (topScaleRect.isValid()) {
					const double minTop = topScaleRect.bottom() - layoutData.tickOffset(XTop);
					const double top    = axisRect.top() + topOffset;
					axisRect.setTop(qwtMaxF(top, minTop));
				} else {
					if (topOffset > 0) {
						axisRect.setTop(axisRect.top() + topOffset);
					}
				}
			}
		}
	}

	/*
	   The canvas has been aligned to the scale with largest
	   border distances. Now we have to realign the other scale.
	 */

	for (int axisPos = 0; axisPos < AxisPositions; axisPos++) {
		{
			const QwtAxisId axisId(axisPos);

			QRectF& sRect                         = scaleRect[ axisPos ];
			const LayoutData::ScaleData& axisData = layoutData.axisData(axisId);

			if (!sRect.isValid())
				continue;

			if (isXAxis(axisPos)) {

				//! 在所有轴相互调整后，函数进行最终的对齐调整，确保轴与画布边缘正确对齐
				if (m_alignCanvas[ YLeft ]) {
					double y = canvasRect.left() - axisData.start;
					if (!(options & QwtPlotLayout::IgnoreFrames))
						y += layoutData.canvasData.contentsMargins[ YLeft ];

					sRect.setLeft(y);
				}

				if (m_alignCanvas[ YRight ]) {
					double y = canvasRect.right() - 1 + axisData.end;
					if (!(options & QwtPlotLayout::IgnoreFrames))
						y -= layoutData.canvasData.contentsMargins[ YRight ];

					sRect.setRight(y);
				}

				if (m_alignCanvas[ axisPos ]) {
					if (axisPos == XTop)
						sRect.setBottom(canvasRect.top());
					else
						sRect.setTop(canvasRect.bottom());
				}
			} else {
				if (m_alignCanvas[ XTop ]) {
					double x = canvasRect.top() - axisData.start;
					if (!(options & QwtPlotLayout::IgnoreFrames))
						x += layoutData.canvasData.contentsMargins[ XTop ];

					sRect.setTop(x);
				}

				if (m_alignCanvas[ XBottom ]) {
					double x = canvasRect.bottom() - 1 + axisData.end;
					if (!(options & QwtPlotLayout::IgnoreFrames))
						x -= layoutData.canvasData.contentsMargins[ XBottom ];

					sRect.setBottom(x);
				}

				if (m_alignCanvas[ axisPos ]) {
					if (axisPos == YLeft)
						sRect.setRight(canvasRect.left());
					else
						sRect.setLeft(canvasRect.right());
				}
			}
		}
	}
}

/**
 * @brief Iteratively calculate the layout dimensions for a QwtPlot / 迭代计算QwtPlot的布局尺寸
 *
 * This routine determines the exact space (in pixels) that every visual
 * component of a QwtPlot—title, footer, and the four axes (X-top,
 * X-bottom, Y-left, Y-right)—needs inside a given rectangle.
 * Because the required size of one component affects the available space
 * for all others (e.g. a taller horizontal axis shrinks the vertical
 * axes, which may then wrap their labels and expand horizontally, and so
 * on), the algorithm loops until the dimensions stabilize.
 *
 * 本函数在给定矩形区域内，精确地计算QwtPlot中每一个可视部件
 * （标题、页脚以及四个坐标轴：上、下、左、右）所占用的像素尺寸。
 * 由于各部件相互影响（例如水平轴变高会压缩垂直轴，垂直轴标签换行
 * 又可能使水平轴再次换行），因此采用迭代方式，直到所有尺寸不再变化。
 *
 * @param plotLayoutOptions Layout options bitmask / 布局选项位掩码
 * @param layoutData Precalculated layout data / 预计算的布局数据
 * @param rect Available rectangle for layout / 布局的可用矩形区域
 * @return Dimensions structure with calculated sizes / 包含计算尺寸的Dimensions结构体
 */
QwtPlotLayoutEngine::Dimensions QwtPlotLayoutEngine::layoutDimensions(int plotLayoutOptions,
                                                                      const LayoutData& layoutData,
                                                                      const QRectF& rect) const
{
	using namespace QwtAxis;
	QwtPlotLayout::Options options = static_cast< QwtPlotLayout::Options >(plotLayoutOptions);
	Dimensions dimensions;

	int backboneOffset[ AxisPositions ];
	for (int axisPos = 0; axisPos < AxisPositions; axisPos++) {
		backboneOffset[ axisPos ] = 0;
		if (!(options & QwtPlotLayout::IgnoreFrames))
			backboneOffset[ axisPos ] += layoutData.canvasData.contentsMargins[ axisPos ];

		if (!m_alignCanvas[ axisPos ])
			backboneOffset[ axisPos ] += m_canvasMargin[ axisPos ];
	}

	bool done = false;
	while (!done) {
		done = true;

		// the size for the 4 axis depend on each other. Expanding
		// the height of a horizontal axis will shrink the height
		// for the vertical axis, shrinking the height of a vertical
		// axis will result in a line break what will expand the
		// width and results in shrinking the width of a horizontal
		// axis what might result in a line break of a horizontal
		// axis ... . So we loop as long until no size changes.
        // 四个坐标轴的尺寸彼此相互影响：
        // 增加水平轴的高度会压缩垂直轴的高度；
        // 垂直轴高度一旦减小，就可能引发换行，从而增加其宽度，
        // 进而压缩水平轴的可用宽度，又可能导致水平轴换行……
        // 因此需要循环计算，直到所有尺寸都不再变化为止。

		if (!(options & QwtPlotLayout::IgnoreTitle)) {
			const int d = heightForWidth(LayoutData::Title, layoutData, options, rect.width(), dimensions.dimYAxes());

			if (d > dimensions.dimTitle) {
				dimensions.dimTitle = d;
				done                = false;
			}
		}

		if (!(options & QwtPlotLayout::IgnoreFooter)) {
			const int d = heightForWidth(LayoutData::Footer, layoutData, options, rect.width(), dimensions.dimYAxes());

			if (d > dimensions.dimFooter) {
				dimensions.dimFooter = d;
				done                 = false;
			}
		}

		for (int axisPos = 0; axisPos < AxisPositions; axisPos++) {
			{
				const QwtAxisId axisId(axisPos);

				const LayoutData::ScaleData& scaleData = layoutData.axisData(axisId);

				if (scaleData.isVisible) {
					double length;
					if (isXAxis(axisPos)) {
						length = rect.width() - dimensions.dimYAxes();
						length -= scaleData.start + scaleData.end;

						if (dimensions.dimAxes(YRight) > 0)
							length -= 1;

						length += qMin(dimensions.dimAxes(YLeft), scaleData.start - backboneOffset[ YLeft ]);

						length += qMin(dimensions.dimAxes(YRight), scaleData.end - backboneOffset[ YRight ]);
                    } else {  // y axis
						length = rect.height() - dimensions.dimXAxes();
						length -= scaleData.start + scaleData.end;
						length -= 1;

						if (dimensions.dimAxes(XBottom) <= 0)
							length -= 1;

						if (dimensions.dimAxes(XTop) <= 0)
							length -= 1;

						/*
						   The tick labels of the y axes are always left/right from the
						   backbone/ticks of the x axes - but we have to take care,
						   that the labels don't overlap.
						 */
						if (dimensions.dimAxes(XBottom) > 0) {
							length += qMin(layoutData.tickOffset(XBottom),
                                           double(scaleData.start - backboneOffset[ XBottom ]));
						}

						if (dimensions.dimAxes(XTop) > 0) {
							length += qMin(layoutData.tickOffset(XTop), double(scaleData.end - backboneOffset[ XTop ]));
						}

						if (dimensions.dimTitle > 0)
							length -= dimensions.dimTitle + m_spacing;
					}

					int d = scaleData.dimWithoutTitle;
					if (!scaleData.scaleWidget->title().isEmpty()) {
						d += scaleData.scaleWidget->titleHeightForWidth(qwtFloor(length));
					}

					if (d > dimensions.dimAxis(axisId)) {
						dimensions.setDimAxis(axisId, d);
						done = false;
					}
				}
			}
		}
	}

	return dimensions;
}

/**
 * @brief Get the legend ratio / 获取图例比例
 * @return Current legend ratio value / 当前图例比例值
 */
double QwtPlotLayoutEngine::legendRatio() const
{
    return m_legendRatio;
}

/**
 * @brief Set the legend ratio / 设置图例比例
 * @param ratio New legend ratio value / 新的图例比例值
 */
void QwtPlotLayoutEngine::setLegendRatio(double ratio)
{
    m_legendRatio = ratio;
}

/**
 * @brief Get the legend position / 获取图例位置
 * @return Current legend position / 当前图例位置
 */
QwtPlot::LegendPosition QwtPlotLayoutEngine::legendPos() const
{
    return m_legendPos;
}

/**
 * @brief Set the legend position / 设置图例位置
 * @param pos New legend position / 新的图例位置
 */
void QwtPlotLayoutEngine::setLegendPos(QwtPlot::LegendPosition pos)
{
    m_legendPos = pos;
}

/**
 * @brief Get canvas margin for a specific axis / 获取特定轴的画布边距
 * @param axisPos Axis position (0-3) / 轴位置（0-3）
 * @return Canvas margin value / 画布边距值
 */
int QwtPlotLayoutEngine::canvasMargin(int axisPos) const
{
    return m_canvasMargin[ axisPos ];
}

/**
 * @brief Set canvas margin for a specific axis / 设置特定轴的画布边距
 * @param axisPos Axis position (0-3) / 轴位置（0-3）
 * @param margin New canvas margin value / 新的画布边距值
 */
void QwtPlotLayoutEngine::setCanvasMargin(int axisPos, int margin)
{
    m_canvasMargin[ axisPos ] = margin;
}

/**
 * @brief Check if canvas is aligned to scale for a specific axis / 检查画布是否对齐到特定轴的刻度
 * @param axisPos Axis position (0-3) / 轴位置（0-3）
 * @return True if canvas is aligned to scale / 如果画布对齐到刻度，则返回true
 */
bool QwtPlotLayoutEngine::alignCanvas(int axisPos) const
{
    return m_alignCanvas[ axisPos ];
}

/**
 * @brief Set canvas alignment to scale for a specific axis / 设置画布对齐到特定轴的刻度
 * @param axisPos Axis position (0-3) / 轴位置（0-3）
 * @param on True to align canvas to scale / 为true时对齐画布到刻度
 */
void QwtPlotLayoutEngine::setAlignCanvas(int axisPos, bool on)
{
    m_alignCanvas[ axisPos ] = on;
}

/**
 * @brief Get spacing value / 获取间距值
 * @return Current spacing value in pixels / 当前的间距值（像素）
 */
unsigned int QwtPlotLayoutEngine::spacing() const
{
    return m_spacing;
}

/**
 * @brief Set spacing value / 设置间距值
 * @param spacing New spacing value in pixels / 新的间距值（像素）
 */
void QwtPlotLayoutEngine::setSpacing(unsigned int spacing)
{
    m_spacing = spacing;
}

/**
 * @brief Calculate height for width for a label / 计算标签的宽度对应高度
 *
 * Determines the required height for a label given a specific width,
 * taking into account text wrapping and frame width.
 *
 * 在给定特定宽度的情况下确定标签所需的高度，考虑文本换行和边框宽度。
 *
 * @param labelType Type of label (Title or Footer) / 标签类型（标题或页脚）
 * @param layoutData Precalculated layout data / 预计算的布局数据
 * @param plotLayoutOptions Layout options bitmask / 布局选项位掩码
 * @param width Available width / 可用宽度
 * @param axesWidth Total width of axes / 轴的总宽度
 * @return Calculated height / 计算出的高度
 */
int QwtPlotLayoutEngine::heightForWidth(LayoutData::Label labelType,
                                        const LayoutData& layoutData,
                                        int plotLayoutOptions,
                                        double width,
                                        int axesWidth) const
{
	QwtPlotLayout::Options options         = static_cast< QwtPlotLayout::Options >(plotLayoutOptions);
	const LayoutData::LabelData& labelData = layoutData.labelData[ labelType ];

	if (labelData.text.isEmpty())
		return 0;

	double w = width;

	if (!layoutData.hasSymmetricYAxes()) {
		// center to the canvas
		w -= axesWidth;
	}

	int d = qwtCeil(labelData.text.heightForWidth(w));
	if (!(options & QwtPlotLayout::IgnoreFrames))
		d += 2 * labelData.frameWidth;

	return d;
}
