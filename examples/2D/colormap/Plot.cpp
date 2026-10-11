#include "Plot.h"

#include <qnamespace.h>
#include <QtMath>
#include <QBrush>
#include <QColor>
#include <QList>
#include <QPalette>
#include <QPen>

#include <qwt_colormap.h>
#include <qwt_colormap_preset.h>
#include <qwt_interval.h>
#include <qwt_math.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_raster_data.h>
#include <qwt_scale_widget.h>

namespace
{
    /**
        The classic "peaks" field: a sharp positive peak next to a deep well
        on top of concentric waves. The values are normalized to [0, 1], the
        range the color maps map to. The field is smooth enough to compare
        the gradients and detailed enough to reveal hue shifts and banding.
     */
    class PeaksField : public QwtRasterData
    {
      public:
        PeaksField()
        {
            setAttribute(QwtRasterData::WithoutGaps, true);

            m_intervals[ Qt::XAxis ] = QwtInterval(-2.0, 2.0);
            m_intervals[ Qt::YAxis ] = QwtInterval(-2.0, 2.0);
            m_intervals[ Qt::ZAxis ] = QwtInterval(0.0, 1.0);
        }

        virtual QwtInterval interval(Qt::Axis axis) const override
        {
            if (axis >= 0 && axis <= 2)
                return m_intervals[ axis ];

            return QwtInterval();
        }

        virtual double value(double x, double y) const override
        {
            const double z = 3.0 * qPow(1.0 - x, 2) * qExp(-x * x - qPow(y + 1.0, 2))
                             - 10.0 * (x / 5.0 - x * x * x - qPow(y, 5)) * qExp(-x * x - y * y)
                             - qExp(-qPow(x + 1.0, 2) - y * y) / 3.0;

            // z ranges in [-6.55, 8.10] on the field above
            return qBound(0.0, (z + 6.55) / 14.65, 1.0);
        }

      private:
        QwtInterval m_intervals[ 3 ];
    };

    const QColor backdropLight(Qt::white);
    const QColor backdropDark("#1b1e24");
    const QColor textOnLight(Qt::black);
    const QColor textOnDark("#d6d6d6");
}

Plot::Plot(QWidget* parent)
    : QwtPlot(parent)
    , m_dark(false)
{
    m_spectrogram = new QwtPlotSpectrogram();
    m_spectrogram->setRenderThreadCount(1);
    m_spectrogram->setCachePolicy(QwtPlotRasterItem::PaintCache);
    m_spectrogram->setData(new PeaksField());

    QList< double > contourLevels;
    for (double level = 0.1; level < 1.0; level += 0.1)
        contourLevels += level;
    m_spectrogram->setContourLevels(contourLevels);

    m_spectrogram->attach(this);

    setAxisTitle(QwtAxis::XBottom, "x");
    setAxisTitle(QwtAxis::YLeft, "y");

    // A color bar on the right axis, painted with the same color map
    QwtScaleWidget* rightAxis = axisWidget(QwtAxis::YRight);
    rightAxis->setTitle("Value");
    rightAxis->setColorBarEnabled(true);
    setAxisScale(QwtAxis::YRight, 0.0, 1.0);
    setAxisVisible(QwtAxis::YRight);

    setColorMapPreset("viridis");
    setBackdropDark(false);
}

void Plot::setColorMapPreset(const QString& name)
{
    if (QwtColorMapPreset::presetFromName(name) < 0)
        return;

    m_preset = name;

    setTitle(name);
    m_spectrogram->setColorMap(QwtColorMapPreset::create(name).release());

    QwtScaleWidget* rightAxis = axisWidget(QwtAxis::YRight);
    rightAxis->setColorMap(QwtInterval(0.0, 1.0), QwtColorMapPreset::create(name).release());

    replot();
}

QString Plot::colorMapPreset() const
{
    return m_preset;
}

void Plot::setBackdropDark(bool on)
{
    m_dark = on;

    applyTheme();
    replot();
}

void Plot::setContoursVisible(bool on)
{
    // The contour lines need the contrast of the backdrop color
    m_spectrogram->setDefaultContourPen(QPen(m_dark ? textOnDark : textOnLight, 0.0));
    m_spectrogram->setDisplayMode(QwtPlotSpectrogram::ContourMode, on);

    replot();
}

void Plot::setColorTableSize(int numColors)
{
    m_spectrogram->setColorTableSize(numColors);
    replot();
}

void Plot::applyTheme()
{
    const QColor& backdrop = m_dark ? backdropDark : backdropLight;
    const QColor& textColor = m_dark ? textOnDark : textOnLight;

    setBackgroundColor(backdrop);
    setCanvasBackground(QBrush(backdrop));

    QPalette pal = palette();
    pal.setColor(QPalette::Text, textColor);
    setPalette(pal);

    for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++)
    {
        QwtScaleWidget* scaleWidget = axisWidget(QwtAxisId(axisPos));
        if (scaleWidget)
        {
            scaleWidget->setTextColor(textColor);
            scaleWidget->setScaleColor(textColor);
        }
    }
}

#include "moc_Plot.cpp"
