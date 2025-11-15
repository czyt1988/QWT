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
// qt
#include <QPainter>
#include <QtMath>
#include <QDebug>
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
QPair< size_t, size_t >
calculateSearchWindow(size_t curveSize, double targetX, const QwtSeriesData< QPointF >& data, int windowSize = -5)
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
    size_t centerIndex = qwtUpperSampleIndex< QPointF >(
        data, targetX, [](const double x, const QPointF& point) -> bool { return (x < point.x()); });

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
    size_t actualWindowSize = endIndex - startIndex + 1;

    // 如果窗口实际上包含了大部分数据，不如搜索整个曲线
    const double FULL_SEARCH_THRESHOLD = 0.8;  // 80%
    if (actualWindowSize >= curveSize * FULL_SEARCH_THRESHOLD) {
        startIndex = 0;
        endIndex   = curveSize - 1;
    }
    return qMakePair(startIndex, endIndex);
}

class QwtPlotSeriesDataPicker::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotSeriesDataPicker)
public:
    PrivateData(QwtPlotSeriesDataPicker* p);
    struct FeaturePoint
    {
        QwtPlotItem* item;  ///< 对应的item
        QPointF feature;    ///< 特征点
        size_t index;       ///< 在item里的索引
    };

public:
    QwtPlotSeriesDataPicker::PickSeriesMode pickMode { QwtPlotSeriesDataPicker::PickYValue };
    QwtPlotSeriesDataPicker::TextPlacement textArea { QwtPlotSeriesDataPicker::TextPlaceAuto };
    QwtPlotSeriesDataPicker::InterpolationMode interpolationMode { QwtPlotSeriesDataPicker::LinearInterpolation };
    // 渲染相关
    QBrush textBackgroundBrush { QColor(255, 255, 255, 125) };
    Qt::Alignment textAlignment { Qt::AlignLeft | Qt::AlignVCenter };
    // 记录找到的特征点
    QList< FeaturePoint > featurePoints;
    int featurePointSize { 4 };      ///< 特征点的大小
    bool markFeaturePoint { true };  ///< 是否标记捕获的特征点
};

QwtPlotSeriesDataPicker::PrivateData::PrivateData(QwtPlotSeriesDataPicker* p) : q_ptr(p)
{
}

//===============================================================
// QwtPlotSeriesDataPicker
//===============================================================

QwtPlotSeriesDataPicker::QwtPlotSeriesDataPicker(QWidget* canvas) : QwtPlotPicker(canvas), QWT_PIMPL_CONSTRUCT
{
    // 设置追踪模式，始终显示追踪信息
    setTrackerMode(QwtPlotPicker::ActiveOnly);
    // 设置橡皮筋为垂直线
    setRubberBand(QwtPlotPicker::UserRubberBand);
    // 设置状态机，用于点选择
    setStateMachine(new QwtPickerTrackerMachine);
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
 * @brief trackerText的重载
 *
 * QwtPlotPicker是基于固定x和y轴的，QwtPlot上的item有可能绑定不同的x轴和y轴，因此这里不使用@ref trackerTextF
 *
 * 通过pickYPoint或
 * @param pos
 * @return
 */
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
    const QList< QwtPlotSeriesDataPicker::PrivateData::FeaturePoint >& pickedFeatureDatas = d->featurePoints;
    if (pickedFeatureDatas.empty()) {
        return QwtText();
    }
    for (int i = 0; i < pickedFeatureDatas.size(); ++i) {
        const QwtPlotSeriesDataPicker::PrivateData::FeaturePoint& fp = pickedFeatureDatas[ i ];
        text += valueString(fp.feature, fp.item, fp.index, i);
    }

    if (text.isEmpty()) {
        // 回退到默认跟踪器文本
        return QwtPlotPicker::trackerText(pos);
    }

    QwtText trackerText(text);
    trackerText.setRenderFlags(int(d->textAlignment) | Qt::TextWordWrap);
    trackerText.setBackgroundBrush(d->textBackgroundBrush);

    return trackerText;
}

/**
 * @brief 生成一个文字内容
 *
 * 如果想自定义文字显示，可重写此函数
 * @param value 值
 * @param item 对应的item
 * @param seriesIndex 对应的series的索引
 * @param order 序号，对于有多个值要显示的，这个order会递增，通过这个值可以判断是否换行，或者显示的值太多进行省略显示
 * @return
 */
