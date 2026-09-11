#include "qwt3d_coordsys.h"

#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

#include <cmath>

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
        , m_tickPosition(TICK_BOTTOM)
        , m_interiorMajor(false)
        , m_interiorMinor(false)
        , m_interiorSides(NO_INTERIOR)
        , m_interiorMajWidth(0.5)
        , m_interiorMinWidth(0.3)
    {
    }

    Triple m_first, m_second;
    COORDSTYLE m_style;
    RGBA m_axesColor;
    RGBA m_numberColor;
    RGBA m_labelColor;
    RGBA m_gridlinecolor;
    bool m_smooth;
    bool m_autodecoration;
    bool m_majorgridlines, m_minorgridlines;
    int m_sides;
    TICKPOSITION m_tickPosition;
    bool m_interiorMajor, m_interiorMinor;
    int m_interiorSides;
    RGBA m_interiorGridColor;
    double m_interiorMajWidth, m_interiorMinWidth;

    // Tic-length model: automatic per-axis derivation (scale-based) by default,
    // or an explicit manual override that survives init() (data changes).
    double m_ticLengthScale = 0.015;   // auto: fraction of per-axis perpendicular range
    bool m_manualTicLength = false;     // user set an explicit length via setTicLength()
    double m_manualMajorTic = 0.0;
    double m_manualMinorTic = 0.0;
};

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
    setInteriorGridLinesColor(RGBA(0.6, 0.6, 0.6, 0.5));
}

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

    axes[ X1 ].setPosition(first, first + Triple(dv.x, 0, 0));
    axes[ Y1 ].setPosition(first, first + Triple(0, dv.y, 0));
    axes[ Z1 ].setPosition(first + Triple(0, dv.y, 0), first + Triple(0, dv.y, dv.z));
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

    axes[ X2 ].setPosition(first + Triple(0, 0, dv.z), first + Triple(dv.x, 0, dv.z));
    axes[ X3 ].setPosition(first + Triple(0, dv.y, dv.z), second);
    axes[ X4 ].setPosition(first + Triple(0, dv.y, 0), first + Triple(dv.x, dv.y, 0));
    axes[ X2 ].setTicOrientation(0, -1, 0);
    axes[ X3 ].setTicOrientation(0, 1, 0);
    axes[ X4 ].setTicOrientation(0, 1, 0);

    axes[ Y2 ].setPosition(first + Triple(dv.x, 0, 0), first + Triple(dv.x, dv.y, 0));
    axes[ Y3 ].setPosition(first + Triple(dv.x, 0, dv.z), second);
    axes[ Y4 ].setPosition(first + Triple(0, 0, dv.z), first + Triple(0, dv.y, dv.z));
    axes[ Y2 ].setTicOrientation(1, 0, 0);
    axes[ Y3 ].setTicOrientation(1, 0, 0);
    axes[ Y4 ].setTicOrientation(-1, 0, 0);

    axes[ Z2 ].setPosition(first, first + Triple(0, 0, dv.z));
    axes[ Z4 ].setPosition(first + Triple(dv.x, dv.y, 0), second);
    axes[ Z3 ].setPosition(first + Triple(dv.x, 0, 0), first + Triple(dv.x, 0, dv.z));
    axes[ Z2 ].setTicOrientation(-1, 0, 0);
    axes[ Z4 ].setTicOrientation(1, 0, 0);
    axes[ Z3 ].setTicOrientation(1, 0, 0);

    // Apply the tic-length model now that all orientations are set. This respects
    // an explicit override (setTicLength) and otherwise derives per-axis lengths
    // from the data range in each tick's pointing direction (anisotropy-proof).
    applyTicLengths();

    setStyle(d->m_style);
}

/**
 * @brief Helper: draws a set of line segments using VBO + line shader
 * @param ctx Render context providing shader and matrices
 */
