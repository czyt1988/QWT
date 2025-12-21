#include "qwt_figure_picker.h"
#include <qwt_plot_marker.h>
#include <qwt_scale_map.h>
#include <QApplication>
#include <QPainterPath>
#include <QDebug>
#include "qwt_scale_div.h"

// 私有数据类
class QwtFigurePicker::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtFigurePicker)
public:
    PrivateData(QwtFigurePicker* p)
        : q_ptr(p)
        , figure(nullptr)
        , enabled(true)
        , active(false)
        , rubberBandStyle(NoRubberBand)
        , trackerMode(AlwaysOn)
        , showRefLines(true)
        , rubberBandPen(Qt::red, 1, Qt::DashLine)
        , trackerPen(Qt::black)
        , trackerFont(QFont("Arial", 9))
        , referenceLinePen(Qt::blue, 1, Qt::DotLine)
        , trackerOffset(10, 10)
        , isDragging(false)
        , currentPlot(nullptr)
        , overlayWidget(nullptr)
    {
        trackerPen.setColor(Qt::white);
        trackerPen.setStyle(Qt::SolidLine);
    }

    ~PrivateData()
    {
        clearReferenceLines();
    }

    void clearReferenceLines()
    {
        qDeleteAll(referenceMarkers);
        referenceMarkers.clear();
    }

    QwtFigure* figure;                ///< 目标QwtFigure
    bool enabled;                     ///< 是否启用
    bool active;                      ///< 是否激活
    RubberBandStyle rubberBandStyle;  ///< 橡皮筋样式
    TrackerMode trackerMode;          ///< 跟踪模式
    bool showRefLines;                ///< 是否显示参考线

    QPen rubberBandPen;     ///< 橡皮筋画笔
    QPen trackerPen;        ///< 跟踪器画笔
    QFont trackerFont;      ///< 跟踪器字体
    QPen referenceLinePen;  ///< 参考线画笔
    QPoint trackerOffset;   ///< 跟踪器偏移

    bool isDragging;       ///< 是否正在拖动
    QPoint dragStartPos;   ///< 拖动开始位置
    QRect rubberBandRect;  ///< 橡皮筋矩形
    QPoint lastMousePos;   ///< 最后鼠标位置

    QwtPlot* currentPlot;  ///< 当前鼠标所在的绘图
    QPointF currentValue;  ///< 当前坐标值

    QMap< QwtPlot*, QRect > canvasRectMap;              ///< 画布区域缓存
    QMap< QwtPlot*, QwtPlotMarker* > referenceMarkers;  ///< 参考线标记

    // 覆盖层部件，用于绘制橡皮筋和跟踪器
    class OverlayWidget : public QWidget
    {
    public:
        OverlayWidget(QWidget* parent, QwtFigurePicker* picker) : QWidget(parent), picker(picker)
        {
            setAttribute(Qt::WA_TransparentForMouseEvents);
            setAttribute(Qt::WA_NoSystemBackground);
            setAutoFillBackground(false);
        }

    protected:
        void paintEvent(QPaintEvent* event) override
        {
            Q_UNUSED(event);
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);

            // 绘制橡皮筋
            if (picker->isActive() && !picker->m_data->rubberBandRect.isEmpty()) {
                picker->paintRubberBand(&painter, picker->m_data->rubberBandRect);
            }

            // 绘制跟踪器
            if (picker->isActive() && picker->m_data->trackerMode != AlwaysOff) {
                QString text = picker->trackerText(picker->m_data->lastMousePos);
                if (!text.isEmpty()) {
                    QRect rect = picker->trackerRect(text, picker->m_data->lastMousePos);
                    picker->drawTracker(&painter, rect, text);
                }
            }
        }

    private:
        QwtFigurePicker* picker;
    };

    OverlayWidget* overlayWidget;  ///< 覆盖层部件
};

QwtFigurePicker::QwtFigurePicker(QwtFigure* figure, QObject* par) : QObject(par), QWT_PIMPL_CONSTRUCT
{
    m_data->figure = figure;
    init();
}

QwtFigurePicker::~QwtFigurePicker()
{
    end();
    removeFilters();
}

