/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "qwt_plot_series_data_picker.h"
// stl
#include <algorithm>
#include <limits>
// qwt
#include "qwt_utils.h"
#include "qwt_picker_machine.h"
#include "qwt_plot.h"
#include "qwt_plot_item.h"
#include "qwt_plot_curve.h"
#include "qwt_scale_map.h"
#include "qwt_painter.h"
#include "qwt_scale_draw.h"
// qt
#include <QPainter>
#include <QtMath>
#include <QDebug>
#include <QHash>

// 是否对x值进行分组，分组会根据item归属不同的x进行区分组别来显示，分组主要针对多x轴的情况
// 目前分组功能在qt5正常，但qt6异常，异常原因暂不明确
#ifndef QwtPlotSeriesDataPicker_XGroup
#define QwtPlotSeriesDataPicker_XGroup 0
#endif
/**
 * @brief 计算在曲线数据中搜索最近点的窗口范围
 *
 * 该函数通过二分查找快速定位目标X坐标在曲线数据中的大致位置，然后根据窗口大小设置
 * 搜索的起始和结束索引。对于小数据量曲线（少于1000点），直接搜索整个范围；对于大
 * 数据量，使用窗口优化以提高性能。
 *
 * @param[in] curveSize 曲线数据点的总数
 * @param[in] targetX 目标X坐标（数据坐标系）
 * @param[in] data 曲线数据序列，必须按X坐标升序排列
 * @param[in] windowSize 窗口大小设置
 *        - 0: 不使用窗口，搜索整个曲线
 *        - 正数: 固定的窗口大小（数据点数量）
 *        - 负数: 自适应窗口，使用曲线数据点总数的百分比（取绝对值，如-5表示5%）
 * @return pair<startIndex,endIndex>, startIndex:计算出的搜索起始索引（包含）;endIndex:计算出的搜索结束索引（包含）
 *
 * @note 当曲线数据量小于1000点时，自动禁用窗口优化，搜索整个曲线以获得最佳精度
 * @note 百分比计算：windowSize = -5 表示使用曲线点数的5%作为窗口大小
 * @note 函数假设曲线数据已经按X坐标升序排列
 * @note 自适应窗口大小有最小50点和最大1000点的限制，避免窗口过小或过大
 * @note 如果计算出的窗口包含80%以上的数据点，会自动退化为搜索整个曲线
 *
 * @par 性能策略：
 * - 数据点 < 1000: 搜索整个曲线（线性搜索开销可接受）
 * - 数据点 ≥ 1000: 使用窗口优化（显著减少比较次数）
 *
 * @see qwtUpperSampleIndex
 * @see QwtSeriesData
 * @see QwtPlotSeriesDataPicker::pickNearestPoint
 */
QPair< size_t, size_t > calculateSearchWindow(
    size_t curveSize, double targetX, const QwtSeriesData< QPointF >& data, int windowSize = -5
)
{
    // 初始化默认范围：整个曲线
    size_t startIndex;
    size_t endIndex;
    startIndex = 0;
    endIndex   = (curveSize > 0) ? curveSize - 1 : 0;

    // 定义性能阈值：小于此值的数据集不使用窗口优化
    const size_t WINDOW_OPTIMIZATION_THRESHOLD = 1000;

    // 如果曲线数据量小，或者明确不使用窗口，则搜索整个范围
    if (curveSize <= 1 || windowSize == 0 || curveSize < WINDOW_OPTIMIZATION_THRESHOLD) {
        return qMakePair(startIndex, endIndex);
    }

    // 计算实际窗口大小
    size_t realWindowSize;
    if (windowSize < 0) {
        // 自适应模式：使用曲线点数的百分比
        // windowSize = -5 表示 5%，windowSize = -10 表示 10%
        double percentage = std::abs(windowSize) / 100.0;
        realWindowSize    = static_cast< size_t >(curveSize * percentage);

        // 确保自适应窗口在合理范围内
        const size_t MIN_ADAPTIVE_WINDOW = 50;
        const size_t MAX_ADAPTIVE_WINDOW = 1000;
        realWindowSize                   = std::max(realWindowSize, MIN_ADAPTIVE_WINDOW);
        realWindowSize                   = std::min(realWindowSize, MAX_ADAPTIVE_WINDOW);
    } else {
        // 固定窗口大小
        realWindowSize = static_cast< size_t >(windowSize);
    }

    // 确保窗口大小在有效范围内
    realWindowSize = std::max< size_t >(1, realWindowSize);
    realWindowSize = std::min< size_t >(realWindowSize, curveSize);

    // 使用二分查找定位目标X坐标的大致位置
    size_t centerIndex = qwtUpperSampleIndex< QPointF >(data, targetX, [](const double x, const QPointF& point) -> bool {
        return (x < point.x());
    });

    // 根据中心位置计算窗口边界
    if (centerIndex == curveSize) {
        // 情况1：目标X大于所有数据点，在曲线右侧
        // 窗口设置在曲线末尾
        if (realWindowSize < curveSize) {
            startIndex = curveSize - realWindowSize;
        }
        // endIndex 已经设置为 curveSize - 1
    } else if (centerIndex == 0) {
        // 情况2：目标X小于等于第一个数据点，在曲线左侧
        // 窗口设置在曲线开头
        endIndex = std::min(realWindowSize - 1, curveSize - 1);
    } else {
        // 情况3：目标X在曲线数据范围内
        // 以centerIndex为中心设置窗口

        // 计算窗口半宽
        size_t halfWindow = realWindowSize / 2;

        // 计算起始索引，确保不小于0
        if (centerIndex > halfWindow) {
            startIndex = centerIndex - halfWindow;
        } else {
            startIndex = 0;
        }

        // 计算结束索引，确保不超过曲线末尾
        endIndex = centerIndex + halfWindow;
        if (endIndex >= curveSize) {
            endIndex = curveSize - 1;
            // 如果结束索引被调整，相应调整起始索引以保持窗口大小
            if (endIndex - startIndex + 1 > realWindowSize) {
                startIndex = endIndex - realWindowSize + 1;
            }
        } else {
            // 如果窗口大小是奇数，调整结束索引以保持精确的窗口大小
            if (realWindowSize % 2 == 1 && endIndex - startIndex + 1 < realWindowSize) {
                endIndex = startIndex + realWindowSize - 1;
                if (endIndex >= curveSize) {
                    endIndex = curveSize - 1;
                }
            }
        }
    }

    // 最终边界检查，确保索引在有效范围内
    startIndex = std::min(startIndex, curveSize - 1);
    endIndex   = std::min(endIndex, curveSize - 1);

    // 确保起始索引不大于结束索引
    if (startIndex > endIndex) {
        std::swap(startIndex, endIndex);
    }

    // 最终验证窗口大小
    // size_t actualWindowSize = endIndex - startIndex + 1;
    // 如果窗口实际上包含了大部分数据，不如搜索整个曲线
    // const double FULL_SEARCH_THRESHOLD = 0.8;  // 80%
    // if (actualWindowSize >= curveSize * FULL_SEARCH_THRESHOLD) {
    //     startIndex = 0;
    //     endIndex   = curveSize - 1;
    // }
    return qMakePair(startIndex, endIndex);
}

