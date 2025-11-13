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
 *  @brief A Widget which contains a scale
 *
 *  This Widget can be used to decorate composite widgets with
 *  a scale.
 *
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

    /**
     * @brief 内置的动作
     */
    enum BuiltinActions
    {
        //! @brief 无任何动作
        ActionNone = 0x00,
        //! @brief 鼠标滚轮缩放(鼠标点击激活坐标轴后，通过滚动滚轮可以实现当前坐标轴的缩放)
        ActionWheelZoom = 0x01,
        //! @brief 鼠标点击拖动(鼠标点击激活坐标轴后，通过鼠标拖动坐标轴实现坐标轴的左右移动)
        ActionClickPan = 0x02,
        //! @brief 所有动作
        ActionAll = 0xFF
    };
    Q_DECLARE_FLAGS(BuiltinActionsFlags, BuiltinActions)
public:
    explicit QwtScaleWidget(QWidget* parent = NULL);
    explicit QwtScaleWidget(QwtScaleDraw::Alignment, QWidget* parent = NULL);
    virtual ~QwtScaleWidget();

Q_SIGNALS:

    /**
     * @brief Signal emitted, whenever the scale division changes/当刻度分度发生变化时发出的信号
     */
    void scaleDivChanged();

    /**
     * @brief Request to change the axis scale division/坐标轴主动请求变更刻度范围
     *
     * Emitted when built-in actions (zoom/pan) need to alter the scale.
     * 内置动作（缩放/平移）需要改变刻度时发射此信号。
     *
     * Unlike normal QwtPlot updates, here the axis drives the change:
     * QwtPlot receives this signal and adjusts item bounds accordingly.
     * 与常规 QwtPlot 更新不同，此处由轴驱动变更：QwtPlot 接收信号后调整图元范围。
     *
     * @param min min scale division requested/请求的最小刻度范围
     * @param min max scale division requested/请求的最大刻度范围
     */
    void requestScaleRangeUpdate(double min, double max);

    /**
     * @brief 当前轴被选中状态发生变化发射信号
     * @param selected
     */
    void selectionChanged(bool selected);

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

    // 去除了colorBar,margin,edgeMargin,BorderDistHint这些区域的矩形，也就是用来绘制刻度的区域
    QRect scaleRect() const;
    // font color of the coordinate axis/设置坐标轴的字体颜色
    void setTextColor(const QColor& c);
    QColor textColor() const;

    // color of the coordinate axis/坐标轴的颜色
    void setScaleColor(const QColor& c);
    QColor scaleColor() const;

    void layoutScale(bool update_geometry = true);

    // 获取此轴窗口对应的axisID
    QwtAxisId axisID() const;
    // 是否是x坐标轴
    bool isXAxis() const;
    // 是否是y坐标轴
    bool isYAxis() const;
    //===============================================
    // 以下接口用于内置动作
    //===============================================

    // 启用/禁用内置交互动作
    void setBuildinActions(BuiltinActionsFlags acts);
    BuiltinActionsFlags buildinActions() const;
    // 检测内置动作是否激活
    bool testBuildinActions(BuiltinActions ba) const;

    // 设置坐标轴选中状态
    void setSelected(bool selected);
    bool isSelected() const;

    // 设置选中状态的颜色
    void setSelectionColor(const QColor& color);
    QColor selectionColor() const;

    // 设置缩放因子(默认1.2)
    void setZoomFactor(double factor);
    double zoomFactor() const;

    // 设置选中后画笔的宽度修正
    void setSelectedPenWidthOffset(qreal offset = 1);
    qreal selectedPenWidthOffset() const;

    // 判断点是否在刻度区域
    bool isOnScale(const QPoint& pos) const;
    // 按像素移动坐标轴
    void panScale(int deltaPixels);
    // 放大坐标轴，会让当前像素显示的刻度越来越大
    void zoomIn(const QPoint& centerPos = QPoint());
    // 缩小坐标轴，会让当前像素显示的刻度越来越小
    void zoomOut(const QPoint& centerPos = QPoint());

protected:
    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;
    virtual void resizeEvent(QResizeEvent*) QWT_OVERRIDE;
    virtual void changeEvent(QEvent*) QWT_OVERRIDE;

    void draw(QPainter*) const;

    void scaleChange();

private:
    void initScale(QwtScaleDraw::Alignment);
    //===============================================
    // 以下函数用于内置动作
    //===============================================
    // 处理缩放
    void doZoom(double factor, const QPoint& centerPos);
    // 把当前scalewidget窗口上的点映射到坐标轴上的值
    double mapPosToScaleValue(const QPoint& pos) const;
    // 把屏幕长度转换为坐标轴的长度
    double mapLengthToScaleValue(double length) const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QwtScaleWidget::LayoutFlags)

#endif
