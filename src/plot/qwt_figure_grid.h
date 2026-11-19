/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef QWT_FIGURE_GRID_H
#define QWT_FIGURE_GRID_H

#include <QList>
#include <QObject>
#include <QRectF>
#include <QVector>

#include "qwt_global.h"

class QWidget;

/**
 * @class QwtFigureGrid
 *
 * @brief Retained grid track model for a figure with several members
 *
 * QwtFigureGrid owns the authoritative track ratios of the columns/rows and
 * the (row, column, rowSpan, columnSpan) registration of every member widget.
 * Normalized rectangles are computed on demand from the retained tracks, so
 * the grid can be resized interactively without touching the registrations.
 *
 * The model only depends on Qt (QObject/QWidget) and never references
 * QwtFigure or QwtFigureLayout, which keeps the geometry math testable in
 * isolation.
 *
 * @code
 * QwtFigureGrid* grid = new QwtFigureGrid(figure);
 * grid->setup(2, 2);                           // enable a 2x2 grid
 * grid->setColumnSizes({ 0.6, 0.4 });          // column ratios
 * grid->addItem(plot, 0, 0, 1, 2);             // plot spans the top row
 * const QRectF cell = grid->normRectFor(1, 0); // normalized geometry of a cell
 * @endcode
 */
class QWT_EXPORT QwtFigureGrid : public QObject
{
    Q_OBJECT
    QWT_DECLARE_PRIVATE(QwtFigureGrid)

public:
    /// Constructor
    explicit QwtFigureGrid(QObject* parent = nullptr);
    /// Destructor
    ~QwtFigureGrid() override;

    bool isEnabled() const;                     // True once setup() succeeded
    void setup(int rowCount, int columnCount);  // Enable the grid and init equal tracks
    void disable();                             // Disable the grid and drop the member registrations
    int rowCount() const;                       // Number of row tracks (0 when disabled)
    int columnCount() const;                    // Number of column tracks (0 when disabled)

    QVector< qreal > rowSizes() const;                   // Normalized row ratios, sum == 1
    QVector< qreal > columnSizes() const;                // Normalized column ratios, sum == 1
    void setRowSizes(const QVector< qreal >& sizes);     // Set the row ratios (normalized on input)
    void setColumnSizes(const QVector< qreal >& sizes);  // Set the column ratios (normalized on input)
    void setSpacing(qreal wspace, qreal hspace);         // Gap between tracks, normalized
    qreal wspace() const;                                // Horizontal gap between columns, normalized
    qreal hspace() const;                                // Vertical gap between rows, normalized
    void setMinimumTrackPixels(int px);                  // Minimum track extent while dragging
    int minimumTrackPixels() const;                      // Minimum track extent while dragging

    bool addItem(QWidget* w, int row, int col, int rowSpan = 1, int colSpan = 1);         // Register a member
    bool removeItem(QWidget* w);                                                          // Unregister a member
    bool gridPosition(QWidget* w, int* row, int* col, int* rowSpan, int* colSpan) const;  // Query a member
    QList< QWidget* > items() const;                                                      // Registered members

    QRectF normRectFor(int row, int col, int rowSpan = 1, int colSpan = 1) const;  // Normalized cell rect

    /**
     * @brief A draggable divider between two adjacent tracks
     *
     * A Horizontal divider is the vertical line between two columns (it changes
     * the column sizes and stores a normalized X position). A Vertical divider
     * is the horizontal line between two rows (it changes the row sizes and
     * stores a normalized Y position).
     */
    struct Divider
    {
        Qt::Orientation orientation;  ///< Horizontal = column divider, Vertical = row divider
        int index;                    ///< Left/top track index (0 .. count - 2)
        qreal position;               ///< Normalized position of the gap center
    };
    QList< Divider > dividers() const;  // Divider list for the current tracks

    void finishChange();  // Emit sizesChangeFinished() when an interactive change session ends

Q_SIGNALS:
    /**
     * @brief Emitted whenever the track ratios or the spacing change
     *
     * Also emitted repeatedly while a divider is being dragged.
     */
    void sizesChanged();

    /**
     * @brief Emitted once when an interactive change session ends
     *
     * The splitter calls finishChange() on mouse release.
     */
    void sizesChangeFinished();
};

#endif  // QWT_FIGURE_GRID_H
