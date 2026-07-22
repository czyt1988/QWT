#include "qwt3d_axis.h"

#include "qwt3d_plot.h"

#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

class Qwt3DAxis::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DAxis)

public:
    PrivateData(Qwt3DAxis* q)
        : q_ptr(q)
        , m_scaleNumberAnchor(Center)
        , m_beg(0.0, 0.0, 0.0)
        , m_end(0.0, 0.0, 0.0)
        , m_start(0.0)
        , m_stop(0.0)
        , m_autoStart(0.0)
        , m_autoStop(0.0)
        , m_lmaj(0.0)
        , m_lmin(0.0)
        , m_orientation(0.0, 0.0, 0.0)
        , m_majorIntervals(0)
        , m_minorIntervals(0)
        , m_lineWidth(1.0)
        , m_majLineWidth(0.9)
        , m_minLineWidth(0.5)
        , m_symtics(false)
        , m_drawNumbers(false)
        , m_drawTics(false)
        , m_drawLabel(false)
        , m_autoScale(true)
        , m_numberFont("Courier", 12)
        , m_labelFont("Courier", 14)
        , m_numberColor(0, 0, 0, 0)
        , m_numberGap(0)
        , m_labelGap(0)
    {
    }

    ANCHOR m_scaleNumberAnchor;
    Qwt3DLabel m_label;
    std::vector< Qwt3DLabel > m_markerLabel;
    Triple m_beg;
    Triple m_end;
    TripleField m_majorPos;
    TripleField m_minorPos;
    Triple m_ncubeBeg;
    Triple m_ncubeEnd;
    double m_start;
    double m_stop;
    double m_autoStart;
    double m_autoStop;
    double m_lmaj;
    double m_lmin;
    Triple m_orientation;
    int m_majorIntervals;
    int m_minorIntervals;
    double m_lineWidth;
    double m_majLineWidth;
    double m_minLineWidth;
    bool m_symtics;
    bool m_drawNumbers;
    bool m_drawTics;
    bool m_drawLabel;
    bool m_autoScale;
    QFont m_numberFont;
    QFont m_labelFont;
    RGBA m_numberColor;
    int m_numberGap;
    int m_labelGap;
    Qwt3DClonePtr< Qwt3DScale > m_scale;
};

/**
 * @brief Default constructor
 * @details Constructs an uninitialized axis with default parameters.
 */
Qwt3DAxis::Qwt3DAxis() : QWT_PIMPL_CONSTRUCT
{
    init();
}

/**
 * @brief Destructor
 */
Qwt3DAxis::~Qwt3DAxis()
{
}

/**
 * @brief Constructs an axis with specified start and end positions
 * @param beg Start position of the axis
 * @param end End position of the axis
 */
Qwt3DAxis::Qwt3DAxis(Triple beg, Triple end) : QWT_PIMPL_CONSTRUCT
{
    init();
    setPosition(beg, end);
}

void Qwt3DAxis::init()
{
    QWT_D(d);

    detachAll();

    d->m_scale = Qwt3DClonePtr< Qwt3DScale >(new Qwt3DLinearScale);

    d->m_beg = Triple(0.0, 0.0, 0.0);
    d->m_end = d->m_beg;

    d->m_majorIntervals = 0;
    d->m_minorIntervals = 0;
    setMajors(1);
    setMinors(1);
    setLimits(0, 0);

    setTicOrientation(0.0, 0.0, 0.0);
    setTicLength(0.0, 0.0);
    setColor(0.0, 0.0, 0.0);
    setLineWidth(1.0);
    d->m_symtics     = false;
    d->m_drawNumbers = false;
    d->m_drawLabel   = false;

    d->m_drawTics  = false;
    d->m_autoScale = true;
    d->m_markerLabel.clear();
    d->m_numberFont = QFont("Courier", 12);
    setLabelFont(QFont("Courier", 14));

    d->m_numberColor = RGBA(0, 0, 0, 0);

    setNumberAnchor(Center);

    d->m_numberGap = 0;
    d->m_labelGap  = 0;
}

void Qwt3DAxis::position(Triple& beg, Triple& end) const
{
    QWT_DC(d);
    beg = d->m_beg;
    end = d->m_end;
}

