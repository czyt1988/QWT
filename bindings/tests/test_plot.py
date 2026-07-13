"""Smoke tests for the qwtplot PySide6 binding (POC: QwtPlot only).

Validates the multi-module CMake toolchain (qwtplot depends on qwtcore via
load-typesystem generate="no") and that QwtPlot's multiple inheritance
(QFrame + QwtPlotDict) resolves correctly under Shiboken6.
"""
import pytest
from PySide6.QtWidgets import QApplication

# Will fail at collection time if qwtplot cannot be imported — that's the POC gate.
import qwtplot


@pytest.fixture(scope="module")
def qapp():
    """QwtPlot is a QWidget, so a QApplication must exist before construction."""
    app = QApplication.instance() or QApplication([])
    yield app


def test_qwtplot_construct(qapp):
    """QwtPlot can be constructed (validates QFrame+QwtPlotDict multi-inheritance)."""
    plot = qwtplot.QwtPlot()
    assert plot is not None


def test_qwtplot_set_title(qapp):
    """QwtPlot.setTitle accepts a string and title() returns a QwtText."""
    plot = qwtplot.QwtPlot()
    plot.setTitle("My Plot")
    title = plot.title()
    assert title.text() == "My Plot"


def test_qwtplot_canvas_exists(qapp):
    """QwtPlot has a canvas widget after construction."""
    plot = qwtplot.QwtPlot()
    canvas = plot.canvas()
    assert canvas is not None


def test_qwtplot_axis_scale(qapp):
    """QwtPlot.setAxisScale sets the axis range (uses QwtAxisId = int)."""
    plot = qwtplot.QwtPlot()
    # QwtAxis::YLeft = 0, QwtAxis::XBottom = 2
    plot.setAxisScale(0, 0.0, 10.0)
    plot.setAxisScale(2, -5.0, 5.0)
    # replot should not crash (offscreen platform)
    plot.replot()


def test_qwtplot_replot_no_crash(qapp):
    """QwtPlot.replot runs without error on an empty plot (offscreen)."""
    plot = qwtplot.QwtPlot()
    plot.replot()
    # A second replot should also be fine.
    plot.replot()


def test_qwtplot_enum_legend_position(qapp):
    """QwtPlot.LegendPosition enum is accessible."""
    lp = qwtplot.QwtPlot.LegendPosition
    assert hasattr(lp, "LeftLegend")
    assert hasattr(lp, "RightLegend")
