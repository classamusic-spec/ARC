#pragma once

// Filters that live inside a recirculating resonator loop.
//
// Stability contract: every filter here has |H(e^jw)| <= 1 for all w
// (loss filter strictly < 1, allpass exactly 1). The resonant network relies
// on this to guarantee stability (see docs/NETWORK_COUPLING.md).

#include <array>
#include <cmath>

#include "Synthesis/DspCore.h"

namespace arc::dsp
{

/** One-pole loss filter  G(z) = b0 / (1 - a1 z^-1),  0 <= a1 < 1.
    Designed so that |G| matches a per-pass gain g1 at w1 and g2 at w2 (w2 > w1),
    which lets a material specify decay at the fundamental and at high frequency. */
/** Upper bound for any in-loop magnitude response (strictly below unity). */
inline constexpr double kMaxLoopGain = 0.999999;

struct OnePoleLoss
{
    float b0 = 0.999f;
    float a1 = 0.0f;
    float state = 0.0f;

    inline float process (float x) noexcept
    {
        state = b0 * x + a1 * state;
        return state;
    }

    void reset() noexcept { state = 0.0f; }

    struct Coeffs
    {
        double b0, a1;
    };

    /** g1 >= g2 expected. a1 is capped at maxPole (limits phase delay). */
    static Coeffs design (double g1, double w1, double g2, double w2, double maxPole = 0.95) noexcept
    {
        g1 = clamp (g1, 1.0e-6, 0.9999999);
        g2 = clamp (g2, 1.0e-6, g1);
        w1 = clamp (w1, 1.0e-5, kPi * 0.999);
        w2 = clamp (w2, w1 * 1.0001, kPi * 0.999);

        double a = 0.0;
        const double ratioSq = (g1 / g2) * (g1 / g2);
        if (ratioSq > 1.0 + 1.0e-12)
        {
            // Maximum achievable ratio with a one-pole (a -> 1).
            const double maxRatio = std::sin (0.5 * w2) / std::sin (0.5 * w1);
            const double r = std::min (ratioSq, maxRatio * maxRatio * 0.999);
            const double p = (r * std::cos (w1) - std::cos (w2)) / (r - 1.0);
            if (p > 1.0)
                a = p - std::sqrt (p * p - 1.0);
            else
                a = 1.0;
        }
        a = clamp (a, 0.0, maxPole);
        double b = g1 * std::sqrt (1.0 - 2.0 * a * std::cos (w1) + a * a);
        // Hard stability bound: a lowpass one-pole peaks at DC, so |G| <= G(0) < 1.
        // (When the pole is clamped the two-point match alone could exceed unity at DC.)
        b = std::min (b, (1.0 - a) * kMaxLoopGain);
        return { b, a };
    }

    static double magnitude (double b0v, double a1v, double w) noexcept
    {
        return b0v / std::sqrt (1.0 - 2.0 * a1v * std::cos (w) + a1v * a1v);
    }

    /** Phase delay in samples at w. */
    static double phaseDelay (double a1v, double w) noexcept
    {
        return std::atan2 (a1v * std::sin (w), 1.0 - a1v * std::cos (w)) / w;
    }
};

/** First-order allpass  H(z) = (a + z^-1) / (1 + a z^-1). */
inline double allpassPhaseDelay (double a, double w) noexcept
{
    const double s = std::sin (w), c = std::cos (w);
    const double phase = -std::atan2 (s, a + c) + std::atan2 (a * s, 1.0 + a * c);
    return -phase / w;
}

/** Coefficient of the first-order allpass whose phase delay at w is exactly tau:
    a = sin(w(1-tau)/2) / sin(w(1+tau)/2)   (reduces to Thiran (1-tau)/(1+tau) as w->0).
    Returns false when no stable solution with |a| <= 0.95 exists (very near Nyquist). */
inline bool allpassForPhaseDelay (double tau, double w, double& aOut) noexcept
{
    const double den = std::sin (0.5 * w * (1.0 + tau));
    if (0.5 * w * (1.0 + tau) >= kPi * 0.98 || std::abs (den) < 1.0e-9)
        return false;
    const double a = std::sin (0.5 * w * (1.0 - tau)) / den;
    if (! std::isfinite (a) || std::abs (a) > 0.95)
        return false;
    aOut = a;
    return true;
}

/** Cascade of identical first-order allpasses providing stiffness-like
    dispersion (higher partials travel faster -> stretched, inharmonic partials).

    Design (sample-rate independent): each stage is the bilinear transform of the
    analog allpass (p - s)/(p + s), whose DC group delay 2/p is set to
    amount * T0 / K (T0 = period). In samples: D = amount * L / K and
    a = (1 - D) / (1 + D). The analog pole p = 2 K f0 / amount does not depend on
    the sample rate, so the partial-stretch pattern is rate- and pitch-invariant
    (up to bilinear warping near Nyquist). D is floored at 1 (a <= 0): a stage can
    only stretch partials upward. */
template <int MaxStages>
struct AllpassChain
{
    std::array<float, static_cast<size_t> (MaxStages)> s {};
    float a = 0.0f;
    int stages = 0;

