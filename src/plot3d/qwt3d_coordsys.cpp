#include "qwt3d_coordsys.h"

using namespace std;

class Qwt3DCoordinateSystem::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DCoordinateSystem)

public:
    PrivateData(Qwt3DCoordinateSystem* p)
        : q_ptr(p)
        , m_style(BOX)
        , m_smooth(true)
        , m_autodecoration(true)
        , m_majorgridlines(false)
        , m_minorgridlines(false)
        , m_sides(0)
    {
    }

    Triple m_first, m_second;
    COORDSTYLE m_style;
    RGBA m_gridlinecolor;
    bool m_smooth;
    bool m_autodecoration;
    bool m_majorgridlines, m_minorgridlines;
    int m_sides;
};

/**
 * @brief Constructs a coordinate system with specified boundaries and style
 * @param first Minimum vertex of the coordinate system box
 * @param second Maximum vertex of the coordinate system box
 * @param st Coordinate system style (NOCOORD, BOX, or FRAME)
 */
Qwt3DCoordinateSystem::Qwt3DCoordinateSystem(Triple first, Triple second, COORDSTYLE st) : QWT_PIMPL_CONSTRUCT
{
    axes = std::vector< Qwt3DAxis >(12);
    setStyle(st);
    init(first, second);

    setAxesColor(RGBA(0, 0, 0, 1));
    setGridLinesColor(RGBA(0.2, 0.2, 0.2, 1));
    setNumberFont("Courier", 12);
    setNumberColor(RGBA(0, 0, 0));
    setLabelFont("Courier", 14, QFont::Bold);
    setGridLines(false, false);
}

/**
 * @brief Destructor
 */
Qwt3DCoordinateSystem::~Qwt3DCoordinateSystem()
{
    destroy();
}

void Qwt3DCoordinateSystem::destroy()
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setLabelString("");

    detachAll();
}

void Qwt3DCoordinateSystem::init(Triple first, Triple second)
{
    QWT_D(d);

    destroy();

    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setScale(LINEARSCALE);

    Triple dv = second - first;

    setPosition(first, second);

    double majl = dv.length() / 100;  // 1 %
    setTicLength(majl, 0.6 * majl);

    axes[ X1 ].setPosition(first, first + Triple(dv.x, 0, 0));                          // front bottom x
    axes[ Y1 ].setPosition(first, first + Triple(0, dv.y, 0));                          // bottom left  y
    axes[ Z1 ].setPosition(first + Triple(0, dv.y, 0), first + Triple(0, dv.y, dv.z));  // back left z
    axes[ X1 ].setTicOrientation(0, -1, 0);
    axes[ Y1 ].setTicOrientation(-1, 0, 0);
    axes[ Z1 ].setTicOrientation(-1, 0, 0);

    axes[ X1 ].setLimits(first.x, second.x);
    axes[ X2 ].setLimits(first.x, second.x);
    axes[ X3 ].setLimits(first.x, second.x);
    axes[ X4 ].setLimits(first.x, second.x);

    axes[ Y1 ].setLimits(first.y, second.y);
    axes[ Y2 ].setLimits(first.y, second.y);
    axes[ Y3 ].setLimits(first.y, second.y);
    axes[ Y4 ].setLimits(first.y, second.y);

    axes[ Z1 ].setLimits(first.z, second.z);
    axes[ Z2 ].setLimits(first.z, second.z);
    axes[ Z3 ].setLimits(first.z, second.z);
    axes[ Z4 ].setLimits(first.z, second.z);

    // remaining x axes
    axes[ X2 ].setPosition(first + Triple(0, 0, dv.z), first + Triple(dv.x, 0, dv.z));  // front top x
    axes[ X3 ].setPosition(first + Triple(0, dv.y, dv.z), second);                      // back top x
    axes[ X4 ].setPosition(first + Triple(0, dv.y, 0), first + Triple(dv.x, dv.y, 0));  // back bottom x
    axes[ X2 ].setTicOrientation(0, -1, 0);
    axes[ X3 ].setTicOrientation(0, 1, 0);
    axes[ X4 ].setTicOrientation(0, 1, 0);

    // remaining y axes
    axes[ Y2 ].setPosition(first + Triple(dv.x, 0, 0), first + Triple(dv.x, dv.y, 0));  // bottom right y
    axes[ Y3 ].setPosition(first + Triple(dv.x, 0, dv.z), second);                      // top right y
    axes[ Y4 ].setPosition(first + Triple(0, 0, dv.z), first + Triple(0, dv.y, dv.z));  // top left y
    axes[ Y2 ].setTicOrientation(1, 0, 0);
    axes[ Y3 ].setTicOrientation(1, 0, 0);
    axes[ Y4 ].setTicOrientation(-1, 0, 0);

    // remaining z axes
    axes[ Z2 ].setPosition(first, first + Triple(0, 0, dv.z));                          // front left z
    axes[ Z4 ].setPosition(first + Triple(dv.x, dv.y, 0), second);                      // back right z
    axes[ Z3 ].setPosition(first + Triple(dv.x, 0, 0), first + Triple(dv.x, 0, dv.z));  // front right z
    axes[ Z2 ].setTicOrientation(-1, 0, 0);
    axes[ Z4 ].setTicOrientation(1, 0, 0);
    axes[ Z3 ].setTicOrientation(1, 0, 0);

    setStyle(d->m_style);
}

