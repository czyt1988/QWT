#include "MainWindow.h"

#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_plot_layout.h"
#include "qwt_plot_renderer.h"
#include "qwt_scale_widget.h"
#include "qwt_text_label.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QSplitter>
#include <QStatusBar>
#include <QTextStream>
#include <QToolBar>
#include <QtMath>

namespace
{
// Generate sine sample data
QVector< QPointF > generateSineData(int count, double amplitude, double frequency, double phase = 0.0)
{
    QVector< QPointF > samples;
    samples.reserve(count);
    for (int i = 0; i < count; ++i) {
        const double x = i * 0.1;
        samples.append(QPointF(x, amplitude * qSin(frequency * x + phase)));
    }
    return samples;
}

// Create an antialiased curve and attach it to a plot
QwtPlotCurve* attachCurve(QwtPlot* plot, const QString& name, const QVector< QPointF >& samples, const QColor& color)
{
    QwtPlotCurve* curve = new QwtPlotCurve(name);
    curve->setSamples(samples);
    curve->setPen(color, 1.5);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curve->attach(plot);
    return curve;
}
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), m_singlePlot(nullptr), m_hostPlot(nullptr)
{
    setWindowTitle("Outside Axis Titles");

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    m_singlePlot        = createSinglePlot(splitter);
    m_hostPlot          = createMultiAxisPlot(splitter);
    setCentralWidget(splitter);

    QToolBar* toolBar = addToolBar("Tools");
    toolBar->setObjectName("ToolsToolBar");

    QAction* actOutsideY = toolBar->addAction("Y Titles Outside");
    actOutsideY->setCheckable(true);
    actOutsideY->setChecked(true);
    connect(actOutsideY, &QAction::toggled, this, &MainWindow::applyOutsideYTitles);

    QAction* actOutsideX = toolBar->addAction("X Titles Outside");
    actOutsideX->setCheckable(true);
    connect(actOutsideX, &QAction::toggled, this, &MainWindow::applyOutsideXTitles);

    toolBar->addSeparator();

    QAction* actPng = toolBar->addAction("Export PNG");
    connect(actPng, &QAction::triggered, this, [this]() { exportPlot("png"); });

    QAction* actSvg = toolBar->addAction("Export SVG");
    connect(actSvg, &QAction::triggered, this, [this]() { exportPlot("svg"); });

    // Apply the initial toolbar state
    applyOutsideYTitles(true);
    applyOutsideXTitles(false);

    statusBar()->showMessage(
        "Vertical axis titles are painted horizontally below their scale widgets "
        "(QwtScaleWidget::TitleOutside). Resize the window - the captions follow the layout.");
    resize(1200, 560);
}

QwtPlot* MainWindow::createSinglePlot(QWidget* parent)
{
    QwtPlot* plot = new QwtPlot(parent);
    plot->setTitle("Single Plot");
    plot->setCanvasBackground(Qt::white);

    QwtPlotGrid* grid = new QwtPlotGrid();
    grid->setPen(Qt::gray, 0.0, Qt::DotLine);
    grid->attach(plot);

    plot->enableAxis(QwtAxis::YRight, true);

    const QColor voltageColor(31, 119, 180);
    const QColor currentColor(214, 39, 40);

    attachCurve(plot, "Voltage", generateSineData(100, 1.2, 0.8), voltageColor);
    QwtPlotCurve* currentCurve = attachCurve(plot, "Current", generateSineData(100, 60.0, 1.6, M_PI / 3.0), currentColor);
    currentCurve->setYAxis(QwtAxis::YRight);

    plot->setAxisTitle(QwtAxis::XBottom, "Time (s)");
    plot->setAxisTitle(QwtAxis::YLeft, "U (V)");
    plot->setAxisTitle(QwtAxis::YRight, "I (mA)");

    plot->axisWidget(QwtAxis::YRight)->setTextColor(currentColor);
    plot->axisWidget(QwtAxis::YRight)->setScaleColor(currentColor);

    plot->replot();
    return plot;
}

