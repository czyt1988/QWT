/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "qwt_figure_grid_splitter.h"

// Qt
#include <QColor>
#include <QCursor>
#include <QDebug>
#include <QEvent>
#include <QHoverEvent>
#include <QKeyEvent>
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QVector>
#include <QWidget>
#include <QtMath>

// qwt
#include "qwt_figure.h"
#include "qwt_figure_grid.h"
#include "qwt_plot.h"
#include "qwt_qt5qt6_compat.hpp"

namespace
{
// Absolute tolerance used to compare track ratio vectors. The symbols in this
// anonymous namespace are prefixed with SPLITTER_ / sameSplitter to stay
// unique: the amalgamated single-file build concatenates this file with
// qwt_figure_grid.cpp, which has its own SIZE_EPSILON / sameSizes().
const qreal SPLITTER_SIZE_EPSILON = 1e-9;

// Returns true for widgets that render through OpenGL; their events are never intercepted
bool isOpenGLWidget(const QWidget* widget)
{
    return widget && (widget->inherits("QOpenGLWidget") || widget->inherits("QGLWidget"));
}

// Compares two track ratio vectors
bool sameSplitterSizes(const QVector< qreal >& lhs, const QVector< qreal >& rhs)
{
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (int i = 0; i < lhs.size(); ++i) {
        if (qAbs(lhs.at(i) - rhs.at(i)) > SPLITTER_SIZE_EPSILON) {
            return false;
        }
    }
    return true;
}

// Converts the position of a mouse or hover event to figure-local coordinates
QPoint figureLocalPos(QwtFigure* figure, QWidget* watched, QEvent* event)
{
    QPoint pos;
    if (event->type() == QEvent::HoverMove || event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverLeave) {
        pos = qwt::compat::eventPos(static_cast< QHoverEvent* >(event));
    } else {
        pos = qwt::compat::eventPos(static_cast< QMouseEvent* >(event));
    }
    if (figure && watched && watched != figure) {
        pos = watched->mapTo(figure, pos);
    }
    return pos;
}

/**
 * @brief Overlay that draws the virtual boundary line of a divider drag
 *
 * The strip is as thick as the boundary pen and spans the whole figure. It is
 * transparent for mouse events and paints only the line, so the members below
 * stay visible. The splitter moves the strip instead of relayouting the grid
 * while a divider is dragged, which is what makes the boundary preview cheap.
 *
 * The strip is created on the first boundary drag and destroyed when the drag
 * ends, so a figure without a running drag has no extra child widget.
 */
class QwtFigureGridBoundaryLine : public QWidget
{
public:
    explicit QwtFigureGridBoundaryLine(QWidget* parent) : QWidget(parent)
    {
        setObjectName(QStringLiteral("QwtFigureGridBoundaryLine"));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setFocusPolicy(Qt::NoFocus);
        hide();
    }

    // Pen of the line, the thickness of the strip follows its width
    void setLinePen(const QPen& pen)
    {
        if (m_pen == pen) {
            return;
        }
        m_pen = pen;
        if (isVisible()) {
            update();
        }
    }

    QPen linePen() const
    {
        return m_pen;
    }

