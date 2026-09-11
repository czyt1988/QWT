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

#ifndef QWT_PLOT_H
#define QWT_PLOT_H

#include "qwt_global.h"
#include "qwt_axis_id.h"
#include "qwt_plot_dict.h"
#include "qwt_scale_widget.h"

#include <qframe.h>

class QwtPlotLayout;
class QwtAbstractLegend;
class QwtScaleWidget;
class QwtScaleEngine;
class QwtScaleDiv;
class QwtScaleMap;
class QwtScaleDraw;
class QwtTextLabel;
class QwtInterval;
class QwtText;
class QwtPlotScaleEventDispatcher;
class QwtColorCycle;

template< typename T >
class QList;

// 6.1 compatibility definitions
#define QWT_AXIS_COMPAT 1

/**
 * @brief A 2-D plotting widget
 * @details QwtPlot is a widget for plotting two-dimensional graphs.
 *          An unlimited number of plot items can be displayed on
 *          its canvas. Plot items might be curves (QwtPlotCurve), markers
 *          (QwtPlotMarker), the grid (QwtPlotGrid), or anything else derived
 *          from QwtPlotItem.
 *          A plot can have up to four axes, with each plot item attached to an x- and
 *          a y axis. The scales at the axes can be explicitly set (QwtScaleDiv), or
 *          are calculated from the plot items, using algorithms (QwtScaleEngine) which
 *          can be configured separately for each axis.
 *
 *          The simpleplot example is a good starting point to see how to set up a
 *          plot widget.
 *
 * @image html plot.png
 *
 * @par Example
 *  The following example shows (schematically) the most simple
 *  way to use QwtPlot. By default, only the left and bottom axes are
 *  visible and their scales are computed automatically.
 *  @code
 * #include <qwt_plot.h>
 * #include <qwt_plot_curve.h>
 *
 *      QwtPlot *myPlot = new QwtPlot( "Two Curves", parent );
 *
 *      // add curves
 *      QwtPlotCurve *curve1 = new QwtPlotCurve( "Curve 1" );
 *      QwtPlotCurve *curve2 = new QwtPlotCurve( "Curve 2" );
 *
 *      // connect or copy the data to the curves
 *      curve1->setData( ... );
 *      curve2->setData( ... );
 *
 *      curve1->attach( myPlot );
 *      curve2->attach( myPlot );
 *
 *      // finally, refresh the plot
 *      myPlot->replot();
 *  @endcode
 */

class QWT_EXPORT QwtPlot : public QFrame, public QwtPlotDict
{
    Q_OBJECT

    Q_PROPERTY(QBrush canvasBackground READ canvasBackground WRITE setCanvasBackground)

    Q_PROPERTY(bool autoReplot READ autoReplot WRITE setAutoReplot)

    QWT_DECLARE_PRIVATE(QwtPlot)

    friend class QwtFigure;

public:
    /**
     * @brief Position of the legend, relative to the canvas
     * @sa insertLegend()
     */
    enum LegendPosition
    {
        ///< The legend will be left from the QwtAxis::YLeft axis
        LeftLegend,

        ///< The legend will be right from the QwtAxis::YRight axis
        RightLegend,

        ///< The legend will be below the footer
        BottomLegend,

        ///< The legend will be above the title
        TopLegend
    };

    /**
     * @brief Tick direction enum
     * @details Controls whether ticks are drawn inside or outside the canvas.
     *          When ticks are inside, they are drawn from the canvas edge toward
     *          the interior, while the backbone and labels remain outside.
     */
    enum TickDirection
    {
        ///< Ticks are drawn outside the canvas (default behavior)
        TickOutside = 0,

        ///< Ticks are drawn inside the canvas
        TickInside = 1
    };

    // Constructor
    explicit QwtPlot(QWidget* = nullptr);
    // Constructor with title
    explicit QwtPlot(const QwtText& title, QWidget* = nullptr);

    // Destructor
    ~QwtPlot() override;

    // Set auto replot
    void setAutoReplot(bool = true);
    // Check if auto replot is enabled
    bool autoReplot() const;

    // plot id
    // Get plot id
    QString plotId() const;
    // Set plot id
    void setPlotId(const QString& id);
    // Layout

    // Set plot layout
    void setPlotLayout(QwtPlotLayout*);

    // Get plot layout
    QwtPlotLayout* plotLayout();
    // Get plot layout (const)
    const QwtPlotLayout* plotLayout() const;

    // Title

    // Set title from string
    void setTitle(const QString&);
    // Set title from QwtText
    void setTitle(const QwtText&);
    // Get title
    QwtText title() const;

