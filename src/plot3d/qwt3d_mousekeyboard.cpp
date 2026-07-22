#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4305)
#pragma warning(disable : 4786)
#endif

#include "qwt3d_plot_p.h"
#include "qwt_qt5qt6_compat.hpp"

#include <cmath>

using namespace std;

/**
        Standard mouse button Function. Prepares the call to mouseMoveEvent
        @see mouseMoveEvent()
*/
void Plot3D::mousePressEvent(QMouseEvent* e)
{
    QWT_D(d);
    d->m_lastMouseMovePosition = qwt::compat::eventPos(e);
    d->m_pressed               = true;
}

/**
        Standard mouse button Function. Completes the call to mouseMoveEvent
        @see mouseMoveEvent()
*/
void Plot3D::mouseReleaseEvent(QMouseEvent*)
{
    QWT_D(d);
    d->m_pressed = false;
}

/**
        Standard mouse button Function
        @see assignMouse()
*/
void Plot3D::mouseMoveEvent(QMouseEvent* e)
{
    QWT_D(d);
    if (!d->m_pressed || !mouseEnabled()) {
        e->ignore();
        return;
    }

    Qwt3DMouseState bstate(e->buttons(), e->modifiers());

    QPoint diff = qwt::compat::eventPos(e) - d->m_lastMouseMovePosition;

    setRotationMouse(bstate, 3, diff);
    setScaleMouse(bstate, 5, diff);
    setShiftMouse(bstate, 2, diff);

    d->m_lastMouseMovePosition = qwt::compat::eventPos(e);
}

void Plot3D::setRotationMouse(Qwt3DMouseState bstate, double accel, QPoint diff)
{
    QWT_D(d);
    // Rotation
    double w = max(1, width());
    double h = max(1, height());

    double relx  = accel * 360 * diff.x() / w;
    double relyz = accel * 360 * diff.y() / h;

    double new_xrot = xRotation();
    double new_yrot = yRotation();
    double new_zrot = zRotation();

    if (bstate == d->m_xrotMState)
        new_xrot = static_cast< int >(std::round(xRotation() + relyz)) % 360;
    if (bstate == d->m_yrotMState)
        new_yrot = static_cast< int >(std::round(yRotation() + relx)) % 360;
    if (bstate == d->m_zrotMState)
        new_zrot = static_cast< int >(std::round(zRotation() + relx)) % 360;

    setRotation(new_xrot, new_yrot, new_zrot);
}

void Plot3D::setScaleMouse(Qwt3DMouseState bstate, double accel, QPoint diff)
{
    QWT_D(d);
    // Scale
    double w = max(1, width());
    double h = max(1, height());

    double relx  = diff.x() * accel / w;
    relx         = exp(relx) - 1;
    double relyz = diff.y() * accel / h;
    relyz        = exp(relyz) - 1;

    double new_xscale = xScale();
    double new_yscale = yScale();
    double new_zscale = zScale();

    if (bstate == d->m_xscaleMState)
        new_xscale = max(0.0, xScale() + relx);
    if (bstate == d->m_yscaleMState)
        new_yscale = max(0.0, yScale() - relyz);
    if (bstate == d->m_zscaleMState)
        new_zscale = max(0.0, zScale() - relyz);

    setScale(new_xscale, new_yscale, new_zscale);

    if (bstate == d->m_zoomMState)
        setZoom(max(0.0, zoom() - relyz));
}

void Plot3D::setShiftMouse(Qwt3DMouseState bstate, double accel, QPoint diff)
{
    QWT_D(d);
    // Shift
    double w = max(1, width());
    double h = max(1, height());

    double relx  = diff.x() * accel / w;
    double relyz = diff.y() * accel / h;

    double new_xshift = xViewportShift();
    double new_yshift = yViewportShift();

    if (bstate == d->m_xshiftMState)
        new_xshift = xViewportShift() + relx;
    if (bstate == d->m_yshiftMState)
        new_yshift = yViewportShift() - relyz;

    setViewportShift(new_xshift, new_yshift);
}

/**
        Standard wheel Function - zoom (wheel only) or z-scale (shift+wheel)
*/
void Plot3D::wheelEvent(QWheelEvent* e)
{
    if (!mouseEnabled())
        return;

    double accel = 0.05;

    double step = accel * qwt::compat::wheelEventDelta(e) / WHEEL_DELTA;
    step        = exp(step) - 1;

    if (e->modifiers() & Qt::ShiftModifier)
        setScale(xScale(), yScale(), max(0.0, zScale() + step));
    else
        setZoom(max(0.0, zoom() + step));
}

