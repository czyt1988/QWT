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
// qt
#include <QPainter>
#include <QtMath>

class QwtPlotSeriesDataPicker::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtPlotSeriesDataPicker)
public:
    PrivateData(QwtPlotSeriesDataPicker* p);

public:
    QwtPlotSeriesDataPicker::PickSeriesMode pickMode { QwtPlotSeriesDataPicker::PickYValue };
    QwtPlotSeriesDataPicker::TextPlacement textArea { QwtPlotSeriesDataPicker::TextOnTop };
    QwtPlotSeriesDataPicker::InterpolationMode interpolationMode { QwtPlotSeriesDataPicker::NoInterpolation };
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
    setRubberBand(QwtPlotPicker::VLineRubberBand);
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
    switch (mode) {
    case PickYValue:
        setRubberBand(QwtPlotPicker::VLineRubberBand);
        break;
    case PickNearestPoint:
        setRubberBand(QwtPlotPicker::NoRubberBand);
        break;
    default:
        break;
    }
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

    if (m_data->pickMode == PickYValue) {
        bool interpolate                                    = (m_data->interpolationMode == LinearInterpolation);
        QList< QPair< QwtPlotItem*, QPointF > > pickedItems = pickYValue(currentPlot, pos, interpolate);
        if (pickedItems.isEmpty()) {
            return QwtText();
        }
        for (int i = 0; i < pickedItems.size(); ++i) {
            QwtPlotItem* plotItem = pickedItems[ i ].first;
            const QPointF& point  = pickedItems[ i ].second;
            text += valueString(point, plotItem, i);
        }

    } else {  // PickNearestPoint
        auto nearestPoint = pickNearestPoint(currentPlot, pos);
        if (nearestPoint.first) {
            const QPointF& point  = nearestPoint.second;
            QwtPlotItem* plotItem = nearestPoint.first;
            text                  = valueString(point, plotItem, 0);
        }
    }

    if (text.isEmpty()) {
        // 回退到默认跟踪器文本
        return QwtPlotPicker::trackerText(pos);
    }

    QwtText trackerText(text);
    trackerText.setRenderFlags(Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap);
    trackerText.setBackgroundBrush(QBrush(Qt::white));

    return trackerText;
}

/**
 * @brief 生成一个文字内容
 *
 * 如果想自定义文字显示，可重写此函数
 * @param value 值
 * @param item 对应的item
 * @return
 */
QString QwtPlotSeriesDataPicker::valueString(const QPointF& value, QwtPlotItem* item, int index) const
{
    if (m_data->pickMode == PickYValue) {
        QString t;
        if (index != 0) {
            t += "<br/>";
        }
        t += QString("<font color=%1>%2</font>:%3")
                 .arg(Qwt::plotItemColor(item).name())
                 .arg(item->title().text())
                 .arg(value.y());
        return t;
    }
    return QString("%1:(%2,%3)").arg(item->title().text()).arg(value.x()).arg(value.y());
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
    if (m_data->textArea == TextFollowMouse) {
        return rect;  // 默认就是跟随
    }

    const QRect plotRect = pickArea().boundingRect().toRect();
    switch (textArea()) {
    case TextOnTop:
        rect.moveTop(plotRect.top());
        break;
    case TextOnBottom: {
        rect.moveBottom(plotRect.bottom());
    } break;
    default:
        break;
    }
    return rect;
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
QList< QPair< QwtPlotItem*, QPointF > >
QwtPlotSeriesDataPicker::pickYValue(const QwtPlot* plot, const QPoint& pos, bool interpolate)
{
    QList< QPair< QwtPlotItem*, QPointF > > result;

    if (!plot) {
        return result;
    }
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
                // 获取曲线的坐标轴映射
                const QwtScaleMap xMap = oneplot->canvasMap(curve->xAxis());

                // 将屏幕坐标转换为曲线坐标系的坐标
                double x        = xMap.invTransform(pos.x());
                const QRectF br = curve->boundingRect();
                if (br.isValid() && x >= br.left() && x <= br.right()) {
                    int index = qwtUpperSampleIndex< QPointF >(
                        *curve->data(), x, [](const double x, const QPointF& pos) -> bool { return (x < pos.x()); });

                    if (index < 0) {
                        // 没有找到合适的
                        continue;
                    }
                    if (interpolate && curveSize > 2 && index != 0) {
                        // 说明要进行插值计算
                        const QPointF& p2 = curve->sample(index);
                        const QPointF& p1 = curve->sample(index - 1);
                        if (qFuzzyCompare(p1.x(), p2.x())) {
                            result.append(qMakePair(item, p2));
                            continue;
                        } else {
                            double t = (x - p1.x()) / (p2.x() - p1.x());
                            QPointF interPoint;
                            interPoint.setX(x);
                            interPoint.setY(p1.y() + t * (p2.y() - p1.y()));
                        }
                    } else {
                        result.append(qMakePair(item, curve->sample(index)));
                    }
                }
            }
        }
    }
    return result;
}

