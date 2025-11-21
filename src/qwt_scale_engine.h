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

#ifndef QWT_SCALE_ENGINE_H
#define QWT_SCALE_ENGINE_H

#include "qwt_global.h"
#include "qwt_scale_div.h"

class QwtInterval;
class QwtTransform;

/*!
   \brief Arithmetic including a tolerance
 */
class QWT_EXPORT QwtScaleArithmetic
{
public:
    static double ceilEps(double value, double intervalSize);
    static double floorEps(double value, double intervalSize);

    static double divideEps(double intervalSize, double numSteps);

    static double divideInterval(double intervalSize, int numSteps, uint base);
};

/**
 * @brief Base class for scale engines./刻度引擎的基类
 *
 * A scale engine tries to find "reasonable" ranges and step sizes
 * for scales.
 *
 * 刻度引擎用于为坐标轴寻找“合理”的数值范围和步长。
 *
 * The layout of the scale can be varied with setAttribute().
 *
 * 可通过 setAttribute() 方法调整刻度的布局样式。
 *
 * Qwt offers implementations for logarithmic and linear scales.
 *
 * Qwt 库提供了对数刻度和线性刻度的具体实现。
 */
class QWT_EXPORT QwtScaleEngine
{
public:
    /*!
       Layout attributes/布局属性
       \sa setAttribute(), testAttribute(), reference(),
           lowerMargin(), upperMargin()
       \sa setAttribute()、testAttribute()、reference()、
           lowerMargin()、upperMargin()
     */
    enum Attribute
    {
        //! No attributes/无属性
        NoAttribute = 0x00,

        //! Build a scale which includes the reference() value./构建包含 reference() 参考值的刻度
        IncludeReference = 0x01,

        //! Build a scale which is symmetric to the reference() value./构建相对于 reference() 参考值对称的刻度
        Symmetric = 0x02,

        /*!
           The endpoints of the scale are supposed to be equal the
           outmost included values plus the specified margins
           (see setMargins()).
           If this attribute is *not* set, the endpoints of the scale will
           be integer multiples of the step size.

           刻度的端点值 = 最外侧包含值 + 指定边距（详见 setMargins() 方法）。
           若未设置此属性，刻度的端点值将为步长的整数倍。
         */
        Floating = 0x04,

        //! Turn the scale upside down./将刻度上下翻转（逆序显示）
        Inverted = 0x08
    };

    Q_DECLARE_FLAGS(Attributes, Attribute)

    explicit QwtScaleEngine(uint base = 10);
    virtual ~QwtScaleEngine();

    void setBase(uint base);
    uint base() const;

    void setAttribute(Attribute, bool on = true);
    bool testAttribute(Attribute) const;

    void setAttributes(Attributes);
    Attributes attributes() const;

    void setReference(double);
    double reference() const;

    void setMargins(double lower, double upper);
    double lowerMargin() const;
    double upperMargin() const;

    /*!
       Align and divide an interval

       \param maxNumSteps Max. number of steps
       \param x1 First limit of the interval (In/Out)
       \param x2 Second limit of the interval (In/Out)
       \param stepSize Step size (Return value)
     */
    virtual void autoScale(int maxNumSteps, double& x1, double& x2, double& stepSize) const = 0;

    /*!
       \brief Calculate a scale division

       \param x1 First interval limit
       \param x2 Second interval limit
       \param maxMajorSteps Maximum for the number of major steps
       \param maxMinorSteps Maximum number of minor steps
       \param stepSize Step size. If stepSize == 0.0, the scaleEngine
                   calculates one.

       \return Calculated scale division
     */
    virtual QwtScaleDiv
    divideScale(double x1, double x2, int maxMajorSteps, int maxMinorSteps, double stepSize = 0.0) const = 0;

    void setTransformation(QwtTransform*);
    QwtTransform* transformation() const;

protected:
    bool contains(const QwtInterval&, double value) const;
    QList< double > strip(const QList< double >&, const QwtInterval&) const;

    double divideInterval(double intervalSize, int numSteps) const;

    QwtInterval buildInterval(double value) const;

private:
    Q_DISABLE_COPY(QwtScaleEngine)

    class PrivateData;
    PrivateData* m_data;
};

/*!
   \brief A scale engine for linear scales

   The step size will fit into the pattern
   \f$\left\{ 1,2,5\right\} \cdot 10^{n}\f$, where n is an integer.
 */

class QWT_EXPORT QwtLinearScaleEngine : public QwtScaleEngine
{
public:
    explicit QwtLinearScaleEngine(uint base = 10);
    virtual ~QwtLinearScaleEngine();

    virtual void autoScale(int maxNumSteps, double& x1, double& x2, double& stepSize) const QWT_OVERRIDE;

    virtual QwtScaleDiv
    divideScale(double x1, double x2, int maxMajorSteps, int maxMinorSteps, double stepSize = 0.0) const QWT_OVERRIDE;

protected:
    QwtInterval align(const QwtInterval&, double stepSize) const;

    void buildTicks(const QwtInterval&, double stepSize, int maxMinorSteps, QList< double > ticks[ QwtScaleDiv::NTickTypes ]) const;

    QList< double > buildMajorTicks(const QwtInterval& interval, double stepSize) const;

    void buildMinorTicks(const QList< double >& majorTicks,
                         int maxMinorSteps,
                         double stepSize,
                         QList< double >& minorTicks,
                         QList< double >& mediumTicks) const;
};

/*!
   \brief A scale engine for logarithmic scales

   The step size is measured in *decades*
   and the major step size will be adjusted to fit the pattern
   \f$\left\{ 1,2,3,5\right\} \cdot 10^{n}\f$, where n is a natural number
   including zero.

   \warning the step size as well as the margins are measured in *decades*.
 */

class QWT_EXPORT QwtLogScaleEngine : public QwtScaleEngine
{
public:
    explicit QwtLogScaleEngine(uint base = 10);
    virtual ~QwtLogScaleEngine();

    virtual void autoScale(int maxNumSteps, double& x1, double& x2, double& stepSize) const QWT_OVERRIDE;

    virtual QwtScaleDiv
    divideScale(double x1, double x2, int maxMajorSteps, int maxMinorSteps, double stepSize = 0.0) const QWT_OVERRIDE;

protected:
    QwtInterval align(const QwtInterval&, double stepSize) const;

    void buildTicks(const QwtInterval&, double stepSize, int maxMinorSteps, QList< double > ticks[ QwtScaleDiv::NTickTypes ]) const;

    QList< double > buildMajorTicks(const QwtInterval& interval, double stepSize) const;

    void buildMinorTicks(const QList< double >& majorTicks,
                         int maxMinorSteps,
                         double stepSize,
                         QList< double >& minorTicks,
                         QList< double >& mediumTicks) const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtScaleEngine::Attributes)

#endif
