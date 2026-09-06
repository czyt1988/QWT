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

#ifndef QWT_SCALE_WIDGET_H
#define QWT_SCALE_WIDGET_H

#include "qwt_global.h"
#include "qwt_text.h"
#include "qwt_scale_draw.h"
#include "qwt_axis_id.h"
#include "qwt_scale_div.h"

#include <qwidget.h>
#include <qfont.h>
#include <qcolor.h>
#include <qstring.h>
// Qt
class QPainter;
class QEvent;
class QPaintEvent;
class QResizeEvent;
class QMouseEvent;
class QWheelEvent;
// Qwt
class QwtTransform;
class QwtColorMap;

/**
 * @brief A Widget which contains a scale
 * @details This Widget can be used to decorate composite widgets with a scale.
 *
 * Layout diagram:
 * │<----------------------------- plot yleft edge
 * │      │       │      │tick ┌       ┌-----------------------------------
 * │      │       │      │label│       │
 * │edge  │YLeft  │space │ 6  -│margin │
 * │margin│Title  │      │     │       │
 * │      │       │      │ 5  -│       │
 * │      │       │      │     │       │
 * │      │       │      │ 4  -│       │ plot cavans
 * │      │       │      │     │       │
 * │      │       │      │ 3  -│       │
 * │      │       │      │     │       │
 * │      │       │      │ 2  -│       │
 * │      │       │      │     │       │
 * │      │       │      │ 1  -│       │_________________________________
 */

class QWT_EXPORT QwtScaleWidget : public QWidget
{
    Q_OBJECT
    QWT_DECLARE_PRIVATE(QwtScaleWidget)
public:
    /// Layout flags of the title
    enum LayoutFlag
    {
        /// The title of vertical scales is painted from top to bottom. Otherwise it is painted from bottom to top.
        TitleInverted = 1
    };

    Q_DECLARE_FLAGS(LayoutFlags, LayoutFlag)

    /**
     * @brief Position of the axis title along the backbone
     * @details Controls where the title is placed along the axis backbone.
     * The title keeps its natural orientation (vertical for Y axes, horizontal
     * for X axes); only its position along the backbone changes.
     *
     * Visual mapping:
     * @code
     *   TitleAtStart   Y axis: bottom end   |   X axis: left end
     *   TitleCentered  Y axis: centered     |   X axis: centered   (default)
     *   TitleAtEnd     Y axis: top end      |   X axis: right end
     * @endcode
     *
     * @sa setTitlePosition(), titlePosition()
     */
    enum TitlePosition
    {
        /// Centered along the backbone (default, equivalent to the legacy behavior).
        TitleCentered = 0,
        /// At the backbone start end: bottom for vertical axes, left for horizontal axes.
        TitleAtStart,
        /// At the backbone end: top for vertical axes, right for horizontal axes.
        TitleAtEnd
    };

    /**
     * @brief Built-in actions
     */
    enum BuiltinActions
    {
        /// @brief No action
        ActionNone = 0x00,
        /// @brief Mouse wheel zoom (after clicking to activate the axis, scrolling the wheel zooms the current axis)
        ActionWheelZoom = 0x01,
        /// @brief Mouse click and drag (after clicking to activate the axis, dragging moves the axis left/right)
        ActionClickPan = 0x02,
        /// @brief All actions
        ActionAll = 0xFF
    };
    Q_DECLARE_FLAGS(BuiltinActionsFlags, BuiltinActions)
public:
    /// Constructor
    explicit QwtScaleWidget(QWidget* parent = nullptr);
    /// Constructor with alignment
    explicit QwtScaleWidget(QwtScaleDraw::Alignment, QWidget* parent = nullptr);
    /// Destructor
    ~QwtScaleWidget() override;

Q_SIGNALS:

    /**
     * @brief Signal emitted whenever the scale division changes
     */
    void scaleDivChanged();

    /**
     * @brief Request to change the axis scale division
     * @details Emitted when built-in actions (zoom/pan) need to alter the scale.
     *          Unlike normal QwtPlot updates, here the axis drives the change:
     *          QwtPlot receives this signal and adjusts item bounds accordingly.
     * @param min Minimum scale division requested
     * @param max Maximum scale division requested
     */
    void requestScaleRangeUpdate(double min, double max);

    /**
     * @brief Signal emitted when the axis selection state changes
     * @param selected True if the axis is selected, false otherwise
     */
    void selectionChanged(bool selected);

public:
    /// Set the title
    void setTitle(const QString& title);
    /// Set the title
    void setTitle(const QwtText& title);
    /// @return the title
    QwtText title() const;

    /// Set the title position along the backbone
    void setTitlePosition(TitlePosition);
    /// @return the title position along the backbone
    TitlePosition titlePosition() const;

    /// Set the title text alignment (horizontal alignment within the title box)
    void setTitleAlignment(Qt::Alignment);
    /// @return the title text alignment
    Qt::Alignment titleAlignment() const;