/**
 * @brief Draws the coordinate system, including grid lines if enabled
 * @details Chooses visible axes automatically when auto-decoration is on,
 *          then draws major and minor grid lines as configured.
 */
void Qwt3DCoordinateSystem::draw()
{
    QWT_D(d);

    GLStateBewarer sb(GL_LINE_SMOOTH, true);

    if (!d->m_smooth)
        sb.turnOff();

    if (d->m_autodecoration)
        chooseAxes();

    Qwt3DDrawable::draw();

    if (d->m_style == NOCOORD)
        return;

    if (d->m_majorgridlines || d->m_minorgridlines)
        recalculateAxesTics();
    if (d->m_majorgridlines)
        drawMajorGridLines();
    if (d->m_minorgridlines)
        drawMinorGridLines();
}

void Qwt3DCoordinateSystem::chooseAxes()
{
    QWT_D(d);

    vector< Triple > beg(axes.size());
    vector< Triple > end(axes.size());
    vector< Tuple > src(2 * axes.size());

    unsigned i;
    // collect axes viewport coordinates and initialize
    for (i = 0; i != axes.size(); ++i) {
        if (d->m_style != NOCOORD)
            attach(&axes[ i ]);

        beg[ i ]               = World2ViewPort(axes[ i ].begin());
        end[ i ]               = World2ViewPort(axes[ i ].end());
        src[ i ]               = Tuple(beg[ i ].x, beg[ i ].y);
        src[ axes.size() + i ] = Tuple(end[ i ].x, end[ i ].y);

        axes[ i ].setScaling(false);
        axes[ i ].setNumbers(false);
        axes[ i ].setLabel(false);
    }

    vector< unsigned > idx;
    convexhull2d(idx, src);

    int rem_x = -1;
    int rem_y = -1;
    int rem_z = -1;

    bool left;

    int choice_x = -1;
    int choice_y = -1;
    int choice_z = -1;

    int other_x = -1;
    int other_y = -1;
    int other_z = -1;

    // traverse convex hull
    for (unsigned k = 0; k != idx.size(); ++k) {
        Triple one, two;

        if (idx[ k ] >= axes.size())  // is end point
            one = end[ idx[ k ] - axes.size() ];
        else  // is begin point
            one = beg[ idx[ k ] ];

        unsigned int next = idx[ (k + 1) % idx.size() ];  // next point in cv (considered as ring buffer of points)

        if (next >= axes.size())
            two = end[ next - axes.size() ];
        else
            two = beg[ next ];

        for (i = 0; i != axes.size(); ++i) {
            if ((one == beg[ i ] && two == end[ i ]) || (two == beg[ i ] && one == end[ i ])) {
                if (i == X1 || i == X2 || i == X3 || i == X4)  // x axes
                {
                    if (rem_x >= 0)  // already second axis of the convex hull?
                    {
                        // lower of the two x axes
                        double y = min(min(end[ rem_x ].y, end[ i ].y), min(beg[ rem_x ].y, beg[ i ].y));
                        choice_x = (y == beg[ i ].y || y == end[ i ].y) ? i : rem_x;

                        other_x = (choice_x == static_cast< int >(i)) ? rem_x : static_cast< int >(i);
                        left = (beg[ choice_x ].x < beg[ other_x ].x || end[ choice_x ].x < end[ other_x ].x) ? true : false;

                        autoDecorateExposedAxis(axes[ choice_x ], left);

                        rem_x = -1;
                    } else {
                        rem_x = i;
                    }
                } else if (i == Y1 || i == Y2 || i == Y3 || i == Y4) {
                    if (rem_y >= 0) {
                        // lower of the two y axes
                        double y = min(min(end[ rem_y ].y, end[ i ].y), min(beg[ rem_y ].y, beg[ i ].y));
                        choice_y = (y == beg[ i ].y || y == end[ i ].y) ? i : rem_y;

                        other_y = (choice_y == static_cast< int >(i)) ? rem_y : static_cast< int >(i);
                        left = (beg[ choice_y ].x < beg[ other_y ].x || end[ choice_y ].x < end[ other_y ].x) ? true : false;
                        autoDecorateExposedAxis(axes[ choice_y ], left);

                        rem_y = -1;
                    } else {
                        rem_y = i;
                    }
                } else if (i == Z1 || i == Z2 || i == Z3 || i == Z4) {
                    if (rem_z >= 0) {
                        // rear of the two z axes
                        double z = max(max(end[ rem_z ].z, end[ i ].z), max(beg[ rem_z ].z, beg[ i ].z));
                        choice_z = (z == beg[ i ].z || z == end[ i ].z) ? i : rem_z;

                        other_z = (choice_z == static_cast< int >(i)) ? rem_z : static_cast< int >(i);

                        rem_z = -1;

                    } else {
                        rem_z = i;
                    }
                }
            }
        }  // for axes
    }      // for idx

    // fit z axis in - the onthewall axis if the decorated axes build a continous line, the opposite
    // else
    if (choice_x >= 0 && choice_y >= 0 && choice_z >= 0) {
        left = (beg[ choice_z ].x < beg[ other_z ].x || end[ choice_z ].x < end[ other_z ].x) ? true : false;

        if (axes[ choice_z ].begin() == axes[ choice_x ].begin() || axes[ choice_z ].begin() == axes[ choice_x ].end()
            || axes[ choice_z ].begin() == axes[ choice_y ].begin() || axes[ choice_z ].begin() == axes[ choice_y ].end()
            || axes[ choice_z ].end() == axes[ choice_x ].begin() || axes[ choice_z ].end() == axes[ choice_x ].end()
            || axes[ choice_z ].end() == axes[ choice_y ].begin() || axes[ choice_z ].end() == axes[ choice_y ].end()

        ) {
            autoDecorateExposedAxis(axes[ choice_z ], left);
        }

        else {
            autoDecorateExposedAxis(axes[ other_z ], !left);
            choice_z = other_z;  // for FRAME
        }
    }

    if (d->m_style == FRAME) {
        for (i = 0; i != axes.size(); ++i) {
            if (static_cast< int >(i) != choice_x && static_cast< int >(i) != choice_y && static_cast< int >(i) != choice_z)
                detach(&axes[ i ]);
        }
    }
}

