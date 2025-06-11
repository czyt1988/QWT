#ifndef QWT_GRID_DATA_HPP
#define QWT_GRID_DATA_HPP
#include <vector>
#include <algorithm>
#include <limits>
#include <cmath>
#include <cassert>
#include <stdexcept>

/**
 * @class QwtGridData
 *
 * @brief A generic container class for storing 2D grid data and providing resampling methods.
 *
 * @details This template class encapsulates a 2D table with x-axis and y-axis data.
 * It supports three resampling methods: Nearest Neighbor, Bilinear Interpolation, and Bicubic Interpolation.
 * The container types for the axes and data are templated to allow flexibility.
 *
 * 此模板类封装了一个二维表格以及对应的 x 轴和 y 轴数据。
 * 它支持三种插值方法：最近邻插值、双线性插值和双三次插值。
 * 轴和数据的容器类型被模板化以提供灵活性。
 *
 * ↓Y轴方向 →X轴方向 n行 m列
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
 * **Example Usage**:
 *
 * Example using `std::vector`:
 * @code
 * std::vector<double> xAxis = {0, 1, 2, 3};
 * std::vector<double> yAxis = {0, 1, 2};
 * std::vector<std::vector<double>> data = {
 *     {1, 2, 3},
 *     {4, 5, 6},
 *     {7, 8, 9},
 *     {10, 11, 12}
 * };
 *
 * // Create an instance with bilinear interpolation
 * QwtGridData<double> gridData(xAxis, yAxis, data, QwtGridData<double>::BilinearInterpolation);
 *
 * // Query the value at (1.5, 1.5)
 * double value = gridData(1.5, 1.5);
 * std::cout << "Value at (1.5, 1.5): " << value << std::endl;
 * @endcode
 *
 * Example using `QVector`:
 * @code
 * QVector<double> xAxis = {0, 1, 2, 3};
 * QVector<double> yAxis = {0, 1, 2};
 * QVector<QVector<double>> data = {
 *     {1, 2, 3},
 *     {4, 5, 6},
 *     {7, 8, 9},
 *     {10, 11, 12}
 * };
 *
 * // Create an instance with bicubic interpolation
 * QwtGridData<double, QVector> gridData(xAxis, yAxis, data, QwtGridData<double, QVector>::BicubicInterpolation);
 *
 * // Query the value at (1.5, 1.5)
 * double value = gridData(1.5, 1.5);
 * qDebug() << "Value at (1.5, 1.5):" << value;
 * @endcode
 *
 * @tparam T The type of the values stored in the grid.
 * @tparam XContainer The container type for the x-axis values (default: std::vector).
 * @tparam YContainer The container type for the y-axis values (default: same as XContainer).
 * @tparam DataColumn The container type for a single column in the data matrix (default: same as XContainer).
 * @tparam DataContainer The container type for the entire data matrix (default: same as XContainer).
 */
template< typename T,
          template< typename > class XContainer    = std::vector,
          template< typename > class YContainer    = XContainer,
          template< typename > class DataColumn    = XContainer,
          template< typename > class DataContainer = XContainer >
class QwtGridData
{
public:
    using value_type       = T;                ///< @brief Type of the stored values / 存储值的类型
    using x_container_type = XContainer< T >;  ///< @brief Type of the x-axis container / x 轴容器的类型
    using y_container_type = YContainer< T >;  ///< @brief Type of the y-axis container / y 轴容器的类型
    using data_column_type = DataColumn< T >;  ///< @brief Type of a single column in the data matrix / 数据矩阵中单列的类型
    using data_container_type = DataContainer< data_column_type >;  ///< @brief Type of the data matrix / 数据矩阵的类型
    using size_type = typename DataColumn< T >::size_type;
    /**
     * @brief Enumeration for resampling methods.
     *
     * Defines the available resampling methods:
     * - NearestNeighbour: Nearest neighbor interpolation.
     * - BilinearInterpolation: Bilinear interpolation.
     * - BicubicInterpolation: Hermite bicubic interpolation.
     *
     * 定义可用的抽样方法：
     * - NearestNeighbour: 最近邻插值。
     * - BilinearInterpolation: 双线性插值。
     * - BicubicInterpolation: Hermite 双三次插值。
     */
    enum ResampleMode
    {
        NearestNeighbour,
        BilinearInterpolation,
        BicubicInterpolation
    };

    /**
     * @brief Default constructor.
     *
     * 初始化一个空的 QwtGridData 对象。
     */
    QwtGridData()
        : m_mode(NearestNeighbour), m_xMin(0.0), m_xMax(0.0), m_yMin(0.0), m_yMax(0.0), m_dataMax(0.0), m_dataMin(0.0)
    {
    }

