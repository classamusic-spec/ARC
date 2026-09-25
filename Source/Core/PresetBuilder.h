#pragma once

// Fluent factory-preset builder shared by the library files (Source/Core/Presets/*.cpp).
// Values are plain (denormalised) parameter values. Anything a preset does not set takes
// the parameter's default, so every preset starts from the same neutral instrument.
//
// Order matters for tuning: set the material and TENSION (macros) before tune() /
// tuneNear() / chord(), because the node radius that lands on a ratio depends on both.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "Core/FactoryPresets.h"
#include "Core/Parameters.h"
#include "Engine/NetworkGeometry.h"
#include "Motion/Gesture.h"

namespace arc::presets::detail
{

using dsp::Topology;

enum Division
{
    quarter = 0,
    half,
    bar1,
    bar2,
    bar4,
    bar8
};

inline constexpr auto glass = MaterialType::glass;
inline constexpr auto metal = MaterialType::metal;
inline constexpr auto wood = MaterialType::wood;
inline constexpr auto membrane = MaterialType::membrane;

inline constexpr auto star = Topology::star;
inline constexpr auto ring = Topology::ring;
inline constexpr auto web = Topology::web;
inline constexpr auto chain = Topology::chain;

/** One semitone of node radius (radius spans +-1 octave over 0..1). */
inline constexpr float kSemitone = 1.0f / 24.0f;

/** Stable per-name seed (FNV-1a): CHAOS and MOTION walks recall identically. */
inline uint32_t seedFor (const char* name) noexcept
{
    uint32_t h = 2166136261u;
    for (const char* c = name; *c != 0; ++c)
        h = (h ^ static_cast<uint8_t> (*c)) * 16777619u;
    return h == 0 ? 1u : h;
}

class Builder
{
public:
    Builder (const char* name, const char* category, const char* tags, const char* description, uint32_t seed)
    {
        p.name = name;
        p.category = category;
        p.tags = tags;
        p.description = description;
        p.seed = seed;
    }

    Builder (const char* name, const char* category, const char* tags, const char* description)
        : Builder (name, category, tags, description, seedFor (name))
    {
    }

    Builder& set (const std::string& id, float v)
    {
        for (auto& kv : p.values)
            if (kv.first == id)
            {
                kv.second = v;
                return *this;
            }
        p.values.emplace_back (id, v);
        return *this;
    }

    // --- exciters -------------------------------------------------------------------
    Builder& strike (float hardness, float length, float tone)
    {
        return exciter (ExciterType::strike).set (params::strikeHardness, hardness).set (params::strikeLength, length).set (params::strikeTone, tone);
    }
    Builder& pluck (float position, float damp, float tone)
    {
        return exciter (ExciterType::pluck).set (params::pluckPosition, position).set (params::pluckDamp, damp).set (params::pluckTone, tone);
    }
    Builder& bow (float pressure, float speed, float friction)
    {
        return exciter (ExciterType::bow).set (params::bowPressure, pressure).set (params::bowSpeed, speed).set (params::bowFriction, friction);
    }
    Builder& air (float flow, float turbulence, float tone)
    {
        return exciter (ExciterType::air).set (params::airFlow, flow).set (params::airTurbulence, turbulence).set (params::airTone, tone);
    }

    // --- body --------------------------------------------------------------------------
    Builder& material (MaterialType m, float mass, float brightness, float loss, float inharmonicity = 0.5f)
    {
        mat = m;
        inharm = inharmonicity;
        return set (params::materialType, static_cast<float> (m))
            .set (params::mass, mass)
            .set (params::brightness, brightness)
            .set (params::loss, loss)
            .set (params::inharmonicity, inharmonicity);
    }

    Builder& macros (float excite, float coupling, float tension, float chaos)
    {
        tens = tension;
        return set (params::excite, excite).set (params::coupling, coupling).set (params::tension, tension).set (params::chaos, chaos);
    }

    Builder& topology (Topology t) { return set (params::topology, static_cast<float> (t)); }
    Builder& quantise() { return set (params::quantise, 1.0f); }

    /** Node field for A..D ("radius", "angle", "decay", "damp", "level", "link"). */
    Builder& nodes (const char* field, float a, float b, float c, float d)
    {
        const float v[4] = { a, b, c, d };
        for (int n = 0; n < 4; ++n)
            set (params::nodeId (n, field).toStdString(), v[n]);
        return *this;
    }
    Builder& decays (float a, float b, float c, float d) { return nodes ("decay", a, b, c, d); }
    Builder& damps (float a, float b, float c, float d) { return nodes ("damp", a, b, c, d); }
    Builder& levels (float a, float b, float c, float d) { return nodes ("level", a, b, c, d); }
    Builder& links (float a, float b, float c, float d) { return nodes ("link", a, b, c, d); }
    Builder& angles (float a, float b, float c, float d) { return nodes ("angle", a, b, c, d); }

