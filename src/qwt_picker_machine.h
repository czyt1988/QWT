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

#ifndef QWT_PICKER_MACHINE
#define QWT_PICKER_MACHINE

#include "qwt_global.h"

class QwtEventPattern;
class QEvent;
template< typename T > class QList;

/*!
   \brief A state machine for QwtPicker selections

   QwtPickerMachine accepts key and mouse events and translates them
   into selection commands.

   \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
 */

class QWT_EXPORT QwtPickerMachine
{
  public:
    /*!
       Type of a selection.
       \sa selectionType()
     */
    enum SelectionType
    {
        //! The state machine not usable for any type of selection.
        NoSelection = -1,

        //! The state machine is for selecting a single point.
        PointSelection,

        //! The state machine is for selecting a rectangle (2 points).
        RectSelection,

        //! The state machine is for selecting a polygon (many points).
        PolygonSelection
    };

    //! Commands - the output of a state machine
    enum Command
    {
        Begin,
        Append,
        Move,
        Remove,
        End
    };

    explicit QwtPickerMachine( SelectionType );
    virtual ~QwtPickerMachine();

    //! Transition
    virtual QList< Command > transition(
        const QwtEventPattern&, const QEvent* ) = 0;
    void reset();

    int state() const;
    void setState( int );

    SelectionType selectionType() const;

  private:
    const SelectionType m_selectionType;
    int m_state;
};

/*!
   \brief A state machine for indicating mouse movements

   QwtPickerTrackerMachine supports displaying information
   corresponding to mouse movements, but is not intended for
   selecting anything. Begin/End are related to Enter/Leave events.
 */
class QWT_EXPORT QwtPickerTrackerMachine : public QwtPickerMachine
{
  public:
    QwtPickerTrackerMachine();

    virtual QList< Command > transition(
        const QwtEventPattern&, const QEvent* ) QWT_OVERRIDE;
};

/*!
   \brief A state machine for point selections

   Pressing QwtEventPattern::MouseSelect1 or
   QwtEventPattern::KeySelect1 selects a point.

   \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
 */
class QWT_EXPORT QwtPickerClickPointMachine : public QwtPickerMachine
{
  public:
    QwtPickerClickPointMachine();

    virtual QList< Command > transition(
        const QwtEventPattern&, const QEvent* ) QWT_OVERRIDE;
};

/*!
   \brief A state machine for point selections

   Pressing QwtEventPattern::MouseSelect1 or QwtEventPattern::KeySelect1
   starts the selection, releasing QwtEventPattern::MouseSelect1 or
   a second press of QwtEventPattern::KeySelect1 terminates it.
 */
class QWT_EXPORT QwtPickerDragPointMachine : public QwtPickerMachine
{
  public:
    QwtPickerDragPointMachine();

    virtual QList< Command > transition(
        const QwtEventPattern&, const QEvent* ) QWT_OVERRIDE;
};

/*!
   \brief A state machine for rectangle selections

   Pressing QwtEventPattern::MouseSelect1 starts
   the selection, releasing it selects the first point. Pressing it
   again selects the second point and terminates the selection.
   Pressing QwtEventPattern::KeySelect1 also starts the
   selection, a second press selects the first point. A third one selects
   the second point and terminates the selection.

   \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
 */

class QWT_EXPORT QwtPickerClickRectMachine : public QwtPickerMachine
{
  public:
    QwtPickerClickRectMachine();

    virtual QList< Command > transition(
        const QwtEventPattern&, const QEvent* ) QWT_OVERRIDE;
};

/*!
   \brief A state machine for rectangle selections

   Pressing QwtEventPattern::MouseSelect1 selects
   the first point, releasing it the second point.
   Pressing QwtEventPattern::KeySelect1 also selects the
   first point, a second press selects the second point and terminates
   the selection.

   \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
 */

class QWT_EXPORT QwtPickerDragRectMachine : public QwtPickerMachine
{
  public:
    QwtPickerDragRectMachine();

    virtual QList< Command > transition(
        const QwtEventPattern&, const QEvent* ) QWT_OVERRIDE;
};

/*!
   \brief A state machine for line selections

   Pressing QwtEventPattern::MouseSelect1 selects
   the first point, releasing it the second point.
   Pressing QwtEventPattern::KeySelect1 also selects the
   first point, a second press selects the second point and terminates
   the selection.

   A common use case of QwtPickerDragLineMachine are pickers for
   distance measurements.

   \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
 */

class QWT_EXPORT QwtPickerDragLineMachine : public QwtPickerMachine
{
  public:
    QwtPickerDragLineMachine();

    virtual QList< Command > transition(
        const QwtEventPattern&, const QEvent* ) QWT_OVERRIDE;
};

/*!
   \brief A state machine for polygon selections

   Pressing QwtEventPattern::MouseSelect1 or QwtEventPattern::KeySelect1
   starts the selection and selects the first point, or appends a point.
   Pressing QwtEventPattern::MouseSelect2 or QwtEventPattern::KeySelect2
   appends the last point and terminates the selection.

   \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
 */

class QWT_EXPORT QwtPickerPolygonMachine : public QwtPickerMachine
{
  public:
    QwtPickerPolygonMachine();

    virtual QList< Command > transition(
        const QwtEventPattern&, const QEvent* ) QWT_OVERRIDE;
};

#endif
