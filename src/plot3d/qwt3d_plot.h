#ifndef QWT3D_PLOT_H
#define QWT3D_PLOT_H

#include <QOpenGLWidget>

#include "qwt3d_coordsys.h"
#include "qwt3d_theme.h"

class Qwt3DPlotItem;

/**
 * @brief Pure rendering window for 3D plots
 * @details Qwt3DPlot is the central rendering window (a QOpenGLWidget subclass).
 *          It manages GL context, view transformation, lighting, coordinate system,
 *          legend, title, and mouse/keyboard interaction. It does NOT hold any
 *          plotting data — data is managed by Qwt3DPlotItem subclasses that are
 *          attached via attach().
 *
 * The rendering pipeline in paintGL():
 *   1. Clear color + depth buffer
 *   2. Calculate view/projection matrices on CPU (QMatrix4x4)
 *   3. For each attached item (sorted by z): item->draw()
 *   4. Draw coordinate system (legacy GL temporarily)
 *   5. Draw legend and title
 *
 * @code
 * auto plot = new Qwt3DPlot(parent);
 * auto item = new MySurfaceItem();
 * item->attach(plot);
 * @endcode
 */
class QWT3D_EXPORT Qwt3DPlot : public QOpenGLWidget
{
    Q_OBJECT

public:
    // Constructor
    Qwt3DPlot(QWidget* parent = nullptr);
    // Destructor
    ~Qwt3DPlot() override;

    // Render to pixmap
    QPixmap renderPixmap(int w = 0, int h = 0, bool useContext = false);
    // Create coordinate system between two points
    void createCoordinateSystem(Triple beg, Triple end);
    // Returns pointer to CoordinateSystem object
    Qwt3DCoordinateSystem* coordinates();
    // Returns pointer to ColorLegend object
    Qwt3DColorLegend* legend();

    // Returns rotation around X axis [-360..360] (some angles are equivalent)
    double xRotation() const;
    // Returns rotation around Y axis [-360..360] (some angles are equivalent)
    double yRotation() const;
    // Returns rotation around Z axis [-360..360] (some angles are equivalent)
    double zRotation() const;

    // Returns shift along X axis (object coordinates)
    double xShift() const;
    // Returns shift along Y axis (object coordinates)
    double yShift() const;
    // Returns shift along Z axis (object coordinates)
    double zShift() const;

    // Returns relative shift [-1..1] along X axis (view coordinates)
    double xViewportShift() const;
    // Returns relative shift [-1..1] along Y axis (view coordinates)
    double yViewportShift() const;

    // Returns scaling for X values [0..inf]
    double xScale() const;
    // Returns scaling for Y values [0..inf]
    double yScale() const;
    // Returns scaling for Z values [0..inf]
    double zScale() const;

    // Returns zoom (0..inf)
    double zoom() const;

    // Returns orthogonal (true) or perspective (false) projection
    bool ortho() const;

    // Sets widgets background color
    void setBackgroundColor(RGBA rgba);
    // Returns the widgets background color
    RGBA backgroundRGBAColor() const;

    // Returns rectangular hull (union of all attached items' hulls)
    ParallelEpiped hull() const;

    // Show/hide color legend
    void showColorLegend(bool);

    // Set title position
    void setTitlePosition(double rely, double relx = 0.5, ANCHOR = TopCenter);
    // Set title font
    void setTitleFont(const QString& family, int pointSize, int weight = QFont::Normal, bool italic = false);
    // Set caption color
    void setTitleColor(RGBA col);
    // Set caption text (one row only)
    void setTitle(const QString& title);

    // Set and apply a theme
    void setTheme(const Qwt3DTheme& theme);
    // Get the current theme
    Qwt3DTheme theme() const;

    // Apply a theme preset by enum
    void applyTheme(Qwt3DTheme::Preset preset);
    // Apply a theme preset by name
    void applyTheme(const QString& presetName);

    // Assign mouse states for rotations, scales, zoom and shifts
    void assignMouse(Qwt3DMouseState xrot,
                     Qwt3DMouseState yrot,
                     Qwt3DMouseState zrot,
                     Qwt3DMouseState xscale,
                     Qwt3DMouseState yscale,
                     Qwt3DMouseState zscale,
                     Qwt3DMouseState zoom,
                     Qwt3DMouseState xshift,
                     Qwt3DMouseState yshift);

    // Returns true, if the widget accept mouse input from the user
    bool mouseEnabled() const;
    // Assign keyboard states for rotations, scales, zoom and shifts
    void assignKeyboard(Qwt3DKeyboardState xrot_n,
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
                        Qwt3DKeyboardState yshift_p);

    // Returns true, if the widget accept keyboard input from the user
    bool keyboardEnabled() const;
    // Sets speed for keyboard driven transformations
    void setKeySpeed(double rot, double scale, double shift);
    // Gets speed for keyboard driven transformations
    void keySpeed(double& rot, double& scale, double& shift) const;

    // Returns true, if Lighting is enabled, false else
    bool lightingEnabled() const;
    // Turn light on
    void illuminate(unsigned light = 0);
    // Turn light off
    void blowout(unsigned light = 0);

