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
        , scaleWidget(NULL)
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

    bool isValid;  ///< 用于表征scaleDiv是否有效

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

    connect(
        m_scaleData->axisData(QwtAxis::YLeft).scaleWidget, &QwtScaleWidget::requestScaleRangeUpdate, this, &QwtPlot::yLeftRequestScaleRangeUpdate
    );
    connect(
        m_scaleData->axisData(QwtAxis::YRight).scaleWidget, &QwtScaleWidget::requestScaleRangeUpdate, this, &QwtPlot::yRightRequestScaleRangeUpdate
    );
    connect(
        m_scaleData->axisData(QwtAxis::XBottom).scaleWidget, &QwtScaleWidget::requestScaleRangeUpdate, this, &QwtPlot::xBottomRequestScaleRangeUpdate
    );
    connect(
        m_scaleData->axisData(QwtAxis::XTop).scaleWidget, &QwtScaleWidget::requestScaleRangeUpdate, this, &QwtPlot::xTopRequestScaleRangeUpdate
    );
}

void QwtPlot::deleteAxesData()
{
    delete m_scaleData;
    m_scaleData = NULL;
}

/*!
   \brief Checks if an axis is valid/检查轴是否有效
   \param axisId axis/轴 ID
   \return \c true if the specified axis exists, otherwise \c false/指定轴存在返回 true，否则返回 false

   \note This method is equivalent to QwtAxis::isValid( axisId ) and simply checks
         if axisId is one of the values of QwtAxis::Position. It is a placeholder
         for future releases, where it will be possible to have a customizable number
         of axes ( multiaxes branch ) at each side.

         本方法等价于 QwtAxis::isValid(axisId)，仅检查 axisId 是否为 QwtAxis::Position 中的值。
         为未来多轴分支预留接口。
 */
bool QwtPlot::isAxisValid(QwtAxisId axisId) const
{
    return QwtAxis::isValid(axisId);
}

/*!
   \brief Return the scale widget of the specified axis/返回指定轴的刻度控件
   \param axisId Axis/轴 ID
   \return Scale widget, or NULL if axisId is invalid/刻度控件指针；轴无效时返回 NULL
 */
const QwtScaleWidget* QwtPlot::axisWidget(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return m_scaleData->axisData(axisId).scaleWidget;

    return NULL;
}

/*!
   \brief Return the scale widget of the specified axis/返回指定轴的刻度控件
   \param axisId Axis/轴 ID
   \return Scale widget, or NULL if axisId is invalid/刻度控件指针；轴无效时返回 NULL
 */
QwtScaleWidget* QwtPlot::axisWidget(QwtAxisId axisId)
{
    if (isAxisValid(axisId)) {
        return m_scaleData->axisData(axisId).scaleWidget;
    }
    return nullptr;
}

/**
 * @brief Return the currently visible X axis/返回当前可用的 X 轴
 *
 * Selection policy (descending priority):
 * 1. Axis must be visible (isAxisVisible() == true);
 * 2. If both XBottom and XTop are visible, prefer XBottom;
 * 3. If only one X axis is visible, return it;
 * 4. If both are invisible, return default QwtAxis::XBottom.
 *
 * 选择策略（按优先级递减）:
 * 1. 轴必须可见（isAxisVisible() == true）；
 * 2. 若 XBottom 与 XTop 均可见，优先返回 XBottom；
 * 3. 若仅一条 X 轴可见，返回该轴；
 * 4. 若两条 X 轴均不可见，返回默认 QwtAxis::XBottom。
 *
 * @return Selected X axis ID/选中的 X 轴 ID
 * @note 此函数必定返回
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
 * @brief Return the currently usable Y axis/返回当前可用的 Y 轴
 *
 * Selection policy (descending priority):
 * 1. Axis must be visible;
 * 2. If both YLeft and YRight are visible, prefer YLeft;
 * 3. If only one Y axis is visible, return it;
 * 4. If both are invisible, return default QwtAxis::YLeft.
 *
 * 选择策略（按优先级递减）:
 * 1. 轴必须可见；
 * 2. 若 YLeft 与 YRight 均可见，优先返回 YLeft；
 * 3. 若仅一条 Y 轴可见，返回该轴；
 * 4. 若两条 Y 轴均不可见，返回默认 QwtAxis::YLeft。
 *
 * @return Selected Y axis ID/选中的 Y 轴 ID
 * @note 此函数必定返回
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
   \brief Change the scale engine for an axis/更改指定轴的刻度引擎
   \param axisId Axis/轴 ID
   \param scaleEngine Scale engine/刻度引擎指针
   \note The old scale engine will be deleted/旧的刻度引擎将被删除
   \sa axisScaleEngine()
 */
