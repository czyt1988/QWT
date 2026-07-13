// global.h — aggregated header for qwtplot Shiboken bindings.
// Shiboken parses this single translation unit to discover all types to wrap.
//
// Include order follows src/plot/CMakeLists.txt header groups.
//
// Core types are referenced by value in plot method signatures (e.g.
// QwtPlot::canvasMap returns QwtScaleMap). The plot module has "forward
// headers" (qwt_scale_map.h, qwt_interval.h, etc.) that just #include
// "../core/xxx.h". We include these so clang's code model has the full type
// definitions. The types themselves are NOT re-declared in the typesystem —
// they are loaded via typesystem_qwtcore.xml generate="no".

#ifndef QWTPLOT_BINDING_GLOBAL_H
#define QWTPLOT_BINDING_GLOBAL_H

// Qt value/widget types must be fully visible before qwt headers (shiboken
// needs complete types for value-returning signatures and QWidget bases).
#include <QWidget>
#include <QFrame>
#include <QPolygon>
#include <QPolygonF>
#include <QVector>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QPixmap>
#include <QPainter>
#include <QSizeF>
#include <QRectF>
#include <QPointF>

// Core forward headers (each just #include "../core/xxx.h") — make core types
// fully visible to clang's code model so shiboken can synthesize wrappers for
// plot methods that return/take core types by value.
#include "qwt_interval.h"
#include "qwt_scale_map.h"
#include "qwt_scale_div.h"
#include "qwt_scale_engine.h"
#include "qwt_transform.h"
#include "qwt_samples.h"
#include "qwt_point_3d.h"
#include "qwt_point_polar.h"
#include "qwt_date.h"

// POC: minimal set for QwtPlot validation.
#include "qwt_text.h"
#include "qwt_axis_id.h"
#include "qwt_plot_abstract_canvas.h"
#include "qwt_plot_dict.h"
#include "qwt_plot_canvas.h"
#include "qwt_plot.h"

#endif // QWTPLOT_BINDING_GLOBAL_H
