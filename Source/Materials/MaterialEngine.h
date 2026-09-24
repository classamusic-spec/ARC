#pragma once

// Morphs between material profiles and applies the Material Inspector modifiers
// and TENSION. Ratios and decay times morph in the log domain, everything else
// linearly, along a smoothstep in time (zero initial glide velocity, peak 1.5x the
// average), so a material change is a continuous glide of the network's
// coefficients (no crossfade, no discontinuity, no extra CPU).

#include "Materials/MaterialProfile.h"

namespace arc
{

class MaterialEngine
{
public:
    /** updatesPerSecond: how often update() is called (control rate). */
    void prepare (double updatesPerSecond, double morphSeconds = 0.3);

    void setMaterial (MaterialType t, bool immediate) noexcept;
    MaterialType getMaterial() const noexcept { return target; }

    /** Advances the morph by one control step and recomputes the effective material. */
    void update (const MaterialModifiers& mods, float tension) noexcept;

    const EffectiveMaterial& effective() const noexcept { return eff; }

    /** 0 while morphing has just begun, 1 when settled. */
    float morphSettled() const noexcept { return settled; }

    /** The TENSION stretch exponent applied to node ratios. */
    static float tensionExponent (float tension, float inharmonicity) noexcept
    {
        return 1.0f + 0.9f * (tension - 0.5f) + 0.25f * (inharmonicity - 0.5f);
    }

private:
    MaterialProfile from, current;
    MaterialType target = MaterialType::metal;
    EffectiveMaterial eff;
    float progress = 1.0f; // 0..1 along the morph
    float step = 0.01f;    // progress per update
    float settled = 1.0f;
};

} // namespace arc
