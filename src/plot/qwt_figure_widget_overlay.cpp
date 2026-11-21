/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "qwt_figure_widget_overlay.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QHash>
#include <QApplication>
#include <QDebug>
#include <QPainter>
// std
#include <algorithm>
// qwt
#include "qwt_algorithm.hpp"
#include "qwt_figure.h"
#include "qwt_plot.h"
#include "qwt_qt5qt6_compat.hpp"

#ifndef QwtFigureWidgetOverlay_DEBUG_PRINT
#define QwtFigureWidgetOverlay_DEBUG_PRINT 0
#endif

class QwtFigureWidgetOverlay::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtFigureWidgetOverlay)
public:
    PrivateData(QwtFigureWidgetOverlay* p);

public:
    QwtFigureWidgetOverlay::BuiltInFunctions mFuntion;  ///< Built-in functions
    QPoint mLastMousePressPos { 0, 0 };                 ///< Records the coordinates of the last window move
    QBrush mContorlPointBrush { Qt::blue };             ///< Brush for drawing control points in edit mode
    QPen mBorderPen { Qt::blue };                       ///< Pen for drawing border in edit mode
    bool mIsStartResize { false };                      ///< Indicates whether resizing has started
    QWidget* mActiveWidget { nullptr };                 /// The currently active widget, or nullptr if none
    QRectF mOldNormRect;                                ///< Saves the old widget position for redo/undo
    QRectF mWillSetNormRect;                            ///< The normalized rect to be set
    QSize mControlPointSize { 8, 8 };                   ///< Control point size
    QwtFigureWidgetOverlay::ControlType mControlType {
        QwtFigureWidgetOverlay::OutSide
    };  ///< Records the current resize control position

    bool mShowPrecentText { true };  ///< Show percentage text
};

QwtFigureWidgetOverlay::PrivateData::PrivateData(QwtFigureWidgetOverlay* p) : q_ptr(p)
{
    mFuntion.setFlag(FunSelectCurrentPlot, true);
    mFuntion.setFlag(FunResizePlot, true);
}

//----------------------------------------------------
// QwtFigureWidgetOverlay
//----------------------------------------------------

/**
 * @brief Constructor
 * @param[in] fig The QwtFigure to attach to
 * @note Passing nullptr is not allowed
 */
QwtFigureWidgetOverlay::QwtFigureWidgetOverlay(QwtFigure* fig) : QwtWidgetOverlay(fig), QWT_PIMPL_CONSTRUCT
{
    Q_ASSERT(fig);

    QwtPlot* gca = fig->currentAxes();
    if (gca) {
        setActiveWidget(gca);
    } else {
        selectNextPlot();
        if (!currentActiveWidget()) {
            selectNextWidget();
        }
    }
    connect(fig, &QwtFigure::axesRemoved, this, &QwtFigureWidgetOverlay::onAxesRemove);
    setMouseTracking(true);
    setTransparentForMouseEvents(false);  // Not transparent for mouse events, to avoid axis event interception
}

QwtFigureWidgetOverlay::~QwtFigureWidgetOverlay()
{
}

/**
 * @brief Returns the associated QwtFigure
 * @return The parent QwtFigure
 */
QwtFigure* QwtFigureWidgetOverlay::figure() const
{
    return static_cast< QwtFigure* >(parent());
}

/**
 * @brief Sets whether the overlay is transparent for mouse events
 * @param[in] on True to make transparent, false otherwise
 */
void QwtFigureWidgetOverlay::setTransparentForMouseEvents(bool on)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, on);
}

/**
 * @brief Checks if the overlay is transparent for mouse events
 * @return True if transparent, false otherwise
 */
bool QwtFigureWidgetOverlay::isTransparentForMouseEvents() const
{
    return testAttribute(Qt::WA_TransparentForMouseEvents);
}

/**
 * @brief Get cursor shape based on control type
 * @param rr The control type
 * @return The cursor shape
 */
