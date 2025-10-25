#include "qwt_figure_layout.h"
#include <QMap>
#include <QDebug>
#include <QWidget>

#ifndef QWTFIGURELAYOUT_DEBUG_PRINT
#define QWTFIGURELAYOUT_DEBUG_PRINT 0
#endif
class QwtFigureLayout::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtFigureLayout)
public:
	PrivateData(QwtFigureLayout* p) : q_ptr(p)
	{
	}

	/**
	 * @brief Item wrapper containing layout information
	 *
	 * 包含布局信息的项包装器
	 */
	struct LayoutItem
	{
		QLayoutItem* item { nullptr };  ///< Pointer to the layout item / 指向布局项的指针
		QRectF normRect;                ///< Normalized coordinates [0,1] / 归一化坐标 [0,1]
	};

public:
	QList< LayoutItem > m_items;  ///< List of layout items / 布局项列表

	// Layout parameters with default values / 布局参数（带默认值）
	qreal m_left { 0.02 };    ///< Left margin / 左边距
	qreal m_bottom { 0.02 };  ///< Bottom margin / 底边距
	qreal m_right { 0.02 };   ///< Right margin / 右边距
	qreal m_top { 0.02 };     ///< Top margin / 上边距
};

//----------------------------------------------------
// QwtFigureLayout
//----------------------------------------------------
QwtFigureLayout::QwtFigureLayout() : QLayout(), QWT_PIMPL_CONSTRUCT
{
}

QwtFigureLayout::QwtFigureLayout(QWidget* parent) : QLayout(parent), QWT_PIMPL_CONSTRUCT
{
}

QwtFigureLayout::~QwtFigureLayout()
{
	while (!m_data->m_items.isEmpty()) {
		QwtFigureLayout::PrivateData::LayoutItem item = m_data->m_items.takeFirst();
		delete item.item;
	}
}

void QwtFigureLayout::addItem(QLayoutItem* item)
{
	if (!item) {
		qWarning() << "Attempted to add null item to QwtFigureLayout";
		return;
	}
	QwtFigureLayout::PrivateData::LayoutItem li;
	li.item     = item;
	li.normRect = QRectF(0, 0, 1, 1);  // Default full size / 默认全尺寸
	m_data->m_items.append(li);
}

QLayoutItem* QwtFigureLayout::itemAt(int index) const
{
	if (index < 0 || index >= m_data->m_items.size()) {
		return nullptr;
	}
	return m_data->m_items[ index ].item;
}

QLayoutItem* QwtFigureLayout::takeAt(int index)
{
	if (index < 0 || index >= m_data->m_items.size()) {
		return nullptr;
	}
	QwtFigureLayout::PrivateData::LayoutItem li = m_data->m_items.takeAt(index);
	QLayoutItem* item                           = li.item;
	return item;
}

int QwtFigureLayout::count() const
{
	return m_data->m_items.size();
}

QSize QwtFigureLayout::sizeHint() const
{
	return minimumSize();
}

QSize QwtFigureLayout::minimumSize() const
{
	QSize size;
	for (const auto& item : qAsConst(m_data->m_items)) {
		if (item.item && item.item->widget())
			size = size.expandedTo(item.item->minimumSize());
	}
	return size;
}

