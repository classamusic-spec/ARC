#pragma once

// Plain parameter / enum types shared by the engine. No JUCE, no allocation.

#include <array>
#include <cstdint>

namespace arc
{

enum class ExciterType : int
{
    strike = 0,
    pluck,
    bow,
    air,
    count
};

enum class MaterialType : int
{
    glass = 0,
    metal,
    wood,
    membrane,
    count
};

enum class VoiceMode : int
{
    poly = 0,
    mono,
    legato,
    count
};

enum class Quality : int
{
    eco = 0,
    normal,
    high,
    count
};

/** Type-specific exciter controls (0..1 each), shown in the Exciter Inspector. */
struct ExciterParams
{
    // STRIKE
    float strikeHardness = 0.55f;
    float strikeLength = 0.35f;
    float strikeTone = 0.6f;
    // PLUCK
    float pluckPosition = 0.22f;
    float pluckDamp = 0.0f;
    float pluckTone = 0.6f;
    // BOW
    float bowPressure = 0.5f;
    float bowSpeed = 0.55f;
    float bowFriction = 0.5f;
    // AIR
    float airFlow = 0.6f;
    float airTurbulence = 0.45f;
    float airTone = 0.5f;
};

/** Material Inspector modifiers; 0.5 = the material's own behaviour. */
struct MaterialModifiers
{
    float mass = 0.5f;
    float brightness = 0.5f;
    float loss = 0.5f;
    float inharmonicity = 0.5f;
};

/** Per-node user controls (A..D). */
struct NodeParams
{
    float radius = 0.5f;  // 0..1, 0.5 = neutral ring (material tuning)
    float angle = 0.0f;   // radians, 0 = up, clockwise positive
    float decay = 0.5f;   // T60 multiplier, 0.5 = neutral
    float damp = 0.5f;    // HF damping, 0.5 = neutral
    float level = 0.75f;  // pickup level
    float link = 0.75f;   // coupling weight
};

} // namespace arc
