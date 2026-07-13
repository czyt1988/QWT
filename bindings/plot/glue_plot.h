// glue_plot.h — declarations for glue functions that wrap template-inherited
// QwtSeriesStore<T> methods (size/sample) for Shiboken binding.
//
// Shiboken6 add-function inject-code has a bug when converting cross-module
// value types (like QPointF, QwtIntervalSample from qwtcore) via copyToPython.
// The auto-generated wrapper code works fine, but inject-code in <add-function>
// crashes. To work around this, we provide C++ functions that return PyObject*
// directly, handling the conversion in the .cpp where the full Shiboken/PySide6
// headers are available. Shiboken's clang parser only sees the declarations
// (signatures), not the implementation.

#ifndef GLUE_PLOT_H
#define GLUE_PLOT_H

#include <cstddef>
struct _object;
typedef struct _object PyObject;

#include "qwt_plot_curve.h"
#include "qwt_plot_histogram.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_multi_barchart.h"
#include "qwt_plot_intervalcurve.h"
#include "qwt_plot_tradingcurve.h"
#include "qwt_plot_boxchart.h"
#include "qwt_plot_spectrocurve.h"
#include "qwt_plot_vectorfield.h"
#include "qwt_point_3d.h"
#include "qwt_samples.h"

// QwtPlotCurve (sample type: QPointF)
std::size_t glue_QwtPlotCurve_size(const QwtPlotCurve* self);
PyObject* glue_QwtPlotCurve_sample(const QwtPlotCurve* self, std::size_t index);

// QwtPlotHistogram (sample type: QwtIntervalSample)
std::size_t glue_QwtPlotHistogram_size(const QwtPlotHistogram* self);
PyObject* glue_QwtPlotHistogram_sample(const QwtPlotHistogram* self, std::size_t index);

// QwtPlotBarChart (sample type: QPointF)
std::size_t glue_QwtPlotBarChart_size(const QwtPlotBarChart* self);
PyObject* glue_QwtPlotBarChart_sample(const QwtPlotBarChart* self, std::size_t index);

// QwtPlotMultiBarChart (sample type: QwtSetSample)
std::size_t glue_QwtPlotMultiBarChart_size(const QwtPlotMultiBarChart* self);
PyObject* glue_QwtPlotMultiBarChart_sample(const QwtPlotMultiBarChart* self, std::size_t index);

// QwtPlotIntervalCurve (sample type: QwtIntervalSample)
std::size_t glue_QwtPlotIntervalCurve_size(const QwtPlotIntervalCurve* self);
PyObject* glue_QwtPlotIntervalCurve_sample(const QwtPlotIntervalCurve* self, std::size_t index);

// QwtPlotTradingCurve (sample type: QwtOHLCSample)
std::size_t glue_QwtPlotTradingCurve_size(const QwtPlotTradingCurve* self);
PyObject* glue_QwtPlotTradingCurve_sample(const QwtPlotTradingCurve* self, std::size_t index);

// QwtPlotBoxChart (sample type: QwtBoxSample)
std::size_t glue_QwtPlotBoxChart_size(const QwtPlotBoxChart* self);
PyObject* glue_QwtPlotBoxChart_sample(const QwtPlotBoxChart* self, std::size_t index);

// QwtPlotSpectroCurve (sample type: QwtPoint3D)
std::size_t glue_QwtPlotSpectroCurve_size(const QwtPlotSpectroCurve* self);
PyObject* glue_QwtPlotSpectroCurve_sample(const QwtPlotSpectroCurve* self, std::size_t index);

// QwtPlotVectorField (sample type: QwtVectorFieldSample)
std::size_t glue_QwtPlotVectorField_size(const QwtPlotVectorField* self);
PyObject* glue_QwtPlotVectorField_sample(const QwtPlotVectorField* self, std::size_t index);

#endif // GLUE_PLOT_H
