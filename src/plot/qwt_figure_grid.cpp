/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "qwt_figure_grid.h"

#include <QDebug>
#include <QPointer>
#include <QWidget>

#include <cmath>

namespace
{
// Absolute tolerance used for normalized track comparisons
const qreal SIZE_EPSILON = 1e-12;

// Sum of 'count' track sizes starting at 'first'
qreal sumSizes(const QVector< qreal >& sizes, int first, int count)
{
    qreal sum = 0.0;
    for (int i = 0; i < count; ++i) {
        sum += sizes.at(first + i);
    }
    return sum;
}

// A valid track vector needs exactly 'trackCount' finite, strictly positive values
bool isValidSizeVector(const QVector< qreal >& sizes, int trackCount)
{
    if (trackCount <= 0 || sizes.size() != trackCount) {
        return false;
    }
    for (qreal size : sizes) {
        if (!std::isfinite(size) || size <= 0.0) {
            return false;
        }
    }
    return true;
}

// Scale the ratios so that they sum up to 1, preserving the caller's intent
QVector< qreal > normalizedSizes(const QVector< qreal >& sizes)
{
    qreal sum = 0.0;
    for (qreal size : sizes) {
        sum += size;
    }

    QVector< qreal > normalized;
    normalized.reserve(sizes.size());
    for (qreal size : sizes) {
        normalized.append(size / sum);
    }
    return normalized;
}

bool sameSizes(const QVector< qreal >& lhs, const QVector< qreal >& rhs)
{
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (int i = 0; i < lhs.size(); ++i) {
        if (qAbs(lhs.at(i) - rhs.at(i)) > SIZE_EPSILON) {
            return false;
        }
    }
    return true;
}
}  // namespace

class QwtFigureGrid::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtFigureGrid)
public:
    PrivateData(QwtFigureGrid* p) : q_ptr(p)
    {
    }

    // Registration of one member widget
    struct Item
    {
        QPointer< QWidget > widget;
        int row { 0 };
        int col { 0 };
        int rowSpan { 1 };
        int colSpan { 1 };
    };

    // Index of the registration for w, -1 when not registered
    int indexOf(const QWidget* w) const;
    // Drop all registrations, including their destroyed() connections
    void clearItems();
    // True when the position fits the current track counts
    bool containsPosition(int row, int col, int rowSpan, int colSpan) const;

public:
    bool m_enabled { false };
    int m_rowCount { 0 };
    int m_columnCount { 0 };
    QVector< qreal > m_rowSizes;
    QVector< qreal > m_columnSizes;
    qreal m_wspace { 0.0 };
    qreal m_hspace { 0.0 };
    int m_minimumTrackPixels { 40 };
    QList< Item > m_items;
};

int QwtFigureGrid::PrivateData::indexOf(const QWidget* w) const
{
    if (!w) {
        return -1;
    }
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).widget.data() == w) {
            return i;
        }
    }
    return -1;
}

void QwtFigureGrid::PrivateData::clearItems()
{
    for (const Item& item : qwt_as_const(m_items)) {
        if (item.widget) {
            QObject::disconnect(item.widget.data(), nullptr, q_func(), nullptr);
        }
    }
    m_items.clear();
}

bool QwtFigureGrid::PrivateData::containsPosition(int row, int col, int rowSpan, int colSpan) const
{
    if (!m_enabled || row < 0 || col < 0 || rowSpan <= 0 || colSpan <= 0) {
        return false;
    }
    return (row + rowSpan <= m_rowCount) && (col + colSpan <= m_columnCount);
}

/**
 * @brief Constructor
 * @param parent Parent object
 */
QwtFigureGrid::QwtFigureGrid(QObject* parent) : QObject(parent), QWT_PIMPL_CONSTRUCT
{
}

/**
 * @brief Destructor
 *
 * Disconnects the destroyed() connections of all registered widgets.
 */
QwtFigureGrid::~QwtFigureGrid()
{
    QWT_D(d);
    d->clearItems();
}

/**
 * @brief Test if the grid is enabled
 * @return True after a successful setup()
 */
bool QwtFigureGrid::isEnabled() const
{
    QWT_DC(d);
    return d->m_enabled;
}

/**
 * @brief Enable the grid and initialize equal tracks
 *
 * All existing member registrations are dropped and the track ratios are reset
 * to 1/n. Calling setup() again re-initializes the model.
 *
 * @param rowCount Number of rows, must be positive
 * @param columnCount Number of columns, must be positive
 */