    /// Set a layout flag
    void setLayoutFlag(LayoutFlag, bool on);
    /// @return true if a layout flag is set
    bool testLayoutFlag(LayoutFlag) const;

    /// Set the border distances
    void setBorderDist(int dist1, int dist2);
    /// @return the start border distance
    int startBorderDist() const;
    /// @return the end border distance
    int endBorderDist() const;

    /// Get the border distance hint
    void getBorderDistHint(int& start, int& end) const;

    /// Get the minimum border distances
    void getMinBorderDist(int& start, int& end) const;
    /// Set the minimum border distances
    void setMinBorderDist(int start, int end);
    /// @return the start minimum border distance
    int startMinBorderDist() const;
    /// @return the end minimum border distance
    int endMinBorderDist() const;

    /// Set the margin
    void setMargin(int);
    /// @return the margin
    int margin() const;

    /// Set the spacing
    void setSpacing(int);
    /// @return the spacing
    int spacing() const;

    /// Set the edge margin (offset between axis and plot canvas)
    void setEdgeMargin(int offset);
    /// @return the edge margin
    int edgeMargin() const;

    /// Set the scale division
    void setScaleDiv(const QwtScaleDiv&);
    /// Set the transformation
    void setTransformation(QwtTransform*);

    /// Set the scale draw
    void setScaleDraw(QwtScaleDraw*);
    /// @return the scale draw (const version)
    const QwtScaleDraw* scaleDraw() const;
    /// @return the scale draw
    QwtScaleDraw* scaleDraw();

    /// Set the label alignment
    void setLabelAlignment(Qt::Alignment);
    /// Set the label rotation
    void setLabelRotation(double rotation);

    /// Enable/disable the color bar
    void setColorBarEnabled(bool);
    /// @return true if color bar is enabled
    bool isColorBarEnabled() const;

    /// Set the color bar width
    void setColorBarWidth(int);
    /// @return the color bar width
    int colorBarWidth() const;

    /// Set the color map
    void setColorMap(const QwtInterval&, QwtColorMap*);

    /// @return the color bar interval
    QwtInterval colorBarInterval() const;
    /// @return the color map
    const QwtColorMap* colorMap() const;

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;

    /// @return the height required for the title for a given width
    int titleHeightForWidth(int width) const;
    /// @return the dimension required for a given length
    int dimForLength(int length, const QFont& scaleFont) const;

    /// Draw the color bar
    void drawColorBar(QPainter*, const QRectF&) const;
    /// Draw the title
    void drawTitle(QPainter*, QwtScaleDraw::Alignment, const QRectF& rect) const;

    /// Set the alignment
    void setAlignment(QwtScaleDraw::Alignment);
    /// @return the alignment
    QwtScaleDraw::Alignment alignment() const;

    /// @return the rectangle for the color bar
    QRectF colorBarRect(const QRectF&) const;

    /// @return the scale rectangle (excluding color bar, margin, edge margin, border distances)
    QRect scaleRect() const;
    /// Set the text color (font color of the coordinate axis)
    void setTextColor(const QColor& c);
    /// @return the text color
    QColor textColor() const;

    /// Set the scale color (color of the coordinate axis)
    void setScaleColor(const QColor& c);
    /// @return the scale color
    QColor scaleColor() const;

    /// Layout the scale
    void layoutScale(bool update_geometry = true);

    /// @return the axis ID for this scale widget
    QwtAxisId axisID() const;
    /// @return true if this is an X axis
    bool isXAxis() const;
    /// @return true if this is a Y axis
    bool isYAxis() const;
    //===============================================
    // Built-in action methods
    //===============================================

    /// Enable/disable built-in actions
    void setBuildinActions(BuiltinActionsFlags acts);
    /// @return the built-in actions flags
    BuiltinActionsFlags buildinActions() const;
    /// @return true if a built-in action is active
    bool testBuildinActions(BuiltinActions ba) const;

    /// Set the selected state
    void setSelected(bool selected);
    /// @return true if selected
    bool isSelected() const;

    /// Set the selection color
    void setSelectionColor(const QColor& color);
    /// @return the selection color
    QColor selectionColor() const;

    /// Set the zoom factor (default 1.2)
    void setZoomFactor(double factor);
    /// @return the zoom factor
    double zoomFactor() const;

    /// Set the selected pen width offset
    void setSelectedPenWidthOffset(qreal offset = 1);
    /// @return the selected pen width offset
    qreal selectedPenWidthOffset() const;

    /// Check if a point is on the scale area
    bool isOnScale(const QPoint& pos) const;

protected:
    virtual void paintEvent(QPaintEvent*) override;
    virtual void resizeEvent(QResizeEvent*) override;
    virtual void changeEvent(QEvent*) override;

    void draw(QPainter*) const;

    void scaleChange();

private:
    void initScale(QwtScaleDraw::Alignment);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtScaleWidget::LayoutFlags)

#endif