    /**
     * @brief Constructor with initial data.
     *
     * Initializes the object with x-axis, y-axis, and data matrix.
     *
     * 使用 x 轴、y 轴和数据矩阵初始化对象。
     *
     * @param xAxis The x-axis values. / x 轴值。
     * @param yAxis The y-axis values. / y 轴值。
     * @param data The 2D data matrix. / 二维数据矩阵。
     * @param mode The resampling mode to use. / 要使用的抽样方法。
     */
    QwtGridData(const XContainer< T >& xAxis,
                const YContainer< T >& yAxis,
                const DataContainer< DataColumn< T > >& data,
                ResampleMode mode = NearestNeighbour)
        : m_xAxis(xAxis), m_yAxis(yAxis), m_data(data), m_mode(mode)
    {
        validate();
        findValueRange();
        m_xMin = xAxis.front();
        m_xMax = xAxis.back();
        m_yMin = yAxis.front();
        m_yMax = yAxis.back();
    }

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
    void setValue(const XContainer< T >& xAxis, const YContainer< T >& yAxis, const DataContainer< DataColumn< T > >& data)
    {
        m_xAxis = xAxis;
        m_yAxis = yAxis;
        m_data  = data;
        validate();
        findValueRange();
        m_xMin = xAxis.front();
        m_xMax = xAxis.back();
        m_yMin = yAxis.front();
        m_yMax = yAxis.back();
    }

    /**
     * @brief Operator to query value at (x, y).
     *
     * 根据给定的 x 和 y 坐标查询值。
     *
     * @param x The x-coordinate. / x 坐标。
     * @param y The y-coordinate. / y 坐标。
     * @return The interpolated or nearest value. / 插值或最近邻值。
     */
    T operator()(T x, T y) const
    {
        return value(x, y);
    }

    /**
     * @brief operator []
     *
     * 根据给定的 x 和 y 坐标查询值。
     *
     * @param xy,std::pair<x,y>
     * @return The interpolated or nearest value. / 插值或最近邻值。
     */
    T operator[](const std::pair< T, T >& xy) const
    {
        return value(xy.first, xy.second);
    }
    /**
     * @brief Query value at (x, y).
     *
     * 根据给定的 x 和 y 坐标查询值。
     *
     * @param x The x-coordinate. / x 坐标。
     * @param y The y-coordinate. / y 坐标。
     * @return The interpolated or nearest value. / 插值或最近邻值。
     */
    T value(T x, T y) const
    {
        switch (m_mode) {
        case NearestNeighbour:
            return nearestNeighbour(x, y);
        case BilinearInterpolation:
            return bilinearInterpolation(x, y);
        case BicubicInterpolation:
            return bicubicInterpolation(x, y);
        default:
            throw std::runtime_error("Unknown resampling mode.");
        }
    }

    /**
     * @brief Set the resampling mode.
     *
     * 设置查询值时使用的抽样方法。
     *
     * @param mode The resampling mode to use. / 要使用的抽样方法。
     */
    void setResampleMode(ResampleMode mode)
    {
        m_mode = mode;
    }

    /**
     * @brief Get the current resampling mode.
     *
     * 返回当前激活的抽样方法。
     *
     * @return The current resampling mode. / 当前抽样方法。
     */
    ResampleMode resampleMode() const
    {
        return m_mode;
    }

    /**
     * @brief x的尺寸
     * @return
     */
    size_type xSize() const
    {
        return m_xAxis.size();
    }

    /**
     * @brief y的尺寸
     * @return
     */
    size_type ySize() const
    {
        return m_yAxis.size();
    }

    /**
     * @brief value矩阵的尺寸
     * @return <xsize,ysize>
     */
    std::pair< size_type, size_type > valueSize() const
    {
        return std::make_pair(xSize(), ySize());
    }

    /**
     * @brief x 值对应的内容
     * @param ix
     * @return
     */
    T atX(size_type ix) const
    {
        return m_xAxis.at(ix);
    }

    /**
     * @brief y值对应的内容
     * @param ix
     * @return
     */
    T atY(size_type iy) const
    {
        return m_yAxis.at(iy);
    }

    /**
     * @brief value值对应的内容
     * @param ix
     * @return
     */
    T atValue(size_type ix, size_type iy) const
    {
        return m_data.at(ix).at(iy);
    }

    /**
     * @brief Get the x-axis values.
     *
     * 获取 x 轴值。
     *
     * @return The x-axis values. / x 轴值。
     */
    const XContainer< T >& xAxis() const
    {
        return m_xAxis;
    }

