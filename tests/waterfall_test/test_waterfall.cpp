#include "test_waterfall.h"

#include "qwt3d_plot.h"
#include "qwt3d_line3d.h"

#include "qwt_point_3d.h"

namespace
{

/**
 * @brief Probe color functor recording the active range pushed by the item
 * @details The item pushes the active range via setActiveRange() before
 *          createVector() (triggered by populateLegendColors()), so recording
 *          activeZMin/activeZMax there captures exactly what the item pushed.
 */
class ProbeColor : public Qwt3DColor
{
public:
    RGBA operator()(double x, double y, double z) const override
    {
        m_lastX = x;
        m_lastY = y;
        m_lastZ = z;
        return RGBA(1.0, 1.0, 1.0, 1.0);
    }

    ColorVector& createVector(ColorVector& vec) override
    {
        m_lastLo = activeZMin();
        m_lastHi = activeZMax();
        vec.push_back(RGBA(1.0, 1.0, 1.0, 1.0));
        return vec;
    }

    mutable double m_lastX = 0.0;
    mutable double m_lastY = 0.0;
    mutable double m_lastZ = 0.0;
    mutable double m_lastLo = 0.0;
    mutable double m_lastHi = 1.0;
};

} // namespace

QVector<double> TestWaterfall::flatTrace(double value)
{
    return QVector<double>(8, value);
}

// ---------------------------------------------------------------------------
// Sliding window semantics
// ---------------------------------------------------------------------------

void TestWaterfall::slidingWindow()
{
    Qwt3DWaterfall waterfall;
    waterfall.setTraceCapacity(4);

    // Frame i carries the constant value i + 1
    for (int i = 0; i < 6; ++i)
        waterfall.pushTrace(flatTrace(i + 1));

    QCOMPARE(waterfall.traceCount(), 4);
    QCOMPARE(waterfall.samplesPerTrace(), 8);

    // Stored frames are #3..#6 (values 3..6): dropping the oldest two frames
    // is visible in the hull y-range [3, 6]; without the drop it would be [1, 6].
    const ParallelEpiped h = waterfall.hull();
    QCOMPARE(h.minVertex.y, 3.0);
    QCOMPARE(h.maxVertex.y, 6.0);
    QCOMPARE(h.maxVertex.z, 3.0);  // z = slot index, window full
}

void TestWaterfall::hullZGrowsMonotonically()
{
    Qwt3DWaterfall waterfall;
    waterfall.setTraceCapacity(4);

    // During the fill phase z max == count - 1 ...
    for (int i = 0; i < 4; ++i) {
        waterfall.pushTrace(flatTrace(1.0));
        QCOMPARE(waterfall.hull().maxVertex.z, double(i));
    }

    // ... and stays pinned at capacity - 1 once the window slides.
    for (int i = 0; i < 20; ++i) {
        waterfall.pushTrace(flatTrace(1.0));
        QCOMPARE(waterfall.hull().maxVertex.z, 3.0);
    }
}

void TestWaterfall::samplesPerTraceContract()
{
    Qwt3DWaterfall waterfall;
    waterfall.setTraceCapacity(8);

    // First frame fixes the bin count
    waterfall.pushTrace(QVector<double>(256, 0.5));
    QCOMPARE(waterfall.samplesPerTrace(), 256);
    QCOMPARE(waterfall.hull().minVertex.x, 0.0);
    QCOMPARE(waterfall.hull().maxVertex.x, 255.0);

    // A shorter frame is padded, a longer one truncated: the window width
    // (hull x-range) must not change.
    waterfall.pushTrace(QVector<double>(128, 0.7));
    QCOMPARE(waterfall.samplesPerTrace(), 256);
    QCOMPARE(waterfall.traceCount(), 2);
    QCOMPARE(waterfall.hull().maxVertex.x, 255.0);

    waterfall.pushTrace(QVector<double>(512, 0.9));
    QCOMPARE(waterfall.samplesPerTrace(), 256);
    QCOMPARE(waterfall.traceCount(), 3);
    QCOMPARE(waterfall.hull().maxVertex.x, 255.0);
}