class QwtPlotSeriesDataPicker::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotSeriesDataPicker)
public:
    PrivateData(QwtPlotSeriesDataPicker* p);
#if QwtPlotSeriesDataPicker_XGroup
    struct GroupKey
    {
        QwtPlot* plot { nullptr };
        QwtAxisId axis { QwtAxis::XBottom };
        // 获取有效的 plot（如果是寄生轴且共享轴，返回宿主 plot）
        QwtPlot* effectivePlot() const
        {
            if (plot && plot->isParasitePlot() && plot->isParasiteShareAxis(axis)) {
                QwtPlot* host = plot->hostPlot();
                if (host)
                    return host;
            }
            return plot;
        }

        bool operator==(const GroupKey& o) const
        {
            if (plot == o.plot && axis == o.axis) {
                return true;
            }

            // 处理寄生轴的情况
            QwtPlot* effPlot1 = effectivePlot();
            QwtPlot* effPlot2 = o.effectivePlot();

            return (effPlot1 == effPlot2) && (axis == o.axis);
        }

        // 为 QHash 提供哈希函数
        friend inline size_t qHash(const GroupKey& key, uint seed = 0)
        {
            // 使用 qHash 来哈希指针和整数
            auto h1 = qHash(reinterpret_cast< quintptr >(key.effectivePlot()), seed);
            auto h2 = qHash(key.axis, seed);

            // 组合哈希值
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };
    struct XGroup
    {
        GroupKey key;
        QString xValue;  // 公共 X 值
        QList< const FeaturePoint* > fps;
    };
    QVector< XGroup > xGroups;
#endif
public:
    QwtPlotSeriesDataPicker::PickSeriesMode pickMode { QwtPlotSeriesDataPicker::PickYValue };
    QwtPlotSeriesDataPicker::TextPlacement textArea { QwtPlotSeriesDataPicker::TextPlaceAuto };
    QwtPlotSeriesDataPicker::InterpolationMode interpolationMode { QwtPlotSeriesDataPicker::LinearInterpolation };
    // 渲染相关
    QBrush textBackgroundBrush { QColor(255, 255, 255, 180) };
    Qt::Alignment textAlignment { Qt::AlignLeft | Qt::AlignVCenter };
    // 记录找到的特征点
    int nearestSearchWindowSize { -5 };
    QList< FeaturePoint > featurePoints;
    int featurePointSize { 4 };      ///< 特征点的大小
    bool markFeaturePoint { true };  ///< 是否标记捕获的特征点
    QPoint mousePos;
    bool enableShowXOnPicker { true };
    QPoint textTrackerOffset { 15, 0 };  ///< 记录文字tracker的偏移，这个参数在TextFollowMouse的模式下生效（设置此参数可以避免文本紧贴鼠标位置）
};

QwtPlotSeriesDataPicker::PrivateData::PrivateData(QwtPlotSeriesDataPicker* p) : q_ptr(p)
{
}

//===============================================================
// QwtPlotSeriesDataPicker
//===============================================================

QwtPlotSeriesDataPicker::QwtPlotSeriesDataPicker(QWidget* canvas) : QwtCanvasPicker(canvas), QWT_PIMPL_CONSTRUCT
{
    // 设置追踪模式，始终显示追踪信息
    setTrackerMode(QwtPicker::ActiveOnly);
    // 设置橡皮筋为垂直线
    setRubberBand(QwtPicker::UserRubberBand);
    // 设置状态机，用于点选择
    setStateMachine(new QwtPickerTrackerMachine);
    //
    QwtPlot* host = plot();
    if (host->isParasitePlot()) {
        host = host->hostPlot();
        if (!host) {
            host = plot();
        }
    }
    QList< QwtPlot* > allPlots = host->plotList();
    for (const QwtPlot* p : allPlots) {
        connect(p, &QwtPlot::itemAttached, this, &QwtPlotSeriesDataPicker::onPlotItemDetached);
    }
    connect(host, &QwtPlot::parasitePlotAttached, this, &QwtPlotSeriesDataPicker::onParasitePlotAttached);
}

QwtPlotSeriesDataPicker::~QwtPlotSeriesDataPicker()
{
}

/**
 * @brief 设置拾取模式
 * @param mode 拾取模式
 */
void QwtPlotSeriesDataPicker::setPickMode(PickSeriesMode mode)

{
    QWT_D(d);
    if (mode == d->pickMode) {
        return;
    }
    d->pickMode = mode;
}

/**
 * @brief 获取当前的拾取模式
 * @return
 */
QwtPlotSeriesDataPicker::PickSeriesMode QwtPlotSeriesDataPicker::pickMode() const
{
    return m_data->pickMode;
}

/**
 * @brief 设置文字显示的区域
 * @param t
 */
void QwtPlotSeriesDataPicker::setTextArea(QwtPlotSeriesDataPicker::TextPlacement t)
{
    m_data->textArea = t;
}

/**
 * @brief 文字显示的位置
 * @return
 * @sa QwtPlotSeriesDataPicker::TextPlacement
 */
QwtPlotSeriesDataPicker::TextPlacement QwtPlotSeriesDataPicker::textArea() const
{
    return m_data->textArea;
}

/**
 * @brief 设置插值模式
 * @param mode 插值模式
 */
void QwtPlotSeriesDataPicker::setInterpolationMode(QwtPlotSeriesDataPicker::InterpolationMode mode)
{
    m_data->interpolationMode = mode;
}

/**
 * @brief 获取插值模式
 * @return 当前的插值模式
 */
QwtPlotSeriesDataPicker::InterpolationMode QwtPlotSeriesDataPicker::interpolationMode() const
{
    return m_data->interpolationMode;
}

/**
 * @brief 判断是否进行插值
 *
 * 如果插值，那么在鼠标不在对应点上时，会插值找到对应的连接线上的点
 * @return
 */
