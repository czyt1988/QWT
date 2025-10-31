/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_SCALE_WIDGET_H
#define QWT_SCALE_WIDGET_H

#include "qwt_global.h"
#include "qwt_text.h"
#include "qwt_scale_draw.h"

#include <qwidget.h>
#include <qfont.h>
#include <qcolor.h>
#include <qstring.h>

class QPainter;
class QwtTransform;
class QwtScaleDiv;
class QwtColorMap;

/**
 *  @brief A Widget which contains a scale
 *
 *  This Widget can be used to decorate composite widgets with
 *  a scale.
 *
 * │<----------------------------- plot yleft edge
 * │      │       │      │tick ┌       ┌-----------------------------------
 * │      │       │      │label│       |
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
 * │      │       │      │ 1  -│       |_________________________________
 */

class QWT_EXPORT QwtScaleWidget : public QWidget
{
    Q_OBJECT

public:
    //! Layout flags of the title
    enum LayoutFlag
    {
        /*!
           The title of vertical scales is painted from top to bottom.
           Otherwise it is painted from bottom to top.
         */
        TitleInverted = 1
    };

    Q_DECLARE_FLAGS(LayoutFlags, LayoutFlag)

    explicit QwtScaleWidget(QWidget* parent = NULL);
    explicit QwtScaleWidget(QwtScaleDraw::Alignment, QWidget* parent = NULL);
    virtual ~QwtScaleWidget();

Q_SIGNALS:
    //! Signal emitted, whenever the scale division changes
    void scaleDivChanged();

public:
    void setTitle(const QString& title);
    void setTitle(const QwtText& title);
    QwtText title() const;

    void setLayoutFlag(LayoutFlag, bool on);
    bool testLayoutFlag(LayoutFlag) const;

    void setBorderDist(int dist1, int dist2);
    int startBorderDist() const;
    int endBorderDist() const;

    void getBorderDistHint(int& start, int& end) const;

    void getMinBorderDist(int& start, int& end) const;
    void setMinBorderDist(int start, int end);
    int startMinBorderDist() const;
    int endMinBorderDist() const;

    void setMargin(int);
    int margin() const;

    void setSpacing(int);
    int spacing() const;

    // 坐标轴和绘图边距的偏移，这个值实际和contentMargin类似，但qwt的contentMargin只用于minimumSizeHint
    // 对于寄生轴，需要宿主轴有很大的空白位能让寄生轴显示，这个edgeOffset主要就是让坐标轴留出一个空白位
    void setEdgeMargin(int offset);
    int edgeMargin() const;

    void setScaleDiv(const QwtScaleDiv&);
    void setTransformation(QwtTransform*);

    void setScaleDraw(QwtScaleDraw*);
    const QwtScaleDraw* scaleDraw() const;
    QwtScaleDraw* scaleDraw();

    void setLabelAlignment(Qt::Alignment);
    void setLabelRotation(double rotation);

    void setColorBarEnabled(bool);
    bool isColorBarEnabled() const;

    void setColorBarWidth(int);
    int colorBarWidth() const;

    void setColorMap(const QwtInterval&, QwtColorMap*);

    QwtInterval colorBarInterval() const;
    const QwtColorMap* colorMap() const;

    virtual QSize sizeHint() const QWT_OVERRIDE;
    virtual QSize minimumSizeHint() const QWT_OVERRIDE;

    int titleHeightForWidth(int width) const;
    int dimForLength(int length, const QFont& scaleFont) const;

    void drawColorBar(QPainter*, const QRectF&) const;
    void drawTitle(QPainter*, QwtScaleDraw::Alignment, const QRectF& rect) const;

    void setAlignment(QwtScaleDraw::Alignment);
    QwtScaleDraw::Alignment alignment() const;

    QRectF colorBarRect(const QRectF&) const;

    // font color of the coordinate axis/设置坐标轴的字体颜色
    void setTextColor(const QColor& c);
    QColor textColor() const;

    // color of the coordinate axis/坐标轴的颜色
    void setScaleColor(const QColor& c);
    QColor scaleColor() const;

    void layoutScale(bool update_geometry = true);

protected:
    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;
    virtual void resizeEvent(QResizeEvent*) QWT_OVERRIDE;
    virtual void changeEvent(QEvent*) QWT_OVERRIDE;

    void draw(QPainter*) const;

    void scaleChange();

private:
    void initScale(QwtScaleDraw::Alignment);

    class PrivateData;
    PrivateData* m_data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtScaleWidget::LayoutFlags)

#endif