void Qwt3DCoordinateSystem::autoDecorateExposedAxis(Qwt3DAxis& ax, bool left)
{
    Triple diff = World2ViewPort(ax.end()) - World2ViewPort(ax.begin());

    diff = Triple(diff.x, diff.y, 0);  // projection

    double s = diff.length();

    if (!s)
        return;

    ax.setScaling(true);
    ax.setNumbers(true);
    ax.setLabel(true);

    const double SQRT_2 = 0.7071067;
    double sina         = fabs(diff.y / s);

    if (left)  // leftmost (compared with antagonist in CV)  axis -> draw decorations on the left
               // side
    {
        if (diff.x >= 0 && diff.y >= 0 && sina < SQRT_2)  // 0..Pi/4
        {
            ax.setNumberAnchor(BottomCenter);
        } else if (diff.x >= 0 && diff.y >= 0 && !left)  // octant 2
        {
            ax.setNumberAnchor(CenterRight);
        } else if (diff.x <= 0 && diff.y >= 0 && sina >= SQRT_2)  // octant 3
        {
            ax.setNumberAnchor(CenterRight);
        } else if (diff.x <= 0 && diff.y >= 0)  // octant 4
        {
            ax.setNumberAnchor(TopCenter);
        } else if (diff.x <= 0 && diff.y <= 0 && sina <= SQRT_2)  // octant 5
        {
            ax.setNumberAnchor(BottomCenter);
        } else if (diff.x <= 0 && diff.y <= 0)  // octant 6
        {
            ax.setNumberAnchor(CenterRight);
        } else if (diff.x >= 0 && diff.y <= 0 && sina >= SQRT_2)  // octant 7
        {
            ax.setNumberAnchor(CenterRight);
        } else if (diff.x >= 0 && diff.y <= 0)  // octant 8
        {
            ax.setNumberAnchor(TopCenter);
        }
    } else  // rightmost axis
    {
        if (diff.x >= 0 && diff.y >= 0 && sina <= SQRT_2) {
            ax.setNumberAnchor(TopCenter);
        } else if (diff.x >= 0 && diff.y >= 0 && !left) {
            ax.setNumberAnchor(CenterLeft);
        } else if (diff.x <= 0 && diff.y >= 0 && sina >= SQRT_2) {
            ax.setNumberAnchor(CenterLeft);
        } else if (diff.x <= 0 && diff.y >= 0) {
            ax.setNumberAnchor(BottomCenter);
        } else if (diff.x <= 0 && diff.y <= 0 && sina <= SQRT_2) {
            ax.setNumberAnchor(TopCenter);
        } else if (diff.x <= 0 && diff.y <= 0) {
            ax.setNumberAnchor(CenterLeft);
        } else if (diff.x >= 0 && diff.y <= 0 && sina >= SQRT_2) {
            ax.setNumberAnchor(CenterLeft);
        } else if (diff.x >= 0 && diff.y <= 0) {
            ax.setNumberAnchor(BottomCenter);
        }
    }
}