Qt::CursorShape QwtFigureWidgetOverlay::controlTypeToCursor(QwtFigureWidgetOverlay::ControlType rr)
{
    switch (rr) {
    case ControlLineTop:
    case ControlLineBottom:
        return (Qt::SizeVerCursor);

    case ControlLineLeft:
    case ControlLineRight:
        return (Qt::SizeHorCursor);

    case ControlPointTopLeft:
    case ControlPointBottomRight:
        return (Qt::SizeFDiagCursor);

    case ControlPointTopRight:
    case ControlPointBottomLeft:
        return (Qt::SizeBDiagCursor);

    case Inner:
        return (Qt::SizeAllCursor);

    default:
        break;
    }
    return (Qt::ArrowCursor);
}

/**
 * @brief Determines control type based on point position relative to rectangle
 * @param[in] pos The point position
 * @param[in] region The rectangle region
 * @param[in] err The error tolerance
 * @return The control type at the given position
 */
QwtFigureWidgetOverlay::ControlType
QwtFigureWidgetOverlay::getPositionControlType(const QPoint& pos, const QRect& region, int err)
{
    if (!region.adjusted(-err, -err, err, err).contains(pos)) {
        return (OutSide);
    }
    if (pos.x() < (region.left() + err)) {
        if (pos.y() < region.top() + err) {
            return (ControlPointTopLeft);
        } else if (pos.y() > region.bottom() - err) {
            return (ControlPointBottomLeft);
        }
        return (ControlLineLeft);
    } else if (pos.x() > (region.right() - err)) {
        if (pos.y() < region.top() + err) {
            return (ControlPointTopRight);
        } else if (pos.y() > region.bottom() - err) {
            return (ControlPointBottomRight);
        }
        return (ControlLineRight);
    } else if (pos.y() < (region.top() + err)) {
        if (pos.x() < region.left() + err) {
            return (ControlPointTopLeft);
        } else if (pos.x() > region.right() - err) {
            return (ControlPointTopRight);
        }
        return (ControlLineTop);
    } else if (pos.y() > region.bottom() - err) {
        if (pos.x() < region.left() + err) {
            return (ControlPointBottomLeft);
        } else if (pos.x() > region.right() - err) {
            return (ControlPointBottomRight);
        }
        return (ControlLineBottom);
    }
    return (Inner);
}

/**
 * @brief Checks if a point is on the edge of a rectangle
 * @param[in] pos The point position
 * @param[in] region The rectangle region
 * @param[in] err The error tolerance
 * @return True if the point is on the edge
 */
bool QwtFigureWidgetOverlay::isPointInRectEdget(const QPoint& pos, const QRect& region, int err)
{
    if (!region.adjusted(-err, -err, err, err).contains(pos)) {
        return (false);
    }
    if ((pos.x() < (region.left() - err)) && (pos.x() < (region.left() + err))) {
        return (true);
    } else if ((pos.x() > (region.right() - err)) && (pos.x() < (region.right() + err))) {
        return (true);
    } else if ((pos.y() > (region.top() - err)) && (pos.y() < (region.top() + err))) {
        return (true);
    } else if ((pos.y() > region.bottom() - err) && (pos.y() < region.bottom() + err)) {
        return (true);
    }
    return (false);
}

/**
 * @brief Enables or disables built-in functions
 * @param[in] flag The function flag to set
 * @param[in] on True to enable, false to disable
 * @sa QwtFigureWidgetOverlay::BuiltInFunctionsFlag
 */
void QwtFigureWidgetOverlay::setBuiltInFunctionsEnable(BuiltInFunctionsFlag flag, bool on)
{
    m_data->mFuntion.setFlag(flag, on);
}

/**
 * @brief Tests if a built-in function is enabled
 * @param[in] flag The function flag to test
 * @return True if enabled, false otherwise
 */
bool QwtFigureWidgetOverlay::testBuiltInFunctions(BuiltInFunctionsFlag flag) const
{
    return m_data->mFuntion.testFlag(flag);
}

/**
 * @brief Checks if there is an active widget
 * @return True if there is an active widget, false otherwise
 */
bool QwtFigureWidgetOverlay::hasActiveWidget() const
{
    return (m_data->mActiveWidget != nullptr);
}

/**
 * @brief Checks if currently resizing
 * @return True if resizing, false otherwise
 */
