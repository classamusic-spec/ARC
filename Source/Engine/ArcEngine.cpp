#include "Engine/ArcEngine.h"

#include <algorithm>
#include <cmath>

namespace arc
{

void ArcEngine::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sr = sampleRate;
    const auto maxLength = static_cast<size_t> (sampleRate / 20.0) + 4;
    for (auto& v : voices)
    {
        v.line.assign (maxLength, 0.0f);
        v.active = false;
    }
}

void ArcEngine::reset()
{
    for (auto& v : voices)
    {
        std::fill (v.line.begin(), v.line.end(), 0.0f);
        v.active = false;
        v.note = -1;
    }
}

void ArcEngine::noteOn (int note, float velocity)
{
    auto& v = voices[static_cast<size_t> (nextVoice)];
    nextVoice = (nextVoice + 1) % kMaxVoices;

    const double freq = 440.0 * std::pow (2.0, (note - 69) / 12.0);
    v.length = std::clamp (static_cast<int> (sr / freq), 2, static_cast<int> (v.line.size()) - 1);
    v.index = 0;
    v.note = note;
    v.active = true;
    v.last = 0.0f;

    for (int i = 0; i < v.length; ++i)
    {
        noiseState = noiseState * 1664525u + 1013904223u;
        const float n = static_cast<float> (noiseState >> 8) * (1.0f / 8388608.0f) - 1.0f;
        v.line[static_cast<size_t> (i)] = n * velocity * 0.5f;
    }
}

void ArcEngine::noteOff (int note)
{
    for (auto& v : voices)
        if (v.active && v.note == note)
            v.damping = 0.97f;
}

void ArcEngine::render (float* left, float* right, int numSamples) noexcept
{
    for (auto& v : voices)
    {
        if (! v.active)
            continue;

        for (int n = 0; n < numSamples; ++n)
        {
            const auto i0 = static_cast<size_t> (v.index);
            const auto i1 = static_cast<size_t> ((v.index + 1) % v.length);
            const float out = v.line[i0];
            v.line[i0] = v.damping * 0.5f * (v.line[i0] + v.line[i1]);
            v.index = static_cast<int> (i1);

            left[n] += out * masterGain;
            right[n] += out * masterGain;
        }
    }
}

} // namespace arc
