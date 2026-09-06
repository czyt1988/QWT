/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *
 * Modified by ChenZongYan in 2024 <czy.t@163.com>
 *   Summary of major modifications (see ChangeLog.md for full history):
 *   1. CMake build system & C++11 throughout.
 *   2. Core panner/ zoomer refactored:
 *        - QwtPanner → QwtCachePanner (pixmap-cache version)
 *        - New real-time QwtPlotPanner derived from QwtPicker.
 *   3. Zoomer supports multi-axis.
 *   4. Parasite-plot framework:
 *        - QwtFigure, QwtPlotParasiteLayout, QwtPlotTransparentCanvas,
 *        - QwtPlotScaleEventDispatcher, built-in pan/zoom on axis.
 *   5. New picker: QwtPlotSeriesDataPicker (works with date axis).
 *   6. Raster & color-map extensions:
 *        - QwtGridRasterData (2-D table + interpolation)
 *        - QwtLinearColorMap::stopColors(), stopPos() API rename.
 *   7. Bar-chart: expose pen/brush control.
 *   8. Amalgamated build: single QwtPlot.h / QwtPlot.cpp pair in src-amalgamate.
 *****************************************************************************/
#include <QtDebug>
#include "qwt_plot.h"
#include "qwt_scale_widget.h"
#include "qwt_scale_map.h"
#include "qwt_scale_div.h"
#include "qwt_scale_engine.h"
#include "qwt_interval.h"
#include "qwt_plot_scale_event_dispatcher.h"

namespace
{
class AxisData
{
public:
    AxisData()
        : isVisible(true)
        , doAutoScale(true)
        , minValue(0.0)
        , maxValue(1000.0)
        , stepSize(0.0)
        , maxMajor(8)
        , maxMinor(5)
        , isValid(false)
        , scaleEngine(new QwtLinearScaleEngine())
        , scaleWidget(nullptr)
    {
    }

    ~AxisData()
    {
        delete scaleEngine;
    }

    void initWidget(QwtScaleDraw::Alignment align, const QString& name, QwtPlot* plot)
    {
        scaleWidget = new QwtScaleWidget(align, plot);
        scaleWidget->setObjectName(name);

#if 1
        // better find the font sizes from the application font
        const QFont fscl(plot->fontInfo().family(), 10);
        const QFont fttl(plot->fontInfo().family(), 12, QFont::Bold);
#endif

        scaleWidget->setTransformation(scaleEngine->transformation());

        scaleWidget->setFont(fscl);
        scaleWidget->setMargin(0);

        QwtText text = scaleWidget->title();
        text.setFont(fttl);
        scaleWidget->setTitle(text);
    }

    bool isVisible;
    bool doAutoScale;

    double minValue;
    double maxValue;
    double stepSize;

    int maxMajor;
    int maxMinor;

    bool isValid;  ///< Indicates whether scaleDiv is valid

    QwtScaleDiv scaleDiv;
    QwtScaleEngine* scaleEngine;
    QwtScaleWidget* scaleWidget;
};
}

class QwtPlot::ScaleData
{
public:
    ScaleData(QwtPlot* plot)
    {
        using namespace QwtAxis;

        m_axisData[ YLeft ].initWidget(QwtScaleDraw::LeftScale, "QwtPlotAxisYLeft", plot);
        m_axisData[ YRight ].initWidget(QwtScaleDraw::RightScale, "QwtPlotAxisYRight", plot);
        m_axisData[ XTop ].initWidget(QwtScaleDraw::TopScale, "QwtPlotAxisXTop", plot);
        m_axisData[ XBottom ].initWidget(QwtScaleDraw::BottomScale, "QwtPlotAxisXBottom", plot);
    }

    inline AxisData& axisData(QwtAxisId axisId)
    {
        return m_axisData[ axisId ];
    }

    inline const AxisData& axisData(QwtAxisId axisId) const
    {
        return m_axisData[ axisId ];
    }

private:
    AxisData m_axisData[ QwtAxis::AxisPositions ];
};

