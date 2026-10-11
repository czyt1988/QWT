#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

#include <qwt_plot_renderer.h>

class QListWidget;
class QComboBox;
class QCheckBox;
class QLabel;
class QToolButton;

class Plot;

/*
    Compares the 22 scientific colormap presets of QwtColorMapPreset:

    The list on the left selects the preset, the spectrogram on the right
    paints the same demonstration field with the selected map, so that
    all presets can be browsed and compared side by side. The backdrop
    can be switched to dark, contour lines and the size of the color
    table can be used to inspect banding artifacts.

    Running the example with --screenshot <directory> writes one PNG per
    preset into the directory ( plus an overview of all gradients and a
    screenshot of the window ), what can be used to document the presets
    without running the GUI.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);

    //! Write one PNG per preset ( overview.png, colormap.png ) into the directory
    bool exportScreenshots(const QString& directory);

  private Q_SLOTS:
    void onPresetSelectionChanged();
    void onBackdropChanged(int index);
    void onContoursToggled(bool on);
    void onColorTableChanged(int index);
    void chooseScreenshotDirectory();

  private:
    void saveOverviewImage(const QString& filePath);
    void saveWindowImage(const QString& filePath);

    QListWidget* m_presetList;
    Plot* m_plot;

    QComboBox* m_backdropBox;
    QCheckBox* m_contourBox;
    QComboBox* m_colorTableBox;

    QwtPlotRenderer m_renderer;
};

#endif
