#include "qwt3d_scale.h"
#include "qwt3d_autoscaler.h"


/****************************
 *
 * Qwt3DScale::PrivateData
 *
 ****************************/

class Qwt3DScale::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DScale)
public:
    explicit PrivateData(Qwt3DScale* p);

    friend class Qwt3DAxis;

    std::vector< double > m_majors, m_minors;
    double m_start, m_stop;
    int m_majorIntervals, m_minorIntervals;
    double m_mstart, m_mstop;
};

Qwt3DScale::PrivateData::PrivateData(Qwt3DScale* p)
    : q_ptr(p), m_start(0.), m_stop(0.), m_majorIntervals(0), m_minorIntervals(0), m_mstart(0.), m_mstop(0.)
{
}

/****************************
 *
 * Qwt3DScale
 *
 ****************************/

Qwt3DScale::Qwt3DScale() : QWT_PIMPL_CONSTRUCT
{
}

Qwt3DScale::~Qwt3DScale() = default;

void Qwt3DScale::destroy() const
{
    delete this;
}

/**
 * @brief Copies Qwt3DScale base state from another Qwt3DScale
 * @param other Source Qwt3DScale to copy from
 * @details Used by derived class clone() implementations to copy
 *          the Qwt3DScale base data without requiring a copy constructor.
 */
void Qwt3DScale::copyFrom(const Qwt3DScale& other)
{
    QWT_D(d);
    const auto* od      = other.d_func();
    d->m_majors         = od->m_majors;
    d->m_minors         = od->m_minors;
    d->m_start          = od->m_start;
    d->m_stop           = od->m_stop;
    d->m_majorIntervals = od->m_majorIntervals;
    d->m_minorIntervals = od->m_minorIntervals;
    d->m_mstart         = od->m_mstart;
    d->m_mstop          = od->m_mstop;
}

/**
 * @brief Maps the double value at tic-position idx to a final representation
 * @param idx The current major tic index
 * @return The QString representation for the value corresponding to a valid index, an empty QString else.
 * @details The default return value is simply the tic values QString representation.
 *          Overwrite this function, if you plan to transform the value in some way.
 *          See e.g. Qwt3DLogScale::ticLabel.
 */
QString Qwt3DScale::ticLabel(unsigned int idx) const
{
    QWT_DC(d);
    if (idx < d->m_majors.size()) {
        return QString::number(d->m_majors[ idx ]);
    }
    return QString("");
}

/**
 * @brief Sets start and stop value for the scale
 * @param start Scale start value
 * @param stop Scale stop value
 */
void Qwt3DScale::setLimits(double start, double stop)
{
    QWT_D(d);
    if (start < stop) {
        d->m_start = start;
        d->m_stop  = stop;
        return;
    }
    d->m_start = stop;
    d->m_stop  = start;
}

/**
 * @brief Sets number of major intervals
 * @param val Number of major intervals
 */
void Qwt3DScale::setMajors(int val)
{
    QWT_D(d);
    d->m_majorIntervals = val;
}

/**
 * @brief Sets number of minor intervals per major interval
 * @param val Number of minor intervals
 */
void Qwt3DScale::setMinors(int val)
{
    QWT_D(d);
    d->m_minorIntervals = val;
}

/**
 * @brief Sets value of first and last major tic
 * @param start First major tic value
 * @param stop Last major tic value
 */
void Qwt3DScale::setMajorLimits(double start, double stop)
{
    QWT_D(d);
    if (start < stop) {
        d->m_mstart = start;
        d->m_mstop  = stop;
        return;
    }
    d->m_mstart = stop;
    d->m_mstop  = start;
}

/**
 * @brief Returns major intervals
 * @return Number of major intervals
 */
int Qwt3DScale::majors() const
{
    QWT_DC(d);
    return d->m_majorIntervals;
}

/**
 * @brief Returns minor intervals
 * @return Number of minor intervals
 */
int Qwt3DScale::minors() const
{
    QWT_DC(d);
    return d->m_minorIntervals;
}

/**
 * @brief Returns const reference to major tic positions
 * @return Const reference to the vector of major tic positions
 */
const std::vector< double >& Qwt3DScale::majorTicks() const
{
    QWT_DC(d);
    return d->m_majors;
}

/**
 * @brief Returns const reference to minor tic positions
 * @return Const reference to the vector of minor tic positions
 */
const std::vector< double >& Qwt3DScale::minorTicks() const
{
    QWT_DC(d);
    return d->m_minors;
}

