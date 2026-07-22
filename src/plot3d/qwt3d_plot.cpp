#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4305)
#pragma warning(disable : 4786)
#endif

#include "qwt3d_plot_p.h"
#include "qwt3d_enrichment.h"


Plot3D::PrivateData::PrivateData(Plot3D* q)
    : q_ptr(q)
    , m_coordinates(Triple(0, 0, 0), Triple(0, 0, 0))
    , m_dataColor(nullptr)
    , m_userPlotStyle(nullptr)
    , m_actualData(nullptr)
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
    , m_meshColor(RGBA(0.0, 0.0, 0.0))
    , m_meshLineWidth(1.0)
    , m_bgColor(RGBA(1.0, 1.0, 1.0, 1.0))
    , m_plotStyle(FILLEDMESH)
    , m_shading(GOURAUD)
    , m_floorStyle(NOFLOOR)
    , m_ortho(true)
    , m_polygonOffset(0.5)
    , m_isolines(10)
    , m_displayLegend(false)
    , m_smoothDataMesh(false)
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
    m_displayLists.resize(DisplayListSize, 0);
    m_lights.resize(8);
}

/**
 * @brief Constructs a Plot3D widget
 * @param parent Parent widget
 * @details This should be the first call in your derived classes constructors.
 */
Plot3D::Plot3D(QWidget* parent) : QOpenGLWidget(parent), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);

    d->m_dataColor = new StandardColor(this, 100);
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
 * @brief Destructor - releases allocated resources
 */
Plot3D::~Plot3D()
{
    QWT_D(d);
    makeCurrent();
    SaveGlDeleteLists(d->m_displayLists[ 0 ], static_cast< GLsizei >(d->m_displayLists.size()));
    d->m_dataColor->destroy();
    delete d->m_userPlotStyle;
    for (ELIT it = d->m_enrichmentList.begin(); it != d->m_enrichmentList.end(); ++it)
        delete (*it);

    d->m_enrichmentList.clear();
}

// Inline getter/setter implementations

Qwt3DCoordinateSystem* Plot3D::coordinates()
{
    QWT_D(d);
    return &d->m_coordinates;
}

Qwt3DColorLegend* Plot3D::legend()
{
    QWT_D(d);
    return &d->m_legend;
}

double Plot3D::xRotation() const
{
    QWT_DC(d);
    return d->m_xRot;
}

double Plot3D::yRotation() const
{
    QWT_DC(d);
    return d->m_yRot;
}

double Plot3D::zRotation() const
{
    QWT_DC(d);
    return d->m_zRot;
}

double Plot3D::xShift() const
{
    QWT_DC(d);
    return d->m_xShift;
}

double Plot3D::yShift() const
{
    QWT_DC(d);
    return d->m_yShift;
}

double Plot3D::zShift() const
{
    QWT_DC(d);
    return d->m_zShift;
}

double Plot3D::xViewportShift() const
{
    QWT_DC(d);
    return d->m_xVPShift;
}

double Plot3D::yViewportShift() const
{
    QWT_DC(d);
    return d->m_yVPShift;
}

double Plot3D::xScale() const
{
    QWT_DC(d);
    return d->m_xScale;
}

double Plot3D::yScale() const
{
    QWT_DC(d);
    return d->m_yScale;
}

double Plot3D::zScale() const
{
    QWT_DC(d);
    return d->m_zScale;
}

double Plot3D::zoom() const
{
    QWT_DC(d);
    return d->m_zoom;
}

bool Plot3D::ortho() const
{
    QWT_DC(d);
    return d->m_ortho;
}

PLOTSTYLE Plot3D::plotStyle() const
{
    QWT_DC(d);
    return d->m_plotStyle;
}

Enrichment* Plot3D::userStyle() const
{
    QWT_DC(d);
    return d->m_userPlotStyle;
}

SHADINGSTYLE Plot3D::shading() const
{
    QWT_DC(d);
    return d->m_shading;
}

int Plot3D::isolines() const
{
    QWT_DC(d);
    return d->m_isolines;
}

void Plot3D::setSmoothMesh(bool val)
{
    QWT_D(d);
    d->m_smoothDataMesh = val;
}

bool Plot3D::smoothDataMesh() const
{
    QWT_DC(d);
    return d->m_smoothDataMesh;
}

RGBA Plot3D::backgroundRGBAColor() const
{
    QWT_DC(d);
    return d->m_bgColor;
}

RGBA Plot3D::meshColor() const
{
    QWT_DC(d);
    return d->m_meshColor;
}

