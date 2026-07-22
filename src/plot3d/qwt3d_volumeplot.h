#ifndef QWT3D_VOLUMEPLOT_H
#define QWT3D_VOLUMEPLOT_H

#include "qwt3d_plot.h"



/**
 * @brief Volume plot widget (TODO: not yet fully implemented)
 * @details VolumePlot is intended to provide 3D volume visualization capabilities.
 *          This class is a placeholder for future implementation.
 */
class QWT3D_EXPORT VolumePlot : public Plot3D
{
public:
    // Constructor
    explicit VolumePlot(QWidget* parent = nullptr)
    {
    }

protected:
    virtual void createData() override = 0;
};


#endif  // QWT3D_VOLUMEPLOT_H