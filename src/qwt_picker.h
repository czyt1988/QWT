/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PICKER
#define QWT_PICKER

#include "qwt_global.h"
#include "qwt_event_pattern.h"

#include <qobject.h>
#include <QPen>
#include <QFont>
#include <QPainterPath>
#include <QPoint>
#include <QPolygon>
#include <QSize>
#include <QRect>
#include <QRegion>
class QwtPickerMachine;
class QwtWidgetOverlay;
class QwtText;
class QWidget;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class QPainter;

/**
 * @brief QwtPicker provides selections on a widget / QwtPicker 在一个部件（widget）上提供选择功能
 *
 * QwtPicker filters all enter, leave, mouse and keyboard events of a widget
 * and translates them into an array of selected points.
 *
 * The way how the points are collected depends on type of state machine
 * that is connected to the picker. Qwt offers a couple of predefined
 * state machines for selecting:
 *
 * - Nothing\n
 *   QwtPickerTrackerMachine
 * - Single points\n
 *   QwtPickerClickPointMachine, QwtPickerDragPointMachine
 * - Rectangles\n
 *   QwtPickerClickRectMachine, QwtPickerDragRectMachine
 * - Polygons\n
 *   QwtPickerPolygonMachine
 *
 * While these state machines cover the most common ways to collect points
 * it is also possible to implement individual machines as well.
 *
 * QwtPicker translates the picked points into a selection using the
 * adjustedPoints() method. adjustedPoints() is intended to be reimplemented
 * to fix up the selection according to application specific requirements.
 * (F.e. when an application accepts rectangles of a fixed aspect ratio only.)
 *
 * Optionally QwtPicker support the process of collecting points by a
 * rubber band and tracker displaying a text for the current mouse
 * position.
 *
 * ---------------------------------------------
 *
 * QwtPicker 会过滤一个部件（widget）的所有进入、离开、鼠标和键盘事件，并将它们转换为一个选定坐标点的数组。
 *
 * 收集点的方式取决于连接到选择器（picker）的状态机（state machine）类型。Qwt 提供了几个预定义的选择状态机：
 *
 * - 无\n
 *   QwtPickerTrackerMachine
 * - 单个点\n
 *   QwtPickerClickPointMachine, QwtPickerDragPointMachine
 * - 矩形\n
 *   QwtPickerClickRectMachine, QwtPickerDragRectMachine
 * - 多边形\n
 *   QwtPickerPolygonMachine
 *
 * 虽然这些状态机涵盖了最常见的点收集方式，但也可以实现自定义的状态机。
 *
 * QwtPicker 使用 adjustedPoints() 方法将拾取的点转换为一个选择区域/集合。adjustedPoints() 方法旨在被重写，
 * 以便根据应用程序的特定要求修正选择结果。（例如：当应用程序只接受固定宽高比的矩形时。）
 *
 * QwtPicker 可以选择性地通过一个橡皮筋（rubber band）和一个显示当前鼠标位置文本的追踪器（tracker）来辅助点的收集过程。
 *
 * @par Example
 * @code
 * #include <qwt_picker.h>
 * #include <qwt_picker_machine.h>
 *
 * QwtPicker *picker = new QwtPicker(widget);
 * picker->setStateMachine(new QwtPickerDragRectMachine);
 * picker->setTrackerMode(QwtPicker::ActiveOnly);
 * picker->setRubberBand(QwtPicker::RectRubberBand);
 * @endcode
 *
 * The state machine triggers the following commands:
 *
 * 状态机会触发以下命令：
 *
 * - begin()\n
 *   Activate/Initialize the selection. / 激活/初始化选择。
 * - append()\n
 *   Add a new point / 添加一个新点。
 * - move() \n
 *   Change the position of the last point. / 改变最后一个点的位置。
 * - remove()\n
 *   Remove the last point. / 移除最后一个点。
 * - end()\n
 *   Terminate the selection and call accept to validate the picked points. / 终止选择，并调用 accept 来验证拾取的点。
 *
 * The picker is active (isActive()), between begin() and end().
 * In active state the rubber band is displayed, and the tracker is visible
 * in case of trackerMode is ActiveOnly or AlwaysOn.
 *
 * 在 begin() 和 end() 之间，选择器处于活动状态（isActive()）。
 * 在活动状态下，橡皮筋会显示。如果追踪器模式（trackerMode）是 ActiveOnly 或 AlwaysOn，追踪器也会可见。
 *
 * The cursor can be moved using the arrow keys. All selections can be aborted
 * using the abort key. (QwtEventPattern::KeyPatternCode)
 *
 * 可以使用方向键移动光标。所有选择都可以使用取消键（abort key）来中止。(QwtEventPattern::KeyPatternCode)
 *
 * @warning In case of QWidget::NoFocus the focus policy of the observed
 *          widget is set to QWidget::WheelFocus and mouse tracking
 *          will be manipulated while the picker is active,
 *          or if trackerMode() is AlwayOn./
 *          如果观察的部件（widget）的焦点策略是 QWidget::NoFocus，
 *          那么在选择器处于活动状态时，或者如果 trackerMode() 是 AlwaysOn 时，
 *          该部件的焦点策略会被设置为 QWidget::WheelFocus，并且鼠标追踪（mouse tracking）会被操控。
 */
