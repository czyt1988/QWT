/******************************************************************************
 * Down-sampling comparison demo for spectral line rendering.
 *
 * Renders two synthetic spectra ( many / few samples per gaussian peak )
 * with the current qwt build. The point reduction algorithm behind
 * QwtPlotCurve::PaintAttribute can be switched at runtime, so the rendering
 * differences of the algorithms can be compared:
 *
 *   - none            : no reduction at all
 *   - weedout         : FilterPoints           ( WeedOutPoints )
 *   - aggressive      : FilterPointsAggressive ( WeedOutIntermediatePoints )
 *   - pixel           : FilterPointsPixel      ( min/max per pixel column )
 *   - lttb            : FilterPointsLTTB       ( largest triangle 3 buckets )
 *   - minmaxlttb      : FilterPointsMinMaxLTTB ( min/max buckets )
 *   - adaptive        : FilterPointsAdaptive   ( density gated ordered column reduce )
 *
 * The spectra are generated at startup from a fixed set of gaussian peaks
 * plus seeded noise, so the demo is self contained and every run is
 * reproducible.
 *
 * Usage:
 *   downsample_demo                  interactive GUI ( wheel = zoom )
 *   downsample_demo --batch [dir]    render every dataset/algorithm/view
 *                                    combination as PNG into dir (offscreen)
 *                                    and report the mapped point counts
 *   downsample_demo --dots           additionally draw a dot at every mapped
 *                                    point ( batch: into the PNGs, GUI: the
 *                                    toolbar checkbox does the same )
 *****************************************************************************/

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFontDatabase>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPushButton>
#include <QRandomGenerator>
#include <QToolBar>
#include <QVector>

#include <cmath>
#include <limits>

#include <QwtPlot>
#include <QwtPlotCanvasZoomer>
#include <QwtPlotCurve>
#include <QwtPlotGrid>
#include <QwtPointMapper>
#include <QwtPainter>
#include <QwtScaleMap>
#include <qwt_axis.h>