    // Place the strip at a pixel position along the axis of the divider
    void placeLine(Qt::Orientation dividerOrientation, int axisPos, const QSize& area)
    {
        m_orientation = dividerOrientation;

        const int thickness = qMax(1, qCeil(m_pen.widthF()));
        if (dividerOrientation == Qt::Horizontal) {
            // A divider between two columns is a vertical line
            const int left = qBound(0, axisPos - thickness / 2, qMax(0, area.width() - thickness));
            setGeometry(left, 0, thickness, area.height());
        } else {
            // A divider between two rows is a horizontal line
            const int top = qBound(0, axisPos - thickness / 2, qMax(0, area.height() - thickness));
            setGeometry(0, top, area.width(), thickness);
        }
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        if (m_pen.style() == Qt::NoPen) {
            return;
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(m_pen);

        if (m_orientation == Qt::Horizontal) {
            const qreal x = width() / 2.0;
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
        } else {
            const qreal y = height() / 2.0;
            painter.drawLine(QPointF(0.0, y), QPointF(width(), y));
        }
    }

private:
    Qt::Orientation m_orientation { Qt::Horizontal };
    QPen m_pen { QColor(0, 122, 204), 2, Qt::DashLine };
};
}  // namespace

class QwtFigureGridSplitter::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtFigureGridSplitter)
public:
    // Result of a hit test against the divider bands
    struct Hit
    {
        bool valid { false };
        Qt::Orientation orientation { Qt::Horizontal };
        int index { -1 };
        int distance { 0 };
    };

    // A widget whose mouse tracking was changed while the splitter is enabled
    struct TrackingEntry
    {
        QPointer< QWidget > widget;
        bool wasEnabled { false };
    };

    explicit PrivateData(QwtFigureGridSplitter* p) : q_ptr(p)
    {
    }

    // The retained grid of the figure (nullptr when no grid is enabled)
    QwtFigureGrid* grid() const;
    // Collect the widgets whose events are filtered and prepare mouse tracking
    void refreshWatchedWidgets();
    // Remove the event filters and restore the mouse tracking states
    void clearWatchedWidgets();
    // Filter one widget (and optionally its descendants) and track its mouse
    void addWatchedWidget(QWidget* widget, bool includeDescendants);

    // Nearest divider whose hit band contains the figure-local position
    Hit hitTest(const QPoint& figurePos) const;
    // Direct child of the figure that contains the position (or the figure itself)
    QWidget* cellWidgetAt(const QPoint& figurePos) const;
    // Widget that owns the cursor at the position: the deepest child under
    // the mouse, skipping transparent children (or the figure itself)
    QWidget* cursorTargetAt(const QPoint& figurePos) const;

    // Updates the cursor and the hover signal for a position outside a drag
    bool handleHover(const QPoint& figurePos);
    // Applies the split cursor on a widget, saving its previous cursor
    void applyCursor(Qt::CursorShape shape, QWidget* target);
    // Restores the cursor of the last cursor target without emitting hoverChanged()
    void restoreCursorInternal();
    // Restores the cursor of the last cursor target and emits hoverChanged(ArrowCursor)
    void restoreCursor();

    // Starts a drag session on the given divider
    void startDrag(const Hit& hit, const QPoint& figurePos);
    // Applies the current ratios while dragging
    void dragTo(const QPoint& figurePos);
    // Ends the drag; cancelled restores the original ratios
    void endDrag(bool cancelled);
    // Resets the drag session state without touching the grid
    void clearDragState();

    // True when the running drag previews with a virtual boundary line
    bool usesBoundaryLine() const;
    // Pixel position of the center of the dragged divider for the given ratios
    int dividerPixelPosition(const QVector< qreal >& trackSizes, qreal spacing, int dimension) const;
    // Shows or moves the boundary line of a boundary drag
    void updateBoundaryLine(Qt::Orientation orientation, int axisPos);
    // Hides and destroys the boundary line overlay
    void removeBoundaryLine();
    // Applies the ratios a boundary drag previewed (on release)
    void applyPendingSizes();

public:
    QPointer< QwtFigure > figure;
    bool enabled { false };
    int hitWidth { 4 };

    DragPreviewMode dragMode { BoundaryLine };
    QPen boundaryLinePen { QColor(0, 122, 204), 2, Qt::DashLine };
    QPointer< QwtFigureGridBoundaryLine > boundaryLine;
    int boundaryAxisPos { 0 };

    bool dragging { false };
    bool pressConsumed { false };
    Qt::Orientation dragOrientation { Qt::Horizontal };
    int dragIndex { -1 };
    int dragStartAxisPos { 0 };
    QVector< qreal > originalRowSizes;
    QVector< qreal > originalColumnSizes;
    QVector< qreal > pendingRowSizes;
    QVector< qreal > pendingColumnSizes;
    bool keyboardGrabbed { false };
    bool mouseGrabbed { false };

    QPointer< QWidget > cursorWidget;
    QCursor originalCursor { Qt::ArrowCursor };
    bool cursorWidgetHadSetCursor { false };
    Qt::CursorShape currentCursor { Qt::ArrowCursor };

    QList< QPointer< QWidget > > watchedWidgets;
    QList< TrackingEntry > trackingEntries;
};

//----------------------------------------------------
// QwtFigureGridSplitter::PrivateData
//----------------------------------------------------

QwtFigureGrid* QwtFigureGridSplitter::PrivateData::grid() const
{
    return figure ? figure->grid() : nullptr;
}