    // Set material component (RGBA)
    void setMaterialComponent(GLenum property, double r, double g, double b, double a = 1.0);
    // Set material component (intensity)
    void setMaterialComponent(GLenum property, double intensity);
    // Set shininess exponent
    void setShininess(double exponent);
    // Set light component (RGBA)
    void setLightComponent(GLenum property, double r, double g, double b, double a = 1.0, unsigned light = 0);
    // Set light component (intensity)
    void setLightComponent(GLenum property, double intensity, unsigned light = 0);

    // Returns Light 'idx' rotation around X axis [-360..360] (some angles are equivalent)
    double xLightRotation(unsigned idx = 0) const;
    // Returns Light 'idx' rotation around Y axis [-360..360] (some angles are equivalent)
    double yLightRotation(unsigned idx = 0) const;
    // Returns Light 'idx' rotation around Z axis [-360..360] (some angles are equivalent)
    double zLightRotation(unsigned idx = 0) const;

    // Returns shift of Light 'idx' along X axis (object coordinates)
    double xLightShift(unsigned idx = 0) const;
    // Returns shift of Light 'idx' along Y axis (object coordinates)
    double yLightShift(unsigned idx = 0) const;
    // Returns shift of Light 'idx' along Z axis (object coordinates)
    double zLightShift(unsigned idx = 0) const;

    // Returns true if the plot has attached items
    bool hasItems() const;

    // Item list management
    void attach(Qwt3DPlotItem* item);
    void detach(Qwt3DPlotItem* item);
    const QList< Qwt3DPlotItem* >& itemList() const;

    // Internal: called by Qwt3DPlotItem to request a redraw
    void itemChanged(Qwt3DPlotItem* item);

Q_SIGNALS:

    /**
     * @brief Signal emitted when the rotation is changed
     * @param xAngle X axis rotation angle
     * @param yAngle Y axis rotation angle
     * @param zAngle Z axis rotation angle
     */
    void rotationChanged(double xAngle, double yAngle, double zAngle);

    /**
     * @brief Signal emitted when the shift is changed
     * @param xShift X axis shift value
     * @param yShift Y axis shift value
     * @param zShift Z axis shift value
     */
    void shiftChanged(double xShift, double yShift, double zShift);

    /**
     * @brief Signal emitted when the viewport shift is changed
     * @param xShift X viewport shift value
     * @param yShift Y viewport shift value
     */
    void vieportShiftChanged(double xShift, double yShift);

    /**
     * @brief Signal emitted when the scaling is changed
     * @param xScale X axis scale factor
     * @param yScale Y axis scale factor
     * @param zScale Z axis scale factor
     */
    void scaleChanged(double xScale, double yScale, double zScale);

    /**
     * @brief Signal emitted when the zoom is changed
     * @param zoom Zoom factor
     */
    void zoomChanged(double);

    /**
     * @brief Signal emitted when the projection mode is changed
     * @param ortho True for orthogonal, false for perspective
     */
    void projectionChanged(bool);

public Q_SLOTS:

    // Set rotation values
    void setRotation(double xVal, double yVal, double zVal);
    // Set shift values
    void setShift(double xVal, double yVal, double zVal);
    // Set viewport shift values
    void setViewportShift(double xVal, double yVal);
    // Set scale values
    void setScale(double xVal, double yVal, double zVal);
    // Set zoom value
    void setZoom(double);

    // Set orthogonal/perspective projection
    void setOrtho(bool);

    // Enable mouse input
    void enableMouse(bool val = true);
    // Disable mouse input
    void disableMouse(bool val = true);
    // Enable keyboard input
    void enableKeyboard(bool val = true);
    // Disable keyboard input
    void disableKeyboard(bool val = true);

    // Turn Lighting on or off
    void enableLighting(bool val = true);
    // Turn Lighting on or off
    void disableLighting(bool val = true);

    // Set light rotation
    void setLightRotation(double xVal, double yVal, double zVal, unsigned int idx = 0);
    // Set light shift
    void setLightShift(double xVal, double yVal, double zVal, unsigned int idx = 0);

    // Saves content to pixmap format
    virtual bool savePixmap(QString const& fileName, QString const& format);
    // Saves content to vector format
    virtual bool saveVector(QString const& fileName,
                            QString const& format,
                            VectorWriter::TEXTMODE text,
                            VectorWriter::SORTMODE sortmode);
    // Saves content
    virtual bool save(QString const& fileName, QString const& format);

protected:
    QWT_DECLARE_PRIVATE(Qwt3DPlot)

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

    void keyPressEvent(QKeyEvent* e) override;

    bool initializedGL() const;

private:
    void setRotationMouse(Qwt3DMouseState bstate, double accel, QPoint diff);
    void setScaleMouse(Qwt3DMouseState bstate, double accel, QPoint diff);
    void setShiftMouse(Qwt3DMouseState bstate, double accel, QPoint diff);

    void setRotationKeyboard(Qwt3DKeyboardState kseq, double speed);
    void setScaleKeyboard(Qwt3DKeyboardState kseq, double speed);
    void setShiftKeyboard(Qwt3DKeyboardState kseq, double speed);

    void applyLight(unsigned idx);
    void applyLights();
};


#endif  // QWT3D_PLOT_H