    // Get title label
    QwtTextLabel* titleLabel();
    // Get title label (const)
    const QwtTextLabel* titleLabel() const;

    // Footer

    // Set footer from string
    void setFooter(const QString&);
    // Set footer from QwtText
    void setFooter(const QwtText&);
    // Get footer
    QwtText footer() const;

    // Get footer label
    QwtTextLabel* footerLabel();
    // Get footer label (const)
    const QwtTextLabel* footerLabel() const;

    // Canvas

    // Set canvas
    void setCanvas(QWidget*);

    // Get canvas
    QWidget* canvas();
    // Get canvas (const)
    const QWidget* canvas() const;

    // Set canvas background
    void setCanvasBackground(const QBrush&);
    // Get canvas background
    QBrush canvasBackground() const;

    // Color cycle

    // Set color cycle for automatic item coloring
    void setColorCycle(const QwtColorCycle&);
    // Get the current color cycle
    QwtColorCycle colorCycle() const;
    // Get the next auto-assigned color for a plot item type
    QColor nextColorForItem(int rtti);

    // Get canvas map for axis
    virtual QwtScaleMap canvasMap(QwtAxisId) const;

    // Inverse transform from pixel position to value
    double invTransform(QwtAxisId, double pos) const;
    // Transform from value to pixel position
    double transform(QwtAxisId, double value) const;

    // Axes

    // Check if axis is valid
    bool isAxisValid(QwtAxisId) const;

    // Set axis visible
    void setAxisVisible(QwtAxisId, bool on = true);
    // Check if axis is visible
    bool isAxisVisible(QwtAxisId) const;

    // Axes data

    // Get axis scale engine
    QwtScaleEngine* axisScaleEngine(QwtAxisId);
    // Get axis scale engine (const)
    const QwtScaleEngine* axisScaleEngine(QwtAxisId) const;
    // Set axis scale engine
    void setAxisScaleEngine(QwtAxisId, QwtScaleEngine*);

    // Set axis auto scale
    void setAxisAutoScale(QwtAxisId, bool on = true);
    // Check if axis auto scale is enabled
    bool axisAutoScale(QwtAxisId) const;

    // Set axis font
    void setAxisFont(QwtAxisId, const QFont&);
    // Get axis font
    QFont axisFont(QwtAxisId) const;

    // Set axis scale
    void setAxisScale(QwtAxisId, double min, double max, double stepSize = 0);
    // Set axis scale division
    void setAxisScaleDiv(QwtAxisId, const QwtScaleDiv&);
    // Set axis scale draw
    void setAxisScaleDraw(QwtAxisId, QwtScaleDraw*);

    // Get axis step size
    double axisStepSize(QwtAxisId) const;
    // Get axis interval
    QwtInterval axisInterval(QwtAxisId) const;
    // Get axis scale division
    const QwtScaleDiv& axisScaleDiv(QwtAxisId) const;

    // Get axis scale draw (const)
    const QwtScaleDraw* axisScaleDraw(QwtAxisId) const;
    // Get axis scale draw
    QwtScaleDraw* axisScaleDraw(QwtAxisId);

    // Get axis widget (const)
    const QwtScaleWidget* axisWidget(QwtAxisId) const;
    // Get axis widget
    QwtScaleWidget* axisWidget(QwtAxisId);

    // Return the currently visible X/Y axis,XBottom/YLeft first
    // Get visible X axis id
    QwtAxisId visibleXAxisId() const;
    // Get visible Y axis id
    QwtAxisId visibleYAxisId() const;

    // Set axis label alignment
    void setAxisLabelAlignment(QwtAxisId, Qt::Alignment);
    // Set axis label rotation
    void setAxisLabelRotation(QwtAxisId, double rotation);

    // Set axis title from string
    void setAxisTitle(QwtAxisId, const QString&);
    // Set axis title from QwtText
    void setAxisTitle(QwtAxisId, const QwtText&);
    // Get axis title
    QwtText axisTitle(QwtAxisId) const;

    // Set axis title position along the backbone
    void setAxisTitlePosition(QwtAxisId, QwtScaleWidget::TitlePosition);
    // Get axis title position along the backbone
    QwtScaleWidget::TitlePosition axisTitlePosition(QwtAxisId) const;
    // Set axis title placement (inside the scale widget or outside as caption below/above it)
    void setAxisTitlePlacement(QwtAxisId, QwtScaleWidget::TitlePlacement);
    // Get axis title placement
    QwtScaleWidget::TitlePlacement axisTitlePlacement(QwtAxisId) const;
    // Set axis title text alignment (horizontal alignment within the title box)
    void setAxisTitleAlignment(QwtAxisId, Qt::Alignment);
    // Get axis title text alignment
    Qt::Alignment axisTitleAlignment(QwtAxisId) const;

