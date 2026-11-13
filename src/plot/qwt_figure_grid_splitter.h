/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef QWT_FIGURE_GRID_SPLITTER_H
#define QWT_FIGURE_GRID_SPLITTER_H

// Qt
#include <QObject>

// qwt
#include "qwt_global.h"

class QEvent;
class QwtFigure;

/**
 * @class QwtFigureGridSplitter
 *
 * @brief Interactive divider of the retained grid layout of a QwtFigure
 *
 * @details
 * The splitter installs event filters on the figure and on the widgets that can
 * receive the mouse over the grid cells. Only events inside a narrow band around
 * a divider are consumed, every other event is passed through unchanged so
 * zooming, panning, picking and hovering of the plots keep working.
 *
 * Dragging a divider changes the ratio of the two adjacent tracks only: a
 * Horizontal divider sits between two columns (the mouse moves along X), a
 * Vertical divider sits between two rows (the mouse moves along Y). The track
 * ratios are clamped by QwtFigureGrid::minimumTrackPixels(), so no track can
 * collapse and the tracks can never overlap.
 *
 * While a divider is dragged the member widgets stop repainting; releasing the
 * mouse restores the repaints and emits QwtFigureGrid::sizesChangeFinished()
 * once, but only when the ratios really changed. Pressing Esc cancels the drag:
 * the original ratios are restored and no finished signal is emitted.
 *
 * QOpenGLWidget members (for example a 3D plot) never get an event filter, so
 * their events are not intercepted. The boundary of a 3D cell is served by the
 * adjacent 2D plot.
 *
 * @par Example:
 * @code
 * QwtFigure* figure = new QwtFigure;
 * figure->enableGrid(2, 2);
 * figure->addAxes(wavePlot, 0, 0, 1, 2);
 * figure->addAxes(spectrumPlot, 1, 0);
 * figure->addAxes(heatmapPlot, 1, 1);
 *
 * // The splitter is owned by the figure, enableGrid() must be called first
 * figure->setGridEditable(true);
 * @endcode
 *
 * @sa QwtFigure::setGridEditable(), QwtFigureGrid
 */
class QWT_EXPORT QwtFigureGridSplitter : public QObject
{
    Q_OBJECT
    QWT_DECLARE_PRIVATE(QwtFigureGridSplitter)

public:
    // Constructor, the figure must not be null and becomes the parent
    explicit QwtFigureGridSplitter(QwtFigure* figure);
    ~QwtFigureGridSplitter() override;

    // The figure the splitter works on
    QwtFigure* figure() const;

    // Enable or disable the divider interaction
    void setEnabled(bool on);
    bool isEnabled() const;

    // Set the half width of the hit band around a divider in pixels
    void setHitWidth(int px);
    int hitWidth() const;

    // True while a divider is dragged
    bool isDragging() const;

Q_SIGNALS:
    /**
     * @brief Emitted when the cursor shape over the divider bands changes
     * @param shape SplitHCursor/SplitVCursor inside a band, ArrowCursor when
     *              the mouse left the band
     */
    void hoverChanged(Qt::CursorShape shape);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif  // QWT_FIGURE_GRID_SPLITTER_H
