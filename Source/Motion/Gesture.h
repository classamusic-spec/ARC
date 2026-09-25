#pragma once

// A recorded node gesture: the path a node was dragged along, stored as offsets from the
// position where recording started, resampled to a fixed number of points over a
// normalised cycle. Sample-rate independent, tempo aware, bounded, serialisable.

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace arc
{

struct Gesture
{
    static constexpr int kPoints = 128;

    std::array<float, kPoints> dRadius {}; // radius offsets (normalised radius units)
    std::array<float, kPoints> dAngle {};  // angle offsets (radians, wrapped to -pi..pi)
    float durationSeconds = 0.0f;          // free-running loop length
    float durationBeats = 0.0f;            // tempo-synced loop length (0 = not synced)
    bool valid = false;

    /** Offset at normalised phase p (0..1), linear interpolation, loops. */
    void sample (double phase, float& dr, float& da) const noexcept;

    std::string serialise() const;
    static Gesture deserialise (const std::string& text);
};

/** Raw pointer-drag samples captured by the UI while recording. */
struct GestureSample
{
    double timeSeconds;
    float radius; // absolute, normalised 0..1
    float angle;  // absolute, radians
};

/** Builds a Gesture from raw samples: unwraps the angle, resamples uniformly, smooths,
    closes the loop (cross-fades the end into the start) and bounds the offsets.
    When beatsPerSecond > 0 the loop length is snapped to the nearest musical division
    (1/4 note .. 8 bars) and stored in beats. */
Gesture buildGesture (const std::vector<GestureSample>& raw, double beatsPerSecond, double beatsPerBar);

/** Procedural gestures (factory presets, tests). durationBeats 0 = free-running only.
    orbit: `turns` full revolutions around the core per cycle (negative = anticlockwise)
           with a radius wobble of +-radiusWobble twice per turn.
    sway:  a pendulum of +-angleSwing radians with +-radiusSwing, once per cycle. */
Gesture orbitGesture (float seconds, float beats, float turns, float radiusWobble);
Gesture swayGesture (float seconds, float beats, float angleSwing, float radiusSwing);

/** figure: a Lissajous path, `a` angle and `b` radius oscillations per cycle (a = 1, b = 2
    is a figure eight). */
Gesture figureGesture (float seconds, float beats, float angleSwing, float radiusSwing, int a, int b);
/** breathe: the node moves in and out only (its tuning swells and returns), `cycles` times
    per loop. */
Gesture breatheGesture (float seconds, float beats, float radiusSwing, int cycles);
/** steps: a sequence of radius offsets, each held for an equal share of the loop, joined by
    raised-cosine glides over `glide` (0..1) of a step. Multiples of 1/24 are semitones
    (with QUANTIZE the steps are exact). A tuning sequence played by the node. */
Gesture stepGesture (float seconds, float beats, const std::vector<float>& radiusSteps, float glide);
/** wander: a smooth, closed, seeded random loop (three harmonics per axis, peak-normalised
    to the swings). */
Gesture wanderGesture (float seconds, float beats, float angleSwing, float radiusSwing, uint32_t seed);

/** Wraps an angle to [-pi, pi). */
float wrapAngle (float a) noexcept;

/** Musical division in beats for SyncDivision index 0..5 (1/4, 1/2, 1, 2, 4, 8 bars). */
double divisionBeats (int division, double beatsPerBar) noexcept;

} // namespace arc