void QwtFigureLayout::setGeometry(const QRect& rect)
{
	QLayout::setGeometry(rect);

	const int width  = rect.width();
	const int height = rect.height();

	// Skip layout if geometry is invalid
	if (width <= 0 || height <= 0) {
		return;
	}

	// Calculate available space after applying margins
	// 计算应用边距后的可用空间
	const qreal availableWidth  = width * (1.0 - m_data->m_left - m_data->m_right);
	const qreal availableHeight = height * (1.0 - m_data->m_bottom - m_data->m_top);
	const qreal startX          = width * m_data->m_left;
	const qreal startY          = height * m_data->m_top;
#if QWTFIGURELAYOUT_DEBUG_PRINT && QWT_DEBUG_DRAW
	qDebug() << "QwtFigureLayout setGeometry(rect=" << rect << "),left=" << m_data->m_left
             << ",right=" << m_data->m_right << ",bottom=" << m_data->m_bottom << ",top=" << m_data->m_top;
#endif
	for (const auto& item : qAsConst(m_data->m_items)) {
		if (!item.item || !item.item->widget() || !item.item->widget()->isVisibleTo(item.item->widget()->parentWidget())) {
			continue;
		}

		QRectF normRect = item.normRect;

		// Convert normalized coordinates to actual pixels using Qt's top-left coordinate system
		// Apply margins to both grid-based and normalized coordinate-based items
		// 将归一化坐标转换为实际像素，使用Qt的左上角坐标系
		// 对基于网格和基于归一化坐标的项应用边距
        const qreal actualLeft   = startX + qRound(normRect.left() * availableWidth);
        const qreal actualTop    = startY + qRound(normRect.top() * availableHeight);
        const qreal actualWidth  = qRound(normRect.width() * availableWidth);
        const qreal actualHeight = qRound(normRect.height() * availableHeight);

		// Ensure the rect is within valid bounds
		QRect actualRect(qMax(0.0, actualLeft),
                         qMax(0.0, actualTop),
                         qMin(actualWidth, width - actualLeft),
                         qMin(actualHeight, height - actualTop));

		item.item->setGeometry(actualRect);
#if QWTFIGURELAYOUT_DEBUG_PRINT && QWT_DEBUG_DRAW
		qDebug() << "normRect=" << normRect << ",actualRect=" << actualRect;
#endif
	}
}

/**
 * @brief Add a widget with normalized coordinates/使用归一化坐标添加窗口部件
 *
 * This method adds a widget to the layout using normalized coordinates in the range [0,1].
 * The coordinates are specified as [left, top, width, height], where:
 * - left: distance from the left edge of the figure
 * - top: distance from the top edge of the figure
 * - width: width of the widget
 * - height: height of the widget
 *
 * 此方法使用[0,1]范围内的归一化坐标将窗口部件添加到布局中。
 * 坐标指定为[左, 上, 宽, 高]，其中：
 * - 左: 距图形左边缘的距离
 * - 上: 距图形上边缘的距离
 * - 宽: 窗口部件的宽度
 * - 高: 窗口部件的高度
 *
 * @note All coordinates must be in the range [0,1]. The sum of left + width should not exceed 1,
 *       and the sum of top + height should not exceed 1.
 *       所有坐标必须在[0,1]范围内。左+宽不应超过1，上+高不应超过1。
 *
 * @param widget Widget to add / 要添加的窗口部件
 * @param rect Normalized coordinates [left, top, width, height] in range [0,1]
 *              归一化坐标 [左, 上, 宽, 高]，范围 [0,1]
 *
 * @code
 * // Add a widget that occupies the top-left quarter of the figure
 * // 添加一个占据图形左上角四分之一的窗口部件
 * QWidget* widget = new QWidget;
 * layout->addAxes(widget, QRectF(0.0, 0.0, 0.5, 0.5));
 * @endcode
 *
 * @code
 * // Add a widget that occupies the bottom-right quarter of the figure
 * // 添加一个占据图形右下角四分之一的窗口部件
 * QWidget* widget = new QWidget;
 * layout->addAxes(widget, QRectF(0.5, 0.5, 0.5, 0.5));
 * @endcode
 */
void QwtFigureLayout::addAxes(QWidget* widget, const QRectF& rect)
{
	if (!widget) {
		qWarning() << "Attempted to add null widget to QwtFigureLayout";
		return;
	}

	QLayoutItem* item = new QWidgetItem(widget);
	QwtFigureLayout::PrivateData::LayoutItem li;
	li.item     = item;
	li.normRect = rect;
	m_data->m_items.append(li);
}

