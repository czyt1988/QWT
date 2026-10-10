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
 *   8. Amalgamated build: single QwtPlot.h / QwtPlot.cpp pair in
 *src-amalgamate.
 *****************************************************************************/

#ifndef QWT_PLOT_CURVE_H
#define QWT_PLOT_CURVE_H

#include "qwt_global.h"
#include "qwt_plot_seriesitem.h"

#include <qstring.h>

class QwtScaleMap;
class QwtSymbol;
class QwtCurveFitter;
template< typename T >
class QwtSeriesData;
class QwtText;
class QPainter;
class QPolygonF;
class QPen;

/**
 * @brief A plot item, that represents a series of points
 * @details A curve is the representation of a series of points in the x-y
 * plane. It supports different display styles, interpolation ( f.e. spline )
 *          and symbols.
 *
 * @par Usage
 * <dl><dt>a) Assign curve properties</dt>
 * <dd>When a curve is created, it is configured to draw black solid lines
 * with in QwtPlotCurve::Lines style and no symbols.
 * You can change this by calling
 * setPen(), setStyle() and setSymbol().</dd>
 * <dt>b) Connect/Assign data.</dt>
 * <dd>QwtPlotCurve gets its points using a QwtSeriesData object offering
 * a bridge to the real storage of the points ( like QAbstractItemModel ).
 * There are several convenience classes derived from QwtSeriesData, that also
 * store the points inside ( like QStandardItemModel ). QwtPlotCurve also offers
 * a couple of variations of setSamples(), that build QwtSeriesData objects from
 * arrays internally.</dd>
 * <dt>c) Attach the curve to a plot</dt>
 * <dd>See QwtPlotItem::attach()
 * </dd></dl>
 *
 * @par Example:
 * see examples/bode
 *
 * @sa QwtPointSeriesData, QwtSymbol, QwtScaleMap
 */
class QWT_EXPORT QwtPlotCurve : public QwtPlotSeriesItem, public QwtSeriesStore< QPointF >
{
public:
    /**
     * @brief Curve styles
     * @sa setStyle(), style()
     */
    enum CurveStyle
    {
        /**
         * Don't draw a curve. Note: This doesn't affect the symbols.
         */
        NoCurve = -1,

        /**
         * Connect the points with straight lines. The lines might
         * be interpolated depending on the 'Fitted' attribute. Curve
         * fitting can be configured using setCurveFitter().
         */
        Lines,

        /**
         * Draw vertical or horizontal sticks ( depending on the
         * orientation() ) from a baseline which is defined by setBaseline().
         */
        Sticks,

        /**
         * Connect the points with a step function. The step function
         * is drawn from the left to the right or vice versa,
         * depending on the QwtPlotCurve::Inverted attribute.
         */
        Steps,

        /**
         * Draw dots at the locations of the data points. Note:
         * This is different from a dotted line (see setPen()), and faster
         * as a curve in QwtPlotCurve::NoStyle style and a symbol
         * painting a point.
         */
        Dots,

        /**
         * Styles >= QwtPlotCurve::UserCurve are reserved for derived
         * classes of QwtPlotCurve that overload drawCurve() with
         * additional application specific curve types.
         */
        UserCurve = 100
    };

    /**
     * @brief Attribute for drawing the curve
     * @sa setCurveAttribute(), testCurveAttribute(), curveFitter()
     */
    enum CurveAttribute
    {
        /**
         * For QwtPlotCurve::Steps only.
         * Draws a step function from the right to the left.
         */
        Inverted = 0x01,

        /**
         * Only in combination with QwtPlotCurve::Lines
         * A QwtCurveFitter tries to
         * interpolate/smooth the curve, before it is painted.
         *
         * @note Curve fitting requires temporary memory
         * for calculating coefficients and additional points.
         * If painting in QwtPlotCurve::Fitted mode is slow it might be better
         * to fit the points, before they are passed to QwtPlotCurve.
         */
        Fitted = 0x02
    };

    Q_DECLARE_FLAGS(CurveAttributes, CurveAttribute)

    /**
     * @brief Attributes how to represent the curve on the legend
     * @sa setLegendAttribute(), testLegendAttribute(),
     *     QwtPlotItem::legendData(), legendIcon()
     */

