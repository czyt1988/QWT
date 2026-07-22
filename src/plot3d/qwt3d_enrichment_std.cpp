#include "qwt3d_enrichment_std.h"

#include <cmath>

#include "qwt3d_color.h"
#include "qwt3d_plot.h"


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
        : q_ptr(q), m_boxed(false), m_smooth(false), m_linewidth(1.0), m_radius(0.0), m_oldstate(GL_FALSE)
    {
    }

    bool m_boxed;
    bool m_smooth;
    double m_linewidth;
    double m_radius;
    GLboolean m_oldstate;
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
    d->m_oldstate         = od->m_oldstate;
}

Qwt3DCrossHair::~Qwt3DCrossHair() = default;

Qwt3DEnrichment* Qwt3DCrossHair::clone() const
{
    return new Qwt3DCrossHair(*this);
}

void Qwt3DCrossHair::configure(double rad, double linewidth, bool smooth, bool boxed)
{
    plot = nullptr;
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
    PrivateData(Qwt3DDot* q) : q_ptr(q), m_smooth(false), m_pointsize(1.0), m_oldstate(GL_FALSE)
    {
    }

    bool m_smooth;
    double m_pointsize;
    GLboolean m_oldstate;
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
    d->m_oldstate         = od->m_oldstate;
}

Qwt3DDot::~Qwt3DDot() = default;

Qwt3DEnrichment* Qwt3DDot::clone() const
{
    return new Qwt3DDot(*this);
}

void Qwt3DDot::configure(double pointsize, bool smooth)
{
    plot = nullptr;
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
    PrivateData(Qwt3DCone* q) : q_ptr(q), m_hat(nullptr), m_disk(nullptr), m_quality(3), m_radius(0.0), m_oldstate(GL_FALSE)
    {
    }

    ~PrivateData()
    {
        if (m_hat)
            gluDeleteQuadric(m_hat);
        if (m_disk)
            gluDeleteQuadric(m_disk);
    }

    void initQuadrics()
    {
        m_hat  = gluNewQuadric();
        m_disk = gluNewQuadric();

        gluQuadricDrawStyle(m_hat, GLU_FILL);
        gluQuadricNormals(m_hat, GLU_SMOOTH);
        gluQuadricOrientation(m_hat, GLU_OUTSIDE);
        gluQuadricDrawStyle(m_disk, GLU_FILL);
        gluQuadricNormals(m_disk, GLU_SMOOTH);
        gluQuadricOrientation(m_disk, GLU_OUTSIDE);
    }

    GLUquadricObj* m_hat;
    GLUquadricObj* m_disk;
    unsigned m_quality;
    double m_radius;
    GLboolean m_oldstate;
};

Qwt3DCone::Qwt3DCone() : QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    d->initQuadrics();
    configure(0, 3);
}

Qwt3DCone::Qwt3DCone(double rad, unsigned quality) : QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    d->initQuadrics();
    configure(rad, quality);
}

Qwt3DCone::Qwt3DCone(const Qwt3DCone& other) : Qwt3DVertexEnrichment(other), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    const PrivateData* od = other.d_func();
    d->m_quality          = od->m_quality;
    d->m_radius           = od->m_radius;
    d->m_oldstate         = od->m_oldstate;
    d->initQuadrics();
}

Qwt3DCone::~Qwt3DCone() = default;

Qwt3DEnrichment* Qwt3DCone::clone() const
{
    return new Qwt3DCone(*this);
}

void Qwt3DCone::configure(double rad, unsigned quality)
{
    plot = nullptr;
    QWT_D(d);
    d->m_radius   = rad;
    d->m_quality  = quality;
    d->m_oldstate = GL_FALSE;
}

// Stub — disabled during refactor
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
        , m_hat(nullptr)
        , m_disk(nullptr)
        , m_base(nullptr)
        , m_bottom(nullptr)
        , m_oldstate(GL_FALSE)
        , m_segments(3)
        , m_relConeLength(0.4)
        , m_relConeRadius(0.06)
        , m_relStemRadius(0.02)
    {
    }

    ~PrivateData()
    {
        if (m_hat)
            gluDeleteQuadric(m_hat);
        if (m_disk)
            gluDeleteQuadric(m_disk);
        if (m_base)
            gluDeleteQuadric(m_base);
        if (m_bottom)
            gluDeleteQuadric(m_bottom);
    }

    void initQuadrics()
    {
        m_hat    = gluNewQuadric();
        m_disk   = gluNewQuadric();
        m_base   = gluNewQuadric();
        m_bottom = gluNewQuadric();

        gluQuadricDrawStyle(m_hat, GLU_FILL);
        gluQuadricNormals(m_hat, GLU_SMOOTH);
        gluQuadricOrientation(m_hat, GLU_OUTSIDE);
        gluQuadricDrawStyle(m_disk, GLU_FILL);
        gluQuadricNormals(m_disk, GLU_SMOOTH);
        gluQuadricOrientation(m_disk, GLU_OUTSIDE);
        gluQuadricDrawStyle(m_base, GLU_FILL);
        gluQuadricNormals(m_base, GLU_SMOOTH);
        gluQuadricOrientation(m_base, GLU_OUTSIDE);
        gluQuadricDrawStyle(m_bottom, GLU_FILL);
        gluQuadricNormals(m_bottom, GLU_SMOOTH);
        gluQuadricOrientation(m_bottom, GLU_OUTSIDE);
    }

    GLUquadricObj* m_hat;
    GLUquadricObj* m_disk;
    GLUquadricObj* m_base;
    GLUquadricObj* m_bottom;
    GLboolean m_oldstate;

    int m_segments;
    double m_relConeLength;
    double m_relConeRadius;
    double m_relStemRadius;

    Triple m_top;
    RGBA m_rgba;
};

Qwt3DArrow::Qwt3DArrow() : QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    d->initQuadrics();
    configure(3, 0.4, 0.06, 0.02);
}

Qwt3DArrow::Qwt3DArrow(const Qwt3DArrow& other) : Qwt3DVertexEnrichment(other), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    const PrivateData* od = other.d_func();
    d->m_oldstate         = od->m_oldstate;
    d->m_segments         = od->m_segments;
    d->m_relConeLength    = od->m_relConeLength;
    d->m_relConeRadius    = od->m_relConeRadius;
    d->m_relStemRadius    = od->m_relStemRadius;
    d->m_top              = od->m_top;
    d->m_rgba             = od->m_rgba;
    d->initQuadrics();
}

Qwt3DArrow::~Qwt3DArrow() = default;

Qwt3DEnrichment* Qwt3DArrow::clone() const
{
    return new Qwt3DArrow(*this);
}

void Qwt3DArrow::configure(int segs, double relconelength, double relconerad, double relstemrad)
{
    plot = nullptr;
    QWT_D(d);
    d->m_segments      = segs;
    d->m_oldstate      = GL_FALSE;
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

// Stub — disabled during refactor
void Qwt3DArrow::draw(Triple const&) {}

double Qwt3DArrow::calcRotation(Triple& axis, FreeVector const& vec)
{
    // Stub implementation — returns 0 rotation during refactor
    (void)axis;
    (void)vec;
    return 0.0;
}
