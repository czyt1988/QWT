#include "QCustomPlotAdapter.h"

#include "qcustomplot.h"

#include <QImage>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QPainter>

namespace chartbench
{

QCustomPlotAdapter::QCustomPlotAdapter(Backend backend)
    : m_backend(backend)
{
}

QCustomPlotAdapter::~QCustomPlotAdapter()
{
    destroy();
}

QString QCustomPlotAdapter::backendDescription() const
{
#ifdef QCUSTOMPLOT_USE_OPENGL
    if (m_backend == Backend::OpenGL)
        return QStringLiteral("QCPPaintBufferGlFbo (QOpenGLPaintDevice on an FBO)");
#endif
    return QStringLiteral("QCPPaintBufferPixmap (QPainter into a QPixmap)");
}

QString QCustomPlotAdapter::downsamplingDescription() const
{
    return m_sampling == QcpSampling::Adaptive ? QStringLiteral("adaptive sampling (pixel min/max)")
                                               : QStringLiteral("none");
}

QStringList QCustomPlotAdapter::downsamplingAlgorithms() const
{
    return QStringList { QStringLiteral("none"), QStringLiteral("adaptive") };
}

bool QCustomPlotAdapter::setDownsamplingAlgorithm(const QString& name)
{
    if (name == QLatin1String("none"))
        m_sampling = QcpSampling::None;
    else if (name == QLatin1String("adaptive"))
        m_sampling = QcpSampling::Adaptive;
    else
        return false;

    applySampling();
    return true;
}

void QCustomPlotAdapter::applySampling()
{
    const bool adaptive = (m_sampling == QcpSampling::Adaptive);
    for (QCPGraph* graph : m_graphs)
        graph->setAdaptiveSampling(adaptive);
}

bool QCustomPlotAdapter::create(QWidget* parent, const PlotSpec& spec, QString& error)
{
    Q_UNUSED(error);
    m_plot = new QCustomPlot(parent);
    m_plot->setBackground(QBrush(Qt::white));
    m_plot->setAutoAddPlottableToLegend(false);
    m_plot->setNoAntialiasingOnDrag(false);
    m_plot->xAxis->setRange(spec.xMin, spec.xMax);
    m_plot->yAxis->setRange(spec.yMin, spec.yMax);
    m_plot->setAntialiasedElements(spec.antialiasing ? QCP::aePlottables : QCP::aeNone);

    if (m_backend == Backend::OpenGL) {
#ifdef QCUSTOMPLOT_USE_OPENGL
        m_plot->setOpenGl(true, 0);
#else
        error = QStringLiteral(
            "QCustomPlot was compiled without QCUSTOMPLOT_USE_OPENGL - use chartbench_qcpgl");
        return false;
#endif
    }

    m_plot->resize(spec.widgetSize);
    m_plot->setMinimumSize(spec.widgetSize);
    m_plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return true;
}

void QCustomPlotAdapter::destroy()
{
    m_graphs.clear();
    delete m_plot;
    m_plot = nullptr;
    m_frames = 0;
}

QWidget* QCustomPlotAdapter::widget() const
{
    return m_plot;
}

void QCustomPlotAdapter::rebuild(const std::vector<std::vector<double>>& x,
                                 const std::vector<std::vector<double>>& y,
                                 const PlotSpec& spec)
{
    m_plot->clearGraphs();
    m_graphs.clear();

    for (size_t c = 0; c < x.size(); ++c) {
        QCPGraph* graph = m_plot->addGraph();
        graph->setPen(QPen(QColor(31, 119, 180), 1.0));
        graph->setAdaptiveSampling(m_sampling == QcpSampling::Adaptive);
        QVector<double> keys(x[c].begin(), x[c].end());
        QVector<double> values(y[c].begin(), y[c].end());
        graph->setData(keys, values, true);
        m_graphs.push_back(graph);
    }

    m_plot->setAntialiasedElements(spec.antialiasing ? QCP::aePlottables : QCP::aeNone);
    setAxesRange(spec.xMin, spec.xMax, spec.yMin, spec.yMax);
}

void QCustomPlotAdapter::updateWindow(int index, const std::vector<double>& x,
                                      const std::vector<double>& y, int offset, int count)
{
    if (index < 0 || index >= static_cast<int>(m_graphs.size()))
        return;

    QVector<double> keys(count);
    QVector<double> values(count);
    std::copy(x.begin() + offset, x.begin() + offset + count, keys.begin());
    std::copy(y.begin() + offset, y.begin() + offset + count, values.begin());
    m_graphs[index]->setData(keys, values, true);
}

void QCustomPlotAdapter::setAxesRange(double xMin, double xMax, double yMin, double yMax)
{
    if (!m_plot)
        return;
    m_plot->xAxis->setRange(xMin, xMax);
    m_plot->yAxis->setRange(yMin, yMax);
}

void QCustomPlotAdapter::rescaleAxes()
{
    if (!m_plot)
        return;
    m_plot->rescaleAxes(false);
}

void QCustomPlotAdapter::frame()
{
    // rpImmediateRefresh draws every layer into the paint buffer and repaints
    // the widget synchronously.
    m_plot->replot(QCustomPlot::rpImmediateRefresh);

#ifdef QCUSTOMPLOT_USE_OPENGL
    if (m_backend == Backend::OpenGL) {
        if (QOpenGLContext* context = QOpenGLContext::currentContext()) {
            if (QOpenGLFunctions* functions = context->functions())
                functions->glFinish();
        }
    }
#endif
    ++m_frames;
}

QImage QCustomPlotAdapter::exportWidget()
{
    const QPixmap pixmap = m_plot->toPixmap(m_plot->width(), m_plot->height(), 1.0);
    return pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

} // namespace chartbench