/**
 * @brief Sets the position of the coordinate system box
 * @param first Front-left-bottom corner of the bounding box
 * @param second Back-right-top corner of the bounding box
 */
void Qwt3DCoordinateSystem::setPosition(Triple first, Triple second)
{
    QWT_D(d);
    d->m_first  = first;
    d->m_second = second;
}

/**
 * @brief Sets the length of major and minor tic marks for all axes
 * @param major Length of major tic marks
 * @param minor Length of minor tic marks
 */
void Qwt3DCoordinateSystem::setTicLength(double major, double minor)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setTicLength(major, minor);
}

/**
 * @brief Adjusts the distance between axis numbering and axis body for all axes
 * @param val Offset value to fine-tune number positioning
 */
void Qwt3DCoordinateSystem::adjustNumbers(int val)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].adjustNumbers(val);
}

/**
 * @brief Adjusts the distance between axis labels and axis body for all axes
 * @param val Offset value to fine-tune label positioning
 */
void Qwt3DCoordinateSystem::adjustLabels(int val)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].adjustLabel(val);
}

/**
 * @brief Enables or disables automatic scaling for all axes
 * @param val True to enable auto-scaling, false to disable
 */
void Qwt3DCoordinateSystem::setAutoScale(bool val)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setAutoScale(val);
}

/**
 * @brief Sets a common color for all axes
 * @param val RGBA color value to apply to all axes
 */
void Qwt3DCoordinateSystem::setAxesColor(RGBA val)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setColor(val);
}

/**
 * @brief Recalculates tic positions for all axes
 */
void Qwt3DCoordinateSystem::recalculateAxesTics()
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].recalculateTics();
}

/**
 * @brief Sets the font used for axis numbering across all axes
 * @param family Font family name
 * @param pointSize Font size in points
 * @param weight Font weight (e.g., QFont::Normal, QFont::Bold)
 * @param italic Whether to use italic style
 */
void Qwt3DCoordinateSystem::setNumberFont(QString const& family, int pointSize, int weight, bool italic)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setNumberFont(family, pointSize, weight, italic);
}

/**
 * @brief Sets the font used for axis numbering across all axes
 * @param font QFont object to apply to all axis numberings
 */
void Qwt3DCoordinateSystem::setNumberFont(QFont const& font)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setNumberFont(font);
}

/**
 * @brief Sets a common color for all axis numberings
 * @param val RGBA color value to apply to axis numbers
 */
void Qwt3DCoordinateSystem::setNumberColor(RGBA val)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setNumberColor(val);
}

/**
 * @brief Sets all axes to use linear scaling with real number items
 */
void Qwt3DCoordinateSystem::setStandardScale()
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setScale(LINEARSCALE);
}

/**
 * @brief Sets the font used for axis labels across all axes
 * @param font QFont object to apply to all axis labels
 */