Triple Qwt3DAxis::begin() const
{
    QWT_DC(d);
    return d->m_beg;
}

Triple Qwt3DAxis::end() const
{
    QWT_DC(d);
    return d->m_end;
}

double Qwt3DAxis::length() const
{
    QWT_DC(d);
    return (d->m_end - d->m_beg).length();
}

void Qwt3DAxis::setPosition(const Triple& beg, const Triple& end)
{
    QWT_D(d);
    d->m_beg = beg;
    d->m_end = end;
}

void Qwt3DAxis::setMajors(int val)
{
    QWT_D(d);
    if (val == d->m_majorIntervals)
        return;

    d->m_majorIntervals = (val <= 0) ? 1 : val;
}

void Qwt3DAxis::setMinors(int val)
{
    QWT_D(d);
    if (val == d->m_minorIntervals)
        return;

    d->m_minorIntervals = (val <= 0) ? 1 : val;
}

void Qwt3DAxis::setTicLength(double majorl, double minorl)
{
    QWT_D(d);
    d->m_lmaj = majorl;
    d->m_lmin = minorl;
}

void Qwt3DAxis::ticLength(double& majorl, double& minorl) const
{
    QWT_DC(d);
    majorl = d->m_lmaj;
    minorl = d->m_lmin;
}

void Qwt3DAxis::setTicOrientation(double tx, double ty, double tz)
{
    setTicOrientation(Triple(tx, ty, tz));
}

void Qwt3DAxis::setTicOrientation(const Triple& val)
{
    QWT_D(d);
    d->m_orientation = val;
    d->m_orientation.normalize();
}

Triple Qwt3DAxis::ticOrientation() const
{
    QWT_DC(d);
    return d->m_orientation;
}

void Qwt3DAxis::setSymmetricTics(bool b)
{
    QWT_D(d);
    d->m_symtics = b;
}

void Qwt3DAxis::setLineWidth(double val, double majfac, double minfac)
{
    QWT_D(d);
    d->m_lineWidth    = val;
    d->m_majLineWidth = majfac * d->m_lineWidth;
    d->m_minLineWidth = minfac * d->m_lineWidth;
}

double Qwt3DAxis::lineWidth() const
{
    QWT_DC(d);
    return d->m_lineWidth;
}

double Qwt3DAxis::majLineWidth() const
{
    QWT_DC(d);
    return d->m_majLineWidth;
}

double Qwt3DAxis::minLineWidth() const
{
    QWT_DC(d);
    return d->m_minLineWidth;
}

void Qwt3DAxis::setLimits(double start, double stop)
{
    QWT_D(d);
    d->m_start = start;
    d->m_stop  = stop;
}

void Qwt3DAxis::limits(double& start, double& stop) const
{
    QWT_DC(d);
    start = d->m_start;
    stop  = d->m_stop;
}

int Qwt3DAxis::majors() const
{
    QWT_DC(d);
    return d->m_majorIntervals;
}

int Qwt3DAxis::minors() const
{
    QWT_DC(d);
    return d->m_minorIntervals;
}

TripleField const& Qwt3DAxis::majorPositions() const
{
    QWT_DC(d);
    return d->m_majorPos;
}

TripleField const& Qwt3DAxis::minorPositions() const
{
    QWT_DC(d);
    return d->m_minorPos;
}

void Qwt3DAxis::setLabel(bool val)
{
    QWT_D(d);
    d->m_drawLabel = val;
}

void Qwt3DAxis::adjustLabel(int val)
{
    QWT_D(d);
    d->m_labelGap = val;
}

void Qwt3DAxis::setScaling(bool val)
{
    QWT_D(d);
    d->m_drawTics = val;
}

bool Qwt3DAxis::scaling() const
{
    QWT_DC(d);
    return d->m_drawTics;
}

void Qwt3DAxis::setNumbers(bool val)
{
    QWT_D(d);
    d->m_drawNumbers = val;
}

bool Qwt3DAxis::numbers() const
{
    QWT_DC(d);
    return d->m_drawNumbers;
}

RGBA Qwt3DAxis::numberColor() const
{
    QWT_DC(d);
    return d->m_numberColor;
}