void QwtFigurePicker::init()
{
    // 创建覆盖层
    m_data->overlayWidget = new PrivateData::OverlayWidget(m_data->figure, this);
    m_data->overlayWidget->raise();
    m_data->overlayWidget->show();

    // 安装事件过滤器
    installFilters();

    // 更新画布区域
    updateCanvasRects();

    // 连接QwtFigure的信号
    if (m_data->figure) {
        connect(m_data->figure, &QwtFigure::axesAdded, this, &QwtFigurePicker::installFilters);
        connect(m_data->figure, &QwtFigure::axesRemoved, this, &QwtFigurePicker::installFilters);
        connect(m_data->figure, &QwtFigure::figureCleared, this, &QwtFigurePicker::clearReferenceLines);
    }
}

void QwtFigurePicker::setEnabled(bool on)
{
    if (m_data->enabled != on) {
        m_data->enabled = on;
        if (!on) {
            end();
        }
        m_data->overlayWidget->setVisible(on);
        emit enabled(on);
    }
}

bool QwtFigurePicker::isEnabled() const
{
    return m_data->enabled;
}

void QwtFigurePicker::setActive(bool on)
{
    if (m_data->active != on) {
        m_data->active = on;
        if (!on) {
            m_data->rubberBandRect = QRect();
            clearReferenceLines();
        }
        m_data->overlayWidget->update();
        emit activated(on);
    }
}

bool QwtFigurePicker::isActive() const
{
    return m_data->active;
}

void QwtFigurePicker::setRubberBand(RubberBandStyle style)
{
    m_data->rubberBandStyle = style;
}

QwtFigurePicker::RubberBandStyle QwtFigurePicker::rubberBand() const
{
    return m_data->rubberBandStyle;
}

void QwtFigurePicker::setRubberBandPen(const QPen& pen)
{
    m_data->rubberBandPen = pen;
}

QPen QwtFigurePicker::rubberBandPen() const
{
    return m_data->rubberBandPen;
}

void QwtFigurePicker::setTrackerMode(TrackerMode mode)
{
    m_data->trackerMode = mode;
}

QwtFigurePicker::TrackerMode QwtFigurePicker::trackerMode() const
{
    return m_data->trackerMode;
}

void QwtFigurePicker::setTrackerFont(const QFont& font)
{
    m_data->trackerFont = font;
}

QFont QwtFigurePicker::trackerFont() const
{
    return m_data->trackerFont;
}

void QwtFigurePicker::setTrackerPen(const QPen& pen)
{
    m_data->trackerPen = pen;
}

QPen QwtFigurePicker::trackerPen() const
{
    return m_data->trackerPen;
}

void QwtFigurePicker::setTrackerPositionOffset(const QPoint& offset)
{
    m_data->trackerOffset = offset;
}

QPoint QwtFigurePicker::trackerPositionOffset() const
{
    return m_data->trackerOffset;
}

void QwtFigurePicker::setShowReferenceLines(bool show)
{
    m_data->showRefLines = show;
    if (!show) {
        clearReferenceLines();
    }
}

bool QwtFigurePicker::showReferenceLines() const
{
    return m_data->showRefLines;
}

void QwtFigurePicker::setReferenceLinePen(const QPen& pen)
{
    m_data->referenceLinePen = pen;
    // 更新现有参考线的样式
    for (QwtPlotMarker* marker : m_data->referenceMarkers) {
        if (marker) {
            marker->setLinePen(m_data->referenceLinePen);
            marker->plot()->replot();
        }
    }
}

QPen QwtFigurePicker::referenceLinePen() const
{
    return m_data->referenceLinePen;
}

QwtPlot* QwtFigurePicker::currentPlot() const
{
    return m_data->currentPlot;
}

QPointF QwtFigurePicker::plotValue(QwtPlot* plot) const
{
    if (!plot || !m_data->canvasRectMap.contains(plot)) {
        return QPointF(0, 0);
    }

    QRect canvasRect = m_data->canvasRectMap[ plot ];
    if (!canvasRect.contains(m_data->lastMousePos)) {
        return QPointF(0, 0);
    }

    return toPlotValue(plot, m_data->lastMousePos - canvasRect.topLeft());
}

QRect QwtFigurePicker::canvasRect(QwtPlot* plot) const
{
    return m_data->canvasRectMap.value(plot, QRect());
}

QMap< QwtPlot*, QRect > QwtFigurePicker::canvasRects() const
{
    return m_data->canvasRectMap;
}

