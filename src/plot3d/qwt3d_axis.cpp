#include "qwt3d_axis.h"


class Axis::PrivateData
{
    QWT_DECLARE_PUBLIC(Axis)

public:
    PrivateData(Axis* q)
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
    Label m_label;
    std::vector< Label > m_markerLabel;
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
    ClonePtr< Scale > m_scale;
};

/**
 * @brief Default constructor
 * @details Constructs an uninitialized axis with default parameters.
 */
Axis::Axis() : QWT_PIMPL_CONSTRUCT
{
    init();
}

/**
 * @brief Destructor
 */
Axis::~Axis()
{
}

/**
 * @brief Constructs an axis with specified start and end positions
 * @param beg Start position of the axis
 * @param end End position of the axis
 */
Axis::Axis(Triple beg, Triple end) : QWT_PIMPL_CONSTRUCT
{
    init();
    setPosition(beg, end);
}

void Axis::init()
{
    QWT_D(d);

    detachAll();

    d->m_scale = ClonePtr< Scale >(new LinearScale);

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

void Axis::position(Triple& beg, Triple& end) const
{
    QWT_DC(d);
    beg = d->m_beg;
    end = d->m_end;
}

Triple Axis::begin() const
{
    QWT_DC(d);
    return d->m_beg;
}

Triple Axis::end() const
{
    QWT_DC(d);
    return d->m_end;
}

double Axis::length() const
{
    QWT_DC(d);
    return (d->m_end - d->m_beg).length();
}

/**
 * @brief Sets the axis position
 * @param beg Start position of the axis
 * @param end End position of the axis
 */
void Axis::setPosition(const Triple& beg, const Triple& end)
{
    QWT_D(d);
    d->m_beg = beg;
    d->m_end = end;
}

/**
 * @brief Sets number of major intervals
 * @param val Number of major intervals (always >= 1)
 */
void Axis::setMajors(int val)
{
    QWT_D(d);
    if (val == d->m_majorIntervals)
        return;

    d->m_majorIntervals = (val <= 0) ? 1 : val;  // always >= 1
}

/**
 * @brief Sets number of minor intervals
 * @param val Number of minor intervals (always >= 1)
 * @see LogScale::setMinors()
 */
void Axis::setMinors(int val)
{
    QWT_D(d);
    if (val == d->m_minorIntervals)
        return;

    d->m_minorIntervals = (val <= 0) ? 1 : val;  // always >= 1
}

/**
 * @brief Sets tic length
 * @param majorl Length of major tics
 * @param minorl Length of minor tics
 */
void Axis::setTicLength(double majorl, double minorl)
{
    QWT_D(d);
    d->m_lmaj = majorl;
    d->m_lmin = minorl;
}

void Axis::ticLength(double& majorl, double& minorl) const
{
    QWT_DC(d);
    majorl = d->m_lmaj;
    minorl = d->m_lmin;
}

/**
 * @brief Sets tic orientation from individual components
 * @param tx X component of tic orientation
 * @param ty Y component of tic orientation
 * @param tz Z component of tic orientation
 */
void Axis::setTicOrientation(double tx, double ty, double tz)
{
    setTicOrientation(Triple(tx, ty, tz));
}

/**
 * @brief Sets tic orientation from a Triple vector
 * @param val Orientation vector for tics (will be normalized)
 */
void Axis::setTicOrientation(const Triple& val)
{
    QWT_D(d);
    d->m_orientation = val;
    d->m_orientation.normalize();
}

Triple Axis::ticOrientation() const
{
    QWT_DC(d);
    return d->m_orientation;
}

void Axis::setSymmetricTics(bool b)
{
    QWT_D(d);
    d->m_symtics = b;
}

/**
 * @brief Sets line width for axis and tics
 * @param val Thickness for axis base line
 * @param majfac Relative thickness for axis major tics (majfac*val)
 * @param minfac Relative thickness for axis minor tics (minfac*val)
 */
void Axis::setLineWidth(double val, double majfac, double minfac)
{
    QWT_D(d);
    d->m_lineWidth    = val;
    d->m_majLineWidth = majfac * d->m_lineWidth;
    d->m_minLineWidth = minfac * d->m_lineWidth;
}

double Axis::lineWidth() const
{
    QWT_DC(d);
    return d->m_lineWidth;
}

double Axis::majLineWidth() const
{
    QWT_DC(d);
    return d->m_majLineWidth;
}

double Axis::minLineWidth() const
{
    QWT_DC(d);
    return d->m_minLineWidth;
}

void Axis::setLimits(double start, double stop)
{
    QWT_D(d);
    d->m_start = start;
    d->m_stop  = stop;
}

void Axis::limits(double& start, double& stop) const
{
    QWT_DC(d);
    start = d->m_start;
    stop  = d->m_stop;
}

int Axis::majors() const
{
    QWT_DC(d);
    return d->m_majorIntervals;
}

int Axis::minors() const
{
    QWT_DC(d);
    return d->m_minorIntervals;
}

TripleField const& Axis::majorPositions() const
{
    QWT_DC(d);
    return d->m_majorPos;
}

TripleField const& Axis::minorPositions() const
{
    QWT_DC(d);
    return d->m_minorPos;
}

void Axis::setLabel(bool val)
{
    QWT_D(d);
    d->m_drawLabel = val;
}

void Axis::adjustLabel(int val)
{
    QWT_D(d);
    d->m_labelGap = val;
}

void Axis::setScaling(bool val)
{
    QWT_D(d);
    d->m_drawTics = val;
}

bool Axis::scaling() const
{
    QWT_DC(d);
    return d->m_drawTics;
}

void Axis::setNumbers(bool val)
{
    QWT_D(d);
    d->m_drawNumbers = val;
}

bool Axis::numbers() const
{
    QWT_DC(d);
    return d->m_drawNumbers;
}

RGBA Axis::numberColor() const
{
    QWT_DC(d);
    return d->m_numberColor;
}

QFont const& Axis::numberFont() const
{
    QWT_DC(d);
    return d->m_numberFont;
}

QFont const& Axis::labelFont() const
{
    QWT_DC(d);
    return d->m_labelFont;
}

void Axis::setNumberAnchor(ANCHOR a)
{
    QWT_D(d);
    d->m_scaleNumberAnchor = a;
}

void Axis::adjustNumbers(int val)
{
    QWT_D(d);
    d->m_numberGap = val;
}

void Axis::setAutoScale(bool val)
{
    QWT_D(d);
    d->m_autoScale = val;
}

bool Axis::autoScale() const
{
    QWT_DC(d);
    return d->m_autoScale;
}

/**
 * @brief Draws the axis including base line, tics, and label
 */
void Axis::draw()
{
    Drawable::draw();

    saveGLState();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4d(color.r, color.g, color.b, color.a);

    drawBase();
    drawTics();
    drawLabel();

    restoreGLState();
}

void Axis::drawLabel()
{
    QWT_D(d);

    if (!d->m_drawLabel)
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

    Triple pos = ViewPort2World(World2ViewPort(center + ticOrientation() * d->m_lmaj) + bnumber);
    setLabelPosition(pos, d->m_scaleNumberAnchor);

    d->m_label.adjust(d->m_labelGap);
    d->m_label.draw();
}

void Axis::drawBase()
{
    QWT_D(d);
    setDeviceLineWidth(d->m_lineWidth);
    glBegin(GL_LINES);
    glVertex3d(d->m_beg.x, d->m_beg.y, d->m_beg.z);
    glVertex3d(d->m_end.x, d->m_end.y, d->m_end.z);
    glEnd();
}

bool Axis::prepTicCalculation(Triple& startpoint)
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

void Axis::recalculateTics()
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

void Axis::drawTics()
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
    setDeviceLineWidth(d->m_majLineWidth);
    for (i = 0; i != majorsVec.size(); ++i) {
        double t = (majorsVec[ i ] - d->m_start) / (d->m_stop - d->m_start);
        nadir    = d->m_beg + t * runningpoint;
        d->m_majorPos.push_back(drawTic(nadir, d->m_lmaj));
        drawTicLabel(nadir + 1.2 * d->m_lmaj * d->m_orientation, i);
    }
    setDeviceLineWidth(d->m_minLineWidth);
    for (i = 0; i != minorsVec.size(); ++i) {
        double t = (minorsVec[ i ] - d->m_start) / (d->m_stop - d->m_start);
        nadir    = d->m_beg + t * runningpoint;
        d->m_minorPos.push_back(drawTic(nadir, d->m_lmin));
    }
}