void Qwt3DCoordinateSystem::setLabelFont(QFont const& font)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setLabelFont(font);
}

/**
 * @brief Sets the font used for axis labels across all axes
 * @param family Font family name
 * @param pointSize Font size in points
 * @param weight Font weight (e.g., QFont::Normal, QFont::Bold)
 * @param italic Whether to use italic style
 */
void Qwt3DCoordinateSystem::setLabelFont(QString const& family, int pointSize, int weight, bool italic)
{
    setLabelFont(QFont(family, pointSize, weight, italic));
}

/**
 * @brief Sets a common color for all axis labels
 * @param val RGBA color value to apply to axis labels
 */
void Qwt3DCoordinateSystem::setLabelColor(RGBA val)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setLabelColor(val);
}

/**
 * @brief Sets line width for axes and tic marks
 * @param val Base line width for axes
 * @param majfac Scaling factor for major tic line width
 * @param minfac Scaling factor for minor tic line width
 */
void Qwt3DCoordinateSystem::setLineWidth(double val, double majfac, double minfac)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setLineWidth(val, majfac, minfac);
}

/**
 * @brief Sets the coordinate system style and selects which axes to display in FRAME mode
 * @param s Coordinate system style (NOCOORD, BOX, or FRAME)
 * @param frame_1 First axis to display when using FRAME style
 * @param frame_2 Second axis to display when using FRAME style
 * @param frame_3 Third axis to display when using FRAME style
 * @details In BOX mode all 12 axes are drawn. In FRAME mode only the three
 *          specified axes are drawn (unless auto-decoration is enabled).
 *          NOCOORD disables all coordinate system rendering.
 */
void Qwt3DCoordinateSystem::setStyle(COORDSTYLE s, AXIS frame_1, AXIS frame_2, AXIS frame_3)
{
    QWT_D(d);
    d->m_style = s;

    switch (s) {
    case NOCOORD: {
        for (unsigned i = 0; i != axes.size(); ++i)
            detach(&axes[ i ]);
    } break;
    case BOX: {
        for (unsigned i = 0; i != axes.size(); ++i)
            attach(&axes[ i ]);
    } break;
    case FRAME: {
        for (unsigned i = 0; i != axes.size(); ++i)
            detach(&axes[ i ]);
        if (!d->m_autodecoration) {
            attach(&axes[ frame_1 ]);
            attach(&axes[ frame_2 ]);
            attach(&axes[ frame_3 ]);
        }
    } break;
    default:
        break;
    }
}

/**
 * @brief Sets grid line visibility
 * @param majors Draw grid between major tics
 * @param minors Draw grid between minor tics
 * @param sides Side(s) where the grid should be drawn
 * @details The axis used for tic calculation is chosen randomly from the respective pair.
 *          For most cases an identical tic distribution is therefore recommended.
 */
void Qwt3DCoordinateSystem::setGridLines(bool majors, bool minors, int sides)
{
    QWT_D(d);
    d->m_sides          = sides;
    d->m_majorgridlines = majors;
    d->m_minorgridlines = minors;
}

void Qwt3DCoordinateSystem::drawMajorGridLines()
{
    QWT_D(d);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4d(d->m_gridlinecolor.r, d->m_gridlinecolor.g, d->m_gridlinecolor.b, d->m_gridlinecolor.a);
    setDeviceLineWidth(axes[ X1 ].majLineWidth());

    glBegin(GL_LINES);
    if (d->m_sides & FLOOR) {
        drawMajorGridLines(axes[ X1 ], axes[ X4 ]);
        drawMajorGridLines(axes[ Y1 ], axes[ Y2 ]);
    }
    if (d->m_sides & CEIL) {
        drawMajorGridLines(axes[ X2 ], axes[ X3 ]);
        drawMajorGridLines(axes[ Y3 ], axes[ Y4 ]);
    }
    if (d->m_sides & LEFT) {
        drawMajorGridLines(axes[ Y1 ], axes[ Y4 ]);
        drawMajorGridLines(axes[ Z1 ], axes[ Z2 ]);
    }
    if (d->m_sides & RIGHT) {
        drawMajorGridLines(axes[ Y2 ], axes[ Y3 ]);
        drawMajorGridLines(axes[ Z3 ], axes[ Z4 ]);
    }
    if (d->m_sides & FRONT) {
        drawMajorGridLines(axes[ X1 ], axes[ X2 ]);
        drawMajorGridLines(axes[ Z2 ], axes[ Z3 ]);
    }
    if (d->m_sides & BACK) {
        drawMajorGridLines(axes[ X3 ], axes[ X4 ]);
        drawMajorGridLines(axes[ Z4 ], axes[ Z1 ]);
    }
    glEnd();
}