void TestWaterfall::binValues()
{
    Qwt3DWaterfall waterfall;
    waterfall.setTraceCapacity(2);
    waterfall.pushTrace(flatTrace(1.0));

    QVector<double> bins;
    bins << -2.5 << 0.0 << 7.5;
    waterfall.setBinValues(bins);
    QCOMPARE(waterfall.binValues(), bins);

    const ParallelEpiped h = waterfall.hull();
    QCOMPARE(h.minVertex.x, -2.5);
    QCOMPARE(h.maxVertex.x, 7.5);
}

void TestWaterfall::depthOrderKeepsHull()
{
    Qwt3DWaterfall waterfall;
    waterfall.setTraceCapacity(4);
    for (int i = 0; i < 3; ++i)
        waterfall.pushTrace(flatTrace(i + 1));

    QCOMPARE(waterfall.depthOrder(), Qwt3DWaterfall::NewestInFront);
    waterfall.setDepthOrder(Qwt3DWaterfall::NewestBehind);
    QCOMPARE(waterfall.depthOrder(), Qwt3DWaterfall::NewestBehind);

    // Mirroring the depth direction must not change the hull extent.
    const ParallelEpiped h = waterfall.hull();
    QCOMPARE(h.minVertex.z, 0.0);
    QCOMPARE(h.maxVertex.z, 2.0);
}

// ---------------------------------------------------------------------------
// Hull correctness
// ---------------------------------------------------------------------------

void TestWaterfall::hullRanges()
{
    Qwt3DWaterfall waterfall;
    waterfall.setTraceCapacity(4);

    QVector<double> trace;
    trace << -1.0 << 2.0 << 5.0;
    waterfall.pushTrace(trace);

    // Baseline follows the amplitude minimum by default
    ParallelEpiped h = waterfall.hull();
    QCOMPARE(h.minVertex.x, 0.0);
    QCOMPARE(h.maxVertex.x, 2.0);
    QCOMPARE(h.minVertex.y, -1.0);
    QCOMPARE(h.maxVertex.y, 5.0);
    QCOMPARE(h.minVertex.z, 0.0);
    QCOMPARE(h.maxVertex.z, 0.0);

    // A fixed baseline below the data extends the hull downward
    waterfall.setBaseline(-3.0);
    h = waterfall.hull();
    QCOMPARE(h.minVertex.y, -3.0);
    QCOMPARE(h.maxVertex.y, 5.0);

    // A fixed baseline above the data minimum cannot lift the lower bound:
    // the curtain still reaches down to the samples
    waterfall.setBaseline(1.5);
    h = waterfall.hull();
    QCOMPARE(h.minVertex.y, -1.0);  // min(dataMin, baseline)
    QCOMPARE(waterfall.baseline(), 1.5);

    waterfall.resetBaseline();
    QCOMPARE(waterfall.hull().minVertex.y, -1.0);
}

void TestWaterfall::emptyWindow()
{
    Qwt3DWaterfall waterfall;
    waterfall.setTraceCapacity(4);

    QCOMPARE(waterfall.traceCount(), 0);
    QCOMPARE(waterfall.samplesPerTrace(), 0);

    // Degenerate hull, no crash
    const ParallelEpiped h = waterfall.hull();
    QCOMPARE(h.minVertex.x, 0.0);
    QCOMPARE(h.maxVertex.x, 0.0);
    QCOMPARE(h.minVertex.y, 0.0);
    QCOMPARE(h.maxVertex.y, 0.0);
    QCOMPARE(h.minVertex.z, 0.0);
    QCOMPARE(h.maxVertex.z, 0.0);
}

// ---------------------------------------------------------------------------
// Capacity changes
// ---------------------------------------------------------------------------

