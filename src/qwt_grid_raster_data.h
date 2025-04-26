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

    virtual QwtInterval interval(Qt::Axis axis) const QWT_OVERRIDE QWT_FINAL;

    /**
     * @brief Set new x-axis, y-axis, and data matrix.
     *
     * data matrix is look like that:
     *
     * |column[0]|column[1]| ... |column[m]|
     * +---------+---------+-----+---------+
     * | [x0,yn] | [x1,yn] | ... | [xm,yn] | → yAxis[n] 对应行
     * +---------+---------+-----+---------+
     * |   ...   |   ...   | ... |   ...   |
     * +---------+---------+-----+---------+
     * | [x0,y1] | [x1,y1] | ... | [xm,y1] | → yAxis[1] 对应行
     * +---------+---------+-----+---------+
     * | [x0,y0] | [x1,y0] | ... | [xm,y0] | → yAxis[0] 对应行
     * +---------+---------+-----+---------+
     *      ↑          ↑      ↑       ↑
     *  xAxis[0]   xAxis[1]  ...   xAxis[m]
     *
     *  so (data matrix).size = xAxis.size,(data matrix).at(n).size = yAxis.szie
     *
     * 设置新的 x 轴、y 轴和数据矩阵。
     * 数据矩阵是一个vector<vector> ,数据矩阵.size = xAxis.size,数据矩阵.at(n).size = yAxis.size
     *
     * @param xAxis The x-axis values. / x 轴值。
     * @param yAxis The y-axis values. / y 轴值。
     * @param data The 2D data matrix. / 二维数据矩阵。
     */
    void setValue(const QVector< double >& x, const QVector< double >& y, const QVector< QVector< double > >& v);
    virtual double value(double x, double y) const QWT_OVERRIDE;

    virtual QRectF pixelHint(const QRectF&) const QWT_OVERRIDE;

private:
    class PrivateData;
    PrivateData* m_data;
};

#endif  // QWTGRIDRASTERDATA_H