void Qwt3DCoordinateSystem::drawGridLines(const Qwt3DRenderContext& ctx, const QVector<float>& vertices, double lineWidth, const RGBA& lineColor)
{
    if (vertices.isEmpty())
        return;

    auto* shader = ctx.lineShader;
    if (!shader)
        return;

    auto* f = QOpenGLContext::currentContext()->functions();

    // TODO: glLineWidth > 1.0 not guaranteed in Core Profile (Plan B)
    f->glLineWidth(static_cast< GLfloat >(std::max(1.0, lineWidth)));

    QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices.constData(), vertices.size() * sizeof(float));

    shader->bind();
    shader->setUniformValue("uModelView", ctx.modelView);
    shader->setUniformValue("uProjection", ctx.projection);

    int stride = 7 * sizeof(float);
    shader->enableAttributeArray(0);
    shader->setAttributeBuffer(0, GL_FLOAT, 0, 3, stride);
    shader->enableAttributeArray(1);
    shader->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 4, stride);

    f->glDrawArrays(GL_LINES, 0, vertices.size() / 7);

    shader->disableAttributeArray(0);
    shader->disableAttributeArray(1);
    shader->release();
    vbo.release();
    vbo.destroy();
}

/**
 * @brief Draws the coordinate system, including grid lines if enabled
 * @param ctx Render context providing shaders, matrices, and coordinate conversion
 */
void Qwt3DCoordinateSystem::draw(const Qwt3DRenderContext& ctx)
{
    QWT_D(d);

    if (d->m_autodecoration)
        chooseAxes(ctx);

    Qwt3DDrawable::draw(ctx);

    if (d->m_style == NOCOORD)
        return;

    if (d->m_majorgridlines || d->m_minorgridlines
        || d->m_interiorMajor || d->m_interiorMinor)
        recalculateAxesTics();
    if (d->m_majorgridlines)
        drawMajorGridLines(ctx);
    if (d->m_minorgridlines)
        drawMinorGridLines(ctx);
    if (d->m_interiorMajor)
        drawInteriorMajorGridLines(ctx);
    if (d->m_interiorMinor)
        drawInteriorMinorGridLines(ctx);
}

