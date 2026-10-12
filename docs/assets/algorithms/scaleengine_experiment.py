"""
Numerical experiments for the scale engine article ( "nice numbers" ).

Faithful Python ports of:
  * QwtScaleArithmetic / QwtLinearScaleEngine   ( src/core/qwt_scale_engine.cpp )
  * QwtLogScaleEngine                           ( src/core/qwt_scale_engine.cpp )
  * QwtDateScaleEngine                          ( src/plot/qwt_date_scale_engine.cpp )
  * QwtDate floor/ceil/week0 helpers            ( src/core/qwt_date.cpp )

Every floating point decision ( epsilon arithmetic, integer halving loop,
fuzzy compares, qRound / qwtCeil / truncation ) mirrors the C++ code path
statement by statement, so the results can be cross-checked against a small
C++ harness ( see --cpp-check below and scaleengine_check.cpp ).

Usage:
    python scaleengine_experiment.py                 # run all experiments, write figures
    python scaleengine_experiment.py --no-figures    # experiments only
    python scaleengine_experiment.py --cpp-check <harness.exe>

All raw numbers are printed to stdout.  Deterministic: seed 0x5CA1E.
Requires: Python 3.11, numpy, matplotlib (Agg).  No other third party deps.
"""

import math
import sys
import os
from datetime import datetime, date, timedelta, timezone

import numpy as np

SEED = 0x5CA1E
DBL_MAX = sys.float_info.max
CS_EPS = 1.0e-6          # cs_eps_ in qwt_scale_engine.cpp
ALIGN_EPS = 1.0e-12      # the hard coded eps in QwtLinearScaleEngine::align
LOG_MIN = 1.0e-150       # QwtLogTransform::LogMin
LOG_MAX = 1.0e150        # QwtLogTransform::LogMax

# QwtPlot axis defaults ( src/plot/qwt_plot_axis.cpp, class AxisData )
PLOT_MAX_MAJOR = 8
PLOT_MAX_MINOR = 5


# ---------------------------------------------------------------- C++ helpers

def qround(v):
    """C++ qRound: round half away from zero."""
    return int(math.floor(v + 0.5)) if v >= 0.0 else int(math.ceil(v - 0.5))


def qwt_ceil(v):
    """qwtCeil: int( ceil( v ) )."""
    return int(math.ceil(v))


def qwt_floor(v):
    """qwtFloor: int( floor( v ) )."""
    return int(math.floor(v))


def c_trunc_div(a, b):
    """C++ integer division of doubles: static_cast truncation toward zero."""
    return int(a / b)


def qwt_fuzzy_compare(v1, v2, interval_size):
    """qwtFuzzyCompare ( src/core/qwt_math.h ): eps relative to interval_size."""
    eps = abs(CS_EPS * interval_size)
    if v2 - v1 > eps:
        return -1
    if v1 - v2 > eps:
        return 1
    return 0


def qt_fuzzy_compare(p1, p2):
    """Qt qFuzzyCompare( double, double ): relative 1e-12 comparison."""
    return abs(p1 - p2) <= 1.0e-12 * min(abs(p1), abs(p2))


def qwt_log(base, value):
    return math.log(value) / math.log(base)


def ceil_eps(value, interval_size):
    """QwtScaleArithmetic::ceilEps."""
    eps = CS_EPS * interval_size
    value = (value - eps) / interval_size
    return math.ceil(value) * interval_size


def floor_eps(value, interval_size):
    """QwtScaleArithmetic::floorEps."""
    eps = CS_EPS * interval_size
    value = (value + eps) / interval_size
    return math.floor(value) * interval_size


def divide_eps(interval_size, num_steps):
    """QwtScaleArithmetic::divideEps."""
    if num_steps == 0.0 or interval_size == 0.0:
        return 0.0
    return (interval_size - (CS_EPS * interval_size)) / num_steps


def divide_interval(interval_size, num_steps, base=10):
    """QwtScaleArithmetic::divideInterval - the core 'nice number' ladder.

    Note the integer halving loop: for base 10 the chain is 10 -> 5 -> 2 -> 1
    ( uint division 5/2 == 2 ), which is exactly what produces the 1-2-5 set.
    """
    if num_steps <= 0:
        return 0.0
    v = divide_eps(interval_size, num_steps)
    if v == 0.0:
        return 0.0
    lx = qwt_log(base, abs(v))
    p = math.floor(lx)
    fraction = float(base) ** (lx - p)
    n = int(base)
    while n > 1 and fraction <= n // 2:
        n //= 2
    step = n * (float(base) ** p)
    if v < 0:
        step = -step
    return step


def engine_contains(lo, hi, value):
    """QwtScaleEngine::contains for a normalized interval [lo, hi]."""
    if lo > hi:
        return False
    width = hi - lo
    if qwt_fuzzy_compare(value, lo, width) < 0:
        return False
    if qwt_fuzzy_compare(value, hi, width) > 0:
        return False
    return True


def engine_strip(ticks, lo, hi):
    """QwtScaleEngine::strip."""
    if lo > hi or len(ticks) == 0:
        return []
    if engine_contains(lo, hi, ticks[0]) and engine_contains(lo, hi, ticks[-1]):
        return list(ticks)
    return [t for t in ticks if engine_contains(lo, hi, t)]


def build_interval(value):
    """QwtScaleEngine::buildInterval ( overflow guards omitted )."""
    delta = 0.5 if value == 0.0 else abs(0.5 * value)
    return (value - delta, value + delta)


class ScaleDiv:
    """Minimal QwtScaleDiv stand-in."""

    def __init__(self, lower=0.0, upper=0.0, major=None, medium=None, minor=None):
        self.lower = lower
        self.upper = upper
        self.major = major if major is not None else []
        self.medium = medium if medium is not None else []
        self.minor = minor if minor is not None else []

    def is_empty(self):
        return (not self.major) and (not self.medium) and (not self.minor)

    def invert(self):
        self.lower, self.upper = self.upper, self.lower
        self.major.reverse()
        self.medium.reverse()
        self.minor.reverse()

    def bounded(self, lo, hi):
        """QwtScaleDiv::bounded - hard ( non fuzzy ) inclusive clipping."""
        mn, mx = min(lo, hi), max(lo, hi)
        keep = lambda lst: [t for t in lst if mn <= t <= mx]
        return ScaleDiv(lo, hi, keep(self.major), keep(self.medium), keep(self.minor))


# ------------------------------------------------------------- linear engine

def linear_align(x1, x2, step):
    """QwtLinearScaleEngine::align."""
    if -DBL_MAX + step <= x1:
        x = floor_eps(x1, step)
        if abs(x) <= ALIGN_EPS or not qt_fuzzy_compare(x1, x):
            x1 = x
    if DBL_MAX - step >= x2:
        x = ceil_eps(x2, step)
        if abs(x) <= ALIGN_EPS or not qt_fuzzy_compare(x2, x):
            x2 = x
    return (x1, x2)


def linear_build_major_ticks(x1, x2, step):
    """QwtLinearScaleEngine::buildMajorTicks ( cap 10000 )."""
    num = qround((x2 - x1) / step) + 1
    if num > 10000:
        num = 10000
    ticks = [x1]
    for i in range(1, num - 1):
        ticks.append(x1 + i * step)
    ticks.append(x2)
    return ticks


def qwt_step_size_minor(interval_size, max_steps, base=10):
    """The static qwtStepSize() used by QwtLinearScaleEngine::buildMinorTicks.

    Returns ( min_step, fell_back ): fell_back is True when the 0.5*interval
    fallback fired ( 'the minor steps don't fit into the interval' ).
    """
    min_step = divide_interval(interval_size, max_steps, base)
    if min_step != 0.0:
        num_ticks = qwt_ceil(abs(interval_size / min_step)) - 1
        if qwt_fuzzy_compare((num_ticks + 1) * abs(min_step), abs(interval_size), interval_size) > 0:
            return 0.5 * interval_size, True
    return min_step, False


def linear_build_minor_ticks(majors, max_minor, step, base=10):
    """QwtLinearScaleEngine::buildMinorTicks."""
    stats = {"fell_back": False, "min_step": 0.0}
    min_step, fell_back = qwt_step_size_minor(step, max_minor, base)
    stats["fell_back"] = fell_back
    stats["min_step"] = min_step
    if min_step == 0.0:
        return [], [], stats
    num_ticks = qwt_ceil(abs(step / min_step)) - 1
    med_index = num_ticks // 2 if (num_ticks % 2) else -1
    minors, mediums = [], []
    for m0 in majors:
        val = m0
        for k in range(num_ticks):
            val += min_step               # same accumulation order as C++
            av = 0.0 if qwt_fuzzy_compare(val, 0.0, step) == 0 else val
            if k == med_index:
                mediums.append(av)
            else:
                minors.append(av)
    return minors, mediums, stats


def linear_divide_scale(x1, x2, max_major, max_minor, step=0.0, base=10, stats=None):
    """QwtLinearScaleEngine::divideScale ( default attributes, no margins )."""
    orig_x1, orig_x2 = x1, x2
    if x1 > x2:
        x1, x2 = x2, x1
    width = x2 - x1
    if width <= 0:
        return ScaleDiv()
    step = abs(step)
    if step == 0.0:
        if max_major < 1:
            max_major = 1
        step = divide_interval(width, max_major, base)
    sd = ScaleDiv()
    if step != 0.0:
        a1, a2 = linear_align(x1, x2, step)
        majors = linear_build_major_ticks(a1, a2, step)
        minors, mediums = [], []
        if max_minor > 0:
            minors, mediums, mstats = linear_build_minor_ticks(majors, max_minor, step, base)
            if stats is not None:
                stats.update(mstats)
        majors = engine_strip(majors, x1, x2)
        mediums = engine_strip(mediums, x1, x2)
        minors = engine_strip(minors, x1, x2)
        snap = lambda lst: [0.0 if qwt_fuzzy_compare(t, 0.0, step) == 0 else t for t in lst]
        sd = ScaleDiv(x1, x2, snap(majors), snap(mediums), snap(minors))
    else:
        sd = ScaleDiv(x1, x2)
    if stats is not None:
        stats["step"] = step
    if orig_x1 > orig_x2:
        sd.invert()
    return sd


def linear_auto_scale(x1, x2, max_num_steps, base=10):
    """QwtLinearScaleEngine::autoScale ( default attributes, no margins )."""
    if x1 > x2:
        x1, x2 = x2, x1
    if x2 - x1 == 0.0:
        x1, x2 = build_interval(x1)
    step = divide_interval(x2 - x1, max(max_num_steps, 1), base)
    x1, x2 = linear_align(x1, x2, step)
    return x1, x2, step


# ---------------------------------------------------------------- log engine