bool QwtFigureWidgetOverlay::isResizing() const
{
    return m_data->mIsStartResize;
}

/**
 * @brief Sets the border pen
 * @param[in] p The pen to set
 */
void QwtFigureWidgetOverlay::setBorderPen(const QPen& p)
{
    m_data->mBorderPen = p;
}

/**
 * @brief Returns the border pen
 * @return The current border pen
 */
QPen QwtFigureWidgetOverlay::borderPen() const
{
    return m_data->mBorderPen;
}

/**
 * @brief Sets the control point brush
 * @param[in] b The brush to set
 */
void QwtFigureWidgetOverlay::setControlPointBrush(const QBrush& b)
{
    m_data->mContorlPointBrush = b;
}

/**
 * @brief Returns the control point brush
 * @return The current control point brush
 */
QBrush QwtFigureWidgetOverlay::controlPointBrush() const
{
    return m_data->mContorlPointBrush;
}

/**
 * @brief Sets the control point size
 * @param[in] c The size to set
 */
void QwtFigureWidgetOverlay::setControlPointSize(const QSize& c)
{
    m_data->mControlPointSize = c;
}

/**
 * @brief Returns the control point size
 * @return The current control point size (default 8x8)
 */
QSize QwtFigureWidgetOverlay::controlPointSize() const
{
    return m_data->mControlPointSize;
}

/**
 * @brief Selects the next widget as the active widget
 * @param[in] forward True for forward selection, false for backward
 */
void QwtFigureWidgetOverlay::selectNextWidget(bool forward)
{
    QList< QWidget* > ws = figure()->findChildren< QWidget* >("", Qt::FindDirectChildrenOnly);
    ws.removeAll(this);
    if (ws.isEmpty()) {
        setActiveWidget(nullptr);
        return;
    }
    // Remove parasite plots
    auto it = std::remove_if(ws.begin(), ws.end(), [](QWidget* w) -> bool {
        if (QwtPlot* plot = qobject_cast< QwtPlot* >(w)) {
            if (plot->isParasitePlot()) {
                return true;
            }
        }
        return false;
    });
    if (it != ws.end()) {
        ws.erase(it, ws.end());  // Remove trailing invalid elements, i.e., all parasite plots
    }
    // Now ws contains only selectable widgets
    auto nextIt = qwtSelectNextIterator(ws.begin(), ws.end(), currentActiveWidget(), forward);
    setActiveWidget((nextIt != ws.end()) ? *nextIt : nullptr);
}

/**
 * @brief Selects the next plot as the active widget
 * @param[in] forward True for forward selection, false for backward
 */
void QwtFigureWidgetOverlay::selectNextPlot(bool forward)
{
    // This function does not return parasite plots
    QList< QwtPlot* > ws = figure()->allAxes();
    if (ws.isEmpty()) {
        setActiveWidget(nullptr);
        return;
    }
    // Cast current element type and get the next iterator
    QwtPlot* current = qobject_cast< QwtPlot* >(currentActiveWidget());
    auto nextIt      = qwtSelectNextIterator(ws.begin(), ws.end(), current, forward);
    setActiveWidget((nextIt != ws.end()) ? *nextIt : nullptr);
}

/**
 * @brief Returns the current active widget
 * @return The current active widget, nullptr if none
 */
QWidget* QwtFigureWidgetOverlay::currentActiveWidget() const
{
    return m_data->mActiveWidget;
}

/**
 * @brief Returns the current active plot
 * @return The current active plot, nullptr if none or not a plot
 */
QwtPlot* QwtFigureWidgetOverlay::currentActivePlot() const
{
    return qobject_cast< QwtPlot* >(m_data->mActiveWidget);
}

/**
 * @brief Shows or hides percentage text
 * @param[in] on True to show, false to hide
 */
void QwtFigureWidgetOverlay::showPercentText(bool on)
{
    m_data->mShowPrecentText = on;
    updateOverlay();
}

/**
 * @brief Cancels the operation
 * @return True on success
 * @note This function emits finished(true) signal. Override should call this explicitly.
 * @code
 * bool MyFigureWidgetOverlay::cancel(){
 *    ...
 *    // Explicit call to trigger finished(true)
 *    QwtFigureWidgetOverlay::cancel();
 *    return true;
 * }
 * @endcode
 */
