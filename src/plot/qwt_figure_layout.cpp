/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "qwt_figure_layout.h"
#include <QMap>
#include <QDebug>
#include <QWidget>

#include "qwt_figure.h"
#include "qwt_figure_grid.h"

#ifndef QWTFIGURELAYOUT_DEBUG_PRINT
#define QWTFIGURELAYOUT_DEBUG_PRINT 0
#endif

// Returns the retained grid of the figure owning this layout (nullptr if none).
static QwtFigureGrid* figureGridOfLayout(const QLayout* lay)
{
    if (!lay) {
        return nullptr;
    }
    QwtFigure* figure = qobject_cast< QwtFigure* >(lay->parentWidget());
    return figure ? figure->grid() : nullptr;
}

// Returns true if the widget is registered as a member of the retained grid.
static bool gridPositionOf(const QwtFigureGrid* grid, QWidget* widget, QRectF* normRect)
{
    if (!grid || !grid->isEnabled()) {
        return false;
    }
    int row = 0, col = 0, rowSpan = 0, colSpan = 0;
    if (!grid->gridPosition(widget, &row, &col, &rowSpan, &colSpan)) {
        return false;
    }
    const QRectF rect = grid->normRectFor(row, col, rowSpan, colSpan);
    if (!rect.isValid()) {
        return false;
    }
    if (normRect) {
        *normRect = rect;
    }
    return true;
}

class QwtFigureLayout::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtFigureLayout)
public:
    PrivateData(QwtFigureLayout* p) : q_ptr(p)
    {
    }

    /**
     * @brief Item wrapper containing layout information
     */
    struct LayoutItem
    {
        QLayoutItem* item { nullptr };  ///< Pointer to the layout item
        QRectF normRect;                ///< Normalized coordinates [0,1]
    };

public:
    QList< LayoutItem > m_items;  ///< List of layout items
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
    li.normRect = QRectF(0, 0, 1, 1);  // Default full size
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
    for (const auto& item : qwt_as_const(m_data->m_items)) {
        if (item.item && item.item->widget())
            size = size.expandedTo(item.item->minimumSize());
    }
    return size;
}

void QwtFigureLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);
    // Skip layout if geometry is invalid
    if (!rect.isValid()) {
        return;
    }
#if QWTFIGURELAYOUT_DEBUG_PRINT && QWT_DEBUG_DRAW
    qDebug() << "QwtFigureLayout setGeometry(rect=" << rect << "),left=" << m_data->m_left
             << ",right=" << m_data->m_right << ",bottom=" << m_data->m_bottom << ",top=" << m_data->m_top;
#endif
    QwtFigureGrid* figureGrid = figureGridOfLayout(this);
    for (const auto& item : qwt_as_const(m_data->m_items)) {
        QWidget* widget = item.item ? item.item->widget() : nullptr;
        if (!widget || !widget->isVisibleTo(widget->parentWidget())) {
            continue;
        }
        // Convert normalized coordinates to actual pixels using Qt's top-left coordinate system.
        // Apply margins to both grid-based and normalized coordinate-based items.
        // Members of the retained grid are laid out from the current track sizes,
        // free members keep the normalized rectangle they were added with.
        QRectF normRect = item.normRect;
        gridPositionOf(figureGrid, widget, &normRect);

        QRect actualRect = calcActualRect(rect, normRect);
        item.item->setGeometry(actualRect);

#if QWTFIGURELAYOUT_DEBUG_PRINT && QWT_DEBUG_DRAW
        qDebug() << "normRect=" << normRect << ",actualRect=" << actualRect;
#endif
    }

    /*
       The members have their new geometry now: refresh the plot area alignment
       of the figure. The pass measures the natural insets of the plots from the
       current member size, so it has to run after the geometries were applied.
     */
    if (QwtFigure* figure = qobject_cast< QwtFigure* >(parentWidget())) {
        figure->applyAllAxisAlignments(false);
    }
}