def log_align(x1, x2, step, base=10.0):
    """QwtLogScaleEngine::align ( in log space )."""
    lx1 = qwt_log(base, x1)
    lx2 = qwt_log(base, x2)
    a = floor_eps(lx1, step)
    if qwt_fuzzy_compare(lx1, a, step) == 0:
        a = lx1
    b = ceil_eps(lx2, step)
    if qwt_fuzzy_compare(lx2, b, step) == 0:
        b = lx2
    return (base ** a, base ** b)


def log_build_major_ticks(x1, x2, step, base=10.0):
    """QwtLogScaleEngine::buildMajorTicks - equidistant in log space."""
    width = qwt_log(base, x2) - qwt_log(base, x1)
    num = qround(width / step) + 1
    if num > 10000:
        num = 10000
    lxmin = math.log(x1)
    lxmax = math.log(x2)
    lstep = (lxmax - lxmin) / float(num - 1) if num > 1 else 0.0
    ticks = [x1]
    for i in range(1, num - 1):
        ticks.append(math.exp(lxmin + float(i) * lstep))
    ticks.append(x2)
    return ticks


def log_build_minor_ticks(majors, max_minor, step, base=10.0):
    """QwtLogScaleEngine::buildMinorTicks - the two regimes."""
    minors, mediums = [], []
    if step < 1.1:
        # major step width is ( close to ) one decade
        min_step = divide_interval(step, max_minor + 1, base)
        if min_step == 0.0:
            return minors, mediums
        num_steps = qround(step / min_step)
        medium_index = num_steps // 2 if (num_steps > 2 and num_steps % 2 == 0) else -1
        for i in range(len(majors) - 1):
            v = majors[i]
            s = float(base) / num_steps
            if s >= 1.0:
                if not qt_fuzzy_compare(s, 1.0):
                    minors.append(v * s)
                for j in range(2, num_steps):
                    minors.append(v * j * s)
            else:
                for j in range(1, num_steps):
                    tick = v + j * v * (base - 1.0) / num_steps
                    if j == medium_index:
                        mediums.append(tick)
                    else:
                        minors.append(tick)
    else:
        min_step = divide_interval(step, max_minor, base)
        if min_step == 0.0:
            return minors, mediums
        if min_step < 1.0:
            min_step = 1.0
        num_ticks = qround(step / min_step) - 1
        if qwt_fuzzy_compare((num_ticks + 1) * min_step, step, step) > 0:
            num_ticks = 0
        if num_ticks < 1:
            return minors, mediums
        medium_index = num_ticks // 2 if (num_ticks > 2 and num_ticks % 2) else -1
        min_factor = max(float(base) ** min_step, float(base))
        for m in majors:
            tick = m
            for j in range(num_ticks):
                tick *= min_factor
                if j == medium_index:
                    mediums.append(tick)
                else:
                    minors.append(tick)
    return minors, mediums


def log_divide_scale(x1, x2, max_major, max_minor, step=0.0, base=10.0, stats=None):
    """QwtLogScaleEngine::divideScale ( default attributes )."""
    orig_x1, orig_x2 = x1, x2
    if x1 > x2:
        x1, x2 = x2, x1
    x1 = min(max(x1, LOG_MIN), LOG_MAX)
    x2 = min(max(x2, LOG_MIN), LOG_MAX)
    if x2 - x1 <= 0:
        return ScaleDiv()
    if x2 / x1 < base:
        # less than one decade -> the log engine delegates to the linear one
        if stats is not None:
            stats["linear_fallback"] = True
        return linear_divide_scale(orig_x1, orig_x2, max_major, max_minor, 0.0, base, stats)
    if stats is not None:
        stats["linear_fallback"] = False
    step = abs(step)
    if step == 0.0:
        if max_major < 1:
            max_major = 1
        step = divide_interval(qwt_log(base, x2) - qwt_log(base, x1), max_major, base)
        if step < 1.0:
            step = 1.0
    a1, a2 = log_align(x1, x2, step, base)
    majors = log_build_major_ticks(a1, a2, step, base)
    minors, mediums = [], []
    if max_minor > 0:
        minors, mediums = log_build_minor_ticks(majors, max_minor, step, base)
    majors = engine_strip(majors, x1, x2)
    mediums = engine_strip(mediums, x1, x2)
    minors = engine_strip(minors, x1, x2)
    if stats is not None:
        stats["step"] = step
    sd = ScaleDiv(x1, x2, majors, mediums, minors)
    if orig_x1 > orig_x2:
        sd.invert()
    return sd


# ---------------------------------------------------------------- date engine

# QwtDate::IntervalType
MILLISECOND, SECOND, MINUTE, HOUR, DAY, WEEK, MONTH, YEAR = range(8)
TYPE_NAMES = ["Millisecond", "Second", "Minute", "Hour", "Day", "Week", "Month", "Year"]

MSECS_FOR_TYPE = [
    1.0,
    1000.0,
    60.0 * 1000.0,
    3600.0 * 1000.0,
    24.0 * 3600.0 * 1000.0,
    7.0 * 24.0 * 3600.0 * 1000.0,
    30.0 * 24.0 * 3600.0 * 1000.0,     # a month is approximated as 30 days
    365.0 * 24.0 * 3600.0 * 1000.0,    # a year as 365 days
]

EPOCH = datetime(1970, 1, 1)
MS_PER_DAY = 86400000
# Qt::Monday.  The C++ code reads QLocale().firstDayOfWeek() at runtime:
# zh-CN -> Sunday ( CLDR ), en-GB -> Monday.  set_first_day_of_week() switches.
FIRST_DAY_OF_WEEK = 1


def set_first_day_of_week(d):
    """1 = Monday ( ISO / en-GB ), 7 = Sunday ( zh-CN CLDR )."""
    global FIRST_DAY_OF_WEEK
    FIRST_DAY_OF_WEEK = d


def dt_total_ms(dt):
    td = dt - EPOCH
    return td.days * MS_PER_DAY + td.seconds * 1000 + td.microseconds // 1000


def to_double(dt):
    """QwtDate::toDouble for naive UTC datetimes ( exact integer ms )."""
    return float(dt_total_ms(dt))


def to_datetime(value):
    """QwtDate::toDateTime ( UTC ): days = floor( v / 86400000 ), rest ms."""
    days = math.floor(value / float(MS_PER_DAY))
    msecs = int(value - days * float(MS_PER_DAY))
    return EPOCH + timedelta(days=int(days), milliseconds=msecs)


def ms_of_day(dt):
    return dt.hour * 3600000 + dt.minute * 60000 + dt.second * 1000 + dt.microsecond // 1000


def msecs_to(a, b):
    return dt_total_ms(b) - dt_total_ms(a)


def secs_to(a, b):
    """QDateTime::secsTo - integer division, truncation toward zero."""
    return c_trunc_div(msecs_to(a, b), 1000)


def days_to(a, b):
    """QDateTime::daysTo - date difference plus truncated time correction."""
    return (b.date() - a.date()).days + c_trunc_div(ms_of_day(a) - ms_of_day(b), MS_PER_DAY)


def add_months(dt, months):
    """QDateTime::addMonths - clamps the day ( Jan31 + 1m -> Feb28/29 )."""
    total = dt.year * 12 + (dt.month - 1) + int(months)
    y, m0 = divmod(total, 12)
    m = m0 + 1
    dim = [31, 29 if (y % 4 == 0 and (y % 100 != 0 or y % 400 == 0)) else 28,
           31, 30, 31, 30, 31, 31, 30, 31, 30, 31][m - 1]
    return dt.replace(year=y, month=m, day=min(dt.day, dim))


def add_years(dt, years):
    """QDateTime::addYears - clamps Feb 29."""
    y = dt.year + int(years)
    dim = 29 if (y % 4 == 0 and (y % 100 != 0 or y % 400 == 0)) else 28
    d = min(dt.day, dim) if dt.month == 2 else dt.day
    return dt.replace(year=y, month=dt.month, day=d)


def date_floor(dt, t):
    """QwtDate::floor ( UTC, no DST path )."""
    if t == MILLISECOND:
        return dt
    if t in (SECOND, MINUTE, HOUR):
        r = dt.replace(microsecond=0)
        if t == SECOND:
            return r
        r = r.replace(second=0)
        if t == MINUTE:
            return r
        return r.replace(minute=0)
    if t == DAY:
        return dt.replace(hour=0, minute=0, second=0, microsecond=0)
    if t == WEEK:
        r = dt.replace(hour=0, minute=0, second=0, microsecond=0)
        days = r.isoweekday() - FIRST_DAY_OF_WEEK
        if days < 0:
            days += 7
        return r - timedelta(days=days)
    if t == MONTH:
        return dt.replace(day=1, hour=0, minute=0, second=0, microsecond=0)
    if t == YEAR:
        return dt.replace(month=1, day=1, hour=0, minute=0, second=0, microsecond=0)
    raise ValueError(t)


def date_ceil(dt, t):
    """QwtDate::ceil ( UTC, no DST path )."""
    if t == MILLISECOND:
        return dt
    if t in (SECOND, MINUTE, HOUR):
        r = date_floor(dt, t)
        if r < dt:
            r += timedelta(seconds={SECOND: 1, MINUTE: 60, HOUR: 3600}[t])
        return r
    if t == DAY:
        r = dt.replace(hour=0, minute=0, second=0, microsecond=0)
        if r < dt:
            r += timedelta(days=1)
        return r
    if t == WEEK:
        r = dt.replace(hour=0, minute=0, second=0, microsecond=0)
        if r < dt:
            r += timedelta(days=1)
        days = FIRST_DAY_OF_WEEK - r.isoweekday()
        if days < 0:
            days += 7
        return r + timedelta(days=days)
    if t == MONTH:
        r = dt.replace(day=1, hour=0, minute=0, second=0, microsecond=0)
        if r < dt:
            r = add_months(r, 1)
        return r
    if t == YEAR:
        year = dt.year
        if dt.month > 1 or dt.day > 1 or ms_of_day(dt) != 0:
            year += 1
        if year == 0:
            year += 1     # there is no year 0
        return dt.replace(year=year, month=1, day=1, hour=0, minute=0, second=0, microsecond=0)
    raise ValueError(t)


