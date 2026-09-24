#pragma once

// Smoothed, bounded random walk: new random targets in [-1, 1] at `rate` per second,
// followed by a critically damped two-pole glide. Output stays in [-1, 1], is C1-smooth
// and fully determined by its seed (preset recall reproduces the same trajectory).

#include "Synthesis/DspCore.h"

namespace arc
{

class RandomWalk
{
public:
    void seed (uint32_t s) noexcept
    {
        rng.seed (s);
        target = rng.bipolar();
        a = b = target;
        timer = 0.0;
    }

    /** dt: seconds since the last call. rate: target changes per second. */
    float next (double dt, double rate) noexcept
    {
        timer += dt * rate;
        if (timer >= 1.0)
        {
            timer -= std::floor (timer);
            target = rng.bipolar();
        }
        // Critically damped glide with time constant ~ 1 / (2 pi rate).
        const float k = static_cast<float> (1.0 - std::exp (-dt * rate * 2.4));
        a += k * (target - a);
        b += k * (a - b);
        return dsp::clamp (b, -1.0f, 1.0f);
    }

    float value() const noexcept { return b; }

private:
    dsp::Random rng;
    float target = 0.0f, a = 0.0f, b = 0.0f;
    double timer = 0.0;
};

} // namespace arc
