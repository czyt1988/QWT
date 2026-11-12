/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "qwt_figure.h"
// Qt
#include <QPainter>
#include <QPointer>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QEvent>
#include <QApplication>
#include <QGuiApplication>
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

class QwtFigure::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtFigure)
public:
    PrivateData(QwtFigure* p);
    // Called when a plot is about to be removed
    void plotWillRemove(QwtPlot* p);
    // Let a running grid splitter pick up added/removed children
    void updateGridSplitter();
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
        widget->installEventFilter(this);
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
    widget->removeEventFilter(this);

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
 * @note Disable the FunResizePlot mode of QwtFigureWidgetOverlay (or make the overlay
 *       transparent for mouse events) while the grid is editable: the overlay covers
 *       the whole figure and would swallow the events of the plots.
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
    plot->removeEventFilter(this);
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
            para->removeEventFilter(this);
            para->setParent(nullptr);
            para->hide();
        }
        if (lay) {
            lay->invalidate();
        }
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
                    w->removeEventFilter(this);
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
        parasitePlot->installEventFilter(this);
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
}

/**
 * @brief Add axis alignment configuration
 * @details This method adds an alignment configuration for the specified plots and axis.
 * @param plots List of plots to align
 * @param axisId Axis ID to align (QwtAxis::XTop/XBottom/YLeft/YRight)
 */
void QwtFigure::addAxisAlignment(const QList< QwtPlot* >& plots, int axisId)
{
    if (plots.isEmpty() || !QwtAxis::isValid(axisId)) {
        return;
    }

    // Filter out plots not in the current figure
    QList< QwtPlot* > validPlots;
    for (QwtPlot* plot : plots) {
        if (plot && hasAxes(plot)) {
            validPlots.append(plot);
        }
    }

    if (validPlots.isEmpty()) {
        return;
    }

    // Add to configuration list
    PrivateData::AlignmentConfig config;
    config.plots  = validPlots;
    config.axisId = axisId;
    m_data->alignmentConfigs.append(config);
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
    return removed;
}
/**
 * @brief Clear all axis alignment configurations
 * @details This method removes all alignment configurations from the figure.
 */
void QwtFigure::clearAxisAlignment()
{
    m_data->alignmentConfigs.clear();
}

/**
 * @brief Apply all axis alignment configurations
 * @details This method applies all recorded alignment configurations to the plots.
 * @param replot If true, replot all affected plots after alignment
 */
void QwtFigure::applyAllAxisAlignments(bool replot)
{
    for (const auto& config : qwt_as_const(m_data->alignmentConfigs)) {
        alignAxes(config.plots, config.axisId, replot);
    }
}

/**
 * @brief Apply alignment configurations for a specific axis
 * @details This method applies all alignment configurations for the specified axis ID.
 * @param axisId Axis ID to apply alignments for
 */
void QwtFigure::applyAlignmentsForAxis(int axisId)
{
    if (!QwtAxis::isValid(axisId)) {
        return;
    }

    for (const auto& config : qwt_as_const(m_data->alignmentConfigs)) {
        if (config.axisId == axisId) {
            alignAxes(config.plots, config.axisId);
        }
    }
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
 * @brief Align axes of multiple plots
 * @details This function unifies the minimumExtent and minBorderDist of the specified axis
 *          to ensure visual alignment of axes.
 * @param plots List of QwtPlot to align (must be non-empty)
 * @param axisId Axis ID to align (QwtAxis::XTop/XBottom/YLeft/YRight)
 * @param replot If true, update layout and replot after alignment
 * @note This function should be called after widget initialization (e.g., in showEvent/resizeEvent).
 * @note Supports any number of plots and any valid axis type, adapting to horizontal/vertical layouts.
 * @note Do not pass parasite axes, currently only supports host axes.
 */
void QwtFigure::alignAxes(QList< QwtPlot* > plots, int axisId, bool update)
{
    // ========== Step 1: Parameter validation ==========
    if (plots.isEmpty()) {
        return;
    }

    if (!QwtAxis::isValid(axisId)) {
        return;
    }

    // Filter out null pointer plots
    plots.erase(std::remove_if(plots.begin(), plots.end(), [](QwtPlot* p) { return p == nullptr; }), plots.end());
    if (plots.isEmpty()) {
        return;
    }

    // ========== Step 2: Unify axis minimumExtent (ensure consistent axis width/height) and EdgeMargin (ensure consistent plot area offset from boundary) ==========
    double maxExtent  = 0.0;
    int maxEdgeMargin = 0;
    int maxStartDist = 0, maxEndDist = 0;
    // Calculate the maximum extent (actual extension size), edgeMargin, BorderDistHint for all plots on the specified axis
    for (QwtPlot* plot : qwt_as_const(plots)) {
        QwtScaleWidget* scaleWidget = plot->axisWidget(axisId);
        if (!scaleWidget) {
            continue;
        }

        QwtScaleDraw* scaleDraw = scaleWidget->scaleDraw();
        if (!scaleDraw)
            continue;

        // Reset minimum extent to calculate the actual extent
        scaleDraw->setMinimumExtent(0.0);
        // Calculate the current axis extent (including tick labels, tick marks, axis title)
        double extent = scaleDraw->extent(scaleWidget->font());
        if (extent > maxExtent) {
            maxExtent = extent;
        }
        // Query edgeMargin and record the maximum edgeMargin
        int em = scaleWidget->edgeMargin();
        if (em > maxEdgeMargin) {
            maxEdgeMargin = em;
        }
        // Unify axis minBorderDist (ensure consistent plot area offset)
        int startDist = 0, endDist = 0;
        scaleWidget->getBorderDistHint(startDist, endDist);

        // Update maximum values
        maxStartDist = qMax(maxStartDist, startDist);
        maxEndDist   = qMax(maxEndDist, endDist);
    }

    // Update data for all plots
    for (QwtPlot* plot : qwt_as_const(plots)) {
        QwtScaleWidget* scaleWidget = plot->axisWidget(axisId);
        if (!scaleWidget) {
            continue;
        }
        scaleWidget->scaleDraw()->setMinimumExtent(maxExtent);
        scaleWidget->setEdgeMargin(maxEdgeMargin);
        scaleWidget->setMinBorderDist(maxStartDist, maxEndDist);
    }
    // ========== Step 4: Force axis update and replot to ensure settings take effect ==========
    if (update) {
        for (QwtPlot* plot : qwt_as_const(plots)) {
            plot->updateLayout();
            plot->replot();  // Replot
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
    applyAllAxisAlignments(false);  // Realign when window size changes
    QFrame::resizeEvent(event);
}

/**
 * @brief Handle mouse presses on the sub widgets of the figure
 * @details A mouse press on a plot makes it the current axes. Presses on a parasite axes
 *          are mapped to the host plot. The event is never consumed, so zooming, panning,
 *          picking and the plot editors keep working unchanged.
 * @param watched Object the event was delivered to
 * @param event Event to process
 * @return Always false, the event is not consumed
 * @note 3D widgets are not handled here; the figure owner is responsible for them.
 */
bool QwtFigure::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (QwtPlot* plot = qobject_cast< QwtPlot* >(watched)) {
            QwtPlot* target = plot->isParasitePlot() ? plot->hostPlot() : plot;
            if (target && currentAxes() != target) {
                setCurrentAxes(target);
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}
