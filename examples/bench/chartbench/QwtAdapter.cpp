#include "QwtAdapter.h"

#include <qwt_plot.h>
#include <qwt_plot_abstract_canvas.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_series_data.h>

#ifdef CHARTBENCH_HAVE_QWT_OPENGL
#include <qwt_plot_opengl_canvas.h>
#endif

#include <QImage>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QPainter>
#include <QVBoxLayout>

namespace chartbench
{

#ifdef CHARTBENCH_HAVE_QWT_OPENGL
/**
 * @brief QwtPlotOpenGLCanvas with the protected GL entry points exposed.
 *
 * QOpenGLWidget defers repaint() to Qt's composition step, so a plain replot()
 * returns without drawing anything. Driving makeCurrent() + paintGL() is what
 * Qt itself does while composing and is the only synchronous way to get a
 * finished Qwt GL frame.
 */
class BenchGlCanvas : public QwtPlotOpenGLCanvas
{
public:
    explicit BenchGlCanvas(QwtPlot* plot)
        : QwtPlotOpenGLCanvas(plot)
    {
    }

    void beginFrame() { makeCurrent(); }
    void renderFrame() { paintGL(); }
    void finishFrame()
    {
        if (QOpenGLContext* ctx = context()) {
            if (QOpenGLFunctions* functions = ctx->functions())
                functions->glFinish();
        }
        doneCurrent();
    }
};
#endif

QwtAdapter::QwtAdapter(Backend backend)
    : m_backend(backend)
{
}

QwtAdapter::~QwtAdapter()
{
    destroy();
}

QString QwtAdapter::backendDescription() const
{
#ifdef CHARTBENCH_HAVE_QWT_OPENGL
    if (m_backend == Backend::OpenGL)
        return QStringLiteral("QwtPlotOpenGLCanvas (QOpenGLWidget + QPainter on FBO)");
#endif
    return QStringLiteral("QwtPlotCanvas (QWidget + QPainter, backing store)");
}

QString QwtAdapter::downsamplingDescription() const
{
    switch (m_filter) {
    case QwtFilter::FilterPoints:
        return QStringLiteral("FilterPoints");
    case QwtFilter::FilterPointsAggressive:
        return QStringLiteral("FilterPointsAggressive");
    case QwtFilter::FilterPointsPixel:
        return QStringLiteral("FilterPointsPixel (pixel column min/max)");
    case QwtFilter::FilterPointsLTTB:
        return QStringLiteral("FilterPointsLTTB");
    case QwtFilter::FilterPointsMinMaxLTTB:
        return QStringLiteral("FilterPointsMinMaxLTTB");
    case QwtFilter::None:
        break;
    }
    return QStringLiteral("none");
}

QStringList QwtAdapter::downsamplingAlgorithms() const
{
    return QStringList { QStringLiteral("none"),       QStringLiteral("filterpoints"),
                         QStringLiteral("aggressive"), QStringLiteral("pixel"),
                         QStringLiteral("lttb"),       QStringLiteral("minmaxlttb") };
}

bool QwtAdapter::setDownsamplingAlgorithm(const QString& name)
{
    if (name == QLatin1String("none"))
        m_filter = QwtFilter::None;
    else if (name == QLatin1String("filterpoints"))
        m_filter = QwtFilter::FilterPoints;
    else if (name == QLatin1String("aggressive"))
        m_filter = QwtFilter::FilterPointsAggressive;
    else if (name == QLatin1String("pixel"))
        m_filter = QwtFilter::FilterPointsPixel;
    else if (name == QLatin1String("lttb"))
        m_filter = QwtFilter::FilterPointsLTTB;
    else if (name == QLatin1String("minmaxlttb"))
        m_filter = QwtFilter::FilterPointsMinMaxLTTB;
    else
        return false;

    applyFilterAttribute();
    return true;
}

bool QwtAdapter::create(QWidget* parent, const PlotSpec& spec, QString& error)
{
    m_plot = new QwtPlot(parent);
    m_plot->setAutoReplot(false);

    if (m_backend == Backend::OpenGL) {
#ifdef CHARTBENCH_HAVE_QWT_OPENGL
        // setCanvas() takes ownership: it deletes the previous canvas itself.
        m_glCanvas = new BenchGlCanvas(m_plot);
        m_plot->setCanvas(m_glCanvas);
        m_canvas = m_plot->canvas();
#else
        error = QStringLiteral("this Qwt build has no OpenGL canvas (QWT_CONFIG_QWTOPENGL=OFF)");
        return false;
#endif
    } else {
        m_canvas = m_plot->canvas();
    }

    m_plot->setCanvasBackground(Qt::white);
    m_plot->setAxisAutoScale(QwtAxis::XBottom, false);
    m_plot->setAxisAutoScale(QwtAxis::YLeft, false);
    m_plot->setAxisScale(QwtAxis::XBottom, spec.xMin, spec.xMax);
    m_plot->setAxisScale(QwtAxis::YLeft, spec.yMin, spec.yMax);

    // ImmediatePaint turns replot() into a synchronous repaint, which is the
    // documented way to get a finished frame out of a Qwt canvas.
    setImmediatePaint();

    m_plot->resize(spec.widgetSize);
    m_plot->setMinimumSize(spec.widgetSize);
    m_plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return true;
}

void QwtAdapter::destroy()
{
    for (QwtPlotCurve* curve : m_curves)
        delete curve;
    m_curves.clear();
    delete m_plot;
    m_plot = nullptr;
    m_canvas = nullptr;
    m_frames = 0;
}

void QwtAdapter::setImmediatePaint()
{
    // QwtPlotAbstractCanvas is not polymorphic, so the cast has to follow the
    // canvas type that create() installed.
    if (m_backend == Backend::OpenGL) {
#ifdef CHARTBENCH_HAVE_QWT_OPENGL
        static_cast<QwtPlotOpenGLCanvas*>(m_canvas)
            ->setPaintAttribute(QwtPlotAbstractGLCanvas::ImmediatePaint, true);
#endif
    } else {
        static_cast<QwtPlotCanvas*>(m_canvas)
            ->setPaintAttribute(QwtPlotCanvas::ImmediatePaint, true);
    }
}

void QwtAdapter::applyFilterAttribute()
{
    int attributes = 0;
    switch (m_filter) {
    case QwtFilter::FilterPoints:
        attributes = QwtPlotCurve::FilterPoints;
        break;
    case QwtFilter::FilterPointsAggressive:
        attributes = QwtPlotCurve::FilterPoints | QwtPlotCurve::FilterPointsAggressive;
        break;
    case QwtFilter::FilterPointsPixel:
        attributes = QwtPlotCurve::FilterPoints | QwtPlotCurve::FilterPointsPixel;
        break;
    case QwtFilter::FilterPointsLTTB:
        attributes = QwtPlotCurve::FilterPoints | QwtPlotCurve::FilterPointsLTTB;
        break;
    case QwtFilter::FilterPointsMinMaxLTTB:
        attributes = QwtPlotCurve::FilterPoints | QwtPlotCurve::FilterPointsMinMaxLTTB;
        break;
    case QwtFilter::None:
        break;
    }
    for (QwtPlotCurve* curve : m_curves) {
        curve->setPaintAttribute(QwtPlotCurve::FilterPoints, attributes & QwtPlotCurve::FilterPoints);
        curve->setPaintAttribute(QwtPlotCurve::FilterPointsAggressive,
                                 attributes & QwtPlotCurve::FilterPointsAggressive);
        curve->setPaintAttribute(QwtPlotCurve::FilterPointsPixel,
                                 attributes & QwtPlotCurve::FilterPointsPixel);
        curve->setPaintAttribute(QwtPlotCurve::FilterPointsLTTB,
                                 attributes & QwtPlotCurve::FilterPointsLTTB);
        curve->setPaintAttribute(QwtPlotCurve::FilterPointsMinMaxLTTB,
                                 attributes & QwtPlotCurve::FilterPointsMinMaxLTTB);
    }
}

void QwtAdapter::rebuild(const std::vector<std::vector<double>>& x,
                         const std::vector<std::vector<double>>& y,
                         const PlotSpec& spec)
{
    for (QwtPlotCurve* curve : m_curves)
        delete curve;
    m_curves.clear();

    for (size_t c = 0; c < x.size(); ++c) {
        QwtPlotCurve* curve = new QwtPlotCurve();
        curve->setPen(QPen(QColor(31, 119, 180), 1.0));
        curve->setRenderHint(QwtPlotItem::RenderAntialiased, spec.antialiasing);
        curve->setSamples(x[c].data(), y[c].data(), static_cast<int>(x[c].size()));
        curve->attach(m_plot);
        m_curves.push_back(curve);
    }

    applyFilterAttribute();
    setAxesRange(spec.xMin, spec.xMax, spec.yMin, spec.yMax);
}

void QwtAdapter::updateWindow(int index, const std::vector<double>& x,
                              const std::vector<double>& y, int offset, int count)
{
    if (index < 0 || index >= static_cast<int>(m_curves.size()))
        return;

    QVector<double> xWindow(count);
    QVector<double> yWindow(count);
    std::copy(x.begin() + offset, x.begin() + offset + count, xWindow.begin());
    std::copy(y.begin() + offset, y.begin() + offset + count, yWindow.begin());
    m_curves[index]->setSamples(std::move(xWindow), std::move(yWindow));
}

void QwtAdapter::setAxesRange(double xMin, double xMax, double yMin, double yMax)
{
    if (!m_plot)
        return;
    m_plot->setAxisAutoScale(QwtAxis::XBottom, false);
    m_plot->setAxisAutoScale(QwtAxis::YLeft, false);
    m_plot->setAxisScale(QwtAxis::XBottom, xMin, xMax);
    m_plot->setAxisScale(QwtAxis::YLeft, yMin, yMax);
}

void QwtAdapter::rescaleAxes()
{
    if (!m_plot)
        return;
    for (QwtPlotCurve* curve : m_curves)
        curve->setItemAttribute(QwtPlotItem::AutoScale, true);
    m_plot->setAxisAutoScale(QwtAxis::XBottom, true);
    m_plot->setAxisAutoScale(QwtAxis::YLeft, true);
}

void QwtAdapter::frame()
{
    m_gpuSyncMs = 0.0;

#ifdef CHARTBENCH_HAVE_QWT_OPENGL
    if (m_backend == Backend::OpenGL) {
        // The plot layout (axes, scales) is computed by replot(); the canvas
        // then draws itself when Qt composes, which is what we drive here.
        m_plot->replot();

        if (m_glCanvas->context()) {
            m_glCanvas->beginFrame();
            m_glCanvas->renderFrame();
            const Stopwatch gpuWatch;
            m_glCanvas->finishFrame();
            m_gpuSyncMs = gpuWatch.elapsedMs();
        }
        ++m_frames;
        return;
    }
#endif

    m_plot->replot();
    ++m_frames;
}

QImage QwtAdapter::exportWidget()
{
    QImage image(m_plot->size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    {
        QPainter painter(&image);
        m_plot->render(&painter);
    }
    return image;
}

} // namespace chartbench