/**
        Sets the key/mousebutton combination for data/coordinatesystem moves inside the widget\n\n
        default behaviour:\n

        @verbatim
        rotate around x axis: Qt::LeftButton
        rotate around y axis: Qt::LeftButton | Qt::ShiftButton
        rotate around z axis: Qt::LeftButton
        scale x:              Qt::LeftButton | Qt::AltButton
        scale y:              Qt::LeftButton | Qt::AltButton
        scale z:              Qt::LeftButton | Qt::AltButton | Qt::ShiftButton
        zoom:                 Qt::LeftButton | Qt::AltButton | Qt::ControlButton
        shifting along x:     Qt::LeftButton | Qt::ControlButton
        shifting along y:     Qt::LeftButton | Qt::ControlButton
        @endverbatim

        mouseMoveEvent() evaluates this function - if overridden, their usefulness becomes somehow
   limited
*/
void Plot3D::assignMouse(Qwt3DMouseState xrot,
                         Qwt3DMouseState yrot,
                         Qwt3DMouseState zrot,
                         Qwt3DMouseState xscale,
                         Qwt3DMouseState yscale,
                         Qwt3DMouseState zscale,
                         Qwt3DMouseState zoom,
                         Qwt3DMouseState xshift,
                         Qwt3DMouseState yshift)
{
    QWT_D(d);
    d->m_xrotMState   = xrot;
    d->m_yrotMState   = yrot;
    d->m_zrotMState   = zrot;
    d->m_xscaleMState = xscale;
    d->m_yscaleMState = yscale;
    d->m_zscaleMState = zscale;
    d->m_zoomMState   = zoom;
    d->m_xshiftMState = xshift;
    d->m_yshiftMState = yshift;
}

/**
The function has no effect if you derive from Plot3D and overrides the mouse Function too careless.
In this case check first against mouseEnabled() in your version of mouseMoveEvent() and
wheelEvent(). A more fine grained input control can be achieved by combining assignMouse() with
enableMouse().
*/
void Plot3D::enableMouse(bool val)
{
    QWT_D(d);
    d->m_mouseInputEnabled = val;
}

/**
@see enableMouse()
*/
void Plot3D::disableMouse(bool val)
{
    QWT_D(d);
    d->m_mouseInputEnabled = !val;
}

bool Plot3D::mouseEnabled() const
{
    QWT_DC(d);
    return d->m_mouseInputEnabled;
}

void Plot3D::keyPressEvent(QKeyEvent* e)
{
    QWT_D(d);
    if (!keyboardEnabled()) {
        e->ignore();
        return;
    }

    Qwt3DKeyboardState keyseq(e->key(), e->modifiers());

    setRotationKeyboard(keyseq, d->m_kbdRotSpeed);
    setScaleKeyboard(keyseq, d->m_kbdScaleSpeed);
    setShiftKeyboard(keyseq, d->m_kbdShiftSpeed);
}

void Plot3D::setRotationKeyboard(Qwt3DKeyboardState kseq, double speed)
{
    QWT_D(d);
    // Rotation
    double w = max(1, width());
    double h = max(1, height());

    double relx  = speed * 360 / w;
    double relyz = speed * 360 / h;

    double new_xrot = xRotation();
    double new_yrot = yRotation();
    double new_zrot = zRotation();

    if (kseq == d->m_xrotKState[ 0 ])
        new_xrot = static_cast< int >(std::round(xRotation() + relyz)) % 360;
    if (kseq == d->m_xrotKState[ 1 ])
        new_xrot = static_cast< int >(std::round(xRotation() - relyz)) % 360;
    if (kseq == d->m_yrotKState[ 0 ])
        new_yrot = static_cast< int >(std::round(yRotation() + relx)) % 360;
    if (kseq == d->m_yrotKState[ 1 ])
        new_yrot = static_cast< int >(std::round(yRotation() - relx)) % 360;
    if (kseq == d->m_zrotKState[ 0 ])
        new_zrot = static_cast< int >(std::round(zRotation() + relx)) % 360;
    if (kseq == d->m_zrotKState[ 1 ])
        new_zrot = static_cast< int >(std::round(zRotation() - relx)) % 360;

    setRotation(new_xrot, new_yrot, new_zrot);
}

void Plot3D::setScaleKeyboard(Qwt3DKeyboardState kseq, double speed)
{
    QWT_D(d);
    // Scale
    double w = max(1, width());
    double h = max(1, height());

    double relx  = speed / w;
    relx         = exp(relx) - 1;
    double relyz = speed / h;
    relyz        = exp(relyz) - 1;

    double new_xscale = xScale();
    double new_yscale = yScale();
    double new_zscale = zScale();

    if (kseq == d->m_xscaleKState[ 0 ])
        new_xscale = max(0.0, xScale() + relx);
    if (kseq == d->m_xscaleKState[ 1 ])
        new_xscale = max(0.0, xScale() - relx);
    if (kseq == d->m_yscaleKState[ 0 ])
        new_yscale = max(0.0, yScale() - relyz);
    if (kseq == d->m_yscaleKState[ 1 ])
        new_yscale = max(0.0, yScale() + relyz);
    if (kseq == d->m_zscaleKState[ 0 ])
        new_zscale = max(0.0, zScale() - relyz);
    if (kseq == d->m_zscaleKState[ 1 ])
        new_zscale = max(0.0, zScale() + relyz);

    setScale(new_xscale, new_yscale, new_zscale);

    if (kseq == d->m_zoomKState[ 0 ])
        setZoom(max(0.0, zoom() - relyz));
    if (kseq == d->m_zoomKState[ 1 ])
        setZoom(max(0.0, zoom() + relyz));
}