    // Set axis max minor ticks
    void setAxisMaxMinor(QwtAxisId, int maxMinor);
    // Get axis max minor ticks
    int axisMaxMinor(QwtAxisId) const;

    // Set axis max major ticks
    void setAxisMaxMajor(QwtAxisId, int maxMajor);
    // Get axis max major ticks
    int axisMaxMajor(QwtAxisId) const;

    // Tick Direction - NEW

    // Set tick direction for an axis
    void setAxisTickDirection(QwtAxisId axisId, TickDirection direction);

    // Get tick direction for an axis
    TickDirection axisTickDirection(QwtAxisId axisId) const;

    // Legend

    // Insert legend
    void insertLegend(QwtAbstractLegend*, LegendPosition = QwtPlot::RightLegend, double ratio = -1.0);

    // Get legend
    QwtAbstractLegend* legend();
    // Get legend (const)
    const QwtAbstractLegend* legend() const;

    // Update legend
    void updateLegend();
    // Update legend for specific item
    void updateLegend(const QwtPlotItem*);

    // Misc

    // Get size hint
    virtual QSize sizeHint() const override;
    // Get minimum size hint
    virtual QSize minimumSizeHint() const override;

    // Update layout
    virtual void updateLayout();
    // Draw canvas
    virtual void drawCanvas(QPainter*);

    // Update axes
    void updateAxes();
    // Update canvas margins
    void updateCanvasMargins();

    // Get canvas margins hint
    virtual void getCanvasMarginsHint(const QwtScaleMap maps[],
                                      const QRectF& canvasRect,
                                      double& left,
                                      double& top,
                                      double& right,
                                      double& bottom) const;

    // Handle events
    virtual bool event(QEvent*) override;
    // Event filter
    virtual bool eventFilter(QObject*, QEvent*) override;

    // Draw items
    virtual void drawItems(QPainter*, const QRectF&, const QwtScaleMap maps[ QwtAxis::AxisPositions ]) const;

    // Convert item to info
    virtual QVariant itemToInfo(QwtPlotItem*) const;
    // Convert info to item
    virtual QwtPlotItem* infoToItem(const QVariant&) const;

    // add since v7.1.0

    // Create a parasite plot based on this axis as host
    QwtPlot* createParasitePlot(QwtAxis::Position enableAxis);

    // Set which axes of the host the parasite shares, only valid for parasite plots
    void setParasiteShareAxis(QwtAxisId axisId, bool isShare = true);

    // Get which axes of the host the parasite shares, only valid for parasite plots
    bool isParasiteShareAxis(QwtAxisId axisId) const;
    // Remove a parasite plot from this host plot
    void removeParasitePlot(QwtPlot* parasite);

    // Get all parasite plots associated with this host plot
    QList< QwtPlot* > parasitePlots() const;
    // Return all plots, including host plot, ascending order by default
    QList< QwtPlot* > plotList(bool descending = false) const;
    // Get the nth parasite plot
    QwtPlot* parasitePlotAt(int index) const;

    // Get parasite plot index (level), higher level means closer to plot boundary
    int parasitePlotIndex(QwtPlot* parasite) const;

    // Get the host plot for this parasite plot
    QwtPlot* hostPlot() const;

    // Check if this plot is a parasite plot
    bool isParasitePlot() const;
    // Check if this plot is the top parasite plot
    bool isTopParasitePlot() const;

    // Check if this plot is a host plot
    bool isHostPlot() const;

    // Set background color
    void setBackgroundColor(const QColor& c);
    // Get background color
    QColor backgroundColor() const;

    // Synchronize the axis ranges of the corresponding plot
    void syncAxis(QwtAxisId axis, const QwtPlot* plot);
    // Rescale the axes to encompass the full range of all data items
    void rescaleAxes(bool onlyVisibleItems = true,
                     double marginPercent  = 0.05,
                     QwtAxisId xAxis       = QwtPlot::xBottom,
                     QwtAxisId yAxis       = QwtPlot::yLeft);

    // Set the specified axis to logarithmic scale
    void setAxisToLogScale(QwtAxisId axisId);

    // Set the specified axis to date-time scale
    void setAxisToDateTime(QwtAxisId axisId, Qt::TimeSpec timeSpec = Qt::LocalTime);

    // Restore the specified axis to linear scale
    void setAxisToLinearScale(QwtAxisId axisId);

    // Align parasite plot to host plot
    void alignToHost();

    // Get the number of parasite plots
    int parasitePlotCount() const;

