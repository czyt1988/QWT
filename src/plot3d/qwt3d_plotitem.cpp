#include "qwt3d_plotitem.h"
#include "qwt3d_plot.h"

class Qwt3DPlotItem::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DPlotItem)

public:
    explicit PrivateData(Qwt3DPlotItem* p);

    Qwt3DPlot* m_plot = nullptr;
    double m_z = 0.0;
    QString m_title;
    bool m_visible = true;
};

Qwt3DPlotItem::PrivateData::PrivateData(Qwt3DPlotItem* p)
    : q_ptr(p)
{
}

/**
 * @brief Default constructor
 * @details Constructs a Qwt3DPlotItem with default settings.
 */
Qwt3DPlotItem::Qwt3DPlotItem()
    : QWT_PIMPL_CONSTRUCT
{
}

/**
 * @brief Destructor
 * @details Destroys the Qwt3DPlotItem and detaches it from any plot.
 */
Qwt3DPlotItem::~Qwt3DPlotItem()
{
    detach();
}

/**
 * @brief Attach the item to a plot
 * @param[in] plot Plot widget to attach to
 * @details This method attaches a Qwt3DPlotItem to the Qwt3DPlot argument.
 *          It will first detach the Qwt3DPlotItem from any plot from a previous
 *          call to attach (if necessary). If a nullptr argument is passed,
 *          it will detach from any Qwt3DPlot it was attached to.
 */
void Qwt3DPlotItem::attach(Qwt3DPlot* plot)
{
    QWT_D(d);

    if (plot == d->m_plot)
        return;

    if (d->m_plot)
        d->m_plot->detach(this);

    d->m_plot = plot;

    if (d->m_plot)
        d->m_plot->attach(this);
}

/**
 * @brief Detach the item from the plot
 * @details This method detaches a Qwt3DPlotItem from any Qwt3DPlot it has been
 *          associated with. detach() is equivalent to calling attach(nullptr).
 */
void Qwt3DPlotItem::detach()
{
    QWT_D(d);

    if (!d->m_plot)
        return;

    d->m_plot->detach(this);
    d->m_plot = nullptr;
}

/**
 * @brief Get the plot the item is attached to
 * @return Attached plot, or nullptr if not attached
 */
Qwt3DPlot* Qwt3DPlotItem::plot() const
{
    QWT_DC(d);
    return d->m_plot;
}

/**
 * @brief Set the item title
 * @param[in] title Title text
 * @sa title()
 */
void Qwt3DPlotItem::setTitle(const QString& title)
{
    QWT_D(d);

    if (d->m_title != title) {
        d->m_title = title;
        itemChanged();
    }
}

/**
 * @brief Get the title of the item
 * @return Title of the item
 * @sa setTitle()
 */
QString Qwt3DPlotItem::title() const
{
    QWT_DC(d);
    return d->m_title;
}

/**
 * @brief Set the z-order value
 * @param[in] z Z-value for the item
 * @details Plot items are drawn in increasing z-order.
 * @sa z()
 */
void Qwt3DPlotItem::setZ(double z)
{
    QWT_D(d);

    if (d->m_z != z) {
        d->m_z = z;
        itemChanged();
    }
}

/**
 * @brief Get the z-order value
 * @return Z-value of the item
 * @details Plot items are drawn in increasing z-order.
 * @sa setZ()
 */
double Qwt3DPlotItem::z() const
{
    QWT_DC(d);
    return d->m_z;
}

/**
 * @brief Set item visibility
 * @param[in] on Show if true, otherwise hide
 * @sa isVisible()
 */
void Qwt3DPlotItem::setVisible(bool on)
{
    QWT_D(d);

    if (d->m_visible != on) {
        d->m_visible = on;
        itemChanged();
    }
}

/**
 * @brief Check if the item is visible
 * @return true if visible, false otherwise
 * @sa setVisible()
 */
bool Qwt3DPlotItem::isVisible() const
{
    QWT_DC(d);
    return d->m_visible;
}

/**
 * @brief Notify the plot that the item has changed
 * @details Triggers a plot update so the item is redrawn.
 */
void Qwt3DPlotItem::itemChanged()
{
    QWT_D(d);
    if (d->m_plot)
        d->m_plot->itemChanged(this);
}

void Qwt3DPlotItem::populateLegendColors(ColorVector&) const
{
}