bool QwtFigureWidgetOverlay::cancel()
{
    Q_EMIT finished(true);
    return true;
}

/**
 * @brief Sets the current active widget
 * @param[in] w The widget to set as active
 * @note If w is the same as current active widget, no action is taken
 * @note This function emits activeWidgetChanged signal
 * @sa activeWidgetChanged
 */
void QwtFigureWidgetOverlay::setActiveWidget(QWidget* w)
{
    QWidget* oldact = currentActiveWidget();
    if (w == oldact) {
        // Avoid nested calls
        return;
    }
    m_data->mActiveWidget = w;
    updateOverlay();
    Q_EMIT activeWidgetChanged(oldact, w);
}

void QwtFigureWidgetOverlay::drawOverlay(QPainter* p) const
{
    if (!hasActiveWidget()) {
        return;
    }
    // For the active widget, draw distance indicator lines to the edges
    p->save();
    if (m_data->mIsStartResize && m_data->mFuntion.testFlag(FunResizePlot)) {
        // In resize state, draw control lines
        drawResizeingControlLine(p, m_data->mWillSetNormRect);
    } else {
        drawActiveWidget(p, currentActiveWidget());
    }
    p->restore();
}

QRegion QwtFigureWidgetOverlay::maskHint() const
{
    return (figure()->rect());
}

/**
 * @brief Draw the active widget
 *
 * Override this function to change the drawing style. Default drawing calls @ref drawControlLine.
 * @param painter
 * @param activeW
 */
void QwtFigureWidgetOverlay::drawActiveWidget(QPainter* painter, QWidget* activeW) const
{
    const QRect& chartRect      = activeW->frameGeometry();
    const QRectF& normalPercent = figure()->widgetNormRect(activeW);
    drawControlLine(painter, chartRect, normalPercent);
}

/**
 * @brief Draw the rubber-band control lines during resize
 *
 * Override this function to change the drawing style. Default drawing calls @ref drawControlLine.
 * @param painter
 * @param willSetNormRect
 */
void QwtFigureWidgetOverlay::drawResizeingControlLine(QPainter* painter, const QRectF& willSetNormRect) const
{
    QRect actualRect = figure()->calcActualRect(willSetNormRect);
    drawControlLine(painter, actualRect, willSetNormRect);
}

/**
 * @brief Draw control lines
 * @param painter
 * @param actualRect Actual size
 * @param normRect Normalized size
 */
void QwtFigureWidgetOverlay::drawControlLine(QPainter* painter, const QRect& actualRect, const QRectF& normRect) const
{
    painter->setBrush(Qt::NoBrush);
    painter->setPen(m_data->mBorderPen);
    QRect edgetRect = actualRect.adjusted(1, 1, -1, -1);

    // Draw rectangle border
    painter->drawRect(edgetRect);
    // Draw lines from border to figure edges
    QPen linePen(m_data->mBorderPen);

    linePen.setStyle(Qt::DotLine);
    painter->setPen(linePen);
    QPoint center = actualRect.center();

    painter->drawLine(center.x(), 0, center.x(), actualRect.top());            // top
    painter->drawLine(center.x(), actualRect.bottom(), center.x(), height());  // bottom
    painter->drawLine(0, center.y(), actualRect.left(), center.y());           // left
    painter->drawLine(actualRect.right(), center.y(), width(), center.y());    // right
    // Draw top data
    QFontMetrics fm = painter->fontMetrics();
    // top text
    QString percentText = QString::number(normRect.y() * 100, 'g', 2) + "%";
    QRectF textRect     = fm.boundingRect(percentText);
    textRect.moveTopLeft(QPoint(center.x(), 0));
    painter->drawText(textRect, Qt::AlignCenter, percentText);
    // left
    percentText = QString::number(normRect.x() * 100, 'g', 2) + "%";
    textRect    = fm.boundingRect(percentText);
    textRect.moveBottomLeft(QPoint(0, center.y()));
    painter->drawText(textRect, Qt::AlignCenter, percentText);

    //    painter->drawText(QPointF(0, actualRect.y()), QString::number(percent.x(), 'g', 2));
    // Draw four corners
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_data->mContorlPointBrush);
    QRect connerRect(0, 0, m_data->mControlPointSize.width(), m_data->mControlPointSize.height());
    QPoint offset = QPoint(m_data->mControlPointSize.width() / 2, m_data->mControlPointSize.height() / 2);
    connerRect.moveTo(edgetRect.topLeft() - offset);
    painter->drawRect(connerRect);
    connerRect.moveTo(edgetRect.topRight() - offset);
    painter->drawRect(connerRect);
    connerRect.moveTo(edgetRect.bottomLeft() - offset);
    painter->drawRect(connerRect);
    connerRect.moveTo(edgetRect.bottomRight() - offset);
    painter->drawRect(connerRect);
}