void QwtFigureGridSplitter::PrivateData::refreshWatchedWidgets()
{
    clearWatchedWidgets();
    if (!figure || !enabled) {
        return;
    }

    // Events over the gaps between the cells are delivered to the figure itself,
    // the plots and their children cover the cells.
    addWatchedWidget(figure.data(), false);

    const QList< QWidget* > children = figure->findChildren< QWidget* >(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget* child : qwt_as_const(children)) {
        if (!child) {
            continue;
        }
        if (child->testAttribute(Qt::WA_TransparentForMouseEvents)) {
            continue;
        }
        if (isOpenGLWidget(child)) {
            continue;  // 3D plots must not be intercepted
        }

        if (QwtPlot* plot = qobject_cast< QwtPlot* >(child)) {
            addWatchedWidget(plot, true);
            const QList< QwtPlot* > parasites = plot->parasitePlots();
            for (QwtPlot* parasite : qwt_as_const(parasites)) {
                if (!parasite) {
                    continue;
                }
                // Parasite plots are usually not alone: they share the host area
                if (parasite->testAttribute(Qt::WA_TransparentForMouseEvents)) {
                    continue;
                }
                if (isOpenGLWidget(parasite)) {
                    continue;
                }
                addWatchedWidget(parasite, true);
            }
            continue;
        }

        // Non-plot grid members (custom widgets) get the same treatment
        QwtFigureGrid* figureGrid = grid();
        int row = 0, col = 0, rowSpan = 0, colSpan = 0;
        if (figureGrid && figureGrid->gridPosition(child, &row, &col, &rowSpan, &colSpan)) {
            addWatchedWidget(child, true);
        }
    }
}

void QwtFigureGridSplitter::PrivateData::addWatchedWidget(QWidget* widget, bool includeDescendants)
{
    if (!widget || !q_ptr) {
        return;
    }

    // Filters are installed on the widget itself and, for the plots, on the
    // interactive children (canvas, axis widgets). The children are needed to
    // consume the press before the picker/axis handlers of the plot start a
    // selection; the plot then never sees the in-band events.
    const auto addOne = [ this ](QWidget* w) {
        if (!w) {
            return;
        }
        w->installEventFilter(q_ptr);
        watchedWidgets.append(w);
        if (!w->hasMouseTracking()) {
            TrackingEntry entry;
            entry.widget     = w;
            entry.wasEnabled = false;
            trackingEntries.append(entry);
            w->setMouseTracking(true);
        }
    };

    addOne(widget);
    if (!includeDescendants) {
        return;
    }

    const QList< QWidget* > descendants = widget->findChildren< QWidget* >();
    for (QWidget* descendant : qwt_as_const(descendants)) {
        if (!descendant) {
            continue;
        }
        if (descendant->testAttribute(Qt::WA_TransparentForMouseEvents)) {
            continue;
        }
        if (isOpenGLWidget(descendant)) {
            continue;
        }
        addOne(descendant);
    }
}

void QwtFigureGridSplitter::PrivateData::clearWatchedWidgets()
{
    for (const QPointer< QWidget >& widget : qwt_as_const(watchedWidgets)) {
        if (widget) {
            widget->removeEventFilter(q_ptr);
        }
    }
    watchedWidgets.clear();

    for (const TrackingEntry& entry : qwt_as_const(trackingEntries)) {
        if (entry.widget) {
            entry.widget->setMouseTracking(entry.wasEnabled);
        }
    }
    trackingEntries.clear();
}

QwtFigureGridSplitter::PrivateData::Hit QwtFigureGridSplitter::PrivateData::hitTest(const QPoint& figurePos) const
{
    Hit hit;
    if (!figure) {
        return hit;
    }
    if (!figure->rect().contains(figurePos)) {
        return hit;
    }

    // A 3D cell never serves a divider, the boundary is hit on the 2D side
    if (isOpenGLWidget(cellWidgetAt(figurePos))) {
        return hit;
    }

    QwtFigureGrid* figureGrid = grid();
    if (!figureGrid || !figureGrid->isEnabled()) {
        return hit;
    }

    const QList< QwtFigureGrid::Divider > dividers = figureGrid->dividers();
    for (const QwtFigureGrid::Divider& divider : qwt_as_const(dividers)) {
        const bool horizontal = (divider.orientation == Qt::Horizontal);
        const int dimension   = horizontal ? figure->width() : figure->height();
        if (dimension <= 0) {
            continue;
        }
        const int dividerPos = qRound(divider.position * dimension);
        const int mousePos   = horizontal ? figurePos.x() : figurePos.y();
        const int distance   = qAbs(mousePos - dividerPos);
        if (distance > hitWidth) {
            continue;
        }
        if (!hit.valid || distance < hit.distance) {
            hit.valid       = true;
            hit.orientation = divider.orientation;
            hit.index       = divider.index;
            hit.distance    = distance;
        }
    }
    return hit;
}

QWidget* QwtFigureGridSplitter::PrivateData::cellWidgetAt(const QPoint& figurePos) const
{
    if (!figure) {
        return nullptr;
    }
    QWidget* widget = figure->childAt(figurePos);
    while (widget && widget->parentWidget() != figure.data()) {
        widget = widget->parentWidget();
    }
    return widget ? widget : figure.data();
}

