/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "qwt_figure.h"
// Qt
#include <QHash>
#include <QPainter>
#include <QPointer>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QEvent>
#include <QApplication>
#include <QGuiApplication>
#include <QTimer>
#include <QScreen>
#include <QWidgetItem>
#include <QMap>
#include <QDebug>
// qwt
#include "qwt_figure_layout.h"
#include "qwt_figure_grid.h"
#include "qwt_figure_grid_splitter.h"
#include "qwt_scale_engine.h"
#include "qwt_scale_widget.h"
#include "qwt_plot_layout.h"
#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include "qwt_plot_transparent_canvas.h"
#include "qwt_parasite_plot_layout.h"

#ifndef QWTFIGURE_SAFEGET_LAY
#define QWTFIGURE_SAFEGET_LAY(lay)                                                                                     \
    QwtFigureLayout* lay = qobject_cast< QwtFigureLayout* >(layout());                                                 \
    if (!lay) {                                                                                                        \
        return;                                                                                                        \
    }
#endif

#ifndef QWTFIGURE_SAFEGET_LAY_RET
#define QWTFIGURE_SAFEGET_LAY_RET(lay, ret)                                                                            \
    QwtFigureLayout* lay = qobject_cast< QwtFigureLayout* >(layout());                                                 \
    if (!lay) {                                                                                                        \
        return ret;                                                                                                    \
    }
#endif

namespace
{
/**
 * @brief Check whether a plot can take part in the alignment
 * @details Parasite plots share the canvas of their host and are laid out by it,
 *          they are never aligned on their own.
 */
bool isAlignablePlot(const QwtPlot* plot)
{
    return plot && !plot->isParasitePlot();
}

/**
 * @brief Measure the natural canvas inset of one side of a plot
 * @details The natural inset is the distance between the plot contents rect and
 *          the canvas edge the content asks for, before any figure alignment is
 *          applied. It contains everything that consumes space on that side:
 *          the axis band of the host and of its parasite layers, the tick labels,
 *          titles, a legend in that band, and so on.
 * @param[in] plot Plot to measure
 * @param[in] axisPos Side of the canvas (QwtAxis::Position)
 * @param[out] inset Natural inset in pixels
 * @return true when the inset could be measured
 */
bool naturalCanvasInset(QwtPlot* plot, int axisPos, qreal* inset)
{
    if (!isAlignablePlot(plot) || !inset)
        return false;

    QwtPlotLayout* layout = plot->plotLayout();
    if (!layout)
        return false;

    const QRectF plotRect = plot->contentsRect();
    if (plotRect.width() <= 0.0 || plotRect.height() <= 0.0)
        return false;

    /*
       A host plot reserves the bands of its parasite layers in the dimensions
       of its own axes. updateAllAxisEdgeMargin() maintains those offsets after a
       layout pass, so settle them before the natural rect is read.
     */
    if (!plot->parasitePlots().isEmpty()) {
        layout->activate(plot, plotRect);
        plot->updateAllAxisEdgeMargin();
    }
    layout->activate(plot, plotRect);

    const QRectF canvas = layout->naturalCanvasRect();
    if (!canvas.isValid())
        return false;

    switch (axisPos) {
    case QwtAxis::YLeft:
        *inset = qMax(qreal(0.0), canvas.left() - plotRect.left());
        break;
    case QwtAxis::YRight:
        *inset = qMax(qreal(0.0), plotRect.right() - canvas.right());
        break;
    case QwtAxis::XTop:
        *inset = qMax(qreal(0.0), canvas.top() - plotRect.top());
        break;
    case QwtAxis::XBottom:
        *inset = qMax(qreal(0.0), plotRect.bottom() - canvas.bottom());
        break;
    default:
        return false;
    }
    return true;
}

//! Canvas inset targets of a plot, indexed by QwtAxis::Position
typedef QVector< qreal > CanvasInsets;

/**
 * @brief Measure the natural canvas insets of all four sides of a plot
 * @return True when every side could be measured
 */
bool naturalCanvasInsets(QwtPlot* plot, CanvasInsets* insets)
{
    if (!insets)
        return false;
    CanvasInsets values;
    values.reserve(QwtAxis::AxisPositions);
    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++) {
        qreal inset = 0.0;
        if (!naturalCanvasInset(plot, axisPos, &inset))
            return false;
        values.append(inset);
    }
    *insets = values;
    return true;
}

//! Apply the minimum canvas insets of a plot
void applyCanvasInsets(QwtPlot* plot, const CanvasInsets& insets)
{
    QwtPlotLayout* layout = plot ? plot->plotLayout() : nullptr;
    if (!layout || insets.size() != QwtAxis::AxisPositions)
        return;
    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++)
        layout->setMinCanvasInset(axisPos, insets.at(axisPos));
}

//! Return a plot to the insets its own content asks for
void resetCanvasInsets(QwtPlot* plot)
{
    if (QwtPlotLayout* layout = plot ? plot->plotLayout() : nullptr)
        layout->resetMinCanvasInsets();
}

/**
 * @brief Compare two inset sets
 * @details Measured insets are pixel values that may jitter by a fraction of a
 *          pixel between passes; a small tolerance avoids relayouts for nothing.
 */
bool sameCanvasInsets(const CanvasInsets& lhs, const CanvasInsets& rhs)
{
    if (lhs.size() != rhs.size())
        return false;
    for (int i = 0; i < lhs.size(); i++) {
        if (qAbs(lhs.at(i) - rhs.at(i)) > 0.01)
            return false;
    }
    return true;
}

//! Raise the inset target of a group of plots on one side to the largest value
void raiseGroupInset(const QList< QwtPlot* >& group, int axisPos, QHash< QwtPlot*, CanvasInsets >& targets)
{
    if (group.size() < 2)
        return;

    qreal maxInset = 0.0;
    for (QwtPlot* plot : qwt_as_const(group)) {
        const auto it = targets.constFind(plot);
        if (it != targets.constEnd())
            maxInset = qMax(maxInset, it.value().at(axisPos));
    }
    for (QwtPlot* plot : qwt_as_const(group))
        targets[ plot ][ axisPos ] = maxInset;
}
}  // namespace

class QwtFigure::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtFigure)
public:
    PrivateData(QwtFigure* p);
    // Called when a plot is about to be removed
    void plotWillRemove(QwtPlot* p);
    // Let a running grid splitter pick up added/removed children
    void updateGridSplitter();
    // Install the press-activation filter on a widget and, for plots, on its canvas
    void watchWidget(QWidget* widget);
    // Remove the press-activation filter from a widget and, for plots, from its canvas
    void unwatchWidget(QWidget* widget);
    // Recompute and apply the plot area alignment of the figure
    void applyAxisAlignment(QwtFigure* figure, bool replot);
    // Check whether a plot takes part in the automatic or in a manual alignment
    bool isAlignmentParticipant(const QwtFigure* figure, const QwtPlot* plot) const;
    // Restore the natural plot area of a plot that leaves the figure
    void releaseAxisAlignment(QwtPlot* plot);
    // Structure for storing alignment configuration
    struct AlignmentConfig
    {
        QList< QwtPlot* > plots;  // List of plots to align
        int axisId;               // Axis ID to align
    };

public:
    QBrush faceBrush { Qt::white };             ///< Background color of the figure
    QColor edgeColor { Qt::black };             ///< Border color of the figure
    int edgeLineWidth { 0 };                    ///< Border line width
    QPointer< QwtPlot > currentAxes;            ///< Current active axes
    QPointer< QwtFigureGrid > grid;             ///< Retained grid layout (created on demand)
    QList< AlignmentConfig > alignmentConfigs;  // All alignment configurations

    bool gridAxisAlignment { true };                ///< Align the members of the retained grid automatically
    bool alignmentPending { false };                ///< A merged alignment pass is queued
    QHash< QwtPlot*, CanvasInsets > appliedInsets;  ///< Insets currently forced on the figure plots

    // Divider interaction (created on demand)
    QPointer< QwtFigureGridSplitter > gridSplitter;
};

QwtFigure::PrivateData::PrivateData(QwtFigure* p) : q_ptr(p)
{
}

void QwtFigure::PrivateData::plotWillRemove(QwtPlot* p)
{
    for (auto& alCfg : alignmentConfigs) {
        alCfg.plots.removeAll(p);
    }
}

void QwtFigure::PrivateData::updateGridSplitter()
{
    if (gridSplitter && gridSplitter->isEnabled()) {
        // Enabling an enabled splitter is idempotent and re-scans the widgets
        gridSplitter->setEnabled(true);
    }
}

