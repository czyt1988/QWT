#include "qwt3d_drawable.h"

#include "qwt3d_plot.h"

#include <algorithm>
#include <list>


class Qwt3DDrawable::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DDrawable)

public:
    PrivateData(Qwt3DDrawable* q) : q_ptr(q)
    {
    }

    std::list< Qwt3DDrawable* > m_dlist;
};

Qwt3DDrawable::Qwt3DDrawable() : QWT_PIMPL_CONSTRUCT
{
}

Qwt3DDrawable::Qwt3DDrawable(Qwt3DDrawable&& other) noexcept
    : m_data(std::move(other.m_data)), color(other.color), m_plot(other.m_plot)
{
}

Qwt3DDrawable& Qwt3DDrawable::operator=(Qwt3DDrawable&& other) noexcept
{
    if (this != &other) {
        m_data = std::move(other.m_data);
        color = other.color;
        m_plot = other.m_plot;
    }
    return *this;
}

Qwt3DDrawable::~Qwt3DDrawable()
{
    detachAll();
}

void Qwt3DDrawable::attach(Qwt3DDrawable* dr)
{
    QWT_D(d);

    if (d->m_dlist.end() == std::find(d->m_dlist.begin(), d->m_dlist.end(), dr))
        if (dr) {
            d->m_dlist.push_back(dr);
        }
}

void Qwt3DDrawable::detach(Qwt3DDrawable* dr)
{
    QWT_D(d);

    std::list< Qwt3DDrawable* >::iterator it = std::find(d->m_dlist.begin(), d->m_dlist.end(), dr);

    if (it != d->m_dlist.end()) {
        d->m_dlist.erase(it);
    }
}

void Qwt3DDrawable::detachAll()
{
    QWT_D(d);
    d->m_dlist.clear();
}

/**
 * @brief Returns the owning plot
 * @return Pointer to the Qwt3DPlot this drawable belongs to (may be null)
 */
Qwt3DPlot* Qwt3DDrawable::plot() const
{
    return m_plot;
}

/**
 * @brief Sets the owning plot
 * @param p Pointer to the Qwt3DPlot
 */
void Qwt3DDrawable::setPlot(Qwt3DPlot* p)
{
    m_plot = p;
}

/**
 * @brief Converts a relative viewport position to world coordinates
 * @param rel Relative position in viewport coordinates
 * @return Corresponding world coordinates
 * @details Uses the plot's screenToWorld method for coordinate conversion.
 */
Triple Qwt3DDrawable::relativePosition(Triple rel) const
{
    if (!m_plot)
        return Triple(0, 0, 0);

    QSize vp = m_plot->viewportSize();
    if (vp.width() <= 0 || vp.height() <= 0)
        return Triple(0, 0, 0);

    QPointF screen(rel.x * vp.width(), rel.y * vp.height());
    return m_plot->screenToWorld(screen);
}

void Qwt3DDrawable::draw()
{
    QWT_D(d);

    for (auto* drawable : d->m_dlist) {
        drawable->draw();
    }
}

void Qwt3DDrawable::setColor(double r, double g, double b, double a)
{
    color = RGBA(r, g, b, a);
}

void Qwt3DDrawable::setColor(RGBA rgba)
{
    color = rgba;
}
