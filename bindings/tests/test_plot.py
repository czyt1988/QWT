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


# ---------------------------------------------------------------------------
# Task 2: Core plotting fundamentals tests
# ---------------------------------------------------------------------------

def test_qwtaxis_position_enum(qapp):
    """QwtAxis.Position enum values are accessible."""
    pos = qwtplot.QwtAxis.Position
    assert pos.YLeft == 0
    assert pos.YRight == 1
    assert pos.XBottom == 2
    assert pos.XTop == 3


def test_qwtplot_grid_attach(qapp):
    """QwtPlotGrid can be created and attached to a plot."""
    plot = qwtplot.QwtPlot()
    grid = qwtplot.QwtPlotGrid()
    grid.attach(plot)
    plot.replot()


def test_qwtplot_marker_attach(qapp):
    """QwtPlotMarker can be created, configured, and attached."""
    plot = qwtplot.QwtPlot()
    marker = qwtplot.QwtPlotMarker()
    marker.setValue(5.0, 10.0)
    marker.setLineStyle(qwtplot.QwtPlotMarker.Cross)
    marker.attach(plot)
    plot.replot()


def test_qwtsymbol_construct(qapp):
    """QwtSymbol can be constructed with style and size."""
    from PySide6.QtCore import QSize
    sym = qwtplot.QwtSymbol(qwtplot.QwtSymbol.Ellipse)
    sym.setSize(QSize(10, 10))
    assert sym.style() == qwtplot.QwtSymbol.Ellipse


def test_qwtplot_marker_set_symbol_no_double_free(qapp):
    """QwtPlotMarker.setSymbol transfers ownership to C++ (no double-free)."""
    import gc
    plot = qwtplot.QwtPlot()
    marker = qwtplot.QwtPlotMarker()
    sym = qwtplot.QwtSymbol(qwtplot.QwtSymbol.Rect)
    marker.setSymbol(sym)
    marker.attach(plot)
    plot.replot()
    # Force GC to exercise destructor paths — should not crash.
    gc.collect()
    del sym, marker
    gc.collect()


def test_qwtplot_set_axis_scale_engine(qapp):
    """QwtPlot.setAxisScaleEngine transfers ownership (no double-free)."""
    import gc
    import qwtcore
    plot = qwtplot.QwtPlot()
    engine = qwtcore.QwtLinearScaleEngine()
    plot.setAxisScaleEngine(0, engine)  # QwtAxis::YLeft = 0
    gc.collect()
    del engine
    gc.collect()
    # Engine should still be accessible through the plot.
    eng = plot.axisScaleEngine(0)
    assert eng is not None


def test_qwttext_construct_and_use(qapp):
    """QwtText value type can be constructed and used."""
    t = qwtplot.QwtText("Hello")
    assert t.text() == "Hello"
    t.setText("World")
    assert t.text() == "World"


def test_qwtcolumnrect_value_type(qapp):
    """QwtColumnRect value type is copyable and has Direction enum."""
    rect = qwtplot.QwtColumnRect()
    rect.direction = qwtplot.QwtColumnRect.BottomToTop
    assert rect.direction == qwtplot.QwtColumnRect.BottomToTop


def test_qwtplot_layout(qapp):
    """QwtPlotLayout can be accessed from QwtPlot and its rects queried."""
    plot = qwtplot.QwtPlot()
    layout = plot.plotLayout()
    assert layout is not None
    # canvasRect is valid after replot
    plot.replot()
    cr = layout.canvasRect()
    assert cr is not None


def test_qwtplot_direct_painter(qapp):
    """QwtPlotDirectPainter can be constructed."""
    dp = qwtplot.QwtPlotDirectPainter()
    assert dp is not None


def test_qwtscale_widget_access(qapp):
    """QwtScaleWidget is accessible from QwtPlot via axisWidget."""
    plot = qwtplot.QwtPlot()
    sw = plot.axisWidget(0)  # QwtAxis::YLeft = 0
    assert sw is not None


def test_qwtcurve_fitter_subclasses(qapp):
    """QwtSplineCurveFitter and QwtWeedingCurveFitter are constructible."""
    from PySide6.QtCore import QPointF
    fitter = qwtplot.QwtSplineCurveFitter()
    assert fitter is not None
    weeding = qwtplot.QwtWeedingCurveFitter()
    assert weeding is not None
