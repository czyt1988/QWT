/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include "MainWindow.h"
#include "Plot.h"
#include "ControlPanel.h"
#include "WindowedSeriesData.h"
#include "ReportDialog.h"

#include <qwt_version_info.h>

#include <QStatusBar>
#include <QHBoxLayout>
#include <QTimer>
#include <QApplication>
#include <QElapsedTimer>

static const char* s_methodNames[] = {
    "None",
    "FilterPoints",
    "FilterPointsAggressive",
    "FilterPointsPixel",
    "FilterPointsLTTB",
    "FilterPointsMinMaxLTTB"};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_panel = new ControlPanel();
    m_plot  = new Plot();

    m_seriesData = new WindowedSeriesData();
    m_plot->setCurveData(m_seriesData);

    QWidget* w = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(w);
    layout->addWidget(m_panel);
    layout->addWidget(m_plot, 10);
    setCentralWidget(w);

    statusBar()->showMessage("Ready");

    connect(m_panel, &ControlPanel::runRequested, this, &MainWindow::startBenchmark);
    connect(m_panel, &ControlPanel::runAllRequested, this, &MainWindow::startBenchmarkAll);
}

QString MainWindow::methodName(int index) const
{
    if (index >= 0 && index < 5)
        return s_methodNames[ index ];
    return "Unknown";
}

void MainWindow::generateData(int numPoints, int waveType)
{
    const int totalSize = static_cast< int >(numPoints * 1.2);
    m_seriesData->generate(totalSize, waveType == 0);
}

void MainWindow::startBenchmark()
{
    const int numPoints  = m_panel->numPoints();
    const int frameCount = m_panel->frameCount();
    const int waveType   = m_panel->waveType();
    const int method     = m_panel->methodIndex();

    generateData(numPoints, waveType);

    m_displaySize   = numPoints;
    m_targetFrames  = frameCount;
    m_currentFrame  = 0;
    m_currentOffset = 0;
    m_step          = qMax(1, m_seriesData->maxOffset(numPoints) / frameCount);
    m_frameTimes.clear();
    m_frameTimes.reserve(frameCount);
    m_batchMode = false;

    m_plot->setRenderingMethod(method);
    m_plot->setAxisScale(QwtAxis::XBottom, 0, numPoints);
    m_plot->setAxisAutoScale(QwtAxis::YLeft);

    m_panel->setRunning(true);
    m_panel->setProgress(0, frameCount);
    m_panel->setStatus(QString("Running: %1 ...").arg(methodName(method)));

    QTimer::singleShot(0, this, &MainWindow::onTimerTick);
}

void MainWindow::startBenchmarkAll()
{
    const int numPoints  = m_panel->numPoints();
    const int waveType   = m_panel->waveType();

    generateData(numPoints, waveType);

    m_batchMode        = true;
    m_batchMethodIndex = 0;
    m_batchResults.clear();

    m_displaySize   = numPoints;
    m_targetFrames  = m_panel->frameCount();
    m_currentFrame  = 0;
    m_currentOffset = 0;
    m_step          = qMax(1, m_seriesData->maxOffset(numPoints) / m_targetFrames);
    m_frameTimes.clear();
    m_frameTimes.reserve(m_targetFrames);

    m_plot->setRenderingMethod(0);
    m_plot->setAxisScale(QwtAxis::XBottom, 0, numPoints);
    m_plot->setAxisAutoScale(QwtAxis::YLeft);

    m_panel->setRunning(true);
    m_panel->setProgress(0, m_targetFrames);
    m_panel->setStatus(QString("Batch: %1 (%2/5) ...")
                           .arg(methodName(0))
                           .arg(1));

    QTimer::singleShot(0, this, &MainWindow::onTimerTick);
}

void MainWindow::onTimerTick()
{
    if (m_currentFrame >= m_targetFrames) {
        BenchmarkResult result;
        result.methodName = methodName(m_batchMode ? m_batchMethodIndex
                                                   : m_panel->methodIndex());
        result.numPoints  = m_displaySize;
        result.frameCount = m_targetFrames;
        result.waveType   = m_panel->waveType();
        result.totalTime  = 0;
        result.frameTimes = m_frameTimes;

        if (!m_frameTimes.isEmpty()) {
            double sum = 0, minT = m_frameTimes.first(), maxT = 0;
            for (double t : qAsConst(m_frameTimes)) {
                sum += t;
                if (t < minT)
                    minT = t;
                if (t > maxT)
                    maxT = t;
            }
            result.totalTime = sum;
            result.avgTime   = sum / m_frameTimes.size();
            result.minTime   = minT;
            result.maxTime   = maxT;
            result.fps       = (sum > 0) ? (m_frameTimes.size() * 1000.0 / sum) : 0;
        }

        if (m_batchMode) {
            m_batchResults.append(result);
            m_batchMethodIndex++;

            if (m_batchMethodIndex < 5) {
                m_currentFrame  = 0;
                m_currentOffset = 0;
                m_frameTimes.clear();
                m_frameTimes.reserve(m_targetFrames);

                m_plot->setRenderingMethod(m_batchMethodIndex);

                m_panel->setProgress(0, m_targetFrames);
                m_panel->setStatus(QString("Batch: %1 (%2/5) ...")
                                       .arg(methodName(m_batchMethodIndex))
                                       .arg(m_batchMethodIndex + 1));

                QTimer::singleShot(0, this, &MainWindow::onTimerTick);
                return;
            }

            m_panel->setRunning(false);
            m_panel->setProgress(m_targetFrames, m_targetFrames);
            m_panel->setStatus("Batch complete");
            statusBar()->showMessage(QString("Batch complete | %1 methods tested")
                                         .arg(m_batchResults.size()));

            showReport(buildCompareReport(m_batchResults));
            return;
        }

        m_panel->setRunning(false);
        m_panel->setProgress(m_targetFrames, m_targetFrames);
        m_panel->setStatus(QString("Done: %1 ms total, %2 FPS")
                               .arg(result.totalTime, 0, 'f', 1)
                               .arg(result.fps, 0, 'f', 1));
        statusBar()->showMessage(QString("Done: %1 ms total | %2 FPS")
                                     .arg(result.totalTime, 0, 'f', 1)
                                     .arg(result.fps, 0, 'f', 1));

        showReport(buildReport(result));
        return;
    }

    m_seriesData->setWindow(m_currentOffset, m_displaySize);
    // Make the X-axis follow the sliding window so the curve always fills the
    // canvas width and the axis labels scroll with the data.
    m_plot->setAxisScale(QwtAxis::XBottom,
        m_currentOffset, m_currentOffset + m_displaySize);

    m_currentOffset += m_step;

    if (m_currentOffset > m_seriesData->maxOffset(m_displaySize))
        m_currentOffset = 0;

    m_frameTimer.start();
    m_plot->replot();
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    const double elapsed = m_frameTimer.elapsed();

    m_frameTimes.append(elapsed);
    m_currentFrame++;

    m_panel->setProgress(m_currentFrame, m_targetFrames);

    const double instantFps = (elapsed > 0) ? (1000.0 / elapsed) : 0;
    statusBar()->showMessage(
        QString("Frame %1/%2 | %3 ms | %4 FPS")
            .arg(m_currentFrame)
            .arg(m_targetFrames)
            .arg(elapsed, 0, 'f', 1)
            .arg(instantFps, 0, 'f', 1));

    QTimer::singleShot(0, this, &MainWindow::onTimerTick);
}