    /**
     * @brief Get the y-axis values.
     *
     * 获取 y 轴值。
     *
     * @return The y-axis values. / y 轴值。
     */
    const YContainer< T >& yAxis() const
    {
        return m_yAxis;
    }

    /**
     * @brief Get the data matrix.
     *
     * 获取数据矩阵。
     *
     * @return The data matrix. / 数据矩阵。
     */
    const DataContainer< DataColumn< T > >& data() const
    {
        return m_data;
    }

    /**
     * @brief Check if the object is valid.
     *
     * 判断对象是否有效。
     *
     * @return True if valid, false otherwise. / 如果有效则返回 true，否则返回 false。
     */
    bool valid() const
    {
        if (m_xAxis.empty() || m_yAxis.empty() || m_data.empty()) {
            return false;
        }
        if (m_xAxis.size() != m_data.size()) {
            return false;
        }
        for (const auto& column : m_data) {
            if (column.size() != m_yAxis.size()) {
                return false;
            }
        }
        if (!std::is_sorted(m_xAxis.begin(), m_xAxis.end())) {
            return false;
        }
        if (!std::is_sorted(m_yAxis.begin(), m_yAxis.end())) {
            return false;
        }
        return true;
    }

    /**
     * @brief Validate the data.
     *
     * 验证数据的有效性。
     */
    void validate()
    {
        if (m_xAxis.empty() || m_yAxis.empty() || m_data.empty()) {
            throw std::invalid_argument("Axes or data cannot be empty.");
        }
        if (m_data.size() != m_xAxis.size()) {
            throw std::invalid_argument("Number of columns in data must match x-axis size.");
        }
        for (const auto& column : m_data) {
            if (column.size() != m_yAxis.size()) {
                throw std::invalid_argument("Number of rows in data must match y-axis size.");
            }
        }
        if (!std::is_sorted(m_xAxis.begin(), m_xAxis.end())) {
            std::sort(m_xAxis.begin(), m_xAxis.end());
        }
        if (!std::is_sorted(m_yAxis.begin(), m_yAxis.end())) {
            std::sort(m_yAxis.begin(), m_yAxis.end());
        }
    }

    T xMin() const
    {
        return m_xMin;
    }
    T xMax() const
    {
        return m_xMax;
    }
    T yMin() const
    {
        return m_yMin;
    }
    T yMax() const
    {
        return m_yMax;
    }
    T dataMin() const
    {
        return m_dataMin;
    }
    T dataMax() const
    {
        return m_dataMax;
    }

public:
    // static pulic function
    /**
     * @brief Find the closest index in a sorted array.
     *
     * 在排序数组中查找最接近的索引。
     *
     * @param arr The sorted array. / 排序数组。
     * @param val The target value. / 目标值。
     * @return The index of the closest value. / 最接近值的索引。
     */
    template< typename Container >
    static size_type findClosestIndex(const Container& arr, T val)
    {
        auto it = std::lower_bound(arr.begin(), arr.end(), val);
        if (it == arr.begin())
            return 0;
        if (it == arr.end())
            return arr.size() - 1;
        size_type idx = std::distance(arr.begin(), it);
        return (std::abs(arr[ idx ] - val) < std::abs(arr[ idx - 1 ] - val)) ? idx : idx - 1;
    }

    /**
     * @brief Find the lower bound index in a sorted array.
     *
     * 在排序数组中查找下界索引。
     *
     * @param arr The sorted array. / 排序数组。
     * @param val The target value. / 目标值。
     * @return The lower bound index. / 下界索引。
     */
    template< typename Container >
    static size_type findLowerIndex(const Container& arr, T val)
    {
        auto it = std::lower_bound(arr.begin(), arr.end(), val);
        if (it == arr.begin())
            return 0;
        return std::distance(arr.begin(), it) - 1;
    }

    template< typename V >
    static const V& clamp(const V& value, const V& lo, const V& hi)
    {
        return (value < lo) ? lo : (hi < value) ? hi : value;
    }

protected:
    /**
     * @brief Get the minimum & maximum value in the data matrix.
     *
     * 查找并返回二维数据矩阵中的最小和最大值。
     *
     * @return std::pair<The minimum value, The maximum value> in the data matrix. / 数据矩阵中的最小和最大值。
     */
    void findValueRange()
    {
        m_dataMin = std::numeric_limits< T >::max();
        m_dataMax = std::numeric_limits< T >::lowest();
        for (const auto& column : m_data) {
            for (const auto& val : column) {
                m_dataMin = std::min(m_dataMin, val);
                m_dataMax = std::max(m_dataMax, val);
            }
        }
    }

