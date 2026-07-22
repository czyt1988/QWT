#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4305)
#endif

#include "qwt3d_colorlegend.h"


class ColorLegend::PrivateData
{
    QWT_DECLARE_PUBLIC(ColorLegend)

public:
    PrivateData(ColorLegend* p) : q_ptr(p)
    {
        m_axisposition = ColorLegend::Left;
        m_orientation  = ColorLegend::BottomTop;
        m_showaxis     = true;
    }

    Label m_caption;
    ParallelEpiped m_pe;
    Tuple m_relMin, m_relMax;
    Axis m_axis;
    ColorLegend::SCALEPOSITION m_axisposition;
    ColorLegend::ORIENTATION m_orientation;
    bool m_showaxis;
};

/**
 * @brief Constructs a legend object with an axis at the left side
 * @details The legend resides in the top-right area and has no caption.
 *          Scale numbering is shown.
 */
ColorLegend::ColorLegend() : QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    d->m_axis.setNumbers(true);
    d->m_axis.setScaling(true);
    d->m_axis.setNumberColor(RGBA(0, 0, 0, 1));
    d->m_axis.setNumberAnchor(CenterRight);
    d->m_axis.setNumberFont(QFont("Courier", 8));

    d->m_caption.setFont("Courier", 10, QFont::Bold);
    d->m_caption.setColor(RGBA(0, 0, 0, 1));
    setRelPosition(Tuple(0.94, 1 - 0.36), Tuple(0.97, 1 - 0.04));
}

ColorLegend::~ColorLegend() = default;

/**
 * @brief Sets the legend title string
 * @param s Title text string
 */
void ColorLegend::setTitleString(QString const& s)
{
    QWT_D(d);
    d->m_caption.setString(s);
}

/**
 * @brief Sets the legend title font
 * @param family Font family name
 * @param pointSize Font point size
 * @param weight Font weight
 * @param italic Whether font is italic
 */
void ColorLegend::setTitleFont(QString const& family, int pointSize, int weight, bool italic)
{
    QWT_D(d);
    d->m_caption.setFont(family, pointSize, weight, italic);
}

/**
 * @brief Sets axis scale limits
 * @param start Start value
 * @param stop Stop value
 */
void ColorLegend::setLimits(double start, double stop)
{
    QWT_D(d);
    d->m_axis.setLimits(start, stop);
}

/**
 * @brief Sets number of major intervals
 * @param majors Number of major intervals
 */
void ColorLegend::setMajors(int majors)
{
    QWT_D(d);
    d->m_axis.setMajors(majors);
}

/**
 * @brief Sets number of minor intervals
 * @param minors Number of minor intervals
 */
void ColorLegend::setMinors(int minors)
{
    QWT_D(d);
    d->m_axis.setMinors(minors);
}

/**
 * @brief Enables or disables auto-scaling
 * @param val True to enable auto-scaling, false to disable
 */
void ColorLegend::setAutoScale(bool val)
{
    QWT_D(d);
    d->m_axis.setAutoScale(val);
}

/**
 * @brief Sets predefined scale type
 * @param val Scale type (LINEARSCALE or LOG10SCALE)
 */
void ColorLegend::setScale(SCALETYPE val)
{
    QWT_D(d);
    d->m_axis.setScale(val);
}

/**
 * @brief Sets a user-defined scale object
 * @param val Pointer to a Scale object
 */
void ColorLegend::setScale(Scale* val)
{
    QWT_D(d);
    d->m_axis.setScale(val);
}

/**
 * @brief Sets the legend orientation and axis scale position
 * @param orientation Legend orientation (BottomTop or TopBottom)
 * @param pos Axis scale position (Left, Right, Top, or Bottom)
 */
void ColorLegend::setOrientation(ORIENTATION orientation, SCALEPOSITION pos)
{
    QWT_D(d);
    d->m_orientation  = orientation;
    d->m_axisposition = pos;

    if (d->m_orientation == BottomTop) {
        if (d->m_axisposition == Bottom || d->m_axisposition == Top)
            d->m_axisposition = Left;
    } else {
        if (d->m_axisposition == Left || d->m_axisposition == Right)
            d->m_axisposition = Bottom;
    }
}

/**
 * @brief Sets relative position of the legend within the plot area
 * @param relMin Minimum relative position (x,y)
 * @param relMax Maximum relative position (x,y)
 */
void ColorLegend::setRelPosition(Tuple relMin, Tuple relMax)
{
    QWT_D(d);
    d->m_relMin = relMin;
    d->m_relMax = relMax;
}

