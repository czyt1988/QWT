#include "QImAdapter.h"

#include "QImFigureWidget.h"
#include "plot/QImPlotAxisInfo.h"
#include "plot/QImPlotLineItemNode.h"
#include "plot/QImPlotNode.h"

#include <implot.h>

#include <QImage>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

namespace chartbench
{

/**
 * @brief QImFigureWidget with the protected GL entry points exposed.
 *
 * QOpenGLWidget::repaint() does not run paintGL() synchronously - Qt defers the
 * GL work to the composition step of the backing store. Driving makeCurrent()
 * and paintGL() directly is what QOpenGLWidget itself does when it composes,
 * and it is the only way to time "one complete frame" of QIm.
 */
class BenchQImFigure : public QIM::QImFigureWidget
{
public:
    explicit BenchQImFigure(QWidget* parent)
        : QIM::QImFigureWidget(parent)
    {
    }

    //! Binds the widget's context and framebuffer object.
    void beginFrame() { makeCurrent(); }
    //! Runs the ImGui/ImPlot frame exactly like the composition step would.
    void renderFrame() { paintGL(); }
    //! Blocks until the GPU finished the frame, then releases the context.
    void finishFrame()
    {
        if (QOpenGLContext* ctx = context()) {
            if (QOpenGLFunctions* functions = ctx->functions())
                functions->glFinish();
        }
        doneCurrent();
    }
};

QImAdapter::QImAdapter(Backend backend)
    : m_backend(backend)
{
}

QImAdapter::~QImAdapter()
{
    destroy();
}

QString QImAdapter::backendDescription() const
{
    return QStringLiteral("QOpenGLWidget + Dear ImGui draw lists (ImPlot)");
}

QString QImAdapter::downsamplingDescription() const
{
    switch (m_sampling) {
    case QImSampling::Auto:
        return QStringLiteral("Auto (LTTB >10k, MinMaxLTTB >100k)");
    case QImSampling::LTTB:
        return QStringLiteral("LTTB");
    case QImSampling::MinMaxLTTB:
        return QStringLiteral("MinMaxLTTB");
    case QImSampling::None:
        break;
    }
    return QStringLiteral("none");
}

QStringList QImAdapter::downsamplingAlgorithms() const
{
    return QStringList { QStringLiteral("none"), QStringLiteral("auto"), QStringLiteral("lttb"),
                         QStringLiteral("minmaxlttb") };
}

bool QImAdapter::setDownsamplingAlgorithm(const QString& name)
{
    if (name == QLatin1String("none"))
        m_sampling = QImSampling::None;
    else if (name == QLatin1String("auto"))
        m_sampling = QImSampling::Auto;
    else if (name == QLatin1String("lttb"))
        m_sampling = QImSampling::LTTB;
    else if (name == QLatin1String("minmaxlttb"))
        m_sampling = QImSampling::MinMaxLTTB;
    else
        return false;

    applySampling();
    return true;
}

void QImAdapter::applySampling()
{
    for (QIM::QImPlotLineItemNode* line : m_lines) {
        switch (m_sampling) {
        case QImSampling::Auto:
            line->setDownsampleAlgorithm(QIM::QImDownsampleAlgorithm::Auto);
            break;
        case QImSampling::LTTB:
            line->setDownsampleAlgorithm(QIM::QImDownsampleAlgorithm::LTTB);
            break;
        case QImSampling::MinMaxLTTB:
            line->setDownsampleAlgorithm(QIM::QImDownsampleAlgorithm::MinMaxLTTB);
            break;
        case QImSampling::None:
            line->setDownsampleAlgorithm(QIM::QImDownsampleAlgorithm::None);
            break;
        }
    }
}

bool QImAdapter::create(QWidget* parent, const PlotSpec& spec, QString& error)
{
    if (m_backend != Backend::OpenGL) {
        error = QStringLiteral("QIm always renders through OpenGL - pass --backend=opengl");
        return false;
    }

    m_benchFigure = new BenchQImFigure(parent);
    m_figure = m_benchFigure;
    m_figure->setRenderMode(QIM::QImWidget::RenderOnDemand);
    m_figure->setStyleColorsTheme(QIM::QImWidget::StyleLight);
    m_figure->resize(spec.widgetSize);
    m_figure->setMinimumSize(spec.widgetSize);
    m_figure->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_plot = m_figure->createPlotNode();
    if (!m_plot) {
        error = QStringLiteral("QImFigureWidget::createPlotNode() returned null");
        return false;
    }
    m_plot->setTitleEnabled(false);
    m_plot->setLegendEnabled(false);
    m_plot->setMenusEnabled(false);
    m_plot->setInputsEnabled(true);
    m_plot->x1Axis()->setLabelEnabled(false);
    m_plot->y1Axis()->setLabelEnabled(false);
    return true;
}

void QImAdapter::destroy()
{
    m_lines.clear();
    m_plot = nullptr;
    delete m_figure;
    m_figure = nullptr;
    m_benchFigure = nullptr;
    m_frames = 0;
}

QWidget* QImAdapter::widget() const
{
    return m_figure;
}

void QImAdapter::applyPlotStyle()
{
    // ImPlot keeps its own style object; the QIm wrapper only forwards the
    // ImGui colour theme. White plot background keeps the exported images
    // comparable with the QPainter based libraries.
    if (ImPlot::GetCurrentContext() == nullptr)
        return;

    ImPlotStyle& style = ImPlot::GetStyle();
    style.Colors[ImPlotCol_PlotBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImPlotCol_FrameBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImPlotCol_AxisGrid] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
}

void QImAdapter::rebuild(const std::vector<std::vector<double>>& x,
                         const std::vector<std::vector<double>>& y,
                         const PlotSpec& spec)
{
    m_plot->clearChildrenNodes();   // removes every plot item node
    m_lines.clear();

    for (size_t c = 0; c < x.size(); ++c) {
        QIM::QImPlotLineItemNode* line = m_plot->addLine(x[c], y[c], QStringLiteral("series %1").arg(c));
        if (!line)
            continue;
        line->setColor(QColor(31, 119, 180));
        m_lines.push_back(line);
    }

    applySampling();
    setAxesRange(spec.xMin, spec.xMax, spec.yMin, spec.yMax);
}

void QImAdapter::updateWindow(int index, const std::vector<double>& x,
                              const std::vector<double>& y, int offset, int count)
{
    if (index < 0 || index >= static_cast<int>(m_lines.size()))
        return;

    std::vector<double> xWindow(x.begin() + offset, x.begin() + offset + count);
    std::vector<double> yWindow(y.begin() + offset, y.begin() + offset + count);
    m_lines[index]->setData(xWindow, yWindow);
}

void QImAdapter::setAxesRange(double xMin, double xMax, double yMin, double yMax)
{
    if (!m_plot)
        return;
    m_plot->x1Axis()->setLimits(xMin, xMax, QIM::QImPlotCondition::Always);
    m_plot->y1Axis()->setLimits(yMin, yMax, QIM::QImPlotCondition::Always);
    m_plot->x1Axis()->setAutoFit(false);
    m_plot->y1Axis()->setAutoFit(false);
}

void QImAdapter::rescaleAxes()
{
    if (!m_plot)
        return;
    m_plot->x1Axis()->setAutoFit(true);
    m_plot->y1Axis()->setAutoFit(true);
    m_plot->setAxesToFit();
}

void QImAdapter::frame()
{
    m_gpuSyncMs = 0.0;

    if (!m_benchFigure->context()) {
        // Qt has not created the GL context yet (the widget was never exposed);
        // fall back to a normal repaint so the first frame still initialises.
        m_figure->repaint();
        ++m_frames;
        return;
    }

    if (!m_styleApplied) {
        applyPlotStyle();
        m_styleApplied = true;
    }

    m_benchFigure->beginFrame();
    m_benchFigure->renderFrame();

    const Stopwatch gpuWatch;
    m_benchFigure->finishFrame();
    m_gpuSyncMs = gpuWatch.elapsedMs();

    ++m_frames;
}

QImage QImAdapter::exportWidget()
{
    if (!m_styleApplied) {
        applyPlotStyle();
        m_styleApplied = true;
    }
    return m_figure->grabFramebuffer();
}

} // namespace chartbench
