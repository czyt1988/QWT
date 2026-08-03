#ifndef QWT3D_BAR_H
#define QWT3D_BAR_H

#include "qwt3d_plotitem.h"
#include "qwt3d_types.h"
#include "qwt3d_color.h"

#include <QVector>

#include "qwt_point_3d.h"

/**
 * @brief 3D bar chart item with VBO/VAO rendering
 * @details Qwt3DBar is a Qwt3DPlotItem that renders 3D bar chart data using
 *          modern OpenGL (VBO/VAO + GLSL shaders). Each sample becomes an
 *          axis-aligned cuboid ("bar") whose height encodes the scalar value.
 *
 * Two data shapes are supported:
 *  - A 1D series of bars placed freely on the xy-plane
 *    (setSamples with QwtPoint3D, where (x,y) is the footprint center and
 *    z is the bar height).
 *  - A 2D grid of bars (bar3 / 3D histogram) sampled over a rectangular
 *    x/y domain, where z[i][j] is the height at cell (i,j).
 *
 * The item manages its own bar specifications, color mapping, and GL
 * resources (VBO, VAO, EBO, shader program). Geometry is uploaded to GPU
 * memory when dirty and reused across frames until the data changes.
 *
 * Bars reuse the lit surface shader (Blinn-Phong) with flat per-face
 * normals, so they respond to Qwt3DPlot::enableLighting(). An optional
 * edge mesh is drawn in a second pass for the FilledMesh / Wireframe
 * styles.
 *
 * @code
 * auto plot = new Qwt3DPlot(parent);
 *
 * auto bars = new Qwt3DBar();
 * bars->setSamples(zMatrix, columns, rows, minX, maxX, minY, maxY);
 * bars->setBarStyle(Qwt3DBar::FilledMesh);
 * bars->setDataColor(new Qwt3DColorMapColor("viridis"));
 * bars->attach(plot);
 * @endcode
 *
 * @sa Qwt3DPlotItem, Qwt3DPlot, Qwt3DColor
 */
class QWT3D_EXPORT Qwt3DBar : public Qwt3DPlotItem
{
    QWT_DECLARE_PRIVATE(Qwt3DBar)

public:
    /// Bar rendering style
    enum BarStyle
    {
        Filled,      ///< Filled bars, no edges
        FilledMesh,  ///< Filled bars with separately colored edge lines
        Wireframe    ///< Edge lines only
    };

    /// Constructs an empty bar chart item
    Qwt3DBar();
    /// Destructor — releases GL resources and data
    ~Qwt3DBar() override;

    // 1D series — bars placed freely on the xy-plane

    /// Set bar positions and heights from 3D points (x,y = footprint center, z = height)
    void setSamples(const QVector<QwtPoint3D>& samples);
    /// Set bar positions (x) and heights along the y = 0 row
    void setSamples(const QVector<double>& x, const QVector<double>& heights);

    // 2D grid — 3D histogram (bar3 style)

    /// Set a grid of bars from a z-value matrix with an explicit x/y domain
    void setSamples(double** z, int columns, int rows,
                    double minX, double maxX, double minY, double maxY);
    /// Set a grid of bars from a Qwt3DFunctionData result (modern vector API)
    void setSamples(const Qwt3DFunctionData& data);

    // Bar footprint and baseline

    /// Returns the bar footprint width (<= 0 means auto)
    double barWidth() const;
    /// Sets the bar footprint width (<= 0 selects auto = 80% of spacing)
    void setBarWidth(double w);
    /// Returns the bar footprint depth (<= 0 means auto)
    double barDepth() const;
    /// Sets the bar footprint depth (<= 0 selects auto = 80% of spacing)
    void setBarDepth(double d);
    /// Returns the baseline z value (bottom of positive bars)
    double baseline() const;
    /// Sets the baseline z value
    void setBaseline(double z);

    // Style and color

    /// Returns the bar rendering style
    BarStyle barStyle() const;
    /// Sets the bar rendering style
    void setBarStyle(BarStyle style);
    /// Sets the data color functor (takes ownership)
    void setDataColor(Qwt3DColor* color);
    /// Returns the data color functor
    const Qwt3DColor* dataColor() const;
    /// Marks per-vertex colors as stale, triggering a VBO rebuild on the next draw
    void invalidateColors();
    /// Returns the mesh line color
    RGBA meshColor() const;
    /// Sets the mesh line color
    void setMeshColor(RGBA color);
    /// Returns the mesh line width
    double meshLineWidth() const;
    /// Sets the mesh line width
    void setMeshLineWidth(double width);

    // Qwt3DPlotItem interface

    void draw() override;
    ParallelEpiped hull() const override;
    void populateLegendColors(ColorVector& colors) const override;

private:
    void buildVBO();
    /// Recomputes bar specs and hull from current m_barWidth / m_barDepth / m_baseline
    void recomputeBarSpecs();
    /// Pushes the plot's union z-range into the color functor (silent data input, no rebuild trigger)
    void pushColorRange() const;
};

#endif // QWT3D_BAR_H