/**
 * @brief Add a widget with normalized coordinates using separate parameters/使用分离参数和归一化坐标添加窗口部件
 *
 * This is a convenience overload that adds a widget to the layout using normalized coordinates
 * in the range [0,1] with separate left, top, width, and height parameters.
 * The coordinates use Qt's standard top-left coordinate system.
 *
 * 这是一个便捷的重载函数，使用[0,1]范围内的归一化坐标和独立的左、上、宽、高参数将窗口部件添加到布局中。
 * 坐标使用Qt的标准左上角坐标系。
 *
 * @param widget Widget to add / 要添加的窗口部件
 * @param left Normalized distance from the left edge [0,1] / 距左边缘的归一化距离 [0,1]
 * @param top Normalized distance from the top edge [0,1] / 距上边缘的归一化距离 [0,1]
 * @param width Normalized width of the widget [0,1] / 窗口部件的归一化宽度 [0,1]
 * @param height Normalized height of the widget [0,1] / 窗口部件的归一化高度 [0,1]
 *
 * @note All parameters must be in the range [0,1]. The sum of left + width should not exceed 1,
 *       and the sum of top + height should not exceed 1.
 *       所有参数必须在[0,1]范围内。左+宽不应超过1，上+高不应超过1。
 *
 * @code
 * // Add a widget that occupies the top-left quarter of the figure
 * // 添加一个占据图形左上角四分之一的窗口部件
 * QWidget* widget = new QWidget;
 * layout->addAxes(widget, 0.0, 0.0, 0.5, 0.5);
 * @endcode
 *
 * @code
 * // Add a widget that occupies the center of the figure
 * // 添加一个占据图形中心的窗口部件
 * QWidget* widget = new QWidget;
 * layout->addAxes(widget, 0.25, 0.25, 0.5, 0.5);
 * @endcode
 */
void QwtFigureLayout::addAxes(QWidget* widget, qreal left, qreal top, qreal width, qreal height)
{
    addAxes(widget, QRectF(left, top, width, height));
}

/**
 * @brief Add a widget by grid layout/添加窗口部件到网格布局
 *
 * This method adds a widget to the grid layout at the specified position with optional row and column spans.
 * The grid position is 0-based, with (0,0) being the top-left cell of the grid.
 * The normalized coordinates are calculated immediately and stored with the widget.
 *
 * 此方法将窗口部件添加到网格布局中的指定位置，可选择跨行和跨列。
 * 网格位置从0开始，(0,0)表示网格的左上角单元格。
 * 归一化坐标会立即计算并与窗口部件一起存储。
 *
 * @param widget Widget to add / 要添加的窗口部件
 * @param rowCnt Total number of rows in the grid / 网格总行数
 * @param colCnt Total number of columns in the grid / 网格总列数
 * @param row Grid row position (0-based) / 网格行位置（从0开始）
 * @param col Grid column position (0-based) / 网格列位置（从0开始）
 * @param rowSpan Number of rows to span (default: 1) / 跨行数（默认：1）
 * @param colSpan Number of columns to span (default: 1) / 跨列数（默认：1）
 * @param wspace Horizontal space between subplots [0,1] / 子图之间的水平间距 [0,1]
 * @param hspace Vertical space between subplots [0,1] / 子图之间的垂直间距 [0,1]
 * @code
 * // Create a 2x2 grid and add widgets
 * // 创建一个2x2网格并添加窗口部件
 * //
 * // Grid layout visualization (2x2):
 * // 网格布局可视化 (2x2):
 * // +-------------------+-------------------+
 * // |                   |                   |
 * // |     (0,0)         |      (0,1)        |
 * // |                   |                   |
 * // +-------------------+-------------------+
 * // |                   |                   |
 * // |     (1,0)         |      (1,1)        |
 * // |                   |                   |
 * // +-------------------+-------------------+
 *
 * // Add a widget that spans the entire top row (row 0, columns 0-1)
 * // 添加一个占据整个顶行（第0行，第0-1列）的窗口部件
 * QWidget* topWidget = new QWidget;
 * layout->addAxes(topWidget, 2, 2, 0, 0, 1, 2);
 * //
 * // After adding topWidget:
 * // 添加 topWidget 后:
 * // +---------------------------------------+
 * // |                                       |
 * // |            topWidget (0,0-1)          |
 * // |                                       |
 * // +-------------------+-------------------+
 * // |                   |                   |
 * // |     (1,0)         |      (1,1)        |
 * // |                   |                   |
 * // +-------------------+-------------------+
 *
 * // Add a widget to the bottom-left cell (row 1, column 0)
 * // 添加一个到底部左侧单元格（第1行，第0列）的窗口部件
 * QWidget* bottomLeftWidget = new QWidget;
 * layout->addAxes(bottomLeftWidget, 2, 2, 1, 0);
 * //
 * // After adding bottomLeftWidget:
 * // 添加 bottomLeftWidget 后:
 * // +---------------------------------------+
 * // |                                       |
 * // |            topWidget (0,0-1)          |
 * // |                                       |
 * // +-------------------+-------------------+
 * // |                   |                   |
 * // | bottomLeft (1,0)  |      (1,1)        |
 * // |                   |                   |
 * // +-------------------+-------------------+
 *
 * // Add a widget to the bottom-right cell (row 1, column 1)
 * // 添加一个到底部右侧单元格（第1行，第1列）的窗口部件
 * QWidget* bottomRightWidget = new QWidget;
 * layout->addAxes(bottomRightWidget, 2, 2, 1, 1);
 * //
 * // Final layout:
 * // 最终布局:
 * // +---------------------------------------+
 * // |                                       |
 * // |            topWidget (0,0-1)          |
 * // |                                       |
 * // +-------------------+-------------------+
 * // |                   |                   |
 * // | bottomLeft (1,0)  | bottomRight (1,1) |
 * // |                   |                   |
 * // +-------------------+-------------------+
 * @endcode
 */