def interval_width(min_dt, max_dt, t):
    """qwtIntervalWidth ( static in qwt_date_scale_engine.cpp )."""
    if t == MILLISECOND:
        return float(msecs_to(min_dt, max_dt))
    if t == SECOND:
        return float(secs_to(min_dt, max_dt))
    if t == MINUTE:
        return math.floor(secs_to(min_dt, max_dt) / 60.0)
    if t == HOUR:
        return math.floor(secs_to(min_dt, max_dt) / 3600.0)
    if t == DAY:
        return float(days_to(min_dt, max_dt))
    if t == WEEK:
        return math.floor(days_to(min_dt, max_dt) / 7.0)
    if t == MONTH:
        years = float(max_dt.year) - min_dt.year
        months = max_dt.month - min_dt.month
        if max_dt.day < min_dt.day:
            months -= 1
        return years * 12 + months
    if t == YEAR:
        years = float(max_dt.year) - min_dt.year
        if max_dt.month < min_dt.month:
            years -= 1.0
        return years
    return 0.0


def rounded_interval_width(min_dt, max_dt, t):
    """qwtRoundedIntervalWidth."""
    return interval_width(date_floor(min_dt, t), date_ceil(max_dt, t), t)


def interval_type(min_dt, max_dt, max_steps, max_weeks=4):
    """QwtDateScaleEngine::intervalType - the classification cascade."""
    jd_diff = (max_dt.date() - min_dt.date()).days
    if jd_diff / 365.0 > max_steps:
        return YEAR
    months = rounded_interval_width(min_dt, max_dt, MONTH)
    if months > max_steps * 6:
        return YEAR
    days = rounded_interval_width(min_dt, max_dt, DAY)
    weeks = rounded_interval_width(min_dt, max_dt, WEEK)
    if weeks > max_weeks:
        if days > 4 * max_steps * 7:
            return MONTH
    if days > max_steps * 7:
        return WEEK
    hours = rounded_interval_width(min_dt, max_dt, HOUR)
    if hours > max_steps * 24:
        return DAY
    seconds = rounded_interval_width(min_dt, max_dt, SECOND)
    if seconds >= max_steps * 3600:
        return HOUR
    if seconds >= max_steps * 60:
        return MINUTE
    if seconds >= max_steps:
        return SECOND
    return MILLISECOND


def divide_interval_limits(interval_size, num_steps, limits):
    """qwtDivideInterval with an integer limits ladder ( date engine )."""
    v = qwt_ceil(interval_size / float(num_steps))
    for i in range(len(limits) - 1):
        if v <= limits[i]:
            return limits[i]
    return limits[-1]


def date_divide_scale_step(interval_size, num_steps, t, use_prefix=True):
    """qwtDivideScale - step size selection per interval type.

    use_prefix=False disables the early 'return 2.0' rule; experiment B4
    verifies that the rule never changes the outcome.
    """
    if use_prefix and t != DAY:
        if interval_size > num_steps and interval_size <= 2 * num_steps:
            return 2.0
    if t in (SECOND, MINUTE):
        return float(divide_interval_limits(interval_size, num_steps, [1, 2, 5, 10, 15, 20, 30, 60]))
    if t == HOUR:
        return float(divide_interval_limits(interval_size, num_steps, [1, 2, 3, 4, 6, 12, 24]))
    if t == DAY:
        v = interval_size / float(num_steps)
        if v <= 5.0:
            return math.ceil(v)
        return math.ceil(v / 7.0) * 7.0
    if t == WEEK:
        return float(divide_interval_limits(interval_size, num_steps, [1, 2, 4, 8, 12, 26, 52]))
    if t == MONTH:
        return float(divide_interval_limits(interval_size, num_steps, [1, 2, 3, 4, 6, 12]))
    # Year, Millisecond, default: the decimal ladder of the linear engine
    return divide_interval(interval_size, num_steps, 10)


def step_count(interval_size, max_steps, limits):
    """qwtStepCount - exact divisors of interval_size within the budget."""
    for L in limits:
        num = interval_size // L
        if num > 1 and num <= max_steps and num * L == interval_size:
            return num
    return 0


def qwt_step_size_int(interval_size, max_steps, base=10):
    """The integer qwtStepSize() variant of the date engine ( Second type )."""
    if max_steps <= 0:
        return 0
    if max_steps > 2:
        for num_steps in range(max_steps, 1, -1):
            step_size = float(interval_size) / num_steps
            p = math.floor(math.log(step_size) / math.log(float(base)))
            fraction = float(base) ** p
            n = base
            while n >= 1:
                if qt_fuzzy_compare(step_size, n * fraction):
                    return qround(step_size)
                if n == 3 and (base % 2) == 0:
                    if qt_fuzzy_compare(step_size, 2 * fraction):
                        return qround(step_size)
                n //= 2
    return 0


def divide_major_step(step_size, max_min_steps, t):
    """qwtDivideMajorStep - minor step per interval type."""
    min_step_size = 0.0
    if t == SECOND:
        min_step_size = qwt_step_size_int(step_size, max_min_steps, 10)
        if min_step_size == 0.0:
            min_step_size = 0.5 * step_size
    elif t == MINUTE:
        limits = [1, 2, 5, 10, 15, 20, 30, 60]
        if step_size > max_min_steps:
            num = step_count(step_size, max_min_steps, limits)
        else:
            num = step_count(step_size * 60, max_min_steps, limits)
        if num > 0:
            min_step_size = step_size / num
    elif t == HOUR:
        num = 0
        if step_size > max_min_steps:
            num = step_count(step_size, max_min_steps, [1, 2, 3, 4, 6, 12, 24, 48, 72])
        else:
            num = step_count(step_size * 60, max_min_steps, [1, 2, 5, 10, 15, 20, 30, 60])
        if num > 0:
            min_step_size = step_size / num
    elif t == DAY:
        num = 0
        if step_size > max_min_steps:
            num = step_count(step_size, max_min_steps, [1, 2, 3, 7, 14, 28])
        else:
            num = step_count(step_size * 24, max_min_steps, [1, 2, 3, 4, 6, 12, 24, 48, 72])
        if num > 0:
            min_step_size = step_size / num
    elif t == WEEK:
        days_in_step = step_size * 7
        if max_min_steps >= days_in_step:
            min_step_size = 1.0 / 7.0          # one tick per day
        else:
            weeks = step_size
            if weeks <= max_min_steps:
                min_step_size = 1.0
            else:
                min_step_size = divide_interval(weeks, max_min_steps, 10)
    elif t == MONTH:
        if step_size < max_min_steps:
            max_min_steps = int(step_size)
        num = step_count(step_size, max_min_steps, [1, 2, 3, 4, 6, 12])
        if num > 0:
            min_step_size = step_size / num
    elif t == YEAR:
        if step_size >= max_min_steps:
            min_step_size = divide_interval(step_size, max_min_steps, 10)
        else:
            num = step_count(12 * step_size, max_min_steps, [1, 2, 3, 4, 6, 12])
            if num > 0:
                min_step_size = step_size / num
    if t != MONTH and min_step_size == 0.0:
        min_step_size = 0.5 * step_size
    return min_step_size


def align_value(value, step_size, up):
    """qwtAlignValue - note the truncating static_cast<int>."""
    d = value / step_size
    d = math.ceil(d) if up else math.floor(d)
    return int(d * step_size)


def date_of_week0(year, first_thursday=True):
    """QwtDate::dateOfWeek0 ( Monday based locale )."""
    dt0 = date(year, 1, 1)
    days = dt0.isoweekday() - FIRST_DAY_OF_WEEK
    if days < 0:
        days += 7
    dt0 = dt0 - timedelta(days=days)
    if first_thursday:
        d = 4 - FIRST_DAY_OF_WEEK       # Qt::Thursday - firstDayOfWeek
        if d < 0:
            d += 7
        if (dt0 + timedelta(days=d)).year < year:
            dt0 = dt0 + timedelta(days=7)
    return dt0