void QwtFigurePicker::paintRubberBand(QPainter* painter, const QRect& rect)
{
    if (!m_data->enabled || !m_data->active || m_data->rubberBandStyle == NoRubberBand || rect.isEmpty()) {
        return;
    }

    painter->save();
    painter->setPen(m_data->rubberBandPen);
    painter->setBrush(Qt::NoBrush);

    switch (m_data->rubberBandStyle) {
    case VLineRubberBand:
        painter->drawLine(rect.center().x(), rect.top(), rect.center().x(), rect.bottom());
        break;
    case HLineRubberBand:
        painter->drawLine(rect.left(), rect.center().y(), rect.right(), rect.center().y());
        break;
    case CrossRubberBand:
        painter->drawLine(rect.center().x(), rect.top(), rect.center().x(), rect.bottom());
        painter->drawLine(rect.left(), rect.center().y(), rect.right(), rect.center().y());
        break;
    case RectRubberBand:
        painter->drawRect(rect);
        break;
    case EllipseRubberBand:
        painter->drawEllipse(rect);
        break;
    case NoRubberBand:
    case UserRubberBand:
    default:
        break;
    }

    painter->restore();
}

QString QwtFigurePicker::trackerText(const QPoint& pos) const
{
    if (!m_data->enabled || !m_data->active || m_data->trackerMode == AlwaysOff) {
        return QString();
    }

    if (!m_data->currentPlot) {
        return QString("X: %1, Y: %2").arg(pos.x()).arg(pos.y());
    }

    QPointF value = toPlotValue(m_data->currentPlot, pos - canvasRect(m_data->currentPlot).topLeft());

    return QString("X: %1, Y: %2").arg(value.x(), 0, 'f', 3).arg(value.y(), 0, 'f', 3);
}

QRect QwtFigurePicker::trackerRect(const QString& text, const QPoint& pos) const
{
    if (text.isEmpty()) {
        return QRect();
    }

    QFontMetrics fm(m_data->trackerFont);
    QSize textSize = fm.size(Qt::TextSingleLine, text);

    QPoint trackerPos = pos + m_data->trackerOffset;

    // 确保跟踪器不会超出窗口
    if (m_data->figure) {
        QRect figRect = m_data->figure->rect();
        if (trackerPos.x() + textSize.width() > figRect.right()) {
            trackerPos.setX(pos.x() - textSize.width() - m_data->trackerOffset.x());
        }
        if (trackerPos.y() + textSize.height() > figRect.bottom()) {
            trackerPos.setY(pos.y() - textSize.height() - m_data->trackerOffset.y());
        }
    }

    return QRect(trackerPos, textSize + QSize(8, 4));  // 添加一些边距
}

void QwtFigurePicker::drawTracker(QPainter* painter, const QRect& rect, const QString& text) const
{
    if (text.isEmpty() || !rect.isValid()) {
        return;
    }

    painter->save();

    // 绘制背景
    painter->setBrush(QColor(0, 0, 0, 180));  // 半透明黑色
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(rect, 3, 3);

    // 绘制文本
    painter->setPen(m_data->trackerPen);
    painter->setFont(m_data->trackerFont);
    painter->drawText(rect.adjusted(4, 2, -4, -2), Qt::AlignCenter, text);

    painter->restore();
}

void QwtFigurePicker::begin()
{
    setActive(true);
}

void QwtFigurePicker::end()
{
    setActive(false);
    m_data->isDragging     = false;
    m_data->rubberBandRect = QRect();
}

void QwtFigurePicker::replot()
{
    if (m_data->figure) {
        m_data->figure->replotAll();
    }
    m_data->overlayWidget->update();
}

bool QwtFigurePicker::eventFilter(QObject* watched, QEvent* event)
{
    if (!m_data->enabled || !m_data->figure) {
        return QObject::eventFilter(watched, event);
    }

    // 检查是否为绘图画布事件
    QwtPlot* plot = nullptr;
    for (QwtPlot* p : m_data->canvasRectMap.keys()) {
        if (p->canvas() == watched) {
            plot = p;
            break;
        }
    }

    if (!plot) {
        // 可能是QwtFigure本身的事件
        if (watched == m_data->figure) {
            // 处理QwtFigure的绘制事件
            if (event->type() == QEvent::Paint) {
                m_data->overlayWidget->update();
            }
        }
        return QObject::eventFilter(watched, event);
    }

    // 处理绘图事件
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        mousePressEvent(static_cast< QMouseEvent* >(event), plot);
        break;
    case QEvent::MouseMove:
        mouseMoveEvent(static_cast< QMouseEvent* >(event), plot);
        break;
    case QEvent::MouseButtonRelease:
        mouseReleaseEvent(static_cast< QMouseEvent* >(event), plot);
        break;
    case QEvent::Enter:
        enterEvent(event, plot);
        break;
    case QEvent::Leave:
        leaveEvent(event, plot);
        break;
    default:
        break;
    }

    return QObject::eventFilter(watched, event);
}

