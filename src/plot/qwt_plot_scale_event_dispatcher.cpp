#include "qwt_plot_scale_event_dispatcher.h"

// qt
#include <QMouseEvent>
#include <QApplication>
#include <QDebug>
#include <QTimer>
// qwt
#include "qwt_plot.h"
#include "qwt_scale_widget.h"
#include "qwt_qt5qt6_compat.hpp"

class QwtPlotScaleEventDispatcher::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotScaleEventDispatcher)
public:
    PrivateData(QwtPlotScaleEventDispatcher* p);
    // Cache structure
    struct ScaleCache
    {
        QwtScaleWidget* scaleWidget;
        QRect eventRect;  // Rectangle in the base plot coordinate system, i.e., the coordinate system of the current event object
        QwtPlot* basePlot;  // Base plot, i.e., the plot corresponding to the current event filter
        bool isValid;

        ScaleCache() : scaleWidget(nullptr), basePlot(nullptr), isValid(false)
        {
        }
        ScaleCache(QwtScaleWidget* scale, QwtPlot* base) : scaleWidget(scale), basePlot(base), isValid(true)
        {
            updateRect();
        }

        void updateRect()
        {
            if (scaleWidget) {
                QRect localRect      = scaleWidget->scaleRect();
                QPoint globalTopLeft = scaleWidget->mapToGlobal(localRect.topLeft());
                eventRect            = QRect(basePlot->mapFromGlobal(globalTopLeft), localRect.size());
                isValid              = true;
            } else {
                isValid = false;
            }
        }

        bool contains(const QPoint& pos) const
        {
            return isValid && eventRect.contains(pos);
        }
    };
    ScaleCache findScaleCache(const QPoint& pos);
    // Reset state
    void resetRecord();
    void updateCursorForMousePress(QwtPlot* p);
    void updateCursorForHover(QwtPlot* p, bool isOnScale);
    void updateCursorForMouseRelease(QwtPlot* p, Qt::MouseButton button, bool isOnScale);
    static QwtPlot* scalePlot(QwtScaleWidget* scaleWidget);

public:
    bool isEnable { true };
    QwtPlot* bindedPlot { nullptr };  ///< Bound plot
    QwtPlot* currentPlot { nullptr };
    QwtScaleWidget* currentScale { nullptr };            ///< Scale widget currently being operated
    QwtAxisId currentAxisId { QwtAxis::AxisPositions };  ///< Current axis ID
    QPoint lastMousePos;                                 ///< Last mouse position (for drag calculation)
    bool isMousePressed { false };                       ///< Track whether mouse button is pressed
    // Cache related
    QList< ScaleCache > scaleCaches;
    bool cacheDirty { true };   ///< Whether cache needs rebuild
    double zoomFactor { 1.5 };  ///< Zoom factor
};

QwtPlotScaleEventDispatcher::PrivateData::PrivateData(QwtPlotScaleEventDispatcher* p) : q_ptr(p)
{
}

QwtPlotScaleEventDispatcher::PrivateData::ScaleCache QwtPlotScaleEventDispatcher::PrivateData::findScaleCache(const QPoint& pos)
{
    for (const auto& cache : qwt_as_const(this->scaleCaches)) {
        if (cache.contains(pos)) {
            return cache;
        }
    }
    return ScaleCache();
}

void QwtPlotScaleEventDispatcher::PrivateData::resetRecord()
{
    currentScale   = nullptr;
    currentPlot    = nullptr;
    isMousePressed = false;
    currentAxisId  = QwtAxis::AxisPositions;
}

void QwtPlotScaleEventDispatcher::PrivateData::updateCursorForMousePress(QwtPlot* p)
{
    if (!currentScale) {
        p->unsetCursor();
    }
    if (currentScale->testBuildinActions(QwtScaleWidget::ActionClickPan)) {
        p->setCursor(Qt::ClosedHandCursor);
    } else {
        p->unsetCursor();
    }
}

void QwtPlotScaleEventDispatcher::PrivateData::updateCursorForHover(QwtPlot* p, bool isOnScale)
{
    if (!currentScale) {
        p->unsetCursor();
    }
    if (currentScale->testBuildinActions(QwtScaleWidget::ActionClickPan)) {
        if (currentScale->isSelected()) {
            if (isMousePressed) {
                p->setCursor(Qt::ClosedHandCursor);
            } else {
                if (isOnScale) {
                    p->setCursor(Qt::OpenHandCursor);
                } else {
                    p->unsetCursor();
                }
            }
        } else {
            p->unsetCursor();
        }
    }
}

void QwtPlotScaleEventDispatcher::PrivateData::updateCursorForMouseRelease(QwtPlot* p, Qt::MouseButton button, bool isOnScale)
{
    if (button == Qt::LeftButton) {
        if (currentScale && currentScale->testBuildinActions(QwtScaleWidget::ActionClickPan)) {
            p->setCursor(Qt::OpenHandCursor);
        } else {
            p->unsetCursor();
        }
    } else if (button == Qt::RightButton) {
        if (isOnScale) {
            p->unsetCursor();
        }
    }
}