    inline float process (float x) noexcept
    {
        for (int i = 0; i < stages; ++i)
        {
            const float y = a * x + s[static_cast<size_t> (i)];
            s[static_cast<size_t> (i)] = x - a * y;
            x = y;
        }
        return x;
    }

    void reset() noexcept { s.fill (0.0f); }

    static double coefficientFor (double amount, double periodSamples, int numStages) noexcept
    {
        if (numStages <= 0 || amount <= 0.0)
            return 0.0;
        const double d = std::max (1.0, amount * periodSamples / numStages);
        return clamp ((1.0 - d) / (1.0 + d), -0.995, 0.0);
    }

    static double phaseDelay (double aCoeff, int numStages, double w) noexcept
    {
        if (numStages <= 0 || aCoeff >= 0.0) // a = 0: pure delay (a is never positive)
            return static_cast<double> (numStages);
        return numStages * allpassPhaseDelay (aCoeff, w);
    }
};

/** Modal selectivity stage for banded loops:  S(z) = (1 - s) + s * BP(z)
    where BP is the constant-0dB-peak bandpass (RBJ) centred on the loop's fundamental.
    |BP| <= 1 and BP(w0) = 1 (real), hence |S| <= 1 everywhere, S(w0) = 1 with zero
    phase (tuning at f0 unaffected), S(0) = 1 - s (DC and sub-modes damped), and the
    loop's own overtones are attenuated by ~(1 - s) per pass. s = 0 is a bypass. */
struct SelectivityStage
{
    float s = 0.0f;
    float b0 = 0.0f, a1 = 0.0f, a2 = 0.0f; // BP: b0 (1 - z^-2) / (1 + a1 z^-1 + a2 z^-2)
    float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;

    struct Coeffs
    {
        float s, b0, a1, a2;
    };

    static Coeffs design (double w0, double q, double selectivity) noexcept
    {
        Coeffs c {};
        c.s = static_cast<float> (clamp (selectivity, 0.0, 0.98));
        w0 = clamp (w0, 1.0e-4, kPi * 0.98);
        const double alpha = std::sin (w0) / (2.0 * q);
        const double a0 = 1.0 + alpha;
        c.b0 = static_cast<float> (alpha / a0);
        c.a1 = static_cast<float> (-2.0 * std::cos (w0) / a0);
        c.a2 = static_cast<float> ((1.0 - alpha) / a0);
        return c;
    }

    void set (const Coeffs& c) noexcept
    {
        s = c.s;
        b0 = c.b0;
        a1 = c.a1;
        a2 = c.a2;
    }

    inline float process (float x) noexcept
    {
        if (s <= 0.0f)
            return x;
        const float bp = b0 * (x - x2) - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = bp;
        return x + s * (bp - x);
    }

    void reset() noexcept { x1 = x2 = y1 = y2 = 0.0f; }

    /** Complex response at w (for analysis / coupling compensation). */
    static void response (const Coeffs& c, double w, double& re, double& im) noexcept
    {
        // BP(e^jw) = b0 (1 - e^-2jw) / (1 + a1 e^-jw + a2 e^-2jw)
        const double b0 = c.b0, a1 = c.a1, a2 = c.a2, sel = c.s;
        const double nr = b0 * (1.0 - std::cos (2.0 * w)), ni = b0 * std::sin (2.0 * w);
        const double dr = 1.0 + a1 * std::cos (w) + a2 * std::cos (2.0 * w);
        const double di = -a1 * std::sin (w) - a2 * std::sin (2.0 * w);
        const double den = dr * dr + di * di;
        const double br = (nr * dr + ni * di) / den, bi = (ni * dr - nr * di) / den;
        re = (1.0 - sel) + sel * br;
        im = sel * bi;
    }
};

/** Normalised DC blocker (|H| <= 1 everywhere):  H = (1+R)/2 * (1 - z^-1) / (1 - R z^-1). */
struct DcBlocker
{
    float r = 0.995f;
    float g = 0.9975f;
    float x1 = 0.0f, y1 = 0.0f;

    void setCutoff (double hz, double sampleRate) noexcept
    {
        r = static_cast<float> (std::exp (-kTwoPi * hz / sampleRate));
        g = 0.5f * (1.0f + r);
    }
    inline float process (float x) noexcept
    {
        const float y = g * (x - x1) + r * y1;
        x1 = x;
        y1 = y;
        return y;
    }
    void reset() noexcept { x1 = y1 = 0.0f; }
};

} // namespace arc::dsp