bool QwtPlotSeriesDataPicker::isInterpolation() const
{
    return m_data->interpolationMode != NoInterpolation;
}

/**
 * @brief 临近点搜索窗口大小
 *
 * 窗口大小决定了临近点搜索的范围，避免全曲线遍历
 *
 * 窗口尺寸可以设置为负值，负值将是以曲线点数的百分比进行窗口设置：
 * - 0: 不使用窗口，搜索整个曲线
 * - 正数: 固定的窗口大小（数据点数量）
 * - 负数: 自适应窗口，使用曲线数据点总数的百分比（取绝对值，如-5表示5%）
 *
 * @param windowSize 窗口尺寸
 *
 * 此属性默认为-5
 */
void QwtPlotSeriesDataPicker::setNearestSearchWindowSize(int windowSize)
{
    m_data->nearestSearchWindowSize = windowSize;
}

/**
 * @brief 临近点搜索窗口大小
 * @return 此尺寸会返回负数，具体可见@ref setNearestSearchWindowSize
 * @sa setNearestSearchWindowSize
 */
int QwtPlotSeriesDataPicker::nearestSearchWindowSize() const
{
    return m_data->nearestSearchWindowSize;
}

/**
 * @brief 设置是否绘制特征点
 * @param on
 */
void QwtPlotSeriesDataPicker::setEnableDrawFeaturePoint(bool on)
{
    m_data->markFeaturePoint = on;
}

/**
 * @brief 是否绘制特征点
 * @return
 */
bool QwtPlotSeriesDataPicker::isEnableDrawFeaturePoint() const
{
    return m_data->markFeaturePoint;
}

/**
 * @brief 设置绘制的特征点的大小
 * @param px
 */
void QwtPlotSeriesDataPicker::setDrawFeaturePointSize(int px)
{
    m_data->featurePointSize = px;
}

/**
 * @brief 设置绘制的特征点的大小
 * @return
 */
int QwtPlotSeriesDataPicker::drawFeaturePointSize() const
{
    return m_data->featurePointSize;
}

/**
 * @brief 设置文本区域的背景颜色
 * @param br
 */
void QwtPlotSeriesDataPicker::setTextBackgroundBrush(const QBrush& br)
{
    m_data->textBackgroundBrush = br;
}

/**
 * @brief 文本区域的背景颜色
 * @return
 */
QBrush QwtPlotSeriesDataPicker::textBackgroundBrush() const
{
    return m_data->textBackgroundBrush;
}

/**
 * @brief 设置文字的对其方式
 * @param al
 */
void QwtPlotSeriesDataPicker::setTextAlignment(Qt::Alignment al)
{
    m_data->textAlignment = al;
}

/**
 * @brief 文字的对其方式
 * @return
 */
Qt::Alignment QwtPlotSeriesDataPicker::textAlignment() const
{
    return m_data->textAlignment;
}

/**
 * @brief 设置是否显示x值
 * @param on
 */
void QwtPlotSeriesDataPicker::setEnableShowXValue(bool on)
{
    m_data->enableShowXOnPicker = on;
}

/**
 * @brief 是否显示x值
 * @return
 */
bool QwtPlotSeriesDataPicker::isEnableShowXValue() const
{
    return m_data->enableShowXOnPicker;
}

/**
 * \if ENGLISH
 * @brief Set the offset of tracker rectangle in TextFollowMouse mode
 *
 * This method configures the positional offset for the tracker rectangle when operating
 * in TextFollowMouse mode. The offset prevents the rectangle from being positioned
 * directly adjacent to the mouse cursor, which enhances visual clarity and prevents
 * the tracker from obscuring the text content beneath the cursor.
 *
 * @param offset The offset value in pixels. Positive values move the tracker away
 *               from the cursor position. Recommended values are typically between
 *               10-30 pixels for optimal user experience.
 * @note The offset is applied relative to the current mouse position.
 * @see textTrackerOffset()
 * @see TextPlacement
 * \endif
 *
 * \if CHINESE
 * @brief 设置文本跟随鼠标模式下追踪矩形的偏移量
 *
 * 此方法用于配置在文本跟随鼠标（TextFollowMouse）模式下，追踪矩形（tracker rectangle）
 * 相对于鼠标位置的位置偏移。通过设置偏移量，可以避免追踪矩形紧贴鼠标光标，从而：
 * 1. 提高视觉清晰度，防止追踪框遮挡光标下方的文本内容
 * 2. 改善用户体验，使文本选择和追踪更加自然流畅
 * 3. 避免因追踪框与鼠标重叠导致的视觉干扰
 *
 * @param offset 偏移量（单位：像素）。正值表示追踪框远离鼠标光标方向移动。
 *               建议偏移量通常在10-30像素之间，以获得最佳用户体验。
 * @note 偏移量是相对于当前鼠标位置进行计算的。
 * @see textTrackerOffset()
 * @see TextPlacement
 * \endif
 */
void QwtPlotSeriesDataPicker::setTextTrackerOffset(const QPoint& offset)
{
    QWT_D(d);
    d->textTrackerOffset = offset;
}

/**
 * \if ENGLISH
 * @brief Get the current tracker rectangle offset in TextFollowMouse mode
 *
 * This method returns the current offset value used to position the tracker rectangle
 * relative to the mouse cursor in TextFollowMouse mode. The offset ensures that the
 * tracker rectangle is not placed directly under the mouse, preventing visual
 * obstruction of the underlying content.
 *
 * @return The current offset as a QPoint, where x and y represent the horizontal
 *         and vertical offsets in pixels respectively.
 * @note A return value of QPoint(0, 0) indicates no offset is applied.
 * @see setTextTrackerOffset()
 * @see TextPlacement
 * \endif
 *
 * \if CHINESE
 * @brief 获取当前文本跟随鼠标模式下追踪矩形的偏移量
 *
 * 此方法返回当前在文本跟随鼠标（TextFollowMouse）模式下，用于定位追踪矩形
 * 相对于鼠标位置的偏移值。该偏移量确保追踪矩形不会直接位于鼠标下方，
 * 从而避免遮挡光标下方的显示内容。
 *
 * @return 当前偏移量，以 QPoint 形式返回，其中 x 和 y 分别表示水平和垂直方向的像素偏移。
 * @see setTextTrackerOffset()
 * @see TextPlacement
 * \endif
 */
QPoint QwtPlotSeriesDataPicker::textTrackerOffset() const
{
    QWT_DC(d);
    return d->textTrackerOffset;
}

