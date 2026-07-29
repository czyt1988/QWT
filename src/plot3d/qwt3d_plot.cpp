#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4305)
#pragma warning(disable : 4786)
#endif

#include "qwt3d_plot_p.h"
#include "qwt3d_plotitem.h"

#include <algorithm>
#include <cfloat>
#include <cmath>


Qwt3DPlot::PrivateData::PrivateData(Qwt3DPlot* q)
    : q_ptr(q)
    , m_coordinates(Triple(0, 0, 0), Triple(0, 0, 0))
    , m_xRot(0.0)
    , m_yRot(0.0)
    , m_zRot(0.0)
    , m_xShift(0.0)
    , m_yShift(0.0)
    , m_zShift(0.0)
    , m_zoom(1.0)
    , m_xScale(1.0)
    , m_yScale(1.0)
    , m_zScale(1.0)
    , m_xVPShift(0.0)
    , m_yVPShift(0.0)
    , m_bgColor(RGBA(1.0, 1.0, 1.0, 1.0))
    , m_ortho(true)
    , m_displayLegend(false)
    , m_titleAnchor(TopCenter)
    , m_lastMouseMovePosition(0, 0)
    , m_pressed(false)
    , m_mouseInputEnabled(true)
    , m_kPressed(false)
    , m_kbdInputEnabled(true)
    , m_kbdRotSpeed(3.0)
    , m_kbdScaleSpeed(5.0)
    , m_kbdShiftSpeed(5.0)
    , m_lightingEnabled(false)
    , m_initializedGL(false)
    , m_renderPixmapRequest(false)
{
    m_lights.resize(8);
}

/**
 * @brief Constructs a Qwt3DPlot widget
 * @param parent Parent widget
 */
Qwt3DPlot::Qwt3DPlot(QWidget* parent) : QOpenGLWidget(parent), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);

    d->m_title.setFont("Courier", 16, QFont::Bold);
    d->m_title.setString("");

    setTitlePosition(0.95);

    setFocusPolicy(Qt::StrongFocus);
    assignMouse(Qt::LeftButton,
                Qwt3DMouseState(Qt::LeftButton, Qt::ShiftModifier),
                Qt::LeftButton,
                Qwt3DMouseState(Qt::LeftButton, Qt::AltModifier),
                Qwt3DMouseState(Qt::LeftButton, Qt::AltModifier),
                Qwt3DMouseState(Qt::LeftButton, Qt::AltModifier | Qt::ShiftModifier),
                Qwt3DMouseState(Qt::LeftButton, Qt::AltModifier | Qt::ControlModifier),
                Qwt3DMouseState(Qt::LeftButton, Qt::ControlModifier),
                Qwt3DMouseState(Qt::LeftButton, Qt::ControlModifier));

    assignKeyboard(Qt::Key_Down,
                   Qt::Key_Up,
                   Qwt3DKeyboardState(Qt::Key_Right, Qt::ShiftModifier),
                   Qwt3DKeyboardState(Qt::Key_Left, Qt::ShiftModifier),
                   Qt::Key_Right,
                   Qt::Key_Left,
                   Qwt3DKeyboardState(Qt::Key_Right, Qt::AltModifier),
                   Qwt3DKeyboardState(Qt::Key_Left, Qt::AltModifier),
                   Qwt3DKeyboardState(Qt::Key_Down, Qt::AltModifier),
                   Qwt3DKeyboardState(Qt::Key_Up, Qt::AltModifier),
                   Qwt3DKeyboardState(Qt::Key_Down, Qt::AltModifier | Qt::ShiftModifier),
                   Qwt3DKeyboardState(Qt::Key_Up, Qt::AltModifier | Qt::ShiftModifier),
                   Qwt3DKeyboardState(Qt::Key_Down, Qt::AltModifier | Qt::ControlModifier),
                   Qwt3DKeyboardState(Qt::Key_Up, Qt::AltModifier | Qt::ControlModifier),
                   Qwt3DKeyboardState(Qt::Key_Right, Qt::ControlModifier),
                   Qwt3DKeyboardState(Qt::Key_Left, Qt::ControlModifier),
                   Qwt3DKeyboardState(Qt::Key_Down, Qt::ControlModifier),
                   Qwt3DKeyboardState(Qt::Key_Up, Qt::ControlModifier));
    setKeySpeed(3, 5, 5);

    d->m_legend.setLimits(0, 100);
    d->m_legend.setMajors(10);
    d->m_legend.setMinors(2);
    d->m_legend.setOrientation(Qwt3DColorLegend::BottomTop, Qwt3DColorLegend::Left);

    disableLighting();
}