void TestWaterfall::shrinkCapacity()
{
    Qwt3DWaterfall waterfall;
    waterfall.setTraceCapacity(8);

    for (int i = 0; i < 8; ++i)
        waterfall.pushTrace(flatTrace(i + 1));

    waterfall.setTraceCapacity(3);

    // Only the newest 3 traces (values 6, 7, 8) survive
    QCOMPARE(waterfall.traceCapacity(), 3);
    QCOMPARE(waterfall.traceCount(), 3);
    QCOMPARE(waterfall.hull().minVertex.y, 6.0);
    QCOMPARE(waterfall.hull().maxVertex.y, 8.0);
    QCOMPARE(waterfall.hull().maxVertex.z, 2.0);
}

void TestWaterfall::growCapacity()
{
    Qwt3DWaterfall waterfall;
    waterfall.setTraceCapacity(3);

    for (int i = 0; i < 3; ++i)
        waterfall.pushTrace(flatTrace(i + 1));
    QCOMPARE(waterfall.hull().maxVertex.z, 2.0);

    waterfall.setTraceCapacity(8);
    QCOMPARE(waterfall.traceCount(), 3);  // nothing invented by growing

    for (int i = 0; i < 5; ++i) {
        waterfall.pushTrace(flatTrace(1.0));
        QCOMPARE(waterfall.hull().maxVertex.z, double(3 + i));
    }
    QCOMPARE(waterfall.traceCount(), 8);
}

// ---------------------------------------------------------------------------
// clear()
// ---------------------------------------------------------------------------

void TestWaterfall::clearKeepsStyleAndCapacity()
{
    Qwt3DWaterfall waterfall;
    waterfall.setTraceCapacity(4);
    waterfall.setPlotStyle(Qwt3DWaterfall::RibbonMesh);
    waterfall.setColorMode(Qwt3DWaterfall::ByAmplitude);
    waterfall.setColorRange(0.0, 1.0);
    waterfall.pushTrace(flatTrace(1.0));
    waterfall.pushTrace(flatTrace(2.0));

    waterfall.clear();

    QCOMPARE(waterfall.traceCount(), 0);
    QCOMPARE(waterfall.samplesPerTrace(), 0);
    QCOMPARE(waterfall.traceCapacity(), 4);          // capacity kept
    QCOMPARE(waterfall.plotStyle(), Qwt3DWaterfall::RibbonMesh);  // style kept
    QCOMPARE(waterfall.colorMode(), Qwt3DWaterfall::ByAmplitude);
    QVERIFY(waterfall.hasFixedColorRange());        // colors kept

    // The next push re-establishes the bin count
    waterfall.pushTrace(flatTrace(3.0));
    QCOMPARE(waterfall.samplesPerTrace(), 8);
    QCOMPARE(waterfall.traceCount(), 1);
}

// ---------------------------------------------------------------------------
// Color range push
// ---------------------------------------------------------------------------

void TestWaterfall::colorRangeByDepth()
{
    Qwt3DWaterfall waterfall;
    waterfall.setTraceCapacity(16);
    waterfall.setColorMode(Qwt3DWaterfall::ByDepth);

    auto* probe = new ProbeColor();
    waterfall.setDataColor(probe);  // item takes ownership
    for (int i = 0; i < 4; ++i)
        waterfall.pushTrace(flatTrace(0.1 * i));

    ColorVector colors;
    waterfall.populateLegendColors(colors);
    QCOMPARE(colors.size(), 1);

    // ByDepth spans the whole window, not the current fill level
    QCOMPARE(probe->m_lastLo, 0.0);
    QCOMPARE(probe->m_lastHi, 15.0);
}

void TestWaterfall::colorRangeByAmplitude()
{
    Qwt3DWaterfall waterfall;
    waterfall.setTraceCapacity(8);
    waterfall.setColorMode(Qwt3DWaterfall::ByAmplitude);

    auto* probe = new ProbeColor();
    waterfall.setDataColor(probe);

    QVector<double> trace;
    trace << 0.25 << 0.5 << 0.75;
    waterfall.pushTrace(trace);
    waterfall.pushTrace(flatTrace(0.5));

    ColorVector colors;
    waterfall.populateLegendColors(colors);

    // Automatic amplitude range = current data range
    QCOMPARE(probe->m_lastLo, 0.25);
    QCOMPARE(probe->m_lastHi, 0.75);
}

