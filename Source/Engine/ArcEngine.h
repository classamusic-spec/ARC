#pragma once

// The ARC engine: voices, MIDI / MPE, global control state, output stage, telemetry.
// Pure C++20. Everything on the audio thread is allocation-free and lock-free after
// prepare(); see docs/VOICE_ARCHITECTURE.md.

#include <array>
#include <cstdint>

#include "Engine/ArcVoice.h"
#include "Engine/EngineParams.h"
#include "Engine/SpscQueue.h"
#include "Engine/Telemetry.h"
#include "FX/OutputStage.h"
#include "Materials/MaterialEngine.h"
#include "Motion/MotionEngine.h"
#include "Nonlinear/ChaosEngine.h"

namespace arc
{

class ArcEngine
{
public:
    static constexpr int kMaxVoices = 20;     // physical voices (steal fades run in spare slots)
    static constexpr int kMaxPolyphony = 16;  // user polyphony limit
    static constexpr int kMaxHeldNotes = 32;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    /** Parameter snapshot, once per block (cheap copy). */
    void setParameters (const EngineParams& p) noexcept;
    void setTransport (const TransportInfo& t) noexcept { transport = t; }

    // --- MIDI (call between render() calls for sample accuracy) ------------------------
    void noteOn (int channel, int note, float velocity) noexcept;
    void noteOff (int channel, int note) noexcept;
    void sustainPedal (int channel, bool down) noexcept;
    void pitchBend (int channel, float normalised) noexcept; // -1..1
    void channelPressure (int channel, float value) noexcept; // 0..1
    void polyPressure (int channel, int note, float value) noexcept;
    void controller (int channel, int number, float value) noexcept; // value 0..1
    void allNotesOff (bool killSound) noexcept;

    /** Writes (does not add) n samples of stereo output. */
    void render (float* left, float* right, int n) noexcept;

    // --- gestures / seeds (message thread -> audio thread, lock-free) ------------------
    /** Queue a gesture for node 0..3 (invalid gesture = clear). Safe from any one thread. */
    bool postGesture (int node, const Gesture& g) noexcept;
    /** Deterministic seed for chaos and motion (preset recall). */
    void postSeed (uint32_t seed) noexcept;
    const MotionEngine& getMotion() const noexcept { return motion; }

    Telemetry& getTelemetry() noexcept { return telemetry; }
    const EngineParams& getParameters() const noexcept { return params; }
    int activeVoiceCount() const noexcept;
    double getSampleRate() const noexcept { return sampleRate; }
    int getControlInterval() const noexcept { return controlInterval; }
    const VoiceControl& getVoiceControl() const noexcept { return control; }
    const ArcVoice& getVoice (int i) const noexcept { return voices[static_cast<size_t> (i)]; }

    /** Statistics for tests / profiling. */
    uint64_t stealCount = 0, hardStealCount = 0, nonFiniteVoiceResets = 0;

private:
    void updateGlobalControl() noexcept;
    void renderChunk (float* left, float* right, int n) noexcept;
    void publishTelemetry (int n, double seconds) noexcept;
    ArcVoice* findVoiceForNewNote() noexcept;
    void startVoice (ArcVoice& v, int channel, int note, float velocity) noexcept;
    uint32_t nextSeed() noexcept;
    bool isMpeMemberChannel (int channel) const noexcept { return params.mpe && channel >= 2 && channel <= 16; }
    void pushHeld (int channel, int note, float velocity) noexcept;
    void removeHeld (int channel, int note) noexcept;

    struct GestureMessage
    {
        int node = 0;
        Gesture gesture;
    };
    SpscQueue<GestureMessage, 8> gestureQueue;
    std::atomic<uint32_t> pendingSeed { 0 };
    std::atomic<bool> seedPending { false };
    uint32_t currentSeed = 0xA2C1u;

    MotionEngine motion;
    ChaosEngine chaos;
    int freezeEpoch = 0;
    bool freezeWasOn = false;
    std::array<NodeParams, 4> effectiveNodes {};
    std::array<float, 4> effectiveAngle {};

    std::array<ArcVoice, kMaxVoices> voices;
    MaterialEngine material;
    dsp::OutputStage output;
    Telemetry telemetry;
    EngineParams params;
    TransportInfo transport;
    VoiceControl control;

    struct Held
    {
        int channel, note;
        float velocity;
    };
    std::array<Held, kMaxHeldNotes> held {};
    int numHeld = 0;

    std::array<float, 17> channelBend {};     // per MIDI channel, semitones
    std::array<float, 17> channelPressureValue {};
    std::array<float, 17> channelTimbre {};
    std::array<bool, 17> sustainDown {};
    float globalBendTarget = 0.0f;
    dsp::Smoothed bendSmooth, freezeSmooth;

    double sampleRate = 48000.0;
    int controlInterval = 16;
    int controlCountdown = 0;
    int maxBlock = 512;
    uint64_t voiceClock = 0;
    uint32_t seedState = 0x2545F491u;
    bool prepared = false;
};

} // namespace arc
