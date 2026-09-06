#ifndef OUTSIDETITLE_MAINWINDOW_H
#define OUTSIDETITLE_MAINWINDOW_H

#include <QMainWindow>
#include <QList>

class QwtPlot;

/**
 * @brief Demo for outside axis titles (QwtScaleWidget::TitleOutside)
 * @details Shows two plots side by side:
 *          - a single plot with left/right Y axes whose titles are painted
 *            horizontally below their scale widgets instead of inside them
 *          - a multi axis plot (host + 2 parasite plots) where every layer
 *            has its own outside Y title below its own axis column
 *
 *          The toolbar toggles the title placement at runtime and exports the
 *          single plot to PNG/SVG to verify that QwtPlotRenderer paints the
 *          captions as well.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    // Automated verification: grabs the widgets, toggles the title placement,
    // resizes and exports the single plot via QwtPlotRenderer into dir.
    // Returns true if all artifacts have been written.
    bool runSelfTest(const QString& dir);

private Q_SLOTS:
    // Apply TitleOutside/TitleInside to all Y axes of all plots
    void applyOutsideYTitles(bool on);
    // Apply TitleOutside/TitleInside to the bottom X axes of all plots
    void applyOutsideXTitles(bool on);
    // Export the single plot with QwtPlotRenderer
    void exportPlot(const QString& format);

private:
    QwtPlot* createSinglePlot(QWidget* parent);
    QwtPlot* createMultiAxisPlot(QWidget* parent);
    QList< QwtPlot* > allPlots() const;

    QwtPlot* m_singlePlot;
    QwtPlot* m_hostPlot;
};

#endif  // OUTSIDETITLE_MAINWINDOW_H