/**
 * @brief Add a widget with normalized coordinates
 *
 * This method adds a widget to the layout using normalized coordinates in the range [0,1].
 * The coordinates are specified as [left, top, width, height], where:
 * - left: distance from the left edge of the figure
 * - top: distance from the top edge of the figure
 * - width: width of the widget
 * - height: height of the widget
 *
 * @param widget Widget to add
 * @param rect Normalized coordinates [left, top, width, height] in range [0,1]
 *
 * @note All coordinates must be in the [0,1] range. left+width should not exceed 1, and top+height should not exceed 1.
 *
 * @code
 * // Add a widget that occupies the top-left quarter of the figure
 * QWidget* widget = new QWidget;
 * layout->addAxes(widget, QRectF(0.0, 0.0, 0.5, 0.5));
 * @endcode
 *
 * @code
 * // Add a widget that occupies the bottom-right quarter of the figure
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
 * @brief Add a widget with normalized coordinates using separate parameters
 *
 * This is a convenience overload that adds a widget to the layout using normalized coordinates
 * in the range [0,1] with separate left, top, width, and height parameters.
 * The coordinates use Qt's standard top-left coordinate system.
 *
 * @param widget Widget to add
 * @param left Normalized distance from the left edge [0,1]
 * @param top Normalized distance from the top edge [0,1]
 * @param width Normalized width of the widget [0,1]
 * @param height Normalized height of the widget [0,1]
 *
 * @note All parameters must be in the range [0,1]. The sum of left + width should not exceed 1,
 *       and the sum of top + height should not exceed 1.
 *
 * @code
 * // Add a widget that occupies the top-left quarter of the figure
 * QWidget* widget = new QWidget;
 * layout->addAxes(widget, 0.0, 0.0, 0.5, 0.5);
 * @endcode
 *
 * @code
 * // Add a widget that occupies the center of the figure
 * QWidget* widget = new QWidget;
 * layout->addAxes(widget, 0.25, 0.25, 0.5, 0.5);
 * @endcode
 */
void QwtFigureLayout::addAxes(QWidget* widget, qreal left, qreal top, qreal width, qreal height)
{
    addAxes(widget, QRectF(left, top, width, height));
}

/**
 * @if ENGLISH
 * @brief Add a widget by grid layout
 *
 * This method adds a widget to the grid layout at the specified position with optional row and column spans.
 * The grid position is 0-based, with (0,0) being the top-left cell of the grid.
 * The normalized coordinates are calculated immediately and stored with the widget.
 *
 * @param[in] widget Widget to add
 * @param[in] rowCnt Total number of rows in the grid
 * @param[in] colCnt Total number of columns in the grid
 * @param[in] row Grid row position (0-based)
 * @param[in] col Grid column position (0-based)
 * @param[in] rowSpan Number of rows to span (default: 1)
 * @param[in] colSpan Number of columns to span (default: 1)
 * @param[in] wspace Horizontal space between subplots [0,1]
 * @param[in] hspace Vertical space between subplots [0,1]
 *
 * @code
 * // Create a 2x2 grid and add widgets
 * //
 * // Grid layout visualization (2x2):
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
 * QWidget* topWidget = new QWidget;
 * layout->addAxes(topWidget, 2, 2, 0, 0, 1, 2);
 * //
 * // After adding topWidget:
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
 * QWidget* bottomLeftWidget = new QWidget;
 * layout->addAxes(bottomLeftWidget, 2, 2, 1, 0);
 * //
 * // After adding bottomLeftWidget:
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
 * QWidget* bottomRightWidget = new QWidget;
 * layout->addAxes(bottomRightWidget, 2, 2, 1, 1);
 * //
 * // Final layout:
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
void QwtFigureLayout::addGridAxes(QWidget* widget,
                                  int rowCnt,
                                  int colCnt,
                                  int row,
                                  int col,
                                  int rowSpan,
                                  int colSpan,
                                  qreal wspace,
                                  qreal hspace)
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
 * @if ENGLISH
 * @brief Change the normalized position of an already added widget
 *
 * @note This function does not automatically refresh the widget position; manual refresh is required.
 *
 * @param widget Widget whose position to change
 * @param rect New normalized coordinates [left, top, width, height] in range [0,1]
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
 * @if ENGLISH
 * @brief Get the normalized rectangle for a widget
 *
 * This method returns the normalized coordinates [0,1] for the specified widget
 * in the layout. If the widget is not found in the layout, an invalid QRectF is returned.
 *
 * @param[in] widget Widget to query
 * @return Normalized coordinates [left, top, width, height] in range [0,1], or invalid QRectF if not found
 *
 * @code
 * // Get the normalized position of a widget
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

    QRectF gridRect;
    if (gridPositionOf(figureGridOfLayout(this), widget, &gridRect)) {
        return gridRect;
    }

    for (const auto& item : qwt_as_const(m_data->m_items)) {
        if (item.item && item.item->widget() == widget) {
            return item.normRect;
        }
    }
    return QRectF();  // Return invalid rect
}

/**
 * @if ENGLISH
 * @brief Calculate normalized coordinates of rect relative to parentRect
 *
 * @param[in] parentRect Parent rectangle (pixel coordinates)
 * @param rect Child rectangle (pixel coordinates, relative to parentRect)
 * @return Normalized coordinates QRectF (left, top, width, height all in [0,1] range)
 */