void QwtPlot::setAxisScaleEngine(QwtAxisId axisId, QwtScaleEngine* scaleEngine)
{
    if (isAxisValid(axisId) && scaleEngine != NULL) {
        AxisData& d = m_scaleData->axisData(axisId);

        delete d.scaleEngine;
        d.scaleEngine = scaleEngine;

        d.scaleWidget->setTransformation(scaleEngine->transformation());

        d.isValid = false;

        autoRefresh();
    }
}

/*!
   \brief Return the scale engine for a specific axis/返回指定轴的刻度引擎
   \param axisId Axis/轴 ID
   \return Scale engine/刻度引擎指针
 */
QwtScaleEngine* QwtPlot::axisScaleEngine(QwtAxisId axisId)
{
    if (isAxisValid(axisId))
        return m_scaleData->axisData(axisId).scaleEngine;
    else
        return NULL;
}

/*!
   \brief Return the scale engine for a specific axis/返回指定轴的刻度引擎（const 重载）
   \param axisId Axis/轴 ID
   \return Scale engine/刻度引擎指针
 */
const QwtScaleEngine* QwtPlot::axisScaleEngine(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return m_scaleData->axisData(axisId).scaleEngine;
    else
        return NULL;
}

/*!
   \brief Return whether autoscaling is enabled/返回指定轴是否启用自动缩放
   \param axisId Axis/轴 ID
   \return \c true if autoscaling is enabled/启用自动缩放返回 true
 */
bool QwtPlot::axisAutoScale(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return m_scaleData->axisData(axisId).doAutoScale;
    else
        return false;
}

/*!
   \brief Return whether the specified axis is visible/返回指定轴是否可见
   \param axisId Axis/轴 ID
   \return \c true if the axis is visible/轴可见返回 true
 */
bool QwtPlot::isAxisVisible(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return m_scaleData->axisData(axisId).isVisible;
    else
        return false;
}

/*!
   \brief Return the font of the scale labels for a specified axis/返回指定轴刻度标签的字体
   \param axisId Axis/轴 ID
   \return Font/字体
 */
QFont QwtPlot::axisFont(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return axisWidget(axisId)->font();
    else
        return QFont();
}

/*!
   \brief Return the maximum number of major ticks for a specified axis/返回指定轴主刻度的最大数量
   \param axisId Axis/轴 ID
   \sa setAxisMaxMajor(), QwtScaleEngine::divideScale()
 */
int QwtPlot::axisMaxMajor(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return m_scaleData->axisData(axisId).maxMajor;
    else
        return 0;
}

/*!
   \brief Return the maximum number of minor ticks for a specified axis/返回指定轴副刻度的最大数量
   \param axisId Axis/轴 ID
   \sa setAxisMaxMinor(), QwtScaleEngine::divideScale()
 */
int QwtPlot::axisMaxMinor(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return m_scaleData->axisData(axisId).maxMinor;
    else
        return 0;
}

/**
 * @brief Return the scale division of a specified axis/返回指定轴的刻度划分
 *
 * axisScaleDiv(axisId).lowerBound(), axisScaleDiv(axisId).upperBound()
 * are the current limits of the axis scale.
 *
 * axisScaleDiv(axisId).lowerBound()、axisScaleDiv(axisId).upperBound()
 * 分别为该轴刻度当前的下限值和上限值。
 *
 * @param axisId Axis/轴标识
 * @return Scale division/刻度划分
 *
 * @sa QwtScaleDiv, setAxisScaleDiv(), QwtScaleEngine::divideScale()
 */
