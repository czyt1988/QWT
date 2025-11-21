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

#ifndef QWT_CACHE_PANNER_H
#define QWT_CACHE_PANNER_H

#include "qwt_global.h"
#include <qwidget.h>

class QCursor;
class QPixmap;

/*!
   \brief QwtCachePanner provides panning of a widget

   QwtCachePanner grabs the contents of a widget, that can be dragged
   in all directions. The offset between the start and the end position
   is emitted by the panned signal.

   QwtCachePanner grabs the content of the widget into a pixmap and moves
   the pixmap around, without initiating any repaint events for the widget.
   Areas, that are not part of content are not painted  while panning.
   This makes panning fast enough for widgets, where
   repaints are too slow for mouse movements.

   For widgets, where repaints are very fast it might be better to
   implement panning manually by mapping mouse events into paint events.
 */
class QWT_EXPORT QwtCachePanner : public QWidget
{
    Q_OBJECT

public:
    explicit QwtCachePanner(QWidget* parent);
    virtual ~QwtCachePanner();

    void setEnabled(bool);
    bool isEnabled() const;

    void setMouseButton(Qt::MouseButton, Qt::KeyboardModifiers = Qt::NoModifier);
    void getMouseButton(Qt::MouseButton& button, Qt::KeyboardModifiers&) const;

    void setAbortKey(int key, Qt::KeyboardModifiers = Qt::NoModifier);
    void getAbortKey(int& key, Qt::KeyboardModifiers&) const;

    void setCursor(const QCursor&);
    const QCursor cursor() const;

    void setOrientations(Qt::Orientations);
    Qt::Orientations orientations() const;

    bool isOrientationEnabled(Qt::Orientation) const;

    virtual bool eventFilter(QObject*, QEvent*) QWT_OVERRIDE;

Q_SIGNALS:
    /*!
       Signal emitted, when panning is done

       \param dx Offset in horizontal direction
       \param dy Offset in vertical direction
     */
    void panned(int dx, int dy);

    /*!
       Signal emitted, while the widget moved, but panning
       is not finished.

       \param dx Offset in horizontal direction
       \param dy Offset in vertical direction
     */
    void moved(int dx, int dy);

protected:
    virtual void widgetMousePressEvent(QMouseEvent*);
    virtual void widgetMouseReleaseEvent(QMouseEvent*);
    virtual void widgetMouseMoveEvent(QMouseEvent*);
    virtual void widgetKeyPressEvent(QKeyEvent*);
    virtual void widgetKeyReleaseEvent(QKeyEvent*);

    virtual void paintEvent(QPaintEvent*) QWT_OVERRIDE;

    virtual QBitmap contentsMask() const;
    virtual QPixmap grab() const;

private:
#ifndef QT_NO_CURSOR
    void showCursor(bool);
#endif

    class PrivateData;
    PrivateData* m_data;
};

#endif
