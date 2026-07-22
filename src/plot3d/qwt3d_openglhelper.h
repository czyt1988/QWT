#ifndef QWT3D_OPENGLHELPER_H
#define QWT3D_OPENGLHELPER_H

#include "qglobal.h"

#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#ifdef Q_OS_WIN
#include "windows.h"
#endif
#include <GL/glu.h>
#endif



#ifndef QWT3D_NOT_FOR_DOXYGEN

/**
 * @brief Helper class for managing OpenGL state enable/disable
 * @details Saves and restores OpenGL enable/disable state. Useful for temporarily
 *          changing GL states within a drawing context.
 */
class GLStateBewarer
{
public:
    GLStateBewarer(GLenum what, bool on, bool persist = false)
    {
        state_    = what;
        stateval_ = glIsEnabled(what);
        if (on)
            turnOn(persist);
        else
            turnOff(persist);
    }

    ~GLStateBewarer()
    {
        if (stateval_)
            glEnable(state_);
        else
            glDisable(state_);
    }

    void turnOn(bool persist = false)
    {
        glEnable(state_);
        if (persist)
            stateval_ = true;
    }

    void turnOff(bool persist = false)
    {
        glDisable(state_);
        if (persist)
            stateval_ = false;
    }

private:
    GLenum state_;
    bool stateval_;
};

// Returns OpenGL error string if an error occurred
inline const GLubyte* gl_error()
{
    GLenum errcode;
    const GLubyte* err = nullptr;

    if ((errcode = glGetError()) != GL_NO_ERROR) {
        err = gluErrorString(errcode);
    }
    return err;
}

// Safely deletes OpenGL display lists
inline void SaveGlDeleteLists(GLuint& lstidx, GLsizei range)
{
    if (glIsList(lstidx))
        glDeleteLists(lstidx, range);
    lstidx = 0;
}

/**
 * @brief Get OpenGL transformation matrices
 * @details Don't rely on (use) this in display lists!
 */
inline void getMatrices(GLdouble* modelMatrix, GLdouble* projMatrix, GLint* viewport)
{
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
}

/**
 * @brief Simplified glut routine (glUnProject): window coordinates -> object coordinates
 * @details Don't rely on (use) this in display lists!
 */
inline bool ViewPort2World(double& objx, double& objy, double& objz, double winx, double winy, double winz)
{
    GLdouble modelMatrix[ 16 ];
    GLdouble projMatrix[ 16 ];
    GLint viewport[ 4 ];

    getMatrices(modelMatrix, projMatrix, viewport);
    int res = gluUnProject(winx, winy, winz, modelMatrix, projMatrix, viewport, &objx, &objy, &objz);

    return (res == GL_FALSE) ? false : true;
}

/**
 * @brief Simplified glut routine (glProject): object coordinates -> window coordinates
 * @details Don't rely on (use) this in display lists!
 */
inline bool World2ViewPort(double& winx, double& winy, double& winz, double objx, double objy, double objz)
{
    GLdouble modelMatrix[ 16 ];
    GLdouble projMatrix[ 16 ];
    GLint viewport[ 4 ];

    getMatrices(modelMatrix, projMatrix, viewport);
    int res = gluProject(objx, objy, objz, modelMatrix, projMatrix, viewport, &winx, &winy, &winz);

    return (res == GL_FALSE) ? false : true;
}

#endif  // QWT3D_NOT_FOR_DOXYGEN


#endif  // QWT3D_OPENGLHELPER_H