/**
 * @brief 获取绘图区域指定屏幕位置上最近的可拾取点
 * @param plot 绘图对象
 * @param pos 屏幕位置
 * @param windowSize 窗口尺寸
 *        -1 : 代表自适应窗口，使用数据量的%5作为窗口长度
 *        0: 不使用窗口
 *        其它正数: 窗口长度
 * @return 包含最近绘图项和对应数据点的配对
 *
 * @note 此函数考虑了寄生绘图，可以传入宿主绘图或寄生绘图，它会把全部绘图的数据进行获取
 */
QPair< QwtPlotItem*, QPointF > QwtPlotSeriesDataPicker::pickNearestPoint(const QwtPlot* plot, const QPoint& pos, int windowSize)
{
    QPair< QwtPlotItem*, QPointF > best { nullptr, QPointF() };
    if (!plot) {
        return best;
    }

    double minScreenDistance         = std::numeric_limits< double >::max();
    const QList< QwtPlot* > plotList = plot->plotList();
    for (QwtPlot* oneplot : plotList) {
        const auto items = oneplot->itemList(QwtPlotItem::Rtti_PlotCurve);
        for (auto* item : items) {
            auto* curve         = static_cast< QwtPlotCurve* >(item);
            const int curveSize = static_cast< int >(curve->dataSize());
            if (!curve->isVisible() || curveSize == 0) {
                continue;
            }
            const auto* series = curve->data();
            bool useWindow = curveSize > 2000 && windowSize != 0;  // 数量大于2000且windowSize不为0，则使用窗口
            int realWindowSize = curveSize;
            if (useWindow) {
                if (windowSize < 0) {
                    realWindowSize = curveSize * 0.05;
                }
            }

            /* 二分找最靠近 x 的一段，再比较距离平方 */
            const QwtScaleMap xMap = oneplot->canvasMap(curve->xAxis());
            const QwtScaleMap yMap = oneplot->canvasMap(curve->yAxis());

            int startIndex = 0;
            int endIndex   = curveSize - 1;
            if (useWindow) {
                // 使用窗，针对大数据量可以提高速度
                double x  = xMap.invTransform(pos.x());
                int index = qwtUpperSampleIndex< QPointF >(
                    *series, x, [](const double x, const QPointF& pos) -> bool { return (x < pos.x()); });
                if (index < 0) {
                    // 说明鼠标不在范围内，一般就是在曲线的左边或者曲线的右边
                    if (x < curve->minXValue()) {
                        // 说明在曲线的左边
                        if (realWindowSize > 0 && realWindowSize < curveSize) {
                            endIndex = realWindowSize;
                        }
                    } else {
                        // 说明在曲线的右边
                        if (realWindowSize > 0 && endIndex > realWindowSize) {
                            startIndex = endIndex - realWindowSize;
                        }
                    }
                } else {
                    // 说明鼠标在曲线的中间位置，
                    //  按窗口设置startIndex和endIndex
                    int halfWindow = realWindowSize / 2;
                    startIndex     = index - halfWindow;
                    endIndex       = index + halfWindow;
                    if (startIndex < 0) {
                        startIndex = 0;
                    }
                    if (endIndex > curveSize - 1) {
                        endIndex = curveSize - 1;
                    }
                }
            }
            // TODO: 这里还可以求出屏幕边界对应的xstart和xend值,遍历时只需考虑xstart和xend值范围内的索引,
            // 先确认屏幕的xstart值是否大于曲线的第一个点，如果是说明这个曲线在这个canvas里没显示全部，在确认xend是否小于曲线的最后一个点
            // 如果是，也说明这个曲线的尾部也没有在这个canvas里显示完全，这时就可以找到xstart对应的索引作为startIndex的最小限定，
            // xend对应的索引就是endIndex对应的最大限定

            // 遍历求距离最近点
            double distance = std::numeric_limits< double >::max();
            QPointF nearestPoint;
            for (int index = startIndex; index <= endIndex; ++index) {
                QPointF candidatePoint = curve->sample(index);
                // 计算屏幕坐标距离
                int screenX           = qRound(xMap.transform(candidatePoint.x()));
                int screenY           = qRound(yMap.transform(candidatePoint.y()));
                double dx             = screenX - pos.x();
                double dy             = screenY - pos.y();
                double screenDistance = dx * dx + dy * dy;
                if (screenDistance < distance) {
                    distance     = screenDistance;
                    nearestPoint = candidatePoint;
                }
            }
            if (distance < minScreenDistance) {
                distance    = minScreenDistance;
                best.first  = item;
                best.second = nearestPoint;
            }
        }
    }
    return best;
}