void Qwt3DCoordinateSystem::drawMinorGridLines()
{
    QWT_D(d);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4d(d->m_gridlinecolor.r, d->m_gridlinecolor.g, d->m_gridlinecolor.b, d->m_gridlinecolor.a);
    setDeviceLineWidth(axes[ X1 ].minLineWidth());

    glBegin(GL_LINES);
    if (d->m_sides & FLOOR) {
        drawMinorGridLines(axes[ X1 ], axes[ X4 ]);
        drawMinorGridLines(axes[ Y1 ], axes[ Y2 ]);
    }
    if (d->m_sides & CEIL) {
        drawMinorGridLines(axes[ X2 ], axes[ X3 ]);
        drawMinorGridLines(axes[ Y3 ], axes[ Y4 ]);
    }
    if (d->m_sides & LEFT) {
        drawMinorGridLines(axes[ Y1 ], axes[ Y4 ]);
        drawMinorGridLines(axes[ Z1 ], axes[ Z2 ]);
    }
    if (d->m_sides & RIGHT) {
        drawMinorGridLines(axes[ Y2 ], axes[ Y3 ]);
        drawMinorGridLines(axes[ Z3 ], axes[ Z4 ]);
    }
    if (d->m_sides & FRONT) {
        drawMinorGridLines(axes[ X1 ], axes[ X2 ]);
        drawMinorGridLines(axes[ Z2 ], axes[ Z3 ]);
    }
    if (d->m_sides & BACK) {
        drawMinorGridLines(axes[ X3 ], axes[ X4 ]);
        drawMinorGridLines(axes[ Z4 ], axes[ Z1 ]);
    }
    glEnd();
}

void Qwt3DCoordinateSystem::drawMajorGridLines(Qwt3DAxis& a0, Qwt3DAxis& a1)
{
    Triple d = a1.begin() - a0.begin();

    for (unsigned int i = 0; i != a0.majorPositions().size(); ++i) {
        glVertex3d(a0.majorPositions()[ i ].x, a0.majorPositions()[ i ].y, a0.majorPositions()[ i ].z);
        glVertex3d(a0.majorPositions()[ i ].x + d.x, a0.majorPositions()[ i ].y + d.y, a0.majorPositions()[ i ].z + d.z);
    }
}

void Qwt3DCoordinateSystem::drawMinorGridLines(Qwt3DAxis& a0, Qwt3DAxis& a1)
{
    Triple d = a1.begin() - a0.begin();

    for (unsigned int i = 0; i != a0.minorPositions().size(); ++i) {
        glVertex3d(a0.minorPositions()[ i ].x, a0.minorPositions()[ i ].y, a0.minorPositions()[ i ].z);
        glVertex3d(a0.minorPositions()[ i ].x + d.x, a0.minorPositions()[ i ].y + d.y, a0.minorPositions()[ i ].z + d.z);
    }
}

COORDSTYLE Qwt3DCoordinateSystem::style() const
{
    QWT_DC(d);
    return d->m_style;
}

void Qwt3DCoordinateSystem::setGridLinesColor(RGBA val)
{
    QWT_D(d);
    d->m_gridlinecolor = val;
}

Triple Qwt3DCoordinateSystem::first() const
{
    QWT_DC(d);
    return d->m_first;
}

Triple Qwt3DCoordinateSystem::second() const
{
    QWT_DC(d);
    return d->m_second;
}

void Qwt3DCoordinateSystem::setAutoDecoration(bool val)
{
    QWT_D(d);
    d->m_autodecoration = val;
}

bool Qwt3DCoordinateSystem::autoDecoration() const
{
    QWT_DC(d);
    return d->m_autodecoration;
}

void Qwt3DCoordinateSystem::setLineSmooth(bool val)
{
    QWT_D(d);
    d->m_smooth = val;
}

bool Qwt3DCoordinateSystem::lineSmooth() const
{
    QWT_DC(d);
    return d->m_smooth;
}

int Qwt3DCoordinateSystem::grids() const
{
    QWT_DC(d);
    return d->m_sides;
}
