#include "SpectrumGenerator.h"

#include <qmath.h>

#include "qwt_global.h"

#include <algorithm>

// Share one fixed Gaussian kernel width sampling: peaks use a Gaussian shape
// exp(-0.5 * ((i - c) / (w / 2.355)) ^ 2) where w is the FWHM in bins.

namespace
{

const double PEAK_FWHM_MIN = 6.0;
const double PEAK_FWHM_MAX = 18.0;
const double INTERFERENCE_PROBABILITY = 0.02;  // per frame
const double INTERFERENCE_DECAY = 0.75;
const double MAXHOLD_DECAY = 0.995;

} // namespace

SpectrumGenerator::SpectrumGenerator(int binCount)
    : m_binCount(binCount)
    , m_rng(42u)
    , m_unit(0.0, 1.0)
{
    setBinCount(binCount);
}

int SpectrumGenerator::binCount() const
{
    return m_binCount;
}

void SpectrumGenerator::setBinCount(int count)
{
    m_binCount = std::max(16, count);

    m_bins.resize(m_binCount);
    for (int i = 0; i < m_binCount; ++i)
        m_bins[i] = double(i);

    m_frame = QVector<double>(m_binCount, 0.0);
    m_peakHold = QVector<double>(m_binCount, 0.0);

    rebuildPeaks();
}

void SpectrumGenerator::setSeed(unsigned seed)
{
    m_rng.seed(seed);
    reset();
}

void SpectrumGenerator::rebuildPeaks()
{
    m_peaks.clear();

    // Three peaks with distinct Lissajous drift parameters
    for (int p = 0; p < 3; ++p) {
        Peak peak;
        peak.centerPhase = m_unit(m_rng) * 6.283;
        // Low frequency Lissajous trajectories; different per peak
        peak.centerFreq1 = 0.011 + 0.004 * p;
        peak.centerFreq2 = 0.007 + 0.005 * p;
        peak.ampPhase = m_unit(m_rng) * 6.283;
        peak.ampFreq = 0.017 + 0.003 * p;
        peak.ampBase = 0.35 + 0.25 * p / 2.0;
        peak.width = PEAK_FWHM_MIN + m_unit(m_rng) * (PEAK_FWHM_MAX - PEAK_FWHM_MIN);
        m_peaks.append(peak);
    }
}

const QVector<double>& SpectrumGenerator::nextFrame()
{
    ++m_frameIndex;

    // 1. Noise floor with slow ripple + gaussian noise
    const double t = double(m_frameIndex);
    const double floorLevel = 0.03 * (0.5 + 0.5 * std::sin(0.05 * t));
    for (int i = 0; i < m_binCount; ++i) {
        // Cheap deterministic pseudo noise: folded sine + frame offset
        const double n = std::sin(0.7 * i + 2.3 * t) * std::sin(1.13 * i - 0.7 * t);
        m_frame[ i ] = floorLevel * (0.5 + 0.5 * n) + 0.008 * std::sin(9.1 * i + t);
    }

    // 2. Drifting peaks (Lissajous center + slow amplitude breathing)
    for (const Peak& peak : qwt_as_const(m_peaks)) {
        const double cx = 0.5 + 0.35 * std::sin(peak.centerFreq1 * t + peak.centerPhase);
        const double cy = 0.5 + 0.35 * std::sin(peak.centerFreq2 * t + peak.centerPhase * 1.7);
        // Combine both into the bin coordinate
        const double center = 0.5 * (cx + cy) * m_binCount;
        const double amp = peak.ampBase * (0.6 + 0.4 * std::sin(peak.ampFreq * t + peak.ampPhase));
        const double sigma = peak.width / 2.355;

        const int from = std::max(0, int(center - 4.0 * sigma) - 1);
        const int to = std::min(m_binCount - 1, int(center + 4.0 * sigma) + 1);
        for (int i = from; i <= to; ++i) {
            const double dz = (i - center) / sigma;
            m_frame[ i ] += amp * std::exp(-0.5 * dz * dz);
        }
    }

    // 3. Occasional decaying narrowband interference (transient)
    if (m_interferenceAmp > 0.02) {
        m_frame[ int(m_interferencePos) ] += m_interferenceAmp;
        if (m_interferencePos + 1 < m_binCount)
            m_frame[ int(m_interferencePos) + 1 ] += 0.5 * m_interferenceAmp;
        m_interferenceAmp *= INTERFERENCE_DECAY;
    } else if (m_unit(m_rng) < INTERFERENCE_PROBABILITY) {
        m_interferencePos = 2.0 + m_unit(m_rng) * (m_binCount - 5);
        m_interferenceAmp = 0.6 + 0.4 * m_unit(m_rng);
    }

    // 4. Clamp to the documented [0, 1] amplitude window
    for (int i = 0; i < m_binCount; ++i)
        m_frame[ i ] = std::min(1.0, std::max(0.0, m_frame[ i ]));

    // 5. Peak hold with slow decay
    for (int i = 0; i < m_binCount; ++i)
        m_peakHold[ i ] = std::max(m_frame[ i ], m_peakHold[ i ] * MAXHOLD_DECAY);

    return m_frame;
}

const QVector<double>& SpectrumGenerator::peakHold() const
{
    return m_peakHold;
}

const QVector<double>& SpectrumGenerator::binValues() const
{
    return m_bins;
}

void SpectrumGenerator::reset()
{
    m_frame = QVector<double>(m_binCount, 0.0);
    m_peakHold = QVector<double>(m_binCount, 0.0);
    m_frameIndex = 0;
    m_interferencePos = -1.0;
    m_interferenceAmp = 0.0;
}