QWidget* QwtFigureGridSplitter::PrivateData::cursorTargetAt(const QPoint& figurePos) const
{
    if (!figure) {
        return nullptr;
    }
    // The cursor must be set on the widget the mouse is really over: an
    // explicitly set cursor of a child (DA sets Qt::ArrowCursor on its canvas)
    // overrides the cursor inherited from the cell widget, so the split
    // cursor applied to the cell would never be visible on the canvas.
    QWidget* widget = figure->childAt(figurePos);
    while (widget && widget != figure.data() && widget->testAttribute(Qt::WA_TransparentForMouseEvents)) {
        widget = widget->parentWidget();
    }
    return (widget && widget != figure.data()) ? widget : figure.data();
}

void QwtFigureGridSplitter::PrivateData::applyCursor(Qt::CursorShape shape, QWidget* target)
{
    if (!target) {
        target = figure.data();
    }
    if (!target) {
        return;
    }

    if (cursorWidget == target) {
        if (currentCursor == shape) {
            return;
        }
    } else {
        restoreCursorInternal();
        cursorWidget             = target;
        cursorWidgetHadSetCursor = target->testAttribute(Qt::WA_SetCursor);
        originalCursor           = target->cursor();
    }

    target->setCursor(shape);
    if (currentCursor != shape) {
        currentCursor = shape;
        Q_EMIT q_func()->hoverChanged(shape);
    }
}

void QwtFigureGridSplitter::PrivateData::restoreCursorInternal()
{
    if (!cursorWidget) {
        return;
    }
    if (cursorWidgetHadSetCursor) {
        cursorWidget->setCursor(originalCursor);
    } else {
        cursorWidget->unsetCursor();
    }
    cursorWidget = nullptr;
}

void QwtFigureGridSplitter::PrivateData::restoreCursor()
{
    restoreCursorInternal();
    if (currentCursor != Qt::ArrowCursor) {
        currentCursor = Qt::ArrowCursor;
        Q_EMIT q_func()->hoverChanged(currentCursor);
    }
}

bool QwtFigureGridSplitter::PrivateData::handleHover(const QPoint& figurePos)
{
    const Hit hit = hitTest(figurePos);
    if (!hit.valid) {
        restoreCursor();
        return false;
    }

    const Qt::CursorShape shape = (hit.orientation == Qt::Horizontal) ? Qt::SplitHCursor : Qt::SplitVCursor;
    applyCursor(shape, cursorTargetAt(figurePos));
    return true;
}

bool QwtFigureGridSplitter::PrivateData::usesBoundaryLine() const
{
    return dragMode == BoundaryLine;
}

int QwtFigureGridSplitter::PrivateData::dividerPixelPosition(const QVector< qreal >& trackSizes, qreal spacing, int dimension) const
{
    if (dragIndex < 0 || dragIndex >= trackSizes.size() || dimension <= 0) {
        return 0;
    }

    // The divider sits in the center of the gap behind the dragged track
    qreal centre = 0.0;
    for (int i = 0; i <= dragIndex; ++i) {
        centre += trackSizes.at(i);
    }
    centre += dragIndex * spacing + spacing / 2.0;
    centre = qBound(0.0, centre, 1.0);

    return qBound(0, qRound(centre * dimension), dimension - 1);
}

void QwtFigureGridSplitter::PrivateData::updateBoundaryLine(Qt::Orientation orientation, int axisPos)
{
    if (!figure) {
        return;
    }

    if (!boundaryLine) {
        boundaryLine = new QwtFigureGridBoundaryLine(figure.data());
    }

    boundaryLine->setLinePen(boundaryLinePen);
    boundaryLine->placeLine(orientation, axisPos, figure->size());
    if (!boundaryLine->isVisible()) {
        boundaryLine->show();
    }
    boundaryLine->raise();
    boundaryAxisPos = axisPos;
}

void QwtFigureGridSplitter::PrivateData::removeBoundaryLine()
{
    if (!boundaryLine) {
        return;
    }
    boundaryLine->hide();
    delete boundaryLine.data();
    boundaryLine    = nullptr;
    boundaryAxisPos = 0;
}

void QwtFigureGridSplitter::PrivateData::applyPendingSizes()
{
    QwtFigureGrid* figureGrid = grid();
    if (!figureGrid || !figureGrid->isEnabled()) {
        return;
    }

    if (dragOrientation == Qt::Horizontal) {
        if (!pendingColumnSizes.isEmpty()) {
            figureGrid->setColumnSizes(pendingColumnSizes);
        }
    } else if (!pendingRowSizes.isEmpty()) {
        figureGrid->setRowSizes(pendingRowSizes);
    }
}