void Axis::drawTicLabel(Triple pos, int mtic)
{
    QWT_D(d);

    if (!d->m_drawNumbers || (mtic < 0))
        return;

    d->m_markerLabel[ mtic ].setFont(
        d->m_numberFont.family(), d->m_numberFont.pointSize(), d->m_numberFont.weight(), d->m_numberFont.italic());
    d->m_markerLabel[ mtic ].setColor(d->m_numberColor);
    d->m_markerLabel[ mtic ].setString(d->m_scale->ticLabel(mtic));
    d->m_markerLabel[ mtic ].setPosition(pos, d->m_scaleNumberAnchor);
    d->m_markerLabel[ mtic ].adjust(d->m_numberGap);
    d->m_markerLabel[ mtic ].draw();
}

Triple Axis::drawTic(Triple nadir, double length)
{
    QWT_D(d);

    double ilength = (d->m_symtics) ? -length : 0.0;

    glBegin(GL_LINES);
    glVertex3d(nadir.x + ilength * d->m_orientation.x,
               nadir.y + ilength * d->m_orientation.y,
               nadir.z + ilength * d->m_orientation.z);
    glVertex3d(nadir.x + length * d->m_orientation.x,
               nadir.y + length * d->m_orientation.y,
               nadir.z + length * d->m_orientation.z);
    glEnd();
    return nadir;
}