    enum LegendAttribute
    {
        /**
         * QwtPlotCurve tries to find a color representing the curve
         * and paints a rectangle with it.
         */
        LegendNoAttribute = 0x00,

        /**
         * If the style() is not QwtPlotCurve::NoCurve a line
         * is painted with the curve pen().
         */
        LegendShowLine = 0x01,

        /**
         * If the curve has a valid symbol it is painted.
         */
        LegendShowSymbol = 0x02,

        /**
         * If the curve has a brush a rectangle filled with the
         * curve brush() is painted.
         */
        LegendShowBrush = 0x04
    };

    Q_DECLARE_FLAGS(LegendAttributes, LegendAttribute)

    /**
     * @brief Attributes to modify the drawing algorithm
     * @details The default setting enables ClipPolygons | FilterPointsLTTB
     * @sa setPaintAttribute(), testPaintAttribute()
     */
    enum PaintAttribute
    {
        /**
         * Clip polygons before painting them. In situations, where points
         * are far outside the visible area (f.e when zooming deep) this
         * might be a substantial improvement for the painting performance
         */
        ClipPolygons = 0x01,

        /**
         * Tries to reduce the data that has to be painted, by sorting out
         * duplicates, or paintings outside the visible area. Might have a
         * notable impact on curves with many close points.
         * Only a couple of very basic filtering algorithms are implemented.
         */
        FilterPoints = 0x02,

        /**
         * Minimize memory usage that is temporarily needed for the
         * translated points, before they get painted.
         * This might slow down the performance of painting
         */
        MinimizeMemory = 0x04,

        /**
         * Render the points to a temporary image and paint the image.
         * This is a very special optimization for Dots style, when
         * having a huge amount of points.
         * With a reasonable number of points QPainter::drawPoints()
         * will be faster.
         */
        ImageBuffer = 0x08,

        /**
         * More aggressive point filtering trying to filter out
         * intermediate points, accepting minor visual differences.
         *
         * Has only an effect, when drawing the curve to a paint device
         * in integer coordinates ( f.e. all widgets on screen ) using the fact,
         * that consecutive points are often mapped to the same x or y coordinate.
         * Each chunk of samples mapped to the same coordinate can be reduced to
         * 4 points ( first, min, max last ).
         *
         * In the worst case the polygon to be rendered will be 4 times the width
         * of the plot canvas.
         *
         * The algorithm is very fast and effective for huge datasets, and can be
         * used inside a replot cycle.
         *
         * @note Implemented for QwtPlotCurve::Lines only
         * @note As this algo replaces many small lines by a long one
         *      a nasty bug of the raster paint engine ( Qt 4.8, Qt 5.1 - 5.3 )
         *      becomes more dominant. For these versions the bug can be
         *      worked around by enabling the QwtPainter::polylineSplitting() mode.
         */
        FilterPointsAggressive = 0x10,

        /**
         * Pixel-column based downsampling for extreme performance.
         *
         * Allocates a bin array indexed by pixel column and stores
         * first/min/max/last Y per column. Each column produces at most 4 points.
         * Combined with binary-search visible range for monotonic X data.
         *
         * Columns holding more than one point are emitted as a cluster in the
         * layout of the adaptive sampling of QCustomPlot: min and max at fixed
         * sub pixel positions ( +0.25 / +0.75 ), so that the vertical stroke of
         * a column stays inside its pixel, plus the real first/last point only
         * where the neighbor column is empty or holds a single point.
         *
         * Output size: ~4 * canvasWidth points regardless of input size.
         * This is the fastest algorithm for datasets exceeding 100k points.
         *
         * @note Implemented for QwtPlotCurve::Lines only
         * @note Overrides FilterPointsAggressive when both are set
         */
        FilterPointsPixel = 0x20,

        /**
         * MinMax bucket downsampling (simplified LTTB variant).
         *
         * Divides the visible data range into N equal-count buckets
         * and keeps the min-Y and max-Y point from each bucket.
         * N is auto-calculated as 2 * canvasWidth.
         *
         * Output size: ~2 * N points. O(n) time complexity.
         * Preserves visual shape better than FilterPointsPixel for
         * data with uneven X distribution.
         *
         * @note Implemented for QwtPlotCurve::Lines only
         * @note Overrides FilterPointsAggressive when both are set
         */
        FilterPointsLTTB = 0x40,
    };

