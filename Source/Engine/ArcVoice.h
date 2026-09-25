#pragma once

// One ARC voice: an exciter injecting energy into its own resonant network.
//
// Lifecycle
//   idle -> active (note held) -> released (exciter stopped / body ringing) -> idle
//   any  -> stealing (short fade) -> restarted by the allocator
// Release semantics: STRIKE / PLUCK keep ringing after note-off (only the optional
// release damper shortens them); BOW / AIR stop driving and the body decays.

#include <array>
#include <cstdint>

#include "Engine/VoiceControl.h"
#include "Exciters/ExciterEngine.h"
#include "Synthesis/ResonantNetwork.h"

namespace arc
{

class ArcVoice
{
public:
    enum class State
    {
        idle,
        active,
        released,
        stealing
    };

    void prepare (double sampleRate, int controlInterval);
    /** Takes effect at the voice's next control boundary (QUALITY switch, glitch free). */
    void setControlInterval (int n) noexcept { pendingInterval = n; }
    void reset() noexcept;

    void start (int note, int channel, float velocity, uint32_t seed, const VoiceControl& ctl) noexcept;
    /** Re-excite the (possibly still ringing) network: a second strike on the same
        resonator adds to its motion instead of stacking another voice. */
    void restrike (float velocity, uint32_t seed, const VoiceControl& ctl) noexcept;
    /** Legato: glide to a new pitch (glideSeconds portamento); re-excite if asked. */
    void glideTo (int note, float velocity, float glideSeconds, bool reexcite, uint32_t seed, const VoiceControl& ctl) noexcept;
    void release() noexcept;
    void beginSteal() noexcept;

    /** FREEZE captures voices started before the engine's current freeze epoch. */
    void setFreezeEpoch (int epoch) noexcept { freezeEpochAtStart = epoch; }
    float getFreezeAmount() const noexcept { return voiceFreeze; }

    /** Per-note expression (MPE or poly aftertouch). */
    void setPressure (float p) noexcept { pressure = p; }
    void setNoteBend (float semitones) noexcept { noteBend = semitones; }
    void setTimbre (float t) noexcept { timbre = t; }

    /** Renders n samples (any size), adding into outL/outR. Control updates happen
        every controlInterval samples of the voice's own clock. */
    void render (float* outL, float* outR, int n, const VoiceControl& ctl) noexcept;

    State getState() const noexcept { return state; }
    bool isActive() const noexcept { return state != State::idle; }
    bool isStealing() const noexcept { return state == State::stealing; }
    bool isReleased() const noexcept { return state == State::released; }
    int getNote() const noexcept { return note; }
    int getChannel() const noexcept { return channel; }
    uint64_t getAge() const noexcept { return age; }
    void setAge (uint64_t a) noexcept { age = a; }

    /** Smoothed output level (mean square) — used for stealing and telemetry. */
    float getLevel() const noexcept { return level; }
    const std::array<float, dsp::kNumNodes>& getNodeEnergy() const noexcept { return nodeEnergy; }
    const std::array<float, dsp::kNumEdges>& getEdgeFlux() const noexcept { return edgeFlux; }
    float getExciterEnergy() const noexcept { return exciterEnergy; }
    float getCoreFrequency() const noexcept { return static_cast<float> (coreFrequency); }
    float getFundamental() const noexcept { return static_cast<float> (fundamental); }
    double getCoreDetuneEstimate() const noexcept { return freqShift[0]; }
    const std::array<double, dsp::kNumNodes>& getGainCorrections() const noexcept { return gainFactor; }
    const dsp::ResonantNetwork& getNetwork() const noexcept { return network; }

    bool sustainedByPedal = false; // note-off arrived while the pedal was down
    bool nonFiniteDetected = false;

private:
    void updateControl (const VoiceControl& ctl, bool immediate) noexcept;
    void renderSpan (float* outL, float* outR, int n) noexcept;
    void finishBlock (int n, const VoiceControl& ctl) noexcept;
    void trackIntonation (float coreSample) noexcept;
    void triggerExciter (const VoiceControl& ctl, uint32_t seed) noexcept;

    dsp::ResonantNetwork network;
    dsp::ExciterEngine exciter;
    dsp::NetworkSettings settings;
    std::array<float, dsp::kNumNodes> injectWeights {};
    std::array<float, dsp::kNumNodes> nodeEnergy {};
    std::array<float, dsp::kNumNodes> smoothEnergy {};
    std::array<float, dsp::kNumEdges> edgeFlux {};

    State state = State::idle;
    int note = 60, channel = 1;
    float velocity = 0.8f, pressure = 0.0f, noteBend = 0.0f, timbre = 0.0f;
    double fundamental = 261.63, coreFrequency = 261.63;
    std::array<double, dsp::kNumNodes> freqShift {};  // coupling-induced mode shifts (smoothed)
    std::array<double, dsp::kNumNodes> gainFactor {}; // coupling-induced per-pass gain (smoothed)
    std::array<double, dsp::kNumNodes> intendedFrequency {};
    double sampleRate = 48000.0;
    int controlInterval = 16;
    int pendingInterval = 0;
    int silentSamples = 0;
    int controlCounter = 0;
    uint64_t age = 0;

    float gain = 1.0f, gainStep = 0.0f;      // steal fade / declick
    // PATCH LEVEL of the patch this note was played in: followed (ramped per control block)
    // while that patch is current, held after a preset change.
    float patchGain = 1.0f, patchGainStep = 0.0f;
    uint32_t patchEpochAtStart = 0;
    void takePatchTrim (const VoiceControl& ctl) noexcept
    {
        patchEpochAtStart = ctl.patchEpoch;
        patchGain = ctl.patchGain;
        patchGainStep = 0.0f;
    }
    float level = 0.0f, levelAcc = 0.0f;
    int levelCount = 0;
    float exciterEnergy = 0.0f, exciterAcc = 0.0f;
    float releaseT60Mult = 1.0f;
    float pluckDampMult = 1.0f;
    int silentBlocks = 0;
    float outputNorm = 1.0f;
    int samplesToControl = 0;

    // FREEZE (per voice) and its energy governor.
    int freezeEpochAtStart = -1;
    float voiceFreeze = 0.0f;
    bool governorCaptured = false;
    int governorSettle = 0;
    float governorReference = 0.0f;
    float governorScale = 1.0f;

    // CHAOS per-note static jitter (fixed for the note, scaled by CHAOS).
    std::array<float, 4> unitJitter {};
    float couplingSaturation = 1.0f;
    float pitchNote = 60.0f;      // gliding note (semitones)
    float glideCoeff = 0.0f;      // per control update
    float blockLevelAcc = 0.0f, blockExciterAcc = 0.0f;
    int blockSamples = 0;

    // Intonation tracker for sustained exciters (see trackIntonation).
    dsp::SelectivityStage intonationFilter;
    float intonationCents = 0.0f, intonationPrev = 0.0f;
    long intonationClock = 0;
    double intonationLastCrossing = -1.0, intonationPeriodAcc = 0.0;
    int intonationPeriods = 0;
};

} // namespace arc