    // Update axis edge margin
    void updateAxisEdgeMargin(QwtAxisId axisId);
    // Update all axis edge margins
    void updateAllAxisEdgeMargin();
    // Align borderDist across host and all parasite layers for the given axis
    void alignAxisBorderDist(QwtAxisId axisId);
    // Align borderDist for all axis positions
    void alignAllAxisBorderDist();
    // Update items to fit scale division range
    void updateItemsToScaleDiv();
    // Enable/disable scale built-in actions
    void setEnableScaleBuildinActions(bool on);
    // Check if scale built-in actions are enabled
    bool isEnableScaleBuildinActions() const;
    // Set scale event dispatcher
    void setupScaleEventDispatcher(QwtPlotScaleEventDispatcher* dispatcher);
    // Save current auto replot state
    void saveAutoReplotState();
    // Restore auto replot state
    void restoreAutoReplotState();
    // Pan axis by pixels
    void panAxis(QwtAxisId axisId, int deltaPixels);
    // Pan canvas
    void panCanvas(const QPoint& offset);
    // Zoom axis
    void zoomAxis(QwtAxisId axisId, double factor, const QPoint& centerPosPixels);
#if QWT_AXIS_COMPAT
    enum Axis
    {
        yLeft   = QwtAxis::YLeft,
        yRight  = QwtAxis::YRight,
        xBottom = QwtAxis::XBottom,
        xTop    = QwtAxis::XTop,

        axisCnt = QwtAxis::AxisPositions
    };

    void enableAxis(int axisId, bool on = true)
    {
        setAxisVisible(axisId, on);
    }

    bool axisEnabled(int axisId) const
    {
        return isAxisVisible(axisId);
    }
#endif

Q_SIGNALS:
    /**
     * @brief A signal indicating that an item has been attached/detached
     * @param plotItem Plot item
     * @param on Attached/Detached
     */
    void itemAttached(QwtPlotItem* plotItem, bool on);

    /**
     * @brief A signal with the attributes how to update
     * the legend entries for a plot item.
     * @param itemInfo Info about a plot item, built from itemToInfo()
     * @param data Attributes of the entries (usually <= 1) for
     *            the plot item.
     * @sa itemToInfo(), infoToItem(), QwtAbstractLegend::updateLegend()
     */
    void legendDataChanged(const QVariant& itemInfo, const QList< QwtLegendData >& data);

    /**
     * @brief Identify the relationship between the parasitic plot and its host plot
     * @param parasitePlot Parasite plot
     * @param on When a parasitic plot is added, on = true. When the parasitic plot is removed, on = false.
     * @note This signal is emitted only by the host plot.
     */
    void parasitePlotAttached(QwtPlot* parasitePlot, bool on);
public Q_SLOTS:
    // Replot the plot
    virtual void replot();
    // Auto refresh
    void autoRefresh();
    // Replot all plots, including parasite or host plots
    virtual void replotAll();
    // Auto refresh all plots
    void autoRefreshAll();

protected:
    // Handle resize event
    virtual void resizeEvent(QResizeEvent*) override;
    // Add a parasite plot to this host plot
    void addParasitePlot(QwtPlot* parasite);
    // Initialize parasite axes basic attributes
    void initParasiteAxes(QwtPlot* parasitePlot) const;
    // Implementation of updateLayout
    void doLayout();

    // NEW: Draw inside ticks for axes with TickInside direction
    void drawInsideTicks(QPainter*, const QRectF& canvasRect, const QwtScaleMap maps[ QwtAxis::AxisPositions ]) const;

private Q_SLOTS:
    // Update legend items
    void updateLegendItems(const QVariant& itemInfo, const QList< QwtLegendData >& legendData);
    // Handle yLeft scale range update request
    void yLeftRequestScaleRangeUpdate(double min, double max);
    // Handle yRight scale range update request
    void yRightRequestScaleRangeUpdate(double min, double max);
    // Handle xBottom scale range update request
    void xBottomRequestScaleRangeUpdate(double min, double max);
    // Handle xTop scale range update request
    void xTopRequestScaleRangeUpdate(double min, double max);

private:
    friend class QwtPlotItem;
    // Attach/detach item
    void attachItem(QwtPlotItem*, bool);

    // Initialize axes data
    void initAxesData();
    // Delete axes data
    void deleteAxesData();

    // Initialize plot
    void initPlot(const QwtText& title);
    // Top parasite plot triggers host to update all axis margins
    void topParasiteTriggerHostUpdateAxisMargins();

    // Helper: Draw a single inside tick
    void drawSingleInsideTick(QPainter* painter, double tickPixelPos, double tickLength, double backbonePos, int axisPos) const;

    class ScaleData;
    ScaleData* m_scaleData;
};

#endif