void QwtFigureGridSplitter::PrivateData::startDrag(const Hit& hit, const QPoint& figurePos)
{
    if (!figure || dragging) {
        return;
    }
    QwtFigureGrid* figureGrid = grid();
    if (!figureGrid || !figureGrid->isEnabled()) {
        return;
    }

    dragging            = true;
    dragOrientation     = hit.orientation;
    dragIndex           = hit.index;
    dragStartAxisPos    = (hit.orientation == Qt::Horizontal) ? figurePos.x() : figurePos.y();
    originalRowSizes    = figureGrid->rowSizes();
    originalColumnSizes = figureGrid->columnSizes();
    pendingRowSizes     = originalRowSizes;
    pendingColumnSizes  = originalColumnSizes;

    if (usesBoundaryLine()) {
        // Show the virtual boundary at the current gap center; the grid is not
        // touched until the mouse is released
        const bool horizontal         = (dragOrientation == Qt::Horizontal);
        const int dimension           = horizontal ? figure->width() : figure->height();
        const QVector< qreal >& sizes = horizontal ? originalColumnSizes : originalRowSizes;
        const qreal spacing           = horizontal ? figureGrid->wspace() : figureGrid->hspace();
        updateBoundaryLine(dragOrientation, dividerPixelPosition(sizes, spacing, dimension));
    }

    const Qt::CursorShape shape = (hit.orientation == Qt::Horizontal) ? Qt::SplitHCursor : Qt::SplitVCursor;
    applyCursor(shape, cursorTargetAt(figurePos));

    // Grab the mouse so the release always comes back to the figure: a filter
    // installed after this one (Qt invokes the filters in reverse installation
    // order) may consume the move/release of the canvas, which would leave the
    // drag session stuck. The grab redirects the events to the figure, whose
    // filter chain is not shared with the canvas.
    figure->grabMouse();
    mouseGrabbed = (QWidget::mouseGrabber() == figure.data());

    figure->grabKeyboard();
    keyboardGrabbed = (QWidget::keyboardGrabber() == figure.data());
}

void QwtFigureGridSplitter::PrivateData::dragTo(const QPoint& figurePos)
{
    if (!dragging || !figure) {
        return;
    }
    QwtFigureGrid* figureGrid = grid();
    if (!figureGrid || !figureGrid->isEnabled()) {
        return;
    }

    const bool horizontal = (dragOrientation == Qt::Horizontal);
    const int dimension   = horizontal ? figure->width() : figure->height();
    if (dimension <= 0) {
        return;
    }

    QVector< qreal > trackSizes = horizontal ? originalColumnSizes : originalRowSizes;
    if (dragIndex < 0 || dragIndex + 1 >= trackSizes.size()) {
        return;
    }

    const qreal pairSum = trackSizes.at(dragIndex) + trackSizes.at(dragIndex + 1);
    const qreal minNorm = static_cast< qreal >(figureGrid->minimumTrackPixels()) / dimension;
    if (pairSum <= 0.0 || pairSum < 2.0 * minNorm) {
        return;  // there is no room to move without collapsing a track
    }

    const int axisPos = horizontal ? figurePos.x() : figurePos.y();
    const qreal delta = static_cast< qreal >(axisPos - dragStartAxisPos) / dimension;
    const qreal newA  = qBound(minNorm, trackSizes.at(dragIndex) + delta, pairSum - minNorm);

    trackSizes[ dragIndex ]     = newA;
    trackSizes[ dragIndex + 1 ] = pairSum - newA;

    if (usesBoundaryLine()) {
        // Keep the preview only: the line follows the divider, the model stays
        // untouched, so the members never relayout or repaint during the drag
        QVector< qreal >& pendingSizes = horizontal ? pendingColumnSizes : pendingRowSizes;
        if (!sameSplitterSizes(pendingSizes, trackSizes)) {
            pendingSizes        = trackSizes;
            const qreal spacing = horizontal ? figureGrid->wspace() : figureGrid->hspace();
            updateBoundaryLine(dragOrientation, dividerPixelPosition(pendingSizes, spacing, dimension));
        }
    } else {
        // Live preview: apply the ratios and replot now. The synchronous layout
        // pass of QwtFigure runs inside setColumnSizes()/setRowSizes(); the
        // explicit replot keeps the axis scales and the canvas maps in sync, so
        // a live drag never shows a stale image. A boundary drag never pays for
        // this: that is the whole point of the default preview mode.
        const QVector< qreal > previous = horizontal ? figureGrid->columnSizes() : figureGrid->rowSizes();
        if (horizontal) {
            figureGrid->setColumnSizes(trackSizes);
        } else {
            figureGrid->setRowSizes(trackSizes);
        }

        const QVector< qreal >& applied = horizontal ? figureGrid->columnSizes() : figureGrid->rowSizes();
        if (!sameSplitterSizes(previous, applied)) {
            figure->replotAll();
        }
    }

    // Keep the split cursor while the mouse is captured far away from the start
    QWidget* target = cursorTargetAt(figurePos);
    if (!isOpenGLWidget(target)) {
        applyCursor(horizontal ? Qt::SplitHCursor : Qt::SplitVCursor, target);
    }
}

