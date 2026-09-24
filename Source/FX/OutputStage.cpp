#include "FX/OutputStage.h"

#include <cmath>

namespace arc::dsp
{

namespace
{
inline float logCosh (float x) noexcept
{
    const float a = std::abs (x);
    // log(cosh(x)) = |x| + log1p(exp(-2|x|)) - log(2), stable for large |x|.
    return a + std::log1p (std::exp (-2.0f * a)) - 0.69314718f;
}

/** First-order antiderivative anti-aliased tanh. */
inline float adaaTanh (float x, float& x1, float& f1) noexcept
{
    const float f = logCosh (x);
    const float dx = x - x1;
    float y;
    if (std::abs (dx) < 1.0e-4f)
        y = std::tanh (0.5f * (x + x1));
    else
        y = (f - f1) / dx;
    x1 = x;
    f1 = f;
    return y;
}

/** Transparent below -3 dBFS, smooth saturation above, never exceeds +-1. */
inline float safetyClip (float x) noexcept
{
    constexpr float t = 0.7079458f; // -3 dBFS
    const float a = std::abs (x);
    if (a <= t)
        return x;
    const float y = t + (1.0f - t) * std::tanh ((a - t) / (1.0f - t));
    return x < 0.0f ? -y : y;
}
} // namespace

void OutputStage::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sr = sampleRate;
    // Mutually prime, short: a small space around the object, not a hall.
    const double lineMs[4] = { 11.3, 13.7, 17.9, 21.1 };
    for (size_t i = 0; i < lines.size(); ++i)
    {
        auto& l = lines[i];
        l.delay = std::max (2, static_cast<int> (lineMs[i] * 0.001 * sr));
        const auto size = nextPowerOfTwo (static_cast<uint32_t> (l.delay + 4));
        l.buf.assign (size, 0.0f);
        l.mask = size - 1;
        // ~0.9 s decay at mid frequencies.
        l.gain = static_cast<float> (gainForT60 (l.delay / sr, 0.9));
        l.damp = static_cast<float> (std::exp (-kTwoPi * 5500.0 / sr));
    }
    const double apMs[4] = { 1.7, 2.9, 2.3, 3.7 };
    for (size_t i = 0; i < diffusers.size(); ++i)
    {
        auto& a = diffusers[i];
        a.delay = std::max (1, static_cast<int> (apMs[i] * 0.001 * sr));
        const auto size = nextPowerOfTwo (static_cast<uint32_t> (a.delay + 4));
        a.buf.assign (size, 0.0f);
        a.mask = size - 1;
    }
    const float c = smoothingCoeff (0.02, sr);
    width.coeff = space.coeff = drive.coeff = gain.coeff = c;
    dcL.setCutoff (7.0, sr);
    dcR.setCutoff (7.0, sr);
    reset();
}

void OutputStage::reset() noexcept
{
    for (auto& l : lines)
    {
        std::fill (l.buf.begin(), l.buf.end(), 0.0f);
        l.lp = 0.0f;
    }
    for (auto& a : diffusers)
        std::fill (a.buf.begin(), a.buf.end(), 0.0f);
    dcL.reset();
    dcR.reset();
    for (int c = 0; c < 2; ++c)
        adaaX1[c] = adaaF1[c] = 0.0f;
}

void OutputStage::setParameters (float widthAmount, float spaceAmount, float driveAmount, float masterGainDb) noexcept
{
    width.setTarget (clamp (widthAmount, 0.0f, 1.5f));
    space.setTarget (clamp (spaceAmount, 0.0f, 1.0f));
    drive.setTarget (clamp (driveAmount, 0.0f, 1.0f));
    gain.setTarget (dbToGain (clamp (masterGainDb, -60.0f, 6.0f)) * (masterGainDb <= -59.9f ? 0.0f : 1.0f));
}

bool OutputStage::process (float* left, float* right, int n) noexcept
{
    bool finite = true;
    for (int i = 0; i < n && finite; ++i)
        finite = std::isfinite (left[i]) && std::isfinite (right[i]);
    if (! finite)
    {
        std::fill (left, left + n, 0.0f);
        std::fill (right, right + n, 0.0f);
        reset();
        return false;
    }

    float peak[2] {}, ms[2] {};
    for (int i = 0; i < n; ++i)
    {
        float l = left[i], r = right[i];

        // Width (M/S).
        const float w = width.next();
        const float m = 0.5f * (l + r), s = 0.5f * (l - r) * w;
        l = m + s;
        r = m - s;

        // Small ambience: 4-line FDN, Householder feedback (orthogonal: A = I - 2/N 11^T).
        const float sp = space.next();
        if (sp > 1.0e-4f)
        {
            float o[4];
            for (size_t k = 0; k < 4; ++k)
            {
                auto& ln = lines[k];
                o[k] = ln.buf[(ln.pos - static_cast<uint32_t> (ln.delay)) & ln.mask];
            }
            const float sum = 0.5f * (o[0] + o[1] + o[2] + o[3]);
            const float inL = diffusers[1].process (diffusers[0].process (l));
            const float inR = diffusers[3].process (diffusers[2].process (r));
            const float in[4] = { inL, inR, inL, -inR };
            for (size_t k = 0; k < 4; ++k)
            {
                auto& ln = lines[k];
                float fb = (o[k] - sum) * ln.gain;
                ln.lp = fb + ln.damp * (ln.lp - fb);
                ln.buf[ln.pos] = ln.lp + 0.35f * in[k];
                ln.pos = (ln.pos + 1) & ln.mask;
            }
            const float wet = 0.35f * sp;
            l = l * (1.0f - 0.5f * wet) + wet * (o[0] + o[2]) * 0.5f;
            r = r * (1.0f - 0.5f * wet) + wet * (o[1] - o[3]) * 0.5f;
        }

        // Drive: ADAA tanh with level compensation, blended in with the amount.
        const float dr = drive.next();
        if (dr > 1.0e-4f)
        {
            const float pre = 1.0f + 5.0f * dr;
            const float post = 1.0f / std::sqrt (pre);
            l = l + dr * (adaaTanh (l * pre, adaaX1[0], adaaF1[0]) * post - l);
            r = r + dr * (adaaTanh (r * pre, adaaX1[1], adaaF1[1]) * post - r);
        }

        const float g = gain.next();
        l = safetyClip (dcL.process (l * g));
        r = safetyClip (dcR.process (r * g));

        left[i] = l;
        right[i] = r;
        peak[0] = std::max (peak[0], std::abs (l));
        peak[1] = std::max (peak[1], std::abs (r));
        ms[0] += l * l;
        ms[1] += r * r;
    }
    for (int c = 0; c < 2; ++c)
    {
        lastPeak[c] = peak[c];
        lastMeanSquare[c] = n > 0 ? ms[c] / static_cast<float> (n) : 0.0f;
    }
    return true;
}

} // namespace arc::dsp
