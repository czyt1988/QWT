#include "qwt3d_helper.h"
#include "qwt3d_autoscaler.h"


namespace
{

double floorExt(int& exponent, double x, std::vector< double >& sortedmantissi)
{
    if (x == 0.0) {
        exponent = 0;
        return 0.0;
    }

    double sign = (x > 0) ? 1.0 : -1.0;
    double lx   = log10(fabs(x));
    exponent    = static_cast< int >(floor(lx));

    double fr = pow(10.0, lx - exponent);
    if (fr >= 10.0) {
        fr = 1.0;
        ++exponent;
    } else {
        for (int i = static_cast< int >(sortedmantissi.size()) - 1; i >= 0; --i) {
            if (fr >= sortedmantissi[ i ]) {
                fr = sortedmantissi[ i ];
                break;
            }
        }
    }
    return sign * fr;
}

/*
  @brief Find the largest value out of {1,2,5}*10^n with an integer number n
  which is smaller than or equal to x
  @param exponent n
  @param x Input value
  @return Mantissa
*/
double floor125(int& exponent, double x)
{
    std::vector< double > m(2);
    m[ 0 ] = 1;
    m[ 1 ] = 2;
    m[ 2 ] = 5;
    return floorExt(exponent, x, m);
}

}  // anon ns

/****************************
 *
 * LinearAutoScaler::PrivateData
 *
 ****************************/

class LinearAutoScaler::PrivateData
{
    QWT_DECLARE_PUBLIC(LinearAutoScaler)
public:
    explicit PrivateData(LinearAutoScaler* p);

    double m_start, m_stop;
    int m_intervals;
    std::vector< double > m_mantissi;
};

LinearAutoScaler::PrivateData::PrivateData(LinearAutoScaler* p) : q_ptr(p), m_start(0.), m_stop(0.), m_intervals(0)
{
}

/****************************
 *
 * LinearAutoScaler
 *
 ****************************/

//! Initializes with an {1,2,5} sequence of mantissas
LinearAutoScaler::LinearAutoScaler() : QWT_PIMPL_CONSTRUCT
{
    init(0, 1, 1);
    QWT_D(d);
    d->m_mantissi      = std::vector< double >(3);
    d->m_mantissi[ 0 ] = 1;
    d->m_mantissi[ 1 ] = 2;
    d->m_mantissi[ 2 ] = 5;
}

//! Initialize with interval [0,1] and one requested interval
/*!
val mantisse A increasing ordered vector of values representing
mantisse values between 1 and 9.
*/
LinearAutoScaler::LinearAutoScaler(std::vector< double >& mantisse) : QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    init(0, 1, 1);
    if (mantisse.empty()) {
        d->m_mantissi      = std::vector< double >(3);
        d->m_mantissi[ 0 ] = 1;
        d->m_mantissi[ 1 ] = 2;
        d->m_mantissi[ 2 ] = 5;
        return;
    }
    d->m_mantissi = mantisse;
}

LinearAutoScaler::~LinearAutoScaler() = default;

/**
 * @brief Copies internal state from another LinearAutoScaler
 * @param other Source object to copy state from
 * @details Used by LinearScale::clone() to copy autoscaler state
 *          without requiring a copy constructor or assignment operator.
 */
void LinearAutoScaler::copyStateFrom(const LinearAutoScaler& other)
{
    QWT_D(d);
    const auto* od = other.d_func();
    d->m_start     = od->m_start;
    d->m_stop      = od->m_stop;
    d->m_intervals = od->m_intervals;
    d->m_mantissi  = od->m_mantissi;
}

/**
 * @brief Returns a deep copy of this autoscaler
 * @return A new LinearAutoScaler with identical state
 */
AutoScaler* LinearAutoScaler::clone() const
{
    auto* copy = new LinearAutoScaler();
    QWT_DC(d);
    auto* copyD        = copy->d_func();
    copyD->m_start     = d->m_start;
    copyD->m_stop      = d->m_stop;
    copyD->m_intervals = d->m_intervals;
    copyD->m_mantissi  = d->m_mantissi;
    return copy;
}

//! Initialize with interval [start,stop] and number of requested intervals
/**
        Switchs start and stop, if stop < start and sets intervals = 1 if ivals < 1
*/
void LinearAutoScaler::init(double start, double stop, int ivals)
{
    QWT_D(d);
    d->m_start     = start;
    d->m_stop      = stop;
    d->m_intervals = ivals;

    if (d->m_start > d->m_stop) {
        double tmp = d->m_start;
        d->m_start = d->m_stop;
        d->m_stop  = tmp;
    }
    if (d->m_intervals < 1)
        d->m_intervals = 1;
}

