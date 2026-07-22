#ifndef QWT3D_GRIDMAPPING_H
#define QWT3D_GRIDMAPPING_H

#include "qwt3d_mapping.h"



class Qwt3DSurface;

/**
 * @brief Abstract base class for mappings acting on rectangular grids
 */
class QWT3D_EXPORT Qwt3DGridMapping : public Qwt3DMapping
{
public:
    // Constructs Qwt3DGridMapping object w/o assigned Qwt3DSurface
    Qwt3DGridMapping();
    ~Qwt3DGridMapping() override;

    // Sets number of rows and columns
    void setMesh(unsigned int columns, unsigned int rows);
    // Sets u-v domain boundaries
    void setDomain(double minu, double maxu, double minv, double maxv);
    // Restrict the mappings range to the parallelepiped
    void restrictRange(ParallelEpiped const&);

protected:
    QWT_DECLARE_PRIVATE(Qwt3DGridMapping)

    // Accessors for subclasses
    Qwt3DSurface* surface() const;
    void setSurface(Qwt3DSurface* surface);

    ParallelEpiped& range();
    const ParallelEpiped& range() const;

    unsigned int meshU() const;
    unsigned int meshV() const;

    double minU() const;
    double maxU() const;
    double minV() const;
    double maxV() const;
};


#endif  // QWT3D_GRIDMAPPING_H
