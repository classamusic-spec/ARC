#pragma once

// AIR — continuous turbulent excitation.
//
// Two sources, both scaled by breath pressure (FLOW, velocity, EXCITE, MPE pressure):
//  * turbulence: gaussian noise shaped by TONE (low-pass) and TURBULENCE — breathy,
//    band-limited energy that the network filters into its own resonances;
//  * air-column drive: a saturating negative resistance *in phase* with the CORE loop
//    (inj = G * s * tanh(y / s)). It adds loop gain at every CORE mode without any
//    delay, so a strong breath sustains a whistle exactly at the network's own pitch
//    (an earlier jet-delay model pulled the pitch by up to 23 cents — see
//    DEVELOPMENT_LOG). Saturation bounds the drive: |inj| <= G * s.
// The ExciterEngine additionally regulates the drive against the CORE amplitude.

#include "Synthesis/DspCore.h"
#include "Synthesis/LoopFilters.h"

namespace arc::dsp
{

class AirExciter
{
public:
    void prepare (double sampleRate, int /*maxLoopSamples*/)
    {
        sr = sampleRate;
        dc.setCutoff (15.0, sampleRate);
        noiseHp.setCutoff (40.0, sampleRate);
        reset();
    }

    void reset() noexcept
    {
        env = 0.0f;
        gate = false;
        active = false;
        noiseLp = 0.0f;
        lastTurbulence = 0.0f;
        dc.reset();
        noiseHp.reset();
    }

    void start (float energy, float flow, float turbulence, float tone, float /*loopSamples*/, uint32_t seed) noexcept
    {
        rng.seed (seed);
        setTargets (energy, flow, turbulence, tone, 0.0f);
        attackCoeff = static_cast<float> (1.0 - std::exp (-1.0 / ((0.2 - 0.14 * static_cast<double> (clamp (energy, 0.0f, 1.0f))) * sr)));
        gate = true;
        active = true;
    }

    void setTargets (float energy, float flow, float turbulence, float tone, float /*loopSamples*/) noexcept
    {
        const float e = clamp (energy, 0.0f, 1.5f);
        const float f = clamp (flow, 0.0f, 1.0f);
        pressure = (0.15f + 0.85f * f) * (0.3f + 0.7f * e);
        noiseAmount = 0.08f + 0.9f * clamp (turbulence, 0.0f, 1.0f);
        // Tonal drive relative to the CORE's per-pass loss: 1.0 is the self-oscillation
        // threshold on every material. FLOW moves from breath (below) to tone (above).
        tonalFactor = (0.6f + 3.0f * f) * (1.0f - 0.5f * clamp (turbulence, 0.0f, 1.0f));
        const float cutoff = 400.0f * std::pow (2.0f, 5.5f * clamp (tone, 0.0f, 1.0f));
        noiseCoeff = std::exp (-kTwoPiF * std::min (cutoff, 0.45f * static_cast<float> (sr)) / static_cast<float> (sr));
    }

    void release() noexcept
    {
        gate = false;
        releaseCoeff = static_cast<float> (1.0 - std::exp (-1.0 / (0.08 * sr)));
    }

    /** Per-pass loss of the CORE loop at its fundamental (1 - |G|) and its frequency. */
    void setLoopLoss (float loss, float frequency) noexcept
    {
        loopLoss = clamp (loss, 1.0e-5f, 0.5f);
        // Above threshold, build the tone at a pitch-independent rate (~6 e-folds per
        // second): otherwise high-Q low notes would take seconds to speak.
        speakPerPass = 6.0f / std::max (frequency, 10.0f);
    }

    /** loopSignal: CORE loop output at the mouth. Returns the injected pressure. */
    inline float tick (float loopSignal) noexcept
    {
        if (! active)
        {
            lastTurbulence = 0.0f;
            return 0.0f;
        }
        if (gate)
            env += attackCoeff * (1.0f - env);
        else
        {
            env -= releaseCoeff * env;
            if (env < 1.0e-4f)
            {
                active = false;
                lastTurbulence = 0.0f;
                return 0.0f;
            }
        }
        const float breath = env * pressure;
        const float white = rng.gaussian();
        noiseLp = white + noiseCoeff * (noiseLp - white);
        const float turbulence = noiseHp.process (noiseLp) * noiseAmount * breath;

        constexpr float sat = 0.3f;
        const float breathNorm = breath / std::max (pressure, 1.0e-3f);
        const float gainPerPass = breathNorm * (loopLoss * tonalFactor + std::max (0.0f, tonalFactor - 1.0f) * speakPerPass);
        const float column = gainPerPass * sat * fastTanh (loopSignal / sat);
        lastTurbulence = 0.25f * turbulence; // already DC-free (noiseHp)
        return dc.process (column) + lastTurbulence;
    }

    /** Feed-forward part of the last tick (turbulence only): safe to spread to nodes. */
    float feedForward() const noexcept { return lastTurbulence; }

    bool isActive() const noexcept { return active; }
    bool isGated() const noexcept { return gate; }
    float envelope() const noexcept { return env; }

private:
    DcBlocker dc, noiseHp;
    Random rng;
    double sr = 48000.0;
    float env = 0.0f, attackCoeff = 0.001f, releaseCoeff = 0.001f;
    float pressure = 0.5f, noiseAmount = 0.3f, tonalFactor = 1.0f, loopLoss = 0.01f, speakPerPass = 0.01f, noiseCoeff = 0.0f, noiseLp = 0.0f;
    float lastTurbulence = 0.0f;
    bool gate = false, active = false;
};

} // namespace arc::dsp