namespace
{

struct DataSet
{
    QString key;    // ascii key used for batch file names
    QString title;  // human readable name
    QVector< QPointF > points;
};

enum AlgoId
{
    AlgoNone = 0,
    AlgoWeedOut,
    AlgoAggressive,
    AlgoPixel,
    AlgoLTTB,
    AlgoMinMaxLTTB,
    AlgoAdaptive,
    AlgoVertical1px,
    AlgoCount
};

struct AlgoInfo
{
    AlgoId id;
    const char* key;
    const char* label;
};

const AlgoInfo kAlgos[AlgoCount] = {
    { AlgoNone, "none", "no reduction" },
    { AlgoWeedOut, "weedout", "FilterPoints" },
    { AlgoAggressive, "aggressive", "FilterPointsAggressive" },
    { AlgoPixel, "pixel", "FilterPointsPixel" },
    { AlgoLTTB, "lttb", "FilterPointsLTTB (default)" },
    { AlgoMinMaxLTTB, "minmaxlttb", "FilterPointsMinMaxLTTB" },
    { AlgoAdaptive, "adaptive", "FilterPointsAdaptive" },
    { AlgoVertical1px, "vertical1px", "vertical 1px per column (reporter's idea)" },
};

struct ViewInfo
{
    const char* key;
    const char* label;
    double xMin;
    double xMax;
    bool fullRange;
};

// win14 is the densest peak window of the data, win50/full make the peaks
// narrower on screen - the condition where the down-sampling artifacts
// show up.
const ViewInfo kViews[] = {
    { "win14", "window 858..872", 858.0, 872.0, false },
    { "win50", "window 850..900", 850.0, 900.0, false },
    { "full", "full range 201..1000", 0.0, 0.0, true },
};
const int kViewCount = int(sizeof(kViews) / sizeof(kViews[0]));

struct GaussianPeak
{
    double center;
    double amplitude;
    double sigma;
};

// Analytic line shape of the synthetic spectra: narrow gaussian emission
// peaks on a flat baseline, like the spectral measurements on which the
// reduction artifacts were first observed.
const GaussianPeak kPeaks[] = {
    // the dense peak cluster around 868 plus isolated lines
    { 857.20, 2.2e5, 0.28 },
    { 859.50, 5.0e5, 0.30 },
    { 863.40, 1.25e6, 0.32 },
    { 866.00, 3.0e5, 0.26 },
    { 867.90, 3.38e6, 0.24 },
    { 868.45, 1.90e6, 0.24 },
    { 868.90, 7.8e5, 0.24 },
    { 870.20, 7.3e5, 0.28 },
    { 871.10, 8.8e5, 0.28 },
    { 871.90, 6.5e5, 0.28 },
    { 874.10, 1.1e5, 0.35 },
    { 876.20, 5.5e4, 0.35 },
    // a few wide, low features, so that the full range view is not empty
    { 305.00, 2.0e4, 1.60 },
    { 520.00, 3.5e4, 2.20 },
    { 700.00, 1.5e4, 1.10 },
};

double spectrumValue(double x)
{
    double y = 0.0;
    for (const GaussianPeak& peak : kPeaks)
    {
        const double t = (x - peak.center) / peak.sigma;
        y += peak.amplitude * std::exp(-0.5 * t * t);
    }
    return y;
}

// Sample the line shape on an equidistant x grid and add seeded measurement
// noise - the fixed seed keeps every run of the demo identical.
QVector< QPointF > makeSpectrum(double step, quint32 seed)
{
    const double xMin = 201.0;
    const double xMax = 1000.0;

    QRandomGenerator rng(seed);

    QVector< QPointF > points;
    const int count = int((xMax - xMin) / step) + 1;
    points.reserve(count);

    for (int i = 0; i < count; ++i)
    {
        const double x = xMin + i * step;
        const double noise = (rng.generateDouble() - 0.5) * 6000.0;
        points.append(QPointF(x, qMax(0.0, spectrumValue(x) + noise)));
    }

    return points;
}

QVector< DataSet > makeDataSets()
{
    QVector< DataSet > sets;

    DataSet many;
    many.key = QStringLiteral("many");
    many.title = QStringLiteral("many points per peak");
    many.points = makeSpectrum(0.004, 0x51de1);

    DataSet few;
    few.key = QStringLiteral("few");
    few.title = QStringLiteral("few points per peak");
    few.points = makeSpectrum(0.043, 0x51de2);

    sets.append(many);
    sets.append(few);

    return sets;
}

void applyAlgo(QwtPlotCurve* curve, AlgoId id)
{
    curve->setPaintAttribute(QwtPlotCurve::FilterPoints, false);
    curve->setPaintAttribute(QwtPlotCurve::FilterPointsAggressive, false);
    curve->setPaintAttribute(QwtPlotCurve::FilterPointsPixel, false);
    curve->setPaintAttribute(QwtPlotCurve::FilterPointsLTTB, false);
    curve->setPaintAttribute(QwtPlotCurve::FilterPointsMinMaxLTTB, false);
    curve->setPaintAttribute(QwtPlotCurve::FilterPointsAdaptive, false);

    switch (id)
    {
        case AlgoWeedOut:
            curve->setPaintAttribute(QwtPlotCurve::FilterPoints, true);
            break;
        case AlgoAggressive:
            curve->setPaintAttribute(QwtPlotCurve::FilterPointsAggressive, true);
            break;
        case AlgoPixel:
            curve->setPaintAttribute(QwtPlotCurve::FilterPointsPixel, true);
            break;
        case AlgoLTTB:
            curve->setPaintAttribute(QwtPlotCurve::FilterPointsLTTB, true);
            break;
        case AlgoMinMaxLTTB:
            curve->setPaintAttribute(QwtPlotCurve::FilterPointsMinMaxLTTB, true);
            break;
        case AlgoAdaptive:
            curve->setPaintAttribute(QwtPlotCurve::FilterPointsAdaptive, true);
            break;
        case AlgoNone:
        default:
            break;
    }
}

// Number of points the mapper of QwtPlotCurve::drawLines would produce for
// the current paint attributes of the curve - reported by the batch mode so
// that the output sizes of the algorithms can be compared.
int mappedPointCount(QwtPlot* plot, QwtPlotCurve* curve, qreal dpr)
{
    if (curve->dataSize() <= 0)
        return 0;

    QwtPointMapper mapper;
    mapper.setDevicePixelRatio(dpr);
    mapper.setFlag(QwtPointMapper::RoundPoints, true);
    mapper.setFlag(QwtPointMapper::AdaptiveColumnReduce, curve->testPaintAttribute(QwtPlotCurve::FilterPointsAdaptive));
    mapper.setFlag(QwtPointMapper::PixelColumnReduce, curve->testPaintAttribute(QwtPlotCurve::FilterPointsPixel));
    mapper.setFlag(QwtPointMapper::LTTBReduce, curve->testPaintAttribute(QwtPlotCurve::FilterPointsLTTB));
    mapper.setFlag(QwtPointMapper::MinMaxReduce, curve->testPaintAttribute(QwtPlotCurve::FilterPointsMinMaxLTTB));
    mapper.setFlag(QwtPointMapper::WeedOutIntermediatePoints, curve->testPaintAttribute(QwtPlotCurve::FilterPointsAggressive));
    mapper.setFlag(QwtPointMapper::WeedOutPoints,
                   curve->testPaintAttribute(QwtPlotCurve::FilterPoints) ||
                       curve->testPaintAttribute(QwtPlotCurve::FilterPointsAggressive));

    return int(mapper.toPolygonF(plot->canvasMap(QwtAxis::XBottom), plot->canvasMap(QwtAxis::YLeft),
                                 curve->data(), 0, int(curve->dataSize()) - 1)
                   .size());
}

// QwtPlot auto scaling always uses the bounding rect of the whole series,
// not of the visible x window - compute the y range of a window manually so
// that zoomed views look like the original measurements.
QwtInterval yRangeOfWindow(const QVector< QPointF >& points, double xMin, double xMax)
{
    double yMax = 0.0;
    for (const QPointF& p : points)
    {
        if (p.x() < xMin || p.x() > xMax)
            continue;
        if (p.y() > yMax)
            yMax = p.y();
    }

    if (yMax <= 0.0)
        yMax = 1.0;

    return QwtInterval(0.0, yMax * 1.05);
}

// Bucket the samples into pixel columns and keep, within each column, every
// sample whose mapped y differs by at least one pixel from the last kept
// one. The deviation of the result from the original curve is bounded by
// 1px vertically and one column horizontally, while flat regions collapse
// to a single point.
QVector< QPointF > verticalPixelReduce(const QVector< QPointF >& points, const QwtScaleMap& xMap, const QwtScaleMap& yMap)
{
    QVector< QPointF > out;
    out.reserve(points.size());

    int lastCol = std::numeric_limits< int >::min();
    double lastY = 0.0;

    for (const QPointF& p : points)
    {
        const double py = yMap.transform(p.y());
        const int col = qFloor(xMap.transform(p.x()));

        if (col != lastCol || qAbs(py - lastY) >= 1.0)
        {
            out.append(p);
            lastCol = col;
            lastY = py;
        }
    }

    if (!points.isEmpty() && (out.isEmpty() || out.last() != points.last()))
        out.append(points.last());

    return out;
}

// Plot item that re-runs exactly the point mapping QwtPlotCurve::drawLines
// would do for its source curve and draws a dot at every mapped vertex.
// This visualizes the distribution of the points that are actually painted:
// with a reduction attribute enabled the kept points show up, without one
// every sample is marked. The mapping is recomputed inside draw(), so the
// dots stay correct while zooming.
class MappedPointsItem : public QwtPlotItem
{
  public:
    explicit MappedPointsItem(const QwtPlotCurve* source = nullptr)
        : m_source(source)
    {
        setRenderHint(QwtPlotItem::RenderAntialiased, true);
        setZ(1.0);  // above the curve ( z = 0 )
    }

