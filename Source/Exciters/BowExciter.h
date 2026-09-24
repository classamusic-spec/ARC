#pragma once

// BOW — sustained friction excitation (feedback, not noise).
//
// The bow moves at velocity vb; the resonator surface at the contact moves at vs
// (the CORE loop signal). Friction force depends on the relative velocity
// dv = vb - vs through a stick-slip characteristic (Smith/Cook bow table):
//
//      F(dv) = dv * r(dv),    r = clamp((|dv| * slope / vb + c)^-4, 0.01, 0.98)
//
// Small dv -> the surface sticks to the bow (large r); large dv -> it slips. The
// force is injected back into the network, so the network's own period decides the
// pitch: Helmholtz-like self-oscillation on string-like materials, singing-bowl
// behaviour on dispersive ones.
//
// The stick region is a *fraction of the bow velocity* (set by PRESSURE: 20 % light,
// flautando .. 65 % pressed), so the bow can always catch and release the surface:
// every PRESSURE x SPEED combination speaks, SPEED sets the amplitude (as with a real
// bow, amplitude ~ bow velocity) and PRESSURE the tone. (With an absolute stick width
// only a diagonal band pressure <= speed spoke; the rest was silence — see
// docs/EXCITERS.md, "Playability maps".) FRICTION sets the knee and rosin noise.
// |F| is bounded for all dv, so the network sees a bounded input.

#include "Synthesis/DspCore.h"
#include "Synthesis/LoopFilters.h"

namespace arc::dsp
{

class BowExciter
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        dc.setCutoff (10.0, sampleRate);
        vbSmooth.coeff = smoothingCoeff (0.004, sampleRate);
        reset();
    }

    void reset() noexcept
    {
        env = 0.0f;
        gate = false;
        active = false;
        vbSmooth.reset (0.0f);
        dc.reset();
        noiseLp = 0.0f;
    }

    void start (float energy, float speed, float pressure, float friction, uint32_t seed) noexcept
    {
        rng.seed (seed);
        setTargets (energy, speed, pressure, friction);
        // Attack 25..140 ms: a stronger bow stroke speaks faster.
        attackCoeff = static_cast<float> (1.0 - std::exp (-1.0 / ((0.14 - 0.115 * static_cast<double> (clamp (energy, 0.0f, 1.0f))) * sr)));
        gate = true;
        active = true;
    }

    /** Control-rate targets (MPE pressure / macro / inspector changes). */
    void setTargets (float energy, float speed, float pressure, float friction) noexcept
    {
        maxVelocity = (0.03f + 0.32f * clamp (speed, 0.0f, 1.0f)) * (0.35f + 0.65f * clamp (energy, 0.0f, 1.5f));
        knee = 0.85f - 0.25f * clamp (friction, 0.0f, 1.0f);
        const float stickFraction = 0.2f + 0.45f * clamp (pressure, 0.0f, 1.0f);
        slope = (1.0f - knee) / stickFraction; // |dv| < stickFraction * vb sticks
        rosin = 0.02f + 0.1f * clamp (friction, 0.0f, 1.0f);
    }

    void release() noexcept
    {
        gate = false;
        releaseCoeff = static_cast<float> (1.0 - std::exp (-1.0 / (0.06 * sr)));
    }

    /** vs: surface velocity at the contact (CORE loop output). Returns the force. */
    inline float tick (float vs) noexcept
    {
        if (! active)
            return 0.0f;
        if (gate)
            env += attackCoeff * (1.0f - env);
        else
        {
            env -= releaseCoeff * env;
            if (env < 1.0e-4f)
            {
                active = false;
                return 0.0f;
            }
        }
        // Rosin noise: band-limited jitter of the bow velocity.
        noiseLp += 0.15f * (rng.bipolar() - noiseLp);
        vbSmooth.setTarget (maxVelocity);
        const float vTarget = vbSmooth.next();
        const float vb = env * vTarget * (1.0f + rosin * noiseLp);

        const float dv = vb - vs;
        const float a = std::abs (dv) * slope / std::max (vTarget, 0.004f) + knee;
        const float a2 = a * a;
        const float r = clamp (1.0f / (a2 * a2), 0.01f, 0.98f);
        // Contact force follows the envelope: the bow lands on attack and is lifted on
        // release. (A bow that only slows down while staying in contact acts as a strong
        // damper — measured -24 dB in 50 ms on GLASS before this was fixed.)
        return dc.process (env * dv * r);
    }

    bool isActive() const noexcept { return active; }
    bool isGated() const noexcept { return gate; }
    float envelope() const noexcept { return env; }

private:
    DcBlocker dc;
    Random rng;
    Smoothed vbSmooth;
    double sr = 48000.0;
    float env = 0.0f, attackCoeff = 0.001f, releaseCoeff = 0.001f;
    float maxVelocity = 0.1f, slope = 3.0f, knee = 0.75f, rosin = 0.05f, noiseLp = 0.0f;
    bool gate = false, active = false;
};

} // namespace arc::dsp