/**
 * @brief Destructor
 */
Qwt3DPlot::~Qwt3DPlot()
{
    // Detach all items (do not delete them — items own their lifetime)
    QWT_D(d);
    while (!d->m_items.isEmpty())
        d->m_items.first()->detach();
}

// Inline getter/setter implementations

Qwt3DCoordinateSystem* Qwt3DPlot::coordinates()
{
    QWT_D(d);
    return &d->m_coordinates;
}

Qwt3DColorLegend* Qwt3DPlot::legend()
{
    QWT_D(d);
    return &d->m_legend;
}

double Qwt3DPlot::xRotation() const
{
    QWT_DC(d);
    return d->m_xRot;
}

double Qwt3DPlot::yRotation() const
{
    QWT_DC(d);
    return d->m_yRot;
}

double Qwt3DPlot::zRotation() const
{
    QWT_DC(d);
    return d->m_zRot;
}

double Qwt3DPlot::xShift() const
{
    QWT_DC(d);
    return d->m_xShift;
}

double Qwt3DPlot::yShift() const
{
    QWT_DC(d);
    return d->m_yShift;
}

double Qwt3DPlot::zShift() const
{
    QWT_DC(d);
    return d->m_zShift;
}

double Qwt3DPlot::xViewportShift() const
{
    QWT_DC(d);
    return d->m_xVPShift;
}

double Qwt3DPlot::yViewportShift() const
{
    QWT_DC(d);
    return d->m_yVPShift;
}

double Qwt3DPlot::xScale() const
{
    QWT_DC(d);
    return d->m_xScale;
}

double Qwt3DPlot::yScale() const
{
    QWT_DC(d);
    return d->m_yScale;
}

double Qwt3DPlot::zScale() const
{
    QWT_DC(d);
    return d->m_zScale;
}

double Qwt3DPlot::zoom() const
{
    QWT_DC(d);
    return d->m_zoom;
}

bool Qwt3DPlot::ortho() const
{
    QWT_DC(d);
    return d->m_ortho;
}

ASPECTRATIOMODE Qwt3DPlot::aspectRatioMode() const
{
    QWT_DC(d);
    return d->m_aspectRatioMode;
}

RGBA Qwt3DPlot::backgroundRGBAColor() const
{
    QWT_DC(d);
    return d->m_bgColor;
}

ParallelEpiped Qwt3DPlot::hull() const
{
    QWT_DC(d);
    return d->m_hull;
}

void Qwt3DPlot::setTitleColor(RGBA col)
{
    QWT_D(d);
    d->m_title.setColor(col);
}

void Qwt3DPlot::setTitle(const QString& title)
{
    QWT_D(d);
    d->m_title.setString(title);
}

void Qwt3DPlot::setTheme(const Qwt3DTheme& theme)
{
    QWT_D(d);
    d->m_theme = theme;
    theme.apply(this);
}

Qwt3DTheme Qwt3DPlot::theme() const
{
    QWT_DC(d);
    return d->m_theme;
}

void Qwt3DPlot::applyTheme(Qwt3DTheme::Preset preset)
{
    setTheme(Qwt3DTheme::create(preset));
}

