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
    const bool hostRunning = t.valid && t.playing;

    // --- autonomous drift ---------------------------------------------------------------
    // Free: irrational rate ratios per node / axis, so the network drifts rather than
    // wobbles. SYNC: musical ratios of the chosen division, phase-locked to the host
    // timeline while it plays, so the movement lands on the beat.
    static constexpr double kFreeMul[8] = { 1.0, 0.7548, 1.3247, 0.5698, 1.1892, 0.8409, 0.6180, 1.4142 };
    static constexpr double kSyncMul[8] = { 1.0, 0.5, 2.0, 0.25, 1.0, 0.5, 0.25, 2.0 };
    const float depth = dsp::clamp (p.motionDepth, 0.0f, 1.0f);
    const double cycleBeats = divisionBeats (static_cast<int> (p.motionDivision), t.beatsPerBar);
    const double rate = p.sync ? beatsPerSecond / cycleBeats : static_cast<double> (p.motionRateHz);
    // Synced drift is purely periodic: the network repeats the same movement every
    // cycle of the division (a pattern over up to 4 cycles, from the ratios above).
    const float walkMix = p.sync ? 0.0f : 0.45f;
    std::array<float, 4> driftR {}, driftA {};
    for (size_t i = 0; i < 8; ++i)
    {
        if (p.sync && hostRunning)
        {
            const double x = t.ppqPosition / cycleBeats * kSyncMul[i] + 0.618034 * static_cast<double> (i + 1);
            lfo[i] = x - std::floor (x);
        }
        else
            lfo[i] = std::fmod (lfo[i] + dt * rate * (p.sync ? kSyncMul[i] : kFreeMul[i]), 1.0);
        const float sine = static_cast<float> (std::sin (6.283185307179586 * lfo[i]));
        const float walk = walks[i].next (dt, rate * kFreeMul[i] * 1.7);
        const float v = (1.0f - walkMix) * sine + walkMix * walk;
        if (i < 4)
            driftR[i] = v;
        else
            driftA[i - 4] = v;
    }
    shownPhase = static_cast<float> (lfo[0]);

    // --- gestures -------------------------------------------------------------------------
    // Offsets are smoothed (~40 ms) so transport relocation, SYNC toggles and gesture
    // replacement never step a node's tuning.
    const float smooth = static_cast<float> (1.0 - std::exp (-dt / 0.04));
    for (size_t n = 0; n < 4; ++n)
    {
        float gr = 0.0f, ga = 0.0f;
        const auto& g = gestures[n];
        if (g.valid && p.gesturePlay)
        {
            const bool synced = p.sync && g.durationBeats > 0.0f;
            if (synced && hostRunning)
            {
                // Locked to the host timeline.
                phase[n] = std::fmod (t.ppqPosition / static_cast<double> (g.durationBeats), 1.0);
                if (phase[n] < 0.0)
                    phase[n] += 1.0;
            }
            else
            {
                const double length = synced ? static_cast<double> (g.durationBeats) / beatsPerSecond : static_cast<double> (g.durationSeconds);
                phase[n] = std::fmod (phase[n] + dt / std::max (0.05, length), 1.0);
            }
            g.sample (phase[n], gr, ga);
            if (n == 0)
                shownPhase = static_cast<float> (phase[n]);
        }
        const float targetR = gr + depth * 0.14f * driftR[n];
        const float targetA = ga + depth * 0.45f * driftA[n];
        dRadius[n] += smooth * (targetR - dRadius[n]);
        // Angle offsets follow along the shorter arc (gesture angles wrap at +-pi).
        dAngle[n] = wrapAngle (dAngle[n] + smooth * wrapAngle (targetA - dAngle[n]));
    }
}

} // namespace arc
