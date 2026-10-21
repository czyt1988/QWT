/*****************************************************************************
 * Qwt Examples - Copyright (C) 2002 Uwe Rathmann
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

class QwtPlot;
class QwtPlotCurve;
class QCheckBox;
class QDoubleSpinBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private Q_SLOTS:
    void onYLeftToggled(bool checked);
    void onYRightToggled(bool checked);
    void onXTopToggled(bool checked);
    void onXBottomToggled(bool checked);
    void onAllInsideClicked();
    void onAllOutsideClicked();
    void onLogYRightToggled(bool checked);
    void onInvertedYLeftToggled(bool checked);
    void onPenWidthChanged(double width);

private:
    void setupPlot();
    QWidget* createControlPanel();

    QwtPlot* m_plot;
    QwtPlotCurve* m_curve;           // Regular curve (XBottom)
    QwtPlotCurve* m_curveLog;        // Curve for log axis (YRight)
    QwtPlotCurve* m_curveInverted;   // Curve for inverted axis (YLeft)

    QCheckBox* m_chkYLeft;
    QCheckBox* m_chkYRight;
    QCheckBox* m_chkXTop;
    QCheckBox* m_chkXBottom;
    QCheckBox* m_chkLogYRight;       // Toggle log scale for YRight
    QCheckBox* m_chkInvertedYLeft;   // Toggle inverted scale for YLeft
    QDoubleSpinBox* m_spinPenWidth;  // Backbone pen width of all axes
};

#endif // MAIN_WINDOW_H