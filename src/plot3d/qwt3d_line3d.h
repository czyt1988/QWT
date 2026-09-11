#ifndef QWT3D_LINE3D_H
#define QWT3D_LINE3D_H

#include "qwt3d_plotitem.h"
#include "qwt3d_types.h"
#include "qwt3d_color.h"

#include <QVector>

#include "qwt_point_3d.h"
#include "qwt_series_data.h"

/**
 * @brief 3D line/curve plot item with VBO/VAO rendering
 * @details Qwt3DLine is a Qwt3DPlotItem that renders a polyline through 3D
 *          space using modern OpenGL (VBO/VAO + GLSL shaders). It accepts a
 *          series of QwtPoint3D samples and draws them as a connected curve.
 *
 * Three rendering styles are provided:
 *  - Lines (default): thin GL line strip (1px). Reliable but width is not
 *    adjustable in OpenGL Core profile (glLineWidth > 1 is not guaranteed).
 *  - Tube: the polyline is swept with a circular cross-section to form a lit,
 *    solid tube. This gives true 3D thickness with Blinn-Phong shading and is
 *    the recommended style for trajectories and streamlines where the line
 *    needs to be visible from all angles. Tube geometry is built with
 *    parallel-transport framing to avoid degeneracies of Frenet frames on
 *    straight segments.
 *  - Dots: per-sample point markers with configurable point size and shape
 *    (Dot/Cube/Tetrahedron/Octahedron/Sphere via setPointShape).
 *
 * Colors may be solid (setColor) or driven per-vertex by a Qwt3DColor functor
 * (setDataColor), e.g. to color the curve by position or arc length.
 *
 * @code
 * auto plot = new Qwt3DPlot(parent);
 *
 * auto line = new Qwt3DLine();
 * line->setSamples(samples);           // QVector<QwtPoint3D>
 * line->setLineStyle(Qwt3DLine::Tube);
 * line->setDataColor(new Qwt3DColorMapColor("plasma"));
 * line->attach(plot);
 * @endcode
 *
 * @sa Qwt3DPlotItem, Qwt3DPlot, Qwt3DColor
 */
class QWT3D_EXPORT Qwt3DLine : public Qwt3DPlotItem
{
    QWT_DECLARE_PRIVATE(Qwt3DLine)

public:
    /// Line rendering style
    enum LineStyle
    {
        Lines,  ///< Thin GL line strip (1px)
        Tube,   ///< Solid lit tube swept along the polyline
        Dots    ///< Per-sample point markers
    };

    /// Point marker shape for the Dots style and the point-marker overlay
    enum PointShape
    {
        Dot,         ///< GL_POINTS (simple screen-space dots)
        Cube,        ///< Axis-aligned cube
        Tetrahedron, ///< Regular 4-faced solid
        Octahedron,  ///< 8-faced diamond solid
        Sphere       ///< UV sphere (smooth, lit)
    };

    /// Constructs an empty line item
    Qwt3DLine();
    /// Destructor — releases GL resources and data
    ~Qwt3DLine() override;

    // Data — setSamples mirrors the 2D QwtPlotCurve API; data is stored as
    // a QwtSeriesData<QwtPoint3D> owned by the item.

    /// Set samples from a vector of 3D points
    void setSamples(const QVector<QwtPoint3D>& samples);
    /// Set samples from parallel x, y, z arrays
    void setSamples(const QVector<double>& x, const QVector<double>& y, const QVector<double>& z);
    /// Set samples from a raw array of 3D points (not held; copied internally)
    void setSamples(const QwtPoint3D* samples, size_t count);
    /// Set the series data object (item takes ownership)
    void setSamples(QwtSeriesData<QwtPoint3D>* data);
    /// Returns the series data
    const QwtSeriesData<QwtPoint3D>* data() const;
    /// Returns the number of samples
    size_t dataSize() const;

    // Style

    /// Returns the line style
    LineStyle lineStyle() const;
    /// Sets the line style
    void setLineStyle(LineStyle style);
    /// Returns the GL line width (Lines style only; > 1 not guaranteed in Core)
    double lineWidth() const;
    /// Sets the GL line width (Lines style only)
    void setLineWidth(double width);
    /// Returns the tube radius (<= 0 means auto = 0.5% of hull diagonal)
    double tubeRadius() const;
    /// Sets the tube radius (<= 0 selects auto)
    void setTubeRadius(double radius);
    /// Returns the tube ring segment count
    int tubeSegments() const;
    /// Sets the tube ring segment count (minimum 3)
    void setTubeSegments(int segments);
    /// Returns the point size (Dots style / point markers)
    double pointSize() const;
    /// Sets the point size (Dots style / point markers)
    void setPointSize(double size);
    /// Returns true if point markers are drawn on top of Lines/Tube styles
    bool pointVisible() const;
    /// Draw point markers on top of Lines/Tube styles
    void setPointVisible(bool on);

    /// Returns the point marker shape
    PointShape pointShape() const;
    /// Sets the point marker shape (Dot/Cube/Tetrahedron/Octahedron/Sphere)
    void setPointShape(PointShape shape);

    // Color

    /// Sets the solid line color
    void setColor(RGBA color);
    /// Returns the solid line color
    RGBA color() const;
    /// Sets the data color functor for per-vertex coloring (takes ownership)
    void setDataColor(Qwt3DColor* color);
    /// Returns the data color functor
    const Qwt3DColor* dataColor() const;
    /// Marks per-vertex colors as stale, triggering a VBO rebuild on the next draw
    void invalidateColors();

    // Qwt3DPlotItem interface

    int rtti() const override;
    void draw() override;
    ParallelEpiped hull() const override;
    void populateLegendColors(ColorVector& colors) const override;

private:
    void buildVBO();
    void buildPointsVBO();
    void buildMarkersVBO();
    void pushColorRange() const;
};

#endif // QWT3D_LINE3D_H
