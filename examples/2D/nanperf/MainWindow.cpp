#include "MainWindow.h"
#include "PlotPanel.h"
#include "FilterModes.h"
#include "BenchmarkRunner.h"

#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QProgressBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLabel>
#include <QApplication>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_modeCombo(new QComboBox(this))
    , m_pointsSpin(new QSpinBox(this))
    , m_nanFractionSpin(new QDoubleSpinBox(this))
    , m_repeatsSpin(new QSpinBox(this))
    , m_applyBtn(new QPushButton(QStringLiteral("Apply & Redraw"), this))
    , m_sweepBtn(new QPushButton(QStringLiteral("Run Benchmark Sweep"), this))
    , m_exportBtn(new QPushButton(QStringLiteral("Export Markdown"), this))
    , m_progress(new QProgressBar(this))
    , m_table(new QTableWidget(this))
    , m_analysisLabel(new QLabel(this))
    , m_runner(nullptr)
{
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({ QStringLiteral("Case"),
                                         QStringLiteral("Mode"),
                                         QStringLiteral("boundingRect(ms)"),
                                         QStringLiteral("replot(ms)"),
                                         QStringLiteral("FPS"),
                                         QStringLiteral("NaN Points") });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_analysisLabel->setWordWrap(true);
    m_analysisLabel->setMinimumHeight(80);

    m_progress->setRange(0, 1);
    m_progress->setValue(0);

    const NanCase cases[ 6 ] = { NanCase::Leading, NanCase::LeadingTrailing, NanCase::Middle, NanCase::Trailing, NanCase::XyNan, NanCase::AllNan };
    auto* grid               = new QGridLayout;
    for (int i = 0; i < 6; ++i) {
        auto* panel = new PlotPanel(cases[ i ]);
        m_panels.append(panel);
        grid->addWidget(panel, i / 3, i % 3);
    }
    m_runner = new BenchmarkRunner(m_panels, this);
    connect(m_runner, &BenchmarkRunner::progress, this, &MainWindow::onProgress);
    connect(m_runner, &BenchmarkRunner::finished, this, &MainWindow::onSweepFinished);

    auto* central = new QWidget(this);
    auto* outer   = new QVBoxLayout(central);
    outer->addLayout(buildControls());
    outer->addWidget(m_progress);
    outer->addLayout(grid, 1);
    outer->addWidget(new QLabel(QStringLiteral("Metrics:")));
    outer->addWidget(m_table, 1);
    outer->addWidget(new QLabel(QStringLiteral("Bottleneck Analysis:")));
    outer->addWidget(m_analysisLabel);
    setCentralWidget(central);

    refreshPanels();
    setWindowTitle(QStringLiteral("nanperf - NaN Rendering & Performance"));
    resize(1280, 960);
}

QLayout* MainWindow::buildControls()
{
    const auto modes = filterModes();
    for (const auto& m : modes)
        m_modeCombo->addItem(m.label);
    m_modeCombo->setCurrentIndex(modes.size() - 1);

    m_pointsSpin->setRange(1000, 10000000);
    m_pointsSpin->setSingleStep(10000);
    m_pointsSpin->setValue(100000);

    m_nanFractionSpin->setRange(0.0, 0.99);
    m_nanFractionSpin->setSingleStep(0.05);
    m_nanFractionSpin->setValue(0.5);

    m_repeatsSpin->setRange(1, 200);
    m_repeatsSpin->setValue(20);

    auto* bar = new QHBoxLayout;
    bar->addWidget(new QLabel(QStringLiteral("Mode:")));
    bar->addWidget(m_modeCombo);
    bar->addWidget(new QLabel(QStringLiteral("Points:")));
    bar->addWidget(m_pointsSpin);
    bar->addWidget(new QLabel(QStringLiteral("NaN Ratio:")));
    bar->addWidget(m_nanFractionSpin);
    bar->addWidget(new QLabel(QStringLiteral("Repeats:")));
    bar->addWidget(m_repeatsSpin);
    bar->addWidget(m_applyBtn);
    bar->addWidget(m_sweepBtn);
    bar->addWidget(m_exportBtn);
    bar->addStretch(1);

    connect(m_applyBtn, &QPushButton::clicked, this, &MainWindow::onApplyRedraw);
    connect(m_sweepBtn, &QPushButton::clicked, this, &MainWindow::onRunSweep);
    connect(m_exportBtn, &QPushButton::clicked, this, &MainWindow::onExport);
    return bar;
}