double Plot3D::meshLineWidth() const
{
    QWT_DC(d);
    return d->m_meshLineWidth;
}

const Color* Plot3D::dataColor() const
{
    QWT_DC(d);
    return d->m_dataColor;
}

ParallelEpiped Plot3D::hull() const
{
    QWT_DC(d);
    return d->m_hull;
}

double Plot3D::polygonOffset() const
{
    QWT_DC(d);
    return d->m_polygonOffset;
}

void Plot3D::setTitleColor(RGBA col)
{
    QWT_D(d);
    d->m_title.setColor(col);
}

void Plot3D::setTitle(const QString& title)
{
    QWT_D(d);
    d->m_title.setString(title);
}

void Plot3D::setTheme(const Qwt3DTheme& theme)
{
    QWT_D(d);
    d->m_theme = theme;
    theme.apply(this);
}

Qwt3DTheme Plot3D::theme() const
{
    QWT_DC(d);
    return d->m_theme;
}

void Plot3D::applyTheme(Qwt3DTheme::Preset preset)
{
    setTheme(Qwt3DTheme::create(preset));
}

void Plot3D::applyTheme(const QString& presetName)
{
    setTheme(Qwt3DTheme::create(presetName));
}

double Plot3D::xLightRotation(unsigned idx) const
{
    QWT_DC(d);
    return (idx < 8) ? d->m_lights[ idx ].rot.x : 0;
}

double Plot3D::yLightRotation(unsigned idx) const
{
    QWT_DC(d);
    return (idx < 8) ? d->m_lights[ idx ].rot.y : 0;
}

double Plot3D::zLightRotation(unsigned idx) const
{
    QWT_DC(d);
    return (idx < 8) ? d->m_lights[ idx ].rot.z : 0;
}

double Plot3D::xLightShift(unsigned idx) const
{
    QWT_DC(d);
    return (idx < 8) ? d->m_lights[ idx ].shift.x : 0;
}

double Plot3D::yLightShift(unsigned idx) const
{
    QWT_DC(d);
    return (idx < 8) ? d->m_lights[ idx ].shift.y : 0;
}

double Plot3D::zLightShift(unsigned idx) const
{
    QWT_DC(d);
    return (idx < 8) ? d->m_lights[ idx ].shift.z : 0;
}

bool Plot3D::hasData() const
{
    QWT_DC(d);
    return (d->m_actualData) ? !d->m_actualData->empty() : false;
}

bool Plot3D::initializedGL() const
{
    QWT_DC(d);
    return d->m_initializedGL;
}

void Plot3D::setHull(ParallelEpiped p)
{
    QWT_D(d);
    d->m_hull = p;
}

std::vector< GLuint >& Plot3D::displayLists()
{
    QWT_D(d);
    return d->m_displayLists;
}

Qwt3DData* Plot3D::actualData() const
{
    QWT_DC(d);
    return d->m_actualData;
}

void Plot3D::setActualData(Qwt3DData* data)
{
    QWT_D(d);
    d->m_actualData = data;
}

/**
 * @brief Sets up the OpenGL rendering state
 */
void Plot3D::initializeGL()
{
    QWT_D(d);

    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    // Set up the lights

    disableLighting();

    GLfloat whiteAmb[ 4 ] = { 1.0, 1.0, 1.0, 1.0 };

    setLightShift(0, 0, 3000);
    glEnable(GL_COLOR_MATERIAL);

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, whiteAmb);

    setMaterialComponent(GL_DIFFUSE, 1.0);
    setMaterialComponent(GL_SPECULAR, 0.3);
    setMaterialComponent(GL_SHININESS, 5.0);
    setLightComponent(GL_DIFFUSE, 1.0);
    setLightComponent(GL_SPECULAR, 1.0);

    d->m_initializedGL = true;
    if (d->m_renderPixmapRequest) {
        updateData();
        d->m_renderPixmapRequest = false;
    }
}

/**
 * @brief Paints the widget's content
 */