void QwtFigureLayout::addAxes(QWidget* widget, int rowCnt, int colCnt, int row, int col, int rowSpan, int colSpan, qreal wspace, qreal hspace)
{
	if (!widget) {
		qWarning() << "QwtFigureLayout::addToGrid get a null widget";
		return;
	}
	if (row < 0 || col < 0 || rowSpan <= 0 || colSpan <= 0 || rowCnt <= 0 || colCnt <= 0) {
		qWarning()
            << "QwtFigureLayout::addToGrid Grid row, column, rowSpan, colSpan, rowCnt and colCnt should be positive.";
		return;
	}

	if (row + rowSpan > rowCnt || col + colSpan > colCnt) {
		qWarning() << "QwtFigureLayout::addToGrid Grid position and span exceed grid dimensions.";
		return;
	}

	// Calculate normalized coordinates
	QRectF normRect = calcGridRect(rowCnt, colCnt, row, col, rowSpan, colSpan, wspace, hspace);

	QLayoutItem* item = new QWidgetItem(widget);
	QwtFigureLayout::PrivateData::LayoutItem li;
	li.item     = item;
	li.normRect = normRect;
    m_data->m_items.append(li);
}

/**
 * @brief 改变已经添加的窗口的位置占比,如果窗口还没添加，此函数无效
 *
 * @note 此函数不会自动刷新窗口位置，需要用户手动刷新
 * @param widget
 * @param rect
 */
void QwtFigureLayout::setAxesNormPos(QWidget* widget, const QRectF& rect)
{
    for (QwtFigureLayout::PrivateData::LayoutItem& i : m_data->m_items) {
        if (i.item->widget() == widget) {
            i.normRect = rect;
        }
    }
}

/**
 * @brief Update layout parameters/更新布局参数
 *
 * This method adjusts the layout parameters similar to matplotlib's subplots_adjust function.
 * It allows fine-tuning the spacing between subplots and the margins around the figure.
 *
 * 此方法调整布局参数，类似于matplotlib的subplots_adjust函数。
 * 它允许微调子图之间的间距和图形周围的边距。
 *
 * @note All parameters must be in the range [0,1]. The sum of left + right should be less than 1,
 *       and the sum of bottom + top should be less than 1 to ensure visible content.
 *       所有参数必须在[0,1]范围内。左+右应小于1，下+上应小于1以确保内容可见。
 *
 * @param left Left margin [0,1] / 左边距 [0,1]
 * @param bottom Bottom margin [0,1] / 底边距 [0,1]
 * @param right Right margin [0,1] / 右边距 [0,1]
 * @param top Top margin [0,1] / 上边距 [0,1]
 *
 * @code
 * // Adjust layout with generous margins and spacing
 * // 调整布局，使用较大的边距和间距
 * layout->adjustLayout(0.1, 0.1, 0.1, 0.1);
 * @endcode
 *
 * @code
 * // Adjust layout with minimal margins and spacing
 * // 调整布局，使用最小的边距和间距
 * layout->adjustLayout(0.05, 0.05, 0.05, 0.05);
 * @endcode
 */
