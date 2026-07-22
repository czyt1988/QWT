#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4305)
#pragma warning(disable : 4786)
#endif

#include <cfloat>
#include "qwt3d_plot_p.h"


namespace
{
inline GLenum lightEnum(unsigned idx)
{
    switch (idx) {
    case 0:
        return GL_LIGHT0;
    case 1:
        return GL_LIGHT1;
    case 2:
        return GL_LIGHT2;
    case 3:
        return GL_LIGHT3;
    case 4:
        return GL_LIGHT4;
    case 5:
        return GL_LIGHT5;
    case 6:
        return GL_LIGHT6;
    case 7:
        return GL_LIGHT7;
    default:
        return GL_LIGHT0;
    }
}

}

void Plot3D::enableLighting(bool val)
{
    QWT_D(d);
    if (d->m_lightingEnabled == val)
        return;

    d->m_lightingEnabled = val;
    makeCurrent();
    if (val)
        glEnable(GL_LIGHTING);
    else
        glDisable(GL_LIGHTING);

    if (!initializedGL())
        return;
    update();
}

void Plot3D::disableLighting(bool val)
{
    enableLighting(!val);
}

bool Plot3D::lightingEnabled() const
{
    QWT_DC(d);
    return d->m_lightingEnabled;
}

/**
  @param light light number [0..7]
  @see setLight
*/
void Plot3D::illuminate(unsigned light)
{
    QWT_D(d);
    if (light > 7)
        return;
    d->m_lights[ light ].unlit = false;
}
/**
  @param light light number [0..7]
  @see setLight
*/
void Plot3D::blowout(unsigned light)
{
    QWT_D(d);
    if (light > 7)
        return;
    d->m_lights[ light ].unlit = true;
}

/**
  Sets GL material properties
*/
void Plot3D::setMaterialComponent(GLenum property, double r, double g, double b, double a)
{
    GLfloat rgba[ 4 ] = {
        static_cast< GLfloat >(r), static_cast< GLfloat >(g), static_cast< GLfloat >(b), static_cast< GLfloat >(a)
    };
    makeCurrent();
    glMaterialfv(GL_FRONT_AND_BACK, property, rgba);
}

/**
  This function is for convenience. It sets GL material properties with the equal r,g,b values
  and a blending alpha with value 1.0
*/
void Plot3D::setMaterialComponent(GLenum property, double intensity)
{
    setMaterialComponent(property, intensity, intensity, intensity, 1.0);
}

/**
  Sets GL shininess
*/
void Plot3D::setShininess(double exponent)
{
    makeCurrent();
    glMaterialf(GL_FRONT, GL_SHININESS, exponent);
}

/**
  Sets GL light properties for light 'light'
*/
void Plot3D::setLightComponent(GLenum property, double r, double g, double b, double a, unsigned light)
{
    GLfloat rgba[ 4 ] = {
        static_cast< GLfloat >(r), static_cast< GLfloat >(g), static_cast< GLfloat >(b), static_cast< GLfloat >(a)
    };
    makeCurrent();
    glLightfv(lightEnum(light), property, rgba);
}

/**
  This function is for convenience. It sets GL light properties with the equal r,g,b values
  and a blending alpha with value 1.0
*/
void Plot3D::setLightComponent(GLenum property, double intensity, unsigned light)
{
    setLightComponent(property, intensity, intensity, intensity, 1.0, light);
}

/**
  Set the rotation angle of the light source. If you look along the respective axis towards
  ascending values, the rotation is performed in mathematical \e negative sense @param xVal angle in
  \e degree to rotate around the X axis @param yVal angle in \e degree to rotate around the Y axis
        @param zVal angle in \e degree to rotate around the Z axis
  @param light light number
*/
void Plot3D::setLightRotation(double xVal, double yVal, double zVal, unsigned light)
{
    QWT_D(d);
    if (light > 7)
        return;
    d->m_lights[ light ].rot.x = xVal;
    d->m_lights[ light ].rot.y = yVal;
    d->m_lights[ light ].rot.z = zVal;
}

/**
  Set the shift in light source (world) coordinates.
        @param xVal shift along (world) X axis
        @param yVal shift along (world) Y axis
        @param zVal shift along (world) Z axis
  @param light light number
        @see setViewportShift()
*/
void Plot3D::setLightShift(double xVal, double yVal, double zVal, unsigned light)
{
    QWT_D(d);
    if (light > 7)
        return;
    d->m_lights[ light ].shift.x = xVal;
    d->m_lights[ light ].shift.y = yVal;
    d->m_lights[ light ].shift.z = zVal;
}

void Plot3D::applyLight(unsigned light)
{
    QWT_D(d);
    if (d->m_lights[ light ].unlit)
        return;

    glEnable(lightEnum(light));
    glLoadIdentity();

    glRotatef(d->m_lights[ light ].rot.x - 90, 1.0, 0.0, 0.0);
    glRotatef(d->m_lights[ light ].rot.y, 0.0, 1.0, 0.0);
    glRotatef(d->m_lights[ light ].rot.z, 0.0, 0.0, 1.0);
    double light4d[ 4 ] = { d->m_lights[ light ].shift.x, d->m_lights[ light ].shift.y, d->m_lights[ light ].shift.z, 1.0 };
    GLfloat lightPos[ 4 ] {};
    for (size_t i = 0; i < 4; i++) {
        lightPos[ i ] = static_cast< GLfloat >(light4d[ i ]);
    }
    GLenum le = lightEnum(light);
    glLightfv(le, GL_POSITION, lightPos);
}

void Plot3D::applyLights()
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    for (unsigned i = 0; i < 8; ++i) {
        applyLight(i);
    }
    glPopMatrix();
}
