#include "qwt3d_drawable.h"

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
    : m_data(std::move(other.m_data)), color(other.color)
{
}

Qwt3DDrawable& Qwt3DDrawable::operator=(Qwt3DDrawable&& other) noexcept
{
    if (this != &other) {
        m_data = std::move(other.m_data);
        color = other.color;
    }
    return *this;
}

Qwt3DDrawable::~Qwt3DDrawable()
{
    detachAll();
}

void Qwt3DDrawable::attach(Qwt3DDrawable* dr)
{
    // A moved-from drawable has a null m_data (its PIMPL was moved out by the
    // move constructor/assignment). All m_data access must tolerate this empty,
    // moved-from state — otherwise destroying the moved-from elements left
    // behind by std::vector reallocation (e.g. growing std::vector<Qwt3DLabel>
    // in Qwt3DAxis::drawTics) dereferences null and crashes.
    if (!m_data || !dr)
        return;

    QWT_D(d);

    if (d->m_dlist.end() == std::find(d->m_dlist.begin(), d->m_dlist.end(), dr))
        d->m_dlist.push_back(dr);
}

void Qwt3DDrawable::detach(Qwt3DDrawable* dr)
{
    if (!m_data)
        return;

    QWT_D(d);

    std::list< Qwt3DDrawable* >::iterator it = std::find(d->m_dlist.begin(), d->m_dlist.end(), dr);

    if (it != d->m_dlist.end()) {
        d->m_dlist.erase(it);
    }
}

void Qwt3DDrawable::detachAll()
{
    if (!m_data)
        return;

    QWT_D(d);
    d->m_dlist.clear();
}

void Qwt3DDrawable::draw(const Qwt3DRenderContext& ctx)
{
    QWT_D(d);

    for (auto* drawable : d->m_dlist) {
        drawable->draw(ctx);
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