void QwtFigureLayout::adjustLayout(qreal left, qreal bottom, qreal right, qreal top)
{
	// Validate parameters
	if (left < 0.0 || left > 1.0 || bottom < 0.0 || bottom > 1.0 || right < 0.0 || right > 1.0 || top < 0.0 || top > 1.0) {
		qWarning() << "All layout parameters must be in range [0,1]";
		return;
	}

	if (left + right >= 1.0) {
		qWarning() << "Left + right margins should be less than 1.0 to have visible content";
	}

	if (bottom + top >= 1.0) {
		qWarning() << "Bottom + top margins should be less than 1.0 to have visible content";
	}

	m_data->m_left   = left;
	m_data->m_bottom = bottom;
	m_data->m_right  = right;
	m_data->m_top    = top;

	// Invalidate the layout to trigger recalculation
	invalidate();
}

/**
 * @brief Get the normalized rectangle for a widget/获取窗口部件的归一化矩形
 *
 * This method returns the normalized coordinates [0,1] for the specified widget
 * in the layout. If the widget is not found in the layout, an invalid QRectF is returned.
 *
 * 此方法返回布局中指定窗口部件的归一化坐标[0,1]。如果在布局中未找到该窗口部件，则返回无效的QRectF。
 *
 * @param widget Widget to query / 要查询的窗口部件
 * @return Normalized coordinates [left, top, width, height] in range [0,1], or invalid QRectF if not found
 *         归一化坐标 [左, 上, 宽, 高]，范围 [0,1]，如果未找到则返回无效QRectF
 *
 * @code
 * // Get the normalized position of a widget
 * // 获取窗口部件的归一化位置
 * QRectF normRect = layout->widgetNormRect(widget);
 * if (normRect.isValid()) {
 *     qDebug() << "Widget position:" << normRect;
 * } else {
 *     qDebug() << "Widget not found in layout";
 * }
 * @endcode
 */
QRectF QwtFigureLayout::widgetNormRect(QWidget* widget) const
{
	if (!widget) {
		qWarning() << "QwtFigureLayout::getAxesNormRect: null widget provided";
		return QRectF();
	}

	for (const auto& item : qAsConst(m_data->m_items)) {
		if (item.item && item.item->widget() == widget) {
			return item.normRect;
		}
	}
    return QRectF();  // Return invalid rect
}

/**
 * @brief 计算rect相对于parentRect的归一化坐标
 * @param parentRect 父矩形（像素坐标）
 * @param rect 子矩形（像素坐标，相对于parentRect）
 * @return 归一化坐标QRectF（left, top, width, height均在[0,1]范围）
 */
QRectF QwtFigureLayout::calcNormRect(const QRect& parentRect, const QRect& rect)
{
    // 处理父矩形为空的边界情况（避免除零）
    if (parentRect.isEmpty()) {
        return QRectF();
    }

    // 提取父矩形的宽高（确保为正数，避免异常值）
    const int parentWidth  = qMax(parentRect.width(), 1);  // 最小为1，防止除零
    const int parentHeight = qMax(parentRect.height(), 1);

    // 计算归一化坐标（使用double确保精度）
    const double left   = static_cast< double >(rect.x()) / parentWidth;
    const double top    = static_cast< double >(rect.y()) / parentHeight;
    const double width  = static_cast< double >(rect.width()) / parentWidth;
    const double height = static_cast< double >(rect.height()) / parentHeight;

    // 优化精度：四舍五入到小数点后6位（兼顾精度和浮点数表示稳定性）
    const double precision = 1e-6;
    auto roundToPrecision  = [ precision ](double value) { return qRound(value / precision) * precision; };

    // 确保归一化坐标在[0,1]范围内（处理可能的边界误差）
    const double clampedLeft   = qBound(0.0, roundToPrecision(left), 1.0);
    const double clampedTop    = qBound(0.0, roundToPrecision(top), 1.0);
    const double clampedWidth  = qBound(0.0, roundToPrecision(width), 1.0 - clampedLeft);
    const double clampedHeight = qBound(0.0, roundToPrecision(height), 1.0 - clampedTop);

    return QRectF(clampedLeft, clampedTop, clampedWidth, clampedHeight);
}