QwtText QwtPlotSeriesDataPicker::trackerText(const QPoint& pos) const
{
    if (!isEnabled()) {
        return QwtText();
    }
    const QwtPlot* currentPlot = plot();
    if (!currentPlot) {
        return QwtText();
    }
    // 如有宿主绘图，也一并查找
    QString text;

    QWT_DC(d);
    if (d->featurePoints.isEmpty()) {
        return QwtText();
    }
    text = valueString(d->featurePoints);
    if (text.isEmpty()) {
        // 回退到默认跟踪器文本
        return QwtPicker::trackerText(pos);
    }

    QwtText trackerText(text);
    trackerText.setRenderFlags(int(d->textAlignment) | Qt::TextWordWrap);
    trackerText.setBackgroundBrush(d->textBackgroundBrush);

    return trackerText;
}

QString QwtPlotSeriesDataPicker::valueString(const QList< FeaturePoint >& fps) const
{
    if (fps.isEmpty()) {
        return {};
    }

    auto fmtX = [ & ](const FeaturePoint& fp) -> QString {
        if (!fp.item) {
            return QString::number(fp.feature.x());
        }
        QwtPlot* p = fp.item->plot();
        if (!p) {
            return QString::number(fp.feature.x());
        }
        return formatAxisValue(fp.feature.x(), fp.item->xAxis(), p);
    };
    auto fmtY = [ & ](const FeaturePoint& fp) -> QString {
        if (!fp.item) {
            return QString::number(fp.feature.y());
        }
        QwtPlot* p = fp.item->plot();
        if (!p) {
            return QString::number(fp.feature.y());
        }
        return formatAxisValue(fp.feature.y(), fp.item->yAxis(), p);
    };

    QString out;

    if (pickMode() == PickYValue) {
#if QwtPlotSeriesDataPicker_XGroup
        QWT_DC(d);
        if (!isEnableShowXValue()) {
            // 不显示X值
            for (int i = 0; i < fps.size(); ++i) {
                if (i > 0)
                    out += "<br/>";
                const FeaturePoint& fp = fps[ i ];
                QString stry           = fmtY(fp);
                out += QString(R"(<font color="%1">■</font>%2:<b>%3</b>)")
                           .arg(Qwt::plotItemColor(fp.item).name(), fp.item->title().text(), stry);
            }
        } else {

            // 显示X值，按X轴分组显示
            for (int ig = 0; ig < d->xGroups.size(); ++ig) {
                const PrivateData::XGroup& g = d->xGroups[ ig ];
                QwtPlot* plot                = g.key.plot;
                if (!plot || g.fps.isEmpty())
                    continue;

                // 检查X轴是否可见
                bool isAxisVisible = false;

                if (g.key.axis == QwtAxis::XTop) {
                    isAxisVisible = plot->isAxisVisible(QwtAxis::XTop);
                } else if (g.key.axis == QwtAxis::XBottom) {
                    isAxisVisible = plot->isAxisVisible(QwtAxis::XBottom);
                } else {
                    // 其他位置的轴，默认视为可见
                    isAxisVisible = true;
                }

                // 如果X轴不可见，则不显示X值
                if (!isAxisVisible) {
                    // 直接显示曲线数据，不带X轴标题
                    for (int il = 0; il < g.fps.size(); ++il) {
                        const FeaturePoint* fp = g.fps[ il ];
                        if (!out.isEmpty())
                            out += "<br/>";
                        QString stry = fmtY(*fp);
                        out += QString(R"(<font color="%1">■</font>%2: <b>%3</b>)")
                                   .arg(Qwt::plotItemColor(fp->item).name(), fp->item->title().text(), stry);
                    }
                } else {
                    // X轴可见，显示分组头
                    if (!out.isEmpty())
                        out += "<br/>";

                    // 获取X轴标题
                    QString axisTitle;
                    if (g.key.axis == QwtAxis::XTop) {
                        axisTitle = plot->axisTitle(QwtAxis::XTop).text();
                    } else if (g.key.axis == QwtAxis::XBottom) {
                        axisTitle = plot->axisTitle(QwtAxis::XBottom).text();
                    }

                    if (axisTitle.isEmpty()) {
                        out += QString("<b>%1</b>").arg(g.xValue);
                    } else {
                        // 显示X轴标题和共享的X值
                        out += QString("<b>%1: %2</b>").arg(axisTitle, g.xValue);
                    }

                    // 添加该组内的所有曲线及其Y值
                    for (int il = 0; il < g.fps.size(); ++il) {
                        const FeaturePoint* fp = g.fps[ il ];
                        out += "<br/>";
                        QString stry = fmtY(*fp);
                        out += QString(R"(<font color="%1">■</font>%2: <b>%3</b>)")
                                   .arg(Qwt::plotItemColor(fp->item).name(), fp->item->title().text(), stry);
                    }
                }
            }

            // 如果没有分组，则回退到非分组显示（备用）
            if (out.isEmpty()) {
                for (int i = 0; i < fps.size(); ++i) {
                    if (i > 0)
                        out += "<br/>";
                    const FeaturePoint& fp = fps[ i ];
                    QString stry           = fmtY(fp);
                    out += QString(R"(<font color="%1">■</font>%2:<b>%3</b>)")
                               .arg(Qwt::plotItemColor(fp.item).name(), fp.item->title().text(), stry);
                }
            }
        }
#else
        if (isEnableShowXValue()) {
            // 显示X值
            const FeaturePoint& fp = fps.first();
            out += fmtX(fp);
            out += "<br/>";
        }
        for (int i = 0; i < fps.size(); ++i) {
            if (i > 0) {
                out += "<br/>";
            }
            const FeaturePoint& fp = fps[ i ];
            QString stry           = fmtY(fp);
            out += QString(R"(<font color="%1">■</font>%2:<b>%3</b>)")
                       .arg(Qwt::plotItemColor(fp.item).name(), fp.item->title().text(), stry);
        }
#endif
    } else {
        // PickNearestPoint 模式
        if (!isEnableShowXValue()) {
            // 只显示Y值
            for (int i = 0; i < fps.size(); ++i) {
                if (i > 0)
                    out += "<br/>";
                out += fmtY(fps[ i ]);
            }
        } else {
            // 显示完整坐标 (X, Y)
            for (int i = 0; i < fps.size(); ++i) {
                if (i > 0)
                    out += "<br/>";
                const FeaturePoint& fp = fps[ i ];
                out += QString("(%1, %2)").arg(fmtX(fp), fmtY(fp));
            }
        }
    }
    return out;
}