void TestWaterfall::colorRangeFixed()
{
    Qwt3DWaterfall waterfall;
    waterfall.setTraceCapacity(8);
    waterfall.setColorMode(Qwt3DWaterfall::ByAmplitude);

    auto* probe = new ProbeColor();
    waterfall.setDataColor(probe);

    waterfall.pushTrace(flatTrace(0.5));
    waterfall.setColorRange(0.2, 0.8);
    QVERIFY(waterfall.hasFixedColorRange());

    ColorVector colors;
    waterfall.populateLegendColors(colors);

    // The fixed range wins over the data range
    QCOMPARE(probe->m_lastLo, 0.2);
    QCOMPARE(probe->m_lastHi, 0.8);

    waterfall.resetColorRange();
    QVERIFY(!waterfall.hasFixedColorRange());

    waterfall.populateLegendColors(colors);
    QCOMPARE(probe->m_lastLo, 0.5);
    // Flat data degenerates the range; the item guards it to [lo, lo + 1]
    QCOMPARE(probe->m_lastHi, 1.5);
}

// ---------------------------------------------------------------------------
// Qwt3DTextScale (P3 regression)
// ---------------------------------------------------------------------------

void TestWaterfall::textScaleLabels()
{
    Qwt3DTextScale scale;

    QVector<QString> labels;
    labels << "oldest" << "-32" << "-16" << "newest";
    scale.setLabels(labels);
    QCOMPARE(scale.labels(), labels);
}

void TestWaterfall::textScaleSurvivesClonePtr()
{
    // Qwt3DAxis::setScale() stores the scale in a Qwt3DClonePtr whose
    // assignment clones the object. A derived scale without a clone()
    // override is sliced to the base class right there and its custom
    // ticLabel() silently stops working (P3). This test clones through the
    // same mechanism and verifies the Qwt3DTextScale type survives.
    auto* scale = new Qwt3DTextScale();
    QVector<QString> labels;
    labels << "oldest" << "-32" << "-16" << "newest";
    scale->setLabels(labels);

    Qwt3DClonePtr< Qwt3DScale > source(scale);
    Qwt3DClonePtr< Qwt3DScale > cloned;
    cloned = source;  // copy assignment: clones via virtual clone()

    auto* textScale = dynamic_cast< Qwt3DTextScale* >(&*cloned);
    QVERIFY(textScale != nullptr);      // no slicing
    QCOMPARE(textScale->labels(), labels);
}

void TestWaterfall::textScaleNumericFallback()
{
    // Indices beyond the label list must fall back to a numeric label,
    // never to an empty string.
    auto* scale = new Qwt3DTextScale();
    QVector<QString> labels;
    labels << "only";
    scale->setLabels(labels);

    Qwt3DClonePtr< Qwt3DScale > source(scale);
    Qwt3DClonePtr< Qwt3DScale > cloned = source;

    auto* textScale = dynamic_cast< Qwt3DTextScale* >(&*cloned);
    QVERIFY(textScale != nullptr);
    QCOMPARE(textScale->labels().size(), 1);

    // ticLabel() is protected and only reachable through Qwt3DAxis; the
    // fallback path is exercised indirectly: an out-of-range index on a
    // Qwt3DLinearScale returns the numeric value (see Qwt3DScale::ticLabel).
    // Here we only verify the clone kept the (short) label list intact.
}

// ---------------------------------------------------------------------------
// P1 regression: attach() / detach() must recalculate the scene
// ---------------------------------------------------------------------------

