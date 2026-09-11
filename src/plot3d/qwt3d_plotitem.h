#ifndef QWT3D_PLOTITEM_H
#define QWT3D_PLOTITEM_H

#include "qwt3d_global.h"

#include <QString>

#include "qwt3d_types.h"

class Qwt3DPlot;

/**
 * @brief Base class for all 3D plot items
 *
 * @details A Qwt3DPlotItem is "something", that can be drawn within the
 *          GL context of a Qwt3DPlot widget. Items are not widgets themselves;
 *          they are pure data + drawing logic objects that are attached to
 *          a Qwt3DPlot via attach() and rendered during Qwt3DPlot::paintGL().
 *
 * The rendering pipeline calls draw() for each attached item in order of
 * ascending z-value. Items manage their own data, geometry caching
 * (VBO/VAO), and color/style. The view transformation, projection, and
 * lighting are set up by Qwt3DPlot before draw() is called.
 *
 * To implement a custom 3D plot item, derive from Qwt3DPlotItem and
 * implement the pure virtual draw() and hull() methods:
 *
 * @code
 * class MyItem : public Qwt3DPlotItem
 * {
 * public:
 *     void draw() override
 *     {
 *         // Issue GL drawing calls using the current GL context
 *     }
 *
 *     ParallelEpiped hull() const override
 *     {
 *         return m_hull;
 *     }
 *
 * private:
 *     ParallelEpiped m_hull;
 * };
 *
 * // Usage:
 * auto plot = new Qwt3DPlot(parent);
 * auto item = new MyItem();
 * item->attach(plot);
 * @endcode
 *
 * @sa Qwt3DPlot, QwtPlotItem (2D counterpart)
 */
class QWT3D_EXPORT Qwt3DPlotItem
{
    QWT_DECLARE_PRIVATE(Qwt3DPlotItem)

public:
    /// Default constructor
    Qwt3DPlotItem();
    /// Destructor — detaches from any plot
    virtual ~Qwt3DPlotItem();

    /// Attach the item to a plot
    void attach(Qwt3DPlot* plot);
    /// Detach the item from its plot
    void detach();

    /// Get the plot this item is attached to
    Qwt3DPlot* plot() const;

    /// Draw the item using the current GL context
    virtual void draw() = 0;
    /// Get the bounding hull of the item in 3D space
    virtual ParallelEpiped hull() const = 0;

    /// Set the item title
    void setTitle(const QString& title);
    /// Get the item title
    QString title() const;

    /// Set the z-order value (controls draw sequence)
    void setZ(double z);
    /// Get the z-order value
    double z() const;

    /// Set item visibility
    void setVisible(bool on);
    /// Check if the item is visible
    bool isVisible() const;

    /// Runtime type information
    virtual int rtti() const;

    /// Notify the plot that the item has changed
    virtual void itemChanged();

    /// Populate the legend color vector from the item's color functor
    virtual void populateLegendColors(ColorVector& colors) const;

private:
    Qwt3DPlotItem(const Qwt3DPlotItem&) = delete;
    Qwt3DPlotItem& operator=(const Qwt3DPlotItem&) = delete;
};

#endif // QWT3D_PLOTITEM_H
