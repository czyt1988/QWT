#ifndef SPECTRUMGENERATOR_H
#define SPECTRUMGENERATOR_H

#include <QVector>

#include <random>

/**
 * @brief Simulated spectrum analyzer data source
 * @details Produces a stream of one-dimensional frames that look like a real
 *          spectrum analyzer: drifting Gaussian peaks on slowly modulated
 *          noise, plus occasional decaying narrowband interference. Frames
 *          never repeat exactly but remain reproducible for a given seed.
 *
 * The generator reuses internal buffers - nextFrame() performs zero heap
 * allocations, which keeps the 30 Hz update loop free of allocation jitter.
 */
class SpectrumGenerator
{
public:
    explicit SpectrumGenerator(int binCount = 256);

    /// Returns the number of bins per frame
    int binCount() const;
    /// Sets the bin count (resets the generator)
    void setBinCount(int count);

    /// Sets the RNG seed (resets the generator)
    void setSeed(unsigned seed);

    /// Produces the next frame; the reference stays valid until the next call
    const QVector<double>& nextFrame();

    /// Returns the peak-hold spectrum of all frames produced so far
    const QVector<double>& peakHold() const;

    /// Returns the bin center values (0 .. binCount-1)
    const QVector<double>& binValues() const;

    /// Clears all state (peak hold, peak phases, interference); keeps the seed
    void reset();

private:
    void rebuildPeaks();

    int m_binCount;
    QVector<double> m_bins;
    QVector<double> m_frame;
    QVector<double> m_peakHold;

    unsigned m_frameIndex = 0;

    // Drifting peaks: center / amplitude / width per peak
    struct Peak
    {
        double centerPhase = 0.0;
        double centerFreq1 = 0.0;
        double centerFreq2 = 0.0;
        double ampPhase = 0.0;
        double ampFreq = 0.0;
        double ampBase = 0.5;
        double width = 10.0;
    };
    QVector<Peak> m_peaks;

    // Decaying narrowband interference
    double m_interferencePos = -1.0;
    double m_interferenceAmp = 0.0;

    std::mt19937 m_rng;
    std::uniform_real_distribution< double > m_unit;
};

#endif // SPECTRUMGENERATOR_H
