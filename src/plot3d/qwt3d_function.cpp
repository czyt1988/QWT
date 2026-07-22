#include "qwt3d_surfaceplot.h"
#include "qwt3d_function.h"


/**
 * @brief Default constructor
 */
Function::Function() : GridMapping()
{
}

/**
 * @brief Constructs a Function object and assigns a SurfacePlot
 * @param pw Reference to a SurfacePlot widget
 */
Function::Function(SurfacePlot& pw) : GridMapping()
{
    setPlotWidget(&pw);
}

/**
 * @brief Constructs a Function object and assigns a SurfacePlot
 * @param pw Pointer to a SurfacePlot widget
 */
Function::Function(SurfacePlot* pw) : GridMapping()
{
    setPlotWidget(pw);
}

/**
 * @brief Assigns the object to another widget - call before create()
 * @param plotWidget Reference to a SurfacePlot widget
 */
void Function::assign(SurfacePlot& plotWidget)
{
    if (&plotWidget != this->plotWidget())
        setPlotWidget(&plotWidget);
}

/**
 * @brief Assigns the object to another widget - call before create()
 * @param plotWidget Pointer to a SurfacePlot widget
 */
void Function::assign(SurfacePlot* plotWidget)
{
    if (plotWidget != this->plotWidget())
        setPlotWidget(plotWidget);
}

/**
 * @brief Sets minimum z value for the function
 * @param val Minimum z value
 */
void Function::setMinZ(double val)
{
    range().minVertex.z = val;
}

/**
 * @brief Sets maximum z value for the function
 * @param val Maximum z value
 */
void Function::setMaxZ(double val)
{
    range().maxVertex.z = val;
}

/**
 * @brief Creates data representation for the actual assigned SurfacePlot
 * @return True on success, false if mesh is too small or no widget assigned
 * @details Allocates data arrays, evaluates the function operator() over the
 *          mesh grid, clips values to the min/max z range, and loads data
 *          into the assigned SurfacePlot.
 */
bool Function::create()
{
    const unsigned int um = meshU();
    const unsigned int vm = meshV();

    if ((um <= 2) || (vm <= 2) || !plotWidget())
        return false;

    /* allocate some space for the mesh */
    double** data = new double*[ um ];

    unsigned i, j;
    for (i = 0; i < um; i++) {
        data[ i ] = new double[ vm ];
    }

    /* get the data */

    double dx = (maxU() - minU()) / (um - 1);
    double dy = (maxV() - minV()) / (vm - 1);

    for (i = 0; i < um; ++i) {
        for (j = 0; j < vm; ++j) {
            data[ i ][ j ] = operator()(minU() + i * dx, minV() + j * dy);

            if (data[ i ][ j ] > range().maxVertex.z)
                data[ i ][ j ] = range().maxVertex.z;
            else if (data[ i ][ j ] < range().minVertex.z)
                data[ i ][ j ] = range().minVertex.z;
        }
    }

    Q_ASSERT(plotWidget());
    if (!plotWidget()) {
        fprintf(stderr, "Function: no valid Plot3D Widget assigned");
    } else {
        static_cast< SurfacePlot* >(plotWidget())->loadFromData(data, um, vm, minU(), maxU(), minV(), maxV());
    }

    for (i = 0; i < um; i++) {
        delete[] data[ i ];
    }

    delete[] data;

    return true;
}

/**
 * @brief Assigns a new SurfacePlot and creates a data representation for it
 * @param pl Reference to a SurfacePlot widget
 * @return True on success
 */
bool Function::create(SurfacePlot& pl)
{
    assign(pl);
    return create();
}