void QwtFigurePicker::mouseMoveEvent(QMouseEvent* event, QwtPlot* plot)
{
    if (!m_data->enabled) {
        return;
    }

    m_data->currentPlot = plot;
    QPoint globalPos    = event->pos();

    // 转换为全局坐标
    QPoint plotPos       = globalPos - canvasRect(plot).topLeft();
    m_data->currentValue = toPlotValue(plot, plotPos);
    m_data->lastMousePos = globalPos;

    // 更新橡皮筋
    if (m_data->isDragging && m_data->rubberBandStyle != NoRubberBand) {
        QPoint start           = m_data->dragStartPos;
        QPoint end             = globalPos;
        m_data->rubberBandRect = QRect(start, end).normalized();
    }

    // 更新参考线
    if (m_data->showRefLines && m_data->active) {
        updateReferenceLines(m_data->currentValue.x(), m_data->currentValue.y());
    }

    // 更新覆盖层
    m_data->overlayWidget->update();

    // 发送信号
    emit moved(globalPos, plot, m_data->currentValue);
}

void QwtFigurePicker::mousePressEvent(QMouseEvent* event, QwtPlot* plot)
{
    if (!m_data->enabled || event->button() != Qt::LeftButton) {
        return;
    }

    setActive(true);
    m_data->isDragging     = true;
    m_data->dragStartPos   = event->pos();
    m_data->rubberBandRect = QRect(m_data->dragStartPos, m_data->dragStartPos);
    m_data->currentPlot    = plot;
}

void QwtFigurePicker::mouseReleaseEvent(QMouseEvent* event, QwtPlot* plot)
{
    Q_UNUSED(plot);

    if (!m_data->enabled || event->button() != Qt::LeftButton) {
        return;
    }

    if (m_data->isDragging) {
        // 获取涉及到的绘图
        QList< QwtPlot* > plotsInRect;
        for (QwtPlot* p : m_data->canvasRectMap.keys()) {
            if (m_data->rubberBandRect.intersects(m_data->canvasRectMap[ p ])) {
                plotsInRect.append(p);
            }
        }

        // 发送选择信号
        emit selected(m_data->rubberBandRect, plotsInRect);
    }

    m_data->isDragging     = false;
    m_data->rubberBandRect = QRect();
    m_data->overlayWidget->update();
}

void QwtFigurePicker::enterEvent(QEvent* event, QwtPlot* plot)
{
    Q_UNUSED(event);
    m_data->currentPlot = plot;

    if (m_data->trackerMode == ActiveOnly) {
        setActive(true);
    }
}

void QwtFigurePicker::leaveEvent(QEvent* event, QwtPlot* plot)
{
    Q_UNUSED(event);
    Q_UNUSED(plot);

    m_data->currentPlot = nullptr;

    if (m_data->trackerMode == ActiveOnly) {
        setActive(false);
    }

    // 清除参考线
    clearReferenceLines();
}

void QwtFigurePicker::paintReferenceLines(QPainter* painter, const QPoint& pos, QwtPlot* plot)
{
    if (!m_data->showRefLines || !plot || !m_data->active) {
        return;
    }

    painter->save();
    painter->setPen(m_data->referenceLinePen);

    // 获取当前绘图区域
    QRect canvasRect = m_data->canvasRectMap.value(plot, QRect());
    if (!canvasRect.isValid()) {
        painter->restore();
        return;
    }

    // 绘制垂直线
    if (pos.x() >= canvasRect.left() && pos.x() <= canvasRect.right()) {
        painter->drawLine(pos.x(), canvasRect.top(), pos.x(), canvasRect.bottom());
    }

    // 绘制水平线
    if (pos.y() >= canvasRect.top() && pos.y() <= canvasRect.bottom()) {
        painter->drawLine(canvasRect.left(), pos.y(), canvasRect.right(), pos.y());
    }

    painter->restore();
}