/**
 * @brief Helper function to start resizing; records the initial resize state
 * @param controlType
 * @param pos
 */
void QwtFigureWidgetOverlay::startResize(QwtFigureWidgetOverlay::ControlType controlType, const QPoint& pos)
{
    QWT_D(d);
    if (!d->mActiveWidget) {
        return;
    }

    QwtFigure* fig = figure();
    Q_ASSERT(fig);

    d->mOldNormRect       = fig->widgetNormRect(d->mActiveWidget);
    d->mLastMousePressPos = pos;
    d->mIsStartResize     = true;
    d->mControlType       = controlType;
    d->mWillSetNormRect   = QRectF();

    //! Capture the mouse to ensure all mouse events are sent to this widget.
    //! This is critical to prevent mouse events from being captured by other widgets
    //! during mouse movement, which would cause the release event to be missed.
    //! grabMouse will:
    //! - Force all mouse events (move, click, release, etc.) to be sent to this widget
    //! - Ignore the actual mouse position, even if the mouse moves to other widgets or screen edges
    //! - Ensure the integrity of the mouse event chain
    //! This MUST be paired with releaseMouse (called in mouseReleaseEvent).
    //!
    //! Without this function, if the mouse moves over underlying plot widgets or other child widgets,
    //! mouse events may be intercepted by those widgets, causing QwtFigureWidgetOverlay to miss
    //! mouseReleaseEvent and leaving the state stuck in "resizing".
    grabMouse();
}

void QwtFigureWidgetOverlay::mousePressEvent(QMouseEvent* me)
{
#if QwtFigureWidgetOverlay_DEBUG_PRINT
    qDebug() << "QwtFigureWidgetOverlay::mousePressEvent(" << qwt::compat::eventPos(me) << ")";
#endif
    if (me->button() != Qt::LeftButton) {
        QwtWidgetOverlay::mousePressEvent(me);
        return;
    }

    QWT_D(d);
    const QPoint pos = qwt::compat::eventPos(me);

    // Get the widget at click position (by z-order)
    const QList< QwtPlot* > plots = figure()->allAxes(true);
    QWidget* hitPlot              = nullptr;
    for (QWidget* w : plots) {
        if (w->frameGeometry().contains(pos, true)) {
            hitPlot = w;
            break;
        }
    }

    // Reset previous resize state
    if (d->mIsStartResize) {
        d->mIsStartResize   = false;
        d->mWillSetNormRect = QRectF();
        releaseMouse();  // Ensure mouse capture is released
    }

    // ========== Step 1: Check if the active widget's control points were clicked ==========
    // This is checked first to avoid resize being interpreted as widget switching
    if (d->mActiveWidget && testBuiltInFunctions(FunResizePlot)) {
        ControlType ct = getPositionControlType(pos, d->mActiveWidget->frameGeometry(), 4);

        // Only start resize when clicking on actual control points (edges and corners)
        if (ct != OutSide) {
            // Clicked inside active widget but hitPlot exists - this is a plot-in-plot scenario
            if (ct == Inner) {
                if (hitPlot && hitPlot != d->mActiveWidget) {
                    setActiveWidget(hitPlot);
                    me->accept();
                    return;
                }
            }
            startResize(ct, pos);
            me->accept();
            return;
        }
        // If Inner, continue to subsequent logic (may switch widget)
    }

    // ========== Step 2: Handle widget switching ==========
    if (hitPlot) {
        // Clicked on a widget
        if (hitPlot != d->mActiveWidget) {
            setActiveWidget(hitPlot);
            // Clicked inside current active widget, keep activation (no switch)
            me->accept();
            return;
        } else {
            // Clicked on already active widget, pass through
            me->ignore();
            return;
        }
    }

    // ========== Step 3: Clicked on empty area ==========
    if (d->mActiveWidget) {
        // Deactivate when clicking empty area with an active widget
        setActiveWidget(nullptr);
        updateOverlay();
        me->accept();
    } else {
        // No active widget, let the event propagate
        me->ignore();
    }
}

