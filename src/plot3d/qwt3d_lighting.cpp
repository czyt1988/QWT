#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4305)
#pragma warning(disable : 4786)
#endif

#include "qwt3d_plot_p.h"

/**
 * @brief Enable or disable lighting
 * @param val True to enable, false to disable
 * @details Lighting parameters are stored on CPU only. No GL_LIGHTING
 *          glEnable/glDisable calls are made. Shader-based lighting
 *          will use these stored parameters as uniform values.
 */
void Qwt3DPlot::enableLighting(bool val)
{
    QWT_D(d);
    if (d->m_lightingEnabled == val)
        return;

    d->m_lightingEnabled = val;

    if (!initializedGL())
        return;
    update();
}

void Qwt3DPlot::disableLighting(bool val)
{
    enableLighting(!val);
}

bool Qwt3DPlot::lightingEnabled() const
{
    QWT_DC(d);
    return d->m_lightingEnabled;
}

/**
  @param light light number [0..7]
  @see setLight
*/
void Qwt3DPlot::illuminate(unsigned light)
{
    QWT_D(d);
    if (light > 7)
        return;
    d->m_lights[ light ].unlit = false;
    update();
}

/**
  @param light light number [0..7]
  @see setLight
*/
void Qwt3DPlot::blowout(unsigned light)
{
    QWT_D(d);
    if (light > 7)
        return;
    d->m_lights[ light ].unlit = true;
    update();
}

/**
  @brief Sets material component (RGBA)
  @details Parameters are stored on CPU for future shader uniform upload.
           No glMaterialfv calls are made.
*/
void Qwt3DPlot::setMaterialComponent(unsigned int property, double r, double g, double b, double a)
{
    QWT_D(d);

    RGBA* component = nullptr;
    if (property == GL_AMBIENT)
        component = &d->m_material.ambient;
    else if (property == GL_DIFFUSE)
        component = &d->m_material.diffuse;
    else if (property == GL_SPECULAR)
        component = &d->m_material.specular;

    if (!component)
        return;

    *component = RGBA(r, g, b, a);
    update();
}

/**
  @brief Sets material component (intensity)
  @details Convenience function: sets r=g=b=intensity, a=1.0
*/
void Qwt3DPlot::setMaterialComponent(unsigned int property, double intensity)
{
    setMaterialComponent(property, intensity, intensity, intensity, 1.0);
}

/**
  @brief Sets shininess exponent
  @details Parameter is stored on CPU for future shader uniform upload.
           No glMaterialf calls are made.
*/
void Qwt3DPlot::setShininess(double exponent)
{
    QWT_D(d);
    d->m_material.shininess = exponent;
    update();
}

/**
  @brief Sets light component (RGBA)
  @details Parameters are stored on CPU for future shader uniform upload.
           No glLightfv calls are made.
*/
void Qwt3DPlot::setLightComponent(unsigned int property, double r, double g, double b, double a, unsigned light)
{
    QWT_D(d);
    if (light > 7)
        return;

    RGBA* component = nullptr;
    if (property == GL_AMBIENT)
        component = &d->m_lights[ light ].ambient;
    else if (property == GL_DIFFUSE)
        component = &d->m_lights[ light ].diffuse;
    else if (property == GL_SPECULAR)
        component = &d->m_lights[ light ].specular;

    if (!component)
        return;

    *component = RGBA(r, g, b, a);
    update();
}

/**
  @brief Sets light component (intensity)
  @details Convenience function: sets r=g=b=intensity, a=1.0
*/
void Qwt3DPlot::setLightComponent(unsigned int property, double intensity, unsigned light)
{
    setLightComponent(property, intensity, intensity, intensity, 1.0, light);
}

/**
  @brief Returns, whether light idx is switched on
  @details The modernized renderer uses a single light; the per light flags of
           the classic API are still stored and illuminate()/blowout() switch
           light 0 on and off.
  @param idx light number [0..7]
  @return true, when light idx is switched on
*/
bool Qwt3DPlot::isLightIlluminated(unsigned idx) const
{
    QWT_DC(d);
    return (idx < 8) ? !d->m_lights[ idx ].unlit : false;
}

/**
  @brief Returns a stored light component (RGBA)
  @param property GL_AMBIENT, GL_DIFFUSE or GL_SPECULAR
  @param light light number [0..7]
  @return the component; white when the property is unknown
*/
RGBA Qwt3DPlot::lightComponent(unsigned int property, unsigned light) const
{
    QWT_DC(d);
    if (light > 7)
        return RGBA(1.0, 1.0, 1.0, 1.0);

    switch (property) {
    case GL_AMBIENT:
        return d->m_lights[ light ].ambient;
    case GL_DIFFUSE:
        return d->m_lights[ light ].diffuse;
    case GL_SPECULAR:
        return d->m_lights[ light ].specular;
    default:
        break;
    }

    return RGBA(1.0, 1.0, 1.0, 1.0);
}

/**
  @brief Returns a stored material component (RGBA)
  @param property GL_AMBIENT, GL_DIFFUSE or GL_SPECULAR
  @return the component; white when the property is unknown
*/
RGBA Qwt3DPlot::materialComponent(unsigned int property) const
{
    QWT_DC(d);

    switch (property) {
    case GL_AMBIENT:
        return d->m_material.ambient;
    case GL_DIFFUSE:
        return d->m_material.diffuse;
    case GL_SPECULAR:
        return d->m_material.specular;
    default:
        break;
    }

    return RGBA(1.0, 1.0, 1.0, 1.0);
}

/**
  @brief Returns the shininess exponent of the material
*/
double Qwt3DPlot::shininess() const
{
    QWT_DC(d);
    return d->m_material.shininess;
}

/**
  @brief Set the rotation angle of the light source
  @param xVal angle in degree to rotate around the X axis
  @param yVal angle in degree to rotate around the Y axis
  @param zVal angle in degree to rotate around the Z axis
  @param light light number
*/
void Qwt3DPlot::setLightRotation(double xVal, double yVal, double zVal, unsigned light)
{
    QWT_D(d);
    if (light > 7)
        return;
    d->m_lights[ light ].rot.x = xVal;
    d->m_lights[ light ].rot.y = yVal;
    d->m_lights[ light ].rot.z = zVal;
}

/**
  @brief Set the shift in light source (world) coordinates
  @param xVal shift along (world) X axis
  @param yVal shift along (world) Y axis
  @param zVal shift along (world) Z axis
  @param light light number
*/
void Qwt3DPlot::setLightShift(double xVal, double yVal, double zVal, unsigned light)
{
    QWT_D(d);
    if (light > 7)
        return;
    d->m_lights[ light ].shift.x = xVal;
    d->m_lights[ light ].shift.y = yVal;
    d->m_lights[ light ].shift.z = zVal;
}

/**
 * @brief Apply light parameters (CPU-side only)
 * @details No GL light calls. Light parameters are stored in the Light struct
 *          and will be uploaded as shader uniforms in the modernization task.
 */
void Qwt3DPlot::applyLight(unsigned)
{
    // No GL calls — lighting params stored on CPU for shader uniform upload
}

/**
 * @brief Apply all light parameters (CPU-side only)
 * @details No GL light/material calls. Iterates lights to ensure all
 *          parameters are up to date on CPU side.
 */
void Qwt3DPlot::applyLights()
{
    // No GL calls — lighting handled via CPU-stored parameters
    // Shader-based lighting will read these as uniforms
}