/*!
@return Anchor value

@verbatim
|_______|____________ _ _ _ _  _____|_____________|________________

0     m*10^n                      start         anchor := c*m*10^n

c 'minimal' (anchor-start < m*10^n)
@endverbatim
*/
double LinearAutoScaler::anchorvalue(double start, double m, int n)
{
    double stepval = m * pow(10.0, n);
    return stepval * ceil(start / stepval);
}

/*!
@return New number of intervals (:= l_intervals + r_intervals)
@param l_intervals  Number of intervals left from anchor
@param r_intervals  Number of intervals right from anchor

@verbatim
                          -l_intervals * i    -2 * i    -i                 +r_intervals * i
                                                                                   |
|______|_______ _ _ _ ____|____|___ _ _ _ _ _ _ _|_______|_______|_ _ _ _ _ _ _____|__|_____
       |                  |                                      |                    |
0   i := m*10^n         start                                  anchor	              stop

c 'minimal' (anchor-start < m*10^n)
@endverbatim
*/
int LinearAutoScaler::segments(int& l_intervals, int& r_intervals, double start, double stop, double anchor, double m, int n)
{
    double val   = m * pow(10.0, n);
    double delta = (stop - anchor) / val;

    r_intervals = static_cast< int >(floor(delta));  // right side intervals

    delta = (anchor - start) / val;

    l_intervals = static_cast< int >(floor(delta));  // left side intervals

    return r_intervals + l_intervals;
}

/*!
        @brief Does the actual scaling
        @return Number of intervals after rescaling. This will in the most cases differ
        from the requested interval number!  Always >0.
        @param a Start value after scaling (always >= start)
        @param b Stop value after scaling  (always <= stop)
  @param start Start value
  @param stop Stop value
  @param ivals Requested intervals
  @return Number of intervals after autoscaling

        If the given interval has zero length the function returns the current
        interval number and a and b remain unchanged.
*/
int LinearAutoScaler::execute(double& a, double& b, double start, double stop, int ivals)
{
    init(start, stop, ivals);

    QWT_D(d);

    double delta = d->m_stop - d->m_start;

    if (isPracticallyZero(delta))
        return d->m_intervals;

    double c;
    int n;

    c = floorExt(n, delta, d->m_mantissi);

    int l_ival, r_ival;

    double anchor = anchorvalue(d->m_start, c, n);
    int ival      = segments(l_ival, r_ival, d->m_start, d->m_stop, anchor, c, n);

    if (ival >= d->m_intervals) {
        a              = anchor - l_ival * c * pow(10.0, n);
        b              = anchor + r_ival * c * pow(10.0, n);
        d->m_intervals = ival;
        return d->m_intervals;
    }

    int prev_ival, prev_l_ival, prev_r_ival;
    double prev_anchor;
    double prev_c;
    int prev_n;

    while (1) {
        prev_c      = c;
        prev_n      = n;
        prev_anchor = anchor;
        prev_ival   = ival;
        prev_l_ival = l_ival;
        prev_r_ival = r_ival;

        if (int(c) == 1) {
            c = d->m_mantissi.back();
            --n;
        } else {
            for (size_t i = d->m_mantissi.size() - 1; i > 0; --i) {
                if (int(c) == d->m_mantissi[ i ]) {
                    c = d->m_mantissi[ i - 1 ];
                    break;
                }
            }
        }

        anchor = anchorvalue(d->m_start, c, n);
        ival   = segments(l_ival, r_ival, d->m_start, d->m_stop, anchor, c, n);

        int prev_diff   = d->m_intervals - prev_ival;
        int actual_diff = ival - d->m_intervals;

        if (prev_diff >= 0 && actual_diff >= 0) {
            if (prev_diff < actual_diff) {
                c      = prev_c;
                n      = prev_n;
                anchor = prev_anchor;
                ival   = prev_ival;
                l_ival = prev_l_ival;
                r_ival = prev_r_ival;
            }
            a              = anchor - l_ival * c * pow(10.0, n);
            b              = anchor + r_ival * c * pow(10.0, n);
            d->m_intervals = ival;
            break;
        }
    }
    return d->m_intervals;
}
