#pragma once

// Small, allocation-free DSP helpers shared by the whole engine.
// Everything here is safe to call on the audio thread.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace arc::dsp
{

inline constexpr double kPi = std::numbers::pi;
inline constexpr double kTwoPi = 2.0 * std::numbers::pi;
inline constexpr float kPiF = std::numbers::pi_v<float>;
inline constexpr float kTwoPiF = 2.0f * std::numbers::pi_v<float>;

template <typename T>
constexpr T clamp (T v, T lo, T hi) noexcept
{
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g) noexcept { return 20.0f * std::log10 (std::max (g, 1.0e-12f)); }

inline double midiToHz (double note) noexcept { return 440.0 * std::exp2 ((note - 69.0) / 12.0); }

/** Per-pass gain of a recirculating structure with the given period so that the
    level falls by 60 dB after t60 seconds. */
inline double gainForT60 (double periodSeconds, double t60Seconds) noexcept
{
    return std::pow (10.0, -3.0 * periodSeconds / std::max (t60Seconds, 1.0e-4));
}

/** Coefficient of a one-pole smoother reaching ~63 % of a step in `timeSeconds`
    when updated at `updateRate` Hz. */
inline float smoothingCoeff (double timeSeconds, double updateRate) noexcept
{
    if (timeSeconds <= 0.0)
        return 0.0f;
    return static_cast<float> (std::exp (-1.0 / (timeSeconds * updateRate)));
}

/** Exponential one-pole smoother. `coeff` from smoothingCoeff(). */
struct Smoothed
{
    float current = 0.0f;
    float target = 0.0f;
    float coeff = 0.0f;

    void reset (float v) noexcept { current = target = v; }
    void setTarget (float v) noexcept { target = v; }
    float next() noexcept
    {
        current = target + coeff * (current - target);
        return current;
    }
    bool settled (float eps = 1.0e-6f) const noexcept { return std::abs (current - target) < eps; }
};

/** xorshift32: tiny deterministic RNG, never allocates, repeatable from a seed. */
struct Random
{
    uint32_t state = 0x9E3779B9u;

    void seed (uint32_t s) noexcept { state = s != 0 ? s : 0x9E3779B9u; }
    uint32_t nextU32() noexcept
    {
        uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return state = x;
    }
    /** Uniform in [0, 1). */
    float uniform() noexcept { return static_cast<float> (nextU32() >> 8) * (1.0f / 16777216.0f); }
    /** Uniform in [-1, 1). */
    float bipolar() noexcept { return uniform() * 2.0f - 1.0f; }
    /** Approximately gaussian (sum of 4 uniforms), unit variance. */
    float gaussian() noexcept
    {
        const float s = uniform() + uniform() + uniform() + uniform();
        return (s - 2.0f) * 1.7320508f;
    }
};

/** Rational tanh approximation, |error| < 2e-3, monotonic, bounded to (-1, 1). */
inline float fastTanh (float x) noexcept
{
    x = clamp (x, -4.97f, 4.97f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

/** Replaces non-finite values with zero. Returns true if the value was finite. */
inline bool sanitise (float& v) noexcept
{
    if (std::isfinite (v))
        return true;
    v = 0.0f;
    return false;
}

inline uint32_t nextPowerOfTwo (uint32_t v) noexcept
{
    uint32_t p = 1;
    while (p < v)
        p <<= 1;
    return p;
}

} // namespace arc::dsp