void Plot3D::setShiftKeyboard(Qwt3DKeyboardState kseq, double speed)
{
    QWT_D(d);
    // Shift
    double w = max(1, width());
    double h = max(1, height());

    double relx  = speed / w;
    double relyz = speed / h;

    double new_xshift = xViewportShift();
    double new_yshift = yViewportShift();

    if (kseq == d->m_xshiftKState[ 0 ])
        new_xshift = xViewportShift() + relx;
    if (kseq == d->m_xshiftKState[ 1 ])
        new_xshift = xViewportShift() - relx;
    if (kseq == d->m_yshiftKState[ 0 ])
        new_yshift = yViewportShift() - relyz;
    if (kseq == d->m_yshiftKState[ 1 ])
        new_yshift = yViewportShift() + relyz;

    setViewportShift(new_xshift, new_yshift);
}

/**
        Sets the keybutton combination for data/coordinatesystem moves inside the widget\n\n
        default behaviour:\n

        @verbatim
        rotate around x axis: [Key_Down, Key_Up]
        rotate around y axis: SHIFT+[Key_Right, Key_Left]
        rotate around z axis: [Key_Right, Key_Left]
        scale x:              ALT+[Key_Right, Key_Left]
        scale y:              ALT+[Key_Up, Key_Down]
        scale z:              ALT+SHIFT[Key_Down, Key_Up]
        zoom:                 ALT+CTRL+[Key_Down, Key_Up]
        shifting along x:     CTRL+[Key_Right, Key_Left]
        shifting along z:     CTRL+[Key_Down, Key_Up]
        @endverbatim
*/
void Plot3D::assignKeyboard(Qwt3DKeyboardState xrot_n,
                            Qwt3DKeyboardState xrot_p,
                            Qwt3DKeyboardState yrot_n,
                            Qwt3DKeyboardState yrot_p,
                            Qwt3DKeyboardState zrot_n,
                            Qwt3DKeyboardState zrot_p,
                            Qwt3DKeyboardState xscale_n,
                            Qwt3DKeyboardState xscale_p,
                            Qwt3DKeyboardState yscale_n,
                            Qwt3DKeyboardState yscale_p,
                            Qwt3DKeyboardState zscale_n,
                            Qwt3DKeyboardState zscale_p,
                            Qwt3DKeyboardState zoom_n,
                            Qwt3DKeyboardState zoom_p,
                            Qwt3DKeyboardState xshift_n,
                            Qwt3DKeyboardState xshift_p,
                            Qwt3DKeyboardState yshift_n,
                            Qwt3DKeyboardState yshift_p)
{
    QWT_D(d);
    d->m_xrotKState[ 0 ] = xrot_n;
    d->m_yrotKState[ 0 ] = yrot_n;
    d->m_zrotKState[ 0 ] = zrot_n;
    d->m_xrotKState[ 1 ] = xrot_p;
    d->m_yrotKState[ 1 ] = yrot_p;
    d->m_zrotKState[ 1 ] = zrot_p;

    d->m_xscaleKState[ 0 ] = xscale_n;
    d->m_yscaleKState[ 0 ] = yscale_n;
    d->m_zscaleKState[ 0 ] = zscale_n;
    d->m_xscaleKState[ 1 ] = xscale_p;
    d->m_yscaleKState[ 1 ] = yscale_p;
    d->m_zscaleKState[ 1 ] = zscale_p;

    d->m_zoomKState[ 0 ]   = zoom_n;
    d->m_xshiftKState[ 0 ] = xshift_n;
    d->m_yshiftKState[ 0 ] = yshift_n;
    d->m_zoomKState[ 1 ]   = zoom_p;
    d->m_xshiftKState[ 1 ] = xshift_p;
    d->m_yshiftKState[ 1 ] = yshift_p;
}

/**
The function has no effect if you derive from Plot3D and overrides the keyboard Functions too
careless. In this case check first against keyboardEnabled() in your version of keyPressEvent() A
more fine grained input control can be achieved by combining assignKeyboard() with enableKeyboard().
*/
void Plot3D::enableKeyboard(bool val)
{
    QWT_D(d);
    d->m_kbdInputEnabled = val;
}

/**
@see enableKeyboard()
*/
void Plot3D::disableKeyboard(bool val)
{
    QWT_D(d);
    d->m_kbdInputEnabled = !val;
}

bool Plot3D::keyboardEnabled() const
{
    QWT_DC(d);
    return d->m_kbdInputEnabled;
}

/**
Values < 0 are ignored. Default is (3,5,5)
*/
void Plot3D::setKeySpeed(double rot, double scale, double shift)
{
    QWT_D(d);
    if (rot > 0)
        d->m_kbdRotSpeed = rot;
    if (scale > 0)
        d->m_kbdScaleSpeed = scale;
    if (shift > 0)
        d->m_kbdShiftSpeed = shift;
}

void Plot3D::keySpeed(double& rot, double& scale, double& shift) const
{
    QWT_DC(d);
    rot   = d->m_kbdRotSpeed;
    scale = d->m_kbdScaleSpeed;
    shift = d->m_kbdShiftSpeed;
}
