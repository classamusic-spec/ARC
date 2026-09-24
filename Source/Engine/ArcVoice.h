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
    void reset() noexcept;

    void start (int note, int channel, float velocity, uint32_t seed, const VoiceControl& ctl) noexcept;
    /** Legato: change pitch without re-exciting (mono legato mode). */
    void glideTo (int note, float velocity) noexcept;
    void release() noexcept;
    void beginSteal() noexcept;

    /** Per-note expression (MPE or poly aftertouch). */
    void setPressure (float p) noexcept { pressure = p; }
    void setNoteBend (float semitones) noexcept { noteBend = semitones; }
    void setTimbre (float t) noexcept { timbre = t; }

    /** Renders n <= controlInterval samples, adding into outL/outR. */
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
    void trackIntonation (float coreSample) noexcept;

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
    int controlCounter = 0;
    uint64_t age = 0;

    float gain = 1.0f, gainStep = 0.0f;      // steal fade / declick
    float level = 0.0f, levelAcc = 0.0f;
    int levelCount = 0;
    float exciterEnergy = 0.0f, exciterAcc = 0.0f;
    float releaseT60Mult = 1.0f;
    float pluckDampMult = 1.0f;
    int silentBlocks = 0;
    float outputNorm = 1.0f;

    // Intonation tracker for sustained exciters (see trackIntonation).
    dsp::SelectivityStage intonationFilter;
    float intonationCents = 0.0f, intonationPrev = 0.0f;
    long intonationClock = 0;
    double intonationLastCrossing = -1.0, intonationPeriodAcc = 0.0;
    int intonationPeriods = 0;
};

} // namespace arc