void Qwt3DPlot::applyTheme(const QString& presetName)
{
    setTheme(Qwt3DTheme::create(presetName));
}

double Qwt3DPlot::xLightRotation(unsigned idx) const
{
    QWT_DC(d);
    return (idx < 8) ? d->m_lights[ idx ].rot.x : 0;
}

double Qwt3DPlot::yLightRotation(unsigned idx) const
{
    QWT_DC(d);
    return (idx < 8) ? d->m_lights[ idx ].rot.y : 0;
}

double Qwt3DPlot::zLightRotation(unsigned idx) const
{
    QWT_DC(d);
    return (idx < 8) ? d->m_lights[ idx ].rot.z : 0;
}

double Qwt3DPlot::xLightShift(unsigned idx) const
{
    QWT_DC(d);
    return (idx < 8) ? d->m_lights[ idx ].shift.x : 0;
}

double Qwt3DPlot::yLightShift(unsigned idx) const
{
    QWT_DC(d);
    return (idx < 8) ? d->m_lights[ idx ].shift.y : 0;
}

double Qwt3DPlot::zLightShift(unsigned idx) const
{
    QWT_DC(d);
    return (idx < 8) ? d->m_lights[ idx ].shift.z : 0;
}

bool Qwt3DPlot::hasItems() const
{
    QWT_DC(d);
    return !d->m_items.isEmpty();
}

bool Qwt3DPlot::initializedGL() const
{
    QWT_DC(d);
    return d->m_initializedGL;
}

/**
 * @brief Returns the current model-view matrix
 * @return The model-view matrix computed during the last paintGL() call
 * @details Items use this matrix in their shader uniforms to transform
 *          vertex positions from model space to view space.
 */
QMatrix4x4 Qwt3DPlot::modelViewMatrix() const
{
    QWT_DC(d);
    return d->m_modelView;
}

/**
 * @brief Returns the current projection matrix
 * @return The projection matrix computed during the last paintGL() call
 * @details Items use this matrix in their shader uniforms to transform
 *          vertex positions from view space to clip space.
 */
QMatrix4x4 Qwt3DPlot::projectionMatrix() const
{
    QWT_DC(d);
    return d->m_projection;
}

/**
 * @brief Sets up the OpenGL rendering state
 * @details Compiles shared GLSL shaders (line, point, polygon, text) for use
 *          by drawables and items. Enables blend and depth test.
 */
void Qwt3DPlot::initializeGL()
{
    QWT_D(d);

    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    // Compile shared generic shaders
    auto compileShader = [](const QString& vertPath, const QString& fragPath) -> std::unique_ptr< QOpenGLShaderProgram > {
        auto program = std::make_unique< QOpenGLShaderProgram >();
        if (!program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertPath)) {
            qWarning("Failed to compile vertex shader %s: %s", vertPath.toLatin1().constData(),
                     program->log().toLatin1().constData());
            return nullptr;
        }
        if (!program->addShaderFromSourceFile(QOpenGLShader::Fragment, fragPath)) {
            qWarning("Failed to compile fragment shader %s: %s", fragPath.toLatin1().constData(),
                     program->log().toLatin1().constData());
            return nullptr;
        }
        if (!program->link()) {
            qWarning("Failed to link shader program: %s", program->log().toLatin1().constData());
            return nullptr;
        }
        return program;
    };

    d->m_lineShader = compileShader(":/shaders/line.vert", ":/shaders/line.frag");
    d->m_pointShader = compileShader(":/shaders/point.vert", ":/shaders/point.frag");
    d->m_polygonShader = compileShader(":/shaders/polygon.vert", ":/shaders/polygon.frag");
    d->m_textShader = compileShader(":/shaders/text.vert", ":/shaders/text.frag");

    d->m_initializedGL = true;
    if (d->m_renderPixmapRequest) {
        update();
        d->m_renderPixmapRequest = false;
    }
}

