#ifndef QWT3D_DRAWABLE_H
#define QWT3D_DRAWABLE_H

#include "qwt3d_global.h"
#include "qwt3d_types.h"
#include "qwt3d_io_gl2ps.h"



/**
 * @brief Abstract base class for Drawables
 */
class QWT3D_EXPORT Qwt3DDrawable
{
    QWT_DECLARE_PRIVATE(Qwt3DDrawable)

public:
    virtual ~Qwt3DDrawable() = 0;

    virtual void draw();

    virtual void saveGLState();
    virtual void restoreGLState();

    void attach(Qwt3DDrawable*);
    void detach(Qwt3DDrawable*);
    void detachAll();

    virtual void setColor(double r, double g, double b, double a = 1);
    virtual void setColor(RGBA rgba);
    Triple relativePosition(Triple rel);

protected:
    RGBA color;
    void Enable(GLenum what, GLboolean val);
    Triple ViewPort2World(Triple win, bool* err = nullptr);
    Triple World2ViewPort(Triple obj, bool* err = nullptr);

    Qwt3DDrawable();
    Qwt3DDrawable(Qwt3DDrawable&& other) noexcept;
    Qwt3DDrawable& operator=(Qwt3DDrawable&& other) noexcept;

    GLdouble modelMatrix[ 16 ];
    GLdouble projMatrix[ 16 ];
    GLint viewport[ 4 ];
};


#endif
