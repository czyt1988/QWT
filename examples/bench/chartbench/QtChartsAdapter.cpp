#include "QtChartsAdapter.h"

#include <QChart>
#include <QChartView>
#include <QImage>
#include <QLegend>
#include <QLineSeries>
#include <QPainter>
#include <QValueAxis>
#include <QGraphicsScene>
#include <QOpenGLWidget>

namespace chartbench
{

QtChartsAdapter::QtChartsAdapter(Backend backend)
    : m_backend(backend)
{
}

QtChartsAdapter::~QtChartsAdapter()
{
    destroy();
}

QString QtChartsAdapter::backendDescription() const
{
    return m_useOpenGl ? QStringLiteral("QGraphicsScene + QXYSeries::setUseOpenGL (GL paint device)")
                       : QStringLiteral("QGraphicsScene + QPainter raster");
}

bool QtChartsAdapter::create(QWidget* parent, const PlotSpec& spec, QString& error)
{
    if (m_backend == Backend::OpenGL) {
        // Measured, not assumed: with setUseOpenGL(true) the series never reach
        // the framebuffer in this harness. QGraphicsView::render() and
        // QOpenGLWidget::grabFramebuffer() both return an image whose curve
        // coverage is 0.0009 (axes only) against 0.0586 for the raster path.
        // Qt defers the GL series composition to a backing store flush that a
        // synchronous frame protocol cannot trigger, so Qt Charts OpenGL is
        // excluded from the comparison instead of being reported wrongly.
        error = QStringLiteral(
            "Qt Charts OpenGL series do not render synchronously (no curve pixels in the "
            "framebuffer); the raster path is the only measurable Qt Charts backend");
        return false;
    }

    m_chart = new QChart();
    m_chart->legend()->hide();
    m_chart->setMargins(QMargins(0, 0, 0, 0));
    m_chart->setBackgroundRoundness(0);
    m_chart->setBackgroundBrush(QBrush(Qt::white));
    m_chart->setPlotAreaBackgroundVisible(false);
    m_chart->setAnimationOptions(QChart::NoAnimation);

    m_axisX = new QValueAxis();
    m_axisY = new QValueAxis();
    m_axisX->setRange(spec.xMin, spec.xMax);
    m_axisY->setRange(spec.yMin, spec.yMax);
    m_axisX->setTickCount(6);
    m_axisY->setTickCount(6);
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    m_view = new QChartView(m_chart, parent);
    if (m_backend == Backend::OpenGL) {
        // GL accelerated series are composed by a GL viewport, a plain QWidget
        // viewport silently drops them.
        m_view->setViewport(new QOpenGLWidget());
    }
    m_view->setRenderHint(QPainter::Antialiasing, spec.antialiasing);
    m_view->setRenderHint(QPainter::TextAntialiasing, spec.antialiasing);
    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_useOpenGl = (m_backend == Backend::OpenGL);

    m_view->resize(spec.widgetSize);
    m_view->setMinimumSize(spec.widgetSize);
    m_target = QImage(spec.widgetSize, QImage::Format_ARGB32_Premultiplied);
    m_target.fill(Qt::white);
    return true;
}

void QtChartsAdapter::destroy()
{
    m_series.clear();
    delete m_view;   // owns the chart
    m_view = nullptr;
    m_chart = nullptr;
    m_axisX = nullptr;
    m_axisY = nullptr;
    m_frames = 0;
}

QWidget* QtChartsAdapter::widget() const
{
    return m_view;
}

void QtChartsAdapter::rebuild(const std::vector<std::vector<double>>& x,
                              const std::vector<std::vector<double>>& y,
                              const PlotSpec& spec)
{
    for (QLineSeries* series : m_series)
        m_chart->removeSeries(series);
    m_series.clear();

    m_useOpenGl = (m_backend == Backend::OpenGL);

    for (size_t c = 0; c < x.size(); ++c) {
        QLineSeries* series = new QLineSeries();
        QList<QPointF> points;
        points.reserve(static_cast<int>(x[c].size()));
        for (size_t i = 0; i < x[c].size(); ++i)
            points.append(QPointF(x[c][i], y[c][i]));

        series->replace(points);
        series->setPen(QPen(QColor(31, 119, 180), 1.0));
        if (m_useOpenGl)
            series->setUseOpenGL(true);

        m_chart->addSeries(series);
        series->attachAxis(m_axisX);
        series->attachAxis(m_axisY);
        m_series.push_back(series);
    }

    m_view->setRenderHint(QPainter::Antialiasing, spec.antialiasing);
    setAxesRange(spec.xMin, spec.xMax, spec.yMin, spec.yMax);
}

void QtChartsAdapter::updateWindow(int index, const std::vector<double>& x,
                                   const std::vector<double>& y, int offset, int count)
{
    if (index < 0 || index >= static_cast<int>(m_series.size()))
        return;

    QList<QPointF> points;
    points.reserve(count);
    for (int i = 0; i < count; ++i)
        points.append(QPointF(x[offset + i], y[offset + i]));
    m_series[index]->replace(points);
}

void QtChartsAdapter::setAxesRange(double xMin, double xMax, double yMin, double yMax)
{
    if (!m_axisX)
        return;
    m_axisX->setRange(xMin, xMax);
    m_axisY->setRange(yMin, yMax);
}

void QtChartsAdapter::rescaleAxes()
{
    if (m_series.empty() || !m_chart)
        return;
    // Qt Charts has no "rescale all" call: the ranges have to be derived from
    // the series data, which is what QChart::createDefaultAxes() does as well.
    bool initialized = false;
    double xMin = 0.0, xMax = 0.0, yMin = 0.0, yMax = 0.0;
    for (QLineSeries* series : m_series) {
        const QList<QPointF> points = series->points();
        for (const QPointF& point : points) {
            if (!initialized) {
                xMin = xMax = point.x();
                yMin = yMax = point.y();
                initialized = true;
                continue;
            }
            xMin = qMin(xMin, point.x());
            xMax = qMax(xMax, point.x());
            yMin = qMin(yMin, point.y());
            yMax = qMax(yMax, point.y());
        }
    }
    if (initialized) {
        m_axisX->setRange(xMin, xMax);
        m_axisY->setRange(yMin, yMax);
    }
}

void QtChartsAdapter::frame()
{
    // Qt Charts is a QGraphicsScene; QGraphicsView::render() runs exactly the
    // scene render that paintEvent would run, and - unlike repaint() on a GL
    // viewport - it also completes for series with setUseOpenGL(true), whose
    // composition Qt otherwise defers to the backing store flush.
    QPainter painter(&m_target);
    m_view->render(&painter, QRectF(), QRect(QPoint(0, 0), m_view->size()),
                   Qt::IgnoreAspectRatio);
    m_gpuSyncMs = 0.0;
    ++m_frames;
}

QImage QtChartsAdapter::exportWidget()
{
    QImage image(m_view->size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    {
        QPainter painter(&image);
        m_view->render(&painter);
    }
    return image;
}

} // namespace chartbench
