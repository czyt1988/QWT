#ifndef QWT3D_HELPER_H
#define QWT3D_HELPER_H

#include <cmath>
#include <cfloat>
#include <vector>
#include <algorithm>

namespace
{
// Returns the minimum of two double values
inline double Min_(double a, double b)
{
    return (a < b) ? a : b;
}
}



// Checks if a value is practically zero (within floating-point epsilon)
inline bool isPracticallyZero(double a, double b = 0)
{
    if (!b)
        return (fabs(a) <= DBL_MIN);

    return (fabs(a - b) <= Min_(fabs(a), fabs(b)) * DBL_EPSILON);
}


#endif