void ColorLegend::setGeometryInternal()
{
    QWT_D(d);

    double ot = .99;

    getMatrices(modelMatrix, projMatrix, viewport);
    d->m_pe.minVertex = relativePosition(Triple(d->m_relMin.x, d->m_relMin.y, ot));
    d->m_pe.maxVertex = relativePosition(Triple(d->m_relMax.x, d->m_relMax.y, ot));

    double diff = 0;
    Triple b;
    Triple e;

    switch (d->m_axisposition) {
    case ColorLegend::Left:
        b   = d->m_pe.minVertex;
        e   = d->m_pe.maxVertex;
        e.x = b.x;
        d->m_axis.setTicOrientation(-1, 0, 0);
        d->m_axis.setNumberAnchor(CenterRight);
        diff = d->m_pe.maxVertex.x - d->m_pe.minVertex.x;
        break;
    case ColorLegend::Right:
        e   = d->m_pe.maxVertex;
        b   = d->m_pe.minVertex;
        b.x = e.x;
        d->m_axis.setTicOrientation(+1, 0, 0);
        d->m_axis.setNumberAnchor(CenterLeft);
        diff = d->m_pe.maxVertex.x - d->m_pe.minVertex.x;
        break;
    case ColorLegend::Top:
        e   = d->m_pe.maxVertex;
        b   = d->m_pe.minVertex;
        b.z = e.z;
        d->m_axis.setTicOrientation(0, 0, +1);
        d->m_axis.setNumberAnchor(BottomCenter);
        diff = d->m_pe.maxVertex.z - d->m_pe.minVertex.z;
        break;
    case ColorLegend::Bottom:
        b   = d->m_pe.minVertex;
        e   = d->m_pe.maxVertex;
        e.z = b.z;
        d->m_axis.setTicOrientation(0, 0, -1);
        d->m_axis.setNumberAnchor(TopCenter);
        diff = d->m_pe.maxVertex.z - d->m_pe.minVertex.z;
        break;
    default:
        break;
    }

    d->m_axis.setPosition(b, e);
    diff /= 10;

    d->m_axis.setTicLength(diff, 0.6 * diff);

    Triple c;
    c.x = d->m_pe.minVertex.x + ((d->m_pe.maxVertex - d->m_pe.minVertex) / 2).x;
    c.z = d->m_pe.maxVertex.z;
    c.z += (d->m_pe.maxVertex.z - d->m_pe.minVertex.z) / 20;
    c.y = d->m_pe.maxVertex.y;

    d->m_caption.setPosition(c, BottomCenter);
}

ParallelEpiped ColorLegend::geometry() const
{
    QWT_DC(d);
    return d->m_pe;
}

void ColorLegend::drawScale(bool val)
{
    QWT_D(d);
    d->m_showaxis = val;
}

void ColorLegend::drawNumbers(bool val)
{
    QWT_D(d);
    d->m_axis.setNumbers(val);
}

/**
 * @brief Draws the color legend
 * @details Renders the color legend including color bar, axis, and caption.
 */
void ColorLegend::draw()
{
    if (colors.empty())
        return;

    QWT_D(d);

    setGeometryInternal();

    saveGLState();

    Triple one = d->m_pe.minVertex;
    Triple two = d->m_pe.maxVertex;

    double h = (d->m_orientation == ColorLegend::BottomTop) ? (two - one).z / colors.size()
                                                            : (two - one).x / colors.size();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    GLStateBewarer(GL_POLYGON_OFFSET_FILL, true);

    glColor4d(0, 0, 0, 1);
    glBegin(GL_LINE_LOOP);
    glVertex3d(one.x, one.y, one.z);
    glVertex3d(one.x, one.y, two.z);
    glVertex3d(two.x, one.y, two.z);
    glVertex3d(two.x, one.y, one.z);
    glEnd();

    size_t size = colors.size();
    RGBA rgb;

    if (d->m_orientation == ColorLegend::BottomTop) {
        for (unsigned i = 1; i <= size; ++i) {
            rgb = colors[ i - 1 ];
            glColor4d(rgb.r, rgb.g, rgb.b, rgb.a);
            glBegin(GL_POLYGON);
            glVertex3d(one.x, one.y, one.z + (i - 1) * h);
            glVertex3d(one.x, one.y, one.z + i * h);
            glVertex3d(two.x, one.y, one.z + i * h);
            glVertex3d(two.x, one.y, one.z + (i - 1) * h);
            glEnd();
        }
    } else {
        for (unsigned i = 1; i <= size; ++i) {
            rgb = colors[ i - 1 ];
            glColor4d(rgb.r, rgb.g, rgb.b, rgb.a);
            glBegin(GL_POLYGON);
            glVertex3d(one.x + (i - 1) * h, one.y, one.z);
            glVertex3d(one.x + i * h, one.y, one.z);
            glVertex3d(one.x + i * h, one.y, two.z);
            glVertex3d(one.x + (i - 1) * h, one.y, two.z);
            glEnd();
        }
    }

    restoreGLState();

    if (d->m_showaxis)
        d->m_axis.draw();

    d->m_caption.draw();
}