void QwtFigureWidgetOverlay::mouseMoveEvent(QMouseEvent* me)
{
    QWT_D(d);
#if QwtFigureWidgetOverlay_DEBUG_PRINT
    qDebug() << "QwtFigureWidgetOverlay::mouseMoveEvent(" << qwt::compat::eventPos(me) << "),have active widget"
             << (d->mActiveWidget != nullptr) << ",have FunResizePlot=" << testBuiltInFunctions(FunResizePlot)
             << ",ControlType=" << d->mControlType;
#endif
    QWidget* activeW = d->mActiveWidget;
    if (!testBuiltInFunctions(FunResizePlot)) {
        // No resize plot feature, exit
        return QwtWidgetOverlay::mouseMoveEvent(me);
    }
    if (!activeW) {
        // No active widget, update cursor and pass event
        unsetCursor();
        QwtWidgetOverlay::mouseMoveEvent(me);
        return;
    }

    const QPoint pos = qwt::compat::eventPos(me);

    if (d->mIsStartResize) {
        // Resizing in progress
        QwtFigure* fig = figure();
        Q_ASSERT(fig);
        const QRectF& oldNormRect = d->mOldNormRect;
        QPoint offset             = pos - d->mLastMousePressPos;

        switch (d->mControlType) {
        case ControlLineTop: {
            // Calculate offset.y() as ratio of height
            qreal dh = static_cast< qreal >(offset.y()) / fig->height();
            // Use figure to calculate normalized coordinates
            QRectF normRect = oldNormRect;
            normRect.setY(oldNormRect.y() + dh);
            normRect.setHeight(oldNormRect.height() - dh);
            d->mWillSetNormRect = normRect;
            break;
        }

        case ControlLineBottom: {
            // Calculate offset.y() as ratio of height
            qreal dh = static_cast< qreal >(offset.y()) / fig->height();
            // Use figure to calculate normalized coordinates
            QRectF normRect = oldNormRect;
            normRect.setHeight(oldNormRect.height() + dh);
            d->mWillSetNormRect = normRect;
            break;
        }

        case ControlLineLeft: {
            // Calculate offset.x() as ratio of width
            qreal dw        = static_cast< qreal >(offset.x()) / fig->width();
            QRectF normRect = oldNormRect;
            normRect.setX(oldNormRect.x() + dw);
            normRect.setWidth(oldNormRect.width() - dw);
            d->mWillSetNormRect = normRect;
            break;
        }

        case ControlLineRight: {
            // Calculate offset.x() as ratio of width
            qreal dw        = static_cast< qreal >(offset.x()) / fig->width();
            QRectF normRect = oldNormRect;
            normRect.setWidth(oldNormRect.width() + dw);
            d->mWillSetNormRect = normRect;
            break;
        }

        case ControlPointTopLeft: {
            qreal dh = static_cast< qreal >(offset.y()) / fig->height();
            qreal dw = static_cast< qreal >(offset.x()) / fig->width();

            QRectF normRect = oldNormRect;
            normRect.setX(oldNormRect.x() + dw);
            normRect.setY(oldNormRect.y() + dh);
            normRect.setWidth(oldNormRect.width() - dw);
            normRect.setHeight(oldNormRect.height() - dh);

            d->mWillSetNormRect = normRect;
            break;
        }

        case ControlPointTopRight: {
            qreal dh = static_cast< qreal >(offset.y()) / fig->height();
            qreal dw = static_cast< qreal >(offset.x()) / fig->width();

            QRectF normRect = oldNormRect;
            normRect.setY(oldNormRect.y() + dh);
            normRect.setWidth(oldNormRect.width() + dw);
            normRect.setHeight(oldNormRect.height() - dh);

            d->mWillSetNormRect = normRect;
            break;
        }

        case ControlPointBottomLeft: {
            qreal dh = static_cast< qreal >(offset.y()) / fig->height();
            qreal dw = static_cast< qreal >(offset.x()) / fig->width();

            QRectF normRect = oldNormRect;
            normRect.setX(oldNormRect.x() + dw);
            normRect.setWidth(oldNormRect.width() - dw);
            normRect.setHeight(oldNormRect.height() + dh);

            d->mWillSetNormRect = normRect;
            break;
        }

        case ControlPointBottomRight: {
            qreal dh = static_cast< qreal >(offset.y()) / fig->height();
            qreal dw = static_cast< qreal >(offset.x()) / fig->width();

            QRectF normRect = oldNormRect;
            normRect.setWidth(oldNormRect.width() + dw);
            normRect.setHeight(oldNormRect.height() + dh);

            d->mWillSetNormRect = normRect;
            break;
        }

        case Inner: {
            qreal dh = static_cast< qreal >(offset.y()) / fig->height();
            qreal dw = static_cast< qreal >(offset.x()) / fig->width();

            QRectF normRect     = oldNormRect.adjusted(dw, dh, dw, dh);
            d->mWillSetNormRect = normRect;
            Q_EMIT widgetNormGeometryChanged(d->mActiveWidget, d->mOldNormRect, d->mWillSetNormRect);
            break;
        }

        default:
            break;
        }
        updateOverlay();
    } else {
        // Not resizing, update cursor
        ControlType ct = getPositionControlType(qwt::compat::eventPos(me), activeW->frameGeometry(), 4);

        // Control point changed
        if (ct == OutSide) {
            unsetCursor();
            me->ignore();  // Let the event propagate
        } else {
            Qt::CursorShape cur = controlTypeToCursor(ct);
            setCursor(cur);
            me->accept();
        }
    }
}

