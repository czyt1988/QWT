#ifndef QWT3D_GRAPHPLOT_H
#define QWT3D_GRAPHPLOT_H

#include "qwt3d_plot.h"



/**
 * @brief Base class for graph plotting widgets
 * @details GraphPlot is an intermediate class between Plot3D and concrete graph plot types.
 *          It provides a common base for different graph plotting implementations.
 */
class QWT3D_EXPORT GraphPlot : public Plot3D
{
public:
    // Constructor
    explicit GraphPlot(QWidget* parent = nullptr);

protected:
    virtual void createData() override = 0;
};


#endif  // QWT3D_GRAPHPLOT_H