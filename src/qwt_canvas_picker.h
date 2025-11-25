/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#ifndef QWTCANVASPICKER_H
#define QWTCANVASPICKER_H
#include "qwt_picker.h"
// qt
class QWidget;
// qwt
class QwtPlot;

/**
 * @brief 专门针对 canvas 的 picker 基类 / Base picker class specifically for canvas
 *
 * 提供 canvas 和 plot 的便捷访问方法，作为其他 canvas 相关工具的基类。
 * Provides convenient access methods for canvas and plot, serving as a base class
 * for other canvas-related tools.
 */
class QWT_EXPORT QwtCanvasPicker : public QwtPicker
{
    Q_OBJECT
public:
    explicit QwtCanvasPicker(QWidget* canvas);
    ~QwtCanvasPicker();

    QwtPlot* plot();
    const QwtPlot* plot() const;

    QWidget* canvas();
    const QWidget* canvas() const;
};

#endif  // QWTCANVASPICKER_H
