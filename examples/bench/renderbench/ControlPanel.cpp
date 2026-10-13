/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include "ControlPanel.h"

#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>

ControlPanel::ControlPanel(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(220);

    QGroupBox* paramGroup = new QGroupBox("Parameters", this);
    QGridLayout* paramLayout = new QGridLayout(paramGroup);

    int row = 0;

    paramLayout->addWidget(new QLabel("Points:", paramGroup), row, 0);
    m_numPoints = new QSpinBox(paramGroup);
    m_numPoints->setRange(1000, 10000000);
    m_numPoints->setSingleStep(100000);
    m_numPoints->setValue(1000000);
    paramLayout->addWidget(m_numPoints, row++, 1);

    paramLayout->addWidget(new QLabel("Frames:", paramGroup), row, 0);
    m_frameCount = new QSpinBox(paramGroup);
    m_frameCount->setRange(10, 1000);
    m_frameCount->setSingleStep(10);
    m_frameCount->setValue(100);
    paramLayout->addWidget(m_frameCount, row++, 1);

    paramLayout->addWidget(new QLabel("Wave:", paramGroup), row, 0);
    m_waveType = new QComboBox(paramGroup);
    m_waveType->addItem("Wave");
    m_waveType->addItem("Noise");
    paramLayout->addWidget(m_waveType, row++, 1);

    paramLayout->addWidget(new QLabel("Method:", paramGroup), row, 0);
    m_method = new QComboBox(paramGroup);
    m_method->addItem("None");
    m_method->addItem("FilterPoints");
    m_method->addItem("FilterPointsAggressive");
    m_method->addItem("FilterPointsPixel");
    m_method->addItem("FilterPointsLTTB");
    m_method->addItem("FilterPointsMinMaxLTTB");
    m_method->setCurrentIndex(2);
    paramLayout->addWidget(m_method, row++, 1);

    paramLayout->setColumnStretch(1, 10);

    m_runBtn = new QPushButton("Run", this);
    m_runAllBtn = new QPushButton("Run All (Compare)", this);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);

    m_status = new QLabel("Ready", this);
    m_status->setWordWrap(true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(paramGroup);
    layout->addWidget(m_runBtn);
    layout->addWidget(m_runAllBtn);
    layout->addWidget(m_progress);
    layout->addWidget(m_status);
    layout->addStretch(10);

    connect(m_runBtn, &QPushButton::clicked, this, &ControlPanel::runRequested);
    connect(m_runAllBtn, &QPushButton::clicked, this, &ControlPanel::runAllRequested);
}

int ControlPanel::numPoints() const { return m_numPoints->value(); }
int ControlPanel::frameCount() const { return m_frameCount->value(); }
int ControlPanel::waveType() const { return m_waveType->currentIndex(); }
int ControlPanel::methodIndex() const { return m_method->currentIndex(); }

void ControlPanel::setProgress(int current, int total)
{
    m_progress->setMaximum(total);
    m_progress->setValue(current);
}

void ControlPanel::setStatus(const QString& text)
{
    m_status->setText(text);
}

void ControlPanel::setRunning(bool running)
{
    m_runBtn->setEnabled(!running);
    m_runAllBtn->setEnabled(!running);
    m_numPoints->setEnabled(!running);
    m_frameCount->setEnabled(!running);
    m_waveType->setEnabled(!running);
    m_method->setEnabled(!running);
}

#include "moc_ControlPanel.cpp"