QString MainWindow::buildReport(const BenchmarkResult& r) const
{
    const QSize canvasSize = m_plot->canvas()->size();

    QString md;
    md += "# Render Benchmark Report\n\n";

    md += "## Environment\n\n";
    md += "| Item | Value |\n";
    md += "|------|-------|\n";
    md += QString("| Qwt Version | %1 |\n").arg(QWT_VERSION_STR);
    md += QString("| Qt Version | %1 |\n").arg(QT_VERSION_STR);
    md += QString("| Canvas Size | %1 x %2 px |\n\n")
              .arg(canvasSize.width())
              .arg(canvasSize.height());

    md += "## Parameters\n\n";
    md += "| Parameter | Value |\n";
    md += "|-----------|-------|\n";
    md += QString("| Rendering Method | %1 |\n").arg(r.methodName);
    md += QString("| Data Points (Display) | %1 |\n").arg(r.numPoints);
    md += QString("| Total Data (Buffer) | %1 |\n").arg(m_seriesData->totalSize());
    md += QString("| Frame Count | %1 |\n").arg(r.frameCount);
    md += QString("| Wave Type | %1 |\n\n").arg(r.waveType == 0 ? "Wave" : "Noise");

    md += "## Results\n\n";
    md += "| Metric | Value |\n";
    md += "|--------|-------|\n";
    md += QString("| Total Time | %1 ms |\n").arg(r.totalTime, 0, 'f', 1);
    md += QString("| Average Frame Time | %1 ms |\n").arg(r.avgTime, 0, 'f', 2);
    md += QString("| Min Frame Time | %1 ms |\n").arg(r.minTime, 0, 'f', 2);
    md += QString("| Max Frame Time | %1 ms |\n").arg(r.maxTime, 0, 'f', 2);
    md += QString("| FPS | %1 |\n").arg(r.fps, 0, 'f', 1);

    return md;
}

QString MainWindow::buildCompareReport(const QVector< BenchmarkResult >& results) const
{
    const QSize canvasSize = m_plot->canvas()->size();

    QString md;
    md += "# Render Benchmark Comparison\n\n";

    md += "## Environment\n\n";
    md += "| Item | Value |\n";
    md += "|------|-------|\n";
    md += QString("| Qwt Version | %1 |\n").arg(QWT_VERSION_STR);
    md += QString("| Qt Version | %1 |\n").arg(QT_VERSION_STR);
    md += QString("| Canvas Size | %1 x %2 px |\n\n")
              .arg(canvasSize.width())
              .arg(canvasSize.height());

    if (!results.isEmpty()) {
        md += "## Parameters\n\n";
        md += QString("- Data Points: %1 | Frames: %2 | Wave: %3\n\n")
                  .arg(results.first().numPoints)
                  .arg(results.first().frameCount)
                  .arg(results.first().waveType == 0 ? "Wave" : "Noise");
    }

    md += "## Comparison\n\n";
    md += "| Method | Total (ms) | Avg Frame (ms) | Min (ms) | Max (ms) | FPS |\n";
    md += "|--------|------------|----------------|----------|----------|-----|\n";

    for (const auto& r : results) {
        md += QString("| %1 | %2 | %3 | %4 | %5 | %6 |\n")
                  .arg(r.methodName)
                  .arg(r.totalTime, 0, 'f', 1)
                  .arg(r.avgTime, 0, 'f', 2)
                  .arg(r.minTime, 0, 'f', 2)
                  .arg(r.maxTime, 0, 'f', 2)
                  .arg(r.fps, 0, 'f', 1);
    }

    return md;
}

void MainWindow::showReport(const QString& markdown)
{
    ReportDialog* dlg = new ReportDialog(markdown, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

#include "moc_MainWindow.cpp"