    Q_DECLARE_FLAGS(PaintAttributes, PaintAttribute)

    /**
     * @brief Algorithm for smoothing the rendered polyline
     * @details The smoothing is applied to the polyline that has been
     *          downsampled and translated to paint coordinates. It targets
     *          the pixel level noise of curves with many samples
     *          ( f.e. spectra with random jitter ), while the sample data
     *          itself is not modified.
     *
     *          In contrast to the Fitted attribute, that interpolates
     *          through all points, the smoothing algorithms are low pass
     *          filters. Significant features ( peaks, dips ) are restored
     *          to their original position and height afterwards, so that
     *          the smoothing does not distort the curve, see
     *          setSmoothPreserveThreshold().
     *
     *          @note Implemented for QwtPlotCurve::Lines only
     * @sa setSmoothAlgorithm(), setSmoothWindow(), setSmoothPolynomialOrder(),
     *     setSmoothPreserveThreshold()
     */
    enum SmoothAlgorithm
    {
        //! No smoothing ( default )
        NoSmoothing = 0,

        //! Gaussian weighted moving window: strong noise suppression
        GaussianSmoothing,

        /*!
           Savitzky-Golay local polynomial regression: keeps the height
           and shape of narrow peaks better than GaussianSmoothing
         */
        SavitzkyGolaySmoothing
    };

    // Constructor
    explicit QwtPlotCurve(const QString& title = QString());

    // Constructor with QwtText title
    explicit QwtPlotCurve(const QwtText& title);

    // Destructor
    ~QwtPlotCurve() override;

    // Get the runtime type information
    virtual int rtti() const override;

    // Attach the curve to a plot (applies color cycle if pen not user-set)
    void attach(QwtPlot* plot) override;

    // Set paint attribute
    void setPaintAttribute(PaintAttribute, bool on = true);

    // Test paint attribute
    bool testPaintAttribute(PaintAttribute) const;

    // Set legend attribute
    void setLegendAttribute(LegendAttribute, bool on = true);

    // Test legend attribute
    bool testLegendAttribute(LegendAttribute) const;

    // Set legend attributes
    void setLegendAttributes(LegendAttributes);

    // Get legend attributes
    LegendAttributes legendAttributes() const;

    // Set raw samples from double arrays
    void setRawSamples(const double* xData, const double* yData, int size);

    // Set raw samples from float arrays
    void setRawSamples(const float* xData, const float* yData, int size);

    // Set raw samples from double array (y-axis only)
    void setRawSamples(const double* yData, int size);

    // Set raw samples from float array (y-axis only)
    void setRawSamples(const float* yData, int size);

    // Set samples from double arrays
    void setSamples(const double* xData, const double* yData, int size);

    // Set samples from float arrays
    void setSamples(const float* xData, const float* yData, int size);

    // Set samples from double array (y-axis only)
    void setSamples(const double* yData, int size);

    // Set samples from float array (y-axis only)
    void setSamples(const float* yData, int size);

    // Set samples from QVector<double> (y-axis only)
    void setSamples(const QVector< double >& yData);

    // Set samples from QVector<float> (y-axis only)
    void setSamples(const QVector< float >& yData);

    // Set samples from QVector<double> arrays
    void setSamples(const QVector< double >& xData, const QVector< double >& yData);

    // Set samples from QVector<float> arrays
    void setSamples(const QVector< float >& xData, const QVector< float >& yData);

    // Set samples from rvalue QVector<double> arrays
    void setSamples(QVector< double >&& xData, QVector< double >&& yData);

    // Set samples from rvalue QVector<float> arrays
    void setSamples(QVector< float >&& xData, QVector< float >&& yData);

    // Set samples from rvalue QVector<QPointF>
    void setSamples(QVector< QPointF >&&);

    // Set samples from QVector<QPointF>
    void setSamples(const QVector< QPointF >&);

    // Set samples from QwtSeriesData
    void setSamples(QwtSeriesData< QPointF >*);

