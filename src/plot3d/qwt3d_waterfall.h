#ifndef QWT3D_WATERFALL_H
#define QWT3D_WATERFALL_H

#include "qwt3d_plotitem.h"
#include "qwt3d_types.h"
#include "qwt3d_color.h"

#include <QVector>

/**
 * @brief 3D waterfall plot item with a built-in sliding window
 * @details A waterfall renders a sequence of one-dimensional traces (spectra,
 *          chromatograms, ...) as stacked curves along the depth axis. The item
 *          owns the storage: push one trace per sample interval with pushTrace()
 *          and the item maintains the sliding window, the depth numbering, the
 *          ribbon geometry and the color mapping.
 *
 * The depth coordinate of a trace is its slot index inside the window, so the
 * hull z-range grows monotonically from [0,0] to [0, traceCapacity()-1] and
 * then stays stable - no per-frame depth autoscale jitter, no user-side depth
 * bookkeeping. The amplitude (y) range still follows the data; fix it with
 * setColorRange() for stable amplitude colors in a live waterfall.
 *
 * @code
 * auto* waterfall = new Qwt3DWaterfall();
 * waterfall->attach(plot);
 * waterfall->setTraceCapacity(64);
 * waterfall->setPlotStyle(Qwt3DWaterfall::RibbonMesh);
 * waterfall->setColorMode(Qwt3DWaterfall::ByAmplitude);
 * waterfall->setColorRange(0.0, 1.0);      // fix the range, no autoscale jitter
 *
 * // per timer tick (30 Hz):
 * waterfall->pushTrace(frame);             // QVector<double>, 256 bins
 * @endcode
 *
 * @sa Qwt3DPlotItem, Qwt3DLine, Qwt3DColor
 */
class QWT3D_EXPORT Qwt3DWaterfall : public Qwt3DPlotItem
{
    QWT_DECLARE_PRIVATE(Qwt3DWaterfall)

public:
    /// Geometry style of a trace
    enum PlotStyle
    {
        Ribbon,      ///< Filled curtain from the baseline up to the trace (default)
        RibbonMesh,  ///< Ribbon plus a line outline drawn on top
        Wireframe,   ///< Line outline only (no fill)
        Tube         ///< Lit tube swept along the trace (thick strokes)
    };

    /// Where the per-vertex color comes from
    enum ColorMode
    {
        ByDepth,      ///< Colormap over the trace position in the window (default)
        ByAmplitude,  ///< Colormap over the sample value
        Solid         ///< Single color (setColor)
    };

    /// Depth direction of the sliding window
    enum DepthOrder
    {
        NewestInFront,  ///< The newest trace sits at the largest z (default)
        NewestBehind    ///< The newest trace sits at z == 0
    };

    Qwt3DWaterfall();
    ~Qwt3DWaterfall() override;

    // ---- Data / sliding window ----

    /// Returns the number of visible traces (window size)
    int traceCapacity() const;
    /// Sets the window size (>= 1); excess traces are dropped
    void setTraceCapacity(int count);
    /// Returns how many traces are currently stored (<= traceCapacity)
    int traceCount() const;
    /// Returns the bin count of the stored traces (0 before the first push)
    int samplesPerTrace() const;

    /// Appends one trace (newest); drops the oldest when the window is full
    void pushTrace(const double* values, int count);
    /// Appends one trace from a QVector
    void pushTrace(const QVector<double>& values);
    /// Clears all traces (capacity, style and colors are kept)
    void clear();

    /// Sets the x value of every bin (empty means 0..samplesPerTrace-1)
    void setBinValues(const QVector<double>& x);
    /// Returns the current bin values
    QVector<double> binValues() const;

    /// Returns the depth order
    DepthOrder depthOrder() const;
    /// Sets the depth order
    void setDepthOrder(DepthOrder order);

    // ---- Style ----

    /// Returns the geometry style
    PlotStyle plotStyle() const;
    /// Sets the geometry style
    void setPlotStyle(PlotStyle style);
    /// Returns the color mode
    ColorMode colorMode() const;
    /// Sets the color mode
    void setColorMode(ColorMode mode);

    /// Sets the solid color (Solid mode, and the default outline color)
    void setColor(RGBA color);
    /// Returns the solid color
    RGBA color() const;

    /// Sets the data color functor (takes ownership)
    void setDataColor(Qwt3DColor* color);
    /// Returns the data color functor
    const Qwt3DColor* dataColor() const;
    /// Marks colors stale, rebuilding the VBO on the next draw
    void invalidateColors();

    /// Fixes the color range (disables the automatic range); essential for
    /// stable amplitude colors in a live waterfall
    void setColorRange(double min, double max);
    /// Returns to the automatic color range
    void resetColorRange();
    /// Returns true when a fixed color range is active
    bool hasFixedColorRange() const;

    /// Returns the ribbon baseline (defaults to the current amplitude minimum)
    double baseline() const;
    /// Sets the ribbon baseline
    void setBaseline(double value);
    /// Lets the baseline follow the amplitude minimum again
    void resetBaseline();

    /// Returns the ribbon alpha (0..1, default 1)
    double opacity() const;
    /// Sets the ribbon alpha
    void setOpacity(double alpha);

    /// Returns the outline color (RibbonMesh / Wireframe / Tube outline)
    RGBA outlineColor() const;
    /// Sets the outline color
    void setOutlineColor(RGBA color);
    /// Returns the outline line width
    double outlineWidth() const;
    /// Sets the outline line width (Lines-based styles only)
    void setOutlineWidth(double width);

    // ---- Qwt3DPlotItem interface ----

    int rtti() const override;
    void draw() override;
    ParallelEpiped hull() const override;
    void populateLegendColors(ColorVector& colors) const override;

private:
    void buildVBO();
    void buildOutlineVBO();
    void buildTubeVBO();
    void pushColorRange() const;
    void ensureDataColor();
};

#endif // QWT3D_WATERFALL_H
