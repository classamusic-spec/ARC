#pragma once

// Control state computed once per control block by the engine and shared by all
// voices (the network geometry, material and coupling are global; pitch, energy
// and per-note expression are per voice).

#include <array>

#include "Engine/EngineTypes.h"
#include "Materials/MaterialProfile.h"
#include "Synthesis/CouplingMatrix.h"

namespace arc
{

struct VoiceControl
{
    EffectiveMaterial material;

    // Outer nodes A..D (after motion / gesture playback).
    std::array<float, 4> nodeOffsetOctaves {}; // from radius (quantised when enabled)
    std::array<float, 4> nodePan {};           // -1..1 from angle
    std::array<float, 4> nodeDecayMult { 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 4> nodeDampMult { 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<float, 4> nodeLevel { 0.75f, 0.75f, 0.75f, 0.75f };

    std::array<float, dsp::kNumEdges> edgeTheta {}; // coupling generators (post macro/topology/link/chaos)

    ExciterType exciterType = ExciterType::strike;
    ExciterParams exciter;
    float excite = 0.6f;

    float bendSemitones = 0.0f;                      // channel pitch bend (non-MPE)
    std::array<float, 5> chaosDetuneCents {};        // CORE, A..D slow bounded random walks
    float chaos = 0.0f;
    float freeze = 0.0f;                             // 0..1 (smoothed)
    int freezeEpoch = 0;                             // voices started before the epoch freeze
    float releaseDamping = 0.25f;                    // 0 = ring freely after note-off, 1 = damp fast
    float patchGain = 1.0f;                          // PATCH LEVEL, linear (smoothed)
    uint32_t patchEpoch = 0;                         // advances on every preset load
    int dispersionStages = 4;
    bool couplingCompensation = true;
};

} // namespace arc