class QWT_EXPORT QwtPicker : public QObject, public QwtEventPattern
{
    Q_OBJECT

    Q_ENUMS(RubberBand DisplayMode ResizeMode)

    Q_PROPERTY(bool isEnabled READ isEnabled WRITE setEnabled)
    Q_PROPERTY(ResizeMode resizeMode READ resizeMode WRITE setResizeMode)

    Q_PROPERTY(DisplayMode trackerMode READ trackerMode WRITE setTrackerMode)
    Q_PROPERTY(QPen trackerPen READ trackerPen WRITE setTrackerPen)
    Q_PROPERTY(QFont trackerFont READ trackerFont WRITE setTrackerFont)

    Q_PROPERTY(RubberBand rubberBand READ rubberBand WRITE setRubberBand)
    Q_PROPERTY(QPen rubberBandPen READ rubberBandPen WRITE setRubberBandPen)

public:
    /*!
       Rubber band style

       The default value is QwtPicker::NoRubberBand.
       \sa setRubberBand(), rubberBand()
     */

    enum RubberBand
    {
        //! No rubberband.
        NoRubberBand = 0,

        //! A horizontal line ( only for QwtPickerMachine::PointSelection )
        HLineRubberBand,

        //! A vertical line ( only for QwtPickerMachine::PointSelection )
        VLineRubberBand,

        //! A crosshair ( only for QwtPickerMachine::PointSelection )
        CrossRubberBand,

        //! A rectangle ( only for QwtPickerMachine::RectSelection )
        RectRubberBand,

        //! An ellipse ( only for QwtPickerMachine::RectSelection )
        EllipseRubberBand,

        //! A polygon ( only for QwtPickerMachine::PolygonSelection )
        PolygonRubberBand,

        /*!
           Values >= UserRubberBand can be used to define additional
           rubber bands.
         */
        UserRubberBand = 100
    };

    /*!
       \brief Display mode
       \sa setTrackerMode(), trackerMode(), isActive()
     */
    enum DisplayMode
    {
        //! Display never
        AlwaysOff,

        //! Display always
        AlwaysOn,

        //! Display only when the selection is active
        ActiveOnly
    };

    /*!
       Controls what to do with the selected points of an active
         selection when the observed widget is resized.

       The default value is QwtPicker::Stretch.
       \sa setResizeMode()
     */

    enum ResizeMode
    {
        //! All points are scaled according to the new size,
        Stretch,

        //! All points remain unchanged.
        KeepSize
    };

    explicit QwtPicker(QWidget* parent);
    explicit QwtPicker(RubberBand rubberBand, DisplayMode trackerMode, QWidget*);