void QwtFigureGrid::setup(int rowCount, int columnCount)
{
    if (rowCount <= 0 || columnCount <= 0) {
        qWarning() << "QwtFigureGrid::setup: row and column counts must be positive, got" << rowCount << columnCount;
        return;
    }

    QWT_D(d);
    d->clearItems();

    d->m_enabled     = true;
    d->m_rowCount    = rowCount;
    d->m_columnCount = columnCount;
    d->m_rowSizes    = QVector< qreal >(rowCount, 1.0 / rowCount);
    d->m_columnSizes = QVector< qreal >(columnCount, 1.0 / columnCount);

    Q_EMIT sizesChanged();
}

/**
 * @brief Number of row tracks
 * @return Row count, 0 when the grid is disabled
 */
int QwtFigureGrid::rowCount() const
{
    QWT_DC(d);
    return d->m_rowCount;
}

/**
 * @brief Number of column tracks
 * @return Column count, 0 when the grid is disabled
 */
int QwtFigureGrid::columnCount() const
{
    QWT_DC(d);
    return d->m_columnCount;
}

/**
 * @brief Row track ratios
 * @return Normalized ratios, summing up to 1, empty when disabled
 */
QVector< qreal > QwtFigureGrid::rowSizes() const
{
    QWT_DC(d);
    return d->m_rowSizes;
}

/**
 * @brief Column track ratios
 * @return Normalized ratios, summing up to 1, empty when disabled
 */
QVector< qreal > QwtFigureGrid::columnSizes() const
{
    QWT_DC(d);
    return d->m_columnSizes;
}

/**
 * @brief Set the row track ratios
 *
 * The values are normalized to sum 1, preserving the caller's proportions. The
 * request is rejected (with a warning and unchanged state) unless the vector
 * holds one finite, strictly positive value per row track.
 *
 * @param sizes Row ratios, one value per track
 */
void QwtFigureGrid::setRowSizes(const QVector< qreal >& sizes)
{
    QWT_D(d);
    if (!d->m_enabled) {
        qWarning() << "QwtFigureGrid::setRowSizes: grid is not enabled";
        return;
    }
    if (!isValidSizeVector(sizes, d->m_rowCount)) {
        qWarning() << "QwtFigureGrid::setRowSizes: expected" << d->m_rowCount << "finite and positive values, got" << sizes;
        return;
    }

    const QVector< qreal > normalized = normalizedSizes(sizes);
    if (sameSizes(normalized, d->m_rowSizes)) {
        return;
    }
    d->m_rowSizes = normalized;
    Q_EMIT sizesChanged();
}

/**
 * @brief Set the column track ratios
 *
 * The values are normalized to sum 1, preserving the caller's proportions. The
 * request is rejected (with a warning and unchanged state) unless the vector
 * holds one finite, strictly positive value per column track.
 *
 * @param sizes Column ratios, one value per track
 */
void QwtFigureGrid::setColumnSizes(const QVector< qreal >& sizes)
{
    QWT_D(d);
    if (!d->m_enabled) {
        qWarning() << "QwtFigureGrid::setColumnSizes: grid is not enabled";
        return;
    }
    if (!isValidSizeVector(sizes, d->m_columnCount)) {
        qWarning() << "QwtFigureGrid::setColumnSizes: expected" << d->m_columnCount << "finite and positive values, got"
                   << sizes;
        return;
    }

    const QVector< qreal > normalized = normalizedSizes(sizes);
    if (sameSizes(normalized, d->m_columnSizes)) {
        return;
    }
    d->m_columnSizes = normalized;
    Q_EMIT sizesChanged();
}

/**
 * @brief Set the gap between adjacent tracks
 *
 * Both values are normalized (the same unit as the track ratios). Negative or
 * non-finite values are rejected with a warning and the state is left unchanged.
 *
 * @param wspace Horizontal gap between columns
 * @param hspace Vertical gap between rows
 */
void QwtFigureGrid::setSpacing(qreal wspace, qreal hspace)
{
    QWT_D(d);
    if (!std::isfinite(wspace) || !std::isfinite(hspace) || wspace < 0.0 || hspace < 0.0) {
        qWarning() << "QwtFigureGrid::setSpacing: spacing must be finite and non-negative, got" << wspace << hspace;
        return;
    }

    if (qAbs(d->m_wspace - wspace) <= SIZE_EPSILON && qAbs(d->m_hspace - hspace) <= SIZE_EPSILON) {
        return;
    }
    d->m_wspace = wspace;
    d->m_hspace = hspace;
    Q_EMIT sizesChanged();
}