def align_date(dt, step_size, t, up):
    """QwtDateScaleEngine::alignDate ( UTC )."""
    if t == MILLISECOND:
        ms = align_value(dt.microsecond // 1000, step_size, up)
        r = date_floor(dt, SECOND)
        return r + timedelta(milliseconds=ms)
    if t == SECOND:
        second = dt.second
        if up and dt.microsecond > 0:
            second += 1
        s = align_value(second, step_size, up)
        return date_floor(dt, MINUTE) + timedelta(seconds=s)
    if t == MINUTE:
        minute = dt.minute
        if up and (dt.microsecond > 0 or dt.second > 0):
            minute += 1
        m = align_value(minute, step_size, up)
        return date_floor(dt, HOUR) + timedelta(seconds=m * 60)
    if t == HOUR:
        hour = dt.hour
        if up and (dt.microsecond > 0 or dt.second > 0 or dt.minute > 0):
            hour += 1
        h = align_value(hour, step_size, up)
        return date_floor(dt, DAY) + timedelta(seconds=h * 3600)
    if t == DAY:
        # aligned to the beginning of the year to avoid jumping ticks when panning
        day = dt.timetuple().tm_yday
        if up and ms_of_day(dt) > 0:
            day += 1
        d = align_value(day, step_size, up)
        return date_floor(dt, YEAR) + timedelta(days=d - 1)
    if t == WEEK:
        w0 = date_of_week0(dt.year, True)
        num_weeks = c_trunc_div((dt.date() - w0).days, 7)
        if up:
            if ms_of_day(dt) > 0 or (dt.date() - w0).days % 7:
                num_weeks += 1
        d = align_value(num_weeks, step_size, up) * 7
        r = date_floor(dt, DAY)
        # C++ does dt.setDate( week0 date ) - the FULL date, week0 may live in
        # the previous year ( e.g. dateOfWeek0( 2015 ) = 2014-12-29 )
        return r.replace(year=w0.year, month=w0.month, day=w0.day) + timedelta(days=d)
    if t == MONTH:
        month = dt.month
        if up and (dt.day > 1 or ms_of_day(dt) > 0):
            month += 1
        m = align_value(month - 1, step_size, up)
        return add_months(date_floor(dt, YEAR), m)
    if t == YEAR:
        year = dt.year
        if up and (dt.timetuple().tm_yday > 1 or ms_of_day(dt) > 0):
            year += 1
        y = align_value(year, step_size, up)
        r = date_floor(dt, DAY)
        if y == 0:
            # no year 0 in the Julian calendar ( BC dates are out of scope here )
            raise ValueError("year 0 not supported by the python port")
        return r.replace(year=int(y), month=1, day=1)
    raise ValueError(t)


def divide_to_seconds(min_dt, max_dt, step_size, max_min_steps, t):
    """qwtDivideToSeconds - UTC path ( no daylight saving correction )."""
    min_step_size = 0.0
    if max_min_steps > 1:
        min_step_size = divide_major_step(step_size, max_min_steps, t)

    s = MSECS_FOR_TYPE[t] / 1000.0
    seconds_major = int(step_size * s)          # static_cast<int> truncation
    seconds_minor = min_step_size * s

    majors, mediums, minors = [], [], []
    dt = min_dt
    while dt <= max_dt:
        value = to_double(dt)
        if not majors or majors[-1] != value:
            majors.append(value)
        if seconds_minor > 0.0:
            num_minor = qwt_floor(seconds_major / seconds_minor)
            for i in range(1, num_minor):
                mt = dt + timedelta(milliseconds=qround(i * seconds_minor * 1000.0))
                mv = to_double(mt)
                if not minors or minors[-1] != mv:
                    is_medium = (num_minor % 2 == 0) and (i != 1) and (i == num_minor // 2)
                    if is_medium:
                        mediums.append(mv)
                    else:
                        minors.append(mv)
        dt = dt + timedelta(seconds=seconds_major)
    return ScaleDiv(to_double(min_dt), to_double(max_dt), majors, mediums, minors)


def divide_to_months(min_dt, max_dt, step_size, max_min_steps):
    """qwtDivideToMonths - months are not equidistant in ms."""
    min_step_days = 0
    min_step_size = 0.0
    if max_min_steps > 1:
        if step_size == 1:
            if max_min_steps >= 30:
                min_step_days = 1
            elif max_min_steps >= 6:
                min_step_days = 5
            elif max_min_steps >= 3:
                min_step_days = 10
            else:
                min_step_days = 15
        else:
            min_step_size = divide_major_step(step_size, max_min_steps, MONTH)

    majors, mediums, minors = [], [], []
    dt = min_dt
    while dt <= max_dt:
        majors.append(to_double(dt))
        if min_step_days > 0:
            days = min_step_days
            while days < 30:
                tick = to_double(dt + timedelta(days=days))
                if days == 15 and min_step_days != 15:
                    mediums.append(tick)
                else:
                    minors.append(tick)
                days += min_step_days
        elif min_step_size > 0.0:
            num_minor = qround(step_size / min_step_size)
            for i in range(1, num_minor):
                mv = to_double(add_months(dt, i * min_step_size))
                if num_minor % 2 == 0 and i == num_minor // 2:
                    mediums.append(mv)
                else:
                    minors.append(mv)
        dt = add_months(dt, step_size)
    return ScaleDiv(to_double(min_dt), to_double(max_dt), majors, mediums, minors)


def divide_to_years(min_dt, max_dt, step_size, max_min_steps):
    """qwtDivideToYears ( AD dates only - the BC/year-0 path is not ported )."""
    majors, mediums, minors = [], [], []
    min_step_size = 0.0
    if max_min_steps > 1:
        min_step_size = divide_major_step(step_size, max_min_steps, YEAR)
    num_minor = qwt_floor(step_size / min_step_size) if min_step_size > 0.0 else 0

    dt = min_dt
    while dt <= max_dt:
        majors.append(to_double(dt))
        for i in range(1, num_minor):
            years = qround(i * min_step_size)
            if years >= 2147483647 // 12:
                tick_dt = add_years(dt, years)
            else:
                tick_dt = add_months(dt, qround(years * 12))
            is_medium = (num_minor > 2) and (num_minor % 2 == 0) and (i == num_minor // 2)
            mv = to_double(tick_dt)
            if is_medium:
                mediums.append(mv)
            else:
                minors.append(mv)
        if dt.year + int(step_size) > 9999:
            break
        dt = add_years(dt, step_size)
    return ScaleDiv(to_double(min_dt), to_double(max_dt), majors, mediums, minors)


def date_build_scale_div(min_dt, max_dt, max_major, max_minor, t):
    """QwtDateScaleEngine::buildScaleDiv."""
    step = date_divide_scale_step(interval_width(min_dt, max_dt, t), max_major, t)
    dt0 = align_date(min_dt, step, t, False)
    if t <= WEEK:
        return divide_to_seconds(dt0, max_dt, step, max_minor, t), step, dt0
    if t == MONTH:
        return divide_to_months(dt0, max_dt, step, max_minor), step, dt0
    return divide_to_years(dt0, max_dt, step, max_minor), step, dt0


def date_auto_scale(x1, x2, max_num_steps, max_weeks=4):
    """QwtDateScaleEngine::autoScale ( UTC, default attributes )."""
    step_out = 0.0
    if x1 > x2:
        x1, x2 = x2, x1
    if x2 - x1 == 0.0:
        x1, x2 = build_interval(x1)
    frm = to_datetime(x1)
    to = to_datetime(x2)
    if max_num_steps < 1:
        max_num_steps = 1
    t = interval_type(frm, to, max_num_steps, max_weeks)
    width = interval_width(frm, to, t)
    step_width = date_divide_scale_step(width, max_num_steps, t)
    if step_width != 0.0:
        d1 = align_date(frm, step_width, t, False)
        d2 = align_date(to, step_width, t, True)
        x1 = to_double(d1)
        x2 = to_double(d2)
    step_out = step_width * MSECS_FOR_TYPE[t]
    return x1, x2, step_out, t


def date_divide_scale(x1, x2, max_major, max_minor, step_hint=0.0, max_weeks=4, stats=None):
    """QwtDateScaleEngine::divideScale ( UTC )."""
    if max_major < 1:
        max_major = 1
    lo, hi = min(x1, x2), max(x1, x2)
    frm = to_datetime(lo)
    to = to_datetime(hi)
    if frm == to:
        return ScaleDiv()
    step = abs(step_hint)
    if step > 0.0:
        # the step size is only a hint - maxMajorSteps gets re-estimated
        max_major = qwt_ceil((hi - lo) / step)
    t = interval_type(frm, to, max_major, max_weeks)
    if stats is not None:
        stats["type"] = t
        stats["max_major_used"] = max_major
    if t == MILLISECOND:
        sd = linear_divide_scale(lo, hi, max_major, max_minor, step)
    else:
        min_dt = date_floor(frm, t)
        max_dt = date_ceil(to, t)
        sd, step_units, dt0 = date_build_scale_div(min_dt, max_dt, max_major, max_minor, t)
        sd = sd.bounded(lo, hi)
        if stats is not None:
            stats["step_units"] = step_units
            stats["dt0"] = dt0
    if x1 > x2:
        sd.invert()
    return sd


# =============================================================== experiments

def mantissa_of(step):
    """Mantissa m in [1,10) with step = m * 10^k ( None when not finite )."""
    if step <= 0 or not np.isfinite(step):
        return None
    k = math.floor(math.log10(step))
    return step / (10.0 ** k)


def classify_mantissa(m):
    for nice in (1.0, 2.0, 5.0):
        if abs(m - nice) <= 1e-9 * nice:
            return nice
    return None


def random_intervals(n, rng):
    """Mixed magnitude intervals, including many that straddle zero."""
    x1 = rng.uniform(-1000.0, 1000.0, n)
    width = 10.0 ** rng.uniform(-3.0, 6.0, n)
    return x1, x1 + width


def fmt_dt(ms):
    return to_datetime(ms).strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]


def experiment_a(rng):
    print("=" * 78)
    print("[A] LINEAR ENGINE - random scan ( N = 10000, maxMajor = %d, maxMinor = %d )"
          % (PLOT_MAX_MAJOR, PLOT_MAX_MINOR))
    print("=" * 78)
    n = 10000
    x1, x2 = random_intervals(n, rng)

    mant_counts = {1.0: 0, 2.0: 0, 5.0: 0, None: 0}
    ratios = np.empty(n)
    direct_intervals = np.empty(n, dtype=int)
    pipe_intervals = np.empty(n, dtype=int)
    direct_bounds_wider = np.zeros(n, dtype=bool)
    mant_totals = {1.0: [0, 0, 0], 2.0: [0, 0, 0], 5.0: [0, 0, 0]}   # gaps, minors, mediums
    fallback_counts = {mm: 0 for mm in (2, 3, 4, 5, 8)}

    for i in range(n):
        a1, a2 = x1[i], x2[i]
        w = a2 - a1
        stats = {}
        sd = linear_divide_scale(a1, a2, PLOT_MAX_MAJOR, PLOT_MAX_MINOR, 0.0, 10, stats)
        step = stats["step"]
        m = classify_mantissa(mantissa_of(step))
        mant_counts[m] += 1
        ratios[i] = step / divide_eps(w, PLOT_MAX_MAJOR)      # step / raw target
        direct_intervals[i] = max(len(sd.major) - 1, 0)
        if m in mant_totals:
            mant_totals[m][0] += direct_intervals[i]
            mant_totals[m][1] += len(sd.minor)
            mant_totals[m][2] += len(sd.medium)

        # full QwtPlot pipeline: autoScale then divideScale with the hint
        p1, p2, ph = linear_auto_scale(a1, a2, PLOT_MAX_MAJOR)
        sd2 = linear_divide_scale(p1, p2, PLOT_MAX_MAJOR, PLOT_MAX_MINOR, ph)
        pipe_intervals[i] = max(len(sd2.major) - 1, 0)
        direct_bounds_wider[i] = (p2 - p1) > w

        for mm in fallback_counts:
            _, fb = qwt_step_size_minor(step, mm, 10)
            if fb:
                fallback_counts[mm] += 1

    print("\n[A1] selected step mantissa distribution ( divideInterval, base 10 ):")
    for k in (1.0, 2.0, 5.0):
        print("     mantissa %g : %5d  ( %5.2f %% )" % (k, mant_counts[k], 100.0 * mant_counts[k] / n))
    print("     other       : %5d" % mant_counts[None])
    # theoretical frequencies for log-uniform targets ( Benford-like ):
    # step mantissa 2 <- target mantissa in [1,2), 5 <- [2,5), 1 <- [5,10)
    print("     theory ( log-uniform target ): 1 -> %.2f %%, 2 -> %.2f %%, 5 -> %.2f %%"
          % (100 * math.log10(2), 100 * math.log10(2), 100 * math.log10(2.5)))

    print("\n[A2] step / raw-target ratio ( raw target = width*(1-1e-6)/maxMajor ):")
    print("     min = %.6f  max = %.6f  mean = %.6f" % (ratios.min(), ratios.max(), ratios.mean()))
    hist, edges = np.histogram(ratios, bins=[1.0, 1.25, 1.5, 1.75, 2.0, 2.25, 2.5])
    for h, lo, hi in zip(hist, edges[:-1], edges[1:]):
        print("     ratio in [%.2f, %.2f) : %5d" % (lo, hi, h))

    print("\n[A3] major interval counts vs maxMajor = %d:" % PLOT_MAX_MAJOR)
    for name, arr in (("direct divideScale", direct_intervals), ("autoScale+divideScale", pipe_intervals)):
        cnt = np.bincount(arr, minlength=14)
        viol = int((arr > PLOT_MAX_MAJOR).sum())
        print("     %-24s mean = %.3f  max = %d  cases > maxMajor: %d ( %.2f %% )"
              % (name, arr.mean(), arr.max(), viol, 100.0 * viol / n))
        print("        histogram k:count -> " + "  ".join("%d:%d" % (k, cnt[k]) for k in range(len(cnt)) if cnt[k]))

    print("\n[A4] minor tick structure per major gap ( canonical, single gap, no strip ):")
    print("     step  maxMinor  minStep   #minor  #medium  positions in the gap")
    for s in (1.0, 2.0, 5.0):
        for mm in (2, 3, 4, 5, 8):
            minors, mediums, mst = linear_build_minor_ticks([0.0], mm, s)
            pos = sorted([(t, "m") for t in minors] + [(t, "M") for t in mediums])
            print("     %5g  %8d  %7g  %6d  %7d  %s"
                  % (s, mm, mst["min_step"], len(minors), len(mediums),
                     ", ".join("%g%s" % (t / s, tag) for t, tag in pos)))
    print("     scan aggregates ( totals over 10000 intervals, boundary gaps included ):")
    for m in (1.0, 2.0, 5.0):
        gaps, nmin, nmed = mant_totals[m]
        if gaps:
            print("     mantissa %g : gaps = %d, minors/gap = %.3f, mediums/gap = %.3f"
                  % (m, gaps, nmin / gaps, nmed / gaps))

    print("\n[A5] qwtStepSize fallback ( 0.5*step ) frequency vs maxMinorSteps:")
    for mm, c in sorted(fallback_counts.items()):
        print("     maxMinor = %d : fallback fired in %5d / %d cases ( %5.2f %% )" % (mm, c, n, 100.0 * c / n))

    print("\n[A6] systematic boundary intervals ( direct divideScale, maxMajor = 8, maxMinor = 5 ):")
    print("     interval          step        #majors  first..last major")
    rows = []
    for k in range(0, 9):
        for lo, hi, tag in ((1.0, 10.0 ** k, "[1, 10^%d]" % k), (0.0, 10.0 ** k, "[0, 10^%d]" % k)):
            if hi <= lo:
                continue
            st = {}
            sd = linear_divide_scale(lo, hi, PLOT_MAX_MAJOR, PLOT_MAX_MINOR, 0.0, 10, st)
            first = sd.major[0] if sd.major else float("nan")
            last = sd.major[-1] if sd.major else float("nan")
            print("     %-14s  %-10g  %d        %g .. %g" % (tag, st["step"], len(sd.major), first, last))
            rows.append((tag, st["step"], len(sd.major)))
    return ratios, direct_intervals, pipe_intervals, mant_counts


def experiment_b(rng):
    print()
    print("=" * 78)
    print("[B] NUMBER THEORY OF THE 1-2-5 LADDER")
    print("=" * 78)

    print("\n[B1] worst case rounding-up ratio r_max(S) = max( s_{i+1}/s_i, 10*s_1/s_k ):")
    pool = [1, 1.5, 2, 2.5, 3, 4, 5, 6, 7, 8, 9]

    def r_max(s):
        r = [s[i + 1] / s[i] for i in range(len(s) - 1)]
        r.append(10.0 * s[0] / s[-1])
        return max(r)

    import itertools
    best3, best4 = {}, {}
    for comb in itertools.combinations([1, 2, 3, 4, 5, 6, 7, 8, 9], 3):
        best3.setdefault(round(r_max(comb), 6), []).append(comb)
    for comb in itertools.combinations(pool, 4):
        best4.setdefault(round(r_max(comb), 6), []).append(comb)
    k3 = min(best3)
    print("     integer 3-subsets of {1..9}: minimal r_max = %.4f, achieved by %s"
          % (k3, sorted(best3[k3])))
    k3b = sorted(best3)[1]
    print("     next best r_max = %.4f, achieved by %s" % (k3b, sorted(best3[k3b])[:8]))
    k4 = min(best4)
    winners4 = sorted(best4[k4])
    print("     4-subsets of {1,1.5,2,2.5,...,9}: minimal r_max = %.4f, achieved by %d of %d subsets:"
          % (k4, len(winners4), len(list(itertools.combinations(pool, 4)))))
    for comb in winners4:
        print("         %s" % (comb,))
    for name, s in (("{1,2,5}", [1, 2, 5]), ("{1,2,4}", [1, 2, 4]),
                    ("{1,2,2.5,5}", [1, 2, 2.5, 5]), ("{2,4,9}", [2, 4, 9]),
                    ("geometric 3", [10 ** (i / 3.0) for i in range(3)]),
                    ("geometric 4", [10 ** (i / 4.0) for i in range(4)])):
        rr = [s[i + 1] / s[i] for i in range(len(s) - 1)] + [10.0 * s[0] / s[-1]]
        print("     %-12s ratios = [%s]  r_max = %.4f"
              % (name, ", ".join("%.3f" % x for x in rr), max(rr)))
    print("     uniform geometric bound: 10^(1/3) = %.6f, 10^(1/4) = %.6f"
          % (10 ** (1 / 3.0), 10 ** (1 / 4.0)))

    print("\n[B2] decade-duality closure ( s in S => mantissa(10/s) in S ):")
    closed = []
    for comb in itertools.combinations([1, 2, 3, 4, 5, 6, 7, 8, 9], 3):
        ok = True
        for s in comb:
            m = 10.0 / s
            while m >= 10.0:
                m /= 10.0
            while m < 1.0:
                m *= 10.0
            if not any(abs(m - c) < 1e-12 for c in comb):
                ok = False
                break
        if ok:
            closed.append(comb)
    print("     closed integer 3-subsets of {1..9}: %s   ( unique: %s )" % (closed, len(closed) == 1))

    print("\n[B3] halving chains n -> n//2 from the base ( the divideInterval loop ):")
    for base in (2, 8, 10, 12, 16):
        chain, nn = [], base
        while True:
            chain.append(nn)
            if nn <= 1:
                break
            nn //= 2
        print("     base %2d -> %s   ( mantissas per decade: %d )" % (base, chain, len(chain) - 1))

    print("\n[B4] date engine: is the 'return 2.0' prefix rule of qwtDivideScale redundant?")
    diffs = 0
    total = 0
    for t in (SECOND, MINUTE, HOUR, DAY, WEEK, MONTH):
        for width in range(1, 401):
            for ns in range(1, 13):
                total += 1
                a = date_divide_scale_step(width, ns, t, True)
                b = date_divide_scale_step(width, ns, t, False)
                if a != b:
                    diffs += 1
                    if diffs <= 5:
                        print("     DIFF type=%s width=%d numSteps=%d: prefix %g vs ladder %g"
                              % (TYPE_NAMES[t], width, ns, a, b))
    print("     combinations tested: %d, differences: %d" % (total, diffs))

    print("\n[B5] tick density sawtooth ( x1 = 0, maxMajor = 8 ):")
    w = np.logspace(0.0, 4.0, 4001)
    steps = np.array([divide_interval(float(x), PLOT_MAX_MAJOR, 10) for x in w])
    n_int = w / steps
    print("     width range [1, 10^4]: major intervals n = width/step  ->  "
          "min = %.4f  max = %.4f" % (n_int.min(), n_int.max()))
    print("     theoretical bounds: [maxMajor/2.5, maxMajor] = [%.2f, %.2f]" % (8 / 2.5, 8.0))
    return w, steps, n_int


def experiment_c():
    print()
    print("=" * 78)
    print("[C] LOG ENGINE - decade structure and minor tick regimes")
    print("=" * 78)

    cases = [
        ("[1, 1000]      3 decades", 1.0, 1000.0, 8, 5),
        ("[1, sqrt(10)]  0.5 decade", 1.0, math.sqrt(10.0), 8, 5),
        ("[3, 300]       non-power bounds", 3.0, 300.0, 8, 5),
        ("[1e-6, 1e6]    12 decades", 1e-6, 1e6, 8, 5),
        ("[1, 100] maxMajor=3", 1.0, 100.0, 3, 9),
    ]
    results = {}
    for tag, x1, x2, mm, mn in cases:
        st = {}
        sd = log_divide_scale(x1, x2, mm, mn, 0.0, 10.0, st)
        print("\n[C1] %s (maxMajor=%d, maxMinor=%d)" % (tag, mm, mn))
        if st.get("linear_fallback"):
            print("     ratio < base -> LINEAR fallback, step = %g" % st["step"])
        else:
            print("     major step = %g decade(s)" % st["step"])
        print("     majors (%d): %s" % (len(sd.major), ", ".join("%.6g" % t for t in sd.major)))
        print("     mediums (%d): %s" % (len(sd.medium), ", ".join("%.6g" % t for t in sd.medium[:12])))
        print("     minors (%d): %s%s" % (len(sd.minor), ", ".join("%.6g" % t for t in sd.minor[:12]),
                                           " ..." if len(sd.minor) > 12 else ""))
        results[tag] = sd

    print("\n[C2] one-decade minor subdivision vs maxMinorSteps ( majors 1..1000, step=1 decade ):")
    print("     maxMinor  minStep(dec)  numSteps  s=10/numSteps  regime        minors in [1,10)  mediums in [1,10)")
    for mn in (0, 1, 2, 3, 5, 7, 9, 12, 13, 14, 15, 16, 17, 18, 19, 20, 30):
        st = {}
        sd = log_divide_scale(1.0, 1000.0, 3, mn, 0.0, 10.0, st)
        ms = divide_interval(1.0, mn + 1, 10) if mn > 0 else 0.0
        nst = qround(1.0 / ms) if ms > 0 else 0
        s = 10.0 / nst if nst else 0.0
        regime = "geometric" if s >= 1.0 else "arithmetic"
        first_gap = [t for t in sd.minor if 1.0 < t < 10.0]
        first_med = [t for t in sd.medium if 1.0 < t < 10.0]
        print("     %8d  %11.4g  %8d  %13.4g  %-13s %-38s %s"
              % (mn, ms, nst, s, regime if mn > 0 else "-",
                 ", ".join("%.4g" % t for t in first_gap),
                 ", ".join("%.4g" % t for t in first_med)))

    print("\n[C3] two-decade majors, minor regime >= 1.1 decades ( [1e0, 1e6], maxMajor = 3 ):")
    st = {}
    sd = log_divide_scale(1.0, 1e6, 3, 5, 0.0, 10.0, st)
    print("     step = %g decades; majors: %s" % (st["step"], ", ".join("%.6g" % t for t in sd.major)))
    print("     mediums: %s" % ", ".join("%.6g" % t for t in sd.medium))
    print("     minors: %s" % ", ".join("%.6g" % t for t in sd.minor))

    print("\n[C4] degenerate clamp: [0, 1000] ( x1 <= 0 clamps to LogMin = 1e-150 ):")
    st = {}
    sd = log_divide_scale(0.0, 1000.0, 8, 5, 0.0, 10.0, st)
    print("     step = %g decades, majors (%d): %s ... %s"
          % (st["step"], len(sd.major), "%.3g" % sd.major[0], "%.3g" % sd.major[-1]))
    return results


def experiment_d():
    print()
    print("=" * 78)
    print("[D] DATE ENGINE - hierarchical fallback ( UTC, maxMajor = 8, maxMinor = 5 )")
    print("=" * 78)

    def scenario(tag, t0, t1, mm=PLOT_MAX_MAJOR, mn=PLOT_MAX_MINOR):
        print("\n[D1] %s : %s -> %s" % (tag, t0.isoformat(), t1.isoformat()))
        x1, x2 = to_double(t0), to_double(t1)
        a1, a2, hint, t_auto = date_auto_scale(x1, x2, mm)
        print("     autoScale: type = %-11s aligned bounds = %s -> %s   stepSize hint = %g ms ( %.6g days )"
              % (TYPE_NAMES[t_auto], fmt_dt(a1), fmt_dt(a2), hint, hint / 86400000.0))
        st = {}
        sd = date_divide_scale(a1, a2, mm, mn, hint, 4, st)
        step_txt = ("%g (type units)" % st["step_units"]) if "step_units" in st else "n/a (linear delegation)"
        print("     divideScale: type = %-11s step = %s, maxMajor used = %d"
              % (TYPE_NAMES[st["type"]], step_txt, st["max_major_used"]))
        print("     majors (%d): %s" % (len(sd.major), ", ".join(fmt_dt(v) for v in sd.major[:14])
                                        + (" ..." if len(sd.major) > 14 else "")))
        print("     mediums (%d): %s" % (len(sd.medium), ", ".join(fmt_dt(v) for v in sd.medium[:10])
                                         + (" ..." if len(sd.medium) > 10 else "")))
        print("     minors (%d): %s" % (len(sd.minor), ", ".join(fmt_dt(v) for v in sd.minor[:10])
                                        + (" ..." if len(sd.minor) > 10 else "")))
        return sd, st

    s1, _ = scenario("100 ms span", datetime(2024, 3, 10, 12, 0, 0), datetime(2024, 3, 10, 12, 0, 0, 100000))
    s2, _ = scenario("1 day span", datetime(2024, 3, 10), datetime(2024, 3, 11))
    s3, st3 = scenario("3 month span", datetime(2024, 1, 15), datetime(2024, 4, 15))
    s4, st4 = scenario("10 year span", datetime(2015, 6, 1), datetime(2025, 6, 1))
    s5, st5 = scenario("8.5 month span ( bonus: Month -> Week reclassification )",
                       datetime(2024, 1, 15), datetime(2024, 9, 15))

    print("\n[D2] week alignment check ( 3 month scenario ):")
    for v in s3.major:
        dt = to_datetime(v)
        print("     major %s  weekday = %s" % (dt.strftime("%Y-%m-%d"), dt.strftime("%A")))

    print("\n[D2b] locale dependence: QLocale().firstDayOfWeek() drives the Week alignment")
    set_first_day_of_week(7)      # zh-CN ( CLDR ): weeks start on Sunday
    x1m, x2m = to_double(datetime(2024, 1, 15)), to_double(datetime(2024, 4, 15))
    a1m, a2m, hintm, tm = date_auto_scale(x1m, x2m, PLOT_MAX_MAJOR)
    stm = {}
    sdm = date_divide_scale(a1m, a2m, PLOT_MAX_MAJOR, PLOT_MAX_MINOR, hintm, 4, stm)
    set_first_day_of_week(1)      # restore Monday ( ISO / en-GB )
    print("     Sunday-based ( zh-CN ): majors = %s"
          % ", ".join("%s(%s)" % (to_datetime(v).strftime("%m-%d"), to_datetime(v).strftime("%a"))
                      for v in sdm.major))
    print("     Monday-based ( en-GB ): majors = %s"
          % ", ".join("%s(%s)" % (to_datetime(v).strftime("%m-%d"), to_datetime(v).strftime("%a"))
                      for v in s3.major))

    print("\n[D3] year tick collision check ( 10 year scenario ):")
    major_set = set(round(v) for v in s4.major)
    medium_set = set(round(v) for v in s4.medium)
    minor_set = set(round(v) for v in s4.minor)
    coll_med_maj = sum(1 for v in s4.medium if round(v) in major_set)
    coll_med_min = sum(1 for v in s4.medium if round(v) in minor_set)
    coll_min = sum(1 for v in s4.minor if round(v) in major_set or round(v) in medium_set)
    print("     majors  (%d): %s" % (len(s4.major), ", ".join(str(to_datetime(v).year) for v in s4.major)))
    print("     mediums (%d): %s" % (len(s4.medium), ", ".join(str(to_datetime(v).year) for v in s4.medium)))
    print("     minors  (%d): %s" % (len(s4.minor), ", ".join(str(to_datetime(v).year) for v in s4.minor)))
    print("     mediums coinciding with a major tick: %d / %d" % (coll_med_maj, len(s4.medium)))
    print("     mediums coinciding with a minor tick: %d / %d" % (coll_med_min, len(s4.medium)))
    print("     minors coinciding with a major/medium tick: %d / %d" % (coll_min, len(s4.minor)))

    print("\n[D4] single point series ( degenerate interval, width == 0 ):")
    t0 = datetime(2024, 3, 10, 12, 0, 0)
    x = to_double(t0)
    a1, a2, hint, t_auto = date_auto_scale(x, x, PLOT_MAX_MAJOR)
    print("     value = %s -> autoScale bounds = %s -> %s  ( span = %.3g days, type = %s )"
          % (t0.isoformat(), fmt_dt(a1), fmt_dt(a2), (a2 - a1) / 86400000.0, TYPE_NAMES[t_auto]))

    print("\n[D5] classification sweep ( x1 = 2024-01-15T06:30:00.123, maxSteps = 8 ):")
    t0 = datetime(2024, 1, 15, 6, 30, 0, 123000)
    spans = np.logspace(0.0, math.log10(40 * 365 * 86400 * 1000.0), 500)
    types = []
    for sp in spans:
        t1 = to_datetime(to_double(t0) + float(sp))
        types.append(interval_type(t0, t1, PLOT_MAX_MAJOR))
    types = np.array(types)
    print("     span boundaries between consecutive interval types:")
    for i in range(1, len(types)):
        if types[i] != types[i - 1]:
            lo_ms, hi_ms = spans[i - 1], spans[i]
            print("       %-11s -> %-11s between %10.4g ms ( %.4g days ) and %10.4g ms ( %.4g days )"
                  % (TYPE_NAMES[types[i - 1]], TYPE_NAMES[types[i]], lo_ms, lo_ms / 86400000.0,
                     hi_ms, hi_ms / 86400000.0))

    print("\n[D6] week0 dates ( FirstThursday, Monday-based locale ):")
    for y in (2021, 2023, 2024, 2025):
        print("     dateOfWeek0(%d) = %s ( %s )" % (y, date_of_week0(y), date_of_week0(y).strftime("%A")))

    print("\n[D7] Year minor steps vs maxMinorSteps ( major step = 2 years ):")
    for mn in (0, 1, 2, 3, 4, 5, 8):
        ms = divide_major_step(2.0, mn, YEAR) if mn > 1 else 0.0
        num = qwt_floor(2.0 / ms) if ms > 0 else 0
        offs = [qround(i * ms) for i in range(1, num)]
        print("     maxMinor = %d -> minStep = %-6g years, numMinorSteps = %d, month offsets per major = %s"
              % (mn, ms, num, [o * 12 for o in offs]))
    return spans, types, (s3, s4, s5)


# ------------------------------------------------------------------- figures

def make_figures(ratios, saw_w, saw_n, log_results, sweep, scen):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    here = os.path.dirname(os.path.abspath(__file__))
    COL1, COL2, COL3, COL4 = "#1f77b4", "#d62728", "#2ca02c", "#ff7f0e"

    def style(ax):
        ax.spines["top"].set_visible(False)
        ax.spines["right"].set_visible(False)
        ax.tick_params(labelsize=9)

    # --- fig_scale_density.png : sawtooth + ratio histogram
    fig, axes = plt.subplots(1, 2, figsize=(10.4, 3.6))
    ax = axes[0]
    ax.plot(saw_w, saw_n, lw=0.9, color=COL1)
    ax.axhline(PLOT_MAX_MAJOR, color="#999999", ls=":", lw=1)
    ax.axhline(PLOT_MAX_MAJOR / 2.5, color=COL2, ls="--", lw=1)
    ax.text(1.2, 8.15, "maxMajorSteps = 8", fontsize=8.5, color="#666666")
    ax.text(1.2, 3.35, "8 / 2.5 = 3.2 ( worst case )", fontsize=8.5, color=COL2)
    ax.set_xscale("log")
    ax.set_xlabel("interval width $w$ (log scale)")
    ax.set_ylabel("major intervals $n = w / \\mathrm{step}(w)$")
    ax.set_title("(a) tick count sawtooth, $x_1 = 0$, maxMajor = 8", fontsize=9.5)
    style(ax)

    ax = axes[1]
    bins = np.linspace(1.0, 2.5, 31)
    ax.hist(ratios, bins=bins, color=COL3, edgecolor="white")
    ax.axvline(10 ** (1 / 3.0), color=COL2, ls="--", lw=1.2)
    ax.text(10 ** (1 / 3.0) + 0.02, ax.get_ylim()[1] * 0.85, "$10^{1/3} = 2.154$\n(geometric optimum)",
            fontsize=8.5, color=COL2)
    ax.axvline(2.5, color="#666666", ls=":", lw=1.2)
    ax.text(2.42, ax.get_ylim()[1] * 0.5, "2.5", fontsize=8.5, color="#666666", ha="right")
    ax.set_xlabel("step / (width / maxMajor)  ratio")
    ax.set_ylabel("count")
    ax.set_title("(b) rounding-up ratio, 10,000 random intervals", fontsize=9.5)
    style(ax)
    fig.tight_layout()
    fig.savefig(os.path.join(here, "fig_scale_density.png"), dpi=200, bbox_inches="tight")
    plt.close(fig)

    # --- fig_scale_mantissa.png : set comparison
    fig, axes = plt.subplots(1, 2, figsize=(10.4, 3.6))
    ax = axes[0]
    names = ["{1,2,5}\n(QWT, Heckbert)", "{1,2,4}", "{2,4,9}\n(integer minimax)",
             "{1,2,2.5,5}", "geometric\n$k=3$", "geometric\n$k=4$"]
    vals = [2.5, 2.5, 2.25, 2.0, 10 ** (1 / 3.0), 10 ** 0.25]
    cols = [COL2, "#888888", "#888888", COL4, COL1, COL1]
    bars = ax.bar(names, vals, color=cols, edgecolor="white")
    for b, v in zip(bars, vals):
        ax.text(b.get_x() + b.get_width() / 2, v + 0.03, "%.3f" % v, ha="center", fontsize=8.5)
    ax.axhline(10 ** (1 / 3.0), color=COL1, ls=":", lw=1)
    ax.set_ylim(0, 3.0)
    ax.set_ylabel("worst case ratio $r_{max}(S)$")
    ax.set_title("(a) rounding ladder quality: worst case step inflation", fontsize=9.5)
    ax.tick_params(axis="x", labelsize=8)
    style(ax)

    ax = axes[1]
    # how the three mantissa classes tile the target axis [1,10)
    for lo, hi, lab, c in ((1, 2, "step mantissa 2", COL1), (2, 5, "step mantissa 5", COL3),
                           (5, 10, "step mantissa 1 (x10)", COL2)):
        ax.axvspan(lo, hi, color=c, alpha=0.25)
        ax.text(math.sqrt(lo * hi), 0.5, lab, ha="center", fontsize=8.5, color=c)
    for v in (1, 2, 5, 10):
        ax.axvline(v, color="#555555", lw=1)
    ax.set_xscale("log")
    ax.set_xlim(1, 10)
    ax.set_xticks([1, 2, 5, 10])
    ax.set_xticklabels(["1", "2", "5", "10"])
    ax.set_yticks([])
    ax.set_xlabel("raw target $w / maxMajor$, mantissa (log scale)")
    ax.set_title("(b) which target picks which nice step", fontsize=9.5)
    style(ax)
    fig.tight_layout()
    fig.savefig(os.path.join(here, "fig_scale_mantissa.png"), dpi=200, bbox_inches="tight")
    plt.close(fig)

    # --- fig_scale_log.png : tick structure on log axes
    fig, axes = plt.subplots(3, 1, figsize=(10.0, 5.6))
    panels = [("[1, 1000], maxMajor = 8 -> majors clamped to 1 decade", log_results["[1, 1000]      3 decades"]),
              ("[3, 300], maxMajor = 8 -> log-aligned to [1, 1000], strip keeps 2 majors", log_results["[3, 300]       non-power bounds"]),
              ("[1e-6, 1e6], maxMajor = 8 -> 2 decade majors", log_results["[1e-6, 1e6]    12 decades"])]
    for ax, (title, sd) in zip(axes, panels):
        ax.set_xscale("log")
        ax.set_xlim(sd.lower * 0.98, sd.upper * 1.02)
        ax.hlines(0, sd.lower * 0.98, sd.upper * 1.02, color="#333333", lw=1.2)
        for t in sd.minor:
            ax.vlines(t, 0, 0.25, color="#999999", lw=0.8)
        for t in sd.medium:
            ax.vlines(t, 0, 0.45, color=COL4, lw=1.0)
        for t in sd.major:
            ax.vlines(t, 0, 0.75, color=COL2, lw=1.4)
        for t in sd.major:
            ax.text(t, 0.80, "%.3g" % t, ha="center", fontsize=7.5, color=COL2)
        ax.set_ylim(0, 1.15)
        ax.set_yticks([])
        ax.set_title(title, fontsize=9, loc="left")
        style(ax)
    axes[-1].set_xlabel("value (log scale)")
    fig.tight_layout()
    fig.savefig(os.path.join(here, "fig_scale_log.png"), dpi=200, bbox_inches="tight")
    plt.close(fig)

    # --- fig_scale_date.png : classification sweep + scenario timelines
    spans, types, (s3, s4, s5) = sweep[0], sweep[1], scen
    fig, axes = plt.subplots(3, 1, figsize=(10.2, 7.6))

    ax = axes[0]
    ax.step(spans / 1000.0, types, where="post", color=COL1, lw=1.3)
    ax.set_xscale("log")
    ax.set_yticks(range(8))
    ax.set_yticklabels(TYPE_NAMES, fontsize=8)
    ax.set_ylim(-0.5, 7.5)
    ax.set_xlabel("span (seconds, log scale)")
    ax.set_title("(a) intervalType classification vs span ( maxSteps = 8, maxWeeks = 4 )", fontsize=9.5)
    for y in range(8):
        ax.axhline(y, color="#eeeeee", lw=0.6, zorder=0)
    style(ax)

    ax = axes[1]
    lo, hi = s3.lower, s3.upper
    ax.hlines(0, lo, hi, color="#333333", lw=1.2)
    for t in s3.minor:
        ax.vlines(t, 0, 0.2, color="#999999", lw=0.8)
    for t in s3.major:
        ax.vlines(t, 0, 0.6, color=COL2, lw=1.4)
        ax.text(t, 0.66, to_datetime(t).strftime("%m-%d"), ha="center", fontsize=7.5, rotation=45)
    ax.set_yticks([])
    ax.set_ylim(0, 1.15)
    ax.set_title("(b) 3 month span -> Week type, 2 week steps, all majors on Mondays", fontsize=9.5, loc="left")
    ax.set_xticks([])
    style(ax)

    ax = axes[2]
    lo, hi = s4.lower, s4.upper
    ax.hlines(0, lo, hi, color="#333333", lw=1.2)
    for t in s4.minor:
        ax.vlines(t, 0, 0.2, color="#999999", lw=0.8)
    for t in s4.medium:
        ax.vlines(t, 0, 0.4, color=COL4, lw=1.0)
    for t in s4.major:
        ax.vlines(t, 0, 0.6, color=COL2, lw=1.4)
        ax.text(t, 0.66, str(to_datetime(t).year), ha="center", fontsize=7.5)
    ax.set_yticks([])
    ax.set_ylim(0, 1.15)
    ax.set_title("(c) 10 year span -> Year type, 2 year steps; minor/medium ticks collapse onto majors (qRound)",
                 fontsize=9.5, loc="left")
    ax.set_xticks([])
    style(ax)

    fig.tight_layout()
    fig.savefig(os.path.join(here, "fig_scale_date.png"), dpi=200, bbox_inches="tight")
    plt.close(fig)

    print("\nfigures written to", here)
    for f in ("fig_scale_density.png", "fig_scale_mantissa.png", "fig_scale_log.png", "fig_scale_date.png"):
        p = os.path.join(here, f)
        print("  %-28s %s ( %d bytes )" % (f, "OK" if os.path.exists(p) else "MISSING",
                                            os.path.getsize(p) if os.path.exists(p) else 0))


# ------------------------------------------------------------- C++ cross-check

def cpp_check(exe):
    """Pipe deterministic cases through the C++ harness and compare."""
    import subprocess
    import tempfile

    rng = np.random.default_rng(SEED)
    tmpdir = tempfile.mkdtemp(prefix="qwt_scale_check_")

    def run_mode(mode, lines):
        infile = os.path.join(tmpdir, mode + ".txt")
        with open(infile, "w", encoding="ascii") as f:
            f.write("\n".join(lines) + "\n")
        out = subprocess.run([exe, mode, infile], capture_output=True, text=True, timeout=600)
        if out.returncode != 0:
            print("harness failed for mode %s:\n%s\n%s" % (mode, out.stdout, out.stderr))
            return None
        return out.stdout.splitlines()

    def close(a, b, scale=1.0):
        if math.isnan(a) and math.isnan(b):
            return True
        return abs(a - b) <= 1e-9 * max(1.0, abs(a), abs(b)) * max(1.0, scale)

    # ---- linear + pipeline
    n_lin = 2000
    x1, x2 = random_intervals(n_lin, rng)
    lines = ["%.17g %.17g %d %d 0" % (x1[i], x2[i], PLOT_MAX_MAJOR, PLOT_MAX_MINOR) for i in range(n_lin)]
    res = run_mode("linear", lines)
    n_ok = n_bad = 0
    bad_examples = []
    auto, div, pipe = {}, {}, {}
    for ln in res:
        parts = ln.split()
        if parts[0] == "A":
            auto[int(parts[1])] = tuple(float(v) for v in parts[2:5])
        elif parts[0] == "D":
            div[int(parts[1])] = (int(parts[2]), int(parts[3]), int(parts[4]), float(parts[5]), float(parts[6]))
        elif parts[0] == "P":
            pipe[int(parts[1])] = (int(parts[2]), int(parts[3]), int(parts[4]), float(parts[5]), float(parts[6]))
    for i in range(n_lin):
        lo, hi = min(x1[i], x2[i]), max(x1[i], x2[i])
        a1, a2, st = linear_auto_scale(lo, hi, PLOT_MAX_MAJOR)
        ca = auto[i]
        ok = close(a1, ca[0]) and close(a2, ca[1]) and close(st, ca[2])
        sd = linear_divide_scale(lo, hi, PLOT_MAX_MAJOR, PLOT_MAX_MINOR, 0.0)
        cd = div[i]
        ok = ok and len(sd.major) == cd[0] and len(sd.minor) == cd[1] and len(sd.medium) == cd[2]
        if sd.major:
            ok = ok and close(sd.major[0], cd[3], abs(hi - lo)) and close(sd.major[-1], cd[4], abs(hi - lo))
        sd2 = linear_divide_scale(a1, a2, PLOT_MAX_MAJOR, PLOT_MAX_MINOR, st)
        cp = pipe[i]
        ok = ok and len(sd2.major) == cp[0] and len(sd2.minor) == cp[1] and len(sd2.medium) == cp[2]
        if ok:
            n_ok += 1
        else:
            n_bad += 1
            if len(bad_examples) < 5:
                bad_examples.append((i, lo, hi))
    print("[X1] linear engine: %d cases, match = %d, mismatch = %d" % (n_lin, n_ok, n_bad))
    for i, lo, hi in bad_examples:
        print("     mismatch case %d: [%.17g, %.17g]" % (i, lo, hi))

    # ---- log
    n_log = 400
    lx1 = 10.0 ** rng.uniform(-20, 19, n_log)
    lratio = 10.0 ** rng.uniform(0.05, 8.0, n_log)
    lx2 = lx1 * lratio
    lines = ["%.17g %.17g %d %d 0" % (lx1[i], lx2[i], PLOT_MAX_MAJOR, PLOT_MAX_MINOR) for i in range(n_log)]
    # a few degenerate / fallback cases
    extra = [(0.0, 1000.0), (-5.0, 500.0), (1.0, math.sqrt(10)), (1e-300, 1e300), (3.0, 300.0)]
    for a, b in extra:
        lines.append("%.17g %.17g %d %d 0" % (a, b, PLOT_MAX_MAJOR, PLOT_MAX_MINOR))
    res = run_mode("log", lines)
    div = {}
    for ln in res:
        parts = ln.split()
        if parts[0] == "D":
            div[int(parts[1])] = (int(parts[2]), int(parts[3]), int(parts[4]), float(parts[5]), float(parts[6]))
    n_ok = n_bad = 0
    bad_examples = []
    allx1 = list(lx1) + [a for a, b in extra]
    allx2 = list(lx2) + [b for a, b in extra]
    for i in range(len(allx1)):
        st = {}
        sd = log_divide_scale(allx1[i], allx2[i], PLOT_MAX_MAJOR, PLOT_MAX_MINOR, 0.0, 10.0, st)
        cd = div[i]
        ok = len(sd.major) == cd[0] and len(sd.minor) == cd[1] and len(sd.medium) == cd[2]
        if sd.major and ok:
            scale = max(abs(sd.major[0]), abs(sd.major[-1]), 1.0)
            ok = close(sd.major[0], cd[3], scale) and close(sd.major[-1], cd[4], scale)
        if ok:
            n_ok += 1
        else:
            n_bad += 1
            if len(bad_examples) < 5:
                bad_examples.append((i, allx1[i], allx2[i], len(sd.major), cd[0]))
    print("[X2] log engine: %d cases, match = %d, mismatch = %d" % (len(allx1), n_ok, n_bad))
    for tup in bad_examples:
        print("     mismatch case %d: [%.10g, %.10g] pyMajors=%d cppMajors=%d" % tup)

    # ---- date ( pipeline + direct )
    n_date = 600
    base_ms = to_double(datetime(2000, 1, 1))
    d_x1 = base_ms + rng.uniform(0, 40 * 365 * 86400 * 1000.0, n_date)
    d_span = 10.0 ** rng.uniform(0.0, math.log10(30 * 365 * 86400 * 1000.0), n_date)
    scen = [
        (to_double(datetime(2024, 3, 10, 12, 0, 0)), 100.0),
        (to_double(datetime(2024, 3, 10)), 86400000.0),
        (to_double(datetime(2024, 1, 15)), to_double(datetime(2024, 4, 15)) - to_double(datetime(2024, 1, 15))),
        (to_double(datetime(2015, 6, 1)), to_double(datetime(2025, 6, 1)) - to_double(datetime(2015, 6, 1))),
        (to_double(datetime(2024, 1, 15)), to_double(datetime(2024, 9, 15)) - to_double(datetime(2024, 1, 15))),
        (to_double(datetime(2024, 2, 25)), to_double(datetime(2024, 3, 5)) - to_double(datetime(2024, 2, 25))),
        (to_double(datetime(1999, 12, 31, 23, 59, 59)), 2000.0),
        (to_double(datetime(2024, 12, 30)), 4 * 86400000.0),
    ]
    lines = []
    cases = []
    for i in range(n_date):
        lines.append("%.17g %.17g %d %d 0" % (d_x1[i], d_x1[i] + d_span[i], PLOT_MAX_MAJOR, PLOT_MAX_MINOR))
        cases.append((d_x1[i], d_x1[i] + d_span[i]))
    for x, sp in scen:
        lines.append("%.17g %.17g %d %d 0" % (x, x + sp, PLOT_MAX_MAJOR, PLOT_MAX_MINOR))
        cases.append((x, x + sp))
    res = run_mode("date", lines)
    auto, ddiv, pipe = {}, {}, {}
    for ln in res:
        parts = ln.split()
        if parts[0] == "A":
            auto[int(parts[1])] = tuple(float(v) for v in parts[2:5])
        elif parts[0] == "D":
            ddiv[int(parts[1])] = (int(parts[2]), int(parts[3]), int(parts[4]), float(parts[5]), float(parts[6]))
        elif parts[0] == "P":
            pipe[int(parts[1])] = (int(parts[2]), int(parts[3]), int(parts[4]), float(parts[5]), float(parts[6]))
    n_ok = n_bad = 0
    bad_examples = []
    for i, (xa, xb) in enumerate(cases):
        # direct divideScale ( no hint )
        st = {}
        sdd = date_divide_scale(xa, xb, PLOT_MAX_MAJOR, PLOT_MAX_MINOR, 0.0, 4, st)
        cdd = ddiv[i]
        ok = len(sdd.major) == cdd[0] and len(sdd.minor) == cdd[1] and len(sdd.medium) == cdd[2]
        if sdd.major and ok:
            ok = abs(sdd.major[0] - cdd[3]) <= 0.5 and abs(sdd.major[-1] - cdd[4]) <= 0.5
        # pipeline: autoScale then divideScale with the hint
        a1, a2, hint, t_auto = date_auto_scale(xa, xb, PLOT_MAX_MAJOR)
        ca = auto[i]
        ok = ok and abs(a1 - ca[0]) <= 0.5 and abs(a2 - ca[1]) <= 0.5 and abs(hint - ca[2]) <= 0.5
        st = {}
        sd = date_divide_scale(a1, a2, PLOT_MAX_MAJOR, PLOT_MAX_MINOR, hint, 4, st)
        cp = pipe[i]
        ok = ok and len(sd.major) == cp[0] and len(sd.minor) == cp[1] and len(sd.medium) == cp[2]
        if sd.major and ok:
            ok = abs(sd.major[0] - cp[3]) <= 0.5 and abs(sd.major[-1] - cp[4]) <= 0.5
        if ok:
            n_ok += 1
        else:
            n_bad += 1
            if len(bad_examples) < 6:
                bad_examples.append((i, xa, xb, len(sd.major), cp[0], len(sd.minor), cp[1], len(sd.medium), cp[2]))
    print("[X3] date engine ( UTC, direct + pipeline ): %d cases, match = %d, mismatch = %d"
          % (len(cases), n_ok, n_bad))
    for tup in bad_examples:
        print("     mismatch case %d: x1=%.0f x2=%.0f  py(maj,min,med)=%d,%d,%d  cpp=%d,%d,%d"
              % (tup[0], tup[1], tup[2], tup[3], tup[5], tup[7], tup[4], tup[6], tup[8]))


# ------------------------------------------------------------------ self test

def self_test():
    print("=" * 78)
    print("[S0] PORT SELF TEST ( hand computed expectations )")
    print("=" * 78)
    checks = []

    def ck(name, got, want, tol=1e-9):
        ok = (abs(got - want) <= tol * max(1.0, abs(want))) if isinstance(want, float) else (got == want)
        checks.append(ok)
        print("     %-58s got %-24s %s" % (name, got, "OK" if ok else "FAIL want %s" % want))

    ck("divideInterval(100, 8)", divide_interval(100, 8), 20.0)
    ck("divideInterval(15, 5)", divide_interval(15, 5), 5.0)
    ck("divideInterval(9.99, 10)", divide_interval(9.99, 10), 1.0)
    ck("divideInterval(2.4, 8)", divide_interval(2.4, 8), 0.5)
    ck("divideInterval(1000, 8)", divide_interval(1000, 8), 200.0)
    ck("divideInterval(0.7, 8)", divide_interval(0.7, 8), 0.1)

    st = {}
    sd = linear_divide_scale(0.5, 10.49, 10, 5, 0.0, 10, st)
    ck("divideScale([0.5,10.49],10,5) step", st["step"], 1.0)
    ck("divideScale([0.5,10.49],10,5) #majors", len(sd.major), 10)

    a1, a2, stp = linear_auto_scale(0.5, 10.49, 10)
    ck("autoScale([0.5,10.49],10) bounds", (a1, a2), (0.0, 11.0))
    sd2 = linear_divide_scale(a1, a2, 10, 5, stp)
    ck("autoScale pipeline #majors ( > maxMajor ! )", len(sd2.major), 12)

    # minor structure of the three mantissa classes, maxMinor = 5
    minors, mediums, _ = linear_build_minor_ticks([0.0], 5, 1.0)
    ck("minor ticks per gap, step = 1 (maxMinor 5)", (len(minors), len(mediums)), (4, 0))
    minors, mediums, _ = linear_build_minor_ticks([0.0], 5, 2.0)
    ck("minor ticks per gap, step = 2 (maxMinor 5)", (len(minors), len(mediums)), (2, 1))
    minors, mediums, _ = linear_build_minor_ticks([0.0], 5, 5.0)
    ck("minor ticks per gap, step = 5 (maxMinor 5)", (len(minors), len(mediums)), (4, 0))
    ms, fb = qwt_step_size_minor(5.0, 3, 10)
    ck("qwtStepSize(5, 3) fallback 0.5*step", (ms, fb), (2.5, True))

    stt = {}
    sd = log_divide_scale(1.0, 1000.0, 8, 5, 0.0, 10.0, stt)
    ck("divideInterval(3, 8) ( pre-clamp log step )", divide_interval(3, 8), 0.5)
    ck("log [1,1000] step ( clamped to >= 1 decade )", stt["step"], 1.0)
    ck("log [1,1000] #majors", len(sd.major), 4)
    stt = {}
    sd = log_divide_scale(1.0, math.sqrt(10.0), 8, 5, 0.0, 10.0, stt)
    ck("log [1,sqrt(10)] linear fallback", stt["linear_fallback"], True)

    t = interval_type(datetime(2024, 1, 15), datetime(2024, 4, 15), 8)
    ck("intervalType(3 months, 8) == Week", TYPE_NAMES[t], "Week")
    t = interval_type(datetime(2015, 6, 1), datetime(2025, 6, 1), 8)
    ck("intervalType(10 years, 8) == Year", TYPE_NAMES[t], "Year")
    t = interval_type(datetime(2024, 3, 10), datetime(2024, 3, 11), 8)
    ck("intervalType(1 day, 8) == Hour", TYPE_NAMES[t], "Hour")
    ck("dateOfWeek0(2021, FirstThursday)", str(date_of_week0(2021)), "2021-01-04")
    ck("dateOfWeek0(2024, FirstThursday)", str(date_of_week0(2024)), "2024-01-01")

    print("     self test: %d/%d passed" % (sum(checks), len(checks)))
    return all(checks)


def main():
    args = sys.argv[1:]
    if "--cpp-check" in args:
        idx = args.index("--cpp-check")
        exe = args[idx + 1] if idx + 1 < len(args) else "harness.exe"
        cpp_check(exe)
        return

    ok = self_test()
    print()
    rng = np.random.default_rng(SEED)
    ratios, direct_iv, pipe_iv, mant = experiment_a(rng)
    saw_w, saw_steps, saw_n = experiment_b(rng)
    log_results = experiment_c()
    spans, types, scen = experiment_d()

    if "--no-figures" not in args:
        make_figures(ratios, saw_w, saw_n, log_results, (spans, types), scen)

    print("\nself test passed:", ok)


if __name__ == "__main__":
    main()
