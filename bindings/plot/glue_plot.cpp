// glue_plot.cpp — implementation of glue functions for qwtplot Shiboken bindings.
//
// Shiboken6 has a bug where copyToPython crashes when called from glue code
// (a separately compiled .cpp file) even though it works in auto-generated
// wrapper code. The root cause appears to be that type struct/converter
// global variables are not properly linked across compilation units within
// the same module DLL.
//
// Workaround: construct Python objects directly using CPython API
// (Py_BuildValue / PyList) instead of Shiboken converters. This returns plain
// Python tuples/lists rather than typed PySide6 objects, but is reliable and
// sufficient for data access.

#include <shiboken.h>
#include <pyside6_qtcore_python.h>
#include <qwtcore_python.h>
#include "glue_plot.h"

// ---- QwtPlotCurve (QPointF -> tuple (x, y)) ----
std::size_t glue_QwtPlotCurve_size(const QwtPlotCurve* self)
{
    return self->dataSize();
}
PyObject* glue_QwtPlotCurve_sample(const QwtPlotCurve* self, std::size_t index)
{
    QPointF result = self->sample(index);
    return Py_BuildValue("(dd)", result.x(), result.y());
}

// ---- QwtPlotHistogram (QwtIntervalSample -> tuple (value, (min, max))) ----
std::size_t glue_QwtPlotHistogram_size(const QwtPlotHistogram* self)
{
    return self->dataSize();
}
PyObject* glue_QwtPlotHistogram_sample(const QwtPlotHistogram* self, std::size_t index)
{
    QwtIntervalSample result = self->sample(index);
    return Py_BuildValue("(d(dd))", result.value,
                         result.interval.minValue(), result.interval.maxValue());
}

// ---- QwtPlotBarChart (QPointF -> tuple (x, y)) ----
std::size_t glue_QwtPlotBarChart_size(const QwtPlotBarChart* self)
{
    return self->dataSize();
}
PyObject* glue_QwtPlotBarChart_sample(const QwtPlotBarChart* self, std::size_t index)
{
    QPointF result = self->sample(index);
    return Py_BuildValue("(dd)", result.x(), result.y());
}

// ---- QwtPlotMultiBarChart (QwtSetSample -> tuple (value, [set...])) ----
std::size_t glue_QwtPlotMultiBarChart_size(const QwtPlotMultiBarChart* self)
{
    return self->dataSize();
}
PyObject* glue_QwtPlotMultiBarChart_sample(const QwtPlotMultiBarChart* self, std::size_t index)
{
    QwtSetSample result = self->sample(index);
    PyObject* pySet = PyList_New(result.set.size());
    for (int i = 0; i < result.set.size(); ++i)
        PyList_SET_ITEM(pySet, i, PyFloat_FromDouble(result.set[i]));
    PyObject* pyResult = PyTuple_New(2);
    PyTuple_SET_ITEM(pyResult, 0, PyFloat_FromDouble(result.value));
    PyTuple_SET_ITEM(pyResult, 1, pySet);
    return pyResult;
}

// ---- QwtPlotIntervalCurve (QwtIntervalSample -> tuple) ----
std::size_t glue_QwtPlotIntervalCurve_size(const QwtPlotIntervalCurve* self)
{
    return self->dataSize();
}
PyObject* glue_QwtPlotIntervalCurve_sample(const QwtPlotIntervalCurve* self, std::size_t index)
{
    QwtIntervalSample result = self->sample(index);
    return Py_BuildValue("(d(dd))", result.value,
                         result.interval.minValue(), result.interval.maxValue());
}

// ---- QwtPlotTradingCurve (QwtOHLCSample -> tuple (time, open, high, low, close)) ----
std::size_t glue_QwtPlotTradingCurve_size(const QwtPlotTradingCurve* self)
{
    return self->dataSize();
}
PyObject* glue_QwtPlotTradingCurve_sample(const QwtPlotTradingCurve* self, std::size_t index)
{
    QwtOHLCSample result = self->sample(index);
    return Py_BuildValue("(ddddd)", result.time, result.open, result.high,
                         result.low, result.close);
}

// ---- QwtPlotBoxChart (QwtBoxSample -> tuple (pos, whiskerLower, q1, median, q3, whiskerUpper)) ----
std::size_t glue_QwtPlotBoxChart_size(const QwtPlotBoxChart* self)
{
    return self->dataSize();
}
PyObject* glue_QwtPlotBoxChart_sample(const QwtPlotBoxChart* self, std::size_t index)
{
    QwtBoxSample result = self->sample(index);
    return Py_BuildValue("(dddddd)", result.position, result.whiskerLower,
                         result.q1, result.median, result.q3, result.whiskerUpper);
}

// ---- QwtPlotSpectroCurve (QwtPoint3D -> tuple (x, y, z)) ----
std::size_t glue_QwtPlotSpectroCurve_size(const QwtPlotSpectroCurve* self)
{
    return self->dataSize();
}
PyObject* glue_QwtPlotSpectroCurve_sample(const QwtPlotSpectroCurve* self, std::size_t index)
{
    QwtPoint3D result = self->sample(index);
    return Py_BuildValue("(ddd)", result.x(), result.y(), result.z());
}

// ---- QwtPlotVectorField (QwtVectorFieldSample -> tuple (x, y, vx, vy)) ----
std::size_t glue_QwtPlotVectorField_size(const QwtPlotVectorField* self)
{
    return self->dataSize();
}
PyObject* glue_QwtPlotVectorField_sample(const QwtPlotVectorField* self, std::size_t index)
{
    QwtVectorFieldSample result = self->sample(index);
    return Py_BuildValue("(dddd)", result.x, result.y, result.vx, result.vy);
}
