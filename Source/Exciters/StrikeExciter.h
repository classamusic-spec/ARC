#pragma once

// STRIKE — transient physical impact.
//
// A mallet transfers a fixed momentum over a contact time Tc. The force pulse is
// F(t) = A sin^p(pi t / Tc) with A normalised so the pulse area (momentum) depends
// only on the strike energy, not on Tc: short/hard contacts are brighter, not louder
// at low frequencies. Hardness sharpens the pulse (exponent p) and adds contact
// noise; velocity shortens the contact and hardens it (felt stiffening). TONE is a
// spectral tilt. The injection is DC-blocked so no DC mode is excited in the loops.

#include "Synthesis/DspCore.h"
#include "Synthesis/LoopFilters.h"

namespace arc::dsp
{

class StrikeExciter
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        dc.setCutoff (8.0, sampleRate);
        reset();
    }

    void reset() noexcept
    {
        pos = 0;
        contact = 0;
        active = false;
        lp = 0.0f;
        dc.reset();
    }

    /** energy: 0..1 overall strike strength (velocity x EXCITE). */
    void trigger (float energy, float hardness, float length, float tone, uint32_t seed) noexcept
    {
        hardness = clamp (hardness, 0.0f, 1.0f);
        // Contact time: 0.25 ms (hard, short) .. 9 ms (soft, long); harder hits shorten it.
        const float lengthSeconds = 0.00025f * std::pow (36.0f, clamp (length, 0.0f, 1.0f));
        const float stiffening = 1.0f - 0.45f * hardness * energy;
        contact = std::max (2, static_cast<int> (lengthSeconds * stiffening * static_cast<float> (sr)));
        shapeExponent = 1.0f + 5.0f * hardness;

        // Normalise momentum: integral of sin^p over the contact ~ contact * c(p).
        const float shapeArea = 0.5f + 0.5f / (1.0f + 0.45f * shapeExponent); // ~ mean of sin^p
        amplitude = energy / (static_cast<float> (contact) * shapeArea) * 24.0f;

        noiseLevel = 0.08f + 0.5f * hardness * hardness;
        const float cutoff = 350.0f * std::pow (2.0f, 6.0f * clamp (tone + 0.25f * energy * hardness, 0.0f, 1.2f));
        lpCoeff = std::exp (-kTwoPiF * std::min (cutoff, 0.45f * static_cast<float> (sr)) / static_cast<float> (sr));
        rng.seed (seed);
        pos = 0;
        active = true;
    }

    inline float tick() noexcept
    {
        if (! active)
            return 0.0f;
        float x = 0.0f;
        if (pos < contact)
        {
            const float t = (static_cast<float> (pos) + 0.5f) / static_cast<float> (contact);
            const float s = std::sin (kPiF * t);
            const float shape = std::pow (s, shapeExponent);
            x = amplitude * shape * (1.0f + noiseLevel * rng.bipolar());
        }
        lp = x + lpCoeff * (lp - x);
        const float y = dc.process (lp);
        ++pos;
        if (pos > contact + static_cast<int> (0.05 * sr))
            active = false;
        return y;
    }

    bool isActive() const noexcept { return active; }

private:
    DcBlocker dc;
    Random rng;
    double sr = 48000.0;
    int pos = 0, contact = 0;
    float amplitude = 0.0f, shapeExponent = 1.0f, noiseLevel = 0.0f;
    float lp = 0.0f, lpCoeff = 0.0f;
    bool active = false;
};

} // namespace arc::dsp
