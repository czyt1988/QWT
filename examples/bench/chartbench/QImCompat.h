/*****************************************************************************
 * chartbench - QIm build workaround
 *
 * QIm 0.0.3 (the pinned submodule revision) uses QPointF in
 * src/core/plot/QImPlotValueTrackerNode.h without including <QPointF>. On Qt
 * 6.7 that header is no longer pulled in transitively and the library fails to
 * compile with "uses undefined class 'QPointF'".
 *
 * This header is force-included (/FI) into the QIm targets from the CMakeLists
 * so the submodule itself stays untouched and reproducible.
 *****************************************************************************/

#ifndef CHARTBENCH_QIM_COMPAT_H
#define CHARTBENCH_QIM_COMPAT_H

#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>

#endif // CHARTBENCH_QIM_COMPAT_H