/**
 * @brief calc the normalized rectangle for a grid cell/获取网格单元格的归一化矩形
 *
 * This method calculates the normalized coordinates for a specific grid cell
 * based on the current layout parameters and grid configuration.
 *
 * 此方法根据当前布局参数和网格配置计算特定网格单元格的归一化坐标。
 *
 * @param rowCnt Total number of rows in the grid / 网格总行数
 * @param colCnt Total number of columns in the grid / 网格总列数
 * @param row Grid row position (0-based) / 网格行位置（从0开始）
 * @param col Grid column position (0-based) / 网格列位置（从0开始）
 * @param rowSpan Number of rows to span (default: 1) / 跨行数（默认：1）
 * @param colSpan Number of columns to span (default: 1) / 跨列数（默认：1）
 * @return Normalized coordinates [left, top, width, height] in range [0,1]
 *         归一化坐标 [左, 上, 宽, 高]，范围 [0,1]
 *
 * @code
 * // Get the normalized rectangle for the top-left cell in a 2x2 grid
 * // 获取2x2网格中左上角单元格的归一化矩形
 * QRectF rect = layout->calcGridRect(2, 2, 0, 0);
 * @endcode
 *
 * @code
 * // Get the normalized rectangle for a cell spanning two columns
 * // 获取跨两列的单元格的归一化矩形
 * QRectF rect = layout->calcGridRect(3, 3, 1, 0, 1, 2);
 * @endcode
 */
QRectF QwtFigureLayout::calcGridRect(int rowCnt, int colCnt, int row, int col, int rowSpan, int colSpan, qreal wspace, qreal hspace) const
{
	if (rowCnt <= 0 || colCnt <= 0 || row < 0 || col < 0 || rowSpan <= 0 || colSpan <= 0 || row + rowSpan > rowCnt
        || col + colSpan > colCnt) {
		qWarning() << "QwtFigureLayout::getGridRect Invalid grid parameters";
		return QRectF(0, 0, 1, 1);  // Return default full size
	}

	// Calculate cell dimensions without considering margins
	// 不考虑边距计算单元格尺寸
	const qreal totalWidth  = 1.0;
	const qreal totalHeight = 1.0;

	// Calculate cell dimensions
	const qreal availableWidth  = totalWidth - (colCnt - 1) * wspace;
	const qreal availableHeight = totalHeight - (rowCnt - 1) * hspace;

	if (availableWidth <= 0 || availableHeight <= 0) {
		qWarning() << "Not enough space for grid cells after applying spacing";
		return QRectF(0, 0, 1, 1);  // Return default full size
	}

	const qreal cellWidth  = availableWidth / colCnt;
	const qreal cellHeight = availableHeight / rowCnt;

	// Calculate position in normalized coordinates using Qt's top-left coordinate system
	QRectF rect;
	rect.setLeft(col * (cellWidth + wspace));
	rect.setTop(row * (cellHeight + hspace));
	rect.setWidth(colSpan * cellWidth + (colSpan - 1) * wspace);
	rect.setHeight(rowSpan * cellHeight + (rowSpan - 1) * hspace);

	// Ensure the rect is within valid bounds
	rect.setLeft(qMax(0.0, rect.left()));
	rect.setTop(qMax(0.0, rect.top()));
	rect.setWidth(qMin(rect.width(), 1.0 - rect.left()));
	rect.setHeight(qMin(rect.height(), 1.0 - rect.top()));

	return rect;
}
