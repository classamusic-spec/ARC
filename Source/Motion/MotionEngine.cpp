#include "Motion/MotionEngine.h"

#include <cmath>

namespace arc
{

void MotionEngine::reset (uint32_t seed) noexcept
{
    for (size_t i = 0; i < walks.size(); ++i)
    {
        walks[i].seed (seed * 2654435761u + static_cast<uint32_t> (i) * 40503u + 1u);
        // Incommensurate starting phases so nodes never move in lock-step.
        lfo[i] = std::fmod (0.618034 * static_cast<double> (i + 1), 1.0);
    }
    phase.fill (0.0);
    dRadius.fill (0.0f);
    dAngle.fill (0.0f);
}

void MotionEngine::setGesture (int node, const Gesture& g) noexcept
{
    if (node < 0 || node > 3)
        return;
    gestures[static_cast<size_t> (node)] = g;
    phase[static_cast<size_t> (node)] = 0.0;
}

void MotionEngine::clearGesture (int node) noexcept
{
    if (node < 0 || node > 3)
        return;
    gestures[static_cast<size_t> (node)].valid = false;
}

void MotionEngine::update (const EngineParams& p, const TransportInfo& t, double dt) noexcept
{
    const double bpm = t.valid && t.bpm > 1.0 ? t.bpm : 120.0;
    const double beatsPerSecond = bpm / 60.0;

    // --- autonomous drift ---------------------------------------------------------------
    double rate = p.motionRateHz;
    if (p.sync)
        rate = beatsPerSecond / divisionBeats (static_cast<int> (p.motionDivision), t.beatsPerBar);
    const float depth = dsp::clamp (p.motionDepth, 0.0f, 1.0f);
    // Irrational rate ratios per node / axis: the network drifts, it does not wobble.
    static constexpr double kRateMul[8] = { 1.0, 0.7548, 1.3247, 0.5698, 1.1892, 0.8409, 0.6180, 1.4142 };
    std::array<float, 4> driftR {}, driftA {};
    for (size_t i = 0; i < 8; ++i)
    {
        lfo[i] = std::fmod (lfo[i] + dt * rate * kRateMul[i], 1.0);
        const float sine = static_cast<float> (std::sin (6.283185307179586 * lfo[i]));
        const float walk = walks[i].next (dt, rate * kRateMul[i] * 1.7);
        const float v = 0.55f * sine + 0.45f * walk;
        if (i < 4)
            driftR[i] = v;
        else
            driftA[i - 4] = v;
    }
    shownPhase = static_cast<float> (lfo[0]);

    // --- gestures -------------------------------------------------------------------------
    for (size_t n = 0; n < 4; ++n)
    {
        float gr = 0.0f, ga = 0.0f;
        const auto& g = gestures[n];
        if (g.valid && p.gesturePlay)
        {
            const bool synced = p.sync && g.durationBeats > 0.0f;
            if (synced && t.valid && t.playing)
            {
                // Locked to the host timeline.
                phase[n] = std::fmod (t.ppqPosition / g.durationBeats, 1.0);
                if (phase[n] < 0.0)
                    phase[n] += 1.0;
            }
            else
            {
                const double length = synced ? g.durationBeats / beatsPerSecond : g.durationSeconds;
                phase[n] = std::fmod (phase[n] + dt / std::max (0.05, length), 1.0);
            }
            g.sample (phase[n], gr, ga);
            if (n == 0)
                shownPhase = static_cast<float> (phase[n]);
        }
        dRadius[n] = gr + depth * 0.14f * driftR[n];
        dAngle[n] = ga + depth * 0.45f * driftA[n];
    }
}

} // namespace arc