/**
 * \if ENGLISH
 * @brief Draw captured feature points
 *
 * Renders all captured feature points using the provided painter.
 *
 * @note Control visibility via setEnableDrawFeaturePoint()
 * @param painter Drawing context
 * \endif
 *
 * \if CHINESE
 * @brief 绘制捕获的特征点
 *
 * 使用提供的painter渲染所有捕获的特征点。
 *
 * @note 通过 setEnableDrawFeaturePoint() 控制可见性
 * @param painter 绘图上下文
 * \endif
 */
void QwtPlotSeriesDataPicker::drawAllFeaturePoints(QPainter* painter) const
{
    QWT_DC(d);
    const QList< QwtPlotSeriesDataPicker::FeaturePoint >& pickedFeatureDatas = d->featurePoints;
    for (int i = 0; i < pickedFeatureDatas.size(); ++i) {
        const QwtPlotSeriesDataPicker::FeaturePoint& fp = pickedFeatureDatas[ i ];
        QwtPlot* itemPlot                               = fp.item->plot();
        drawFeaturePoint(painter, itemPlot, fp.item, fp.feature);
    }
}

/**
 * \if ENGLISH
 * @brief Draw a single feature point on the plot
 *
 * This virtual method is responsible for rendering individual feature points
 * on the QwtPlot canvas. Users can override this function to implement
 * custom drawing styles for feature points (e.g., different shapes, colors,
 * or visual effects).
 *
 * The default implementation draws a circular point at the specified position:
 * - Size: determined by drawFeaturePointSize()
 * - Border: 1-pixel outline using the curve color
 * - Fill: solid fill with the curve color darkened by 150%
 *
 * @param painter The QPainter object for drawing operations
 * @param plot Reference to the QwtPlot widget where drawing occurs
 * @param item The curve item associated with this feature point
 * @param itemPoint The coordinates of the feature point in plot coordinates
 *
 * @see drawFeaturePointSize()
 * @see setDrawFeaturePointSize()
 * @see drawAllFeaturePoints()
 * \endif
 *
 * \if CHINESE
 * @brief 在绘图区域绘制单个特征点
 *
 * 此虚方法负责在画布上渲染单个特征点。用户可以重写此函数来实现
 * 自定义的特征点绘制样式（例如：不同的形状、颜色或视觉效果）。
 *
 * 默认实现会在指定位置绘制一个圆形点：
 * - 大小：由 drawFeaturePointSize() 决定
 * - 边框：使用曲线颜色的1像素轮廓
 * - 填充：使用曲线颜色加深150%进行实心填充
 *
 * @param painter 用于绘图操作的 QPainter 对象
 * @param plot 绘制操作所在的 QwtPlot 部件引用
 * @param item 与此特征点关联的曲线项
 * @param itemPoint 特征点在绘图坐标系中的坐标
 *
 * @see drawFeaturePointSize()
 * @see setDrawFeaturePointSize()
 * @see drawAllFeaturePoints()
 * \endif
 */
void QwtPlotSeriesDataPicker::drawFeaturePoint(
    QPainter* painter, const QwtPlot* plot, const QwtPlotItem* item, const QPointF& itemPoint
) const
{
    if (!plot || !item) {
        return;
    }
    QWT_DC(d);
    const QwtScaleMap xMap = plot->canvasMap(item->xAxis());
    const QwtScaleMap yMap = plot->canvasMap(item->yAxis());
    // 把点转换到屏幕坐标
    QPointF screenPos = QwtScaleMap::transform(xMap, yMap, itemPoint);
    QColor itemColor  = Qwt::plotItemColor(item, Qt::black);
    // 绘制点
    painter->save();
    QColor fillColor = itemColor.darker(150);  // 150% 变暗，可根据需要调整
    // 设置画笔（边框）
    painter->setPen(QPen(fillColor, 1));
    // 设置画刷（填充）
    painter->setBrush(QBrush(fillColor));
    painter->drawEllipse(screenPos.toPoint(), d->featurePointSize, d->featurePointSize);
    painter->restore();
}

void QwtPlotSeriesDataPicker::move(const QPoint& pos)
{
    updateFeaturePoint(pos);
    QwtPicker::move(pos);
}

QString QwtPlotSeriesDataPicker::formatAxisValue(double value, int axisId, QwtPlot* plot) const
{
    if (!plot) {
        return QString::number(value);
    }

    // 获取坐标轴的刻度绘制器
    const QwtScaleDraw* scaleDraw = plot->axisScaleDraw(axisId);
    if (scaleDraw) {
        // 使用坐标轴的格式化器
        QwtText text = scaleDraw->label(value);
        return text.text();
    }

    // 回退到默认数值显示
    return QString::number(value);
}

void QwtPlotSeriesDataPicker::updateFeaturePoint(const QPoint& pos)
{
    const QwtPlot* currentPlot = plot();
    if (!currentPlot) {
        return;
    }
    m_data->mousePos = pos;
    switch (pickMode()) {
    case PickYValue:
        pickYValue(currentPlot, pos, isInterpolation());
        break;
    case PickNearestPoint:
        pickNearestPoint(currentPlot, pos, nearestSearchWindowSize());
        break;
    default:
        break;
    }
}

QRect QwtPlotSeriesDataPicker::trackerRect(const QFont& f) const
{
    QRect r = QwtPicker::trackerRect(f);
    // 提前处理不需要改变 rect 位置的情况
    QwtPlotSeriesDataPicker::TextPlacement ta = textArea();
    if (QwtPlotSeriesDataPicker::TextPlaceAuto == ta && pickMode() == PickNearestPoint) {
        return r;
    }
    QWT_DC(d);
    const QRect plotRect = pickArea().boundingRect().toRect();
    // 根据 textArea 和 pickMode 调整 rect 位置
    if (QwtPlotSeriesDataPicker::TextPlaceAuto == ta) {
        // 对于 TextPlaceAuto, 只有 PickYValue 模式需要特殊处理
        if (pickMode() == PickYValue) {
            // 预测偏移后的位置
            QPoint offset    = textTrackerOffset();
            QRect offsetRect = r.translated(offset);
            return ensureRectInBounds(offsetRect, plotRect);
        }
        // 其他 pickMode 保持 rect 不变
        return r;
    }
    // 根据指定的 textArea 位置调整
    switch (ta) {
    case TextFollowOnTop:
        r.moveTop(plotRect.top());
        break;
    case TextFollowOnBottom:
        r.moveBottom(plotRect.bottom());
        break;
    case TextOnCanvasTopRight:
        r.moveTopRight(plotRect.topRight());
        break;
    case TextOnCanvasTopLeft:
        r.moveTopLeft(plotRect.topLeft());
        break;
    case TextOnCanvasBottomRight:
        r.moveBottomRight(plotRect.bottomRight());
        break;
    case TextOnCanvasBottomLeft:
        r.moveBottomLeft(plotRect.bottomLeft());
        break;
    case TextOnCanvasTopAuto:
        // 对于自动模式，要根据当前鼠标的位置判断
        if (d->mousePos.x() >= plotRect.width() - r.width()) {
            r.moveTopLeft(plotRect.topLeft());
        } else {
            r.moveTopRight(plotRect.topRight());
        }
        break;
    case TextOnCanvasBottomAuto:
        // 对于自动模式，要根据当前鼠标的位置判断
        if (d->mousePos.x() >= plotRect.width() - r.width()) {
            r.moveBottomLeft(plotRect.bottomLeft());
        } else {
            r.moveBottomRight(plotRect.bottomRight());
        }
        break;
    default:
        // 对于未明确指定的 textArea，保持 rect 不变
        break;
    }

    return r;
}