void QwtFigure::PrivateData::watchWidget(QWidget* widget)
{
    if (!widget) {
        return;
    }
    // Real mouse presses are delivered to the canvas of the plot, not to the
    // plot itself, so the canvas needs the filter as well.
    widget->installEventFilter(q_ptr);
    if (QwtPlot* plot = qobject_cast< QwtPlot* >(widget)) {
        if (QWidget* canvas = plot->canvas()) {
            canvas->installEventFilter(q_ptr);
        }
    }
}

void QwtFigure::PrivateData::unwatchWidget(QWidget* widget)
{
    if (!widget) {
        return;
    }
    widget->removeEventFilter(q_ptr);
    if (QwtPlot* plot = qobject_cast< QwtPlot* >(widget)) {
        if (QWidget* canvas = plot->canvas()) {
            canvas->removeEventFilter(q_ptr);
        }
    }
}

/**
 * @brief Recompute the canvas inset targets and apply them to the plots
 * @details The pass has three stages:
 *          1. measure the natural canvas inset of every plot that takes part
 *          2. raise the inset of a plot to the largest inset of its alignment
 *             group (the grid rows/columns and the manual configurations)
 *          3. set the resulting minimum insets and relayout the changed plots
 *
 *          The grid groups are derived from the cell registrations: the members
 *          of a column share the left and right canvas edges, the members of a
 *          row share the top and bottom ones. A member spanning several tracks
 *          belongs to the group of the first and of the last track it covers.
 *
 *          Manual configurations are merged per side into connected components,
 *          so overlapping configurations cannot leave a plot out of an equality
 *          chain. Their targets start from the grid targets: a manual group never
 *          undoes the automatic alignment.
 * @param figure Figure the alignment belongs to
 * @param replot True to replot the participants after the pass
 */
void QwtFigure::PrivateData::applyAxisAlignment(QwtFigure* figure, bool replot)
{
    if (!figure)
        return;

    QList< QwtPlot* > participants;
    QHash< QwtPlot*, CanvasInsets > targets;

    const auto addParticipant = [ & ](QwtPlot* plot) -> bool {
        if (!isAlignablePlot(plot))
            return false;
        if (targets.contains(plot))
            return true;

        CanvasInsets insets;
        if (!naturalCanvasInsets(plot, &insets))
            return false;

        targets.insert(plot, insets);
        participants.append(plot);
        return true;
    };

    // ---- 1. automatic groups of the retained grid ----
    if (gridAxisAlignment && grid && grid->isEnabled()) {
        QHash< int, QList< QwtPlot* > > leftGroups, rightGroups, topGroups, bottomGroups;

        const QList< QWidget* > members = grid->items();
        for (QWidget* member : qwt_as_const(members)) {
            QwtPlot* plot = qobject_cast< QwtPlot* >(member);
            if (!plot || !plot->isVisibleTo(figure))
                continue;

            int row = 0, column = 0, rowSpan = 0, columnSpan = 0;
            if (!grid->gridPosition(plot, &row, &column, &rowSpan, &columnSpan))
                continue;
            if (!addParticipant(plot))
                continue;

            leftGroups[ column ].append(plot);
            rightGroups[ column + columnSpan ].append(plot);
            topGroups[ row ].append(plot);
            bottomGroups[ row + rowSpan ].append(plot);
        }

        for (auto it = leftGroups.constBegin(); it != leftGroups.constEnd(); ++it)
            raiseGroupInset(it.value(), QwtAxis::YLeft, targets);
        for (auto it = rightGroups.constBegin(); it != rightGroups.constEnd(); ++it)
            raiseGroupInset(it.value(), QwtAxis::YRight, targets);
        for (auto it = topGroups.constBegin(); it != topGroups.constEnd(); ++it)
            raiseGroupInset(it.value(), QwtAxis::XTop, targets);
        for (auto it = bottomGroups.constBegin(); it != bottomGroups.constEnd(); ++it)
            raiseGroupInset(it.value(), QwtAxis::XBottom, targets);
    }

    // ---- 2. manual configurations ----
    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++) {
        QList< QList< QwtPlot* > > groups;
        for (const AlignmentConfig& config : qwt_as_const(alignmentConfigs)) {
            if (config.axisId != axisPos)
                continue;

            QList< QwtPlot* > group;
            for (QwtPlot* plot : qwt_as_const(config.plots)) {
                if (plot && figure->hasAxes(plot) && addParticipant(plot))
                    group.append(plot);
            }
            if (group.size() > 1)
                groups.append(group);
        }
        if (groups.isEmpty())
            continue;

        // Merge the overlapping groups of this side with a union-find: all
        // plots of one component have to end up with the same inset.
        QHash< QwtPlot*, QwtPlot* > parent;
        const auto findRoot = [ &parent ](QwtPlot* plot) {
            QwtPlot* root = plot;
            while (parent.contains(root) && parent.value(root) != root)
                root = parent.value(root);
            return root;
        };

        for (const QList< QwtPlot* >& group : qwt_as_const(groups)) {
            QwtPlot* head = group.first();
            if (!parent.contains(head))
                parent.insert(head, head);

            for (int i = 1; i < group.size(); i++) {
                QwtPlot* plot = group.at(i);
                if (!parent.contains(plot))
                    parent.insert(plot, plot);

                QwtPlot* rootHead = findRoot(head);
                QwtPlot* rootPlot = findRoot(plot);
                if (rootHead != rootPlot)
                    parent[ rootPlot ] = rootHead;
            }
        }

        QHash< QwtPlot*, qreal > componentInset;
        for (auto it = parent.constBegin(); it != parent.constEnd(); ++it) {
            QwtPlot* plot          = it.key();
            QwtPlot* root          = findRoot(plot);
            const qreal inset      = targets.value(plot).at(axisPos);
            componentInset[ root ] = qMax(componentInset.value(root, 0.0), inset);
        }
        for (auto it = parent.constBegin(); it != parent.constEnd(); ++it) {
            QwtPlot* plot              = it.key();
            targets[ plot ][ axisPos ] = componentInset.value(findRoot(plot));
        }
    }

    // ---- 3. apply ----
    // Plots that left every alignment group go back to their natural layout.
    const QList< QwtPlot* > previouslyAligned = appliedInsets.keys();
    for (QwtPlot* plot : qwt_as_const(previouslyAligned)) {
        if (targets.contains(plot))
            continue;

        appliedInsets.remove(plot);

        // A plot that is not part of the figure anymore has already been
        // released when it was removed: do not touch a possibly stale pointer.
        if (!figure->hasAxes(plot))
            continue;

        resetCanvasInsets(plot);
        plot->updateLayout();
    }

    // Only the plots whose insets really changed need another layout pass.
    for (QwtPlot* plot : qwt_as_const(participants)) {
        const CanvasInsets target = targets.value(plot);
        if (sameCanvasInsets(appliedInsets.value(plot), target))
            continue;

        applyCanvasInsets(plot, target);
        plot->updateLayout();
    }
    appliedInsets = targets;

    if (replot) {
        for (QwtPlot* plot : qwt_as_const(participants))
            plot->replot();
    }
}

bool QwtFigure::PrivateData::isAlignmentParticipant(const QwtFigure* figure, const QwtPlot* plot) const
{
    Q_UNUSED(figure);
    if (!isAlignablePlot(plot))
        return false;

    QwtPlot* nonConstPlot = const_cast< QwtPlot* >(plot);

    if (gridAxisAlignment && grid && grid->isEnabled()) {
        int row = 0, column = 0, rowSpan = 0, columnSpan = 0;
        if (grid->gridPosition(nonConstPlot, &row, &column, &rowSpan, &columnSpan))
            return true;
    }

    for (const AlignmentConfig& config : qwt_as_const(alignmentConfigs)) {
        if (config.plots.contains(nonConstPlot))
            return true;
    }
    return false;
}

void QwtFigure::PrivateData::releaseAxisAlignment(QwtPlot* plot)
{
    if (!plot || !appliedInsets.contains(plot))
        return;

    appliedInsets.remove(plot);
    resetCanvasInsets(plot);
    plot->updateLayout();
}

//----------------------------------------------------
// QwtFigure
//----------------------------------------------------

/**
 * @brief Constructor
 * @param parent Parent widget
 * @param f Window flags
 */
