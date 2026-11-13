/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "qwt_figure_grid_splitter.h"

// Qt
#include <QCursor>
#include <QDebug>
#include <QEvent>
#include <QHoverEvent>
#include <QKeyEvent>
#include <QLayout>
#include <QMouseEvent>
#include <QPointer>
#include <QVector>
#include <QWidget>

// qwt
#include "qwt_figure.h"
#include "qwt_figure_grid.h"
#include "qwt_plot.h"
#include "qwt_qt5qt6_compat.hpp"

namespace
{
// Absolute tolerance used to compare track ratio vectors
const qreal SIZE_EPSILON = 1e-9;

// Returns true for widgets that render through OpenGL; their events are never intercepted
bool isOpenGLWidget(const QWidget* widget)
{
    return widget && (widget->inherits("QOpenGLWidget") || widget->inherits("QGLWidget"));
}

// Compares two track ratio vectors
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
    // Disables or restores the repaints of all grid members
    void setMembersUpdatesEnabled(bool on);
    // Resets the drag session state without touching the grid
    void clearDragState();

public:
    QPointer< QwtFigure > figure;
    bool enabled { false };
    int hitWidth { 4 };

    bool dragging { false };
    bool pressConsumed { false };
    Qt::Orientation dragOrientation { Qt::Horizontal };
    int dragIndex { -1 };
    int dragStartAxisPos { 0 };
    QVector< qreal > originalRowSizes;
    QVector< qreal > originalColumnSizes;
    QList< QPointer< QWidget > > updateDisabledWidgets;
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

void QwtFigureGridSplitter::PrivateData::setMembersUpdatesEnabled(bool on)
{
    if (on) {
        for (const QPointer< QWidget >& widget : qwt_as_const(updateDisabledWidgets)) {
            if (!widget) {
                continue;
            }
            widget->setUpdatesEnabled(true);
            widget->update();
        }
        updateDisabledWidgets.clear();
        return;
    }

    QwtFigureGrid* figureGrid = grid();
    if (!figureGrid) {
        return;
    }
    const QList< QWidget* > members = figureGrid->items();
    for (QWidget* member : qwt_as_const(members)) {
        if (!member || !member->updatesEnabled()) {
            continue;
        }
        member->setUpdatesEnabled(false);
        updateDisabledWidgets.append(member);
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

    setMembersUpdatesEnabled(false);

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

    QVector< qreal > originalSizes = horizontal ? originalColumnSizes : originalRowSizes;
    if (dragIndex < 0 || dragIndex + 1 >= originalSizes.size()) {
        return;
    }

    const qreal pairSum = originalSizes.at(dragIndex) + originalSizes.at(dragIndex + 1);
    const qreal minNorm = static_cast< qreal >(figureGrid->minimumTrackPixels()) / dimension;
    if (pairSum <= 0.0 || pairSum < 2.0 * minNorm) {
        return;  // there is no room to move without collapsing a track
    }

    const int axisPos = horizontal ? figurePos.x() : figurePos.y();
    const qreal delta = static_cast< qreal >(axisPos - dragStartAxisPos) / dimension;
    const qreal newA  = qBound(minNorm, originalSizes.at(dragIndex) + delta, pairSum - minNorm);

    originalSizes[ dragIndex ]     = newA;
    originalSizes[ dragIndex + 1 ] = pairSum - newA;

    if (horizontal) {
        figureGrid->setColumnSizes(originalSizes);
    } else {
        figureGrid->setRowSizes(originalSizes);
    }

    // Re-apply the geometries immediately: the member widgets have their
    // repaints disabled, so the layout must not wait for an update request.
    if (QLayout* lay = figure->layout()) {
        lay->setGeometry(figure->rect());
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

    if (cancelled && figureGrid) {
        // Restore the snapshot before the repaints are enabled again, so the
        // plots never paint an intermediate state
        if (orientation == Qt::Horizontal) {
            figureGrid->setColumnSizes(originalColumnSizes);
        } else {
            figureGrid->setRowSizes(originalRowSizes);
        }
    }

    setMembersUpdatesEnabled(true);

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

    bool sizesDiffer = false;
    if (figureGrid) {
        sizesDiffer = !sameSizes(figureGrid->rowSizes(), originalRowSizes)
                      || !sameSizes(figureGrid->columnSizes(), originalColumnSizes);
    }

    if (figure) {
        if (QLayout* lay = figure->layout()) {
            lay->setGeometry(figure->rect());
        }
        if (!cancelled && sizesDiffer) {
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
    updateDisabledWidgets.clear();
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
        // End the drag as cancelled so the members repaint again instead of
        // staying stuck with their repaints disabled.
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