void Plot3D::paintGL()
{
    QWT_D(d);

    glClearColor(d->m_bgColor.r, d->m_bgColor.g, d->m_bgColor.b, d->m_bgColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    applyLights();

    glRotatef(-90, 1.0, 0.0, 0.0);
    glRotatef(0.0, 0.0, 1.0, 0.0);
    glRotatef(0.0, 0.0, 0.0, 1.0);

    if (d->m_displayLegend) {
        d->m_legend.draw();
    }
    d->m_title.setRelPosition(d->m_titleRel, d->m_titleAnchor);
    d->m_title.draw();

    Triple beg = d->m_coordinates.first();
    Triple end = d->m_coordinates.second();

    Triple center = beg + (end - beg) / 2;
    double radius = (center - beg).length();

    glLoadIdentity();

    glRotatef(d->m_xRot - 90, 1.0, 0.0, 0.0);
    glRotatef(d->m_yRot, 0.0, 1.0, 0.0);
    glRotatef(d->m_zRot, 0.0, 0.0, 1.0);

    glScalef(d->m_zoom * d->m_xScale, d->m_zoom * d->m_yScale, d->m_zoom * d->m_zScale);

    glTranslatef(d->m_xShift - center.x, d->m_yShift - center.y, d->m_zShift - center.z);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (beg != end) {
        if (d->m_ortho) {
            glOrtho(-radius, +radius, -radius, +radius, 0, 40 * radius);
        } else {
            glFrustum(-radius, +radius, -radius, +radius, 5 * radius, 400 * radius);
        }
    } else {
        if (d->m_ortho)
            glOrtho(-1.0, 1.0, -1.0, 1.0, 10.0, 100.0);
        else
            glFrustum(-1.0, 1.0, -1.0, 1.0, 10.0, 100.0);
    }

    glTranslatef(d->m_xVPShift * 2 * radius, d->m_yVPShift * 2 * radius, -7 * radius);

    if (d->m_lightingEnabled)
        glEnable(GL_NORMALIZE);

    for (unsigned i = 0; i != d->m_displayLists.size(); ++i) {
        if (i != LegendObject)
            glCallList(d->m_displayLists[ i ]);
    }
    d->m_coordinates.draw();

    if (d->m_lightingEnabled)
        glDisable(GL_NORMALIZE);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

/**
 * @brief Sets up the OpenGL view port
 * @param w New width
 * @param h New height
 */
void Plot3D::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    paintGL();
}

/**
 * @brief Creates a coordinate system with generating corners beg and end
 * @param beg Minimum vertex of the coordinate system
 * @param end Maximum vertex of the coordinate system
 */
void Plot3D::createCoordinateSystem(Triple beg, Triple end)
{
    QWT_D(d);
    if (beg != d->m_coordinates.first() || end != d->m_coordinates.second())
        d->m_coordinates.init(beg, end);
}

/**
 * @brief Creates a coordinate system from data
 * @details Calculates the hull first, then creates the coordinate system from hull boundaries.
 */
void Plot3D::createCoordinateSystem()
{
    calculateHull();
    Triple beg = hull().minVertex;  // Irix 6.5 compiler bug
    Triple end = hull().maxVertex;
    createCoordinateSystem(beg, end);
}

/**
 * @brief Shows or hides the color legend
 * @param show True to show, false to hide
 */
void Plot3D::showColorLegend(bool show)
{
    QWT_D(d);
    d->m_displayLegend = show;
    if (show)
        d->m_dataColor->createVector(d->m_legend.colors);
    update();
}

/**
 * @brief Sets the mesh color
 * @param rgba Mesh color as RGBA value
 */
void Plot3D::setMeshColor(RGBA rgba)
{
    QWT_D(d);
    d->m_meshColor = rgba;
}

/**
 * @brief Sets the background color
 * @param rgba Background color as RGBA value
 */
void Plot3D::setBackgroundColor(RGBA rgba)
{
    QWT_D(d);
    d->m_bgColor = rgba;
}

/**
 * @brief Assigns a new coloring object for the data
 * @param col Pointer to a new Color object
 */
void Plot3D::setDataColor(Color* col)
{
    QWT_D(d);
    Q_ASSERT(d->m_dataColor);

    d->m_dataColor->destroy();
    d->m_dataColor = col;

    if (d->m_displayLegend)
        d->m_dataColor->createVector(d->m_legend.colors);
}

/**
 * @brief Sets up orthogonal or perspective mode and updates widget
 * @param val True for orthogonal projection, false for perspective
 */
void Plot3D::setOrtho(bool val)
{
    QWT_D(d);
    if (val == d->m_ortho)
        return;
    d->m_ortho = val;
    update();

    emit projectionChanged(val);
}

/**
 * @brief Sets style of coordinate system
 * @param st Coordinate system style (NOCOORD, BOX, or FRAME)
 */
void Plot3D::setCoordinateStyle(COORDSTYLE st)
{
    QWT_D(d);
    d->m_coordinates.setStyle(st);
    update();
}

/**
 * @brief Sets plot style for the standard plotting types
 * @param val Plot style value. An argument of value USER is ignored.
 */
void Plot3D::setPlotStyle(PLOTSTYLE val)
{
    QWT_D(d);
    if (val == USER)
        return;
    delete d->m_userPlotStyle;
    d->m_userPlotStyle = nullptr;
    d->m_plotStyle     = val;
}

/**
 * @brief Sets plot style to USER with an associated enrichment object
 * @param obj Reference to an Enrichment object
 * @return Pointer to the cloned enrichment object
 */
Enrichment* Plot3D::setPlotStyle(Enrichment const& obj)
{
    QWT_D(d);
    if (&obj == d->m_userPlotStyle)
        return d->m_userPlotStyle;

    delete d->m_userPlotStyle;
    d->m_userPlotStyle = obj.clone();
    d->m_plotStyle     = USER;
    return d->m_userPlotStyle;
}

/**
 * @brief Sets shading style
 * @param val Shading style (FLAT or GOURAUD)
 */
void Plot3D::setShading(SHADINGSTYLE val)
{
    QWT_D(d);
    if (val == d->m_shading)
        return;

    d->m_shading = val;

    switch (d->m_shading) {
    case FLAT:
        glShadeModel(GL_FLAT);
        break;
    case GOURAUD:
        glShadeModel(GL_SMOOTH);
        break;
    default:
        break;
    }
    update();
}

/**
 * @brief Sets number of isolines
 * @param steps Number of isolines. The lines are equidistant between minimal and maximal Z value.
 */
void Plot3D::setIsolines(int steps)
{
    QWT_D(d);
    if (steps < 0)
        return;

    d->m_isolines = steps;
}

/**
 * @brief Sets polygon offset
 * @param val Polygon offset value
 * @details The function affects the OpenGL rendering process. Try different values
 *          for surfaces with polygons only and with mesh and polygons.
 */
void Plot3D::setPolygonOffset(double val)
{
    QWT_D(d);
    d->m_polygonOffset = val;
}

/**
 * @brief Sets the mesh line width
 * @param val Line width value (must be >= 0)
 */
void Plot3D::setMeshLineWidth(double val)
{
    QWT_D(d);
    Q_ASSERT(val >= 0);

    if (val < 0)
        return;

    d->m_meshLineWidth = val;
}

/**
 * @brief Sets relative caption position
 * @param rely Relative Y position (0-1). (0.5,0.5) means the anchor point lies in the center of the screen.
 * @param relx Relative X position (0-1)
 * @param anchor Anchor type for title alignment
 */
void Plot3D::setTitlePosition(double rely, double relx, ANCHOR anchor)
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
void Plot3D::setTitleFont(const QString& family, int pointSize, int weight, bool italic)
{
    QWT_D(d);
    d->m_title.setFont(family, pointSize, weight, italic);
}

/**
 * @brief Adds an enrichment object to the plot
 * @param e Reference to an Enrichment object
 * @return Pointer to the cloned enrichment object added to the list
 */
Enrichment* Plot3D::addEnrichment(Enrichment const& e)
{
    QWT_D(d);
    if (d->m_enrichmentList.end() == std::find(d->m_enrichmentList.begin(), d->m_enrichmentList.end(), &e))
        d->m_enrichmentList.push_back(e.clone());
    return d->m_enrichmentList.back();
}

/**
 * @brief Removes an enrichment object from the plot
 * @param e Pointer to the Enrichment object to remove
 * @return True if the enrichment was found and removed, false otherwise
 */
bool Plot3D::degrade(Enrichment* e)
{
    QWT_D(d);
    ELIT it = std::find(d->m_enrichmentList.begin(), d->m_enrichmentList.end(), e);

    if (it != d->m_enrichmentList.end()) {
        delete (*it);
        d->m_enrichmentList.erase(it);
        return true;
    }
    return false;
}

void Plot3D::createEnrichments()
{
    QWT_D(d);
    for (ELIT it = d->m_enrichmentList.begin(); it != d->m_enrichmentList.end(); ++it) {
        this->createEnrichment(**it);
    }
}

/**
 * @brief Updates OpenGL data representation
 */
void Plot3D::updateData()
{
    QWT_D(d);
    makeCurrent();
    GLStateBewarer dt(GL_DEPTH_TEST, true);
    GLStateBewarer ls(GL_LINE_SMOOTH, true);

    calculateHull();

    SaveGlDeleteLists(d->m_displayLists[ DataObject ], 1);  // data only

    d->m_displayLists[ DataObject ] = glGenLists(1);
    glNewList(d->m_displayLists[ DataObject ], GL_COMPILE);

    this->createEnrichments();
    this->createData();

    glEndList();
}