    virtual int rtti() const override
    {
        return QwtPlotItem::Rtti_PlotItem;
    }

    virtual void draw(QPainter* painter, const QwtScaleMap& xMap, const QwtScaleMap& yMap,
                      const QRectF& canvasRect) const override
    {
        if (m_source == nullptr || m_source->dataSize() <= 0)
            return;

        // replicate the mapper setup of QwtPlotCurve::drawLines - the demo
        // never enables Fitted, so doFit is false and doAlign reduces to
        // QwtPainter::roundingAlignment( painter )
        const bool doAlign = QwtPainter::roundingAlignment(painter);

        QwtPointMapper mapper;
        mapper.setDevicePixelRatio(QwtPainter::devicePixelRatio(painter->device()));

        if (doAlign)
        {
            mapper.setFlag(QwtPointMapper::RoundPoints, true);
            mapper.setFlag(QwtPointMapper::WeedOutIntermediatePoints,
                           m_source->testPaintAttribute(QwtPlotCurve::FilterPointsAggressive));
        }

        mapper.setFlag(QwtPointMapper::AdaptiveColumnReduce,
                       m_source->testPaintAttribute(QwtPlotCurve::FilterPointsAdaptive));
        mapper.setFlag(QwtPointMapper::PixelColumnReduce,
                       m_source->testPaintAttribute(QwtPlotCurve::FilterPointsPixel));
        mapper.setFlag(QwtPointMapper::LTTBReduce,
                       m_source->testPaintAttribute(QwtPlotCurve::FilterPointsLTTB));
        mapper.setFlag(QwtPointMapper::MinMaxReduce,
                       m_source->testPaintAttribute(QwtPlotCurve::FilterPointsMinMaxLTTB));
        mapper.setFlag(QwtPointMapper::WeedOutPoints,
                       m_source->testPaintAttribute(QwtPlotCurve::FilterPoints) ||
                           m_source->testPaintAttribute(QwtPlotCurve::FilterPointsAggressive));

        mapper.setBoundingRect(canvasRect);

        // toPolygonF returns paint coordinates snapped to the device pixel
        // grid - exactly the vertices drawLines paints ( the demo uses no
        // smoothing and no curve fitter, so nothing modifies them anymore )
        const QPolygonF mapped = mapper.toPolygonF(xMap, yMap, m_source->data(), 0, int(m_source->dataSize()) - 1);

        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(224, 40, 40, 140));

