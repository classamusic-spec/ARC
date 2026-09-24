#pragma once

// A material is a description of how the *network* behaves — not an EQ curve:
// where the outer nodes sit relative to the CORE (modal distribution), how
// dispersive each loop is (inharmonicity), how decay depends on frequency, how
// strongly the body couples, where exciter energy enters, and how pitch reacts to
// energy. See docs/MATERIAL_SYSTEM.md.

#include <array>

#include "Engine/EngineTypes.h"

namespace arc
{

struct MaterialProfile
{
    std::array<float, 4> nodeRatio {};       // A..D, relative to CORE, at neutral TENSION
    std::array<float, 4> nodeDetuneCents {}; // small fixed offsets (doublets / beating)
    float coreDispersion = 0.0f;             // AllpassChain amount (0..0.6)
    float nodeDispersion = 0.0f;
    float t60Low = 3.0f;                     // seconds at 100 Hz
    float t60Mid = 2.0f;                     // seconds at 1 kHz
    float t60High = 0.8f;                    // seconds at 8 kHz
    float couplingScale = 1.0f;              // multiplies every edge generator
    float coreSelectivity = 0.5f;            // 0 = loop keeps its full harmonic series,
    float nodeSelectivity = 0.5f;            // 1 = loop rings as a single mode (banded)
    std::array<float, 4> inject {};          // exciter energy into A..D (CORE = 1)
    std::array<float, 4> level {};           // pickup level of A..D
    float coreLevel = 1.0f;
    float energyTuning = 0.0f;               // relative pitch rise at full node energy
    float hardnessBias = 0.0f;               // added to STRIKE hardness
    float toneBias = 0.0f;                   // added to exciter tone controls
    float outputGain = 1.0f;                 // loudness normalisation (measured)
    std::array<float, 4> exciterGainDb {};   // per exciter (STRIKE, PLUCK, BOW, AIR), measured
};

/** Factory definitions of GLASS, METAL, WOOD, MEMBRANE. */
const MaterialProfile& materialProfile (MaterialType t) noexcept;

/** Values the voices consume after morphing, inspector modifiers and TENSION. */
struct EffectiveMaterial
{
    std::array<float, 4> nodeRatio {};
    std::array<float, 4> nodeDetuneCents {};
    float coreDispersion = 0.0f, nodeDispersion = 0.0f;
    float t60Low = 3.0f, t60Mid = 2.0f, t60High = 0.8f;
    float couplingScale = 1.0f;
    float coreSelectivity = 0.5f, nodeSelectivity = 0.5f;
    std::array<float, 4> inject {};
    std::array<float, 4> level {};
    float coreLevel = 1.0f;
    float energyTuning = 0.0f;
    float hardnessBias = 0.0f;
    float toneBias = 0.0f;
    float outputGain = 1.0f;
    std::array<float, 4> exciterGainDb {};

    /** Decay time at frequency f (log-log through 100 Hz / 1 kHz / 8 kHz). */
    float t60At (float f) const noexcept;
};

} // namespace arc