QFont const& Qwt3DAxis::numberFont() const
{
    QWT_DC(d);
    return d->m_numberFont;
}

QFont const& Qwt3DAxis::labelFont() const
{
    QWT_DC(d);
    return d->m_labelFont;
}

void Qwt3DAxis::setNumberAnchor(ANCHOR a)
{
    QWT_D(d);
    d->m_scaleNumberAnchor = a;
}

void Qwt3DAxis::adjustNumbers(int val)
{
    QWT_D(d);
    d->m_numberGap = val;
}

void Qwt3DAxis::setAutoScale(bool val)
{
    QWT_D(d);
    d->m_autoScale = val;
}

bool Qwt3DAxis::autoScale() const
{
    QWT_DC(d);
    return d->m_autoScale;
}

void Qwt3DAxis::setNumberFont(QString const& family, int pointSize, int weight, bool italic)
{
    QWT_D(d);
    d->m_numberFont = QFont(family, pointSize, weight, italic);
}

void Qwt3DAxis::setNumberFont(QFont const& font)
{
    QWT_D(d);
    d->m_numberFont = font;
}

void Qwt3DAxis::setNumberColor(RGBA col)
{
    QWT_D(d);
    d->m_numberColor = col;
}

void Qwt3DAxis::setLabelFont(QString const& family, int pointSize, int weight, bool italic)
{
    QWT_D(d);
    d->m_labelFont = QFont(family, pointSize, weight, italic);
    d->m_label.setFont(family, pointSize, weight, italic);
}

void Qwt3DAxis::setLabelFont(QFont const& font)
{
    setLabelFont(font.family(), font.pointSize(), font.weight(), font.italic());
}

void Qwt3DAxis::setLabelString(QString const& name)
{
    QWT_D(d);
    d->m_label.setString(name);
}

void Qwt3DAxis::setLabelPosition(const Triple& pos, ANCHOR an)
{
    QWT_D(d);
    d->m_label.setPosition(pos, an);
}

void Qwt3DAxis::setLabelColor(RGBA col)
{
    QWT_D(d);
    d->m_label.setColor(col);
}

void Qwt3DAxis::setScale(Qwt3DScale* val)
{
    QWT_D(d);
    d->m_scale = Qwt3DClonePtr< Qwt3DScale >(val);
}

void Qwt3DAxis::setScale(SCALETYPE val)
{
    switch (val) {
    case LINEARSCALE:
        setScale(new Qwt3DLinearScale);
        break;
    case LOG10SCALE:
        setScale(new Qwt3DLogScale);
        setMinors(9);
        break;
    default:
        break;
    }
}

/**
 * @brief Helper: draws a set of line segments using VBO + line shader
 * @param vertices Interleaved vertex data: 7 floats per vertex (pos.xyz + color.rgba)
 * @param lineWidth Line width in pixels (TODO: may not be supported > 1.0 in Core Profile)
 */