const QwtScaleDiv& QwtPlot::axisScaleDiv(QwtAxisId axisId) const
{
    return m_scaleData->axisData(axisId).scaleDiv;
}

/**
 * @brief Return the scale draw of a specified axis/返回指定轴的刻度绘制对象
 *
 * @param axisId Axis/轴标识
 * @return Specified scaleDraw for axis, or NULL if axis is invalid./指定轴的刻度绘制对象；若轴无效则返回NULL
 */
const QwtScaleDraw* QwtPlot::axisScaleDraw(QwtAxisId axisId) const
{
    if (!isAxisValid(axisId))
        return NULL;

    return axisWidget(axisId)->scaleDraw();
}

/**
 * @brief Return the scale draw of a specified axis/返回指定轴的刻度绘制对象
 *
 * @param axisId Axis/轴标识
 * @return Specified scaleDraw for axis, or NULL if axis is invalid./指定轴的刻度绘制对象；若轴无效则返回NULL
 */
QwtScaleDraw* QwtPlot::axisScaleDraw(QwtAxisId axisId)
{
    if (!isAxisValid(axisId)) {
        return nullptr;
    }

    return axisWidget(axisId)->scaleDraw();
}

/**
 * @brief Return the step size parameter that has been set in setAxisScale./返回在 setAxisScale 中设置的步长参数
 *
 * This doesn't need to be the step size of the current scale.
 *
 * 该参数不一定是当前刻度的实际步长。
 *
 * @param axisId Axis/轴标识
 * @return step size parameter value/步长参数值
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
 * @brief Return the current interval of the specified axis/返回指定轴的当前区间
 *
 * This is only a convenience function for axisScaleDiv( axisId )->interval();
 *
 * 此函数仅为 axisScaleDiv( axisId )->interval() 的便捷封装函数。
 *
 * @param axisId Axis/轴标识
 * @return Scale interval/刻度区间
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
 * @brief Get the title of a specified axis/获取指定轴的标题
 *
 * @param axisId Axis/轴标识
 * @return Title of a specified axis/指定轴的标题
 */
QwtText QwtPlot::axisTitle(QwtAxisId axisId) const
{
    if (isAxisValid(axisId))
        return axisWidget(axisId)->title();
    else
        return QwtText();
}

/**
 * @brief Hide or show a specified axis/显示或隐藏指定轴
 *
 * Curves, markers and other items can be attached
 * to hidden axes, and transformation of screen coordinates
 * into values works as normal.
 *
 * 曲线、标记点及其他元素仍可关联至隐藏的轴，且屏幕坐标与数值之间的转换
 * 仍正常生效。
 *
 * Only QwtAxis::XBottom and QwtAxis::YLeft are enabled by default.
 *
 * 默认情况下，仅启用 QwtAxis::XBottom（下X轴）和 QwtAxis::YLeft（左Y轴）。
 *
 * @param axisId Axis/轴标识
 * @param on \c true (visible) or \c false (hidden)/\c true 表示显示，\c false 表示隐藏
 */
void QwtPlot::setAxisVisible(QwtAxisId axisId, bool on)
{
    if (isAxisValid(axisId) && on != m_scaleData->axisData(axisId).isVisible) {
        m_scaleData->axisData(axisId).isVisible = on;
        updateLayout();
    }
}

/**
 * @brief Transform the x or y coordinate of a position in the drawing region into a value./将绘图区域中某个位置的X或Y坐标转换为轴对应的值
 *
 * Transform the x or y coordinate of a position in the
 * drawing region into a value.
 *
 * 将绘图区域中某个位置的X坐标或Y坐标，转换为对应轴上的数值。
 *
 * @param axisId Axis/轴标识
 * @param pos position/位置坐标
 * @return Position as axis coordinate/转换后的轴坐标值
 *
 * @warning The position can be an x or a y coordinate,
 *           depending on the specified axis.
 * @warning 该位置坐标可能是X坐标或Y坐标，具体取决于指定的轴类型。
 */