        const qreal radius = 1.75;
        for (const QPointF& p : mapped)
            painter->drawEllipse(p, radius, radius);

        painter->restore();
    }

  private:
    const QwtPlotCurve* m_source;
};

QwtPlot* makePlot()
{
    QwtPlot* plot = new QwtPlot();
    plot->setCanvasBackground(Qt::white);

    QwtPlotGrid* grid = new QwtPlotGrid();
    grid->setMajorPen(QPen(QColor(210, 210, 210), 0, Qt::DotLine));
    grid->attach(plot);

    plot->setAxisTitle(QwtAxis::XBottom, "Wavelength");
    plot->setAxisTitle(QwtAxis::YLeft, "Intensity");
    return plot;
}

QwtPlotCurve* makeCurve(QwtPlot* plot)
{
    QwtPlotCurve* curve = new QwtPlotCurve();
    curve->setPen(QPen(QColor(32, 48, 192), 2.0));
    curve->setRenderHint(QwtPlotCurve::RenderAntialiased, true);
    curve->attach(plot);
    return curve;
}

void applyView(QwtPlot* plot, const QVector< QPointF >& points, const ViewInfo& view)
{
    if (view.fullRange)
    {
        plot->setAxisAutoScale(QwtAxis::XBottom);
        plot->setAxisAutoScale(QwtAxis::YLeft);
    }
    else
    {
        plot->setAxisScale(QwtAxis::XBottom, view.xMin, view.xMax);
        const QwtInterval y = yRangeOfWindow(points, view.xMin, view.xMax);
        plot->setAxisScale(QwtAxis::YLeft, y.minValue(), y.maxValue());
    }
}

int runBatch(const QString& outDir, const QVector< DataSet >& sets, qreal dpr, bool showDots)
{
    QDir().mkpath(outDir);

    QwtPlot* plot = makePlot();
    QwtPlotCurve* curve = makeCurve(plot);
    plot->resize(1400, 700);
    plot->show();

    // the dots item reads samples and paint attributes from the curve at
    // draw time, no per image update needed
    if (showDots)
    {
        MappedPointsItem* dots = new MappedPointsItem(curve);
        dots->attach(plot);
    }

    int count = 0;
    for (const DataSet& ds : sets)
    {
        if (ds.points.isEmpty())
            continue;

        curve->setSamples(ds.points);

        for (int v = 0; v < kViewCount; ++v)
        {
            applyView(plot, ds.points, kViews[v]);

            for (int a = 0; a < AlgoCount; ++a)
            {
                const AlgoId algo = kAlgos[a].id;
                if (algo == AlgoVertical1px)
                {
                    // client side pre-reduction with the column rule
                    curve->setSamples(ds.points);
                    applyAlgo(curve, AlgoNone);
                    plot->replot();  // establish the scale maps
                    const QVector< QPointF > reduced =
                        verticalPixelReduce(ds.points, plot->canvasMap(QwtAxis::XBottom), plot->canvasMap(QwtAxis::YLeft));
                    qInfo("%s/%s: %d -> %d points", qPrintable(ds.key), kViews[v].key, int(ds.points.size()), int(reduced.size()));
                    curve->setSamples(reduced);
                }
                else
                {
                    curve->setSamples(ds.points);
                    applyAlgo(curve, algo);
                }

                plot->setTitle(QStringLiteral("%1 | %2 | %3")
                                   .arg(ds.title, QLatin1String(kAlgos[a].label), QLatin1String(kViews[v].label)));
                plot->replot();

                if (algo != AlgoVertical1px)
                {
                    qInfo("%s/%s/%s: mapped %d points", qPrintable(ds.key), kViews[v].key, kAlgos[a].key,
                          mappedPointCount(plot, curve, dpr));
                }

                // render into a device with the given pixel ratio, so that
                // QwtPainter::devicePixelRatio() sees it inside the curve
                QImage image(qRound(plot->width() * dpr), qRound(plot->height() * dpr), QImage::Format_ARGB32_Premultiplied);
                image.setDevicePixelRatio(dpr);
                image.fill(Qt::white);

                QPainter painter(&image);
                plot->render(&painter);
                painter.end();

                const QString name = QStringLiteral("%1__%2__%3.png").arg(ds.key, QLatin1String(kAlgos[a].key), QLatin1String(kViews[v].key));
                if (image.save(outDir + QLatin1Char('/') + name))
                    ++count;
                else
                    qWarning("failed to save %s", qPrintable(name));
            }
        }
    }

    delete plot;
    qInfo("wrote %d images to %s", count, qPrintable(outDir));
    return count > 0 ? 0 : 1;
}