/**
 * @brief Autoscales the axis
 * @param[out] a First major tic after applying autoscaling
 * @param[out] b Last major tic after applying autoscaling
 * @param start Scale begin
 * @param stop Scale end
 * @param ivals Requested number of major intervals
 * @return Number of major intervals after autoscaling
 * @details The default implementation sets a=start, b=stop and returns ivals.
 */
int Qwt3DScale::autoscale(double& a, double& b, double start, double stop, int ivals)
{
    a = start;
    b = stop;
    return ivals;
}

/****************************
 *
 * Qwt3DLinearScale::PrivateData
 *
 ****************************/

class Qwt3DLinearScale::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DLinearScale)
public:
    explicit PrivateData(Qwt3DLinearScale* p);

    Qwt3DLinearAutoScaler m_autoscaler;
};

Qwt3DLinearScale::PrivateData::PrivateData(Qwt3DLinearScale* p) : q_ptr(p)
{
}

/****************************
 *
 * Qwt3DLinearScale
 *
 ****************************/

Qwt3DLinearScale::Qwt3DLinearScale() : QWT_PIMPL_CONSTRUCT
{
}

Qwt3DLinearScale::~Qwt3DLinearScale() = default;

/**
 * @brief Applies Qwt3DLinearAutoScaler::execute() for autoscaling
 * @param[out] a First major tic after autoscaling
 * @param[out] b Last major tic after autoscaling
 * @param start Scale begin
 * @param stop Scale end
 * @param ivals Requested number of major intervals
 * @return Number of major intervals after autoscaling
 */
int Qwt3DLinearScale::autoscale(double& a, double& b, double start, double stop, int ivals)
{
    QWT_D(d);
    return d->m_autoscaler.execute(a, b, start, stop, ivals);
}

/**
 * @brief Returns a new heap based object utilized from Qwt3DClonePtr
 * @return A new Qwt3DLinearScale copy
 */
Qwt3DScale* Qwt3DLinearScale::clone() const
{
    auto* copy = new Qwt3DLinearScale();
    copy->copyFrom(*this);
    QWT_DC(d);
    auto* copyD = copy->d_func();
    copyD->m_autoscaler.copyStateFrom(d->m_autoscaler);
    return copy;
}

/**
 * @brief Creates the major and minor vector for the scale
 */
void Qwt3DLinearScale::calculate()
{
    // Access Qwt3DScale base data through Qwt3DScale::d_func()
    auto* sd = Qwt3DScale::d_func();

    sd->m_majors.clear();
    sd->m_minors.clear();

    double interval = sd->m_mstop - sd->m_mstart;

    double runningval;
    int i = 0;

    // majors

    // first tic
    sd->m_majors.push_back(sd->m_mstart);

    // remaining tics
    for (i = 1; i <= sd->m_majorIntervals; ++i) {
        double t   = double(i) / sd->m_majorIntervals;
        runningval = sd->m_mstart + t * interval;
        if (runningval > sd->m_stop)
            break;
        if (isPracticallyZero(sd->m_mstart, -t * interval))  // prevent rounding errors near 0
            runningval = 0.0;
        sd->m_majors.push_back(runningval);
    }
    sd->m_majorIntervals = static_cast< int >(sd->m_majors.size());
    if (sd->m_majorIntervals)
        --sd->m_majorIntervals;

    // minors

    if (!sd->m_majorIntervals || !sd->m_minorIntervals)  // no valid interval
    {
        sd->m_minorIntervals = 0;
        return;
    }

    // sd->m_start      sd->m_mstart
    //  |_____________|_____ _ _ _

    double step = (sd->m_majors[ 1 ] - sd->m_majors[ 0 ]) / sd->m_minorIntervals;
    if (isPracticallyZero(step))
        return;

    runningval = sd->m_mstart - step;
    while (runningval > sd->m_start) {
        sd->m_minors.push_back(runningval);
        runningval -= step;
    }

    //       sd->m_mstart            sd->m_mstop
    //  ________|_____ _ _ _ _ _ ___|__________

    for (i = 0; i != sd->m_majorIntervals; ++i) {
        runningval = sd->m_majors[ i ] + step;
        for (int j = 0; j != sd->m_minorIntervals; ++j) {
            sd->m_minors.push_back(runningval);
            runningval += step;
        }
    }

    //    sd->m_mstop       sd->m_stop
    // _ _ _|_____________|

    runningval = sd->m_mstop + step;
    while (runningval < sd->m_stop) {
        sd->m_minors.push_back(runningval);
        runningval += step;
    }
}

/****************************
 *
 * Qwt3DLogScale
 *
 ****************************/

