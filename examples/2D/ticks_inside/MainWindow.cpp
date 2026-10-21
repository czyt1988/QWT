/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include "MainWindow.h"

#include <QwtPlot>
#include <QwtPlotCurve>
#include <QwtPlotGrid>
#include <QwtLegend>
#include <QwtScaleDraw>

#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDoubleSpinBox>

#include <cmath>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_plot(nullptr)
    , m_curve(nullptr)
    , m_curveLog(nullptr)
    , m_curveInverted(nullptr)
    , m_chkYLeft(nullptr)
    , m_chkYRight(nullptr)
    , m_chkXTop(nullptr)
    , m_chkXBottom(nullptr)
    , m_chkLogYRight(nullptr)
    , m_chkInvertedYLeft(nullptr)
    , m_spinPenWidth(nullptr)
{
    setupPlot();

    QWidget* centralWidget  = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    mainLayout->addWidget(m_plot, 1);
    mainLayout->addWidget(createControlPanel());

    setCentralWidget(centralWidget);
    setWindowTitle("Ticks Inside Demo - Log & Inverted Axis Test");
}

void MainWindow::setupPlot()
{
    m_plot = new QwtPlot(this);
    m_plot->setTitle("Tick Direction Demo\n(Log scale on YRight, Inverted scale on YLeft)");
    m_plot->setCanvasBackground(Qt::white);

    // Enable all axes
    m_plot->setAxisVisible(QwtAxis::YLeft, true);
    m_plot->setAxisVisible(QwtAxis::YRight, true);
    m_plot->setAxisVisible(QwtAxis::XTop, true);
    m_plot->setAxisVisible(QwtAxis::XBottom, true);

    // YLeft: Inverted scale (100 -> 0, values decrease upward)
    m_plot->setAxisScale(QwtAxis::YLeft, 100.0, 0.0);
    m_plot->setAxisTitle(QwtAxis::YLeft, "YLeft (Inverted 100->0)");

    // YRight: Logarithmic scale (0.1 -> 1000)
    m_plot->setAxisToLogScale(QwtAxis::YRight);
    m_plot->setAxisScale(QwtAxis::YRight, 0.1, 1000.0);
    m_plot->setAxisTitle(QwtAxis::YRight, "YRight (Log scale)");

    // X axes: Linear scale
    m_plot->setAxisScale(QwtAxis::XBottom, 0.0, 10.0);
    m_plot->setAxisScale(QwtAxis::XTop, 0.0, 10.0);

    // Add legend
    m_plot->insertLegend(new QwtLegend(), QwtPlot::RightLegend);

    // Add grid
    QwtPlotGrid* grid = new QwtPlotGrid();
    grid->setPen(QColor("#c0c0c0"), 0.5, Qt::DotLine);
    grid->attach(m_plot);

    // Add curve for YLeft (inverted axis) - values mapped to inverted YLeft
    m_curveInverted = new QwtPlotCurve();
    m_curveInverted->setTitle("Inverted Axis Curve (YLeft)");
    m_curveInverted->setPen(QColor("#1f77b4"), 2);
    m_curveInverted->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    m_curveInverted->setYAxis(QwtAxis::YLeft);

    QPolygonF pointsInverted;
    for (int i = 0; i <= 10; i++) {
        double x = i;
        double y = 20.0 + 60.0 * (i / 10.0);  // 20 -> 80 (inverted axis shows 80 at bottom)
        pointsInverted << QPointF(x, y);
    }
    m_curveInverted->setSamples(pointsInverted);
    m_curveInverted->attach(m_plot);

    // Add curve for YRight (logarithmic axis) - exponential growth
    m_curveLog = new QwtPlotCurve();
    m_curveLog->setTitle("Log Axis Curve (YRight)");
    m_curveLog->setPen(QColor("#d62728"), 2);
    m_curveLog->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    m_curveLog->setYAxis(QwtAxis::YRight);

    QPolygonF pointsLog;
    for (int i = 0; i <= 10; i++) {
        double x = i;
        double y = 0.1 * pow(10.0, i * 0.3);  // 0.1 -> ~1000
        pointsLog << QPointF(x, y);
    }
    m_curveLog->setSamples(pointsLog);
    m_curveLog->attach(m_plot);

    // Regular curve for XBottom
    m_curve = new QwtPlotCurve();
    m_curve->setTitle("Regular Curve (XBottom)");
    m_curve->setPen(QColor("#2ca02c"), 2);
    m_curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);

    QPolygonF points;
    for (int i = 0; i <= 10; i++) {
        double x = i;
        double y = 4.0 + 3.0 * sin(i * 0.5);
        points << QPointF(x, y);
    }
    m_curve->setSamples(points);
    m_curve->attach(m_plot);
}