QRect QwtPlotSeriesDataPicker::ensureRectInBounds(const QRect& rect, const QRect& bounds) const
{
    QRect constrainedRect = rect;

    // 水平方向约束
    if (constrainedRect.width() > bounds.width()) {
        // 如果矩形比绘图区域还宽，左对齐
        constrainedRect.moveLeft(bounds.left());
    } else {
        if (constrainedRect.left() < bounds.left()) {
            constrainedRect.moveLeft(bounds.left());
        }
        if (constrainedRect.right() > bounds.right()) {
            constrainedRect.moveRight(bounds.right());
        }
    }

    // 垂直方向约束
    if (constrainedRect.height() > bounds.height()) {
        // 如果矩形比绘图区域还高，top对齐
        constrainedRect.moveTop(bounds.top());
    } else {
        if (constrainedRect.top() < bounds.top()) {
            constrainedRect.moveTop(bounds.top());
        }
        if (constrainedRect.bottom() > bounds.bottom()) {
            constrainedRect.moveBottom(bounds.bottom());
        }
    }

    return constrainedRect;
}

void QwtPlotSeriesDataPicker::drawRubberBand(QPainter* painter) const
{
    // 主要针对pick PickNearestPoint
    if (!isActive()) {
        return;
    }
    if (!painter || !painter->isActive()) {
        return;
    }

    QPen rbPen              = rubberBandPen();
    const QPoint mousePoint = trackerPosition();
    if (mousePoint.isNull() || mousePoint.x() < 0 || mousePoint.y() < 0) {
        return;
    }
    switch (pickMode()) {
    case PickYValue: {
        painter->save();
        painter->setPen(rbPen);
        const QRect pRect = pickArea().boundingRect().toRect();
        QwtPainter::drawLine(painter, mousePoint.x(), pRect.top(), mousePoint.x(), pRect.bottom());
        painter->restore();
    } break;
    case PickNearestPoint: {
        if (m_data->featurePoints.isEmpty()) {
            return;
        }
        const QwtPlotSeriesDataPicker::FeaturePoint& fp = m_data->featurePoints.first();
        QwtPlot* itemPlot                               = fp.item->plot();
        if (!itemPlot) {
            return;
        }
        const QwtScaleMap xMap = itemPlot->canvasMap(fp.item->xAxis());
        const QwtScaleMap yMap = itemPlot->canvasMap(fp.item->yAxis());
        // 把点转换到屏幕坐标
        QPointF screenPos = QwtScaleMap::transform(xMap, yMap, fp.feature);
        QColor itemColor  = Qwt::plotItemColor(fp.item, Qt::black);
        rbPen.setColor(itemColor);
        painter->save();
        painter->setPen(rbPen);
        QwtPainter::drawLine(painter, mousePoint, screenPos);
        painter->restore();
    } break;
    default:
        break;
    }

    if (isEnableDrawFeaturePoint()) {
        drawAllFeaturePoints(painter);
    }
}

void QwtPlotSeriesDataPicker::setTrackerPosition(const QPoint& pos)
{
    updateFeaturePoint(pos);
    QwtPicker::setTrackerPosition(pos);
}

/**
 * \if ENGLISH
 * @brief Get all pickable Y values at a specified screen position
 *
 * This method scans all pickable curves at a given screen coordinate position
 * and collects their corresponding data points. It supports both host and
 * parasitic plots, traversing through all related plot items regardless of
 * their hosting relationship.
 *
 * The function returns the total count of feature points successfully picked.
 * For each picked point, the internal data structures are updated with the
 * curve information and calculated Y values.
 *
 * @param plot The plot widget (can be either host or parasitic plot)
 * @param pos Screen position in widget coordinates
 * @param interpolate Whether to perform interpolation between data points.
 *                   When true, linear interpolation is applied if the position
 *                   falls between two data points; otherwise, the nearest
 *                   data point is selected.
 * @return The number of feature points successfully picked and stored.
 *         Returns 0 if no pickable items are found at the position.
 *
 * @note This function considers both host and parasitic plots. When called
 *       with a host plot, it includes all its parasitic plots; when called
 *       with a parasitic plot, it includes its host and all siblings.
 * @see featurePointList()
 * @see drawFeaturePoints()
 * @see clearFeaturePoints()
 * \endif
 *
 * \if CHINESE
 * @brief 获取绘图区域指定屏幕位置上所有可拾取的Y值
 *
 * 此方法在给定的屏幕坐标位置扫描所有可拾取曲线，并收集它们对应的数据点。
 * 它同时支持宿主绘图和寄生绘图，无论绘图项之间的宿主关系如何，都会遍历所有相关绘图项。
 *
 * 函数返回成功拾取的特征点总数。对于每个拾取的点，内部数据结构会更新曲线信息和计算的Y值。
 *
 * @param plot 绘图部件（可以是宿主绘图或寄生绘图）
 * @param pos 部件坐标系中的屏幕位置
 * @param interpolate 是否在数据点之间执行插值计算。
 *                   当为true时，如果位置落在两个数据点之间，则应用线性插值；
 *                   否则，选择最近的数据点。
 * @return 成功拾取并存储的特征点数量。如果在该位置未找到可拾取项，则返回0。
 *
 * @note 此函数同时考虑宿主和寄生绘图。当传入宿主绘图时，包含其所有寄生绘图；
 *       当传入寄生绘图时，包含其宿主和所有兄弟绘图。
 * @see featurePointList()
 * @see drawFeaturePoints()
 * @see clearFeaturePoints()
 * \endif
 */