/**
 * @brief Paints the widget's content
 * @details Uses CPU-side QMatrix4x4 for view/projection calculation.
 *          All rendering uses VBO/VAO + GLSL shaders.
 */
void Qwt3DPlot::paintGL()
{
    QWT_D(d);

    glClearColor(d->m_bgColor.r, d->m_bgColor.g, d->m_bgColor.b, d->m_bgColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Calculate view transform on CPU (no glRotatef/glTranslatef/glPushMatrix)
    Triple beg = d->m_coordinates.first();
    Triple end = d->m_coordinates.second();
    Triple center = beg + (end - beg) / 2;
    Triple dv = end - beg;

    double normX = 1.0, normY = 1.0, normZ = 1.0;
    double radius;

    if (d->m_aspectRatioMode == AUTOFILL) {
        double maxRange = std::max({dv.x, dv.y, dv.z});
        if (maxRange > 0) {
            normX = (dv.x > 0) ? maxRange / dv.x : 1.0;
            normY = (dv.y > 0) ? maxRange / dv.y : 1.0;
            normZ = (dv.z > 0) ? maxRange / dv.z : 1.0;
        }
        radius = (maxRange > 0) ? maxRange * sqrt(3.0) / 2.0 : 1.0;
    } else {
        radius = (center - beg).length();
    }

    QMatrix4x4 modelView;
    modelView.setToIdentity();
    modelView.rotate(d->m_xRot - 90, 1.0f, 0.0f, 0.0f);
    modelView.rotate(d->m_yRot, 0.0f, 1.0f, 0.0f);
    modelView.rotate(d->m_zRot, 0.0f, 0.0f, 1.0f);
    modelView.scale(static_cast< float >(d->m_zoom * d->m_xScale * normX),
                    static_cast< float >(d->m_zoom * d->m_yScale * normY),
                    static_cast< float >(d->m_zoom * d->m_zScale * normZ));
    modelView.translate(static_cast< float >(d->m_xShift - center.x),
                         static_cast< float >(d->m_yShift - center.y),
                         static_cast< float >(d->m_zShift - center.z));
    d->m_modelView = modelView;

    QMatrix4x4 projection;
    projection.setToIdentity();
    if (beg != end) {
        if (d->m_ortho) {
            projection.ortho(-radius, +radius, -radius, +radius, 0, 40 * radius);
        } else {
            projection.frustum(-radius, +radius, -radius, +radius, 5 * radius, 400 * radius);
        }
    } else {
        if (d->m_ortho)
            projection.ortho(-1.0, 1.0, -1.0, 1.0, 10.0, 100.0);
        else
            projection.frustum(-1.0, 1.0, -1.0, 1.0, 10.0, 100.0);
    }
    projection.translate(static_cast< float >(d->m_xVPShift * 2 * radius),
                          static_cast< float >(d->m_yVPShift * 2 * radius),
                          static_cast< float >(-7 * radius));
    d->m_projection = projection;

    // Build render context for the user-rotation phase (items + coordinate system)
    Qwt3DRenderContext ctx;
    ctx.modelView = d->m_modelView;
    ctx.projection = d->m_projection;
    ctx.viewport = viewportSize();
    ctx.lineShader = d->m_lineShader.get();
    ctx.polygonShader = d->m_polygonShader.get();
    ctx.textShader = d->m_textShader.get();

    // Render all attached items (sorted by z-order)
    for (Qwt3DPlotItem* item : d->m_items) {
        if (item->isVisible())
            item->draw();
    }

    // Draw coordinate system
    d->m_coordinates.draw(ctx);

    // Draw legend and title with a FIXED modelview (no user rotation)
    // so they stay anchored to the screen regardless of 3D scene rotation.
    // This matches the original libqwtplot3d behavior where legend/title
    // were drawn before user rotation was applied to the modelview matrix.
    QMatrix4x4 savedModelView = d->m_modelView;
    d->m_modelView.setToIdentity();
    d->m_modelView.rotate(-90.0f, 1.0f, 0.0f, 0.0f);

    // Build context for the fixed-MV phase (legend + title)
    Qwt3DRenderContext ctxFixed = ctx;
    ctxFixed.modelView = d->m_modelView;

    if (d->m_displayLegend) {
        for (Qwt3DPlotItem* item : d->m_items)
            item->populateLegendColors(d->m_legend.colors);
        d->m_legend.draw(ctxFixed);
    }

    d->m_title.setRelPosition(d->m_titleRel, d->m_titleAnchor, ctxFixed);
    d->m_title.draw(ctxFixed);

    // Restore user modelview for subsequent operations (mouse picking, etc.)
    d->m_modelView = savedModelView;
}

/**
 * @brief Sets up the OpenGL view port
 * @param w New width
 * @param h New height
 */
void Qwt3DPlot::resizeGL(int w, int h)
{
    QWT_D(d);
    d->m_viewportWidth = w;
    d->m_viewportHeight = h;
    glViewport(0, 0, w, h);
    paintGL();
}

/**
 * @brief Creates a coordinate system with generating corners beg and end
 * @param beg Minimum vertex of the coordinate system
 * @param end Maximum vertex of the coordinate system
 */
void Qwt3DPlot::createCoordinateSystem(Triple beg, Triple end)
{
    QWT_D(d);
    if (beg != d->m_coordinates.first() || end != d->m_coordinates.second())
        d->m_coordinates.init(beg, end);
}

/**
 * @brief Shows or hides the color legend
 * @param show True to show, false to hide
 * @details The color legend colors are populated from attached items'
 *          dataColor functors during paintGL.
 */
void Qwt3DPlot::showColorLegend(bool show)
{
    QWT_D(d);
    d->m_displayLegend = show;
    update();
}

void Qwt3DPlot::setLegendPosition(Qwt3DColorLegend::Position pos)
{
    QWT_D(d);
    d->m_legend.setPosition(pos);
    update();
}

void Qwt3DPlot::setLegendAbsolutePosition(const QRectF& pixelRect)
{
    QWT_D(d);
    d->m_legend.setAbsolutePosition(pixelRect);
    update();
}

/**
 * @brief Sets the background color
 * @param rgba Background color as RGBA value
 */
void Qwt3DPlot::setBackgroundColor(RGBA rgba)
{
    QWT_D(d);
    d->m_bgColor = rgba;
}

/**
 * @brief Sets up orthogonal or perspective mode and updates widget
 * @param val True for orthogonal projection, false for perspective
 */
void Qwt3DPlot::setOrtho(bool val)
{
    QWT_D(d);
    if (val == d->m_ortho)
        return;
    d->m_ortho = val;
    update();

    emit projectionChanged(val);
}

/**
 * @brief Sets the aspect ratio mode for the 3D coordinate box
 * @param mode AUTOFILL to independently scale each axis to fill the viewport,
 *             DATARATIO to preserve original data proportions
 * @details In AUTOFILL mode (default), each axis is normalized to the same
 *          visual length so the coordinate box appears as a cube, making the
 *          plot fill the viewport. In DATARATIO mode, the original data
 *          proportions are preserved (equal aspect ratio), similar to
 *          matplotlib's plt.axis('equal').
 */
void Qwt3DPlot::setAspectRatioMode(ASPECTRATIOMODE mode)
{
    QWT_D(d);
    if (mode == d->m_aspectRatioMode)
        return;
    d->m_aspectRatioMode = mode;
    update();

    emit aspectRatioModeChanged(mode);
}

/**
 * @brief Sets relative caption position
 * @param rely Relative Y position (0-1). (0.5,0.5) means the anchor point lies in the center of the screen.
 * @param relx Relative X position (0-1)
 * @param anchor Anchor type for title alignment
 */
void Qwt3DPlot::setTitlePosition(double rely, double relx, ANCHOR anchor)
{
    QWT_D(d);
    d->m_titleRel.y = (rely < 0 || rely > 1) ? 0.5 : rely;
    d->m_titleRel.x = (relx < 0 || relx > 1) ? 0.5 : relx;

    d->m_titleAnchor = anchor;
}

/**
 * @brief Sets the caption font
 * @param family Font family name
 * @param pointSize Font point size
 * @param weight Font weight
 * @param italic Whether font is italic
 */
void Qwt3DPlot::setTitleFont(const QString& family, int pointSize, int weight, bool italic)
{
    QWT_D(d);
    d->m_title.setFont(family, pointSize, weight, italic);
}

/**
 * @brief Renders the plot to a pixmap
 * @param w Width (0 for default)
 * @param h Height (0 for default)
 * @param useContext Whether to use existing GL context
 * @return QPixmap of the rendered scene
 */
QPixmap Qwt3DPlot::renderPixmap(int w, int h, bool useContext)
{
    QWT_D(d);
    if (useContext && d->m_initializedGL) {
        return QPixmap::fromImage(grabFramebuffer());
    } else {
        d->m_renderPixmapRequest = true;
        return QPixmap::fromImage(grabFramebuffer());
    }
}

/**
 * @brief Converts a world coordinate to screen (viewport) coordinates
 * @param world World-space triple
 * @return Screen-space QPointF (pixel coordinates)
 * @details Uses the CPU-side modelView and projection matrices to transform
 *          world coordinates to normalized device coordinates, then maps
 *          to viewport pixels. Replaces the legacy gluProject call.
 */
QPointF Qwt3DPlot::worldToScreen(const Triple& world) const
{
    QWT_DC(d);
    QVector4D worldVec(static_cast< float >(world.x), static_cast< float >(world.y), static_cast< float >(world.z), 1.0f);
    QVector4D clipVec = d->m_projection.map(d->m_modelView.map(worldVec));

    if (clipVec.w() == 0.0f)
        return QPointF(0, 0);

    float ndcX = clipVec.x() / clipVec.w();
    float ndcY = clipVec.y() / clipVec.w();

    int w = (d->m_viewportWidth > 0) ? d->m_viewportWidth : width();
    int h = (d->m_viewportHeight > 0) ? d->m_viewportHeight : height();

    float screenX = (ndcX + 1.0f) * 0.5f * w;
    float screenY = (1.0f - (ndcY + 1.0f) * 0.5f) * h;

    return QPointF(screenX, screenY);
}

/**
 * @brief Converts screen (viewport) coordinates to a world coordinate
 * @param screen Screen-space point (pixel coordinates)
 * @return World-space Triple
 * @details Uses the inverse of the CPU-side modelView and projection matrices
 *          to unproject screen coordinates. The z-component is determined
 *          by the near plane (z=0 in NDC). Replaces the legacy gluUnProject call.
 */
Triple Qwt3DPlot::screenToWorld(const QPointF& screen) const
{
    QWT_DC(d);
    int w = (d->m_viewportWidth > 0) ? d->m_viewportWidth : width();
    int h = (d->m_viewportHeight > 0) ? d->m_viewportHeight : height();

    if (w <= 0 || h <= 0)
        return Triple(0, 0, 0);

    float ndcX = 2.0f * static_cast< float >(screen.x()) / w - 1.0f;
    float ndcY = 1.0f - 2.0f * static_cast< float >(screen.y()) / h;

    QVector3D clipVec(ndcX, ndcY, 0.0f);

    QMatrix4x4 invMVP = (d->m_projection * d->m_modelView).inverted();
    QVector3D worldVec = invMVP.map(clipVec);

    return Triple(worldVec.x(), worldVec.y(), worldVec.z());
}

/**
 * @brief Returns the viewport size in pixels
 */
QSize Qwt3DPlot::viewportSize() const
{
    QWT_DC(d);
    int w = (d->m_viewportWidth > 0) ? d->m_viewportWidth : width();
    int h = (d->m_viewportHeight > 0) ? d->m_viewportHeight : height();
    return QSize(w, h);
}

/**
 * @brief Returns the shared line shader program
 */
QOpenGLShaderProgram* Qwt3DPlot::lineShader() const
{
    QWT_DC(d);
    return d->m_lineShader.get();
}

/**
 * @brief Returns the shared point shader program
 */
QOpenGLShaderProgram* Qwt3DPlot::pointShader() const
{
    QWT_DC(d);
    return d->m_pointShader.get();
}

/**
 * @brief Returns the shared polygon shader program
 */
QOpenGLShaderProgram* Qwt3DPlot::polygonShader() const
{
    QWT_DC(d);
    return d->m_polygonShader.get();
}

/**
 * @brief Returns the shared text shader program
 */
QOpenGLShaderProgram* Qwt3DPlot::textShader() const
{
    QWT_DC(d);
    return d->m_textShader.get();
}

// --- Item list management ---

/**
 * @brief Attach a plot item to this plot
 * @param item Pointer to the item to attach
 * @details Inserts the item into the internal list and sorts by z-order.
 *          If the item is already attached, it is not duplicated.
 */
void Qwt3DPlot::attach(Qwt3DPlotItem* item)
{
    QWT_D(d);
    if (!item || d->m_items.contains(item))
        return;

    d->m_items.append(item);
    // Sort by z-order (ascending)
    std::sort(d->m_items.begin(), d->m_items.end(),
        [](const Qwt3DPlotItem* a, const Qwt3DPlotItem* b) {
            return a->z() < b->z();
        });

    update();
}

/**
 * @brief Detach a plot item from this plot
 * @param item Pointer to the item to detach
 * @details Removes the item from the internal list. Does not delete the item.
 */
void Qwt3DPlot::detach(Qwt3DPlotItem* item)
{
    QWT_D(d);
    if (!item)
        return;

    d->m_items.removeAll(item);
    update();
}

/**
 * @brief Returns the list of attached items (sorted by z-order)
 */
const QList< Qwt3DPlotItem* >& Qwt3DPlot::itemList() const
{
    QWT_DC(d);
    return d->m_items;
}

/**
 * @brief Called by Qwt3DPlotItem when its data or properties change
 * @param item The item that changed
 * @details Re-sorts the item list by z-order, recalculates the plot hull
 *          as the union of all items' hulls, updates the coordinate system,
 *          and triggers a repaint.
 */
void Qwt3DPlot::itemChanged(Qwt3DPlotItem*)
{
    QWT_D(d);
    // Re-sort by z-order
    std::sort(d->m_items.begin(), d->m_items.end(),
        [](const Qwt3DPlotItem* a, const Qwt3DPlotItem* b) {
            return a->z() < b->z();
        });

    // Recalculate hull from all items' hulls
    if (!d->m_items.isEmpty()) {
        Triple minV(DBL_MAX, DBL_MAX, DBL_MAX);
        Triple maxV(-DBL_MAX, -DBL_MAX, -DBL_MAX);
        for (const Qwt3DPlotItem* item : qwt_as_const(d->m_items)) {
            ParallelEpiped h = item->hull();
            minV.x = std::min(minV.x, h.minVertex.x);
            minV.y = std::min(minV.y, h.minVertex.y);
            minV.z = std::min(minV.z, h.minVertex.z);
            maxV.x = std::max(maxV.x, h.maxVertex.x);
            maxV.y = std::max(maxV.y, h.maxVertex.y);
            maxV.z = std::max(maxV.z, h.maxVertex.z);
        }
        d->m_hull = ParallelEpiped(minV, maxV);
        createCoordinateSystem(minV, maxV);
    }

    update();
}
