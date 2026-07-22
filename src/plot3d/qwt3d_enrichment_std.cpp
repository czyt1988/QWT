#include "qwt3d_enrichment_std.h"

#include <cmath>

#include "qwt3d_color.h"
#include "qwt3d_plot.h"


/////////////////////////////////////////////////////////////////
//
//   CrossHair
//
/////////////////////////////////////////////////////////////////

class CrossHair::PrivateData
{
    QWT_DECLARE_PUBLIC(CrossHair)

public:
    PrivateData(CrossHair* q)
        : q_ptr(q), m_boxed(false), m_smooth(false), m_linewidth(1.0), m_radius(0.0), m_oldstate(GL_FALSE)
    {
    }

    bool m_boxed;
    bool m_smooth;
    double m_linewidth;
    double m_radius;
    GLboolean m_oldstate;
};

/**
 * @brief Default constructor
 */
CrossHair::CrossHair() : QWT_PIMPL_CONSTRUCT
{
    configure(0, 1, false, false);
}

/**
 * @brief Constructs a CrossHair with specified parameters
 * @param rad Relative radius
 * @param linewidth Line width
 * @param smooth Smooth lines
 * @param boxed Draw a box around the crosshair
 */
CrossHair::CrossHair(double rad, double linewidth, bool smooth, bool boxed) : QWT_PIMPL_CONSTRUCT
{
    configure(rad, linewidth, smooth, boxed);
}

CrossHair::CrossHair(const CrossHair& other) : VertexEnrichment(other), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    const PrivateData* od = other.d_func();
    d->m_boxed            = od->m_boxed;
    d->m_smooth           = od->m_smooth;
    d->m_linewidth        = od->m_linewidth;
    d->m_radius           = od->m_radius;
    d->m_oldstate         = od->m_oldstate;
}

CrossHair::~CrossHair() = default;

Enrichment* CrossHair::clone() const
{
    return new CrossHair(*this);
}

void CrossHair::configure(double rad, double linewidth, bool smooth, bool boxed)
{
    plot = nullptr;
    QWT_D(d);
    d->m_radius    = rad;
    d->m_linewidth = linewidth;
    d->m_smooth    = smooth;
    d->m_boxed     = boxed;
}

void CrossHair::drawBegin()
{
    QWT_D(d);
    setDeviceLineWidth(d->m_linewidth);
    d->m_oldstate = glIsEnabled(GL_LINE_SMOOTH);
    if (d->m_smooth)
        glEnable(GL_LINE_SMOOTH);
    else
        glDisable(GL_LINE_SMOOTH);
    glBegin(GL_LINES);
}

void CrossHair::drawEnd()
{
    QWT_D(d);
    glEnd();

    if (d->m_oldstate)
        glEnable(GL_LINE_SMOOTH);
    else
        glDisable(GL_LINE_SMOOTH);
}

void CrossHair::draw(Triple const& pos)
{
    QWT_D(d);
    RGBA rgba = (*plot->dataColor())(pos);
    glColor4d(rgba.r, rgba.g, rgba.b, rgba.a);

    double diag = (plot->hull().maxVertex - plot->hull().minVertex).length() * d->m_radius;

    glVertex3d(pos.x - diag, pos.y, pos.z);
    glVertex3d(pos.x + diag, pos.y, pos.z);

    glVertex3d(pos.x, pos.y - diag, pos.z);
    glVertex3d(pos.x, pos.y + diag, pos.z);

    glVertex3d(pos.x, pos.y, pos.z - diag);
    glVertex3d(pos.x, pos.y, pos.z + diag);

    // hull

    if (!d->m_boxed)
        return;

    glVertex3d(pos.x - diag, pos.y - diag, pos.z + diag);
    glVertex3d(pos.x + diag, pos.y - diag, pos.z + diag);
    glVertex3d(pos.x - diag, pos.y - diag, pos.z - diag);
    glVertex3d(pos.x + diag, pos.y - diag, pos.z - diag);

    glVertex3d(pos.x - diag, pos.y + diag, pos.z + diag);
    glVertex3d(pos.x + diag, pos.y + diag, pos.z + diag);
    glVertex3d(pos.x - diag, pos.y + diag, pos.z - diag);
    glVertex3d(pos.x + diag, pos.y + diag, pos.z - diag);

    glVertex3d(pos.x - diag, pos.y - diag, pos.z + diag);
    glVertex3d(pos.x - diag, pos.y + diag, pos.z + diag);
    glVertex3d(pos.x - diag, pos.y - diag, pos.z - diag);
    glVertex3d(pos.x - diag, pos.y + diag, pos.z - diag);

    glVertex3d(pos.x + diag, pos.y - diag, pos.z + diag);
    glVertex3d(pos.x + diag, pos.y + diag, pos.z + diag);
    glVertex3d(pos.x + diag, pos.y - diag, pos.z - diag);
    glVertex3d(pos.x + diag, pos.y + diag, pos.z - diag);

    glVertex3d(pos.x - diag, pos.y - diag, pos.z - diag);
    glVertex3d(pos.x - diag, pos.y - diag, pos.z + diag);
    glVertex3d(pos.x + diag, pos.y - diag, pos.z - diag);
    glVertex3d(pos.x + diag, pos.y - diag, pos.z + diag);

    glVertex3d(pos.x - diag, pos.y + diag, pos.z - diag);
    glVertex3d(pos.x - diag, pos.y + diag, pos.z + diag);
    glVertex3d(pos.x + diag, pos.y + diag, pos.z - diag);
    glVertex3d(pos.x + diag, pos.y + diag, pos.z + diag);
}

