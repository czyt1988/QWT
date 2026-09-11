#include "qwt3d_color.h"
#include "qwt_colormap.h"
#include "qwt_colormap_preset.h"

#include <qcolor.h>
#include <qstring.h>


class Qwt3DStandardColor::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DStandardColor)

public:
    PrivateData(Qwt3DStandardColor* q) : q_ptr(q)
    {
    }

    ColorVector m_colors;
    QString m_presetName;
    double m_alpha = 1.0;
};

/**
 * @brief Constructs a Qwt3DStandardColor object
 * @param size Number of color entries in the color vector
 * @details Creates a standard color mapping with the specified size and resets
 *          the color vector to default gradient values. The z-range used for
 *          color normalization is pushed in by the owning Qwt3DPlotItem via
 *          setActiveRange() (defaults to [0, 1] until set).
 */
Qwt3DStandardColor::Qwt3DStandardColor(unsigned size) : QWT_PIMPL_CONSTRUCT
{
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
 * @details This is a silent mutation: the owning Qwt3DPlotItem is not notified.
 *          Call Qwt3DSurface::invalidateColors() afterwards to trigger a VBO
 *          rebuild.
 */
void Qwt3DStandardColor::setColorVector(ColorVector const& cv)
{
    QWT_D(d);
    d->m_presetName.clear();
    d->m_colors = cv;
}

/**
 * @brief Sets the alpha value for all colors
 * @param a Alpha value (0.0 to 1.0)
 * @details Silent mutation: call Qwt3DSurface::invalidateColors() to rebuild.
 */
void Qwt3DStandardColor::setAlpha(double a)
{
    QWT_D(d);
    if (a < 0 || a > 1)
        return;

    d->m_alpha = a;

    RGBA elem;

    for (unsigned int i = 0; i != d->m_colors.size(); ++i) {
        elem             = d->m_colors[ i ];
        elem.a           = a;
        d->m_colors[ i ] = elem;
    }
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
 * @details Maps the z value to a color index based on the active z-range
 *          pushed in by the owning Qwt3DPlotItem via setActiveRange().
 */
RGBA Qwt3DStandardColor::operator()(double, double, double z) const
{
    QWT_DC(d);
    const double zMin = activeZMin();
    const double zMax = activeZMax();
    int index = static_cast< int >((d->m_colors.size() - 1) * (z - zMin) / (zMax - zMin));
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

    d->m_presetName = presetName;

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
}

/**
 * @brief 返回当前 preset 名称
 * @return preset 名称字符串，如果通过 setColorVector() 设置则返回空字符串
 */
QString Qwt3DStandardColor::presetName() const
{
    QWT_DC(d);
    return d->m_presetName;
}

/**
 * @brief 返回颜色向量中的颜色数量
 * @return 颜色数量
 */
unsigned Qwt3DStandardColor::colorCount() const
{
    QWT_DC(d);
    return static_cast< unsigned >(d->m_colors.size());
}

/**
 * @brief 返回上次 setAlpha() 设置的 alpha 值
 * @return alpha 值（0.0 ~ 1.0），默认 1.0
 */
double Qwt3DStandardColor::alpha() const
{
    QWT_DC(d);
    return d->m_alpha;
}