double QwtPlot::invTransform(QwtAxisId axisId, double pos) const
{
    if (isAxisValid(axisId))
        return (canvasMap(axisId).invTransform(pos));
    else
        return 0.0;
}

/**
 * @brief Transform a value into a coordinate in the plotting region/将数值转换为绘图区域中的坐标
 *
 * @param axisId Axis/轴标识
 * @param value value/待转换的数值
 * @return X or Y coordinate in the plotting region corresponding to the value./与该数值对应的绘图区域中的X坐标或Y坐标
 */
double QwtPlot::transform(QwtAxisId axisId, double value) const
{
    if (isAxisValid(axisId))
        return (canvasMap(axisId).transform(value));
    else
        return 0.0;
}

/**
 * @brief Change the font of an axis/修改轴的字体
 *
 * @param axisId Axis/轴标识
 * @param font Font/要设置的字体
 *
 * @warning This function changes the font of the tick labels,
 *           not of the axis title.
 * @warning 此函数仅修改刻度标签的字体，不会改变轴标题的字体。
 */
void QwtPlot::setAxisFont(QwtAxisId axisId, const QFont& font)
{
    if (isAxisValid(axisId))
        axisWidget(axisId)->setFont(font);
}

/**
 * @brief Enable autoscaling for a specified axis/为指定轴启用自动缩放
 *
 * This member function is used to switch back to autoscaling mode
 * after a fixed scale has been set. Autoscaling is enabled by default.
 *
 * 此成员函数用于在设置了固定刻度后，切换回自动缩放模式。默认情况下自动缩放处于启用状态。
 *
 * @param axisId Axis/轴标识
 * @param on On/Off/启用（true）或禁用（false）
 *
 * @sa setAxisScale(), setAxisScaleDiv(), updateAxes()
 *
 * @note The autoscaling flag has no effect until updateAxes() is executed
 *        ( called by replot() ).
 *
 *       自动缩放标志在执行 updateAxes() 函数（由 replot() 调用）之前不会生效。
 */
void QwtPlot::setAxisAutoScale(QwtAxisId axisId, bool on)
{
    if (isAxisValid(axisId) && (m_scaleData->axisData(axisId).doAutoScale != on)) {
        m_scaleData->axisData(axisId).doAutoScale = on;
        autoRefresh();
    }
}

/**
 * @brief Disable autoscaling and specify a fixed scale for a selected axis/为指定坐标轴关闭自动缩放并设定固定刻度范围
 *
 * In updateAxes() the scale engine calculates a scale division from the
 * specified parameters, that will be assigned to the scale widget. So
 * updates of the scale widget usually happen delayed with the next replot.
 *
 * 在 updateAxes() 中，刻度引擎会根据给定参数计算出刻度分段，并将其赋给刻度控件。
 * 因此，刻度控件的更新通常会延迟到下一次重绘时才发生。
 *
 * @param axisId Axis/坐标轴索引
 * @param min Minimum of the scale/刻度最小值
 * @param max Maximum of the scale/刻度最大值
 * @param stepSize Major step size. If <code>step == 0</code>, the step size is
 *                 calculated automatically using the maxMajor setting.
 *                 主步长。若 <code>step == 0</code>，则使用 maxMajor 设置自动计算步长。
 *
 * @sa setAxisMaxMajor(), setAxisAutoScale(), axisStepSize(), QwtScaleEngine::divideScale()
 */