    /** Places node n exactly on `ratio` x CORE for the material / TENSION / INHARMONICITY
        set so far (radius is the only per-node tuning control). */
    Builder& tune (int n, float ratio)
    {
        const float radius = radiusForRatio (mat, n, ratio, tens, inharm);
        return set (params::nodeId (n, "radius").toStdString(), std::clamp (radius, 0.0f, 1.0f));
    }

    /** As tune(), but folds the ratio by whole octaves into the node's reach first (the same
        pitch class; the way sympathetic strings of a chord would be tuned). */
    Builder& tuneNear (int n, float ratio)
    {
        float r = ratio;
        int folds = 0;
        for (; folds < 8 && radiusForRatio (mat, n, r, tens, inharm) < 0.02f; ++folds)
            r *= 2.0f;
        for (int down = 0; down < 8 && radiusForRatio (mat, n, r, tens, inharm) > 0.98f; ++down, ++folds)
            r *= 0.5f;
        if (folds > 0)
        {
            ++p.foldedTunings;
            char buf[48];
            std::snprintf (buf, sizeof (buf), "%s%c %.3g -> %.3g", p.tuningNotes.empty() ? "" : "; ", "ABCD"[n & 3], static_cast<double> (ratio),
                           static_cast<double> (r));
            p.tuningNotes += buf;
        }
        return tune (n, r);
    }

    /** Tunes all four nodes (A..D) to a chord / spectrum, each folded into reach. */
    Builder& chord (float a, float b, float c, float d) { return tuneNear (0, a).tuneNear (1, b).tuneNear (2, c).tuneNear (3, d); }

    // --- motion ------------------------------------------------------------------------
    Builder& motion (float depth, float rateHz) { return set (params::motionDepth, depth).set (params::motionRate, rateHz); }
    Builder& synced (Division division, float depth)
    {
        return set (params::sync, 1.0f).set (params::motionDivision, static_cast<float> (division)).set (params::motionDepth, depth);
    }
    Builder& gesture (int n, const Gesture& g)
    {
        p.gestures[static_cast<size_t> (n)] = g.serialise();
        return *this;
    }

    // --- voice / output ----------------------------------------------------------------
    Builder& voice (VoiceMode mode, float glideSeconds)
    {
        return set (params::voiceMode, static_cast<float> (mode)).set (params::glide, glideSeconds);
    }
    Builder& release (float damping) { return set (params::releaseDamping, damping); }
    Builder& fx (float width, float space, float drive = 0.0f)
    {
        return set (params::width, width).set (params::space, space).set (params::drive, drive);
    }
    Builder& level (float dB) { return set (params::patchLevel, dB); }

    PresetDefinition build() const { return p; }

private:
    Builder& exciter (ExciterType t) { return set (params::exciterType, static_cast<float> (t)); }

    PresetDefinition p;
    MaterialType mat = MaterialType::metal;
    float tens = 0.5f, inharm = 0.5f;
};

using Library = std::vector<PresetDefinition>;

// Musical interval shorthands (just intonation) for chord() / tune().
inline constexpr float kUnison = 1.0f, kMinor2 = 16.0f / 15.0f, kMajor2 = 9.0f / 8.0f, kMinor3 = 6.0f / 5.0f,
                       kMajor3 = 5.0f / 4.0f, kFourth = 4.0f / 3.0f, kTritone = 45.0f / 32.0f, kFifth = 3.0f / 2.0f,
                       kMinor6 = 8.0f / 5.0f, kMajor6 = 5.0f / 3.0f, kMinor7 = 9.0f / 5.0f, kHarm7 = 7.0f / 4.0f,
                       kMajor7 = 15.0f / 8.0f, kOctave = 2.0f;

/** Equal-tempered ratio of n semitones. */
inline float semis (float n) { return std::exp2 (n / 12.0f); }

// Library sections (one file per category, see FactoryPresets.cpp for the order).
void addPads (Library&);
void addPlucked (Library&);
void addStruck (Library&);
void addBowed (Library&);
void addAir (Library&);
void addGlass (Library&);
void addMetal (Library&);
void addWood (Library&);
void addMembrane (Library&);
void addDrones (Library&);
void addPercussion (Library&);
void addExperimental (Library&);

} // namespace arc::presets::detail
