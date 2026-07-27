#include "qwt3d_color.h"
#include "qwt3d_plot.h"
#include "qwt3d_surface.h"
#include "qwt_colormap.h"
#include "qwt_colormap_preset.h"

#include <qcolor.h>
#include <qstring.h>


/**
 * @brief Notifies the owning surface that per-vertex colors are stale
 * @details Called by subclasses after mutating color state (setColorMap,
 *          setAlpha, etc). Triggers m_vboDirty and itemChanged on the surface,
 *          causing buildVBO() to re-run the color functor on the next draw.
 */
void Qwt3DColor::notifyColorChanged()
{
    if (m_surface)
        m_surface->invalidateColors();
}


class Qwt3DStandardColor::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DStandardColor)

public:
    PrivateData(Qwt3DStandardColor* q) : q_ptr(q), m_data(nullptr)
    {
    }

    ColorVector m_colors;
    Qwt3DPlot* m_data;
};

/**
 * @brief Constructs a Qwt3DStandardColor object
 * @param data Plot3D data source for color mapping
 * @param size Number of color entries in the color vector
 * @details Creates a standard color mapping with the specified size and resets
 *          the color vector to default gradient values.
 */
Qwt3DStandardColor::Qwt3DStandardColor(Qwt3DPlot* data, unsigned size) : QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    Q_ASSERT(data);
    d->m_data = data;

    reset(size);
}

Qwt3DStandardColor::~Qwt3DStandardColor() = default;

/**
 * @brief Resets the color vector to the default colormap
 * @param size Number of color entries to generate
 * @details Creates a color vector of the given size sampled from the viridis
 *          colormap, the modern perceptually-uniform default.
 */
void Qwt3DStandardColor::reset(unsigned size)
{
    setPreset(QStringLiteral("viridis"), size);
}

/**
 * @brief Assigns a new ColorVector
 * @param cv The new color vector (also overwrites the constructor's size argument)
 */
void Qwt3DStandardColor::setColorVector(ColorVector const& cv)
{
    QWT_D(d);
    d->m_colors = cv;
    notifyColorChanged();
}

/**
 * @brief Sets the alpha value for all colors
 * @param a Alpha value (0.0 to 1.0)
 */
void Qwt3DStandardColor::setAlpha(double a)
{
    QWT_D(d);
    if (a < 0 || a > 1)
        return;

    RGBA elem;

    for (unsigned int i = 0; i != d->m_colors.size(); ++i) {
        elem             = d->m_colors[ i ];
        elem.a           = a;
        d->m_colors[ i ] = elem;
    }

    notifyColorChanged();
}

/**
 * @brief Creates color vector for Qwt3DColorLegend - essentially a copy from the internal vector
 * @param vec The vector to fill
 * @return Reference to the filled color vector
 */
ColorVector& Qwt3DStandardColor::createVector(ColorVector& vec)
{
    QWT_D(d);
    vec = d->m_colors;
    return vec;
}

/**
 * @brief Returns the color for a given z value
 * @param z The z coordinate value for color lookup
 * @return RGBA color corresponding to the z value
 * @details Maps the z value to a color index based on the data hull's z range.
 */
RGBA Qwt3DStandardColor::operator()(double, double, double z) const
{
    QWT_DC(d);
    Q_ASSERT(d->m_data);
    int index = static_cast< int >((d->m_colors.size() - 1) * (z - d->m_data->hull().minVertex.z)
                                   / (d->m_data->hull().maxVertex.z - d->m_data->hull().minVertex.z));
    if (index < 0)
        index = 0;
    if (static_cast< unsigned int >(index) > d->m_colors.size() - 1)
        index = static_cast< int >(d->m_colors.size() - 1);
    return d->m_colors[ index ];
}

/**
 * @brief Set colormap from a preset name
 * @param presetName Name of the colormap preset (e.g. "viridis", "plasma", "jet")
 * @param size Number of color stops to sample from the colormap
 * @details Uses QwtColorMapPreset to create a QwtLinearColorMap and samples it
 *          at the specified number of points to fill the internal color vector.
 */
void Qwt3DStandardColor::setPreset(const QString& presetName, unsigned size)
{
    QWT_D(d);

    auto colorMap = QwtColorMapPreset::create(presetName);

    d->m_colors.resize(size);
    for (unsigned i = 0; i < size; ++i) {
        const double t     = (size > 1) ? static_cast< double >(i) / (size - 1) : 0.5;
        const QColor color = colorMap->color(0.0, 1.0, t);

        RGBA rgba;
        rgba.r = color.redF();
        rgba.g = color.greenF();
        rgba.b = color.blueF();
        rgba.a = color.alphaF();

        d->m_colors[ i ] = rgba;
    }

    notifyColorChanged();
}
