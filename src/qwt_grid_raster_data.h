#ifndef QWT_GRID_RASTER_DATA_H
#define QWT_GRID_RASTER_DATA_H
#include "qwt_global.h"
#include "qwt_raster_data.h"

#if QT_VERSION < 0x060000
template< typename T >
class QVector;
#endif

/**
 * \class QwtGridRasterData
 * \brief A class that encapsulates grid data and provides interpolation methods.
 *
 * This class inherits from QwtRasterData and is used to represent 2D grid data.
 * It supports various interpolation methods such as nearest neighbor and bilinear interpolation.
 *
 * 此类继承自 QwtRasterData，用于表示二维网格数据。
 * 它支持多种插值方法，例如最近邻插值和双线性插值。
 */
class QWT_EXPORT QwtGridRasterData : public QwtRasterData
{
public:
    /*!
       \brief Resampling algorithm
       The default setting is NearestNeighbour;
     */
    enum ResampleMode
    {
        /*!
           Return the value from the matrix, that is nearest to the
           the requested position.
         */
        NearestNeighbour,

        /*!
           Interpolate the value from the distances and values of the
           4 surrounding values in the matrix,
         */
        BilinearInterpolation,

        /*!
           Interpolate the value from the 16 surrounding values in the
           matrix using hermite bicubic interpolation
         */
        BicubicInterpolation
    };

public:
    QwtGridRasterData();
    virtual ~QwtGridRasterData();

    //  Set the resampling algorithm
    void setResampleMode(ResampleMode mode);
    ResampleMode resampleMode() const;

    void setInterval(Qt::Axis, const QwtInterval&);
    virtual QwtInterval interval(Qt::Axis axis) const QWT_OVERRIDE QWT_FINAL;

private:
    void update();

    class PrivateData;
    PrivateData* m_data;
};

#endif  // QWTGRIDRASTERDATA_H
