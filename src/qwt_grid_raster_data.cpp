#include "qwt_grid_raster_data.h"
#include "qwt_interval.h"
#include <QVector>
#include <QRectF>
#include "qwt_grid_data.hpp"
class QwtGridRasterData::PrivateData
{
public:
    using GridData = QwtGridData< double, QVector >;

public:
    PrivateData() : resampleMode(QwtGridRasterData::NearestNeighbour)
    {
    }

    static QwtGridRasterData::ResampleMode resampleModeCast(GridData::ResampleMode m);
    static GridData::ResampleMode resampleModeCast(QwtGridRasterData::ResampleMode m);

public:
    QwtGridRasterData::ResampleMode resampleMode;
    QwtInterval intervals[ 3 ];
    GridData gridData;
    double dxMin, dyMin;  ///< 把x轴和y轴相邻最小值记录下来，用于pixelHint
};

QwtGridRasterData::ResampleMode QwtGridRasterData::PrivateData::resampleModeCast(GridData::ResampleMode m)
{
    switch (m) {
    case GridData::NearestNeighbour:
        return QwtGridRasterData::NearestNeighbour;
    case GridData::BilinearInterpolation:
        return QwtGridRasterData::BilinearInterpolation;
    case GridData::BicubicInterpolation:
        return QwtGridRasterData::BicubicInterpolation;
    default:
        break;
    }
    return QwtGridRasterData::NearestNeighbour;
}

QwtGridRasterData::PrivateData::GridData::ResampleMode QwtGridRasterData::PrivateData::resampleModeCast(ResampleMode m)
{
    switch (m) {
    case QwtGridRasterData::NearestNeighbour:
        return GridData::NearestNeighbour;
    case QwtGridRasterData::BilinearInterpolation:
        return GridData::BilinearInterpolation;
    case QwtGridRasterData::BicubicInterpolation:
        return GridData::BicubicInterpolation;
    default:
        break;
    }
    return GridData::NearestNeighbour;
}

//===============================================================
// name
//===============================================================

QwtGridRasterData::QwtGridRasterData()
{
}

QwtGridRasterData::~QwtGridRasterData()
{
}

/**
 * @brief Set the resample method.
 *
 * Sets the resample method to be used when querying values.
 *
 * 设置查询值时使用的插值方法。
 *
 * @param method The resample method to use.
 *               要使用的插值方法。
 *
 * @sa resampleMode(), value()
 */
void QwtGridRasterData::setResampleMode(QwtGridRasterData::ResampleMode mode)
{
    m_data->gridData.setResampleMode(PrivateData::resampleModeCast(mode));
}

/**
 * @brief Get the current resample method.
 *
 * Returns the currently active resample method.
 *
 * 返回当前激活的插值方法。
 *
 * @return The current resample method.
 *         当前插值方法。
 *
 * @sa setResampleMode(), value()
 */
QwtGridRasterData::ResampleMode QwtGridRasterData::resampleMode() const
{
    return PrivateData::resampleModeCast(m_data->gridData.resampleMode());
}

QwtInterval QwtGridRasterData::interval(Qt::Axis axis) const
{
    switch (axis) {
    case Qt::XAxis:
        return QwtInterval(m_data->gridData.xMin(), m_data->gridData.xMax());
    case Qt::YAxis:
        return QwtInterval(m_data->gridData.yMin(), m_data->gridData.yMax());
    case Qt::ZAxis:
        return QwtInterval(m_data->gridData.dataMin(), m_data->gridData.dataMax());
    default:
        break;
    }
    return QwtInterval(qQNaN(), qQNaN());
}

void QwtGridRasterData::setValue(const QVector< double >& x, const QVector< double >& y, const QVector< QVector< double > >& v)
{
    m_data->gridData.setValue(x, y, v);
    const QVector< double >& sortedX = m_data->gridData.xAxis();
    const QVector< double >& sortedY = m_data->gridData.yAxis();
    // 计算dxmin和dymin
    QVector< double > dx, dy;
    dx.reserve(x.size());
    dy.reserve(y.size());
    for (QVector< double >::size_type i = 1; i < sortedX.size(); ++i) {
        double delta = sortedX[ i ] - sortedX[ i - 1 ];
        dx.push_back(delta);
    }
    for (QVector< double >::size_type i = 1; i < sortedY.size(); ++i) {
        double delta = sortedY[ i ] - sortedY[ i - 1 ];
        dy.push_back(delta);
    }
    if (dx.empty()) {
        m_data->dxMin = 0;
    } else {
        m_data->dxMin = *std::min_element(dx.begin(), dx.end());
    }

    if (dy.empty()) {
        m_data->dyMin = 0;
    } else {
        m_data->dyMin = *std::max_element(dy.begin(), dy.end());
    }
}

double QwtGridRasterData::value(double x, double y) const
{
    return m_data->gridData.value(x, y);
}

QRectF QwtGridRasterData::pixelHint(const QRectF& area) const
{
    Q_UNUSED(area)

    QRectF rect;
    if (resampleMode() == NearestNeighbour) {
        const QwtInterval intervalX = interval(Qt::XAxis);
        const QwtInterval intervalY = interval(Qt::YAxis);
        if (intervalX.isValid() && intervalY.isValid()) {
            rect = QRectF(intervalX.minValue(), intervalY.minValue(), m_data->dxMin, m_data->dyMin);
        }
    }

    return rect;
}