void QwtPlot::initAxesData()
{
    m_scaleData = new ScaleData(this);

    m_scaleData->axisData(QwtAxis::YRight).isVisible = false;
    m_scaleData->axisData(QwtAxis::XTop).isVisible   = false;

    connect(m_scaleData->axisData(QwtAxis::YLeft).scaleWidget,
            &QwtScaleWidget::requestScaleRangeUpdate,
            this,
            &QwtPlot::yLeftRequestScaleRangeUpdate);
    connect(m_scaleData->axisData(QwtAxis::YRight).scaleWidget,
            &QwtScaleWidget::requestScaleRangeUpdate,
            this,
            &QwtPlot::yRightRequestScaleRangeUpdate);
    connect(m_scaleData->axisData(QwtAxis::XBottom).scaleWidget,
            &QwtScaleWidget::requestScaleRangeUpdate,
            this,
            &QwtPlot::xBottomRequestScaleRangeUpdate);
    connect(m_scaleData->axisData(QwtAxis::XTop).scaleWidget,
            &QwtScaleWidget::requestScaleRangeUpdate,
            this,
            &QwtPlot::xTopRequestScaleRangeUpdate);
}

void QwtPlot::deleteAxesData()
{
    delete m_scaleData;
    m_scaleData = nullptr;
}

/*!
   @brief Checks if an axis is valid
   @param axisId axis ID
   @return \c true if the specified axis exists, otherwise \c false

   @note This method is equivalent to QwtAxis::isValid( axisId ) and simply checks
         if axisId is one of the values of QwtAxis::Position. It is a placeholder
         for future releases, where it will be possible to have a customizable number
         of axes ( multiaxes branch ) at each side.
 */
bool QwtPlot::isAxisValid(QwtAxisId axisId) const
{
    return QwtAxis::isValid(axisId);
}

/*!
   @brief Return the scale widget of the specified axis
   @param axisId Axis ID
   @return Scale widget, or nullptr if axisId is invalid
 */
const QwtScaleWidget* QwtPlot::axisWidget(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return m_scaleData->axisData(axisId).scaleWidget;

    return nullptr;
}

/*!
   @brief Return the scale widget of the specified axis
   @param axisId Axis ID
   @return Scale widget, or nullptr if axisId is invalid
 */
QwtScaleWidget* QwtPlot::axisWidget(QwtAxisId axisId)
{
    if (isAxisValid(axisId)) {
        return m_scaleData->axisData(axisId).scaleWidget;
    }
    return nullptr;
}

/**
 * @brief Return the currently visible X axis
 *
 * Selection policy (descending priority):
 * 1. Axis must be visible (isAxisVisible() == true);
 * 2. If both XBottom and XTop are visible, prefer XBottom;
 * 3. If only one X axis is visible, return it;
 * 4. If both are invisible, return default QwtAxis::XBottom.
 *
 * @return Selected X axis ID
 *
 */
QwtAxisId QwtPlot::visibleXAxisId() const
{
    if (isAxisVisible(QwtAxis::XBottom)) {
        return QwtAxis::XBottom;
    } else if (isAxisVisible(QwtAxis::XTop)) {
        return QwtAxis::XTop;
    }
    return QwtAxis::XBottom;
}

/**
 * @brief Return the currently usable Y axis
 *
 * Selection policy (descending priority):
 * 1. Axis must be visible;
 * 2. If both YLeft and YRight are visible, prefer YLeft;
 * 3. If only one Y axis is visible, return it;
 * 4. If both are invisible, return default QwtAxis::YLeft.
 *
 * @return Selected Y axis ID
 *
 */
QwtAxisId QwtPlot::visibleYAxisId() const
{
    if (isAxisVisible(QwtAxis::YLeft)) {
        return QwtAxis::YLeft;
    } else if (isAxisVisible(QwtAxis::YRight)) {
        return QwtAxis::YRight;
    }
    return QwtAxis::YLeft;
}

/*!
   @brief Change the scale engine for an axis
   @param axisId Axis ID
   @param scaleEngine Scale engine
   @note The old scale engine will be deleted
   @sa axisScaleEngine()
 */
