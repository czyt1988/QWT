/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_UTILS_H
#define QWT_UTILS_H

#include "qwt_global.h"
#include <QColor>

class QSize;
class QwtPlotItem;

/*!
   Some constants for use within Qwt.
 */
namespace Qwt
{
// 获取item的颜色
QColor QWT_EXPORT plotItemColor(QwtPlotItem* item, const QColor& defaultColor = QColor());
}

QWT_EXPORT QSize qwtExpandedToGlobalStrut(const QSize&);

#endif