void Qwt3DCoordinateSystem::chooseAxes(const Qwt3DRenderContext& ctx)
{
    QWT_D(d);

    vector< QPointF > beg(axes.size());
    vector< QPointF > end(axes.size());
    vector< Tuple > src(2 * axes.size());

    unsigned i;
    for (i = 0; i != axes.size(); ++i) {
        if (d->m_style != NOCOORD)
            attach(&axes[ i ]);

        beg[ i ]               = ctx.worldToScreen(axes[ i ].begin());
        end[ i ]               = ctx.worldToScreen(axes[ i ].end());
        src[ i ]               = Tuple(beg[ i ].x(), beg[ i ].y());
        src[ axes.size() + i ] = Tuple(end[ i ].x(), end[ i ].y());

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

    for (unsigned k = 0; k != idx.size(); ++k) {
        QPointF one, two;

        if (idx[ k ] >= axes.size())
            one = end[ idx[ k ] - axes.size() ];
        else
            one = beg[ idx[ k ] ];

        unsigned int next = idx[ (k + 1) % idx.size() ];

        if (next >= axes.size())
            two = end[ next - axes.size() ];
        else
            two = beg[ next ];

        for (i = 0; i != axes.size(); ++i) {
            if ((one == beg[ i ] && two == end[ i ]) || (two == beg[ i ] && one == end[ i ])) {
                if (i == X1 || i == X2 || i == X3 || i == X4)
                {
                    if (rem_x >= 0)
                    {
                        // Screen y increases downward; max y = visually lower, min y = visually upper
                        double y;
                        if (d->m_tickPosition == TICK_BOTTOM) {
                            y = max(max(end[ rem_x ].y(), end[ i ].y()), max(beg[ rem_x ].y(), beg[ i ].y()));
                        } else {
                            y = min(min(end[ rem_x ].y(), end[ i ].y()), min(beg[ rem_x ].y(), beg[ i ].y()));
                        }
                        choice_x = (y == beg[ i ].y() || y == end[ i ].y()) ? static_cast< int >(i) : rem_x;

                        other_x = (choice_x == static_cast< int >(i)) ? rem_x : static_cast< int >(i);
                        left = (beg[ choice_x ].x() < beg[ other_x ].x() || end[ choice_x ].x() < end[ other_x ].x()) ? true : false;

                        autoDecorateExposedAxis(ctx, axes[ choice_x ], left);

                        rem_x = -1;
                    } else {
                        rem_x = static_cast< int >(i);
                    }
                } else if (i == Y1 || i == Y2 || i == Y3 || i == Y4) {
                    if (rem_y >= 0) {
                        // Screen y increases downward; max y = visually lower, min y = visually upper
                        double y;
                        if (d->m_tickPosition == TICK_BOTTOM) {
                            y = max(max(end[ rem_y ].y(), end[ i ].y()), max(beg[ rem_y ].y(), beg[ i ].y()));
                        } else {
                            y = min(min(end[ rem_y ].y(), end[ i ].y()), min(beg[ rem_y ].y(), beg[ i ].y()));
                        }
                        choice_y = (y == beg[ i ].y() || y == end[ i ].y()) ? static_cast< int >(i) : rem_y;

                        other_y = (choice_y == static_cast< int >(i)) ? rem_y : static_cast< int >(i);
                        left = (beg[ choice_y ].x() < beg[ other_y ].x() || end[ choice_y ].x() < end[ other_y ].x()) ? true : false;
                        autoDecorateExposedAxis(ctx, axes[ choice_y ], left);

                        rem_y = -1;
                    } else {
                        rem_y = static_cast< int >(i);
                    }
                } else if (i == Z1 || i == Z2 || i == Z3 || i == Z4) {
                    if (rem_z >= 0) {
                        // Two Z axes found on hull — track for post-loop connection check
                        choice_z = rem_z;
                        other_z = static_cast< int >(i);
                        rem_z = -1;
                    } else {
                        rem_z = static_cast< int >(i);
                    }
                }
            }
        }
    }

    if (choice_x >= 0 && choice_y >= 0 && choice_z >= 0) {
        if (static_cast< int >(axes[ choice_z ].begin() == axes[ choice_x ].begin() ||
            axes[ choice_z ].begin() == axes[ choice_x ].end() ||
            axes[ choice_z ].begin() == axes[ choice_y ].begin() ||
            axes[ choice_z ].begin() == axes[ choice_y ].end() ||
            axes[ choice_z ].end() == axes[ choice_x ].begin() ||
            axes[ choice_z ].end() == axes[ choice_x ].end() ||
            axes[ choice_z ].end() == axes[ choice_y ].begin() ||
            axes[ choice_z ].end() == axes[ choice_y ].end()))
        {
            autoDecorateExposedAxis(ctx, axes[ choice_z ], left);
        }
        else
        {
            autoDecorateExposedAxis(ctx, axes[ other_z ], !left);
            choice_z = other_z;
        }
    }

    if (d->m_style == FRAME) {
        for (i = 0; i != axes.size(); ++i) {
            if (static_cast< int >(i) != choice_x && static_cast< int >(i) != choice_y && static_cast< int >(i) != choice_z)
                detach(&axes[ i ]);
        }
    }
}

void Qwt3DCoordinateSystem::autoDecorateExposedAxis(const Qwt3DRenderContext& ctx, Qwt3DAxis& ax, bool left)
{
    (void)left;  // polarity is now derived from the outward direction below

    // The exposed axis lies on the projected box silhouette, so the screen
    // vector from the box center to the axis midpoint always points outward.
    // This is independent of the axis' own screen direction (which is what the
    // previous sina-vs-SQRT_2 heuristic used) and therefore does not flip when
    // the axis projects near 45 degrees or when the viewport aspect changes.
    Triple midWorld = ax.begin() + (ax.end() - ax.begin()) / 2.0;
    Triple centerWorld = first() + (second() - first()) / 2.0;
    QPointF midScreen = ctx.worldToScreen(midWorld);
    QPointF centerScreen = ctx.worldToScreen(centerWorld);
    QPointF outward = midScreen - centerScreen;  // screen y grows downward

    double s = sqrt(outward.x() * outward.x() + outward.y() * outward.y());

    if (!s)
        return;

    ax.setScaling(true);
    ax.setNumbers(true);
    ax.setLabel(true);

    // Map the outward screen direction to an anchor so the label text extends
    // outward (away from the box). Screen y is downward, hence outward.y() > 0
    // means the axis is visually below the center and the text must extend
    // further down; the horizontal cases are symmetric.
    if (fabs(outward.y()) >= fabs(outward.x()))
        ax.setNumberAnchor(outward.y() > 0 ? BottomCenter : TopCenter);
    else
        ax.setNumberAnchor(outward.x() > 0 ? CenterLeft : CenterRight);
}

void Qwt3DCoordinateSystem::setPosition(Triple first, Triple second)
{
    QWT_D(d);
    d->m_first  = first;
    d->m_second = second;
}

void Qwt3DCoordinateSystem::setTicLength(double major, double minor)
{
    QWT_D(d);
    // Store as an explicit override so init() (re-run on every data change) honors
    // it instead of clobbering it with the automatic per-axis derivation.
    d->m_manualTicLength = true;
    d->m_manualMajorTic = major;
    d->m_manualMinorTic = minor;
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setTicLength(major, minor);
}

/**
 * @brief 返回显式设置的 tic 长度
 * @param[out] major 主刻度长度
 * @param[out] minor 次刻度长度
 * @details 仅返回通过 setTicLength() 设置的值。如果使用自动模式
 *          (ticLengthScale())，返回的值为上次显式设置或默认值 0。
 */
void Qwt3DCoordinateSystem::ticLength(double& major, double& minor) const
{
    QWT_DC(d);
    major = d->m_manualMajorTic;
    minor = d->m_manualMinorTic;
}

void Qwt3DCoordinateSystem::setTicLengthScale(double scale)
{
    QWT_D(d);
    if (scale < 0.0)
        scale = 0.0;
    d->m_ticLengthScale = scale;
    d->m_manualTicLength = false;  // back to automatic per-axis derivation
    applyTicLengths();
}

double Qwt3DCoordinateSystem::ticLengthScale() const
{
    QWT_DC(d);
    return d->m_ticLengthScale;
}

void Qwt3DCoordinateSystem::setAutoTicLength()
{
    QWT_D(d);
    d->m_manualTicLength = false;
    applyTicLengths();
}

void Qwt3DCoordinateSystem::applyTicLengths()
{
    QWT_D(d);

    if (d->m_manualTicLength) {
        // Explicit user override: identical length on every axis, preserved across init()
        for (unsigned i = 0; i != axes.size(); ++i)
            axes[ i ].setTicLength(d->m_manualMajorTic, d->m_manualMinorTic);
        return;
    }

    // Automatic per-axis derivation: each tick's length is a fraction of the data
    // range in the direction the tick points. Under AUTOFILL the visual length
    // then collapses to scale * maxRange on every axis (anisotropy-proof); under
    // DATARATIO it is scale * that axis's own range (consistent per axis).
    const Triple dv = d->m_second - d->m_first;
    const double scale = d->m_ticLengthScale;
    for (unsigned i = 0; i != axes.size(); ++i) {
        const Triple o = axes[ i ].ticOrientation();
        double rangeDir = std::abs(o.x) * dv.x + std::abs(o.y) * dv.y + std::abs(o.z) * dv.z;
        if (!(rangeDir > 0.0))
            rangeDir = 1.0;
        const double majl = scale * rangeDir;
        axes[ i ].setTicLength(majl, 0.6 * majl);
    }
}

void Qwt3DCoordinateSystem::adjustNumbers(int val)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].adjustNumbers(val);
}