void QwtPlot::setAxisScaleEngine(QwtAxisId axisId, QwtScaleEngine* scaleEngine)
{
    if (isAxisValid(axisId) && scaleEngine != nullptr) {
        AxisData& d = m_scaleData->axisData(axisId);

        delete d.scaleEngine;
        d.scaleEngine = scaleEngine;

        d.scaleWidget->setTransformation(scaleEngine->transformation());

        d.isValid = false;

        autoRefresh();
    }
}

/*!
   @brief Return the scale engine for a specific axis
   @param axisId Axis ID
   @return Scale engine
 */
QwtScaleEngine* QwtPlot::axisScaleEngine(QwtAxisId axisId)
{
    if (isAxisValid(axisId))
        return m_scaleData->axisData(axisId).scaleEngine;
    else
        return nullptr;
}

/*!
   @brief Return the scale engine for a specific axis (const overload)
   @param axisId Axis ID
   @return Scale engine
 */
const QwtScaleEngine* QwtPlot::axisScaleEngine(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return m_scaleData->axisData(axisId).scaleEngine;
    else
        return nullptr;
}

/*!
   @brief Return whether autoscaling is enabled
   @param axisId Axis ID
   @return \c true if autoscaling is enabled
 */
bool QwtPlot::axisAutoScale(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return m_scaleData->axisData(axisId).doAutoScale;
    else
        return false;
}

/*!
   @brief Return whether the specified axis is visible
   @param axisId Axis ID
   @return \c true if the axis is visible
 */
bool QwtPlot::isAxisVisible(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return m_scaleData->axisData(axisId).isVisible;
    else
        return false;
}

/*!
   @brief Return the font of the scale labels for a specified axis
   @param axisId Axis ID
   @return Font
 */
QFont QwtPlot::axisFont(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return axisWidget(axisId)->font();
    else
        return QFont();
}

/*!
   @brief Return the maximum number of major ticks for a specified axis
   @param axisId Axis ID
   @sa setAxisMaxMajor(), QwtScaleEngine::divideScale()
 */
int QwtPlot::axisMaxMajor(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return m_scaleData->axisData(axisId).maxMajor;
    else
        return 0;
}

/*!
   @brief Return the maximum number of minor ticks for a specified axis
   @param axisId Axis ID
   @sa setAxisMaxMinor(), QwtScaleEngine::divideScale()
 */
int QwtPlot::axisMaxMinor(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return m_scaleData->axisData(axisId).maxMinor;
    else
        return 0;
}

/**
 * @brief Return the scale division of a specified axis
 *
 * axisScaleDiv(axisId).lowerBound(), axisScaleDiv(axisId).upperBound()
 * are the current limits of the axis scale.
 *
 * @param axisId Axis ID
 * @return Scale division
 *
 * @sa QwtScaleDiv, setAxisScaleDiv(), QwtScaleEngine::divideScale()
 */
const QwtScaleDiv& QwtPlot::axisScaleDiv(QwtAxisId axisId) const
{
    return m_scaleData->axisData(axisId).scaleDiv;
}

/**
 * @brief Return the scale draw of a specified axis
 *
 * @param axisId Axis ID
 * @return Specified scaleDraw for axis, or nullptr if axis is invalid.
 */
const QwtScaleDraw* QwtPlot::axisScaleDraw(QwtAxisId axisId) const
{
    if (!isAxisValid(axisId))
        return nullptr;

    return axisWidget(axisId)->scaleDraw();
}

/**
 * @brief Return the scale draw of a specified axis
 *
 * @param axisId Axis ID
 * @return Specified scaleDraw for axis, or nullptr if axis is invalid.
 */
QwtScaleDraw* QwtPlot::axisScaleDraw(QwtAxisId axisId)
{
    if (!isAxisValid(axisId)) {
        return nullptr;
    }

    return axisWidget(axisId)->scaleDraw();
}

/**
 * @brief Return the step size parameter that has been set in setAxisScale.
 *
 * This doesn't need to be the step size of the current scale.
 *
 * @param axisId Axis ID
 * @return step size parameter value
 *
 * @sa setAxisScale(), QwtScaleEngine::divideScale()
 */