class DemoWindow : public QMainWindow
{
  public:
    explicit DemoWindow(const QVector< DataSet >& sets)
        : m_sets(sets)
    {
        m_plot = makePlot();
        m_curve = makeCurve(m_plot);
        setCentralWidget(m_plot);

        m_zoomer = new QwtPlotCanvasZoomer(m_plot->canvas());

        m_dotsItem = new MappedPointsItem(m_curve);
        m_dotsItem->attach(m_plot);
        m_dotsItem->setVisible(false);

        QToolBar* bar = addToolBar(QStringLiteral("view"));
        bar->setMovable(false);

        bar->addWidget(new QLabel(QStringLiteral(" dataset ")));
        m_dataCombo = new QComboBox();
        for (const DataSet& ds : m_sets)
            m_dataCombo->addItem(QStringLiteral("%1 (%2 points)").arg(ds.title).arg(ds.points.size()));
        bar->addWidget(m_dataCombo);

        bar->addWidget(new QLabel(QStringLiteral("  reduction ")));
        m_algoCombo = new QComboBox();
        for (int a = 0; a < AlgoCount; ++a)
            m_algoCombo->addItem(QLatin1String(kAlgos[a].label));
        m_algoCombo->setCurrentIndex(AlgoLTTB);  // the default of QwtPlotCurve
        bar->addWidget(m_algoCombo);

        bar->addWidget(new QLabel(QStringLiteral("  view ")));
        m_viewCombo = new QComboBox();
        for (int v = 0; v < kViewCount; ++v)
            m_viewCombo->addItem(QLatin1String(kViews[v].label));
        bar->addWidget(m_viewCombo);

        m_dotsCheck = new QCheckBox(QStringLiteral(" mapped points"));
        m_dotsCheck->setToolTip(QStringLiteral("Draw a dot at every point that is actually painted,\n"
                                               "i.e. after the selected reduction has been applied."));
        bar->addWidget(m_dotsCheck);

        QPushButton* saveButton = new QPushButton(QStringLiteral("save PNG"));
        bar->addWidget(saveButton);

        connect(m_dataCombo, qOverload< int >(&QComboBox::currentIndexChanged), this, &DemoWindow::updatePlot);
        connect(m_algoCombo, qOverload< int >(&QComboBox::currentIndexChanged), this, &DemoWindow::updatePlot);
        connect(m_viewCombo, qOverload< int >(&QComboBox::currentIndexChanged), this, &DemoWindow::updatePlot);
        // QwtPlotItem is not a QObject in qwt v7, so it cannot be the
        // receiver of a connect - route through a lambda; setVisible()
        // triggers a replot via itemChanged()
        connect(m_dotsCheck, &QCheckBox::toggled, this, [this](bool on) { m_dotsItem->setVisible(on); });
        connect(saveButton, &QPushButton::clicked, this, &DemoWindow::saveImage);

        resize(1400, 700);
        updatePlot();
    }