void Qwt3DCoordinateSystem::adjustLabels(int val)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].adjustLabel(val);
}

void Qwt3DCoordinateSystem::setAutoScale(bool val)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setAutoScale(val);
}

/**
 * @brief 返回是否启用自动缩放
 * @return true 如果自动缩放已启用（委托到第一个轴的 autoScale()）
 */
bool Qwt3DCoordinateSystem::autoScale() const
{
    return axes[ 0 ].autoScale();
}

void Qwt3DCoordinateSystem::setAxesColor(RGBA val)
{
    QWT_D(d);
    d->m_axesColor = val;
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setColor(val);
}

/**
 * @brief 返回轴线颜色
 * @return RGBA 颜色值
 */
RGBA Qwt3DCoordinateSystem::axesColor() const
{
    QWT_DC(d);
    return d->m_axesColor;
}

void Qwt3DCoordinateSystem::recalculateAxesTics()
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].recalculateTics();
}

void Qwt3DCoordinateSystem::setNumberFont(QString const& family, int pointSize, int weight, bool italic)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setNumberFont(family, pointSize, weight, italic);
}

void Qwt3DCoordinateSystem::setNumberFont(QFont const& font)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setNumberFont(font);
}

/**
 * @brief 返回数字字体
 * @return QFont 值（委托到第一个轴的 numberFont()）
 * @details setNumberFont() 统一设置所有轴的数字字体，因此读取 axes[0] 即可代表当前值。
 */
QFont Qwt3DCoordinateSystem::numberFont() const
{
    return axes[ 0 ].numberFont();
}

void Qwt3DCoordinateSystem::setNumberColor(RGBA val)
{
    QWT_D(d);
    d->m_numberColor = val;
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setNumberColor(val);
}

/**
 * @brief 返回数字颜色
 * @return RGBA 颜色值
 */
