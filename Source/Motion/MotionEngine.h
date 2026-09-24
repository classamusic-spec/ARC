#pragma once

// Makes the network itself move: slow autonomous drift of the nodes (MOTION) and
// playback of recorded node gestures, free-running or locked to host tempo (SYNC).
// Produces per-node offsets that are *added* to the node parameters, so automation
// and gestures never overwrite each other. Runs at control rate on the audio thread.

#include <array>

#include "Engine/EngineParams.h"
#include "Motion/Gesture.h"
#include "Motion/RandomWalk.h"

namespace arc
{

class MotionEngine
{
public:
    void reset (uint32_t seed) noexcept;

    void setGesture (int node, const Gesture& g) noexcept;
    void clearGesture (int node) noexcept;
    bool hasGesture (int node) const noexcept { return gestures[static_cast<size_t> (node)].valid; }
    const Gesture& getGesture (int node) const noexcept { return gestures[static_cast<size_t> (node)]; }

    /** Advance by dt seconds and compute offsets. */
    void update (const EngineParams& p, const TransportInfo& t, double dt) noexcept;

    float radiusOffset (int node) const noexcept { return dRadius[static_cast<size_t> (node)]; }
    float angleOffset (int node) const noexcept { return dAngle[static_cast<size_t> (node)]; }
    /** Normalised phase of node 0's gesture (or of the drift cycle) for display. */
    float displayPhase() const noexcept { return shownPhase; }
    float gesturePhase (int node) const noexcept { return static_cast<float> (phase[static_cast<size_t> (node)]); }

private:
    std::array<Gesture, 4> gestures {};
    std::array<double, 4> phase {};
    std::array<RandomWalk, 8> walks {};
    std::array<double, 8> lfo {};
    std::array<float, 4> dRadius {}, dAngle {};
    float shownPhase = 0.0f;
};

} // namespace arc