double QwtPlot::axisStepSize(QwtAxisId axisId) const
{
    if (!isAxisValid(axisId))
        return 0;

    return m_scaleData->axisData(axisId).stepSize;
}

/**
 * @brief Return the current interval of the specified axis
 *
 * This is only a convenience function for axisScaleDiv( axisId )->interval();
 *
 * @param axisId Axis ID
 * @return Scale interval
 *
 * @sa QwtScaleDiv, axisScaleDiv()
 */
QwtInterval QwtPlot::axisInterval(QwtAxisId axisId) const
{
    if (!isAxisValid(axisId))
        return QwtInterval();

    return m_scaleData->axisData(axisId).scaleDiv.interval();
}

/**
 * @brief Get the title of a specified axis
 *
 * @param axisId Axis ID
 * @return Title of a specified axis
 */
QwtText QwtPlot::axisTitle(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return axisWidget(axisId)->title();
    else
        return QwtText();
}

/**
 * @brief Hide or show a specified axis
 *
 * Curves, markers and other items can be attached
 * to hidden axes, and transformation of screen coordinates
 * into values works as normal.
 *
 * Only QwtAxis::XBottom and QwtAxis::YLeft are enabled by default.
 *
 * @param axisId Axis ID
 * @param on \c true (visible) or \c false (hidden)
 */
void QwtPlot::setAxisVisible(QwtAxisId axisId, bool on)
{
    if (isAxisValid(axisId) && on != m_scaleData->axisData(axisId).isVisible) {
        m_scaleData->axisData(axisId).isVisible = on;
        updateLayout();
    }
}

/**
 * @brief Transform the x or y coordinate of a position in the drawing region into a value.
 *
 * Transform the x or y coordinate of a position in the
 * drawing region into a value.
 *
 * @param axisId Axis ID
 * @param pos position
 * @return Position as axis coordinate
 *
 * @warning The position can be an x or a y coordinate,
 *           depending on the specified axis.
 */
double QwtPlot::invTransform(QwtAxisId axisId, double pos) const
{
    if (isAxisValid(axisId))
        return (canvasMap(axisId).invTransform(pos));
    else
        return 0.0;
}

/**
 * @brief Transform a value into a coordinate in the plotting region
 *
 * @param axisId Axis ID
 * @param value value
 * @return X or Y coordinate in the plotting region corresponding to the value.
 */
double QwtPlot::transform(QwtAxisId axisId, double value) const
{
    if (isAxisValid(axisId))
        return (canvasMap(axisId).transform(value));
    else
        return 0.0;
}

/**
 * @brief Change the font of an axis
 *
 * @param axisId Axis ID
 * @param font Font
 *
 * @warning This function changes the font of the tick labels,
 *           not of the axis title.
 */
void QwtPlot::setAxisFont(QwtAxisId axisId, const QFont& font)
{
    if (isAxisValid(axisId))
        axisWidget(axisId)->setFont(font);
}

/**
 * @brief Enable autoscaling for a specified axis
 *
 * This member function is used to switch back to autoscaling mode
 * after a fixed scale has been set. Autoscaling is enabled by default.
 *
 * @param axisId Axis ID
 * @param on On/Off
 *
 * @sa setAxisScale(), setAxisScaleDiv(), updateAxes()
 *
 * @note The autoscaling flag has no effect until updateAxes() is executed
 *        ( called by replot() ).
 */
void QwtPlot::setAxisAutoScale(QwtAxisId axisId, bool on)
{
    if (isAxisValid(axisId) && (m_scaleData->axisData(axisId).doAutoScale != on)) {
        m_scaleData->axisData(axisId).doAutoScale = on;
        autoRefresh();
    }
}

/**
 * @brief Disable autoscaling and specify a fixed scale for a selected axis
 *
 * In updateAxes() the scale engine calculates a scale division from the
 * specified parameters, that will be assigned to the scale widget. So
 * updates of the scale widget usually happen delayed with the next replot.
 *
 * @param axisId Axis ID
 * @param min Minimum of the scale
 * @param max Maximum of the scale
 * @param stepSize Major step size. If <code>step == 0</code>, the step size is
 *                 calculated automatically using the maxMajor setting.
 *
 * @sa setAxisMaxMajor(), setAxisAutoScale(), axisStepSize(), QwtScaleEngine::divideScale()
 */
