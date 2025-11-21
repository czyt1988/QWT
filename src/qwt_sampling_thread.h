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

#ifndef QWT_SAMPLING_THREAD_H
#define QWT_SAMPLING_THREAD_H

#include "qwt_global.h"
#include <qthread.h>

/*!
   \brief A thread collecting samples at regular intervals.

   Continuous signals are converted into a discrete signal by
   collecting samples at regular intervals. A discrete signal
   can be displayed by a QwtPlotSeriesItem on a QwtPlot widget.

   QwtSamplingThread starts a thread calling periodically sample(),
   to collect and store ( or emit ) a single sample.

   \sa QwtPlotCurve, QwtPlotSeriesItem
 */
class QWT_EXPORT QwtSamplingThread : public QThread
{
    Q_OBJECT

  public:
    virtual ~QwtSamplingThread();

    double interval() const;
    double elapsed() const;

  public Q_SLOTS:
    void setInterval( double interval );
    void stop();

  protected:
    explicit QwtSamplingThread( QObject* parent = NULL );

    virtual void run() QWT_OVERRIDE;

    /*!
       Collect a sample

       \param elapsed Time since the thread was started in seconds
       \note Due to a bug in previous version elapsed was passed as
             seconds instead of miliseconds. To avoid breaking existing
             code we stay with seconds for now.
     */
    virtual void sample( double elapsed ) = 0;

  private:
    class PrivateData;
    PrivateData* m_data;
};

#endif