void QwtFigureGridSplitter::PrivateData::endDrag(bool cancelled)
{
    if (!dragging) {
        return;
    }

    dragging      = false;
    pressConsumed = false;

    QwtFigureGrid* figureGrid         = grid();
    const Qt::Orientation orientation = dragOrientation;
    const bool boundaryPreview        = usesBoundaryLine();

    // The boundary line is a preview only: it must never survive the drag
    removeBoundaryLine();

    bool sizesDiffer = false;
    if (figureGrid) {
        if (cancelled) {
            if (!boundaryPreview) {
                // Restore the snapshot of a live drag; the members are visible
                // again, so the restored sizes have to be painted below
                sizesDiffer = !sameSplitterSizes(figureGrid->rowSizes(), originalRowSizes)
                              || !sameSplitterSizes(figureGrid->columnSizes(), originalColumnSizes);
                if (orientation == Qt::Horizontal) {
                    figureGrid->setColumnSizes(originalColumnSizes);
                } else {
                    figureGrid->setRowSizes(originalRowSizes);
                }
            }
            // A boundary preview leaves the grid untouched: nothing to restore
        } else {
            if (boundaryPreview) {
                // The only model update of a boundary drag: apply the preview
                applyPendingSizes();
            }
            sizesDiffer = !sameSplitterSizes(figureGrid->rowSizes(), originalRowSizes)
                          || !sameSplitterSizes(figureGrid->columnSizes(), originalColumnSizes);
        }
    }

    if (figure && keyboardGrabbed) {
        figure->releaseKeyboard();
    }
    keyboardGrabbed = false;

    // Release the mouse grab of the drag session. The grab may already be gone
    // (a popup opened, the window was deactivated), so check the grabber before
    // releasing; releasing without holding the grab is a harmless no-op anyway.
    if (figure && mouseGrabbed) {
        if (QWidget::mouseGrabber() == figure.data()) {
            figure->releaseMouse();
        }
        mouseGrabbed = false;
    }
    restoreCursor();

    if (figure) {
        if (QLayout* lay = figure->layout()) {
            lay->setGeometry(figure->rect());
        }
        if (!cancelled && sizesDiffer && figureGrid) {
            // The only place the finished signal is emitted: exactly once per drag
            figureGrid->finishChange();
        }
        if (sizesDiffer) {
            figure->replotAll();
        }
    } else if (!cancelled && sizesDiffer && figureGrid) {
        figureGrid->finishChange();
    }

    clearDragState();
}

void QwtFigureGridSplitter::PrivateData::clearDragState()
{
    dragOrientation  = Qt::Horizontal;
    dragIndex        = -1;
    dragStartAxisPos = 0;
    originalRowSizes.clear();
    originalColumnSizes.clear();
    pendingRowSizes.clear();
    pendingColumnSizes.clear();
}

//----------------------------------------------------
// QwtFigureGridSplitter
//----------------------------------------------------

/**
 * @brief Constructor
 * @param figure Figure whose retained grid is edited, must not be null
 * @note The splitter becomes a child of the figure, the interaction stays
 *       disabled until setEnabled(true) is called
 */
QwtFigureGridSplitter::QwtFigureGridSplitter(QwtFigure* figure) : QObject(figure), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    if (!figure) {
        qWarning() << "QwtFigureGridSplitter: a non-null QwtFigure is required";
        return;
    }
    d->figure = figure;
}

/**
 * @brief Destructor
 *
 * Ends a running drag, removes all event filters, restores the mouse tracking
 * states and the cursor of the widget that received the split cursor.
 */
QwtFigureGridSplitter::~QwtFigureGridSplitter()
{
    QWT_D(d);
    if (d->dragging) {
        d->endDrag(true);
    }
    d->removeBoundaryLine();
    d->clearWatchedWidgets();
    d->restoreCursor();
    d->enabled = false;
}

/**
 * @brief The figure the splitter works on
 * @return The figure, nullptr when the splitter was created with a null pointer
 */