void QwtFigurePicker::updateReferenceLines(double x, double y)
{
    if (!m_data->showRefLines) {
        return;
    }

    // 清除旧标记
    clearReferenceLines();

    // 为每个绘图添加参考线标记
    for (QwtPlot* plot : m_data->canvasRectMap.keys()) {
        if (!plot)
            continue;

        // 检查坐标是否在绘图范围内
        const QwtScaleDiv& xScaleDiv = plot->axisScaleDiv(QwtAxis::XBottom);
        const QwtScaleDiv& yScaleDiv = plot->axisScaleDiv(QwtAxis::YLeft);


        // 垂直线
        if (x >= xScaleDiv.lowerBound() && x <= xScaleDiv.upperBound()) {
            QwtPlotMarker* vMarker = new QwtPlotMarker();
            vMarker->setLineStyle(QwtPlotMarker::VLine);
            vMarker->setLinePen(m_data->referenceLinePen);
            vMarker->setXValue(x);
            vMarker->attach(plot);
            m_data->referenceMarkers[ plot ] = vMarker;
        }

        // 水平线
        if (y >= yScaleDiv.lowerBound() && y <= yScaleDiv.upperBound()) {
            QwtPlotMarker* hMarker = new QwtPlotMarker();
            hMarker->setLineStyle(QwtPlotMarker::HLine);
            hMarker->setLinePen(m_data->referenceLinePen);
            hMarker->setYValue(y);
            hMarker->attach(plot);
            // 如果已有标记，合并
            if (m_data->referenceMarkers.contains(plot)) {
                // 这里简化处理，实际可能需要合并两个标记
            } else {
                m_data->referenceMarkers[ plot ] = hMarker;
            }
        }

        plot->replot();
    }
}

void QwtFigurePicker::clearReferenceLines()
{
    m_data->clearReferenceLines();

    // 重绘所有绘图
    for (QwtPlot* plot : m_data->canvasRectMap.keys()) {
        if (plot) {
            plot->replot();
        }
    }
}

void QwtFigurePicker::installFilters()
{
    removeFilters();

    if (!m_data->figure) {
        return;
    }

    // 安装过滤器到QwtFigure
    m_data->figure->installEventFilter(this);

    // 安装过滤器到所有绘图画布
    updateCanvasRects();
    for (QwtPlot* plot : m_data->canvasRectMap.keys()) {
        if (plot && plot->canvas()) {
            plot->canvas()->installEventFilter(this);
        }
    }
}

void QwtFigurePicker::removeFilters()
{
    if (m_data->figure) {
        m_data->figure->removeEventFilter(this);
    }

    for (QwtPlot* plot : m_data->canvasRectMap.keys()) {
        if (plot && plot->canvas()) {
            plot->canvas()->removeEventFilter(this);
        }
    }
}

void QwtFigurePicker::updateCanvasRects()
{
    m_data->canvasRectMap.clear();

    if (!m_data->figure) {
        return;
    }

    QList< QwtPlot* > plots = m_data->figure->allAxes();
    for (QwtPlot* plot : plots) {
        if (plot && plot->canvas()) {
            QRect plotRect   = plot->geometry();
            QRect canvasRect = plot->canvas()->geometry();

            // 转换为全局坐标
            QPoint globalPos = plot->mapTo(m_data->figure, canvasRect.topLeft());
            QRect globalRect(globalPos, canvasRect.size());

            m_data->canvasRectMap[ plot ] = globalRect;
        }
    }

    // 更新覆盖层大小
    if (m_data->overlayWidget && m_data->figure) {
        m_data->overlayWidget->setGeometry(m_data->figure->rect());
        m_data->overlayWidget->raise();
    }
}

QPointF QwtFigurePicker::toPlotValue(QwtPlot* plot, const QPoint& pos) const
{
    if (!plot) {
        return QPointF(0, 0);
    }

    const QwtScaleMap xMap = plot->canvasMap(QwtAxis::XBottom);
    const QwtScaleMap yMap = plot->canvasMap(QwtAxis::YLeft);

    double x = xMap.invTransform(pos.x());
    double y = yMap.invTransform(pos.y());

    return QPointF(x, y);
}

QPoint QwtFigurePicker::toPlotPosition(QwtPlot* plot, const QPointF& value) const
{
    if (!plot) {
        return QPoint(0, 0);
    }

    const QwtScaleMap xMap = plot->canvasMap(QwtAxis::XBottom);
    const QwtScaleMap yMap = plot->canvasMap(QwtAxis::YLeft);

    int x = qRound(xMap.transform(value.x()));
    int y = qRound(yMap.transform(value.y()));

    return QPoint(x, y);
}

QPoint QwtFigurePicker::toGlobalPosition(QwtPlot* plot, const QPoint& plotPos) const
{
    if (!plot || !m_data->canvasRectMap.contains(plot)) {
        return plotPos;
    }

    QRect canvasRect = m_data->canvasRectMap[ plot ];
    return canvasRect.topLeft() + plotPos;
}