    // Find the closest point to a position
    virtual int closestPoint(const QPointF& pos, double* dist = nullptr) const;

    // Get minimum x value
    double minXValue() const;

    // Get maximum x value
    double maxXValue() const;

    // Get minimum y value
    double minYValue() const;

    // Get maximum y value
    double maxYValue() const;

    // Set curve attribute
    void setCurveAttribute(CurveAttribute, bool on = true);

    // Test curve attribute
    bool testCurveAttribute(CurveAttribute) const;

    // Set pen
    void setPen(const QColor&, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine);

    // Set pen
    void setPen(const QPen&);

    // Get pen
    const QPen& pen() const;

    // Set brush
    void setBrush(const QBrush&);

    // Get brush
    const QBrush& brush() const;

    // Set baseline
    void setBaseline(double);

    // Get baseline
    double baseline() const;

    // Set curve style
    void setStyle(CurveStyle style);

    // Get curve style
    CurveStyle style() const;

    // Set symbol
    void setSymbol(QwtSymbol*);

    // Get symbol
    const QwtSymbol* symbol() const;

    // Set curve fitter
    void setCurveFitter(QwtCurveFitter*);

    // Get curve fitter
    QwtCurveFitter* curveFitter() const;

    // Set the render smoothing algorithm
    void setSmoothAlgorithm(SmoothAlgorithm);

    // Get the render smoothing algorithm
    SmoothAlgorithm smoothAlgorithm() const;

    // Set the smoothing window ( number of polyline points, odd values >= 3 )
    void setSmoothWindow(int numPoints);

    // Get the smoothing window
    int smoothWindow() const;

    // Set the polynomial order for SavitzkyGolaySmoothing
    void setSmoothPolynomialOrder(int order);

    // Get the polynomial order for SavitzkyGolaySmoothing
    int smoothPolynomialOrder() const;

    // Set the feature preservation threshold of the render smoothing
    void setSmoothPreserveThreshold(double threshold);

    // Get the feature preservation threshold of the render smoothing
    double smoothPreserveThreshold() const;

    // Draw the series
    virtual void drawSeries(QPainter*,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QRectF& canvasRect,
                            int from,
                            int to) const override;

    // Get the legend icon
    virtual QwtGraphic legendIcon(int index, const QSizeF&) const override;

protected:
    //! Initialize the curve
    void init();

    //! Draw the curve
    virtual void drawCurve(QPainter*,
                           int style,
                           const QwtScaleMap& xMap,
                           const QwtScaleMap& yMap,
                           const QRectF& canvasRect,
                           int from,
                           int to) const;

    //! Draw symbols
    virtual void drawSymbols(QPainter*,
                             const QwtSymbol&,
                             const QwtScaleMap& xMap,
                             const QwtScaleMap& yMap,
                             const QRectF& canvasRect,
                             int from,
                             int to) const;

    //! Draw lines
    virtual void
    drawLines(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect, int from, int to) const;

    //! Draw sticks
    virtual void
    drawSticks(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect, int from, int to) const;

    //! Draw dots
    virtual void
    drawDots(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect, int from, int to) const;

    //! Draw steps
    virtual void
    drawSteps(QPainter*, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect, int from, int to) const;

    //! Fill the curve
    virtual void fillCurve(QPainter*, const QwtScaleMap&, const QwtScaleMap&, const QRectF& canvasRect, QPolygonF&) const;

    //! Close the polyline
    void closePolyline(QPainter*, const QwtScaleMap&, const QwtScaleMap&, QPolygonF&) const;

private:
    QWT_DECLARE_PRIVATE(QwtPlotCurve)
};

//! boundingRect().left()
inline double QwtPlotCurve::minXValue() const
{
    return boundingRect().left();
}

//! boundingRect().right()
inline double QwtPlotCurve::maxXValue() const
{
    return boundingRect().right();
}

//! boundingRect().top()
inline double QwtPlotCurve::minYValue() const
{
    return boundingRect().top();
}

//! boundingRect().bottom()
inline double QwtPlotCurve::maxYValue() const
{
    return boundingRect().bottom();
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotCurve::PaintAttributes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotCurve::LegendAttributes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotCurve::CurveAttributes)

#endif