/**
 * @brief Horizontal gap between adjacent columns
 * @return The normalized gap set by setSpacing(), 0 by default
 */
qreal QwtFigureGrid::wspace() const
{
    QWT_DC(d);
    return d->m_wspace;
}

/**
 * @brief Vertical gap between adjacent rows
 * @return The normalized gap set by setSpacing(), 0 by default
 */
qreal QwtFigureGrid::hspace() const
{
    QWT_DC(d);
    return d->m_hspace;
}

/**
 * @brief Set the minimum extent of a track in pixels
 *
 * Used by the splitter to clamp a drag so that no track collapses completely.
 *
 * @param px Minimum extent in pixels, negative values are ignored
 */
void QwtFigureGrid::setMinimumTrackPixels(int px)
{
    if (px < 0) {
        qWarning() << "QwtFigureGrid::setMinimumTrackPixels: the minimum must be non-negative, got" << px;
        return;
    }
    QWT_D(d);
    d->m_minimumTrackPixels = px;
}

/**
 * @brief Minimum extent of a track in pixels
 * @return Minimum track extent, 40 by default
 */
int QwtFigureGrid::minimumTrackPixels() const
{
    QWT_DC(d);
    return d->m_minimumTrackPixels;
}

/**
 * @brief Register a member widget with its grid position
 *
 * The registration is dropped automatically when the widget is destroyed. A
 * widget can only be registered once.
 *
 * @param w Member widget
 * @param row Grid row of the top-left cell (0-based)
 * @param col Grid column of the top-left cell (0-based)
 * @param rowSpan Number of rows covered by the member
 * @param colSpan Number of columns covered by the member
 * @return True when the widget was registered
 */
bool QwtFigureGrid::addItem(QWidget* w, int row, int col, int rowSpan, int colSpan)
{
    QWT_D(d);
    if (!w) {
        qWarning() << "QwtFigureGrid::addItem: null widget";
        return false;
    }
    if (!d->m_enabled) {
        qWarning() << "QwtFigureGrid::addItem: grid is not enabled";
        return false;
    }
    if (!d->containsPosition(row, col, rowSpan, colSpan)) {
        qWarning() << "QwtFigureGrid::addItem: position" << row << col << rowSpan << colSpan << "does not fit the grid"
                   << d->m_rowCount << "x" << d->m_columnCount;
        return false;
    }
    if (d->indexOf(w) >= 0) {
        qWarning() << "QwtFigureGrid::addItem: widget is already registered";
        return false;
    }

    PrivateData::Item item;
    item.widget  = w;
    item.row     = row;
    item.col     = col;
    item.rowSpan = rowSpan;
    item.colSpan = colSpan;
    d->m_items.append(item);

    // Drop the registration when the widget dies
    QObject::connect(w, &QObject::destroyed, this, [ this ](QObject* obj) {
        QWT_D(d);
        for (int i = d->m_items.size() - 1; i >= 0; --i) {
            QWidget* registered = d->m_items.at(i).widget.data();
            if (!registered || registered == obj) {
                d->m_items.removeAt(i);
            }
        }
    });

    return true;
}

/**
 * @brief Unregister a member widget
 * @param w Member widget
 * @return True when the widget was registered and has been removed
 */
bool QwtFigureGrid::removeItem(QWidget* w)
{
    if (!w) {
        return false;
    }

    QWT_D(d);
    const int index = d->indexOf(w);
    if (index < 0) {
        return false;
    }

    QObject::disconnect(w, nullptr, this, nullptr);
    d->m_items.removeAt(index);
    return true;
}

/**
 * @brief Query the grid position of a registered member
 *
 * The output pointers are only written when the widget is registered, and may
 * be nullptr when a value is not needed.
 *
 * @param w Member widget
 * @param[out] row Grid row of the top-left cell
 * @param[out] col Grid column of the top-left cell
 * @param[out] rowSpan Number of rows covered by the member
 * @param[out] colSpan Number of columns covered by the member
 * @return True when the widget is registered
 */