QwtFigure* QwtFigureGridSplitter::figure() const
{
    QWT_DC(d);
    return d->figure.data();
}

/**
 * @brief Enable or disable the divider interaction
 *
 * Enabling installs event filters on the figure and on the widgets that cover
 * the grid cells and prepares the mouse tracking needed to receive the hover
 * moves. Calling setEnabled(true) while already enabled re-scans the widgets,
 * which is how new members are picked up without re-creating the splitter.
 *
 * Disabling releases a running drag (the original ratios are restored), the
 * mouse and the keyboard grab, all event filters and the split cursor.
 *
 * @param on True to enable the interaction
 * @note Qt invokes the event filters of a widget in reverse installation order:
 *       the filter installed last runs first. Install the filters of the host
 *       (canvas pickers, drag handlers, ...) before enabling the grid editing;
 *       a filter installed on a canvas afterwards runs ahead of the splitter and
 *       can consume the events of the divider bands. Enabling an already enabled
 *       splitter re-scans and re-installs its filters, so calling
 *       setEnabled(true) once more puts the splitter back in front.
 */
void QwtFigureGridSplitter::setEnabled(bool on)
{
    QWT_D(d);
    if (!d->figure) {
        return;
    }

    if (on) {
        d->enabled = true;
        d->refreshWatchedWidgets();
        return;
    }

    if (!d->enabled) {
        return;
    }
    if (d->dragging) {
        d->endDrag(true);
    }
    d->removeBoundaryLine();
    d->clearWatchedWidgets();
    d->enabled = false;
    d->restoreCursor();
}

/**
 * @brief Test whether the divider interaction is enabled
 * @return True when the splitter is enabled
 */
bool QwtFigureGridSplitter::isEnabled() const
{
    QWT_DC(d);
    return d->enabled;
}

/**
 * @brief Set the half width of the hit band around a divider
 *
 * A divider is hit when the mouse is at most @p px pixels away from the center
 * of the gap. The default is 4 pixels.
 *
 * @param px Half width in pixels, negative values are ignored
 */
void QwtFigureGridSplitter::setHitWidth(int px)
{
    if (px < 0) {
        qWarning() << "QwtFigureGridSplitter::setHitWidth: the hit width must be non-negative, got" << px;
        return;
    }
    QWT_D(d);
    d->hitWidth = px;
}

/**
 * @brief The half width of the hit band around a divider
 * @return Half width in pixels, 4 by default
 */
int QwtFigureGridSplitter::hitWidth() const
{
    QWT_DC(d);
    return d->hitWidth;
}

/**
 * @brief Test whether a divider is currently dragged
 * @return True while a drag session is active
 */
bool QwtFigureGridSplitter::isDragging() const
{
    QWT_DC(d);
    return d->dragging;
}

/**
 * @brief Set how the members follow a divider while it is dragged
 *
 * BoundaryLine (the default) only moves a virtual boundary line while the
 * mouse is down: the tracks keep their size and the new ratios are applied
 * once, on release. The members are neither relaid out nor repainted during
 * the drag, which is the cheapest option for figures with expensive members.
 *
 * Live resizes the adjacent tracks on every mouse move, runs the synchronous
 * figure layout and replots the members, so the plots really follow the
 * divider. It costs a full layout and replot per mouse move.
 *
 * Changing the mode while a divider is dragged cancels the running drag: a
 * half-applied live drag must not be left behind.
 *
 * @param mode Preview mode of the drag
 * @sa dragPreviewMode(), setBoundaryLinePen()
 */
void QwtFigureGridSplitter::setDragPreviewMode(DragPreviewMode mode)
{
    QWT_D(d);
    if (d->dragMode == mode) {
        return;
    }
    if (d->dragging) {
        d->endDrag(true);
    }
    d->dragMode = mode;
}

/**
 * @brief How the members follow a divider while it is dragged
 * @return The preview mode, BoundaryLine by default
 * @sa setDragPreviewMode()
 */
QwtFigureGridSplitter::DragPreviewMode QwtFigureGridSplitter::dragPreviewMode() const
{
    QWT_DC(d);
    return d->dragMode;
}

/**
 * @brief Set the pen of the virtual boundary line
 *
 * The line is drawn in BoundaryLine mode only. Its width defines the thickness
 * of the line strip; the default is a two pixel dashed blue line. Set a pen
 * with Qt::NoPen to hide the line while the members still keep their size
 * until the release.
 *
 * @param pen Pen of the boundary line
 * @sa boundaryLinePen(), setDragPreviewMode()
 */