void QwtPlot::setAxisScale(QwtAxisId axisId, double min, double max, double stepSize)
{
    if (isAxisValid(axisId)) {

        AxisData& d = m_scaleData->axisData(axisId);
        if (qFuzzyCompare(d.minValue, min) && qFuzzyCompare(d.maxValue, max) && qFuzzyCompare(d.stepSize, stepSize)) {
            // Nothing to change
            return;
        }

        d.doAutoScale = false;
        d.isValid     = false;

        d.minValue = min;
        d.maxValue = max;
        d.stepSize = stepSize;

        autoRefresh();
    }
}

/**
 * @brief Disable autoscaling and specify a fixed scale for a selected axis.
 *
 * The scale division will be stored locally only until the next call
 * of updateAxes(). So updates of the scale widget usually happen delayed with
 * the next replot.
 *
 * @param axisId Axis ID
 * @param scaleDiv Scale division
 *
 * @sa setAxisScale(), setAxisAutoScale()
 */
void QwtPlot::setAxisScaleDiv(QwtAxisId axisId, const QwtScaleDiv& scaleDiv)
{
    if (isAxisValid(axisId)) {
        AxisData& d = m_scaleData->axisData(axisId);
        if (d.scaleDiv.fuzzyCompare(scaleDiv)) {
            // Nothing to change
            return;
        }
        d.doAutoScale = false;
        d.scaleDiv    = scaleDiv;
        d.isValid     = true;

        autoRefresh();
    }
}

/**
 * @brief Set a scale draw
 *
 * By passing scaleDraw it is possible to extend QwtScaleDraw
 * functionality and let it take place in QwtPlot. Please note
 * that scaleDraw has to be created with new and will be deleted
 * by the corresponding QwtScale member ( like a child object ).
 *
 * @param axisId Axis ID
 * @param scaleDraw Object responsible for drawing scales.
 *
 * @sa QwtScaleDraw, QwtScaleWidget
 *
 * @warning The attributes of scaleDraw will be overwritten by those of the
 *           previous QwtScaleDraw.
 */
void QwtPlot::setAxisScaleDraw(QwtAxisId axisId, QwtScaleDraw* scaleDraw)
{
    if (isAxisValid(axisId)) {
        axisWidget(axisId)->setScaleDraw(scaleDraw);
        autoRefresh();
    }
}

/**
 * @brief Change the alignment of the tick labels
 *
 * @param axisId Axis ID
 * @param alignment Or'd Qt::AlignmentFlags see <qnamespace.h>
 *
 * @sa QwtScaleDraw::setLabelAlignment()
 */
void QwtPlot::setAxisLabelAlignment(QwtAxisId axisId, Qt::Alignment alignment)
{
    if (isAxisValid(axisId))
        axisWidget(axisId)->setLabelAlignment(alignment);
}

/**
 * @brief Rotate all tick labels
 *
 * @param axisId Axis ID
 * @param rotation Angle in degrees. When changing the label rotation, the label alignment might be adjusted
 * too.
 *
 * @sa QwtScaleDraw::setLabelRotation(), setAxisLabelAlignment()
 */
void QwtPlot::setAxisLabelRotation(QwtAxisId axisId, double rotation)
{
    if (isAxisValid(axisId))
        axisWidget(axisId)->setLabelRotation(rotation);
}

/**
 * @brief Set the maximum number of minor scale intervals for a specified axis
 *
 * @param axisId Axis ID
 * @param maxMinor Maximum number of minor steps
 *
 * @sa axisMaxMinor()
 */
void QwtPlot::setAxisMaxMinor(QwtAxisId axisId, int maxMinor)
{
    if (isAxisValid(axisId)) {
        maxMinor = qBound(0, maxMinor, 100);

        AxisData& d = m_scaleData->axisData(axisId);
        if (maxMinor != d.maxMinor) {
            d.maxMinor = maxMinor;
            d.isValid  = false;
            autoRefresh();
        }
    }
}

