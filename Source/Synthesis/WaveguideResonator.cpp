#include "Synthesis/WaveguideResonator.h"

namespace arc::dsp
{

namespace
{
double minLineDelayFor (Interpolation mode) noexcept
{
    switch (mode)
    {
        case Interpolation::linear:    return 1.0;
        case Interpolation::hermite:   return 2.0;
        case Interpolation::lagrange3: return 2.0;
        case Interpolation::thiran1:   return 1.5;
    }
    return 2.0;
}

/** First-order allpass fractional delay matched exactly at w0, with hysteresis on k. */
FractionalDelaySetting designAllpassDelay (double line, double w0, int preferredK) noexcept
{
    FractionalDelaySetting s;
    // Keep the previous integer tap while the fractional part stays inside [0.35, 1.65).
    int k = static_cast<int> (std::floor (line - 0.5));
    if (preferredK >= 1)
    {
        const double d = line - preferredK;
        if (d >= 0.35 && d < 1.65)
            k = preferredK;
    }
    k = std::max (k, 1);
    const double tau = line - k;
    double a = 0.0;
    if (! allpassForPhaseDelay (tau, w0, a))
        a = clamp ((1.0 - tau) / (1.0 + tau), -0.95, 0.95); // near-Nyquist fallback (DC-exact)
    s.k = k;
    s.a = static_cast<float> (a);
    return s;
}
} // namespace

LoopCoefficients designLoop (const ResonatorSettings& s, double sampleRate, int maxLineDelay, int preferredK) noexcept
{
    LoopCoefficients c;
    const double f0 = clamp (s.frequency, 1.0, 0.45 * sampleRate);
    const double w0 = kTwoPi * f0 / sampleRate;
    const double period = std::min (sampleRate / f0, static_cast<double> (maxLineDelay - 4));
    const double minLine = minLineDelayFor (s.interpolation);

    // --- loss: per-pass gains at the fundamental and at the HF reference -------------
    const double periodSeconds = period / sampleRate;
    const double g1 = gainForT60 (periodSeconds, s.t60Fundamental);
    double w2 = kTwoPi * s.hfReference / sampleRate;
    w2 = clamp (std::max (w2, 1.5 * w0), w0 * 1.0001, 0.95 * kPi);
    const double g2 = std::min (g1, gainForT60 (periodSeconds, s.t60High));

    // The loss pole adds ~a/(1-a) samples of delay; keep it within 25 % of the loop.
    double maxPole = 0.95;
    const double poleBudget = 0.25 * std::max (0.0, period - minLine);
    if (poleBudget < 19.0)
        maxPole = poleBudget / (1.0 + poleBudget);

    const auto loss = OnePoleLoss::design (g1, w0, g2, w2, maxPole);
    c.lossB0 = static_cast<float> (loss.b0);
    c.lossA1 = static_cast<float> (loss.a1);
    c.lossDelay = loss.a1 > 0.0 ? OnePoleLoss::phaseDelay (loss.a1, w0) : 0.0;

    // --- dispersion ---------------------------------------------------------------------
    // Each first-order stage with a <= 0 costs >= 1 sample of delay at every frequency,
    // so the usable stage count is bounded by the loop budget.
    const double dispBudget = 0.5 * (period - c.lossDelay - minLine);
    int stages = clamp (s.dispersionStages, 0, kMaxDispersionStages);
    stages = std::min (stages, static_cast<int> (std::floor (dispBudget)));
    double dispA = 0.0, dispDelay = 0.0;
    if (stages > 0 && s.dispersion > 0.0)
    {
        dispA = AllpassChain<kMaxDispersionStages>::coefficientFor (s.dispersion, period, stages);
        dispDelay = AllpassChain<kMaxDispersionStages>::phaseDelay (dispA, stages, w0);
        if (dispDelay > dispBudget)
        {
            // Bisection on a in [dispA, 0]: chain delay is monotonic in a.
            double lo = dispA, hi = 0.0;
            for (int it = 0; it < 30; ++it)
            {
                const double mid = 0.5 * (lo + hi);
                if (AllpassChain<kMaxDispersionStages>::phaseDelay (mid, stages, w0) > dispBudget)
                    lo = mid;
                else
                    hi = mid;
            }
            dispA = hi;
            dispDelay = AllpassChain<kMaxDispersionStages>::phaseDelay (dispA, stages, w0);
        }
    }
    else
    {
        stages = 0;
    }
    c.dispA = static_cast<float> (dispA);
    c.dispStages = stages;
    c.dispersionDelay = stages > 0 ? dispDelay : 0.0;

    // --- line delay -------------------------------------------------------------------
    double line = period - c.lossDelay - c.dispersionDelay;
    line = clamp (line, minLine, static_cast<double> (maxLineDelay - 4));
    c.lineDelay = line;
    c.totalDelay = period;

    if (s.interpolation == Interpolation::thiran1)
        c.delay = designAllpassDelay (line, w0, preferredK);
    else
        c.delay = FractionalDelaySetting::make (line, s.interpolation);

    return c;
}

LoopCoefficients retuneLoop (const LoopCoefficients& base, double frequency, double sampleRate, int maxLineDelay,
                             Interpolation interpolation, int preferredK) noexcept
{
    LoopCoefficients c = base;
    const double f0 = clamp (frequency, 1.0, 0.45 * sampleRate);
    const double w0 = kTwoPi * f0 / sampleRate;
    const double period = std::min (sampleRate / f0, static_cast<double> (maxLineDelay - 4));
    double line = period - c.lossDelay - c.dispersionDelay;
    line = clamp (line, minLineDelayFor (interpolation), static_cast<double> (maxLineDelay - 4));
    c.lineDelay = line;
    c.totalDelay = period;
    if (interpolation == Interpolation::thiran1)
        c.delay = designAllpassDelay (line, w0, preferredK);
    else
        c.delay = FractionalDelaySetting::make (line, interpolation);
    return c;
}

void WaveguideResonator::prepare (double newSampleRate, double minFrequency)
{
    sampleRate = newSampleRate;
    const int maxDelay = static_cast<int> (std::ceil (sampleRate / std::max (minFrequency, 1.0))) + 16;
    line.allocate (maxDelay);
    hasCoeffs = false;
    reset();
}

void WaveguideResonator::setCoefficients (const LoopCoefficients& c, int rampSamples) noexcept
{
    const bool canRamp = hasCoeffs && rampSamples > 0 && c.delay.k == current.k;
    coeffs = c;
    dispersion.a = c.dispA;
    dispersion.stages = c.dispStages;
    if (canRamp)
    {
        rampA = (c.delay.a - current.a) / static_cast<float> (rampSamples);
        rampF = (c.delay.f - current.f) / static_cast<float> (rampSamples);
        rampRemaining = rampSamples;
    }
    else
    {
        current = c.delay;
        rampRemaining = 0;
    }
    hasCoeffs = true;
}

void WaveguideResonator::reset() noexcept
{
    line.clear();
    dispersion.reset();
    thiranState = 0.0f;
    lossState = 0.0f;
}

void WaveguideResonator::clearState() noexcept
{
    line.clearRecent (static_cast<int> (coeffs.lineDelay) + 4);
    dispersion.reset();
    thiranState = 0.0f;
    lossState = 0.0f;
}

} // namespace arc::dsp