void QwtPlot::setAxisScale(QwtAxisId axisId, double min, double max, double stepSize)
{
    if (isAxisValid(axisId)) {

        AxisData& d = m_scaleData->axisData(axisId);
        if (qFuzzyCompare(d.minValue, min) && qFuzzyCompare(d.maxValue, max) && qFuzzyCompare(d.stepSize, stepSize)) {
            // 都一样就不设置
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
 * @brief Disable autoscaling and specify a fixed scale for a selected axis./禁用自动缩放并为选定轴指定固定刻度
 *
 * The scale division will be stored locally only until the next call
 * of updateAxes(). So updates of the scale widget usually happen delayed with
 * the next replot.
 *
 * 该刻度划分仅在本地存储，直至下一次调用 updateAxes() 为止。因此，刻度部件的更新通常会延迟至
 * 下一次 replot() 时执行。
 *
 * @param axisId Axis/轴标识
 * @param scaleDiv Scale division/刻度划分
 *
 * @sa setAxisScale(), setAxisAutoScale()
 */
void QwtPlot::setAxisScaleDiv(QwtAxisId axisId, const QwtScaleDiv& scaleDiv)
{
    if (isAxisValid(axisId)) {
        AxisData& d = m_scaleData->axisData(axisId);
        if (d.scaleDiv.fuzzyCompare(scaleDiv)) {
            // 都一样就不设置
            return;
        }
        d.doAutoScale = false;
        d.scaleDiv    = scaleDiv;
        d.isValid     = true;

        autoRefresh();
    }
}

/**
 * @brief Set a scale draw/设置刻度绘制对象
 *
 * By passing scaleDraw it is possible to extend QwtScaleDraw
 * functionality and let it take place in QwtPlot. Please note
 * that scaleDraw has to be created with new and will be deleted
 * by the corresponding QwtScale member ( like a child object ).
 *
 * 通过传入 scaleDraw 对象，可以扩展 QwtScaleDraw 的功能并使其在 QwtPlot 中生效。
 * 请注意，scaleDraw 必须通过 new 关键字创建，且会由对应的 QwtScale 成员（类似子对象）自动销毁。
 *
 * @param axisId Axis/轴标识
 * @param scaleDraw Object responsible for drawing scales./负责刻度绘制的对象
 *
 * @sa QwtScaleDraw, QwtScaleWidget
 *
 * @warning The attributes of scaleDraw will be overwritten by those of the
 *           previous QwtScaleDraw.
 * @warning scaleDraw 的属性会被之前的 QwtScaleDraw 对象的属性覆盖。
 */
void QwtPlot::setAxisScaleDraw(QwtAxisId axisId, QwtScaleDraw* scaleDraw)
{
    if (isAxisValid(axisId)) {
        axisWidget(axisId)->setScaleDraw(scaleDraw);
        autoRefresh();
    }
}

/**
 * @brief Change the alignment of the tick labels/修改刻度标签的对齐方式
 *
 * @param axisId Axis/轴标识
 * @param alignment Or'd Qt::AlignmentFlags see <qnamespace.h>/Qt::AlignmentFlags 组合值（详见 <qnamespace.h>）
 *
 * @sa QwtScaleDraw::setLabelAlignment()
 */
void QwtPlot::setAxisLabelAlignment(QwtAxisId axisId, Qt::Alignment alignment)
{
    if (isAxisValid(axisId))
        axisWidget(axisId)->setLabelAlignment(alignment);
}

/**
 * @brief Rotate all tick labels/旋转所有刻度标签
 *
 * @param axisId Axis/轴标识
 * @param rotation Angle in degrees. When changing the label rotation, the label alignment might be adjusted
 * too./旋转角度（以度为单位）。修改标签旋转角度时，标签对齐方式可能也会随之调整
 *
 * @sa QwtScaleDraw::setLabelRotation(), setAxisLabelAlignment()
 */
void QwtPlot::setAxisLabelRotation(QwtAxisId axisId, double rotation)
{
    if (isAxisValid(axisId))
        axisWidget(axisId)->setLabelRotation(rotation);
}

/**
 * @brief Set the maximum number of minor scale intervals for a specified axis/为指定轴设置次要刻度区间的最大数量
 *
 * @param axisId Axis/轴标识
 * @param maxMinor Maximum number of minor steps/次要刻度的最大数量
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
 * @brief Set the maximum number of major scale intervals for a specified axis/为指定轴设置主要刻度区间的最大数量
 *
 * @param axisId Axis/轴标识
 * @param maxMajor Maximum number of major steps/主要刻度的最大数量
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
 * @brief Change the title of a specified axis/修改指定轴的标题
 *
 * @param axisId Axis/轴标识
 * @param title axis title/轴标题
 */
void QwtPlot::setAxisTitle(QwtAxisId axisId, const QString& title)
{
    if (isAxisValid(axisId))
        axisWidget(axisId)->setTitle(title);
}

/**
 * @brief Change the title of a specified axis/修改指定轴的标题
 *
 * @param axisId Axis/轴标识
 * @param title Axis title/轴标题
 */
void QwtPlot::setAxisTitle(QwtAxisId axisId, const QwtText& title)
{
    if (isAxisValid(axisId))
        axisWidget(axisId)->setTitle(title);
}

/**
 * @brief Rebuild the axes scales/重建坐标轴刻度
 *
 * In case of autoscaling the boundaries of a scale are calculated
 * from the bounding rectangles of all plot items, having the
 * QwtPlotItem::AutoScale flag enabled ( QwtScaleEngine::autoScale() ).
 * Then a scale division is calculated ( QwtScaleEngine::divideScale() )
 * and assigned to scale widget.
 *
 * 若启用自动缩放，刻度的边界将根据所有启用了 QwtPlotItem::AutoScale 标志的绘图项的边界矩形计算得出
 *（调用 QwtScaleEngine::autoScale() 方法）。随后会计算刻度划分（调用 QwtScaleEngine::divideScale() 方法），
 * 并将其分配给刻度部件。
 *
 * When the scale boundaries have been assigned with setAxisScale() a
 * scale division is calculated ( QwtScaleEngine::divideScale() )
 * for this interval and assigned to the scale widget.
 *
 * 若已通过 setAxisScale() 方法指定了刻度边界，则会针对该区间计算刻度划分（调用 QwtScaleEngine::divideScale() 方法），
 * 并将其分配给刻度部件。
 *
 * When the scale has been set explicitly by setAxisScaleDiv() the
 * locally stored scale division gets assigned to the scale widget.
 *
 * 若已通过 setAxisScaleDiv() 方法显式设置了刻度，则会将本地存储的刻度划分直接分配给刻度部件。
 *
 * The scale widget indicates modifications by emitting a
 * QwtScaleWidget::scaleDivChanged() signal.
 *
 * 刻度部件会通过发送 QwtScaleWidget::scaleDivChanged() 信号来通知刻度的修改。
 *
 * updateAxes() is usually called by replot().
 *
 * updateAxes() 方法通常由 replot() 方法调用。
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
 * @brief 把plot item的范围更新为坐标轴的范围，这个函数一般是用于坐标轴变动的时候，强制让所有绘图范围进行更新
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
 * @brief yleft轴请求改变坐标轴范围更新
 * @param min
 * @param max
 */
void QwtPlot::yLeftRequestScaleRangeUpdate(double min, double max)
{

    setAxisScale(QwtAxis::YLeft, min, max);
    replotAll();
}

/**
 * @brief yright轴请求改变坐标轴范围更新
 * @param min
 * @param max
 */
void QwtPlot::yRightRequestScaleRangeUpdate(double min, double max)
{
    setAxisScale(QwtAxis::YRight, min, max);
    replotAll();
}

/**
 * @brief xbottom轴请求改变坐标轴范围更新
 * @param min
 * @param max
 */
void QwtPlot::xBottomRequestScaleRangeUpdate(double min, double max)
{
    setAxisScale(QwtAxis::XBottom, min, max);
    replotAll();
}

/**
 * @brief xtop轴请求改变坐标轴范围更新
 * @param min
 * @param max
 */
void QwtPlot::xTopRequestScaleRangeUpdate(double min, double max)
{
    setAxisScale(QwtAxis::XTop, min, max);
    replotAll();
}