void Qwt3DAxis::drawLines(const QVector<float>& vertices, double lineWidth)
{
    if (vertices.isEmpty() || !plot())
        return;

    auto* shader = plot()->lineShader();
    if (!shader)
        return;

    auto* f = QOpenGLContext::currentContext()->functions();

    // TODO: glLineWidth > 1.0 is not guaranteed in Core Profile (Plan B)
    f->glLineWidth(static_cast< GLfloat >(std::max(1.0, lineWidth)));

    QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(vertices.constData(), vertices.size() * sizeof(float));

    shader->bind();
    shader->setUniformValue("uModelView", plot()->modelViewMatrix());
    shader->setUniformValue("uProjection", plot()->projectionMatrix());

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
 * @brief Draws the axis including base line, tics, and label
 * @details Uses VBO + line shader for line geometry. Labels are drawn
 *          via Qwt3DLabel::draw() which uses its own texture-based rendering.
 */
void Qwt3DAxis::draw()
{
    // Draw children first
    Qwt3DDrawable::draw();

    if (!plot())
        return;

    // Ensure labels have plot pointer set
    QWT_D(d);
    d->m_label.setPlot(plot());
    for (auto& ml : d->m_markerLabel)
        ml.setPlot(plot());

    // Collect and draw base line
    {
        QVector<float> verts;
        // vertex: pos.xyz(3) + color.rgba(4) = 7 floats
        float r = static_cast< float >(color.r);
        float g = static_cast< float >(color.g);
        float b = static_cast< float >(color.b);
        float a = static_cast< float >(color.a);
        verts << static_cast< float >(d->m_beg.x) << static_cast< float >(d->m_beg.y) << static_cast< float >(d->m_beg.z) << r << g << b << a;
        verts << static_cast< float >(d->m_end.x) << static_cast< float >(d->m_end.y) << static_cast< float >(d->m_end.z) << r << g << b << a;
        drawLines(verts, d->m_lineWidth);
    }

    drawTics();
    drawLabel();
}

void Qwt3DAxis::drawLabel()
{
    QWT_D(d);

    if (!d->m_drawLabel || !plot())
        return;

    Triple diff   = end() - begin();
    Triple center = begin() + diff / 2;

    Triple bnumber = biggestNumberString();

    switch (d->m_scaleNumberAnchor) {
    case BottomLeft:
    case TopLeft:
    case CenterLeft:
        bnumber.y = 0;
        break;
    case BottomRight:
    case TopRight:
    case CenterRight:
        bnumber.x = -bnumber.x;
        bnumber.y = 0;
        break;
    case TopCenter:
        bnumber.x = 0;
        bnumber.y = -bnumber.y;
        break;
    case BottomCenter:
        bnumber.x = 0;
        break;
    default:
        break;
    }

    QPointF screen = plot()->worldToScreen(center + ticOrientation() * d->m_lmaj);
    Triple pos = plot()->screenToWorld(screen + QPointF(bnumber.x, bnumber.y));
    setLabelPosition(pos, d->m_scaleNumberAnchor);

    d->m_label.adjust(d->m_labelGap);
    d->m_label.draw();
}

void Qwt3DAxis::drawBase()
{
    // Base line is now drawn directly in draw() via drawLines()
    // This method is kept for API compatibility but does nothing
}

bool Qwt3DAxis::prepTicCalculation(Triple& startpoint)
{
    QWT_D(d);

    if (isPracticallyZero(d->m_start, d->m_stop))
        return false;

    d->m_autoStart = d->m_start;
    d->m_autoStop  = d->m_stop;

    if (autoScale()) {
        setMajors(d->m_scale->autoscale(d->m_autoStart, d->m_autoStop, d->m_start, d->m_stop, majors()));
        if (isPracticallyZero(d->m_autoStart, d->m_autoStop))
            return false;
    }

    d->m_scale->setLimits(d->m_start, d->m_stop);
    d->m_scale->setMajors(majors());
    d->m_scale->setMinors(minors());
    d->m_scale->setMajorLimits(d->m_autoStart, d->m_autoStop);
    d->m_scale->calculate();

    startpoint = d->m_end - d->m_beg;

    d->m_majorPos.clear();
    d->m_minorPos.clear();

    return true;
}

void Qwt3DAxis::recalculateTics()
{
    QWT_D(d);

    Triple runningpoint;
    if (false == prepTicCalculation(runningpoint))
        return;

    unsigned int i;

    const auto& majorsVec = d->m_scale->majorTicks();
    const auto& minorsVec = d->m_scale->minorTicks();
    for (i = 0; i != majorsVec.size(); ++i) {
        double t = (majorsVec[ i ] - d->m_start) / (d->m_stop - d->m_start);
        d->m_majorPos.push_back(d->m_beg + t * runningpoint);
    }
    for (i = 0; i != minorsVec.size(); ++i) {
        double t = (minorsVec[ i ] - d->m_start) / (d->m_stop - d->m_start);
        d->m_minorPos.push_back(d->m_beg + t * runningpoint);
    }
}

void Qwt3DAxis::drawTics()
{
    QWT_D(d);

    Triple runningpoint;
    if (!d->m_drawTics || false == prepTicCalculation(runningpoint))
        return;

    unsigned int i;
    Triple nadir;

    const auto& majorsVec = d->m_scale->majorTicks();
    const auto& minorsVec = d->m_scale->minorTicks();
    d->m_markerLabel.resize(majorsVec.size());

    float r = static_cast< float >(color.r);
    float g = static_cast< float >(color.g);
    float b = static_cast< float >(color.b);
    float a = static_cast< float >(color.a);

    // Collect and draw major tics
    {
        QVector<float> verts;
        for (i = 0; i != majorsVec.size(); ++i) {
            double t = (majorsVec[ i ] - d->m_start) / (d->m_stop - d->m_start);
            nadir    = d->m_beg + t * runningpoint;
            d->m_majorPos.push_back(nadir);

            double ilen = (d->m_symtics) ? -d->m_lmaj : 0.0;
            Triple p1 = nadir + ilen * d->m_orientation;
            Triple p2 = nadir + d->m_lmaj * d->m_orientation;
            verts << static_cast< float >(p1.x) << static_cast< float >(p1.y) << static_cast< float >(p1.z) << r << g << b << a;
            verts << static_cast< float >(p2.x) << static_cast< float >(p2.y) << static_cast< float >(p2.z) << r << g << b << a;
        }
        drawLines(verts, d->m_majLineWidth);

        // Draw tic labels
        for (i = 0; i != majorsVec.size(); ++i) {
            drawTicLabel(nadir + 1.2 * d->m_lmaj * d->m_orientation, static_cast< int >(i));
        }
    }

    // Collect and draw minor tics
    {
        QVector<float> verts;
        for (i = 0; i != minorsVec.size(); ++i) {
            double t = (minorsVec[ i ] - d->m_start) / (d->m_stop - d->m_start);
            nadir    = d->m_beg + t * runningpoint;
            d->m_minorPos.push_back(nadir);

            double ilen = (d->m_symtics) ? -d->m_lmin : 0.0;
            Triple p1 = nadir + ilen * d->m_orientation;
            Triple p2 = nadir + d->m_lmin * d->m_orientation;
            verts << static_cast< float >(p1.x) << static_cast< float >(p1.y) << static_cast< float >(p1.z) << r << g << b << a;
            verts << static_cast< float >(p2.x) << static_cast< float >(p2.y) << static_cast< float >(p2.z) << r << g << b << a;
        }
        drawLines(verts, d->m_minLineWidth);
    }
}

void Qwt3DAxis::drawTicLabel(Triple pos, int mtic)
{
    QWT_D(d);

    if (!d->m_drawNumbers || (mtic < 0) || mtic >= static_cast< int >(d->m_markerLabel.size()))
        return;

    d->m_markerLabel[ mtic ].setFont(
        d->m_numberFont.family(), d->m_numberFont.pointSize(), d->m_numberFont.weight(), d->m_numberFont.italic());
    d->m_markerLabel[ mtic ].setColor(d->m_numberColor);
    d->m_markerLabel[ mtic ].setString(d->m_scale->ticLabel(mtic));
    d->m_markerLabel[ mtic ].setPosition(pos, d->m_scaleNumberAnchor);
    d->m_markerLabel[ mtic ].adjust(d->m_numberGap);
    d->m_markerLabel[ mtic ].setPlot(plot());
    d->m_markerLabel[ mtic ].draw();
}

Triple Qwt3DAxis::drawTic(Triple nadir, double length)
{
    // This method is no longer used for direct GL drawing.
    // Tic geometry is now collected and drawn via drawLines() in drawTics().
    // Kept for API compatibility — returns the nadir position.
    QWT_D(d);
    (void)length;
    (void)d;
    return nadir;
}

Triple Qwt3DAxis::biggestNumberString()
{
    QWT_D(d);

    Triple ret;
    if (!plot())
        return ret;

    size_t size = d->m_markerLabel.size();

    for (unsigned i = 0; i != size; ++i) {
        QPointF first = plot()->worldToScreen(d->m_markerLabel[ i ].first());
        QPointF second = plot()->worldToScreen(d->m_markerLabel[ i ].second());

        double width = fabs(second.x() - first.x());
        double height = fabs(second.y() - first.y());

        if (width > ret.x)
            ret.x = width + d->m_markerLabel[ i ].gap();
        if (height > ret.y)
            ret.y = height + d->m_markerLabel[ i ].gap();
    }
    return ret;
}
