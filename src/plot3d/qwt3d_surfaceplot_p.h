#ifndef QWT3D_SURFACEPLOT_P_H
#define QWT3D_SURFACEPLOT_P_H

#include "qwt3d_surfaceplot.h"
#include "qwt3d_plot_p.h"



class SurfacePlot::PrivateData
{
    QWT_DECLARE_PUBLIC(SurfacePlot)

public:
    PrivateData(SurfacePlot* q);

    bool m_dataNormals;
    double m_normalLength;
    int m_normalQuality;
    int m_resolution;

    FLOORSTYLE m_floorStyle;

    Qwt3DGridData* m_actualDataG;
    Qwt3DCellData* m_actualDataC;
};


#endif  // QWT3D_SURFACEPLOT_P_H