bool QwtFigureGrid::gridPosition(QWidget* w, int* row, int* col, int* rowSpan, int* colSpan) const
{
    if (!w) {
        return false;
    }

    QWT_DC(d);
    const int index = d->indexOf(w);
    if (index < 0) {
        return false;
    }

    const PrivateData::Item& item = d->m_items.at(index);
    if (row) {
        *row = item.row;
    }
    if (col) {
        *col = item.col;
    }
    if (rowSpan) {
        *rowSpan = item.rowSpan;
    }
    if (colSpan) {
        *colSpan = item.colSpan;
    }
    return true;
}

/**
 * @brief List the registered member widgets
 * @return Registered widgets, dangling pointers are skipped
 */
QList< QWidget* > QwtFigureGrid::items() const
{
    QWT_DC(d);
    QList< QWidget* > result;
    result.reserve(d->m_items.size());
    for (const PrivateData::Item& item : qwt_as_const(d->m_items)) {
        if (item.widget) {
            result.append(item.widget.data());
        }
    }
    return result;
}

/**
 * @brief Calculate the normalized rectangle of a grid area
 *
 * @details
 * left = sum(columnSizes[0..col-1]) + col * wspace
 * width = sum(columnSizes[col..col+colSpan-1]) + (colSpan - 1) * wspace
 *
 * The row direction is computed the same way with hspace. Left/top are clamped
 * to [0,1] and width/height so that the right/bottom edge stays within 1; a
 * positive spacing therefore narrows the last track after clamping, which is
 * expected.
 *
 * @param row Grid row of the top-left cell (0-based)
 * @param col Grid column of the top-left cell (0-based)
 * @param rowSpan Number of rows covered
 * @param colSpan Number of columns covered
 * @return Normalized rectangle in [0,1], an invalid rectangle on bad input
 */
QRectF QwtFigureGrid::normRectFor(int row, int col, int rowSpan, int colSpan) const
{
    QWT_DC(d);
    if (!d->containsPosition(row, col, rowSpan, colSpan)) {
        qWarning() << "QwtFigureGrid::normRectFor: invalid grid position" << row << col << rowSpan << colSpan;
        return QRectF();
    }

    qreal left         = sumSizes(d->m_columnSizes, 0, col) + col * d->m_wspace;
    const qreal width  = sumSizes(d->m_columnSizes, col, colSpan) + (colSpan - 1) * d->m_wspace;
    qreal top          = sumSizes(d->m_rowSizes, 0, row) + row * d->m_hspace;
    const qreal height = sumSizes(d->m_rowSizes, row, rowSpan) + (rowSpan - 1) * d->m_hspace;

    left = qBound(0.0, left, 1.0);
    top  = qBound(0.0, top, 1.0);

    const qreal clampedWidth  = qBound(0.0, width, 1.0 - left);
    const qreal clampedHeight = qBound(0.0, height, 1.0 - top);

    return QRectF(left, top, clampedWidth, clampedHeight);
}

/**
 * @brief List the draggable dividers between adjacent tracks
 *
 * One Horizontal divider per column gap (positions along X) and one Vertical
 * divider per row gap (positions along Y). The position is the normalized
 * center of the gap: sum(sizes[0..i]) + i * spacing + spacing / 2.
 *
 * @return Dividers, empty when the grid is disabled or has a single track
 */
QList< QwtFigureGrid::Divider > QwtFigureGrid::dividers() const
{
    QWT_DC(d);
    QList< Divider > result;
    if (!d->m_enabled) {
        return result;
    }

    for (int i = 0; i < d->m_columnCount - 1; ++i) {
        Divider divider;
        divider.orientation = Qt::Horizontal;
        divider.index       = i;
        divider.position    = sumSizes(d->m_columnSizes, 0, i + 1) + i * d->m_wspace + d->m_wspace / 2.0;
        result.append(divider);
    }
    for (int i = 0; i < d->m_rowCount - 1; ++i) {
        Divider divider;
        divider.orientation = Qt::Vertical;
        divider.index       = i;
        divider.position    = sumSizes(d->m_rowSizes, 0, i + 1) + i * d->m_hspace + d->m_hspace / 2.0;
        result.append(divider);
    }
    return result;
}

/**
 * @brief Emit sizesChangeFinished() when an interactive change session ends
 *
 * The splitter calls this on mouse release, so undo logic in the DA layer can
 * push a single command for the whole drag.
 */
void QwtFigureGrid::finishChange()
{
    Q_EMIT sizesChangeFinished();
}