QwtFigure::QwtFigure(QWidget* parent, Qt::WindowFlags f) : QFrame(parent, f), QWT_PIMPL_CONSTRUCT
{
    setLayout(new QwtFigureLayout());
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QwtFigure::~QwtFigure()
{
    // Tear the splitter down while the figure and its children are still alive:
    // it has to release the keyboard grab, the cursor and the event filters.
    if (m_data->gridSplitter) {
        m_data->gridSplitter->setEnabled(false);
        delete m_data->gridSplitter;
        m_data->gridSplitter = nullptr;
    }
}

/**
 * @brief Add a widget with normalized coordinates
 * @param widget QWidget to add
 * @param left Normalized coordinates left in range [0,1]
 * @param top Normalized coordinates top in range [0,1]
 * @param width Normalized coordinates width in range [0,1]
 * @param height Normalized coordinates height in range [0,1]
 *
 * @note Even if the added widget is a QwtPlot, this function will not emit axesAdded signal.
 *       Use addAxes() instead if you need to add QwtPlot widgets.
 * @sa addAxes
 */
void QwtFigure::addWidget(QWidget* widget, qreal left, qreal top, qreal width, qreal height)
{
    QWTFIGURE_SAFEGET_LAY(lay)
    if (widget && widget->parentWidget() != this) {
        widget->setParent(this);
    }
    if (widget) {
        // Track mouse presses to make the clicked sub plot the current one.
        // Real presses are delivered to the canvas, so it is watched as well.
        m_data->watchWidget(widget);
    }
    lay->addAxes(widget, left, top, width, height);
    m_data->updateGridSplitter();
}

/**
 * @brief Add a widget by grid layout
 * @details This method adds a widget to the grid layout at the specified position with optional row and column spans.
 * @param widget Widget to add
 * @param rowCnt Number of rows in the grid
 * @param colCnt Number of columns in the grid
 * @param row Grid row position (0-based)
 * @param col Grid column position (0-based)
 * @param rowSpan Number of rows to span (default: 1)
 * @param colSpan Number of columns to span (default: 1)
 * @param wspace Horizontal space between subplots [0,1]
 * @param hspace Vertical space between subplots [0,1]
 *
 * @note Even if the added widget is a QwtPlot, this function will not emit axesAdded signal.
 *       Use addAxes() instead if you need to add QwtPlot widgets.
 * @sa addAxes
 */
void QwtFigure::addWidget(QWidget* widget, int rowCnt, int colCnt, int row, int col, int rowSpan, int colSpan, qreal wspace, qreal hspace)
{
    QWTFIGURE_SAFEGET_LAY(lay)
    if (widget && widget->parentWidget() != this) {
        widget->setParent(this);
    }
    // Track mouse presses like the normalized coordinate overload does
    m_data->watchWidget(widget);
    lay->addGridAxes(widget, rowCnt, colCnt, row, col, rowSpan, colSpan, wspace, hspace);
    m_data->updateGridSplitter();
}

/**
 * @brief Add a plot with normalized coordinates
 * @details This method adds a QwtPlot to the figure using normalized coordinates in the range [0,1].
 *          The coordinates are specified as [left, bottom, width, height].
 * @param plot QwtPlot to add
 * @param rect Normalized coordinates [left, bottom, width, height] in range [0,1]
 *
 * @note This function will emit axesAdded signal, followed by currentAxesChanged signal
 */
void QwtFigure::addAxes(QwtPlot* plot, const QRectF& rect)
{
    addAxes(plot, rect.x(), rect.y(), rect.width(), rect.height());
}

/**
 * @brief Add a plot with normalized coordinates using separate parameters
 * @param plot QwtPlot to add
 * @param left Normalized coordinates left in range [0,1]
 * @param top Normalized coordinates top in range [0,1]
 * @param width Normalized coordinates width in range [0,1]
 * @param height Normalized coordinates height in range [0,1]
 *
 * @note This function will emit axesAdded signal, followed by currentAxesChanged signal
 */
void QwtFigure::addAxes(QwtPlot* plot, qreal left, qreal top, qreal width, qreal height)
{
    addWidget(plot, left, top, width, height);
    Q_EMIT axesAdded(plot);
    setCurrentAxes(plot);
}

/**
 * @brief Add a plot by grid layout
 * @details This method adds a QwtPlot to the grid layout at the specified position with optional row and column spans.
 * @param plot QwtPlot to add
 * @param rowCnt Number of rows in the grid
 * @param colCnt Number of columns in the grid
 * @param row Grid row position (0-based)
 * @param col Grid column position (0-based)
 * @param rowSpan Number of rows to span (default: 1)
 * @param colSpan Number of columns to span (default: 1)
 * @param wspace Horizontal space between subplots [0,1]
 * @param hspace Vertical space between subplots [0,1]
 *
 * @note This function will emit axesAdded signal, followed by currentAxesChanged signal
 */
void QwtFigure::addGridAxes(QwtPlot* plot, int rowCnt, int colCnt, int row, int col, int rowSpan, int colSpan, qreal wspace, qreal hspace)
{
    addWidget(plot, rowCnt, colCnt, row, col, rowSpan, colSpan, wspace, hspace);
    Q_EMIT axesAdded(plot);
    setCurrentAxes(plot);
}

/**
 * @brief Enable the retained grid layout
 * @details The retained grid keeps the row/column track sizes and the member positions
 *          as the single source of truth. The layout asks the grid for the normalized
 *          rectangle of every member whenever the figure geometry changes, so the track
 *          proportions survive resizes. Calling this again re-initializes the tracks with
 *          equal sizes and drops the previous member registrations.
 * @param rowCount Number of rows (must be positive)
 * @param columnCount Number of columns (must be positive)
 * @return true if the grid was enabled, false for invalid counts
 * @note Old addGridAxes() is unaffected and still converts the grid coordinates once.
 * @sa addAxes(QwtPlot*, int, int, int, int), addGridItem(QWidget*, int, int, int, int)
 */
bool QwtFigure::enableGrid(int rowCount, int columnCount)
{
    if (rowCount <= 0 || columnCount <= 0) {
        qWarning() << "QwtFigure::enableGrid: row and column count must be positive";
        return false;
    }

    if (!m_data->grid) {
        m_data->grid = new QwtFigureGrid(this);
        connect(m_data->grid.data(), &QwtFigureGrid::sizesChanged, this, &QwtFigure::gridSizesChanged);
        connect(m_data->grid.data(), &QwtFigureGrid::sizesChangeFinished, this, &QwtFigure::gridSizesChangeFinished);
        connect(m_data->grid.data(), &QwtFigureGrid::sizesChanged, this, [ this ]() {
            if (QLayout* lay = layout()) {
                lay->setGeometry(rect());
            }
        });
    }
    m_data->grid->setup(rowCount, columnCount);

    if (QLayout* lay = layout()) {
        lay->invalidate();
        lay->activate();
    }
    recomputeAxisAlignment(false);
    return true;
}

/**
 * @brief Leave the retained grid layout
 *
 * Every member registered in the grid is turned into a free member whose normalized
 * rectangle is derived from the geometry the layout actually assigned to it, so the
 * sub plots keep their current position and size and nothing jumps. Members that are
 * hidden at that moment are converted from their grid cell instead, because their last
 * pixel geometry may predate a resize of the figure.
 *
 * Free members that were never part of the grid are left untouched, so a figure that
 * mixes both kinds ends up with free members only.
 *
 * @return true when a grid was enabled and has been disabled, false when no grid was
 *         enabled (repeated calls therefore do nothing and report false)
 *
 * @note The track ratios and the grid spacing are dropped and the grid member
 *       registrations are cleared: this is not reversible. Enabling the grid again
 *       requires the row/column counts and the desired ratios to be supplied again
 *       (see enableGrid()).
 * @note The divider interaction is switched off; call setGridEditable(true) again
 *       after re-enabling the grid.
 * @sa enableGrid(), QwtFigureGrid::disable()
 */
bool QwtFigure::disableGrid()
{
    if (!isGridEnabled()) {
        return false;
    }

    QwtFigureGrid* g                = m_data->grid.data();
    QwtFigureLayout* lay            = qobject_cast< QwtFigureLayout* >(layout());
    const QRect figureRect          = rect();
    const QList< QWidget* > members = g->items();
    for (QWidget* w : qwt_as_const(members)) {
        if (!w) {
            continue;
        }
        // Prefer the geometry currently on screen: freezing the very same pixels keeps
        // the layout stable when the grid disappears.
        QRectF normRect;
        if (lay && w->isVisibleTo(this)) {
            normRect = QwtFigureLayout::calcNormRect(figureRect, w->geometry());
        }
        if (!normRect.isValid()) {
            // Hidden member, unlaid figure or empty geometry: fall back to its grid cell,
            // which is the normalized rectangle the layout would produce anyway.
            int row = 0, col = 0, rowSpan = 0, colSpan = 0;
            if (g->gridPosition(w, &row, &col, &rowSpan, &colSpan)) {
                normRect = g->normRectFor(row, col, rowSpan, colSpan);
            }
        }
        if (lay && normRect.isValid()) {
            lay->setAxesNormPos(w, normRect);
        }
    }

    // Stop the divider interaction before the tracks disappear: the splitter holds
    // event filters and mouse grabs on the members.
    if (m_data->gridSplitter) {
        m_data->gridSplitter->setEnabled(false);
    }
    g->disable();

    if (lay) {
        lay->invalidate();
        lay->setGeometry(figureRect);
    }
    // The members are free members now: the automatic grid alignment is gone,
    // only manual configurations keep their plot areas aligned.
    recomputeAxisAlignment(false);
    update();
    return true;
}

/**
 * @brief Get the retained grid model
 * @return Pointer to the grid, or nullptr if enableGrid() was never called
 */
QwtFigureGrid* QwtFigure::grid() const
{
    return m_data->grid.data();
}

/**
 * @brief Check whether the retained grid layout is enabled
 * @return true if a grid was set up and is enabled
 */
bool QwtFigure::isGridEnabled() const
{
    return m_data->grid && m_data->grid->isEnabled();
}

/**
 * @brief Add a plot as a member of the retained grid layout
 * @details The plot keeps its grid cell when the figure is resized. The free normalized
 *          rectangle stored for it is only a snapshot used if the grid is unavailable.
 * @param plot QwtPlot to add
 * @param row Grid row (0-based)
 * @param col Grid column (0-based)
 * @param rowSpan Number of rows to span (default 1)
 * @param colSpan Number of columns to span (default 1)
 * @note enableGrid() must be called before adding grid members.
 * @note This function emits axesAdded, followed by currentAxesChanged.
 * @sa addGridAxes, addGridItem(QWidget*, int, int, int, int)
 */
void QwtFigure::addAxes(QwtPlot* plot, int row, int col, int rowSpan, int colSpan)
{
    if (!plot) {
        qWarning() << "QwtFigure::addAxes: null plot";
        return;
    }
    if (!isGridEnabled()) {
        qWarning() << "QwtFigure::addAxes: enableGrid() must be called before adding grid members";
        return;
    }
    if (hasAxes(plot)) {
        qWarning() << "QwtFigure::addAxes: plot is already part of the figure";
        return;
    }
    if (!m_data->grid->addItem(plot, row, col, rowSpan, colSpan)) {
        return;  // QwtFigureGrid already reported the invalid position/span
    }
    const QRectF rect = m_data->grid->normRectFor(row, col, rowSpan, colSpan);
    addWidget(plot, rect.x(), rect.y(), rect.width(), rect.height());
    recomputeAxisAlignment(false);

    Q_EMIT axesAdded(plot);
    setCurrentAxes(plot);
}

/**
 * @brief Add a non-plot widget as a member of the retained grid layout
 * @details Typical usage is a 3D plot widget or any custom widget that should share the
 *          grid tracks with the plots. No axes signals are emitted.
 * @param widget Widget to add
 * @param row Grid row (0-based)
 * @param col Grid column (0-based)
 * @param rowSpan Number of rows to span (default 1)
 * @param colSpan Number of columns to span (default 1)
 * @return true if the widget was registered, false for invalid arguments
 * @note enableGrid() must be called before adding grid members.
 * @note This is not an addWidget() overload on purpose: a 5 argument addWidget() call
 *       would be ambiguous with the legacy addWidget(widget, rowCnt, colCnt, row, col).
 */
bool QwtFigure::addGridItem(QWidget* widget, int row, int col, int rowSpan, int colSpan)
{
    if (!widget) {
        qWarning() << "QwtFigure::addGridItem: null widget";
        return false;
    }
    if (!isGridEnabled()) {
        qWarning() << "QwtFigure::addGridItem: enableGrid() must be called before adding grid members";
        return false;
    }
    if (!m_data->grid->addItem(widget, row, col, rowSpan, colSpan)) {
        return false;  // QwtFigureGrid already reported the invalid position/span
    }
    const QRectF rect = m_data->grid->normRectFor(row, col, rowSpan, colSpan);
    addWidget(widget, rect.x(), rect.y(), rect.width(), rect.height());
    return true;
}

/**
 * @brief Remove a widget from the figure without deleting it
 * @details Plots are removed through takeAxes(), which also updates the current axes and
 *          the parasite axes. Other widgets are unregistered from the layout and the grid,
 *          hidden and re-parented to nullptr.
 * @param widget Widget to remove
 * @note The caller keeps ownership of the widget.
 */
void QwtFigure::removeWidget(QWidget* widget)
{
    if (!widget) {
        qWarning() << "QwtFigure::removeWidget: null widget";
        return;
    }

    if (QwtPlot* plot = qobject_cast< QwtPlot* >(widget)) {
        if (hasAxes(plot)) {
            takeAxes(plot);
            plot->hide();
            plot->setParent(nullptr);
            return;
        }
    }

    if (m_data->grid) {
        m_data->grid->removeItem(widget);
    }
    m_data->unwatchWidget(widget);

    QLayout* lay = layout();
    if (lay) {
        for (int i = 0; i < lay->count(); ++i) {
            QLayoutItem* item = lay->itemAt(i);
            if (item && item->widget() == widget) {
                lay->removeItem(item);
                delete item;
                break;
            }
        }
        lay->invalidate();
    }
    if (widget->parentWidget() == this) {
        widget->hide();
        widget->setParent(nullptr);
    }
    m_data->updateGridSplitter();
}

/**
 * @brief Enable or disable dragging the dividers of the retained grid
 * @details The grid splitter is created on the first enable and stays owned by the
 *          figure afterwards. Enabling requires a retained grid, otherwise the call is
 *          ignored with a warning and isGridEditable() keeps returning false.
 *
 *          While editing is enabled the mouse events in a narrow band around the grid
 *          dividers are consumed to resize the adjacent tracks; all other events are
 *          passed through unchanged.
 * @param on True to make the dividers draggable
 * @note A divider drag previews with a virtual boundary line by default, without
 *       relaying out or repainting the members; the splitter returned by
 *       findChild<QwtFigureGridSplitter*>() selects the live relayout/replot with
 *       QwtFigureGridSplitter::setDragPreviewMode().
 * @note Disable the FunResizePlot mode of QwtFigureWidgetOverlay (or make the overlay
 *       transparent for mouse events) while the grid is editable: the overlay covers
 *       the whole figure and would swallow the events of the plots.
 * @note Qt invokes the event filters of a widget in reverse installation order: the
 *       filter installed last runs first. The splitter filters must therefore be
 *       installed last - install the filters of the host (canvas pickers, drag
 *       handlers, ...) before calling setGridEditable(true). When the host installs
 *       a filter on a canvas afterwards, call setGridEditable(true) again: enabling
 *       an already enabled splitter re-scans and re-installs its filters, which puts
 *       them back in front of the host filters.
 * @sa isGridEditable(), enableGrid(), QwtFigureGridSplitter
 */
void QwtFigure::setGridEditable(bool on)
{
    if (on && !isGridEnabled()) {
        qWarning() << "QwtFigure::setGridEditable: enableGrid() must be called before editing the grid";
        return;
    }

    if (on) {
        if (!m_data->gridSplitter) {
            m_data->gridSplitter = new QwtFigureGridSplitter(this);
        }
        m_data->gridSplitter->setEnabled(true);
        return;
    }

    if (m_data->gridSplitter) {
        m_data->gridSplitter->setEnabled(false);
    }
}

/**
 * @brief Check whether the grid dividers can be dragged
 * @return True when grid editing is enabled
 */
bool QwtFigure::isGridEditable() const
{
    return m_data->gridSplitter && m_data->gridSplitter->isEnabled();
}

/**
 * @brief Change the normalized position of an already added widget. If the widget hasn't been added yet, this function has no effect.
 * @param widget
 * @param rect
 */
void QwtFigure::setWidgetNormPos(QWidget* widget, const QRectF& rect)
{
    QWTFIGURE_SAFEGET_LAY(lay)
    lay->setAxesNormPos(widget, rect);
    lay->invalidate();
}

/**
 * @brief Get all axes (plots) in the figure (not including parasite axes)
 * @details This method returns a list of all QwtPlot objects added to the figure.
 * @param byZOrder If true, sort by z-order from top to bottom
 * @return List of all QwtPlot objects (not including parasite axes)
 *
 * @note Parasite axes are not included in the returned list
 */
QList< QwtPlot* > QwtFigure::allAxes(bool byZOrder) const
{
    QList< QwtPlot* > plots;
    QLayout* lay = layout();
    if (lay) {
        for (int i = 0; i < lay->count(); ++i) {
            QLayoutItem* item = lay->itemAt(i);
            if (item && item->widget()) {
                if (QwtPlot* plot = qobject_cast< QwtPlot* >(item->widget())) {
                    if (plot->isHostPlot()) {
                        plots.append(plot);
                    }
                }
            }
        }
    }
    if (!byZOrder || plots.isEmpty()) {
        return plots;  // Return in original order
    }

    /* Reorder by z-order from high to low (later children have higher z) */
    const QObjectList& oc = children();
    QList< QwtPlot* > zOrdered;
    zOrdered.reserve(plots.size());

    // Scan in reverse order, move matching items
    for (auto it = oc.crbegin(); it != oc.crend(); ++it) {
        if (QwtPlot* p = qobject_cast< QwtPlot* >(*it)) {
            if (plots.contains(p)) {  // O(n) small list lookup
                zOrdered.append(p);
            }
        }
    }

    return zOrdered;  // Already sorted from top to bottom
}

/**
 * @brief Check if the figure has any axes
 * @details This method returns true if the figure contains at least one QwtPlot.
 * @return true if the figure has axes, false otherwise
 */
bool QwtFigure::hasAxes() const
{
    QLayout* lay = layout();
    if (!lay) {
        return false;
    }

    for (int i = 0; i < lay->count(); ++i) {
        QLayoutItem* item = lay->itemAt(i);
        if (item && item->widget() && qobject_cast< QwtPlot* >(item->widget())) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Check if the figure contains a specific plot
 * @details This method returns true if the figure contains the specified QwtPlot.
 * @param plot QwtPlot to check
 * @return true if the figure contains the plot, false otherwise
 */
bool QwtFigure::hasAxes(QwtPlot* plot) const
{
    QLayout* lay = layout();
    if (!lay || !plot) {
        return false;
    }

    for (int i = 0; i < lay->count(); ++i) {
        QLayoutItem* item = lay->itemAt(i);
        if (item) {
            if (QwtPlot* ax = qobject_cast< QwtPlot* >(item->widget())) {
                if (ax == plot) {
                    return true;
                }
            }
        }
    }
    return false;
}

/**
 * @brief Remove a specific axes (plot) from the figure
 * @details This method removes the specified QwtPlot from the figure.
 * @param plot QwtPlot to remove
 *
 * @note This function does not destroy the QwtPlot object. You need to call deleteLater() manually.
 */
void QwtFigure::removeAxes(QwtPlot* plot)
{
    takeAxes(plot);
}

/**
 * @brief Take a specific axes (plot) from the figure without deleting it
 * @param plot Pointer to the QwtPlot to take
 * @return true if successfully taken, false otherwise
 *
 * @note If the removed plot is the current active axes, currentAxesChanged signal is emitted first, then axesRemoved signal.
 * @note If the figure has no plots after removal, currentAxesChanged signal is emitted with nullptr.
 * @note If a plot has parasite axes, they will be hidden and have parent widget set to nullptr.
 */
bool QwtFigure::takeAxes(QwtPlot* plot)
{
    if (!plot) {
        return false;
    }

    // Remove from layout
    bool isRemove = false;
    // Check if the plot to remove is the current axes
    bool removingCurrent = (plot == currentAxes());
    m_data->plotWillRemove(plot);
    m_data->unwatchWidget(plot);
    if (m_data->grid) {
        m_data->grid->removeItem(plot);
    }
    QLayout* lay = layout();
    if (lay) {
        for (int i = 0; i < lay->count(); ++i) {
            QLayoutItem* item = lay->itemAt(i);
            if (!item) {
                continue;
            }
            QWidget* w = item->widget();
            if (!w) {
                continue;
            }
            if (w == plot) {
                lay->removeItem(item);
                delete item;
                isRemove = true;
                break;
            }
        }
        if (removingCurrent) {
            // The current axes was removed, need to update currentAxes
            const int count = lay->count();
            if (count == 0) {
                // If the figure is empty, emit currentAxesChanged with nullptr
                setCurrentAxes(nullptr);
            } else {
                for (int i = 0; i < count; ++i) {
                    QLayoutItem* item = lay->itemAt(i);
                    if (!item) {
                        continue;
                    }
                    if (QwtPlot* w = qobject_cast< QwtPlot* >(item->widget())) {
                        setCurrentAxes(w);
                    }
                }
            }
        }
    }
    if (isRemove) {
        // Handle parasite axes
        const QList< QwtPlot* > parasites = plot->parasitePlots();
        for (QwtPlot* para : parasites) {
            m_data->unwatchWidget(para);
            para->setParent(nullptr);
            para->hide();
        }
        if (lay) {
            lay->invalidate();
        }
        // The plot keeps its own content but leaves the alignment groups of the
        // figure; the remaining members may get smaller shared insets.
        m_data->releaseAxisAlignment(plot);
        recomputeAxisAlignment(false);
        Q_EMIT axesRemoved(plot);
    }
    m_data->updateGridSplitter();
    return isRemove;
}

/**
 * @brief Clear all axes from the figure
 * @details This method removes all QwtPlot objects from the figure and deletes them.
 *
 * @note This method emits axesRemoved signal during removal process.
 * @note After all removals, currentAxesChanged signal is emitted with nullptr, then figureCleared signal.
 * @note This method deletes all held plot widgets.
 */
void QwtFigure::clear()
{
    // Remove from layout
    QLayout* lay  = layout();
    int removeCnt = 0;
    if (lay) {
        // lay->count() cannot be placed inside the for loop; it changes with each iteration
        const int itemCnt = lay->count();
        // Delete widgets first, then remove items uniformly. Do not call removeItem inside this loop
        // or the list size will change with each iteration and prevent normal traversal.
        for (int i = 0; i < itemCnt; ++i) {
            QLayoutItem* item = lay->itemAt(i);
            if (item) {
                if (QwtPlot* plot = qobject_cast< QwtPlot* >(item->widget())) {
                    Q_EMIT axesRemoved(plot);
                }
                if (QWidget* w = item->widget()) {
                    m_data->unwatchWidget(w);
                    if (m_data->grid) {
                        m_data->grid->removeItem(w);
                    }
                    w->hide();
                    w->deleteLater();
                }
                ++removeCnt;
            }
        }
        // Finally, remove items uniformly
        for (int i = 0; i < itemCnt; ++i) {
            QLayoutItem* item = lay->itemAt(i);
            lay->removeItem(item);
            delete item;
        }
    }
    setCurrentAxes(nullptr);
    // The members are deleted with deleteLater(): drop the alignment bookkeeping
    // without touching them again.
    m_data->appliedInsets.clear();
    if (removeCnt > 0) {
        Q_EMIT figureCleared();
    }
    m_data->updateGridSplitter();
}

/**
 * @brief Get the size of the figure in inches
 * @details This method calculates the physical size of the figure in inches based on
 *          the current pixel size and screen DPI.
 * @return Size of the figure in inches
 */
QSize QwtFigure::getSizeInches() const
{
    QScreen* screen = QGuiApplication::primaryScreen();
    int dpi         = screen ? screen->logicalDotsPerInch() : 96;

    QSize size = this->size();
    return QSize(size.width() / dpi, size.height() / dpi);
}

/**
 * @brief Set the size of the figure in inches
 * @details This method sets the size of the figure in inches, converting to pixels
 *          based on the screen DPI.
 * @param width Width in inches
 * @param height Height in inches
 */
void QwtFigure::setSizeInches(float width, float height)
{
    QScreen* screen = QGuiApplication::primaryScreen();
    int dpi         = screen ? screen->logicalDotsPerInch() : 96;

    int pixelWidth  = width * dpi;
    int pixelHeight = height * dpi;

    resize(pixelWidth, pixelHeight);
}

/**
 * @brief Set the size of the figure in inches
 * @details This method sets the size of the figure in inches, converting to pixels
 *          based on the screen DPI.
 * @param size Size in inches
 */
void QwtFigure::setSizeInches(const QSizeF& size)
{
    setSizeInches(size.width(), size.height());
}

/**
 * @brief Set the face color of the figure
 * @details This method sets the background color of the figure.
 * @param color Background color
 */
void QwtFigure::setFaceColor(const QColor& color)
{
    m_data->faceBrush = color;
}

/**
 * @brief Get the face color of the figure
 * @details This method returns the background color of the figure.
 * @return Background color
 */
QColor QwtFigure::faceColor() const
{
    return m_data->faceBrush.color();
}

/**
 * @brief Set the face brush of the figure
 * @details This method sets the background brush of the figure, allowing for
 *          more complex backgrounds (gradients, textures, etc.).
 * @param brush Background brush
 */
void QwtFigure::setFaceBrush(const QBrush& brush)
{
    m_data->faceBrush = brush;
}

/**
 * @brief Get the face brush of the figure
 * @details This method returns the background brush of the figure.
 * @return Background brush
 */
QBrush QwtFigure::faceBrush() const
{
    return m_data->faceBrush;
}

/**
 * @brief Set the edge color of the figure
 * @details This method sets the border color of the figure.
 * @param color Border color
 */
void QwtFigure::setEdgeColor(const QColor& color)
{
    m_data->edgeColor = color;
}

/**
 * @brief Get the edge color of the figure
 * @details This method returns the border color of the figure.
 * @return Border color
 */
QColor QwtFigure::edgeColor() const
{
    return m_data->edgeColor;
}

/**
 * @brief Set the edge line width of the figure
 * @details This method sets the border line width of the figure.
 * @param width Border line width in pixels
 */
void QwtFigure::setEdgeLineWidth(int width)
{
    m_data->edgeLineWidth = width;
}

/**
 * @brief Get the edge line width of the figure
 * @details This method returns the border line width of the figure.
 * @return Border line width in pixels
 */
int QwtFigure::edgeLineWidth() const
{
    return m_data->edgeLineWidth;
}

/**
 * @brief Create parasite axes for a host plot
 * @details This method creates a parasite axes that shares the same plotting area as the host plot
 *          but with independent axis scaling and labeling. The parasite axes will be positioned
 *          exactly on top of the host plot and will automatically synchronize its geometry.
 * @param hostPlot Pointer to the host QwtPlot
 * @param enableAxis The axis position to enable on the parasite axes
 * @return Pointer to the created parasite QwtPlot
 * @retval nullptr if hostPlot is invalid or not in the figure
 * @note The parasite axes will have a transparent background and only the specified axis will be visible.
 * @note The parasite axes will automatically be deleted when the host plot is removed from the figure.
 * @note Parasitic axes are not stored in QwtFigureLayout, but are separately controlled by QwtFigure for layout management.
 * @note Parasite axes must be managed by the figure because they only overlap the plotting area with the host, while the coordinate window positions are different from the host.
 */
QwtPlot* QwtFigure::createParasiteAxes(QwtPlot* hostPlot, QwtAxis::Position enableAxis)
{
    if (!hostPlot || !hasAxes(hostPlot)) {
        qWarning() << "Invalid host plot or host plot not in figure";
        return nullptr;
    }
    if (hostPlot->isParasitePlot()) {
        // Not a host plot, switch to the host
        hostPlot = hostPlot->hostPlot();
    }
    // Create parasite axes
    QwtPlot* parasitePlot = hostPlot->createParasitePlot(enableAxis);
    if (parasitePlot) {
        // A press on the parasite axes activates its host plot.
        m_data->watchWidget(parasitePlot);
        m_data->updateGridSplitter();
    }

    return parasitePlot;
}

/**
 * @brief Get all parasite axes for a host plot
 * @details This method returns a list of all parasite axes associated with the specified host plot.
 * @param hostPlot Pointer to the host QwtPlot
 * @return List of parasite QwtPlot pointers
 * @retval Empty list if hostPlot is invalid or has no parasite axes
 */
QList< QwtPlot* > QwtFigure::getParasiteAxes(QwtPlot* hostPlot) const
{
    if (!hostPlot) {
        return QList< QwtPlot* >();
    }
    return hostPlot->parasitePlots();
}

/**
 * @brief Save the figure to a QPixmap with specified DPI
 * @details This method renders the figure to a QPixmap with the specified DPI.
 *          If DPI is -1, the current screen DPI is used.
 * @param dpi Dots per inch for the saved image (-1 to use screen DPI)
 * @return QPixmap containing the rendered figure
 */
QPixmap QwtFigure::saveFig(int dpi) const
{
    // Calculate the target size based on DPI
    QSize targetSize;
    int targetDpi;

    if (dpi <= 0) {
        QScreen* screen = QGuiApplication::primaryScreen();
        targetDpi       = screen ? screen->logicalDotsPerInch() : 96;
        targetSize      = size();
    } else {
        targetDpi                 = dpi;
        QSizeF physicalSizeInches = getSizeInches();
        targetSize                = QSize(static_cast< int >(physicalSizeInches.width() * dpi),
                           static_cast< int >(physicalSizeInches.height() * dpi));
    }

    // Use const_cast to call non-const methods
    QwtFigure* nonConstThis = const_cast< QwtFigure* >(this);

    // The exported image must show the aligned plot areas, even when the
    // alignment was not applied since the last content change (f.e. a plot
    // that was never shown/relaid out before the export).
    nonConstThis->recomputeAxisAlignment(false);

    if (dpi <= 0) {
        // No scaling needed, just grab the current state
        return nonConstThis->grab();
    }
    // Create pixmap with target size
    QPixmap pixmap(targetSize);

    // Use QPainter for high-quality scaling
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Calculate scaling factors
    qreal scaleX = static_cast< qreal >(targetSize.width()) / width();
    qreal scaleY = static_cast< qreal >(targetSize.height()) / height();
    painter.scale(scaleX, scaleY);

    // Render the figure with scaling
    nonConstThis->render(&painter);
    painter.end();

    // Set DPI information if needed
    // When you set DPI information in an image file, image processing software (such as Photoshop, GIMP, etc.)
    // and printers will know how to correctly interpret the physical size of the image.
    // Without DPI information, software typically uses a default DPI (usually 72 or 96), which can
    // lead to incorrect physical size calculations. Different devices and software may have different
    // default DPI settings. Setting DPI explicitly ensures consistent physical dimensions across platforms.
    QImage image = pixmap.toImage();
    // Convert DPI to dots per meter (1 inch = 2.54 cm, so 1 meter = 100/2.54 inches)
    image.setDotsPerMeterX(targetDpi * 100 / 2.54);
    image.setDotsPerMeterY(targetDpi * 100 / 2.54);
    return QPixmap::fromImage(image);
}

/**
 * @brief Save the figure to a QPixmap with specified size in inches
 * @details This method renders the figure to a QPixmap with the specified physical size in inches.
 *          The current DPI setting of the figure is used to calculate the pixel size.
 * @param inchesSize Physical size in inches
 * @return QPixmap containing the rendered figure
 */
QPixmap QwtFigure::saveFig(QSizeF& inchesSize) const
{
    // Use current DPI to calculate target pixel size
    QScreen* screen = QGuiApplication::primaryScreen();
    int currentDpi  = screen ? screen->logicalDotsPerInch() : 96;
    QSize targetSize(static_cast< int >(inchesSize.width() * currentDpi),
                     static_cast< int >(inchesSize.height() * currentDpi));

    // Use const_cast to call non-const methods
    QwtFigure* nonConstThis = const_cast< QwtFigure* >(this);

    // Align the plot areas before the figure is rendered (see saveFig(int))
    nonConstThis->recomputeAxisAlignment(false);

    // Create pixmap with target size
    QPixmap pixmap(targetSize);

    // Use QPainter for high-quality scaling
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Calculate scaling factors
    qreal scaleX = static_cast< qreal >(targetSize.width()) / width();
    qreal scaleY = static_cast< qreal >(targetSize.height()) / height();
    painter.scale(scaleX, scaleY);

    // Render the figure with scaling
    nonConstThis->render(&painter);
    painter.end();

    // Set DPI information
    QImage image = pixmap.toImage();
    image.setDotsPerMeterX(currentDpi * 100 / 2.54);
    image.setDotsPerMeterY(currentDpi * 100 / 2.54);
    return QPixmap::fromImage(image);
}

/**
 * @brief Save the figure to a file with specified DPI
 * @details This method saves the figure to an image file with the specified DPI.
 * @param filename Name of the file to save
 * @param dpi Dots per inch for the saved image (-1 to use screen DPI)
 * @return true if saved successfully, false otherwise
 */
bool QwtFigure::saveFig(const QString& filename, int dpi) const
{
    QPixmap pixmap = saveFig(dpi);
    return pixmap.save(filename, nullptr, -1);
}

/**
 * @brief Set the current axes (plot)
 * @details This method sets the specified QwtPlot as the current active axes in the figure.
 * @param plot QwtPlot to set as current
 */
void QwtFigure::setCurrentAxes(QwtPlot* plot)
{
    // Allow setting to nullptr, or set only when the plot belongs to this figure
    if (plot == nullptr || hasAxes(plot)) {
        m_data->currentAxes = plot;
        Q_EMIT currentAxesChanged(plot);
    }
}

/**
 * @brief Set the current axes (plot)
 * @details This is a convenience method that calls setCurrentAxes.
 * @param plot QwtPlot to set as current
 */
void QwtFigure::sca(QwtPlot* plot)
{
    setCurrentAxes(plot);
}

/**
 * @brief Get the current axes (plot)
 * @details This method returns the current active QwtPlot in the figure.
 *          The current axes is typically the last axes that was added, modified, or plotted on.
 * @return Pointer to the current QwtPlot, or nullptr if no axes exist
 */
QwtPlot* QwtFigure::currentAxes() const
{
    return m_data->currentAxes;
}

/**
 * @brief Get the current axes (plot)
 * @details This is a convenience method that calls currentAxes.
 * @return Pointer to the current QwtPlot, or nullptr if no axes exist
 */
QwtPlot* QwtFigure::gca() const
{
    return currentAxes();
}

/**
 * @brief Get the normalized rectangle for a axes
 * @details This method returns the normalized coordinates [0,1] for the specified axes
 *          in the figure. If the axes is not found in the figure, an invalid QRectF is returned.
 * @param widget Widget to query
 * @return Normalized coordinates [left, top, width, height] in range [0,1], or invalid QRectF if not found
 */
QRectF QwtFigure::axesNormRect(QwtPlot* plot) const
{
    QWTFIGURE_SAFEGET_LAY_RET(lay, QRect())
    return lay->widgetNormRect(plot);
}

/**
 * @brief Get the normalized rectangle for a widget
 * @details This method returns the normalized coordinates [0,1] for the specified axes
 *          in the figure. If the widget is not found in the figure, an invalid QRectF is returned.
 * @param widget Widget to query
 * @return Normalized coordinates [left, top, width, height] in range [0,1], or invalid QRectF if not found
 */
QRectF QwtFigure::widgetNormRect(QWidget* w) const
{
    QWTFIGURE_SAFEGET_LAY_RET(lay, QRect())
    return lay->widgetNormRect(w);
}

/**
 * @brief Get the plot under a position
 * @details This method returns the QwtPlot under the specified position.
 *          If no plot is found, nullptr is returned. Hidden windows are not considered.
 * @param pos Position to query
 * @return Pointer to the QwtPlot under the position, or nullptr if not found
 */
QwtPlot* QwtFigure::plotUnderPos(const QPoint& pos) const
{
    const QList< QwtPlot* > result = findChildren< QwtPlot* >(QString(), Qt::FindDirectChildrenOnly);
    if (result.empty()) {
        return nullptr;
    }
    for (QwtPlot* plot : result) {
        if (!(plot->isVisibleTo(this))) {
            continue;
        }
        // Check if the child widget region contains the point
        if (plot->geometry().contains(pos)) {
            return plot;
        }
    }
    return nullptr;
}

/**
 * @brief Calculate normalized coordinates from actual window coordinates
 * @details This method converts actual window coordinates (geometry) to normalized coordinates.
 * @param geoRect Actual window coordinates (geometry)
 * @return Normalized coordinates QRectF
 */
QRectF QwtFigure::calcNormRect(const QRect& geoRect) const
{
    return QwtFigureLayout::calcNormRect(rect(), geoRect);
}

/**
 * @brief Calculate actual rectangle from normalized coordinates
 * @details This method converts normalized coordinates to actual window coordinates.
 * @param normRect Normalized coordinates QRectF
 * @return Actual window coordinates QRect
 */
QRect QwtFigure::calcActualRect(const QRectF& normRect)
{
    QWTFIGURE_SAFEGET_LAY_RET(lay, QRect())
    return lay->calcActualRect(rect(), normRect);
}

/**
 * @brief Update all plots in the figure
 * @details This method calls replot on all plots in the figure.
 */
void QwtFigure::replotAll()
{
    const QList< QwtPlot* > plots = allAxes();
    for (QwtPlot* plot : plots) {
        plot->replotAll();
    }
    // A replot may have changed the tick label dimensions: refresh the aligned
    // plot areas. No replot is needed here, the canvas maps are read live from
    // the widget geometries.
    recomputeAxisAlignment(false);
}

/**
 * @brief Recompute the plot area alignment of the figure
 * @details The pass measures the natural canvas insets of the plots that take
 *          part in the alignment (the members of the retained grid and the plots
 *          of the manual configurations) and raises them to the largest inset of
 *          their group. An inset is the distance between the plot widget and the
 *          plot area: aligning it aligns the axis backbones, the tick marks and
 *          the plotted data of the sub plots, independent of the width of their
 *          tick labels, titles, legends or parasite axes.
 *
 *          The pass is idempotent and only relayouts the plots whose insets
 *          changed, so it can be called on every figure layout.
 * @param replot True to replot the plots after the alignment was applied
 * @sa setGridAxisAlignment(), addAxisAlignment()
 */
void QwtFigure::applyAllAxisAlignments(bool replot)
{
    recomputeAxisAlignment(replot);
}

void QwtFigure::recomputeAxisAlignment(bool replot)
{
    m_data->applyAxisAlignment(this, replot);
}

/**
 * @brief Enable or disable the automatic plot area alignment of the grid
 * @details When enabled (the default), the members of the retained grid share
 *          the canvas insets of their column (left and right edge) and of their
 *          row (top and bottom edge): the axis backbones of the sub plots line
 *          up even when their tick labels have very different widths. The insets
 *          take all decorations of a plot area into account, including parasite
 *          axes and legends.
 *
 *          Disabling the alignment restores the natural plot area of the
 *          members. Manual configurations (addAxisAlignment()) are not affected.
 * @param on True to align the retained grid members automatically
 * @sa isGridAxisAlignment(), enableGrid()
 */
void QwtFigure::setGridAxisAlignment(bool on)
{
    if (m_data->gridAxisAlignment == on) {
        return;
    }
    m_data->gridAxisAlignment = on;
    recomputeAxisAlignment(false);
}

/**
 * @brief Check whether the retained grid members are aligned automatically
 * @return True when the automatic grid alignment is enabled (the default)
 * @sa setGridAxisAlignment()
 */
bool QwtFigure::isGridAxisAlignment() const
{
    return m_data->gridAxisAlignment;
}

/**
 * @brief Add axis alignment configuration
 * @details The plots of one configuration get the same canvas inset on the
 *          given side, using the largest inset its content asks for. This aligns
 *          the axis backbone (the plot area edge) of the plots, which is useful
 *          for figures whose members are not part of a retained grid.
 *
 *          Overlapping configurations of the same side are merged, all their
 *          plots end up with the same inset.
 * @param plots List of plots to align
 * @param axisId Side to align (QwtAxis::XTop/XBottom/YLeft/YRight)
 * @note The members of the retained grid are aligned automatically already,
 *       unless setGridAxisAlignment() is switched off.
 * @sa removeAxisAlignment(), clearAxisAlignment(), alignAxes()
 */
void QwtFigure::addAxisAlignment(const QList< QwtPlot* >& plots, int axisId)
{
    if (plots.isEmpty() || !QwtAxis::isValid(axisId)) {
        return;
    }

    // Filter out plots not in the current figure (parasite plots share the
    // canvas of their host, which is aligned instead)
    QList< QwtPlot* > validPlots;
    for (QwtPlot* plot : qwt_as_const(plots)) {
        if (isAlignablePlot(plot) && hasAxes(plot)) {
            validPlots.append(plot);
        }
    }

    if (validPlots.isEmpty()) {
        return;
    }

    // Adding the same configuration twice (f.e. a layout helper that is called
    // on every replot) must not accumulate redundant groups.
    for (const PrivateData::AlignmentConfig& existing : qwt_as_const(m_data->alignmentConfigs)) {
        if (existing.axisId == axisId && existing.plots == validPlots) {
            return;
        }
    }

    // Add to configuration list
    PrivateData::AlignmentConfig config;
    config.plots  = validPlots;
    config.axisId = axisId;
    m_data->alignmentConfigs.append(config);

    recomputeAxisAlignment(false);
}

/**
 * @brief Remove specified axis alignment configuration
 * @details This method removes the alignment configuration for the specified plots and axis.
 * @param plots List of plots to remove from alignment
 * @param axisId Axis ID to remove from alignment
 * @return true if successfully removed, false otherwise
 */
bool QwtFigure::removeAxisAlignment(const QList< QwtPlot* >& plots, int axisId)
{
    if (plots.isEmpty() || !QwtAxis::isValid(axisId)) {
        return false;
    }

    bool removed = false;
    auto it      = m_data->alignmentConfigs.begin();
    while (it != m_data->alignmentConfigs.end()) {
        if (it->axisId == axisId && it->plots == plots) {
            it      = m_data->alignmentConfigs.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }

    if (removed) {
        recomputeAxisAlignment(false);
    }
    return removed;
}

/**
 * @brief Clear all axis alignment configurations
 * @details This method removes all alignment configurations from the figure.
 *          The plots that were only aligned by a configuration return to their
 *          natural plot area; the automatic alignment of the retained grid
 *          members stays active.
 */
void QwtFigure::clearAxisAlignment()
{
    if (m_data->alignmentConfigs.isEmpty()) {
        return;
    }
    m_data->alignmentConfigs.clear();
    recomputeAxisAlignment(false);
}

/**
 * @brief Apply alignment configurations for a specific axis
 * @details This method applies all alignment configurations for the specified axis ID.
 *          The alignment pass works on all sides at once, so the configurations
 *          of the other axes are refreshed as well.
 * @param axisId Axis ID to apply alignments for
 */
void QwtFigure::applyAlignmentsForAxis(int axisId)
{
    if (!QwtAxis::isValid(axisId)) {
        return;
    }
    recomputeAxisAlignment(false);
}

/**
 * @brief Get the number of axis alignment configurations
 * @details This method returns the count of alignment configurations added via addAxisAlignment.
 * @return Number of alignment configurations
 */
int QwtFigure::axisAligmentCount() const
{
    return m_data->alignmentConfigs.size();
}

/**
 * @brief Get axis alignment information by index
 * @details This method returns the alignment configuration at the specified index.
 * @param index Index of the alignment configuration to retrieve
 * @return QPair containing the plot list and axis ID
 */
QPair< QList< QwtPlot* >, int > QwtFigure::axisAligmentInfo(int index) const
{
    if (index >= axisAligmentCount() || index < 0) {
        return {};
    }
    auto ali = m_data->alignmentConfigs.value(index);
    return qMakePair(ali.plots, ali.axisId);
}

/**
 * @brief Align the plot area of multiple plots on one side
 * @details The canvas inset of the given side is measured for every plot and
 *          the largest one is applied to all of them, which puts the axis
 *          backbones on one line. Compared to manipulating the scale widgets
 *          this also covers the space taken by titles, legends and parasite
 *          axes and works for hidden axes as well.
 * @param plots List of QwtPlot to align (must be non-empty)
 * @param axisId Side to align (QwtAxis::XTop/XBottom/YLeft/YRight)
 * @param update If true, update the layout and replot after the alignment
 * @note Parasite axes are shared with their host and do not need to be passed.
 */
void QwtFigure::alignAxes(QList< QwtPlot* > plots, int axisId, bool update)
{
    if (plots.isEmpty() || !QwtAxis::isValid(axisId)) {
        return;
    }

    qreal maxInset = 0.0;
    QList< QwtPlot* > validPlots;
    for (QwtPlot* plot : qwt_as_const(plots)) {
        if (!isAlignablePlot(plot) || !plot->plotLayout()) {
            continue;
        }

        qreal inset = 0.0;
        if (!naturalCanvasInset(plot, axisId, &inset)) {
            continue;
        }

        // Keep a larger inset that is already in effect (f.e. from the figure
        // alignment of a grid column) so an external group never undoes it.
        maxInset = qMax(maxInset, inset);
        maxInset = qMax(maxInset, plot->plotLayout()->minCanvasInset(axisId));
        validPlots.append(plot);
    }

    if (validPlots.size() < 2) {
        return;
    }

    for (QwtPlot* plot : qwt_as_const(validPlots)) {
        plot->plotLayout()->setMinCanvasInset(axisId, maxInset);
        if (update) {
            plot->updateLayout();
            plot->replot();
        }
    }
}

void QwtFigure::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw background
    painter.fillRect(rect(), m_data->faceBrush);

    // Draw border
    if (m_data->edgeLineWidth > 0) {
        QPen pen(m_data->edgeColor);
        pen.setWidth(m_data->edgeLineWidth);
        painter.setPen(pen);
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }

    QFrame::paintEvent(event);
}

void QwtFigure::resizeEvent(QResizeEvent* event)
{
    // The alignment itself is refreshed at the end of QwtFigureLayout::setGeometry(),
    // where the members already have their new geometry.
    QFrame::resizeEvent(event);
}

/**
 * @brief Queue one merged alignment pass
 * @details A plot requests a new layout whenever its axis labels, fonts or
 *          legend change. Several requests can arrive in one event loop
 *          iteration (f.e. from a replot of every member); the queued pass
 *          merges them and runs after the plots finished their own layout.
 */
void QwtFigure::scheduleAxisAlignment()
{
    if (m_data->alignmentPending) {
        return;
    }

    m_data->alignmentPending = true;
    QTimer::singleShot(0, this, [ this ]() {
        m_data->alignmentPending = false;
        recomputeAxisAlignment(false);
    });
}

/**
 * @brief Check whether a plot takes part in any active alignment
 * @param plot Plot to check
 * @return True when the plot is a grid member or in a manual configuration
 */
bool QwtFigure::isAxisAlignmentParticipant(const QwtPlot* plot) const
{
    return m_data->isAlignmentParticipant(this, plot);
}

/**
 * @brief Handle mouse presses on the sub widgets of the figure
 * @details A mouse press on a plot makes it the current axes. Real presses are
 *          delivered to the canvas of the plot, so the filter is installed on
 *          the canvas as well and mapped back to its parent plot here. Presses
 *          on a parasite axes are mapped to the host plot. The event is never
 *          consumed, so zooming, panning, picking and the plot editors keep
 *          working unchanged.
 *
 *          A layout request of a member marks the alignment as dirty: the label
 *          dimensions may have changed and the plot areas have to be realigned
 *          after the member settled its own layout. Showing or hiding a member
 *          changes the membership of the alignment groups, so those events
 *          schedule the pass as well.
 * @param watched Object the event was delivered to
 * @param event Event to process
 * @return Always false, the event is not consumed
 * @note 3D widgets are not handled here; the figure owner is responsible for them.
 * @note Replacing the canvas of an added plot (QwtPlot::setCanvas()) removes the
 *       activation for the new canvas; presses on the plot frame still work.
 */
bool QwtFigure::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QwtPlot* plot = qobject_cast< QwtPlot* >(watched);
        if (!plot) {
            // The press was delivered to the canvas: map it back to the plot
            QWidget* widget = qobject_cast< QWidget* >(watched);
            if (widget) {
                plot = qobject_cast< QwtPlot* >(widget->parentWidget());
            }
        }
        if (plot) {
            QwtPlot* target = plot->isParasitePlot() ? plot->hostPlot() : plot;
            if (target && currentAxes() != target) {
                setCurrentAxes(target);
            }
        }
    } else if (event->type() == QEvent::LayoutRequest || event->type() == QEvent::Show || event->type() == QEvent::Hide) {
        // A plot that re-laid its axes may have changed its label dimensions; a
        // member that is shown or hidden enters or leaves the grid groups.
        QwtPlot* plot = qobject_cast< QwtPlot* >(watched);
        if (!plot) {
            if (QWidget* widget = qobject_cast< QWidget* >(watched)) {
                plot = qobject_cast< QwtPlot* >(widget->parentWidget());
            }
        }
        if (plot && isAxisAlignmentParticipant(plot)) {
            scheduleAxisAlignment();
        }
    } else if (event->type() == QEvent::Destroy) {
        // A plot deleted without removeWidget() must not leave a dangling entry
        QwtPlot* plot = qobject_cast< QwtPlot* >(watched);
        if (plot) {
            m_data->appliedInsets.remove(plot);
            for (auto& config : m_data->alignmentConfigs) {
                config.plots.removeAll(plot);
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}