QString QwtPlotSeriesDataPicker::valueString(const QPointF& value, QwtPlotItem* item, size_t seriesIndex, int order) const
{
    Q_UNUSED(seriesIndex);
    if (m_data->pickMode == PickYValue) {
        QString t;
        if (order != 0) {
            t += "<br/>";
        }
        t += QString("<font color=%1>%2</font>:%3")
                 .arg(Qwt::plotItemColor(item).name())
                 .arg(item->title().text())
                 .arg(value.y());
        return t;
    }
    // Pick Nearest Point
    return QString("(%1 , %2)").arg(value.x()).arg(value.y());
}

/**
 * @brief 绘制特征点，把捕获到的点进行绘制
 *
 * 如果想特征点绘制进行自定义，可重写此函数
 *
 * @note setEnableDrawFeaturePoint
 * @param  painter
 */
void QwtPlotSeriesDataPicker::drawFeaturePoints(QPainter* painter) const
{
    QWT_DC(d);
    const QList< QwtPlotSeriesDataPicker::PrivateData::FeaturePoint >& pickedFeatureDatas = d->featurePoints;
    for (int i = 0; i < pickedFeatureDatas.size(); ++i) {
        const QwtPlotSeriesDataPicker::PrivateData::FeaturePoint& fp = pickedFeatureDatas[ i ];
        QwtPlot* itemPlot                                            = fp.item->plot();
        if (!itemPlot) {
            continue;
        }
        const QwtScaleMap xMap = itemPlot->canvasMap(fp.item->xAxis());
        const QwtScaleMap yMap = itemPlot->canvasMap(fp.item->yAxis());
        // 把点转换到屏幕坐标
        QPointF screenPos = QwtScaleMap::transform(xMap, yMap, fp.feature);
        QColor itemColor  = Qwt::plotItemColor(fp.item, Qt::black);
        // 绘制点
        painter->save();
        painter->setPen(QPen(itemColor, 1));
        painter->drawEllipse(screenPos.toPoint(), d->featurePointSize, d->featurePointSize);
        painter->restore();
    }
}

void QwtPlotSeriesDataPicker::move(const QPoint& pos)
{
    const QwtPlot* currentPlot = plot();
    if (!currentPlot) {
        return;
    }
    switch (pickMode()) {
    case PickYValue:
        pickYValue(currentPlot, pos, isInterpolation());
        break;
    case PickNearestPoint:
        pickNearestPoint(currentPlot, pos);
        break;
    default:
        break;
    }
    QwtPlotPicker::move(pos);
}

/**
 * @brief 绘制的区域在
 *
 * 根据文字显示的位置，调整矩形
 * @param f
 * @return
 */
QRect QwtPlotSeriesDataPicker::trackerRect(const QFont& f) const
{
    QRect rect = QwtPlotPicker::trackerRect(f);
    // 提前处理不需要改变 rect 位置的情况
    if (textArea() == QwtPlotSeriesDataPicker::TextPlaceAuto && pickMode() == PickNearestPoint) {
        return rect;
    }
    const QRect plotRect = pickArea().boundingRect().toRect();

    // 根据 textArea 和 pickMode 调整 rect 位置
    if (textArea() == QwtPlotSeriesDataPicker::TextPlaceAuto) {
        // 对于 TextPlaceAuto, 只有 PickYValue 模式需要特殊处理
        if (pickMode() == PickYValue) {
            rect.moveTop(plotRect.top());
        }
        // 其他 pickMode 保持 rect 不变
    } else {
        // 根据指定的 textArea 位置调整
        switch (textArea()) {
        case TextOnTop:
            rect.moveTop(plotRect.top());
            break;
        case TextOnBottom:
            rect.moveBottom(plotRect.bottom());
            break;
        default:
            // 对于未明确指定的 textArea，保持 rect 不变
            break;
        }
    }

    return rect;
}

void QwtPlotSeriesDataPicker::drawRubberBand(QPainter* painter) const
{
    // 主要针对pick PickNearestPoint
    if (!isActive()) {
        return;
    }
    QPen rbPen              = rubberBandPen();
    const QPoint mousePoint = trackerPosition();

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
        const QwtPlotSeriesDataPicker::PrivateData::FeaturePoint& fp = m_data->featurePoints.first();
        QwtPlot* itemPlot                                            = fp.item->plot();
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
        drawFeaturePoints(painter);
    }
}

