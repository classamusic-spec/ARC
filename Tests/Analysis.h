#pragma once

// Offline audio analysis used by the ARC test-suite. Double precision, not realtime.

#include <cmath>
#include <string>
#include <vector>

namespace arctest
{
using Signal = std::vector<float>;

/** Magnitude spectrum (Hann window, zero padded to >= minSize, power of two). */
struct Spectrum
{
    std::vector<double> mag; // linear magnitude, bins 0..N/2
    double binHz = 0.0;
    int fftSize = 0;
};

Spectrum computeSpectrum (const Signal& x, double sampleRate, int minSize = 1 << 16, int start = 0, int length = -1);

/** Frequency of the largest spectral peak in [fLo, fHi], refined by
    parabolic interpolation of the log-magnitude. Returns 0 if none. */
double peakFrequency (const Spectrum& s, double fLo, double fHi, double* peakMagnitude = nullptr);
double peakFrequency (const Signal& x, double sampleRate, double fLo, double fHi);

/** High-precision frequency of an isolated component near fGuess: complex
    demodulation, then a linear fit of the unwrapped phase over the part of the
    envelope within `rangeDb` of its peak. */
double refineFrequencyByPhase (const Signal& x, double sampleRate, double fGuess, double periods = 8.0,
                               double rangeDb = 30.0);

inline double centsBetween (double measured, double expected)
{
    return 1200.0 * std::log2 (measured / expected);
}

/** Envelope (dB) of the component near `freq` via complex demodulation +
    triangular smoothing over `periods` cycles. Returns (time, dB) pairs. */
std::vector<std::pair<double, double>> componentEnvelopeDb (const Signal& x, double sampleRate, double freq,
                                                             double periods = 8.0, double spacingHz = 0.0);

/** Decay time (T60, seconds) of the component near freq, fitted between
    (peak - startDb) and (peak - endDb) or the noise floor. Returns <= 0 on failure. */
double measureT60 (const Signal& x, double sampleRate, double freq, double startDb = 5.0, double endDb = 35.0,
                   double periods = 8.0, double spacingHz = 0.0);

/** Broadband decay: T60 fitted on the RMS envelope (10 ms frames). */
double measureBroadbandT60 (const Signal& x, double sampleRate, double startDb = 5.0, double endDb = 35.0);

double spectralCentroid (const Signal& x, double sampleRate, int start = 0, int length = -1,
                         double maxHz = 20000.0);
double rms (const Signal& x, int start = 0, int length = -1);
double peakAbs (const Signal& x);
bool allFinite (const Signal& x);

/** Energy above cutoffHz relative to total (dB), via FFT of the given range. */
double highBandRatioDb (const Signal& x, double sampleRate, double cutoffHz, int start = 0, int length = -1);

struct Partial
{
    double freq;
    double magDb;
};

/** The `count` strongest spectral peaks between fLo and fHi, sorted by frequency.
    Peaks closer than minSeparationHz to a stronger one are dropped. */
std::vector<Partial> strongestPartials (const Spectrum& s, int count, double fLo, double fHi,
                                        double minSeparationHz);

/** Tracks partials 1..K sequentially from f1 (each searched around the previous
    partial plus the previous spacing) and returns their frequencies (0 if lost). */
std::vector<double> trackPartials (const Spectrum& s, double f1, int K);

/** Mean |f_k / (k f_1) - 1| over tracked partials k = 2..K. */
double inharmonicity (const Spectrum& s, double f1, int K);

void writeWav (const std::string& path, const Signal& left, const Signal& right, double sampleRate);
void writeWav (const std::string& path, const Signal& mono, double sampleRate);

/** Wall-clock ns per call of fn over `iterations` calls (fn must be cheap). */
template <typename Fn>
double nanosPerCall (Fn&& fn, long iterations);

} // namespace arctest

#include <chrono>
#include <cmath>

template <typename Fn>
double arctest::nanosPerCall (Fn&& fn, long iterations)
{
    const auto t0 = std::chrono::steady_clock::now();
    for (long i = 0; i < iterations; ++i)
        fn();
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::nano> (t1 - t0).count() / static_cast<double> (iterations);
}
