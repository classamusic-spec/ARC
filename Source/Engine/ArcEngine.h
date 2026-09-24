#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace arc
{

// Phase 1 bootstrap engine: a minimal polyphonic Karplus-Strong synth used only
// to prove the MIDI -> voice -> audio path through the plugin wrapper.
// It is replaced by the resonant network engine in Phase 2+.
class ArcEngine
{
public:
    static constexpr int kMaxVoices = 8;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    void noteOn (int note, float velocity);
    void noteOff (int note);

    void setMasterGain (float linearGain) noexcept { masterGain = linearGain; }

    // Adds into the buffers.
    void render (float* left, float* right, int numSamples) noexcept;

private:
    struct Voice
    {
        std::vector<float> line;
        int length = 0;
        int index = 0;
        int note = -1;
        bool active = false;
        float damping = 0.996f;
        float last = 0.0f;
    };

    std::array<Voice, kMaxVoices> voices;
    double sr = 48000.0;
    float masterGain = 1.0f;
    uint32_t noiseState = 0x12345678u;
    int nextVoice = 0;
};

} // namespace arc