/**
 * @brief 获取绘图区域指定屏幕位置上所有可拾取的Y值
 * @param plot 绘图对象
 * @param pos 屏幕位置
 * @param interpolate 是否进行插值计算
 * @return 包含绘图项和对应数据点的列表
 *
 * @note 此函数考虑了寄生绘图，可以传入宿主绘图或寄生绘图，它会把全部绘图的数据进行获取
 */
int QwtPlotSeriesDataPicker::pickYValue(const QwtPlot* plot, const QPoint& pos, bool interpolate)
{
    if (!plot) {
        return 0;
    }
    QWT_D(d);
    QList< QwtPlotSeriesDataPicker::PrivateData::FeaturePoint >& featurePoints = d->featurePoints;
    featurePoints.clear();
    const QList< QwtPlot* > plotList = plot->plotList();

    // 遍历所有绘图项
    for (QwtPlot* oneplot : plotList) {
        const QwtPlotItemList& items = oneplot->itemList();
        for (QwtPlotItem* item : items) {
            if (item->rtti() == QwtPlotItem::Rtti_PlotCurve) {
                QwtPlotCurve* curve    = static_cast< QwtPlotCurve* >(item);
                const size_t curveSize = curve->dataSize();
                if (!curve->isVisible() || curveSize == 0) {
                    continue;
                }
                // 提前计算并缓存边界矩形
                const QRectF br = curve->boundingRect();
                if (!br.isValid()) {
                    continue;
                }
                // 获取曲线的坐标轴映射
                const QwtScaleMap xMap = oneplot->canvasMap(curve->xAxis());
                // 将屏幕坐标转换为曲线坐标系的坐标
                double x = xMap.invTransform(pos.x());
                // 快速边界检查
                if (x < br.left() || x > br.right()) {
                    continue;
                }

                size_t index = qwtUpperSampleIndex< QPointF >(
                    *curve->data(), x, [](const double x, const QPointF& pos) -> bool { return (x < pos.x()); });

                if (index == curveSize) {
                    // 没有找到合适的
                    continue;
                }
                if (interpolate && curveSize > 2 && index > 0) {
                    // 说明要进行插值计算
                    const QPointF& p2 = curve->sample(index);
                    const QPointF& p1 = curve->sample(index - 1);
                    if (qFuzzyCompare(p1.x(), p2.x())) {
                        QwtPlotSeriesDataPicker::PrivateData::FeaturePoint fp;
                        fp.item    = item;
                        fp.feature = p2;
                        fp.index   = index;
                        featurePoints.append(fp);
                        continue;
                    } else {
                        double t = (x - p1.x()) / (p2.x() - p1.x());
                        QPointF interPoint;
                        interPoint.setX(x);
                        interPoint.setY(p1.y() + t * (p2.y() - p1.y()));

                        QwtPlotSeriesDataPicker::PrivateData::FeaturePoint fp;
                        fp.item    = item;
                        fp.feature = interPoint;
                        fp.index   = index;
                        featurePoints.append(fp);
                    }
                } else {
                    QwtPlotSeriesDataPicker::PrivateData::FeaturePoint fp;
                    fp.item    = item;
                    fp.feature = curve->sample(index);
                    fp.index   = index;
                    featurePoints.append(fp);
                }
            }
        }
    }
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
    QList< QwtPlotSeriesDataPicker::PrivateData::FeaturePoint >& featurePoints = d->featurePoints;
    featurePoints.clear();

    QwtPlotSeriesDataPicker::PrivateData::FeaturePoint fp;

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
            size_t startIndex   = 0;
            size_t endIndex     = curveSize - 1;
            double targetX      = xMap.invTransform(pos.x());
            auto searchWinIndex = calculateSearchWindow(curveSize, targetX, *series, windowSize);
            startIndex          = searchWinIndex.first;
            endIndex            = searchWinIndex.second;

            // 在计算出的窗口内搜索最近点
            double minDistance = std::numeric_limits< double >::max();
            QPointF candidateNearestPoint;
            size_t candidateIndex = startIndex;

            for (size_t i = startIndex; i <= endIndex; ++i) {
                QPointF point         = curve->sample(i);
                int screenX           = qRound(xMap.transform(point.x()));
                int screenY           = qRound(yMap.transform(point.y()));
                double dx             = screenX - pos.x();
                double dy             = screenY - pos.y();
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