void QwtFigureGridSplitter::setBoundaryLinePen(const QPen& pen)
{
    QWT_D(d);
    if (d->boundaryLinePen == pen) {
        return;
    }
    d->boundaryLinePen = pen;

    if (d->boundaryLine) {
        // The strip thickness follows the pen width: place it again
        d->updateBoundaryLine(d->dragOrientation, d->boundaryAxisPos);
    }
}

/**
 * @brief The pen of the virtual boundary line
 * @return Pen of the boundary line
 * @sa setBoundaryLinePen()
 */
QPen QwtFigureGridSplitter::boundaryLinePen() const
{
    QWT_DC(d);
    return d->boundaryLinePen;
}

bool QwtFigureGridSplitter::eventFilter(QObject* watched, QEvent* event)
{
    QWT_D(d);
    QWidget* widget = qobject_cast< QWidget* >(watched);
    if (!d->enabled || !d->figure || !widget) {
        return QObject::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        QMouseEvent* mouseEvent = static_cast< QMouseEvent* >(event);
        if (mouseEvent->button() != Qt::LeftButton) {
            d->pressConsumed = false;
            break;
        }
        if (d->dragging) {
            break;
        }
        const QPoint pos           = figureLocalPos(d->figure.data(), widget, event);
        const PrivateData::Hit hit = d->hitTest(pos);
        if (!hit.valid) {
            d->pressConsumed = false;
            break;
        }
        d->pressConsumed = true;
        d->startDrag(hit, pos);
        return true;
    }
    case QEvent::MouseButtonDblClick: {
        if (d->dragging) {
            return true;
        }
        const QPoint pos = figureLocalPos(d->figure.data(), widget, event);
        if (!d->hitTest(pos).valid) {
            break;
        }
        d->pressConsumed = true;
        return true;
    }
    case QEvent::MouseMove: {
        const QPoint pos = figureLocalPos(d->figure.data(), widget, event);
        if (d->dragging) {
            d->dragTo(pos);
            return true;
        }
        if (static_cast< QMouseEvent* >(event)->buttons() != Qt::NoButton) {
            break;  // another button is held: the event belongs to the plot
        }
        return d->handleHover(pos);
    }
    case QEvent::HoverMove: {
        if (d->dragging) {
            return true;
        }
        const QPoint pos = figureLocalPos(d->figure.data(), widget, event);
        return d->handleHover(pos);
    }
    case QEvent::Resize: {
        // Keep the virtual boundary line aligned when the figure is resized
        // while a divider is dragged (a live drag is relaid out anyway)
        if (d->dragging && d->usesBoundaryLine() && widget == d->figure.data()) {
            const bool horizontal           = (d->dragOrientation == Qt::Horizontal);
            const int dimension             = horizontal ? d->figure->width() : d->figure->height();
            const QVector< qreal >& sizes   = horizontal ? d->pendingColumnSizes : d->pendingRowSizes;
            const QwtFigureGrid* figureGrid = d->grid();
            const qreal spacing = figureGrid ? (horizontal ? figureGrid->wspace() : figureGrid->hspace()) : 0.0;
            d->updateBoundaryLine(d->dragOrientation, d->dividerPixelPosition(sizes, spacing, dimension));
        }
        break;
    }
    case QEvent::MouseButtonRelease: {
        if (d->dragging) {
            d->endDrag(false);
            return true;
        }
        if (d->pressConsumed) {
            d->pressConsumed = false;
            return true;
        }
        break;
    }
    case QEvent::Leave:
    case QEvent::HoverLeave: {
        // The cursor target is the widget under the mouse (typically the
        // canvas); leaving it does not mean the mouse left the figure - it may
        // just have moved to a sibling child of the same cell, and the next
        // move event re-applies or clears the cursor. Only the leave of the
        // figure itself ends the hover for good.
        if (!d->dragging && widget == d->figure.data()) {
            d->restoreCursor();
        }
        break;
    }
    case QEvent::UngrabMouse:
    case QEvent::WindowDeactivate:
    case QEvent::ApplicationDeactivate: {
        // The release will never arrive: the mouse grab was lost or the window
        // was deactivated (a modal dialog opened, the user switched tasks).
        // End the drag as cancelled so a preview is removed or a live drag is
        // rolled back instead of leaving the figure in a half-dragged state.
        if (d->dragging) {
            d->endDrag(true);
        }
        break;
    }
    case QEvent::KeyPress: {
        if (!d->dragging) {
            break;
        }
        QKeyEvent* keyEvent = static_cast< QKeyEvent* >(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            d->endDrag(true);  // cancel: no finished signal
            return true;
        }
        break;
    }
    default:
        break;
    }

    return QObject::eventFilter(watched, event);
}