QWidget* MainWindow::createControlPanel()
{
    QWidget* panel = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(panel);

    // Tick Direction Group
    QGroupBox* tickGroup = new QGroupBox("Tick Direction", this);
    QVBoxLayout* tickLayout = new QVBoxLayout(tickGroup);

    m_chkYLeft   = new QCheckBox("YLeft Inside (Inverted)", this);
    m_chkYRight  = new QCheckBox("YRight Inside (Log)", this);
    m_chkXTop    = new QCheckBox("XTop Inside", this);
    m_chkXBottom = new QCheckBox("XBottom Inside", this);

    tickLayout->addWidget(m_chkYLeft);
    tickLayout->addWidget(m_chkYRight);
    tickLayout->addWidget(m_chkXTop);
    tickLayout->addWidget(m_chkXBottom);

    tickLayout->addSpacing(10);

    QHBoxLayout* buttonLayout  = new QHBoxLayout();
    QPushButton* btnAllInside  = new QPushButton("All Inside", this);
    QPushButton* btnAllOutside = new QPushButton("All Outside", this);
    buttonLayout->addWidget(btnAllInside);
    buttonLayout->addWidget(btnAllOutside);
    tickLayout->addLayout(buttonLayout);

    layout->addWidget(tickGroup);

    // Axis Type Group
    QGroupBox* axisGroup = new QGroupBox("Axis Scale Type", this);
    QVBoxLayout* axisLayout = new QVBoxLayout(axisGroup);

    m_chkLogYRight = new QCheckBox("YRight Log Scale", this);
    m_chkLogYRight->setChecked(true);  // Default: log scale

    m_chkInvertedYLeft = new QCheckBox("YLeft Inverted (100->0)", this);
    m_chkInvertedYLeft->setChecked(true);  // Default: inverted

    axisLayout->addWidget(m_chkLogYRight);
    axisLayout->addWidget(m_chkInvertedYLeft);

    // Info label
    QLabel* infoLabel = new QLabel(
        "YLeft: Inverted axis (100 at bottom, 0 at top)\n"
        "YRight: Logarithmic scale (0.1 to 1000)\n"
        "\n"
        "Test: Enable 'Inside' for each axis and verify\n"
        "ticks are correctly positioned.", this);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
    axisLayout->addWidget(infoLabel);

    layout->addWidget(axisGroup);

    // Backbone Pen Width Group
    QGroupBox* penGroup = new QGroupBox("Backbone Pen Width", this);
    QHBoxLayout* penLayout = new QHBoxLayout(penGroup);

    penLayout->addWidget(new QLabel("Axis pen width (device pixels)", this));

    m_spinPenWidth = new QDoubleSpinBox(this);
    m_spinPenWidth->setRange(0.5, 3.0);
    m_spinPenWidth->setSingleStep(0.5);
    m_spinPenWidth->setDecimals(2);
    m_spinPenWidth->setValue(1.0);
    penLayout->addWidget(m_spinPenWidth);

    layout->addWidget(penGroup);

    layout->addStretch();

    // Connect signals
    connect(m_chkYLeft, &QCheckBox::toggled, this, &MainWindow::onYLeftToggled);
    connect(m_chkYRight, &QCheckBox::toggled, this, &MainWindow::onYRightToggled);
    connect(m_chkXTop, &QCheckBox::toggled, this, &MainWindow::onXTopToggled);
    connect(m_chkXBottom, &QCheckBox::toggled, this, &MainWindow::onXBottomToggled);
    connect(btnAllInside, &QPushButton::clicked, this, &MainWindow::onAllInsideClicked);
    connect(btnAllOutside, &QPushButton::clicked, this, &MainWindow::onAllOutsideClicked);
    connect(m_chkLogYRight, &QCheckBox::toggled, this, &MainWindow::onLogYRightToggled);
    connect(m_chkInvertedYLeft, &QCheckBox::toggled, this, &MainWindow::onInvertedYLeftToggled);
    connect(m_spinPenWidth, &QDoubleSpinBox::valueChanged, this, &MainWindow::onPenWidthChanged);

    return panel;
}