/**
 * @brief Set the maximum number of major scale intervals for a specified axis
 *
 * @param axisId Axis ID
 * @param maxMajor Maximum number of major steps
 *
 * @sa axisMaxMajor()
 */
void QwtPlot::setAxisMaxMajor(QwtAxisId axisId, int maxMajor)
{
    if (isAxisValid(axisId)) {
        maxMajor = qBound(1, maxMajor, 10000);

        AxisData& d = m_scaleData->axisData(axisId);
        if (maxMajor != d.maxMajor) {
            d.maxMajor = maxMajor;
            d.isValid  = false;
            autoRefresh();
        }
    }
}

/**
 * @brief Change the title of a specified axis
 *
 * @param axisId Axis ID
 * @param title axis title
 */
void QwtPlot::setAxisTitle(QwtAxisId axisId, const QString& title)
{
    if (isAxisValid(axisId))
        axisWidget(axisId)->setTitle(title);
}

/**
 * @brief Change the title of a specified axis
 *
 * @param axisId Axis ID
 * @param title Axis title
 */
void QwtPlot::setAxisTitle(QwtAxisId axisId, const QwtText& title)
{
    if (isAxisValid(axisId))
        axisWidget(axisId)->setTitle(title);
}

/**
 * @brief Set the position of an axis title along the backbone
 * @param axisId Axis ID
 * @param position Title position along the backbone
 * @sa QwtScaleWidget::setTitlePosition(), axisTitlePosition()
 */
void QwtPlot::setAxisTitlePosition(QwtAxisId axisId, QwtScaleWidget::TitlePosition position)
{
    if (isAxisValid(axisId))
        axisWidget(axisId)->setTitlePosition(position);
}

/**
 * @brief Get the position of an axis title along the backbone
 * @param axisId Axis ID
 * @return Title position along the backbone
 * @sa setAxisTitlePosition()
 */
QwtScaleWidget::TitlePosition QwtPlot::axisTitlePosition(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return axisWidget(axisId)->titlePosition();

    return QwtScaleWidget::TitleCentered;
}

/**
 * @brief Set the horizontal alignment of an axis title
 * @param axisId Axis ID
 * @param alignment Horizontal alignment within the title box
 * @sa QwtScaleWidget::setTitleAlignment(), axisTitleAlignment()
 */
void QwtPlot::setAxisTitleAlignment(QwtAxisId axisId, Qt::Alignment alignment)
{
    if (isAxisValid(axisId))
        axisWidget(axisId)->setTitleAlignment(alignment);
}

/**
 * @brief Get the horizontal alignment of an axis title
 * @param axisId Axis ID
 * @return Horizontal alignment within the title box
 * @sa setAxisTitleAlignment()
 */
Qt::Alignment QwtPlot::axisTitleAlignment(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return axisWidget(axisId)->titleAlignment();

    return Qt::AlignHCenter;
}

/**
 * @brief Rebuild the axes scales
 *
 * In case of autoscaling the boundaries of a scale are calculated
 * from the bounding rectangles of all plot items, having the
 * QwtPlotItem::AutoScale flag enabled ( QwtScaleEngine::autoScale() ).
 * Then a scale division is calculated ( QwtScaleEngine::divideScale() )
 * and assigned to scale widget.
 *
 * When the scale boundaries have been assigned with setAxisScale() a
 * scale division is calculated ( QwtScaleEngine::divideScale() )
 * for this interval and assigned to the scale widget.
 *
 * When the scale has been set explicitly by setAxisScaleDiv() the
 * locally stored scale division gets assigned to the scale widget.
 *
 * The scale widget indicates modifications by emitting a
 * QwtScaleWidget::scaleDivChanged() signal.
 *
 * updateAxes() is usually called by replot().
 *
 * @sa setAxisAutoScale(), setAxisScale(), setAxisScaleDiv(), replot(),
 *     QwtPlotItem::boundingRect()
 */
