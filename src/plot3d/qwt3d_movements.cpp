#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4305)
#pragma warning(disable : 4786)
#endif

#include <cfloat>
#include "qwt3d_plot_p.h"


/**
  Set the rotation angle of the object. If you look along the respective axis towards ascending
  values, the rotation is performed in mathematical \e negative sense @param xVal angle in \e degree
  to rotate around the X axis @param yVal angle in \e degree to rotate around the Y axis @param zVal
  angle in \e degree to rotate around the Z axis
*/
void Plot3D::setRotation(double xVal, double yVal, double zVal)
{
    QWT_D(d);
    if (d->m_xRot == xVal && d->m_yRot == yVal && d->m_zRot == zVal)
        return;

    d->m_xRot = xVal;
    d->m_yRot = yVal;
    d->m_zRot = zVal;

    update();
    emit rotationChanged(xVal, yVal, zVal);
}

/**
  Set the shift in object (world) coordinates.
        @param xVal shift along (world) X axis
        @param yVal shift along (world) Y axis
        @param zVal shift along (world) Z axis
        @see setViewportShift()
*/
void Plot3D::setShift(double xVal, double yVal, double zVal)
{
    QWT_D(d);
    if (d->m_xShift == xVal && d->m_yShift == yVal && d->m_zShift == zVal)
        return;

    d->m_xShift = xVal;
    d->m_yShift = yVal;
    d->m_zShift = zVal;
    update();
    emit shiftChanged(xVal, yVal, zVal);
}

/**
  Performs shifting along screen axes.
  The shift moves points inside a sphere,
  which encloses the unscaled and unzoomed data
        by multiples of the spheres diameter

        @param xVal shift along (view) X axis
        @param yVal shift along (view) Y axis
        @see setShift()
*/
void Plot3D::setViewportShift(double xVal, double yVal)
{
    QWT_D(d);
    if (d->m_xVPShift == xVal && d->m_yVPShift == yVal)
        return;

    d->m_xVPShift = xVal;
    d->m_yVPShift = yVal;

    update();
    emit vieportShiftChanged(d->m_xVPShift, d->m_yVPShift);
}

/**
  Set the scale in object (world) coordinates.
        @param xVal scaling for X values
        @param yVal scaling for Y values
        @param zVal scaling for Z values

        A respective value of 1 represents no scaling;
*/
void Plot3D::setScale(double xVal, double yVal, double zVal)
{
    QWT_D(d);
    if (d->m_xScale == xVal && d->m_yScale == yVal && d->m_zScale == zVal)
        return;

    d->m_xScale = (xVal < DBL_EPSILON) ? DBL_EPSILON : xVal;
    d->m_yScale = (yVal < DBL_EPSILON) ? DBL_EPSILON : yVal;
    d->m_zScale = (zVal < DBL_EPSILON) ? DBL_EPSILON : zVal;

    update();
    emit scaleChanged(xVal, yVal, zVal);
}

/**
  Set the (zoom in addition to scale).
        @param val zoom value (value == 1 indicates no zooming)
*/
void Plot3D::setZoom(double val)
{
    QWT_D(d);
    if (d->m_zoom == val)
        return;

    d->m_zoom = (val < DBL_EPSILON) ? DBL_EPSILON : val;
    update();
    emit zoomChanged(val);
}