/////////////////////////////////////////////////////////////////
//
//   Dot
//
/////////////////////////////////////////////////////////////////

class Dot::PrivateData
{
    QWT_DECLARE_PUBLIC(Dot)

public:
    PrivateData(Dot* q) : q_ptr(q), m_smooth(false), m_pointsize(1.0), m_oldstate(GL_FALSE)
    {
    }

    bool m_smooth;
    double m_pointsize;
    GLboolean m_oldstate;
};

/**
 * @brief Default constructor
 */
Dot::Dot() : QWT_PIMPL_CONSTRUCT
{
    configure(1, false);
}

/**
 * @brief Constructs a Dot with specified parameters
 * @param pointsize Point size
 * @param smooth Smooth point rendering
 */
Dot::Dot(double pointsize, bool smooth) : QWT_PIMPL_CONSTRUCT
{
    configure(pointsize, smooth);
}

Dot::Dot(const Dot& other) : VertexEnrichment(other), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    const PrivateData* od = other.d_func();
    d->m_smooth           = od->m_smooth;
    d->m_pointsize        = od->m_pointsize;
    d->m_oldstate         = od->m_oldstate;
}

Dot::~Dot() = default;

Enrichment* Dot::clone() const
{
    return new Dot(*this);
}

void Dot::configure(double pointsize, bool smooth)
{
    plot = nullptr;
    QWT_D(d);
    d->m_pointsize = pointsize;
    d->m_smooth    = smooth;
}

void Dot::drawBegin()
{
    QWT_D(d);
    setDevicePointSize(d->m_pointsize);
    d->m_oldstate = glIsEnabled(GL_POINT_SMOOTH);
    if (d->m_smooth)
        glEnable(GL_POINT_SMOOTH);
    else
        glDisable(GL_POINT_SMOOTH);

    glBegin(GL_POINTS);
}

void Dot::drawEnd()
{
    QWT_D(d);
    glEnd();

    if (d->m_oldstate)
        glEnable(GL_POINT_SMOOTH);
    else
        glDisable(GL_POINT_SMOOTH);
}

void Dot::draw(Triple const& pos)
{
    RGBA rgba = (*plot->dataColor())(pos);
    glColor4d(rgba.r, rgba.g, rgba.b, rgba.a);
    glVertex3d(pos.x, pos.y, pos.z);
}

/////////////////////////////////////////////////////////////////
//
//   Cone
//
/////////////////////////////////////////////////////////////////

class Cone::PrivateData
{
    QWT_DECLARE_PUBLIC(Cone)

public:
    PrivateData(Cone* q) : q_ptr(q), m_hat(nullptr), m_disk(nullptr), m_quality(3), m_radius(0.0), m_oldstate(GL_FALSE)
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

/**
 * @brief Default constructor
 */
Cone::Cone() : QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    d->initQuadrics();
    configure(0, 3);
}

/**
 * @brief Constructs a Cone with specified radius and quality
 * @param rad Cone radius
 * @param quality Number of faces for the cone
 */
Cone::Cone(double rad, unsigned quality) : QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    d->initQuadrics();
    configure(rad, quality);
}

Cone::Cone(const Cone& other) : VertexEnrichment(other), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    const PrivateData* od = other.d_func();
    d->m_quality          = od->m_quality;
    d->m_radius           = od->m_radius;
    d->m_oldstate         = od->m_oldstate;
    d->initQuadrics();
}

/**
 * @brief Destructor
 */
Cone::~Cone() = default;

Enrichment* Cone::clone() const
{
    return new Cone(*this);
}

void Cone::configure(double rad, unsigned quality)
{
    plot = nullptr;
    QWT_D(d);
    d->m_radius   = rad;
    d->m_quality  = quality;
    d->m_oldstate = GL_FALSE;
}