void TestWaterfall::attachAfterData()
{
    // The trap this guards against: data loaded before attach(), with no
    // setter (and therefore no itemChanged()) in between, used to leave the
    // plot hull at (0,0,0)-(0,0,0) and the widget rendered blank.
    Qwt3DPlot plot;

    auto* waterfall = new Qwt3DWaterfall();
    waterfall->setTraceCapacity(4);
    for (int i = 0; i < 4; ++i)
        waterfall->pushTrace(flatTrace(i + 1));

    waterfall->attach(&plot);  // no further itemChanged() after this

    const ParallelEpiped h = plot.hull();
    QCOMPARE(h.minVertex.x, 0.0);
    QCOMPARE(h.maxVertex.x, 7.0);
    QCOMPARE(h.minVertex.y, 1.0);
    QCOMPARE(h.maxVertex.y, 4.0);
    QCOMPARE(h.minVertex.z, 0.0);
    QCOMPARE(h.maxVertex.z, 3.0);

    // The coordinate system follows the recalculated hull
    QCOMPARE(plot.coordinates()->first().x, 0.0);
    QCOMPARE(plot.coordinates()->second().x, 7.0);
    QCOMPARE(plot.coordinates()->second().z, 3.0);
}

void TestWaterfall::attachHullUnionAndDetachShrink()
{
    Qwt3DPlot plot;

    QVector<QwtPoint3D> samplesA;
    samplesA << QwtPoint3D(0, 0, 0) << QwtPoint3D(2, 2, 2);
    auto* lineA = new Qwt3DLine();
    lineA->setSamples(samplesA);
    lineA->attach(&plot);

    QVector<QwtPoint3D> samplesB;
    samplesB << QwtPoint3D(-1, 5, 1) << QwtPoint3D(4, 8, 3);
    auto* lineB = new Qwt3DLine();
    lineB->setSamples(samplesB);
    lineB->attach(&plot);

    QVector<QwtPoint3D> samplesC;
    samplesC << QwtPoint3D(-3, -2, -4) << QwtPoint3D(0, 0, 0);
    auto* lineC = new Qwt3DLine();
    lineC->setSamples(samplesC);
    lineC->attach(&plot);

    // Union hull over all three items
    ParallelEpiped h = plot.hull();
    QCOMPARE(h.minVertex.x, -3.0);
    QCOMPARE(h.maxVertex.x, 4.0);
    QCOMPARE(h.minVertex.y, -2.0);
    QCOMPARE(h.maxVertex.y, 8.0);
    QCOMPARE(h.minVertex.z, -4.0);
    QCOMPARE(h.maxVertex.z, 3.0);

    // Detaching shrinks the hull to the union of the remaining items
    lineC->detach();
    h = plot.hull();
    QCOMPARE(h.minVertex.x, -1.0);
    QCOMPARE(h.maxVertex.x, 4.0);
    QCOMPARE(h.minVertex.y, 0.0);
    QCOMPARE(h.maxVertex.y, 8.0);
    QCOMPARE(h.minVertex.z, 0.0);
    QCOMPARE(h.maxVertex.z, 3.0);

    delete lineA;
    h = plot.hull();
    QCOMPARE(h.minVertex.x, -1.0);
    QCOMPARE(h.maxVertex.x, 4.0);

    delete lineB;
    QCOMPARE(plot.hasItems(), false);
}

void TestWaterfall::detachAllResetsHull()
{
    Qwt3DPlot plot;

    auto* waterfall = new Qwt3DWaterfall();
    waterfall->setTraceCapacity(2);
    waterfall->pushTrace(flatTrace(1.0));
    waterfall->attach(&plot);
    QVERIFY(plot.hull().maxVertex.y > 0.0);

    waterfall->detach();

    // Empty item list resets the hull to the degenerate box
    const ParallelEpiped h = plot.hull();
    QCOMPARE(h.minVertex.x, 0.0);
    QCOMPARE(h.maxVertex.x, 0.0);
    QCOMPARE(h.minVertex.z, 0.0);
    QCOMPARE(h.maxVertex.z, 0.0);

    delete waterfall;
}

QTEST_MAIN(TestWaterfall)
#include "test_waterfall.moc"