QwtPlot* QwtPlotScaleEventDispatcher::PrivateData::scalePlot(QwtScaleWidget* scaleWidget)
{
    return qobject_cast< QwtPlot* >(scaleWidget->parent());
}

//----------------------------------------------------
// QwtParasitePlotEventFilter
//----------------------------------------------------

/**
 * @brief Constructor
 * @param[in] plot Plot widget
 * @param[in] par Parent object
 *
 */
QwtPlotScaleEventDispatcher::QwtPlotScaleEventDispatcher(QwtPlot* plot, QObject* par)
    : QObject(par), QWT_PIMPL_CONSTRUCT
{
    m_data->bindedPlot = plot;
    if (!plot->hasMouseTracking()) {
        plot->setMouseTracking(true);
    }
    rebuildCache();
}

/**
 * @brief Destructor
 *
 */
QwtPlotScaleEventDispatcher::~QwtPlotScaleEventDispatcher()
{
}

/**
 * @brief Set enabled state
 * @param[in] on Enable/disable
 *
 */
void QwtPlotScaleEventDispatcher::setEnable(bool on)
{
    m_data->isEnable = on;
}

/**
 * @brief Check if enabled
 * @return True if enabled
 *
 */
bool QwtPlotScaleEventDispatcher::isEnable() const
{
    return m_data->isEnable;
}

/**
 * @brief Find axis ID corresponding to QwtScaleWidget
 * @param[in] plot QwtPlot pointer
 * @param[in] scaleWidget QwtScaleWidget to find
 * @return Axis ID, returns QwtAxis::AxisPositions if not found
 *
 */
QwtAxisId QwtPlotScaleEventDispatcher::findAxisIdByScaleWidget(const QwtPlot* plot, const QwtScaleWidget* scaleWidget)
{
    if (!plot || !scaleWidget) {
        return QwtAxis::AxisPositions;
    }

    // Iterate over all possible axes
    for (int axis = 0; axis < QwtAxis::AxisPositions; axis++) {
        if (plot->axisWidget(axis) == scaleWidget) {
            return axis;
        }
    }

    return QwtAxis::AxisPositions;  // Not found
}

/**
 * @brief Rebuild all cache data; must be called when a new parasite plot is added to cache the plot's axes
 */
void QwtPlotScaleEventDispatcher::rebuildCache()
{
    QWT_D(d);
    d->scaleCaches.clear();
    const QList< QwtPlot* > plotslist = d->bindedPlot->plotList();
    for (QwtPlot* plot : plotslist) {
        // Check all scale widgets of this plot
        for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; ++axisPos) {
            QwtAxisId axisId(axisPos);
            QwtScaleWidget* scale = plot->axisWidget(axisId);
            if (scale && plot->isAxisVisible(axisId)) {
                d->scaleCaches.append(PrivateData::ScaleCache(scale, d->bindedPlot));
            }
        }
    }
    d->cacheDirty = false;
}

/**
 * @brief Update cache, rebuild if necessary
 * @details Called when cache is dirty or needs refresh.
 *
 */
void QwtPlotScaleEventDispatcher::updateCache()
{
    if (m_data->cacheDirty) {
        rebuildCache();
    } else {
        // Only update existing cache entries
        for (PrivateData::ScaleCache& cache : m_data->scaleCaches) {
            if (cache.scaleWidget) {
                cache.updateRect();
            }
        }
    }
}

bool QwtPlotScaleEventDispatcher::eventFilter(QObject* obj, QEvent* e)
{
    if (!m_data->isEnable) {
        return false;
    }
    QwtPlot* plot = qobject_cast< QwtPlot* >(obj);
    if (!plot) {
        return false;
    }
    switch (e->type()) {
    case QEvent::Resize:
    case QEvent::LayoutRequest:
    case QEvent::Polish: {
        // Handle events that may affect the cache
        m_data->cacheDirty = true;
        updateCache();
        break;
    }
    case QEvent::MouseButtonPress:
        return handleMousePress(plot, static_cast< QMouseEvent* >(e));
    case QEvent::MouseMove:
        return handleMouseMove(plot, static_cast< QMouseEvent* >(e));
    case QEvent::MouseButtonRelease:
        return handleMouseRelease(plot, static_cast< QMouseEvent* >(e));
    case QEvent::Wheel:
        return handleWheelEvent(plot, static_cast< QWheelEvent* >(e));
    default:
        break;
    }
    return QObject::eventFilter(obj, e);
}