RGBA Qwt3DCoordinateSystem::numberColor() const
{
    QWT_DC(d);
    return d->m_numberColor;
}

void Qwt3DCoordinateSystem::setStandardScale()
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setScale(LINEARSCALE);
}

void Qwt3DCoordinateSystem::setLabelFont(QFont const& font)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setLabelFont(font);
}

/**
 * @brief 返回标签字体
 * @return QFont 值（委托到第一个轴的 labelFont()）
 */
QFont Qwt3DCoordinateSystem::labelFont() const
{
    return axes[ 0 ].labelFont();
}

void Qwt3DCoordinateSystem::setLabelFont(QString const& family, int pointSize, int weight, bool italic)
{
    setLabelFont(QFont(family, pointSize, weight, italic));
}

void Qwt3DCoordinateSystem::setLabelColor(RGBA val)
{
    QWT_D(d);
    d->m_labelColor = val;
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setLabelColor(val);
}

/**
 * @brief 返回标签颜色
 * @return RGBA 颜色值
 */
RGBA Qwt3DCoordinateSystem::labelColor() const
{
    QWT_DC(d);
    return d->m_labelColor;
}

void Qwt3DCoordinateSystem::setLineWidth(double val, double majfac, double minfac)
{
    for (unsigned i = 0; i != axes.size(); ++i)
        axes[ i ].setLineWidth(val, majfac, minfac);
}

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

void Qwt3DCoordinateSystem::setGridLines(bool majors, bool minors, int sides)
{
    QWT_D(d);
    d->m_sides          = sides;
    d->m_majorgridlines = majors;
    d->m_minorgridlines = minors;
}

void Qwt3DCoordinateSystem::drawMajorGridLines(const Qwt3DRenderContext& ctx)
{
    QWT_D(d);

    float r = static_cast< float >(d->m_gridlinecolor.r);
    float g = static_cast< float >(d->m_gridlinecolor.g);
    float b = static_cast< float >(d->m_gridlinecolor.b);
    float a = static_cast< float >(d->m_gridlinecolor.a);

    QVector<float> verts;

    if (d->m_sides & FLOOR) {
        drawMajorGridLines(axes[ X1 ], axes[ X4 ], verts);
        drawMajorGridLines(axes[ Y1 ], axes[ Y2 ], verts);
    }
    if (d->m_sides & CEIL) {
        drawMajorGridLines(axes[ X2 ], axes[ X3 ], verts);
        drawMajorGridLines(axes[ Y3 ], axes[ Y4 ], verts);
    }
    if (d->m_sides & LEFT) {
        drawMajorGridLines(axes[ Y1 ], axes[ Y4 ], verts);
        drawMajorGridLines(axes[ Z1 ], axes[ Z2 ], verts);
    }
    if (d->m_sides & RIGHT) {
        drawMajorGridLines(axes[ Y2 ], axes[ Y3 ], verts);
        drawMajorGridLines(axes[ Z3 ], axes[ Z4 ], verts);
    }
    if (d->m_sides & FRONT) {
        drawMajorGridLines(axes[ X1 ], axes[ X2 ], verts);
        drawMajorGridLines(axes[ Z2 ], axes[ Z3 ], verts);
    }
    if (d->m_sides & BACK) {
        drawMajorGridLines(axes[ X3 ], axes[ X4 ], verts);
        drawMajorGridLines(axes[ Z4 ], axes[ Z1 ], verts);
    }

    drawGridLines(ctx, verts, axes[ X1 ].majLineWidth(), d->m_gridlinecolor);
}