QRectF QwtFigureLayout::calcNormRect(const QRect& parentRect, const QRect& rect)
{
    // Handle edge case where parent rectangle is empty (avoid division by zero)
    if (parentRect.isEmpty()) {
        return QRectF();
    }

    // Extract parent rectangle dimensions (ensure positive values to avoid edge cases)
    const int parentWidth  = qMax(parentRect.width(), 1);  // Minimum of 1 to avoid division by zero
    const int parentHeight = qMax(parentRect.height(), 1);

    // Calculate normalized coordinates (use double for precision)
    const double left   = static_cast< double >(rect.x()) / parentWidth;
    const double top    = static_cast< double >(rect.y()) / parentHeight;
    const double width  = static_cast< double >(rect.width()) / parentWidth;
    const double height = static_cast< double >(rect.height()) / parentHeight;

    // Optimize precision: round to 6 decimal places (balancing precision and floating-point representation stability)
    const double precision = 1e-6;
    auto roundToPrecision  = [ precision ](double value) { return qRound(value / precision) * precision; };

    // Ensure normalized coordinates are within [0,1] range (handle possible boundary errors)
    const double clampedLeft   = qBound(0.0, roundToPrecision(left), 1.0);
    const double clampedTop    = qBound(0.0, roundToPrecision(top), 1.0);
    const double clampedWidth  = qBound(0.0, roundToPrecision(width), 1.0 - clampedLeft);
    const double clampedHeight = qBound(0.0, roundToPrecision(height), 1.0 - clampedTop);

    return QRectF(clampedLeft, clampedTop, clampedWidth, clampedHeight);
}

/**
 * @if ENGLISH
 * @brief Calculate actual rectangle from normalized coordinates
 *
 * Both edges are rounded from the accumulated normalized position
 * ( left and left + width ), and the pixel extent is derived from the two
 * rounded edges instead of being rounded on its own. Neighbouring cells that
 * share an edge therefore land on exactly the same pixel: the right edge of a
 * cell equals the left edge of the next one and no one pixel seam appears
 * between them.
 *
 * @param parentRect Parent window size
 * @param normRect Normalized rectangle
 * @return Actual rectangle in pixel coordinates, always inside parentRect
 */
QRect QwtFigureLayout::calcActualRect(const QRect& parentRect, const QRectF& normRect)
{
    const int availableWidth  = parentRect.width();
    const int availableHeight = parentRect.height();

    // Cumulative sum rounding: round the two edges of the interval, not the
    // normalized origin and extent independently
    const int left   = qRound(normRect.left() * availableWidth);
    const int top    = qRound(normRect.top() * availableHeight);
    const int right  = qRound((normRect.left() + normRect.width()) * availableWidth);
    const int bottom = qRound((normRect.top() + normRect.height()) * availableHeight);

    // Ensure the rect is within valid bounds
    const int x      = qMax(0, left);
    const int y      = qMax(0, top);
    const int width  = qMax(0, qMin(right, availableWidth) - x);
    const int height = qMax(0, qMin(bottom, availableHeight) - y);
    return QRect(x, y, width, height);
}

/**
 * @brief Calculate the normalized rectangle for a grid cell
 *
 * This method calculates the normalized coordinates for a specific grid cell
 * based on the current layout parameters and grid configuration.
 *
 * @param rowCnt Total number of rows in the grid
 * @param colCnt Total number of columns in the grid
 * @param row Grid row position (0-based)
 * @param col Grid column position (0-based)
 * @param rowSpan Number of rows to span (default: 1)
 * @param colSpan Number of columns to span (default: 1)
 * @return Normalized coordinates [left, top, width, height] in range [0,1]
 *
 * @code
 * // Get the normalized rectangle for the top-left cell in a 2x2 grid
 * QRectF rect = layout->calcGridRect(2, 2, 0, 0);
 * @endcode
 *
 * @code
 * // Get the normalized rectangle for a cell spanning two columns
 * QRectF rect = layout->calcGridRect(3, 3, 1, 0, 1, 2);
 * @endcode
 */
QRectF
QwtFigureLayout::calcGridRect(int rowCnt, int colCnt, int row, int col, int rowSpan, int colSpan, qreal wspace, qreal hspace) const
{
    if (rowCnt <= 0 || colCnt <= 0 || row < 0 || col < 0 || rowSpan <= 0 || colSpan <= 0 || row + rowSpan > rowCnt
        || col + colSpan > colCnt) {
        qWarning() << "QwtFigureLayout::getGridRect Invalid grid parameters";
        return QRectF(0, 0, 1, 1);  // Return default full size
    }

    // Calculate cell dimensions without considering margins
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
