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
// plot methods that return/take core types by value. These resolve via the
// plot module's forward headers in src/plot/ (which #include "../core/xxx.h").
#include "qwt_interval.h"
#include "qwt_scale_map.h"
#include "qwt_scale_div.h"
#include "qwt_scale_engine.h"
#include "qwt_transform.h"
#include "qwt_samples.h"
#include "qwt_point_3d.h"
#include "qwt_point_polar.h"
#include "qwt_date.h"
#include "qwt_series_data.h"
#include "qwt_series_store.h"
// QwtColorMap has no forward header in src/plot; include from src/core directly
// (src/ is on the include path via qwt::plot's INTERFACE_INCLUDE_DIRECTORIES).
#include "core/qwt_colormap.h"

// ---- QWTPLOT_HEADER_BASE (always-on plot base headers) ----
#include "qwt_text.h"
#include "qwt_graphic.h"
#include "qwt_null_paintdevice.h"
#include "qwt_interval_symbol.h"
#include "qwt_column_symbol.h"
#include "qwt_symbol.h"
#include "qwt_text_label.h"
#include "qwt_abstract_scale_draw.h"
#include "qwt_scale_draw.h"
#include "qwt_round_scale_draw.h"
#include "qwt_date_scale_draw.h"
#include "qwt_date_scale_engine.h"
#include "qwt_curve_fitter.h"
#include "qwt_spline_curve_fitter.h"
#include "qwt_weeding_curve_fitter.h"

// ---- QWTPLOT_HEADER_PLOT (core 2D plot API) ----
#include "qwt_axis.h"
#include "qwt_axis_id.h"
#include "qwt_plot_abstract_canvas.h"
#include "qwt_plot_dict.h"
#include "qwt_plot_canvas.h"
#include "qwt_plot_transparent_canvas.h"
#include "qwt_plot.h"
#include "qwt_plot_item.h"
#include "qwt_plot_seriesitem.h"
#include "qwt_plot_grid.h"
#include "qwt_plot_marker.h"
#include "qwt_plot_arrowmarker.h"
#include "qwt_scale_widget.h"
#include "qwt_plot_layout.h"
#include "qwt_plot_renderer.h"
#include "qwt_plot_directpainter.h"

// ---- Task 3: Series items + legends ----
#include "qwt_plot_rasteritem.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_histogram.h"
#include "qwt_plot_abstract_barchart.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_multi_barchart.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_tradingcurve.h"
#include "qwt_plot_boxchart.h"
#include "qwt_plot_spectrocurve.h"
#include "qwt_plot_vectorfield.h"
#include "qwt_vectorfield_symbol.h"
#include "qwt_plot_shapeitem.h"
#include "qwt_plot_zoneitem.h"
#include "qwt_plot_textlabel.h"   // item version (different from qwt_text_label.h)
#include "qwt_plot_scaleitem.h"
#include "qwt_plot_graphicitem.h"
#include "qwt_plot_legenditem.h"
#include "qwt_abstract_legend.h"
#include "qwt_legend_data.h"
#include "qwt_legend.h"
#include "qwt_legend_label.h"
#include "qwt_dyngrid_layout.h"

// Glue functions for series item size()/sample() (workaround for shiboken
// add-function inject-code crash with cross-module value type conversion).
#include "glue_plot.h"

#endif // QWTPLOT_BINDING_GLOBAL_H
