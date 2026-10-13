/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *
 * Modified by ChenZongYan in 2024 <czy.t@163.com>
 *****************************************************************************/

#ifndef QWT_PLOT_BOXCHART_H
#define QWT_PLOT_BOXCHART_H

#include "qwt_global.h"
#include "qwt_plot_seriesitem.h"
#include "qwt_series_store.h"
#include "qwt_samples.h"

class QwtSymbol;
template< typename T >
class QwtSeriesData;

/**
 * @brief Plot item for box-and-whisker (boxplot) visualization
 * @details QwtPlotBoxChart displays statistical distributions using the boxplot format:
 *          - Box body showing Q1-Q3 interquartile range
 *          - Median line inside the box
 *          - Whiskers extending to data range or calculated bounds
 *          - Outlier symbols for points outside whiskers
 *
 *          Supports multiple box styles (Rectangle, Diamond, Notch),
 *          horizontal/vertical orientation, and extensive customization.
 *
 */
class QWT_EXPORT QwtPlotBoxChart : public QwtPlotSeriesItem, public QwtSeriesStore< QwtBoxSample >
{
public:
    /**
     * @brief Box body display style
     */
    enum BoxStyle
    {
        //! No box body, only whiskers and outliers
        NoBox,

        //! Traditional rectangular box (Q1-Q3)
        Rect,

        //! Diamond shape connecting extremes
        Diamond,

        //! Rectangle with notch indentation at median
        Notch
    };

    /**
     * @brief Whisker display style
     */
    enum WhiskerStyle
    {
        //! No whisker lines drawn
        NoWhiskers,

        //! Traditional T-bar whiskers with horizontal caps (default)
        StandardWhisker,

        //! Simple line from whiskerLower to whiskerUpper
        MinMaxLine
    };

    /**
     * @brief Paint attributes for performance optimization
     */
    enum PaintAttribute
    {
        //! Clip boxes before painting (performance for zoomed views)
        ClipBoxes = 0x01,

        //! Clip outlier symbols before painting
        ClipOutliers = 0x02,

        //! Use image buffer for rendering (optimization for many boxes)
        ImageBuffer = 0x04
    };

    Q_DECLARE_FLAGS(PaintAttributes, PaintAttribute)

    /**
     * @brief Constructor
     */
    explicit QwtPlotBoxChart(const QString& title = QString());

    /**
     * @brief Constructor with QwtText title
     */
    explicit QwtPlotBoxChart(const QwtText& title);

    /**
     * @brief Destructor
     */
    ~QwtPlotBoxChart() override;

    // Get runtime type information
    virtual int rtti() const override;

    // Attach the box chart to a plot (applies color cycle if pen not user-set)
    void attach(QwtPlot* plot) override;

    // Set paint attribute
    void setPaintAttribute(PaintAttribute, bool on = true);

    // Test paint attribute
    bool testPaintAttribute(PaintAttribute) const;

    // Set box samples
    void setSamples(const QVector< QwtBoxSample >&);
    void setSamples(QwtSeriesData< QwtBoxSample >*);

    // Set outlier samples (optional)
    void setOutliers(const QVector< QwtBoxOutlierSample >&);
    void setOutliers(QwtSeriesData< QwtBoxOutlierSample >*);

    // Get outlier data
    const QwtSeriesData< QwtBoxOutlierSample >* outlierData() const;

    // Set box style
    void setBoxStyle(BoxStyle);
    BoxStyle boxStyle() const;

    // Set whisker style
    void setWhiskerStyle(WhiskerStyle);
    WhiskerStyle whiskerStyle() const;

    // Set orientation (vertical: x-position, horizontal: y-position)
    void setOrientation(Qt::Orientation);
    Qt::Orientation orientation() const;

    // Set box width in scale coordinates
    void setBoxExtent(double extent);
    double boxExtent() const;

    // Set minimum box width in pixels
    void setMinBoxWidth(double pixels);
    double minBoxWidth() const;

    // Set maximum box width in pixels (negative = unlimited)
    void setMaxBoxWidth(double pixels);
    double maxBoxWidth() const;

    // Set pen for box outline and whiskers
    void setPen(const QColor&, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine);
    void setPen(const QPen&);
    const QPen& pen() const;

    // Set brush for box body fill
    void setBrush(const QBrush&);
    const QBrush& brush() const;

    // Set pen for median line
    void setMedianPen(const QPen&);
    QPen medianPen() const;

    // Set symbol for outliers
    void setOutlierSymbol(const QwtSymbol*);
    const QwtSymbol* outlierSymbol() const;

    // Set symbol for the mean marker (drawn at QwtBoxSample::mean)
    void setMeanSymbol(const QwtSymbol*);
    const QwtSymbol* meanSymbol() const;

    // Show/hide median line
    void setMedianVisible(bool);
    bool isMedianVisible() const;

    // Show/hide mean marker (samples without mean draw nothing)
    void setMeanVisible(bool);
    bool isMeanVisible() const;

    // Set outlier jitter width (for overlapping outliers)
    void setOutlierJitter(double jitterWidth);
    double outlierJitter() const;

    // Draw the series
    virtual void drawSeries(QPainter*,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QRectF& canvasRect,
                            int from,
                            int to) const override;

    // Get bounding rectangle
    virtual QRectF boundingRect() const override;

    // Get legend icon
    virtual QwtGraphic legendIcon(int index, const QSizeF&) const override;

protected:
    //! Initialize
    void init();

    //! Calculate scaled box width in pixels
    virtual double scaledBoxWidth(const QwtScaleMap& posMap, const QwtScaleMap& valueMap, const QRectF& canvasRect) const;

    //! Draw a single box
    virtual void
    drawBox(QPainter*, const QwtBoxSample&, Qt::Orientation, double boxWidth, double posPixel, const QwtScaleMap& valueMap) const;

    //! Draw whiskers for a single box
    virtual void drawWhiskers(QPainter*,
                              const QwtBoxSample&,
                              Qt::Orientation,
                              double boxWidth,
                              double posPixel,
                              const QwtScaleMap& valueMap) const;

    //! Draw median line for a single box
    virtual void drawMedian(QPainter*,
                            const QwtBoxSample&,
                            Qt::Orientation,
                            double boxWidth,
                            double posPixel,
                            const QwtScaleMap& valueMap) const;

    //! Draw outliers for boxes in range
    virtual void drawOutliers(QPainter*,
                              const QwtScaleMap& posMap,
                              const QwtScaleMap& valueMap,
                              const QRectF& canvasRect,
                              int from,
                              int to) const;

    //! Draw a single outlier symbol
    virtual void drawOutlierSymbol(QPainter*, double posPixel, double valuePixel, const QwtSymbol& symbol) const;

private:
    QWT_DECLARE_PRIVATE(QwtPlotBoxChart)

    QwtSeriesData< QwtBoxOutlierSample >* m_outlierData;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtPlotBoxChart::PaintAttributes)

#endif  // QWT_PLOT_BOXCHART_H