    virtual ~QwtPicker();

    void setStateMachine(QwtPickerMachine*);
    const QwtPickerMachine* stateMachine() const;
    QwtPickerMachine* stateMachine();

    void setRubberBand(RubberBand);
    RubberBand rubberBand() const;

    void setTrackerMode(DisplayMode);
    DisplayMode trackerMode() const;

    void setResizeMode(ResizeMode);
    ResizeMode resizeMode() const;

    void setRubberBandPen(const QPen&);
    QPen rubberBandPen() const;

    void setTrackerPen(const QPen&);
    QPen trackerPen() const;

    void setTrackerFont(const QFont&);
    QFont trackerFont() const;

    bool isEnabled() const;
    bool isActive() const;

    virtual bool eventFilter(QObject*, QEvent*) QWT_OVERRIDE;

    QWidget* parentWidget();
    const QWidget* parentWidget() const;

    virtual QPainterPath pickArea() const;

    virtual void drawRubberBand(QPainter*) const;
    virtual void drawTracker(QPainter*) const;

    virtual QRegion trackerMask() const;
    virtual QRegion rubberBandMask() const;

    virtual QwtText trackerText(const QPoint& pos) const;
    QPoint trackerPosition() const;
    virtual QRect trackerRect(const QFont&) const;

    QPolygon selection() const;

public Q_SLOTS:
    void setEnabled(bool);

Q_SIGNALS:
    /*!
       A signal indicating, when the picker has been activated.
       Together with setEnabled() it can be used to implement
       selections with more than one picker.

       \param on True, when the picker has been activated
     */
    void activated(bool on);

    /*!
       A signal emitting the selected points,
       at the end of a selection.

       \param polygon Selected points
     */
    void selected(const QPolygon& polygon);

    /*!
       A signal emitted when a point has been appended to the selection

       \param pos Position of the appended point.
       \sa append(). moved()
     */
    void appended(const QPoint& pos);

    /*!
       A signal emitted whenever the last appended point of the
       selection has been moved.

       \param pos Position of the moved last point of the selection.
       \sa move(), appended()
     */
    void moved(const QPoint& pos);

    /*!
       A signal emitted whenever the last appended point of the
       selection has been removed.

       \param pos Position of the point, that has been removed
       \sa remove(), appended()
     */
    void removed(const QPoint& pos);
    /*!
       A signal emitted when the active selection has been changed.
       This might happen when the observed widget is resized.

       \param selection Changed selection
       \sa stretchSelection()
     */
    void changed(const QPolygon& selection);

protected:
    virtual QPolygon adjustedPoints(const QPolygon&) const;

    virtual void transition(const QEvent*);

    virtual void begin();
    virtual void append(const QPoint&);
    virtual void move(const QPoint&);
    virtual void remove();
    virtual bool end(bool ok = true);

    virtual bool accept(QPolygon&) const;
    virtual void reset();

    virtual void widgetMousePressEvent(QMouseEvent*);
    virtual void widgetMouseReleaseEvent(QMouseEvent*);
    virtual void widgetMouseDoubleClickEvent(QMouseEvent*);
    virtual void widgetMouseMoveEvent(QMouseEvent*);
    virtual void widgetWheelEvent(QWheelEvent*);
    virtual void widgetKeyPressEvent(QKeyEvent*);
    virtual void widgetKeyReleaseEvent(QKeyEvent*);
    virtual void widgetEnterEvent(QEvent*);
    virtual void widgetLeaveEvent(QEvent*);

    virtual void stretchSelection(const QSize& oldSize, const QSize& newSize);

    virtual void updateDisplay();

    const QwtWidgetOverlay* rubberBandOverlay() const;
    const QwtWidgetOverlay* trackerOverlay() const;

    const QPolygon& pickedPoints() const;

private:
    void init(QWidget*, RubberBand rubberBand, DisplayMode trackerMode);

    void setMouseTracking(bool);

    class PrivateData;
    PrivateData* m_data;
};

#endif
