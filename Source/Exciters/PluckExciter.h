#pragma once

// PLUCK — displacement-style excitation.
//
// A finger/plectrum drags the string (slow ramp, 0.6..6 ms) and releases it
// (sudden drop). At the release a short Karplus-Strong-style noise burst adds the
// grain of the contact, so repeated notes are never identical. The result passes a
// pluck-position comb (x[n] - x[n - P], P = position * loop length), which removes
// the harmonics that have a node at the pluck point — exactly what a real string
// does. TONE = release sharpness (low-pass). DAMP is applied by the voice as extra
// loop damping (palm mute).

#include <vector>

#include "Synthesis/DspCore.h"
#include "Synthesis/LoopFilters.h"

namespace arc::dsp
{

class PluckExciter
{
public:
    void prepare (double sampleRate, int maxLoopSamples)
    {
        sr = sampleRate;
        const auto size = nextPowerOfTwo (static_cast<uint32_t> (maxLoopSamples + 4));
        comb.assign (size, 0.0f);
        mask = size - 1;
        dc.setCutoff (8.0, sampleRate);
        reset();
    }

    void reset() noexcept
    {
        std::fill (comb.begin(), comb.end(), 0.0f);
        write = 0;
        pos = 0;
        active = false;
        lp = 0.0f;
        dc.reset();
    }

    /** loopSamples: CORE loop period in samples (for the position comb and burst). */
    void trigger (float energy, float position, float tone, float loopSamples, uint32_t seed) noexcept
    {
        energy = clamp (energy, 0.0f, 1.5f);
        const float t = clamp (tone + 0.2f * (energy - 0.5f), 0.0f, 1.0f);
        rampLength = std::max (2, static_cast<int> ((0.006f - 0.0054f * t) * static_cast<float> (sr)));
        burstLength = std::clamp (static_cast<int> (loopSamples), 8, static_cast<int> (0.02 * sr));
        // Fractional comb delay so the notches land exactly on the loop's harmonics
        // (an integer delay misplaces the k-th notch by k times the rounding error).
        const float p = clamp (clamp (position, 0.02f, 0.5f) * loopSamples, 1.0f, static_cast<float> (mask) - 2.0f);
        combDelay = static_cast<int> (p);
        combFrac = p - static_cast<float> (combDelay);
        amplitude = energy * 1.6f;
        burstLevel = 0.25f + 0.5f * t;
        const float cutoff = 700.0f * std::pow (2.0f, 4.8f * t);
        lpCoeff = std::exp (-kTwoPiF * std::min (cutoff, 0.45f * static_cast<float> (sr)) / static_cast<float> (sr));
        rng.seed (seed);
        // Clear the comb history this pluck will read (bounded by one loop period).
        for (int i = 1; i <= combDelay + 2; ++i)
            comb[(write - static_cast<uint32_t> (i)) & mask] = 0.0f;
        pos = 0;
        active = true;
    }

    inline float tick() noexcept
    {
        if (! active)
            return 0.0f;
        float x = 0.0f;
        if (pos < rampLength)
            x = amplitude * static_cast<float> (pos) / static_cast<float> (rampLength) * 0.35f;
        else if (pos < rampLength + burstLength)
        {
            // Release: the displacement drops to zero; the burst carries the grain.
            const int b = pos - rampLength;
            const float env = 1.0f - static_cast<float> (b) / static_cast<float> (burstLength);
            x = amplitude * burstLevel * env * rng.bipolar();
        }
        lp = x + lpCoeff * (lp - x);
        const float d = dc.process (lp);
        comb[write] = d;
        const float c0 = comb[(write - static_cast<uint32_t> (combDelay)) & mask];
        const float c1 = comb[(write - static_cast<uint32_t> (combDelay + 1)) & mask];
        const float y = d - (c0 + combFrac * (c1 - c0));
        write = (write + 1) & mask;
        ++pos;
        if (pos > rampLength + burstLength + combDelay + static_cast<int> (0.03 * sr))
            active = false;
        return y;
    }

    bool isActive() const noexcept { return active; }

private:
    std::vector<float> comb;
    uint32_t mask = 0, write = 0;
    DcBlocker dc;
    Random rng;
    double sr = 48000.0;
    int pos = 0, rampLength = 0, burstLength = 0, combDelay = 1;
    float combFrac = 0.0f;
    float amplitude = 0.0f, burstLevel = 0.0f, lp = 0.0f, lpCoeff = 0.0f;
    bool active = false;
};

} // namespace arc::dsp
