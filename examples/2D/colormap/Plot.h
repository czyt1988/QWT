#ifndef PLOT_H
#define PLOT_H

#include <qwt_plot.h>

class QwtPlotSpectrogram;

/**
    Gallery plot for the scientific colormap presets.

    A spectrogram of the classic "peaks" demonstration field is painted with
    the currently selected preset, the color bar of the right axis uses the
    same map. Switching the preset only exchanges the color maps, so that
    the geometry of the plot does not change.
 */
class Plot : public QwtPlot
{
    Q_OBJECT

  public:
    Plot(QWidget* parent = nullptr);

    //! Select one of the presets of QwtColorMapPreset
    void setColorMapPreset(const QString& name);

    //! Current preset name
    QString colorMapPreset() const;

    //! Light or dark backdrop of the plot area
    void setBackdropDark(bool on);

    //! Show the contour lines of the field on top of the image
    void setContoursVisible(bool on);

    //! Number of colors of the color table ( 0: smooth interpolation )
    void setColorTableSize(int numColors);

  private:
    void applyTheme();

    QwtPlotSpectrogram* m_spectrogram;
    QString m_preset;
    bool m_dark;
};

#endif