    /**
     * @brief Nearest neighbor interpolation.
     *
     * 最近邻插值。
     *
     * @param x The x-coordinate. / x 坐标。
     * @param y The y-coordinate. / y 坐标。
     * @return The nearest value. / 最近邻值。
     */
    T nearestNeighbour(T x, T y) const
    {
        size_type xIdx = findClosestIndex(m_xAxis, x);
        size_type yIdx = findClosestIndex(m_yAxis, y);
        return m_data[ xIdx ][ yIdx ];
    }

    /**
     * @brief Bilinear interpolation.
     *
     * 双线性插值。
     *
     * @param x The x-coordinate. / x 坐标。
     * @param y The y-coordinate. / y 坐标。
     * @return The interpolated value. / 插值结果。
     */
    T bilinearInterpolation(T x, T y) const
    {
        size_type x0Idx = findLowerIndex(m_xAxis, x);
        size_type x1Idx = x0Idx + 1;
        size_type y0Idx = findLowerIndex(m_yAxis, y);
        size_type y1Idx = y0Idx + 1;

        T x0 = m_xAxis[ x0Idx ], x1 = m_xAxis[ x1Idx ];
        T y0 = m_yAxis[ y0Idx ], y1 = m_yAxis[ y1Idx ];

        T f00 = m_data[ x0Idx ][ y0Idx ];
        T f10 = m_data[ x1Idx ][ y0Idx ];
        T f01 = m_data[ x0Idx ][ y1Idx ];
        T f11 = m_data[ x1Idx ][ y1Idx ];

        T dx = (x - x0) / (x1 - x0);
        T dy = (y - y0) / (y1 - y0);

        return (1 - dx) * (1 - dy) * f00 + dx * (1 - dy) * f10 + (1 - dx) * dy * f01 + dx * dy * f11;
    }