void Cone::draw(Triple const& pos)
{
    QWT_D(d);
    RGBA rgba = (*plot->dataColor())(pos);
    glColor4d(rgba.r, rgba.g, rgba.b, rgba.a);

    GLint mode;
    glGetIntegerv(GL_MATRIX_MODE, &mode);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glTranslatef(pos.x, pos.y, pos.z);

    gluCylinder(d->m_hat, 0.0, d->m_radius, d->m_radius * 2, d->m_quality, 1);
    glTranslatef(0, 0, d->m_radius * 2);
    gluDisk(d->m_disk, 0.0, d->m_radius, d->m_quality, 1);

    glPopMatrix();
    glMatrixMode(mode);
}

/////////////////////////////////////////////////////////////////
//
//   Arrow
//
/////////////////////////////////////////////////////////////////

class Arrow::PrivateData
{
    QWT_DECLARE_PUBLIC(Arrow)

public:
    PrivateData(Arrow* q)
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

Arrow::Arrow() : QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    d->initQuadrics();
    configure(3, 0.4, 0.06, 0.02);
}

Arrow::Arrow(const Arrow& other) : VertexEnrichment(other), QWT_PIMPL_CONSTRUCT
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

/**
 * @brief Destructor
 */
Arrow::~Arrow() = default;

Enrichment* Arrow::clone() const
{
    return new Arrow(*this);
}

/**
 * @brief Configures the arrow appearance
 * @param segs Number of faces for the arrows (see the gallery for examples)
 * @param relconelength Relative cone length (see arrowanatomy.png)
 * @param relconerad Relative cone radius (see arrowanatomy.png)
 * @param relstemrad Relative stem radius (see arrowanatomy.png)
 * @image html arrowanatomy.png
 */
void Arrow::configure(int segs, double relconelength, double relconerad, double relstemrad)
{
    plot = nullptr;
    QWT_D(d);
    d->m_segments      = segs;
    d->m_oldstate      = GL_FALSE;
    d->m_relConeLength = relconelength;
    d->m_relConeRadius = relconerad;
    d->m_relStemRadius = relstemrad;
}

void Arrow::setQuality(int val)
{
    QWT_D(d);
    d->m_segments = val;
}

void Arrow::setTop(Triple t)
{
    QWT_D(d);
    d->m_top = t;
}

void Arrow::setColor(RGBA rgba)
{
    QWT_D(d);
    d->m_rgba = rgba;
}

void Arrow::draw(Triple const& pos)
{
    QWT_D(d);
    Triple end    = d->m_top;
    Triple beg    = pos;
    Triple vdiff  = end - beg;
    double length = vdiff.length();
    glColor4d(d->m_rgba.r, d->m_rgba.g, d->m_rgba.b, d->m_rgba.a);

    double radius[ 2 ];
    radius[ 0 ] = d->m_relConeRadius * length;
    radius[ 1 ] = d->m_relStemRadius * length;

    GLint mode;
    glGetIntegerv(GL_MATRIX_MODE, &mode);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    Triple axis;
    double phi = calcRotation(axis, FreeVector(beg, end));

    glTranslatef(beg.x, beg.y, beg.z);
    glRotatef(phi, axis.x, axis.y, axis.z);

    double baseheight = (1 - d->m_relConeLength) * length;

    glTranslatef(0, 0, baseheight);

    gluCylinder(d->m_hat, radius[ 0 ], 0.0, d->m_relConeLength * length, d->m_segments, 1);
    gluDisk(d->m_disk, radius[ 1 ], radius[ 0 ], d->m_segments, 1);

    glTranslatef(0, 0, -baseheight);

    gluCylinder(d->m_base, radius[ 1 ], radius[ 1 ], baseheight, d->m_segments, 1);
    gluDisk(d->m_disk, 0, radius[ 1 ], d->m_segments, 1);

    glPopMatrix();
    glMatrixMode(mode);
}

/**
 * @brief Calculates rotation angle to transform a z-axis vector to coincide with a given vector
 * @param[out] axis The axis to rotate around
 * @param vec The target free vector
 * @return Angle in degrees to rotate
 * @details Transforms a vector on the z axis with length |beg-end| to get them
 *          in coincidence with the vector(beg,end).
 */
double Arrow::calcRotation(Triple& axis, FreeVector const& vec)
{

    Triple end = vec.top;
    Triple beg = vec.base;

    Triple firstbeg(0.0, 0.0, 0.0);
    Triple firstend(0.0, 0.0, (end - beg).length());

    Triple first = firstend - firstbeg;
    first.normalize();

    Triple second = end - beg;
    second.normalize();

    axis          = normalizedcross(first, second);
    double cosphi = dotProduct(first, second);

    return 180 * acos(cosphi) / Qwt3D_PI;
}
