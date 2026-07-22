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

    GLboolean m_ls          = 0;
    GLboolean m_pols        = 0;
    GLint m_polmode[ 2 ]    = { 0, 0 };
    GLfloat m_lw            = 0.0f;
    GLint m_blsrc           = 0;
    GLint m_bldst           = 0;
    GLdouble m_col[ 4 ]     = { 0.0, 0.0, 0.0, 0.0 };
    GLint m_pattern         = 0;
    GLint m_factor          = 0;
    GLboolean m_sallowed    = 0;
    GLboolean m_tex2d       = 0;
    GLint m_matrixmode      = 0;
    GLfloat m_poloffs[ 2 ]  = { 0.0f, 0.0f };
    GLboolean m_poloffsfill = 0;

    std::list< Qwt3DDrawable* > m_dlist;
};

Qwt3DDrawable::Qwt3DDrawable() : QWT_PIMPL_CONSTRUCT
{
}

Qwt3DDrawable::Qwt3DDrawable(Qwt3DDrawable&& other) noexcept : m_data(std::move(other.m_data)), color(other.color)
{
    std::copy(std::begin(other.modelMatrix), std::end(other.modelMatrix), std::begin(modelMatrix));
    std::copy(std::begin(other.projMatrix), std::end(other.projMatrix), std::begin(projMatrix));
    std::copy(std::begin(other.viewport), std::end(other.viewport), std::begin(viewport));
}

Qwt3DDrawable& Qwt3DDrawable::operator=(Qwt3DDrawable&& other) noexcept
{
    if (this != &other) {
        m_data = std::move(other.m_data);
        color  = other.color;
        std::copy(std::begin(other.modelMatrix), std::end(other.modelMatrix), std::begin(modelMatrix));
        std::copy(std::begin(other.projMatrix), std::end(other.projMatrix), std::begin(projMatrix));
        std::copy(std::begin(other.viewport), std::end(other.viewport), std::begin(viewport));
    }
    return *this;
}

Qwt3DDrawable::~Qwt3DDrawable()
{
    detachAll();
}

void Qwt3DDrawable::saveGLState()
{
    QWT_D(d);

    glGetBooleanv(GL_LINE_SMOOTH, &d->m_ls);
    glGetBooleanv(GL_POLYGON_SMOOTH, &d->m_pols);
    glGetFloatv(GL_LINE_WIDTH, &d->m_lw);
    glGetIntegerv(GL_BLEND_SRC, &d->m_blsrc);
    glGetIntegerv(GL_BLEND_DST, &d->m_bldst);
    glGetDoublev(GL_CURRENT_COLOR, d->m_col);
    glGetIntegerv(GL_LINE_STIPPLE_PATTERN, &d->m_pattern);
    glGetIntegerv(GL_LINE_STIPPLE_REPEAT, &d->m_factor);
    glGetBooleanv(GL_LINE_STIPPLE, &d->m_sallowed);
    glGetBooleanv(GL_TEXTURE_2D, &d->m_tex2d);
    glGetIntegerv(GL_POLYGON_MODE, d->m_polmode);
    glGetIntegerv(GL_MATRIX_MODE, &d->m_matrixmode);
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &d->m_poloffs[ 0 ]);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &d->m_poloffs[ 1 ]);
    glGetBooleanv(GL_POLYGON_OFFSET_FILL, &d->m_poloffsfill);
}

void Qwt3DDrawable::restoreGLState()
{
    QWT_D(d);

    Enable(GL_LINE_SMOOTH, d->m_ls);
    Enable(GL_POLYGON_SMOOTH, d->m_pols);

    setDeviceLineWidth(d->m_lw);
    glBlendFunc(d->m_blsrc, d->m_bldst);
    glColor4dv(d->m_col);

    glLineStipple(d->m_factor, d->m_pattern);
    Enable(GL_LINE_STIPPLE, d->m_sallowed);
    Enable(GL_TEXTURE_2D, d->m_tex2d);
    glPolygonMode(d->m_polmode[ 0 ], d->m_polmode[ 1 ]);
    glMatrixMode(d->m_matrixmode);
    glPolygonOffset(d->m_poloffs[ 0 ], d->m_poloffs[ 1 ]);
    setDevicePolygonOffset(d->m_poloffs[ 0 ], d->m_poloffs[ 1 ]);

    Enable(GL_POLYGON_OFFSET_FILL, d->m_poloffsfill);
}

void Qwt3DDrawable::Enable(GLenum what, GLboolean val)
{
    if (val)
        glEnable(what);
    else
        glDisable(what);
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
 * @brief Converts viewport coordinates to world coordinates (glUnProject)
 * @param win Viewport (window) coordinates
 * @param[out] err Optional error flag (true on failure)
 * @return World (object) coordinates
 * @warning Don't rely on (use) this in display lists!
 */
Triple Qwt3DDrawable::ViewPort2World(Triple win, bool* err)
{
    Triple obj;

    getMatrices(modelMatrix, projMatrix, viewport);
    int res = gluUnProject(win.x, win.y, win.z, modelMatrix, projMatrix, viewport, &obj.x, &obj.y, &obj.z);

    if (err)
        *err = (res) ? false : true;
    return obj;
}

/**
 * @brief Converts world coordinates to viewport coordinates (glProject)
 * @param obj World (object) coordinates
 * @param[out] err Optional error flag (true on failure)
 * @return Viewport (window) coordinates
 * @warning Don't rely on (use) this in display lists!
 */
Triple Qwt3DDrawable::World2ViewPort(Triple obj, bool* err)
{
    Triple win;

    getMatrices(modelMatrix, projMatrix, viewport);
    int res = gluProject(obj.x, obj.y, obj.z, modelMatrix, projMatrix, viewport, &win.x, &win.y, &win.z);

    if (err)
        *err = (res) ? false : true;
    return win;
}

/**
 * @brief Calculates world coordinates from relative viewport position
 * @param rel Relative position in viewport coordinates
 * @return Corresponding world coordinates
 * @warning Don't rely on (use) this in display lists!
 */
Triple Qwt3DDrawable::relativePosition(Triple rel)
{
    return ViewPort2World(Triple((rel.x - viewport[ 0 ]) * viewport[ 2 ], (rel.y - viewport[ 1 ]) * viewport[ 3 ], rel.z));
}

void Qwt3DDrawable::draw()
{
    QWT_D(d);

    saveGLState();

    for (auto* drawable : d->m_dlist) {
        drawable->draw();
    }
    restoreGLState();
}

void Qwt3DDrawable::setColor(double r, double g, double b, double a)
{
    color = RGBA(r, g, b, a);
}

void Qwt3DDrawable::setColor(RGBA rgba)
{
    color = rgba;
}
