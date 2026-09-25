#pragma once

// Complete plain-data snapshot of every ARC parameter, filled by the plugin once per
// block from the host-automatable parameters (or directly by tests). No JUCE.

#include <array>
#include <cstdint>

#include "Engine/EngineTypes.h"
#include "Synthesis/CouplingMatrix.h"

namespace arc
{

enum class SyncDivision : int
{
    quarter = 0, // 1/4 note
    half,        // 1/2
    bar1,        // 1 bar
    bar2,
    bar4,
    bar8,
    count
};

struct EngineParams
{
    // Performance macros
    float excite = 0.6f;
    float coupling = 0.35f;
    float tension = 0.5f;
    float chaos = 0.1f;

    // Network
    dsp::Topology topology = dsp::Topology::ring;
    bool quantise = false;
    std::array<NodeParams, 4> nodes {};

    // Exciter / material
    ExciterType exciter = ExciterType::strike;
    ExciterParams exciterParams {};
    MaterialType material = MaterialType::metal;
    MaterialModifiers materialMods {};

    // Utility
    bool freeze = false;
    bool sync = false;

    // Motion
    float motionDepth = 0.0f;  // autonomous drift amount 0..1
    float motionRateHz = 0.25f;
    SyncDivision motionDivision = SyncDivision::bar1;
    bool gesturePlay = true;

    // Voices / MIDI
    int polyphony = 8;
    VoiceMode voiceMode = VoiceMode::poly;
    float bendRange = 2.0f;       // semitones
    float releaseDamping = 0.25f; // 0 = ring freely after note-off
    float glideSeconds = 0.06f;   // legato portamento
    bool mpe = false;

    Quality quality = Quality::normal;

    // Output
    float masterGainDb = -3.0f;
    float width = 1.0f;  // 0 mono .. 1 natural .. 1.5 wide
    float space = 0.12f; // small ambience amount
    float drive = 0.0f;  // soft output saturation
    float patchLevelDb = 0.0f;
    uint32_t patchEpoch = 0; // advances on every preset load (voices keep the trim of their patch) // per-preset loudness trim, added to the master gain

    EngineParams()
    {
        const float angles[4] = { 0.375f, 0.625f, 0.125f, 0.875f }; // normalised 0..1 = -pi..pi
        for (int i = 0; i < 4; ++i)
            nodes[static_cast<size_t> (i)].angle = angles[i];
    }
};

/** Normalised angle (0..1) -> radians (-pi..pi, 0 = up). */
inline float normToAngle (float a) noexcept { return (a - 0.5f) * 6.283185307f; }

/** Transport information for tempo-synced motion. */
struct TransportInfo
{
    double bpm = 120.0;
    double ppqPosition = 0.0;
    bool playing = false;
    bool valid = false;
    double beatsPerBar = 4.0;
};

} // namespace arc
