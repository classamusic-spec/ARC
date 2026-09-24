#include "Analysis.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <algorithm>
#include <complex>
#include <numbers>

namespace arctest
{
namespace
{
constexpr double kPi = std::numbers::pi;

void fft (std::vector<std::complex<double>>& a)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap (a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = -2.0 * kPi / static_cast<double> (len);
        const std::complex<double> wl (std::cos (ang), std::sin (ang));
        for (size_t i = 0; i < n; i += len)
        {
            std::complex<double> w (1.0, 0.0);
            for (size_t j = 0; j < len / 2; ++j)
            {
                const auto u = a[i + j];
                const auto v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

int nextPow2 (int v)
{
    int p = 1;
    while (p < v)
        p <<= 1;
    return p;
}

void resolveRange (const Signal& x, int& start, int& length)
{
    start = std::clamp (start, 0, static_cast<int> (x.size()));
    if (length < 0 || start + length > static_cast<int> (x.size()))
        length = static_cast<int> (x.size()) - start;
}
} // namespace

Spectrum computeSpectrum (const Signal& x, double sampleRate, int minSize, int start, int length)
{
    resolveRange (x, start, length);
    const int n = nextPow2 (std::max (minSize, length));
    std::vector<std::complex<double>> buf (static_cast<size_t> (n));
    for (int i = 0; i < length; ++i)
    {
        const double w = 0.5 - 0.5 * std::cos (2.0 * kPi * (i + 0.5) / length);
        buf[static_cast<size_t> (i)] = x[static_cast<size_t> (start + i)] * w;
    }
    fft (buf);
    Spectrum s;
    s.fftSize = n;
    s.binHz = sampleRate / n;
    s.mag.resize (static_cast<size_t> (n / 2 + 1));
    for (int i = 0; i <= n / 2; ++i)
        s.mag[static_cast<size_t> (i)] = std::abs (buf[static_cast<size_t> (i)]);
    return s;
}

double peakFrequency (const Spectrum& s, double fLo, double fHi, double* peakMagnitude)
{
    const int lo = std::max (1, static_cast<int> (std::floor (fLo / s.binHz)));
    const int hi = std::min (static_cast<int> (s.mag.size()) - 2, static_cast<int> (std::ceil (fHi / s.binHz)));
    if (hi <= lo)
        return 0.0;
    int best = lo;
    for (int i = lo; i <= hi; ++i)
        if (s.mag[static_cast<size_t> (i)] > s.mag[static_cast<size_t> (best)])
            best = i;
    const double a = std::log (s.mag[static_cast<size_t> (best - 1)] + 1e-300);
    const double b = std::log (s.mag[static_cast<size_t> (best)] + 1e-300);
    const double c = std::log (s.mag[static_cast<size_t> (best + 1)] + 1e-300);
    const double denom = a - 2.0 * b + c;
    const double delta = std::abs (denom) > 1e-12 ? 0.5 * (a - c) / denom : 0.0;
    if (peakMagnitude != nullptr)
        *peakMagnitude = std::exp (b - 0.25 * (a - c) * delta);
    return (best + std::clamp (delta, -0.5, 0.5)) * s.binHz;
}

double peakFrequency (const Signal& x, double sampleRate, double fLo, double fHi)
{
    return peakFrequency (computeSpectrum (x, sampleRate, 1 << 18), fLo, fHi);
}

std::vector<std::pair<double, double>> componentEnvelopeDb (const Signal& x, double sampleRate, double freq,
                                                             double periods, double spacingHz)
{
    // The smoothing window spans `periods` cycles of the partial spacing, so
    // neighbouring harmonics fall into the (triangular) window's nulls.
    const double spacing = spacingHz > 0.0 ? spacingHz : freq;
    const int w = std::max (16, static_cast<int> (std::lround (periods * sampleRate / spacing)));
    const size_t n = x.size();
    std::vector<std::complex<double>> demod (n);
    const double dw = 2.0 * kPi * freq / sampleRate;
    for (size_t i = 0; i < n; ++i)
        demod[i] = static_cast<double> (x[i]) * std::polar (1.0, -dw * static_cast<double> (i));

    // Two cascaded boxcars (triangular window) via running sums.
    auto boxcar = [w] (const std::vector<std::complex<double>>& in)
    {
        std::vector<std::complex<double>> out (in.size());
        std::complex<double> acc = 0.0;
        for (size_t i = 0; i < in.size(); ++i)
        {
            acc += in[i];
            if (i >= static_cast<size_t> (w))
                acc -= in[i - static_cast<size_t> (w)];
            out[i] = acc / static_cast<double> (w);
        }
        return out;
    };
    const auto sm = boxcar (boxcar (demod));

    std::vector<std::pair<double, double>> env;
    const int hop = std::max (16, w / 4);
    for (size_t i = static_cast<size_t> (2 * w); i < n; i += static_cast<size_t> (hop))
    {
        const double t = (static_cast<double> (i) - w) / sampleRate; // centre of the 2w window
        env.emplace_back (t, 20.0 * std::log10 (2.0 * std::abs (sm[i]) + 1e-300));
    }
    return env;
}

double refineFrequencyByPhase (const Signal& x, double sampleRate, double fGuess, double periods, double rangeDb)
{
    const int w = std::max (64, static_cast<int> (std::lround (periods * sampleRate / fGuess)));
    const size_t n = x.size();
    const double dw = 2.0 * kPi * fGuess / sampleRate;
    std::vector<std::complex<double>> demod (n);
    for (size_t i = 0; i < n; ++i)
        demod[i] = static_cast<double> (x[i]) * std::polar (1.0, -dw * static_cast<double> (i));
    auto boxcar = [w] (const std::vector<std::complex<double>>& in)
    {
        std::vector<std::complex<double>> out (in.size());
        std::complex<double> acc = 0.0;
        for (size_t i = 0; i < in.size(); ++i)
        {
            acc += in[i];
            if (i >= static_cast<size_t> (w))
                acc -= in[i - static_cast<size_t> (w)];
            out[i] = acc / static_cast<double> (w);
        }
        return out;
    };
    const auto sm = boxcar (boxcar (demod));

    double peak = 0;
    for (size_t i = static_cast<size_t> (2 * w); i < n; ++i)
        peak = std::max (peak, std::abs (sm[i]));
    const double thresh = peak * std::pow (10.0, -rangeDb / 20.0);

    double sx = 0, sy = 0, sxx = 0, sxy = 0, prevPhase = 0, unwrap = 0;
    int cnt = 0;
    bool first = true;
    const int hop = std::max (1, w / 8);
    for (size_t i = static_cast<size_t> (2 * w); i < n; i += static_cast<size_t> (hop))
    {
        if (std::abs (sm[i]) < thresh)
        {
            if (! first)
                break;
            continue;
        }
        const double ph = std::arg (sm[i]);
        if (! first)
        {
            double d = ph - prevPhase;
            while (d > kPi) d -= 2 * kPi;
            while (d < -kPi) d += 2 * kPi;
            unwrap += d;
        }
        else
            unwrap = ph;
        prevPhase = ph;
        first = false;
        const double t = static_cast<double> (i) / sampleRate;
        sx += t; sy += unwrap; sxx += t * t; sxy += t * unwrap;
        ++cnt;
    }
    if (cnt < 8)
        return fGuess;
    const double slope = (cnt * sxy - sx * sy) / (cnt * sxx - sx * sx); // rad / s
    return fGuess + slope / (2.0 * kPi);
}

namespace
{
double fitDecay (const std::vector<std::pair<double, double>>& env, double startDb, double endDb)
{
    if (env.size() < 4)
        return -1.0;
    size_t peakIdx = 0;
    for (size_t i = 0; i < env.size(); ++i)
        if (env[i].second > env[peakIdx].second)
            peakIdx = i;
    const double peakDb = env[peakIdx].second;

    // Noise floor estimate: minimum of the tail.
    double floorDb = 1e9;
    for (size_t i = env.size() * 3 / 4; i < env.size(); ++i)
        floorDb = std::min (floorDb, env[i].second);
    const double stopDb = std::max (peakDb - endDb, floorDb + 10.0);

    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    int cnt = 0;
    for (size_t i = peakIdx; i < env.size(); ++i)
    {
        const double db = env[i].second;
        if (db > peakDb - startDb)
            continue;
        if (db < stopDb)
            break;
        const double t = env[i].first;
        sx += t;
        sy += db;
        sxx += t * t;
        sxy += t * db;
        ++cnt;
    }
    if (cnt < 3)
        return -1.0;
    const double slope = (cnt * sxy - sx * sy) / (cnt * sxx - sx * sx); // dB / s
    if (slope >= 0.0)
        return 1.0e9;
    return -60.0 / slope;
}
} // namespace

double measureT60 (const Signal& x, double sampleRate, double freq, double startDb, double endDb, double periods,
                   double spacingHz)
{
    return fitDecay (componentEnvelopeDb (x, sampleRate, freq, periods, spacingHz), startDb, endDb);
}

double measureBroadbandT60 (const Signal& x, double sampleRate, double startDb, double endDb)
{
    const int frame = static_cast<int> (0.01 * sampleRate);
    std::vector<std::pair<double, double>> env;
    for (size_t i = 0; i + static_cast<size_t> (frame) <= x.size(); i += static_cast<size_t> (frame))
    {
        double e = 0;
        for (int j = 0; j < frame; ++j)
            e += static_cast<double> (x[i + static_cast<size_t> (j)]) * x[i + static_cast<size_t> (j)];
        env.emplace_back (static_cast<double> (i) / sampleRate, 10.0 * std::log10 (e / frame + 1e-300));
    }
    return fitDecay (env, startDb, endDb);
}

double spectralCentroid (const Signal& x, double sampleRate, int start, int length, double maxHz)
{
    const auto s = computeSpectrum (x, sampleRate, 1 << 14, start, length);
    double num = 0, den = 0;
    for (size_t i = 1; i < s.mag.size(); ++i)
    {
        if (static_cast<double> (i) * s.binHz > maxHz)
            break;
        num += static_cast<double> (i) * s.binHz * s.mag[i];
        den += s.mag[i];
    }
    return den > 0 ? num / den : 0.0;
}

double rms (const Signal& x, int start, int length)
{
    resolveRange (x, start, length);
    if (length <= 0)
        return 0.0;
    double e = 0;
    for (int i = 0; i < length; ++i)
        e += static_cast<double> (x[static_cast<size_t> (start + i)]) * x[static_cast<size_t> (start + i)];
    return std::sqrt (e / length);
}

double peakAbs (const Signal& x)
{
    double p = 0;
    for (float v : x)
        p = std::max (p, static_cast<double> (std::abs (v)));
    return p;
}

bool allFinite (const Signal& x)
{
    return std::all_of (x.begin(), x.end(), [] (float v) { return std::isfinite (v); });
}

double highBandRatioDb (const Signal& x, double sampleRate, double cutoffHz, int start, int length)
{
    const auto s = computeSpectrum (x, sampleRate, 1 << 12, start, length);
    double hi = 0, tot = 0;
    for (size_t i = 1; i < s.mag.size(); ++i)
    {
        const double p = s.mag[i] * s.mag[i];
        tot += p;
        if (static_cast<double> (i) * s.binHz >= cutoffHz)
            hi += p;
    }
    return 10.0 * std::log10 ((hi + 1e-300) / (tot + 1e-300));
}

std::vector<Partial> strongestPartials (const Spectrum& s, int count, double fLo, double fHi, double minSeparationHz)
{
    std::vector<Partial> candidates;
    const int lo = std::max (2, static_cast<int> (fLo / s.binHz));
    const int hi = std::min (static_cast<int> (s.mag.size()) - 3, static_cast<int> (fHi / s.binHz));
    for (int i = lo; i <= hi; ++i)
    {
        const double m = s.mag[static_cast<size_t> (i)];
        if (m > s.mag[static_cast<size_t> (i - 1)] && m >= s.mag[static_cast<size_t> (i + 1)]
            && m > s.mag[static_cast<size_t> (i - 2)] && m >= s.mag[static_cast<size_t> (i + 2)])
        {
            const double a = std::log (s.mag[static_cast<size_t> (i - 1)] + 1e-300);
            const double b = std::log (m + 1e-300);
            const double c = std::log (s.mag[static_cast<size_t> (i + 1)] + 1e-300);
            const double den = a - 2 * b + c;
            const double d = std::abs (den) > 1e-12 ? 0.5 * (a - c) / den : 0.0;
            candidates.push_back ({ (i + d) * s.binHz, 20.0 * std::log10 (m + 1e-300) });
        }
    }
    std::sort (candidates.begin(), candidates.end(), [] (auto& p, auto& q) { return p.magDb > q.magDb; });
    std::vector<Partial> out;
    for (auto& c : candidates)
    {
        bool ok = true;
        for (auto& o : out)
            if (std::abs (o.freq - c.freq) < minSeparationHz)
                ok = false;
        if (ok)
            out.push_back (c);
        if (static_cast<int> (out.size()) >= count)
            break;
    }
    std::sort (out.begin(), out.end(), [] (auto& p, auto& q) { return p.freq < q.freq; });
    return out;
}

std::vector<double> trackPartials (const Spectrum& s, double f1, int K)
{
    std::vector<double> f (static_cast<size_t> (K), 0.0);
    f[0] = peakFrequency (s, f1 * 0.97, f1 * 1.03);
    if (f[0] <= 0)
        return f;
    double spacing = f[0];
    for (int k = 1; k < K; ++k)
    {
        const double prev = f[static_cast<size_t> (k - 1)];
        const double lo = prev + 0.55 * spacing, hi = prev + 1.6 * spacing;
        const double fk = peakFrequency (s, lo, hi);
        if (fk <= 0)
            break;
        f[static_cast<size_t> (k)] = fk;
        spacing = fk - prev;
    }
    return f;
}

double inharmonicity (const Spectrum& s, double f1, int K)
{
    const auto f = trackPartials (s, f1, K);
    if (f[0] <= 0)
        return 0.0;
    double acc = 0;
    int n = 0;
    for (int k = 2; k <= K; ++k)
    {
        const double fk = f[static_cast<size_t> (k - 1)];
        if (fk <= 0)
            break;
        acc += std::abs (fk / (k * f[0]) - 1.0);
        ++n;
    }
    return n > 0 ? acc / n : 0.0;
}

void writeWav (const std::string& path, const Signal& left, const Signal& right, double sampleRate)
{
    juce::File file (path);
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
    if (stream == nullptr)
        return;
    const auto options = juce::AudioFormatWriterOptions {}
                             .withSampleRate (sampleRate)
                             .withNumChannels (2)
                             .withBitsPerSample (24);
    auto writer = wav.createWriterFor (stream, options);
    if (writer == nullptr)
        return;
    juce::AudioBuffer<float> buf (2, static_cast<int> (left.size()));
    for (size_t i = 0; i < left.size(); ++i)
    {
        buf.setSample (0, static_cast<int> (i), std::clamp (left[i], -1.0f, 1.0f));
        buf.setSample (1, static_cast<int> (i), std::clamp (i < right.size() ? right[i] : left[i], -1.0f, 1.0f));
    }
    writer->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
}

void writeWav (const std::string& path, const Signal& mono, double sampleRate)
{
    writeWav (path, mono, mono, sampleRate);
}

} // namespace arctest