bool QwtPlotScaleEventDispatcher::handleMousePress(QwtPlot* bindPlot, QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) {
        return false;
    }
    QWT_D(d);

    // Check if any scale widget on the current plot should handle this event
    QwtScaleWidget* targetScale = findTargetOnScale(e->pos());
    if (!targetScale) {
        // Not on any scale
        if (d->currentScale) {
            // A scale was already selected, deselect it
            d->currentScale->setSelected(false);
        }
        d->resetRecord();
        return false;
    }
    // Click is on a scale; check if it differs from currentScale, and deselect currentScale if so

    if (targetScale != d->currentScale) {
        if (d->currentScale) {
            d->currentScale->setSelected(false);
        }
        d->currentScale  = targetScale;
        d->currentPlot   = PrivateData::scalePlot(targetScale);
        d->currentAxisId = findAxisIdByScaleWidget(d->currentPlot, targetScale);
        // Record current selection
        d->currentScale->setSelected(true);
    }
    d->lastMousePos   = e->pos();
    d->isMousePressed = true;

    d->updateCursorForMousePress(bindPlot);

    return true;  // Event forwarding logic has been handled
}

bool QwtPlotScaleEventDispatcher::handleMouseMove(QwtPlot* bindPlot, QMouseEvent* e)
{
    // Check if any scale widget on the current plot should handle this event
    QWT_D(d);
    if (d->currentScale) {
        if (d->isMousePressed) {
            // A scale is selected and mouse is pressed, currently in dragging state
            if (d->currentScale->testBuildinActions(QwtScaleWidget::ActionClickPan)) {

                d->updateCursorForHover(bindPlot, true);
                const QPoint delta   = e->pos() - d->lastMousePos;
                const int deltaPixel = QwtAxis::isYAxis(d->currentAxisId) ? delta.y() : delta.x();
                if (deltaPixel != 0) {
                    d->currentPlot->panAxis(d->currentAxisId, deltaPixel);
                    // Manually trigger a repaint
                    d->currentPlot->replotAll();
                    d->lastMousePos = e->pos();
                    return true;
                }
            }
        } else {
            // A scale was previously selected
            // but mouse is not pressed
            QwtScaleWidget* targetScale = findTargetOnScale(e->pos());
            if (targetScale == d->currentScale) {
                d->updateCursorForHover(bindPlot, true);
            } else {
                bindPlot->unsetCursor();
            }
        }
    } else {
        // No scale selected, normal handling
        bindPlot->unsetCursor();
    }
    return false;
}

bool QwtPlotScaleEventDispatcher::handleMouseRelease(QwtPlot* bindPlot, QMouseEvent* e)
{
    QWT_D(d);
    Q_UNUSED(bindPlot);

    QwtScaleWidget* targetScale = findTargetOnScale(e->pos());
    d->updateCursorForMouseRelease(bindPlot, e->button(), targetScale == d->currentScale);
    if (e->button() == Qt::RightButton) {
        if (!targetScale) {
            return false;
        }
        // Right button: handle if in current scale area, otherwise ignore
        if (d->currentScale == targetScale) {
            d->isMousePressed = false;
            d->currentScale->setSelected(false);  // This function checks for changes; repeated calls won't re-trigger the signal
            return true;
        }
        return false;
    }

    if (e->button() == Qt::LeftButton) {
        d->isMousePressed = false;
        // Left button: only handle if previously pressed and in current scale area; do not combine with onMyScale check here, dragging outside the plot area would not be recognized
        return (targetScale
                != nullptr);  // Return true when targetScale is non-null to intercept the event, so upper-level handlers can process it, e.g., QwtFigureWidgetOverlay
    }
    return false;
}

bool QwtPlotScaleEventDispatcher::handleWheelEvent(QwtPlot* bindPlot, QWheelEvent* e)
{
    QWT_D(d);
    Q_UNUSED(bindPlot);
    // Check if any scale widget on the current plot should handle this event
    QwtScaleWidget* targetScale = findTargetOnScale(qwt::compat::eventPos(e));
    if (d->currentScale && d->currentScale == targetScale) {
        if (d->currentScale->testBuildinActions(QwtScaleWidget::ActionWheelZoom)) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            QPoint p = e->globalPosition().toPoint();
#else
            QPoint p = e->globalPos();
#endif
            p        = d->currentScale->mapFromGlobal(p);
            if (qwt::compat::wheelEventDelta(e) > 0) {
                d->currentPlot->zoomAxis(d->currentAxisId, d->zoomFactor, p);
            } else {
                d->currentPlot->zoomAxis(d->currentAxisId, 1.0 / d->zoomFactor, p);
            }
            d->currentPlot->replot();
            return true;
        }
    }
    return false;
}

QwtScaleWidget* QwtPlotScaleEventDispatcher::findTargetOnScale(const QPoint& pos)
{
    // Check from highest to lowest z-order (cache list is already sorted by z-order)
    for (const auto& cache : qwt_as_const(m_data->scaleCaches)) {
        if (cache.contains(pos)) {
            return cache.scaleWidget;
        }
    }
    return nullptr;
}