void QwtFigureWidgetOverlay::mouseReleaseEvent(QMouseEvent* me)
{
#if QwtFigureWidgetOverlay_DEBUG_PRINT
    qDebug() << "QwtFigureWidgetOverlay::onMouseReleaseEvent" << me->pos();
#endif
    QWT_D(d);
    if (!testBuiltInFunctions(FunResizePlot)) {
        // No resize plot feature, exit
        return QwtWidgetOverlay::mouseReleaseEvent(me);
    }
    if (me->button() == Qt::LeftButton && d->mIsStartResize) {
        // Finish resize operation
        d->mIsStartResize = false;

        if (d->mActiveWidget && d->mWillSetNormRect.isValid()) {
            Q_EMIT widgetNormGeometryChanged(d->mActiveWidget, d->mOldNormRect, d->mWillSetNormRect);
        }

        d->mWillSetNormRect = QRectF();
        updateOverlay();

        //! Must release mouse since it was captured in startResize
        releaseMouse();
        me->accept();
        return;
    }

    // Pass other cases to parent class
    QwtWidgetOverlay::mouseReleaseEvent(me);
}

void QwtFigureWidgetOverlay::keyPressEvent(QKeyEvent* ke)
{
    switch (ke->key()) {
    case Qt::Key_Return: {
        selectNextWidget(true);
        ke->accept();
    } break;

    case Qt::Key_Up:
    case Qt::Key_Left: {
        selectNextWidget(true);
        ke->accept();
    } break;

    case Qt::Key_Right:
    case Qt::Key_Down: {
        selectNextWidget(false);
        ke->accept();
    } break;

    case Qt::Key_Escape: {
        if (cancel()) {
            hide();
        }
        ke->accept();
    } break;

    default:
        break;
    }
    QwtWidgetOverlay::keyPressEvent(ke);
}

void QwtFigureWidgetOverlay::onAxesRemove(QwtPlot* removedAxes)
{
    if (m_data->mActiveWidget == removedAxes) {
        m_data->mActiveWidget = nullptr;
    }
}