void QwtPlot::updateAxes()
{
    // Find bounding interval of the item data
    // for all axes, where autoscaling is enabled

    QwtInterval boundingIntervals[ QwtAxis::AxisPositions ];

    const QwtPlotItemList& itmList = itemList();

    QwtPlotItemIterator it;
    for (it = itmList.begin(); it != itmList.end(); ++it) {
        const QwtPlotItem* item = *it;

        if (!item->testItemAttribute(QwtPlotItem::AutoScale))
            continue;

        if (!item->isVisible())
            continue;

        if (axisAutoScale(item->xAxis()) || axisAutoScale(item->yAxis())) {
            const QRectF rect = item->boundingRect();

            if (rect.width() >= 0.0)
                boundingIntervals[ item->xAxis() ] |= QwtInterval(rect.left(), rect.right());

            if (rect.height() >= 0.0)
                boundingIntervals[ item->yAxis() ] |= QwtInterval(rect.top(), rect.bottom());
        }
    }

    // Adjust scales

    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++) {
        {
            const QwtAxisId axisId(axisPos);

            AxisData& d = m_scaleData->axisData(axisId);

            double minValue = d.minValue;
            double maxValue = d.maxValue;
            double stepSize = d.stepSize;

            const QwtInterval& interval = boundingIntervals[ axisId ];

            if (d.doAutoScale && interval.isValid()) {
                d.isValid = false;

                minValue = interval.minValue();
                maxValue = interval.maxValue();

                d.scaleEngine->autoScale(d.maxMajor, minValue, maxValue, stepSize);
            }
            if (!d.isValid) {
                d.scaleDiv = d.scaleEngine->divideScale(minValue, maxValue, d.maxMajor, d.maxMinor, stepSize);
                d.isValid  = true;
            }

            QwtScaleWidget* scaleWidget = axisWidget(axisId);
            scaleWidget->setScaleDiv(d.scaleDiv);

            int startDist, endDist;
            scaleWidget->getBorderDistHint(startDist, endDist);
            scaleWidget->setBorderDist(startDist, endDist);
        }
    }

    for (it = itmList.begin(); it != itmList.end(); ++it) {
        QwtPlotItem* item = *it;
        if (item->testItemInterest(QwtPlotItem::ScaleInterest)) {
            item->updateScaleDiv(axisScaleDiv(item->xAxis()), axisScaleDiv(item->yAxis()));
        }
    }
}

/**
 * @brief Update the scale range of all plot items to match the current axis scales.
 *
 * This is typically called when axes change, forcing all plot items to update their ranges.
 */
void QwtPlot::updateItemsToScaleDiv()
{
    const QwtPlotItemList& itmList = itemList();
    QwtPlotItemIterator it;
    for (it = itmList.begin(); it != itmList.end(); ++it) {
        QwtPlotItem* item = *it;
        item->updateScaleDiv(axisScaleDiv(item->xAxis()), axisScaleDiv(item->yAxis()));
    }
}

/**
 * @brief Request scale range update for the YLeft axis
 * @param min Minimum scale value
 * @param max Maximum scale value
 */
void QwtPlot::yLeftRequestScaleRangeUpdate(double min, double max)
{

    setAxisScale(QwtAxis::YLeft, min, max);
    replotAll();
}

/**
 * @brief Request scale range update for the YRight axis
 * @param min Minimum scale value
 * @param max Maximum scale value
 */
void QwtPlot::yRightRequestScaleRangeUpdate(double min, double max)
{
    setAxisScale(QwtAxis::YRight, min, max);
    replotAll();
}

/**
 * @brief Request scale range update for the XBottom axis
 * @param min Minimum scale value
 * @param max Maximum scale value
 */
void QwtPlot::xBottomRequestScaleRangeUpdate(double min, double max)
{
    setAxisScale(QwtAxis::XBottom, min, max);
    replotAll();
}

/**
 * @brief Request scale range update for the XTop axis
 * @param min Minimum scale value
 * @param max Maximum scale value
 */
void QwtPlot::xTopRequestScaleRangeUpdate(double min, double max)
{
    setAxisScale(QwtAxis::XTop, min, max);
    replotAll();
}