    /**
     * @brief Perform bicubic interpolation.
     *
     * @details Returns the bicubically interpolated value at the specified (x, y) position.
     * This implementation uses the Hermite bicubic interpolation method.
     *
     * 返回指定 (x, y) 位置的双三次插值结果。
     * 此实现使用 Hermite 双三次插值方法。
     *
     * @param x The x-coordinate / x 坐标
     * @param y The y-coordinate / y 坐标
     * @return The bicubically interpolated value / 双三次插值结果
     */
    T bicubicInterpolation(T x, T y) const
    {
        // Find surrounding x indices
        auto xIt = std::lower_bound(m_xAxis.begin(), m_xAxis.end(), x);
        size_type x0, x1, x2, x3;
        T xWeight;

        if (xIt == m_xAxis.begin()) {
            x0      = 0;
            x1      = 0;
            x2      = 1;
            x3      = 2;
            xWeight = 0.0;
        } else if (xIt == m_xAxis.end()) {
            x0      = m_xAxis.size() - 3;
            x1      = m_xAxis.size() - 2;
            x2      = m_xAxis.size() - 1;
            x3      = m_xAxis.size() - 1;
            xWeight = 1.0;
        } else {
            x0 = xIt - 2 - m_xAxis.begin();
            if (x0 < 0)
                x0 = 0;
            x1 = xIt - 1 - m_xAxis.begin();
            x2 = xIt - m_xAxis.begin();
            x3 = xIt + 1 - m_xAxis.begin();
            if (x3 >= m_xAxis.size())
                x3 = m_xAxis.size() - 1;
            xWeight = static_cast< T >(x - m_xAxis[ x1 ]) / (m_xAxis[ x2 ] - m_xAxis[ x1 ]);
        }

        // Find surrounding y indices
        auto yIt = std::lower_bound(m_yAxis.begin(), m_yAxis.end(), y);
        size_type y0, y1, y2, y3;
        T yWeight;

        if (yIt == m_yAxis.begin()) {
            y0      = 0;
            y1      = 0;
            y2      = 1;
            y3      = 2;
            yWeight = 0.0;
        } else if (yIt == m_yAxis.end()) {
            y0      = m_yAxis.size() - 3;
            y1      = m_yAxis.size() - 2;
            y2      = m_yAxis.size() - 1;
            y3      = m_yAxis.size() - 1;
            yWeight = 1.0;
        } else {
            y0 = yIt - 2 - m_yAxis.begin();
            if (y0 < 0)
                y0 = 0;
            y1 = yIt - 1 - m_yAxis.begin();
            y2 = yIt - m_yAxis.begin();
            y3 = yIt + 1 - m_yAxis.begin();
            if (y3 >= m_yAxis.size())
                y3 = m_yAxis.size() - 1;
            yWeight = static_cast< T >(y - m_yAxis[ y1 ]) / (m_yAxis[ y2 ] - m_yAxis[ y1 ]);
        }

        // Hermite basis functions
        auto h00 = [](T t) { return (1 + 2 * t) * (1 - t) * (1 - t); };
        auto h10 = [](T t) { return t * (1 - t) * (1 - t); };
        auto h01 = [](T t) { return t * t * (3 - 2 * t); };
        auto h11 = [](T t) { return t * t * (t - 1); };

        // Interpolate in x direction for each y position
        T values[ 4 ][ 4 ];
        for (int i = 0; i < 4; ++i) {
            // Get the four y values for current x positions
            T v[ 4 ];
            v[ 0 ] = m_data[ x0 ][ y0 + i ];
            v[ 1 ] = m_data[ x1 ][ y0 + i ];
            v[ 2 ] = m_data[ x2 ][ y0 + i ];
            v[ 3 ] = m_data[ x3 ][ y0 + i ];

            // Interpolate in x direction
            values[ 0 ][ i ] = h00(xWeight) * v[ 1 ]
                               + h10(xWeight) * (m_xAxis[ x2 ] - m_xAxis[ x1 ])
                                     * ((v[ 2 ] - v[ 1 ]) / (m_xAxis[ x2 ] - m_xAxis[ x1 ])
                                        + (v[ 2 ] - v[ 1 ] - (v[ 1 ] - v[ 0 ]) / (m_xAxis[ x1 ] - m_xAxis[ x0 ]))
                                              / (m_xAxis[ x2 ] - m_xAxis[ x0 ]) * (m_xAxis[ x2 ] - m_xAxis[ x1 ]))
                               + h01(xWeight) * v[ 2 ]
                               + h11(xWeight) * (m_xAxis[ x2 ] - m_xAxis[ x1 ])
                                     * ((v[ 2 ] - v[ 1 ]) / (m_xAxis[ x2 ] - m_xAxis[ x1 ])
                                        + (v[ 3 ] - v[ 2 ] - (v[ 2 ] - v[ 1 ]) / (m_xAxis[ x2 ] - m_xAxis[ x1 ]))
                                              / (m_xAxis[ x3 ] - m_xAxis[ x1 ]) * (m_xAxis[ x2 ] - m_xAxis[ x1 ]));
        }

        // Interpolate in y direction
        T v[ 4 ];
        v[ 0 ] = values[ 0 ][ 0 ];
        v[ 1 ] = values[ 0 ][ 1 ];
        v[ 2 ] = values[ 0 ][ 2 ];
        v[ 3 ] = values[ 0 ][ 3 ];

        return h00(yWeight) * v[ 1 ]
               + h10(yWeight) * (m_yAxis[ y2 ] - m_yAxis[ y1 ])
                     * ((v[ 2 ] - v[ 1 ]) / (m_yAxis[ y2 ] - m_yAxis[ y1 ])
                        + (v[ 2 ] - v[ 1 ] - (v[ 1 ] - v[ 0 ]) / (m_yAxis[ y1 ] - m_yAxis[ y0 ]))
                              / (m_yAxis[ y2 ] - m_yAxis[ y0 ]) * (m_yAxis[ y2 ] - m_yAxis[ y1 ]))
               + h01(yWeight) * v[ 2 ]
               + h11(yWeight) * (m_yAxis[ y2 ] - m_yAxis[ y1 ])
                     * ((v[ 2 ] - v[ 1 ]) / (m_yAxis[ y2 ] - m_yAxis[ y1 ])
                        + (v[ 3 ] - v[ 2 ] - (v[ 2 ] - v[ 1 ]) / (m_yAxis[ y2 ] - m_yAxis[ y1 ]))
                              / (m_yAxis[ y3 ] - m_yAxis[ y1 ]) * (m_yAxis[ y2 ] - m_yAxis[ y1 ]));
    }

private:
    XContainer< T > m_xAxis;  ///< @brief x-axis values / x 轴值
    YContainer< T > m_yAxis;  ///< @brief y-axis values / y 轴值
    /**
     * @brief The 2D data matrix / 二维数据矩阵
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
     */
    DataContainer< data_column_type > m_data;
    ResampleMode m_mode;                                     ///< @brief Current resampling mode / 当前抽样方法
    T m_xMin, m_xMax, m_yMin, m_yMax, m_dataMin, m_dataMax;  ///< @brief Bounds of the grid / 网格的边界
};

#endif  // QWT_GRID_DATA_HPP