int QwtPlotSeriesDataPicker::pickYValue(const QwtPlot* p, const QPoint& pos, bool interpolate)
{
    if (!p) {
        return 0;
    }
    QWT_D(d);
    QList< QwtPlotSeriesDataPicker::FeaturePoint >& featurePoints = d->featurePoints;
    featurePoints.clear();
#if QwtPlotSeriesDataPicker_XGroup
    d->xGroups.clear();
#endif
    const QList< QwtPlot* > plotList = p->plotList();

    // 收集所有曲线
    QList< QwtPlotCurve* > allCurves;
    for (QwtPlot* oneplot : plotList) {
        const QwtPlotItemList& items = oneplot->itemList();
        for (QwtPlotItem* item : items) {
            if (item->rtti() == QwtPlotItem::Rtti_PlotCurve) {
                QwtPlotCurve* curve = static_cast< QwtPlotCurve* >(item);
                if (curve->isVisible() && curve->dataSize() > 0) {
                    allCurves.append(curve);
                }
            }
        }
    }
#if QwtPlotSeriesDataPicker_XGroup
    if (pickMode() == PickYValue && isEnableShowXValue()) {
        // 按X轴分组（同一个plot中的同一个X轴为一组）
        QHash< PrivateData::GroupKey, QList< QwtPlotCurve* > > curvesByAxis;

        for (QwtPlotCurve* curve : allCurves) {
            QwtPlot* p = curve->plot();
            PrivateData::GroupKey key { p, curve->xAxis() };
            curvesByAxis[ key ].append(curve);
        }

        // 处理每个X轴组
        for (auto it = curvesByAxis.begin(); it != curvesByAxis.end(); ++it) {
            const PrivateData::GroupKey& key     = it.key();
            const QList< QwtPlotCurve* >& curves = it.value();

            if (curves.isEmpty()) {
                continue;
            }

            // 创建分组
            PrivateData::XGroup group;
            group.key = key;

            // 获取该X轴对应的plot
            QwtPlot* plotForAxis = key.plot;
            if (!plotForAxis) {
                continue;
            }

            // 获取该X轴的映射
            const QwtScaleMap xMap = plotForAxis->canvasMap(key.axis);

            // 计算鼠标位置对应的X值（这是该组所有曲线共享的X值）
            double mouseXValue = xMap.invTransform(pos.x());

            // 格式化X值，作为该组的公共X值
            group.xValue = formatAxisValue(mouseXValue, key.axis, plotForAxis);

            // 处理该组内的所有曲线
            for (QwtPlotCurve* curve : curves) {
                const size_t curveSize = curve->dataSize();
                if (curveSize == 0) {
                    continue;
                }

                // 获取曲线的边界矩形
                const QRectF br = curve->boundingRect();
                // 边界检查：鼠标X值是否在曲线X范围内
                if (mouseXValue < br.left() || mouseXValue > br.right()) {
                    continue;
                }

                // 在曲线数据中查找对应的点
                size_t index =
                    qwtUpperSampleIndex< QPointF >(*curve->data(), mouseXValue, [](const double x, const QPointF& pos) -> bool {
                        return (x < pos.x());
                    });

                if (index == curveSize) {
                    continue;
                }

                // 创建特征点
                FeaturePoint fp;
                fp.item  = curve;
                fp.index = index;

                if (interpolate && curveSize > 2 && index > 0) {
                    // 插值计算
                    const QPointF& p2 = curve->sample(index);
                    const QPointF& p1 = curve->sample(index - 1);
                    if (qFuzzyCompare(p1.x(), p2.x())) {
                        fp.feature = p2;
                    } else {
                        double t = (mouseXValue - p1.x()) / (p2.x() - p1.x());
                        QPointF interPoint;
                        interPoint.setX(mouseXValue);  // 使用统一的X值
                        interPoint.setY(p1.y() + t * (p2.y() - p1.y()));
                        fp.feature = interPoint;
                    }
                } else {
                    QPointF point = curve->sample(index);
                    // 使用统一的X值，而不是曲线上的实际X值
                    fp.feature = QPointF(mouseXValue, point.y());
                }

                featurePoints.append(fp);
                group.fps.append(&featurePoints.last());
            }

            // 如果该组有特征点，添加到分组列表
            if (!group.fps.isEmpty()) {
                d->xGroups.append(group);
            }
        }
    } else {
        // 不分组显示X值，或不是PickYValue模式
        for (QwtPlotCurve* curve : allCurves) {
            const size_t curveSize = curve->dataSize();
            if (curveSize == 0)
                continue;

            QwtPlot* oneplot = curve->plot();

            // 获取曲线的坐标轴映射
            const QwtScaleMap xMap = oneplot->canvasMap(curve->xAxis());

            // 将屏幕坐标转换为曲线坐标系的坐标
            double x = xMap.invTransform(pos.x());

            // 提前计算并缓存边界矩形
            const QRectF br = curve->boundingRect();

            // 快速边界检查
            if (x < br.left() || x > br.right()) {
                continue;
            }

            size_t index = qwtUpperSampleIndex< QPointF >(*curve->data(), x, [](const double x, const QPointF& pos) -> bool {
                return (x < pos.x());
            });

            if (index == curveSize) {
                continue;
            }

            FeaturePoint fp;
            fp.item  = curve;
            fp.index = index;

            if (interpolate && curveSize > 2 && index > 0) {
                // 插值计算
                const QPointF& p2 = curve->sample(index);
                const QPointF& p1 = curve->sample(index - 1);
                if (qFuzzyCompare(p1.x(), p2.x())) {
                    fp.feature = p2;
                } else {
                    double t = (x - p1.x()) / (p2.x() - p1.x());
                    QPointF interPoint;
                    interPoint.setX(x);
                    interPoint.setY(p1.y() + t * (p2.y() - p1.y()));
                    fp.feature = interPoint;
                }
            } else {
                fp.feature = curve->sample(index);
            }

            featurePoints.append(fp);
        }
    }
#else
    for (QwtPlotCurve* curve : allCurves) {
        const size_t curveSize = curve->dataSize();
        if (curveSize == 0) {
            continue;
        }

        QwtPlot* oneplot = curve->plot();

        // 获取曲线的坐标轴映射
        const QwtScaleMap xMap = oneplot->canvasMap(curve->xAxis());

        // 将屏幕坐标转换为曲线坐标系的坐标
        double x = xMap.invTransform(pos.x());

        // 提前计算并缓存边界矩形
        const QRectF br = curve->boundingRect();
        if (x < br.left() || x > br.right()) {
            continue;
        }

        // 快速边界检查
        if (x < br.left() || x > br.right()) {
            continue;
        }

        size_t index = qwtUpperSampleIndex< QPointF >(*curve->data(), x, [](const double x, const QPointF& pos) -> bool {
            return (x < pos.x());
        });

        if (index == curveSize) {
            continue;
        }

        FeaturePoint fp;
        fp.item  = curve;
        fp.index = index;

        if (interpolate && curveSize > 2 && index > 0) {
            // 插值计算
            const QPointF& p2 = curve->sample(index);
            const QPointF& p1 = curve->sample(index - 1);
            if (qFuzzyCompare(p1.x(), p2.x())) {
                fp.feature = p2;
            } else {
                double t = (x - p1.x()) / (p2.x() - p1.x());
                QPointF interPoint;
                interPoint.setX(x);
                interPoint.setY(p1.y() + t * (p2.y() - p1.y()));
                fp.feature = interPoint;
            }
        } else {
            fp.feature = curve->sample(index);
        }

        featurePoints.append(fp);
    }
#endif
    return featurePoints.size();
}