void MainWindow::onYLeftToggled(bool checked)
{
    m_plot->setAxisTickDirection(QwtAxis::YLeft, checked ? QwtPlot::TickInside : QwtPlot::TickOutside);
    m_plot->replot();
}

void MainWindow::onYRightToggled(bool checked)
{
    m_plot->setAxisTickDirection(QwtAxis::YRight, checked ? QwtPlot::TickInside : QwtPlot::TickOutside);
    m_plot->replot();
}

void MainWindow::onXTopToggled(bool checked)
{
    m_plot->setAxisTickDirection(QwtAxis::XTop, checked ? QwtPlot::TickInside : QwtPlot::TickOutside);
    m_plot->replot();
}

void MainWindow::onXBottomToggled(bool checked)
{
    m_plot->setAxisTickDirection(QwtAxis::XBottom, checked ? QwtPlot::TickInside : QwtPlot::TickOutside);
    m_plot->replot();
}

void MainWindow::onAllInsideClicked()
{
    m_chkYLeft->setChecked(true);
    m_chkYRight->setChecked(true);
    m_chkXTop->setChecked(true);
    m_chkXBottom->setChecked(true);
}

void MainWindow::onAllOutsideClicked()
{
    m_chkYLeft->setChecked(false);
    m_chkYRight->setChecked(false);
    m_chkXTop->setChecked(false);
    m_chkXBottom->setChecked(false);
}

void MainWindow::onLogYRightToggled(bool checked)
{
    if (checked) {
        // Switch to logarithmic scale
        m_plot->setAxisToLogScale(QwtAxis::YRight);
        m_plot->setAxisScale(QwtAxis::YRight, 0.1, 1000.0);
        m_plot->setAxisTitle(QwtAxis::YRight, "YRight (Log scale)");
    } else {
        // Switch to linear scale
        m_plot->setAxisToLinearScale(QwtAxis::YRight);
        m_plot->setAxisScale(QwtAxis::YRight, 0.0, 1000.0);
        m_plot->setAxisTitle(QwtAxis::YRight, "YRight (Linear scale)");
    }
    m_plot->replot();
}

void MainWindow::onPenWidthChanged(double width)
{
    const QwtAxisId axes[] = { QwtAxis::XBottom, QwtAxis::XTop, QwtAxis::YLeft, QwtAxis::YRight };
    for (QwtAxisId axisId : axes)
        m_plot->axisScaleDraw(axisId)->setPenWidthF(width);

    m_plot->replot();
}

void MainWindow::onInvertedYLeftToggled(bool checked)
{
    if (checked) {
        // Inverted scale: 100 at bottom, 0 at top
        m_plot->setAxisScale(QwtAxis::YLeft, 100.0, 0.0);
        m_plot->setAxisTitle(QwtAxis::YLeft, "YLeft (Inverted 100->0)");
    } else {
        // Normal scale: 0 at bottom, 100 at top
        m_plot->setAxisScale(QwtAxis::YLeft, 0.0, 100.0);
        m_plot->setAxisTitle(QwtAxis::YLeft, "YLeft (Normal 0->100)");
    }
    m_plot->replot();
}
