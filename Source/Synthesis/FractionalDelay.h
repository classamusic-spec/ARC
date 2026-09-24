#pragma once

// Circular delay line with four fractional interpolators.
//
// Convention: during sample n the reader runs *before* the writer. A read with
// integer delay k returns x[n - k] (k >= 1). Interpolators:
//
//   Linear      2 taps,  delay = k + f,  f in [0, 1)      needs k >= 1
//   Hermite     4 taps,  delay = k + f,  f in [0, 1)      needs k >= 2
//   Lagrange3   4 taps,  delay = k + f,  f in [0, 1)      needs k >= 2
//   Thiran1     1st-order allpass, delay = k + d, d in [0.5, 1.5)  needs k >= 1
//
// See docs/RESONATOR_DESIGN.md for the measured comparison that selected the
// interpolator used inside the resonant network.

#include <cstdint>
#include <vector>

#include "Synthesis/DspCore.h"

namespace arc::dsp
{

enum class Interpolation : int
{
    linear = 0,
    hermite,
    lagrange3,
    thiran1
};

class DelayLine
{
public:
    /** Allocates storage for delays up to maxDelaySamples (not realtime safe). */
    void allocate (int maxDelaySamples)
    {
        const auto size = nextPowerOfTwo (static_cast<uint32_t> (maxDelaySamples + 8));
        buffer.assign (size, 0.0f);
        mask = size - 1;
        writePos = 0;
    }

    void clear() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
    }

    /** Clears only the most recent `samples` samples (cheap voice reset). */
    void clearRecent (int samples) noexcept
    {
        const int n = std::min (samples + 8, static_cast<int> (buffer.size()));
        for (int i = 1; i <= n; ++i)
            buffer[(writePos - static_cast<uint32_t> (i)) & mask] = 0.0f;
    }

    int maxDelay() const noexcept { return static_cast<int> (buffer.size()) - 8; }

    inline void write (float x) noexcept
    {
        buffer[writePos] = x;
        writePos = (writePos + 1) & mask;
    }

    /** x[n - k], k >= 1. */
    inline float tap (int k) const noexcept { return buffer[(writePos - static_cast<uint32_t> (k)) & mask]; }

    inline float readLinear (int k, float f) const noexcept
    {
        const float a = tap (k);
        const float b = tap (k + 1);
        return a + f * (b - a);
    }

    /** 4-point, 3rd-order Hermite (Catmull-Rom) between taps k and k+1. */
    inline float readHermite (int k, float f) const noexcept
    {
        const float xm1 = tap (k - 1);
        const float x0 = tap (k);
        const float x1 = tap (k + 1);
        const float x2 = tap (k + 2);
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }

    /** 4-point, 3rd-order Lagrange between taps k and k+1 (maximally flat at DC). */
    inline float readLagrange3 (int k, float f) const noexcept
    {
        const float xm1 = tap (k - 1);
        const float x0 = tap (k);
        const float x1 = tap (k + 1);
        const float x2 = tap (k + 2);
        const float d = f + 1.0f; // delay relative to tap k-1
        const float dm1 = d - 1.0f, dm2 = d - 2.0f, dm3 = d - 3.0f;
        const float h0 = -dm1 * dm2 * dm3 * (1.0f / 6.0f);
        const float h1 = d * dm2 * dm3 * 0.5f;
        const float h2 = -d * dm1 * dm3 * 0.5f;
        const float h3 = d * dm1 * dm2 * (1.0f / 6.0f);
        return h0 * xm1 + h1 * x0 + h2 * x1 + h3 * x2;
    }

    /** First-order Thiran allpass. `a` = (1 - d) / (1 + d), d in [0.5, 1.5).
        `state` holds the previous output. */
    inline float readThiran (int k, float a, float& state) const noexcept
    {
        const float x0 = tap (k);
        const float x1 = tap (k + 1);
        const float y = a * (x0 - state) + x1;
        state = y;
        return y;
    }

private:
    std::vector<float> buffer;
    uint32_t mask = 0;
    uint32_t writePos = 0;
};

/** Splits a total fractional delay into integer part + interpolator parameter,
    respecting each interpolator's valid range. */
struct FractionalDelaySetting
{
    int k = 1;       // integer tap
    float f = 0.0f;  // fraction (linear/hermite/lagrange)
    float a = 0.0f;  // Thiran coefficient

    static FractionalDelaySetting make (double delay, Interpolation mode) noexcept
    {
        FractionalDelaySetting s;
        if (mode == Interpolation::thiran1)
        {
            delay = std::max (delay, 1.5);
            const double base = std::floor (delay - 0.5);
            const double d = delay - base; // [0.5, 1.5)
            s.k = static_cast<int> (base);
            s.a = static_cast<float> ((1.0 - d) / (1.0 + d));
        }
        else
        {
            const double minDelay = mode == Interpolation::linear ? 1.0 : 2.0;
            delay = std::max (delay, minDelay);
            const double base = std::floor (delay);
            s.k = static_cast<int> (base);
            s.f = static_cast<float> (delay - base);
        }
        return s;
    }
};

inline float readInterpolated (const DelayLine& line, const FractionalDelaySetting& s, Interpolation mode,
                               float& thiranState) noexcept
{
    switch (mode)
    {
        case Interpolation::linear:    return line.readLinear (s.k, s.f);
        case Interpolation::hermite:   return line.readHermite (s.k, s.f);
        case Interpolation::lagrange3: return line.readLagrange3 (s.k, s.f);
        case Interpolation::thiran1:   return line.readThiran (s.k, s.a, thiranState);
    }
    return 0.0f;
}

} // namespace arc::dsp