void Qwt3DLogScale::setupCounter(double& k, int& step)
{
    auto* sd = Qwt3DScale::d_func();
    switch (sd->m_minorIntervals) {
    case 9:
        k    = 9;
        step = 1;
        break;
    case 5:
        k    = 8;
        step = 2;
        break;
    case 3:
        k    = 5;
        step = 3;
        break;
    case 2:
        k    = 5;
        step = 5;
        break;
    default:
        k    = 9;
        step = 1;
    }
}

/**
 * @brief Creates major and minor vectors for the logarithmic scale
 * @warning If the interval is too small, the scale becomes empty or will contain
 *          only a single major tic. There is no automatism (also not planned for now)
 *          for an 'intelligent' guess, what to do. Better switch manually to linear
 *          scales in such cases.
 */
void Qwt3DLogScale::calculate()
{
    auto* sd = Qwt3DScale::d_func();

    sd->m_majors.clear();
    sd->m_minors.clear();

    if (sd->m_start < DBL_MIN_10_EXP)
        sd->m_start = DBL_MIN_10_EXP;
    if (sd->m_stop > DBL_MAX_10_EXP)
        sd->m_stop = DBL_MAX_10_EXP;

    double interval = sd->m_stop - sd->m_start;
    if (interval <= 0)
        return;

    double runningval = floor(sd->m_start);
    while (runningval <= sd->m_stop) {
        if (runningval >= sd->m_start)
            sd->m_majors.push_back(runningval);
        ++runningval;
    }
    sd->m_majorIntervals = static_cast< int >(sd->m_majors.size());
    if (sd->m_majorIntervals)
        --sd->m_majorIntervals;

    if (sd->m_majors.size() < 1)  // not even a single major tic
    {
        return;
    }

    // minors

    // sd->m_start      sd->m_mstart
    //  |_____________|_____ _ _ _

    double k;
    int step;
    setupCounter(k, step);
    runningval = log10(k) + (sd->m_majors[ 0 ] - 1);
    while (runningval > sd->m_start && k > 1) {
        sd->m_minors.push_back(runningval);
        k -= step;
        runningval = log10(k) + (sd->m_majors[ 0 ] - 1);
    }

    //       sd->m_mstart            sd->m_mstop
    //  ________|_____ _ _ _ _ _ ___|__________

    for (int i = 0; i != sd->m_majorIntervals; ++i) {
        setupCounter(k, step);
        runningval = log10(k) + (sd->m_majors[ i ]);
        while (k > 1) {
            sd->m_minors.push_back(runningval);
            k -= step;
            runningval = log10(k) + (sd->m_majors[ i ]);
        }
    }

    //    sd->m_mstop       sd->m_stop
    // _ _ _|_____________|

    setupCounter(k, step);
    runningval = log10(k) + (sd->m_majors.back());
    do {
        k -= step;
        runningval = log10(k) + (sd->m_majors.back());
    } while (runningval >= sd->m_stop);
    while (k > 1) {
        sd->m_minors.push_back(runningval);
        k -= step;
        runningval = log10(k) + (sd->m_majors.back());
    }
}

/**
 * @brief Sets the minor intervals for the logarithmic scale
 * @param val Number of minor intervals (only 9, 5, 3, or 2 are accepted)
 * @details They will produce mantissa sets of {2,3,4,5,6,7,8,9}, {2,4,6,8}, {2,5} or {5} respectively.
 */
void Qwt3DLogScale::setMinors(int val)
{
    auto* sd = Qwt3DScale::d_func();
    if ((val == 2) || (val == 3) || (val == 5) || (val == 9))
        sd->m_minorIntervals = val;
}

/**
 * @brief Default constructor - sets 9 minor intervals
 */
Qwt3DLogScale::Qwt3DLogScale()
{
    auto* sd             = Qwt3DScale::d_func();
    sd->m_minorIntervals = 9;
}

Qwt3DLogScale::~Qwt3DLogScale() = default;

/**
 * @brief Returns a new heap based object utilized from Qwt3DClonePtr
 * @return A new Qwt3DLogScale copy
 */
Qwt3DScale* Qwt3DLogScale::clone() const
{
    auto* copy = new Qwt3DLogScale();
    copy->copyFrom(*this);
    return copy;
}

/**
 * @brief Returns a power of 10 associated to the major value at index idx
 * @param idx The current major tic index
 * @return The QString representation of 10^value for valid index, empty QString else
 */
QString Qwt3DLogScale::ticLabel(unsigned int idx) const
{
    QWT_DC(d);
    if (idx < d->m_majors.size()) {
        double val = d->m_majors[ idx ];
        return QString::number(pow(double(10), val));
    }
    return QString("");
}