void Qwt3DCoordinateSystem::drawMinorGridLines(const Qwt3DRenderContext& ctx)
{
    QWT_D(d);

    float r = static_cast< float >(d->m_gridlinecolor.r);
    float g = static_cast< float >(d->m_gridlinecolor.g);
    float b = static_cast< float >(d->m_gridlinecolor.b);
    float a = static_cast< float >(d->m_gridlinecolor.a);

    QVector<float> verts;

    if (d->m_sides & FLOOR) {
        drawMinorGridLines(axes[ X1 ], axes[ X4 ], verts);
        drawMinorGridLines(axes[ Y1 ], axes[ Y2 ], verts);
    }
    if (d->m_sides & CEIL) {
        drawMinorGridLines(axes[ X2 ], axes[ X3 ], verts);
        drawMinorGridLines(axes[ Y3 ], axes[ Y4 ], verts);
    }
    if (d->m_sides & LEFT) {
        drawMinorGridLines(axes[ Y1 ], axes[ Y4 ], verts);
        drawMinorGridLines(axes[ Z1 ], axes[ Z2 ], verts);
    }
    if (d->m_sides & RIGHT) {
        drawMinorGridLines(axes[ Y2 ], axes[ Y3 ], verts);
        drawMinorGridLines(axes[ Z3 ], axes[ Z4 ], verts);
    }
    if (d->m_sides & FRONT) {
        drawMinorGridLines(axes[ X1 ], axes[ X2 ], verts);
        drawMinorGridLines(axes[ Z2 ], axes[ Z3 ], verts);
    }
    if (d->m_sides & BACK) {
        drawMinorGridLines(axes[ X3 ], axes[ X4 ], verts);
        drawMinorGridLines(axes[ Z4 ], axes[ Z1 ], verts);
    }

    drawGridLines(ctx, verts, axes[ X1 ].minLineWidth(), d->m_gridlinecolor);
}

void Qwt3DCoordinateSystem::drawMajorGridLines(Qwt3DAxis& a0, Qwt3DAxis& a1, QVector<float>& verts)
{
    QWT_D(d);
    Triple diff = a1.begin() - a0.begin();

    for (unsigned int i = 0; i != a0.majorPositions().size(); ++i) {
        const Triple& p = a0.majorPositions()[ i ];
        verts << static_cast< float >(p.x) << static_cast< float >(p.y) << static_cast< float >(p.z);
        verts << static_cast< float >(d->m_gridlinecolor.r) << static_cast< float >(d->m_gridlinecolor.g)
              << static_cast< float >(d->m_gridlinecolor.b) << static_cast< float >(d->m_gridlinecolor.a);
        verts << static_cast< float >(p.x + diff.x) << static_cast< float >(p.y + diff.y) << static_cast< float >(p.z + diff.z);
        verts << static_cast< float >(d->m_gridlinecolor.r) << static_cast< float >(d->m_gridlinecolor.g)
              << static_cast< float >(d->m_gridlinecolor.b) << static_cast< float >(d->m_gridlinecolor.a);
    }
}

void Qwt3DCoordinateSystem::drawMinorGridLines(Qwt3DAxis& a0, Qwt3DAxis& a1, QVector<float>& verts)
{
    QWT_D(d);
    Triple diff = a1.begin() - a0.begin();

    for (unsigned int i = 0; i != a0.minorPositions().size(); ++i) {
        const Triple& p = a0.minorPositions()[ i ];
        verts << static_cast< float >(p.x) << static_cast< float >(p.y) << static_cast< float >(p.z);
        verts << static_cast< float >(d->m_gridlinecolor.r) << static_cast< float >(d->m_gridlinecolor.g)
              << static_cast< float >(d->m_gridlinecolor.b) << static_cast< float >(d->m_gridlinecolor.a);
        verts << static_cast< float >(p.x + diff.x) << static_cast< float >(p.y + diff.y) << static_cast< float >(p.z + diff.z);
        verts << static_cast< float >(d->m_gridlinecolor.r) << static_cast< float >(d->m_gridlinecolor.g)
              << static_cast< float >(d->m_gridlinecolor.b) << static_cast< float >(d->m_gridlinecolor.a);
    }
}

void Qwt3DCoordinateSystem::drawInteriorMajorGridLines(const Qwt3DRenderContext& ctx)
{
    QWT_D(d);

    QVector<float> verts;

    if (d->m_interiorSides & Z_INNER) {
        drawInteriorGridLines(axes[ X1 ], axes[ Y1 ], 2, true, verts);
    }
    if (d->m_interiorSides & X_INNER) {
        drawInteriorGridLines(axes[ Y1 ], axes[ Z2 ], 0, true, verts);
    }
    if (d->m_interiorSides & Y_INNER) {
        drawInteriorGridLines(axes[ X1 ], axes[ Z2 ], 1, true, verts);
    }

    drawGridLines(ctx, verts, d->m_interiorMajWidth, d->m_interiorGridColor);
}

