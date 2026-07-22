#ifndef QWT3D_MULTIPLOT_H
#define QWT3D_MULTIPLOT_H

#include "qwt3d_plot.h"



/**
 * @brief Multi-plot widget for combining multiple plot areas
 * @details MultiPlot is an intermediate class between Plot3D and concrete multi-plot
 *          implementations, allowing multiple plot areas in a single widget.
 */
class QWT3D_EXPORT MultiPlot : public Plot3D
{
public:
    // Constructor
    explicit MultiPlot(QWidget* parent = nullptr)
    {
    }

protected:
    virtual void createData() override = 0;
};


#endif  // QWT3D_MULTIPLOT_H