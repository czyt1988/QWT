#include "qwt3d_enrichment_std.h"

#include <cmath>


/////////////////////////////////////////////////////////////////
//
//   Qwt3DCrossHair
//
/////////////////////////////////////////////////////////////////

class Qwt3DCrossHair::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DCrossHair)

public:
    PrivateData(Qwt3DCrossHair* q)
        : q_ptr(q), m_boxed(false), m_smooth(false), m_linewidth(1.0), m_radius(0.0)
    {
    }

    bool m_boxed;
    bool m_smooth;
    double m_linewidth;
    double m_radius;
};

Qwt3DCrossHair::Qwt3DCrossHair() : QWT_PIMPL_CONSTRUCT
{
    configure(0, 1, false, false);
}

Qwt3DCrossHair::Qwt3DCrossHair(double rad, double linewidth, bool smooth, bool boxed) : QWT_PIMPL_CONSTRUCT
{
    configure(rad, linewidth, smooth, boxed);
}

Qwt3DCrossHair::Qwt3DCrossHair(const Qwt3DCrossHair& other) : Qwt3DVertexEnrichment(other), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    const PrivateData* od = other.d_func();
    d->m_boxed            = od->m_boxed;
    d->m_smooth           = od->m_smooth;
    d->m_linewidth        = od->m_linewidth;
    d->m_radius           = od->m_radius;
}

Qwt3DCrossHair::~Qwt3DCrossHair() = default;

Qwt3DEnrichment* Qwt3DCrossHair::clone() const
{
    return new Qwt3DCrossHair(*this);
}

void Qwt3DCrossHair::configure(double rad, double linewidth, bool smooth, bool boxed)
{
    QWT_D(d);
    d->m_radius    = rad;
    d->m_linewidth = linewidth;
    d->m_smooth    = smooth;
    d->m_boxed     = boxed;
}

// Stubs — full implementations disabled during Plot+Item refactor.
// Will be reimplemented when enrichment system works with Qwt3DPlotItem.
void Qwt3DCrossHair::drawBegin() {}
void Qwt3DCrossHair::drawEnd() {}
void Qwt3DCrossHair::draw(Triple const&) {}

/////////////////////////////////////////////////////////////////
//
//   Qwt3DDot
//
/////////////////////////////////////////////////////////////////

class Qwt3DDot::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DDot)

public:
    PrivateData(Qwt3DDot* q) : q_ptr(q), m_smooth(false), m_pointsize(1.0)
    {
    }

    bool m_smooth;
    double m_pointsize;
};

Qwt3DDot::Qwt3DDot() : QWT_PIMPL_CONSTRUCT
{
    configure(1, false);
}

Qwt3DDot::Qwt3DDot(double pointsize, bool smooth) : QWT_PIMPL_CONSTRUCT
{
    configure(pointsize, smooth);
}

Qwt3DDot::Qwt3DDot(const Qwt3DDot& other) : Qwt3DVertexEnrichment(other), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    const PrivateData* od = other.d_func();
    d->m_smooth           = od->m_smooth;
    d->m_pointsize        = od->m_pointsize;
}

Qwt3DDot::~Qwt3DDot() = default;

Qwt3DEnrichment* Qwt3DDot::clone() const
{
    return new Qwt3DDot(*this);
}

void Qwt3DDot::configure(double pointsize, bool smooth)
{
    QWT_D(d);
    d->m_pointsize = pointsize;
    d->m_smooth    = smooth;
}

// Stubs — disabled during refactor
void Qwt3DDot::drawBegin() {}
void Qwt3DDot::drawEnd() {}
void Qwt3DDot::draw(Triple const&) {}

/////////////////////////////////////////////////////////////////
//
//   Qwt3DCone
//
/////////////////////////////////////////////////////////////////

class Qwt3DCone::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DCone)

public:
    PrivateData(Qwt3DCone* q) : q_ptr(q), m_quality(3), m_radius(0.0)
    {
    }

    unsigned m_quality;
    double m_radius;
};

Qwt3DCone::Qwt3DCone() : QWT_PIMPL_CONSTRUCT
{
    configure(0, 3);
}

Qwt3DCone::Qwt3DCone(double rad, unsigned quality) : QWT_PIMPL_CONSTRUCT
{
    configure(rad, quality);
}

Qwt3DCone::Qwt3DCone(const Qwt3DCone& other) : Qwt3DVertexEnrichment(other), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    const PrivateData* od = other.d_func();
    d->m_quality          = od->m_quality;
    d->m_radius           = od->m_radius;
}

Qwt3DCone::~Qwt3DCone() = default;

Qwt3DEnrichment* Qwt3DCone::clone() const
{
    return new Qwt3DCone(*this);
}

void Qwt3DCone::configure(double rad, unsigned quality)
{
    QWT_D(d);
    d->m_radius   = rad;
    d->m_quality  = quality;
}

// Stub — disabled during refactor.
// When implemented: CPU-generate cone triangle mesh, upload to VBO,
// render with polygon shader.
void Qwt3DCone::draw(Triple const&) {}

/////////////////////////////////////////////////////////////////
//
//   Qwt3DArrow
//
/////////////////////////////////////////////////////////////////

class Qwt3DArrow::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DArrow)

public:
    PrivateData(Qwt3DArrow* q)
        : q_ptr(q)
        , m_segments(3)
        , m_relConeLength(0.4)
        , m_relConeRadius(0.06)
        , m_relStemRadius(0.02)
    {
    }

    int m_segments;
    double m_relConeLength;
    double m_relConeRadius;
    double m_relStemRadius;

    Triple m_top;
    RGBA m_rgba;
};

Qwt3DArrow::Qwt3DArrow() : QWT_PIMPL_CONSTRUCT
{
    configure(3, 0.4, 0.06, 0.02);
}

Qwt3DArrow::Qwt3DArrow(const Qwt3DArrow& other) : Qwt3DVertexEnrichment(other), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    const PrivateData* od = other.d_func();
    d->m_segments         = od->m_segments;
    d->m_relConeLength    = od->m_relConeLength;
    d->m_relConeRadius    = od->m_relConeRadius;
    d->m_relStemRadius    = od->m_relStemRadius;
    d->m_top              = od->m_top;
    d->m_rgba             = od->m_rgba;
}

Qwt3DArrow::~Qwt3DArrow() = default;

Qwt3DEnrichment* Qwt3DArrow::clone() const
{
    return new Qwt3DArrow(*this);
}

void Qwt3DArrow::configure(int segs, double relconelength, double relconerad, double relstemrad)
{
    QWT_D(d);
    d->m_segments      = segs;
    d->m_relConeLength = relconelength;
    d->m_relConeRadius = relconerad;
    d->m_relStemRadius = relstemrad;
}

void Qwt3DArrow::setQuality(int val)
{
    QWT_D(d);
    d->m_segments = val;
}

void Qwt3DArrow::setTop(Triple t)
{
    QWT_D(d);
    d->m_top = t;
}

void Qwt3DArrow::setColor(RGBA rgba)
{
    QWT_D(d);
    d->m_rgba = rgba;
}

// Stub — disabled during refactor.
// When implemented: CPU-generate cone + cylinder triangle mesh,
// upload to VBO, render with polygon shader.
void Qwt3DArrow::draw(Triple const&) {}

double Qwt3DArrow::calcRotation(Triple& axis, FreeVector const& vec)
{
    // Stub implementation — returns 0 rotation during refactor
    (void)axis;
    (void)vec;
    return 0.0;
}
