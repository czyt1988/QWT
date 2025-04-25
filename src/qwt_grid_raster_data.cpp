#include "qwt_grid_raster_data.h"
#include "qwt_interval.h"
#include <QVector>

class QwtGridRasterData::PrivateData
{
public:
    PrivateData() : resampleMode(QwtGridRasterData::NearestNeighbour)
    {
    }

public:
    QwtGridRasterData::ResampleMode resampleMode;
    QwtInterval intervals[ 3 ];
};

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
    m_data->resampleMode = mode;
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
    return m_data->resampleMode;
}

/**
 * @brief Assign the bounding interval for an axis
 * @param axis
 * @param interval
 */
void QwtGridRasterData::setInterval(Qt::Axis axis, const QwtInterval& interval)
{
    if (axis >= 0 && axis <= 2) {
        m_data->intervals[ axis ] = interval;
        update();
    }
}

QwtInterval QwtGridRasterData::interval(Qt::Axis axis) const
{
    if (axis >= 0 && axis <= 2)
        return m_data->intervals[ axis ];
}