QwtPlot* MainWindow::createMultiAxisPlot(QWidget* parent)
{
    QwtPlot* hostPlot = new QwtPlot(parent);
    hostPlot->setTitle("Multi Axis Plot (host + 2 parasites)");
    hostPlot->setCanvasBackground(Qt::white);

    QwtPlotGrid* grid = new QwtPlotGrid();
    grid->setPen(Qt::gray, 0.0, Qt::DotLine);
    grid->attach(hostPlot);

    const QColor hostColor(31, 119, 180);
    attachCurve(hostPlot, "Host Sine", generateSineData(100, 1.2, 0.8), hostColor);
    hostPlot->setAxisTitle(QwtAxis::XBottom, "Frequency (Hz)");
    hostPlot->setAxisTitle(QwtAxis::YLeft, "v (m/s)");
    hostPlot->axisWidget(QwtAxis::YLeft)->setTextColor(hostColor);
    hostPlot->axisWidget(QwtAxis::YLeft)->setScaleColor(hostColor);

    // The host owns the axis bands: a parasite column can only be placed where
    // the host has a visible axis, so enable YRight on the host as well
    const QColor hostRightColor(148, 103, 189);
    hostPlot->enableAxis(QwtAxis::YRight, true);
    hostPlot->setAxisTitle(QwtAxis::YRight, "v2 (m/s)");
    hostPlot->axisWidget(QwtAxis::YRight)->setTextColor(hostRightColor);
    hostPlot->axisWidget(QwtAxis::YRight)->setScaleColor(hostRightColor);

    // Parasite 1: an additional YLeft column
    QwtPlot* parasite1 = hostPlot->createParasitePlot(QwtAxis::YLeft);
    parasite1->setParasiteShareAxis(QwtAxis::XBottom);
    const QColor p1Color(44, 160, 44);
    attachCurve(parasite1, "Pressure", generateSineData(100, 2000.0, 2.3), p1Color);
    parasite1->setAxisTitle(QwtAxis::YLeft, "p (kPa)");
    parasite1->axisWidget(QwtAxis::YLeft)->setTextColor(p1Color);
    parasite1->axisWidget(QwtAxis::YLeft)->setScaleColor(p1Color);

    // Parasite 2: an additional YRight column
    QwtPlot* parasite2 = hostPlot->createParasitePlot(QwtAxis::YRight);
    parasite2->setParasiteShareAxis(QwtAxis::XBottom);
    const QColor p2Color(255, 127, 14);
    QwtPlotCurve* p2Curve = attachCurve(parasite2, "Temperature", generateSineData(100, 85.0, 0.5, M_PI / 2.0), p2Color);
    p2Curve->setYAxis(QwtAxis::YRight);
    parasite2->setAxisTitle(QwtAxis::YRight, "T (K)");
    parasite2->axisWidget(QwtAxis::YRight)->setTextColor(p2Color);
    parasite2->axisWidget(QwtAxis::YRight)->setScaleColor(p2Color);

    hostPlot->replotAll();
    return hostPlot;
}

QList< QwtPlot* > MainWindow::allPlots() const
{
    QList< QwtPlot* > plots;
    plots << m_singlePlot << m_hostPlot;
    if (m_hostPlot)
        plots += m_hostPlot->parasitePlots();
    return plots;
}

void MainWindow::applyOutsideYTitles(bool on)
{
    const QwtScaleWidget::TitlePlacement placement = on ? QwtScaleWidget::TitleOutside : QwtScaleWidget::TitleInside;

    const QList< QwtPlot* > plots = allPlots();
    for (QwtPlot* plot : plots) {
        if (!plot)
            continue;
        plot->setAxisTitlePlacement(QwtAxis::YLeft, placement);
        plot->setAxisTitlePlacement(QwtAxis::YRight, placement);
    }

    // replotAll recalculates the layers (edgeMargin) of host and parasites
    m_singlePlot->replot();
    m_hostPlot->replotAll();
}

void MainWindow::applyOutsideXTitles(bool on)
{
    const QwtScaleWidget::TitlePlacement placement = on ? QwtScaleWidget::TitleOutside : QwtScaleWidget::TitleInside;

    const QList< QwtPlot* > plots = allPlots();
    for (QwtPlot* plot : plots) {
        if (!plot)
            continue;
        plot->setAxisTitlePlacement(QwtAxis::XBottom, placement);
    }

    m_singlePlot->replot();
    m_hostPlot->replotAll();
}

void MainWindow::exportPlot(const QString& format)
{
    const QString defaultName = QStringLiteral("outsideTitle.%1").arg(format);
    const QString filter      = (format == "svg") ? QStringLiteral("SVG files (*.svg)") : QStringLiteral("PNG images (*.png)");

    const QString fileName = QFileDialog::getSaveFileName(this, QStringLiteral("Export Single Plot"), defaultName, filter);
    if (fileName.isEmpty())
        return;

    QwtPlotRenderer renderer;
    renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground, false);
    renderer.renderDocument(m_singlePlot, fileName, QSizeF(300, 200), 85);

    statusBar()->showMessage(QStringLiteral("Exported: %1").arg(fileName), 5000);
}

bool MainWindow::runSelfTest(const QString& dir)
{
    bool ok = true;

    const auto grabToFile = [ & ](QWidget* w, const QString& name) {
        const QString fileName = dir + QLatin1Char('/') + name;
        if (!w->grab().save(fileName)) {
            qCritical("runSelfTest: failed to save %s", qPrintable(fileName));
            ok = false;
        }
    };

    // Process pending layout/paint events before grabbing
    QApplication::processEvents();

    // 1. Initial state: Y titles outside
    grabToFile(this, "01_outside.png");

    // 2. Renderer export of the single plot (PNG + SVG)
    {
        QwtPlotRenderer renderer;
        renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground, false);
        renderer.renderDocument(m_singlePlot, dir + "/02_single_render.png", QSizeF(300, 200), 85);
        renderer.renderDocument(m_singlePlot, dir + "/03_single_render.svg", QSizeF(300, 200), 85);
        renderer.renderDocument(m_hostPlot, dir + "/04_multi_render.png", QSizeF(300, 200), 85);
    }

    // 3. Toggle back to inside titles (runtime switch, exercises the
    //    LayoutRequest chain incl. parasite -> host forwarding)
    applyOutsideYTitles(false);
    QApplication::processEvents();
    grabToFile(this, "05_inside.png");

    // 4. Toggle to outside again and resize down: the captions must follow
    //    the scale widgets and the bottom band must be reserved
    applyOutsideYTitles(true);
    resize(760, 420);
    QApplication::processEvents();
    grabToFile(this, "06_outside_small.png");

    // 5. X bottom titles outside as well
    applyOutsideXTitles(true);
    QApplication::processEvents();
    grabToFile(this, "07_outside_xy.png");

    return ok;
}
