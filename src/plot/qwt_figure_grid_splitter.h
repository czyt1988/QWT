/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef QWT_FIGURE_GRID_SPLITTER_H
#define QWT_FIGURE_GRID_SPLITTER_H

// Qt
#include <QObject>
#include <QPen>

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
 * A drag can be previewed in two ways ( DragPreviewMode ):
 *  - BoundaryLine ( default ): the tracks keep their size while the mouse is
 *    down. A virtual boundary line follows the divider and the new ratios are
 *    applied exactly once, when the mouse is released. The members are not
 *    relaid out or repainted during the drag, which is the cheapest option for
 *    figures with expensive members ( rasters, spectrograms, OpenGL ).
 *  - Live: the ratios are applied on every mouse move, the figure is relaid
 *    out synchronously and the members are replotted, so the plots really
 *    follow the divider. This costs a full layout and replot per mouse move.
 *
 * The boundary line is a thin strip above the members; its pen is
 * configurable with setBoundaryLinePen(). Releasing the mouse emits
 * QwtFigureGrid::sizesChangeFinished() once, but only when the ratios really
 * changed. Pressing Esc cancels the drag: the original ratios are restored
 * (Live) or simply never applied (BoundaryLine) and no finished signal is
 * emitted.
 *
 * The drag grabs the mouse on the figure, so the release always returns to the
 * splitter even when a filter installed after this one (Qt invokes the event
 * filters in reverse installation order) consumes the move/release of the
 * canvas. When the grab is lost (QEvent::UngrabMouse) or the window is
 * deactivated, the drag ends as if Esc was pressed.
 *
 * The split cursor is applied to the widget the mouse is really over (the
 * canvas of a plot, not the plot itself), together with its original
 * WA_SetCursor state, so it stays visible even when the canvas has a cursor
 * of its own (an explicitly set child cursor overrides the inherited one) and
 * the original cursor is restored when the band is left.
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
 *
 * // Optional: show the plots live while the divider is dragged instead of
 * // the default virtual boundary line
 * if (QwtFigureGridSplitter* splitter = figure->findChild< QwtFigureGridSplitter* >())
 *     splitter->setDragPreviewMode(QwtFigureGridSplitter::Live);
 * @endcode
 *
 * @sa QwtFigure::setGridEditable(), QwtFigureGrid
 */
class QWT_EXPORT QwtFigureGridSplitter : public QObject
{
    Q_OBJECT
    QWT_DECLARE_PRIVATE(QwtFigureGridSplitter)

public:
    /**
     * @brief How the members follow a divider while it is dragged
     */
    enum DragPreviewMode
    {
        /// Only a virtual boundary line follows the divider; the tracks are resized on release (default)
        BoundaryLine = 0,

        /// The tracks are resized, relaid out and replotted on every mouse move
        Live = 1
    };
    Q_ENUM(DragPreviewMode)

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

    // Set how a drag is previewed, a running drag is cancelled first
    void setDragPreviewMode(DragPreviewMode mode);
    DragPreviewMode dragPreviewMode() const;

    // Set the pen of the virtual boundary line (BoundaryLine mode)
    void setBoundaryLinePen(const QPen& pen);
    QPen boundaryLinePen() const;

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