/**
 * @brief 获取绘图区域指定屏幕位置上最近的可拾取点
 * @param plot 绘图对象
 * @param pos 屏幕位置
 * @param windowSize 窗口尺寸
 *        - 0: 不使用窗口，搜索整个曲线
 *        - 正数: 固定的窗口大小（数据点数量）
 *        - 负数: 自适应窗口，使用曲线数据点总数的百分比（取绝对值，如-5表示5%,-10代表10%）
 * @return 包含最近绘图项和对应数据点的配对
 *
 * @note 此函数考虑了寄生绘图，可以传入宿主绘图或寄生绘图，它会把全部绘图的数据进行获取
 */
int QwtPlotSeriesDataPicker::pickNearestPoint(const QwtPlot* plot, const QPoint& pos, int windowSize)
{
    if (!plot) {
        return 0;
    }
    QWT_D(d);
    QList< QwtPlotSeriesDataPicker::FeaturePoint >& featurePoints = d->featurePoints;
    featurePoints.clear();
#if QwtPlotSeriesDataPicker_XGroup
    d->xGroups.clear();
#endif
    QwtPlotSeriesDataPicker::FeaturePoint fp;

    double minScreenDistance         = std::numeric_limits< double >::max();
    const QList< QwtPlot* > plotList = plot->plotList();
    for (QwtPlot* oneplot : plotList) {
        const auto items = oneplot->itemList(QwtPlotItem::Rtti_PlotCurve);
        for (auto* item : items) {
            auto* curve            = static_cast< QwtPlotCurve* >(item);
            const size_t curveSize = static_cast< int >(curve->dataSize());
            if (!curve->isVisible() || curveSize == 0) {
                continue;
            }
            const auto* series = curve->data();

            /* 二分找最靠近 x 的一段，再比较距离平方 */
            const QwtScaleMap xMap = oneplot->canvasMap(curve->xAxis());
            const QwtScaleMap yMap = oneplot->canvasMap(curve->yAxis());

            // 计算搜索窗口
            double targetX      = xMap.invTransform(pos.x());
            auto searchWinIndex = calculateSearchWindow(curveSize, targetX, *series, windowSize);
            size_t startIndex   = searchWinIndex.first;
            size_t endIndex     = searchWinIndex.second;

            // 在计算出的窗口内搜索最近点
            double minDistance = std::numeric_limits< double >::max();
            QPointF candidateNearestPoint;
            size_t candidateIndex = startIndex;

            for (size_t i = startIndex; i <= endIndex; ++i) {
                QPointF point         = curve->sample(i);
                int screenX           = qRound(xMap.transform(point.x()));
                int screenY           = qRound(yMap.transform(point.y()));
                double dx             = double(screenX) - pos.x();
                double dy             = double(screenY) - pos.y();
                double screenDistance = dx * dx + dy * dy;

                if (screenDistance < minDistance) {
                    minDistance           = screenDistance;
                    candidateNearestPoint = point;
                    candidateIndex        = i;
                }
            }

            if (minDistance < minScreenDistance) {
                minScreenDistance = minDistance;
                fp.item           = item;
                fp.feature        = candidateNearestPoint;
                fp.index          = candidateIndex;
            }
        }
    }
    if (minScreenDistance < std::numeric_limits< double >::max()) {
        featurePoints.append(fp);
        return 1;
    }
    return featurePoints.size();
}

void QwtPlotSeriesDataPicker::onPlotItemDetached(QwtPlotItem* item, bool on)
{
    // 遍历看看是否有此item
    QWT_D(d);
    if (!on) {
        QList< QwtPlotSeriesDataPicker::FeaturePoint >& pickedFeatureDatas = d->featurePoints;
        for (int i = pickedFeatureDatas.size() - 1; i >= 0; --i) {
            const QwtPlotSeriesDataPicker::FeaturePoint& fp = pickedFeatureDatas[ i ];
            if (fp.item == item) {
#if QwtPlotSeriesDataPicker_XGroup
                // 同步要删除xGroups
                for (int j = d->xGroups.size() - 1; j >= 0; --j) {
                    PrivateData::XGroup& g = d->xGroups[ j ];
                    for (int k = g.fps.size() - 1; k >= 0; --k) {
                        if (g.fps[ k ] == &fp) {
                            g.fps.removeAt(k);
                        }
                    }
                }
#endif
                pickedFeatureDatas.removeAt(i);  // 反向删除，避免索引混乱
            }
        }
    }
}

void QwtPlotSeriesDataPicker::onParasitePlotAttached(QwtPlot* parasiteplot, bool on)
{
    QWT_D(d);
    if (on) {
        // 寄生轴新增，需要绑定寄生轴的onPlotItemDetached
        connect(parasiteplot, &QwtPlot::itemAttached, this, &QwtPlotSeriesDataPicker::onPlotItemDetached);
    } else {
        disconnect(parasiteplot, nullptr, this, nullptr);
        // clear featurePoints make it invalid
        d->featurePoints.clear();
#if QwtPlotSeriesDataPicker_XGroup
        // 同步把已有的信息删除
        QWT_D(d);
        for (int i = d->xGroups.size() - 1; i >= 0; --i) {
            PrivateData::XGroup& g = d->xGroups[ i ];
            if (g.key.plot == parasiteplot) {
                d->xGroups.removeAt(i);
            }
        }
#endif
    }
}