/**
 * @brief Sets the font for axis numbers
 * @param family Font family name
 * @param pointSize Font point size
 * @param weight Font weight
 * @param italic Whether font is italic
 */
void Axis::setNumberFont(QString const& family, int pointSize, int weight, bool italic)
{
    QWT_D(d);
    d->m_numberFont = QFont(family, pointSize, weight, italic);
}

/**
 * @brief Sets the font for axis numbers
 * @param font QFont object to use for axis numbers
 */
void Axis::setNumberFont(QFont const& font)
{
    QWT_D(d);
    d->m_numberFont = font;
}

/**
 * @brief Sets the color for axis numbers
 * @param col RGBA color value for axis numbers
 */
void Axis::setNumberColor(RGBA col)
{
    QWT_D(d);
    d->m_numberColor = col;
}

/**
 * @brief Sets the font for the axis label
 * @param family Font family name
 * @param pointSize Font point size
 * @param weight Font weight
 * @param italic Whether font is italic
 */
void Axis::setLabelFont(QString const& family, int pointSize, int weight, bool italic)
{
    QWT_D(d);
    d->m_labelFont = QFont(family, pointSize, weight, italic);
    d->m_label.setFont(family, pointSize, weight, italic);
}

/**
 * @brief Sets the font for the axis label
 * @param font QFont object to use for the axis label
 */
void Axis::setLabelFont(QFont const& font)
{
    setLabelFont(font.family(), font.pointSize(), font.weight(), font.italic());
}

/**
 * @brief Sets the axis label string
 * @param name The label text string
 */
void Axis::setLabelString(QString const& name)
{
    QWT_D(d);
    d->m_label.setString(name);
}

/**
 * @brief Sets label position in conjunction with an anchoring strategy
 * @param pos Position for the label
 * @param an Anchor strategy for the label
 */
void Axis::setLabelPosition(const Triple& pos, ANCHOR an)
{
    QWT_D(d);
    d->m_label.setPosition(pos, an);
}

/**
 * @brief Sets color for the axis label
 * @param col RGBA color value for the label
 */
void Axis::setLabelColor(RGBA col)
{
    QWT_D(d);
    d->m_label.setColor(col);
}

Triple Axis::biggestNumberString()
{
    QWT_D(d);

    Triple ret;
    size_t size = d->m_markerLabel.size();

    double width, height;

    for (unsigned i = 0; i != size; ++i) {
        width = fabs((World2ViewPort(d->m_markerLabel[ i ].second()) - World2ViewPort(d->m_markerLabel[ i ].first())).x);
        height = fabs((World2ViewPort(d->m_markerLabel[ i ].second()) - World2ViewPort(d->m_markerLabel[ i ].first())).y);

        if (width > ret.x)
            ret.x = width + d->m_markerLabel[ i ].gap();
        if (height > ret.y)
            ret.y = height + d->m_markerLabel[ i ].gap();
    }
    return ret;
}

/**
 * @brief Sets a user-defined scale object
 * @param val Pointer to a Scale object. Use with a heap based initialized pointer only.
 *            The axis adopts ownership.
 */
void Axis::setScale(Scale* val)
{
    QWT_D(d);
    d->m_scale = ClonePtr< Scale >(val);
}

/**
 * @brief Sets one of the predefined scaling types
 * @param val Predefined scale type (LINEARSCALE or LOG10SCALE)
 * @warning Too small intervals in logarithmic scales lead to empty scales
 *          (or perhaps a scale only containing an isolated major tic).
 *          Better switch to linear scales in such cases.
 */
void Axis::setScale(SCALETYPE val)
{
    switch (val) {
    case LINEARSCALE:
        setScale(new LinearScale);
        break;
    case LOG10SCALE:
        setScale(new LogScale);
        setMinors(9);
        break;
    default:
        break;
    }
}