void MainWindow::refreshPanels()
{
    const int n    = m_pointsSpin->value();
    const double f = m_nanFractionSpin->value();
    const int mode = m_modeCombo->currentIndex();
    for (auto* panel : qwt_as_const(m_panels)) {
        panel->apply(n, f, mode);
    }
}

void MainWindow::onApplyRedraw()
{
    refreshPanels();
}

void MainWindow::onRunSweep()
{
    m_sweepBtn->setEnabled(false);
    m_applyBtn->setEnabled(false);
    m_exportBtn->setEnabled(false);
    m_progress->setRange(0, m_panels.size() * filterModes().size() + filterModes().size());
    m_progress->setValue(0);
    m_analysisLabel->setText(QStringLiteral("Sweeping..."));
    QApplication::processEvents();

    m_runner->runSweep(m_pointsSpin->value(), m_nanFractionSpin->value(), m_repeatsSpin->value());
    // finished() handled synchronously above via direct connection.
}

void MainWindow::onProgress(int done, int total)
{
    m_progress->setRange(0, total);
    m_progress->setValue(done);
}

void MainWindow::onSweepFinished(const QVector< BenchResult >& results)
{
    m_lastResults = results;
    fillTable(results);
    m_analysisLabel->setText(BenchmarkRunner::analyze(results));
    refreshPanels();  // restore panel 0 (was used for baseline) to its own case
    m_sweepBtn->setEnabled(true);
    m_applyBtn->setEnabled(true);
    m_exportBtn->setEnabled(true);
}

void MainWindow::fillTable(const QVector< BenchResult >& results)
{
    m_table->setRowCount(results.size());
    for (int i = 0; i < results.size(); ++i) {
        const BenchResult& r = results[ i ];
        m_table->setItem(i, 0, new QTableWidgetItem(r.caseLabel));
        m_table->setItem(i, 1, new QTableWidgetItem(r.modeLabel));
        m_table->setItem(i, 2, new QTableWidgetItem(QString::number(r.boundingRectMs, 'f', 3)));
        m_table->setItem(i, 3, new QTableWidgetItem(QString::number(r.replotMs, 'f', 3)));
        m_table->setItem(i, 4, new QTableWidgetItem(QString::number(r.fps, 'f', 1)));
        m_table->setItem(i, 5, new QTableWidgetItem(QString::number(r.nanPoints)));
    }
}

void MainWindow::onExport()
{
    if (m_lastResults.isEmpty())
        return;
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export Markdown"), QStringLiteral("nanperf-report.md"), QStringLiteral("Markdown (*.md)")
    );
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream out(&file);
    out << "# nanperf benchmark report\n\n";
    out << "| Case | Mode | boundingRect(ms) | replot(ms) | FPS | NaN Points |\n";
    out << "|------|------|------------------|------------|-----|--------|\n";
    for (const auto& r : qwt_as_const(m_lastResults)) {
        out << QString("| %1 | %2 | %3 | %4 | %5 | %6 |\n")
                   .arg(r.caseLabel)
                   .arg(r.modeLabel)
                   .arg(r.boundingRectMs, 0, 'f', 3)
                   .arg(r.replotMs, 0, 'f', 3)
                   .arg(r.fps, 0, 'f', 1)
                   .arg(r.nanPoints);
    }
    out << "\n" << BenchmarkRunner::analyze(m_lastResults);
}