  private:
    const DataSet& currentDataSet() const
    {
        const int index = qBound(0, m_dataCombo->currentIndex(), m_sets.size() - 1);
        return m_sets[index];
    }

    void updatePlot()
    {
        const DataSet& ds = currentDataSet();
        if (ds.points.isEmpty())
            return;

        const AlgoId algo = static_cast< AlgoId >(m_algoCombo->currentIndex());
        const ViewInfo& view = kViews[m_viewCombo->currentIndex()];

        m_curve->setSamples(ds.points);
        applyView(m_plot, ds.points, view);

        if (algo == AlgoVertical1px)
        {
            // client side pre-reduction with the column rule
            applyAlgo(m_curve, AlgoNone);
            m_plot->replot();  // establish the scale maps
            m_curve->setSamples(
                verticalPixelReduce(ds.points, m_plot->canvasMap(QwtAxis::XBottom), m_plot->canvasMap(QwtAxis::YLeft)));
        }
        else
        {
            applyAlgo(m_curve, algo);
        }

        m_plot->setTitle(QStringLiteral("%1 | %2 | %3")
                             .arg(ds.title, m_algoCombo->currentText(), m_viewCombo->currentText()));
        m_plot->replot();
        m_zoomer->setZoomBase(false);
        setWindowTitle(QStringLiteral("downsample demo - %1 points drawn").arg(m_curve->dataSize()));
    }

    void saveImage()
    {
        const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("save plot"), QString(), QStringLiteral("PNG (*.png)"));
        if (!path.isEmpty())
            m_plot->grab().save(path);
    }

    const QVector< DataSet >& m_sets;
    QwtPlot* m_plot = nullptr;
    QwtPlotCurve* m_curve = nullptr;
    QwtPlotCanvasZoomer* m_zoomer = nullptr;
    MappedPointsItem* m_dotsItem = nullptr;
    QComboBox* m_dataCombo = nullptr;
    QComboBox* m_algoCombo = nullptr;
    QComboBox* m_viewCombo = nullptr;
    QCheckBox* m_dotsCheck = nullptr;
};

}  // namespace

int main(int argc, char* argv[])
{
    QStringList args;
    for (int i = 0; i < argc; ++i)
        args << QString::fromLocal8Bit(argv[i]);

    const int batchIndex = args.indexOf(QStringLiteral("--batch"));
    const bool batch = batchIndex >= 0;
    QString outDir = QStringLiteral("downsample_demo_out");
    if (batch && batchIndex + 1 < args.size())
        outDir = args[batchIndex + 1];

    qreal dpr = 1.0;
    const int dprIndex = args.indexOf(QStringLiteral("--dpr"));
    if (dprIndex >= 0 && dprIndex + 1 < args.size())
        dpr = qMax< qreal >(0.25, args[dprIndex + 1].toDouble());

    const bool showDots = args.contains(QStringLiteral("--dots"));

    // batch mode renders offscreen, no window system required
    QVector< QByteArray > holder;
    QVector< char* > argv2;
    for (const QString& arg : args)
    {
        holder.append(arg.toLocal8Bit());
        argv2.append(holder.last().data());
    }
    if (batch)
    {
        holder.append(QByteArrayLiteral("-platform"));
        argv2.append(holder.last().data());
        holder.append(QByteArrayLiteral("offscreen"));
        argv2.append(holder.last().data());
    }

    int argc2 = int(argv2.size());
    QApplication app(argc2, argv2.data());

    // the offscreen platform uses the generic font database, which ships
    // without fonts - register system fonts, so that titles and scale
    // labels are rendered as text instead of as empty boxes
    if (QFontDatabase::families().isEmpty()) {
        const QDir fontDir(QStringLiteral("C:/Windows/Fonts"));
        const QStringList candidates {
            QStringLiteral("msyh.ttc"), QStringLiteral("simhei.ttf"), QStringLiteral("arial.ttf")
        };
        for (const QString& file : candidates) {
            if (fontDir.exists(file)) {
                QFontDatabase::addApplicationFont(fontDir.absoluteFilePath(file));
                break;
            }
        }

        const QStringList families = QFontDatabase::families();
        if (!families.isEmpty())
            app.setFont(QFont(families.first()));
    }

    const QVector< DataSet > sets = makeDataSets();

    if (batch)
        return runBatch(outDir, sets, dpr, showDots);

    DemoWindow window(sets);
    window.show();
    return app.exec();
}