void Qwt3DCoordinateSystem::drawInteriorMinorGridLines(const Qwt3DRenderContext& ctx)
{
    QWT_D(d);

    QVector<float> verts;

    if (d->m_interiorSides & Z_INNER) {
        drawInteriorGridLines(axes[ X1 ], axes[ Y1 ], 2, false, verts);
    }
    if (d->m_interiorSides & X_INNER) {
        drawInteriorGridLines(axes[ Y1 ], axes[ Z2 ], 0, false, verts);
    }
    if (d->m_interiorSides & Y_INNER) {
        drawInteriorGridLines(axes[ X1 ], axes[ Z2 ], 1, false, verts);
    }

    drawGridLines(ctx, verts, d->m_interiorMinWidth, d->m_interiorGridColor);
}

void Qwt3DCoordinateSystem::drawInteriorGridLines(
    Qwt3DAxis& axisA, Qwt3DAxis& axisB, int dirAxis, bool major, QVector<float>& verts)
{
    QWT_D(d);

    Triple begA = axisA.begin(), endA = axisA.end();
    Triple begB = axisB.begin(), endB = axisB.end();

    int dimA = (!isPracticallyZero(endA.x - begA.x)) ? 0
             : (!isPracticallyZero(endA.y - begA.y)) ? 1 : 2;
    int dimB = (!isPracticallyZero(endB.x - begB.x)) ? 0
             : (!isPracticallyZero(endB.y - begB.y)) ? 1 : 2;

    if (dimA == dirAxis || dimB == dirAxis || dimA == dimB)
        return;

    Triple bmin = first();
    Triple bmax = second();

    float r = static_cast< float >(d->m_interiorGridColor.r);
    float g = static_cast< float >(d->m_interiorGridColor.g);
    float bl = static_cast< float >(d->m_interiorGridColor.b);
    float al = static_cast< float >(d->m_interiorGridColor.a);

    const auto& positionsA = major ? axisA.majorPositions() : axisA.minorPositions();
    const auto& positionsB = major ? axisB.majorPositions() : axisB.minorPositions();

    for (const auto& pa : positionsA) {
        double coordA = (dimA == 0) ? pa.x : (dimA == 1) ? pa.y : pa.z;
        for (const auto& pb : positionsB) {
            double coordB = (dimB == 0) ? pb.x : (dimB == 1) ? pb.y : pb.z;

            Triple p1 = bmin;
            Triple p2 = bmax;

            if (dimA == 0) { p1.x = coordA; p2.x = coordA; }
            else if (dimA == 1) { p1.y = coordA; p2.y = coordA; }
            else { p1.z = coordA; p2.z = coordA; }

            if (dimB == 0) { p1.x = coordB; p2.x = coordB; }
            else if (dimB == 1) { p1.y = coordB; p2.y = coordB; }
            else { p1.z = coordB; p2.z = coordB; }

            verts << static_cast< float >(p1.x) << static_cast< float >(p1.y) << static_cast< float >(p1.z)
                  << r << g << bl << al;
            verts << static_cast< float >(p2.x) << static_cast< float >(p2.y) << static_cast< float >(p2.z)
                  << r << g << bl << al;
        }
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

/**
 * @brief 返回网格线颜色
 * @return RGBA 颜色值
 */
RGBA Qwt3DCoordinateSystem::gridLinesColor() const
{
    QWT_DC(d);
    return d->m_gridlinecolor;
}

void Qwt3DCoordinateSystem::setInteriorGridLines(bool majors, bool minors, int directions)
{
    QWT_D(d);
    d->m_interiorSides  = directions;
    d->m_interiorMajor  = majors;
    d->m_interiorMinor  = minors;
}

int Qwt3DCoordinateSystem::interiorGrids() const
{
    QWT_DC(d);
    return d->m_interiorSides;
}

void Qwt3DCoordinateSystem::setInteriorGridLinesColor(RGBA val)
{
    QWT_D(d);
    d->m_interiorGridColor = val;
}

/**
 * @brief 返回内部网格线颜色
 * @return RGBA 颜色值
 */
RGBA Qwt3DCoordinateSystem::interiorGridLinesColor() const
{
    QWT_DC(d);
    return d->m_interiorGridColor;
}

void Qwt3DCoordinateSystem::setInteriorGridLinesWidth(double major, double minor)
{
    QWT_D(d);
    d->m_interiorMajWidth = major;
    d->m_interiorMinWidth = minor;
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

void Qwt3DCoordinateSystem::setTickPosition(TICKPOSITION val)
{
    QWT_D(d);
    d->m_tickPosition = val;
}

TICKPOSITION Qwt3DCoordinateSystem::tickPosition() const
{
    QWT_DC(d);
    return d->m_tickPosition;
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
